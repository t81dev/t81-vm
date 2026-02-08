#include "t81/vm/vm.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <expected>
#include <optional>
#include <string>
#include <vector>

#include "t81/vm/loader.hpp"

namespace t81::vm {
namespace {

class Interpreter final : public IVirtualMachine {
 public:
  void load_program(const t81::tisc::Program& program) override {
    const auto loaded = load_program_image(program);
    program_ = loaded.program;
    state_ = loaded.initial_state;
    state_.register_tags.fill(ValueTag::Int);
    state_.option_pool.clear();
    state_.result_pool.clear();
    state_.enum_pool.clear();
    state_.tensor_pool.clear();
    state_.shape_pool.clear();
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
        set_register_value(static_cast<std::size_t>(insn.a), insn.b, ValueTag::Int);
        set_flags(insn.b);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      case t81::tisc::Opcode::Load:
        if (!valid_mem(insn.b)) {
          log_bounds_fault(insn.opcode, MemorySegmentKind::Unknown, insn.b, "memory load");
          return trap(Trap::BoundsFault, insn.opcode, pc);
        }
        set_register_value(static_cast<std::size_t>(insn.a), state_.memory[static_cast<std::size_t>(insn.b)],
                           ValueTag::Int);
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
        set_register_value(static_cast<std::size_t>(insn.a), result, ValueTag::Int);
        set_flags(result);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      }
      case t81::tisc::Opcode::Mov:
        set_register_value(static_cast<std::size_t>(insn.a), state_.registers[static_cast<std::size_t>(insn.b)],
                           state_.register_tags[static_cast<std::size_t>(insn.b)]);
        set_flags(state_.registers[static_cast<std::size_t>(insn.a)]);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      case t81::tisc::Opcode::Inc:
        ++state_.registers[static_cast<std::size_t>(insn.a)];
        state_.register_tags[static_cast<std::size_t>(insn.a)] = ValueTag::Int;
        set_flags(state_.registers[static_cast<std::size_t>(insn.a)]);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      case t81::tisc::Opcode::Dec:
        --state_.registers[static_cast<std::size_t>(insn.a)];
        state_.register_tags[static_cast<std::size_t>(insn.a)] = ValueTag::Int;
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
        set_register_value(static_cast<std::size_t>(insn.a), -state_.registers[static_cast<std::size_t>(insn.b)],
                           ValueTag::Int);
        set_flags(state_.registers[static_cast<std::size_t>(insn.a)]);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      case t81::tisc::Opcode::TNot: {
        const auto t = clamp_trit(state_.registers[static_cast<std::size_t>(insn.b)]);
        set_register_value(static_cast<std::size_t>(insn.a), -t, ValueTag::Int);
        set_flags(state_.registers[static_cast<std::size_t>(insn.a)]);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      }
      case t81::tisc::Opcode::TAnd:
      case t81::tisc::Opcode::TOr:
      case t81::tisc::Opcode::TXor: {
        const auto lhs = clamp_trit(state_.registers[static_cast<std::size_t>(insn.b)]);
        const auto rhs = clamp_trit(state_.registers[static_cast<std::size_t>(insn.c)]);
        std::int64_t result = 0;
        if (insn.opcode == t81::tisc::Opcode::TAnd) {
          result = lhs < rhs ? lhs : rhs;
        } else if (insn.opcode == t81::tisc::Opcode::TOr) {
          result = lhs > rhs ? lhs : rhs;
        } else {
          result = lhs - rhs;
          if (result > 1) result = -1;
          if (result < -1) result = 1;
        }
        set_register_value(static_cast<std::size_t>(insn.a), result, ValueTag::Int);
        set_flags(state_.registers[static_cast<std::size_t>(insn.a)]);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      }
      case t81::tisc::Opcode::I2F:
      case t81::tisc::Opcode::F2I:
      case t81::tisc::Opcode::I2Frac:
      case t81::tisc::Opcode::Frac2I:
        // This VM currently models scalar registers in deterministic int64 space.
        // Conversion ops are represented as canonical scalar moves.
        set_register_value(static_cast<std::size_t>(insn.a), state_.registers[static_cast<std::size_t>(insn.b)],
                           state_.register_tags[static_cast<std::size_t>(insn.b)]);
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
        set_register_value(static_cast<std::size_t>(insn.a), result ? 1 : 0, ValueTag::Int);
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
      case t81::tisc::Opcode::AxRead: {
        const auto guard_addr = insn.b;
        const auto guard_kind = guard_addr >= 0 ? segment_of(static_cast<std::size_t>(guard_addr))
                                                : MemorySegmentKind::Unknown;
        const auto denied = axion_denied();
        log_axion_guard(insn.opcode, "AxRead guard", guard_kind, guard_addr, denied);
        if (denied) {
          return trap(Trap::SecurityFault, insn.opcode, pc);
        }
        set_register_value(static_cast<std::size_t>(insn.a), insn.b, ValueTag::Int);
        set_flags(state_.registers[static_cast<std::size_t>(insn.a)]);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      }
      case t81::tisc::Opcode::AxSet: {
        const auto value = state_.registers[static_cast<std::size_t>(insn.b)];
        const auto guard_addr = state_.registers[static_cast<std::size_t>(insn.a)];
        const auto guard_kind = guard_addr >= 0 ? segment_of(static_cast<std::size_t>(guard_addr))
                                                : MemorySegmentKind::Unknown;
        const auto denied = axion_denied();
        log_axion_guard(insn.opcode, "AxSet guard", guard_kind, guard_addr, denied, value);
        if (denied) {
          return trap(Trap::SecurityFault, insn.opcode, pc);
        }
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      }
      case t81::tisc::Opcode::AxVerify: {
        const auto denied = axion_denied();
        log_axion_guard(insn.opcode, "AxVerify guard", MemorySegmentKind::Meta, static_cast<std::int64_t>(pc), denied);
        if (denied) {
          return trap(Trap::SecurityFault, insn.opcode, pc);
        }
        set_register_value(static_cast<std::size_t>(insn.a), 0, ValueTag::Int);
        set_flags(state_.registers[static_cast<std::size_t>(insn.a)]);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      }
      case t81::tisc::Opcode::TVecAdd:
      case t81::tisc::Opcode::TVecMul: {
        if (state_.register_tags[static_cast<std::size_t>(insn.b)] != ValueTag::TensorHandle ||
            state_.register_tags[static_cast<std::size_t>(insn.c)] != ValueTag::TensorHandle) {
          return trap(Trap::TypeFault, insn.opcode, pc);
        }
        const auto* lhs = tensor_ptr(state_.registers[static_cast<std::size_t>(insn.b)]);
        const auto* rhs = tensor_ptr(state_.registers[static_cast<std::size_t>(insn.c)]);
        if (lhs == nullptr || rhs == nullptr) {
          return trap(Trap::DecodeFault, insn.opcode, pc);
        }
        if (lhs->shape.size() != 1 || rhs->shape.size() != 1 || lhs->shape != rhs->shape ||
            lhs->data.size() != rhs->data.size()) {
          return trap(Trap::ShapeFault, insn.opcode, pc);
        }
        std::vector<std::int64_t> out(lhs->data.size(), 0);
        for (std::size_t i = 0; i < out.size(); ++i) {
          out[i] = insn.opcode == t81::tisc::Opcode::TVecAdd ? lhs->data[i] + rhs->data[i] : lhs->data[i] * rhs->data[i];
        }
        const auto handle = intern_tensor(lhs->shape, out);
        set_register_value(static_cast<std::size_t>(insn.a), handle, ValueTag::TensorHandle);
        set_flags(state_.registers[static_cast<std::size_t>(insn.a)]);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      }
      case t81::tisc::Opcode::TMatMul: {
        if (state_.register_tags[static_cast<std::size_t>(insn.b)] != ValueTag::TensorHandle ||
            state_.register_tags[static_cast<std::size_t>(insn.c)] != ValueTag::TensorHandle) {
          return trap(Trap::TypeFault, insn.opcode, pc);
        }
        const auto* lhs = tensor_ptr(state_.registers[static_cast<std::size_t>(insn.b)]);
        const auto* rhs = tensor_ptr(state_.registers[static_cast<std::size_t>(insn.c)]);
        if (lhs == nullptr || rhs == nullptr) {
          return trap(Trap::DecodeFault, insn.opcode, pc);
        }
        if (lhs->shape.size() != 2 || rhs->shape.size() != 2) {
          return trap(Trap::ShapeFault, insn.opcode, pc);
        }
        const auto rows = static_cast<std::size_t>(lhs->shape[0]);
        const auto inner = static_cast<std::size_t>(lhs->shape[1]);
        const auto rhs_inner = static_cast<std::size_t>(rhs->shape[0]);
        const auto cols = static_cast<std::size_t>(rhs->shape[1]);
        if (inner != rhs_inner || lhs->data.size() != rows * inner || rhs->data.size() != rhs_inner * cols) {
          return trap(Trap::ShapeFault, insn.opcode, pc);
        }
        std::vector<std::int64_t> out(rows * cols, 0);
        for (std::size_t r = 0; r < rows; ++r) {
          for (std::size_t c = 0; c < cols; ++c) {
            std::int64_t sum = 0;
            for (std::size_t k = 0; k < inner; ++k) {
              sum += lhs->data[r * inner + k] * rhs->data[k * cols + c];
            }
            out[r * cols + c] = sum;
          }
        }
        const auto handle = intern_tensor({static_cast<std::int64_t>(rows), static_cast<std::int64_t>(cols)}, out);
        set_register_value(static_cast<std::size_t>(insn.a), handle, ValueTag::TensorHandle);
        set_flags(state_.registers[static_cast<std::size_t>(insn.a)]);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      }
      case t81::tisc::Opcode::TTenDot: {
        if (state_.register_tags[static_cast<std::size_t>(insn.b)] != ValueTag::TensorHandle ||
            state_.register_tags[static_cast<std::size_t>(insn.c)] != ValueTag::TensorHandle) {
          return trap(Trap::TypeFault, insn.opcode, pc);
        }
        const auto* lhs = tensor_ptr(state_.registers[static_cast<std::size_t>(insn.b)]);
        const auto* rhs = tensor_ptr(state_.registers[static_cast<std::size_t>(insn.c)]);
        if (lhs == nullptr || rhs == nullptr) {
          return trap(Trap::DecodeFault, insn.opcode, pc);
        }
        if (lhs->data.size() != rhs->data.size()) {
          return trap(Trap::ShapeFault, insn.opcode, pc);
        }
        std::int64_t sum = 0;
        for (std::size_t i = 0; i < lhs->data.size(); ++i) {
          sum += lhs->data[i] * rhs->data[i];
        }
        const auto handle = intern_tensor({1}, {sum});
        set_register_value(static_cast<std::size_t>(insn.a), handle, ValueTag::TensorHandle);
        set_flags(state_.registers[static_cast<std::size_t>(insn.a)]);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      }
      case t81::tisc::Opcode::TTranspose: {
        if (state_.register_tags[static_cast<std::size_t>(insn.b)] != ValueTag::TensorHandle) {
          return trap(Trap::TypeFault, insn.opcode, pc);
        }
        const auto* in = tensor_ptr(state_.registers[static_cast<std::size_t>(insn.b)]);
        if (in == nullptr) {
          return trap(Trap::DecodeFault, insn.opcode, pc);
        }
        if (in->shape.size() != 2) {
          return trap(Trap::ShapeFault, insn.opcode, pc);
        }
        const auto rows = static_cast<std::size_t>(in->shape[0]);
        const auto cols = static_cast<std::size_t>(in->shape[1]);
        if (in->data.size() != rows * cols) {
          return trap(Trap::ShapeFault, insn.opcode, pc);
        }
        std::vector<std::int64_t> out(in->data.size(), 0);
        for (std::size_t r = 0; r < rows; ++r) {
          for (std::size_t c = 0; c < cols; ++c) {
            out[c * rows + r] = in->data[r * cols + c];
          }
        }
        const auto handle = intern_tensor({static_cast<std::int64_t>(cols), static_cast<std::int64_t>(rows)}, out);
        set_register_value(static_cast<std::size_t>(insn.a), handle, ValueTag::TensorHandle);
        set_flags(state_.registers[static_cast<std::size_t>(insn.a)]);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      }
      case t81::tisc::Opcode::TExp:
      case t81::tisc::Opcode::TSqrt:
      case t81::tisc::Opcode::TSiLU:
      case t81::tisc::Opcode::TSoftmax:
      case t81::tisc::Opcode::TRMSNorm:
      case t81::tisc::Opcode::TRoPE: {
        if (state_.register_tags[static_cast<std::size_t>(insn.b)] != ValueTag::TensorHandle) {
          return trap(Trap::TypeFault, insn.opcode, pc);
        }
        const auto* in = tensor_ptr(state_.registers[static_cast<std::size_t>(insn.b)]);
        if (in == nullptr) {
          return trap(Trap::DecodeFault, insn.opcode, pc);
        }
        std::vector<std::int64_t> out = in->data;
        if (insn.opcode == t81::tisc::Opcode::TExp) {
          for (auto& v : out) {
            const auto clamped = std::clamp<std::int64_t>(v, -20, 20);
            v = static_cast<std::int64_t>(std::llround(std::exp(static_cast<double>(clamped))));
          }
        } else if (insn.opcode == t81::tisc::Opcode::TSqrt) {
          for (auto& v : out) {
            const auto non_neg = v < 0 ? 0 : v;
            v = static_cast<std::int64_t>(std::llround(std::sqrt(static_cast<double>(non_neg))));
          }
        } else if (insn.opcode == t81::tisc::Opcode::TSiLU) {
          for (auto& v : out) {
            const double x = static_cast<double>(v);
            const double sig = 1.0 / (1.0 + std::exp(-x));
            v = static_cast<std::int64_t>(std::llround(x * sig));
          }
        } else if (insn.opcode == t81::tisc::Opcode::TSoftmax) {
          if (out.empty()) {
            return trap(Trap::ShapeFault, insn.opcode, pc);
          }
          const auto max_it = std::max_element(out.begin(), out.end());
          const double max_v = static_cast<double>(*max_it);
          std::vector<double> exps(out.size(), 0.0);
          double sum = 0.0;
          for (std::size_t i = 0; i < out.size(); ++i) {
            exps[i] = std::exp(static_cast<double>(out[i]) - max_v);
            sum += exps[i];
          }
          if (sum == 0.0) {
            return trap(Trap::ShapeFault, insn.opcode, pc);
          }
          for (std::size_t i = 0; i < out.size(); ++i) {
            out[i] = static_cast<std::int64_t>(std::llround((exps[i] / sum) * 1000.0));
          }
        } else if (insn.opcode == t81::tisc::Opcode::TRMSNorm) {
          if (out.empty()) {
            return trap(Trap::ShapeFault, insn.opcode, pc);
          }
          double mean_sq = 0.0;
          for (const auto v : out) {
            const double d = static_cast<double>(v);
            mean_sq += d * d;
          }
          mean_sq /= static_cast<double>(out.size());
          const double rms = std::sqrt(mean_sq);
          if (rms == 0.0) {
            std::fill(out.begin(), out.end(), 0);
          } else {
            for (auto& v : out) {
              v = static_cast<std::int64_t>(std::llround(static_cast<double>(v) / rms));
            }
          }
        } else if (insn.opcode == t81::tisc::Opcode::TRoPE) {
          if (out.size() % 2 != 0) {
            return trap(Trap::ShapeFault, insn.opcode, pc);
          }
          for (std::size_t i = 0; i + 1 < out.size(); i += 2) {
            const auto x = out[i];
            const auto y = out[i + 1];
            out[i] = y;
            out[i + 1] = -x;
          }
        }
        const auto handle = intern_tensor(in->shape, out);
        set_register_value(static_cast<std::size_t>(insn.a), handle, ValueTag::TensorHandle);
        set_flags(state_.registers[static_cast<std::size_t>(insn.a)]);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      }
      case t81::tisc::Opcode::ChkShape: {
        if (state_.register_tags[static_cast<std::size_t>(insn.b)] != ValueTag::TensorHandle ||
            state_.register_tags[static_cast<std::size_t>(insn.c)] != ValueTag::ShapeHandle) {
          return trap(Trap::TypeFault, insn.opcode, pc);
        }
        const auto* tensor = tensor_ptr(state_.registers[static_cast<std::size_t>(insn.b)]);
        const auto* shape = shape_ptr(state_.registers[static_cast<std::size_t>(insn.c)]);
        if (tensor == nullptr || shape == nullptr) {
          return trap(Trap::DecodeFault, insn.opcode, pc);
        }
        const auto match = tensor->shape == *shape;
        set_register_value(static_cast<std::size_t>(insn.a), match ? 1 : 0, ValueTag::Int);
        set_flags(state_.registers[static_cast<std::size_t>(insn.a)]);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      }
      case t81::tisc::Opcode::WeightsLoad: {
        const auto handle = insn.b > 0 ? insn.b : (1000 + static_cast<std::int64_t>(pc));
        set_register_value(static_cast<std::size_t>(insn.a), handle, ValueTag::WeightsTensorHandle);
        state_.axion_log.push_back({insn.opcode, "weights handle loaded"});
        set_flags(state_.registers[static_cast<std::size_t>(insn.a)]);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      }
      case t81::tisc::Opcode::SetF:
        set_register_value(static_cast<std::size_t>(insn.a), state_.registers[static_cast<std::size_t>(insn.b)],
                           ValueTag::Int);
        set_flags(state_.registers[static_cast<std::size_t>(insn.a)]);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      case t81::tisc::Opcode::MakeOptionSome: {
        const auto handle = intern_option(true, state_.register_tags[static_cast<std::size_t>(insn.b)],
                                          state_.registers[static_cast<std::size_t>(insn.b)]);
        set_register_value(static_cast<std::size_t>(insn.a), handle, ValueTag::OptionHandle);
        set_flags(state_.registers[static_cast<std::size_t>(insn.a)]);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      }
      case t81::tisc::Opcode::MakeOptionNone: {
        const auto handle = intern_option(false, ValueTag::Int, 0);
        set_register_value(static_cast<std::size_t>(insn.a), handle, ValueTag::OptionHandle);
        set_flags(state_.registers[static_cast<std::size_t>(insn.a)]);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      }
      case t81::tisc::Opcode::MakeResultOk: {
        const auto handle = intern_result(true, state_.register_tags[static_cast<std::size_t>(insn.b)],
                                          state_.registers[static_cast<std::size_t>(insn.b)]);
        set_register_value(static_cast<std::size_t>(insn.a), handle, ValueTag::ResultHandle);
        set_flags(state_.registers[static_cast<std::size_t>(insn.a)]);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      }
      case t81::tisc::Opcode::MakeResultErr: {
        const auto handle = intern_result(false, state_.register_tags[static_cast<std::size_t>(insn.b)],
                                          state_.registers[static_cast<std::size_t>(insn.b)]);
        set_register_value(static_cast<std::size_t>(insn.a), handle, ValueTag::ResultHandle);
        set_flags(state_.registers[static_cast<std::size_t>(insn.a)]);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      }
      case t81::tisc::Opcode::OptionIsSome: {
        if (state_.register_tags[static_cast<std::size_t>(insn.b)] != ValueTag::OptionHandle) {
          return trap(Trap::TypeFault, insn.opcode, pc);
        }
        auto* option = option_ptr(state_.registers[static_cast<std::size_t>(insn.b)]);
        if (option == nullptr) {
          return trap(Trap::DecodeFault, insn.opcode, pc);
        }
        set_register_value(static_cast<std::size_t>(insn.a), option->has_value ? 1 : 0, ValueTag::Int);
        set_flags(state_.registers[static_cast<std::size_t>(insn.a)]);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      }
      case t81::tisc::Opcode::OptionUnwrap: {
        if (state_.register_tags[static_cast<std::size_t>(insn.b)] != ValueTag::OptionHandle) {
          return trap(Trap::TypeFault, insn.opcode, pc);
        }
        auto* option = option_ptr(state_.registers[static_cast<std::size_t>(insn.b)]);
        if (option == nullptr || !option->has_value) {
          return trap(Trap::DecodeFault, insn.opcode, pc);
        }
        set_register_value(static_cast<std::size_t>(insn.a), option->payload, option->payload_tag);
        set_flags(state_.registers[static_cast<std::size_t>(insn.a)]);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      }
      case t81::tisc::Opcode::ResultIsOk: {
        if (state_.register_tags[static_cast<std::size_t>(insn.b)] != ValueTag::ResultHandle) {
          return trap(Trap::TypeFault, insn.opcode, pc);
        }
        auto* result = result_ptr(state_.registers[static_cast<std::size_t>(insn.b)]);
        if (result == nullptr) {
          return trap(Trap::DecodeFault, insn.opcode, pc);
        }
        set_register_value(static_cast<std::size_t>(insn.a), result->is_ok ? 1 : 0, ValueTag::Int);
        set_flags(state_.registers[static_cast<std::size_t>(insn.a)]);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      }
      case t81::tisc::Opcode::ResultUnwrapOk: {
        if (state_.register_tags[static_cast<std::size_t>(insn.b)] != ValueTag::ResultHandle) {
          return trap(Trap::TypeFault, insn.opcode, pc);
        }
        auto* result = result_ptr(state_.registers[static_cast<std::size_t>(insn.b)]);
        if (result == nullptr || !result->is_ok) {
          return trap(Trap::DecodeFault, insn.opcode, pc);
        }
        set_register_value(static_cast<std::size_t>(insn.a), result->payload, result->payload_tag);
        set_flags(state_.registers[static_cast<std::size_t>(insn.a)]);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      }
      case t81::tisc::Opcode::ResultUnwrapErr: {
        if (state_.register_tags[static_cast<std::size_t>(insn.b)] != ValueTag::ResultHandle) {
          return trap(Trap::TypeFault, insn.opcode, pc);
        }
        auto* result = result_ptr(state_.registers[static_cast<std::size_t>(insn.b)]);
        if (result == nullptr || result->is_ok) {
          return trap(Trap::DecodeFault, insn.opcode, pc);
        }
        set_register_value(static_cast<std::size_t>(insn.a), result->payload, result->payload_tag);
        set_flags(state_.registers[static_cast<std::size_t>(insn.a)]);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      }
      case t81::tisc::Opcode::MakeEnumVariant: {
        const auto handle = intern_enum(insn.b, false, ValueTag::Int, 0);
        set_register_value(static_cast<std::size_t>(insn.a), handle, ValueTag::EnumHandle);
        set_flags(state_.registers[static_cast<std::size_t>(insn.a)]);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      }
      case t81::tisc::Opcode::MakeEnumVariantPayload: {
        if (insn.c < 0) {
          return trap(Trap::DecodeFault, insn.opcode, pc);
        }
        const auto handle = intern_enum(insn.c, true, state_.register_tags[static_cast<std::size_t>(insn.b)],
                                        state_.registers[static_cast<std::size_t>(insn.b)]);
        set_register_value(static_cast<std::size_t>(insn.a), handle, ValueTag::EnumHandle);
        set_flags(state_.registers[static_cast<std::size_t>(insn.a)]);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      }
      case t81::tisc::Opcode::EnumIsVariant: {
        if (state_.register_tags[static_cast<std::size_t>(insn.b)] != ValueTag::EnumHandle) {
          return trap(Trap::TypeFault, insn.opcode, pc);
        }
        auto* enum_value = enum_ptr(state_.registers[static_cast<std::size_t>(insn.b)]);
        if (enum_value == nullptr) {
          return trap(Trap::DecodeFault, insn.opcode, pc);
        }
        set_register_value(static_cast<std::size_t>(insn.a), enum_value->variant_id == insn.c ? 1 : 0, ValueTag::Int);
        set_flags(state_.registers[static_cast<std::size_t>(insn.a)]);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      }
      case t81::tisc::Opcode::EnumUnwrapPayload: {
        if (state_.register_tags[static_cast<std::size_t>(insn.b)] != ValueTag::EnumHandle) {
          return trap(Trap::TypeFault, insn.opcode, pc);
        }
        auto* enum_value = enum_ptr(state_.registers[static_cast<std::size_t>(insn.b)]);
        if (enum_value == nullptr || !enum_value->has_payload) {
          return trap(Trap::DecodeFault, insn.opcode, pc);
        }
        set_register_value(static_cast<std::size_t>(insn.a), enum_value->payload, enum_value->payload_tag);
        set_flags(state_.registers[static_cast<std::size_t>(insn.a)]);
        ++state_.pc;
        return trace_ok(insn.opcode, pc);
      }
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
        set_register_value(static_cast<std::size_t>(insn.a), static_cast<std::int64_t>(state_.sp), ValueTag::Int);
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
        set_register_value(static_cast<std::size_t>(insn.a), static_cast<std::int64_t>(addr), ValueTag::Int);
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

