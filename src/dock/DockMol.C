#include "dock/DockMol.H"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <queue>
#include <set>

namespace {

struct ElemRec { const char* sym; double rcov; };

// Covalent radii (Cordero 2008), enough elements for proteins and drug-like
// ligands; anything missing falls back to 1.5 A.
const ElemRec ELEMENTS[] = {
  {"X",0.0},  {"H",0.31}, {"He",0.28},{"Li",1.28},{"Be",0.96},{"B",0.84},
  {"C",0.76}, {"N",0.71}, {"O",0.66}, {"F",0.57}, {"Ne",0.58},{"Na",1.66},
  {"Mg",1.41},{"Al",1.21},{"Si",1.11},{"P",1.07}, {"S",1.05}, {"Cl",1.02},
  {"Ar",1.06},{"K",2.03}, {"Ca",1.76},{"Sc",1.70},{"Ti",1.60},{"V",1.53},
  {"Cr",1.39},{"Mn",1.39},{"Fe",1.32},{"Co",1.26},{"Ni",1.24},{"Cu",1.32},
  {"Zn",1.22},{"Ga",1.22},{"Ge",1.20},{"As",1.19},{"Se",1.20},{"Br",1.20},
  {"Kr",1.16},{"Rb",2.20},{"Sr",1.95},{"Y",1.90}, {"Zr",1.75},{"Nb",1.64},
  {"Mo",1.54},{"Tc",1.47},{"Ru",1.46},{"Rh",1.42},{"Pd",1.39},{"Ag",1.45},
  {"Cd",1.44},{"In",1.42},{"Sn",1.39},{"Sb",1.39},{"Te",1.38},{"I",1.39},
  {"Xe",1.40}
};
const int NELEM = (int)(sizeof(ELEMENTS)/sizeof(ElemRec));


}

int DockMol::elementFromSymbol(const std::string& sym)
{
  if (sym.empty()) return 0;
  std::string s = sym;
  // Trim and normalise "CL" / "cl" to "Cl".
  s.erase(0, s.find_first_not_of(" \t"));
  size_t e = s.find_last_not_of(" \t0123456789");
  if (e != std::string::npos) s = s.substr(0, e+1);
  if (s.empty()) return 0;
  s[0] = (char)toupper(s[0]);
  for (size_t i = 1; i < s.size(); i++) s[i] = (char)tolower(s[i]);
  for (int i = 1; i < NELEM; i++)
    if (s == ELEMENTS[i].sym) return i;
  return 0;
}

std::string DockMol::symbolFromElement(int z)
{
  if (z > 0 && z < NELEM) return ELEMENTS[z].sym;
  return "X";
}

double DockMol::covalentRadius(int z)
{
  if (z > 0 && z < NELEM) return ELEMENTS[z].rcov;
  return 1.5;
}

bool DockMol::isMetal(int z)
{
  if (z == 3 || z == 4) return true;
  if (z >= 11 && z <= 13 && z != 13) return true;   // Na, Mg
  if (z == 13) return true;                          // Al
  if (z == 19 || z == 20) return true;               // K, Ca
  if (z >= 21 && z <= 31) return true;               // Sc..Ga
  if (z >= 37 && z <= 49) return true;
  return false;
}

void DockMol::addBond(int i, int j, int order)
{
  if (i == j) return;
  if (findBond(i, j) >= 0) return;
  DockBond b; b.i = i; b.j = j; b.order = order;
  bonds.push_back(b);
  p_adjValid = false;
}

int DockMol::findBond(int i, int j) const
{
  for (size_t k = 0; k < bonds.size(); k++)
    if ((bonds[k].i == i && bonds[k].j == j) ||
        (bonds[k].i == j && bonds[k].j == i)) return (int)k;
  return -1;
}

void DockMol::buildAdjacency() const
{
  p_adj.assign(atoms.size(), std::vector<int>());
  for (size_t k = 0; k < bonds.size(); k++) {
    p_adj[bonds[k].i].push_back(bonds[k].j);
    p_adj[bonds[k].j].push_back(bonds[k].i);
  }
  p_adjValid = true;
}

const std::vector<int>& DockMol::neighbors(int i) const
{
  if (!p_adjValid) buildAdjacency();
  return p_adj[i];
}

