#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "tdat/CifIO.H"
#include "tdat/Fragment.H"
#include "tdat/LatticeDef.H"
#include "tdat/SymmetryOps.H"
#include "tdat/TAtm.H"
#include "tdat/TPerTab.H"
#include "tdat/PointGroup.H"
#include "util/Transformations.H"

using namespace std;

namespace {

string s_error;
vector<string> s_warnings;

struct Tok { string text; bool quoted; };

string lower(string s)
{
  for (size_t i = 0; i < s.size(); i++) s[i] = tolower((unsigned char)s[i]);
  return s;
}

// CIF tokens: bare words, 'quoted' and "quoted" (the closing quote is
// followed by white space), ;text fields; # starts a comment.
vector<Tok> tokenize(istream& in)
{
  vector<Tok> toks;
  string line;
  while (getline(in, line)) {
    if (!line.empty() && line[line.size()-1] == '\r') line.erase(line.size()-1);
    if (!line.empty() && line[0] == ';') {
      string text = line.substr(1);
      while (getline(in, line)) {
        if (!line.empty() && line[line.size()-1] == '\r')
          line.erase(line.size()-1);
        if (!line.empty() && line[0] == ';') break;
        text += "\n" + line;
      }
      toks.push_back({text, true});
      continue;
    }
    size_t i = 0, n = line.size();
    while (i < n) {
      if (isspace((unsigned char)line[i])) { i++; continue; }
      if (line[i] == '#') break;
      if (line[i] == '\'' || line[i] == '"') {
        char q = line[i];
        size_t j = i + 1;
        while (j < n && !(line[j] == q &&
               (j + 1 == n || isspace((unsigned char)line[j+1])))) j++;
        toks.push_back({line.substr(i + 1, j - i - 1), true});
        i = j + 1;
      } else {
        size_t j = i;
        while (j < n && !isspace((unsigned char)line[j])) j++;
        toks.push_back({line.substr(i, j - i), false});
        i = j;
      }
    }
  }
  return toks;
}

struct Loop { vector<string> tags; vector<string> cells; };

struct Block {
  string name;
  map<string, string> items;   // lower-case tag -> value
  vector<Loop> loops;
};

bool isTag(const Tok& t) { return !t.quoted && !t.text.empty() && t.text[0] == '_'; }
bool isKeyword(const Tok& t, const char *kw)
{
  return !t.quoted && lower(t.text).compare(0, strlen(kw), kw) == 0;
}

// Only the first data block is parsed; the count of the rest is returned.
bool parseFirstBlock(const vector<Tok>& toks, Block& blk, int& skipped)
{
  size_t i = 0, n = toks.size();
  skipped = 0;
  while (i < n && !isKeyword(toks[i], "data_")) i++;
  if (i == n) { s_error = "No data_ block found: this is not a CIF file."; return false; }
  blk.name = toks[i].text.substr(5);
  i++;
  bool inFirst = true;
  while (i < n) {
    const Tok& t = toks[i];
    if (isKeyword(t, "data_")) {
      if (inFirst) { inFirst = false; }
      skipped++;
      i++;
      continue;
    }
    if (!inFirst) { i++; continue; }
    if (!t.quoted && lower(t.text) == "loop_") {
      Loop lp;
      i++;
      while (i < n && isTag(toks[i])) lp.tags.push_back(lower(toks[i++].text));
      while (i < n && !isTag(toks[i]) && !isKeyword(toks[i], "data_") &&
             !(!toks[i].quoted && lower(toks[i].text) == "loop_"))
        lp.cells.push_back(toks[i++].text);
      if (lp.tags.empty() || lp.cells.size() % lp.tags.size() != 0) {
        s_error = "Malformed loop_ in the CIF file (the number of values is not a "
                  "multiple of the number of columns).";
        return false;
      }
      blk.loops.push_back(lp);
    } else if (isTag(t)) {
      if (i + 1 >= n || isTag(toks[i+1]) ) {
        s_error = "Malformed CIF: no value for " + t.text + ".";
        return false;
      }
      blk.items[lower(t.text)] = toks[i+1].text;
      i += 2;
    } else {
      s_error = "Malformed CIF: unexpected text '" + t.text + "'.";
      return false;
    }
  }
  return true;
}

// 5.4307(2) -> 5.4307; "?" and "." are missing.
bool number(string s, double& v)
{
  size_t p = s.find('(');
  if (p != string::npos) s.erase(p);
  if (s.empty() || s == "?" || s == ".") return false;
  char *end = 0;
  v = strtod(s.c_str(), &end);
  return end && *end == '\0';
}

struct SymOp { double r[3][3]; double t[3]; };

// "-x+1/2, y+1/2, -z", "x-y, x, z+1/3", "2/3+x"
bool parseSymOp(const string& text, SymOp& op)
{
  for (int a = 0; a < 3; a++) {
    op.t[a] = 0;
    for (int b = 0; b < 3; b++) op.r[a][b] = 0;
  }
  string s;
  for (size_t i = 0; i < text.size(); i++)
    if (!isspace((unsigned char)text[i])) s += tolower((unsigned char)text[i]);
  size_t pos = 0;
  for (int row = 0; row < 3; row++) {
    size_t end = s.find(',', pos);
    string comp = s.substr(pos, end == string::npos ? string::npos : end - pos);
    if ((row < 2 && end == string::npos) || comp.empty()) return false;
    pos = end + 1;
    size_t i = 0;
    while (i < comp.size()) {
      double sign = 1;
      if (comp[i] == '+') i++;
      else if (comp[i] == '-') { sign = -1; i++; }
      if (i >= comp.size()) return false;
      if (comp[i] == 'x' || comp[i] == 'y' || comp[i] == 'z') {
        op.r[row][comp[i] - 'x'] += sign;
        i++;
      } else if (isdigit((unsigned char)comp[i]) || comp[i] == '.') {
        size_t j = i;
        while (j < comp.size() && (isdigit((unsigned char)comp[j]) || comp[j] == '.')) j++;
        double v = atof(comp.substr(i, j - i).c_str());
        if (j < comp.size() && comp[j] == '/') {
          size_t k = j + 1;
          while (k < comp.size() && isdigit((unsigned char)comp[k])) k++;
          double d = atof(comp.substr(j + 1, k - j - 1).c_str());
          if (d == 0) return false;
          v /= d;
          j = k;
        }
        if (j < comp.size() && (comp[j] == 'x' || comp[j] == 'y' || comp[j] == 'z')) {
          op.r[row][comp[j] - 'x'] += sign * v;  // "2x"
          j++;
        } else {
          op.t[row] += sign * v;
        }
        i = j;
      } else {
        return false;
      }
    }
  }
  return true;
}

const Loop *findLoop(const Block& b, const string& tag)
{
  for (size_t i = 0; i < b.loops.size(); i++)
    if (find(b.loops[i].tags.begin(), b.loops[i].tags.end(), tag) != b.loops[i].tags.end())
      return &b.loops[i];
  return 0;
}

int col(const Loop& l, const string& tag)
{
  for (size_t i = 0; i < l.tags.size(); i++) if (l.tags[i] == tag) return (int)i;
  return -1;
}

// "Si4+", "O2-", "Co1", "OW" -> a symbol the periodic table knows, or "".
string element(const string& s)
{
  TPerTab pt;
  size_t i = 0;
  while (i < s.size() && !isalpha((unsigned char)s[i])) i++;
  if (i >= s.size()) return "";
  string one(1, toupper((unsigned char)s[i]));
  if (i + 1 < s.size() && isalpha((unsigned char)s[i+1])) {
    string two = one + string(1, tolower((unsigned char)s[i+1]));
    if (pt.atomicNumber(two) > 0) return two;
  }
  return pt.atomicNumber(one) > 0 ? one : "";
}

struct Site { string label, sym; double f[3]; double occ; };

string wrapName(const string& list, size_t max)
{
  return list.size() > max ? list.substr(0, max) + " ..." : list;
}

}  // namespace

