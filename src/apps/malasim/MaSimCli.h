#pragma once

#include <string_view>

#include "Utils/Cli.h"
#include "apps/malasim/MaSimAppInput.h"

namespace utils {

struct MaSimCli {
  using Input = MaSimAppInput;

  static constexpr std::string_view NAME{"MaSim"};
  static constexpr std::string_view DESCRIPTION{"Individual-based simulation for malaria"};

  static void add_options(CLI::App &app, Input &input);
  static bool validate(Input &input);
};

}  // namespace utils
