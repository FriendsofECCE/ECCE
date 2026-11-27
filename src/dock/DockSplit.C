#include "dock/DockSplit.H"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <unordered_map>

namespace {

// Covalent radii (Angstrom); the ionic radius for Li, Na, K, Mg and Ca, so a
// bonded metal ion in a host-guest complex is a guest, not part of the host.
// Deliberately not TPerTab: that table would move which atoms are bonded.
double radius(const std::string& el)
{
  static const std::map<std::string, double> r = {
    {"H",0.31},{"He",0.28},{"Li",0.68},{"Be",0.96},{"B",0.84},{"C",0.76},
    {"N",0.71},{"O",0.66},{"F",0.57},{"Ne",0.58},{"Na",0.97},{"Mg",0.72},
    {"Al",1.21},{"Si",1.11},{"P",1.07},{"S",1.05},{"Cl",1.02},{"Ar",1.06},
    {"K",1.33},{"Ca",0.99},{"Fe",1.32},{"Co",1.26},{"Ni",1.24},{"Cu",1.32},
    {"Zn",1.22},{"Br",1.20},{"I",1.39}};
  std::map<std::string, double>::const_iterator it = r.find(el);
  return it == r.end() ? 1.5 : it->second;
}

bool isWater(const std::string& n)
{
  static const std::set<std::string> s = {"HOH","WAT","H2O","DOD","TIP","TIP3",
                                          "SOL","T3P","SPC"};
  return s.count(n) > 0;
}

bool isPolymer(const std::string& n)
{
  static const std::set<std::string> s = {
    "ALA","ARG","ASN","ASP","CYS","GLN","GLU","GLY","HIS","ILE","LEU","LYS",
    "MET","PHE","PRO","SER","THR","TRP","TYR","VAL","HID","HIE","HIP","HSD",
    "HSE","HSP","CYX","CYM","ASH","GLH","LYN","MSE","SEC","PYL","A","C","G",
    "U","T","DA","DC","DG","DT","DU","ADE","CYT","GUA","URA","THY"};
  return s.count(n) > 0;
}

std::string stripDigits(const std::string& s)
{
  size_t e = s.size();
  while (e > 0 && isdigit((unsigned char)s[e-1])) e--;
  return s.substr(0, e);
}

std::string plural(int n)
{
  std::ostringstream o;
  o << n << " water molecule" << (n == 1 ? "" : "s");
  return o.str();
}

// Waters and identical chains left in the receptor.  @p nwater < 0: count by
// residue name; otherwise the molecule count the caller found.
void receptorWarnings(const std::vector<DockSplitAtom>& a,
                      const std::vector<char>& lig, int nwater,
                      std::vector<std::string>& out)
{
  std::set<std::string> wat;
  std::map<std::string, std::vector<std::string> > chain;
  for (size_t i = 0; i < a.size(); i++) {
    if (lig[i] || a[i].resName.empty()) continue;
    if (isWater(a[i].resName)) wat.insert(a[i].chain + ":" + a[i].resNum);
    if (isPolymer(a[i].resName))
      chain[a[i].chain].push_back(a[i].resName + a[i].resNum);
  }
  if (nwater < 0 || (nwater == 0 && !wat.empty())) nwater = (int)wat.size();
  if (nwater)
    out.push_back("the receptor still holds " + plural(nwater) + ", which "
      "Vina treats as part of the receptor and which can block the binding "
      "site. Remove them in the Builder if that is not wanted.");
  std::map<std::string, std::vector<std::string> > seq;
  for (std::map<std::string, std::vector<std::string> >::const_iterator c =
         chain.begin(); c != chain.end(); ++c) {
    std::string key;
    for (size_t k = 0; k < c->second.size(); k++) {
      if (k) key += " ";
      key += stripDigits(c->second[k]);
    }
    seq[key].push_back(c->first == " " ? "(blank)" : c->first);
  }
  for (std::map<std::string, std::vector<std::string> >::const_iterator s =
         seq.begin(); s != seq.end(); ++s) {
    if (s->second.size() < 2) continue;
    std::string l;
    for (size_t k = 0; k < s->second.size(); k++) l += (k ? ", " : "") + s->second[k];
    out.push_back("the receptor holds identical chains (" + l + "); the "
      "ligand may be docked against any of them. Delete the extra chains in "
      "the Builder if one site is wanted.");
  }
}

bool allDigits(const std::string& s)
{
  if (s.empty()) return false;
  for (size_t i = 0; i < s.size(); i++) if (!isdigit((unsigned char)s[i])) return false;
  return true;
}

} // namespace

std::string DockSplit::ranges(const std::vector<int>& zeroBased)
{
  std::vector<int> n(zeroBased);
  std::sort(n.begin(), n.end());
  std::ostringstream o;
  for (size_t i = 0; i < n.size();) {
    size_t j = i;
    while (j + 1 < n.size() && n[j+1] == n[j] + 1) j++;
    if (i) o << ",";
    if (j == i) o << n[i] + 1; else o << n[i] + 1 << "-" << n[j] + 1;
    i = j + 1;
  }
  return o.str();
}

