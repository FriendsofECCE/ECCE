#include "wx/statusbr.h"
#include "wx/frame.h"
#include "wx/bmpbuttn.h"

#include "dsm/EDSIServerCentral.H"

#include "wxgui/ewxFrame.H"
#include "wxgui/CalcDropHandler.H"
#include "wxgui/ewxStatusBar.H"
#include "wxgui/ewxTool.H"


ewxTool::ewxTool()
  : ewxFrame(),
    CalcDropHandler(),
    p_statusbar(NULL)
{

}


ewxTool::ewxTool(wxWindow* parent, wxWindowID id, const wxString& title,
                 const wxPoint& pos, const wxSize& size, long style,
                 const wxString& name)
  : ewxFrame(),
    CalcDropHandler(),
    p_statusbar(NULL)
{
  Create(parent, id, title, pos, size, style, name);
}


ewxTool::~ewxTool()
{}


bool ewxTool::Create(wxWindow* parent, wxWindowID id, const wxString& title,
                     const wxPoint& pos, const wxSize& size, long style,
                     const wxString& name)
{
  bool ret = ewxFrame::Create(parent, id, title, pos, size, style, name);
  p_statusbar = new ewxStatusBar(this);
  SetStatusBar(p_statusbar);
  return ret;
}


wxStatusBar* ewxTool::CreateStatusBar()
{
  return p_statusbar;
}


/**
 * The CalcDropHandler method.
 * setContext is automatically called.
 */
void ewxTool::processDrop(const string &url)
{
  setContext(url);
}


/**
 * Destroy the window.
 * Override to do prompting before quitting, etc.
 */
void ewxTool::quit()
{
  Destroy();
}


void ewxTool::saveSettings()
{}


void ewxTool::restoreSettings()
{}


void ewxTool::setSaveHandler(FeedbackSaveHandler *handler)
{
  if (p_statusbar)
    p_statusbar->setSaveHandler(handler);
}


void ewxTool::setEditStatus(const string & editStatus)
{
  p_statusbar->setEditStatus(editStatus);
}


void ewxTool::setRunState(long state)
{
  p_statusbar->setRunState(state);
}


void ewxTool::showRunState(bool show)
{
  p_statusbar->showRunState(show);
}


/**
 * Set context of tool to the url.
 * The default implementation simply sets the context state variable.
 * Tools should override this method.
 */
void ewxTool::setContext(const string& url)
{
  p_context = url;
  if (p_statusbar)
    p_statusbar->SetStatusText(EDSIServerCentral().mapURLtoName(url).c_str(),
                               ewxStatusBar::FIELD_URL);
}


string ewxTool::getEditStatus() const
{
  return p_statusbar->getEditStatus();
}


long ewxTool::getRunState() const
{
  return p_statusbar->getRunState();
}


/**
 * Get the tools context.
 * Tools probably don't need to override this method.
 */
string ewxTool::getContext() const
{
  return p_context;
}


void ewxTool::showCalcInfo(ResourceDescriptor::RUNSTATE state, bool writable,
                           const string& name)
{
  ewxTool::setContext(name);
  ewxTool::setRunState(state);
  
  if (state >= ResourceDescriptor::STATE_SUBMITTED || !writable)
    ewxTool::setEditStatus("READONLY");
  else
    ewxTool::setEditStatus("EDIT");
}
