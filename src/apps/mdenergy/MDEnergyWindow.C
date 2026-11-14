/**
 * @file
 *
 * The editor as a window of the Organizer's process (#983): what
 * MDEnergyApp::OnInit does, without the once-per-process part.
 */
#include "wx/wx.h"
#include "dsm/ResourceDescriptor.H"
#include "wxgui/InProcessTools.H"
#include "MDEnergy.H"

void registerMDEnergyInProcess()
{
  InProcessTools::add(MDENERGY, []() {
    return MDEdBase::openInProcess(new MDEnergy((wxFrame*)NULL));
  });
}
