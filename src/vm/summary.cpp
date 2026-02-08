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

std::string escape_payload_detail(const std::string& in) {
  std::string out;
  out.reserve(in.size());
  for (char ch : in) {
    if (ch == '"' || ch == '\\') {
      out.push_back('\\');
      out.push_back(ch);
      continue;
    }
    if (ch == '\n') {
      out += "\\n";
      continue;
    }
    out.push_back(ch);
  }
  return out;
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
    mix_u64(entry.write_reg.has_value() ? static_cast<std::uint64_t>(*entry.write_reg + 1U) : 0ULL, &h);
    mix_u64(entry.write_value.has_value() ? static_cast<std::uint64_t>(*entry.write_value) : 0ULL, &h);
    mix_u64(entry.write_tag.has_value() ? static_cast<std::uint64_t>(*entry.write_tag) + 1ULL : 0ULL, &h);
    mix_u64(entry.trap.has_value() ? static_cast<std::uint64_t>(*entry.trap) : 0ULL, &h);
  }

  if (state.last_trap_payload.has_value()) {
    const auto& p = *state.last_trap_payload;
    mix_u64(1, &h);
    mix_u64(static_cast<std::uint64_t>(p.trap), &h);
    mix_u64(p.pc, &h);
    mix_u64(static_cast<std::uint64_t>(p.opcode), &h);
    mix_u64(static_cast<std::uint64_t>(p.a), &h);
    mix_u64(static_cast<std::uint64_t>(p.b), &h);
    mix_u64(static_cast<std::uint64_t>(p.c), &h);
    mix_u64(static_cast<std::uint64_t>(p.segment), &h);
    mix_u64(p.detail.size(), &h);
    for (const auto ch : p.detail) {
      mix_u64(static_cast<std::uint64_t>(static_cast<unsigned char>(ch)), &h);
    }
  } else {
    mix_u64(0, &h);
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

  const auto trap_payload = trap_payload_summary_line(state);
  if (!trap_payload.empty()) {
    out << trap_payload << "\n";
  }

  out << "STATE_HASH 0x" << std::hex << std::setfill('0') << std::setw(16) << state_hash(state)
      << std::dec << "\n";
  return out.str();
}

std::string trap_payload_summary_line(const State& state) {
  if (!state.last_trap_payload.has_value()) {
    return {};
  }
  const auto& p = *state.last_trap_payload;
  std::ostringstream out;
  out << "TRAP_PAYLOAD"
      << " trap=" << to_string(p.trap)
      << " pc=" << p.pc
      << " opcode=" << static_cast<std::uint64_t>(p.opcode)
      << " a=" << p.a
      << " b=" << p.b
      << " c=" << p.c
      << " segment=" << to_string(p.segment)
      << " detail=\"" << escape_payload_detail(p.detail) << "\"";
  return out.str();
}

}  // namespace t81::vm
