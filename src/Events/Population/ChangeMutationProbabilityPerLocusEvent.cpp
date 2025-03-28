//
// Created by kient on 8/22/2023.
//

#include "ChangeMutationProbabilityPerLocusEvent.h"

#include "Configuration/Config.h"
#include "Utils/Helpers/StringHelpers.h"

ChangeMutationProbabilityPerLocusEvent::ChangeMutationProbabilityPerLocusEvent(const double& value,
                                                                               const int& at_time)
        : value { value } {
    time = at_time;
}
void ChangeMutationProbabilityPerLocusEvent::execute() {
    Model::get_config()->get_genotype_parameters().set_mutation_probability_per_locus(value);
    spdlog::info("{}: Change mutation probability per locus to {}",
      StringHelpers::date_as_string(date::year_month_day{scheduler->calendar_date}),value);
}