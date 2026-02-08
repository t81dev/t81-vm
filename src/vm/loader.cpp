#include "t81/vm/loader.hpp"

#include <regex>

#include "t81/vm/validator.hpp"

namespace t81::vm {

namespace {

void parse_policy(const std::string& text, State* state) {
  std::smatch match;
  static const std::regex tier_re(R"(\(tier\s+([0-9]+)\))");
  if (std::regex_search(text, match, tier_re) && match.size() > 1) {
    state->policy = Policy{std::stoi(match[1].str())};
  }
}

}  // namespace

LoadedProgram load_program_image(const t81::tisc::Program& program) {
  constexpr std::size_t kDefaultStackSize = 256;
  constexpr std::size_t kDefaultHeapSize = 768;
  constexpr std::size_t kDefaultTensorSize = 256;
  constexpr std::size_t kDefaultMetaSize = 256;

  LoadedProgram loaded;
  loaded.program = program;
  loaded.initial_state = {};
  loaded.initial_state.layout.code.start = 0;
  loaded.initial_state.layout.code.limit = loaded.program.insns.size();
  loaded.initial_state.layout.stack.start = loaded.initial_state.layout.code.limit;
  loaded.initial_state.layout.stack.limit = loaded.initial_state.layout.stack.start + kDefaultStackSize;
  loaded.initial_state.layout.heap.start = loaded.initial_state.layout.stack.limit;
  loaded.initial_state.layout.heap.limit = loaded.initial_state.layout.heap.start + kDefaultHeapSize;
  loaded.initial_state.layout.tensor.start = loaded.initial_state.layout.heap.limit;
  loaded.initial_state.layout.tensor.limit = loaded.initial_state.layout.tensor.start + kDefaultTensorSize;
  loaded.initial_state.layout.meta.start = loaded.initial_state.layout.tensor.limit;
  loaded.initial_state.layout.meta.limit = loaded.initial_state.layout.meta.start + kDefaultMetaSize;
  loaded.initial_state.memory.assign(loaded.initial_state.layout.total_size(), 0);
  loaded.initial_state.sp = loaded.initial_state.layout.stack.limit;
  loaded.initial_state.heap_ptr = loaded.initial_state.layout.heap.start;
  parse_policy(program.axion_policy_text, &loaded.initial_state);
  loaded.preload_trap = validate_program(program);
  return loaded;
}

}  // namespace t81::vm
