/**
 * @file
 *
 * A Register Machines window inside the Organizer's process (#1066): what
 * WxMachineRegisterApp::OnInit does, without the once-per-process part
 * (pipe login, auth listener, which stay the Organizer's). A utility
 * window with modal dialogs of its own, so a window rather than a tab.
 */
#include "wx/wx.h"
#include "dsm/ResourceDescriptor.H"
#include "util/JMSMessage.H"
#include "wxgui/InProcessTools.H"
#include "wxgui/WxJMSMessageDispatch.H"
#include "wxgui/ewxWindowUtils.H"
#include "WxMachineRegister.H"

namespace {

// Carries the window's messages; deleted after the frame, with frame
// cleared first so a message that arrives in between does nothing.
class MachRegMessages : public WxJMSMessageDispatch
{
public:
  MachRegMessages(WxMachineRegister *frame)
    : WxJMSMessageDispatch(MACHREGISTER, true), frame(frame) {}

  void subscribeMessages()
  {
    subscribe("ecce_invoke", (wxJmsCBFunc)&MachRegMessages::invokeMCB);
    subscribe("ecce_quit", (wxJmsCBFunc)&MachRegMessages::quitMCB);
    subscribe("ecce_identify", (wxJmsCBFunc)&MachRegMessages::identifyMCB);
  }

  void invokeMCB(JMSMessage& msg)
  {
    if (!frame) return;
    string refname = msg.getProperty("initmachine");
    if (!refname.empty()) frame->selectMachine(refname);
    string page = msg.getProperty("initpage");
    if (!page.empty()) frame->showPage(page);
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
    JMSMessage *resp = newMessage();
    resp->addProperty("info", MACHREGISTER);
    publish("ecce_identify_reply", *resp);
    delete resp;
  }

  WxMachineRegister *frame;
};

WxJMSMessageDispatch *openMachRegister()
{
  WxMachineRegister *frame = new WxMachineRegister(NULL, false);
  MachRegMessages *msgs = new MachRegMessages(frame);
  msgs->registerTopShell(frame);
  msgs->subscribeMessages();
  msgs->startSubscriber();
  InProcessTools::track(MACHREGISTER, msgs, frame);
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

void registerMachRegisterInProcess()
{
  InProcessTools::add(MACHREGISTER, openMachRegister);
}
