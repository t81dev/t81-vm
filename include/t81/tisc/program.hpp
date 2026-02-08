#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "t81/tisc/opcodes.hpp"

namespace t81::tisc {

struct Insn {
  Opcode opcode;
  std::int64_t a;
  std::int64_t b;
  std::int64_t c;
};

struct Program {
  std::vector<Insn> insns;
  std::string axion_policy_text;
};

}  // namespace t81::tisc
