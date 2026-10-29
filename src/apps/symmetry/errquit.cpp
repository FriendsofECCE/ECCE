#include "symmetry_cpp.h"

#include <iostream>

namespace ecce_symmetry {

// Match errquit.f: diagnose on stderr and guarantee an abnormal status.
int errorExit(const std::string& message, int status) {
  std::cerr << message << '\n';
  return status > 0 && status < 256 ? status : 1;
}

} // namespace ecce_symmetry
