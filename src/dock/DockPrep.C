#include "dock/DockPrep.H"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <set>
#include <sstream>

// ---------------------------------------------------------------- typing ---

namespace {

bool isPolarHeavy(int z) { return z==7 || z==8 || z==9 || z==15 || z==16; }

/** The amide/amidine pattern [NX3]-[CX3]=[O,N,S] that AutoDock keeps rigid. */
bool isAmideBond(const DockMol& mol, const DockBond& b)
{
  for (int pass = 0; pass < 2; pass++) {
    int n = pass ? b.j : b.i;
    int c = pass ? b.i : b.j;
    if (mol.atoms[n].z != 7 || mol.connections(n) != 3) continue;
    if (mol.atoms[c].z != 6 || mol.connections(c) != 3) continue;
    // An aromatic ring's own C=N is not an amide: an anilino-pyrimidine or
    // an N-acyl indole rotates freely about that bond.
    if (mol.atoms[n].aromatic || mol.atoms[c].aromatic) continue;
    const std::vector<int>& nb = mol.neighbors(c);
    for (size_t k = 0; k < nb.size(); k++) {
      if (nb[k] == n) continue;
      int z = mol.atoms[nb[k]].z;
      if (z != 7 && z != 8 && z != 16) continue;
      int bi = mol.findBond(c, nb[k]);
      if (bi >= 0 && mol.bonds[bi].order == 2) return true;
    }
  }
  return false;
}

}

void DockTyper::assign(DockMol& mol)
{
  // The rules and their order are AutoDock 4's, as the reference ligand
  // preparation applies them: a later rule overrides an earlier one.
  for (int i = 0; i < mol.numAtoms(); i++) {
    DockAtom& a = mol.atoms[i];
    int z = a.z;
    if (z == 1) {
      a.adType = "H";
      const std::vector<int>& nb = mol.neighbors(i);
      for (size_t k = 0; k < nb.size(); k++)
        if (isPolarHeavy(mol.atoms[nb[k]].z)) a.adType = "HD";
      continue;
    }
    if (z == 6)       a.adType = a.aromatic ? "A" : "C";
    else if (z == 7)  a.adType = "NA";
    else if (z == 8)  a.adType = "OA";
    else if (z == 16) a.adType = (mol.connections(i) == 2) ? "SA" : "S";
    else              a.adType = DockMol::symbolFromElement(z);
  }

  // Nitrogens that are not acceptors: pyrrole/aniline, amide, triazene and
  // any positively charged nitrogen.
  for (int i = 0; i < mol.numAtoms(); i++) {
    DockAtom& a = mol.atoms[i];
    if (a.z != 7) continue;
    if (a.formalCharge > 0 || a.donorOnly) { a.adType = "N"; continue; }
    if (mol.connections(i) != 3 || mol.valence(i) != 3) continue;
    const std::vector<int>& nb = mol.neighbors(i);
    for (size_t k = 0; k < nb.size(); k++) {
      int j = nb[k];
      const DockAtom& n = mol.atoms[j];
      if (n.aromatic) { a.adType = "N"; break; }
      if (n.z == 6 && mol.connections(j) == 3 && mol.valence(j) == 4) {
        a.adType = "N"; break;
      }
      if (n.z == 7 && mol.connections(j) == 2) {
        const std::vector<int>& nn = mol.neighbors(j);
        for (size_t m = 0; m < nn.size(); m++) {
          int bi = mol.findBond(j, nn[m]);
          if (nn[m] != i && bi >= 0 && mol.bonds[bi].order == 2) a.adType = "N";
        }
        if (a.adType == "N") break;
      }
    }
  }
}

// ------------------------------------------------------------- Gasteiger ---

