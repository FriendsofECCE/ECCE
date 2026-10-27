#ifndef LAUNCHBATCH_HH
#define LAUNCHBATCH_HH

#include <string>
#include <vector>

#include "dsm/TaskJob.H"

class TaskJob;
class MachinePreferences;
class EcceMap;
class Launch;

/**
 * Launching calculations with the settings they already store, without the
 * Launcher window (Organizer > Run Mgmt > Launch Selected, #492). No wx: the
 * Organizer drives the steps from a worker thread and does the asking.
 */
class LaunchBatch
{
public:
  struct Plan {
    std::string url, name, machine, queue, wall;
    unsigned long procs;
    bool ready;            // can be submitted from its stored settings
    std::string problem;   // why not, or a check that only warns
    Launchdata data;       // what the launch will use (stored when retarget)
    bool retarget;         // another machine than the one stored
    std::string changes;   // what differs from the stored settings
    Plan() : procs(0), ready(false), retarget(false) {}
  };

  /** Launch on another machine than the stored one (machine empty: as set up).
      profile "" is the machine's Default. */
  struct Target {
    std::string machine, profile;
    bool any() const { return !machine.empty(); }
  };

  /** A wall time is only needed where a queue manager takes it; a direct
      (Shell) or workstation launch has no limit to pass. */
  static bool wallTimeRequired(MachinePreferences *prefs);
  static bool noWallTime(const std::string& maxwall);
  /** The wall time the Launcher would fill in when none is stored. */
  static std::string defaultWallTime(MachinePreferences *prefs,
                                     const std::string& queue);

  /** What launching `task` would use; problem is set when it cannot. */
  static Plan plan(TaskJob *task, const Target& target = Target());

  /** Registered machines' allocation profile names (Default first). */
  static std::vector<std::string> profileNames(const std::string& machine);

  /** Store the machine and settings of a retargeted plan with the
      calculation, as the Launcher does; true when nothing needs storing. */
  static bool storeTarget(TaskJob *task, const Plan& plan);

  /** The substitutions Launch needs, as WxLauncher::buildArgs sets them. */
  static void buildArgs(TaskJob *task, EcceMap& kvargs);

  /** Store the run directory the Launcher would store (rundir + job path). */
  static bool storeRunDirectory(TaskJob *task);

  /** Keep why a submit failed with the calculation: in its run log and in
      runStatusReason, which the Organizer shows. The state is not changed. */
  static void recordFailure(TaskJob *task, const std::string& reason);

  /** A submit that went through removes a reason left by an earlier failure. */
  static void clearFailure(TaskJob *task);

  /** Steps of one launch: begin(), then step() until done() or failed. */
  class Job {
  public:
    explicit Job(TaskJob *task);
    ~Job();
    bool begin(std::string& error);
    bool done() const;
    std::string description() const;
    bool step();                       // false: failed, see message()
    std::string message() const;
    std::string takeWarning();         // launch check text, "" if none
  private:
    TaskJob *p_task;
    Launch *p_launch;
    EcceMap *p_args;
  };
};

#endif
