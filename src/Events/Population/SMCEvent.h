#pragma once

#include "Events/Event.h"
#include <vector>

/**
 * @brief Event that represents a single round of Seasonal Malaria Chemoprevention (SMC)
 *
 * This event handles the administration of drugs to a targeted fraction of the population
 * over a specified number of days.
 */
class SMCEvent : public WorldEvent {
private:
    std::vector<double> fraction_population_targeted;
    int days_to_complete_all_treatments{14};
    
    std::vector<int> districts;
    std::vector<int> age_range{0, 120};  // Default age range from 0 to 120 years
    int smc_year;
    int smc_month;
    
    
    void do_execute() override;

public:
    // Disallow copy
    SMCEvent(const SMCEvent&) = delete;
    SMCEvent& operator=(const SMCEvent&) = delete;

    // Disallow move
    SMCEvent(SMCEvent&&) = delete;
    SMCEvent& operator=(SMCEvent&&) = delete;

    /**
     * @brief Constructs a new Single Round SMC Event
     * @param at_time The time at which the event should execute (-1 for immediate execution)
     */
    explicit SMCEvent(const int& at_time = -1);

    /**
     * @brief Gets the targeted population fractions
     * @return The vector of population fractions targeted for treatment
     */
    [[nodiscard]] const std::vector<double>& get_fraction_population_targeted() const { return fraction_population_targeted; }

    /**
     * @brief Sets the targeted population fractions
     * @param fractions The vector of population fractions to target
     */
    void set_fraction_population_targeted(const std::vector<double>& fractions) { fraction_population_targeted = fractions; }

    /**
     * @brief Adds a single population fraction to the target vector
     * @param fraction The fraction to add
     */
    void add_fraction_population_targeted(double fraction) { fraction_population_targeted.push_back(fraction); }

    /**
     * @brief Gets the number of days to complete all treatments
     * @return The number of days
     */
    [[nodiscard]] int get_days_to_complete() const { return days_to_complete_all_treatments; }

    /**
     * @brief Sets the number of days to complete all treatments
     * @param days The number of days
     */
    void set_days_to_complete(const int days) { days_to_complete_all_treatments = days; }

    /**
     * @brief Gets the districts targeted for SMC
     * @return The vector of district IDs
     */
    [[nodiscard]] const std::vector<int>& get_districts() const { return districts; }

    /**
     * @brief Sets the districts targeted for SMC
     * @param district_ids The vector of district IDs
     */
    void set_districts(const std::vector<int>& district_ids) { districts = district_ids; }

    /**
     * @brief Gets the age range targeted for SMC
     * @return The vector containing the minimum and maximum ages
     */
    [[nodiscard]] const std::vector<int>& get_age_range() const { return age_range; }

    /**
     * @brief Sets the age range targeted for SMC
     * @param ages The vector containing the minimum and maximum ages
     */
    void set_age_range(const std::vector<int>& ages) { age_range = ages;}

    /**
     * @brief Gets the year for SMC administration
     * @return The year
     */
    [[nodiscard]] int get_smc_year() const { return smc_year; }

    /**
     * @brief Sets the year for SMC administration
     * @param year The year
     */
    void set_smc_year(const int year) { smc_year = year; }

    /**
     * @brief Gets the month for SMC administration
     * @return The month
     */
    [[nodiscard]] int get_smc_month() const { return smc_month; }

    /**
     * @brief Sets the month for SMC administration
     * @param month The month
     */
    void set_smc_month(const int month) { smc_month = month; }

    /**
     * @brief Gets the name of the event
     * @return The event name
     */
    static constexpr std::string_view EVENT_NAME{"SMC"};
    [[nodiscard]] std::string_view name() const noexcept override {
    return EVENT_NAME;
    }
};
