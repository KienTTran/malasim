#include "Config.h"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>

#include "Simulation/Model.h"

int inline get_pipe_count(const std::string &str) {
  int pipe_count = 0;
  for (const char ch : str) {
    if (ch == '|') { pipe_count++; }
  }
  return pipe_count;
}

void validate_share(const double share, const std::string &field_name) {
  if (!std::isfinite(share) || share < 0.0 || share > 1.0) {
    throw std::invalid_argument(field_name + " must be finite and in [0,1]");
  }
}

bool Config::load(const std::string &filename) {
  config_file_path_ = filename;
  try {
    YAML::Node config = YAML::LoadFile(filename);

    reset_load_state();
    spdlog::info("Configuration file loaded successfully: " + Model::get_cli_input().input_path);
    parse_configuration(config);
    parse_version6_pfpr_incidence_calibrations(config);

    spdlog::info("Configuration file parsed successfully");

    // Validate all cross field validations
    validate_all_cross_field_validations();

    log_genotype_configuration();

    process_configuration(config);
    parse_population_events(config);
    spdlog::info("Configuration file validated successfully");

    return true;
  } catch (YAML::BadFile) {
    std::cerr << "Error: File not found" << '\n';
    return false;
  }
  return false;
}

void Config::reset_load_state() {
  rapt_settings_.reset_to_defaults();
  version6_pfpr_incidence_calibrations_ = ImmuneSystemParameterOverrides{};
  has_version6_pfpr_incidence_calibrations_ = false;
  population_events_ = PopulationEvents{};
}

void Config::parse_configuration(const YAML::Node &config) {
  YAML::convert<ModelSettings>::decode(config["model_settings"], model_settings_);
  if (model_settings_.get_initial_seed_number() > 0) {
    Model::get_random()->set_seed(static_cast<uint64_t>(model_settings_.get_initial_seed_number()));
  }

  YAML::convert<SimulationTimeframe>::decode(config["simulation_timeframe"], simulation_timeframe_);
  YAML::convert<TransmissionSettings>::decode(config["transmission_settings"], transmission_settings_);
  YAML::convert<PopulationDemographic>::decode(config["population_demographic"], population_demographic_);
  YAML::convert<SpatialSettings>::decode(config["spatial_settings"], spatial_settings_);
  YAML::convert<SeasonalitySettings>::decode(config["seasonality_settings"], seasonality_settings_);
  YAML::convert<MovementSettings>::decode(config["movement_settings"], movement_settings_);
  YAML::convert<ParasiteParameters>::decode(config["parasite_parameters"], parasite_parameters_);
  YAML::convert<ImmuneSystemParameters>::decode(config["immune_system_parameters"],
                                                immune_system_parameters_);
  YAML::convert<GenotypeParameters>::decode(config["genotype_parameters"], genotype_parameters_);
  YAML::convert<DrugParameters>::decode(config["drug_parameters"], drug_parameters_);
  YAML::convert<TherapyParameters>::decode(config["therapy_parameters"], therapy_parameters_);
  YAML::convert<StrategyParameters>::decode(config["strategy_parameters"], strategy_parameters_);
  YAML::convert<EpidemiologicalParameters>::decode(config["epidemiological_parameters"], epidemiological_parameters_);
  YAML::convert<MosquitoParameters>::decode(config["mosquito_parameters"], mosquito_parameters_);

  if (config["rapt_settings"]) {
    YAML::convert<RaptSettings>::decode(config["rapt_settings"], rapt_settings_);
  }
}

void Config::parse_version6_pfpr_incidence_calibrations(const YAML::Node &config) {
  const auto calibration_ids_node = config["version6_pfpr_incidence_calibrations"];
  if (!calibration_ids_node) {
    spdlog::info(
        "No version6_pfpr_incidence_calibrations section found — using default parameters");
    return;
  }

  spdlog::info("Found version6_pfpr_incidence_calibrations section — parsing overrides");
  YAML::convert<ImmuneSystemParameterOverrides>::decode(
      calibration_ids_node, version6_pfpr_incidence_calibrations_);
  if (version6_pfpr_incidence_calibrations_.get_random_selection()) {
    select_random_immune_system_parameter_calibration_id();
  }

  has_version6_pfpr_incidence_calibrations_ = true;
  version6_pfpr_incidence_calibrations_.log_all();
  apply_selected_immune_system_parameter_calibration_id();
}

