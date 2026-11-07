#include "FragOrbBox.H"

#include <algorithm>
#include <cmath>
#include <cstdio>

using std::string;
using std::vector;

namespace {

// Degenerate within the CLI's own --levels tolerance (1e-4 Eh).
const double kDegenerateEv = 0.003;

string f(const char *fmt, double v)
{
  char b[64];
  snprintf(b, sizeof b, fmt, v);
  return b;
}

// "C6O6" -> "C₆O₆"
string subscripts(const string& formula)
{
  string o;
  for (char c : formula)
    if (c >= '0' && c <= '9') o += string("\xE2\x82") + (char)(0x80 + (c - '0'));
    else o += c;
  return o;
}

// 2 -> "²⁺", -1 -> "⁻", 0 -> ""
string chargeText(int q)
{
  if (q == 0) return "";
  const char *sup[] = {"\xE2\x81\xB0", "\xC2\xB9", "\xC2\xB2", "\xC2\xB3", "\xE2\x81\xB4",
                       "\xE2\x81\xB5", "\xE2\x81\xB6", "\xE2\x81\xB7", "\xE2\x81\xB8", "\xE2\x81\xB9"};
  string o;
  const int a = std::abs(q);
  if (a > 1) for (char c : std::to_string(a)) o += sup[c - '0'];
  return o + (q > 0 ? "\xE2\x81\xBA" : "\xE2\x81\xBB");
}

string multName(int m)
{
  const char *n[] = {"", "singlet", "doublet", "triplet", "quartet", "quintet", "sextet", "septet"};
  return m >= 1 && m <= 7 ? n[m] : "multiplicity " + std::to_string(m);
}

// Indices of the channel's MOs grouped into degenerate levels.
vector<std::pair<int, int> > moLevels(const FragOrbChannel& ch)
{
  vector<std::pair<int, int> > out;
  for (int i = 0; i < (int)ch.mo.size();) {
    int j = i;
    while (j + 1 < (int)ch.mo.size() && std::fabs(ch.mo[j + 1].energy - ch.mo[i].energy) < kDegenerateEv) j++;
    out.push_back(std::make_pair(i, j));
    i = j + 1;
  }
  return out;
}

// Share of an FO level in an MO level: the sum over the FOs, averaged over
// the MOs (invariant to how the programs mix degenerate orbitals).
double levelShare(const FragOrbChannel& ch, const std::pair<int, int>& mos, const FragLevel& l)
{
  double s = 0;
  for (int m = mos.first; m <= mos.second; m++)
    for (int k = l.first; k <= l.last; k++) s += ch.mo[m].share[k];
  return s / (mos.second - mos.first + 1);
}

string moRange(const FragOrbChannel& ch, const std::pair<int, int>& l)
{
  return l.first == l.second ? "MO " + std::to_string(ch.mo[l.first].index)
                             : "MO " + std::to_string(ch.mo[l.first].index) + "-" +
                                   std::to_string(ch.mo[l.second].index);
}

}  // namespace


string fragPercent(double share)
{
  return f("%.0f %%", 100 * share);
}


bool fragOrbRequest(const vector<FragStep>& steps, string& complex, vector<string>& fragments,
                    vector<string>& names, string& waiting)
{
  const FragStep *cx = 0;
  vector<const FragStep*> fr;
  for (const FragStep& s : steps) {
    if (s.fragment == 0 && !cx) cx = &s;
    else if (s.fragment > 0) fr.push_back(&s);
  }
  if (!cx || fr.size() < 2) return false;
  std::sort(fr.begin(), fr.end(), [](const FragStep *a, const FragStep *b) {
    return a->fragment < b->fragment;
  });
  vector<string> missing, failed;
  auto path = [&](const FragStep *s) {
    if (!s->complete) {
      (s->state.empty() ? missing : failed).push_back(s->name);
      return string();
    }
    if (s->url.compare(0, 7, "file://") == 0) return s->url.substr(7);
    if (!s->url.empty() && s->url[0] == '/') return s->url;
    failed.push_back(s->name + " (a server calculation)");
    return string();
  };
  complex = path(cx);
  fragments.clear();
  names.clear();
  for (const FragStep *s : fr) {
    fragments.push_back(path(s));
    names.push_back(s->formula.empty() ? s->name : subscripts(s->formula));
  }
  waiting.clear();
  auto list = [](const vector<string>& v) {
    string o;
    for (size_t i = 0; i < v.size(); i++) o += (i ? ", " : "") + v[i];
    return o;
  };
  if (!failed.empty()) waiting = "Not available: " + list(failed) + ". ";
  if (!missing.empty()) waiting += "Waiting for " + list(missing) + " to finish.";
  return true;
}


