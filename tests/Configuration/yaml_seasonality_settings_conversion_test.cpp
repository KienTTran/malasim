#include <gtest/gtest.h>
#include <yaml-cpp/yaml.h>
#include "Configuration/SeasonalitySettings.h"

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
