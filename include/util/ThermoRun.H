#ifndef THERMORUN_HH
#define THERMORUN_HH

#include <string>
#include <vector>

/**
 * The thermochemistry panel and Compare Energies show numbers only from
 * ecce-thermo (#692), so the treatments are defined in one place and this
 * runs the program (--json, all treatments) and reads the answer back.
 */
struct ThermoTreatmentInfo
{
   const char *cli;       ///< as ecce-thermo names it ("qrrho-grimme:100")
   const char *label;     ///< as the GUI shows it (UTF-8)
};

struct ThermoValue
{
   double H = 0, S = 0, G = 0;   ///< Hartree, J/mol/K, Hartree
};

struct ThermoRunResult
{
   bool ok = false;
   std::string error;            ///< why not, when !ok
   std::string name;
   double T = 0;
   std::vector<std::string> notes;
   std::vector<std::string> treatments;   ///< cli names, in order
   std::vector<ThermoValue> values;       ///< parallel to treatments
   const ThermoValue *find(const std::string& cli) const;
};

const std::vector<ThermoTreatmentInfo>& thermoTreatments();

/** Runs ecce-thermo on one calculation; oneMolar selects the 1 M standard state. */
ThermoRunResult runThermo(const std::string& url, double kelvin, bool oneMolar);

/**
 * Runs `ecce-thermo --reactions file --chosen treatment --format csv` (#709)
 * and returns its CSV; false with `error` when it could not be run.
 */
bool runThermoReactions(const std::string& file, const std::string& treatment,
                        std::string& csv, std::string& error);

/** Hartree -> kJ/mol, the constant ecce-thermo uses. */
double thermoHartreeToKJ(double hartree);

#endif
