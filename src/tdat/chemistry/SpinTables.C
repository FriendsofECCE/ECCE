#include "tdat/SpinTables.H"

#include <algorithm>
#include <cmath>
#include <cstdio>

using std::string;
using std::vector;

static vector<SpinTables::Part> byShare(const vector<SpinTables::Part>& parts)
{
  vector<SpinTables::Part> sorted(parts);
  std::stable_sort(sorted.begin(), sorted.end(),
      [](const SpinTables::Part& a, const SpinTables::Part& b) {
        return a.second > b.second; });
  return sorted;
}


string SpinTables::compositionText(const vector<Part>& parts, size_t maxShown,
                                   double minShare)
{
  const vector<Part> sorted = byShare(parts);
  string text;
  double rest = 0.0;
  size_t shown = 0;
  for (size_t k = 0; k < sorted.size(); k++) {
    if (shown < maxShown && sorted[k].second >= minShare) {
      char buf[96];
      snprintf(buf, sizeof buf, "%s%s %.0f%%", shown ? ", " : "",
               sorted[k].first.c_str(), 100.0*sorted[k].second);
      text += buf;
      shown++;
    } else {
      rest += sorted[k].second;
    }
  }
  // Mulliken shares can be negative; only a positive remainder is worth a word.
  if (shown && rest >= 0.005) {
    char buf[48];
    snprintf(buf, sizeof buf, ", other %.0f%%", 100.0*rest);
    text += buf;
  }
  return text;
}


string SpinTables::fullCompositionText(const vector<Part>& parts)
{
  string text;
  for (const Part& p : byShare(parts)) {
    if (p.second < 0.01) break;
    char buf[96];
    snprintf(buf, sizeof buf, "%s%s %.1f%%", text.empty() ? "" : ", ",
             p.first.c_str(), 100.0*p.second);
    text += buf;
  }
  return text;
}


vector<SpinTables::Population> SpinTables::populations(
    const vector<string>& names, const vector<double>& spin, double threshold)
{
  vector<Population> rows;
  double total = 0.0;
  for (size_t a = 0; a < spin.size(); a++)
    total += std::fabs(spin[a]);
  for (size_t a = 0; a < spin.size(); a++) {
    if (std::fabs(spin[a]) < threshold) continue;
    rows.push_back({ a < names.size() ? names[a] : string(), spin[a],
                     total > 0.0 ? 100.0*std::fabs(spin[a])/total : 0.0 });
  }
  std::stable_sort(rows.begin(), rows.end(),
      [](const Population& x, const Population& y) {
        return std::fabs(x.spin) > std::fabs(y.spin); });
  return rows;
}