void Config::select_random_immune_system_parameter_calibration_id() {
  const auto &calibration_ids = version6_pfpr_incidence_calibrations_.get_calibration_ids();
  if (calibration_ids.empty()) {
    spdlog::warn(
        "version6_pfpr_incidence_calibrations: random_selection=true but calibration_ids is empty "
        "— skipping random selection");
    return;
  }

  std::vector<int> calibration_id_keys;
  calibration_id_keys.reserve(calibration_ids.size());
  for (const auto &calibration_id : calibration_ids) {
    calibration_id_keys.push_back(calibration_id.first);
  }

  const auto pick = static_cast<std::size_t>(
      Model::get_random()->random_uniform(static_cast<uint64_t>(calibration_id_keys.size())));
  const int selected_idx = calibration_id_keys[pick];
  version6_pfpr_incidence_calibrations_.set_chosen_calibration_id(selected_idx);
  spdlog::info(
      "version6_pfpr_incidence_calibrations: random_selection=true, num_calibration_ids={}, "
      "sampled chosen_calibration_id={}",
      calibration_id_keys.size(), selected_idx);
}

void Config::apply_selected_immune_system_parameter_calibration_id() {
  namespace P = ImmuneSystemOverridePaths;

  if (!version6_pfpr_incidence_calibrations_.has_selected_calibration_id()) {
    spdlog::warn(
        "version6_pfpr_incidence_calibrations: chosen_calibration_id={} not found in "
        "calibration_ids — no overrides applied",
        version6_pfpr_incidence_calibrations_.get_chosen_calibration_id());
    return;
  }

  const auto &calibration_id = version6_pfpr_incidence_calibrations_.get_selected_calibration_id();
  const int calibration_id_id = version6_pfpr_incidence_calibrations_.get_chosen_calibration_id();
  spdlog::info("Applying calibration_id[{}] overrides:", calibration_id_id);

  // immune_system_parameters overrides
  if (calibration_id.has(P::K_Z)) {
    const double immune_slope = calibration_id.get(P::K_Z, 0.0);
    spdlog::info("  {}={}", P::K_Z, immune_slope);
    immune_system_parameters_.set_immune_effect_on_progression_to_clinical(immune_slope);
  }
  if (calibration_id.has(P::K_KAPPA)) {
    const double kappa = calibration_id.get(P::K_KAPPA, 0.0);
    spdlog::info("  {}={}", P::K_KAPPA, kappa);
    immune_system_parameters_.set_factor_effect_age_mature_immunity(kappa);
  }
  if (calibration_id.has(P::K_MIDPOINT)) {
    const double midpoint = calibration_id.get(P::K_MIDPOINT, 0.0);
    spdlog::info("  {}={}", P::K_MIDPOINT, midpoint);
    immune_system_parameters_.set_midpoint(midpoint);
  }

  // epidemiological_parameters overrides
  if (calibration_id.has(P::K_P_CI_SYMP)) {
    const double p_ci_symp = calibration_id.get(P::K_P_CI_SYMP, 0.0);
    spdlog::info("  {}={}", P::K_P_CI_SYMP, p_ci_symp);
    auto coinfection = epidemiological_parameters_.get_allow_new_coinfection_to_cause_symptoms();
    coinfection.set_probability(p_ci_symp);
    epidemiological_parameters_.set_allow_new_coinfection_to_cause_symptoms(coinfection);
  }
  if (calibration_id.has(P::K_P_SEEK_BASE)) {
    const double p_seek_base = calibration_id.get(P::K_P_SEEK_BASE, 0.0);
    spdlog::info("  {}={}", P::K_P_SEEK_BASE, p_seek_base);
    auto age_based = epidemiological_parameters_.get_age_based_probability_of_seeking_treatment();
    auto power = age_based.get_power();
    power.base = p_seek_base;
    age_based.set_power(power);
    epidemiological_parameters_.set_age_based_probability_of_seeking_treatment(age_based);
  }

  // genotype_parameters override (skipped if value < 0)
  if (calibration_id.has(P::K_MUTATION_PROB)) {
    const double mutation_prob = calibration_id.get(P::K_MUTATION_PROB, -1.0);
    if (mutation_prob >= 0.0) {
      genotype_parameters_.set_mutation_probability_per_locus(mutation_prob);
      spdlog::info("  {} overridden to {}", P::K_MUTATION_PROB,
                   genotype_parameters_.get_mutation_probability_per_locus());
    } else {
      spdlog::info("  {} value < 0 -> keeping default {}", P::K_MUTATION_PROB,
                   genotype_parameters_.get_mutation_probability_per_locus());
    }
  }

  if (calibration_id.has(P::K_DEFAULT_CNV_REVERSION_MULTIPLIER)) {
    const double cnv_mult = calibration_id.get(P::K_DEFAULT_CNV_REVERSION_MULTIPLIER, -1.0);
    if (cnv_mult >= 0.0) {
      genotype_parameters_.set_default_cnv_reversion_multiplier(cnv_mult);
      spdlog::info("  {} overridden to {}", P::K_DEFAULT_CNV_REVERSION_MULTIPLIER,
                   genotype_parameters_.get_default_cnv_reversion_multiplier());
    } else {
      spdlog::info("  {} value < 0 -> keeping default {}", P::K_DEFAULT_CNV_REVERSION_MULTIPLIER,
                   genotype_parameters_.get_default_cnv_reversion_multiplier());
    }
  }

  spdlog::info("calibration_id[{}] overrides applied successfully", calibration_id_id);
}