vector<FragLevel> fragLevels(const FragOrbResult& r, size_t channel, const vector<string>& names)
{
  vector<FragLevel> out;
  if (channel >= r.channels.size()) return out;
  const FragOrbChannel& ch = r.channels[channel];
  // Each fragment's HOMO: its last FO with electrons.
  vector<int> homo(names.size() + r.fragments.size(), -1);
  for (size_t k = 0; k < ch.fo.size(); k++)
    if (ch.fo[k].occupation > 1e-6 && ch.fo[k].fragment < (int)homo.size())
      homo[ch.fo[k].fragment] = ch.fo[k].index;
  for (int k = 0; k < (int)ch.fo.size();) {
    int j = k;
    while (j + 1 < (int)ch.fo.size() && ch.fo[j + 1].fragment == ch.fo[k].fragment &&
           std::fabs(ch.fo[j + 1].energy - ch.fo[k].energy) < kDegenerateEv) j++;
    FragLevel l;
    l.fragment = ch.fo[k].fragment;
    l.first = k;
    l.last = j;
    l.energy = ch.fo[k].energy;
    l.occupation = ch.fo[k].occupation;
    const int h = homo[l.fragment];
    const int lo = ch.fo[k].index, hi = ch.fo[j].index;
    string where;
    if (h >= lo && h <= hi) where = "HOMO";
    else if (hi < h) where = "HOMO\xE2\x88\x92" + std::to_string(h - hi);
    else if (lo == h + 1) where = "LUMO";
    else where = "LUMO+" + std::to_string(lo - h - 1);
    const string name = l.fragment < (int)names.size() ? names[l.fragment]
                        : l.fragment < (int)r.fragments.size() ? r.fragments[l.fragment] : "?";
    l.label = name + " " + where + (j > k ? " (\xC3\x97" + std::to_string(j - k + 1) + ")" : "");
    out.push_back(l);
    k = j + 1;
  }
  return out;
}


FragOrbBoxResult fragOrbBox(const vector<FragStep>& steps)
{
  FragOrbBoxResult b;
  string complex, waiting;
  vector<string> fragments;
  b.applicable = fragOrbRequest(steps, complex, fragments, b.names, waiting);
  if (!b.applicable) return b;
  vector<const FragStep*> fr;
  for (const FragStep& s : steps) if (s.fragment > 0) fr.push_back(&s);
  std::sort(fr.begin(), fr.end(), [](const FragStep *x, const FragStep *y) {
    return x->fragment < y->fragment;
  });
  for (size_t k = 0; k < fr.size(); k++) {
    b.level += (k ? " | " : "") + b.names[k] + chargeText(fr[k]->charge);
    if (fr[k]->mult > 1) b.level += " (" + multName(fr[k]->mult) + ")";
  }
  if (!waiting.empty()) { b.note = waiting; return b; }
  b.data = runFragOrb(complex, fragments);
  if (!b.data.ok) { b.note = b.data.error; return b; }
  b.ready = true;
  b.level += ", Mulliken";
  const bool open = b.data.channels.size() > 1;
  for (size_t c = 0; c < b.data.channels.size(); c++) {
    const FragOrbChannel& ch = b.data.channels[c];
    const vector<std::pair<int, int> > mos = moLevels(ch);
    const vector<FragLevel> fos = fragLevels(b.data, c, b.names);
    int homo = -1;
    for (size_t i = 0; i < mos.size(); i++)
      for (int m = mos[i].first; m <= mos[i].second; m++)
        if (ch.mo[m].occupation > 1e-6) homo = (int)i;
    if (homo < 0) continue;
    const int lo = std::max(0, homo - 2), hi = std::min((int)mos.size() - 1, homo + 3);
    for (int i = hi; i >= lo; i--) {
      vector<std::pair<double, size_t> > v;
      for (size_t k = 0; k < fos.size(); k++)
        v.push_back(std::make_pair(-levelShare(ch, mos[i], fos[k]), k));
      std::sort(v.begin(), v.end());
      string value;
      for (size_t t = 0; t < v.size() && t < 3; t++) {
        if (t > 0 && -v[t].first < b.data.threshold) break;
        value += (t ? ", " : "") + fragPercent(-v[t].first) + " " + fos[v[t].second].label;
      }
      string label = (open ? (ch.spin ? "\xCE\xB2 " : "\xCE\xB1 ") : "") + moRange(ch, mos[i]);
      if (i == homo) label += " HOMO";
      if (i == homo + 1) label += " LUMO";
      label += "  " + f("%.2f", ch.mo[mos[i].first].energy) + " eV";
      b.rows.push_back(std::make_pair(label, value));
    }
  }
  if (b.data.fieldApplicable && !b.data.fieldOk)
    b.warning = "The metal fragment's occupied d orbitals do not match the complex's; "
                "the d shares are unreliable (field-split guess failed).";
  b.note = "Shares of the fragment orbitals (FOs) of " + fragPercent(b.data.threshold) +
           " or more; degenerate orbitals taken together.";
  return b;
}


