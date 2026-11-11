#ifndef UMBRELLAWINDOWS_HH
#define UMBRELLAWINDOWS_HH

#include <string>
#include <vector>

class MdTask;
class MiniJson;
class Resource;

/**
 * Umbrella sampling (#762): the window calculations cut from a GROMACS pull
 * run.  Each window is an equilibration task and a production task in the
 * pull task's study, copies of its model with the pull rate 0.  Which pull
 * run a task belongs to is kept in its ecce:umbrella.* properties.
 */
class UmbrellaWindows
{
public:
  struct Window {
    int number;            ///< 1-based
    double distance;       ///< nm, where the pull run was at this window
    double target;         ///< nm, where the window is restrained
    std::string groFile;   ///< local .gro of the whole system with its box
  };

  /** Make the tasks; their names are appended to *names. */
  static bool create(MdTask *pull, const std::vector<Window>& windows,
                     int eqSteps, int prodSteps,
                     std::vector<std::string> *names, std::string *why);

  /** The study's tasks of the given role ("prod" or "eq") for this pull run,
      in window order. */
  static std::vector<Resource*> tasks(MdTask *pull, const std::string& role);

  /** Run `ecce-umbrella --json <args>`; false with *why on any failure,
      including an {"error": ...} answer. */
  static bool runCli(const std::vector<std::string>& args, MiniJson *out,
                     std::string *why);

  /** A folder on this machine holding <files> of the task under Outputs/
      (the task's own folder for a local calculation, else a scratch copy). */
  static std::string localFolder(Resource *task,
                                 const std::vector<std::string>& files);

  /** `windows`, or with outDir `extract`, on the pull run. A blank from/to
      is the whole pull. The CLI's answer is in *json. */
  static bool find(MdTask *pull, double spacing, const std::string& from,
                   const std::string& to, const std::string& outDir,
                   std::vector<Window> *windows, MiniJson *json,
                   std::string *why);

  /** `wham` over the production tasks that are Complete; csv gives the CSV
      text in *csv instead of the JSON. */
  static bool wham(MdTask *pull, double temperature, bool jacobian,
                   bool csv, MiniJson *json, std::string *csvText,
                   int *used, std::string *why);

  static std::string propName(const char *what);
};

#endif
