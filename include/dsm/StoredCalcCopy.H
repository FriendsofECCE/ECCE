#ifndef STOREDCALCCOPY_HH
#define STOREDCALCCOPY_HH

#include <string>

class Resource;

/**
 * Copies a calculation that was parsed once, at packaging time, into a
 * project: files by PUT and properties by PROPPATCH through the EDSI, so it
 * works on the data server and in local mode alike.  The source is a
 * directory in FileEDSI's layout (files plus one .ecce-meta sidecar per
 * directory).  What belongs to the person or machine that made the copy
 * (owner, dates, job directory, launch host) is not copied; the new
 * calculation gets its own.
 */
namespace StoredCalcCopy {
  // "<format> <fnv1a-64 of the output file> <size>": what a pre-parsed copy
  // records about the output it was made from.
  std::string stampOf(const std::string& outputFile);

  // True if dir/stamp says the copy was made from this output by this format.
  bool isCurrent(const std::string& dir, const std::string& outputFile);

  // The name of the one calculation folder inside a pre-parsed copy, or "".
  std::string calcName(const std::string& dir);

  // Copy the tree at srcDir into project as calcName.  On failure the
  // partly made calculation is removed and why says what went wrong.
  bool copy(Resource *project, const std::string& srcDir,
            const std::string& calcName, std::string& why);
}

#endif
