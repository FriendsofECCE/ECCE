#ifndef LAUNCHSELECTED_HH
#define LAUNCHSELECTED_HH

#include <functional>
#include <string>
#include <vector>

#include "comm/LaunchBatch.H"

class TaskJob;
class wxWindow;

/**
 * Run Mgmt > Launch Selected (#492): a summary of the selected calculations'
 * stored launch settings, then sequential submits with a progress dialog and
 * a result list. The submit steps are the Launcher's (Launch, via LaunchBatch).
 */
class LaunchSelected
{
public:
  struct Row {
    LaunchBatch::Plan plan;
    TaskJob *task;
    bool attempted, submitted;
    std::string outcome;       // reason for a failure, check text, or ""
    Row() : task(0), attempted(false), submitted(false) {}
  };

  static std::vector<Row> plan(const std::vector<TaskJob*>& tasks,
                               const LaunchBatch::Target& target = LaunchBatch::Target());

  /** The summary; true when the user chose Launch. `target` is the machine
   *  to launch on ("As set up" when empty) and is updated to the user's
   *  choice; the rows are re-planned when it changes. `openInLauncher` gets the
   *  URL of a skipped calculation. `shot` (test hook): save and return. */
  static bool confirm(wxWindow *parent, std::vector<Row>& rows,
                      LaunchBatch::Target& target,
                      const std::function<void(const std::string&)>& openInLauncher,
                      const std::string& shot = "");

  /** Submit every ready row in turn. `progress`: show the progress dialog. */
  static void run(wxWindow *parent, std::vector<Row>& rows, bool progress);

  static void showResults(wxWindow *parent, const std::vector<Row>& rows);

  /** One line per row: "<name>: submitted|failed|skipped[: why]". */
  static std::string report(const std::vector<Row>& rows);
};

#endif
