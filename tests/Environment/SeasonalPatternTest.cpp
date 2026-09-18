#include <fmt/format.h>
#include <gtest/gtest.h>
#include <yaml-cpp/yaml.h>

#include <fstream>

#include "Configuration/SeasonalitySettings.h"
#include "Environment/SeasonalPattern.h"
#include "SeasonalPatternFixture.h"

#include "Configuration/Config.h"
#include "Simulation/Model.h"

class TestSeasonalPattern : public SeasonalPattern {
public:
  static std::unique_ptr<SeasonalPattern> build(const YAML::Node &node, SpatialData* spatial_data) {
    auto pattern = std::make_unique<TestSeasonalPattern>();
    YAML::convert<SeasonalPattern*>::decode(node["pattern"], pattern.get());
    pattern->SeasonalPattern::build(spatial_data);
    return pattern;
  }
  static std::unique_ptr<SpatialData> create_fake_spatial_data() {
    std::cout << "Creating fake spatial data" << std::endl;
    auto fake_spatial_data = std::make_unique<SpatialData>(nullptr);
    fake_spatial_data->get_admin_level_manager()->register_level("district");
    auto boundary = BoundaryData();
    boundary.location_to_unit = {1, 2};
    boundary.unit_to_locations = {{1, 2}, {3, 4}};
    boundary.min_unit_id = 1;
    boundary.max_unit_id = 2;
    boundary.unit_count = 2;
    fake_spatial_data->get_admin_level_manager()->set_boundary(0, boundary);
    return fake_spatial_data;
  }

protected:
  int get_admin_unit_for_location(int location) const override {
    if (Model::get_config() && Model::get_config()->get_spatial_settings().get_mode() == SpatialSettings::LOCATION_BASED_MODE) {
      return SeasonalPattern::get_admin_unit_for_location(location);
    }
    if (location == 999) { throw std::out_of_range("Location is out of range"); }
    return location;
  }
};

class SeasonalPatternTest : public ::testing::Test, protected SeasonalPatternFixture {
protected:
  void SetUp() override {
    SeasonalPatternFixture::SetUp();
    Model::set_config(std::make_unique<Config>());
    Model::get_config()->get_spatial_settings().set_mode(SpatialSettings::GRID_BASED_MODE);
  }
  void TearDown() override {
    SeasonalPatternFixture::TearDown();
    Model::get_instance()->release();
  }
};

TEST_F(SeasonalPatternTest, CanCreateWithMonthlyData) {
  auto node = YAML::Load(fmt::format(R"(
      enable: true
      mode: "pattern"
      pattern:
        admin_level: "district"
        filename: "{}"
        period: 12
    )",
                                     monthly_file));

  auto fake_spatial_data =
      std::unique_ptr<SpatialData>(TestSeasonalPattern::create_fake_spatial_data());
  auto pattern = TestSeasonalPattern::build(node, fake_spatial_data.get());
  ASSERT_NE(pattern, nullptr);
  EXPECT_TRUE(pattern->get_is_monthly());
  EXPECT_EQ(pattern->get_period(), 12);
  EXPECT_DOUBLE_EQ(pattern->get_seasonal_factor(date::sys_days{date::year{2000} / 1 / 1}, 1),
                   0.5);
}

TEST_F(SeasonalPatternTest, CanCreateWithDailyData) {
  auto node = YAML::Load(fmt::format(R"(
      enable: true
      mode: "pattern"
      pattern:
        admin_level: "district"
        filename: "{}"
        period: 365
    )",
                                     daily_file));

  auto fake_spatial_data =
      std::unique_ptr<SpatialData>(TestSeasonalPattern::create_fake_spatial_data());
  auto pattern = TestSeasonalPattern::build(node, fake_spatial_data.get());
  ASSERT_NE(pattern, nullptr);
  EXPECT_FALSE(pattern->get_is_monthly());
  EXPECT_EQ(pattern->get_period(), 365);
  EXPECT_NEAR(pattern->get_seasonal_factor(date::sys_days{date::year{2000} / 1 / 1}, 1),
              0.501, 1e-9);
}

TEST_F(SeasonalPatternTest, ThrowsOnInvalidPeriod) {
  auto node = YAML::Load(fmt::format(R"(
      enable: true
      mode: "pattern"
      pattern:
        admin_level: "district"
        filename: "{}"
        period: 100
    )",
                                     "test_pattern.csv"));

  auto fake_spatial_data =
      std::unique_ptr<SpatialData>(TestSeasonalPattern::create_fake_spatial_data());
  EXPECT_THROW(TestSeasonalPattern::build(node, fake_spatial_data.get()), std::invalid_argument);
}

TEST_F(SeasonalPatternTest, HandlesMissingDistrict) {
  auto node = YAML::Load(fmt::format(R"(
      enable: true
      mode: "pattern"
      pattern:
        admin_level: "district"
        filename: "{}"
        period: 12
    )",
                                     monthly_file));

  auto fake_spatial_data =
      std::unique_ptr<SpatialData>(TestSeasonalPattern::create_fake_spatial_data());
  auto pattern = TestSeasonalPattern::build(node, fake_spatial_data.get());
  ASSERT_NE(pattern, nullptr);
  EXPECT_THROW(pattern->get_seasonal_factor(date::sys_days{date::year{2000} / 1 / 1}, 999),
               std::out_of_range);
}