void Config::log_genotype_configuration() const {
  spdlog::info("Genotype info:");
  for (const auto &parasites : genotype_parameters_.get_initial_parasite_info_raw()) {
    for (const auto &parasite : parasites.get_parasite_info()) {
      spdlog::info("{} {}", parasite.get_aa_sequence(), parasite.get_prevalence());
    }
  }

  for (const auto &chromosome : genotype_parameters_.get_pf_genotype_info().chromosome_infos) {
    log_chromosome_configuration(chromosome);
  }
}

void Config::log_chromosome_configuration(const GenotypeParameters::ChromosomeInfo &chromosome) {
  if (chromosome.get_chromosome_id() == -1) { return; }

  spdlog::debug("chromosome:{}", chromosome.get_chromosome_id());
  for (const auto &gene : chromosome.get_genes()) { log_gene_configuration(gene); }
}

void Config::log_gene_configuration(const GenotypeParameters::GeneInfo &gene) {
  spdlog::debug("\tgene:{}", gene.get_name());
  if (gene.get_max_copies() != -1) { spdlog::debug("\tmax copies:{}", gene.get_max_copies()); }
  if (gene.get_average_daily_crs() != -1) {
    spdlog::debug("\taverage crs:{}", gene.get_average_daily_crs());
  }
  for (const auto cnv_crs : gene.get_cnv_daily_crs()) { spdlog::debug("\tcnv crs:{}", cnv_crs); }
  for (const auto &cnv_ec50 : gene.get_cnv_multiplicative_effect_on_EC50()) {
    spdlog::debug("\tcnv ec50:{}", cnv_ec50.get_drug_id());
    for (const auto factor : cnv_ec50.get_factors()) { spdlog::debug("\t\tfactor:{}", factor); }
  }
  for (const auto &cnv_ec50 : gene.get_multiplicative_effect_on_ec50_for_2_or_more_mutations()) {
    spdlog::debug("\tcnv_ec50_2_or_more id:{}", cnv_ec50.get_drug_id());
    spdlog::debug("\tcnv_ec50_2_or_more factor:{}", cnv_ec50.get_factor());
  }
  for (const auto &amino_acid_position : gene.get_aa_positions()) {
    log_amino_acid_position_configuration(amino_acid_position);
  }
}

