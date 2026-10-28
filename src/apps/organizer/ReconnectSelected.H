#ifndef RECONNECTSELECTED_HH
#define RECONNECTSELECTED_HH

#include <functional>
#include <string>
#include <vector>

#include "comm/JobCatchUp.H"

class TaskJob;
class wxWindow;
class wxProgressDialog;

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

  /** `one` reconnects a single row (the Organizer's own single-job path);
   *  its progress dialog (0 without one) is for openLogin.  A Cancel stops
   *  the rows after the current one. */
  static void run(wxWindow *parent, std::vector<Row>& rows, bool progress,
                  const std::function<bool(Row&, wxProgressDialog*)>& one);

  /** Opens `login` on a worker thread, the GUI answering meanwhile: `pd`
   *  pulses, or a dialog of its own shows after a second.  Cancel abandons
   *  the connect (the worker drops its late result) and returns false, as
   *  does a failed login; `why` says which.  Every connect from the
   *  Organizer's GUI thread goes through here: a host that accepts TCP and
   *  never answers otherwise freezes the window for a minute per try. */
  static bool openLogin(wxWindow *parent, wxProgressDialog *pd,
                        const RCommandLogin& login, RCommand*& conn,
                        std::string& why, bool *cancelled = 0);

  /** Runs `work` (remote commands after the login) on a worker thread as
   *  openLogin connects: `pd` pulses with `what`, or a dialog of its own
   *  shows after a second.  False on Cancel: the worker is abandoned and
   *  finishes alone, so `work` must own what it uses.  Else `result` is
   *  what `work` returned. */
  static bool runRemote(wxWindow *parent, wxProgressDialog *pd,
                        const std::string& what,
                        const std::function<bool()>& work, bool& result);

  /** Broker messages wait while this lives (as for Launch Selected). */
  struct DeferMessages {
    DeferMessages();
    ~DeferMessages();
  };

  static void showResults(wxWindow *parent, const std::vector<Row>& rows);

  /** One line per row: "<name>|<machine>|<state>|<eligible or reason>". */
  static std::string listing(const std::vector<Row>& rows);

  /** One line per row: "<name>: reconnected|failed|skipped: why". */
  static std::string report(const std::vector<Row>& rows);
};

#endif
