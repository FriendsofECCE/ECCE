/**
 * @file
 * @author Ken Swanson
 *
 */


#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "util/Ecce.H"
#include "util/ResourceUtils.H"
#include "util/EcceURL.H"

#include "dsm/EDSIFactory.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/ResourceDescriptor.H"

#include "wxgui/ewxWindowUtils.H"
#include "WxLauncherApp.H"
#include "LauncherMessages.H"
#include "WxLauncherGUI.H"
#include "WxLauncher.H"
#include "WxLauncherScript.H"

IMPLEMENT_APP(WxLauncherApp)
//IMPLEMENT_CLASS(WxLauncherApp, ewxApp);




WxLauncherApp::WxLauncherApp()
    : ewxApp()
{
    p_launchFrame = NULL;
    p_msgs = NULL;
}


string WxLauncherApp::getName() const
{
    return LAUNCHER;
}


bool WxLauncherApp::OnInit()
{
    string cntxt = "";

    ewxApp::OnInit();

   if (argc>2 && strcmp(argv[1].ToStdString().c_str(),"-pipe")==0) {
     AuthCache::getCache().pipeIn(argv[2].ToStdString());
   }

    p_launchFrame = new WxLauncher(NULL);
    p_msgs = new LauncherMessages(p_launchFrame->panel(), p_launchFrame);
    p_launchFrame->setMessages(p_msgs);
    EDSIFactory::addAuthEventListener(p_launchFrame->panel());

    //  "-context URL" may follow "-pipe FILE" (tests/apps/qe_walkthrough_test.py).
  for (int a = 1; a + 1 < this->argc; a++) {
    if (strcmp(this->argv[a].ToStdString().c_str(), "-context") == 0) {
      cntxt = this->argv[a + 1];
      break;
    }
  }

    p_launchFrame->panel()->setContext(cntxt);
    p_launchFrame->loadSettings("Launcher");

    // Show the main window
    p_launchFrame->Show();
    SetTopWindow(p_launchFrame);
    p_msgs->registerTopShell(p_launchFrame);

    //  Test hook (WxLauncherScript.H): drive the real frame without a broker.
    const char* script = getenv("ECCE_LAUNCHER_SCRIPT");
    if (script != NULL)
    {
        fprintf(stderr, "[LAUNCHER] binary=%s ECCE_HOME=%s\n",
                (const char*)wxString(this->argv[0]).c_str(), Ecce::ecceHome());
        LauncherScript* run = new LauncherScript(p_launchFrame->panel(), script);
        run->start();
        return true;
    }

    //  Subscribe to Messages
    p_msgs->subscribeMessages();
    p_msgs->startSubscriber();
    p_msgs->notifyReady();

    return true;
}




int WxLauncherApp::OnExit()
{
    // The window owns its message handlers and unsubscribes them itself.
    return 0;
}
