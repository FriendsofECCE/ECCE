#include <cctype>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

#include "tdat/MolIO.H"
#include "tdat/Fragment.H"
#include "tdat/TPerTab.H"

using namespace std;

namespace {

string s_error;

string trim(const string& s)
{
  size_t b = s.find_first_not_of(" \t\r\n");
  if (b == string::npos) return "";
  size_t e = s.find_last_not_of(" \t\r\n");
  return s.substr(b, e - b + 1);
}

// Fixed-column field, tolerating a line that stops early.
string col(const string& line, size_t start, size_t len)
{
  if (line.size() <= start) return "";
  return trim(line.substr(start, len));
}

// "FE", "fe", "Fe" all mean iron; "" when it is not an element.  The
// deuterium and tritium spellings MDL files use are hydrogen.
string element(const string& raw)
{
  string s = trim(raw);
  if (s.empty()) return "";
  if (s == "D" || s == "T" || s == "d" || s == "t") return "H";
  s[0] = toupper((unsigned char)s[0]);
  for (size_t i = 1; i < s.size(); i++) s[i] = tolower((unsigned char)s[i]);
  TPerTab tpt;
  return tpt.isValid(s.c_str()) ? s : "";
}

// An atom name such as "C12", "CA" or "Cl3": the leading letters, then the
// longest leading element among them.
string elementFromName(const string& name)
{
  string letters;
  for (size_t i = 0; i < name.size(); i++) {
    if (isalpha((unsigned char)name[i])) letters += name[i];
    else break;
  }
  if (letters.size() > 2) letters = letters.substr(0, 2);
  while (!letters.empty()) {
    string e = element(letters);
    if (!e.empty()) return e;
    letters.erase(letters.size() - 1);
  }
  return "";
}

// The file's own bonds are in, so only fill in the rest when it had none.
void finish(Fragment& frag, bool genBonds, bool hadBonds)
{
  if (genBonds && !hadBonds) frag.addCovalentBonds(true);
  if (genBonds) frag.setEnvIndices();
}

}  // namespace


const string& MolIO::lastError()
{
  return s_error;
}


bool MolIO::readSdf(Fragment& frag, istream& in, bool genBonds)
{
  s_error = "";
  string title, prog, comment, counts;
  if (!getline(in, title) || !getline(in, prog) || !getline(in, comment) ||
      !getline(in, counts)) {
    s_error = "not an MDL mol/sdf file: it ends before the counts line";
    return false;
  }

  //  "  6  6  0  0  0  0  0  0  0  0999 V2000": the version is the last
  //  field and decides how the rest of the file is laid out.
  if (counts.find("V3000") != string::npos) {
    s_error = "this is an MDL V3000 file; only V2000 mol/sdf can be read";
    return false;
  }
  if (counts.size() < 6) {
    s_error = "short counts line in the mol/sdf file";
    return false;
  }
  int na = atoi(counts.substr(0, 3).c_str());
  int nb = atoi(counts.substr(3, 3).c_str());
  if (na <= 0) {
    s_error = "the mol/sdf file holds no atoms";
    return false;
  }

  frag.name(trim(title));

  double coords[3];
  for (int i = 0; i < na; i++) {
    string line;
    if (!getline(in, line)) {
      s_error = "the mol/sdf atom block stops after " + to_string(i)
              + " of " + to_string(na) + " atoms";
      return false;
    }
    coords[0] = atof(col(line, 0, 10).c_str());
    coords[1] = atof(col(line, 10, 10).c_str());
    coords[2] = atof(col(line, 20, 10).c_str());
    string sym = element(col(line, 31, 3));
    if (sym.empty()) {
      s_error = "unknown element '" + col(line, 31, 3) + "' on atom "
              + to_string(i + 1) + " of the mol/sdf file";
      return false;
    }
    frag.addAtom(sym, coords);
  }

  bool hadBonds = false;
  for (int i = 0; i < nb; i++) {
    string line;
    if (!getline(in, line)) {
      s_error = "the mol/sdf bond block stops after " + to_string(i)
              + " of " + to_string(nb) + " bonds";
      return false;
    }
    int a1 = atoi(col(line, 0, 3).c_str()) - 1;
    int a2 = atoi(col(line, 3, 3).c_str()) - 1;
    int type = atoi(col(line, 6, 3).c_str());
    if (a1 < 0 || a2 < 0 || a1 >= na || a2 >= na || a1 == a2) continue;
    //  4 is aromatic, and 5-8 are the query types a database file can
    //  carry; all of them draw as the tree's aromatic 1.5.
    double order = (type >= 1 && type <= 3) ? (double)type : 1.5;
    frag.addBond(a1, a2, order);
    hadBonds = true;
  }

  finish(frag, genBonds, hadBonds);
  return true;
}


