//
// Created by Nguyen Tran on 2019-04-18.
//

#include "ReporterUtils.h"

#include <Configuration/Config.h>

#include <vector>

#include "Core/Scheduler/Scheduler.h"
#include "Mosquito/Mosquito.h"
#include "Parasites/Genotype.h"
#include "Population/ClonalParasitePopulation.h"
#include "Population/SingleHostClonalParasitePopulations.h"
#include "Simulation/Model.h"
#include "Utils/Index/PersonIndexByLocationStateAgeClass.h"
#include "spdlog/sinks/basic_file_sink.h"

void ReporterUtils::output_genotype_frequency1(std::stringstream &ss,
                                               const int &number_of_genotypes,
                                               PersonIndexByLocationStateAgeClass* pi) {
  auto sum1_all = 0.0;
  std::vector<double> result1_all(number_of_genotypes, 0.0);

  const auto number_of_locations = pi->vPerson().size();
  const auto number_of_age_classes = pi->vPerson()[0][0].size();

  for (auto loc = 0; loc < number_of_locations; loc++) {
    std::vector<double> result1(number_of_genotypes, 0.0);
    auto sum1 = 0.0;

    for (auto hs = 0; hs < Person::NUMBER_OF_STATE - 1; hs++) {
      for (auto ac = 0; ac < number_of_age_classes; ac++) {
        const auto size = pi->vPerson()[loc][hs][ac].size();
        for (auto i = 0ULL; i < size; i++) {
          auto* person = pi->vPerson()[loc][hs][ac][i];

          if (!person->get_all_clonal_parasite_populations()->empty()) {
            sum1 += 1;
            sum1_all += 1;
          }
          // sum2 += person->get_all_clonal_parasite_populations()->parasites()->size();
          std::map<int, int> individual_genotype_map;

          for (auto &parasite_population : *person->get_all_clonal_parasite_populations()) {
            const auto g_id = parasite_population->genotype()->genotype_id();
            // result2[g_id] += 1;
            if (!individual_genotype_map.contains(g_id)) {
              individual_genotype_map[parasite_population->genotype()->genotype_id()] = 1;
            } else {
              individual_genotype_map[parasite_population->genotype()->genotype_id()] += 1;
            }
          }

          for (const auto genotype : individual_genotype_map) {
            result1[genotype.first] += 1;
            result1_all[genotype.first] += 1;
          }
        }
      }
    }

    for (auto &genotype_count : result1) {
      genotype_count /= sum1;
      ss << (sum1 == 0 ? 0 : genotype_count) << tsv::SEP;
    }
  }
  ss << tsv::GROUP_SEP;
  for (auto &weighted : result1_all) {
    weighted /= sum1_all;
    ss << (sum1_all == 0 ? 0 : weighted) << tsv::SEP;
  }
}

void ReporterUtils::output_genotype_frequency2(std::stringstream &ss,
                                               const int &number_of_genotypes,
                                               PersonIndexByLocationStateAgeClass* pi) {
  auto sum2_all = 0.0;
  std::vector<double> result2_all(number_of_genotypes, 0.0);
  const auto number_of_locations = pi->vPerson().size();
  const auto number_of_age_classes = pi->vPerson()[0][0].size();

  for (auto loc = 0; loc < number_of_locations; loc++) {
    std::vector<double> result2(number_of_genotypes, 0.0);
    auto sum2 = 0.0;

    for (auto hs = 0; hs < Person::NUMBER_OF_STATE - 1; hs++) {
      for (auto ac = 0; ac < number_of_age_classes; ac++) {
        const auto size = pi->vPerson()[loc][hs][ac].size();
        for (auto i = 0ULL; i < size; i++) {
          auto* person = pi->vPerson()[loc][hs][ac][i];

          sum2 += static_cast<double>(person->get_all_clonal_parasite_populations()->size());
          sum2_all += sum2;
          std::map<int, int> individual_genotype_map;

          for (auto &parasite_population : *person->get_all_clonal_parasite_populations()) {
            const auto g_id = parasite_population->genotype()->genotype_id();
            result2[g_id] += 1;
            result2_all[g_id] += 1;
          }
        }
      }
    }

    // output for each location
    for (auto &genotype_count : result2) {
      genotype_count /= sum2;
      ss << (sum2 == 0 ? 0 : genotype_count) << tsv::SEP;
    }
  }
  ss << tsv::GROUP_SEP;
  // output for all locations
  for (auto &weighted : result2_all) {
    weighted /= sum2_all;
    ss << (sum2_all == 0 ? 0 : weighted) << tsv::SEP;
  }
}