namespace {

struct PeoeParam { int z; DockHybrid h; double a, b, c; };

// Gasteiger and Marsili, Tetrahedron 36 (1980) 3219.
const PeoeParam PEOE[] = {
  { 1, DOCK_HYB_UNKNOWN,  7.17,  6.24, -0.56},
  { 6, DOCK_HYB_SP3,      7.98,  9.18,  1.88},
  { 6, DOCK_HYB_SP2,      8.79,  9.32,  1.51},
  { 6, DOCK_HYB_SP,      10.39,  9.45,  0.73},
  { 7, DOCK_HYB_SP3,     11.54, 10.82,  1.36},
  { 7, DOCK_HYB_SP2,     12.87, 11.15,  0.85},
  { 7, DOCK_HYB_SP,      15.68, 11.70, -0.27},
  { 8, DOCK_HYB_SP3,     14.18, 12.92,  1.39},
  { 8, DOCK_HYB_SP2,     17.07, 13.79,  0.47},
  { 9, DOCK_HYB_UNKNOWN, 14.66, 13.85,  2.31},
  {15, DOCK_HYB_UNKNOWN,  8.90,  8.24,  0.96},
  {16, DOCK_HYB_UNKNOWN, 10.14,  9.13,  1.38},
  {17, DOCK_HYB_UNKNOWN, 11.00,  9.69,  1.35},
  {35, DOCK_HYB_UNKNOWN, 10.08,  8.47,  1.16},
  {53, DOCK_HYB_UNKNOWN,  9.90,  7.96,  0.96}
};
const int NPEOE = (int)(sizeof(PEOE)/sizeof(PeoeParam));

bool peoeFor(int z, DockHybrid h, double& a, double& b, double& c)
{
  const PeoeParam* fallback = 0;
  for (int i = 0; i < NPEOE; i++) {
    if (PEOE[i].z != z) continue;
    if (PEOE[i].h == DOCK_HYB_UNKNOWN || PEOE[i].h == h) {
      a = PEOE[i].a; b = PEOE[i].b; c = PEOE[i].c; return true;
    }
    if (!fallback) fallback = &PEOE[i];
  }
  if (fallback) { a = fallback->a; b = fallback->b; c = fallback->c; return true; }
  return false;
}

}

bool DockGasteiger::assign(DockMol& mol)
{
  int n = mol.numAtoms();
  std::vector<double> q(n, 0.0), A(n), B(n), C(n), Xplus(n);
  bool complete = true;
  for (int i = 0; i < n; i++) {
    if (!peoeFor(mol.atoms[i].z, mol.atoms[i].hybrid, A[i], B[i], C[i])) {
      // Metals and anything else without parameters simply keep their formal
      // charge; Vina's scoring ignores the charge column anyway.
      A[i] = B[i] = C[i] = 0.0;
      complete = false;
    }
    q[i] = (double)mol.atoms[i].formalCharge;
    Xplus[i] = (mol.atoms[i].z == 1) ? 20.02 : (A[i] + B[i] + C[i]);
    if (Xplus[i] <= 0.0) Xplus[i] = 1.0;
  }

  std::vector<double> X(n);
  for (int iter = 1; iter <= 6; iter++) {
    double damp = pow(0.5, iter);
    for (int i = 0; i < n; i++) X[i] = A[i] + B[i]*q[i] + C[i]*q[i]*q[i];
    std::vector<double> dq(n, 0.0);
    for (size_t k = 0; k < mol.bonds.size(); k++) {
      int i = mol.bonds[k].i, j = mol.bonds[k].j;
      if (A[i] == 0.0 || A[j] == 0.0) continue;
      int acc = i, don = j;
      if (X[j] > X[i]) { acc = j; don = i; }
      double d = (X[acc] - X[don]) / Xplus[don] * damp;
      dq[acc] -= d; dq[don] += d;
    }
    for (int i = 0; i < n; i++) q[i] += dq[i];
  }
  for (int i = 0; i < n; i++) mol.atoms[i].charge = q[i];
  return complete;
}

// ----------------------------------------------------------- torsion tree ---

