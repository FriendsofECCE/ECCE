/**
 * @file
 */

#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <array>
#include <vector>

#include "dsm/GromacsStructure.H"
#include "tdat/CompactCell.H"

using std::string;
using std::vector;

static string trim(const string& s)
{
  size_t a = s.find_first_not_of(" \t\r\n");
  if (a == string::npos) return "";
  size_t b = s.find_last_not_of(" \t\r\n");
  return s.substr(a, b - a + 1);
}

static string upper(string s)
{
  for (size_t i = 0; i < s.size(); i++) s[i] = toupper((unsigned char)s[i]);
  return s;
}

// The element of a GROMACS atom.  An ion is a residue named for the ion
// whose one atom has the same name (NA, CL, MG ...); everything else is
// taken from the first letter of the atom name, which is what the force
// fields' naming amounts to for H, C, N, O, S and P.
static string elementOf(const string& atomName, const string& resName)
{
  string a = upper(atomName);
  string r = upper(resName);
  static const char* ions[] = { "NA", "CL", "MG", "ZN", "CA", "FE", "CU",
                                "MN", "LI", "BR", "RB", "CS", "CD", "NI",
                                "CO", 0 };
  for (int i = 0; ions[i]; i++) {
    if (a == ions[i] && (r == ions[i] || r == string(ions[i]) + "+" ||
                         r == string(ions[i]) + "-")) {
      string e = ions[i];
      return string(1, e[0]) + string(1, (char)tolower(e[1]));
    }
  }
  if (a == "K" || a == "K+") return "K";
  size_t i = 0;
  while (i < a.size() && !isalpha((unsigned char)a[i])) i++;
  if (i == a.size()) return "X";
  return string(1, a[i]);
}

int GromacsStructure::atomCount(const string& gro)
{
  std::istringstream in(gro);
  string line;
  if (!std::getline(in, line)) return -1;
  if (!std::getline(in, line)) return -1;
  string t = trim(line);
  if (t.empty() || t.find_first_not_of("0123456789") != string::npos) return -1;
  return atoi(t.c_str());
}

