#ifndef QMSOLVENTWARNING_HH
#define QMSOLVENTWARNING_HH

#include <string>

/// ECCE-QM has an analytic CPCM gradient (and finite-difference Hessians of
/// it), so every run type works with a solvent.  Raman activities need the
/// solvent response to a field, which is not implemented: that run stops at
/// job start.  One text for the Calculation Editor's notes and the pre-launch
/// warning.  Empty when the run is fine.
inline std::string qmSolventWarning(const std::string& solvent,
                                    const std::string& runType,
                                    bool raman = false)
{
  if (solvent.empty() || solvent == "None" || !raman)
    return std::string();
  return "Solvent (" + solvent + ") does not work with Raman activities: "
         "ECCE-QM has no solvent response to an electric field, so the " +
         runType + " run will stop at job start. Set the solvent to None or "
         "switch Raman off.";
}

#endif