TEST_F(SeasonalPatternTest, RejectsUnknownAdminLevelAndMismatchedBoundaryCount) {
  const auto unknown_node = YAML::Load(fmt::format(R"(
      enable: true
      mode: "pattern"
      pattern:
        admin_level: "unknown"
        filename: "{}"
        period: 12
    )",
                                                  monthly_file));
  auto fake_spatial_data =
      std::unique_ptr<SpatialData>(TestSeasonalPattern::create_fake_spatial_data());
  EXPECT_THROW(TestSeasonalPattern::build(unknown_node, fake_spatial_data.get()),
               std::runtime_error);

  const std::string one_unit_file = "test_one_unit_pattern.csv";
  {
    std::ofstream output(one_unit_file);
    output << "admin_unit_id,jan,feb,mar,apr,may,jun,jul,aug,sep,oct,nov,dec\n"
            "1,0.5,0.6,0.7,0.8,0.9,1.0,1.1,1.0,0.9,0.8,0.7,0.6\n";
  }
  const auto mismatch_node = YAML::Load(fmt::format(R"(
      enable: true
      mode: "pattern"
      pattern:
        admin_level: "district"
        filename: "{}"
        period: 12
    )",
                                                    one_unit_file));
  EXPECT_THROW(TestSeasonalPattern::build(mismatch_node, fake_spatial_data.get()),
               std::runtime_error);
  std::remove(one_unit_file.c_str());
}

TEST_F(SeasonalPatternTest, HandlesNonExistentFile) {
  auto node = YAML::Load(R"(
      enable: true
      mode: "pattern"
      pattern:
        admin_level: "district"
        filename: "non_existent_file.csv"
        period: 12
    )");
  auto fake_spatial_data =
      std::unique_ptr<SpatialData>(TestSeasonalPattern::create_fake_spatial_data());
  EXPECT_THROW(TestSeasonalPattern::build(node, fake_spatial_data.get()), std::runtime_error);
}

TEST_F(SeasonalPatternTest, LocationBasedMode) {
  // Set to location_based mode
  Model::get_config()->get_spatial_settings().set_mode(SpatialSettings::LOCATION_BASED_MODE);

  auto node = YAML::Load(fmt::format(R"(
      enable: true
      mode: "pattern"
      pattern:
        admin_level: "district"
        filename: "{}"
        period: 12
    )",
                                     monthly_file));

  auto fake_spatial_data =
      std::unique_ptr<SpatialData>(TestSeasonalPattern::create_fake_spatial_data());
  
  // This should not throw even though spatial data's admin boundaries are minimal
  auto pattern = TestSeasonalPattern::build(node, fake_spatial_data.get());

  ASSERT_NE(pattern, nullptr);
  EXPECT_TRUE(pattern->get_is_monthly());
  EXPECT_EQ(pattern->get_period(), 12);

  // In LOCATION_BASED_MODE, all locations map to min_admin_unit_id (which is 0)
  // and they receive factors parsed from the first row of the CSV independent of what was passed.
  EXPECT_DOUBLE_EQ(pattern->get_seasonal_factor(date::sys_days{date::year{2000}/1/1}, 999), 0.5);
  EXPECT_DOUBLE_EQ(pattern->get_seasonal_factor(date::sys_days{date::year{2000}/2/1}, 1), 0.6);
  EXPECT_DOUBLE_EQ(pattern->get_seasonal_factor(date::sys_days{date::year{2000}/3/1}, 42), 0.7);
}

TEST_F(SeasonalPatternTest, RejectsNegativeFactorsAndWrongFactorCount) {
  const std::string negative_file = "test_negative_pattern.csv";
  {
    std::ofstream output(negative_file);
    output << "admin_unit_id,jan,feb,mar,apr,may,jun,jul,aug,sep,oct,nov,dec\n"
            << "1,0.5,-0.1,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5\n";
  }
  auto negative_node = YAML::Load(fmt::format(R"(
      enable: true
      mode: "pattern"
      pattern:
        admin_level: "district"
        filename: "{}"
        period: 12
    )", negative_file));
  auto fake_spatial_data = TestSeasonalPattern::create_fake_spatial_data();
  EXPECT_THROW(TestSeasonalPattern::build(negative_node, fake_spatial_data.get()),
               std::runtime_error);
  std::remove(negative_file.c_str());

  const std::string short_file = "test_short_pattern.csv";
  {
    std::ofstream output(short_file);
    output << "admin_unit_id,jan,feb,mar,apr,may,jun,jul,aug,sep,oct,nov,dec\n"
            << "1,0.5,0.6,0.7,0.8,0.9,1.0,1.1,1.0,0.9,0.8,0.7\n";
  }
  auto short_node = YAML::Load(fmt::format(R"(
      enable: true
      mode: "pattern"
      pattern:
        admin_level: "district"
        filename: "{}"
        period: 12
    )", short_file));
  EXPECT_THROW(TestSeasonalPattern::build(short_node, fake_spatial_data.get()),
               std::runtime_error);
  std::remove(short_file.c_str());
}

TEST_F(SeasonalPatternTest, RejectsAdminUnitsThatDoNotStartAtZeroOrOne) {
  const std::string invalid_file = "test_invalid_index_pattern.csv";
  {
    std::ofstream output(invalid_file);
    output << "admin_unit_id,jan,feb,mar,apr,may,jun,jul,aug,sep,oct,nov,dec\n"
            << "2,0.5,0.6,0.7,0.8,0.9,1.0,1.1,1.0,0.9,0.8,0.7,0.6\n";
  }
  auto node = YAML::Load(fmt::format(R"(
      enable: true
      mode: "pattern"
      pattern:
        admin_level: "district"
        filename: "{}"
        period: 12
    )", invalid_file));
  auto fake_spatial_data = TestSeasonalPattern::create_fake_spatial_data();
  EXPECT_THROW(TestSeasonalPattern::build(node, fake_spatial_data.get()), std::runtime_error);
  std::remove(invalid_file.c_str());
}
