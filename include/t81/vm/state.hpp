#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "t81/tisc/program.hpp"
#include "t81/vm/traps.hpp"

namespace t81::vm {

struct TraceEntry {
  std::size_t pc;
  t81::tisc::Opcode opcode;
  std::optional<Trap> trap;
};

enum class ValueTag : std::uint8_t {
  Int = 0,
};

struct Policy {
  int tier = 0;
};

struct State {
  std::size_t pc = 0;
  bool halted = false;
  std::array<std::int64_t, 16> registers{};
  std::vector<std::int64_t> memory;
  std::vector<TraceEntry> trace;
  std::optional<Policy> policy;
  std::size_t gc_cycles = 0;
};

}  // namespace t81::vm
