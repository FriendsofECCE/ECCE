#ifndef QMSOLVENTWARNING_HH
#define QMSOLVENTWARNING_HH

#include <string>

/// ECCE-QM has a CPCM solvent energy but no solvent gradient, so every run
/// type except Energy with a solvent stops at once at job start.  One text
/// for the Calculation Editor's notes and the pre-launch warning.
/// Empty when the run is fine.
inline std::string qmSolventWarning(const std::string& solvent,
                                    const std::string& runType)
{
  if (solvent.empty() || solvent == "None" || runType == "Energy")
    return std::string();
  return "Solvent (" + solvent + ") works for Energy runs only: ECCE-QM has no "
         "solvent gradient, so the " + runType + " run will stop at job start. "
         "Set the solvent to None or the run type to Energy.";
}

#endif
