#include <iostream>
  using std::cout;
  using std::endl;

#include "wx/log.h"
#include "wx/wx.h"

#include "util/JMSMessage.H"
#include "util/JMSPublisher.H"
#include "util/Preferences.H"
#include "util/PreferenceLabels.H"
#include "util/Color.H"
#include "wxgui/ewxLogTextCtrl.H"
#include "wxgui/ewxStyledWindow.H"
#include "wxgui/ewxColor.H"

IMPLEMENT_CLASS( ewxLogTextCtrl, wxEvtHandler )

BEGIN_EVENT_TABLE( ewxLogTextCtrl, wxEvtHandler )
  EVT_TIMER( wxID_ANY, ewxLogTextCtrl::FlashOff )
END_EVENT_TABLE()


ewxLogTextCtrl::ewxLogTextCtrl(wxTextCtrl * textCtrl, wxStatusBar * statusBar)
  :wxLogTextCtrl(textCtrl), wxEvtHandler()
{
  p_textCtrl = textCtrl;
  p_textMenu.Append(wxID_CLEAR, "Clear", "", wxITEM_NORMAL);
  Connect(wxID_CLEAR,
          wxEVT_COMMAND_MENU_SELECTED,
          wxCommandEventHandler(ewxLogTextCtrl::OnClearText));
  
  p_textMenu.AppendSeparator();
  p_textMenu.Append(wxID_HOME, "Show Gateway", "", wxITEM_NORMAL);
  Connect(wxID_HOME,
          wxEVT_COMMAND_MENU_SELECTED,
          wxCommandEventHandler(ewxLogTextCtrl::OnShowGateway));

  p_beepError = p_beepWarn = true;

  Preferences userPrefs(PrefLabels::GLOBALPREFFILE);
  userPrefs.getBool(PrefLabels::ERRORBEEP, p_beepError);
  userPrefs.getBool(PrefLabels::WARNINGBEEP, p_beepWarn);

  Connect(wxEVT_CONTEXT_MENU,
          wxContextMenuEventHandler(ewxLogTextCtrl::OnContextMenu));
  p_textCtrl->PushEventHandler(this);
  p_statusBar = statusBar;

  p_timer = new wxTimer(this);
}


void ewxLogTextCtrl::FlashOn(const char* color)
{
  ewxColor bkg(color);
  wxString saved = p_textCtrl->GetValue();
  p_textCtrl->Clear();
  p_textCtrl->SetBackgroundColour(bkg);
  p_textCtrl->SetValue(saved);

  if (p_statusBar)
    p_statusBar->SetBackgroundColour(bkg);

  p_timer->Start(500, wxTIMER_ONE_SHOT);
}


/**
 * Event handling function for timer.
 *
 * Restores the read-only background color for the ewxLogTextCtrl.
 */
void ewxLogTextCtrl::FlashOff(wxTimerEvent& WXUNUSED(event))
{

  wxString saved = p_textCtrl->GetValue();
  p_textCtrl->Clear();
  p_textCtrl->SetBackgroundColour(ewxStyledWindow::getReadonlyColor());
  p_textCtrl->SetValue(saved);

  if (p_statusBar)
    p_statusBar->SetBackgroundColour(ewxStyledWindow::getWindowColor());
}


void ewxLogTextCtrl::DoLog(wxLogLevel level, const wxChar *szString, time_t t)
{
  switch ( level ) {
  case wxLOG_FatalError:
    
    DoLogString(wxString(_("Fatal error: ")) + szString +"\n", t);
    DoLogString(_("Program aborted.\n"), t);
    Flush();
    abort();
    break;

  case wxLOG_Error:
    
    DoLogString(wxString(_("Error: ")) + szString +"\n", t);
    if (p_statusBar) {
      p_statusBar->SetStatusText(szString, 1);
      p_statusBar->SetToolTip(szString);
    }

    if (p_beepError)
      wxBell();

    FlashOn(Color::ERROR);
    break;

  case wxLOG_Warning:
    DoLogString(wxString(_("Warning: ")) + szString +"\n", t);
    if (p_statusBar) {
      p_statusBar->SetStatusText(szString, 1);
      p_statusBar->SetToolTip(szString);
    }

    if (p_beepWarn)
      wxBell();

    FlashOn(Color::WARNING);
    break;

  case wxLOG_Status:
    if (p_statusBar) {
      p_statusBar->SetStatusText(szString, 1);
      p_statusBar->SetToolTip(szString);
    }

  case wxLOG_Info:
    if ( !GetVerbose() )
      break;
  case wxLOG_Message:
  default:    // log unknown log levels too
    DoLogString(szString, t);
    break;

  case wxLOG_Trace:
  case wxLOG_Debug:
#ifdef __WXDEBUG__
    {
      wxString msg = level == wxLOG_Trace ? wxT("Trace: ")
        : wxT("Debug: ");
      msg << szString;
      DoLogString(msg, t);
    }
#endif
    break;
  }
}


void ewxLogTextCtrl::OnContextMenu(wxContextMenuEvent& event)
{
  p_textCtrl->PopupMenu(&p_textMenu);
}


void ewxLogTextCtrl::OnClearText(wxCommandEvent& event)
{
  p_textCtrl->Clear();
}


void ewxLogTextCtrl::OnShowGateway(wxCommandEvent& event)
{
  JMSPublisher publisher("Feedback");
  JMSMessage *msg = publisher.newMessage();
  publisher.publish("ecce_gateway_raise",*msg);
  delete msg;
}
