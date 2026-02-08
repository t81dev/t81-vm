#include <iostream>
#include <string_view>
#include <vector>

#include "t81/tisc/opcodes.hpp"
#include "t81/tisc/program.hpp"
#include "t81/vm/vm.hpp"

static bool contains_reason(const t81::vm::State& state, std::string_view needle) {
  for (const auto& e : state.axion_log) {
    if (e.reason.find(needle) != std::string::npos) {
      return true;
    }
  }
  return false;
}

static int run_and_expect(std::vector<t81::tisc::Insn> insns, t81::vm::Trap expected, std::string_view reason) {
  t81::tisc::Program p;
  p.insns = std::move(insns);
  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(p);
  auto result = vm->run_to_halt();
  if (result.has_value()) {
    std::cerr << "expected trap but run succeeded\n";
    return 1;
  }
  if (result.error() != expected) {
    std::cerr << "unexpected trap code\n";
    return 1;
  }
  if (!contains_reason(vm->state(), reason)) {
    std::cerr << "missing reason: " << reason << "\n";
    return 1;
  }
  return 0;
}

int main() {
  {
    std::vector<t81::tisc::Insn> p{
        {t81::tisc::Opcode::StackAlloc, 0, 0x7fffffff, 0},
        {t81::tisc::Opcode::Halt, 0, 0, 0},
    };
    if (run_and_expect(std::move(p), t81::vm::Trap::StackFault, "bounds fault segment=stack") != 0) {
      return 1;
    }
  }

  {
    std::vector<t81::tisc::Insn> p{
        {t81::tisc::Opcode::HeapAlloc, 0, 0x7fffffff, 0},
        {t81::tisc::Opcode::Halt, 0, 0, 0},
    };
    if (run_and_expect(std::move(p), t81::vm::Trap::BoundsFault, "bounds fault segment=heap") != 0) {
      return 1;
    }
  }

  {
    std::vector<t81::tisc::Insn> p{
        {t81::tisc::Opcode::Load, 0, -1, 0},
        {t81::tisc::Opcode::Halt, 0, 0, 0},
    };
    if (run_and_expect(std::move(p), t81::vm::Trap::BoundsFault, "action=memory load") != 0) {
      return 1;
    }
  }

  {
    std::vector<t81::tisc::Insn> p{
        {t81::tisc::Opcode::LoadImm, 1, 5, 0},
        {t81::tisc::Opcode::Store, 1000000, 1, 0},
        {t81::tisc::Opcode::Halt, 0, 0, 0},
    };
    if (run_and_expect(std::move(p), t81::vm::Trap::BoundsFault, "action=memory store") != 0) {
      return 1;
    }
  }

  return 0;
}