int GromacsStructure::groToPdb(const string& gro, string& pdb, string& err)
{
  pdb = "";
  err = "";
  std::istringstream in(gro);
  string title, line;
  if (!std::getline(in, title) || !std::getline(in, line)) {
    err = "The file is too short to be a GROMACS structure (.gro) file.";
    return -1;
  }
  int n = atomCount(gro);
  if (n < 1) {
    err = "The second line of a .gro file is the number of atoms; this "
          "file has \"" + trim(line) + "\".";
    return -1;
  }

  vector<string> lines;
  for (int i = 0; i < n; i++) {
    if (!std::getline(in, line)) {
      char buf[160];
      snprintf(buf, sizeof(buf), "The file says it has %d atoms but ends "
               "after %d.", n, i);
      err = buf;
      return -1;
    }
    lines.push_back(line);
  }
  string boxline;
  std::getline(in, boxline);

  // The coordinate columns are as wide as the distance between the first
  // two decimal points; 8 for the usual three decimals.
  const string& first = lines[0];
  size_t d1 = first.find('.', 20);
  size_t d2 = (d1 == string::npos) ? string::npos : first.find('.', d1 + 1);
  if (d1 == string::npos || d2 == string::npos || d2 <= d1) {
    err = "Cannot read the coordinates on atom line 3 of the .gro file.";
    return -1;
  }
  size_t w = d2 - d1;

  std::ostringstream out;
  char buf[200];
  for (int i = 0; i < n; i++) {
    const string& l = lines[i];
    if (l.size() < 20 + 3 * w) {
      snprintf(buf, sizeof(buf), "Atom line %d of the .gro file is too "
               "short to hold coordinates.", i + 3);
      err = buf;
      return -1;
    }
    string resNum  = trim(l.substr(0, 5));
    string resName = trim(l.substr(5, 5));
    string atName  = trim(l.substr(10, 5));
    double x = atof(l.substr(20, w).c_str()) * 10.0;
    double y = atof(l.substr(20 + w, w).c_str()) * 10.0;
    double z = atof(l.substr(20 + 2 * w, w).c_str()) * 10.0;
    string el = elementOf(atName, resName);

    // ECCE's PDB reader takes the element from the first characters of the
    // name field: a leading blank means a one letter element, none means
    // two.  Four characters are all the field has.
    string nameField;
    if (el.size() == 2) {
      nameField = atName.substr(0, 4);
      nameField.resize(4, ' ');
    } else {
      nameField = " " + atName.substr(0, 3);
      nameField.resize(4, ' ');
    }
    snprintf(buf, sizeof(buf),
             "ATOM  %5d %4s %-4.4s%c%4d    %8.3f%8.3f%8.3f  1.00  0.00"
             "          %2s",
             (i + 1) % 100000, nameField.c_str(), resName.c_str(), ' ',
             atoi(resNum.c_str()) % 10000, x, y, z, el.c_str());
    out << buf << "\n";
  }

  // Box: v1x v2y v3z [v1y v1z v2x v2z v3x v3y], nm; the 9-value form is
  // triclinic and needs the angles from the vectors.
  double b[9] = {0,0,0,0,0,0,0,0,0};
  int nb = sscanf(boxline.c_str(), "%lf %lf %lf %lf %lf %lf %lf %lf %lf",
                  &b[0],&b[1],&b[2],&b[3],&b[4],&b[5],&b[6],&b[7],&b[8]);
  if ((nb == 3 || nb == 9) && b[0] > 0 && b[1] > 0 && b[2] > 0) {
    double v1[3] = {b[0], b[3], b[4]};
    double v2[3] = {b[5], b[1], b[6]};
    double v3[3] = {b[7], b[8], b[2]};
    double la = sqrt(v1[0]*v1[0] + v1[1]*v1[1] + v1[2]*v1[2]);
    double lb = sqrt(v2[0]*v2[0] + v2[1]*v2[1] + v2[2]*v2[2]);
    double lc = sqrt(v3[0]*v3[0] + v3[1]*v3[1] + v3[2]*v3[2]);
    double ca = (v2[0]*v3[0] + v2[1]*v3[1] + v2[2]*v3[2]) / (lb * lc);
    double cb = (v1[0]*v3[0] + v1[1]*v3[1] + v1[2]*v3[2]) / (la * lc);
    double cg = (v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2]) / (la * lb);
    const double deg = 180.0 / 3.14159265358979323846;
    snprintf(buf, sizeof(buf),
             "CRYST1%9.3f%9.3f%9.3f%7.2f%7.2f%7.2f P 1           1",
             la * 10.0, lb * 10.0, lc * 10.0,
             acos(ca) * deg, acos(cb) * deg, acos(cg) * deg);
    pdb = string(buf) + "\n";
  }
  pdb += out.str() + "END\n";
  return n;
}

// Columns 6-10 of each atom line, as grompp reads them.
vector<string> GromacsStructure::residueNames(const string& gro)
{
  vector<string> names;
  std::istringstream in(gro);
  string line;
  std::getline(in, line);
  if (!std::getline(in, line)) return names;
  int n = atoi(line.c_str());
  for (int i = 0; i < n && std::getline(in, line); i++) {
    if (line.size() < 10) continue;
    string r = trim(line.substr(5, 5));
    bool seen = false;
    for (size_t j = 0; j < names.size(); j++) if (names[j] == r) seen = true;
    if (!seen) names.push_back(r);
  }
  return names;
}

