#include "t81/vm/program_io.hpp"

#include <cctype>
#include <cstdint>
#include <fstream>
#include <optional>
#include <regex>
#include <sstream>
#include <string>

#include "t81/tisc/opcodes.hpp"

namespace t81::vm {
namespace {

std::optional<t81::tisc::Opcode> opcode_from_string(const std::string& raw) {
  std::string s;
  s.reserve(raw.size());
  for (char ch : raw) {
    if (ch == '_' || ch == '-') {
      continue;
    }
    s.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
  }

  using t81::tisc::Opcode;
  if (s == "NOP") return Opcode::Nop;
  if (s == "HALT") return Opcode::Halt;
  if (s == "LOADIMM") return Opcode::LoadImm;
  if (s == "LOAD") return Opcode::Load;
  if (s == "STORE") return Opcode::Store;
  if (s == "ADD") return Opcode::Add;
  if (s == "SUB") return Opcode::Sub;
  if (s == "MUL") return Opcode::Mul;
  if (s == "DIV") return Opcode::Div;
  if (s == "MOD") return Opcode::Mod;
  if (s == "JUMP" || s == "JMP") return Opcode::Jump;
  if (s == "JUMPIFZERO" || s == "JZ") return Opcode::JumpIfZero;
  if (s == "JUMPIFNOTZERO" || s == "JNZ") return Opcode::JumpIfNotZero;
  if (s == "JUMPIFNEGATIVE" || s == "JN") return Opcode::JumpIfNegative;
  if (s == "JUMPIFPOSITIVE" || s == "JP") return Opcode::JumpIfPositive;
  if (s == "MOV") return Opcode::Mov;
  if (s == "INC") return Opcode::Inc;
  if (s == "DEC") return Opcode::Dec;
  if (s == "CMP") return Opcode::Cmp;
  if (s == "PUSH") return Opcode::Push;
  if (s == "POP") return Opcode::Pop;
  if (s == "CALL") return Opcode::Call;
  if (s == "RET") return Opcode::Ret;
  if (s == "TRAP") return Opcode::Trap;
  if (s == "NEG") return Opcode::Neg;
  if (s == "I2F") return Opcode::I2F;
  if (s == "F2I") return Opcode::F2I;
  if (s == "I2FRAC") return Opcode::I2Frac;
  if (s == "FRAC2I") return Opcode::Frac2I;
  if (s == "FADD") return Opcode::FAdd;
  if (s == "FSUB") return Opcode::FSub;
  if (s == "FMUL") return Opcode::FMul;
  if (s == "FDIV") return Opcode::FDiv;
  if (s == "FRACADD") return Opcode::FracAdd;
  if (s == "FRACSUB") return Opcode::FracSub;
  if (s == "FRACMUL") return Opcode::FracMul;
  if (s == "FRACDIV") return Opcode::FracDiv;
  if (s == "LESS" || s == "LT") return Opcode::Less;
  if (s == "LESSEQUAL" || s == "LE") return Opcode::LessEqual;
  if (s == "GREATER" || s == "GT") return Opcode::Greater;
  if (s == "GREATEREQUAL" || s == "GE") return Opcode::GreaterEqual;
  if (s == "EQUAL" || s == "EQ") return Opcode::Equal;
  if (s == "NOTEQUAL" || s == "NEQ") return Opcode::NotEqual;
  if (s == "STACKALLOC") return Opcode::StackAlloc;
  if (s == "STACKFREE") return Opcode::StackFree;
  if (s == "HEAPALLOC") return Opcode::HeapAlloc;
  if (s == "HEAPFREE") return Opcode::HeapFree;
  return std::nullopt;
}

std::string trim(const std::string& in) {
  std::size_t b = 0;
  std::size_t e = in.size();
  while (b < e && std::isspace(static_cast<unsigned char>(in[b]))) {
    ++b;
  }
  while (e > b && std::isspace(static_cast<unsigned char>(in[e - 1]))) {
    --e;
  }
  return in.substr(b, e - b);
}

ProgramLoadResult load_text_v1(std::istream& in) {
  ProgramLoadResult out;
  out.format = ProgramFormat::TextV1;

  std::string line;
  while (std::getline(in, line)) {
    const std::string t = trim(line);
    if (t.empty() || t[0] == '#') {
      continue;
    }

    std::istringstream iss(t);
    std::string op;
    iss >> op;
    if (op == "POLICY") {
      std::string rest;
      std::getline(iss, rest);
      out.program.axion_policy_text = trim(rest);
      continue;
    }

    std::int64_t a = 0;
    std::int64_t b = 0;
    std::int64_t c = 0;
    iss >> a >> b >> c;

    const auto opcode = opcode_from_string(op);
    if (!opcode.has_value()) {
      out.error = "unknown opcode in text program: " + op;
      return out;
    }
    out.program.insns.push_back({*opcode, a, b, c});
  }

  out.ok = true;
  return out;
}

std::string read_all(std::istream& in) {
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

bool parse_int_field(const std::string& obj, const std::string& key, std::int64_t* out) {
  const std::regex re("\\\"" + key + "\\\"\\s*:\\s*(-?[0-9]+)");
  std::smatch m;
  if (!std::regex_search(obj, m, re) || m.size() < 2) {
    return false;
  }
  *out = std::stoll(m[1].str());
  return true;
}

std::optional<std::string> parse_string_field(const std::string& obj, const std::string& key) {
  const std::regex re("\\\"" + key + "\\\"\\s*:\\s*\\\"([^\\\"]*)\\\"");
  std::smatch m;
  if (!std::regex_search(obj, m, re) || m.size() < 2) {
    return std::nullopt;
  }
  return m[1].str();
}

ProgramLoadResult load_tisc_json_v1(std::istream& in) {
  ProgramLoadResult out;
  out.format = ProgramFormat::TiscJsonV1;

  const std::string s = read_all(in);
  const auto policy = parse_string_field(s, "axion_policy_text");
  if (policy.has_value()) {
    out.program.axion_policy_text = *policy;
  }

  const auto insns_pos = s.find("\"insns\"");
  if (insns_pos == std::string::npos) {
    out.error = "missing insns array";
    return out;
  }
  const auto lb = s.find('[', insns_pos);
  if (lb == std::string::npos) {
    out.error = "invalid insns array";
    return out;
  }

  int depth = 0;
  std::size_t rb = std::string::npos;
  for (std::size_t i = lb; i < s.size(); ++i) {
    if (s[i] == '[') {
      ++depth;
    } else if (s[i] == ']') {
      --depth;
      if (depth == 0) {
        rb = i;
        break;
      }
    }
  }
  if (rb == std::string::npos) {
    out.error = "unterminated insns array";
    return out;
  }

  const std::string body = s.substr(lb + 1, rb - lb - 1);
  std::size_t pos = 0;
  while (true) {
    const auto ob = body.find('{', pos);
    if (ob == std::string::npos) {
      break;
    }
    int obj_depth = 0;
    std::size_t cb = std::string::npos;
    for (std::size_t i = ob; i < body.size(); ++i) {
      if (body[i] == '{') {
        ++obj_depth;
      } else if (body[i] == '}') {
        --obj_depth;
        if (obj_depth == 0) {
          cb = i;
          break;
        }
      }
    }
    if (cb == std::string::npos) {
      out.error = "unterminated insn object";
      return out;
    }

    const std::string obj = body.substr(ob, cb - ob + 1);
    const auto opname = parse_string_field(obj, "opcode");
    if (!opname.has_value()) {
      out.error = "missing opcode in insn object";
      return out;
    }
    const auto opcode = opcode_from_string(*opname);
    if (!opcode.has_value()) {
      out.error = "unknown opcode in json program: " + *opname;
      return out;
    }

    std::int64_t a = 0;
    std::int64_t b = 0;
    std::int64_t c = 0;
    if (!parse_int_field(obj, "a", &a) || !parse_int_field(obj, "b", &b) || !parse_int_field(obj, "c", &c)) {
      out.error = "insn object requires integer fields a,b,c";
      return out;
    }

    out.program.insns.push_back({*opcode, a, b, c});
    pos = cb + 1;
  }

  if (out.program.insns.empty()) {
    out.error = "insns array is empty";
    return out;
  }

  out.ok = true;
  return out;
}

bool looks_like_json_path(const std::string& path) {
  return path.size() >= 5 && path.substr(path.size() - 5) == ".json";
}

}  // namespace

ProgramLoadResult load_program_from_file(const std::string& path) {
  std::ifstream in(path);
  if (!in) {
    return ProgramLoadResult{.ok = false, .format = ProgramFormat::TextV1, .error = "unable to open file: " + path};
  }

  if (looks_like_json_path(path)) {
    return load_tisc_json_v1(in);
  }
  return load_text_v1(in);
}

}  // namespace t81::vm
