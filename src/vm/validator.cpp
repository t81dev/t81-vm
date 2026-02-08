#include "t81/vm/validator.hpp"

#include <cstddef>

namespace t81::vm {

namespace {

constexpr std::size_t kRegisterCount = 16;

bool valid_reg(std::int64_t idx) {
  return idx >= 0 && static_cast<std::size_t>(idx) < kRegisterCount;
}

bool valid_opcode(t81::tisc::Opcode opcode) {
  using t81::tisc::Opcode;
  switch (opcode) {
    case Opcode::Nop:
    case Opcode::Halt:
    case Opcode::LoadImm:
    case Opcode::Load:
    case Opcode::Store:
    case Opcode::Div:
    case Opcode::Mod:
    case Opcode::Jump:
      return true;
  }
  return false;
}

}  // namespace

std::optional<Trap> validate_program(const t81::tisc::Program& program) {
  for (const auto& insn : program.insns) {
    if (!valid_opcode(insn.opcode)) {
      return Trap::DecodeFault;
    }
    switch (insn.opcode) {
      case t81::tisc::Opcode::LoadImm:
        if (!valid_reg(insn.a)) {
          return Trap::DecodeFault;
        }
        break;
      case t81::tisc::Opcode::Load:
        if (!valid_reg(insn.a)) {
          return Trap::DecodeFault;
        }
        break;
      case t81::tisc::Opcode::Store:
        if (!valid_reg(insn.b)) {
          return Trap::DecodeFault;
        }
        break;
      case t81::tisc::Opcode::Div:
      case t81::tisc::Opcode::Mod:
        if (!valid_reg(insn.a) || !valid_reg(insn.b) || !valid_reg(insn.c)) {
          return Trap::DecodeFault;
        }
        break;
      case t81::tisc::Opcode::Jump:
        if (insn.a < 0 || static_cast<std::size_t>(insn.a) >= program.insns.size()) {
          return Trap::DecodeFault;
        }
        break;
      case t81::tisc::Opcode::Nop:
      case t81::tisc::Opcode::Halt:
        break;
    }
  }
  return std::nullopt;
}

}  // namespace t81::vm
