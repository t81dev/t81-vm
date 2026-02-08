#include <cassert>
#include <cstddef>

#include "t81/tisc/program.hpp"
#include "t81/vm/vm.hpp"

using namespace t81;

int main() {
  {
    tisc::Program p;
    p.insns.push_back({tisc::Opcode::TVecAdd, 3, 1, 2});
    p.insns.push_back({tisc::Opcode::TVecMul, 4, 1, 2});
    p.insns.push_back({tisc::Opcode::TTenDot, 5, 1, 2});
    p.insns.push_back({tisc::Opcode::TMatMul, 6, 7, 8});
    p.insns.push_back({tisc::Opcode::TTranspose, 9, 7, 0});
    p.insns.push_back({tisc::Opcode::TExp, 10, 1, 0});
    p.insns.push_back({tisc::Opcode::TSqrt, 11, 10, 0});
    p.insns.push_back({tisc::Opcode::TSiLU, 12, 1, 0});
    p.insns.push_back({tisc::Opcode::TSoftmax, 13, 1, 0});
    p.insns.push_back({tisc::Opcode::TRMSNorm, 14, 1, 0});
    p.insns.push_back({tisc::Opcode::TRoPE, 15, 16, 0});
    p.insns.push_back({tisc::Opcode::ChkShape, 17, 1, 20});
    p.insns.push_back({tisc::Opcode::WeightsLoad, 18, 123, 0});
    p.insns.push_back({tisc::Opcode::SetF, 19, 18, 0});
    p.insns.push_back({tisc::Opcode::Halt, 0, 0, 0});

    auto vm = vm::make_interpreter_vm();
    vm->load_program(p);
    auto& s = const_cast<vm::State&>(vm->state());

    s.tensor_pool.push_back({{3}, {1, 2, 3}});
    s.tensor_pool.push_back({{3}, {4, 5, 6}});
    s.tensor_pool.push_back({{2, 2}, {1, 2, 3, 4}});
    s.tensor_pool.push_back({{2, 2}, {5, 6, 7, 8}});
    s.tensor_pool.push_back({{4}, {1, 2, 3, 4}});
    s.shape_pool.push_back({3});

    vm->set_register(1, 1, vm::ValueTag::TensorHandle);
    vm->set_register(2, 2, vm::ValueTag::TensorHandle);
    vm->set_register(7, 3, vm::ValueTag::TensorHandle);
    vm->set_register(8, 4, vm::ValueTag::TensorHandle);
    vm->set_register(16, 5, vm::ValueTag::TensorHandle);
    vm->set_register(20, 1, vm::ValueTag::ShapeHandle);

    const auto result = vm->run_to_halt();
    assert(result.has_value());

    const auto vec_add = s.tensor_pool[static_cast<std::size_t>(vm->state().registers[3] - 1)];
    assert(vec_add.shape.size() == 1);
    assert(vec_add.data[0] == 5 && vec_add.data[2] == 9);

    const auto vec_mul = s.tensor_pool[static_cast<std::size_t>(vm->state().registers[4] - 1)];
    assert(vec_mul.data[0] == 4 && vec_mul.data[2] == 18);

    const auto dot = s.tensor_pool[static_cast<std::size_t>(vm->state().registers[5] - 1)];
    assert(dot.shape.size() == 1 && dot.shape[0] == 1);
    assert(dot.data[0] == 32);

    const auto mat = s.tensor_pool[static_cast<std::size_t>(vm->state().registers[6] - 1)];
    assert(mat.shape[0] == 2 && mat.shape[1] == 2);
    assert(mat.data[0] == 19);

    const auto transposed = s.tensor_pool[static_cast<std::size_t>(vm->state().registers[9] - 1)];
    assert(transposed.shape[0] == 2 && transposed.shape[1] == 2);
    assert(transposed.data[1] == 3 && transposed.data[2] == 2);

    const auto rope = s.tensor_pool[static_cast<std::size_t>(vm->state().registers[15] - 1)];
    assert(rope.data[0] == 2 && rope.data[1] == -1);

    assert(vm->state().registers[17] == 1);
    assert(vm->state().registers[18] == 123);
    assert(vm->state().register_tags[18] == vm::ValueTag::WeightsTensorHandle);
    assert(vm->state().registers[19] == 123);
    assert(vm->state().register_tags[19] == vm::ValueTag::Int);
  }

  {
    tisc::Program bad;
    bad.insns.push_back({tisc::Opcode::TVecAdd, 3, 1, 2});
    auto vm = vm::make_interpreter_vm();
    vm->load_program(bad);
    auto& s = const_cast<vm::State&>(vm->state());
    s.tensor_pool.push_back({{3}, {1, 2, 3}});
    s.tensor_pool.push_back({{2, 2}, {1, 2, 3, 4}});
    vm->set_register(1, 1, vm::ValueTag::TensorHandle);
    vm->set_register(2, 2, vm::ValueTag::TensorHandle);
    const auto step = vm->step();
    assert(!step.has_value());
    assert(step.error() == vm::Trap::ShapeFault);
  }

  {
    tisc::Program bad;
    bad.insns.push_back({tisc::Opcode::TExp, 1, 2, 0});
    auto vm = vm::make_interpreter_vm();
    vm->load_program(bad);
    vm->set_register(2, 999, vm::ValueTag::TensorHandle);
    const auto step = vm->step();
    assert(!step.has_value());
    assert(step.error() == vm::Trap::DecodeFault);
  }

  return 0;
}
