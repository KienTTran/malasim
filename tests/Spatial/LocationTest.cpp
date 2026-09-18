#include <gtest/gtest.h>

#include <sstream>

#include "Spatial/Location/Location.h"

TEST(LocationTest, StreamsAllLocationFields) {
  Spatial::Location location;
  location.id = 7;
  location.population_size = 42;
  location.beta = 0.3;
  location.coordinate.latitude = 1.5;
  location.coordinate.longitude = 2.5;
  location.age_distribution = {0.2, 0.8};
  location.p_treatment_under_5 = 0.1;
  location.p_treatment_over_5 = 0.4;
  location.mosquito_size = 12;
  location.mosquito_ifr = 0.25;

  std::ostringstream output;
  output << location;
  const auto text = output.str();
  EXPECT_NE(text.find("id: 7"), std::string::npos);
  EXPECT_NE(text.find("populationSize: 42"), std::string::npos);
  EXPECT_NE(text.find("age_distribution: [0.2,0.8,"), std::string::npos);
  EXPECT_NE(text.find("mosquito_ifr: 0.25"), std::string::npos);
}