const string& CifIO::lastError() { return s_error; }

vector<string> CifIO::takeWarnings()
{
  vector<string> w;
  w.swap(s_warnings);
  return w;
}

string CifIO::tableName(const string& hm)
{
  vector<string> w;
  istringstream is(hm);
  string t, out;
  while (is >> t) w.push_back(t);
  for (size_t i = 0; i < w.size(); i++) {
    t = w[i];
    // 21 31 32 41 42 43 61..65: screw axes, spelled N_k in the tables.
    if (t[0] == ':') break;   // ":H" / ":R" cell-choice suffix
    {
      string res;
      for (size_t j = 0; j < t.size(); j++) {
        res += t[j];
        if (isdigit((unsigned char)t[j]) && j + 1 < t.size() &&
            isdigit((unsigned char)t[j+1]) && t[j+1] < t[j] && t[j] >= '2' &&
            (j == 0 || !isdigit((unsigned char)t[j-1]))) {
          res += '_';
        }
      }
      t = res;
    }
    out += t;
  }
  return out;
}

bool CifIO::read(Fragment& frag, istream& in, bool genBonds)
{
  s_error.clear();
  s_warnings.clear();
  Block blk;
  int skipped = 0;
  if (!parseFirstBlock(tokenize(in), blk, skipped)) return false;
  if (skipped > 0)
    s_warnings.push_back("The CIF file has " + to_string(skipped + 1) +
        " data blocks; only the first ('" + blk.name + "') was read.");

  // The cell.
  double cell[6];
  static const char *ctags[6] = {"_cell_length_a", "_cell_length_b",
      "_cell_length_c", "_cell_angle_alpha", "_cell_angle_beta",
      "_cell_angle_gamma"};
  for (int i = 0; i < 6; i++) {
    map<string, string>::const_iterator it = blk.items.find(ctags[i]);
    if (it == blk.items.end() || !number(it->second, cell[i])) {
      s_error = string("The CIF file has no usable ") + ctags[i] +
                ", so there is no unit cell to build.";
      return false;
    }
  }
  const double rad = M_PI / 180.0;
  const double ca = cos(cell[3]*rad), cb = cos(cell[4]*rad);
  const double cg = cos(cell[5]*rad), sg = sin(cell[5]*rad);
  const double cy = (ca - cb*cg) / (sg == 0 ? 1e-12 : sg);
  const double cz2 = 1.0 - cb*cb - cy*cy;
  if (cell[0] <= 0 || cell[1] <= 0 || cell[2] <= 0 || sg <= 1e-6 || cz2 <= 0) {
    s_error = "The cell lengths and angles in the CIF file do not describe a cell.";
    return false;
  }
  const MPoint va(cell[0], 0.0, 0.0), vb(cell[1]*cg, cell[1]*sg, 0.0),
               vc(cell[2]*cb, cell[2]*cy, cell[2]*sqrt(cz2));

  // The asymmetric unit.
  const Loop *al = findLoop(blk, "_atom_site_fract_x");
  if (!al) {
    s_error = "The CIF file has no _atom_site_fract_x/y/z columns (an atom_site "
              "loop with fractional coordinates).";
    return false;
  }
  int cl = col(*al, "_atom_site_label"), ct = col(*al, "_atom_site_type_symbol");
  int cx = col(*al, "_atom_site_fract_x"), cyy = col(*al, "_atom_site_fract_y");
  int cz = col(*al, "_atom_site_fract_z"), co = col(*al, "_atom_site_occupancy");
  if (cyy < 0 || cz < 0 || (cl < 0 && ct < 0)) {
    s_error = "The CIF atom_site loop lacks a label or one of fract_x/y/z.";
    return false;
  }
  vector<Site> sites;
  size_t ncol = al->tags.size();
  for (size_t r = 0; r < al->cells.size() / ncol; r++) {
    const string *row = &al->cells[r * ncol];
    Site s;
    s.label = cl >= 0 ? row[cl] : row[ct];
    s.sym = ct >= 0 ? element(row[ct]) : "";
    if (s.sym.empty()) s.sym = element(s.label);
    if (s.sym.empty()) {
      s_error = "Cannot tell the element of atom '" + s.label + "' in the CIF file.";
      return false;
    }
    if (!number(row[cx], s.f[0]) || !number(row[cyy], s.f[1]) ||
        !number(row[cz], s.f[2])) {
      s_error = "Atom '" + s.label + "' has no fractional coordinates in the CIF file.";
      return false;
    }
    s.occ = 1.0;
    if (co >= 0) number(row[co], s.occ);
    sites.push_back(s);
  }
  if (sites.empty()) { s_error = "The CIF atom_site loop has no atoms."; return false; }

  // Partial occupancy: of the entries on one position keep the largest.
  vector<bool> keep(sites.size(), true);
  string partial;
  for (size_t i = 0; i < sites.size(); i++) {
    if (!keep[i]) continue;
    size_t best = i;
    bool shared = false;
    for (size_t j = i + 1; j < sites.size(); j++) {
      double d = 0;
      for (int k = 0; k < 3; k++) d += fabs(sites[i].f[k] - sites[j].f[k]);
      if (d < 1.0e-3) {
        shared = true;
        if (sites[j].occ > sites[best].occ) { keep[best] = false; best = j; }
        else keep[j] = false;
      }
    }
    if (shared || sites[i].occ < 0.999) {
      if (!partial.empty()) partial += ", ";
      char b[64];
      snprintf(b, sizeof b, "%s (%.2f)", sites[best].label.c_str(), sites[best].occ);
      partial += b;
    }
  }
  if (!partial.empty())
    s_warnings.push_back("Partially occupied sites: kept the majority species "
        "at full occupancy on " + wrapName(partial, 200) + ".");

  // Symmetry operations: as listed, else from the space-group name.
  vector<SymOp> ops;
  const char *optags[2] = {"_space_group_symop_operation_xyz",
                           "_symmetry_equiv_pos_as_xyz"};
  for (int t = 0; t < 2 && ops.empty(); t++) {
    const Loop *sl = findLoop(blk, optags[t]);
    if (!sl) {
      map<string, string>::const_iterator it = blk.items.find(optags[t]);
      SymOp op;
      if (it != blk.items.end() && parseSymOp(it->second, op)) ops.push_back(op);
      continue;
    }
    int c = col(*sl, optags[t]);
    size_t nc = sl->tags.size();
    for (size_t r = 0; r < sl->cells.size() / nc; r++) {
      SymOp op;
      if (!parseSymOp(sl->cells[r * nc + c], op)) {
        s_error = "Cannot read the symmetry operation '" + sl->cells[r * nc + c] +
                  "' in the CIF file.";
        return false;
      }
      ops.push_back(op);
    }
  }
  string sgName;
  if (ops.empty()) {
    const char *ntags[2] = {"_space_group_name_h-m_alt", "_symmetry_space_group_name_h-m"};
    for (int t = 0; t < 2 && sgName.empty(); t++) {
      map<string, string>::const_iterator it = blk.items.find(ntags[t]);
      if (it != blk.items.end()) sgName = tableName(it->second);
    }
    if (sgName.empty()) {
      s_error = "The CIF file lists no symmetry operations and no space-group name.";
      return false;
    }
    if (sgName == "P1") {
      SymOp id;
      for (int a = 0; a < 3; a++) { id.t[a] = 0; for (int b = 0; b < 3; b++) id.r[a][b] = a == b; }
      ops.push_back(id);
      sgName.clear();
    } else if (!PointGroup::isSpaceGroup(sgName)) {
      s_error = "The space group '" + sgName + "' of the CIF file is not in the "
                "space-group tables, and the file lists no symmetry operations.";
      return false;
    }
  }

  frag.clear();
  LatticeDef *lat = new LatticeDef(1., 1., 1., 90., 90., 90.);
  lat->assign(va, vb, vc);
  frag.setLattice(lat);

  // Cartesian from fractional with the cell vectors as rows.
  struct Cart {
    static void xyz(const double *f, MPoint a, MPoint b,
                    MPoint c, double *x) {
      x[0] = f[0]*a.x() + f[1]*b.x() + f[2]*c.x();
      x[1] = f[0]*a.y() + f[1]*b.y() + f[2]*c.y();
      x[2] = f[0]*a.z() + f[1]*b.z() + f[2]*c.z();
    }
  };

  if (!sgName.empty()) {
    // The tables work on the asymmetric unit in the fragment.
    for (size_t i = 0; i < sites.size(); i++) {
      if (!keep[i]) continue;
      double x[3];
      Cart::xyz(sites[i].f, va, vb, vc, x);
      frag.addAtom(new TAtm(sites[i].sym.c_str(), x), 0);
    }
    try {
      frag.pointGroup(sgName);
      SymmetryOps::generateLatticeFragment(frag, 0.1);
    } catch (...) {
      s_error = "Could not generate the unit cell from space group " + sgName +
                " (the symmetry helper failed).";
      return false;
    }
    frag.pointGroup("C1");
  } else {
    vector<Site> out;
    for (size_t i = 0; i < sites.size(); i++) {
      if (!keep[i]) continue;
      for (size_t o = 0; o < ops.size(); o++) {
        Site g = sites[i];
        for (int a = 0; a < 3; a++) {
          double v = ops[o].t[a];
          for (int b = 0; b < 3; b++) v += ops[o].r[a][b] * sites[i].f[b];
          v -= floor(v);
          if (v > 1.0 - 1.0e-8) v = 0.0;
          g.f[a] = v;
        }
        bool dup = false;
        for (size_t k = 0; k < out.size() && !dup; k++) {
          if (out[k].sym != g.sym) continue;
          double d[3];
          for (int a = 0; a < 3; a++) {
            d[a] = g.f[a] - out[k].f[a];
            d[a] -= floor(d[a] + 0.5);
          }
          double x[3];
          Cart::xyz(d, va, vb, vc, x);
          dup = sqrt(x[0]*x[0] + x[1]*x[1] + x[2]*x[2]) < 0.1;
        }
        if (!dup) out.push_back(g);
      }
    }
    for (size_t i = 0; i < out.size(); i++) {
      double x[3];
      Cart::xyz(out[i].f, va, vb, vc, x);
      TAtm *atom = new TAtm(out[i].sym.c_str(), x);
      atom->atomName(out[i].label.c_str());
      frag.addAtom(atom, 0);
    }
    if (genBonds) {
      frag.addCovalentBonds(true);
      frag.setEnvIndices();
    }
  }

  bool hasH = false, hasON = false;
  for (int i = 0; i < frag.numAtoms(); i++) {
    string e = frag.atomRef(i)->atomicSymbol();
    if (e == "H" || e == "D") hasH = true;
    if (e == "O" || e == "N") hasON = true;
  }
  if (!hasH && hasON)
    s_warnings.push_back("The structure has no hydrogen atoms although it contains "
        "O or N; hydrogens are often missing from mineral CIF files.");
  return frag.numAtoms() > 0;
}

