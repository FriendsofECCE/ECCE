/**
 * @file
 *
 * Live instance counts of the classes a tool tab makes, so a test can tell
 * that closing a tab frees them (#1136).  Hook: `livecount`.
 */
#ifndef LIVECOUNT_H
#define LIVECOUNT_H

#include <string>

namespace LiveCount {
  // Call with +1 in each constructor and -1 in the destructor.
  void add(const char *cls, int delta);
  // "Builder=1 SGViewer=1 ..." in name order; classes never made are absent.
  std::string report();
}

#endif
