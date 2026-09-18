#include "apps/efficacy_estimator/DxGCli.h"

#include <spdlog/spdlog.h>

#include <cstddef>
#include <string>

namespace utils {

std::vector<char*> DxGCli::prepare_arguments(int argc, char** argv) {
  std::vector<char*> arguments;
  arguments.reserve(static_cast<std::size_t>(argc));
  for (int i = 0; i < argc; ++i) {
    const std::string argument = argv[i];
    if (argument == "DxG=1" || argument == "DxG=true") { continue; }
    arguments.push_back(argv[i]);
  }
  return arguments;
}

void DxGCli::add_options(CLI::App &app, Input &input) {
  app.add_flag("--DxG")->description("Run the DxG application mode");
  app.add_option("-i,--input", input.input_file, "Input filename for DxG")
      ->check(CLI::ExistingFile);
  app.add_option("-o,--output", input.output_file, "Output file path");
  app.add_option("-t,--therapies", input.therapies, "Therapy range (e.g., 0 1 2 ...)")
      ->expected(-1);
  app.add_option("--tl,--therapy_list", input.therapy_list, "List of therapies (e.g., 0 1 2 ...)")
      ->expected(-1);
  app.add_option("-g,--genotypes", input.genotypes, "List of genotypes (e.g., WT KEL1 KEL1/PLA1)")
      ->expected(-1);
  app.add_flag("--cc", input.is_crt_calibration, "Enable PfCRT EC50 calibration");
  app.add_option("--iov", input.as_iov, "AS inter-occasion variability")->default_val(-1.0);
  app.add_option("--iiv", input.as_iiv, "AS inter-individual variability")->default_val(-1.0);
  app.add_option("--as_ec50", input.as_ec50, "AS EC50 value (C580)")->default_val(-1.0);
  app.add_flag("--ee", input.is_ee_calibration, "Enable EE calibration");
  app.add_option("--nd", input.number_of_drugs_in_combination, "Number of drugs in combination")
      ->default_val(1);
  app.add_flag("--art", input.is_art, "Use ART configuration");
  app.add_option("--dose", input.dosing_days, "EE dosing days (e.g., 0 1 2 ...)")->expected(-1);
  app.add_option("--halflife", input.half_life, "EE half-life values")->expected(-1);
  app.add_option("--kmax", input.k_max, "EE kmax values")->expected(-1);
  app.add_option("--EC50", input.ec50, "EE EC50 values")->expected(-1);
  app.add_option("--slope", input.slope, "EE slope values")->expected(-1);
  app.add_option("--mda", input.mean_drug_absorption, "EE mean drug absorption values")
      ->expected(-1);
  app.add_option("--popsize", input.population_size, "Population size for EE")->default_val(10000);
  app.add_flag("--pil", input.is_print_immunity_level, "Print immunity level");
  app.add_flag("--old_format", input.is_old_format, "Print output in old format");
  app.add_flag("--recurrence-test", input.is_recurrence_test,
               "Run recurrence test mode (TMS-style per-person CSV with recrudescence tracking)");
}

bool DxGCli::validate(Input &input) {
  if (!input.is_ee_calibration) { return true; }

  const auto drug_count = input.half_life.size();
  input.number_of_drugs_in_combination = static_cast<int>(drug_count);
  if (drug_count > 5) {
    spdlog::error("Number of drugs in combination must not exceed 5");
    return false;
  }

  if (input.k_max.size() != drug_count || input.ec50.size() != drug_count
      || input.slope.size() != drug_count || input.dosing_days.size() != drug_count) {
    spdlog::error("DxG drug parameter lists must have the same length");
    return false;
  }

  for (const auto k_max : input.k_max) {
    if (k_max >= 1 || k_max < 0) {
      spdlog::error("k_max must be in the range [0, 1)");
      return false;
    }
  }
  for (const auto ec50 : input.ec50) {
    if (ec50 < 0) {
      spdlog::error("EC50 must not be negative");
      return false;
    }
  }
  for (const auto slope : input.slope) {
    if (slope < 0) {
      spdlog::error("Slope must not be negative");
      return false;
    }
  }
  for (const auto dosing_day : input.dosing_days) {
    if (dosing_day < 0) {
      spdlog::error("Dosing days must not be negative");
      return false;
    }
  }

  if (input.mean_drug_absorption.empty()) {
    input.mean_drug_absorption.assign(drug_count, 1.0);
  } else if (input.mean_drug_absorption.size() != drug_count) {
    spdlog::error("Mean drug absorption must be specified once per drug");
    return false;
  }

  if (input.genotypes.size() > 1) {
    spdlog::error("Efficacy Estimator accepts at most one genotype");
    return false;
  }

  return true;
}

}  // namespace utils
