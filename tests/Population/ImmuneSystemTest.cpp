#include "gtest/gtest.h"
#include "Population/ImmuneSystem/ImmuneSystem.h"
#include "Population/ImmuneSystem/ImmuneComponent.h"
#include "Population/ImmuneSystem/InfantImmuneComponent.h"
#include "Population/Person/Person.h"
#include <memory>
#include "Configuration/Config.h"
#include "Simulation/Model.h"

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

// get_parasite_size_after_t_days and get_clinical_progression_probability require more mocks or integration

// Test treatment-linked immunity memory
TEST(ImmuneSystemTest, TreatmentImmunityBoost) {
    // Set up config
    auto config = std::make_unique<Config>();
    TreatmentImmunityConfig ti;
    ti.enable = true;
    ti.boost_on_treatment = 0.05;
    ti.max_extra_boost = 0.3;
    ti.half_life_days = 365.0;
    config->get_immune_system_parameters().set_treatment_immunity(ti);
    Model::set_config(std::move(config));

    // Set up scheduler
    auto scheduler = std::make_unique<Scheduler>();
    scheduler->set_current_time(0); // Set initial time
    Model::set_scheduler(std::move(scheduler));

    ImmuneSystem immune;
    auto component = std::make_unique<DummyImmuneComponent>(&immune);
    component->value = 0.5; // Set base immune value
    immune.set_immune_component(std::move(component));

    // Initially, effective immune should be base value
    EXPECT_DOUBLE_EQ(immune.get_effective_immune_value(), 0.5);

    // After treated clinical episode, boost should be added
    immune.on_treated_clinical_episode();
    EXPECT_DOUBLE_EQ(immune.get_effective_immune_value(), 0.55); // 0.5 + 0.05

    // Another boost should increase further
    immune.on_treated_clinical_episode();
    EXPECT_DOUBLE_EQ(immune.get_effective_immune_value(), 0.6); // 0.5 + 0.1

    // Boost to max
    for (int i = 0; i < 10; ++i) {
        immune.on_treated_clinical_episode();
    }
    EXPECT_DOUBLE_EQ(immune.get_effective_immune_value(), 0.8); // 0.5 + 0.3
}

TEST(ImmuneSystemTest, TreatmentImmunityDecay) {
    // Set up config
    auto config = std::make_unique<Config>();
    TreatmentImmunityConfig ti;
    ti.enable = true;
    ti.boost_on_treatment = 0.1;
    ti.max_extra_boost = 1.0;
    ti.half_life_days = 365.0;
    config->get_immune_system_parameters().set_treatment_immunity(ti);
    Model::set_config(std::move(config));

    // Set up scheduler
    auto scheduler = std::make_unique<Scheduler>();
    scheduler->set_current_time(0);
    Model::set_scheduler(std::move(scheduler));

    ImmuneSystem immune;
    auto component = std::make_unique<DummyImmuneComponent>(&immune);
    component->value = 0.0;
    immune.set_immune_component(std::move(component));

    // Boost
    immune.on_treated_clinical_episode();
    EXPECT_DOUBLE_EQ(immune.get_effective_immune_value(), 0.1);

    // Advance time by half life
    Model::get_scheduler()->set_current_time(365);

    // Effective immune should decay
    double expected = 0.1 * std::exp(-std::log(2.0));
    EXPECT_NEAR(immune.get_effective_immune_value(), expected, 1e-6);
}

TEST(ImmuneSystemTest, TreatmentImmunityDisabled) {
    // Config with disabled treatment immunity
    auto config = std::make_unique<Config>();
    TreatmentImmunityConfig ti;
    ti.enable = false;
    config->get_immune_system_parameters().set_treatment_immunity(ti);
    Model::set_config(std::move(config));

    // Set up scheduler
    auto scheduler = std::make_unique<Scheduler>();
    scheduler->set_current_time(0);
    Model::set_scheduler(std::move(scheduler));

    ImmuneSystem immune;
    auto component = std::make_unique<DummyImmuneComponent>(&immune);
    component->value = 0.5;
    immune.set_immune_component(std::move(component));

    // Should have no effect
    immune.on_treated_clinical_episode();
    EXPECT_DOUBLE_EQ(immune.get_effective_immune_value(), 0.5);
}