  void set_register(int idx, std::int64_t value, ValueTag tag) override {
    if (idx >= 0 && static_cast<std::size_t>(idx) < state_.registers.size()) {
      set_register_value(static_cast<std::size_t>(idx), value, tag);
    }
  }

 private:
  static constexpr std::size_t kDeterministicGcInterval = 64;
  static constexpr std::int64_t kTritMax = 1;
  static constexpr std::int64_t kTritMin = -1;

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
    state_.register_tags[reg_index] = ValueTag::Int;
    ++state_.sp;
    return true;
  }

  void set_register_value(std::size_t reg_index, std::int64_t value, ValueTag tag) {
    state_.registers[reg_index] = value;
    state_.register_tags[reg_index] = tag;
  }

  static std::int64_t clamp_trit(std::int64_t value) {
    if (value > kTritMax) {
      return kTritMax;
    }
    if (value < kTritMin) {
      return kTritMin;
    }
    return value;
  }

  bool axion_denied() const { return state_.policy.has_value() && state_.policy->tier == 0; }

  void log_axion_guard(t81::tisc::Opcode opcode, const char* label, MemorySegmentKind segment, std::int64_t addr,
                       bool denied, std::optional<std::int64_t> value = std::nullopt) {
    std::string reason = label;
    reason += " segment=";
    reason += to_string(segment);
    reason += " addr=";
    reason += std::to_string(addr);
    if (value.has_value()) {
      reason += " value=";
      reason += std::to_string(*value);
    }
    if (denied) {
      reason += " deny=tier0";
    } else {
      reason += " allow";
    }
    state_.axion_log.push_back({opcode, reason});
  }

