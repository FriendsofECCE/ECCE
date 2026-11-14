
#include "wx/timer.h"

#include "util/EcceURL.H"

#include "tdat/AuthCache.H"

#include "dsm/EDSIFactory.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/ResourceDescriptor.H"

#include "wxgui/ewxWindowUtils.H"
#include "CalcEdApp.H"
#include "CalcEdMessages.H"
#include "CalcEd.H"

IMPLEMENT_APP(CalcEdApp)


CalcEdApp::CalcEdApp()
  : ewxApp(),
    p_calced(NULL),
    p_msgs(NULL)
{

}


CalcEdApp::~CalcEdApp()
{

}


bool CalcEdApp::OnInit()
{
  ewxApp::OnInit();

  if (argc>2 && strcmp(argv[1].ToStdString().c_str(),"-pipe")==0) {
    AuthCache::getCache().pipeIn(argv[2].ToStdString());
  }

  p_calced = new CalcEd(NULL);
  p_msgs = new CalcEdMessages(p_calced);
  p_calced->setMessages(p_msgs);

  string context("");

  //  "-context URL" may follow "-pipe FILE" (tests/apps/qe_walkthrough_test.py).
  for (int a = 1; a + 1 < this->argc; a++) {
    if (strcmp(this->argv[a].ToStdString().c_str(), "-context") == 0) {
      context = this->argv[a + 1];
      break;
    }
  }

  SetTopWindow(p_calced);
  if (!context.empty()) {
    p_calced->setContext(context);
    p_msgs->showEditor();
  } else {
    //  Started by the gateway, the calculation arrives in ecce_invoke after
    //  notifyReady(); shown before it, the empty editor is resized to the
    //  filled one's size on screen.  The timer covers a start without one.
    wxTimer *timer = new wxTimer();   // lives until the process exits
    CalcEdMessages *msgs = p_msgs;
    timer->Bind(wxEVT_TIMER, [msgs](wxTimerEvent&) {
      if (!msgs->shown()) msgs->showEditor();
    });
    timer->StartOnce(5000);
  }

  //  Subscribe to Messages
  p_msgs->subscribeMessages();
  p_msgs->startSubscriber();
  p_msgs->notifyReady();

  return true;
}


int CalcEdApp::OnExit()
{
  //  The editor window owns its dispatcher and unsubscribes it.

  return 0;
}


string CalcEdApp::getName() const
{
  return CALCED;
}
