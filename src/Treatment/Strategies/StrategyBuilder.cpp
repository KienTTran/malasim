#include "StrategyBuilder.h"
#include "IStrategy.h"
#include "SFTStrategy.h"
#include "Configuration/Config.h"
#include "CyclingStrategy.h"
#include "AdaptiveCyclingStrategy.h"
#include "MFTStrategy.h"
#include "NestedMFTStrategy.h"
#include "MFTRebalancingStrategy.h"
#include "MFTMultiLocationStrategy.h"
#include "NestedMFTMultiLocationStrategy.h"
#include "NovelDrugIntroductionStrategy.h"

StrategyBuilder::StrategyBuilder() = default;

StrategyBuilder::~StrategyBuilder() = default;

IStrategy* StrategyBuilder::build(const YAML::Node &ns, const int &strategy_id) {
  const auto type = IStrategy::StrategyTypeMap[ns["type"].as<std::string>()];
  switch (type) {
    case IStrategy::SFT:
      return buildSFTStrategy(ns, strategy_id);
    case IStrategy::Cycling:
      return buildCyclingStrategy(ns, strategy_id);
    case IStrategy::AdaptiveCycling:
      return buildAdaptiveCyclingStrategy(ns, strategy_id);
    case IStrategy::MFT:
      return buildMFTStrategy(ns, strategy_id);
    case IStrategy::MFTRebalancing:
      return buildMFTRebalancingStrategy(ns, strategy_id);
    case IStrategy::NestedMFT:
      return buildNestedSwitchingStrategy(ns, strategy_id);
    case IStrategy::MFTMultiLocation:
      return buildMFTMultiLocationStrategy(ns, strategy_id);
    case IStrategy::NestedMFTMultiLocation:
      return buildNestedMFTDifferentDistributionByLocationStrategy(ns,strategy_id);
    case IStrategy::NovelDrugIntroduction:
      return buildNovelDrugIntroductionStrategy(ns, strategy_id);
    default:
      return nullptr;
  }
}

void StrategyBuilder::add_therapies(const YAML::Node &ns, IStrategy* result) {
  for (auto i = 0; i < ns["therapy_ids"].size(); i++) {
    result->add_therapy(Model::get_config()->get_therapy_parameters().therapy_db[ns["therapy_ids"][i].as<int>()]);
  }
}

void StrategyBuilder::add_distributions(const YAML::Node &ns, DoubleVector &v) {
  for (auto i = 0; i < ns.size(); i++) {
    v.push_back(ns[i].as<double>());
  }
}

IStrategy* StrategyBuilder::buildSFTStrategy(const YAML::Node &ns, const int &strategy_id) {
  auto* result = new SFTStrategy();
  result->id = strategy_id;
  result->name = ns["name"].as<std::string>();
  result->add_therapy(Model::get_config()->get_therapy_parameters().therapy_db[ns["therapy_ids"][0].as<int>()]);
  return result;
}

IStrategy* StrategyBuilder::buildCyclingStrategy(const YAML::Node &ns, const int &strategy_id) {
  auto* result = new CyclingStrategy();
  result->id = strategy_id;
  result->name = ns["name"].as<std::string>();

  result->cycling_time = ns["cycling_time"].as<int>();
  result->next_switching_day = ns["cycling_time"].as<int>();

  add_therapies(ns, result);

  return result;
}

IStrategy* StrategyBuilder::buildAdaptiveCyclingStrategy(const YAML::Node &ns, const int &strategy_id) {
  auto* result = new AdaptiveCyclingStrategy();
  result->id = strategy_id;
  result->name = ns["name"].as<std::string>();

  result->trigger_value = ns["trigger_value"].as<double>();
  result->delay_until_actual_trigger = ns["delay_until_actual_trigger"].as<int>();
  result->turn_off_days = ns["turn_off_days"].as<int>();

  add_therapies(ns, result);
  return result;
}

IStrategy* StrategyBuilder::buildMFTStrategy(const YAML::Node &ns, const int &strategy_id) {
  auto* result = new MFTStrategy();
  result->id = strategy_id;
  result->name = ns["name"].as<std::string>();

  add_distributions(ns["distribution"], result->distribution);
  add_therapies(ns, result);
  return result;
}

IStrategy* StrategyBuilder::buildNestedSwitchingStrategy(const YAML::Node &ns, const int &strategy_id) {
  auto* result = new NestedMFTStrategy();
  result->id = strategy_id;
  result->name = ns["name"].as<std::string>();

  add_distributions(ns["start_distribution"], result->start_distribution);
  add_distributions(ns["start_distribution"], result->distribution);
  add_distributions(ns["peak_distribution"], result->peak_distribution);

  result->peak_after = ns["peak_after"].as<int>();

  for (int i = 0; i < ns["strategy_ids"].size(); i++) {
    result->add_strategy(
        Model::get_config()->get_strategy_parameters().strategy_db[ns["strategy_ids"][i].as<int>()]);
  }

  return result;
}

IStrategy* StrategyBuilder::buildMFTRebalancingStrategy(const YAML::Node &ns, const int &strategy_id) {
  auto* result = new MFTRebalancingStrategy();
  result->id = strategy_id;
  result->name = ns["name"].as<std::string>();

  add_distributions(ns["distribution"], result->distribution);
  add_distributions(ns["distribution"], result->next_distribution);

  add_therapies(ns, result);

  result->update_duration_after_rebalancing = ns["update_duration_after_rebalancing"].as<int>();
  result->delay_until_actual_trigger = ns["delay_until_actual_trigger"].as<int>();
  result->latest_adjust_distribution_time = 0;

  return result;
}

