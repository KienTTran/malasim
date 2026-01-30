#include "gtest/gtest.h"
#include "Population/ImmuneSystem/ImmuneSystem.h"
#include "Population/ImmuneSystem/ImmuneComponent.h"
#include "fixtures/MockFactories.h"
#include "fixtures/TestFileGenerators.h"
#include "Utils/Cli.h"

using namespace testing;

// Minimal Dummy component to control base immunity
struct DummyImmuneComponentFixed : public ImmuneComponent {
  double value{0.0};
  explicit DummyImmuneComponentFixed(ImmuneSystem* s = nullptr) : ImmuneComponent(s) {}
  double get_current_value() override { return value; }
  double get_decay_rate(const int &age) const override { return 0.0; }
  double get_acquire_rate(const int &age) const override { return 0.0; }
  void update() override {}
  void draw_random_immune() override {}
};

TEST(ImmuneSystemBoostTest, ExtraBoostClampAndHalfLife) {
  // Prepare test environment and model mocks
  test_fixtures::create_complete_test_environment();
  utils::Cli::get_instance().set_input_path("test_input.yml");
  auto *model = Model::get_instance();
  auto mocks = test_fixtures::setup_model_with_mocks(model);

  // Configure immunity boost parameters
  auto isp = model->get_config()->get_immune_system_parameters();
  ImmuneSystemParameters::ImmunityBoostConfig boost_cfg{};
  boost_cfg.enable = true;
  boost_cfg.boost_on_asymptomatic_recrudescence = 0.05;
  boost_cfg.boost_per_exposure_day = 0.002;
  boost_cfg.exposure_gate_days = 0;
  boost_cfg.max_extra_boost = 0.3;
  boost_cfg.half_life_days = 365.0;
  isp.set_immunity_boost(boost_cfg);
  model->get_config()->set_immune_system_parameters(isp);

  // Create immune system and attach dummy component
  ImmuneSystem immune;
  auto comp = std::make_unique<DummyImmuneComponentFixed>(&immune);
  comp->value = 0.2; // base immunity
  immune.set_immune_component(std::move(comp));
  immune.set_latest_immune_value(0.2);

  // Day 0: add extra boost
  mocks.scheduler->set_current_time(0);
  immune.add_extra_boost(0.2, 0);
  double eff0 = immune.get_effective_immunity(0);
  EXPECT_NEAR(eff0, 0.4, 1e-9); // 0.2 base + 0.2 extra

  // Add again on same day - should clamp to max_extra_boost (0.3)
  immune.add_extra_boost(0.2, 0);
  double eff1 = immune.get_effective_immunity(0);
  EXPECT_NEAR(eff1, 0.5, 1e-9); // base 0.2 + extra 0.3 (clamped)

  // Advance to half-life day and update - extra should halve
  mocks.scheduler->set_current_time(365);
  immune.update(); // will run decay
  double eff2 = immune.get_effective_immunity(365);
  // extra was 0.3 -> decays to 0.15 => effective 0.35
  EXPECT_NEAR(eff2, 0.35, 1e-6);

  // Cleanup
  model->release();
  test_fixtures::cleanup_test_files();
}

TEST(ImmuneSystemBoostTest, DailyExposureBoostOncePerDay) {
  test_fixtures::create_complete_test_environment();
  utils::Cli::get_instance().set_input_path("test_input.yml");
  auto *model = Model::get_instance();
  auto mocks = test_fixtures::setup_model_with_mocks(model);

  auto isp = model->get_config()->get_immune_system_parameters();
  ImmuneSystemParameters::ImmunityBoostConfig boost_cfg{};
  boost_cfg.enable = true;
  boost_cfg.boost_per_exposure_day = 0.01;
  boost_cfg.max_extra_boost = 1.0;
  boost_cfg.half_life_days = 365.0;
  boost_cfg.exposure_gate_days = 0;
  isp.set_immunity_boost(boost_cfg);
  model->get_config()->set_immune_system_parameters(isp);

  ImmuneSystem immune;
  auto comp = std::make_unique<DummyImmuneComponentFixed>(&immune);
  comp->value = 0.0;
  immune.set_immune_component(std::move(comp));
  immune.set_latest_immune_value(0.0);

  // Day 10: first exposure boost
  mocks.scheduler->set_current_time(10);
  immune.add_daily_exposure_boost(10);
  double eff_a = immune.get_effective_immunity(10);
  EXPECT_NEAR(eff_a, 0.01, 1e-9);

  // Re-apply same day -> no change
  immune.add_daily_exposure_boost(10);
  double eff_b = immune.get_effective_immunity(10);
  EXPECT_NEAR(eff_b, 0.01, 1e-9);

  // Next day -> applied again
  mocks.scheduler->set_current_time(11);
  immune.add_daily_exposure_boost(11);
  double eff_c = immune.get_effective_immunity(11);
  // extra at day 10 was 0.01; it decays for 1 day before adding new daily boost
  const double lambda = std::log(2.0) / 365.0;
  const double expected_extra = 0.01 * std::exp(-lambda * 1) + 0.01;
  const double expected_eff = expected_extra; // base immunity is 0.0 here
  EXPECT_NEAR(eff_c, expected_eff, 1e-9);

  model->release();
  test_fixtures::cleanup_test_files();
}
