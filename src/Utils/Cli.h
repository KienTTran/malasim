#include <spdlog/spdlog.h>

#include <CLI/CLI.hpp>
#include <string>

namespace utils {
class Cli {
public:
  struct MaSimAppInput {
    std::string input_path{"input.yml"};
    std::string output_path;
    std::vector<std::string> reporters; // allow multiple reporters via CLI
    int verbosity{0};
    int job_number{0};
    int replicate{1};
    std::string list_reporters{"lr"};
    std::string help{"h"};
    bool dump_movement_matrix{false};
    bool record_individual_movement{false};
    bool record_cell_movement{false};
    bool record_district_movement{false};
    bool record_movement{false};
  };
  struct DxGAppInput {
    std::string input_file{ "input.yml" };
    std::string output_file{ "" };
    std::vector<int> therapies{};
    std::vector<int> therapy_list{};
    std::vector<std::string> genotypes{};
    bool is_crt_calibration = false;
    double as_iov = -1.0;
    double as_iiv = -1.0;
    double as_ec50 = -1.0;
    bool is_ee_calibration = false;
    int number_of_drugs_in_combination{ 1 };
    bool is_art{ false };
    std::vector<int> dosing_days{};
    std::vector<double> half_life{};
    std::vector<double> k_max{};
    std::vector<double> EC50{};
    std::vector<double> slope{};
    std::vector<double> mean_drug_absorption{};
    int population_size{ 10000 };
    bool is_print_immunity_level{ false };
    bool is_old_format{ false };
  };


public:
  // Static method to get the single instance of the class
  static Cli &get_instance() {
    static Cli instance;  // Guaranteed to be destroyed and instantiated on first use.
    return instance;
  }

  Cli(Cli &&) = delete;
  Cli &operator=(Cli &&) = delete;
  // Delete copy constructor and assignment operator
  Cli(const Cli &) = delete;
  void operator=(const Cli &) = delete;

  explicit Cli(MaSimAppInput cli_input) : cli_input_(std::move(cli_input)) {}

  // Parse function
  void parse(int argc, char** argv) {
    bool isDxG = false;

    for (int i = 1; i < argc; ++i) {
      std::string arg = argv[i];
      if (arg == "--DxG" || arg == "DxG=1" || arg == "DxG=true") {
        isDxG = true;
        break;
      }
    }

    spdlog::info("Parsing command line arguments");

    try {
      if (isDxG) {
        create_dxg_cli_options(app_, dxg_input_);
        app_.parse(argc, argv);
      } else {
        create_cli_options(app_, cli_input_);
        app_.parse(argc, argv);
        validate_config(cli_input_);
      }
    } catch (const CLI::ParseError &e) {
      spdlog::error("CLI parsing failed: {}", e.what());
      throw;  // or return false if you refactor to use a return value
    }
  }



  // Accessors for parameters
  [[nodiscard]] std::string get_input_path() const { return cli_input_.input_path; }
  void set_input_path(const std::string &input_path) { cli_input_.input_path = input_path; }
  [[nodiscard]] int get_job_number() const { return cli_input_.job_number; }
  [[nodiscard]] int get_replicate() const { return cli_input_.replicate; }
  [[nodiscard]] const std::vector<std::string>& get_reporters() const { return cli_input_.reporters; }
  [[nodiscard]] std::string get_output_path() const { return cli_input_.output_path; }
  void set_output_path(const std::string &output_path) { cli_input_.output_path = output_path; }
  [[nodiscard]] int get_verbosity() const { return cli_input_.verbosity; }
  [[nodiscard]] bool get_dump_movement_matrix() const { return cli_input_.dump_movement_matrix; }
  [[nodiscard]] bool get_record_individual_movement() const {
    return cli_input_.record_individual_movement;
  }
  [[nodiscard]] bool get_record_cell_movement() const { return cli_input_.record_cell_movement; }
  [[nodiscard]] bool get_record_district_movement() const {
    return cli_input_.record_district_movement;
  }
  [[nodiscard]] bool get_record_movement() const { return cli_input_.record_movement; }
  [[nodiscard]] DxGAppInput get_dxg_app_input() {
    return dxg_input_;
  }

