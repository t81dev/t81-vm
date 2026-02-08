#include <cassert>

#include "t81/tisc/program.hpp"
#include "t81/vm/vm.hpp"

using namespace t81;

int main() {
  {
    tisc::Program p;
    p.insns.push_back({tisc::Opcode::LoadImm, 0, 7, 0});
    p.insns.push_back({tisc::Opcode::LoadImm, 1, 3, 0});

    p.insns.push_back({tisc::Opcode::Less, 2, 1, 0});          // 3 < 7 => 1
    p.insns.push_back({tisc::Opcode::LessEqual, 3, 0, 0});     // 7 <= 7 => 1
    p.insns.push_back({tisc::Opcode::Greater, 4, 0, 1});       // 7 > 3 => 1
    p.insns.push_back({tisc::Opcode::GreaterEqual, 5, 1, 0});  // 3 >= 7 => 0
    p.insns.push_back({tisc::Opcode::Equal, 6, 0, 0});         // 7 == 7 => 1
    p.insns.push_back({tisc::Opcode::NotEqual, 7, 0, 1});      // 7 != 3 => 1
    p.insns.push_back({tisc::Opcode::Halt, 0, 0, 0});

    auto vm = vm::make_interpreter_vm();
    vm->load_program(p);
    auto result = vm->run_to_halt();
    assert(result.has_value());

    assert(vm->state().registers[2] == 1);
    assert(vm->state().registers[3] == 1);
    assert(vm->state().registers[4] == 1);
    assert(vm->state().registers[5] == 0);
    assert(vm->state().registers[6] == 1);
    assert(vm->state().registers[7] == 1);
    assert(vm->state().flags.positive);
    assert(!vm->state().flags.zero);
    assert(!vm->state().flags.negative);
  }

  {
    tisc::Program p;
    p.insns.push_back({tisc::Opcode::LoadImm, 0, 42, 0});
    p.insns.push_back({tisc::Opcode::I2F, 1, 0, 0});
    p.insns.push_back({tisc::Opcode::F2I, 2, 1, 0});
    p.insns.push_back({tisc::Opcode::I2Frac, 3, 2, 0});
    p.insns.push_back({tisc::Opcode::Frac2I, 4, 3, 0});
    p.insns.push_back({tisc::Opcode::Halt, 0, 0, 0});

    auto vm = vm::make_interpreter_vm();
    vm->load_program(p);
    auto result = vm->run_to_halt();
    assert(result.has_value());
    assert(vm->state().registers[1] == 42);
    assert(vm->state().registers[2] == 42);
    assert(vm->state().registers[3] == 42);
    assert(vm->state().registers[4] == 42);
    assert(vm->state().flags.positive);
  }

  {
    tisc::Program p;
    p.insns.push_back({tisc::Opcode::LoadImm, 0, -3, 0});
    p.insns.push_back({tisc::Opcode::LoadImm, 1, 2, 0});
    p.insns.push_back({tisc::Opcode::TNot, 2, 0, 0});   // clamp(-3)=-1 => 1
    p.insns.push_back({tisc::Opcode::TAnd, 3, 0, 1});   // min(-1,1)=-1
    p.insns.push_back({tisc::Opcode::TOr, 4, 0, 1});    // max(-1,1)=1
    p.insns.push_back({tisc::Opcode::TXor, 5, 0, 1});   // -1 - 1 => -2 => 1 (wrapped)
    p.insns.push_back({tisc::Opcode::Halt, 0, 0, 0});

    auto vm = vm::make_interpreter_vm();
    vm->load_program(p);
    const auto result = vm->run_to_halt();
    assert(result.has_value());
    assert(vm->state().registers[2] == 1);
    assert(vm->state().registers[3] == -1);
    assert(vm->state().registers[4] == 1);
    assert(vm->state().registers[5] == 1);
  }

  {
    tisc::Program p;
    p.axion_policy_text = "(policy (tier 2))";
    p.insns.push_back({tisc::Opcode::LoadImm, 1, 99, 0});
    p.insns.push_back({tisc::Opcode::AxRead, 2, 42, 0});
    p.insns.push_back({tisc::Opcode::AxSet, 2, 1, 0});
    p.insns.push_back({tisc::Opcode::AxVerify, 3, 0, 0});
    p.insns.push_back({tisc::Opcode::Halt, 0, 0, 0});

    auto vm = vm::make_interpreter_vm();
    vm->load_program(p);
    const auto result = vm->run_to_halt();
    assert(result.has_value());
    assert(vm->state().registers[2] == 42);
    assert(vm->state().registers[3] == 0);
    assert(vm->state().axion_log.size() == 3);
    assert(vm->state().axion_log[0].opcode == tisc::Opcode::AxRead);
  }

  {
    tisc::Program p;
    p.axion_policy_text = "(policy (tier 0))";
    p.insns.push_back({tisc::Opcode::AxRead, 0, 1, 0});

    auto vm = vm::make_interpreter_vm();
    vm->load_program(p);
    const auto step = vm->step();
    assert(!step.has_value());
    assert(step.error() == vm::Trap::SecurityFault);
  }

  return 0;
}
