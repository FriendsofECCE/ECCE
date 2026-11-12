#include "dock/DockPrep.H"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <map>
#include <set>
#include <sstream>

namespace {

std::string trim(const std::string& s)
{
  size_t a = s.find_first_not_of(" \t\r\n");
  if (a == std::string::npos) return "";
  size_t b = s.find_last_not_of(" \t\r\n");
  return s.substr(a, b-a+1);
}

std::string col(const std::string& line, size_t from, size_t len)
{
  if (line.size() <= from) return "";
  return trim(line.substr(from, std::min(len, line.size()-from)));
}

}

// ------------------------------------------------------------------ SDF ---

bool DockReader::readSdf(const std::string& path, DockMol& mol, std::string& err)
{
  std::ifstream is(path.c_str());
  if (!is) { err = "cannot open " + path; return false; }
  std::string l1, l2, l3, counts;
  std::getline(is, l1); std::getline(is, l2); std::getline(is, l3);
  std::getline(is, counts);
  if (counts.size() < 6) { err = "short counts line in " + path; return false; }
  int na = atoi(counts.substr(0,3).c_str());
  int nb = atoi(counts.substr(3,3).c_str());
  if (na <= 0) { err = "no atoms in " + path; return false; }

  mol.atoms.clear(); mol.bonds.clear();
  for (int i = 0; i < na; i++) {
    std::string line;
    if (!std::getline(is, line)) { err = "truncated atom block"; return false; }
    DockAtom a;
    a.x  = atof(line.substr(0,10).c_str());
    a.y  = atof(line.substr(10,10).c_str());
    a.zc = atof(line.substr(20,10).c_str());
    a.z  = DockMol::elementFromSymbol(col(line, 31, 3));
    // The old charge code in columns 37-39: 1 means +3 down to 7 meaning -3.
    int cc = (line.size() >= 39) ? atoi(line.substr(36,3).c_str()) : 0;
    if (cc >= 1 && cc <= 7 && cc != 4) a.formalCharge = 4 - cc;
    a.resName = "UNL";
    mol.atoms.push_back(a);
  }
  for (int i = 0; i < nb; i++) {
    std::string line;
    if (!std::getline(is, line)) { err = "truncated bond block"; return false; }
    int ai = atoi(line.substr(0,3).c_str()) - 1;
    int aj = atoi(line.substr(3,3).c_str()) - 1;
    int o  = atoi(line.substr(6,3).c_str());
    if (ai < 0 || aj < 0 || ai >= na || aj >= na) continue;
    mol.addBond(ai, aj, o);
  }
  // M  CHG overrides the old charge column.
  std::string line;
  while (std::getline(is, line)) {
    if (line.compare(0, 6, "M  CHG") == 0) {
      std::istringstream ss(line.substr(6));
      int cnt; ss >> cnt;
      for (int k = 0; k < cnt; k++) {
        int idx, ch; ss >> idx >> ch;
        if (idx >= 1 && idx <= na) mol.atoms[idx-1].formalCharge = ch;
      }
    }
    if (line.compare(0, 4, "$$$$") == 0) break;
    if (line.compare(0, 6, "M  END") == 0) break;
  }
  mol.perceive(false);
  return true;
}

// ------------------------------------------------------------------ PDB ---

bool DockReader::readPdb(const std::string& path, DockMol& mol,
                         const std::string& hetSelect, std::string& err)
{
  std::ifstream is(path.c_str());
  if (!is) { err = "cannot open " + path; return false; }
  mol.atoms.clear(); mol.bonds.clear();

  std::set<std::string> want;
  { std::string s = hetSelect; size_t p;
    while (!(s = trim(s)).empty()) {
      p = s.find(',');
      want.insert(trim(s.substr(0, p)));
      if (p == std::string::npos) break;
      s = s.substr(p+1);
    } }

  std::string line;
  std::map<int,int> bySerial;
  while (std::getline(is, line)) {
    if (line.compare(0, 6, "ENDMDL") == 0) break;
    bool isAtom = line.compare(0, 4, "ATOM") == 0;
    bool isHet  = line.compare(0, 6, "HETATM") == 0;
    if (!isAtom && !isHet) continue;
    if (line.size() < 54) continue;
    std::string resName = col(line, 17, 3);
    if (resName == "HOH" || resName == "WAT" || resName == "DOD") continue;
    if (isAtom && !want.empty()) continue;      // ligand mode: HETATM only
    if (isHet && want.empty()) continue;        // receptor mode: protein only
    if (isHet && !want.empty() && !want.count(resName)) continue;
    // Keep only the first alternate location.
    std::string alt = col(line, 16, 1);
    if (!alt.empty() && alt != "A") continue;

    DockAtom a;
    a.name    = col(line, 12, 4);
    a.resName = resName;
    a.chain   = col(line, 21, 1);
    a.resSeq  = atoi(col(line, 22, 4).c_str());
    a.x  = atof(line.substr(30, 8).c_str());
    a.y  = atof(line.substr(38, 8).c_str());
    a.zc = atof(line.substr(46, 8).c_str());
    std::string sym = (line.size() >= 78) ? col(line, 76, 2) : "";
    if (sym.empty() || DockMol::elementFromSymbol(sym) == 0) {
      // Fall back to the atom name; a leading digit is a hydrogen counter.
      std::string n = a.name;
      size_t p = n.find_first_not_of("0123456789");
      if (p != std::string::npos) n = n.substr(p);
      sym = n.substr(0, (n.size() > 1 && isalpha(n[1]) && n.size() == 2) ? 2 : 1);
    }
    a.z = DockMol::elementFromSymbol(sym);
    if (a.z == 0) continue;
    bySerial[atoi(col(line, 6, 5).c_str())] = (int)mol.atoms.size();
    mol.atoms.push_back(a);
  }
  if (mol.atoms.empty()) { err = "no atoms selected from " + path; return false; }
  mol.perceive(true);
  return true;
}

