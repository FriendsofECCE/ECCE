#include <algorithm>
#include <cmath>
#include <locale>
#include <sstream>

#include "PullAnalysis.H"

PullSummary pullSummary(const PullSeries& s)
{
  PullSummary r;
  const size_t n = std::min(s.time.size(), std::min(s.dist.size(), s.force.size()));
  r.points = n;
  if (n == 0) return r;
  double mt = 0, md = 0, mf = 0;
  for (size_t i = 0; i < n; i++) { mt += s.time[i]; md += s.dist[i]; mf += s.force[i]; }
  mt /= n; md /= n; r.meanForce = mf / n;
  double sxy = 0, sxx = 0;
  size_t peak = 0;
  for (size_t i = 0; i < n; i++) {
    sxy += (s.time[i] - mt) * (s.dist[i] - md);
    sxx += (s.time[i] - mt) * (s.time[i] - mt);
    if (std::fabs(s.force[i]) > std::fabs(s.force[peak])) peak = i;
  }
  r.rate = sxx > 0 ? sxy / sxx : 0;
  r.peakForce = s.force[peak];
  r.peakTime = s.time[peak];
  r.peakDist = s.dist[peak];
  return r;
}

std::string pullCsv(const PullSeries& s, const std::string& calculation)
{
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out.precision(10);
  out << "# ECCE pull run\n# calculation: " << calculation << "\n";
  out << "time_ps,distance_nm,force_kJ/mol/nm\n";
  const size_t n = std::min(s.time.size(), std::min(s.dist.size(), s.force.size()));
  for (size_t i = 0; i < n; i++)
    out << s.time[i] << "," << s.dist[i] << "," << s.force[i] << "\n";
  return out.str();
}
