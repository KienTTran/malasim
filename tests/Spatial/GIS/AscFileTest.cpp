#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>

#include "Spatial/GIS/AscFile.h"

TEST(AscFileTest, ReportsMissingAndConflictingHeaderFields) {
  AscFile empty;
  const auto missing = AscFileManager::check_asc_file(&empty);
  EXPECT_NE(missing.find("number of columns is not set"), std::string::npos);
  EXPECT_NE(missing.find("number of rows is not set"), std::string::npos);
  EXPECT_NE(missing.find("cell size is not set"), std::string::npos);
  EXPECT_NE(missing.find("no location provided"), std::string::npos);

  AscFile conflicting;
  conflicting.ncols = 1;
  conflicting.nrows = 1;
  conflicting.cellsize = 1;
  conflicting.xllcenter = 0;
  conflicting.yllcenter = 0;
  conflicting.xllcorner = 0;
  conflicting.yllcorner = 0;
  EXPECT_NE(AscFileManager::check_asc_file(&conflicting).find("conflicting raster coordinates"),
            std::string::npos);
}

TEST(AscFileTest, ReadsCenterCoordinateRasterAndRoundTripsWrittenData) {
  const std::string filename = "test_center.asc";
  {
    std::ofstream file(filename);
    file << "NCOLS 2\nNROWS 1\nXLLCENTER 10\nYLLCENTER 20\nCELLSIZE 5\nNODATA_VALUE -9\n1.25 2.5\n";
  }
  auto raster = AscFileManager::read(filename);
  ASSERT_EQ(raster->data.size(), 1U);
  ASSERT_EQ(raster->data[0].size(), 2U);
  EXPECT_DOUBLE_EQ(raster->xllcenter, 10.0);
  EXPECT_FLOAT_EQ(raster->data[0][1], 2.5F);

  AscFile output = *raster;
  output.xllcenter = AscFile::NOT_SET;
  output.yllcenter = AscFile::NOT_SET;
  output.xllcorner = 0;
  output.yllcorner = 0;
  const std::string roundtrip = "test_roundtrip.asc";
  AscFileManager::write(&output, roundtrip);
  auto reread = AscFileManager::read(roundtrip);
  EXPECT_EQ(reread->ncols, 2);
  EXPECT_FLOAT_EQ(reread->data[0][0], 1.25F);
  std::remove(filename.c_str());
  std::remove(roundtrip.c_str());
}

TEST(AscFileTest, RejectsTruncatedRasterData) {
  const std::string filename = "test_truncated.asc";
  {
    std::ofstream file(filename);
    file << "ncols 2\nnrows 1\nxllcorner 0\nyllcorner 0\ncellsize 1\nNODATA_value -9\n1\n";
  }
  EXPECT_THROW(AscFileManager::read(filename), std::runtime_error);
  std::remove(filename.c_str());
}
