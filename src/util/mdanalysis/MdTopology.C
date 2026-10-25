#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>

#include "MdTopology.H"

namespace {

std::string trim(const std::string& s)
{
  size_t a = s.find_first_not_of(" \t\r\n");
  if (a == std::string::npos) return "";
  return s.substr(a, s.find_last_not_of(" \t\r\n") - a + 1);
}

std::string dirOf(const std::string& path)
{
  size_t p = path.find_last_of("/\\");
  return p == std::string::npos ? std::string(".") : path.substr(0, p);
}

bool isNumber(const std::string& s)
{
  if (s.empty()) return false;
  char *end = 0;
  strtod(s.c_str(), &end);
  return end && *end == 0;
}

struct Parser
{
  MdTopology& top;
  std::map<std::string, double> typeMass;
  std::string section;
  int depth = 0;

  explicit Parser(MdTopology& t) : top(t) {}

  void file(const std::string& path)
  {
    std::ifstream in(path.c_str());
    if (!in || depth > 8) return;
    depth++;
    std::string line;
    while (std::getline(in, line)) {
      size_t c = line.find(';');
      if (c != std::string::npos) line = line.substr(0, c);
      line = trim(line);
      if (line.empty()) continue;
      if (line[0] == '#') {
        std::istringstream ls(line);
        std::string d, name;
        ls >> d >> name;
        if (d == "#include" && name.size() > 2)
          file(dirOf(path) + "/" + name.substr(1, name.size() - 2));
        continue;
      }
      if (line[0] == '[') {
        size_t e = line.find(']');
        section = trim(line.substr(1, e == std::string::npos ? std::string::npos : e - 1));
        continue;
      }
      std::istringstream ls(line);
      std::vector<std::string> f;
      std::string w;
      while (ls >> w) f.push_back(w);
      if (section == "atomtypes") {
        // name [bond_type] [at.num] mass charge ptype sigma epsilon: the
        // particle-type letter is the anchor, the mass sits two fields before.
        for (size_t i = 2; i < f.size(); i++)
          if (f[i].size() == 1 && strchr("ASVD", f[i][0]) && isNumber(f[i - 2])) {
            typeMass[f[0]] = atof(f[i - 2].c_str());
            break;
          }
      } else if (section == "moleculetype" && !f.empty()) {
        top.types.push_back(MdTopology::MolType());
        top.types.back().name = f[0];
      } else if (section == "atoms" && f.size() >= 5 && !top.types.empty() &&
                 isNumber(f[0])) {
        MdTopology::Atom a;
        a.type = f[1];
        a.name = f[4];
        if (f.size() >= 8 && isNumber(f[7])) a.mass = atof(f[7].c_str());
        else if (typeMass.count(f[1])) a.mass = typeMass[f[1]];
        top.types.back().atoms.push_back(a);
      } else if (section == "molecules" && f.size() >= 2) {
        MdTopology::Entry e;
        e.type = f[0];
        e.count = atoi(f[1].c_str());
        top.molecules.push_back(e);
      }
    }
    depth--;
  }
};

}  // namespace

int MdTopology::atomCount() const
{
  int n = 0;
  for (const Entry& e : molecules) {
    const MolType *t = find(e.type);
    if (t) n += e.count * (int)t->atoms.size();
  }
  return n;
}

const MdTopology::MolType *MdTopology::find(const std::string& name) const
{
  for (const MolType& t : types) if (t.name == name) return &t;
  return 0;
}

void MdTopology::expand(std::vector<double>& mass, std::vector<int>& molecule,
                        std::vector<int>& typeIndex) const
{
  mass.clear(); molecule.clear(); typeIndex.clear();
  int mol = 0;
  for (const Entry& e : molecules) {
    const MolType *t = find(e.type);
    if (!t) continue;
    const int ti = (int)(t - &types[0]);
    for (int k = 0; k < e.count; k++, mol++)
      for (const Atom& a : t->atoms) {
        mass.push_back(a.mass);
        molecule.push_back(mol);
        typeIndex.push_back(ti);
      }
  }
}

bool readTopology(const std::string& path, MdTopology& top, std::string& error)
{
  top = MdTopology();
  std::ifstream probe(path.c_str());
  if (!probe) {
    error = "cannot open " + path;
    return false;
  }
  Parser p(top);
  p.file(path);
  if (top.molecules.empty() || top.atomCount() == 0) {
    error = path + " has no [ molecules ] with atoms";
    return false;
  }
  for (const MdTopology::MolType& t : top.types)
    for (const MdTopology::Atom& a : t.atoms)
      if (a.mass <= 0) {
        error = "no mass for atom " + a.name + " of " + t.name + " in " + path;
        return false;
      }
  return true;
}

double mdpReferenceTemperature(const std::string& path)
{
  std::ifstream in(path.c_str());
  std::string line;
  while (std::getline(in, line)) {
    size_t c = line.find(';');
    if (c != std::string::npos) line = line.substr(0, c);
    size_t eq = line.find('=');
    if (eq == std::string::npos) continue;
    std::string key = trim(line.substr(0, eq));
    std::replace(key.begin(), key.end(), '_', '-');
    if (key == "ref-t") return atof(trim(line.substr(eq + 1)).c_str());
  }
  return 0;
}

std::string elementForMass(double m)
{
  static const struct { const char *sym; double mass; } kElements[] = {
    {"H", 1.008}, {"He", 4.0026}, {"Li", 6.94}, {"C", 12.011}, {"N", 14.007},
    {"O", 15.9994}, {"F", 18.998}, {"Ne", 20.18}, {"Na", 22.99}, {"Mg", 24.305},
    {"S", 32.06}, {"Cl", 35.45}, {"Ar", 39.948}, {"K", 39.098}, {"Ca", 40.078},
    {"Kr", 83.798}, {"Xe", 131.293}};
  for (const auto& e : kElements)
    if (std::fabs(e.mass - m) < 0.02 * (m > 5 ? 1 : 0.1)) return e.sym;
  return "";
}
