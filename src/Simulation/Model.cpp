#include "Model.h"

#include <Core/Scheduler/Scheduler.h>
#include <Population/Population.h>
#include <Utils/Random.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>

#include "Configuration/Config.h"
#include "MDC/ModelDataCollector.h"
#include "Mosquito/Mosquito.h"
#include "Reporters/Reporter.h"
#include "Treatment/LinearTCM.h"
#include "Utils/Helpers/StringHelpers.h"
#include "Treatment/SteadyTCM.h"

namespace {

namespace isop = ImmuneSystemOverridePaths;

// Strips leading/trailing whitespace so `-r A, B` behaves like `-r A,B`.
std::string trim_copy(const std::string &text) {
  const auto begin = text.find_first_not_of(" \t\r\n");
  if (begin == std::string::npos) { return {}; }
  const auto end = text.find_last_not_of(" \t\r\n");
  return text.substr(begin, end - begin + 1);
}

double get_effective_override_value(const Config &config, const std::string &path) {
  if (path == isop::K_Z) {
    return config.get_immune_system_parameters().get_immune_effect_on_progression_to_clinical();
  }
  if (path == isop::K_KAPPA) {
    return config.get_immune_system_parameters().get_factor_effect_age_mature_immunity();
  }
  if (path == isop::K_MIDPOINT) { return config.get_immune_system_parameters().get_midpoint(); }
  if (path == isop::K_P_CI_SYMP) {
    return config.get_epidemiological_parameters()
        .get_allow_new_coinfection_to_cause_symptoms()
        .get_probability();
  }
  if (path == isop::K_P_SEEK_BASE) {
    return config.get_epidemiological_parameters()
        .get_age_based_probability_of_seeking_treatment()
        .get_power()
        .base;
  }
  if (path == isop::K_MUTATION_PROB) {
    return config.get_genotype_parameters().get_mutation_probability_per_locus();
  }
  if (path == isop::K_DEFAULT_CNV_REVERSION_MULTIPLIER) {
    return config.get_genotype_parameters().get_default_cnv_reversion_multiplier();
  }
  return std::numeric_limits<double>::quiet_NaN();
}

bool is_keep_default_sentinel(const std::string &path, double override_value) {
  const bool is_sentinel_path =
      path == isop::K_MUTATION_PROB || path == isop::K_DEFAULT_CNV_REVERSION_MULTIPLIER;
  return is_sentinel_path && override_value < 0.0;
}

void verify_override(const Config &config, const std::string &path, double override_value) {
  const double effective = get_effective_override_value(config, path);

  if (std::isnan(effective)) {
    spdlog::warn("    [UNKNOWN PATH] {} = {} (no live config field to verify)", path,
                 override_value);
    return;
  }

  // A negative mutation/reversion value means "keep the existing default".
  if (is_keep_default_sentinel(path, override_value)) {
    spdlog::info("    [KEPT DEFAULT] {}: override={} (<0 sentinel), effective={}", path,
                 override_value, effective);
    return;
  }

  const bool applied =
      std::fabs(effective - override_value) <= (1e-9 * std::max(1.0, std::fabs(override_value)));
  spdlog::info("    [{}] {}: override={}, effective={}", applied ? "OK" : "MISMATCH", path,
               override_value, effective);
  if (!applied) { spdlog::warn("    ^ override for '{}' was NOT applied correctly!", path); }
}

void verify_immune_system_overrides(const Config* config) {
  if (config == nullptr) { return; }

  const bool section_present = config->has_version6_pfpr_incidence_calibrations();
  const auto &overrides = config->get_version6_pfpr_incidence_calibrations();

  spdlog::info("===== version6_pfpr_incidence_calibrations (before_run verification) =====");
  spdlog::info("  section_present    = {}", section_present);
  spdlog::info("  random_selection   = {}", overrides.get_random_selection());
  spdlog::info("  chosen_calibration_id = {}", overrides.get_chosen_calibration_id());

  if (!section_present) {
    spdlog::info("  No overrides section -> running with default immune-system parameters.");
  } else if (!overrides.has_selected_calibration_id()) {
    spdlog::warn(
        "  chosen_calibration_id={} not found among calibration_ids -> NO overrides applied!",
        overrides.get_chosen_calibration_id());
  } else {
    const auto &calibration_id = overrides.get_selected_calibration_id();
    spdlog::info("  selected calibration_id[{}] has {} override(s):",
                 overrides.get_chosen_calibration_id(), calibration_id.overrides.size());
    for (const auto &[path, override_value] : calibration_id.overrides) {
      verify_override(*config, path, override_value);
    }
  }

  spdlog::info("======================================================================");
}
}  // namespace

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

  if (cli_input_.input_path.empty()) {
    // spdlog::error("Input path is empty. Please provide a valid input path.");
    // return false;
    spdlog::warn("Input path is empty. Model initialized without configuration file.");
    return true;
  }

  // if input path is not empty, load configuration file
  spdlog::info("Loading configuration file: " + cli_input_.input_path);
  if (config_->load(cli_input_.input_path)) {
    if (config_->get_model_settings().get_initial_seed_number() <= 0) {
      random_->set_seed(std::chrono::system_clock::now().time_since_epoch().count());
    } else {
      random_->set_seed(config_->get_model_settings().get_initial_seed_number());
    }

    spdlog::info("Model initialized with seed: " + std::to_string(random_->get_seed()));

    if (cli_input_.output_path.empty()) { cli_input_.output_path = "./"; }

    // add reporter here
    if (!setup_reporters()) { return false; }

#ifdef ENABLE_TRAVEL_TRACKING
    if (auto travel_reporter =
            Reporter::make_report(Reporter::ReportType::TRAVEL_TRACKING_REPORTER)) {
      add_reporter(std::move(travel_reporter));
    }
#endif

    // initialize reporters
    for (auto &reporter : reporters_) {
      reporter->initialize(cli_input_.job_number, cli_input_.output_path);
    }
    spdlog::info("Model initialized reporters.");

    scheduler_->initialize(config_->get_simulation_timeframe().get_starting_date(),
                           config_->get_simulation_timeframe().get_ending_date());
    spdlog::info("Model initialized scheduler.");

    set_treatment_strategy(config_->get_strategy_parameters().get_initial_strategy_id());
    spdlog::info("Model initialized treatment strategy.");

    set_second_line_strategy(config_->get_strategy_parameters().get_second_line_strategy_id());
    spdlog::info("Model initialized second-line treatment strategy.");

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

    if (cli_input_.record_movement) {
      // Generate a movement reporter
      auto reporter = Reporter::make_report(Reporter::ReportType::MOVEMENT_REPORTER);
      if (reporter) {
        reporter->initialize(cli_input_.job_number, cli_input_.output_path);
        add_reporter(std::move(reporter));
      } else {
        spdlog::error(
            "Movement recording was requested (--im/--mc/--md) but no movement reporter is "
            "available in this build; movement data will NOT be written.");
        return false;
      }
    }
    is_initialized_ = true;
  } else {
    spdlog::error("Failed to load configuration file: " + cli_input_.input_path);
  }
  return is_initialized_;
}

