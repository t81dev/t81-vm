#pragma once

#include <cstdint>
#include <string>

#include "t81/vm/state.hpp"

namespace t81::vm {

std::uint64_t state_hash(const State& state);
std::string snapshot_summary(const State& state);

}  // namespace t81::vm
