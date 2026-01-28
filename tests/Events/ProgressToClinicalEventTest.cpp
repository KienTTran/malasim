#include <gtest/gtest.h>

#include "Events/ProgressToClinicalEvent.h"
#include "Population/Person/Person.h"
#include "Simulation/Model.h"
#include "Utils/Cli.h"
#include "fixtures/TestFileGenerators.h"
#include "Treatment/ITreatmentCoverageModel.h"
#include "Utils/Random.h"

using namespace ::testing;

// Minimal deterministic Random stub that returns a fixed value for random_flat
class TestRandomFixed : public utils::Random {
public:
    explicit TestRandomFixed(double fixed_value) : utils::Random(nullptr, 42), fixed_(fixed_value) {}
    double random_flat(double from, double to) override { (void)from; (void)to; return fixed_; }
private:
    double fixed_;
};

// Constant treatment coverage model returning a fixed probability
struct ConstTCM : public ITreatmentCoverageModel {
    explicit ConstTCM(double p) { p_treatment_under_5.clear(); p_treatment_over_5.clear(); value = p; }
    double get_probability_to_be_treated(const int &location, const int &age) override { (void)location; (void)age; return value; }
    void monthly_update() override {}
    double value{0.0};
};

TEST(ProgressToClinicalEventDeterministicTest, ShouldReceiveTreatmentRespectsAgeBasedModifierAndEnableFlag) {
    // Prepare a config enabling the age-based seeking with known breakpoints
    test_fixtures::setup_test_environment("test_input.yml", [](YAML::Node &cfg){
        if (!cfg["epidemiological_parameters"]) cfg["epidemiological_parameters"] = YAML::Node(YAML::NodeType::Map);
        auto n = cfg["epidemiological_parameters"]["age_based_probability_of_seeking_treatment"];
        n["enable"] = true;
        n["type"] = "power";
        n["power"]["base"] = 0.9;
        n["power"]["exponent_source"] = "index";
        n["ages"] = std::vector<int>{0,5,10,15,20,30,40};
    });

    // Initialize model
    utils::Cli::get_instance().set_input_path("test_input.yml");
    ASSERT_TRUE(Model::get_instance()->initialize());

    // Replace treatment coverage with a constant value (base_p)
    Model::get_instance()->set_treatment_coverage(std::make_unique<ConstTCM>(0.6));

    // Replace global RNG with a deterministic stub returning 0.55
    Model::set_random(std::make_unique<TestRandomFixed>(0.55));

    // Create and initialize a Person
    auto person = std::make_unique<Person>();
    person->initialize();
    person->set_location(0);

    // Age 2 -> modifier = 0.9^0 = 1.0 -> effective_p = 0.6 -> random 0.55 <= 0.6 -> expect true
    person->set_age(2);
    EXPECT_TRUE(ProgressToClinicalEvent::should_receive_treatment(person.get()));

    // Age 12 -> modifier = 0.9^2 = 0.81 -> effective_p = 0.6 * 0.81 = 0.486 -> random 0.55 > 0.486 -> expect false
    person->set_age(12);
    EXPECT_FALSE(ProgressToClinicalEvent::should_receive_treatment(person.get()));

    // Now disable the age-based modifier in config by reinitializing model with enable=false
    Model::get_instance()->release();
    test_fixtures::setup_test_environment("test_input.yml", [](YAML::Node &cfg){
        if (!cfg["epidemiological_parameters"]) cfg["epidemiological_parameters"] = YAML::Node(YAML::NodeType::Map);
        auto n = cfg["epidemiological_parameters"]["age_based_probability_of_seeking_treatment"];
        n["enable"] = false;
        n["type"] = "power";
        n["power"]["base"] = 0.9;
        n["power"]["exponent_source"] = "index";
        n["ages"] = std::vector<int>{0,5,10,15,20,30,40};
    });

    utils::Cli::get_instance().set_input_path("test_input.yml");
    ASSERT_TRUE(Model::get_instance()->initialize());

    // Reinstall deterministic RNG and constant TCM
    Model::set_random(std::make_unique<TestRandomFixed>(0.55));
    Model::get_instance()->set_treatment_coverage(std::make_unique<ConstTCM>(0.6));

    // Recreate person
    person.reset(new Person());
    person->initialize();
    person->set_location(0);

    // Age 12 with disabled modifier -> modifier = 1.0 -> effective_p = 0.6 -> random 0.55 <= 0.6 -> expect true
    person->set_age(12);
    EXPECT_TRUE(ProgressToClinicalEvent::should_receive_treatment(person.get()));

    Model::get_instance()->release();
    test_fixtures::cleanup_test_files();
}