void Config::log_amino_acid_position_configuration(
    const GenotypeParameters::AminoAcidPosition &amino_acid_position) {
  for (const auto &amino_acid : amino_acid_position.get_amino_acids()) {
    spdlog::debug("\t\taa:{}", amino_acid);
  }
  for (const auto daily_crs : amino_acid_position.get_daily_crs()) {
    spdlog::debug("\t\tcrs:{}", daily_crs);
  }
  for (const auto &effect : amino_acid_position.get_multiplicative_effect_on_EC50()) {
    spdlog::debug("\t\tmultiplicative_effect_on_EC50: {}", effect.get_drug_id());
    for (const auto factor : effect.get_factors()) { spdlog::debug("\t\t\tfactor:{}", factor); }
  }
}

void Config::process_configuration(const YAML::Node &config) {
  // Settings that depend on other settings use explicit processing methods to
  // avoid circular dependencies.
  model_settings_.process_config();
  simulation_timeframe_.process_config();
  transmission_settings_.process_config();
  population_demographic_.process_config();
  spatial_settings_.process_config();
  seasonality_settings_.process_config_using_number_of_locations(
      Model::get_spatial_data(), get_spatial_settings().get_number_of_locations());
  movement_settings_.process_config_using_spatial_settings(
      get_spatial_settings().get_number_of_locations());
  parasite_parameters_.process_config();
  immune_system_parameters_.process_config_with_parasite_density(
      get_parasite_parameters()
          .get_parasite_density_levels()
          .get_log_parasite_density_asymptomatic(),
      get_parasite_parameters().get_parasite_density_levels().get_log_parasite_density_cured());

  // Drugs must be processed before genotype parameters.
  drug_parameters_.process_config();
  therapy_parameters_.process_config();
  strategy_parameters_.process_config();
  genotype_parameters_.process_config_with_number_of_locations(
      get_spatial_settings().get_number_of_locations());
  epidemiological_parameters_.process_config();
  mosquito_parameters_.process_config_using_locations(location_db());

  if (config["rapt_settings"] && rapt_settings_.get_is_enabled()) {
    rapt_settings_.process_config_with_starting_date(
        get_simulation_timeframe().get_starting_date());
  }
}

void Config::parse_population_events(const YAML::Node &config) {
  // Population events depend on all processed settings and must be parsed last.
  YAML::convert<PopulationEvents>::decode(config["population_events"], population_events_);
  validate_population_events();
  spdlog::info("Population events parsed successfully");
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
  validate_model_settings();
  validate_simulation_timeframe();
  validate_transmission_settings();
  validate_population_demographic();
  validate_spatial_settings();
  validate_seasonality_settings();
  validate_movement_settings();
  validate_parasite_parameters();
  validate_immune_system_parameters();
  validate_genotype_parameters();
  validate_drug_parameters();
  const int therapy_max_dosing_days = validate_therapy_parameters();
  validate_strategy_parameters();
  validate_epidemiological_parameters(therapy_max_dosing_days);
  validate_mosquito_parameters();
  validate_population_events();
}

void Config::validate_model_settings() const {
  /*----------------------------
  Validate model settings
  ----------------------------*/
  // Check if days between stdout output is a positive number
  if (model_settings_.get_days_between_stdout_output() < 0) {
    throw std::invalid_argument("Days between stdout output should be a positive number");
  }
}

void Config::validate_simulation_timeframe() const {
  /*----------------------------
  Validate simulation timeframe
  ----------------------------*/
  const auto &simulation_timeframe = simulation_timeframe_;
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
}

void Config::validate_transmission_settings() const {
  /*----------------------------
  Validate transmission settings
  ----------------------------*/
  const auto &transmission_settings = transmission_settings_;
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
}

void Config::validate_population_demographic() const {
  /*----------------------------
  Validate population demographic
  ----------------------------*/
  const auto &population_demographic = population_demographic_;
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
}

