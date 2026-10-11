/**
 * @file
 *
 * The Progress box of the calculation summary (#284): where a running job
 * has got to, from the snapshot eccejobstore relays (ecce_url_progress) or
 * stores (ecce:progress).  Only paints; JobProgress (util) decides what
 * the lines say.
 */
#ifndef PROGRESSBOX_H
#define PROGRESSBOX_H

#include <string>
#include <vector>

#include <wx/panel.h>
#include <wx/timer.h>

#include "util/JobProgress.H"

class ProgressBox : public wxPanel
{
public:
  explicit ProgressBox(wxWindow *parent);
  ~ProgressBox();

  /**
   * text: the snapshot ("" for none); received: when it arrived, seconds
   * since 1970 (0: take the job store's stamp in the text); finished: the
   * job is over (summary instead of live lines); completed: it ended well.
   * Returns true when the box's size changed (the caller lays out again).
   */
  bool set(const std::string& text, double received, bool finished,
           bool completed);

  bool hasProgress() const { return !p_progress.empty(); }

  /** What the box shows, a line per row, for tests. */
  std::vector<std::string> describe() const;

  int paints() const { return p_paints; }

private:
  void OnPaint(wxPaintEvent& event);
  void OnTimer(wxTimerEvent& event);

  // Draws (dc != 0) or only measures; returns the size used.
  wxSize layout(wxDC *dc) const;
  double age() const;

  JobProgress p_progress;
  std::string p_text;
  double p_received;
  bool p_finished;
  bool p_completed;
  wxTimer p_timer;
  int p_paints;
};

#endif
