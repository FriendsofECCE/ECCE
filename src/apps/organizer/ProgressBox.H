/**
 * @file
 *
 * The Progress box of the calculation summary (#284): where a running job
 * has got to, from the snapshot eccejobstore relays (ecce_url_progress) or
 * stores (ecce:progress).  Only paints; JobProgress (util) decides what
 * the lines say.  The heading folds it to one line (remembered per user);
 * out of contact it greys the last progress and offers three actions.
 */
#ifndef PROGRESSBOX_H
#define PROGRESSBOX_H

#include <functional>
#include <string>
#include <vector>

#include <wx/panel.h>
#include <wx/timer.h>

#include "util/JobProgress.H"

class wxButton;
class wxStaticText;

class ProgressBox : public wxPanel
{
public:
  explicit ProgressBox(wxWindow *parent);
  ~ProgressBox();

  /**
   * text: the snapshot ("" for none); received: when it arrived, seconds
   * since 1970 (0: take the job store's stamp in the text); finished: the
   * job is over (summary instead of live lines); completed: it ended well;
   * lastHeard: seconds since 1970 while the job is out of contact, else 0;
   * checkLines: what Check Job on Machine said.
   * Returns true when the box's size changed (the caller lays out again).
   */
  bool set(const std::string& text, double received, bool finished,
           bool completed, double lastHeard = 0,
           const std::vector<std::string>& checkLines =
               std::vector<std::string>());

  // A submitted job waiting in its queue: shows when it was submitted
  // (seconds since 1970, 0 unknown) and its job id ("" unknown); set
  // before set().
  void setQueued(bool queued, double submitted = 0,
                 const std::string& jobId = "")
  { p_queued = queued; p_submitted = submitted; p_jobId = jobId; }

  bool hasProgress() const { return !p_progress.empty(); }
  bool outOfContact() const { return p_lastHeard > 0 && !p_finished; }
  // Shown at all: progress to show, or a job out of contact.
  bool wanted() const
  { return hasProgress() || outOfContact() || p_queued ||
           (p_finished && !p_finalEnergy.empty()); }

  // The plain final energy line of a finished calculation, shown after the
  // outcome, and its value and unit for Copy (#408); set before set().
  void setFinalEnergy(const std::string& line, const std::string& value = "",
                      const std::string& units = "")
  { p_finalEnergy = line; p_finalValue = value; p_finalUnits = units; }
  // What the right-click menu copies: 0 the final energy's value, 1 with
  // its unit, 2 every line the box shows.
  std::string copyText(int what) const;
  // The final energy is shown as a selectable label (for tests).
  bool energySelectable() const;

  bool collapsed() const { return p_collapsed; }
  // Folds or unfolds, remembers it for this user and lays out again.
  void setCollapsed(bool collapsed);

  // 0 Reconnect, 1 Check Job on Machine, 2 Show Output So Far.
  void setActionHandler(std::function<void(int)> handler)
  { p_action = handler; }
  // Called after the box changed size by itself (a fold).
  void setRelayout(std::function<void()> relayout) { p_relayout = relayout; }

  /** What the box shows, a line per row, for tests. */
  std::vector<std::string> describe() const;

  int paints() const { return p_paints; }

private:
  void OnPaint(wxPaintEvent& event);
  void OnTimer(wxTimerEvent& event);
  void OnSize(wxSizeEvent& event);
  void OnClick(wxMouseEvent& event);
  void OnContextMenu(wxContextMenuEvent& event);

  // Draws (dc != 0) or only measures and places the buttons; returns the
  // size used.
  wxSize layout(wxDC *dc);
  double age() const;
  std::string foldedText() const;
  std::vector<std::string> outcome() const;
  int headingHeight() const;

  // The blocks the box draws: the queued status, or the progress.
  std::vector<JobProgress::Block> blocks() const;

  bool p_queued = false;
  double p_submitted = 0;
  std::string p_jobId;
  JobProgress p_progress;
  std::string p_text;
  double p_received;
  bool p_finished;
  bool p_completed;
  double p_lastHeard;
  std::vector<std::string> p_check;
  std::string p_finalEnergy;
  std::string p_finalValue;
  std::string p_finalUnits;
  bool p_collapsed;
  wxTimer p_timer;
  int p_paints;
  // Per section of the last layout, for tests: text rows and plot frame.
  std::vector<std::string> p_geom;
  int p_laidOutWidth = 0;
  wxButton *p_buttons[3];
  wxStaticText *p_energyLabel;
  std::function<void(int)> p_action;
  std::function<void()> p_relayout;
};

#endif
