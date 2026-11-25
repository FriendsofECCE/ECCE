/**
 * @file
 *
 * Tool windows opened inside the Organizer's process (#983, with
 * ECCE_ONE_PROCESS=1): each tool registers a function that makes one
 * window with its own dispatcher, and an ecce_get_app for that tool opens
 * or reuses a window here instead of starting a program.
 */
#ifndef INPROCESSTOOLS_HH
#define INPROCESSTOOLS_HH

#include <functional>
#include <string>
using std::string;

class JMSMessage;
class WxJMSMessageDispatch;
class wxTopLevelWindow;

class InProcessTools
{
public:
  // Makes a window and its dispatcher, subscribed, not yet shown; the
  // invoke that follows shows it.  Returns the dispatcher, or 0.
  using Opener = std::function<WxJMSMessageDispatch*()>;

  static void add(const string& app, Opener opener);
  // The tool to open instead of app for this calculation URL, or "" for app
  // itself (the Builder on a run calculation is its Viewer, #1254).
  using Alias = std::function<string(const string& url)>;
  static void alias(const string& app, Alias to);
  // True if the switch is on and the tool opens in this process.
  static bool handles(const string& app);
  // The start request (appname, forcenew, calcurl, codename, ...): reuses
  // the window whose poll context matches unless forcenew, else opens one,
  // and sends it the request as its ecce_invoke.
  static bool open(JMSMessage& startMsg);

  // The opener calls this for the window it made: the window's dispatcher
  // is found by later requests, and while the window is active, data
  // notices go out under its tool name and id.  Untracked when the
  // dispatcher is deleted.
  static void track(const string& app, WxJMSMessageDispatch *d,
                    wxTopLevelWindow *frame);
  static void untrack(WxJMSMessageDispatch *d);
  // Open windows of a tool (tests and Quit).
  static int count(const string& app);
  // The calculation URL a tracked window has open (its poll context, else
  // the one it was opened for); empty if none.
  static string context(const wxTopLevelWindow *frame);
  // What the Organizer does for a calculation on behalf of another tool's
  // window (the editor's Results box): action is duplicate, duplicate-last,
  // duplicate-orbitals, rerun or terminate; answer "yes" or "cancel" stands
  // in for the confirm dialog, "" shows it.  The result is the new
  // calculation's URL for a duplicate, else ok/cancelled, or a reason.
  using CalcAction = std::function<string(const string& action,
                                          const string& url,
                                          const string& answer)>;
  static void setCalcAction(CalcAction fn);
  // "no Organizer in this process" when none is registered.
  static string calcAction(const string& action, const string& url,
                           const string& answer);
  // Asks every tool window to close; false if one refused (Organizer Quit).
  static bool closeAll(bool force);
  // A tool's close handler is running (it may yield): a Quit meanwhile
  // waits for it rather than tearing down under it.
  static void setClosing(wxTopLevelWindow *frame, bool on);
  static bool closing();
};

#endif
