/**
 * @file
 *
 * The editor as a window of the Organizer's process (#983): what
 * MDDynamicsApp::OnInit does, without the once-per-process part.
 */
#include "wx/wx.h"
#include "dsm/ResourceDescriptor.H"
#include "wxgui/InProcessTools.H"
#include "MDDynamics.H"

void registerMDDynamicsInProcess()
{
  InProcessTools::add(MDDYNAMICS, []() {
    return MDEdBase::openInProcess(new MDDynamics((wxFrame*)NULL));
  });
}