void DockTorsionTree::build(DockMol& mol, bool flexibleAmides)
{
  nodes.clear();
  int n = mol.numAtoms();

  // A bond is rotatable when it is single, outside a ring and neither end is
  // terminal once the nonpolar hydrogens are gone.  Amides stay rigid.
  std::vector<int> heavyDeg(n, 0);
  for (int i = 0; i < n; i++) {
    if (mol.atoms[i].merged) continue;
    const std::vector<int>& nb = mol.neighbors(i);
    for (size_t k = 0; k < nb.size(); k++)
      if (!mol.atoms[nb[k]].merged) heavyDeg[i]++;
  }

  for (size_t k = 0; k < mol.bonds.size(); k++) {
    DockBond& b = mol.bonds[k];
    b.rotatable = false;
    if (b.order != 1 || b.inRing) continue;
    if (mol.atoms[b.i].merged || mol.atoms[b.j].merged) continue;
    if (heavyDeg[b.i] < 2 || heavyDeg[b.j] < 2) continue;
    if (mol.atoms[b.i].z == 1 || mol.atoms[b.j].z == 1) continue;
    if (!flexibleAmides && isAmideBond(mol, b)) continue;
    b.rotatable = true;
  }

  // Rigid groups: connected components once the rotatable bonds are cut.
  std::vector<int> group(n, -1);
  int ngroup = 0;
  for (int s = 0; s < n; s++) {
    if (group[s] >= 0 || mol.atoms[s].merged) continue;
    std::vector<int> stack(1, s);
    group[s] = ngroup;
    while (!stack.empty()) {
      int c = stack.back(); stack.pop_back();
      const std::vector<int>& nb = mol.neighbors(c);
      for (size_t k = 0; k < nb.size(); k++) {
        int j = nb[k];
        if (mol.atoms[j].merged || group[j] >= 0) continue;
        int bi = mol.findBond(c, j);
        if (bi >= 0 && mol.bonds[bi].rotatable) continue;
        group[j] = ngroup; stack.push_back(j);
      }
    }
    ngroup++;
  }

  nodes.resize(ngroup);
  for (int i = 0; i < n; i++)
    if (group[i] >= 0) nodes[group[i]].atoms.push_back(i);

  // Group adjacency through the rotatable bonds.
  std::vector< std::vector< std::pair<int,int> > > gadj(ngroup);
  for (size_t k = 0; k < mol.bonds.size(); k++) {
    if (!mol.bonds[k].rotatable) continue;
    int gi = group[mol.bonds[k].i], gj = group[mol.bonds[k].j];
    gadj[gi].push_back(std::make_pair(gj, (int)k));
    gadj[gj].push_back(std::make_pair(gi, (int)k));
  }

  // Root choice: the group that makes the tree shallowest, with the biggest
  // group winning a tie.  A deep tree costs Vina more per step.
  int best = 0, bestDepth = 1 << 30, bestSize = -1;
  for (int g = 0; g < ngroup; g++) {
    std::vector<int> depth(ngroup, -1);
    std::vector<int> q(1, g); depth[g] = 0;
    int maxd = 0;
    for (size_t h = 0; h < q.size(); h++) {
      int c = q[h];
      for (size_t k = 0; k < gadj[c].size(); k++) {
        int o = gadj[c][k].first;
        if (depth[o] >= 0) continue;
        depth[o] = depth[c] + 1; maxd = std::max(maxd, depth[o]);
        q.push_back(o);
      }
    }
    int size = (int)nodes[g].atoms.size();
    if (maxd < bestDepth || (maxd == bestDepth && size > bestSize)) {
      best = g; bestDepth = maxd; bestSize = size;
    }
  }
  p_root = best;

  std::vector<bool> done(ngroup, false);
  std::vector<int> order(1, best);
  done[best] = true;
  for (size_t h = 0; h < order.size(); h++) {
    int c = order[h];
    for (size_t k = 0; k < gadj[c].size(); k++) {
      int o = gadj[c][k].first;
      if (done[o]) continue;
      done[o] = true;
      const DockBond& b = mol.bonds[gadj[c][k].second];
      nodes[o].parentAtom = (group[b.i] == c) ? b.i : b.j;
      nodes[o].childAtom  = (group[b.i] == c) ? b.j : b.i;
      nodes[c].children.push_back(o);
      order.push_back(o);
    }
  }
}

// ---------------------------------------------------------------- writer ---

void DockPdbqtWriter::mergeNonpolarHydrogens(DockMol& mol)
{
  for (int i = 0; i < mol.numAtoms(); i++) {
    DockAtom& a = mol.atoms[i];
    if (a.z != 1 || a.adType == "HD") continue;
    a.merged = true;
    const std::vector<int>& nb = mol.neighbors(i);
    if (!nb.empty()) mol.atoms[nb[0]].charge += a.charge;
    a.charge = 0.0;
  }
}

