#include "StrategyParameters.h"

#include "Simulation/Model.h"

void StrategyParameters::process_config() {
  spdlog::info("Processing StrategyParameters");
  /*
   * Here we have to parse directly from YAML config file
   * because strategies are implemented in different classes
   * and this will make implementation more flexible.
   */
  for (std::size_t i = 0; i < node_.size(); i++) {
    auto s = read_strategy(node_, static_cast<int>(i));
    Model::get_strategy_db().push_back(std::move(s));
  }
  std::vector<MassDrugAdministration::beta_distribution_params>
      prob_individual_present_at_mda_distribution_;
  for (std::size_t i = 0;
       i < mass_drug_administration_.get_mean_prob_individual_present_at_mda()
               .size();
       i++) {
    const auto mean =
        mass_drug_administration_.get_mean_prob_individual_present_at_mda()[i];
    const auto sd =
        mass_drug_administration_.get_sd_prob_individual_present_at_mda()[i];

    MassDrugAdministration::beta_distribution_params params{};

    if (NumberHelpers::is_zero(sd)) {
      params.alpha = mean;
      params.beta = 0.0;
    } else {
      params.alpha = mean * mean * (1 - mean) / (sd * sd) - mean;
      params.beta = params.alpha / mean - params.alpha;
    }

    prob_individual_present_at_mda_distribution_.push_back(params);
  }
  // for (auto mda_prob : prob_individual_present_at_mda_distribution_) {
  //   std::cout << "alpha: " << mda_prob.alpha << " beta: " << mda_prob.beta <<
  //   std::endl;
  // }
  mass_drug_administration_.set_prob_individual_present_at_mda_distribution(
      prob_individual_present_at_mda_distribution_);
}

