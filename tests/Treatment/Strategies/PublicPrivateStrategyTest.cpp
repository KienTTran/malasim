#include <gtest/gtest.h>

#include <memory>

#include "Population/Person/Person.h"
#include "Simulation/Model.h"
#include "Treatment/Strategies/PublicPrivateMultiLocationStrategy.h"
#include "Treatment/Strategies/PublicPrivateStrategy.h"
#include "Treatment/Strategies/SFTStrategy.h"
#include "Treatment/Strategies/StrategyBuilder.h"
#include "Treatment/Therapies/Therapy.h"
#include "apps/malasim/MaSimAppInput.h"
#include "Utils/Random.h"
#include "fixtures/TestFileGenerators.h"

namespace {

class FixedFlatRandom : public utils::Random {
public:
  explicit FixedFlatRandom(const double value) : utils::Random(nullptr, 42), value_(value) {}

  double random_flat(double from, double to) override {
    (void)from;
    (void)to;
    return value_;
  }

private:
  double value_;
};

class CountingStrategy : public IStrategy {
public:
  explicit CountingStrategy(Therapy* therapy)
      : IStrategy("CountingStrategy", StrategyType::SFT), therapy_(therapy) {}

  void add_therapy(Therapy* therapy) override { therapy_ = therapy; }
  Therapy* get_therapy(Person* person) override {
    (void)person;
    return therapy_;
  }
  [[nodiscard]] std::string to_string() const override { return name; }
  void adjust_started_time_point(const int &current_time) override {
    adjusted_time = current_time;
    adjust_calls++;
  }
  void update_end_of_time_step() override { end_step_calls++; }
  void monthly_update() override { monthly_calls++; }

  int adjusted_time{-1};
  int adjust_calls{0};
  int end_step_calls{0};
  int monthly_calls{0};

private:
  Therapy* therapy_;
};

class PublicPrivateStrategyTest : public ::testing::Test {
protected:
  void SetUp() override {
    test_fixtures::setup_test_environment();
    test_fixtures::create_test_raster_2_locations("test_init_pop.asc", 1000.0);
    test_fixtures::create_test_raster_2_locations("test_beta.asc", 0.5);
    test_fixtures::create_test_raster_2_locations("test_treatment.asc", 0.6);
    test_fixtures::create_test_raster_2_locations("test_ecozone.asc", 1.0);
    test_fixtures::create_test_raster_2_locations("test_travel.asc", 0.1);

    Model::get_instance()->release();
    utils::MaSimAppInput input;
    input.input_path = "test_input.yml";
    Model::set_cli_input(input);
    ASSERT_TRUE(Model::get_instance()->initialize());

    ASSERT_GE(Model::get_therapy_db().size(), 2);
    public_child_ = std::make_unique<SFTStrategy>();
    public_child_->id = 100;
    public_child_->add_therapy(Model::get_therapy_db()[0].get());
    private_child_ = std::make_unique<SFTStrategy>();
    private_child_->id = 101;
    private_child_->add_therapy(Model::get_therapy_db()[1].get());

    person_0_ = std::make_unique<Person>();
    person_0_->set_location(0);
    person_1_ = std::make_unique<Person>();
    person_1_->set_location(1);
  }

  void TearDown() override {
    person_1_.reset();
    person_0_.reset();
    private_child_.reset();
    public_child_.reset();
    Model::get_instance()->release();
    test_fixtures::cleanup_test_files();
  }

