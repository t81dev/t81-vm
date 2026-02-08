#include "t81/tisc/opcodes.hpp"
#include "t81/tisc/program.hpp"
#include "t81/vm/vm.hpp"

#include <cassert>
#include <vector>

namespace {
t81::vm::Trap run_expected_trap(const std::vector<t81::tisc::Insn>& insns) {
  t81::tisc::Program program;
  program.insns = insns;
  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);
  auto result = vm->run_to_halt();
  assert(!result.has_value());
  return result.error();
}
}  // namespace

int main() {
  t81::tisc::Insn load_ten{t81::tisc::Opcode::LoadImm, 0, 10, 0};
  t81::tisc::Insn load_zero{t81::tisc::Opcode::LoadImm, 1, 0, 0};
  t81::tisc::Insn div{t81::tisc::Opcode::Div, 0, 0, 1};
  t81::tisc::Insn halt{t81::tisc::Opcode::Halt, 0, 0, 0};
  auto trap_div_zero = run_expected_trap({load_ten, load_zero, div, halt});
  assert(trap_div_zero == t81::vm::Trap::DivisionFault);

  t81::tisc::Insn load_bad{t81::tisc::Opcode::Load, 0, 9999, 0};
  auto trap_bad_load = run_expected_trap({load_bad, halt});
  assert(trap_bad_load == t81::vm::Trap::DecodeFault);

  return 0;
}
