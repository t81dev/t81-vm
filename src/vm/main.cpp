#include <cctype>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "t81/tisc/program.hpp"
#include "t81/vm/summary.hpp"
#include "t81/vm/traps.hpp"
#include "t81/vm/vm.hpp"

namespace {

bool parse_program(const std::string& path, t81::tisc::Program* out) {
  std::ifstream in(path);
  if (!in) {
    return false;
  }

  std::string line;
  while (std::getline(in, line)) {
    if (line.empty() || line[0] == '#') {
      continue;
    }
    std::istringstream iss(line);
    std::string op;
    iss >> op;
    if (op == "POLICY") {
      std::string rest;
      std::getline(iss, rest);
      out->axion_policy_text = rest;
      continue;
    }

    std::int64_t a = 0;
    std::int64_t b = 0;
    std::int64_t c = 0;
    iss >> a >> b >> c;

    using t81::tisc::Opcode;
    Opcode opcode = Opcode::Nop;
    if (op == "NOP") opcode = Opcode::Nop;
    else if (op == "HALT") opcode = Opcode::Halt;
    else if (op == "LOADIMM") opcode = Opcode::LoadImm;
    else if (op == "LOAD") opcode = Opcode::Load;
    else if (op == "STORE") opcode = Opcode::Store;
    else if (op == "DIV") opcode = Opcode::Div;
    else if (op == "MOD") opcode = Opcode::Mod;
    else if (op == "JUMP") opcode = Opcode::Jump;
    else return false;

    out->insns.push_back({opcode, a, b, c});
  }

  return true;
}

void print_trace(const t81::vm::State& s) {
  for (const auto& e : s.trace) {
    std::cout << e.pc << ":" << static_cast<int>(e.opcode);
    if (e.trap.has_value()) {
      std::cout << ":trap=" << t81::vm::to_string(*e.trap);
    }
    std::cout << "\n";
  }
}

std::string canonical_passthrough(const std::string& in) {
  std::string out;
  bool seen_non_space = false;
  for (char ch : in) {
    if (!seen_non_space && std::isspace(static_cast<unsigned char>(ch))) {
      continue;
    }
    seen_non_space = true;
    out.push_back(ch);
  }
  while (!out.empty() && std::isspace(static_cast<unsigned char>(out.back()))) {
    out.pop_back();
  }
  return out;
}

void usage() {
  std::cerr << "usage: t81vm [--trace] [--snapshot] <program.t81>\n";
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    usage();
    return 2;
  }

  std::vector<std::string> args(argv + 1, argv + argc);

  if (args.size() == 2 && args[0] == "--canonical-bigint") {
    std::cout << canonical_passthrough(args[1]) << "\n";
    return 0;
  }
  if (args.size() == 2 && args[0] == "--canonical-fraction") {
    std::cout << canonical_passthrough(args[1]) << "\n";
    return 0;
  }
  if (args.size() == 2 && args[0] == "--canonical-tensor") {
    std::cout << canonical_passthrough(args[1]) << "\n";
    return 0;
  }

  bool emit_trace = false;
  bool emit_snapshot = false;
  std::string program_path;

  for (const auto& arg : args) {
    if (arg == "--trace") {
      emit_trace = true;
    } else if (arg == "--snapshot") {
      emit_snapshot = true;
    } else if (!arg.empty() && arg[0] == '-') {
      usage();
      return 2;
    } else {
      if (!program_path.empty()) {
        usage();
        return 2;
      }
      program_path = arg;
    }
  }

  if (program_path.empty()) {
    usage();
    return 2;
  }

  if (!emit_trace && !emit_snapshot) {
    emit_trace = true;
  }

  t81::tisc::Program p;
  if (!parse_program(program_path, &p)) {
    std::cerr << "FAULT ParseError\n";
    return 1;
  }

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(p);
  auto res = vm->run_to_halt();

  if (emit_trace) {
    print_trace(vm->state());
  }
  if (emit_snapshot) {
    std::cout << t81::vm::snapshot_summary(vm->state());
  }

  if (!res.has_value()) {
    std::cerr << "FAULT " << t81::vm::to_string(res.error()) << "\n";
    return 1;
  }

  return 0;
}
