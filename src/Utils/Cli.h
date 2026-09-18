#pragma once

#include <CLI/CLI.hpp>
#include <concepts>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace utils::cli {

namespace detail {
void parse_app(CLI::App &app, std::vector<char*> &arguments, std::string_view application_name);

template <typename App>
[[nodiscard]] std::vector<char*> prepare_arguments(int argc, char** argv) {
  if constexpr (requires {
                  { App::prepare_arguments(argc, argv) } -> std::same_as<std::vector<char*>>;
                }) {
    return App::prepare_arguments(argc, argv);
  } else {
    std::vector<char*> arguments;
    arguments.reserve(static_cast<std::size_t>(argc));
    for (int i = 0; i < argc; ++i) { arguments.push_back(argv[i]); }
    return arguments;
  }
}

template <typename App>
void post_parse(CLI::App &app, typename App::Input &input) {
  if constexpr (requires { App::post_parse(app, input); }) { App::post_parse(app, input); }
}
}  // namespace detail

template <typename App>
concept CliApplication = requires(CLI::App &app, typename App::Input &input) {
  typename App::Input;
  { App::NAME } -> std::convertible_to<std::string_view>;
  { App::DESCRIPTION } -> std::convertible_to<std::string_view>;
  { App::add_options(app, input) } -> std::same_as<void>;
  { App::validate(input) } -> std::same_as<bool>;
};

template <CliApplication App>
[[nodiscard]] typename App::Input parse(int argc, char** argv) {
  typename App::Input input;
  CLI::App app{std::string{App::DESCRIPTION}};
  App::add_options(app, input);
  auto arguments = detail::prepare_arguments<App>(argc, argv);
  detail::parse_app(app, arguments, App::NAME);
  detail::post_parse<App>(app, input);
  return input;
}

template <CliApplication App>
bool validate(typename App::Input &input) {
  return App::validate(input);
}

}  // namespace utils::cli
