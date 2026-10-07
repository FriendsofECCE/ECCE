/**
 * @file
 */

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <vector>

#include "dsm/GromacsStructure.H"

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

  double bx = 0, by = 0, bz = 0;
  if (sscanf(boxline.c_str(), "%lf %lf %lf", &bx, &by, &bz) == 3 &&
      bx > 0 && by > 0 && bz > 0) {
    snprintf(buf, sizeof(buf),
             "CRYST1%9.3f%9.3f%9.3f  90.00  90.00  90.00 P 1           1",
             bx * 10.0, by * 10.0, bz * 10.0);
    pdb = string(buf) + "\n";
  }
  pdb += out.str() + "END\n";
  return n;
}
