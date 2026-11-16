/**
 * @file
 *
 * A Machine Browser window inside the Organizer's process (#1066): what
 * MachineBrowserApp::OnInit does, without the once-per-process part (the
 * pipe login, the auth listener, which stay the Organizer's). A utility
 * window, so it stays a window of its own rather than a tab.
 */
#include "wx/wx.h"
#include "dsm/ResourceDescriptor.H"
#include "tdat/RefMachine.H"
#include "dsm/MachinePreferences.H"
#include "util/JMSMessage.H"
#include "wxgui/InProcessTools.H"
#include "wxgui/WxJMSMessageDispatch.H"
#include "wxgui/ewxWindowUtils.H"
#include "MachineBrowser.H"

namespace {

// Carries the window's messages; deleted after the frame, with frame
// cleared first so a message that arrives in between does nothing.
class MachBrowserMessages : public WxJMSMessageDispatch
{
public:
  MachBrowserMessages(MachineBrowser *frame)
    : WxJMSMessageDispatch(MACHBROWSER, true), frame(frame) {}

  void subscribeMessages()
  {
    subscribe("ecce_invoke", (wxJmsCBFunc)&MachBrowserMessages::invokeMCB);
    subscribe("ecce_quit", (wxJmsCBFunc)&MachBrowserMessages::quitMCB);
    subscribe("ecce_identify", (wxJmsCBFunc)&MachBrowserMessages::identifyMCB);
    subscribe("ecce_machconf_changed",
              (wxJmsCBFunc)&MachBrowserMessages::machineConfigChangedMCB);
    subscribe("ecce_machreg_changed",
              (wxJmsCBFunc)&MachBrowserMessages::machineRegChangedMCB);
  }

  void invokeMCB(JMSMessage&)
  {
    if (!frame) return;
    frame->Iconize(false);
    frame->Show(true);
    ewxRaiseWindow(frame);
    frame->RequestUserAttention();
  }

  void quitMCB(JMSMessage& msg)
  {
    if (frame && msg.getSender().getName() == GATEWAY) frame->Close(true);
  }

  void identifyMCB(JMSMessage&)
  {
    string info = "Machine Browser";
    if (frame) {
      string sel = frame->getSelectionName();
      if (!sel.empty()) info += ":  " + sel;
    }
    JMSMessage *resp = newMessage();
    resp->addProperty("info", info);
    publish("ecce_identify_reply", *resp);
    delete resp;
  }

  void machineConfigChangedMCB(JMSMessage& msg)
  {
    string id = msg.getSender().getID();
    if (frame && id != getMyID() && id != frame->getMyID()) refresh();
  }

  void machineRegChangedMCB(JMSMessage&)
  {
    if (frame) refresh();
  }

  void refresh()
  {
    RefMachine::markUpdateUserPrefs();
    MachinePreferences::refresh(true);
    frame->refreshList();
  }

  MachineBrowser *frame;
};

WxJMSMessageDispatch *openMachBrowser()
{
  MachineBrowser *frame = new MachineBrowser(NULL);
  MachBrowserMessages *msgs = new MachBrowserMessages(frame);
  msgs->registerTopShell(frame);
  msgs->subscribeMessages();
  msgs->startSubscriber();
  InProcessTools::track(MACHBROWSER, msgs, frame);
  frame->Bind(wxEVT_DESTROY, [frame, msgs](wxWindowDestroyEvent& ev) {
    if (ev.GetEventObject() == frame) {
      InProcessTools::untrack(msgs);
      msgs->frame = 0;
      wxTheApp->CallAfter([msgs]() { delete msgs; });
    }
    ev.Skip();
  });
  return msgs;
}

}

void registerMachBrowserInProcess()
{
  InProcessTools::add(MACHBROWSER, openMachBrowser);
}