IStrategy*
StrategyBuilder::buildMFTMultiLocationStrategy(const YAML::Node &ns, const int &strategy_id) {
  auto* result = new MFTMultiLocationStrategy();
  result->id = strategy_id;
  result->name = ns["name"].as<std::string>();

  result->distribution.clear();
  result->distribution.resize(static_cast<unsigned long long int>(Model::get_config()->get_spatial_settings().get_number_of_locations()));

  result->start_distribution.clear();
  result->start_distribution.resize(static_cast<unsigned long long int>(Model::get_config()->get_spatial_settings().get_number_of_locations()));

  result->peak_distribution.clear();
  result->peak_distribution.resize(static_cast<unsigned long long int>(Model::get_config()->get_spatial_settings().get_number_of_locations()));

  for (auto loc = 0; loc < Model::get_config()->get_spatial_settings().get_number_of_locations(); loc++) {
    auto input_loc = ns["start_distribution_by_location"].size() < Model::get_config()->get_spatial_settings().get_number_of_locations() ? 0 : loc;
    add_distributions(ns["start_distribution_by_location"][input_loc], result->distribution[loc]);
  }
  for (auto loc = 0; loc < Model::get_config()->get_spatial_settings().get_number_of_locations(); loc++) {
    auto input_loc = ns["start_distribution_by_location"].size() < Model::get_config()->get_spatial_settings().get_number_of_locations() ? 0 : loc;
    add_distributions(ns["start_distribution_by_location"][input_loc], result->start_distribution[loc]);
  }

  for (auto loc = 0; loc < Model::get_config()->get_spatial_settings().get_number_of_locations(); loc++) {
    auto input_loc = ns["peak_distribution_by_location"].size() < Model::get_config()->get_spatial_settings().get_number_of_locations() ? 0 : loc;
    add_distributions(ns["peak_distribution_by_location"][input_loc], result->peak_distribution[loc]);
  }

  add_therapies(ns, result);
  result->peak_after = ns["peak_after"].as<int>();
  return result;
}

IStrategy* StrategyBuilder::buildNestedMFTDifferentDistributionByLocationStrategy(const YAML::Node &ns,
                                                                                  const int &strategy_id) {
  auto* result = new NestedMFTMultiLocationStrategy();
  result->id = strategy_id;
  result->name = ns["name"].as<std::string>();

  result->distribution.clear();
  result->distribution.resize(static_cast<unsigned long long int>(Model::get_config()->get_spatial_settings().get_number_of_locations()));

  result->start_distribution.clear();
  result->start_distribution.resize(static_cast<unsigned long long int>(Model::get_config()->get_spatial_settings().get_number_of_locations()));

  result->peak_distribution.clear();
  result->peak_distribution.resize(static_cast<unsigned long long int>(Model::get_config()->get_spatial_settings().get_number_of_locations()));

  for (auto loc = 0; loc < Model::get_config()->get_spatial_settings().get_number_of_locations(); loc++) {
    auto input_loc = ns["start_distribution_by_location"].size() < Model::get_config()->get_spatial_settings().get_number_of_locations() ? 0 : loc;
    add_distributions(ns["start_distribution_by_location"][input_loc], result->distribution[loc]);
  }
  for (auto loc = 0; loc < Model::get_config()->get_spatial_settings().get_number_of_locations(); loc++) {
    auto input_loc = ns["start_distribution_by_location"].size() < Model::get_config()->get_spatial_settings().get_number_of_locations() ? 0 : loc;
    add_distributions(ns["start_distribution_by_location"][input_loc], result->start_distribution[loc]);
  }

  for (auto loc = 0; loc < Model::get_config()->get_spatial_settings().get_number_of_locations(); loc++) {
    auto input_loc = ns["peak_distribution_by_location"].size() < Model::get_config()->get_spatial_settings().get_number_of_locations() ? 0 : loc;
    add_distributions(ns["peak_distribution_by_location"][input_loc], result->peak_distribution[loc]);
  }

  for (auto i = 0; i < ns["strategy_ids"].size(); i++) {
    result->add_strategy(Model::get_config()->get_strategy_parameters().strategy_db[ns["strategy_ids"][i].as<int>()]);
  }

  result->peak_after = ns["peak_after"].as<int>();
  //    std::cout << result->to_string() << std::endl;

  return result;
}

IStrategy*
StrategyBuilder::buildNovelDrugIntroductionStrategy(const YAML::Node &ns, const int strategy_id) {
  auto* result = new NovelDrugIntroductionStrategy();
  result->id = strategy_id;
  result->name = ns["name"].as<std::string>();

  add_distributions(ns["start_distribution"], result->start_distribution);
  add_distributions(ns["start_distribution"], result->distribution);
  add_distributions(ns["peak_distribution"], result->peak_distribution);

  result->peak_after = ns["peak_after"].as<int>();

  for (int i = 0; i < ns["strategy_ids"].size(); i++) {
    result->add_strategy(
        Model::get_config()->get_strategy_parameters().strategy_db[ns["strategy_ids"][i].as<int>()]);
  }

  result->newly_introduced_strategy_id = ns["newly_introduced_strategy_id"].as<int>();
  result->tf_threshold = ns["tf_threshold"].as<double>();

  result->replacement_fraction =  ns["replacement_fraction"].as<double>();
  result->replacement_duration =  ns["replacement_duration"].as<double>();

  return result;
}
