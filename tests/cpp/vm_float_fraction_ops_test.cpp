#include <cassert>

#include "t81/tisc/program.hpp"
#include "t81/vm/vm.hpp"

using namespace t81;

int main() {
  {
    tisc::Program p;
    p.insns.push_back({tisc::Opcode::LoadImm, 0, 20, 0});
    p.insns.push_back({tisc::Opcode::LoadImm, 1, 5, 0});
    p.insns.push_back({tisc::Opcode::FAdd, 2, 0, 1});
    p.insns.push_back({tisc::Opcode::FSub, 3, 0, 1});
    p.insns.push_back({tisc::Opcode::FMul, 4, 0, 1});
    p.insns.push_back({tisc::Opcode::FDiv, 5, 0, 1});
    p.insns.push_back({tisc::Opcode::FracAdd, 6, 0, 1});
    p.insns.push_back({tisc::Opcode::FracSub, 7, 0, 1});
    p.insns.push_back({tisc::Opcode::FracMul, 8, 0, 1});
    p.insns.push_back({tisc::Opcode::FracDiv, 9, 0, 1});
    p.insns.push_back({tisc::Opcode::Halt, 0, 0, 0});

    auto vm = vm::make_interpreter_vm();
    vm->load_program(p);
    auto result = vm->run_to_halt();
    assert(result.has_value());
    assert(vm->state().registers[2] == 25);
    assert(vm->state().registers[3] == 15);
    assert(vm->state().registers[4] == 100);
    assert(vm->state().registers[5] == 4);
    assert(vm->state().registers[6] == 25);
    assert(vm->state().registers[7] == 15);
    assert(vm->state().registers[8] == 100);
    assert(vm->state().registers[9] == 4);
  }

  {
    tisc::Program bad;
    bad.insns.push_back({tisc::Opcode::LoadImm, 0, 7, 0});
    bad.insns.push_back({tisc::Opcode::LoadImm, 1, 0, 0});
    bad.insns.push_back({tisc::Opcode::FDiv, 2, 0, 1});

    auto vm = vm::make_interpreter_vm();
    vm->load_program(bad);
    auto step = vm->step();
    assert(step.has_value());
    step = vm->step();
    assert(step.has_value());
    step = vm->step();
    assert(!step.has_value());
    assert(step.error() == vm::Trap::DivisionFault);
  }

  {
    tisc::Program bad;
    bad.insns.push_back({tisc::Opcode::LoadImm, 0, 7, 0});
    bad.insns.push_back({tisc::Opcode::LoadImm, 1, 0, 0});
    bad.insns.push_back({tisc::Opcode::FracDiv, 2, 0, 1});

    auto vm = vm::make_interpreter_vm();
    vm->load_program(bad);
    auto step = vm->step();
    assert(step.has_value());
    step = vm->step();
    assert(step.has_value());
    step = vm->step();
    assert(!step.has_value());
    assert(step.error() == vm::Trap::DivisionFault);
  }

  return 0;
}
