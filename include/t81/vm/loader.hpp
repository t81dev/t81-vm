#pragma once

#include <optional>

#include "t81/tisc/program.hpp"
#include "t81/vm/state.hpp"
#include "t81/vm/traps.hpp"

namespace t81::vm {

struct LoadedProgram {
  t81::tisc::Program program;
  State initial_state;
  std::optional<Trap> preload_trap;
};

LoadedProgram load_program_image(const t81::tisc::Program& program);

}  // namespace t81::vm
