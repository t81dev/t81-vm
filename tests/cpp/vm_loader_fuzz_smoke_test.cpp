#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <vector>

#include "t81/vm/program_io.hpp"
#include "t81/vm/vm.hpp"

namespace {

std::string random_token(std::mt19937_64* rng) {
  static constexpr char kAlphabet[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-";
  std::uniform_int_distribution<int> len_dist(1, 18);
  std::uniform_int_distribution<int> ch_dist(0, static_cast<int>(sizeof(kAlphabet) - 2));
  std::string out;
  const int n = len_dist(*rng);
  out.reserve(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) {
    out.push_back(kAlphabet[ch_dist(*rng)]);
  }
  return out;
}

std::string random_program_text(std::mt19937_64* rng) {
  std::uniform_int_distribution<int> line_dist(1, 40);
  std::uniform_int_distribution<int> mode_dist(0, 9);
  std::uniform_int_distribution<int> imm_dist(-2048, 2048);
  const std::vector<std::string> valid_ops = {
      "LoadImm", "Add", "Sub", "Mul", "Div", "Cmp", "Jump", "JumpIfZero", "JumpIfNotZero", "Halt"};

  std::string out;
  const int lines = line_dist(*rng);
  for (int i = 0; i < lines; ++i) {
    const int mode = mode_dist(*rng);
    if (mode <= 2) {
      out += random_token(rng) + " " + std::to_string(imm_dist(*rng)) + " " + std::to_string(imm_dist(*rng)) + " " +
             std::to_string(imm_dist(*rng)) + "\n";
      continue;
    }
    if (mode <= 7) {
      const auto& op = valid_ops[static_cast<std::size_t>(mode) % valid_ops.size()];
      out += op + " " + std::to_string(imm_dist(*rng)) + " " + std::to_string(imm_dist(*rng)) + " " +
             std::to_string(imm_dist(*rng)) + "\n";
      continue;
    }
    out += "# " + random_token(rng) + "\n";
  }
  return out;
}

}  // namespace

int main() {
  std::mt19937_64 rng(0xA11CE5EEDULL);
  const auto tmp_root = std::filesystem::temp_directory_path();

  for (int i = 0; i < 250; ++i) {
    const auto tmp_path = tmp_root / ("t81_vm_loader_fuzz_" + std::to_string(i) + ".t81");
    {
      std::ofstream out(tmp_path);
      assert(out.good());
      out << random_program_text(&rng);
    }

    const auto loaded = t81::vm::load_program_from_file(tmp_path.string());
    if (loaded.ok) {
      auto vm = t81::vm::make_interpreter_vm();
      vm->load_program(loaded.program);
      // Smoke only: ensure arbitrary loader-accepted programs do not hang.
      (void)vm->run_to_halt(2000);
    }

    std::error_code ec;
    std::filesystem::remove(tmp_path, ec);
  }

  return 0;
}
