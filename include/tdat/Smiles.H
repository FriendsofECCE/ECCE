// ECCE's own SMILES reader and 3-D builder (#867).  No wx, no Open Babel.
#ifndef ECCE_SMILES_H
#define ECCE_SMILES_H

#include <array>
#include <string>
#include <vector>

namespace Smiles {

struct Atom {
   std::string el;        // "*" for the attachment dummy
   bool aromatic = false;
   bool bracket = false;
   int isotope = 0;
   int charge = 0;
   int hcount = 0;        // total hydrogens, explicit and implicit
   int chir = 0;          // 0 none, 1 '@', 2 '@@'
};

struct Bond {
   int a, b;
   int ord = 1;           // 1,2,3 after kekulisation
   bool arom = false;     // was aromatic in the SMILES
   char dir = 0;          // '/' or '\\', meaning b is above/below dirFrom
   int dirFrom = -1;
   bool closure = false;  // ring-closure bond (not an edge of the build tree)
};

struct Graph {
   std::vector<Atom> atoms;
   std::vector<Bond> bonds;
   // Neighbours in SMILES order (ring-closure digits where they are
   // written); implicit hydrogens are not listed.
   std::vector<std::vector<int> > nbr;
   std::vector<int> prior;   // atom written just before (or -1): chirality order
};

/// Parse (organic subset, bracket atoms, branches, rings, '.', stereo marks),
/// kekulise aromatic systems and fill in implicit hydrogens.  err names the
/// column of the offending character.
bool parse(const std::string& smiles, Graph& g, std::string& err);

struct Built {
   std::vector<std::string> el;
   std::vector<std::array<double,3> > xyz;
   std::vector<int> charge;
   struct B { int a, b; double order; bool arom; };
   std::vector<B> bonds;
   int dummy = -1;        // index of the atom that stood for [*], written as H
};

/// Starting 3-D structure: ideal bond lengths and angles, rings from
/// polygon and chair templates, tetrahedral and E/Z stereo as written,
/// anti torsions, then a short relaxation.  Hydrogens are explicit.
bool build3D(const Graph& g, Built& out, std::string& err);

/// Typical bond length used by the builder.
double idealBond(const std::string& a, const std::string& b, double order);

}

#endif
