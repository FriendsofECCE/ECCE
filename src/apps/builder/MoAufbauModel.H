/**
 * @file
 *
 * The subshells and electrons the MO Diagram panel draws for a single atom
 * (MoAufbauCanvas.H); no wx, so it can be tested alone.
 */
#ifndef MOAUFBAUMODEL_HH
#define MOAUFBAUMODEL_HH

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include <string>
#include <vector>

using std::string;
using std::vector;

struct AufbauOrbital {
  int index;        // 0-based into ORBENG (the alpha orbital)
  double eV;
  int up, down;     // arrows
};

struct AufbauShell {
  string label;     // "2p"; an unnamed empty set is numbered above the named ones
  int l;
  int n;
  double mean;      // eV
  bool core;
  int electrons;
  vector<AufbauOrbital> orbs;
};

namespace aufbau {

static const double HARTREE_EV = 27.211386245988;

static const char *LETTERS = "spdfgh";

static inline int lOfLabel(const string& stem)
{
  if (stem.empty()) return -1;
  const char *p = strchr(LETTERS, stem[stem.size() - 1]);
  return p ? (int)(p - LETTERS) : -1;
}

static inline string stemOf(const string& label)
{
  const size_t v = label.find(" (virtual)");
  return v == string::npos ? label : label.substr(0, v);
}

/**
 * The subshells to draw, in energy order, from the orbital energies
 * (Hartree), occupations and subshell labels (RadialDist::shellLabels).
 * `eB`/`oB`/`labelsB` are the beta orbitals of an unrestricted atom (empty
 * for restricted/ROKS, where one occupation 0..2 gives both arrows).
 * Orbitals without a label are left out.  Only the lowest `empties` empty
 * subshells are kept.
 */
static inline vector<AufbauShell> shells(const vector<double>& e,
    const vector<double>& o, const vector<string>& labels,
    const vector<double>& eB, const vector<double>& oB,
    const vector<string>& labelsB, int empties = 2)
{
  vector<AufbauShell> out;
  if (e.empty() || o.size() != e.size() || labels.size() != e.size()) return out;
  const bool uks = !eB.empty() && oB.size() == eB.size();

  //  Alpha orbitals by subshell name, lowest first; beta ones likewise, so
  //  the k-th of each pair up.
  vector<string> keys;
  vector< vector<int> > byKey;
  for (size_t m = 0; m < e.size(); m++) {
    if (labels[m].empty()) continue;
    const string k = stemOf(labels[m]);
    size_t at = 0;
    while (at < keys.size() && keys[at] != k) at++;
    if (at == keys.size()) { keys.push_back(k); byKey.push_back(vector<int>()); }
    byKey[at].push_back((int)m);
  }
  vector<int> betaOf(e.size(), -1);
  if (uks) {
    for (size_t k = 0; k < keys.size(); k++) {
      vector<int> b;
      for (size_t m = 0; m < eB.size() && m < labelsB.size(); m++)
        if (!labelsB[m].empty() && stemOf(labelsB[m]) == keys[k]) b.push_back((int)m);
      for (size_t i = 0; i < byKey[k].size() && i < b.size(); i++) betaOf[byKey[k][i]] = b[i];
    }
    if (labelsB.size() != eB.size())
      for (size_t m = 0; m < e.size() && m < eB.size(); m++) betaOf[m] = (int)m;
  }

  //  An unnamed empty set of type l is the next shell of that type above the
  //  named ones (Ar: s (virtual) is 4s), so it gets that n for the drawing.
  int topN[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  for (const string& key : keys) {
    const int l = lOfLabel(key);
    if (l >= 0 && l < 8 && isdigit((unsigned char)key[0]))
      topN[l] = std::max(topN[l], atoi(key.c_str()));
  }

  for (size_t k = 0; k < keys.size(); k++) {
    vector<int>& ms = byKey[k];
    std::stable_sort(ms.begin(), ms.end(), [&](int a, int b) { return e[a] < e[b]; });
    const int l = lOfLabel(keys[k]);
    if (l < 0) continue;
    const size_t per = (size_t)(2 * l + 1);
    //  Only the first set of an unnamed empty type: the rest is just more basis.
    const bool virt = labels[ms[0]].find("virtual") != string::npos &&
                      !isdigit((unsigned char)keys[k][0]);
    for (size_t from = 0; from < ms.size(); from += per) {
      AufbauShell s;
      s.l = l;
      s.n = atoi(keys[k].c_str());
      s.label = keys[k];
      if (virt && l < 8) {
        s.n = std::max(topN[l] + 1, l + 1);
        s.label = std::to_string(s.n) + keys[k];
      }
      s.core = false;
      s.electrons = 0;
      double sum = 0;
      bool whole = true;
      double total = 0;
      for (size_t i = from; i < ms.size() && i < from + per; i++) {
        AufbauOrbital ob;
        ob.index = ms[i];
        ob.eV = e[ms[i]] * HARTREE_EV;
        const double oa = o[ms[i]];
        if (uks) {
          const int b = betaOf[ms[i]];
          ob.up = oa > 0.5 ? 1 : 0;
          ob.down = (b >= 0 && oB[b] > 0.5) ? 1 : 0;
        } else {
          ob.up = oa > 0.5 ? 1 : 0;
          ob.down = oa > 1.5 ? 1 : 0;
          if (fabs(oa - floor(oa + 0.5)) > 0.05) whole = false;
          total += oa;
        }
        sum += ob.eV;
        s.orbs.push_back(ob);
      }
      if (!uks && !whole) {       // fractional: fill the subshell by Hund's rule
        int n = (int)(total + 0.5);
        const int cnt = (int)s.orbs.size();
        for (int i = 0; i < cnt; i++) { s.orbs[i].up = i < n ? 1 : 0; s.orbs[i].down = 0; }
        for (int i = 0; i < cnt; i++) s.orbs[i].down = (n - cnt > i) ? 1 : 0;
      }
      for (const AufbauOrbital& ob : s.orbs) s.electrons += ob.up + ob.down;
      s.mean = sum / (double)s.orbs.size();
      out.push_back(s);
      if (virt) break;
    }
  }
  std::stable_sort(out.begin(), out.end(),
                   [](const AufbauShell& a, const AufbauShell& b) { return a.mean < b.mean; });

  //  Keep every shell with electrons and the lowest few empty ones.
  vector<AufbauShell> kept;
  int empty = 0;
  for (const AufbauShell& s : out) {
    if (s.electrons == 0 && empty++ >= empties) continue;
    kept.push_back(s);
  }

  //  Core: a filled shell far below the highest occupied orbital (well
  //  beyond the valence spread), which is only drawn to scale if it
  //  squeezes the valence levels together.
  double homo = -1e9;
  for (const AufbauShell& s : kept)
    for (const AufbauOrbital& ob : s.orbs)
      if (ob.up + ob.down > 0) homo = std::max(homo, ob.eV);
  for (AufbauShell& s : kept)
    s.core = s.electrons > 0 && s.mean < homo - 60.0;
  return kept;
}

static inline string sup(int n)
{
  static const char *d[10] = { "\xE2\x81\xB0", "\xC2\xB9", "\xC2\xB2", "\xC2\xB3",
      "\xE2\x81\xB4", "\xE2\x81\xB5", "\xE2\x81\xB6", "\xE2\x81\xB7",
      "\xE2\x81\xB8", "\xE2\x81\xB9" };
  string s = std::to_string(n), r;
  for (char c : s) r += d[c - '0'];
  return r;
}

/** "O: 1s2 2s2 2p4" with the powers as superscripts (or plain digits). */
static inline string configuration(const string& symbol,
                                   const vector<AufbauShell>& sh, bool plain = false)
{
  string r = symbol.empty() ? string() : symbol + ":";
  for (const AufbauShell& s : sh) {
    if (s.electrons == 0) continue;
    r += " " + s.label + (plain ? std::to_string(s.electrons) : sup(s.electrons));
  }
  return r;
}

}  // namespace aufbau


#endif