DockSplitResult DockSplit::split(const std::vector<DockSplitAtom>& a,
                                 const std::string& spec)
{
  DockSplitResult res;
  const int natoms = (int)a.size();
  std::vector<char> lig(natoms, 0);

  if (spec.find_first_not_of(" \t\r\n") != std::string::npos) {
    std::string s(spec);
    for (size_t i = 0; i < s.size(); i++) if (s[i] == ',' || isspace((unsigned char)s[i])) s[i] = ' ';
    std::istringstream ss(s);
    std::string part;
    std::set<long long> bad;
    while (ss >> part) {
      long long lo = -1, hi = -1;
      size_t d = part.find('-');
      if (d != std::string::npos && allDigits(part.substr(0, d)) &&
          allDigits(part.substr(d + 1)) &&
          (lo = atoll(part.substr(0, d).c_str())) <= (hi = atoll(part.substr(d + 1).c_str()))) {
      } else if (allDigits(part)) {
        lo = hi = atoll(part.c_str());
      } else {
        res.error = "docking error: ligand atoms '" + spec + "': '" + part +
                    "' is not a number or a range";
        return res;
      }
      // Out-of-range atoms are reported, smallest first, after the whole
      // spec parsed; never loop over a huge range.
      if (lo < 1) bad.insert(lo);
      else if (hi > natoms) bad.insert(std::max<long long>(lo, natoms + 1));
      for (long long k = std::max<long long>(lo, 1); k <= std::min<long long>(hi, natoms); k++)
        lig[k-1] = 1;
    }
    if (!bad.empty()) {
      std::ostringstream o;
      o << "docking error: ligand atom " << *bad.begin() << " is not in the "
        << "system (" << natoms << " atoms)";
      res.error = o.str();
      return res;
    }
    receptorWarnings(a, lig, -1, res.warnings);
  } else {
    // Cells as large as the longest possible bond keep the neighbour search
    // linear in the number of atoms.
    std::vector<double> r(natoms);
    double rmax = 0;
    for (int i = 0; i < natoms; i++) { r[i] = radius(a[i].element); rmax = std::max(rmax, r[i]); }
    const double cs = 1.25 * 2 * rmax;
    typedef std::vector<long long> Key;
    std::vector<Key> key(natoms);
    std::map<Key, std::vector<int> > cell;
    for (int i = 0; i < natoms; i++) {
      Key k = { (long long)std::floor(a[i].x / cs), (long long)std::floor(a[i].y / cs),
                (long long)std::floor(a[i].z / cs) };
      key[i] = k;
      cell[k].push_back(i);
    }
    std::vector<int> comp(natoms, -1);
    int n = 0;
    for (int s = 0; s < natoms; s++) {
      if (comp[s] >= 0) continue;
      std::vector<int> stack(1, s);
      comp[s] = n;
      while (!stack.empty()) {
        int i = stack.back(); stack.pop_back();
        for (int da = -1; da <= 1; da++) for (int db = -1; db <= 1; db++)
        for (int dc = -1; dc <= 1; dc++) {
          Key k = { key[i][0] + da, key[i][1] + db, key[i][2] + dc };
          std::map<Key, std::vector<int> >::const_iterator it = cell.find(k);
          if (it == cell.end()) continue;
          for (size_t q = 0; q < it->second.size(); q++) {
            int j = it->second[q];
            if (comp[j] >= 0) continue;
            double dx = a[i].x - a[j].x, dy = a[i].y - a[j].y, dz = a[i].z - a[j].z;
            double lim = 1.25 * (r[i] + r[j]);
            if (dx*dx + dy*dy + dz*dz < lim*lim) { comp[j] = n; stack.push_back(j); }
          }
        }
      }
      n++;
    }
    if (n < 2) {
      res.error = "docking error: the system is one molecule; give the ligand "
                  "atoms in the Runtype Details";
      return res;
    }
    std::vector<std::vector<int> > members(n);
    for (int i = 0; i < natoms; i++) members[comp[i]].push_back(i);

    std::vector<char> water(n), ion(n), poly(n);
    std::vector<std::string> name(n);
    for (int c = 0; c < n; c++) {
      const std::vector<int>& m = members[c];
      std::set<std::string> rn;
      int np = 0, o = 0, h = 0;
      for (size_t q = 0; q < m.size(); q++) {
        const DockSplitAtom& at = a[m[q]];
        rn.insert(at.resName);
        if (isPolymer(at.resName)) np++;
        if (at.element == "O") o++;
        if (at.element == "H" || at.element == "D") h++;
      }
      ion[c] = m.size() == 1;
      water[c] = (m.size() <= 3 && o == 1 && o + h == (int)m.size()) ||
                 (rn.size() == 1 && isWater(*rn.begin()));
      poly[c] = np > 0 && np * 2 > (int)m.size();
      for (std::set<std::string>::const_iterator it = rn.begin(); it != rn.end(); ++it)
        if (!it->empty()) name[c] += (name[c].empty() ? "" : "/") + *it;
    }
    std::vector<int> cand, np;
    for (int c = 0; c < n; c++) if (!water[c] && !ion[c]) cand.push_back(c);
    int best = 0;
    if (cand.size() >= 2) {
      for (size_t q = 0; q < cand.size(); q++) if (!poly[cand[q]]) np.push_back(cand[q]);
      if (np.size() < cand.size() && !np.empty()) {
        best = np[0];   // a polymer is present: the largest other molecule
        for (size_t q = 0; q < np.size(); q++)
          if (members[np[q]].size() > members[best].size()) best = np[q];
      } else {
        best = cand[0];
        for (size_t q = 0; q < cand.size(); q++)
          if (members[cand[q]].size() < members[best].size()) best = cand[q];
      }
    } else {
      for (int c = 1; c < n; c++) if (members[c].size() < members[best].size()) best = c;
    }
    for (size_t q = 0; q < members[best].size(); q++) lig[members[best][q]] = 1;
    std::ostringstream w;
    w << "the ligand was chosen automatically: "
      << (name[best].empty() ? "" : "residue " + name[best] + ", ")
      << members[best].size() << " atoms (atoms " << ranges(members[best])
      << "). If that is not the molecule to dock, give the Ligand atoms in "
         "the Runtype Details.";
    res.warnings.push_back(w.str());
    int nw = 0;
    for (int c = 0; c < n; c++) if (water[c] && c != best) nw++;
    receptorWarnings(a, lig, nw, res.warnings);
    // A receptor with no ligand leaves a piece of the protein as "smallest
    // molecule"; the old script did that silently.
    if (poly[best])
      res.warnings.push_back("the automatically chosen ligand is made of "
        "standard amino-acid or nucleotide residues, so the system may hold "
        "no ligand at all. Check the Ligand atoms in the Runtype Details.");
  }
  for (int i = 0; i < natoms; i++) (lig[i] ? res.ligand : res.receptor).push_back(i);
  if (res.receptor.empty() || res.ligand.empty()) {
    res.error = "docking error: the receptor or the ligand has no atoms";
    return res;
  }
  res.ok = true;
  return res;
}