bool MolIO::readMol2(Fragment& frag, istream& in, bool genBonds)
{
  s_error = "";
  string line;
  string section;
  int na = 0, nb = 0, molLine = 0, nAtomsRead = 0;
  bool hadBonds = false;
  vector<int> idToIndex;   // mol2 atom ids need not be 1..N in order
  double coords[3];

  while (getline(in, line)) {
    string t = trim(line);
    if (t.empty() || t[0] == '#') continue;
    if (t.compare(0, 9, "@<TRIPOS>") == 0) {
      //  A second MOLECULE record starts the next molecule in the file.
      if (section == "ATOM" || section == "BOND") {
        if (t.substr(9, 8) == "MOLECULE") break;
      }
      section = trim(t.substr(9));
      molLine = 0;
      continue;
    }

    if (section == "MOLECULE") {
      molLine++;
      if (molLine == 1) frag.name(t);
      else if (molLine == 2) {
        istringstream ss(t);
        ss >> na >> nb;
      }
    } else if (section == "ATOM") {
      istringstream ss(t);
      int id = 0;
      string name, type;
      double x, y, z;
      if (!(ss >> id >> name >> x >> y >> z >> type)) {
        s_error = "cannot read a mol2 ATOM line: " + t;
        return false;
      }
      //  SYBYL type "C.3", "N.ar", "Fe" -> the part before the dot; a type
      //  that is not an element (Du, LP, "Any") leaves the name to decide.
      string sym = element(type.substr(0, type.find('.')));
      if (sym.empty()) sym = elementFromName(name);
      if (sym.empty()) {
        s_error = "cannot tell the element of mol2 atom " + to_string(id)
                + " (type '" + type + "', name '" + name + "')";
        return false;
      }
      coords[0] = x; coords[1] = y; coords[2] = z;
      frag.addAtom(sym, coords);
      if ((int)idToIndex.size() <= id) idToIndex.resize(id + 1, -1);
      if (id >= 0) idToIndex[id] = nAtomsRead;
      nAtomsRead++;
    } else if (section == "BOND") {
      istringstream ss(t);
      int id = 0, a1 = 0, a2 = 0;
      string type;
      if (!(ss >> id >> a1 >> a2 >> type)) {
        s_error = "cannot read a mol2 BOND line: " + t;
        return false;
      }
      if (a1 < 0 || a2 < 0 || a1 >= (int)idToIndex.size() ||
          a2 >= (int)idToIndex.size()) continue;
      int i1 = idToIndex[a1], i2 = idToIndex[a2];
      if (i1 < 0 || i2 < 0 || i1 == i2) continue;
      //  "ar" is the tree's aromatic 1.5; an amide C-N is formally single,
      //  and dummy/unknown/not-connected bonds are drawn as single too.
      double order = 1.0;
      if (type == "2") order = 2.0;
      else if (type == "3") order = 3.0;
      else if (type == "ar") order = 1.5;
      frag.addBond(i1, i2, order);
      hadBonds = true;
    }
  }

  if (nAtomsRead == 0) {
    s_error = s_error.empty() ? string("the mol2 file holds no atoms")
                              : s_error;
    return false;
  }
  if (nb > 0 && !hadBonds) {
    s_error = "the mol2 file says " + to_string(nb)
            + " bonds but holds no @<TRIPOS>BOND record";
    return false;
  }
  if (na > 0 && nAtomsRead != na) {
    s_error = "the mol2 file says " + to_string(na) + " atoms but holds "
            + to_string(nAtomsRead);
    return false;
  }

  finish(frag, genBonds, hadBonds);
  return true;
}
