#ifndef LAUNCHBATCH_HH
#define LAUNCHBATCH_HH

#include <string>
#include <vector>

class TaskJob;
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
    Plan() : procs(0), ready(false) {}
  };

  /** What launching `task` would use; problem is set when it cannot. */
  static Plan plan(TaskJob *task);

  /** The substitutions Launch needs, as WxLauncher::buildArgs sets them. */
  static void buildArgs(TaskJob *task, EcceMap& kvargs);

  /** Store the run directory the Launcher would store (rundir + job path). */
  static bool storeRunDirectory(TaskJob *task);

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
