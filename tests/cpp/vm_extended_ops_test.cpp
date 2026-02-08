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

  return 0;
}
