#include <cassert>
#include <expected>
#include <t81/tisc/program.hpp>
#include <t81/vm/vm.hpp>

using namespace t81;

int main() {
  tisc::Program p;
  for (int i = 0; i < 80; ++i) {
    p.insns.push_back({tisc::Opcode::Nop, 0, 0, 0});
  }
  p.insns.push_back({tisc::Opcode::LoadImm, 0, 1, 0});
  p.insns.push_back({tisc::Opcode::Load, 1, 9999, 0});
  p.axion_policy_text = "(policy (tier 2) (max-stack 1024))";

  auto vm = vm::make_interpreter_vm();
  vm->load_program(p);
  auto r1 = vm->step();
  assert(r1.has_value());

  std::expected<void, vm::Trap> r2;
  while (true) {
    r2 = vm->step();
    if (!r2.has_value()) break;
  }

  assert(r2.error() == vm::Trap::BoundsFault);
  assert(!vm->state().trace.empty());
  bool saw_loadimm_write = false;
  for (const auto& entry : vm->state().trace) {
    if (entry.opcode == tisc::Opcode::LoadImm && entry.write_reg.has_value()) {
      assert(*entry.write_reg == 0);
      assert(entry.write_value.has_value());
      assert(*entry.write_value == 1);
      assert(entry.write_tag.has_value());
      assert(*entry.write_tag == vm::ValueTag::Int);
      saw_loadimm_write = true;
      break;
    }
  }
  assert(saw_loadimm_write);
  auto last = vm->state().trace.back();
  assert(last.trap.has_value());
  assert(vm->state().last_trap_payload.has_value());
  const auto& payload = *vm->state().last_trap_payload;
  assert(payload.trap == vm::Trap::BoundsFault);
  assert(payload.pc == 81);
  assert(payload.opcode == tisc::Opcode::Load);
  assert(payload.a == 1);
  assert(payload.b == 9999);
  assert(payload.c == 0);
  assert(payload.segment == vm::MemorySegmentKind::Unknown);
  assert(payload.detail == "memory load");
  assert(vm->state().policy.has_value());
  assert(vm->state().policy->tier == 2);
  assert(vm->state().gc_cycles > 0);

  return 0;
}
