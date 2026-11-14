/**
 * @file
 *
 * MD Prepare as a window of the Organizer's process (#983): what
 * PrepareApp::OnInit does, without the once-per-process part.
 */
#include "wx/wx.h"
#include "dsm/ResourceDescriptor.H"
#include "wxgui/InProcessTools.H"
#include "wxgui/WxJMSMessageDispatch.H"
#include "MDPrep.H"

static WxJMSMessageDispatch *openMDPrep()
{
  MDPrep *prep = new MDPrep((wxFrame*)NULL);
  WxJMSMessageDispatch *d = prep->getDispatcher();
  prep->Show(true);
  InProcessTools::track(MDPREPARE, d, prep);
  return d;
}

void registerMDPrepInProcess()
{
  InProcessTools::add(MDPREPARE, openMDPrep);
}
