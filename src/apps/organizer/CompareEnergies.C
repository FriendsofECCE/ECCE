#include "CompareEnergies.H"

#include <cctype>
#include <cstdlib>

#include "tdat/EnergyFormat.H"

using std::string;
using std::vector;

bool finalEnergyHartree(const FinalEnergy& e, double *h)
{
  if (e.units.empty() || e.units == "NA" || e.units == "Hartree") *h = e.value;
  else if (e.units == "kcal/Mole") *h = e.value / hartreeTo(1.0, EU_KCAL);
  else if (e.units == "kJoule/Mole") *h = e.value / hartreeTo(1.0, EU_KJ);
  else if (e.units == "eV") *h = e.value / hartreeTo(1.0, EU_EV);
  else return false;
  return true;
}

vector<std::pair<string, int> > parseSide(const string& spec)
{
  vector<std::pair<string, int> > out;
  size_t pos = 0;
  while (pos <= spec.size()) {
    size_t end = spec.find(',', pos);
    if (end == string::npos) end = spec.size();
    string item = spec.substr(pos, end - pos);
    pos = end + 1;
    if (item.empty()) continue;
    int sign = 1, n = 1;
    if (item[0] == '-') {
      sign = -1;
      item.erase(0, 1);
    }
    const size_t star = item.find('*');
    if (star != string::npos && star > 0 &&
        item.find_first_not_of("0123456789") == star) {
      n = atoi(item.substr(0, star).c_str());
      item.erase(0, star + 1);
    }
    out.push_back(std::make_pair(item, sign * n));
  }
  return out;
}

bool calcNameLess(const string& a, const string& b)
{
  size_t i = 0, j = 0;
  while (i < a.size() && j < b.size()) {
    if (isdigit((unsigned char)a[i]) && isdigit((unsigned char)b[j])) {
      size_t i2 = i, j2 = j;
      while (i2 < a.size() && isdigit((unsigned char)a[i2])) i2++;
      while (j2 < b.size() && isdigit((unsigned char)b[j2])) j2++;
      // Leading zeros do not count; the longer number is the larger.
      size_t ti = i, tj = j;
      while (ti + 1 < i2 && a[ti] == '0') ti++;
      while (tj + 1 < j2 && b[tj] == '0') tj++;
      if (i2 - ti != j2 - tj) return i2 - ti < j2 - tj;
      int c = a.compare(ti, i2 - ti, b, tj, j2 - tj);
      if (c != 0) return c < 0;
      i = i2;
      j = j2;
    } else {
      int x = tolower((unsigned char)a[i]), y = tolower((unsigned char)b[j]);
      if (x != y) return x < y;
      i++;
      j++;
    }
  }
  return a.size() - i < b.size() - j;
}