void ReporterUtils::output_genotype_frequency3(std::stringstream &ss,
                                               const int &number_of_genotypes,
                                               PersonIndexByLocationStateAgeClass* pi) {
  auto sum1_all = 0.0;
  std::vector<double> result3_all(number_of_genotypes, 0.0);
  const auto number_of_locations = pi->vPerson().size();
  const auto number_of_age_classes = pi->vPerson()[0][0].size();

  for (auto loc = 0; loc < number_of_locations; loc++) {
    std::vector<double> result3(number_of_genotypes, 0.0);
    auto sum1 = 0.0;

    for (auto hs = 0; hs < Person::NUMBER_OF_STATE - 1; hs++) {
      for (auto ac = 0; ac < number_of_age_classes; ac++) {
        const auto size = pi->vPerson()[loc][hs][ac].size();
        for (auto i = 0ULL; i < size; i++) {
          auto* person = pi->vPerson()[loc][hs][ac][i];

          if (!person->get_all_clonal_parasite_populations()->empty()) {
            sum1 += 1;
            sum1_all += 1;
          }

          std::map<int, int> individual_genotype_map;

          for (auto &parasite_population : *person->get_all_clonal_parasite_populations()) {
            const auto g_id = parasite_population->genotype()->genotype_id();
            if (!individual_genotype_map.contains(g_id)) {
              individual_genotype_map[parasite_population->genotype()->genotype_id()] = 1;
            } else {
              individual_genotype_map[parasite_population->genotype()->genotype_id()] += 1;
            }
            //            individual_genotype_cr[parasite_population->genotype()->genotype_id()] =
            //            parasite_population->genotype()->daily_fitness_multiple_infection;
          }

          for (const auto genotype : individual_genotype_map) {
            result3[genotype.first] +=
                genotype.second
                / static_cast<double>(person->get_all_clonal_parasite_populations()->size());
            result3_all[genotype.first] +=
                genotype.second
                / static_cast<double>(person->get_all_clonal_parasite_populations()->size());
          }
        }
      }
    }
    // output per location
    // TODO: implement dynamic way to output for each location
  }
  //  ss << group_sep;

  // this is for all locations
  for (auto &weighted_genotype : result3_all) {
    weighted_genotype /= sum1_all;
    ss << (sum1_all == 0 ? 0 : weighted_genotype) << tsv::SEP;
  }
  //
  //  ss << group_sep;
  //  ss << sum1_all << sep;
}

void ReporterUtils::output_genotype_frequency4(std::stringstream &ss,
                                               std::stringstream &ss2,
                                               const int &number_of_genotypes,
                                               PersonIndexByLocationStateAgeClass* pi) {
  auto sum1_all = 0.0;
  std::vector<double> result4_all(number_of_genotypes, 0.0);
  const auto number_of_locations = pi->vPerson().size();
  const auto number_of_age_classes = pi->vPerson()[0][0].size();

  for (auto loc = 0; loc < number_of_locations; loc++) {
    std::vector<double> result4(number_of_genotypes, 0.0);
    std::vector<double> prmc4(number_of_genotypes, 0.0);
    std::vector<double> prmc4_all(number_of_genotypes, 0.0);
    auto sum1 = 0.0;

    for (auto hs = 0; hs < Person::NUMBER_OF_STATE - 1; hs++) {
      for (auto ac = 0; ac < number_of_age_classes; ac++) {
        const auto size = pi->vPerson()[loc][hs][ac].size();
        for (auto i = 0ULL; i < size; i++) {
          auto* person = pi->vPerson()[loc][hs][ac][i];

          if (!person->get_all_clonal_parasite_populations()->empty()) {
            sum1 += 1;
            sum1_all += 1;
          }

          std::map<int, int> individual_genotype_map;

          for (auto &parasite_population : *person->get_all_clonal_parasite_populations()) {
            const auto g_id = parasite_population->genotype()->genotype_id();
            if (!individual_genotype_map.contains(g_id)) {
              individual_genotype_map[parasite_population->genotype()->genotype_id()] = 1;
            } else {
              individual_genotype_map[parasite_population->genotype()->genotype_id()] += 1;
            }
            //            individual_genotype_cr[parasite_population->genotype()->genotype_id()] =
            //            parasite_population->genotype()->daily_fitness_multiple_infection;
          }

          for (const auto genotype : individual_genotype_map) {
            result4[genotype.first] +=
                genotype.second
                / static_cast<double>(person->get_all_clonal_parasite_populations()->size());
            result4_all[genotype.first] +=
                genotype.second
                / static_cast<double>(person->get_all_clonal_parasite_populations()->size());
          }
        }
      }
    }
    // output per location
    // TODO: implement dynamic way to output for each location

    //    for (auto& i : result3) {
    //      i /= sum1;
    //      ss << (sum1 == 0 ? 0 : i) << sep;
    //    }

    std::map<int, int> prmc_genotype_map;
    auto tracking_day =
        Model::get_scheduler()->current_time() % Model::get_config()->number_of_tracking_days();
    int sum_nullptr = 0;
    for (auto* prmc_genotypes : Model::get_mosquito()->genotypes_table[tracking_day][loc]) {
      if (prmc_genotypes == nullptr) {
        sum_nullptr++;
        continue;
      }
    }
    if (sum_nullptr == 0) {
      for (auto* prmc_genotypes : Model::get_mosquito()->genotypes_table[tracking_day][loc]) {
        const auto g_id = prmc_genotypes->genotype_id();
        if (!prmc_genotype_map.contains(g_id)) {
          prmc_genotype_map[prmc_genotypes->genotype_id()] = 1;
        } else {
          prmc_genotype_map[prmc_genotypes->genotype_id()] += 1;
        }
      }
      for (const auto genotype : prmc_genotype_map) {
        prmc4[genotype.first] +=
            genotype.second
            / static_cast<double>(Model::get_mosquito()->genotypes_table[tracking_day][loc].size());
        prmc4_all[genotype.first] +=
            genotype.second
            / static_cast<double>(Model::get_mosquito()->genotypes_table[tracking_day][loc].size());
      }

      for (auto &weighted_genotype : prmc4) { ss2 << weighted_genotype << tsv::SEP; }
    }
  }
  //  ss << group_sep;

  // this is for all locations
  for (auto &weighted_genotyp : result4_all) {
    weighted_genotyp /= sum1_all;
    ss << (sum1_all == 0 ? 0 : weighted_genotyp) << tsv::SEP;
  }
  //
  //  ss << group_sep;
  //  ss << sum1_all << sep;
}

