#include <cstdlib>
#include <string_view>

#include <spdlog/spdlog.h>

#include "gtest/gtest.h"

namespace {

spdlog::level::level_enum test_log_level() {
  const char* configured_level = std::getenv("MALASIM_LOG_LEVEL");
  if (configured_level == nullptr) { return spdlog::level::warn; }

  const std::string_view level{configured_level};
  if (level == "trace") { return spdlog::level::trace; }
  if (level == "debug") { return spdlog::level::debug; }
  if (level == "info") { return spdlog::level::info; }
  if (level == "warn") { return spdlog::level::warn; }
  if (level == "err" || level == "error") { return spdlog::level::err; }
  if (level == "critical") { return spdlog::level::critical; }
  if (level == "off") { return spdlog::level::off; }

  return spdlog::level::warn;
}

void configure_test_logging() {
  const auto level = test_log_level();
  spdlog::set_level(level);
  spdlog::apply_all([level](const std::shared_ptr<spdlog::logger>& logger) {
    logger->set_level(level);
  });
}

class GlobalEnvironment : public ::testing::Environment {
public:
  void SetUp() override { configure_test_logging(); }

  void TearDown() override { spdlog::shutdown(); }
};

class TestLoggingListener : public ::testing::EmptyTestEventListener {
public:
  void OnTestStart(const ::testing::TestInfo&) override { configure_test_logging(); }
};

}  // namespace

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  ::testing::AddGlobalTestEnvironment(new GlobalEnvironment);
  ::testing::UnitTest::GetInstance()->listeners().Append(new TestLoggingListener);
  return RUN_ALL_TESTS();
}
