/**
 * @file
 *
 * The log target of the one-program process (#983, #1123).
 *
 * wx has one log target per process.  Its default, wxLogGui, shows errors
 * in a modal box at the next idle; on macOS that box (an NSAlert) stops
 * idle and queued events, so a stray warning from any tool blocked Quit.
 * Here a message goes to the log panel of the tool it concerns (the
 * active tab, or the focused tool window), as it would in the tool's own
 * process; any other goes to a non-modal box over the main window.
 */
#ifndef TOOLLOG_HH
#define TOOLLOG_HH

#include "wx/log.h"

class wxTopLevelWindow;

class ToolLog : public wxLogGui
{
public:
  // Makes a ToolLog the process's log target (deletes the old one).
  static void install();
  // Whether install() has run: a tool then registers its panel here
  // rather than taking the process's target.
  static bool installed();
  // The tool's log panel; ToolLog does not own it.  Unregister before
  // deleting it.
  static void registerTool(wxTopLevelWindow *tool, wxLog *sink);
  static void unregisterTool(wxTopLevelWindow *tool);

protected:
  void DoLogRecord(wxLogLevel level, const wxString& msg,
                   const wxLogRecordInfo& info) override;

private:
  void DoShowSingleLogMessage(const wxString& message, const wxString& title,
                              int style) override;
  void DoShowMultipleLogMessages(const wxArrayString& messages,
                                 const wxArrayInt& severities,
                                 const wxArrayLong& times,
                                 const wxString& title, int style) override;
};

#endif