void ReporterUtils::output_3_genotype_frequency(std::stringstream &ss,
                                                const int &number_of_genotypes,
                                                PersonIndexByLocationStateAgeClass* pi) {
  auto sum1_all = 0.0;
  auto sum2_all = 0.0;
  std::vector<double> result1_all(number_of_genotypes, 0.0);
  std::vector<double> result2_all(number_of_genotypes, 0.0);
  std::vector<double> result3_all(number_of_genotypes, 0.0);

  const auto number_of_locations = pi->vPerson().size();
  const auto number_of_age_classes = pi->vPerson()[0][0].size();

  for (auto loc = 0; loc < number_of_locations; loc++) {
    std::vector<double> result1(number_of_genotypes, 0.0);
    std::vector<double> result2(number_of_genotypes, 0.0);
    std::vector<double> result3(number_of_genotypes, 0.0);

    auto sum2 = 0.0;
    auto sum1 = 0.0;

    for (auto hs = 0; hs < Person::NUMBER_OF_STATE - 1; hs++) {
      for (auto ac = 0; ac < number_of_age_classes; ac++) {
        const auto size = pi->vPerson()[loc][hs][ac].size();
        for (auto i = 0ULL; i < size; i++) {
          auto* person = pi->vPerson()[loc][hs][ac][i];

          if (!person->get_all_clonal_parasite_populations()->empty()) {
            sum1 += 1;
            sum1_all += 1;
          }
          sum2 += static_cast<double>(person->get_all_clonal_parasite_populations()->size());
          sum2_all += sum2;
          std::map<int, int> individual_genotype_map;

          for (auto &parasite_population : *person->get_all_clonal_parasite_populations()) {
            const auto g_id = parasite_population->genotype()->genotype_id();
            result2[g_id] += 1;
            result2_all[g_id] += 1;
            if (!individual_genotype_map.contains(g_id)) {
              individual_genotype_map[parasite_population->genotype()->genotype_id()] = 1;
            } else {
              individual_genotype_map[parasite_population->genotype()->genotype_id()] += 1;
            }
          }

          for (const auto genotype : individual_genotype_map) {
            result1[genotype.first] += 1;
            result1_all[genotype.first] += 1;
            result3[genotype.first] +=
                genotype.second
                / static_cast<double>(person->get_all_clonal_parasite_populations()->size());
            result3_all[genotype.first] +=
                genotype.second
                / static_cast<double>(person->get_all_clonal_parasite_populations()->size());
          }
        }
      }
    }

    for (auto &genotype_count : result1) { genotype_count /= sum1; }

    for (auto &genotype_count : result2) { genotype_count /= sum2; }

    for (auto &weighted_genotype : result3) { weighted_genotype /= sum1; }

    for (auto j = 0; j < number_of_genotypes; ++j) {
      if (sum1 == 0) {
        ss << 0 << tsv::SEP << 0 << tsv::SEP << 0 << tsv::SEP;
      } else {
        ss << result1[j] << tsv::SEP;
        ss << result2[j] << tsv::SEP;
        ss << result3[j] << tsv::SEP;
      }
    }

    ss << tsv::GROUP_SEP;
  }

  // this is for all locations
  ss << tsv::GROUP_SEP;
  for (auto j = 0; j < number_of_genotypes; ++j) {
    if (sum1_all == 0) {
      ss << 0 << tsv::SEP << 0 << tsv::SEP << 0 << tsv::SEP;
    } else {
      result1_all[j] /= sum1_all;
      result2_all[j] /= sum2_all;
      result3_all[j] /= sum1_all;

      ss << result1_all[j] << tsv::SEP;
      ss << result1_all[j] << tsv::SEP;
      ss << result1_all[j] << tsv::SEP;
    }
  }
}

