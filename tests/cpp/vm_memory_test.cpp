#include <cassert>
#include <string_view>
#include <vector>

#include "t81/tisc/opcodes.hpp"
#include "t81/tisc/program.hpp"
#include "t81/vm/vm.hpp"

namespace {

bool contains_reason(const t81::vm::State& state, std::string_view needle) {
  for (const auto& e : state.axion_log) {
    if (e.reason.find(needle) != std::string::npos) {
      return true;
    }
  }
  return false;
}

std::unique_ptr<t81::vm::IVirtualMachine> run_program(const std::vector<t81::tisc::Insn>& insns) {
  t81::tisc::Program p;
  p.insns = insns;
  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(p);
  auto result = vm->run_to_halt();
  assert(result.has_value());
  return vm;
}

t81::vm::Trap run_expected_trap(const std::vector<t81::tisc::Insn>& insns, t81::vm::State* out_state) {
  t81::tisc::Program p;
  p.insns = insns;
  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(p);
  auto result = vm->run_to_halt();
  assert(!result.has_value());
  if (out_state != nullptr) {
    *out_state = vm->state();
  }
  return result.error();
}

}  // namespace

int main() {
  t81::tisc::Insn stack_alloc{t81::tisc::Opcode::StackAlloc, 0, 16, 0};
  t81::tisc::Insn stack_free{t81::tisc::Opcode::StackFree, 0, 16, 0};
  t81::tisc::Insn halt{t81::tisc::Opcode::Halt, 0, 0, 0};

  {
    auto vm = run_program({stack_alloc, stack_free, halt});
    assert(vm->state().stack_frames.empty());
    assert(vm->state().sp == vm->state().layout.stack.limit);
    assert(contains_reason(vm->state(), "stack frame allocated"));
    assert(contains_reason(vm->state(), "stack frame freed"));
  }

  {
    t81::vm::State state;
    t81::tisc::Insn huge{t81::tisc::Opcode::StackAlloc, 1, 1000000, 0};
    auto trap = run_expected_trap({huge, halt}, &state);
    assert(trap == t81::vm::Trap::StackFault);
    assert(contains_reason(state, "bounds fault segment=stack"));
  }

  t81::tisc::Insn heap_alloc{t81::tisc::Opcode::HeapAlloc, 3, 32, 0};
  t81::tisc::Insn heap_free{t81::tisc::Opcode::HeapFree, 3, 32, 0};
  {
    auto vm = run_program({heap_alloc, heap_free, halt});
    assert(vm->state().heap_frames.empty());
    assert(vm->state().heap_ptr == vm->state().layout.heap.start);
    assert(contains_reason(vm->state(), "heap block allocated"));
    assert(contains_reason(vm->state(), "heap block freed"));
  }

  {
    t81::vm::State state;
    t81::tisc::Insn huge{t81::tisc::Opcode::HeapAlloc, 4, 1000000, 0};
    auto trap = run_expected_trap({huge, halt}, &state);
    assert(trap == t81::vm::Trap::BoundsFault);
    assert(contains_reason(state, "bounds fault segment=heap"));
  }

  return 0;
}
