/*
 * AscFile.cpp
 *
 * Implementation of defined functions.
 */
#include "AscFile.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>

// Check that the contents fo the ASC file are correct. Returns TRUE if any
// errors are found, which are enumerated in the string provided.
std::string AscFileManager::check_asc_file(const AscFile* file) {
  std::string errors;
  // Check values that must be set
  if (file->ncols == AscFile::NOT_SET) { errors += "number of columns is not set;"; }
  if (file->nrows == AscFile::NOT_SET) { errors += "number of rows is not set;"; }
  if (file->cellsize == AscFile::NOT_SET) { errors += "cell size is not set;"; }

  // The coordinate to fix the raster should either be the lower-left, or the
  // center
  if (file->xllcenter == AscFile::NOT_SET && file->yllcenter == AscFile::NOT_SET
      && file->xllcorner == AscFile::NOT_SET && file->yllcorner == AscFile::NOT_SET) {
    errors += "no location provided for raster coordinate;";
  }
  if (file->xllcenter != AscFile::NOT_SET && file->yllcenter != AscFile::NOT_SET
      && file->xllcorner != AscFile::NOT_SET && file->yllcorner != AscFile::NOT_SET) {
    errors += "conflicting raster coordinates;";
  }

  // Return true if errors were found
  return errors;
}

// Read the indicated file from disk, caller is responsible for checking if
// data is integer or floating point.
std::unique_ptr<AscFile> AscFileManager::read(const std::string &file_name) {
  // Treat the struct as POD
  auto results = std::make_unique<AscFile>();

  // Open the file and verify it
  std::string field;
  std::string value;
  std::ifstream in(file_name);

  if (!in.good()) { throw std::runtime_error("Error opening ASC file: " + file_name); }
  if (in.peek() == std::ifstream::traits_type::eof()) {
    throw std::runtime_error("EOF encountered at start of the file: " + file_name);
  }

  // Read the first six lines of the header
  for (auto ndx = 0; ndx < 6; ndx++) {
    // Read the field and value, cast to upper case
    in >> field >> value;
    std::ranges::transform(field, field.begin(), ::toupper);

    // Store the values
    if (field == "NCOLS") {
      results->ncols = std::stoi(value);
    } else if (field == "NROWS") {
      results->nrows = std::stoi(value);
    } else if (field == "XLLCENTER") {
      results->xllcenter = std::stod(value);
    } else if (field == "YLLCENTER") {
      results->yllcenter = std::stod(value);
    } else if (field == "XLLCORNER") {
      results->xllcorner = std::stod(value);
    } else if (field == "YLLCORNER") {
      results->yllcorner = std::stod(value);
    } else if (field == "CELLSIZE") {
      results->cellsize = std::stod(value);
    } else if (field == "NODATA_VALUE") {
      results->nodata_value = std::stod(value);
    }
  }

  // Check the header to make sure it is valid
  std::string errors = check_asc_file(results.get());
  if (!errors.empty()) { throw std::runtime_error(errors); }

  // Allocate the memory and read the remainder of the actual raster data
  // Allocate the memory using vectors
  results->data.resize(results->nrows);
  for (auto &row : results->data) { row.resize(results->ncols); }

  // Remainder of the file is the actual raster data
  for (auto ndx = 0; ndx < results->nrows; ndx++) {
    for (auto ndy = 0; ndy < results->ncols; ndy++) {
      if (!(in >> value)) {
        if (in.eof()) {
          throw std::runtime_error("EOF encountered while reading data from: " + file_name);
        }
        throw std::runtime_error("Invalid raster value in ASC file: " + file_name);
      }
      results->data[ndx][ndy] = std::stof(value);
    }
  }
  // Clean-up and return
  in.close();
  return results;
}

// Write the contents of the AscFile to disk.
void AscFileManager::write(AscFile* file, const std::string &file_name) {
  // Open the file for writing
  std::ofstream out(file_name);

  // Write the header
  out << std::setprecision(16) << std::left << std::setw(HEADER_WIDTH) << "ncols" << file->ncols
      << AscFile::CRLF << std::setw(HEADER_WIDTH) << "nrows" << file->nrows << AscFile::CRLF
      << std::setw(HEADER_WIDTH) << "xllcorner" << file->xllcorner << AscFile::CRLF
      << std::setw(HEADER_WIDTH) << "yllcorner" << file->yllcorner << AscFile::CRLF
      << std::setw(HEADER_WIDTH) << "cellsize" << file->cellsize << AscFile::CRLF
      << std::setw(HEADER_WIDTH) << "NODATA_value" << file->nodata_value << AscFile::CRLF;

  // Write the raster data
  for (auto ndx = 0; ndx < file->nrows; ndx++) {
    for (auto ndy = 0; ndy < file->ncols; ndy++) {
      out << std::setprecision(8) << file->data[ndx][ndy] << " ";
    }
    out << AscFile::CRLF;
  }

  // Clean-up
  out.close();
}
