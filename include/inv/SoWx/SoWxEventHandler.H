#ifndef __SOWXEVENTHANDLER_H
#define __SOWXEVENTHANDLER_H

#include <wx/wx.h>
#include <wx/event.h>
#include <wx/timer.h>

#include "inv/SbTime.H"

#define ID_SOWXTIMER 1000000


class SoWxEventHandler : public wxEvtHandler {
public:
  SoWxEventHandler(wxWindow * topWin);
  SoWxEventHandler();

  ~SoWxEventHandler();


  void setUpCallbacks();
  
  void OnTimer(wxTimerEvent & event);
  void OnIdle(wxIdleEvent & event);

  static void sensorManagerChangedCallback(void * data);

private:
  DECLARE_EVENT_TABLE()

  wxTimer * p_timer;
  wxWindow * p_topWin;
  SbTime p_currentDeadline;
};

#endif
