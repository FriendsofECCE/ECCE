/**
 * @file
 *
 * The editor as a window of the Organizer's process (#983): what
 * GmxPrepareApp::OnInit does, without the once-per-process part.
 */
#include "wx/wx.h"
#include "dsm/ResourceDescriptor.H"
#include "wxgui/InProcessTools.H"
#include "GmxPrepare.H"

void registerGmxPrepareInProcess()
{
  InProcessTools::add(GMXPREPARE, []() {
    return MDEdBase::openInProcess(new GmxPrepare((wxFrame*)NULL));
  });
}