bool CifIO::write(Fragment& frag, ostream& out)
{
  s_error.clear();
  LatticeDef *lat = frag.getLattice();
  if (!lat) {
    s_error = "The structure has no unit cell; a CIF file needs one. "
              "Define a cell in the Periodic Builder first.";
    return false;
  }
  double a, b, c, al, be, ga;
  lat->getLattice(a, b, c, al, be, ga);
  vector<MPoint> *v = lat->toVectors();
  MPoint A = (*v)[0], B = (*v)[1], C = (*v)[2];
  delete v;
  MPoint o = lat->getLatticeCorner();
  double vol = A.dotProduct(B.crossProduct1(C));
  MPoint ra = B.crossProduct1(C), rb = C.crossProduct1(A), rc = A.crossProduct1(B);

  string name = frag.name();
  for (size_t i = 0; i < name.size(); i++)
    if (isspace((unsigned char)name[i])) name[i] = '_';
  if (name.empty()) name = "ecce";
  char buf[200];
  out << "data_" << name << "\n";
  out << "_audit_creation_method 'ECCE Builder'\n";
  snprintf(buf, sizeof buf, "_cell_length_a %.6f\n_cell_length_b %.6f\n"
           "_cell_length_c %.6f\n_cell_angle_alpha %.6f\n_cell_angle_beta %.6f\n"
           "_cell_angle_gamma %.6f\n", a, b, c, al, be, ga);
  out << buf;
  out << "_symmetry_space_group_name_H-M 'P 1'\n_symmetry_Int_Tables_number 1\n"
         "loop_\n_symmetry_equiv_pos_as_xyz\n'x, y, z'\n";
  out << "loop_\n_atom_site_label\n_atom_site_type_symbol\n_atom_site_fract_x\n"
         "_atom_site_fract_y\n_atom_site_fract_z\n_atom_site_occupancy\n";
  map<string, int> count;
  for (int i = 0; i < frag.numAtoms(); i++) {
    TAtm *at = frag.atomRef(i);
    string sym = at->atomicSymbol();
    if (sym == "Nub" || sym == "X") continue;
    const double *x = at->coordinates();
    MPoint d(x[0] - o.x(), x[1] - o.y(), x[2] - o.z());
    double f[3] = {d.dotProduct(ra) / vol, d.dotProduct(rb) / vol,
                   d.dotProduct(rc) / vol};
    snprintf(buf, sizeof buf, "%s%d %s %.8f %.8f %.8f 1.0\n", sym.c_str(),
             ++count[sym], sym.c_str(), f[0], f[1], f[2]);
    out << buf;
  }
  return out.good();
}
