#include "Config.h"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <filesystem>

#include "Simulation/Model.h"
#include "Utils/Cli.h"

int inline get_pipe_count(const std::string &str) {
  int pipe_count = 0;
  for (const char ch : str) {
    if (ch == '|') { pipe_count++; }
  }
  return pipe_count;
}

bool Config::load(const std::string &filename) {
  config_file_path_ = filename;
  try {
    YAML::Node config = YAML::LoadFile(filename);

    spdlog::info("Configuration file loaded successfully: "
                 + utils::Cli::get_instance().get_input_path());

    model_settings_ = config["model_settings"].as<ModelSettings>();
    simulation_timeframe_ = config["simulation_timeframe"].as<SimulationTimeframe>();

    transmission_settings_ = config["transmission_settings"].as<TransmissionSettings>();

    population_demographic_ = config["population_demographic"].as<PopulationDemographic>();

    spatial_settings_ = config["spatial_settings"].as<SpatialSettings>();

    seasonality_settings_ = config["seasonality_settings"].as<SeasonalitySettings>();

    movement_settings_ = config["movement_settings"].as<MovementSettings>();

    parasite_parameters_ = config["parasite_parameters"].as<ParasiteParameters>();

    immune_system_parameters_ = config["immune_system_parameters"].as<ImmuneSystemParameters>();

    genotype_parameters_ = config["genotype_parameters"].as<GenotypeParameters>();

    drug_parameters_ = config["drug_parameters"].as<DrugParameters>();

    therapy_parameters_ = config["therapy_parameters"].as<TherapyParameters>();

    strategy_parameters_ = config["strategy_parameters"].as<StrategyParameters>();

    epidemiological_parameters_ =
        config["epidemiological_parameters"].as<EpidemiologicalParameters>();

    mosquito_parameters_ = config["mosquito_parameters"].as<MosquitoParameters>();

    if (config["agent_parameters"]) {
      agent_parameters_ = config["agent_parameters"].as<AgentParameters>();
    }

    if (config["rapt_settings"]) {
      rapt_settings_ = config["rapt_settings"].as<RaptSettings>();
    }

    spdlog::info("Configuration file parsed successfully");

    // Validate all cross field validations
    validate_all_cross_field_validations();

    spdlog::info("Configuration file validated successfully");

    // test config
    spdlog::info("Genotype info:");
    for (const auto &parasites : get_genotype_parameters().get_initial_parasite_info_raw()) {
      for (const auto &parasite : parasites.get_parasite_info()) {
        spdlog::info("{} {}", parasite.get_aa_sequence(), parasite.get_prevalence());
      }
    }

    spdlog::debug("Pf genotype info: {}", get_genotype_parameters()
                                              .get_pf_genotype_info()
                                              .chromosome_infos.back()
                                              .get_genes()
                                              .back()
                                              .get_cnv_multiplicative_effect_on_EC50()
                                              .back()
                                              .get_drug_id());
    for (const auto &chromosome :
         get_genotype_parameters().get_pf_genotype_info().chromosome_infos) {
      if (chromosome.get_chromosome_id() != -1) {
        spdlog::debug("chromosome:{}", chromosome.get_chromosome_id());
        for (const auto &genes : chromosome.get_genes()) {
          spdlog::debug("\tgene:{}", genes.get_name());
          if (genes.get_max_copies() != -1) {
            spdlog::debug("\tmax copies:{}", genes.get_max_copies());
          }
          if (genes.get_average_daily_crs() != -1) {
            spdlog::debug("\taverage crs:{}", genes.get_average_daily_crs());
          }
          for (const auto &cnv_crs : genes.get_cnv_daily_crs()) {
            spdlog::debug("\tcnv crs:{}", cnv_crs);
          }
          for (const auto &cnv_ec50 : genes.get_cnv_multiplicative_effect_on_EC50()) {
            spdlog::debug("\tcnv ec50:{}", cnv_ec50.get_drug_id());
            for (const auto &factor : cnv_ec50.get_factors()) {
              spdlog::debug("\t\tfactor:{}", factor);
            }
          }
          for (const auto &cnv_ec50 :
               genes.get_multiplicative_effect_on_ec50_for_2_or_more_mutations()) {
            spdlog::debug("\tcnv_ec50_2_or_more id:{}", cnv_ec50.get_drug_id());
            spdlog::debug("\tcnv_ec50_2_or_more factor:{}", cnv_ec50.get_factor());
          }
          for (const auto &aa_pos : genes.get_aa_positions()) {
            // spdlog::debug("\tpos {}",aa_pos.get_position());
            //   spdlog::debug("\t\taa:{}",aa_pos.get_amino_acids_string());
            //   spdlog::debug("\t\tcrs:{}",aa_pos.get_daily_crs_string());
            //   spdlog::debug("\t\tmultiplicative_effect_on_EC50:{}",aa_pos.get_multiplicative_effect_on_EC50_string());
            for (const auto &aa : aa_pos.get_amino_acids()) { spdlog::debug("\t\taa:{}", aa); }
            for (const auto &crs : aa_pos.get_daily_crs()) { spdlog::debug("\t\tcrs:{}", crs); }
            for (const auto &crs : aa_pos.get_multiplicative_effect_on_EC50()) {
              spdlog::debug("\t\tmultiplicative_effect_on_EC50: {}", crs.get_drug_id());
              for (const auto &factor : crs.get_factors()) {
                spdlog::debug("\t\t\tfactor:{}", factor);
              }
            }
          }
        }
      }
    }

    /* Any settings processed using settings from other settings should be called
     * with setting names rather than process_config()
     * This is to avoid circular linking of settings
     * SeasonalitySettings and MovementSettings are examples of such settings
     */

    model_settings_.process_config();
    simulation_timeframe_.process_config();
    transmission_settings_.process_config();
    population_demographic_.process_config();
    spatial_settings_.process_config();
    seasonality_settings_.process_config_using_number_of_locations(
        Model::get_spatial_data(), get_spatial_settings().get_number_of_locations());
    movement_settings_.process_config_using_spatial_settings(
        get_spatial_settings().get_spatial_distance_matrix(),
        get_spatial_settings().get_number_of_locations());
    parasite_parameters_.process_config();
    immune_system_parameters_.process_config_with_parasite_density(
        get_parasite_parameters()
            .get_parasite_density_levels()
            .get_log_parasite_density_asymptomatic(),
        get_parasite_parameters().get_parasite_density_levels().get_log_parasite_density_cured());
    /* Drug mut be parsed before genotype */
    drug_parameters_.process_config();
    therapy_parameters_.process_config();
    strategy_parameters_.process_config();
    genotype_parameters_.process_config_with_number_of_locations(
        get_spatial_settings().get_number_of_locations());
    epidemiological_parameters_.process_config();
    mosquito_parameters_.process_config_using_locations(location_db());
    if (config["agent_parameters"]) {
      agent_parameters_.process_config();
    }
    if (config["rapt_settings"] && rapt_settings_.get_is_enabled()) {
      rapt_settings_.process_config_with_starting_date(
          get_simulation_timeframe().get_starting_date());
    }

    /*
     * Parse population events last because it depends on all other settings
     */
    population_events_ = config["population_events"].as<PopulationEvents>();

    spdlog::info("Population events parsed successfully");

    return true;
  } catch (YAML::BadFile) {
    std::cerr << "Error: File not found" << '\n';
    return false;
  }
  return false;
}

