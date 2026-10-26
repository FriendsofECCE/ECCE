#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

#include "util/FrameTiming.H"

namespace {
  struct State {
    FILE *out = 0;
    bool open = false;
    int step = -1;
    double t0 = 0;
    double lastEnd = -1;
    std::vector<std::pair<std::string, double> > phases;
  };

  State& state()
  {
    static State s;
    static bool init = false;
    if (!init) {
      init = true;
      const char *f = getenv("ECCE_FRAME_TIMING");
      if (f && *f) s.out = fopen(f, "a");
    }
    return s;
  }
}

namespace frametiming {

bool enabled() { return state().out != 0; }

double now()
{
  using namespace std::chrono;
  return duration<double, std::milli>(
      steady_clock::now().time_since_epoch()).count();
}

void add(const char *phase, double ms)
{
  State& s = state();
  if (!s.out || !s.open) return;
  for (auto& p : s.phases)
    if (p.first == phase) { p.second += ms; return; }
  s.phases.emplace_back(phase, ms);
}

void begin(int step)
{
  State& s = state();
  if (!s.out) return;
  s.open = true;
  s.step = step;
  s.t0 = now();
  s.phases.clear();
}

void end()
{
  State& s = state();
  if (!s.out || !s.open) return;
  const double t = now();
  fprintf(s.out, "frame %d total=%.2f", s.step, t - s.t0);
  for (auto& p : s.phases) fprintf(s.out, " %s=%.2f", p.first.c_str(), p.second);
  if (s.lastEnd >= 0) fprintf(s.out, " idle=%.2f", s.t0 - s.lastEnd);
  fprintf(s.out, "\n");
  fflush(s.out);
  s.lastEnd = t;
  s.open = false;
}

}
