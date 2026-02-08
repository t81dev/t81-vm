#include "t81/vm/vm.hpp"

#include <cstddef>
#include <expected>
#include <optional>

#include "t81/vm/loader.hpp"

namespace t81::vm {
namespace {

class Interpreter final : public IVirtualMachine {
 public:
  void load_program(const t81::tisc::Program& program) override {
    const auto loaded = load_program_image(program);
    program_ = loaded.program;
    state_ = loaded.initial_state;
    preload_trap_ = loaded.preload_trap;
    steps_ = 0;
  }

  std::expected<void, Trap> step() override {
    if (state_.halted) {
      return {};
    }
    if (preload_trap_.has_value()) {
      return trap(*preload_trap_, current_opcode(), state_.pc);
    }
    if (state_.pc >= program_.insns.size()) {
      return trap(Trap::DecodeFault, t81::tisc::Opcode::Nop, state_.pc);
    }

    const std::size_t pc = state_.pc;
    const t81::tisc::Insn insn = program_.insns[pc];

    if (++steps_ % kDeterministicGcInterval == 0) {
      ++state_.gc_cycles;
    }

    switch (insn.opcode) {
      case t81::tisc::Opcode::Nop:
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      case t81::tisc::Opcode::Halt:
        state_.halted = true;
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      case t81::tisc::Opcode::LoadImm:
        state_.registers[static_cast<std::size_t>(insn.a)] = insn.b;
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      case t81::tisc::Opcode::Load:
        if (!valid_mem(insn.b)) {
          return trap(Trap::DecodeFault, insn.opcode, pc);
        }
        state_.registers[static_cast<std::size_t>(insn.a)] = state_.memory[static_cast<std::size_t>(insn.b)];
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      case t81::tisc::Opcode::Store:
        if (!valid_mem(insn.a)) {
          return trap(Trap::BoundsFault, insn.opcode, pc);
        }
        state_.memory[static_cast<std::size_t>(insn.a)] = state_.registers[static_cast<std::size_t>(insn.b)];
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      case t81::tisc::Opcode::Div:
      case t81::tisc::Opcode::Mod: {
        const auto lhs = state_.registers[static_cast<std::size_t>(insn.b)];
        const auto rhs = state_.registers[static_cast<std::size_t>(insn.c)];
        if (rhs == 0) {
          return trap(Trap::DivisionFault, insn.opcode, pc);
        }
        state_.registers[static_cast<std::size_t>(insn.a)] =
            insn.opcode == t81::tisc::Opcode::Div ? lhs / rhs : lhs % rhs;
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      }
      case t81::tisc::Opcode::Jump:
        state_.pc = static_cast<std::size_t>(insn.a);
        return trace_ok(insn.opcode, pc);
      default:
        return trap(Trap::DecodeFault, insn.opcode, pc);
    }
  }

  std::expected<void, Trap> run_to_halt(std::size_t max_steps = 100000) override {
    for (std::size_t i = 0; i < max_steps; ++i) {
      auto res = step();
      if (!res.has_value()) {
        return std::unexpected(res.error());
      }
      if (state_.halted) {
        return {};
      }
    }
    return std::unexpected(Trap::TrapInstruction);
  }

  const State& state() const override { return state_; }

  void set_register(int idx, std::int64_t value, ValueTag /*tag*/) override {
    if (idx >= 0 && static_cast<std::size_t>(idx) < state_.registers.size()) {
      state_.registers[static_cast<std::size_t>(idx)] = value;
    }
  }

 private:
  static constexpr std::size_t kDeterministicGcInterval = 64;

  bool valid_mem(std::int64_t idx) const {
    return idx >= 0 && static_cast<std::size_t>(idx) < state_.memory.size();
  }

  t81::tisc::Opcode current_opcode() const {
    if (state_.pc < program_.insns.size()) {
      return program_.insns[state_.pc].opcode;
    }
    return t81::tisc::Opcode::Nop;
  }

  std::expected<void, Trap> trace_ok(t81::tisc::Opcode opcode, std::size_t pc) {
    state_.trace.push_back(TraceEntry{pc, opcode, std::nullopt});
    return {};
  }

  std::expected<void, Trap> trap(Trap trap_code, t81::tisc::Opcode opcode, std::size_t pc) {
    state_.trace.push_back(TraceEntry{pc, opcode, trap_code});
    preload_trap_.reset();
    return std::unexpected(trap_code);
  }

  t81::tisc::Program program_;
  State state_;
  std::optional<Trap> preload_trap_;
  std::size_t steps_ = 0;
};

}  // namespace

std::unique_ptr<IVirtualMachine> make_interpreter_vm() {
  return std::make_unique<Interpreter>();
}

}  // namespace t81::vm
