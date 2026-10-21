// Plain-text structure readers shared by atommap_cli and atommap_test:
// .xyz, and the atom_list block of a Structure Library .mvm file.
#ifndef ATOMMAP_XYZIO_HH
#define ATOMMAP_XYZIO_HH

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

struct Xyz {
   std::vector<std::string> el;
   std::vector<double> c;
};

inline bool readXyz(const std::string& path, Xyz& s)
{
   std::ifstream in(path.c_str());
   if (!in) return false;
   std::string line;
   int n = 0;
   if (!std::getline(in, line)) return false;
   std::istringstream(line) >> n;
   std::getline(in, line);   // comment
   s.el.clear(); s.c.clear();
   for (int i = 0; i < n && std::getline(in, line); i++) {
      std::istringstream l(line);
      std::string e; double x, y, z;
      if (!(l >> e >> x >> y >> z)) return false;
      s.el.push_back(e);
      s.c.push_back(x); s.c.push_back(y); s.c.push_back(z);
   }
   return (int)s.el.size() == n && n > 0;
}

// Nub pseudo-atoms (library ligand markers) are skipped.
inline bool readMvm(const std::string& path, Xyz& s)
{
   std::ifstream in(path.c_str());
   if (!in) return false;
   std::string line;
   bool inList = false;
   s.el.clear(); s.c.clear();
   while (std::getline(in, line)) {
      if (line.compare(0, 10, "atom_list:") == 0) { inList = true; continue; }
      if (!inList) continue;
      std::istringstream l(line);
      std::string e; double x, y, z;
      if (!(l >> e >> x >> y >> z)) break;
      if (e == "Nub") continue;
      s.el.push_back(e);
      s.c.push_back(x); s.c.push_back(y); s.c.push_back(z);
   }
   return !s.el.empty();
}

#endif
