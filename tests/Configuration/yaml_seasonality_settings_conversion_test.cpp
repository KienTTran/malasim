#include <gtest/gtest.h>
#include <yaml-cpp/yaml.h>
#include "Configuration/SeasonalitySettings.h"
#include "fixtures/TestFileGenerators.h"

class SeasonalitySettingsTest : public ::testing::Test {
protected:
    SeasonalitySettings default_settings = SeasonalitySettings();

    void SetUp() override {
        // Initialize default SeasonalitySettings object using setters
        auto equation = std::make_unique<SeasonalEquation>();
        equation->set_raster_base(std::vector<double>{2.5});
        equation->set_raster_A(std::vector<double>{0.4});
        equation->set_raster_B(std::vector<double>{0.6});
        equation->set_raster_phi(std::vector<int>{146});
        equation->set_base(std::vector<double>{2.5});
        equation->set_A(std::vector<double>{0.4});
        equation->set_B(std::vector<double>{0.6});
        equation->set_phi(std::vector<int>{146});
        equation->set_raster(true);

        auto rainfall = std::make_unique<SeasonalRainfall>();
        rainfall->set_filename("sample_inputs/dev_seasonality.csv");
        rainfall->set_period(365);

        auto pattern = std::make_unique<SeasonalPattern>();
        pattern->set_filename("sample_inputs/dev_seasonality_pattern.csv");
        pattern->set_period(365);

        default_settings.set_enable(true);
        default_settings.set_mode("pattern");
        default_settings.set_seasonal_rainfall(std::move(rainfall));
        default_settings.set_seasonal_equation(std::move(equation));
        default_settings.set_seasonal_pattern(std::move(pattern));

    }
};

// Test encoding functionality
TEST_F(SeasonalitySettingsTest, EncodeSeasonalitySettings) {
    YAML::Node node = YAML::convert<SeasonalitySettings>::encode(default_settings);

    EXPECT_EQ(node["enable"].as<bool>(), default_settings.get_enable());
    EXPECT_EQ(node["mode"].as<std::string>(), default_settings.get_mode());

    if (default_settings.get_mode() == "rainfall") {
      auto rainfall = default_settings.get_seasonal_rainfall();
      EXPECT_EQ(node["rainfall"]["filename"].as<std::string>(), rainfall->get_filename());
      EXPECT_EQ(node["rainfall"]["period"].as<int>(), rainfall->get_period());
    }

    if (default_settings.get_mode() == "rainfall") {
      auto pattern = default_settings.get_seasonal_pattern();
      EXPECT_EQ(node["pattern"]["filename"].as<std::string>(), pattern->get_filename());
      EXPECT_EQ(node["pattern"]["period"].as<int>(), pattern->get_period());
    }

    if (default_settings.get_mode() == "equation") {
      auto equation = default_settings.get_seasonal_equation();
      EXPECT_EQ(node["equation"]["base"].as<std::vector<double>>(), equation->get_base());
      EXPECT_EQ(node["equation"]["a"].as<std::vector<double>>(), equation->get_A());
      EXPECT_EQ(node["equation"]["b"].as<std::vector<double>>(), equation->get_B());
      EXPECT_EQ(node["equation"]["phi"].as<std::vector<int>>(), equation->get_phi());
    }
}

// Test decoding functionality
TEST_F(SeasonalitySettingsTest, DecodeSeasonalitySettings) {
    YAML::Node node;
    node["enable"] = true;
    node["mode"] = "rainfall";
    node["equation"]["base"] = std::vector<double>{2.5};
    node["equation"]["a"] = std::vector<double>{0.4};
    node["equation"]["b"] = std::vector<double>{0.6};
    node["equation"]["phi"] = std::vector<int>{146};
    node["equation"]["raster"] = true;
    node["rainfall"]["filename"] = "sample_inputs/dev_seasonality.csv";
    node["rainfall"]["period"] = 365;
    node["pattern"]["filename"] = "sample_inputs/dev_seasonality_pattern.csv";
    node["pattern"]["period"] = 365;

    SeasonalitySettings decoded_settings;
    EXPECT_NO_THROW(YAML::convert<SeasonalitySettings>::decode(node, decoded_settings));
    EXPECT_EQ(decoded_settings.get_enable(), true);

    if (decoded_settings.get_mode() == "rainfall") {
      auto decoded_rainfall = decoded_settings.get_seasonal_rainfall();
      EXPECT_EQ(decoded_rainfall->get_filename(), "sample_inputs/dev_seasonality.csv");
      EXPECT_EQ(decoded_rainfall->get_period(), 365);
    }

    if (decoded_settings.get_mode() == "pattern") {
      auto decoded_pattern = decoded_settings.get_seasonal_pattern();
      EXPECT_EQ(decoded_pattern->get_filename(), "sample_inputs/dev_seasonality_pattern.csv");
      EXPECT_EQ(decoded_pattern->get_period(), 365);
    }

    if (decoded_settings.get_mode() == "equation") {
      auto decoded_equation = decoded_settings.get_seasonal_equation();
      EXPECT_EQ(decoded_equation->get_base(), std::vector<double>{2.5});
      EXPECT_EQ(decoded_equation->get_A(), std::vector<double>{0.4});
      EXPECT_EQ(decoded_equation->get_B(), std::vector<double>{0.6});
      EXPECT_EQ(decoded_equation->get_phi(), std::vector<int>{146});
    }
}