void Model::release() {
  // Clean up the memory used by the model

  // A released singleton must be safe to initialize again. Reset this guard
  // before destroying the owned subsystems so run() cannot dereference stale
  // state after release().
  is_initialized_ = false;

  treatment_strategy_ = nullptr;
  second_line_strategy_ = nullptr;
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
  verify_immune_system_overrides(config_.get());
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
  if (second_line_strategy_ != nullptr && second_line_strategy_ != treatment_strategy_) {
    second_line_strategy_->update_end_of_time_step();
  }

  report_after_time_step();
}

void Model::daily_update() {
  const auto tracking_index = scheduler_->current_time() % config_->number_of_tracking_days();
  const auto circulation_context = Population::prepare_circulation_context();

  for (int location = 0; location < config_->number_of_locations(); ++location) {
    population_->prepare_daily_state_at_location(location);
    population_->perform_infection_event_at_location(location, tracking_index);
    population_->perform_circulation_from_location(location, circulation_context);
    mosquito_->infect_new_cohort_at_location(config_.get(), random_.get(), population_.get(),
                                             location, tracking_index);
    population_->persist_force_of_infection_at_location(location, tracking_index);
  }
}

void Model::monthly_update() {
  monthly_report();

  // reset monthly variables
  mdc_->monthly_update();

  //
  treatment_strategy_->monthly_update();
  if (second_line_strategy_ != nullptr && second_line_strategy_ != treatment_strategy_) {
    second_line_strategy_->monthly_update();
  }

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

void Model::report_after_time_step() {
  for (auto &reporter : reporters_) { reporter->after_time_step(); }
}

void Model::add_reporter(std::unique_ptr<Reporter> reporter) {
  if (reporter == nullptr) {
    spdlog::error("Attempted to register a null reporter; ignoring.");
    return;
  }
  reporter->set_model(this);
  reporters_.push_back(std::move(reporter));
}

bool Model::setup_reporters() {
  // No -r given: keep the historical default.
  if (cli_input_.reporter.empty()) {
    add_reporter(Reporter::make_report(Reporter::ReportType::SQLITE_MONTHLY_REPORTER));
    return true;
  }

  // -r accepts a comma separated list, e.g.
  //   -r SQLiteMonthlyReporter,SQLiteValidationReporter
  const auto tokens = StringHelpers::split<char>(cli_input_.reporter, ',');

  std::set<Reporter::ReportType> already_added;
  std::size_t added = 0;

  for (const auto &token : tokens) {
    const auto name = trim_copy(token);
    if (name.empty()) { continue; }

    const auto found = Reporter::report_type_map.find(name);
    if (found == Reporter::report_type_map.end()) {
      spdlog::error("Unknown reporter '{}'. Run with -l to list the valid reporter names.", name);
      continue;
    }

    if (!already_added.insert(found->second).second) {
      spdlog::warn("Reporter '{}' listed more than once; ignoring the duplicate.", name);
      continue;
    }

    auto reporter = Reporter::make_report(found->second);
    if (reporter == nullptr) {
      spdlog::error("Reporter '{}' could not be created.", name);
      continue;
    }

    add_reporter(std::move(reporter));
    added++;
    spdlog::info("Added reporter: {}", name);
  }

  if (added == 0) {
    spdlog::error("No valid reporter could be resolved from -r '{}'. Aborting.",
                  cli_input_.reporter);
    return false;
  }

  return true;
}

IStrategy* Model::get_treatment_strategy() { return get_instance()->treatment_strategy_; }

void Model::set_treatment_strategy(const int &strategy_id) {
  treatment_strategy_ = strategy_id == -1 ? nullptr : Model::get_strategy_db()[strategy_id].get();
  assert(treatment_strategy_ != nullptr);
  treatment_strategy_->adjust_started_time_point(Model::get_scheduler()->current_time());
}

IStrategy* Model::get_second_line_strategy() { return get_instance()->second_line_strategy_; }

void Model::set_second_line_strategy(const int strategy_id) {
  second_line_strategy_ = strategy_id == -1 ? nullptr : Model::get_strategy_db()[strategy_id].get();
  if (second_line_strategy_ != nullptr && second_line_strategy_ != treatment_strategy_) {
    second_line_strategy_->adjust_started_time_point(Model::get_scheduler()->current_time());
  }
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

std::vector<std::unique_ptr<Reporter>> &Model::get_reporters() { return reporters_; }
