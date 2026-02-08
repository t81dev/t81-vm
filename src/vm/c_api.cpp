#include "t81/vm/c_api.h"

#include <expected>
#include <memory>
#include <new>

#include "t81/vm/program_io.hpp"
#include "t81/vm/summary.hpp"
#include "t81/vm/vm.hpp"

struct t81vm_handle {
  std::unique_ptr<t81::vm::IVirtualMachine> vm;
  int last_trap = 0;
};

namespace {

constexpr int kStatusOk = 0;
constexpr int kStatusHalted = 1;
constexpr int kStatusInvalidArg = -1;
constexpr int kStatusParseFault = -2;

int trap_to_status(t81::vm::Trap trap) {
  return static_cast<int>(trap);
}

}  // namespace

t81vm_handle* t81vm_create(void) {
  auto* handle = new (std::nothrow) t81vm_handle();
  if (handle == nullptr) {
    return nullptr;
  }
  handle->vm = t81::vm::make_interpreter_vm();
  if (!handle->vm) {
    delete handle;
    return nullptr;
  }
  return handle;
}

void t81vm_destroy(t81vm_handle* handle) {
  delete handle;
}

int t81vm_load_file(t81vm_handle* handle, const char* path) {
  if (handle == nullptr || handle->vm == nullptr || path == nullptr) {
    return kStatusInvalidArg;
  }
  const auto loaded = t81::vm::load_program_from_file(path);
  if (!loaded.ok) {
    handle->last_trap = trap_to_status(t81::vm::Trap::DecodeFault);
    return kStatusParseFault;
  }
  handle->vm->load_program(loaded.program);
  handle->last_trap = trap_to_status(t81::vm::Trap::None);
  return kStatusOk;
}

int t81vm_step(t81vm_handle* handle) {
  if (handle == nullptr || handle->vm == nullptr) {
    return kStatusInvalidArg;
  }
  auto result = handle->vm->step();
  if (!result.has_value()) {
    handle->last_trap = trap_to_status(result.error());
    return handle->last_trap;
  }
  if (handle->vm->state().halted) {
    return kStatusHalted;
  }
  return kStatusOk;
}

int t81vm_run_to_halt(t81vm_handle* handle, size_t max_steps) {
  if (handle == nullptr || handle->vm == nullptr) {
    return kStatusInvalidArg;
  }
  auto result = handle->vm->run_to_halt(max_steps);
  if (!result.has_value()) {
    handle->last_trap = trap_to_status(result.error());
    return handle->last_trap;
  }
  return kStatusOk;
}

int t81vm_last_trap(const t81vm_handle* handle) {
  if (handle == nullptr) {
    return kStatusInvalidArg;
  }
  return handle->last_trap;
}

size_t t81vm_pc(const t81vm_handle* handle) {
  if (handle == nullptr || handle->vm == nullptr) {
    return 0;
  }
  return handle->vm->state().pc;
}

int t81vm_halted(const t81vm_handle* handle) {
  if (handle == nullptr || handle->vm == nullptr) {
    return 0;
  }
  return handle->vm->state().halted ? 1 : 0;
}

uint64_t t81vm_state_hash(const t81vm_handle* handle) {
  if (handle == nullptr || handle->vm == nullptr) {
    return 0;
  }
  return t81::vm::state_hash(handle->vm->state());
}

int64_t t81vm_register(const t81vm_handle* handle, size_t index) {
  if (handle == nullptr || handle->vm == nullptr) {
    return 0;
  }
  const auto& regs = handle->vm->state().registers;
  if (index >= regs.size()) {
    return 0;
  }
  return regs[index];
}

size_t t81vm_trace_len(const t81vm_handle* handle) {
  if (handle == nullptr || handle->vm == nullptr) {
    return 0;
  }
  return handle->vm->state().trace.size();
}

int t81vm_trace_get(const t81vm_handle* handle, size_t index, t81vm_trace_entry* out) {
  if (handle == nullptr || handle->vm == nullptr || out == nullptr) {
    return kStatusInvalidArg;
  }
  const auto& trace = handle->vm->state().trace;
  if (index >= trace.size()) {
    return kStatusInvalidArg;
  }
  out->pc = trace[index].pc;
  out->opcode = static_cast<uint8_t>(trace[index].opcode);
  out->trap = trace[index].trap.has_value() ? static_cast<int>(*trace[index].trap) : -1;
  return kStatusOk;
}