// Test missing fields during decoding
TEST_F(SeasonalitySettingsTest, DecodeSeasonalitySettingsMissingField) {
    YAML::Node node;
    node["enable"] = true;  // Intentionally omit other fields

    SeasonalitySettings decoded_settings;
    EXPECT_THROW(YAML::convert<SeasonalitySettings>::decode(node, decoded_settings), std::runtime_error);
}

TEST_F(SeasonalitySettingsTest, EncodesAllConfiguredSeasonalityModels) {
    const auto node = YAML::convert<SeasonalitySettings>::encode(default_settings);
    EXPECT_EQ(node["equation"]["base"].as<std::vector<double>>(), std::vector<double>{2.5});
    EXPECT_EQ(node["rainfall"]["filename"].as<std::string>(), "sample_inputs/dev_seasonality.csv");
    EXPECT_EQ(node["rainfall"]["period"].as<int>(), 365);
}

TEST_F(SeasonalitySettingsTest, DecodesEquationRainfallAndPatternModes) {
    const auto equation = YAML::Load(R"(
      enable: true
      mode: equation
      equation:
        raster: true
        base: [2.5]
        a: [0.4]
        b: [0.6]
        phi: [146]
    )");
    SeasonalitySettings equation_settings;
    ASSERT_TRUE(YAML::convert<SeasonalitySettings>::decode(equation, equation_settings));
    ASSERT_NE(equation_settings.get_seasonal_equation(), nullptr);

    const auto rainfall = YAML::Load(R"(
      enable: true
      mode: rainfall
      rainfall:
        filename: sample_inputs/dev_seasonality.csv
        period: 365
    )");
    SeasonalitySettings rainfall_settings;
    ASSERT_TRUE(YAML::convert<SeasonalitySettings>::decode(rainfall, rainfall_settings));
    ASSERT_NE(rainfall_settings.get_seasonal_rainfall(), nullptr);

    const auto pattern = YAML::Load(R"(
      enable: true
      mode: pattern
      pattern:
        filename: sample_inputs/dev_seasonality_pattern.csv
        period: 365
        admin_level: district
    )");
    SeasonalitySettings pattern_settings;
    ASSERT_TRUE(YAML::convert<SeasonalitySettings>::decode(pattern, pattern_settings));
    ASSERT_NE(pattern_settings.get_seasonal_pattern(), nullptr);
}

TEST_F(SeasonalitySettingsTest, RejectsMissingModeAndInvalidPatternPeriod) {
    YAML::Node missing_mode;
    missing_mode["enable"] = true;
    EXPECT_THROW(YAML::convert<SeasonalitySettings>::decode(missing_mode,
                                                            default_settings), std::runtime_error);

    SeasonalPattern pattern;
    const auto invalid_period = YAML::Load("filename: pattern.csv\nperiod: 30\nadmin_level: district");
    EXPECT_THROW(YAML::convert<SeasonalPattern*>::decode(invalid_period, &pattern),
                 std::invalid_argument);
}

class SeasonalitySettingsStandaloneTest : public ::testing::Test {
protected:
    void TearDown() override { test_fixtures::cleanup_test_files(); }
};

TEST_F(SeasonalitySettingsStandaloneTest, RejectsMissingNestedModelFields) {
    SeasonalEquation equation;
    EXPECT_THROW(YAML::convert<SeasonalEquation*>::decode(YAML::Load("base: [1]"), &equation),
                 std::runtime_error);
    SeasonalRainfall rainfall;
    EXPECT_THROW(YAML::convert<SeasonalRainfall*>::decode(YAML::Load("period: 365"), &rainfall),
                 std::runtime_error);
    SeasonalPattern pattern;
    EXPECT_THROW(YAML::convert<SeasonalPattern*>::decode(YAML::Load("filename: x"), &pattern),
                 std::runtime_error);
}

TEST_F(SeasonalitySettingsStandaloneTest, RejectsMissingTopLevelSeasonalityFields) {
    SeasonalitySettings settings;
    EXPECT_THROW(YAML::convert<SeasonalitySettings>::decode(YAML::Load("{}"), settings),
                 std::runtime_error);

    const auto enabled_without_model = YAML::Load("enable: true\nmode: equation\n");
    EXPECT_THROW(YAML::convert<SeasonalitySettings>::decode(enabled_without_model, settings),
                 std::runtime_error);
}

