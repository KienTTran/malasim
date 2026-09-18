/*
 * PopulationEventBuilder.h
 *
 * Define the functions that are needed to prepare country-level (i.e., spatial)
 * or population-level events. This class roughly follows the factory pattern
 * and the build() method is called by preconfig_population_events::set_value
 * to defer the construction of the events.
 */
#ifndef POPULATIONEVENTBUILDER_H
#define POPULATIONEVENTBUILDER_H

#include <memory>
#include <vector>

#include "Events/Event.h"

class Config;

namespace YAML {
class Node;
}

class PopulationEventBuilder {
private:
  static void verify_single_node(const YAML::Node &node, const std::string_view &name);

public:
  static std::vector<std::unique_ptr<WorldEvent>> build(const YAML::Node &node);

  static std::vector<std::unique_ptr<WorldEvent>> build_introduce_parasite_events(
      const YAML::Node &node, Config* config);

  static std::vector<std::unique_ptr<WorldEvent>> build_introduce_parasites_periodically_events(
      const YAML::Node &node, Config* config);

  static std::vector<std::unique_ptr<WorldEvent>> build_introduce_parasites_periodically_events_v2(
      const YAML::Node &node, Config* config);

  static std::vector<std::unique_ptr<WorldEvent>> build_change_treatment_coverage_event(
      const YAML::Node &node, Config* config);

  static std::vector<std::unique_ptr<WorldEvent>> build_change_treatment_strategy_event(
      const YAML::Node &node, Config* config);

  static std::vector<std::unique_ptr<WorldEvent>> build_single_round_mda_event(
      const YAML::Node &node, Config* config);

  static std::vector<std::unique_ptr<WorldEvent>> build_smc_event(
      const YAML::Node &node, Config* config); //SMC

  static std::vector<std::unique_ptr<WorldEvent>> build_modify_nested_mft_strategy_event(
      const YAML::Node &node, Config* config);

  static std::vector<std::unique_ptr<WorldEvent>> build_introduce_plas2_parasite_events(
      const YAML::Node &node, Config* config);

  static std::vector<std::unique_ptr<WorldEvent>> build_turn_on_mutation_event(
      const YAML::Node &node, Config* config);

  static std::vector<std::unique_ptr<WorldEvent>> build_turn_off_mutation_event(
      const YAML::Node &node, Config* config);

  static std::vector<std::unique_ptr<WorldEvent>>
  build_change_within_host_induced_free_recombination_events(const YAML::Node node,
                                                             Config* pConfig);

  static std::vector<std::unique_ptr<WorldEvent>>
  build_change_mutation_probability_per_locus_events(const YAML::Node node, Config* pConfig);

  static std::vector<std::unique_ptr<WorldEvent>> build_change_interrupted_feeding_rate_event(
      const YAML::Node &node, Config* config);

  static std::vector<std::unique_ptr<WorldEvent>>
  build_introduce_amodiaquine_mutant_parasite_events(const YAML::Node &node, Config* config);

  static std::vector<std::unique_ptr<WorldEvent>>
  build_introduce_lumefantrine_mutant_parasite_events(const YAML::Node &node, Config* config);

  static std::vector<std::unique_ptr<WorldEvent>> build_introduce_580Y_mutant_events(
      const YAML::Node &node, Config* config);

  static std::vector<std::unique_ptr<WorldEvent>>
  build_introduce_triple_mutant_to_dpm_parasite_events(const YAML::Node &node, Config* config);

  static std::vector<std::unique_ptr<WorldEvent>> build_introduce_mutant_event(
      const YAML::Node &node, Config* config, const std::string &admin_level_name);

  static std::vector<std::unique_ptr<WorldEvent>> build_introduce_mutant_raster_event(
      const YAML::Node &node, Config* config);

  static std::vector<std::unique_ptr<WorldEvent>> build_annual_coverage_update_event(
      const YAML::Node &node, Config* config);

  static std::vector<std::unique_ptr<WorldEvent>> build_annual_beta_update_event(
      const YAML::Node &node, Config* config);

  static std::vector<std::unique_ptr<WorldEvent>> build_change_circulation_percent_event(
      const YAML::Node &node, Config* config);

  static std::vector<std::unique_ptr<WorldEvent>> build_importation_periodically_random_event(
      const YAML::Node &node, Config* config);

  static std::vector<std::unique_ptr<WorldEvent>> build_update_beta_raster_event(
      const YAML::Node &node, Config* config);

  static std::vector<std::unique_ptr<WorldEvent>> build_rotate_treatment_strategy_event(
      const YAML::Node &node, Config* config);

  static std::vector<std::unique_ptr<WorldEvent>> build_import_district_mutant_daily_events(
      const YAML::Node &node, Config* config);

  static std::vector<std::unique_ptr<WorldEvent>> build_change_mutation_mask_events(
      const YAML::Node &node, Config* config);
};

#endif
