#include "Utils/Cli.h"

#include <spdlog/spdlog.h>

#include <cstdlib>
#include <iostream>
#include <vector>

namespace utils::cli {

void detail::parse_app(CLI::App &app,
                       std::vector<char*> &arguments,
                       std::string_view application_name) {
  spdlog::info("Parsing {} command line arguments", application_name);
  try {
    app.parse(static_cast<int>(arguments.size()), arguments.data());
  } catch (const CLI::CallForHelp &) {
    std::cout << app.help();
    std::exit(0);
  } catch (const CLI::ParseError &error) {
    spdlog::error("{} CLI parsing failed: {}", application_name, error.what());
    throw;
  }
}

}  // namespace utils::cli