string FragOrbBoxResult::text() const
{
  if (!applicable) return "";
  string t = "Fragment orbitals\n";
  if (!level.empty()) t += level + "\n";
  for (size_t i = 0; i < rows.size(); i++) t += rows[i].first + ": " + rows[i].second + "\n";
  if (!note.empty()) t += note + "\n";
  if (!warning.empty()) t += warning + "\n";
  return t;
}


string fragOrbDetailsText(const FragOrbResult& r, const vector<string>& names)
{
  string t;
  for (size_t c = 0; c < r.channels.size(); c++) {
    const FragOrbChannel& ch = r.channels[c];
    const vector<FragLevel> fos = fragLevels(r, c, names);
    if (r.channels.size() > 1) t += string(ch.spin ? "Beta" : "Alpha") + " orbitals\n";
    t += "   MO     E / eV    occ  outside FOs  FO levels with 1 % or more\n";
    for (size_t i = ch.mo.size(); i-- > 0;) {
      const FragOrbMo& mo = ch.mo[i];
      vector<std::pair<double, size_t> > v;
      for (size_t k = 0; k < fos.size(); k++) {
        double s = 0;
        for (int q = fos[k].first; q <= fos[k].last; q++) s += mo.share[q];
        if (s >= 0.01) v.push_back(std::make_pair(-s, k));
      }
      std::sort(v.begin(), v.end());
      char head[96];
      // No "-0.0 %" for a remainder that is zero within rounding.
      const double rest = std::fabs(mo.remainder) < 5e-4 ? 0.0 : mo.remainder;
      snprintf(head, sizeof head, "%5d %10.3f  %5.3f  %8.1f %%  ", mo.index, mo.energy,
               mo.occupation, 100 * rest);
      t += head;
      for (size_t q = 0; q < v.size(); q++)
        t += (q ? ", " : "") + f("%.1f %%", -100 * v[q].first) + " " + fos[v[q].second].label;
      t += "\n";
    }
    t += "\n";
  }
  t += "FO levels: fragment, position, energy / eV, occupation\n";
  for (size_t c = 0; c < r.channels.size(); c++)
    for (const FragLevel& l : fragLevels(r, c, names))
      t += "  " + l.label + f("  %.3f", l.energy) + f("  %.3f", l.occupation) + "\n";
  return t;
}


string fragOrbCsv(const FragOrbResult& r, const vector<string>& names)
{
  string t = "spin,mo,energy_ev,occupation,remainder";
  if (r.channels.empty()) return t + "\n";
  for (const FragOrbFo& fo : r.channels[0].fo) {
    const string n = fo.fragment < (int)names.size() ? names[fo.fragment] : r.fragments[fo.fragment];
    t += "," + n + "_fo" + std::to_string(fo.index);
  }
  t += "\n";
  for (const FragOrbChannel& ch : r.channels)
    for (const FragOrbMo& mo : ch.mo) {
      t += std::to_string(ch.spin) + "," + std::to_string(mo.index) + "," + f("%.6f", mo.energy) +
           "," + f("%.6f", mo.occupation) + "," + f("%.8f", mo.remainder);
      for (double s : mo.share) t += "," + f("%.8f", s);
      t += "\n";
    }
  return t;
}
