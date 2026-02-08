#include <cctype>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "t81/vm/program_io.hpp"
#include "t81/vm/summary.hpp"
#include "t81/vm/traps.hpp"
#include "t81/vm/vm.hpp"

namespace {

void print_trace(const t81::vm::State& s) {
  for (const auto& e : s.trace) {
    std::cout << e.pc << ":" << static_cast<int>(e.opcode);
    if (e.write_reg.has_value() && e.write_value.has_value() && e.write_tag.has_value()) {
      std::cout << ":write=r" << *e.write_reg << "=" << *e.write_value << ":" << t81::vm::to_string(*e.write_tag);
    }
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
  std::cerr
      << "usage: t81vm [--trace] [--snapshot] [--max-steps N] [--mode interpreter|accelerated-preview] "
         "<program.t81vm|program.tisc.json>\n";
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
  std::size_t max_steps = 100000;
  std::string mode = "interpreter";
  std::string program_path;

  for (std::size_t i = 0; i < args.size(); ++i) {
    const auto& arg = args[i];
    if (arg == "--trace") {
      emit_trace = true;
    } else if (arg == "--snapshot") {
      emit_snapshot = true;
    } else if (arg == "--max-steps") {
      if (i + 1 >= args.size()) {
        usage();
        return 2;
      }
      const auto& raw = args[++i];
      try {
        max_steps = static_cast<std::size_t>(std::stoull(raw));
      } catch (...) {
        usage();
        return 2;
      }
      if (max_steps == 0) {
        usage();
        return 2;
      }
    } else if (arg == "--mode") {
      if (i + 1 >= args.size()) {
        usage();
        return 2;
      }
      mode = args[++i];
      if (mode != "interpreter" && mode != "accelerated-preview") {
        usage();
        return 2;
      }
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

  const auto loaded = t81::vm::load_program_from_file(program_path);
  if (!loaded.ok) {
    std::cerr << "FAULT ParseError: " << loaded.error << "\n";
    return 1;
  }

  auto vm = t81::vm::make_interpreter_vm();
  if (mode == "accelerated-preview") {
    // Acceleration mode is intentionally preview-only; behavior remains contract-compatible interpreter execution.
    std::cerr << "MODE accelerated-preview (preview): using interpreter backend\n";
  }
  vm->load_program(loaded.program);
  auto res = vm->run_to_halt(max_steps);

  if (emit_trace) {
    print_trace(vm->state());
  }
  if (emit_snapshot) {
    std::cout << t81::vm::snapshot_summary(vm->state());
  }

  if (!res.has_value()) {
    std::cerr << "FAULT " << t81::vm::to_string(res.error()) << "\n";
    const auto payload_line = t81::vm::trap_payload_summary_line(vm->state());
    if (!payload_line.empty()) {
      std::cerr << payload_line << "\n";
    }
    return 1;
  }

  return 0;
}
