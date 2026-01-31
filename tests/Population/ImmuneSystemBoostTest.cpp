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
  // clearance defaults for this test
  boost_cfg.clearance.max_extra_boost = 0.3;
  boost_cfg.clearance.half_life_days = 365.0;
  isp.set_immunity_boost(boost_cfg);
  model->get_config()->set_immune_system_parameters(isp);

  // Create immune system and attach dummy component
  ImmuneSystem immune;
  auto comp = std::make_unique<DummyImmuneComponentFixed>(&immune);
  comp->value = 0.2; // base immunity
  immune.set_immune_component(std::move(comp));
  immune.set_latest_immune_value(0.2);

  // Day 0: add extra boost (mapped to clearance/event boost in new API)
  mocks.scheduler->set_current_time(0);
  immune.add_clearance_event_boost(0.2, 0);
  double eff0 = immune.get_effective_immunity();
  EXPECT_NEAR(eff0, 0.4, 1e-9); // 0.2 base + 0.2 extra

  // Add again on same day - should clamp to max_extra_boost (0.3)
  immune.add_clearance_event_boost(0.2, 0);
  double eff1 = immune.get_effective_immunity();
  EXPECT_NEAR(eff1, 0.5, 1e-9); // base 0.2 + extra 0.3 (clamped)

  // Advance to half-life day and update - extra should halve
  mocks.scheduler->set_current_time(365);
  immune.update(); // will run decay
  double eff2 = immune.get_effective_immunity();
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
  // set daily clearance exposure amount and caps for this test
  boost_cfg.clearance.boost_per_exposure_day = 0.01;
  boost_cfg.clearance.max_extra_boost = 1.0;
  boost_cfg.clearance.half_life_days = 365.0;
  isp.set_immunity_boost(boost_cfg);
  model->get_config()->set_immune_system_parameters(isp);

  ImmuneSystem immune;
  auto comp = std::make_unique<DummyImmuneComponentFixed>(&immune);
  comp->value = 0.0;
  immune.set_immune_component(std::move(comp));
  immune.set_latest_immune_value(0.0);

  // Day 10: first exposure boost
  mocks.scheduler->set_current_time(10);
  // Legacy daily exposure mapped to clearance daily exposure in new API
  immune.add_daily_clearance_exposure_boost(10, 1.0);
  double eff_a = immune.get_effective_immunity();
  EXPECT_NEAR(eff_a, 0.01, 1e-9);

  // Re-apply same day -> no change
  immune.add_daily_clearance_exposure_boost(10, 1.0);
  double eff_b = immune.get_effective_immunity();
  EXPECT_NEAR(eff_b, 0.01, 1e-9);

  // Next day -> applied again
  mocks.scheduler->set_current_time(11);
  immune.add_daily_clearance_exposure_boost(11, 1.0);
  double eff_c = immune.get_effective_immunity();
  // extra at day 10 was 0.01; it decays for 1 day before adding new daily boost
  const double lambda = std::log(2.0) / 365.0;
  const double expected_extra = 0.01 * std::exp(-lambda * 1) + 0.01;
  const double expected_eff = expected_extra; // base immunity is 0.0 here
  EXPECT_NEAR(eff_c, expected_eff, 1e-9);

  model->release();
  test_fixtures::cleanup_test_files();
}

TEST(ImmuneSystemBoostTest, ClinicalChannelOnlyDailyAndEventBoost) {
  test_fixtures::create_complete_test_environment();
  utils::Cli::get_instance().set_input_path("test_input.yml");
  auto *model = Model::get_instance();
  auto mocks = test_fixtures::setup_model_with_mocks(model);

  auto isp = model->get_config()->get_immune_system_parameters();
  ImmuneSystemParameters::ImmunityBoostConfig boost_cfg{};
  boost_cfg.enable = true;
  // Clinical channel only
  boost_cfg.clinical.boost_per_exposure_day = 0.02;
  boost_cfg.clinical.max_extra_boost = 0.5;
  boost_cfg.clinical.half_life_days = 365.0;
  boost_cfg.clinical.exposure_gate_days = 0;
  // Leave clearance defaults (zero)
  isp.set_immunity_boost(boost_cfg);
  model->get_config()->set_immune_system_parameters(isp);

  ImmuneSystem immune;
  auto comp = std::make_unique<DummyImmuneComponentFixed>(&immune);
  comp->value = 0.10; // base immunity
  immune.set_immune_component(std::move(comp));
  immune.set_latest_immune_value(0.10);

  // Day 1: apply daily clinical exposure
  mocks.scheduler->set_current_time(1);
  immune.add_daily_clinical_exposure_boost(1, 1.0);
  double eff_clin = immune.get_effective_clinical_immunity();
  double eff_clear = immune.get_effective_clearance_immunity();

  EXPECT_NEAR(eff_clin, 0.12, 1e-9); // 0.10 + 0.02
  EXPECT_NEAR(eff_clear, 0.10, 1e-9); // unaffected

  // Event boost (clinical channel)
  immune.add_clinical_event_boost(0.05, 1);
  double eff_clin_evt = immune.get_effective_clinical_immunity();
  EXPECT_NEAR(eff_clin_evt, std::min(1.0, 0.12 + 0.05), 1e-9);

  model->release();
  test_fixtures::cleanup_test_files();
}

