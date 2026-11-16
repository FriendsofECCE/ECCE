/**
 * @file
 *
 * A Calculation Editor window inside the Organizer's process (#983): what
 * CalcEdApp::OnInit does for a start from the Organizer, without the
 * once-per-process part.
 */
#include "dsm/ResourceDescriptor.H"

#include "wxgui/InProcessTools.H"
#include "wxgui/ToolTabs.H"
#include "util/WindowLayout.H"

#include "CalcEd.H"
#include "CalcEdMessages.H"


static WxJMSMessageDispatch *openCalcEd()
{
  CalcEd *calced = new CalcEd(NULL);
  CalcEdMessages *msgs = new CalcEdMessages(calced);
  calced->setMessages(msgs);
  // The editor is shown by its invoke (showEditor), at its filled size.
  msgs->subscribeMessages();
  msgs->startSubscriber();
  InProcessTools::track(CALCED, msgs, calced);
  // Normal layout: one Set-up tab, the editor beside a detail area.
  ToolTabs::SideFn side;
  if (WindowLayout::combinedSetupTab())
    side = [calced](wxWindow *area) { calced->makeSetupArea(area); };
  ToolTabs::open(calced, 0, _("Editor"), "calced", side);
  return msgs;
}


void registerCalcEdInProcess()
{
  ToolTabs::addCommand("setup", [](const wxString& arg) -> wxString {
    CalcEd *ed = dynamic_cast<CalcEd*>(ToolTabs::activeTool());
    return ed ? ed->setupCommand(arg) : wxString("not a set-up tab");
  });
  InProcessTools::add(CALCED, openCalcEd);
}
