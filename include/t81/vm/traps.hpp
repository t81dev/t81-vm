#pragma once

#include <string_view>

namespace t81::vm {

enum class Trap {
  None = 0,
  DecodeFault,
  TypeFault,
  BoundsFault,
  StackFault,
  DivisionFault,
  SecurityFault,
  ShapeFault,
  TrapInstruction,
};

inline std::string_view to_string(Trap trap) {
  switch (trap) {
    case Trap::None:
      return "None";
    case Trap::DecodeFault:
      return "DecodeFault";
    case Trap::TypeFault:
      return "TypeFault";
    case Trap::BoundsFault:
      return "BoundsFault";
    case Trap::StackFault:
      return "StackFault";
    case Trap::DivisionFault:
      return "DivisionFault";
    case Trap::SecurityFault:
      return "SecurityFault";
    case Trap::ShapeFault:
      return "ShapeFault";
    case Trap::TrapInstruction:
      return "TrapInstruction";
  }
  return "UnknownTrap";
}

}  // namespace t81::vm
