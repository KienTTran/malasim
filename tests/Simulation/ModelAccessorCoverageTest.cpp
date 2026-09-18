#include <gtest/gtest.h>

#include "Simulation/Model.h"

TEST(ModelAccessorCoverageTest, ClearsOptionalSubsystemsThroughPublicSetters) {
  Model::set_genotype_db(nullptr);
  Model::set_mosquito(nullptr);
  Model::set_progress_to_clinical_update_function(nullptr);
  Model::set_having_drug_update_function(nullptr);
  Model::set_immunity_clearance_update_function(nullptr);
  Model::set_clinical_update_function(nullptr);

  EXPECT_EQ(Model::get_genotype_db(), nullptr);
  EXPECT_EQ(Model::get_mosquito(), nullptr);
  EXPECT_EQ(Model::progress_to_clinical_update_function(), nullptr);
  EXPECT_EQ(Model::having_drug_update_function(), nullptr);
  EXPECT_EQ(Model::immunity_clearance_update_function(), nullptr);
  EXPECT_EQ(Model::clinical_update_function(), nullptr);

  Model::get_instance()->release();
}
