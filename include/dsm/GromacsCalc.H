#ifndef GROMACSCALC_HH
#define GROMACSCALC_HH

#include <string>

#include "util/EcceURL.H"

class Fragment;
class Resource;
class TaskJob;

/**
 * A single GROMACS calculation made from the user's own files (#1038,
 * content type ecceMdRun): how structures go into and out of it.  Its
 * starting structure is its own .gro input; its final structure is the .gro
 * the run wrote (confout.gro).  No wx: the Organizer and Series use it.
 */
namespace GromacsCalc {

  /** True for a GROMACS calculation made from files. */
  bool isRun(const Resource* r);

  /** The .gro the run starts from (its own input); empty when it has none
      (a run made from a ready .tpr). */
  EcceURL startGro(TaskJob* run);

  /** The final frame a finished run wrote, "" when it has not run. */
  std::string finalGro(TaskJob* run);

  /** The final frame as a fragment, else the starting .gro, else the task's
      own fragment; 0 when there is none.  The caller deletes it. */
  Fragment* finalFragment(TaskJob* run);

  /** gro with its coordinates replaced by frag's (same atoms in the same
      order, else false with why).  Velocities are dropped, since they
      belong to the old coordinates; the box line is kept. */
  bool replaceCoordinates(const std::string& gro, const Fragment& frag,
                          std::string& out, std::string& why);

  /** Puts frag's coordinates into the run's starting .gro. */
  bool setStructure(TaskJob* run, const Fragment& frag, std::string& why);

  /** A copy of a finished run (outputs still present): its final frame
      becomes its starting .gro, velocities included. */
  bool continueFromFinal(TaskJob* copy, std::string& why);
}

#endif
