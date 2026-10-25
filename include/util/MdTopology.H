#ifndef MDTOPOLOGY__H
#define MDTOPOLOGY__H

#include <string>
#include <vector>

/**
 * The parts of a GROMACS .top that the trajectory analyses need: per
 * molecule type its atoms (name, type, mass in u), and the [ molecules ]
 * list.  #include files are read from the .top's directory; one that
 * cannot be found (the force field library) is skipped, which only matters
 * for an atom whose mass is not on its [ atoms ] line.  No wx.
 */
struct MdTopology
{
   struct Atom { std::string name, type; double mass = 0; };
   struct MolType { std::string name; std::vector<Atom> atoms; };
   struct Entry { std::string type; int count = 0; };

   std::vector<MolType> types;
   std::vector<Entry> molecules;

   int atomCount() const;
   const MolType *find(const std::string& name) const;
   /** Mass of every atom of the system in order; molecule index and type
       index of each atom are returned alongside. */
   void expand(std::vector<double>& mass, std::vector<int>& molecule,
               std::vector<int>& typeIndex) const;
};

bool readTopology(const std::string& path, MdTopology& top, std::string& error);

/** ref-t (K) of the first temperature-coupling group of a .mdp; 0 if none. */
double mdpReferenceTemperature(const std::string& path);

/** "Ar" for 39.948, "O" for 15.9994 ...; empty if the mass is not a known element's. */
std::string elementForMass(double mass);

#endif
