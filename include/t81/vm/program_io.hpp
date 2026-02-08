#pragma once

#include <string>

#include "t81/tisc/program.hpp"

namespace t81::vm {

enum class ProgramFormat {
  TextV1,
  TiscJsonV1,
};

struct ProgramLoadResult {
  bool ok = false;
  ProgramFormat format = ProgramFormat::TextV1;
  t81::tisc::Program program;
  std::string error;
};

ProgramLoadResult load_program_from_file(const std::string& path);

}  // namespace t81::vm
