/**
 * @file
 *
 * Live instance counts for the `livecount` test hook (#1136).
 */
#include <map>
#include <string>

#include "util/LiveCount.H"

namespace {
// Never destroyed: counted objects may die during static destruction.
std::map<std::string, int>& counts()
{
  static auto *m = new std::map<std::string, int>;
  return *m;
}
}


void LiveCount::add(const char *cls, int delta)
{
  counts()[cls] += delta;
}


std::string LiveCount::report()
{
  std::string out;
  for (const auto& c : counts())
    out += (out.empty() ? "" : " ") + c.first + "=" + std::to_string(c.second);
  return out;
}
