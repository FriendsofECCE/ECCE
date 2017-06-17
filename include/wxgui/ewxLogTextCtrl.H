#ifndef EWXLOGTEXTCTRL_H
#define EWXLOGTEXTCTRL_H

#include "wx/log.h"
#include "wx/event.h"
#include "wx/wx.h"

class ewxLogTextCtrl: public wxLogTextCtrl, public wxEvtHandler
{
  DECLARE_CLASS(ewxLogTextCtrl)
  DECLARE_EVENT_TABLE()

public:
  ewxLogTextCtrl(wxTextCtrl * textCtrl, wxStatusBar * statusBar);
  virtual void DoLog(wxLogLevel level, const wxChar *szString, time_t t);
  virtual void OnContextMenu(wxContextMenuEvent& event);
  virtual void OnClearText(wxCommandEvent& event);
  virtual void OnShowGateway(wxCommandEvent& event);

protected:
  void FlashOn(const char* color);
  void FlashOff(wxTimerEvent& event);

  wxTextCtrl * p_textCtrl;
  wxStatusBar * p_statusBar;
  wxMenu p_textMenu;
  bool p_beepError;
  bool p_beepWarn;
  wxTimer* p_timer;
};

#endif
