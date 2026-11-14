/**
 * @file
 *
 * A Basis Set Tool window inside the Organizer's process (#983): what
 * WxBasisToolApp::OnInit does, without the once-per-process part (login,
 * the process's auth listener, which stays the Organizer's).
 */
#include "wx/wx.h"
#include "dsm/EDSIServerCentral.H"
#include "dsm/ResourceDescriptor.H"
#include "wxgui/InProcessTools.H"
#include "BasisToolMessages.H"
#include "WxBasisTool.H"

static WxJMSMessageDispatch *openBasisTool()
{
  WxBasisTool *tool = new WxBasisTool(NULL);
  tool->openLibraryAccess(EDSIServerCentral().getDefaultBasisSetLibrary());
  tool->loadSettings("BasisTool");
  BasisToolMessages *msgs = new BasisToolMessages(tool);
  tool->setMessages(msgs);
  // The invoke that follows sets the calculation and shows the window.
  msgs->registerTopShell(tool);
  msgs->subscribeMessages();
  msgs->startSubscriber();
  InProcessTools::track(BASISTOOL, msgs, tool);
  return msgs;
}

void registerBasisToolInProcess()
{
  InProcessTools::add(BASISTOOL, openBasisTool);
}