bool DockSplit::readPdb(const std::string& path,
                        std::vector<DockSplitAtom>& atoms, std::string& err)
{
  std::ifstream in(path.c_str());
  if (!in) { err = "cannot open " + path; return false; }
  std::string l;
  auto field = [](const std::string& s, size_t p, size_t n) {
    std::string f = p < s.size() ? s.substr(p, n) : "";
    size_t b = f.find_first_not_of(" \t\r\n"), e = f.find_last_not_of(" \t\r\n");
    return b == std::string::npos ? std::string() : f.substr(b, e - b + 1);
  };
  while (std::getline(in, l)) {
    if (l.compare(0, 6, "ENDMDL") == 0) break;
    if (l.compare(0, 6, "ATOM  ") != 0 && l.compare(0, 6, "HETATM") != 0) continue;
    DockSplitAtom a;
    std::string el = field(l, 76, 2);
    if (el.empty()) {
      std::string nm = l.size() > 12 ? l.substr(12, 2) : "";
      for (size_t i = 0; i < nm.size(); i++)
        if (!isspace((unsigned char)nm[i]) && !isdigit((unsigned char)nm[i])) el += nm[i];
    }
    for (size_t i = 0; i < el.size(); i++) el[i] = i ? tolower(el[i]) : toupper(el[i]);
    a.element = el;
    a.x = atof(field(l, 30, 8).c_str());
    a.y = atof(field(l, 38, 8).c_str());
    a.z = atof(field(l, 46, 8).c_str());
    a.resName = field(l, 17, 3);
    a.resNum = field(l, 22, 4);
    a.chain = l.size() > 21 ? l.substr(21, 1) : " ";
    atoms.push_back(a);
  }
  return true;
}

std::string DockSplit::report(const std::vector<DockSplitAtom>& a,
                              const DockSplitResult& r)
{
  std::ostringstream o;
  if (!r.ok) { o << "ERROR\t" << r.error << "\n"; return o.str(); }
  std::set<std::string> seen;
  std::string rn;
  for (size_t q = 0; q < r.ligand.size(); q++) {
    const DockSplitAtom& at = a[r.ligand[q]];
    std::string k = at.resName + at.resNum + at.chain;
    if (seen.insert(k).second) rn += (rn.empty() ? "" : ",") + k;
  }
  o << "atoms\t" << a.size() << "\nligand_n\t" << r.ligand.size()
    << "\nligand_res\t" << rn << "\nligand_atoms\t" << ranges(r.ligand)
    << "\nreceptor_n\t" << r.receptor.size() << "\n";
  for (size_t q = 0; q < r.warnings.size(); q++) o << "warn\t" << r.warnings[q] << "\n";
  return o.str();
}
