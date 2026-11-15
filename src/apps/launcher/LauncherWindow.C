/**
 * @file
 *
 * A Launcher as a tab or window of the Organizer's process (#983): what
 * WxLauncherApp::OnInit does, without the once-per-process part (the
 * auth listener stays the Organizer's).
 */
#include "wx/wx.h"
#include "dsm/ResourceDescriptor.H"
#include "wxgui/InProcessTools.H"
#include "wxgui/ToolTabs.H"
#include "LauncherMessages.H"
#include "WxLauncher.H"

static WxJMSMessageDispatch *openLauncher()
{
  WxLauncher *frame = new WxLauncher(NULL);
  ToolTabs::open(frame, 0, _("Launcher"), "launcher");
  frame->loadSettings("Launcher");
  LauncherMessages *msgs = new LauncherMessages(frame);
  frame->setMessages(msgs);
  // The invoke that follows sets the calculation and shows the window.
  msgs->registerTopShell(frame);
  msgs->subscribeMessages();
  msgs->startSubscriber();
  InProcessTools::track(LAUNCHER, msgs, frame);
  return msgs;
}

// Organizer test hook "launcher machine|pick NAME", "launch", "get WHAT":
// the Launcher script's commands on the active Launcher tab.
static wxString launcherCommand(const wxString& arg)
{
  WxLauncher *l = dynamic_cast<WxLauncher*>(ToolTabs::activeTool());
  if (!l) return "not a Launcher tab";
  wxString cmd = arg.BeforeFirst(' '), rest = arg.AfterFirst(' ');
  string name = std::string(rest.utf8_str());
  if (cmd == "machine") return l->selectMachine(name) ? "ok" : "not in the list";
  if (cmd == "pick") return l->pickMachine(name) ? "ok" : "not in the list";
  if (cmd == "launch") return l->clickLaunch() ? "ok" : "Launch is missing or disabled";
  if (cmd == "get") return wxString::FromUTF8(l->hookGet(name).c_str());
  return "bad command";
}

void registerLauncherInProcess()
{
  ToolTabs::addCommand("launcher", launcherCommand);
  InProcessTools::add(LAUNCHER, openLauncher);
}
