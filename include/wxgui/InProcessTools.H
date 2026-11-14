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
  // Asks every tool window to close; false if one refused (Organizer Quit).
  static bool closeAll(bool force);
};

#endif
