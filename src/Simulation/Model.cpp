#include "Model.h"

#include <Core/Scheduler/Scheduler.h>
#include <Population/Population.h>
#include <Utils/Random.h>

#include <memory>
#include <stdexcept>

#include "Agents/AdaptiveCyclingAgent.h"
#include "Configuration/Config.h"
#include "MDC/ModelDataCollector.h"
#include "Mosquito/Mosquito.h"
#include "Reporters/Reporter.h"
#include "Treatment/LinearTCM.h"
#include "Treatment/SteadyTCM.h"
#include "Utils/Cli.h"

bool Model::initialize() {
  config_ = std::make_unique<Config>();
  random_ = std::make_unique<utils::Random>(nullptr, -1);
  scheduler_ = std::make_unique<Scheduler>();
  population_ = std::make_unique<Population>();
  mdc_ = std::make_unique<ModelDataCollector>();
  mosquito_ = std::make_unique<Mosquito>();

  progress_to_clinical_update_function_ = std::make_unique<ClinicalUpdateFunction>(this);
  immunity_clearance_update_function_ = std::make_unique<ImmunityClearanceUpdateFunction>(this);
  having_drug_update_function_ = std::make_unique<ImmunityClearanceUpdateFunction>(this);
  clinical_update_function_ = std::make_unique<ImmunityClearanceUpdateFunction>(this);
  reporters_.clear();

  genotype_db_ = std::make_unique<GenotypeDatabase>();
  drug_db_ = std::make_unique<DrugDatabase>();

  // AdaptiveCyclingAgent will be created after configuration is loaded and
  // only if enabled in the config (agent_parameters.adc_agent.enabled)

  if (utils::Cli::get_instance().get_input_path().empty()) {
    // spdlog::error("Input path is empty. Please provide a valid input path.");
    // return false;
    spdlog::warn("Input path is empty. Model initialized without configuration file.");
    return true;
  }

  // if input path is not empty, load configuration file
  spdlog::info("Loading configuration file: " + utils::Cli::get_instance().get_input_path());
  if (config_->load(utils::Cli::get_instance().get_input_path())) {
    if (config_->get_model_settings().get_initial_seed_number() <= 0) {
      random_->set_seed(std::chrono::system_clock::now().time_since_epoch().count());
    } else {
      random_->set_seed(config_->get_model_settings().get_initial_seed_number());
    }

    if (utils::Cli::get_instance().get_output_path().empty()) {
      utils::Cli::get_instance().set_output_path("./");
    }

    // Create and initialize ADC agent only if enabled in configuration
    if (config_->get_agent_parameters().get_adc_agent().is_enabled()) {
      adaptive_cycling_agent_ = std::make_unique<AdaptiveCyclingAgent>();
      adaptive_cycling_agent_->initialize();
      spdlog::info("Adaptive Cycling Agent enabled and initialized.");
    } else {
      spdlog::info("Adaptive Cycling Agent is disabled in configuration.");
    }

    spdlog::info("Model initialized with seed: " + std::to_string(random_->get_seed()));
    // add reporter here
    const auto &cli_reporters = utils::Cli::get_instance().get_reporters();
    if (cli_reporters.empty()) {
      // default
      add_reporter(Reporter::MakeReport(Reporter::SQLITE_MONTHLY_REPORTER));
    } else {
      for (const auto &r : cli_reporters) {
        auto it = Reporter::ReportTypeMap.find(r);
        if (it != Reporter::ReportTypeMap.end()) {
          add_reporter(Reporter::MakeReport(it->second));
        } else {
          spdlog::warn("Unknown reporter specified on CLI: '{}', skipping.", r);
        }
      }
    }

#ifdef ENABLE_TRAVEL_TRACKING
    add_reporter(Reporter::MakeReport(Reporter::TRAVEL_TRACKING_REPORTER));
#endif

    // initialize reporters
    for (auto &reporter : reporters_) {
      reporter->initialize(utils::Cli::get_instance().get_job_number(),
                           utils::Cli::get_instance().get_output_path());
    }
    spdlog::info("Model initialized reporters.");

    scheduler_->initialize(config_->get_simulation_timeframe().get_starting_date(),
                           config_->get_simulation_timeframe().get_ending_date());
    spdlog::info("Model initialized scheduler.");

    set_treatment_strategy(config_->get_strategy_parameters().get_initial_strategy_id());
    spdlog::info("Model initialized treatment strategy.");

    build_initial_treatment_coverage();
    spdlog::info("Model initialized treatment coverage model.");

    mdc_->initialize();
    spdlog::info("Model initialized data collector.");

    spdlog::info("Model initializing population...");
    population_->initialize();
    spdlog::info("Model initialized population.");

    config_->get_movement_settings().get_spatial_model()->prepare();
    spdlog::info("Model initialized movement model.");

    mosquito_->initialize(config_.get());
    spdlog::info("Model initialized mosquito.");

    population_->introduce_initial_cases();
    spdlog::info("Model initialized initial cases.");

    // Take ownership of the events from the config
    auto population_events = config_->get_population_events().release_events();
    for (auto &event : population_events) {
      if (event) {  // Check if the pointer is valid before using
        spdlog::info("Scheduling population event: {} at {}", event->name(), event->get_time());
        scheduler_->schedule_population_event(std::move(event));
      } else {
        spdlog::warn("Encountered a null event pointer during initialization.");
      }
    }

    if (utils::Cli::get_instance().get_record_movement()) {
      // Generate a movement reporter
      auto reporter = Reporter::MakeReport(Reporter::ReportType::MOVEMENT_REPORTER);
      reporter->initialize(utils::Cli::get_instance().get_job_number(),
                           utils::Cli::get_instance().get_output_path());
      add_reporter(std::move(reporter));
    }

    is_initialized_ = true;
  } else {
    spdlog::error("Failed to load configuration file: "
                  + utils::Cli::get_instance().get_input_path());
  }
  return is_initialized_;
}

