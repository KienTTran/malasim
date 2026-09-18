#include <gtest/gtest.h>

#include "Parasites/Genotype.h"
#include "Population/ClonalParasitePopulation.h"
#include "Population/Person/Person.h"
#include "Population/SingleHostClonalParasitePopulations.h"

TEST(SingleHostClonalParasitePopulationsEdgeCoverageTest, RejectsInvalidRemovalIndexAndMetadata) {
  Person person;
  SingleHostClonalParasitePopulations populations(&person);
  Genotype genotype("abcdef");
  auto parasite = std::make_unique<ClonalParasitePopulation>(&genotype);
  populations.add(std::move(parasite));

  EXPECT_THROW(populations.remove(1), std::out_of_range);
  populations[0]->set_index(99);
  EXPECT_THROW(populations.remove(0), std::runtime_error);
}

TEST(SingleHostClonalParasitePopulationsEdgeCoverageTest, RejectsNullDrugCollection) {
  Person person;
  SingleHostClonalParasitePopulations populations(&person);
  EXPECT_THROW(populations.update_with_drug_effects(nullptr), std::invalid_argument);
  EXPECT_THROW(populations.update_with_drug_effects_and_clear_cured(nullptr, -1.0),
               std::invalid_argument);
}
