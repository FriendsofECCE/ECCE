#ifndef LINEARPOINTGROUP_H
#define LINEARPOINTGROUP_H

#include <string>
#include <cctype>

// A fragment's point group is the one ECCE works in, and it must be finite:
// genmol cannot expand an infinite group, and no code takes one as input
// (Gaussian's PG=C*V is a route-card syntax error).  Yet the job parser
// stores each code's own report of the group -- Gaussian "C*V", ORCA
// "C(inf)v" -- back onto the molecule, so it has to be folded here to the
// largest finite subgroup autosym itself chooses for a linear molecule.
inline std::string finitePointGroup(const std::string& group)
{
   std::string key;
   for (size_t i = 0; i < group.size(); i++) {
      unsigned char c = group[i];
      if (std::isspace(c) || c == '(' || c == ')') continue;
      key += (char)std::toupper(c);
   }
   const std::string inf[] = { "*", "INF", "OO", "\xe2\x88\x9e" };
   for (const std::string& s : inf) {
      if (key == "C" + s + "V") return "C4v";
      if (key == "D" + s + "H") return "D4h";
   }
   return group;
}

#endif
