/**
 * @file
 *
 * A periodic table window inside the Organizer's process (#1066): what
 * PerTabApp::OnInit does, without the once-per-process part (the pipe
 * login, the auth listener, which stay the Organizer's). A tab when the
 * window layout puts it in one (#1213), so ECCE stays one window.
 */
#include <sstream>

#include "wx/wx.h"
#include "dsm/ResourceDescriptor.H"
#include "util/JMSMessage.H"
#include "wxgui/InProcessTools.H"
#include "wxgui/PerTabFrame.H"
#include "wxgui/ToolTabs.H"
#include "wxgui/WxJMSMessageDispatch.H"
#include "wxgui/ewxWindowUtils.H"

namespace {

// The frame is the tool; this only carries its messages. It is deleted
// after the frame is gone, with frame cleared first so a message that
// arrives in between does nothing.
class PerTabMessages : public WxJMSMessageDispatch
{
public:
  PerTabMessages(PerTabFrame *frame) : WxJMSMessageDispatch(PERTAB, true),
                                       frame(frame) {}

  void subscribeMessages()
  {
    subscribe("ecce_invoke", (wxJmsCBFunc)&PerTabMessages::invokeMCB);
    subscribe("ecce_quit", (wxJmsCBFunc)&PerTabMessages::quitMCB);
    subscribe("ecce_identify", (wxJmsCBFunc)&PerTabMessages::identifyMCB);
    subscribe("ecce_preferences_pertab",
              (wxJmsCBFunc)&PerTabMessages::preferenceMCB);
  }

  void invokeMCB(JMSMessage&)
  {
    if (!frame) return;
    // A tab is brought forward by InProcessTools; showing the emptied frame
    // would map a blank window.
    if (ToolTabs::hosts(frame)) { ToolTabs::select(frame); return; }
    frame->Iconize(false);
    frame->Show(true);
    ewxRaiseWindow(frame);
    frame->RequestUserAttention();
  }

  void quitMCB(JMSMessage& msg)
  {
    if (frame && msg.getSender().getName() == GATEWAY) frame->Close(true);
  }

  void preferenceMCB(JMSMessage&)
  {
    if (frame) frame->resetUserPreferences();
  }

  void identifyMCB(JMSMessage&)
  {
    std::ostringstream os;
    os << "Periodic Table";
    if (frame) {
      vector<string> atoms = frame->selectedAtomSymbols();
      for (size_t i = 0; i < atoms.size(); i++) os << ": " << atoms[i];
    }
    JMSMessage *resp = newMessage();
    resp->addProperty("info", os.str());
    publish("ecce_identify_reply", *resp);
    delete resp;
  }

  PerTabFrame *frame;
};

WxJMSMessageDispatch *openPerTab()
{
  PerTabFrame *frame = new PerTabFrame(NULL);
  ToolTabs::open(frame, 0, _("Periodic Table"), "pertable");
  PerTabMessages *msgs = new PerTabMessages(frame);
  msgs->registerTopShell(frame);
  msgs->subscribeMessages();
  msgs->startSubscriber();
  InProcessTools::track(PERTAB, msgs, frame);
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

void registerPerTabInProcess()
{
  InProcessTools::add(PERTAB, openPerTab);
}