TEST_F(SeasonalitySettingsStandaloneTest, ReturnsDefaultFactorWhenDisabledOrModeIsUnknown) {
    SeasonalitySettings settings;
    const auto today = date::sys_days{date::year{2024} / 1 / 1};

    EXPECT_DOUBLE_EQ(settings.get_seasonal_factor(today, 0), 1.0);
    settings.set_enable(true);
    settings.set_mode("unknown");
    EXPECT_DOUBLE_EQ(settings.get_seasonal_factor(today, 0), 1.0);
    EXPECT_THROW(settings.process_config_using_number_of_locations(nullptr, 0), std::runtime_error);
}

TEST_F(SeasonalitySettingsStandaloneTest, DelegatesPatternModeFactorLookup) {
    class FixedPattern final : public SeasonalPattern {
    public:
        double get_seasonal_factor(const date::sys_days &, const int &) override { return 0.75; }
    };

    SeasonalitySettings settings;
    settings.set_enable(true);
    settings.set_mode("pattern");
    settings.set_seasonal_pattern(std::make_unique<FixedPattern>());
    EXPECT_DOUBLE_EQ(settings.get_seasonal_factor(date::sys_days{date::year{2024} / 1 / 1}, 0),
                     0.75);
}

TEST_F(SeasonalitySettingsStandaloneTest, BuildsAndDelegatesEquationAndRainfallModes) {
    const auto today = date::sys_days{date::year{2024} / 1 / 1};
    constexpr double expected_rainfall_factor = 0.5;
    test_fixtures::create_test_rainfall_file(
        "test_rainfall.csv", 365, expected_rainfall_factor);

    SeasonalitySettings equation_settings;
    auto equation = std::make_unique<SeasonalEquation>();
    equation->set_base({2.0});
    equation->set_A({0.0});
    equation->set_B({1.0});
    equation->set_phi({1});
    equation->set_raster_base({2.0});
    equation->set_raster_A({0.0});
    equation->set_raster_B({1.0});
    equation->set_raster_phi({1});
    equation_settings.set_enable(true);
    equation_settings.set_mode("equation");
    equation_settings.set_seasonal_equation(std::move(equation));
    EXPECT_NO_THROW(equation_settings.process_config_using_number_of_locations(nullptr, 1));
    EXPECT_DOUBLE_EQ(equation_settings.get_seasonal_factor(today, 0), 2.0);

    SeasonalitySettings rainfall_settings;
    auto rainfall = std::make_unique<SeasonalRainfall>();
    rainfall->set_filename("test_rainfall.csv");
    rainfall->set_period(365);
    rainfall_settings.set_enable(true);
    rainfall_settings.set_mode("rainfall");
    rainfall_settings.set_seasonal_rainfall(std::move(rainfall));
    EXPECT_NO_THROW(rainfall_settings.process_config_using_number_of_locations(nullptr, 0));
    EXPECT_DOUBLE_EQ(
        rainfall_settings.get_seasonal_factor(today, 0), expected_rainfall_factor);
}

TEST_F(SeasonalitySettingsStandaloneTest, EncodesIndividualSeasonalityModels) {
    SeasonalEquation equation;
    equation.set_raster(true);
    equation.set_raster_base({2.0});
    equation.set_raster_A({0.1});
    equation.set_raster_B({0.2});
    equation.set_raster_phi({3});
    const auto equation_node = YAML::convert<SeasonalEquation*>::encode(&equation);
    EXPECT_TRUE(equation_node["raster"].as<bool>());
    EXPECT_EQ(equation_node["base"].as<std::vector<double>>(), std::vector<double>{2.0});

    SeasonalRainfall rainfall;
    rainfall.set_filename("rain.csv");
    rainfall.set_period(12);
    const auto rainfall_node = YAML::convert<SeasonalRainfall*>::encode(&rainfall);
    EXPECT_EQ(rainfall_node["filename"].as<std::string>(), "rain.csv");
    EXPECT_EQ(rainfall_node["period"].as<int>(), 12);

    SeasonalPattern pattern;
    pattern.set_filename("pattern.csv");
    pattern.set_period(365);
    pattern.set_admin_level("district");
    const auto pattern_node = YAML::convert<SeasonalPattern*>::encode(&pattern);
    EXPECT_EQ(pattern_node["filename"].as<std::string>(), "pattern.csv");
    EXPECT_EQ(pattern_node["period"].as<int>(), 365);
    EXPECT_EQ(pattern_node["admin_level"].as<std::string>(), "district");
}
