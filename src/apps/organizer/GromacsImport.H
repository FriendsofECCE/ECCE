#ifndef GROMACSIMPORT_HH
#define GROMACSIMPORT_HH

#include <functional>
#include <string>

#include <string>

class Resource;

/**
 * Import a finished GROMACS run as a GROMACS MD study.
 *
 * runDir holds one subdirectory per task, in run order (sorted by name,
 * e.g. 1-prepare, 2-optimize, 3-equilibrate, 4-dynamics); the kind of a
 * task is the word prepare, optimize, equilibrate or dynamics in its name.
 * A study may start at any task and need not have a Prepare.
 *
 * In a task directory the files are told apart by extension:
 *   Optimize/Equilibrate/Dynamics
 *     *.mdp                  the task's input (stored as gromacs.mdp)
 *     *.top, *.gmxtop, *.itp, *.ndx   inputs, attached as on the Inputs tab
 *     *.gro except confout.gro        the start structure, an input
 *     confout.gro            the final structure (the task's output)
 *     *.gmxlog, *.log        the mdrun log
 *     ecce-parse-traj.txt    the trajectory frames, if any
 *   Prepare
 *     *.gmxprep, *.recipe    the recipe (input)
 *     *.pdb                  the source structure (input, kept as .gmxpdb)
 *     *.top, *.gmxtop, *.itp, *.gro   the outputs of the run
 *     *.gmxlog, *.log        its output, with the ECCE-PREPARE lines
 *
 * Properties are not written here: each task's log is handed to the job
 * monitor, which parses it with gromacs.desc and stores the result as for
 * a live run, so they appear shortly after this returns.
 *
 * beforeTask(kind) is called before each task is parsed (a task takes a
 * few seconds), so the caller can keep its progress display and window alive.
 *
 * Returns the study's URL, or "" with the reason in message (a failure
 * after the study was made returns its URL with the reason in message).
 */
std::string importGromacsRun(Resource* parent, const std::string& studyName,
                             const std::string& runDir, std::string& message,
                             const std::function<void(const std::string&)>&
                                 beforeTask = nullptr);

#endif
