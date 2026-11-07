/**
 * @file
 *
 * ECCE's mdanalysis program run on a GROMACS run's files, as the job script
 * does it (gensub, gromacsMdAnalysis): displacements from the .xtc and, when
 * the .mdp saved velocities, speeds from the .trr.  Used where the job could
 * not run it: an imported run directory and a run whose host lacks ECCE.
 * No wx.
 */
#ifndef MDANALYSISRUN_HH
#define MDANALYSISRUN_HH

#include <string>

namespace MdAnalysisRun
{
    /// The value of KEY (nstvout, nstxout-compressed ...) in .mdp text; 0 if absent.
    int mdpSteps(const std::string& mdpText, const std::string& key);

    /// $ECCE_HOME/bin/mdanalysis (.exe on Windows).
    std::string program();

    /// The text of ecce-parse-md.txt, or "" when mdanalysis gave nothing.
    /// trr may be ""; work is a writable scratch directory.
    std::string run(const std::string& mda, const std::string& top,
                    const std::string& mdp, const std::string& xtc,
                    const std::string& trr, const std::string& work);
}

#endif
