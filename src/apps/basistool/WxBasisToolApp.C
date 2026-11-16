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

#include "wxgui/WxFeedback.H"
#include "wxgui/ewxWindowUtils.H"

#include "BasisToolMessages.H"
#include "WxBasisToolApp.H"
#include "WxBasisTool.H"


IMPLEMENT_APP(WxBasisToolApp)

WxBasisToolApp::WxBasisToolApp()
    : ewxApp()
{
    p_basisToolFrame = NULL;
}


string WxBasisToolApp::getName() const
{

    return BASISTOOL;
    //return JMS_BASISTOOL;
}


bool WxBasisToolApp::OnInit()
{
    string cntxt = "";

    ewxApp::OnInit();

    if (argc>2 && strcmp(argv[1].ToStdString().c_str(),"-pipe")==0) {
      AuthCache::getCache().pipeIn(argv[2].ToStdString());
    }

    p_basisToolFrame = new WxBasisTool(NULL);
    EDSIFactory::addAuthEventListener(p_basisToolFrame);

    EDSIServerCentral central;

    p_basisToolFrame->openLibraryAccess(central.getDefaultBasisSetLibrary());
    p_basisToolFrame->loadSettings("BasisTool");

    if ((argc == 3) && (strcmp(argv[1].ToStdString().c_str(), "-context") == 0))
    {
        //  Allows for 'stand-alone' invocation for debug purposes; with calculation
        //      context relative to the user's home directory supplied as an option.

        string homePath = central.getDefaultUserHome().toString();
        string calcName = argv[2].ToStdString();

        EcceURL *url = new EcceURL(homePath + "/" + calcName);

        WxBasisTool::contextChangeType chngType = WxBasisTool::cntxtNewCalculation;
        string chngMesg = "";
        string theory = "";
        p_basisToolFrame->setContext(url, chngType, chngMesg, theory);
    }

    // Show the main window
    p_basisToolFrame->Show();
    SetTopWindow(p_basisToolFrame);
    BasisToolMessages *msgs = new BasisToolMessages(p_basisToolFrame);
    p_basisToolFrame->setMessages(msgs);
    msgs->registerTopShell(p_basisToolFrame);
    msgs->subscribeMessages();
    msgs->startSubscriber();
    msgs->notifyReady();
    return true;
}


