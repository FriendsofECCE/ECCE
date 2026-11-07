#ifndef FRAGORBRUN_HH
#define FRAGORBRUN_HH

#include <string>
#include <vector>

/**
 * Runs `ecce-fragorb analyse|dos ... --json` (#201) and reads its answer.
 * The GUI shows what the CLI computed; no formula is repeated here.
 * Energies in eV; shares as fractions (0.71 is 71 %).
 */
struct FragOrbFo
{
   int fragment = 0;          ///< 0-based, in the order the fragments were given
   int index = 0;             ///< 1-based within its fragment
   double energy = 0, occupation = 0;
};

struct FragOrbMo
{
   int index = 0;             ///< 1-based
   double energy = 0, occupation = 0, remainder = 0;
   std::vector<double> share; ///< one per FO of the channel
};

struct FragOrbChannel
{
   int spin = 0;              ///< 0 alpha (or restricted), 1 beta
   std::vector<FragOrbFo> fo;
   std::vector<FragOrbMo> mo;
};

struct FragOrbResult
{
   bool ok = false;
   std::string error;         ///< ecce-fragorb's own message when !ok
   std::string method;        ///< mulliken or scpa
   double threshold = 0.10;
   std::vector<std::string> fragments;   ///< names (folder names)
   std::vector<FragOrbChannel> channels;
};

struct FragDosResult
{
   bool ok = false;
   std::string error;
   double fermi = 0;          ///< HOMO/LUMO midpoint, eV
   std::vector<std::string> sets, pairs;
   std::vector<double> occupiedOverlap;  ///< per pair
   std::vector<double> energy, tdos;
   std::vector<std::vector<double> > pdos, opdos;   ///< [set or pair][grid point]
};

/** `ecce-fragorb analyse <complex> <fragment>... --method M --json`. */
FragOrbResult runFragOrb(const std::string& complex,
                         const std::vector<std::string>& fragments,
                         const std::string& method = "mulliken");

/** `ecce-fragorb dos <complex> <fragment>... --fwhm F --json`. */
FragDosResult runFragDos(const std::string& complex,
                         const std::vector<std::string>& fragments,
                         double fwhm = 0.3);

/** One level's share in another (an MO level's FO levels, an FO level's MO levels). */
struct FragDiagramShare
{
   int level = -1;            ///< drawn level id, -1 when not drawn
   int fragment = -1;
   std::vector<int> numbers;
   double energy = 0, share = 0;
   std::string position, irrep, shell;
};

/** A level of `ecce-fragorb diagram`: complex MOs (fragment -1) or FOs. */
struct FragDiagramLevel
{
   int id = 0, column = 1, fragment = -1;
   std::vector<int> numbers;  ///< 1-based MO numbers or FO indices
   double energy = 0;         ///< eV
   std::vector<double> occupation;
   std::string position, kind;
   std::string irrep;         ///< "2t2g", "" when not determined
   std::string shell;         ///< one-atom fragment: "Cr 3d"
   bool partner = false, representative = true;
   double remainder = 0, covered = 0, overlapPopulation = 0;
   std::vector<int> fragments;
   std::vector<FragDiagramShare> shares;
};

struct FragDiagramLine
{
   int fo = 0, mo = 0, fragment = 0;
   double share = 0;
};

struct FragDiagramChannel
{
   int spin = 0, homo = -1, lumo = -1;
   double gap = 0;
   bool representative = true;
   std::string verdict;
   std::vector<std::string> notes;
   std::vector<FragDiagramLevel> levels;
   std::vector<FragDiagramLine> lines;
};

struct FragDiagramResult
{
   bool ok = false;
   std::string error, method;
   double threshold = 0.10, covered = 0.90;
   int occupied = 4, virtuals = 4;
   std::vector<std::string> fragments;   ///< the names given with --name
   std::vector<std::string> groups;      ///< complex, then each fragment; "" not determined
   std::vector<FragDiagramChannel> channels;
};

/** `ecce-fragorb diagram <complex> <fragment>... --name N... --window O,V --threshold T --json`. */
FragDiagramResult runFragDiagram(const std::string& complex,
                                 const std::vector<std::string>& fragments,
                                 const std::vector<std::string>& names,
                                 int occupied = 4, int virtuals = 4,
                                 double threshold = 0.10);

#endif