void Config::validate_spatial_settings() {
  /*----------------------------
  Validate spatial settings
  ----------------------------*/
  spatial_settings_.cross_validate();
}

void Config::validate_seasonality_settings() {
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
}

void Config::validate_movement_settings() const {
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
}

void Config::validate_parasite_parameters() const {
  /*----------------------------
  Validate parasite parameters
  ----------------------------*/
  const auto &parasite_parameters = parasite_parameters_;
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
}

void Config::validate_immune_system_parameters() const {
  /*----------------------------
  Validate immune system parameters
  ----------------------------*/
  const auto &immune_system_parameters = immune_system_parameters_;
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
}

void Config::validate_genotype_parameters() const {
  /*----------------------------
  Validate genotype parameters
  ----------------------------*/
  const auto &genotype_parameters = genotype_parameters_;
  // Mutation mask entries are indexed by genotype aa_sequence character position.
  if (genotype_parameters.get_mutation_mask().empty()) {
    throw std::invalid_argument("Mutation mask should not be empty");
  }
  // Check if mutation rate is in mutation_probability_per_locus [0,1]
  if (genotype_parameters.get_mutation_probability_per_locus() < 0
      || genotype_parameters.get_mutation_probability_per_locus() > 1) {
    throw std::invalid_argument("Mutation rate should be in range [0,1]");
  }
  GenotypeParameters::validate_cnv_reversion_multipliers(genotype_parameters);
  // Check override_ec50_patterns, each pattern size should match mutation mask size
  const auto mutation_mask_size = genotype_parameters.get_mutation_mask().size();
  for (const auto &override_ec50_pattern : genotype_parameters.get_override_ec50_patterns()) {
    if (override_ec50_pattern.get_pattern().size() != mutation_mask_size) {
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
      if (parasite_info.get_aa_sequence().size() != mutation_mask_size) {
        throw std::invalid_argument(
            "Initial genotype aa sequence size should match mutation mask size");
      }
    }
  }
}

void Config::validate_drug_parameters() const {
  /*----------------------------
  Validate drug parameters
  ----------------------------*/
  // Loop through all drug parameters
  //  DrugParameters drug_parameters = drug_parameters_;
  for (const auto &drug : drug_parameters_.get_drug_db_raw()) {
    if (drug.second.get_half_life() < 0 || drug.second.get_maximum_parasite_killing_rate() < 0
        || drug.second.get_n() < 0 || drug.second.get_k() < 0 || drug.second.get_base_ec50() < 0) {
      throw std::invalid_argument("All drug parameters should be positive numbers");
    }
    // maximum_parasite_killing_rate should be in range [0,1]
    if (drug.second.get_maximum_parasite_killing_rate() < 0
        || drug.second.get_maximum_parasite_killing_rate() > 1) {
      throw std::invalid_argument("Maximum parasite killing rate should be in range [0,1]");
    }
    if (drug.second.get_age_specific_drug_concentration_sd().size()
        != population_demographic_.get_number_of_age_classes()) {
      throw std::invalid_argument(
          "Age specific drug concentration size should match number of age classes");
    }
    if (drug.second.get_age_specific_drug_absorption().size() > 0
        && drug.second.get_age_specific_drug_absorption().size()
               != population_demographic_.get_number_of_age_classes()) {
      throw std::invalid_argument(
          "Age specific drug absorption size should match number of age classes");
    }
  }
}

int Config::validate_therapy_parameters() const {
  /*----------------------------
  Validate therapy parameters
  ----------------------------*/
  const auto &therapy_parameters = therapy_parameters_;
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
  return therapy_max_dosing_days;
}

void Config::validate_strategy_parameters() const {
  validate_strategy_database_ids();
  validate_public_private_strategies();
}

