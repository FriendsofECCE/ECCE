// libcint's headers define lower-case macros (atm, bas, env...) that clash
// with ordinary identifiers; include it only through this file.
#pragma once
extern "C" {
#include <cint.h>
#include <cint_funcs.h>
}
#undef atm
#undef bas
#undef env
