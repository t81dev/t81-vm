#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct t81vm_handle t81vm_handle;

typedef struct t81vm_trace_entry {
  size_t pc;
  uint8_t opcode;
  int trap;  // -1 when no trap, otherwise t81::vm::Trap integral value.
} t81vm_trace_entry;

t81vm_handle* t81vm_create(void);
void t81vm_destroy(t81vm_handle* handle);

int t81vm_load_file(t81vm_handle* handle, const char* path);
int t81vm_step(t81vm_handle* handle);
int t81vm_run_to_halt(t81vm_handle* handle, size_t max_steps);

int t81vm_last_trap(const t81vm_handle* handle);
size_t t81vm_pc(const t81vm_handle* handle);
int t81vm_halted(const t81vm_handle* handle);
uint64_t t81vm_state_hash(const t81vm_handle* handle);
int64_t t81vm_register(const t81vm_handle* handle, size_t index);

size_t t81vm_trace_len(const t81vm_handle* handle);
int t81vm_trace_get(const t81vm_handle* handle, size_t index, t81vm_trace_entry* out);

#ifdef __cplusplus
}  // extern "C"
#endif
