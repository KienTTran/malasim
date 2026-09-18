#include <fstream>
#include <filesystem>
#include <string>

#include <gtest/gtest.h>

#include "Events/Population/IntroduceMutantEvent.hxx"
#include "Events/Population/IntroduceMutantRasterEvent.hxx"
#include "Events/Population/PopulationEventBuilder.h"
#include "Simulation/Model.h"
#include "apps/malasim/MaSimAppInput.h"
#include "fixtures/TestFileGenerators.h"

std::vector<int> get_locations_from_raster(const std::string &filename);

class IntroduceMutantEventBuilderCoverageTest : public ::testing::Test {
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
    std::filesystem::remove("mutant_builder_valid.asc");
    std::filesystem::remove("mutant_builder_invalid.asc");
    std::filesystem::remove("mutant_builder_overflow.asc");
    std::filesystem::remove("mutant_builder_coverage.asc");
    test_fixtures::cleanup_test_files();
  }
};

TEST_F(IntroduceMutantEventBuilderCoverageTest, LogsAndSkipsMultiCharacterAlleles) {
  const auto events = PopulationEventBuilder::build_introduce_mutant_event(
      YAML::Load(R"(
        - day: 2024/01/10
          unit_id: 0
          fraction: 0.25
          alleles:
            - chromosome: 1
              locus: 2
              allele: AA
      )"),
      Model::get_config(), "district");
  ASSERT_EQ(events.size(), 1U);
  EXPECT_EQ(events.front()->name(), IntroduceMutantEvent::EVENT_NAME);
}

TEST_F(IntroduceMutantEventBuilderCoverageTest, LogsAndSkipsMultiCharacterRasterAlleles) {
  const std::string raster_name = "mutant_builder_coverage.asc";
  std::ofstream raster(raster_name);
  raster << "ncols 8\n"
         << "nrows 1\n"
         << "xllcorner 0\n"
         << "yllcorner 0\n"
         << "cellsize 1\n"
         << "NODATA_value -9999\n"
         << "1 0 1 0 1 0 1 0\n";
  raster.close();
  const auto events = PopulationEventBuilder::build_introduce_mutant_raster_event(
      YAML::Load(R"(
        - date: 2024/01/11
          raster: mutant_builder_coverage.asc
          fraction: 0.5
          alleles:
            - chromosome: 1
              locus: 3
              allele: TT
      )"),
      Model::get_config());
  ASSERT_EQ(events.size(), 1U);
  EXPECT_EQ(events.front()->name(), IntroduceMutantRasterEvent::EVENT_NAME);
}

TEST_F(IntroduceMutantEventBuilderCoverageTest, ParsesBinaryMutationRasterLocations) {
  std::ofstream raster("mutant_builder_valid.asc");
  raster << "ncols 8\nnrows 1\nxllcorner 0\nyllcorner 0\ncellsize 1\n"
            "NODATA_value -9999\n1 0 1 0 1 0 1 0\n";
  raster.close();

  const auto locations = get_locations_from_raster("mutant_builder_valid.asc");
  EXPECT_EQ(locations, std::vector<int>({0, 2, 4, 6}));
}

TEST_F(IntroduceMutantEventBuilderCoverageTest, RejectsInvalidMutationRasterValuesAndAlignment) {
  std::ofstream invalid("mutant_builder_invalid.asc");
  invalid << "ncols 8\nnrows 1\nxllcorner 0\nyllcorner 0\ncellsize 1\n"
             "NODATA_value -9999\n1 0 2 0 1 0 1 0\n";
  invalid.close();
  EXPECT_THROW(get_locations_from_raster("mutant_builder_invalid.asc"), std::runtime_error);

  std::ofstream overflow("mutant_builder_overflow.asc");
  overflow << "ncols 9\nnrows 1\nxllcorner 0\nyllcorner 0\ncellsize 1\n"
              "NODATA_value -9999\n1 0 1 0 1 0 1 0 1\n";
  overflow.close();
  EXPECT_THROW(get_locations_from_raster("mutant_builder_overflow.asc"), std::runtime_error);
}

TEST_F(IntroduceMutantEventBuilderCoverageTest, RejectsNegativeMutationUnitId) {
  EXPECT_THROW(PopulationEventBuilder::build_introduce_mutant_event(
                   YAML::Load(R"(
                     - day: 2024/01/10
                       unit_id: -1
                       fraction: 0.25
                       alleles: []
                   )"),
                   Model::get_config(), "district"),
               std::invalid_argument);
}

TEST_F(IntroduceMutantEventBuilderCoverageTest, RejectsUnitIdAtAdminLevelCount) {
  const auto unit_count = Model::get_spatial_data()->get_unit_count(
      Model::get_spatial_data()->get_admin_level_id("district"));
  EXPECT_THROW(PopulationEventBuilder::build_introduce_mutant_event(
                   YAML::Load("- day: 2024/01/10\n  unit_id: " + std::to_string(unit_count) +
                              "\n  fraction: 0.25\n  alleles: []"),
                   Model::get_config(), "district"),
               std::invalid_argument);
}
