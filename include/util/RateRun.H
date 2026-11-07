#ifndef RATERUN_HH
#define RATERUN_HH

#include <string>
#include <vector>

/**
 * Runs `ecce-rate ... --json` (#872) and reads its answer.  The GUI draws
 * what the CLI computed; no formula is repeated here.  Absent values
 * (no Eckart, no product) are NaN.
 */
struct RateRequest
{
   std::vector<std::string> reactants;   ///< calculation folders
   std::string ts;
   std::vector<std::string> products;    ///< may be empty
   std::string temperatures = "298.15";  ///< --T: 298.15, or LOW:HIGH:STEP
   std::string treatment = "qrrho-grimme:100";
   std::string standardState = "1M";
};

struct RateRow
{
   double T = 0, dG = 0, dH = 0, dS = 0;            ///< activation, kJ/mol, J/mol/K
   double kappaWigner = 0, kappaEckart = 0;
   double k = 0, kWigner = 0, kEckart = 0;          ///< no tunnelling, Wigner, Eckart
   double dGrxn = 0, dGreverse = 0, Keq = 0;        ///< with a product
   double kRev = 0, kRevWigner = 0, kRevEckart = 0;
};

struct RateFit
{
   std::string method;                              ///< TST, Wigner, Eckart
   double A = 0, Ea = 0, dH = 0, dS = 0;
};

struct RateResult
{
   bool ok = false;
   std::string error;                               ///< ecce-rate's own message when !ok
   std::vector<std::string> notes;
   std::string unit, unitReverse, treatment, standardState;
   bool hasProduct = false;
   double nu = 0;                                   ///< imaginary frequency, cm-1
   double barrier = 0;                              ///< ZPE-corrected, kJ/mol
   std::vector<RateRow> rows;
   std::vector<RateFit> fits;
};

RateResult runRate(const RateRequest& req);

/**
 * Runs `ecce-morse <calc> --json` (#858), the Morse fit of a bond scan; it
 * shares ecce-rate's JSON reader.  `report` is the CLI's own text.
 */
struct MorseResult
{
   bool ok = false;
   std::string error;
   std::string report, bond, windowMode;
   double De_eV = 0, De_kJ = 0, a = 0, re = 0;      ///< eV, kJ/mol, 1/A, A
   double omega = 0, omegaX = 0;                    ///< cm-1
   double D0_eV = 0, D0_kJ = 0, rms = 0, window = 0; ///< rms, window: kJ/mol
   int used = 0, total = 0;
   std::vector<double> curveR, curveE;              ///< A, Hartree
};

/** windowKj <= 0: every bound point. */
MorseResult runMorse(const std::string& calcFolder, double windowKj);

/** The folder of a stored calculation URL; empty for a remote (server) one. */
std::string rateLocalPath(const std::string& url);

#endif