void Config::validate_strategy_database_ids() const {
  /*----------------------------
  Validate strategy database ids
  ----------------------------*/
  const auto &strategy_parameters = strategy_parameters_;
  // Check if initial_strategy_id is in strategy_db keys
  if (!strategy_parameters.get_strategy_db_raw().contains(
          strategy_parameters.get_initial_strategy_id())) {
    throw std::invalid_argument("Initial strategy id should be in strategy db keys");
  }
  const int second_line_strategy_id = strategy_parameters.get_second_line_strategy_id();
  if (second_line_strategy_id != -1
      && !strategy_parameters.get_strategy_db_raw().contains(second_line_strategy_id)) {
    throw std::invalid_argument("Second line strategy id should be -1 or in strategy db keys");
  }
}

void Config::validate_public_private_strategies() const {
  const auto &strategy_parameters = strategy_parameters_;
  for (const auto &[strategy_id, strategy] : strategy_parameters.get_strategy_db_raw()) {
    if (strategy.get_type() != "PublicPrivate"
        && strategy.get_type() != "PublicPrivateMultiLocation") {
      continue;
    }

    const auto public_id = strategy.get_public_strategy_id();
    const auto private_id = strategy.get_private_strategy_id();
    if (public_id == private_id) {
      throw std::invalid_argument("Public and private strategy ids must be distinct");
    }
    if (public_id < 0 || private_id < 0 || public_id >= strategy_id || private_id >= strategy_id
        || !strategy_parameters.get_strategy_db_raw().contains(public_id)
        || !strategy_parameters.get_strategy_db_raw().contains(private_id)) {
      throw std::invalid_argument(
          "Public and private strategy ids must reference previously defined strategies");
    }
    if (strategy.get_peak_after() < 0) {
      throw std::invalid_argument("peak_after must be non-negative");
    }

    if (strategy.get_type() == "PublicPrivate") {
      validate_share(strategy.get_start_public_share(), "start_public_share");
      validate_share(strategy.get_peak_public_share(), "peak_public_share");
      continue;
    }

    const auto expected_locations =
        static_cast<std::size_t>(spatial_settings_.get_number_of_locations());
    if (strategy.get_start_public_share_by_location().size() != expected_locations
        || strategy.get_peak_public_share_by_location().size() != expected_locations) {
      throw std::invalid_argument(
          "Public/private location shares must contain exactly one value per location");
    }
    for (const auto share : strategy.get_start_public_share_by_location()) {
      validate_share(share, "start_public_share_by_location");
    }
    for (const auto share : strategy.get_peak_public_share_by_location()) {
      validate_share(share, "peak_public_share_by_location");
    }
  }
}

void Config::validate_epidemiological_parameters(const int therapy_max_dosing_days) const {
  validate_epidemiological_treatment_parameters(therapy_max_dosing_days);
  validate_epidemiological_transmission_parameters();
}

void Config::validate_epidemiological_treatment_parameters(
    const int therapy_max_dosing_days) const {
  /*----------------------------
  Validate epidemiological treatment parameters
  ----------------------------*/
  const auto &epidemiological_parameters = epidemiological_parameters_;
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
}

void Config::validate_epidemiological_transmission_parameters() const {
  /*----------------------------
  Validate epidemiological transmission parameters
  ----------------------------*/
  const auto &epidemiological_parameters = epidemiological_parameters_;
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
}

void Config::validate_mosquito_parameters() const {
  /*----------------------------
  Validate mosquito parameters
  ----------------------------*/
  const auto &mosquito_parameters = mosquito_parameters_;
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
      throw std::invalid_argument(
          "Number of locations for interrupted feeding rate and prmc size should be equal");
    }
  }
}

void Config::validate_population_events() const {
  // Loop through all events
  for (const auto &population_event : population_events_.get_events_raw()) {
    // Check if name is provided
    if (population_event.get_name().empty()) {
      throw std::invalid_argument("Name should be provided for all population events");
    }
    for (auto event_info : population_event.get_info()) {
      // Check if event date is valid
      if (event_info.get_date() < simulation_timeframe_.get_starting_date()
          || event_info.get_date() > simulation_timeframe_.get_ending_date()) {
        throw std::invalid_argument(
            "Event date should be in between simulation starting and ending date");
      }
    }
  }
}

void Config::reload() { load(config_file_path_); }
