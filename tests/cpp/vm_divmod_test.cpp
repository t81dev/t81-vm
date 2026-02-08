#include <cassert>
#include <t81/tisc/program.hpp>
#include <t81/vm/vm.hpp>

using namespace t81;

int main() {
  tisc::Program p;
  p.insns.push_back({tisc::Opcode::LoadImm, 0, 10, 0});
  p.insns.push_back({tisc::Opcode::LoadImm, 1, 3, 0});
  p.insns.push_back({tisc::Opcode::Div, 2, 0, 1});
  p.insns.push_back({tisc::Opcode::Mod, 3, 0, 1});
  p.insns.push_back({tisc::Opcode::Halt, 0, 0, 0});

  auto vm = vm::make_interpreter_vm();
  vm->load_program(p);
  auto res = vm->run_to_halt();
  assert(res.has_value());
  assert(vm->state().registers[2] == 10 / 3);
  assert(vm->state().registers[3] == 10 % 3);

  tisc::Program bad;
  bad.insns.push_back({tisc::Opcode::LoadImm, 0, 5, 0});
  bad.insns.push_back({tisc::Opcode::LoadImm, 1, 0, 0});
  bad.insns.push_back({tisc::Opcode::Div, 2, 0, 1});

  auto vm2 = vm::make_interpreter_vm();
  vm2->load_program(bad);
  auto step = vm2->step();
  assert(step.has_value());
  step = vm2->step();
  assert(step.has_value());
  step = vm2->step();
  assert(!step.has_value());
  assert(step.error() == vm::Trap::DivisionFault);

  return 0;
}
