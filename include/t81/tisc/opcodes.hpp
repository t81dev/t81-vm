#pragma once

#include <cstdint>

namespace t81::tisc {

enum class Opcode : std::uint8_t {
  Nop = 0,
  Halt,
  LoadImm,
  Load,
  Store,
  Div,
  Mod,
  Jump,
};

}  // namespace t81::tisc
