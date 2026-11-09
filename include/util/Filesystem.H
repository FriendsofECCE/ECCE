// ecce::fs is std::filesystem, except on macOS below 10.15 where Apple's
// libc++ declares it unavailable; there it is the header-only ghc::filesystem
// (third_party/ghc), which has the same interface.
#ifndef ECCE_UTIL_FILESYSTEM_HH
#define ECCE_UTIL_FILESYSTEM_HH

#ifdef __APPLE__
#include <Availability.h>
#endif

#if defined(__APPLE__) && defined(__MAC_OS_X_VERSION_MIN_REQUIRED) && \
    __MAC_OS_X_VERSION_MIN_REQUIRED < 101500
#include <ghc/filesystem.hpp>
namespace ecce { namespace fs = ghc::filesystem; }
#else
#include <filesystem>
namespace ecce { namespace fs = std::filesystem; }
#endif

#endif
