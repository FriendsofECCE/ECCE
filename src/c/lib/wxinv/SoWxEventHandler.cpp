#include <wx/wx.h>
#include <wx/timer.h>

#include "inv/SoDB.H"
#include "inv/SbTime.H"

#include "inv/SoWx/SoWxEventHandler.H"

#include <iostream>
using std::cerr;


BEGIN_EVENT_TABLE(SoWxEventHandler, wxEvtHandler)
  EVT_TIMER(ID_SOWXTIMER, SoWxEventHandler::OnTimer)
  EVT_IDLE(SoWxEventHandler::OnIdle)
END_EVENT_TABLE()


SoWxEventHandler::SoWxEventHandler(wxWindow * topWin)
  : wxEvtHandler() {
  p_topWin = topWin;
  p_topWin->PushEventHandler(this);
  //  p_workProcOn = false;
  p_timer = new wxTimer(this, ID_SOWXTIMER);

  SoDB::getSensorManager()->setChangedCallback
    (SoWxEventHandler::sensorManagerChangedCallback, this);

  setUpCallbacks();
}


/**
 * Change the sensor callback func to NULL when the evt handler obj is
 * destroyed. Doing this because it will be deleted by the wx automatically
 * and the sensor manager may still call its setUpCallbacks function after
 * which caused crash on exit.
 */
SoWxEventHandler::~SoWxEventHandler() {
  SoDB::getSensorManager()->setChangedCallback
    (SoWxEventHandler::sensorManagerChangedCallback, NULL);

  delete p_timer;
}


void SoWxEventHandler::setUpCallbacks() {
  // cerr << "SoWxEventHandler::setUpCallbacks\n";

  SbTime nextEvent;

  if (SoDB::getSensorManager()->isTimerSensorPending(nextEvent)) {
    // Only change the timer if its deadline is different from the current one.
    if (!p_timer->IsRunning() || nextEvent != p_currentDeadline) {
      long msec;

      if (p_timer->IsRunning()) {
        p_timer->Stop();
      }

      p_currentDeadline = nextEvent;
      msec = (nextEvent - SbTime::getTimeOfDay()).getMsecValue();
      p_timer->Start((msec > 0 ? msec : 0), true);
    }
  }
  /*
  else if (p_timer->IsRunning()) {
    // @todo Do we still need this? I think not
    p_timer->Stop();
  }
  */

  if (SoDB::getSensorManager()->isDelaySensorPending()) {
    wxIdleEvent event;
    ProcessEvent(event);
  }
}


void SoWxEventHandler::OnTimer(wxTimerEvent& WXUNUSED(event)) {
  //  cerr << "SoWxEventHandler::OnTimer\n";
  // process the timer queue
  SoDB::getSensorManager()->processTimerQueue();

  setUpCallbacks();
}


void SoWxEventHandler::OnIdle(wxIdleEvent & event) {
  // process the idle queue
  SoDB::getSensorManager()->processDelayQueue(true);

  if (SoDB::getSensorManager()->isDelaySensorPending())
    event.RequestMore();
}


void SoWxEventHandler::sensorManagerChangedCallback(void *data) {
  if (data != NULL) {
    SoWxEventHandler *eh = (SoWxEventHandler *) data;
    eh->setUpCallbacks();
  }
}