  std::int64_t intern_option(bool has_value, ValueTag payload_tag, std::int64_t payload) {
    state_.option_pool.push_back(OptionValue{
        .has_value = has_value,
        .payload_tag = payload_tag,
        .payload = payload,
    });
    return static_cast<std::int64_t>(state_.option_pool.size());
  }

  std::int64_t intern_result(bool is_ok, ValueTag payload_tag, std::int64_t payload) {
    state_.result_pool.push_back(ResultValue{
        .is_ok = is_ok,
        .payload_tag = payload_tag,
        .payload = payload,
    });
    return static_cast<std::int64_t>(state_.result_pool.size());
  }

  std::int64_t intern_enum(std::int64_t variant_id, bool has_payload, ValueTag payload_tag, std::int64_t payload) {
    state_.enum_pool.push_back(EnumValue{
        .variant_id = variant_id,
        .has_payload = has_payload,
        .payload_tag = payload_tag,
        .payload = payload,
    });
    return static_cast<std::int64_t>(state_.enum_pool.size());
  }

  std::int64_t intern_tensor(const std::vector<std::int64_t>& shape, const std::vector<std::int64_t>& data) {
    state_.tensor_pool.push_back(TensorValue{
        .shape = shape,
        .data = data,
    });
    return static_cast<std::int64_t>(state_.tensor_pool.size());
  }

