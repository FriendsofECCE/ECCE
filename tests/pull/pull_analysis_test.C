// pullSummary and pullCsv (no wx) against numbers worked out by hand:
// a distance that rises by exactly 0.01 nm/ps from 0.5 nm, and a force
// that is a known sawtooth.
#include <cmath>
#include <cstdio>
#include <sstream>
#include <string>

#include "PullAnalysis.H"

static int failures = 0;
static void check(bool ok, const char *what)
{
  printf("%s: %s\n", ok ? "ok  " : "FAIL", what);
  if (!ok) failures++;
}

int main()
{
  PullSeries s;
  const double f[] = {0, 10, 30, -50, 20};
  for (int i = 0; i < 5; i++) {
    s.time.push_back(i * 5.0);
    s.dist.push_back(0.5 + 0.01 * i * 5.0);
    s.force.push_back(f[i]);
  }
  PullSummary r = pullSummary(s);
  check(r.points == 5, "five points");
  check(std::fabs(r.rate - 0.01) < 1e-12, "slope of the distance is 0.01 nm/ps");
  check(std::fabs(r.meanForce - 2.0) < 1e-12, "mean force is 2");
  check(r.peakForce == -50 && r.peakTime == 15 && std::fabs(r.peakDist - 0.65) < 1e-12,
        "peak is the largest magnitude, with its sign, time and distance");
  check(pullSummary(PullSeries()).points == 0, "empty series");

  std::string csv = pullCsv(s, "calc");
  std::istringstream in(csv);
  std::string line, last;
  int lines = 0;
  while (std::getline(in, line)) { last = line; lines++; }
  check(lines == 8, "two comment lines, a header and five rows");
  check(csv.find("time_ps,distance_nm,force_kJ/mol/nm\n") != std::string::npos, "header");
  check(last == "20,0.7,20", "last row");
  return failures ? 1 : 0;
}
