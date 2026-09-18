#pragma once

#include <string_view>
#include <vector>

#include "apps/efficacy_estimator/DxGAppInput.h"
#include "Utils/Cli.h"

namespace utils {

struct DxGCli {
  using Input = DxGAppInput;

  static constexpr std::string_view NAME{"DxG"};
  static constexpr std::string_view DESCRIPTION{"Drug efficacy estimator for malaria"};

  static void add_options(CLI::App &app, Input &input);
  [[nodiscard]] static std::vector<char*> prepare_arguments(int argc, char** argv);
  static bool validate(Input &input);
};

}  // namespace utils