std::vector<int> DockMol::heavyNeighbors(int i) const
{
  std::vector<int> out;
  const std::vector<int>& nb = neighbors(i);
  for (size_t k = 0; k < nb.size(); k++)
    if (atoms[nb[k]].z != 1) out.push_back(nb[k]);
  return out;
}

int DockMol::valence(int i) const
{
  int v = 0;
  for (size_t k = 0; k < bonds.size(); k++)
    if (bonds[k].i == i || bonds[k].j == i)
      v += (bonds[k].order == 4 ? 1 : bonds[k].order);
  // An aromatic-flagged ring atom has one more formal bond than the
  // single bonds say; approximate the SMARTS "v" primitive.
  if (atoms[i].aromatic) {
    int narom = 0;
    for (size_t k = 0; k < bonds.size(); k++)
      if ((bonds[k].i == i || bonds[k].j == i) && bonds[k].order == 4) narom++;
    if (narom >= 2) v += 1;
  }
  return v;
}

void DockMol::perceiveBonds()
{
  bonds.clear();
  p_adjValid = false;
  // O(N^2) is fine: receptors are split per chain before this is called and
  // the largest case here is a few thousand atoms.
  for (size_t i = 0; i < atoms.size(); i++) {
    for (size_t j = i+1; j < atoms.size(); j++) {
      if (atoms[i].z == 1 && atoms[j].z == 1) continue;
      double dx = atoms[i].x - atoms[j].x;
      double dy = atoms[i].y - atoms[j].y;
      double dz = atoms[i].zc - atoms[j].zc;
      double d2 = dx*dx + dy*dy + dz*dz;
      double cut = covalentRadius(atoms[i].z) + covalentRadius(atoms[j].z) + 0.45;
      if (d2 < cut*cut && d2 > 0.16) addBond((int)i, (int)j, 1);
    }
  }
  // No bond block means no bond orders; a short C=O or C=N still has to be
  // seen as a double bond or every amide would come out rotatable.
  for (size_t k = 0; k < bonds.size(); k++) {
    int zi = atoms[bonds[k].i].z, zj = atoms[bonds[k].j].z;
    if (zi > zj) std::swap(zi, zj);
    double dx = atoms[bonds[k].i].x  - atoms[bonds[k].j].x;
    double dy = atoms[bonds[k].i].y  - atoms[bonds[k].j].y;
    double dz = atoms[bonds[k].i].zc - atoms[bonds[k].j].zc;
    double d = sqrt(dx*dx + dy*dy + dz*dz);
    if      (zi == 6 && zj == 8  && d < 1.30) bonds[k].order = 2;
    else if (zi == 6 && zj == 7  && d < 1.32) bonds[k].order = 2;
    else if (zi == 6 && zj == 6  && d < 1.36) bonds[k].order = 2;
    else if (zi == 7 && zj == 8  && d < 1.30) bonds[k].order = 2;
    else if (zi == 6 && zj == 16 && d < 1.70) bonds[k].order = 2;
  }
}

void DockMol::perceiveRings()
{
  p_rings.clear();
  for (size_t k = 0; k < bonds.size(); k++) bonds[k].inRing = false;
  for (size_t i = 0; i < atoms.size(); i++) atoms[i].inRing = false;

  // Smallest ring through each bond, found by a BFS that is not allowed to
  // use that bond.  This is the usual cheap stand-in for a proper SSSR and is
  // adequate because only ring membership and ring size are used downstream.
  std::set< std::vector<int> > seen;
  for (size_t k = 0; k < bonds.size(); k++) {
    int s = bonds[k].i, t = bonds[k].j;
    std::vector<int> prev(atoms.size(), -1);
    std::vector<int> dist(atoms.size(), -1);
    std::queue<int> q;
    q.push(s); dist[s] = 0;
    while (!q.empty()) {
      int c = q.front(); q.pop();
      if (c == t) break;
      if (dist[c] >= 9) continue;
      const std::vector<int>& nb = neighbors(c);
      for (size_t m = 0; m < nb.size(); m++) {
        int n = nb[m];
        if (c == s && n == t) continue;   // the bond itself
        if (c == t && n == s) continue;
        if (dist[n] >= 0) continue;
        dist[n] = dist[c] + 1; prev[n] = c; q.push(n);
      }
    }
    if (dist[t] < 0) continue;
    std::vector<int> ring;
    for (int c = t; c >= 0; c = prev[c]) { ring.push_back(c); if (c == s) break; }
    if ((int)ring.size() < 3 || (int)ring.size() > 10) continue;
    std::vector<int> key = ring;
    std::sort(key.begin(), key.end());
    if (seen.count(key)) continue;
    seen.insert(key);
    p_rings.push_back(ring);
  }

  for (size_t r = 0; r < p_rings.size(); r++) {
    const std::vector<int>& ring = p_rings[r];
    for (size_t a = 0; a < ring.size(); a++) {
      atoms[ring[a]].inRing = true;
      int b = findBond(ring[a], ring[(a+1)%ring.size()]);
      if (b >= 0) bonds[b].inRing = true;
    }
  }
}

