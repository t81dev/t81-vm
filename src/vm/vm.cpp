#include "t81/vm/vm.hpp"

#include <cstddef>
#include <expected>
#include <optional>
#include <string>

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
    call_stack_.clear();
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

    auto set_flags = [this](std::int64_t value) {
      state_.flags.zero = (value == 0);
      state_.flags.negative = (value < 0);
      state_.flags.positive = (value > 0);
    };

    auto check_jump_target = [this, insn, pc](std::int64_t target) -> std::expected<void, Trap> {
      if (target < 0 || static_cast<std::size_t>(target) >= program_.insns.size()) {
        return trap(Trap::DecodeFault, insn.opcode, pc);
      }
      state_.pc = static_cast<std::size_t>(target);
      return trace_ok(insn.opcode, pc);
    };

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
        set_flags(insn.b);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      case t81::tisc::Opcode::Load:
        if (!valid_mem(insn.b)) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Unknown, insn.b, "memory load");
          return trap(Trap::BoundsFault, insn.opcode, pc);
        }
        state_.registers[static_cast<std::size_t>(insn.a)] = state_.memory[static_cast<std::size_t>(insn.b)];
        set_flags(state_.registers[static_cast<std::size_t>(insn.a)]);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      case t81::tisc::Opcode::Store:
        if (!valid_mem(insn.a)) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Unknown, insn.a, "memory store");
          return trap(Trap::BoundsFault, insn.opcode, pc);
        }
        state_.memory[static_cast<std::size_t>(insn.a)] = state_.registers[static_cast<std::size_t>(insn.b)];
        log_segment_event(insn.opcode, segment_of(static_cast<std::size_t>(insn.a)));
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      case t81::tisc::Opcode::Add:
      case t81::tisc::Opcode::Sub:
      case t81::tisc::Opcode::Mul:
      case t81::tisc::Opcode::Div:
      case t81::tisc::Opcode::Mod:
      case t81::tisc::Opcode::FAdd:
      case t81::tisc::Opcode::FSub:
      case t81::tisc::Opcode::FMul:
      case t81::tisc::Opcode::FDiv:
      case t81::tisc::Opcode::FracAdd:
      case t81::tisc::Opcode::FracSub:
      case t81::tisc::Opcode::FracMul:
      case t81::tisc::Opcode::FracDiv: {
        const auto lhs = state_.registers[static_cast<std::size_t>(insn.b)];
        const auto rhs = state_.registers[static_cast<std::size_t>(insn.c)];
        if ((insn.opcode == t81::tisc::Opcode::Div || insn.opcode == t81::tisc::Opcode::Mod ||
             insn.opcode == t81::tisc::Opcode::FDiv || insn.opcode == t81::tisc::Opcode::FracDiv) &&
            rhs == 0) {
          return trap(Trap::DivisionFault, insn.opcode, pc);
        }
        std::int64_t result = 0;
        if (insn.opcode == t81::tisc::Opcode::Add || insn.opcode == t81::tisc::Opcode::FAdd ||
            insn.opcode == t81::tisc::Opcode::FracAdd) {
          result = lhs + rhs;
        } else if (insn.opcode == t81::tisc::Opcode::Sub || insn.opcode == t81::tisc::Opcode::FSub ||
                   insn.opcode == t81::tisc::Opcode::FracSub) {
          result = lhs - rhs;
        } else if (insn.opcode == t81::tisc::Opcode::Mul || insn.opcode == t81::tisc::Opcode::FMul ||
                   insn.opcode == t81::tisc::Opcode::FracMul) {
          result = lhs * rhs;
        } else if (insn.opcode == t81::tisc::Opcode::Div || insn.opcode == t81::tisc::Opcode::FDiv ||
                   insn.opcode == t81::tisc::Opcode::FracDiv) {
          result = lhs / rhs;
        } else {
          result = lhs % rhs;
        }
        state_.registers[static_cast<std::size_t>(insn.a)] = result;
        set_flags(result);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      }
      case t81::tisc::Opcode::Mov:
        state_.registers[static_cast<std::size_t>(insn.a)] = state_.registers[static_cast<std::size_t>(insn.b)];
        set_flags(state_.registers[static_cast<std::size_t>(insn.a)]);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      case t81::tisc::Opcode::Inc:
        ++state_.registers[static_cast<std::size_t>(insn.a)];
        set_flags(state_.registers[static_cast<std::size_t>(insn.a)]);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      case t81::tisc::Opcode::Dec:
        --state_.registers[static_cast<std::size_t>(insn.a)];
        set_flags(state_.registers[static_cast<std::size_t>(insn.a)]);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      case t81::tisc::Opcode::Cmp: {
        const auto lhs = state_.registers[static_cast<std::size_t>(insn.a)];
        const auto rhs = state_.registers[static_cast<std::size_t>(insn.b)];
        set_flags(lhs - rhs);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      }
      case t81::tisc::Opcode::Neg:
        state_.registers[static_cast<std::size_t>(insn.a)] = -state_.registers[static_cast<std::size_t>(insn.b)];
        set_flags(state_.registers[static_cast<std::size_t>(insn.a)]);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      case t81::tisc::Opcode::I2F:
      case t81::tisc::Opcode::F2I:
      case t81::tisc::Opcode::I2Frac:
      case t81::tisc::Opcode::Frac2I:
        // This VM currently models scalar registers in deterministic int64 space.
        // Conversion ops are represented as canonical scalar moves.
        state_.registers[static_cast<std::size_t>(insn.a)] = state_.registers[static_cast<std::size_t>(insn.b)];
        set_flags(state_.registers[static_cast<std::size_t>(insn.a)]);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      case t81::tisc::Opcode::Less:
      case t81::tisc::Opcode::LessEqual:
      case t81::tisc::Opcode::Greater:
      case t81::tisc::Opcode::GreaterEqual:
      case t81::tisc::Opcode::Equal:
      case t81::tisc::Opcode::NotEqual: {
        const auto lhs = state_.registers[static_cast<std::size_t>(insn.b)];
        const auto rhs = state_.registers[static_cast<std::size_t>(insn.c)];
        bool result = false;
        if (insn.opcode == t81::tisc::Opcode::Less) {
          result = lhs < rhs;
        } else if (insn.opcode == t81::tisc::Opcode::LessEqual) {
          result = lhs <= rhs;
        } else if (insn.opcode == t81::tisc::Opcode::Greater) {
          result = lhs > rhs;
        } else if (insn.opcode == t81::tisc::Opcode::GreaterEqual) {
          result = lhs >= rhs;
        } else if (insn.opcode == t81::tisc::Opcode::Equal) {
          result = lhs == rhs;
        } else {
          result = lhs != rhs;
        }
        state_.registers[static_cast<std::size_t>(insn.a)] = result ? 1 : 0;
        set_flags(state_.registers[static_cast<std::size_t>(insn.a)]);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      }
      case t81::tisc::Opcode::Push:
        if (!push_register(static_cast<std::size_t>(insn.a))) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Stack, static_cast<std::int64_t>(state_.sp),
                           "stack push");
          return trap(Trap::StackFault, insn.opcode, pc);
        }
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      case t81::tisc::Opcode::Pop:
        if (!pop_register(static_cast<std::size_t>(insn.a))) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Stack, static_cast<std::int64_t>(state_.sp),
                           "stack pop");
          return trap(Trap::StackFault, insn.opcode, pc);
        }
        set_flags(state_.registers[static_cast<std::size_t>(insn.a)]);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      case t81::tisc::Opcode::Jump:
        return check_jump_target(insn.a);
      case t81::tisc::Opcode::JumpIfZero:
        if (state_.flags.zero) {
          return check_jump_target(insn.a);
        }
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      case t81::tisc::Opcode::JumpIfNotZero:
        if (!state_.flags.zero) {
          return check_jump_target(insn.a);
        }
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      case t81::tisc::Opcode::JumpIfNegative:
        if (state_.flags.negative) {
          return check_jump_target(insn.a);
        }
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      case t81::tisc::Opcode::JumpIfPositive:
        if (state_.flags.positive) {
          return check_jump_target(insn.a);
        }
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      case t81::tisc::Opcode::Call: {
        const auto target = state_.registers[static_cast<std::size_t>(insn.a)];
        if (target < 0 || static_cast<std::size_t>(target) >= program_.insns.size()) {
          return trap(Trap::DecodeFault, insn.opcode, pc);
        }
        call_stack_.push_back(pc + 1);
        state_.pc = static_cast<std::size_t>(target);
        return trace_ok(insn.opcode, pc);
      }
      case t81::tisc::Opcode::Ret:
        if (call_stack_.empty()) {
          return trap(Trap::StackFault, insn.opcode, pc);
        }
        state_.pc = call_stack_.back();
        call_stack_.pop_back();
        return trace_ok(insn.opcode, pc);
      case t81::tisc::Opcode::Trap:
        return trap(Trap::TrapInstruction, insn.opcode, pc);
      case t81::tisc::Opcode::StackAlloc: {
        if (insn.b <= 0) {
          return trap(Trap::StackFault, insn.opcode, pc);
        }
        const std::size_t bytes = static_cast<std::size_t>(insn.b);
        if (bytes > state_.sp || state_.sp - bytes < state_.layout.stack.start) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Stack, insn.b, "stack frame allocate");
          return trap(Trap::StackFault, insn.opcode, pc);
        }
        state_.sp -= bytes;
        state_.stack_frames.push_back({state_.sp, bytes});
        state_.registers[static_cast<std::size_t>(insn.a)] = static_cast<std::int64_t>(state_.sp);
        state_.axion_log.push_back({insn.opcode, "stack frame allocated"});
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      }
      case t81::tisc::Opcode::StackFree: {
        if (state_.stack_frames.empty()) {
          return trap(Trap::StackFault, insn.opcode, pc);
        }
        const auto top = state_.stack_frames.back();
        const auto expected = static_cast<std::size_t>(state_.registers[static_cast<std::size_t>(insn.a)]);
        const auto requested = insn.b > 0 ? static_cast<std::size_t>(insn.b) : 0;
        if (expected != top.first || requested != top.second) {
          return trap(Trap::StackFault, insn.opcode, pc);
        }
        state_.stack_frames.pop_back();
        state_.sp += top.second;
        state_.axion_log.push_back({insn.opcode, "stack frame freed"});
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      }
      case t81::tisc::Opcode::HeapAlloc: {
        if (insn.b <= 0) {
          return trap(Trap::DecodeFault, insn.opcode, pc);
        }
        const std::size_t bytes = static_cast<std::size_t>(insn.b);
        if (state_.heap_ptr + bytes > state_.layout.heap.limit) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Heap, insn.b, "heap block allocate");
          return trap(Trap::BoundsFault, insn.opcode, pc);
        }
        const std::size_t addr = state_.heap_ptr;
        state_.heap_ptr += bytes;
        state_.heap_frames.push_back({addr, bytes});
        state_.registers[static_cast<std::size_t>(insn.a)] = static_cast<std::int64_t>(addr);
        state_.axion_log.push_back({insn.opcode, "heap block allocated"});
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      }
      case t81::tisc::Opcode::HeapFree: {
        if (state_.heap_frames.empty()) {
          return trap(Trap::DecodeFault, insn.opcode, pc);
        }
        const auto top = state_.heap_frames.back();
        const auto expected = static_cast<std::size_t>(state_.registers[static_cast<std::size_t>(insn.a)]);
        const auto requested = insn.b > 0 ? static_cast<std::size_t>(insn.b) : 0;
        if (expected != top.first || requested != top.second) {
          return trap(Trap::DecodeFault, insn.opcode, pc);
        }
        state_.heap_frames.pop_back();
        state_.heap_ptr = top.first;
        state_.axion_log.push_back({insn.opcode, "heap block freed"});
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      }
    }

    return trap(Trap::DecodeFault, insn.opcode, pc);
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
    if (idx < 0) {
      return false;
    }
    const std::size_t addr = static_cast<std::size_t>(idx);
    if (addr >= state_.memory.size()) {
      return false;
    }
    return segment_of(addr) != MemorySegmentKind::Unknown;
  }

  MemorySegmentKind segment_of(std::size_t addr) const {
    if (state_.layout.code.contains(addr)) return MemorySegmentKind::Code;
    if (state_.layout.stack.contains(addr)) return MemorySegmentKind::Stack;
    if (state_.layout.heap.contains(addr)) return MemorySegmentKind::Heap;
    if (state_.layout.tensor.contains(addr)) return MemorySegmentKind::Tensor;
    if (state_.layout.meta.contains(addr)) return MemorySegmentKind::Meta;
    return MemorySegmentKind::Unknown;
  }

  void log_segment_event(t81::tisc::Opcode opcode, MemorySegmentKind kind) {
    if (kind == MemorySegmentKind::Unknown) {
      return;
    }
    state_.axion_log.push_back({opcode, std::string("segment access ") + to_string(kind)});
  }

  void log_bounds_fault(t81::tisc::Opcode opcode, MemorySegmentKind segment, std::int64_t addr, std::string action) {
    std::string reason = "bounds fault segment=";
    reason += to_string(segment);
    reason += " addr=";
    reason += std::to_string(addr);
    reason += " action=";
    reason += action;
    state_.axion_log.push_back({opcode, reason});
  }

  bool push_register(std::size_t reg_index) {
    if (state_.sp == 0 || state_.sp - 1 < state_.layout.stack.start) {
      return false;
    }
    --state_.sp;
    state_.memory[state_.sp] = state_.registers[reg_index];
    return true;
  }

  bool pop_register(std::size_t reg_index) {
    if (state_.sp >= state_.layout.stack.limit) {
      return false;
    }
    state_.registers[reg_index] = state_.memory[state_.sp];
    ++state_.sp;
    return true;
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
  std::vector<std::size_t> call_stack_;
};

}  // namespace

std::unique_ptr<IVirtualMachine> make_interpreter_vm() {
  return std::make_unique<Interpreter>();
}

}  // namespace t81::vm
