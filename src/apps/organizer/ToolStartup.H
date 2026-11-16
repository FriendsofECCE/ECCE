/**
 * @file
 *
 * Feedback in the Organizer while a tool starts (#1071): busy cursor and
 * status line at once, a small "Starting ..." window after 1.5 s, a "has not
 * started" note after 30 s, all cleared by up(), which is the one place that
 * says a tool's window is there.
 */
#ifndef TOOLSTARTUP_HH
#define TOOLSTARTUP_HH

#include <functional>
#include <string>
#include <vector>

#include "wx/timer.h"

using std::string;

class wxFrame;
class wxGauge;
class wxStaticText;
class wxWindow;

class ToolStartup : public wxEvtHandler
{
public:
  // setStatus("") clears the Organizer's status line.
  ToolStartup(wxWindow *parent, std::function<void(const string&)> setStatus);
  ~ToolStartup();

  // A start request for tool `app` (ResourceDescriptor name) on `url`;
  // `label` is the tool's name for the user.  False if the same tool is
  // already starting for the same calculation: the caller must not start
  // another copy (never for forcenew requests).
  bool begin(const string& app, const string& label, const string& url,
             bool forceNew);

  // The tool's window is up (its invoke has been handled).  Called when the
  // tool's "ecce_tool_up" notice arrives; the tab code of one-process mode
  // calls it when a tab appears.
  void up(const string& app, const string& url);

  // Test hook: "starting", "up", "timed out" or "none", and the details.
  string state() const;
  string report() const;
  // Saves the small window as a PNG (tests, screenshots); false if none.
  bool shot(const string& file);

private:
  struct Entry {
    string app, label, url, calc;
    long started;     // wxGetLocalTimeMillis at begin
    bool timedOut;
    bool deferred;    // up arrived, held back by the test delay
  };

  void tick(wxTimerEvent&);
  void finish(size_t i);
  void refresh();
  void showWindow();
  void hideWindow();
  void dismiss();
  int starting() const;
  void setBusy(bool on);

  wxWindow *p_parent;
  std::function<void(const string&)> p_setStatus;
  std::vector<Entry> p_entries;
  wxTimer p_timer;
  wxFrame *p_frame = 0;
  wxStaticText *p_text = 0;
  wxGauge *p_gauge = 0;
  bool p_busy = false;
  string p_last = "none";   // outcome of the latest tool that is not starting
  string p_status;          // what the status line shows
  int p_launches = 0;
};

#endif
