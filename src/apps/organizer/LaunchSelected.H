#ifndef LAUNCHSELECTED_HH
#define LAUNCHSELECTED_HH

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "comm/LaunchBatch.H"
#include "wx/event.h"
#include "wx/timer.h"

class TaskJob;
class wxWindow;
class wxFrame;
class wxStaticText;
class wxGauge;

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

  /** The batch as a state machine: one tick() advances it without waiting
   *  (a submit step runs on a worker thread), so the Organizer's event loop
   *  keeps running between ticks. */
  class Batch : public wxEvtHandler
  {
  public:
    /** `parent` null: no progress window and no timer, the caller ticks
     *  (test hook). `onDone` runs once, from the last tick. */
    Batch(wxWindow *parent, const std::vector<Row>& rows,
          const std::function<void(const std::vector<Row>&)>& onDone);
    ~Batch();
    void start();
    /** No more calculations after the one being submitted. */
    void stop();
    bool tick();                       // true once finished
    bool finished() const { return p_finished; }
    size_t number() const { return p_n; }
    size_t total() const { return p_total; }
    const std::vector<Row>& rows() const { return p_rows; }
  private:
    bool nextRow();
    void endRow(bool ok);
    void showProgress();
    void finish();
    std::vector<Row> p_rows;
    std::function<void(const std::vector<Row>&)> p_onDone;
    wxWindow *p_parent;
    wxFrame *p_frame;
    wxStaticText *p_label;
    wxGauge *p_gauge;
    wxTimer p_timer;
    size_t p_next, p_n, p_total;
    Row *p_cur;
    std::unique_ptr<LaunchBatch::Job> p_job;
    std::thread p_worker;
    std::atomic<bool> p_stepDone;
    bool p_stepOk, p_stopped, p_finished, p_deferred;
  };

  /** Submit every ready row, waiting here (test hook; no window). */
  static void run(std::vector<Row>& rows);

  static void showResults(wxWindow *parent, const std::vector<Row>& rows);

  /** One line per row: "<name>: submitted|failed|skipped[: why]". */
  static std::string report(const std::vector<Row>& rows);
};

#endif
