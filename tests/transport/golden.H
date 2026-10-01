// Recorded results of operations run through RCommand: what a transport must
// keep returning.  Lines are stored with the test's temporary paths replaced
// by placeholders.  ECCE_GOLDEN_RECORD=1 rewrites a golden file from the
// reference run the test still carries; without it the transport under test
// is compared against the file.

#ifndef TESTS_GOLDEN_H
#define TESTS_GOLDEN_H

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace golden {

typedef std::vector<std::pair<std::string, std::string> > Subs;

inline bool recording() { return getenv("ECCE_GOLDEN_RECORD") != 0; }

inline std::string file(const std::string& name)
{
  std::string here = __FILE__;
  return here.substr(0, here.rfind('/')) + "/golden/" + name + ".txt";
}

inline std::string normalize(std::string s, const Subs& subs)
{
  for (size_t i = 0; i < subs.size(); i++) {
    if (subs[i].first.empty()) continue;
    size_t p = 0;
    while ((p = s.find(subs[i].first, p)) != std::string::npos) {
      s.replace(p, subs[i].first.size(), subs[i].second);
      p += subs[i].second.size();
    }
  }
  return s;
}

inline void record(const std::string& name, const std::vector<std::string>& log,
                   const Subs& subs)
{
  std::ofstream out(file(name).c_str());
  for (size_t i = 0; i < log.size(); i++)
    out << normalize(log[i], subs) << "\n";
  std::cout << "recorded " << log.size() << " lines in " << file(name)
            << std::endl;
}

// Prints one ok/FAIL line per entry; returns the number of differences.
inline int compare(const std::string& name, const std::vector<std::string>& log,
                   const Subs& subs)
{
  std::ifstream in(file(name).c_str());
  if (!in) {
    std::cout << "FAIL no golden file " << file(name)
              << " (ECCE_GOLDEN_RECORD=1 makes one)" << std::endl;
    return 1;
  }
  std::vector<std::string> want;
  std::string line;
  while (std::getline(in, line)) want.push_back(line);
  int failures = 0;
  size_t n = want.size() > log.size() ? want.size() : log.size();
  for (size_t i = 0; i < n; i++) {
    std::string w = i < want.size() ? want[i] : "<missing>";
    std::string g = i < log.size() ? normalize(log[i], subs) : "<missing>";
    if (w == g) std::cout << "ok   " << g << std::endl;
    else {
      failures++;
      std::cout << "FAIL\n  want " << w << "\n  got  " << g << std::endl;
    }
  }
  return failures;
}

}  // namespace golden

#endif