size_t Config::number_of_parasite_types() { return Model::get_genotype_db()->size(); }

size_t Config::number_of_locations() const { return spatial_settings_.get_number_of_locations(); }

int Config::number_of_age_classes() const {
  return population_demographic_.get_number_of_age_classes();
}

int Config::number_of_tracking_days() const {
  return epidemiological_parameters_.get_number_of_tracking_days();
}

const std::vector<int> &Config::age_structure() const {
  return population_demographic_.get_age_structure();
}

std::vector<Spatial::Location> &Config::location_db() { return spatial_settings_.location_db(); }

// std::vector<IStrategy *>& Config::strategy_db() {
//   return strategy_parameters_.strategy_db;
// }

// GenotypeDatabase* Config::genotype_db() {
//   return Model::get_genotype_db();
// }

void Config::validate_all_cross_field_validations() {
  spdlog::info("Validating all cross field validations");
  /*----------------------------
  Validate model settings
  ----------------------------*/
  // Check if days between stdout output is a positive number
  if (model_settings_.get_days_between_stdout_output() < 0) {
    throw std::invalid_argument("Days between stdout output should be a positive number");
  }

  /*----------------------------
  Validate simulation timeframe
  ----------------------------*/
  SimulationTimeframe simulation_timeframe = simulation_timeframe_;
  // Check if ending date is greater than starting date
  if (simulation_timeframe.get_starting_date() > simulation_timeframe.get_ending_date()) {
    throw std::invalid_argument("Simulation ending date should be greater than starting date");
  }
  // Comparison period should be in between starting and ending date
  if (simulation_timeframe.get_start_of_comparison_period_date()
          < simulation_timeframe.get_starting_date()
      || simulation_timeframe.get_start_of_comparison_period_date()
             > simulation_timeframe.get_ending_date()) {
    throw std::invalid_argument("Comparison period should be in between starting and ending date");
  }
  // start_collect_data_day should be >= 0
  if (simulation_timeframe.get_start_collect_data_day() < 0) {
    throw std::invalid_argument("Start collect data day should be greater than or equal to 0");
  }

  /*----------------------------
  Validate transmission settings
  ----------------------------*/
  TransmissionSettings transmission_settings = transmission_settings_;
  // Check transmission_parameter in range [0,1]
  if (transmission_settings.get_transmission_parameter() < 0
      || transmission_settings.get_transmission_parameter() > 1) {
    throw std::invalid_argument("Transmission parameter should be in range [0,1]");
  }
  // Check if p_infection_from_an_infectious_bite is in range [0,1]
  if (transmission_settings.get_p_infection_from_an_infectious_bite() < 0
      || transmission_settings.get_p_infection_from_an_infectious_bite() > 1) {
    throw std::invalid_argument("P infection from an infectious bite should be in range [0,1]");
  }

  /*----------------------------
  Validate population demographic
  ----------------------------*/
  PopulationDemographic population_demographic = population_demographic_;
  // Check if number_of_age_classes is a positive number
  if (population_demographic.get_number_of_age_classes() <= 0) {
    throw std::invalid_argument("Number of age classes should be a positive number");
  }
  // Check if age_structure size is equal to number_of_age_classes
  if (population_demographic.get_age_structure().size()
      != population_demographic.get_number_of_age_classes()) {
    throw std::invalid_argument("Age structure size should be equal to number of age classes");
  }
  // Check if age_structure is positive array
  for (auto age_structure : population_demographic.get_age_structure()) {
    if (age_structure < 0) {
      throw std::invalid_argument("Age structure should be a positive array");
    }
  }
  // Check if initial_age_structure is positive array
  for (auto initial_age_structure : population_demographic.get_initial_age_structure()) {
    if (initial_age_structure < 0) {
      throw std::invalid_argument("Initial age structure should be a positive array");
    }
  }
  // Check if birth_rate is a positive number
  if (population_demographic.get_birth_rate() < 0) {
    throw std::invalid_argument("Birth rate should be a positive number");
  }
  // Check if death_rate_by_age_class is a positive array
  for (auto death_rate_by_age_class : population_demographic.get_death_rate_by_age_class()) {
    if (death_rate_by_age_class < 0) {
      throw std::invalid_argument("Death rate by age class should be a positive array");
    }
  }
  // Check if mortality_when_treatment_fail_by_age_class is a positive array
  for (auto mortality_when_treatment_fail_by_age_class :
       population_demographic.get_mortality_when_treatment_fail_by_age_class()) {
    if (mortality_when_treatment_fail_by_age_class < 0) {
      throw std::invalid_argument(
          "Mortality when treatment fail by age class should be a positive array");
    }
  }
  // Check if artificial_rescaling_of_population_size is a positive number
  if (population_demographic.get_artificial_rescaling_of_population_size() <= 0) {
    throw std::invalid_argument(
        "Artificial rescaling of population size should be a positive number");
  }

  /*----------------------------
  Validate spatial settings
  ----------------------------*/
  spatial_settings_.cross_validate();

  /*----------------------------
  Validate seasonality settings
  ----------------------------*/
  // SeasonalitySettings seasonality_settings = seasonality_settings_;
  // Check if rainfall file name is provided
  if (seasonality_settings_.get_enable() && seasonality_settings_.get_mode().empty()) {
    throw std::invalid_argument("Rainfall is enabled but mode is not provided");
  }
  // Check if rainfall file exists
  if (seasonality_settings_.get_enable() && seasonality_settings_.get_mode() == "rainfall"
      && !std::filesystem::exists(seasonality_settings_.get_seasonal_rainfall()->get_filename())) {
    throw std::invalid_argument("Rainfall file does not exist");
  }
  if (seasonality_settings_.get_enable() && seasonality_settings_.get_mode() == "rainfall"
      && seasonality_settings_.get_seasonal_rainfall()->get_period() > 365) {
    throw std::invalid_argument("Rainfall period should be less than or equal to 365");
  }
  if (seasonality_settings_.get_enable() && seasonality_settings_.get_mode() == "equation") {
    if (seasonality_settings_.get_seasonal_equation()->get_raster()
        && spatial_settings_.spatial_data()->get_raster(SpatialData::SpatialFileType::ECOCLIMATIC)
               == nullptr) {
      throw std::invalid_argument(
          "Ecoclimatic raster should be provided for equation based seasonality.");
    }
  }

  /*----------------------------
  Validate movement settings
  ----------------------------*/
  // Check if Barabasi parameters are valid
  if (movement_settings_.get_spatial_model_settings().get_name() == "Barabasi") {
    MovementSettings::BarabasiSM barabasi =
        movement_settings_.get_spatial_model_settings().get_barabasi_sm();
    if (barabasi.get_r_g_0() <= 0 || barabasi.get_beta_r() <= 0 || barabasi.get_kappa() <= 0) {
      throw std::invalid_argument("Barabasi parameters should be positive numbers");
    }
  }
  // Check if Wesolowski parameters are valid
  if (movement_settings_.get_spatial_model_settings().get_name() == "Wesolowski") {
    MovementSettings::WesolowskiSM wesolowski =
        movement_settings_.get_spatial_model_settings().get_wesolowski_sm();
    if (wesolowski.get_alpha() <= 0 || wesolowski.get_beta() <= 0 || wesolowski.get_gamma() <= 0
        || wesolowski.get_kappa() <= 0) {
      throw std::invalid_argument("Wesolowski parameters should be positive numbers");
    }
  }
  // Check if WesolowskiSurface parameters are valid
  if (movement_settings_.get_spatial_model_settings().get_name() == "WesolowskiSurface") {
    MovementSettings::WesolowskiSurfaceSM wesolowski_surface =
        movement_settings_.get_spatial_model_settings().get_wesolowski_surface_sm();
    if (wesolowski_surface.get_alpha() <= 0 || wesolowski_surface.get_beta() <= 0
        || wesolowski_surface.get_gamma() <= 0 || wesolowski_surface.get_kappa() <= 0) {
      throw std::invalid_argument("WesolowskiSurface parameters should be positive numbers");
    }
  }
  // Check if Marshall parameters are valid
  if (movement_settings_.get_spatial_model_settings().get_name() == "Marshall") {
    MovementSettings::MarshallSM marshall =
        movement_settings_.get_spatial_model_settings().get_marshall_sm();
    if (marshall.get_alpha() <= 0 || marshall.get_log_rho() <= 0 || marshall.get_tau() <= 0) {
      throw std::invalid_argument("Marshall parameters should be positive numbers");
    }
  }
  // Check if BurkinaFaso parameters are valid
  if (movement_settings_.get_spatial_model_settings().get_name() == "BurkinaFaso") {
    MovementSettings::BurkinaFasoSM burkina_faso =
        movement_settings_.get_spatial_model_settings().get_burkina_faso_sm();
    if (burkina_faso.get_alpha() <= 0 || burkina_faso.get_tau() <= 0
        || burkina_faso.get_log_rho() <= 0 || burkina_faso.get_capital() <= 0
        || burkina_faso.get_penalty() <= 0) {
      throw std::invalid_argument("BurkinaFaso parameters should be positive numbers");
    }
  }
  // Check if circular parameters are valid
  const MovementSettings::CirculationInfo &circulation_info =
      movement_settings_.get_circulation_info();
  if (circulation_info.get_max_relative_moving_value() < 0
      || circulation_info.get_number_of_moving_levels() <= 0) {
    throw std::invalid_argument(
        "Max relative moving value should be >= 0 and number of moving levels should be > 0");
  }
  // Check if circulation_percent is a positive number
  if (circulation_info.get_circulation_percent() < 0) {
    throw std::invalid_argument("Circulation percent cannot be negative");
  }

  /*----------------------------
  Validate parasite parameters
  ----------------------------*/
  ParasiteParameters parasite_parameters = parasite_parameters_;
  // All log numbers should be smaller than 6
  if (parasite_parameters.get_parasite_density_levels().get_log_parasite_density_cured() >= 6
      || parasite_parameters.get_parasite_density_levels().get_log_parasite_density_from_liver()
             >= 6
      || parasite_parameters.get_parasite_density_levels().get_log_parasite_density_asymptomatic()
             >= 6
      || parasite_parameters.get_parasite_density_levels().get_log_parasite_density_clinical() >= 6
      || parasite_parameters.get_parasite_density_levels().get_log_parasite_density_clinical_from()
             >= 6
      || parasite_parameters.get_parasite_density_levels().get_log_parasite_density_clinical_to()
             >= 6
      || parasite_parameters.get_parasite_density_levels().get_log_parasite_density_detectable()
             >= 6
      || parasite_parameters.get_parasite_density_levels()
                 .get_log_parasite_density_detectable_pfpr()
             >= 6
      || parasite_parameters.get_parasite_density_levels().get_log_parasite_density_pyrogenic()
             >= 6) {
    throw std::invalid_argument("All log parasite density should be smaller than 6");
  }
  // Check if recombination_parameters is in range[0,1]
  if (parasite_parameters.get_recombination_parameters().get_within_chromosome_recombination_rate()
          < 0
      || parasite_parameters.get_recombination_parameters()
                 .get_within_chromosome_recombination_rate()
             > 1) {
    throw std::invalid_argument("Within_chromosome_recombination_rate should be in range [0,1]");
  }

  /*----------------------------
  Validate immune system parameters
  ----------------------------*/
  ImmuneSystemParameters immune_system_parameters = immune_system_parameters_;
  // Check if all parameters are positive numbers
  if (immune_system_parameters.get_b1() < 0 || immune_system_parameters.get_b2() < 0
      || immune_system_parameters.get_duration_for_naive() < 0
      || immune_system_parameters.get_duration_for_fully_immune() < 0
      || immune_system_parameters.get_mean_initial_condition() < 0
      || immune_system_parameters.get_sd_initial_condition() < 0
      || immune_system_parameters.get_immune_inflation_rate() < 0
      || immune_system_parameters.get_min_clinical_probability() < 0
      || immune_system_parameters.get_max_clinical_probability() < 0
      || immune_system_parameters.get_immune_effect_on_progression_to_clinical() < 0
      || immune_system_parameters.get_age_mature_immunity() < 0
      || immune_system_parameters.get_factor_effect_age_mature_immunity() < 0
      || immune_system_parameters.get_midpoint() < 0) {
    throw std::invalid_argument("All immune system parameters should be positive numbers");
  }

  /*----------------------------
  Validate genotype parameters
  ----------------------------*/
  GenotypeParameters genotype_parameters = genotype_parameters_;
  // Check if mask contains 13 "|" characters
  if (get_pipe_count(genotype_parameters.get_mutation_mask()) != 13) {
    throw std::invalid_argument("Mutation mask should contain 13 '|' characters");
  }
  // Check if mutation rate is in mutation_probability_per_locus [0,1]
  if (genotype_parameters.get_mutation_probability_per_locus() < 0
      || genotype_parameters.get_mutation_probability_per_locus() > 1) {
    throw std::invalid_argument("Mutation rate should be in range [0,1]");
  }
  // Check override_ec50_patterns, each pattern size should match mutation mask size
  for (const auto &override_ec50_pattern : genotype_parameters.get_override_ec50_patterns()) {
    if (override_ec50_pattern.get_pattern().size()
        != genotype_parameters.get_mutation_mask().size()) {
      throw std::invalid_argument("Override EC50 pattern size should match mutation mask size");
    }
    // Pattern should contains 13 "|" characters
    if (get_pipe_count(override_ec50_pattern.get_pattern()) != 13) {
      throw std::invalid_argument("Override EC50 pattern should contain 13 '|' characters");
    }
  }
  // Check if aa_sequence in parasite_info of initial_parasite_info has correct string size and
  // format
  for (const auto &initial_parasite_info : genotype_parameters.get_initial_parasite_info_raw()) {
    for (const auto &parasite_info : initial_parasite_info.get_parasite_info()) {
      if (get_pipe_count(parasite_info.get_aa_sequence()) != 13) {
        throw std::invalid_argument(
            "Initial genotype aa sequence should contain 13 '|' characters");
      }
      if (parasite_info.get_aa_sequence().size()
          != genotype_parameters.get_mutation_mask().size()) {
        throw std::invalid_argument(
            "Initial genotype aa sequence size should match mutation mask size");
      }
    }
  }

  /*----------------------------
  Validate drug parameters
  ----------------------------*/
  // Loop through all drug parameters
  //  DrugParameters drug_parameters = drug_parameters_;
  for (const auto &drug : drug_parameters_.get_drug_db_raw()) {
    if (drug.second.get_half_life() < 0 || drug.second.get_maximum_parasite_killing_rate() < 0
        || drug.second.get_n() < 0 || drug.second.get_k() < 0 || drug.second.get_base_EC50() < 0) {
      throw std::invalid_argument("All drug parameters should be positive numbers");
    }
    // maximum_parasite_killing_rate should be in range [0,1]
    if (drug.second.get_maximum_parasite_killing_rate() < 0
        || drug.second.get_maximum_parasite_killing_rate() > 1) {
      throw std::invalid_argument("Maximum parasite killing rate should be in range [0,1]");
    }
    if (drug.second.get_age_specific_drug_concentration_sd().size()
        != population_demographic.get_number_of_age_classes()) {
      throw std::invalid_argument(
          "Age specific drug concentration size should match number of age classes");
    }
    if (drug.second.get_age_specific_drug_absorption().size() > 0
        && drug.second.get_age_specific_drug_absorption().size()
               != population_demographic.get_number_of_age_classes()) {
      throw std::invalid_argument(
          "Age specific drug absorption size should match number of age classes");
    }
  }

  /*----------------------------
  Validate therapy parameters
  ----------------------------*/
  TherapyParameters therapy_parameters = therapy_parameters_;
  // Check if tf_testing_day is positive number
  if (therapy_parameters.get_tf_testing_day() < 0) {
    throw std::invalid_argument("TF testing day should be a positive number");
  }
  // Check if tf_rate is in range [0,1]
  if (therapy_parameters.get_tf_rate() < 0 || therapy_parameters.get_tf_rate() > 1) {
    throw std::invalid_argument("TF rate should be in range [0,1]");
  }
  int therapy_max_dosing_days = 0;
  // Loop through therapy_db
  for (const auto &therapy : therapy_parameters.get_therapy_db_raw()) {
    for (auto drug_id : therapy.second.get_drug_ids()) {
      // Check if drug id is in drug_db keys
      if (!drug_parameters_.get_drug_db_raw().contains(drug_id)) {
        throw std::invalid_argument("Drug id should be in drug db keys");
      }
      // Check if dosing days are positive numbers
      for (auto dosing_day : therapy.second.get_dosing_days()) {
        if (dosing_day < 0) {
          throw std::invalid_argument("Dosing days should be positive numbers");
        }
        therapy_max_dosing_days = std::max(dosing_day, therapy_max_dosing_days);
      }
    }
  }

  /*----------------------------
  Validate strategy parameters
  ----------------------------*/
  StrategyParameters strategy_parameters = strategy_parameters_;
  // Check if initial_strategy_id is in strategy_db keys
  if (!strategy_parameters.get_strategy_db_raw().contains(
          strategy_parameters.get_initial_strategy_id())) {
    throw std::invalid_argument("Initial strategy id should be in strategy db keys");
  }

  /*----------------------------
  Validate epidemiological parameters
  ----------------------------*/
  EpidemiologicalParameters epidemiological_parameters = epidemiological_parameters_;
  // Check if number_of_tracking_days is a positive number
  if (epidemiological_parameters.get_number_of_tracking_days() <= 0) {
    throw std::invalid_argument("Number of tracking days should be a positive number");
  }
  // Check if days_to_clinical_under_five and days_to_clinical_over_five are positive numbers
  if (epidemiological_parameters.get_days_to_clinical_under_five() < 0
      || epidemiological_parameters.get_days_to_clinical_over_five() < 0) {
    throw std::invalid_argument(
        "Days to clinical under five and over five should be positive numbers");
  }
  // Check if days_mature_gametocyte_under_five and days_mature_gametocyte_over_five are positive
  // numbers
  if (epidemiological_parameters.get_days_mature_gametocyte_under_five() < 0
      || epidemiological_parameters.get_days_mature_gametocyte_over_five() < 0) {
    throw std::invalid_argument(
        "Days mature gametocyte under five and over five should be positive numbers");
  }
  // Check if p_compliance is in range [0,1]
  if (epidemiological_parameters.get_p_compliance() < 0
      || epidemiological_parameters.get_p_compliance() > 1) {
    throw std::invalid_argument("P compliance should be in range [0,1]");
  }
  // Check if min_dosing_days is positive number
  if (epidemiological_parameters.get_min_dosing_days() < 0) {
    throw std::invalid_argument("Min dosing days should be a positive number");
  }
  // Check if min_dosing_days is less than therapy_max_dosing_days
  if (epidemiological_parameters.get_min_dosing_days() >= therapy_max_dosing_days) {
    throw std::invalid_argument("Min dosing days should be less than therapy max dosing days");
  }
  // Check if gametocyte_level_under_artemisinin_action is in range [0,1]
  if (epidemiological_parameters.get_gametocyte_level_under_artemisinin_action() < 0
      || epidemiological_parameters.get_gametocyte_level_under_artemisinin_action() > 1) {
    throw std::invalid_argument(
        "Gametocyte level under artemisinin action should be in range [0,1]");
  }
  // Check if gametocyte_level_full is in range [0,1]
  if (epidemiological_parameters.get_gametocyte_level_full() < 0
      || epidemiological_parameters.get_gametocyte_level_full() > 1) {
    throw std::invalid_argument("Gametocyte level full transmission should be in range [0,1]");
  }
  // Check if p_relapse is in range [0,1]
  if (epidemiological_parameters.get_p_relapse() < 0
      || epidemiological_parameters.get_p_relapse() > 1) {
    throw std::invalid_argument("P relapse should be in range [0,1]");
  }
  // Check if relapse_duration is a positive number
  if (epidemiological_parameters.get_relapse_duration() < 0) {
    throw std::invalid_argument("Relapse duration should be a positive number");
  }
  // Check if relapse_rate is positive number
  if (epidemiological_parameters.get_relapse_rate() < 0) {
    throw std::invalid_argument("Relapse rate should be a positive number");
  }
  // Check if update_frequency is a positive number
  if (epidemiological_parameters.get_update_frequency() < 0) {
    throw std::invalid_argument("Update frequency should be a positive number");
  }
  // Check if tf_window_size is a positive number
  if (epidemiological_parameters.get_tf_window_size() < 0) {
    throw std::invalid_argument("TF window size should be a positive number");
  }
  // Check if fraction_mosquitoes_interrupted_feeding is in range [0,1]
  if (epidemiological_parameters.get_fraction_mosquitoes_interrupted_feeding() < 0
      || epidemiological_parameters.get_fraction_mosquitoes_interrupted_feeding() > 1) {
    throw std::invalid_argument("Fraction mosquitoes interrupted feeding should be in range [0,1]");
  }
  // Check if inflation_factor is positive number
  if (epidemiological_parameters.get_inflation_factor() < 0) {
    throw std::invalid_argument("Inflation factor should be a positive number");
  }
  const EpidemiologicalParameters::RelativeBitingInfo &relative_biting_info =
      epidemiological_parameters.get_relative_biting_info();
  // Check if relative_biting_info is positive number
  if (relative_biting_info.get_max_relative_biting_value() < 0
      || relative_biting_info.get_min_relative_biting_value() <= 0
      || relative_biting_info.get_number_of_biting_levels() <= 0) {
    throw std::invalid_argument("Relative biting info should be positive numbers");
  }
  // Check if min_relative_biting_value <= max_relative_biting_value
  if (relative_biting_info.get_min_relative_biting_value()
      >= relative_biting_info.get_max_relative_biting_value()) {
    throw std::invalid_argument(
        "Min relative biting value should be less than max relative biting value");
  }
  EpidemiologicalParameters::RelativeInfectivity relative_infectivity =
      epidemiological_parameters.get_relative_infectivity();
  // Check if relative_infectivity is positive number
  if (relative_infectivity.get_sigma() < 0 || relative_infectivity.get_ro_star() <= 0
      || relative_infectivity.get_blood_meal_volume() <= 0) {
    throw std::invalid_argument("Relative infectivity should be positive numbers");
  }

  /*----------------------------
  Validate mosquito parameters
  ----------------------------*/
  MosquitoParameters mosquito_parameters = mosquito_parameters_;
  // Check if mosquito_mode is either grid_based or location_based
  if (mosquito_parameters.get_mosquito_config().get_mode() != "grid_based"
      && mosquito_parameters.get_mosquito_config().get_mode() != "location_based") {
    throw std::invalid_argument("Mosquito mode should be either grid_based or location_based");
  }
  // If mode is grid_based, check if all raster file paths are provided
  if (mosquito_parameters.get_mosquito_config().get_mode() == "grid_based") {
    MosquitoParameters::GridBased grid_based =
        mosquito_parameters.get_mosquito_config().get_grid_based();
    if (grid_based.get_interrupted_feeding_rate_raster().empty()
        || grid_based.get_prmc_size_raster().empty()) {
      throw std::invalid_argument(
          "All raster file paths should be provided for grid based mosquito mode");
    }
  }
  // If location_based, check if all location sizes are equal
  if (mosquito_parameters.get_mosquito_config().get_mode() == "location_based") {
    MosquitoParameters::LocationBased location_based =
        mosquito_parameters.get_mosquito_config().get_location_based();
    if (location_based.get_interrupted_feeding_rate().empty()
        || location_based.get_prmc_size().empty()) {
      throw std::invalid_argument(
          "All locations should be provided for location based mosquito mode");
    }
    // Check if all location sizes are equal
    if (location_based.get_interrupted_feeding_rate().size()
        != location_based.get_prmc_size().size()) {
      throw std::invalid_argument("All location sizes should be equal");
    }
  }

  /*----------------------------
  Validate population events
  ----------------------------*/
  // Loop through all events
  for (const auto &population_event : population_events_.get_events_raw()) {
    // Check if name is provided
    if (population_event.get_name().empty()) {
      throw std::invalid_argument("Name should be provided for all population events");
    }
    for (auto event_info : population_event.get_info()) {
      // Check if event date is valid
      if (event_info.get_date() < simulation_timeframe.get_starting_date()
          || event_info.get_date() > simulation_timeframe.get_ending_date()) {
        throw std::invalid_argument(
            "Event date should be in between simulation starting and ending date");
      }
    }
  }
}

void Config::reload() { load(config_file_path_); }

