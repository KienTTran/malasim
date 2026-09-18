#include <fstream>

#include <gtest/gtest.h>

#include "Events/Population/IntroduceMutantEvent.hxx"
#include "Events/Population/IntroduceMutantRasterEvent.hxx"
#include "Events/Population/PopulationEventBuilder.h"
#include "Simulation/Model.h"
#include "apps/malasim/MaSimAppInput.h"
#include "fixtures/TestFileGenerators.h"

class PopulationEventBuilderMutantTest : public ::testing::Test {
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

TEST_F(PopulationEventBuilderMutantTest, BuildsMutantEventsFromUnitsAndRasters) {
  const auto unit_events = PopulationEventBuilder::build_introduce_mutant_event(
      YAML::Load(R"(
        - day: 2024/01/10
          unit_id: 0
          fraction: 0.25
          alleles:
            - chromosome: 1
              locus: 2
              allele: A
      )"),
      Model::get_config(), "district");
  ASSERT_EQ(unit_events.size(), 1U);
  EXPECT_EQ(unit_events[0]->name(), IntroduceMutantEvent::EVENT_NAME);
  unit_events[0]->set_executable(true);
  EXPECT_NO_THROW(unit_events[0]->execute());

  const std::string raster_name = "mutant_event.asc";
  std::ofstream raster(raster_name);
  raster << "ncols 8\n"
         << "nrows 1\n"
         << "xllcorner 0\n"
         << "yllcorner 0\n"
         << "cellsize 1\n"
         << "NODATA_value -9999\n"
         << "1 0 1 0 1 0 1 0\n";
  raster.close();
  const auto raster_events = PopulationEventBuilder::build_introduce_mutant_raster_event(
      YAML::Load(R"(
        - date: 2024/01/11
          raster: mutant_event.asc
          fraction: 0.5
          alleles:
            - chromosome: 1
              locus: 3
              allele: T
      )"),
      Model::get_config());
  ASSERT_EQ(raster_events.size(), 1U);
  EXPECT_EQ(raster_events[0]->name(), IntroduceMutantRasterEvent::EVENT_NAME);
  raster_events[0]->set_executable(true);
  EXPECT_NO_THROW(raster_events[0]->execute());
}