  OptionValue* option_ptr(std::int64_t handle) {
    if (handle <= 0 || static_cast<std::size_t>(handle) > state_.option_pool.size()) {
      return nullptr;
    }
    return &state_.option_pool[static_cast<std::size_t>(handle - 1)];
  }

  ResultValue* result_ptr(std::int64_t handle) {
    if (handle <= 0 || static_cast<std::size_t>(handle) > state_.result_pool.size()) {
      return nullptr;
    }
    return &state_.result_pool[static_cast<std::size_t>(handle - 1)];
  }

  EnumValue* enum_ptr(std::int64_t handle) {
    if (handle <= 0 || static_cast<std::size_t>(handle) > state_.enum_pool.size()) {
      return nullptr;
    }
    return &state_.enum_pool[static_cast<std::size_t>(handle - 1)];
  }

  TensorValue* tensor_ptr(std::int64_t handle) {
    if (handle <= 0 || static_cast<std::size_t>(handle) > state_.tensor_pool.size()) {
      return nullptr;
    }
    return &state_.tensor_pool[static_cast<std::size_t>(handle - 1)];
  }

  std::vector<std::int64_t>* shape_ptr(std::int64_t handle) {
    if (handle <= 0 || static_cast<std::size_t>(handle) > state_.shape_pool.size()) {
      return nullptr;
    }
    return &state_.shape_pool[static_cast<std::size_t>(handle - 1)];
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
