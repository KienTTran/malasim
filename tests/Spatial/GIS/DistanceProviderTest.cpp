#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <iostream>
#include <utility>
#include <vector>

#include "Configuration/SpatialSettings/SpatialSettings.h"
#include "Spatial/GIS/DistanceProvider.h"
#include "Spatial/Location/Location.h"

namespace {
std::vector<Spatial::Location> make_locations(const std::vector<std::pair<int, int>> &cells) {
  std::vector<Spatial::Location> locations(cells.size());
  for (std::size_t index = 0; index < cells.size(); ++index) {
    locations[index].coordinate = {.latitude = static_cast<float>(cells[index].first),
                                   .longitude = static_cast<float>(cells[index].second)};
  }
  return locations;
}

void expect_exact_match(const std::vector<Spatial::Location> &locations, float cell_size) {
  const DenseGridDistanceProvider dense(locations, cell_size);
  const GridLutDistanceProvider lut(locations, cell_size);

  ASSERT_EQ(dense.size(), locations.size());
  ASSERT_EQ(lut.size(), locations.size());
  for (std::size_t from = 0; from < locations.size(); ++from) {
    for (std::size_t to = 0; to < locations.size(); ++to) {
      EXPECT_DOUBLE_EQ(dense.distance(from, to), lut.distance(from, to))
          << "from=" << from << ", to=" << to << ", cell_size=" << cell_size;
    }
  }
}
}  // namespace

TEST(DistanceProviderTest, CompleteRasterMatchesForEveryPair) {
  std::vector<std::pair<int, int>> cells;
  for (int row = 0; row < 5; ++row) {
    for (int column = 0; column < 7; ++column) { cells.emplace_back(row, column); }
  }

  expect_exact_match(make_locations(cells), 5.0F);
}

TEST(DistanceProviderTest, IrregularRasterWithNoDataGapsMatchesForEveryPair) {
  const auto locations =
      make_locations({{0, 0}, {0, 3}, {1, 1}, {2, 6}, {4, 0}, {4, 5}, {7, 2}, {9, 9}});

  expect_exact_match(locations, 2.5F);
}

TEST(DistanceProviderTest, MatchesForMultipleCellSizes) {
  const auto locations = make_locations({{0, 0}, {0, 4}, {3, 0}, {3, 4}, {11, 17}});
  constexpr std::array<float, 5> cell_sizes = {0.25F, 1.0F, 5.0F, 17.5F, 1000.0F};

  for (const float cell_size : cell_sizes) { expect_exact_match(locations, cell_size); }
}

TEST(DistanceProviderTest, DistanceIsZeroAndSymmetric) {
  const auto locations = make_locations({{1, 9}, {5, 2}, {8, 12}});
  const GridLutDistanceProvider lut(locations, 3.0F);

  for (std::size_t from = 0; from < locations.size(); ++from) {
    EXPECT_DOUBLE_EQ(lut.distance(from, from), 0.0);
    for (std::size_t to = 0; to < locations.size(); ++to) {
      EXPECT_DOUBLE_EQ(lut.distance(from, to), lut.distance(to, from));
    }
  }
}

TEST(DistanceProviderTest, HardCodedRasterBackendSelectsLut) {
  const auto locations = make_locations({{0, 0}, {1, 1}});
  auto provider = make_grid_distance_provider(locations, 5.0F);

  EXPECT_EQ(GRID_DISTANCE_BACKEND, GridDistanceBackend::LUT);
  EXPECT_NE(dynamic_cast<GridLutDistanceProvider*>(provider.get()), nullptr);
}

TEST(DistanceProviderTest, LocationBasedSettingsHaveNoProviderBeforeProcessing) {
  SpatialSettings settings;
  settings.set_mode(SpatialSettings::LOCATION_BASED_MODE);

  EXPECT_EQ(settings.get_distance_provider(), nullptr);
}

TEST(DistanceProviderTest, DenseProviderPreservesHaversineMatrixRows) {
  const std::vector<std::vector<double>> matrix = {{0.0, 1.5}, {1.5, 0.0}};
  const DenseDistanceProvider provider(matrix);

  ASSERT_EQ(provider.size(), matrix.size());
  ASSERT_NE(provider.dense_row(0), nullptr);
  EXPECT_EQ(*provider.dense_row(0), matrix[0]);
  EXPECT_DOUBLE_EQ(provider.distance(1, 0), matrix[1][0]);
  EXPECT_EQ(provider.grid_table(), nullptr);
}

TEST(DistanceProviderTest, LutStorageIsSmallerThanDenseStorage) {
  std::vector<std::pair<int, int>> cells;
  for (int row = 0; row < 10; ++row) {
    for (int column = 0; column < 12; ++column) { cells.emplace_back(row, column); }
  }
  const auto locations = make_locations(cells);
  const GridLutDistanceProvider lut(locations, 5.0F);
  const std::size_t dense_bytes = locations.size() * locations.size() * sizeof(double);

  std::cout << "LUT memory footprint: " << lut.memory_bytes() << " bytes\n";
  std::cout << "Dense memory footprint: " << dense_bytes << " bytes\n" << std::flush;

  EXPECT_LT(lut.memory_bytes(), dense_bytes);
}

TEST(DistanceProviderTest, BaseProviderOptionalViewsAreNull) {
  class MinimalProvider final : public DistanceProvider {
   public:
    double distance(std::size_t, std::size_t) const noexcept override { return 0.0; }
    std::size_t size() const noexcept override { return 0; }
  } provider;

  EXPECT_EQ(provider.dense_row(0), nullptr);
  EXPECT_EQ(provider.grid_table(), nullptr);
}

TEST(DistanceProviderTest, RejectsUnknownGridDistanceBackend) {
  const auto locations = make_locations({{0, 0}});
  EXPECT_THROW(
      static_cast<void>(
          make_grid_distance_provider(locations, 1.0F, static_cast<GridDistanceBackend>(255))),
      std::logic_error);
}

TEST(DistanceProviderTest, DenseBackendExposesRowsAndMatrix) {
  const auto locations = make_locations({{0, 0}, {2, 1}});
  auto provider = make_grid_distance_provider(locations, 2.0F, GridDistanceBackend::DENSE);
  const auto* dense = dynamic_cast<const DenseGridDistanceProvider*>(provider.get());

  ASSERT_NE(dense, nullptr);
  ASSERT_EQ(dense->matrix().size(), 2U);
  ASSERT_NE(dense->dense_row(1), nullptr);
  EXPECT_DOUBLE_EQ((*dense->dense_row(1))[0], dense->distance(1, 0));
  EXPECT_EQ(dense->grid_table(), nullptr);
}

TEST(DistanceProviderTest, LutBackendExposesPairTable) {
  const auto locations = make_locations({{0, 0}, {1, 0}});
  const GridLutDistanceProvider provider(locations, 1.0F);

  ASSERT_NE(provider.grid_table(), nullptr);
  EXPECT_EQ(provider.grid_table(), &provider.table());
}