// --------------------------------------------------------------- PDBQT ----

bool DockReader::readPdbqtModels(const std::string& path,
                                 std::vector< std::vector<double> >& coords,
                                 std::vector<double>& scores,
                                 std::string& err)
{
  std::ifstream is(path.c_str());
  if (!is) { err = "cannot open " + path; return false; }
  coords.clear(); scores.clear();
  std::vector<double> cur;
  std::string line;
  bool inModel = false;
  while (std::getline(is, line)) {
    if (line.compare(0, 5, "MODEL") == 0) { inModel = true; cur.clear(); continue; }
    if (line.compare(0, 6, "ENDMDL") == 0) {
      if (inModel) coords.push_back(cur);
      inModel = false; continue;
    }
    if (line.compare(0, 6, "REMARK") == 0 &&
        line.find("VINA RESULT") != std::string::npos) {
      std::istringstream ss(line.substr(line.find("RESULT:") + 7));
      double v; ss >> v; scores.push_back(v);
      continue;
    }
    if ((line.compare(0, 4, "ATOM") == 0 || line.compare(0, 6, "HETATM") == 0)
        && line.size() >= 54) {
      cur.push_back(atof(line.substr(30, 8).c_str()));
      cur.push_back(atof(line.substr(38, 8).c_str()));
      cur.push_back(atof(line.substr(46, 8).c_str()));
    }
  }
  if (coords.empty()) { err = "no MODEL records in " + path; return false; }
  return true;
}

// -------------------------------------------------------- protonation ----

namespace {

/**
 * Polar hydrogens of the standard residues at pH 7: Asp and Glu are
 * deprotonated, Lys and Arg are protonated, His is the neutral N-epsilon
 * tautomer, Cys stays as the thiol.  Per-residue edits go through the same
 * table, so changing a state is a data change, not a code change.
 */
struct PolarH { const char* res; const char* atom; int n; bool sp2; };

const PolarH POLAR_H[] = {
  {"*",   "N",   1, true },   // backbone amide, except proline and the N terminus
  {"SER", "OG",  1, false},
  {"THR", "OG1", 1, false},
  {"TYR", "OH",  1, false},
  {"CYS", "SG",  1, false},
  {"LYS", "NZ",  3, false},
  {"ARG", "NE",  1, true },
  {"ARG", "NH1", 2, true },
  {"ARG", "NH2", 2, true },
  {"ASN", "ND2", 2, true },
  {"GLN", "NE2", 2, true },
  {"HIS", "NE2", 1, true },
  {"HIE", "NE2", 1, true },
  {"HID", "ND1", 1, true },
  {"HIP", "ND1", 1, true },
  {"HIP", "NE2", 1, true },
  {"TRP", "NE1", 1, true }
};
const int NPOLAR = (int)(sizeof(POLAR_H)/sizeof(PolarH));

/** Formal charges of the standard residues at pH 7. */
struct ResCharge { const char* res; const char* atom; int q; };
const ResCharge RES_CHARGE[] = {
  {"LYS", "NZ",  +1}, {"ARG", "NH1", +1}, {"HIP", "ND1", +1},
  {"ASP", "OD2", -1}, {"GLU", "OE2", -1}
};
/** Nitrogens a residue only ever donates from: ammonium and guanidinium. */
const ResCharge RES_DONOR[] = {
  {"LYS", "NZ", 1}, {"ARG", "NE", 1}, {"ARG", "NH1", 1}, {"ARG", "NH2", 1}
};
const int NRESD = (int)(sizeof(RES_DONOR)/sizeof(ResCharge));
const int NRESQ = (int)(sizeof(RES_CHARGE)/sizeof(ResCharge));

void norm(double v[3])
{
  double n = sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
  if (n > 1e-9) { v[0]/=n; v[1]/=n; v[2]/=n; }
}

void cross(const double a[3], const double b[3], double o[3])
{
  o[0] = a[1]*b[2]-a[2]*b[1];
  o[1] = a[2]*b[0]-a[0]*b[2];
  o[2] = a[0]*b[1]-a[1]*b[0];
}

double bondLen(int z) { return (z == 8) ? 0.96 : (z == 16 ? 1.34 : 1.01); }

}

