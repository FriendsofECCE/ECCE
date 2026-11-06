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

#endif
