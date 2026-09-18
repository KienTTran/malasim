#pragma once

#include <string>

namespace utils {

struct MaSimAppInput {
  std::string input_path{"input.yml"};
  std::string output_path;
  std::string reporter;
  int verbosity{0};
  int job_number{0};
  int replicate{1};
  bool list_reporters{false};
  bool dump_movement_matrix{false};
  bool record_individual_movement{false};
  bool record_cell_movement{false};
  bool record_district_movement{false};
  bool record_movement{false};
  bool print_memory_stats{false};
};

}  // namespace utils