void DockMol::perceiveAromaticity()
{
  for (size_t i = 0; i < atoms.size(); i++) {
    atoms[i].aromatic = false;
    int maxOrder = 1, nDouble = 0;
    for (size_t k = 0; k < bonds.size(); k++) {
      if (bonds[k].i != (int)i && bonds[k].j != (int)i) continue;
      if (bonds[k].order == 3) maxOrder = std::max(maxOrder, 3);
      if (bonds[k].order == 2) { nDouble++; maxOrder = std::max(maxOrder, 2); }
    }
    if (maxOrder == 3 || nDouble >= 2) atoms[i].hybrid = DOCK_HYB_SP;
    else if (maxOrder == 2)            atoms[i].hybrid = DOCK_HYB_SP2;
    else                               atoms[i].hybrid = DOCK_HYB_SP3;
  }

  // Huckel, ring by ring first and then over the fused system.  A ring
  // carrying an exocyclic double bond to O, N or S is not aromatic - that is
  // what keeps caffeine's pyrimidinedione and warfarin's lactone aliphatic -
  // while indole and purine only reach 4n+2 when the system is taken whole.
  int nr = (int)p_rings.size();
  std::vector< std::set<int> > groups;
  for (int r = 0; r < nr; r++)
    groups.push_back(std::set<int>(p_rings[r].begin(), p_rings[r].end()));

  std::vector<int> sys(nr, -1);
  int nsys = 0;
  for (int r = 0; r < nr; r++) {
    if (sys[r] >= 0) continue;
    sys[r] = nsys;
    bool changed = true;
    while (changed) {
      changed = false;
      for (int a = 0; a < nr; a++) {
        if (sys[a] != nsys) continue;
        for (int b = 0; b < nr; b++) {
          if (sys[b] >= 0) continue;
          int shared = 0;
          for (std::set<int>::iterator it = groups[b].begin();
               it != groups[b].end(); ++it)
            if (groups[a].count(*it)) shared++;
          if (shared >= 2) { sys[b] = nsys; changed = true; }
        }
      }
    }
    nsys++;
  }
  for (int s = 0; s < nsys; s++) {
    std::set<int> u;
    int count = 0;
    for (int r = 0; r < nr; r++)
      if (sys[r] == s) { u.insert(groups[r].begin(), groups[r].end()); count++; }
    if (count > 1) groups.push_back(u);
  }

  for (size_t g = 0; g < groups.size(); g++) {
    const std::set<int>& members = groups[g];
    if (members.empty()) continue;
    int pi = 0; bool ok = true;
    for (std::set<int>::const_iterator it = members.begin();
         it != members.end(); ++it) {
      int i = *it;
      const DockAtom& a = atoms[i];
      if (a.z != 6 && a.z != 7 && a.z != 8 && a.z != 16) { ok = false; break; }
      int dblIn = 0, dblOutC = 0, dblOutHet = 0;
      for (size_t k = 0; k < bonds.size(); k++) {
        if (bonds[k].order != 2) continue;
        int other = -1;
        if (bonds[k].i == i) other = bonds[k].j;
        else if (bonds[k].j == i) other = bonds[k].i;
        else continue;
        if (members.count(other)) dblIn++;
        else if (atoms[other].z == 6) dblOutC++;
        else dblOutHet++;
      }
      if (dblOutHet) { ok = false; break; }
      if (dblIn >= 1)  { pi += 1; continue; }
      if (dblOutC >= 1) { pi += 0; continue; }
      if (a.z == 7 || a.z == 8 || a.z == 16) {
        if (a.formalCharge > 0) { ok = false; break; }
        pi += 2; continue;                         // pyrrole, furan, thiophene
      }
      if (a.formalCharge < 0) { pi += 2; continue; }
      ok = false; break;                           // an sp3 carbon kills it
    }
    if (!ok) continue;
    if (pi < 2 || (pi - 2) % 4 != 0) continue;
    for (std::set<int>::const_iterator it = members.begin();
         it != members.end(); ++it) {
      atoms[*it].aromatic = true;
      atoms[*it].hybrid = DOCK_HYB_SP2;
    }
  }
}