namespace {

std::string atomLine(const DockMol& mol, int i, int serial)
{
  const DockAtom& a = mol.atoms[i];
  std::string name = a.name;
  if (name.empty()) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%s%d", DockMol::symbolFromElement(a.z).c_str(),
             serial);
    name = buf;
  }
  if (name.size() > 4) name = name.substr(0, 4);
  std::string res = a.resName.empty() ? std::string("UNL") : a.resName;
  if (res.size() > 3) res = res.substr(0, 3);
  char chain = a.chain.empty() ? 'A' : a.chain[0];
  char buf[128];
  snprintf(buf, sizeof(buf),
           "ATOM  %5d %-4s %-3s %c%4d    %8.3f%8.3f%8.3f%6.2f%6.2f    %6.3f %-2s",
           serial, name.c_str(), res.c_str(), chain, a.resSeq,
           a.x, a.y, a.zc, 1.00, 0.00, a.charge, a.adType.c_str());
  return std::string(buf);
}

void writeNode(const DockMol& mol, const DockTorsionTree& tree, int node,
               int& serial, std::vector<int>& serialOf, std::ostringstream& os,
               std::vector<int>* order)
{
  const DockTorsionNode& nd = tree.nodes[node];
  for (size_t k = 0; k < nd.atoms.size(); k++) {
    int i = nd.atoms[k];
    serialOf[i] = serial;
    if (order) order->push_back(i);
    os << atomLine(mol, i, serial) << "\n";
    serial++;
  }
  for (size_t k = 0; k < nd.children.size(); k++) {
    int ch = nd.children[k];
    const DockTorsionNode& cn = tree.nodes[ch];
    // The parent atom already has a serial; the child atom gets the next one,
    // which is why the child group is written immediately after the header.
    int pser = serialOf[cn.parentAtom];
    os << "BRANCH " << pser << " " << serial << "\n";
    // Make sure the bonded atom of the child group comes first.
    DockTorsionNode reordered = cn;
    for (size_t m = 0; m < reordered.atoms.size(); m++)
      if (reordered.atoms[m] == cn.childAtom) {
        std::swap(reordered.atoms[0], reordered.atoms[m]); break;
      }
    DockTorsionTree tmp = tree;
    tmp.nodes[ch] = reordered;
    int cser = serial;
    writeNode(mol, tmp, ch, serial, serialOf, os, order);
    os << "ENDBRANCH " << pser << " " << cser << "\n";
  }
}

}

std::string DockPdbqtWriter::ligand(DockMol& mol, const DockTorsionTree& tree,
                                    const std::string& name,
                                    std::vector<int>* order)
{
  if (order) order->clear();
  std::ostringstream os;
  os << "REMARK  Prepared by ECCE\n";
  os << "REMARK  Name = " << name << "\n";
  os << "REMARK  " << tree.numTorsions() << " active torsions\n";
  int serial = 1;
  std::vector<int> serialOf(mol.numAtoms(), 0);
  os << "ROOT\n";
  const DockTorsionNode& root = tree.nodes[tree.rootNode()];
  for (size_t k = 0; k < root.atoms.size(); k++) {
    serialOf[root.atoms[k]] = serial;
    if (order) order->push_back(root.atoms[k]);
    os << atomLine(mol, root.atoms[k], serial) << "\n";
    serial++;
  }
  os << "ENDROOT\n";
  for (size_t k = 0; k < root.children.size(); k++) {
    int ch = root.children[k];
    const DockTorsionNode& cn = tree.nodes[ch];
    int pser = serialOf[cn.parentAtom];
    os << "BRANCH " << pser << " " << serial << "\n";
    DockTorsionTree tmp = tree;
    for (size_t m = 0; m < tmp.nodes[ch].atoms.size(); m++)
      if (tmp.nodes[ch].atoms[m] == cn.childAtom) {
        std::swap(tmp.nodes[ch].atoms[0], tmp.nodes[ch].atoms[m]); break;
      }
    int cser = serial;
    writeNode(mol, tmp, ch, serial, serialOf, os, order);
    os << "ENDBRANCH " << pser << " " << cser << "\n";
  }
  os << "TORSDOF " << tree.numTorsions() << "\n";
  return os.str();
}

std::string DockPdbqtWriter::receptor(DockMol& mol)
{
  std::ostringstream os;
  os << "REMARK  Rigid receptor prepared by ECCE\n";
  int serial = 1;
  for (int i = 0; i < mol.numAtoms(); i++) {
    if (mol.atoms[i].merged) continue;
    os << atomLine(mol, i, serial) << "\n";
    serial++;
  }
  os << "TER\n";
  return os.str();
}