void Model::release() {
  // Clean up the memory used by the model

  treatment_strategy_ = nullptr;
  treatment_coverage_.reset();

  progress_to_clinical_update_function_.reset();
  immunity_clearance_update_function_.reset();
  having_drug_update_function_.reset();
  clinical_update_function_.reset();

  drug_db_.reset();
  genotype_db_.reset();
  mosquito_.reset();
  mdc_.reset();
  population_.reset();
  random_.reset();
  scheduler_.reset();
  config_.reset();

  // simply clear the vector, the unique_ptr will be deleted automatically
  reporters_.clear();
}

void Model::run() {
  if (!is_initialized_) {
    throw std::runtime_error("Model is not initialized. Call Initialize() first.");
  }
  before_run();
  scheduler_->run();
  after_run();
}

void Model::before_run() {
  spdlog::info("Perform before run events");
  for (auto &reporter : reporters_) { reporter->before_run(); }
}

void Model::after_run() {
  spdlog::info("Perform after run events");

  mdc_->update_after_run();

  for (auto &reporter : reporters_) { reporter->after_run(); }
}

void Model::begin_time_step() {
  // reset daily variables
  mdc_->begin_time_step();
  report_begin_of_time_step();
}

void Model::end_time_step() {
  // update / calculate daily UTL
  mdc_->end_of_time_step();

  // check to switch strategy
  treatment_strategy_->update_end_of_time_step();
}

void Model::daily_update() {
  population_->update_all_individuals();
  // for safety remove all dead by calling perform_death_event
  population_->perform_death_event();
  population_->perform_birth_event();

  // update current foi should be call after perform death, birth event
  // in order to obtain the right all alive individuals,
  // infection event will use pre-calculated individual relative biting rate to
  // infect new infections circulation event will use pre-calculated individual
  // relative moving rate to migrate individual to new location
  population_->update_current_foi();

  population_->perform_infection_event();
  population_->perform_circulation_event();

  // infect new mosquito cohort in prmc must be run after population perform
  // infection event and update current foi because the prmc at the tracking
  // index will be overridden with new cohort to use N days later and infection
  // event used the prmc at the tracking index for the today infection
  auto tracking_index = scheduler_->current_time() % config_->number_of_tracking_days();
  mosquito_->infect_new_cohort_in_PRMC(config_.get(), random_.get(), population_.get(),
                                       tracking_index);

  // this function must be called after mosquito infect new cohort in prmc
  population_->persist_current_force_of_infection_to_use_n_days_later();
}

void Model::monthly_update() {
  monthly_report();

  // reset monthly variables
  mdc_->monthly_update();

  //
  treatment_strategy_->monthly_update();

  // update treatment coverage
  treatment_coverage_->monthly_update();
}

void Model::yearly_update() { mdc_->yearly_update(); }

void Model::monthly_report() {
  mdc_->perform_population_statistic();

  for (auto &reporter : reporters_) { reporter->monthly_report(); }
}

void Model::report_begin_of_time_step() {
  for (auto &reporter : reporters_) { reporter->begin_time_step(); }
}

void Model::add_reporter(std::unique_ptr<Reporter> reporter) {
  reporter->set_model(this);
  reporters_.push_back(std::move(reporter));
}

IStrategy* Model::get_treatment_strategy() { return get_instance()->treatment_strategy_; }

void Model::set_treatment_strategy(const int &strategy_id) {
  treatment_strategy_ = strategy_id == -1 ? nullptr : Model::get_strategy_db()[strategy_id].get();
  assert(treatment_strategy_ != nullptr);
  treatment_strategy_->adjust_started_time_point(Model::get_scheduler()->current_time());
}

ITreatmentCoverageModel* Model::get_treatment_coverage() {
  return get_instance()->treatment_coverage_.get();
}

void Model::set_treatment_coverage(std::unique_ptr<ITreatmentCoverageModel> tcm) {
  if (treatment_coverage_.get() != tcm.get()) {
    if (tcm->p_treatment_under_5.empty() || tcm->p_treatment_over_5.empty()) {
      // copy current value
      tcm->p_treatment_under_5 = treatment_coverage_->p_treatment_under_5;
      tcm->p_treatment_over_5 = treatment_coverage_->p_treatment_over_5;
    }

    if (auto* linear_tcm = dynamic_cast<LinearTCM*>(tcm.get())) {
      linear_tcm->update_rate_of_change();
    }
  }
  treatment_coverage_ = std::move(tcm);
}

void Model::build_initial_treatment_coverage() {
  auto tcm_ptr = std::make_unique<SteadyTCM>();
  for (const auto &location : config_->location_db()) {
    tcm_ptr->p_treatment_under_5.push_back(location.p_treatment_under_5);
    tcm_ptr->p_treatment_over_5.push_back(location.p_treatment_over_5);
  }
  set_treatment_coverage(std::move(tcm_ptr));
}

std::vector<std::unique_ptr<Reporter>>& Model::get_reporters() {
  return reporters_;
}