vector<string> GromacsStructure::moleculeNames(const string& top)
{
  vector<string> names;
  std::istringstream in(top);
  string line;
  bool inMolecules = false;
  while (std::getline(in, line)) {
    size_t semi = line.find(';');
    if (semi != string::npos) line = line.substr(0, semi);
    line = trim(line);
    if (line.empty() || line[0] == '#') continue;
    if (line[0] == '[') {
      string sect = trim(line.substr(1, line.find(']') - 1));
      inMolecules = (sect == "molecules");
      continue;
    }
    if (!inMolecules) continue;
    std::istringstream f(line);
    string n;
    f >> n;
    bool seen = false;
    for (size_t j = 0; j < names.size(); j++) if (names[j] == n) seen = true;
    if (!seen) names.push_back(n);
  }
  return names;
}

namespace {

bool isHydrogen(const string& atomName)
{
  for (size_t i = 0; i < atomName.size(); i++)
    if (isalpha((unsigned char)atomName[i]))
      return toupper((unsigned char)atomName[i]) == 'H';
  return false;
}

}  // namespace

string GromacsStructure::wrapCompact(const string& gro)
{
  std::istringstream in(gro);
  string title, countLine;
  if (!std::getline(in, title) || !std::getline(in, countLine)) return gro;
  int n = atoi(countLine.c_str());
  if (n <= 0) return gro;
  vector<string> lines(n);
  for (int i = 0; i < n; i++)
    if (!std::getline(in, lines[i])) return gro;
  string boxline;
  if (!std::getline(in, boxline)) return gro;

  double b[9] = {0,0,0,0,0,0,0,0,0};
  int nb = sscanf(boxline.c_str(), "%lf %lf %lf %lf %lf %lf %lf %lf %lf",
                  &b[0],&b[1],&b[2],&b[3],&b[4],&b[5],&b[6],&b[7],&b[8]);
  if (nb != 9) return gro;
  // GROMACS keeps the box lower triangular: a along x, b in the xy plane.
  if (b[3] != 0 || b[4] != 0 || b[6] != 0) return gro;
  if (b[5] == 0 && b[7] == 0 && b[8] == 0) return gro;
  if (b[0] <= 0 || b[1] <= 0 || b[2] <= 0) return gro;

  const string& first = lines[0];
  size_t d1 = first.find('.', 20);
  size_t d2 = (d1 == string::npos) ? string::npos : first.find('.', d1 + 1);
  if (d1 == string::npos || d2 == string::npos || d2 <= d1) return gro;
  const size_t w = d2 - d1;
  char decfmt[24];
  snprintf(decfmt, sizeof decfmt, "%%%d.%df", (int)w, (int)(w - 5 > 0 ? w - 5 : 3));

  // A vector of C arrays is ill-formed; Xcode 26 libc++ rejects it.
  vector<std::array<double, 3> > x(n);
  vector<string> res(n), name(n);
  for (int i = 0; i < n; i++) {
    const string& l = lines[i];
    if (l.size() < 20 + 3 * w) return gro;
    for (int k = 0; k < 3; k++) x[i][k] = atof(l.substr(20 + k * w, w).c_str());
    res[i] = l.substr(0, 10);
    name[i] = l.substr(10, 5);
  }

  vector<int> resId(n);
  {
    int r = 0;
    for (int i = 0; i < n; i++) {
      if (i > 0 && res[i] != res[i - 1]) r++;
      resId[i] = r;
    }
  }
  vector<char> heavy(n);
  for (int i = 0; i < n; i++) heavy[i] = !isHydrogen(name[i]);
  const double cellv[3][3] = { { b[0], 0, 0 }, { b[5], b[1], 0 }, { b[7], b[8], b[2] } };
  if (!CompactCell::wrap(x, resId, heavy, cellv, 0.175)) return gro;

  std::ostringstream out;
  out << title << "\n" << countLine << "\n";
  char buf[64];
  for (int i = 0; i < n; i++) {
    string l = lines[i];
    string xyz;
    for (int k = 0; k < 3; k++) {
      snprintf(buf, sizeof buf, decfmt, x[i][k]);
      xyz += buf;
    }
    l.replace(20, 3 * w, xyz);
    out << l << "\n";
  }
  out << boxline << "\n";
  return out.str();
}
