#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>

#include "t81/tisc/program.hpp"
#include "t81/vm/state.hpp"

namespace t81::vm {

class IVirtualMachine {
 public:
  virtual ~IVirtualMachine() = default;
  virtual void load_program(const t81::tisc::Program& program) = 0;
  virtual std::expected<void, Trap> step() = 0;
  virtual std::expected<void, Trap> run_to_halt(std::size_t max_steps = 100000) = 0;
  virtual const State& state() const = 0;
  virtual void set_register(int idx, std::int64_t value, ValueTag tag = ValueTag::Int) = 0;
};

std::unique_ptr<IVirtualMachine> make_interpreter_vm();

}  // namespace t81::vm
