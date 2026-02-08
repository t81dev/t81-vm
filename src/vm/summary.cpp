#include "t81/vm/summary.hpp"

#include <iomanip>
#include <sstream>

namespace t81::vm {

namespace {

constexpr std::uint64_t kFnvOffsetBasis = 1469598103934665603ULL;
constexpr std::uint64_t kFnvPrime = 1099511628211ULL;

void mix_u64(std::uint64_t v, std::uint64_t* h) {
  for (int i = 0; i < 8; ++i) {
    const std::uint8_t b = static_cast<std::uint8_t>((v >> (i * 8)) & 0xffU);
    *h ^= b;
    *h *= kFnvPrime;
  }
}

}  // namespace

std::uint64_t state_hash(const State& state) {
  std::uint64_t h = kFnvOffsetBasis;
  mix_u64(state.pc, &h);
  mix_u64(state.halted ? 1 : 0, &h);
  mix_u64(state.gc_cycles, &h);

  for (const auto reg : state.registers) {
    mix_u64(static_cast<std::uint64_t>(reg), &h);
  }
  for (const auto mem : state.memory) {
    mix_u64(static_cast<std::uint64_t>(mem), &h);
  }

  mix_u64(state.trace.size(), &h);
  for (const auto& entry : state.trace) {
    mix_u64(entry.pc, &h);
    mix_u64(static_cast<std::uint64_t>(entry.opcode), &h);
    mix_u64(entry.trap.has_value() ? static_cast<std::uint64_t>(*entry.trap) : 0ULL, &h);
  }

  if (state.policy.has_value()) {
    mix_u64(1, &h);
    mix_u64(static_cast<std::uint64_t>(state.policy->tier), &h);
  } else {
    mix_u64(0, &h);
  }

  return h;
}

std::string snapshot_summary(const State& state) {
  std::ostringstream out;
  out << "SNAPSHOT pc=" << state.pc << " halted=" << (state.halted ? 1 : 0)
      << " gc_cycles=" << state.gc_cycles;
  if (state.policy.has_value()) {
    out << " policy_tier=" << state.policy->tier;
  }
  out << "\n";

  out << "REGISTERS";
  for (std::size_t i = 0; i < state.registers.size(); ++i) {
    out << " r" << i << "=" << state.registers[i];
  }
  out << "\n";

  out << "STATE_HASH 0x" << std::hex << std::setfill('0') << std::setw(16) << state_hash(state)
      << std::dec << "\n";
  return out.str();
}

}  // namespace t81::vm
