#include <cassert>

#include "t81/tisc/program.hpp"
#include "t81/vm/vm.hpp"

using namespace t81;

int main() {
  {
    tisc::Program p;
    p.insns.push_back({tisc::Opcode::LoadImm, 0, 0, 0});
    p.insns.push_back({tisc::Opcode::JumpIfZero, 3, 0, 0});
    p.insns.push_back({tisc::Opcode::LoadImm, 1, 1, 0});
    p.insns.push_back({tisc::Opcode::Halt, 0, 0, 0});

    auto vm = vm::make_interpreter_vm();
    vm->load_program(p);
    auto r = vm->run_to_halt();
    assert(r.has_value());
    assert(vm->state().registers[1] == 0);
    assert(vm->state().flags.zero);
    assert(!vm->state().flags.negative);
  }

  {
    tisc::Program p;
    p.insns.push_back({tisc::Opcode::Jump, 5, 0, 0});
    auto vm = vm::make_interpreter_vm();
    vm->load_program(p);
    auto r = vm->step();
    assert(!r.has_value());
    assert(r.error() == vm::Trap::DecodeFault);
  }

  {
    tisc::Program p;
    p.insns.push_back({tisc::Opcode::LoadImm, 0, 1, 0});
    p.insns.push_back({tisc::Opcode::JumpIfNotZero, 3, 0, 0});
    p.insns.push_back({tisc::Opcode::LoadImm, 1, 99, 0});
    p.insns.push_back({tisc::Opcode::Halt, 0, 0, 0});
    auto vm = vm::make_interpreter_vm();
    vm->load_program(p);
    auto r = vm->run_to_halt();
    assert(r.has_value());
    assert(vm->state().registers[1] == 0);
  }

  {
    tisc::Program p;
    p.insns.push_back({tisc::Opcode::LoadImm, 0, 4, 0});
    p.insns.push_back({tisc::Opcode::Call, 0, 0, 0});
    p.insns.push_back({tisc::Opcode::LoadImm, 2, 7, 0});
    p.insns.push_back({tisc::Opcode::Trap, 1, 0, 0});
    p.insns.push_back({tisc::Opcode::LoadImm, 1, 42, 0});
    p.insns.push_back({tisc::Opcode::Ret, 0, 0, 0});

    auto vm = vm::make_interpreter_vm();
    vm->load_program(p);
    auto r = vm->run_to_halt();
    assert(!r.has_value());
    assert(r.error() == vm::Trap::TrapInstruction);
    assert(vm->state().registers[1] == 42);
    assert(vm->state().registers[2] == 7);
  }

  return 0;
}