static int addPolarH(DockMol& mol, bool byTemplate);

int DockProtonator::addPolarHydrogens(DockMol& mol)
{ return addPolarH(mol, true); }

int DockProtonator::completePolarHydrogens(DockMol& mol)
{ return addPolarH(mol, false); }

static int addPolarH(DockMol& mol, bool byTemplate)
{
  int n0 = mol.numAtoms();
  if (byTemplate)
    for (int i = 0; i < n0; i++)
      for (int r = 0; r < NRESQ; r++)
        if (mol.atoms[i].resName == RES_CHARGE[r].res &&
            mol.atoms[i].name == RES_CHARGE[r].atom)
          mol.atoms[i].formalCharge = RES_CHARGE[r].q;
  if (byTemplate)
    for (int i = 0; i < n0; i++)
      for (int r = 0; r < NRESD; r++)
        if (mol.atoms[i].resName == RES_DONOR[r].res &&
            mol.atoms[i].name == RES_DONOR[r].atom)
          mol.atoms[i].donorOnly = true;
  std::vector<DockAtom> add;
  std::vector< std::pair<int,int> > addBonds;   // (heavy, index into add)

  for (int i = 0; i < n0; i++) {
    const DockAtom& a = mol.atoms[i];
    if (a.z != 7 && a.z != 8 && a.z != 16) continue;
    // Already has a hydrogen in the file: trust the file.
    bool hasH = false;
    const std::vector<int>& nb = mol.neighbors(i);
    for (size_t k = 0; k < nb.size(); k++) if (mol.atoms[nb[k]].z == 1) hasH = true;
    if (hasH) continue;

    int need = 0; bool sp2 = true;
    if (!byTemplate) {
      int want = (a.z == 7) ? 3 : 2;
      need = want - mol.valence(i) + a.formalCharge;
      // An aromatic ring atom's missing bond order is the ring's, not a
      // hydrogen's: only a five-ring N-H is a real donor here.
      if (a.aromatic && mol.connections(i) >= 2) {
        bool fiveRing = false;
        for (size_t r = 0; r < mol.rings().size(); r++)
          if (mol.rings()[r].size() == 5)
            for (size_t m = 0; m < mol.rings()[r].size(); m++)
              if (mol.rings()[r][m] == i) fiveRing = true;
        if (!(fiveRing && a.z == 7 && mol.connections(i) == 2)) need = 0;
      }
      sp2 = (a.hybrid == DOCK_HYB_SP2);
      if (need > 0) {
        std::vector<int> hv0 = mol.heavyNeighbors(i);
        if ((int)hv0.size() + need > 4) need = 4 - (int)hv0.size();
      }
    }
    for (int r = 0; byTemplate && r < NPOLAR; r++) {
      if (a.name != POLAR_H[r].atom) continue;
      if (std::string(POLAR_H[r].res) != "*" && a.resName != POLAR_H[r].res) continue;
      need = POLAR_H[r].n; sp2 = POLAR_H[r].sp2;
    }
    if (byTemplate && a.name == "N" && a.resName == "PRO") need = 0;
    if (need == 0) continue;

    std::vector<int> hv = mol.heavyNeighbors(i);
    if (hv.empty()) continue;
    if ((int)hv.size() + need > 4) need = 4 - (int)hv.size();
    if (need <= 0) continue;
    double L = bondLen(a.z);

    double u[3][3];
    for (size_t k = 0; k < hv.size() && k < 3; k++) {
      u[k][0] = mol.atoms[hv[k]].x  - a.x;
      u[k][1] = mol.atoms[hv[k]].y  - a.y;
      u[k][2] = mol.atoms[hv[k]].zc - a.zc;
      norm(u[k]);
    }

    std::vector< std::vector<double> > dirs;
    if (hv.size() >= 2) {
      // Two heavy neighbours: the hydrogen bisects them on the far side.
      double d[3] = {-(u[0][0]+u[1][0]), -(u[0][1]+u[1][1]), -(u[0][2]+u[1][2])};
      norm(d);
      dirs.push_back(std::vector<double>(d, d+3));
    } else {
      // One heavy neighbour: build a local frame from a second-shell atom so
      // the hydrogen is staggered rather than eclipsing.
      double ref[3] = {0.0, 0.0, 1.0};
      std::vector<int> hv2 = mol.heavyNeighbors(hv[0]);
      for (size_t k = 0; k < hv2.size(); k++) {
        if (hv2[k] == i) continue;
        ref[0] = mol.atoms[hv2[k]].x  - mol.atoms[hv[0]].x;
        ref[1] = mol.atoms[hv2[k]].y  - mol.atoms[hv[0]].y;
        ref[2] = mol.atoms[hv2[k]].zc - mol.atoms[hv[0]].zc;
        break;
      }
      double perp[3], tmp[3];
      cross(u[0], ref, tmp); norm(tmp);
      if (fabs(tmp[0])+fabs(tmp[1])+fabs(tmp[2]) < 1e-6) { tmp[0]=1; tmp[1]=0; tmp[2]=0; }
      cross(tmp, u[0], perp); norm(perp);
      double theta = sp2 ? (120.0*M_PI/180.0) : (109.5*M_PI/180.0);
      for (int h = 0; h < need; h++) {
        double phi = (need == 1) ? (sp2 ? M_PI : M_PI)
                                 : (2.0*M_PI*h/need + (sp2 ? 0.0 : M_PI));
        double ax[3];
        cross(u[0], perp, ax); norm(ax);
        double d[3];
        for (int c = 0; c < 3; c++)
          d[c] = -cos(theta)*u[0][c]
                 + sin(theta)*(cos(phi)*perp[c] + sin(phi)*ax[c]);
        norm(d);
        dirs.push_back(std::vector<double>(d, d+3));
      }
    }

    for (size_t k = 0; k < dirs.size() && (int)k < need; k++) {
      DockAtom h;
      h.z = 1;
      h.x  = a.x  + L*dirs[k][0];
      h.y  = a.y  + L*dirs[k][1];
      h.zc = a.zc + L*dirs[k][2];
      h.resName = a.resName; h.chain = a.chain; h.resSeq = a.resSeq;
      h.name = "H" + a.name.substr(a.name.size() > 1 ? 1 : 0);
      if (h.name.size() > 4) h.name = h.name.substr(0, 4);
      addBonds.push_back(std::make_pair(i, (int)add.size()));
      add.push_back(h);
    }
  }

  for (size_t k = 0; k < add.size(); k++) mol.atoms.push_back(add[k]);
  for (size_t k = 0; k < addBonds.size(); k++)
    mol.addBond(addBonds[k].first, n0 + addBonds[k].second, 1);
  return (int)add.size();
}

