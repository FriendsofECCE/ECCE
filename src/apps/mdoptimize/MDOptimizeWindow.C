/**
 * @file
 *
 * The editor as a window of the Organizer's process (#983): what
 * MDOptimizeApp::OnInit does, without the once-per-process part.
 */
#include "wx/wx.h"
#include "dsm/ResourceDescriptor.H"
#include "wxgui/InProcessTools.H"
#include "MDOptimize.H"

void registerMDOptimizeInProcess()
{
  InProcessTools::add(MDOPTIMIZE, []() {
    return MDEdBase::openInProcess(new MDOptimize((wxFrame*)NULL));
  });
}
