#include "t81/vm/validator.hpp"

#include <cstddef>

namespace t81::vm {

namespace {

constexpr std::size_t kRegisterCount = 243;

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
    case Opcode::Add:
    case Opcode::Sub:
    case Opcode::Mul:
    case Opcode::Div:
    case Opcode::Mod:
    case Opcode::Jump:
    case Opcode::JumpIfZero:
    case Opcode::Mov:
    case Opcode::Inc:
    case Opcode::Dec:
    case Opcode::Cmp:
    case Opcode::Push:
    case Opcode::Pop:
    case Opcode::JumpIfNotZero:
    case Opcode::Call:
    case Opcode::Ret:
    case Opcode::Trap:
    case Opcode::Neg:
    case Opcode::JumpIfNegative:
    case Opcode::JumpIfPositive:
    case Opcode::I2F:
    case Opcode::F2I:
    case Opcode::I2Frac:
    case Opcode::Frac2I:
    case Opcode::FAdd:
    case Opcode::FSub:
    case Opcode::FMul:
    case Opcode::FDiv:
    case Opcode::FracAdd:
    case Opcode::FracSub:
    case Opcode::FracMul:
    case Opcode::FracDiv:
    case Opcode::Less:
    case Opcode::LessEqual:
    case Opcode::Greater:
    case Opcode::GreaterEqual:
    case Opcode::Equal:
    case Opcode::NotEqual:
    case Opcode::StackAlloc:
    case Opcode::StackFree:
    case Opcode::HeapAlloc:
    case Opcode::HeapFree:
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
      case t81::tisc::Opcode::Add:
      case t81::tisc::Opcode::Sub:
      case t81::tisc::Opcode::Mul:
      case t81::tisc::Opcode::FAdd:
      case t81::tisc::Opcode::FSub:
      case t81::tisc::Opcode::FMul:
      case t81::tisc::Opcode::FDiv:
      case t81::tisc::Opcode::FracAdd:
      case t81::tisc::Opcode::FracSub:
      case t81::tisc::Opcode::FracMul:
      case t81::tisc::Opcode::FracDiv:
      case t81::tisc::Opcode::Less:
      case t81::tisc::Opcode::LessEqual:
      case t81::tisc::Opcode::Greater:
      case t81::tisc::Opcode::GreaterEqual:
      case t81::tisc::Opcode::Equal:
      case t81::tisc::Opcode::NotEqual:
        if (!valid_reg(insn.a) || !valid_reg(insn.b) || !valid_reg(insn.c)) {
          return Trap::DecodeFault;
        }
        break;
      case t81::tisc::Opcode::Cmp:
        if (!valid_reg(insn.a) || !valid_reg(insn.b)) {
          return Trap::DecodeFault;
        }
        break;
      case t81::tisc::Opcode::Mov:
        if (!valid_reg(insn.a) || !valid_reg(insn.b)) {
          return Trap::DecodeFault;
        }
        break;
      case t81::tisc::Opcode::Inc:
      case t81::tisc::Opcode::Dec:
      case t81::tisc::Opcode::Push:
      case t81::tisc::Opcode::Pop:
      case t81::tisc::Opcode::Neg:
      case t81::tisc::Opcode::Call:
      case t81::tisc::Opcode::I2F:
      case t81::tisc::Opcode::F2I:
      case t81::tisc::Opcode::I2Frac:
      case t81::tisc::Opcode::Frac2I:
      case t81::tisc::Opcode::StackAlloc:
      case t81::tisc::Opcode::StackFree:
      case t81::tisc::Opcode::HeapAlloc:
      case t81::tisc::Opcode::HeapFree:
        if (!valid_reg(insn.a)) {
          return Trap::DecodeFault;
        }
        break;
      case t81::tisc::Opcode::Jump:
      case t81::tisc::Opcode::JumpIfZero:
      case t81::tisc::Opcode::JumpIfNotZero:
      case t81::tisc::Opcode::JumpIfNegative:
      case t81::tisc::Opcode::JumpIfPositive:
        if (insn.a < 0 || static_cast<std::size_t>(insn.a) >= program.insns.size()) {
          return Trap::DecodeFault;
        }
        break;
      case t81::tisc::Opcode::Nop:
      case t81::tisc::Opcode::Halt:
      case t81::tisc::Opcode::Ret:
      case t81::tisc::Opcode::Trap:
        break;
    }
  }
  return std::nullopt;
}

}  // namespace t81::vm