// ----------------------------------------------------------------- frag ---

bool DockReader::readFrag(const std::string& path, DockMol& mol,
                          std::string& err)
{
  std::ifstream is(path.c_str());
  if (!is) { err = "cannot open " + path; return false; }
  mol.atoms.clear(); mol.bonds.clear();
  int natoms = 0, nbonds = 0;
  bool haveBonds = false;
  std::string line;
  while (std::getline(is, line)) {
    std::string t = trim(line);
    if (t.compare(0, 10, "num_atoms:") == 0) natoms = atoi(t.substr(10).c_str());
    else if (t.compare(0, 10, "num_bonds:") == 0) nbonds = atoi(t.substr(10).c_str());
    else if (t.compare(0, 10, "atom_list:") == 0) {
      for (int i = 0; i < natoms && std::getline(is, line); i++) {
        std::istringstream ss(line);
        std::string sym; DockAtom a;
        ss >> sym >> a.x >> a.y >> a.zc;
        a.z = DockMol::elementFromSymbol(sym);
        a.resName = "UNL";
        mol.atoms.push_back(a);
      }
    }
    else if (t.compare(0, 10, "bond_list:") == 0) {
      for (int i = 0; i < nbonds && std::getline(is, line); i++) {
        std::istringstream ss(line);
        int ai, aj; double o = 1.0;
        ss >> ai >> aj >> o;
        if (ai < 1 || aj < 1 || ai > (int)mol.atoms.size() ||
            aj > (int)mol.atoms.size()) continue;
        int order = (int)(o + 0.5);
        if (o > 1.4 && o < 1.6) order = 4;        // ECCE's aromatic order 1.5
        if (order < 1) order = 1;
        mol.addBond(ai-1, aj-1, order);
      }
      haveBonds = true;
    }
  }
  if (mol.atoms.empty()) { err = "no atoms in " + path; return false; }
  mol.perceive(!haveBonds);
  return true;
}
