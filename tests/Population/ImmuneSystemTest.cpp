#include "gtest/gtest.h"
#include "Population/ImmuneSystem/ImmuneSystem.h"
#include "Population/ImmuneSystem/ImmuneComponent.h"
#include "Population/ImmuneSystem/InfantImmuneComponent.h"
#include "Population/Person/Person.h"
#include <memory>
#include "Simulation/Model.h"
#include "Utils/Cli.h"
#include "MDC/ModelDataCollector.h"

// Dummy ImmuneComponent for full control
class DummyImmuneComponent : public ImmuneComponent {
public:
    double value = 0.0;
    DummyImmuneComponent(ImmuneSystem* immune_system = nullptr) : ImmuneComponent(immune_system) {}
    double get_current_value() override { return value; }
    double get_decay_rate(const int &age) const override { return 0.1; }
    double get_acquire_rate(const int &age) const override { return 0.2; }
    void update() override{};
    void draw_random_immune() override { /* can be overridden in derived */ }
};

TEST(ImmuneSystemTest, ConstructionAndSetters) {
    ImmuneSystem immune;
    EXPECT_EQ(immune.person(), nullptr);
    Person person;
    immune.set_person(&person);
    EXPECT_EQ(immune.person(), &person);
}

TEST(ImmuneSystemTest, SetAndGetImmuneComponent) {
    ImmuneSystem immune;
    auto component = std::make_unique<DummyImmuneComponent>(&immune);
    auto* ptr = component.get();
    immune.set_immune_component(std::move(component));
    EXPECT_EQ(immune.immune_component(), ptr);
}

TEST(ImmuneSystemTest, SetLatestImmuneValueAndGetCurrent) {
    ImmuneSystem immune;
    auto component = std::make_unique<DummyImmuneComponent>(&immune);
    auto* ptr = component.get();
    immune.set_immune_component(std::move(component));
    immune.set_latest_immune_value(0.5);
    ptr->value = 0.5;
    EXPECT_DOUBLE_EQ(immune.get_current_value(), 0.5);
}

TEST(ImmuneSystemTest, IncreaseFlag) {
    ImmuneSystem immune;
    EXPECT_FALSE(immune.increase());
}

TEST(ImmuneSystemTest, DrawRandomImmuneDelegates) {
    ImmuneSystem immune;
    struct DrawImmuneComponent : public DummyImmuneComponent {
        bool called = false;
        DrawImmuneComponent(ImmuneSystem* immune_system = nullptr) : DummyImmuneComponent(immune_system) {}
        void draw_random_immune() override { called = true; }
    };
    auto component = std::make_unique<DrawImmuneComponent>(&immune);
    auto* ptr = component.get();
    immune.set_immune_component(std::move(component));
    immune.draw_random_immune();
    EXPECT_TRUE(ptr->called);
}

TEST(ImmuneSystemTest, UpdateCallsComponentUpdate) {
    ImmuneSystem immune;
    struct FlagImmuneComponent : public DummyImmuneComponent {
        bool updated = false;
        FlagImmuneComponent(ImmuneSystem* immune_system = nullptr) : DummyImmuneComponent(immune_system) {}
        void update() override { updated = true; }
    };
    auto component = std::make_unique<FlagImmuneComponent>(&immune);
    auto* ptr = component.get();
    immune.set_immune_component(std::move(component));
    immune.update();
    EXPECT_TRUE(ptr->updated);
}

TEST(ImmuneSystemTest, PercentageDecidingToNotSeekTreatment) {
    // Set up model with config
    utils::Cli::get_instance().set_input_path("sample_inputs/input.yml");
    ASSERT_TRUE(Model::get_instance()->initialize());

    // Create a person with age >= 25 (the threshold in config)
    Person person;
    person.set_age(30);  // Age 30 > 25
    person.set_location(0);

    // Create immune system
    ImmuneSystem immune;
    immune.set_person(&person);

    // Set up immune component
    auto component = std::make_unique<DummyImmuneComponent>(&immune);
    component->value = 0.5;  // Set immune value
    immune.set_immune_component(std::move(component));

    // Enable MDC recording
    Model::get_mdc()->set_recording(true);

    // Run the test multiple times to check statistics
    const int num_trials = 100;
    int not_seeking_treatment_count = 0;

    for (int i = 0; i < num_trials; ++i) {
        double prob = immune.get_clinical_progression_probability();
        if (prob == 0.0) {
            not_seeking_treatment_count++;
        }
    }

    // Check that the count is approximately 50% (with some tolerance for randomness)
    // The config has percentage: 0.5, so we expect around 50 out of 100
    const double expected_percentage = 0.5;
    const int expected_count = static_cast<int>(num_trials * expected_percentage);
    const int tolerance = 20;  // Allow some variance due to randomness

    EXPECT_NEAR(not_seeking_treatment_count, expected_count, tolerance);

    // Determine which MDC bucket should have been incremented based on active configuration
    const auto& epi = Model::get_config()->get_epidemiological_parameters();
    int bucket_idx = 0;
    // If new age-based config is enabled, compute index like runtime logic
    if (epi.get_age_based_probability_of_seeking_treatment().enabled() &&
        !epi.get_age_based_probability_of_seeking_treatment().get_ages().empty()) {
        const int age = person.get_age();
        const auto& ages = epi.get_age_based_probability_of_seeking_treatment().get_ages();
        if (age <= ages.front()) bucket_idx = 0;
        else if (age >= ages.back()) bucket_idx = static_cast<int>(ages.size()) - 1;
        else {
            int hi = 1;
            while (hi < static_cast<int>(ages.size()) && age >= ages[hi]) ++hi;
            bucket_idx = hi - 1;
        }
    }
    // Otherwise fallback to legacy percentage list semantics
    else {
        const auto& list = epi.get_percentage_deciding_to_not_seek_treatment();
        if (!list.empty()) {
            const int age = person.get_age();
            // Find the last entry whose age <= person's age
            bucket_idx = 0;
            for (size_t i = 0; i < list.size(); ++i) {
                if (age >= list[i].get_age()) bucket_idx = static_cast<int>(i);
            }
        }
    }

    // Also check that MDC recorded the events in the expected bucket
    EXPECT_EQ(Model::get_mdc()->monthly_number_of_not_seeking_treatment_by_location_index()[0][bucket_idx], not_seeking_treatment_count);

    // Clean up
    Model::get_instance()->release();
}

// get_parasite_size_after_t_days and get_clinical_progression_probability require more mocks or integration
