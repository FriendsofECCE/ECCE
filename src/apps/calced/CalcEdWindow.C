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
  if (ToolTabs::enabled()) ToolTabs::adopt(calced, 0, _("Editor"));
  return msgs;
}


void registerCalcEdInProcess()
{
  InProcessTools::add(CALCED, openCalcEd);
}