TEST(ImmuneSystemBoostTest, ClearanceChannelOnlyDailyBoost) {
  test_fixtures::create_complete_test_environment();
  utils::Cli::get_instance().set_input_path("test_input.yml");
  auto *model = Model::get_instance();
  auto mocks = test_fixtures::setup_model_with_mocks(model);

  auto isp = model->get_config()->get_immune_system_parameters();
  ImmuneSystemParameters::ImmunityBoostConfig boost_cfg{};
  boost_cfg.enable = true;
  // Clearance channel only
  boost_cfg.clearance.boost_per_exposure_day = 0.03;
  boost_cfg.clearance.max_extra_boost = 0.4;
  boost_cfg.clearance.half_life_days = 180.0;
  boost_cfg.clearance.exposure_gate_days = 0;
  isp.set_immunity_boost(boost_cfg);
  model->get_config()->set_immune_system_parameters(isp);

  ImmuneSystem immune;
  auto comp = std::make_unique<DummyImmuneComponentFixed>(&immune);
  comp->value = 0.20; // base immunity
  immune.set_immune_component(std::move(comp));
  immune.set_latest_immune_value(0.20);

  mocks.scheduler->set_current_time(2);
  immune.add_daily_clearance_exposure_boost(2, 1.0);
  double eff_clear = immune.get_effective_clearance_immunity();
  double eff_clin = immune.get_effective_clinical_immunity();

  EXPECT_NEAR(eff_clear, 0.23, 1e-9); // 0.20 + 0.03
  EXPECT_NEAR(eff_clin, 0.20, 1e-9);   // unaffected

  model->release();
  test_fixtures::cleanup_test_files();
}

TEST(ImmuneSystemBoostTest, BothChannelsDailyBoostsSeparateCapsAndDecay) {
  test_fixtures::create_complete_test_environment();
  utils::Cli::get_instance().set_input_path("test_input.yml");
  auto *model = Model::get_instance();
  auto mocks = test_fixtures::setup_model_with_mocks(model);

  auto isp = model->get_config()->get_immune_system_parameters();
  ImmuneSystemParameters::ImmunityBoostConfig boost_cfg{};
  boost_cfg.enable = true;
  // Both channels
  boost_cfg.clinical.boost_per_exposure_day = 0.01;
  boost_cfg.clinical.max_extra_boost = 0.05;
  boost_cfg.clinical.half_life_days = 100.0;
  boost_cfg.clinical.exposure_gate_days = 0;

  boost_cfg.clearance.boost_per_exposure_day = 0.02;
  boost_cfg.clearance.max_extra_boost = 0.06;
  boost_cfg.clearance.half_life_days = 200.0;
  boost_cfg.clearance.exposure_gate_days = 0;

  isp.set_immunity_boost(boost_cfg);
  model->get_config()->set_immune_system_parameters(isp);

  ImmuneSystem immune;
  auto comp = std::make_unique<DummyImmuneComponentFixed>(&immune);
  comp->value = 0.30; // base immunity
  immune.set_immune_component(std::move(comp));
  immune.set_latest_immune_value(0.30);

  // Day 5: apply both daily boosts
  mocks.scheduler->set_current_time(5);
  immune.add_daily_clinical_exposure_boost(5, 1.0);
  immune.add_daily_clearance_exposure_boost(5, 1.0);

  double eff_clin = immune.get_effective_clinical_immunity();
  double eff_clear = immune.get_effective_clearance_immunity();

  EXPECT_NEAR(eff_clin, 0.31, 1e-9); // 0.30 + 0.01
  EXPECT_NEAR(eff_clear, 0.32, 1e-9); // 0.30 + 0.02

  // Advance many days to test decay halves over half-life
  mocks.scheduler->set_current_time(5 + static_cast<int>(boost_cfg.clinical.half_life_days));
  immune.update();
  double eff_clin_after = immune.get_effective_clinical_immunity();
  // clinical extra 0.01 -> decays to ~0.005 => effective ~0.305
  EXPECT_NEAR(eff_clin_after, 0.30 + 0.01 * 0.5, 1e-6);

  model->release();
  test_fixtures::cleanup_test_files();
}
