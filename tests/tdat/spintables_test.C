// The spin density tables' rules: composition text cut to a short line,
// atoms sorted by |spin|, share of the total |spin|.
#include <cmath>
#include <cstdio>
#include <string>

#include "tdat/SpinTables.H"

using namespace std;

static int failures = 0;
#define CHECK(cond, msg) do { if (!(cond)) { ++failures; \
  fprintf(stderr, "FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__); } } while (0)

int main()
{
  // Many contributing atoms: the line still holds three entries plus "other".
  vector<SpinTables::Part> parts;
  parts.push_back({ "O5", 0.20 });
  parts.push_back({ "Nb1", 0.40 });
  for (int i = 0; i < 28; i++) parts.push_back({ "Na" + to_string(i), 0.01 });
  parts.push_back({ "O7", 0.10 });
  string text = SpinTables::compositionText(parts);
  fprintf(stderr, "%s\n", text.c_str());
  CHECK(text == "Nb1 40%, O5 20%, O7 10%, other 28%", "truncated text");
  CHECK(text.size() < 60, "short");
  CHECK(SpinTables::fullCompositionText(parts).find("Na27 1.0%") != string::npos,
        "tooltip keeps the small ones");
  CHECK(SpinTables::compositionText({ { "O1", 0.60 }, { "O2", 0.40 } })
        == "O1 60%, O2 40%", "nothing left over, no 'other'");
  CHECK(SpinTables::compositionText({}).empty(), "empty");

  vector<string> names = { "H1", "Nb2", "O3", "Na4", "O5" };
  vector<double> spin = { 0.10, -0.40, 0.70, 0.0001, -0.10 };
  vector<SpinTables::Population> rows = SpinTables::populations(names, spin);
  CHECK(rows.size() == 4, "the negligible atom is left out");
  CHECK(rows[0].atom == "O3" && rows[1].atom == "Nb2", "largest |spin| first");
  CHECK(rows[2].atom == "H1" && rows[3].atom == "O5", "ties keep their order");
  // sum |s| = 0.1 + 0.4 + 0.7 + 0.0001 + 0.1 = 1.3001
  CHECK(fabs(rows[0].share - 100.0*0.7/1.3001) < 1e-9, "share of O3");
  double sum = 0;
  for (size_t i = 0; i < rows.size(); i++) sum += rows[i].share;
  CHECK(fabs(sum - 100.0*1.3/1.3001) < 1e-9, "shares add up");
  CHECK(SpinTables::populations({}, {}).empty(), "no atoms");

  // Largest |spin| and the per-element sums.
  SpinTables::Population big = SpinTables::largest(names, spin);
  CHECK(big.atom == "O3" && fabs(big.spin - 0.70) < 1e-12, "largest spin atom");
  CHECK(SpinTables::largest({}, {}).atom.empty(), "no atoms, no largest");
  vector<string> el = { "H", "Nb", "O", "Na", "O" };
  vector<pair<string, double> > sums = SpinTables::byElement(el, spin);
  CHECK(sums.size() == 4 && sums[0].first == "O" && fabs(sums[0].second - 0.60) < 1e-12,
        "O sums 0.7 - 0.1");
  CHECK(sums[1].first == "Nb" && fabs(sums[1].second + 0.40) < 1e-12, "Nb negative");
  double all = 0; for (auto& e : sums) all += e.second;
  CHECK(fabs(all - 0.3001) < 1e-9, "element sums add up to the total");
  float c[3];
  SpinTables::colour(0.0, 1.0, c);
  CHECK(c[0] == 1.0f && c[1] == 1.0f && c[2] == 1.0f, "zero spin is white");
  SpinTables::colour(1.0, 1.0, c);
  CHECK(c[0] > c[2], "positive is red");
  SpinTables::colour(-1.0, 1.0, c);
  CHECK(c[2] > c[0], "negative is blue");

  if (failures) return 1;
  fprintf(stderr, "spintables_test: ok\n");
  return 0;
}
