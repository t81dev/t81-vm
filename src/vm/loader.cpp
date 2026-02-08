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
  LoadedProgram loaded;
  loaded.program = program;
  loaded.initial_state = {};
  loaded.initial_state.memory.assign(1024, 0);
  parse_policy(program.axion_policy_text, &loaded.initial_state);
  loaded.preload_trap = validate_program(program);
  return loaded;
}

}  // namespace t81::vm