  std::unique_ptr<SFTStrategy> public_child_;
  std::unique_ptr<SFTStrategy> private_child_;
  std::unique_ptr<Person> person_0_;
  std::unique_ptr<Person> person_1_;
};

TEST_F(PublicPrivateStrategyTest, GlobalStrategyReturnsTherapyAndSector) {
  PublicPrivateStrategy strategy;
  strategy.set_public_strategy(public_child_.get());
  strategy.set_private_strategy(private_child_.get());
  strategy.public_share = 0.25;

  Model::set_random(std::make_unique<FixedFlatRandom>(0.10));
  const auto public_selection = strategy.select_treatment(person_0_.get());
  EXPECT_EQ(public_selection.therapy, Model::get_therapy_db()[0].get());
  EXPECT_EQ(public_selection.sector, TreatmentSector::Public);

  Model::set_random(std::make_unique<FixedFlatRandom>(0.50));
  const auto private_selection = strategy.select_treatment(person_1_.get());
  EXPECT_EQ(private_selection.therapy, Model::get_therapy_db()[1].get());
  EXPECT_EQ(private_selection.sector, TreatmentSector::Private);

  strategy.public_share = 0.0;
  Model::set_random(std::make_unique<FixedFlatRandom>(0.0));
  EXPECT_EQ(strategy.select_treatment(person_0_.get()).sector, TreatmentSector::Private);

  strategy.public_share = 1.0;
  Model::set_random(std::make_unique<FixedFlatRandom>(0.999));
  EXPECT_EQ(strategy.select_treatment(person_0_.get()).sector, TreatmentSector::Public);
}

TEST_F(PublicPrivateStrategyTest, GlobalStrategyInterpolatesAndAppliesImmediatePeak) {
  PublicPrivateStrategy strategy;
  strategy.start_public_share = 0.2;
  strategy.peak_public_share = 0.8;
  strategy.public_share = 0.2;
  strategy.starting_time = 10;
  strategy.peak_after = 100;

  strategy.adjust_public_share(60);
  EXPECT_NEAR(strategy.public_share, 0.5, 1e-12);
  strategy.adjust_public_share(110);
  EXPECT_DOUBLE_EQ(strategy.public_share, 0.8);

  strategy.peak_after = 0;
  strategy.public_share = 0.2;
  strategy.adjust_public_share(10);
  EXPECT_DOUBLE_EQ(strategy.public_share, 0.8);
}

TEST_F(PublicPrivateStrategyTest, StrategyForwardsLifecycleToBothChildren) {
  CountingStrategy public_child(Model::get_therapy_db()[0].get());
  CountingStrategy private_child(Model::get_therapy_db()[1].get());
  PublicPrivateStrategy strategy;
  strategy.set_public_strategy(&public_child);
  strategy.set_private_strategy(&private_child);
  strategy.start_public_share = 0.3;

  strategy.adjust_started_time_point(42);
  strategy.update_end_of_time_step();
  strategy.monthly_update();

  EXPECT_EQ(public_child.adjusted_time, 42);
  EXPECT_EQ(private_child.adjusted_time, 42);
  EXPECT_EQ(public_child.adjust_calls, 1);
  EXPECT_EQ(private_child.adjust_calls, 1);
  EXPECT_EQ(public_child.end_step_calls, 1);
  EXPECT_EQ(private_child.end_step_calls, 1);
  EXPECT_EQ(public_child.monthly_calls, 1);
  EXPECT_EQ(private_child.monthly_calls, 1);
}

TEST_F(PublicPrivateStrategyTest, MultiLocationStrategyUsesLocationSpecificShares) {
  PublicPrivateMultiLocationStrategy strategy;
  strategy.set_public_strategy(public_child_.get());
  strategy.set_private_strategy(private_child_.get());
  strategy.public_share_by_location = {0.2, 0.8};
  strategy.start_public_share_by_location = {0.2, 0.8};
  strategy.peak_public_share_by_location = {0.6, 0.4};
  strategy.peak_after = 100;

  Model::set_random(std::make_unique<FixedFlatRandom>(0.5));
  EXPECT_EQ(strategy.select_treatment(person_0_.get()).sector, TreatmentSector::Private);
  EXPECT_EQ(strategy.select_treatment(person_1_.get()).sector, TreatmentSector::Public);

  strategy.adjust_public_shares(50);
  EXPECT_NEAR(strategy.public_share_by_location[0], 0.4, 1e-12);
  EXPECT_NEAR(strategy.public_share_by_location[1], 0.6, 1e-12);

  // Keep the person's location valid for the model while making it invalid for
  // this strategy's now-smaller location-share table.
  strategy.public_share_by_location.resize(1);
  EXPECT_THROW((void)strategy.select_treatment(person_1_.get()), std::out_of_range);
}

TEST_F(PublicPrivateStrategyTest, BuilderCreatesBothTypesAndValidatesLocationCounts) {
  YAML::Node global;
  global["name"] = "Global public/private";
  global["type"] = "PublicPrivate";
  global["public_strategy_id"] = 1;
  global["private_strategy_id"] = 0;
  global["start_public_share"] = 0.25;
  global["peak_public_share"] = 0.75;
  global["peak_after"] = 100;

  auto global_result = StrategyBuilder::build(global, 17);
  auto* global_strategy = dynamic_cast<PublicPrivateStrategy*>(global_result.get());
  ASSERT_NE(global_strategy, nullptr);
  EXPECT_EQ(global_strategy->get_public_strategy(), Model::get_strategy_db()[1].get());
  EXPECT_EQ(global_strategy->get_private_strategy(), Model::get_strategy_db()[0].get());

  YAML::Node multi;
  multi["name"] = "Multi-location public/private";
  multi["type"] = "PublicPrivateMultiLocation";
  multi["public_strategy_id"] = 1;
  multi["private_strategy_id"] = 0;
  multi["start_public_share_by_location"] = std::vector<double>{0.2, 0.8};
  multi["peak_public_share_by_location"] = std::vector<double>{0.6, 0.4};
  multi["peak_after"] = 100;

  auto multi_result = StrategyBuilder::build(multi, 17);
  auto* multi_strategy = dynamic_cast<PublicPrivateMultiLocationStrategy*>(multi_result.get());
  ASSERT_NE(multi_strategy, nullptr);
  EXPECT_EQ(multi_strategy->public_share_by_location, (DoubleVector{0.2, 0.8}));

  multi["start_public_share_by_location"] = std::vector<double>{0.2};
  EXPECT_THROW(StrategyBuilder::build(multi, 17), std::invalid_argument);

  global["private_strategy_id"] = 1;
  EXPECT_THROW(StrategyBuilder::build(global, 17), std::invalid_argument);
  global["private_strategy_id"] = 0;
  global["start_public_share"] = 1.1;
  EXPECT_THROW(StrategyBuilder::build(global, 17), std::invalid_argument);
  global["start_public_share"] = 0.2;
  global["peak_after"] = -1;
  EXPECT_THROW(StrategyBuilder::build(global, 17), std::invalid_argument);
}

TEST_F(PublicPrivateStrategyTest, MultiLocationValidatesChildrenAndFormatsIdentity) {
  PublicPrivateMultiLocationStrategy strategy;
  EXPECT_THROW(strategy.set_public_strategy(nullptr), std::invalid_argument);
  EXPECT_THROW(strategy.set_private_strategy(nullptr), std::invalid_argument);
  EXPECT_THROW(static_cast<void>(strategy.select_treatment(nullptr)), std::invalid_argument);
  strategy.id = 12;
  EXPECT_NE(strategy.to_string().find("12-"), std::string::npos);
  EXPECT_NE(strategy.to_string().find("public:-1-private:-1"), std::string::npos);
}

TEST_F(PublicPrivateStrategyTest, MultiLocationForwardsLifecycleAndAppliesImmediatePeak) {
  CountingStrategy public_child(Model::get_therapy_db()[0].get());
  CountingStrategy private_child(Model::get_therapy_db()[1].get());
  PublicPrivateMultiLocationStrategy strategy;
  strategy.set_public_strategy(&public_child);
  strategy.set_private_strategy(&private_child);
  strategy.start_public_share_by_location = {0.2, 0.8};
  strategy.peak_public_share_by_location = {0.7, 0.3};
  strategy.public_share_by_location = strategy.start_public_share_by_location;
  strategy.peak_after = 0;

  strategy.adjust_started_time_point(25);
  strategy.update_end_of_time_step();
  strategy.monthly_update();

  EXPECT_EQ(strategy.starting_time, 25);
  EXPECT_EQ(strategy.public_share_by_location, (DoubleVector{0.7, 0.3}));
  EXPECT_EQ(public_child.adjust_calls, 1);
  EXPECT_EQ(private_child.adjust_calls, 1);
  EXPECT_EQ(public_child.end_step_calls, 1);
  EXPECT_EQ(private_child.end_step_calls, 1);
  EXPECT_EQ(public_child.monthly_calls, 1);
  EXPECT_EQ(private_child.monthly_calls, 1);
}

}  // namespace
