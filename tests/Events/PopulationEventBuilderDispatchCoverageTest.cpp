#include <gtest/gtest.h>

#include "Events/Population/PopulationEventBuilder.h"
#include "Simulation/Model.h"
#include "apps/malasim/MaSimAppInput.h"
#include "fixtures/TestFileGenerators.h"

class PopulationEventBuilderDispatchCoverageTest : public ::testing::Test {
protected:
  void SetUp() override {
    test_fixtures::setup_test_environment();
    utils::MaSimAppInput cli_input;
    cli_input.input_path = "test_input.yml";
    Model::set_cli_input(cli_input);
    ASSERT_TRUE(Model::get_instance()->initialize());
  }

  void TearDown() override {
    Model::get_instance()->release();
    test_fixtures::cleanup_test_files();
  }
};

TEST_F(PopulationEventBuilderDispatchCoverageTest, BuildsSupportedMutationAndControlEvents) {
  const auto date = "2000/1/1";

  YAML::Node v2;
  v2["name"] = "introduce_parasites_periodically_v2";
  v2["info"][0]["location"] = -1;
  v2["info"][0]["parasite_info"][0]["duration"] = 1;
  v2["info"][0]["parasite_info"][0]["number_of_cases"] = 0;
  v2["info"][0]["parasite_info"][0]["start_date"] = date;
  EXPECT_EQ(PopulationEventBuilder::build(v2).size(), Model::get_config()->number_of_locations());

  for (const auto name : {"introduce_plas2_copy_parasite", "introduce_amodiaquine_mutant",
                          "introduce_lumefantrine_mutant", "introduce_580Y_mutant",
                          "introduce_triple_mutant_to_dpm"}) {
    YAML::Node event;
    event["name"] = name;
    event["info"][0]["location"] = 0;
    event["info"][0]["fraction"] = 0.0;
    event["info"][0]["date"] = date;
    event["info"][0]["alleles"] = YAML::Load("[]");
    EXPECT_EQ(PopulationEventBuilder::build(event).size(), 1U) << name;
  }

  for (const auto name : {"turn_on_mutation", "turn_off_mutation"}) {
    YAML::Node event;
    event["name"] = name;
    event["info"][0]["date"] = date;
    if (std::string{name} == "turn_on_mutation") {
      event["info"][0]["mutation_probability"] = 0.1;
    }
    EXPECT_EQ(PopulationEventBuilder::build(event).size(), 1U) << name;
  }

  YAML::Node within_host;
  within_host["name"] = "change_within_host_induced_free_recombination";
  within_host["info"][0]["date"] = date;
  within_host["info"][0]["value"] = true;
  EXPECT_EQ(PopulationEventBuilder::build(within_host).size(), 1U);

  YAML::Node interrupted;
  interrupted["name"] = "change_interrupted_feeding_rate";
  interrupted["info"][0]["location"] = 0;
  interrupted["info"][0]["date"] = date;
  interrupted["info"][0]["interrupted_feeding_rate"] = 0.2;
  EXPECT_EQ(PopulationEventBuilder::build(interrupted).size(), 1U);
  interrupted["info"][0]["location"] = -1;
  EXPECT_TRUE(PopulationEventBuilder::build(interrupted).empty());

  YAML::Node annual_coverage;
  annual_coverage["name"] = "annual_coverage_update";
  annual_coverage["info"][0]["date"] = date;
  annual_coverage["info"][0]["rate"] = 0.1;
  EXPECT_EQ(PopulationEventBuilder::build(annual_coverage).size(), 1U);

  YAML::Node circulation;
  circulation["name"] = "change_circulation_percent";
  circulation["info"][0]["date"] = date;
  circulation["info"][0]["circulation_percent"] = 0.1;
  EXPECT_EQ(PopulationEventBuilder::build(circulation).size(), 1U);
}
