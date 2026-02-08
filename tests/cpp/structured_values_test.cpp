#include <cassert>

#include "t81/tisc/program.hpp"
#include "t81/vm/vm.hpp"

using namespace t81;

int main() {
  {
    tisc::Program p;
    p.insns.push_back({tisc::Opcode::LoadImm, 0, 42, 0});
    p.insns.push_back({tisc::Opcode::MakeOptionSome, 1, 0, 0});
    p.insns.push_back({tisc::Opcode::OptionIsSome, 2, 1, 0});
    p.insns.push_back({tisc::Opcode::OptionUnwrap, 3, 1, 0});
    p.insns.push_back({tisc::Opcode::MakeOptionNone, 4, 0, 0});
    p.insns.push_back({tisc::Opcode::OptionIsSome, 5, 4, 0});
    p.insns.push_back({tisc::Opcode::MakeResultOk, 6, 0, 0});
    p.insns.push_back({tisc::Opcode::ResultIsOk, 7, 6, 0});
    p.insns.push_back({tisc::Opcode::ResultUnwrapOk, 8, 6, 0});
    p.insns.push_back({tisc::Opcode::MakeResultErr, 9, 0, 0});
    p.insns.push_back({tisc::Opcode::ResultIsOk, 10, 9, 0});
    p.insns.push_back({tisc::Opcode::ResultUnwrapErr, 11, 9, 0});
    p.insns.push_back({tisc::Opcode::MakeEnumVariant, 12, 7, 0});
    p.insns.push_back({tisc::Opcode::EnumIsVariant, 13, 12, 7});
    p.insns.push_back({tisc::Opcode::MakeEnumVariantPayload, 14, 0, 8});
    p.insns.push_back({tisc::Opcode::EnumIsVariant, 15, 14, 7});
    p.insns.push_back({tisc::Opcode::EnumIsVariant, 16, 14, 8});
    p.insns.push_back({tisc::Opcode::EnumUnwrapPayload, 17, 14, 0});
    p.insns.push_back({tisc::Opcode::Halt, 0, 0, 0});

    auto vm = vm::make_interpreter_vm();
    vm->load_program(p);
    auto result = vm->run_to_halt();
    assert(result.has_value());

    assert(vm->state().registers[2] == 1);
    assert(vm->state().registers[3] == 42);
    assert(vm->state().registers[5] == 0);
    assert(vm->state().registers[7] == 1);
    assert(vm->state().registers[8] == 42);
    assert(vm->state().registers[10] == 0);
    assert(vm->state().registers[11] == 42);
    assert(vm->state().registers[13] == 1);
    assert(vm->state().registers[15] == 0);
    assert(vm->state().registers[16] == 1);
    assert(vm->state().registers[17] == 42);

    assert(vm->state().register_tags[1] == vm::ValueTag::OptionHandle);
    assert(vm->state().register_tags[6] == vm::ValueTag::ResultHandle);
    assert(vm->state().register_tags[12] == vm::ValueTag::EnumHandle);
  }

  {
    tisc::Program bad;
    bad.insns.push_back({tisc::Opcode::LoadImm, 0, 1, 0});
    bad.insns.push_back({tisc::Opcode::OptionIsSome, 1, 0, 0});

    auto vm = vm::make_interpreter_vm();
    vm->load_program(bad);
    auto step = vm->step();
    assert(step.has_value());
    step = vm->step();
    assert(!step.has_value());
    assert(step.error() == vm::Trap::TypeFault);
  }

  {
    tisc::Program bad;
    bad.insns.push_back({tisc::Opcode::LoadImm, 0, 1, 0});
    bad.insns.push_back({tisc::Opcode::MakeOptionSome, 1, 0, 0});
    bad.insns.push_back({tisc::Opcode::ResultUnwrapOk, 2, 1, 0});

    auto vm = vm::make_interpreter_vm();
    vm->load_program(bad);
    auto step = vm->step();
    assert(step.has_value());
    step = vm->step();
    assert(step.has_value());
    step = vm->step();
    assert(!step.has_value());
    assert(step.error() == vm::Trap::TypeFault);
  }

  {
    tisc::Program bad;
    bad.insns.push_back({tisc::Opcode::MakeEnumVariant, 0, 7, 0});
    bad.insns.push_back({tisc::Opcode::EnumUnwrapPayload, 1, 0, 0});

    auto vm = vm::make_interpreter_vm();
    vm->load_program(bad);
    auto step = vm->step();
    assert(step.has_value());
    step = vm->step();
    assert(!step.has_value());
    assert(step.error() == vm::Trap::DecodeFault);
  }

  {
    tisc::Program bad;
    bad.insns.push_back({tisc::Opcode::EnumIsVariant, 1, 0, 7});

    auto vm = vm::make_interpreter_vm();
    vm->load_program(bad);
    vm->set_register(0, 999, vm::ValueTag::EnumHandle);
    const auto step = vm->step();
    assert(!step.has_value());
    assert(step.error() == vm::Trap::DecodeFault);
  }

  return 0;
}
