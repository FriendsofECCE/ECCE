// A running job whose output has stopped growing is "stalled" (#769): a note
// next to Running, never a state of its own.  No wx and no I/O, so the rule
// is tested without the job store.
#ifndef STALLEDRULE_HH
#define STALLEDRULE_HH

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

class StalledRule
{
public:
  // floorSeconds: never judge sooner than this.  fewStepsSeconds: the fixed
  // limit while fewer than three steps have been seen.
  StalledRule(double floorSeconds = 1800, double fewStepsSeconds = 7200)
    : p_floor(floorSeconds), p_few(fewStepsSeconds) {}

  void setLimits(double floorSeconds, double fewStepsSeconds)
  { p_floor = floorSeconds; p_few = fewStepsSeconds; }

  void reset() { p_times.clear(); }

  // A progress step finished at `when` (seconds, any epoch).
  void step(double when)
  {
    p_times.push_back(when);
    if (p_times.size() > 51) p_times.erase(p_times.begin());
  }
  size_t steps() const { return p_times.size(); }

  // Seconds without output after which the job is stalled.
  double threshold() const
  {
    if (p_times.size() < 3) return p_few;
    std::vector<double> gap;
    for (size_t i = 1; i < p_times.size(); i++)
      gap.push_back(p_times[i] - p_times[i - 1]);
    std::sort(gap.begin(), gap.end());
    const size_t n = gap.size();
    const double median = n % 2 ? gap[n / 2] : (gap[n / 2 - 1] + gap[n / 2]) / 2;
    return std::max(3 * median, p_floor);
  }

  // A job out of contact is not judged: its silence is the monitor's.
  bool stalled(double idleSeconds, bool outOfContact) const
  {
    return !outOfContact && idleSeconds > 0 && idleSeconds >= threshold();
  }

  // "no output for 2 h 10 min".
  static std::string text(double seconds)
  {
    char b[64];
    const long m = (long)(seconds / 60);
    if (m < 60) snprintf(b, sizeof b, "no output for %ld min", m < 1 ? 1L : m);
    else if (m % 60) snprintf(b, sizeof b, "no output for %ld h %ld min", m / 60, m % 60);
    else snprintf(b, sizeof b, "no output for %ld h", m / 60);
    return b;
  }

  // The values of the step=, cycle= and iter= fields of a snapshot's stage
  // lines (not its history lines): it changes when a step of any stage does.
  static std::string stepKey(const std::string& snapshot)
  {
    std::string key;
    size_t pos = 0;
    while (pos < snapshot.size()) {
      size_t eol = snapshot.find('\n', pos);
      if (eol == std::string::npos) eol = snapshot.size();
      const std::string line = snapshot.substr(pos, eol - pos);
      pos = eol + 1;
      if (line.empty() || line[0] == '#') continue;
      const size_t sp = line.find(' ');
      if (sp == std::string::npos || line[sp - 1] == '~') continue;
      size_t i = sp;
      while (i < line.size()) {
        while (i < line.size() && line[i] == ' ') i++;
        size_t e = line.find(' ', i);
        if (e == std::string::npos) e = line.size();
        const std::string w = line.substr(i, e - i);
        if (w.compare(0, 5, "step=") == 0 || w.compare(0, 6, "cycle=") == 0 ||
            w.compare(0, 5, "iter=") == 0)
          key += line.substr(0, sp) + ":" + w + ";";
        i = e;
      }
    }
    return key;
  }

  // The monitor's "idle=<s>" header field, 0 when absent.
  static double idleOf(const std::string& snapshot)
  {
    const size_t eol = snapshot.find('\n');
    const size_t at = snapshot.find(" idle=");
    if (at == std::string::npos || at > eol) return 0;
    return atof(snapshot.c_str() + at + 6);
  }

private:
  double p_floor, p_few;
  std::vector<double> p_times;
};

#endif