void ReporterUtils::output_moi(std::stringstream &ss, PersonIndexByLocationStateAgeClass* pi) {
  const auto number_of_locations = pi->vPerson().size();
  const auto number_of_age_classes = pi->vPerson()[0][0].size();

  for (auto loc = 0UL; loc < number_of_locations; loc++) {
    auto pop_sum_location = 0;
    for (auto hs = 0; hs < Person::NUMBER_OF_STATE - 1; hs++) {
      for (auto ac = 0UL; ac < number_of_age_classes; ac++) {
        std::size_t size = pi->vPerson()[loc][hs][ac].size();
        for (int i = 0; i < size; i++) {
          Person* person = pi->vPerson()[loc][hs][ac][i];
          int moi = static_cast<int>(person->get_all_clonal_parasite_populations()->size());
          ss << moi << "\n";
        }
      }
    }
  }
  spdlog::get("moi_reporter")->info(ss.str());
  ss.str("");
}

void ReporterUtils::initialize_moi_file_logger() {
  // Construct the log file name
  std::string log_filename = fmt::format("moi_{}.txt", Model::get_cli_input().job_number);

  // Create a file sink
  auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_filename, true);
  file_sink->set_pattern("[%^%l%$] [%n] [%!] [%s:%#] %v");

  // Create the logger with the file sink
  auto logger = std::make_shared<spdlog::logger>("moi_reporter", file_sink);

  // Set log level and patterns for different levels
  logger->set_level(spdlog::level::debug);  // Set to debug to capture all levels

  // Setting custom patterns based on log levels
  logger->set_pattern("[%^%l%$] [%n] [%!] [%s:%#] %v");  // Default pattern

  spdlog::set_default_logger(logger);

  // Example of custom formatting per level
  logger->debug("Debug message");
  logger->error("Error message");
  logger->critical("Fatal message");
  logger->trace("Trace message");
  logger->info("Info message");
  logger->warn("Warning message");

  // Enable auto-flush on every log message
  logger->flush_on(spdlog::level::info);
}

//
// void ReporterUtils::initialize_moi_file_logger() {
//   const std::string OUTPUT_FORMAT = "[%level] [%logger] [%host] [%func] [%loc] %msg";
//
//   el::Configurations moi_reporter_logger;
//   moi_reporter_logger.setToDefault();
//   moi_reporter_logger.set(el::Level::Debug, el::ConfigurationType::Format, OUTPUT_FORMAT);
//   moi_reporter_logger.set(el::Level::Error, el::ConfigurationType::Format, OUTPUT_FORMAT);
//   moi_reporter_logger.set(el::Level::Fatal, el::ConfigurationType::Format, OUTPUT_FORMAT);
//   moi_reporter_logger.set(el::Level::Trace, el::ConfigurationType::Format, OUTPUT_FORMAT);
//   moi_reporter_logger.set(el::Level::Info, el::ConfigurationType::Format, "%msg");
//   moi_reporter_logger.set(el::Level::Warning, el::ConfigurationType::Format, "[%level] [%logger]
//   %msg"); moi_reporter_logger.set(el::Level::Verbose, el::ConfigurationType::Format,
//   "[%level-%vlevel] [%logger] %msg");
//
//   moi_reporter_logger.setGlobally(el::ConfigurationType::ToFile, "true");
//   moi_reporter_logger.setGlobally(el::ConfigurationType::Filename,
//                                   fmt::format("{}moi_{}.txt", "",
//                                   Model::MODEL->cluster_job_number()));
//   moi_reporter_logger.setGlobally(el::ConfigurationType::ToStandardOutput, "false");
//   moi_reporter_logger.setGlobally(el::ConfigurationType::LogFlushThreshold, "100");
//   // default logger uses default configurations
//   el::Loggers::reconfigureLogger("moi_reporter", moi_reporter_logger);
// }

//
//
//
// void MonthlyReporter::print_monthly_incidence_by_location() {
//  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); ++loc) {
//    ss << Model::get_mdc()->monthly_number_of_treatment_by_location()[loc] << sep;
//  }
//
//  ss << group_sep;
//
//  for (auto loc = 0; loc < Model::get_config()->number_of_locations(); ++loc) {
//    ss << Model::get_mdc()->monthly_number_of_clinical_episode_by_location()[loc] << sep;
//  }
//}
