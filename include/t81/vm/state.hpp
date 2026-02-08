#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "t81/tisc/program.hpp"
#include "t81/vm/traps.hpp"

namespace t81::vm {

struct TraceEntry {
  std::size_t pc;
  t81::tisc::Opcode opcode;
  std::optional<Trap> trap;
};

enum class ValueTag : std::uint8_t {
  Int = 0,
};

struct Flags {
  bool zero = false;
  bool negative = false;
  bool positive = false;
};

enum class MemorySegmentKind : std::int32_t {
  Unknown = 0,
  Code,
  Stack,
  Heap,
  Tensor,
  Meta,
};

inline const char* to_string(MemorySegmentKind kind) {
  switch (kind) {
    case MemorySegmentKind::Code:
      return "code";
    case MemorySegmentKind::Stack:
      return "stack";
    case MemorySegmentKind::Heap:
      return "heap";
    case MemorySegmentKind::Tensor:
      return "tensor";
    case MemorySegmentKind::Meta:
      return "meta";
    case MemorySegmentKind::Unknown:
      break;
  }
  return "unknown";
}

struct MemorySegment {
  std::size_t start = 0;
  std::size_t limit = 0;  // exclusive

  [[nodiscard]] bool valid() const { return limit > start; }
  [[nodiscard]] bool contains(std::size_t addr) const { return valid() && addr >= start && addr < limit; }
};

struct MemoryLayout {
  MemorySegment code;
  MemorySegment stack;
  MemorySegment heap;
  MemorySegment tensor;
  MemorySegment meta;

  [[nodiscard]] std::size_t total_size() const { return meta.limit; }
};

struct AxionEvent {
  t81::tisc::Opcode opcode;
  std::string reason;
};

struct Policy {
  int tier = 0;
};

struct State {
  std::size_t pc = 0;
  bool halted = false;
  std::array<std::int64_t, 243> registers{};
  std::vector<std::int64_t> memory;
  std::vector<TraceEntry> trace;
  std::vector<AxionEvent> axion_log;
  Flags flags{};
  MemoryLayout layout{};
  std::size_t sp = 0;
  std::size_t heap_ptr = 0;
  std::vector<std::pair<std::size_t, std::size_t>> stack_frames;
  std::vector<std::pair<std::size_t, std::size_t>> heap_frames;
  std::optional<Policy> policy;
  std::size_t gc_cycles = 0;
};

}  // namespace t81::vm