void DockMol::perceive(bool withBonds)
{
  if (withBonds) perceiveBonds();
  p_adjValid = false;
  perceiveRings();
  perceiveAromaticity();
  // Geometry-perceived bonds are all single, so the electron count can never
  // find a ring; fall back to the planarity test.
  if (withBonds) perceiveAromaticityPlanar();
}

void DockMol::perceiveAromaticityPlanar()
{
  for (size_t r = 0; r < p_rings.size(); r++) {
    const std::vector<int>& ring = p_rings[r];
    if (ring.size() != 5 && ring.size() != 6) continue;
    bool ok = true;
    double cx = 0, cy = 0, cz = 0;
    for (size_t a = 0; a < ring.size(); a++) {
      const DockAtom& at = atoms[ring[a]];
      if (at.z != 6 && at.z != 7 && at.z != 8 && at.z != 16) { ok = false; break; }
      if (connections(ring[a]) > 3) { ok = false; break; }
      cx += at.x; cy += at.y; cz += at.zc;
    }
    if (!ok) continue;
    cx /= ring.size(); cy /= ring.size(); cz /= ring.size();
    // Normal from three well-separated ring atoms, then the largest
    // out-of-plane deviation.
    const DockAtom& a0 = atoms[ring[0]];
    const DockAtom& a1 = atoms[ring[ring.size()/3]];
    const DockAtom& a2 = atoms[ring[2*ring.size()/3]];
    double u[3] = {a1.x-a0.x, a1.y-a0.y, a1.zc-a0.zc};
    double v[3] = {a2.x-a0.x, a2.y-a0.y, a2.zc-a0.zc};
    double n[3] = {u[1]*v[2]-u[2]*v[1], u[2]*v[0]-u[0]*v[2], u[0]*v[1]-u[1]*v[0]};
    double len = sqrt(n[0]*n[0]+n[1]*n[1]+n[2]*n[2]);
    if (len < 1e-6) continue;
    n[0]/=len; n[1]/=len; n[2]/=len;
    double worst = 0.0;
    for (size_t a = 0; a < ring.size(); a++) {
      const DockAtom& at = atoms[ring[a]];
      double d = fabs(n[0]*(at.x-cx) + n[1]*(at.y-cy) + n[2]*(at.zc-cz));
      worst = std::max(worst, d);
    }
    if (worst > 0.12) continue;
    for (size_t a = 0; a < ring.size(); a++) {
      atoms[ring[a]].aromatic = true;
      atoms[ring[a]].hybrid = DOCK_HYB_SP2;
    }
  }
}

void DockMol::subset(const std::vector<int>& keep, DockMol& out,
                     std::vector<int>& map) const
{
  out.atoms.clear(); out.bonds.clear();
  map.clear();
  std::map<int,int> back;
  for (size_t k = 0; k < keep.size(); k++) {
    int i = keep[k];
    if (i < 0 || i >= (int)atoms.size()) continue;
    if (back.count(i)) continue;
    back[i] = (int)out.atoms.size();
    map.push_back(i);
    out.atoms.push_back(atoms[i]);
  }
  for (size_t b = 0; b < bonds.size(); b++) {
    std::map<int,int>::const_iterator ii = back.find(bonds[b].i);
    std::map<int,int>::const_iterator jj = back.find(bonds[b].j);
    if (ii == back.end() || jj == back.end()) continue;
    out.addBond(ii->second, jj->second, bonds[b].order);
  }
}
