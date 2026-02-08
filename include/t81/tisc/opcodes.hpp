#pragma once

#include <cstdint>

namespace t81::tisc {

enum class Opcode : std::uint8_t {
  Nop = 0,
  Halt,
  LoadImm,
  Load,
  Store,
  Add,
  Sub,
  Mul,
  Div,
  Mod,
  Jump,
  JumpIfZero,
  Mov,
  Inc,
  Dec,
  Cmp,
  Push,
  Pop,
  JumpIfNotZero,
  Call,
  Ret,
  Trap,
  Neg,
  JumpIfNegative,
  JumpIfPositive,
  StackAlloc,
  StackFree,
  HeapAlloc,
  HeapFree,
};

}  // namespace t81::tisc