  static void create_cli_options(CLI::App &app, MaSimAppInput &input) {
    app.add_option("-i,--input", input.input_path, "Input filename. Default: `input.yml`.");

    app.add_option("-o,--output", input.output_path, "Output path. Default: `./`.");

    // Allow specifying multiple reporters: -r rep1 -r rep2 ...
    app.add_option("-r,--reporter", input.reporters, "Reporter type(s). Can be supplied multiple times or as a space-separated list.")
        ->expected(-1);

    app.add_option("-v,--verbosity", input.verbosity,
                   "Sets the verbosity of the logging. Default: 0");

    app.add_option(
        "-j,--job", input.job_number,
        "Sets the study to associate with the configuration (or database id). Default: 0");

    app.add_option("-d,--dump", input.dump_movement_matrix,
                   "Dump the movement matrix as calculated.");

    app.add_option("-l,--list", input.list_reporters, "List the possible reporters.");

    app.add_option("--im", input.record_individual_movement, "Record individual movement data.");

    app.add_option("--mc", input.record_cell_movement, "Record the movement between cells.");

    app.add_option("--md", input.record_district_movement,
                   "Record the movement between districts.");

    app.add_option("--replicate", input.replicate, "Replicate number. Default: 1");
  }

  static void create_dxg_cli_options(CLI::App &app, DxGAppInput &input) {
      app.add_option("-i,--input", input.input_file, "Input filename for DxG")->check(CLI::ExistingFile);
      app.add_option("-o,--output", input.output_file, "Output file path");

      app.add_option("-t,--therapies", input.therapies, "Therapy range (e.g., 0 1 2 ...)")->expected(-1);
      app.add_option("--tl,--therapy_list", input.therapy_list, "List of therapies (e.g., 0 1 2 ...)")->expected(-1);
      app.add_option("-g,--genotypes", input.genotypes, "List of genotypes (e.g., WT KEL1 KEL1/PLA1)")->expected(-1);

      app.add_flag("--cc", input.is_crt_calibration, "Enable PfCRT EC50 calibration");
      app.add_option("--iov", input.as_iov, "AS inter-occasion variability")->default_val(-1.0);
      app.add_option("--iiv", input.as_iiv, "AS inter-individual variability")->default_val(-1.0);
      app.add_option("--as_ec50", input.as_ec50, "AS EC50 value (C580)")->default_val(-1.0);
      app.add_flag("--ee", input.is_ee_calibration, "Enable EE calibration");

      app.add_option("--nd", input.number_of_drugs_in_combination, "Number of drugs in combination")->default_val(1);
      app.add_flag("--art", input.is_art, "Use ART configuration");

      app.add_option("--dose", input.dosing_days, "EE dosing days (e.g., 0 1 2 ...)")->expected(-1);
      app.add_option("--halflife", input.half_life, "EE half-life values")->expected(-1);
      app.add_option("--kmax", input.k_max, "EE kmax values")->expected(-1);
      app.add_option("--EC50", input.EC50, "EE EC50 values")->expected(-1);
      app.add_option("--slope", input.slope, "EE slope values")->expected(-1);
      app.add_option("--mda", input.mean_drug_absorption, "EE mean drug absorption values")->expected(-1);

      app.add_option("--popsize", input.population_size, "Population size for EE")->default_val(10000);

      app.add_flag("--pil", input.is_print_immunity_level, "Print immunity level");
      app.add_flag("--old_format", input.is_old_format, "Print output in old format");
  }


  static bool validate_config(MaSimAppInput &input) {
    std::ifstream f_input(input.input_path.c_str());
    if (!f_input.good()) {
      spdlog::error("Err: Input file error or not found!");
      return false;
    }

    if (input.record_cell_movement && input.record_district_movement) {
      spdlog::error("--mc and --md are mutual exclusive and may not be run together.");
      return false;
    }

    input.record_movement = input.record_individual_movement || input.record_cell_movement
                            || input.record_district_movement;

    if (input.record_movement) { spdlog::info("Movement data will be recorded."); }

    switch (input.verbosity) {
      case 0: {
        spdlog::set_level(spdlog::level::info);
        spdlog::info("Verbosity level set to 0. Only info will be logged.");
        break;
      }
      case 1: {
        spdlog::set_level(spdlog::level::debug);
        spdlog::info("Verbosity level set to 1. Info, debug will be logged.");
        break;
      }
      case 2: {
        spdlog::set_level(spdlog::level::trace);
        spdlog::info("Verbosity level set to 2. Info, debug and trace will be logged.");
        break;
      }
      default: {
        spdlog::set_level(spdlog::level::info);
        spdlog::info("Verbosity level set to 0. Only info will be logged.");
        break;
      }
    }

    return true;
  }

private:
  // Private constructor for Singleton
  Cli() = default;
  ~Cli() = default;
  CLI::App app_{"Individual-based simulation for malaria"};
  MaSimAppInput cli_input_;
  DxGAppInput dxg_input_;
};
}  // namespace utils

