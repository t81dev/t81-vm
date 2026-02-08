#pragma once

#include <optional>

#include "t81/tisc/program.hpp"
#include "t81/vm/traps.hpp"

namespace t81::vm {

// Performs static validation that does not require runtime state.
std::optional<Trap> validate_program(const t81::tisc::Program& program);

}  // namespace t81::vm
