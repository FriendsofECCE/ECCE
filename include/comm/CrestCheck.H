// Warnings the job store gives a CREST conformer search that ended normally
// but whose result needs a second look (#831).  Header only, no ECCE
// dependencies (the radius rule is passed in), so tests/jobstore compiles
// it with plain g++.
#ifndef CRESTCHECK_H
#define CRESTCHECK_H

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace CrestCheck {

struct Structure {
  std::vector<std::string> sym;
  std::vector<double> xyz;   // 3 per atom
  double relative = 0;       // kcal/mol, conformers only
};

inline bool atomLine(const std::string& line, std::string& sym, double c[3])
{
  std::istringstream in(line);
  std::string rest;
  if (!(in >> sym >> c[0] >> c[1] >> c[2])) return false;
  return !(in >> rest) && !sym.empty() && isalpha((unsigned char)sym[0]);
}

// The input molecule (ECCE-CREST-INPUT block, appended by gensub) and the
// conformers (ECCE-CREST-FRAME blocks), in the file's order.
inline void parseStructures(const std::string& text, Structure& input,
                            std::vector<Structure>& frames)
{
  std::istringstream in(text);
  std::string line;
  Structure* cur = 0;
  Structure tmp;
  bool isInput = false;
  while (std::getline(in, line)) {
    if (line.rfind("ECCE-CREST-INPUT", 0) == 0) {
      tmp = Structure(); cur = &tmp; isInput = true;
    } else if (line.rfind("ECCE-CREST-FRAME", 0) == 0) {
      tmp = Structure(); cur = &tmp; isInput = false;
    } else if (line.rfind("ECCE-CREST-ENDINPUT", 0) == 0 ||
               line.rfind("ECCE-CREST-ENDFRAME", 0) == 0) {
      if (cur) {
        if (isInput) input = *cur; else frames.push_back(*cur);
      }
      cur = 0;
    } else if (cur) {
      std::string sym; double c[3];
      double rel;
      if (line.find("relative:") != std::string::npos &&
          sscanf(line.c_str() + line.find("relative:") + 9, "%lf", &rel) == 1) {
        cur->relative = rel;
      } else if (atomLine(line, sym, c)) {
        cur->sym.push_back(sym);
        cur->xyz.insert(cur->xyz.end(), c, c + 3);
      }
    }
  }
}

typedef std::set<std::pair<int, int> > Bonds;

// ECCE's covalent-bond rule (Fragment::addCovalentBonds): distance within
// the sum of the radii, each already times its tolerance; no H-H bonds.
inline Bonds bondsOf(const Structure& s, const std::function<double(const std::string&)>& radius)
{
  Bonds b;
  const int n = (int)s.sym.size();
  std::vector<double> r(n);
  for (int i = 0; i < n; i++) r[i] = radius(s.sym[i]);
  for (int i = 0; i < n; i++)
    for (int j = i + 1; j < n; j++) {
      if (s.sym[i] == "H" && s.sym[j] == "H") continue;
      double dx = s.xyz[3*i] - s.xyz[3*j], dy = s.xyz[3*i+1] - s.xyz[3*j+1],
             dz = s.xyz[3*i+2] - s.xyz[3*j+2];
      double d = r[i] + r[j];
      if (dx*dx + dy*dy + dz*dz <= d*d) b.insert(std::make_pair(i, j));
    }
  return b;
}

// Atoms are named by element and their number among that element's atoms.
inline std::vector<std::string> atomNames(const Structure& s)
{
  std::vector<std::string> names;
  std::vector<std::pair<std::string, int> > seen;
  for (size_t i = 0; i < s.sym.size(); i++) {
    size_t k = 0;
    while (k < seen.size() && seen[k].first != s.sym[i]) k++;
    if (k == seen.size()) seen.push_back(std::make_pair(s.sym[i], 0));
    names.push_back(s.sym[i] + std::to_string(++seen[k].second));
  }
  return names;
}

inline std::string joinLimited(const std::vector<std::string>& v)
{
  std::string out;
  const size_t cap = 6;
  for (size_t i = 0; i < v.size() && i < cap; i++) out += (i ? ", " : "") + v[i];
  if (v.size() > cap) out += " and " + std::to_string(v.size() - cap) + " more";
  return out;
}

// "" when nothing is wrong, else the text for runStatusReason.  radius:
// covalent radius times tolerance of an element symbol.
inline std::string warning(const std::string& text,
                           const std::function<double(const std::string&)>& radius)
{
  // Protonation, tautomer and QCG runs change the atoms on purpose.
  const size_t cl = text.find("Command line input:");
  if (cl != std::string::npos) {
    const std::string call = text.substr(cl, text.find('\n', text.find('\n', cl) + 1) - cl);
    if (call.find("--protonate") != std::string::npos ||
        call.find("--tautomeri") != std::string::npos ||
        call.find("--qcg") != std::string::npos) return "";
  }
  Structure input;
  std::vector<Structure> frames;
  parseStructures(text, input, frames);
  if (frames.empty()) return "";

  long removed = 0;
  const std::string tag = "number of topology-based structure removals:";
  for (size_t p = text.find(tag); p != std::string::npos; p = text.find(tag, p + 1))
    removed = std::max(removed, atol(text.c_str() + p + tag.size()));
  if (removed > 0 && frames.size() <= 1)
    return "All conformers were removed by the topology check; the result is "
           "the input structure. Tick 'No topology check' in the Runtype "
           "Details to keep them, and check the bonding of the result.";

  const Structure* best = &frames[0];
  for (size_t i = 1; i < frames.size(); i++)
    if (frames[i].relative < best->relative) best = &frames[i];
  if (input.sym.empty() || input.sym != best->sym) return "";

  const Bonds in = bondsOf(input, radius), out = bondsOf(*best, radius);
  const std::vector<std::string> names = atomNames(input);
  std::vector<std::string> made, broken;
  for (Bonds::const_iterator b = out.begin(); b != out.end(); ++b) {
    if (in.count(*b)) continue;
    const int i = b->first, j = b->second;
    double dx = best->xyz[3*i] - best->xyz[3*j], dy = best->xyz[3*i+1] - best->xyz[3*j+1],
           dz = best->xyz[3*i+2] - best->xyz[3*j+2];
    char d[32];
    snprintf(d, sizeof d, " (%.2f \xc3\x85)", std::sqrt(dx*dx + dy*dy + dz*dz));
    made.push_back(names[i] + "\xe2\x80\x93" + names[j] + d);
  }
  for (Bonds::const_iterator b = in.begin(); b != in.end(); ++b)
    if (!out.count(*b))
      broken.push_back(names[b->first] + "\xe2\x80\x93" + names[b->second]);
  if (made.empty() && broken.empty()) return "";

  std::string w = "Best conformer: ";
  if (!made.empty()) w += std::string(made.size() == 1 ? "bond made " : "bonds made ") + joinLimited(made);
  if (!made.empty() && !broken.empty()) w += "; ";
  if (!broken.empty()) w += std::string(broken.size() == 1 ? "bond broken " : "bonds broken ") + joinLimited(broken);
  return w + ". The bonding differs from the input structure.";
}

} // namespace CrestCheck

#endif // CRESTCHECK_H
