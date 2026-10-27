#ifndef RECONNECTSELECTED_HH
#define RECONNECTSELECTED_HH

#include <functional>
#include <string>
#include <vector>

#include "comm/JobCatchUp.H"

class TaskJob;
class wxWindow;

/**
 * Run Mgmt > Reconnect Selected (#601): a list of the selected calculations
 * with what can and cannot be reconnected, then the single-job reconnect for
 * each eligible one in turn, grouped by machine. The rules are
 * comm/ReconnectRules.H; this only shows them.
 */
class ReconnectSelected
{
public:
  struct Row {
    JobCatchUp::Plan plan;
    TaskJob *task;
    bool attempted, ok;
    std::string outcome;
    Row() : task(0), attempted(false), ok(false) {}
    bool eligible() const { return plan.problem.empty(); }
  };

  /** Eligible rows come first, in machine order, so the first job of a
   *  machine makes the connection the later ones reuse. */
  static std::vector<Row> plan(const std::vector<TaskJob*>& tasks);

  /** The list; true when the user chose Reconnect. `shot` (test hook): save
   *  the dialog as PNG and return false. */
  static bool confirm(wxWindow *parent, std::vector<Row>& rows,
                      const std::string& shot = "");

  /** `one` reconnects a single row (the Organizer's own single-job path). */
  static void run(wxWindow *parent, std::vector<Row>& rows, bool progress,
                  const std::function<bool(Row&)>& one);

  static void showResults(wxWindow *parent, const std::vector<Row>& rows);

  /** One line per row: "<name>|<machine>|<state>|<eligible or reason>". */
  static std::string listing(const std::vector<Row>& rows);

  /** One line per row: "<name>: reconnected|failed|skipped: why". */
  static std::string report(const std::vector<Row>& rows);
};

#endif
