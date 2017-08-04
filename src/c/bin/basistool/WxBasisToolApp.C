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

#include "WxBasisToolApp.H"
#include "WxBasisTool.H"


IMPLEMENT_APP(WxBasisToolApp)

WxBasisToolApp::WxBasisToolApp()
    : ewxApp(), WxJMSMessageDispatch(BASISTOOL, true)
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

    if (argc>2 && strcmp(argv[1],"-pipe")==0) {
      AuthCache::getCache().pipeIn(argv[2]);
    }

    p_basisToolFrame = new WxBasisTool(NULL);
    EDSIFactory::addAuthEventListener(p_basisToolFrame);

    EDSIServerCentral central;

    p_basisToolFrame->openLibraryAccess(central.getDefaultBasisSetLibrary());
    p_basisToolFrame->loadSettings("BasisTool");

    if ((argc == 3) && (strcmp(argv[1], "-context") == 0))
    {
        //  Allows for 'stand-alone' invocation for debug purposes; with calculation
        //      context relative to the user's home directory supplied as an option.

        string homePath = central.getDefaultUserHome().toString();
        string calcName = argv[2];

        EcceURL *url = new EcceURL(homePath + "/" + calcName);

        WxBasisTool::contextChangeType chngType = WxBasisTool::cntxtNewCalculation;
        string chngMesg = "";
        string theory = "";
        p_basisToolFrame->setContext(url, chngType, chngMesg, theory);
    }

    // Show the main window
    p_basisToolFrame->Show();
    SetTopWindow(p_basisToolFrame);
    registerTopShell(p_basisToolFrame);

    //  Subscribe to Messages
    subscribeMessages();
    startSubscriber();
    notifyReady();
    return true;
}


void WxBasisToolApp::subscribeMessages()
{
    //  Desktop Messages; handled by WxBasisToolApp application class
    subscribe("ecce_invoke", (wxJmsCBFunc)&WxBasisToolApp::invokeMCB);
    subscribe("ecce_quit", (wxJmsCBFunc)&WxBasisToolApp::quitMCB);
    subscribe("ecce_identify", (wxJmsCBFunc)&WxBasisToolApp::identifyMCB);

     // Handle return message from a request to start another tool
    subscribe("ecce_invoke_status", (wxJmsCBFunc)&WxBasisToolApp::invokeStatusMCB, false);

    // Global Preference Messages
    subscribe("ecce_preferences_pertab",(wxJmsCBFunc)&WxBasisToolApp::perTabPrefsChangedMCB);
    subscribe("ecce_preferences_misc",(wxJmsCBFunc)&WxBasisToolApp::miscPrefsChangedMCB);
    subscribe("ecce_preferences_states",(wxJmsCBFunc)&WxBasisToolApp::statePrefsChangedMCB);

    // Changes possibly relating to the state of the calc the Basistool
    // is currently working on
    subscribe("ecce_url_renamed",(wxJmsCBFunc)&WxBasisToolApp::projectRenamedMCB);
    subscribe("ecce_url_removed",(wxJmsCBFunc)&WxBasisToolApp::projectDeletedMCB);
    subscribe("ecce_url_basis",(wxJmsCBFunc)&WxBasisToolApp::projectBasisChangedMCB);
    subscribe("ecce_url_code",(wxJmsCBFunc)&WxBasisToolApp::projectCalcChangedMCB);
    subscribe("ecce_url_state",(wxJmsCBFunc)&WxBasisToolApp::projectStatusChangedMCB, false);
    subscribe("ecce_url_subject",(wxJmsCBFunc)&WxBasisToolApp::projectFormulaChangedMCB);
    subscribe("ecce_url_theory",(wxJmsCBFunc)&WxBasisToolApp::projectCalcChangedMCB);
    subscribe("ecce_auth_changed",(wxJmsCBFunc)&WxBasisToolApp::msgAuthMCB);

    // Message indicating that we want to save to a file
    subscribe("ecce_filechooser_url",(wxJmsCBFunc)&WxBasisToolApp::filechooserMCB);
    subscribe("ecce_filechooser_error",(wxJmsCBFunc)&WxBasisToolApp::filechooserErrorMCB);
}


int WxBasisToolApp::OnExit()
{
    //  Unsubscribe from Messaging Service
    unsubscribe();

    return 0;
}


 void WxBasisToolApp::invokeStatusMCB(JMSMessage& msg)
 {
    string status = msg.getProperty("status");

    if (status == "failed")
    {
//   static const char *errMsg =
//                "Unable to start the application.  "
//                "Please use the ECCE Support tool to report this problem.";
//        p_basisToolFrame->showFeedbackMessage(errMsg, WxFeedback::SYSTEM);
    }
}


string WxBasisToolApp::identify()
{
    EcceURL *url = p_basisToolFrame->getContext();
    string idnty = getName();
    string path = EDSIServerCentral().mapURLtoName(*url);

    if (!path.empty())
        idnty += ": " + path;

    return idnty;
}


/**
 *  Send identification to the sender of the message.  For lack of
 *   anything better to use as a unique identifier, use the GBSConfig
 *   name.  This may be empty.  Currently if there is no config or the
 *   config is empty, the unique string will be empty except for the
 *   tool name.
 */
void WxBasisToolApp::identifyMCB(JMSMessage& msg)
{
    JMSMessage* resp = newMessage();
    resp->addProperty("info", identify());
    publish("ecce_identify_reply", *resp);

    delete resp;
}


/**
 *    If a preferences file is passed in the message, then this is the first
 *    time the basis tool is being invoked, so load the preferences first.
 *    Otherwise, just load the calculation.
 */
void WxBasisToolApp::invokeMCB(JMSMessage& msg)
{
    if (msg.containsProperty("calcurl"))
    {
        string urlstr = msg.getProperty("calcurl");
        EcceURL *url = new EcceURL(urlstr);

        p_basisToolFrame->setContext(url, WxBasisTool::cntxtNewCalculation,
                                     "", msg.getProperty("actiontopic"));
        this->setPollContext(urlstr);

        p_basisToolFrame->Iconize(false);
        p_basisToolFrame->Raise();
        p_basisToolFrame->Show(true);
        p_basisToolFrame->SetFocus();
        p_basisToolFrame->RequestUserAttention();
   }
}


void WxBasisToolApp::quitMCB(JMSMessage& msg)
{
   if (msg.getSender().getName() == GATEWAY)
   {
        //  Force the window to close; disallows veto
        (void)p_basisToolFrame->promptContextSave(false);
        p_basisToolFrame->Close(true);
        this->OnExit();
   }
}


/**
 *  Update global pertab preferences for the PerTab.
 */
void WxBasisToolApp::perTabPrefsChangedMCB(JMSMessage& msg)
{
    p_basisToolFrame->refreshPeriodicTable();
}


/**
 *  This message indicates that the user has changed their State
 *  color preferences so we have to have the feedback template read
 *  in the new states and refresh the display.
 */
void WxBasisToolApp::statePrefsChangedMCB(JMSMessage& msg)
{
    p_basisToolFrame->refreshFeedbackPanel();
}


/**
 *    Misc. preferences include error/warning signals of the Feedback
 *    area.  We need to have Feedback re-initialize.
 */
void WxBasisToolApp::miscPrefsChangedMCB(JMSMessage& msg)
{
    p_basisToolFrame->refreshFeedbackPanel();
}

/**
 *    Project was deleted message.  First we have to determine if
 *    the current Basistool calculation was part of that project.  If so,
 *    the interface has to reflect the fact that there is no longer
 *    a calculation.
 */
void WxBasisToolApp::projectDeletedMCB(JMSMessage& msg)
{
    EcceURL *urlProj = new EcceURL(msg.getProperty("url"));
    EcceURL *urlCtxt = p_basisToolFrame->getContext();

    if ((urlCtxt != NULL) && urlCtxt->isChildOrMe(urlProj->toString()))
    {
        //  Really changing fundamental behavior here.  We no longer support
        //  browse mode on the Basis tool, or at least it isn't meaningful.
        //  Instead of entering into browse mode, just close the dialogue.
        //  Force the window to close; disallows veto
        p_basisToolFrame->Close(true);
        EDSIFactory::removePoolResource(*urlProj);

        this->OnExit();
    }

    delete urlProj;
}


/**
 *   A project name has been changed.  This matters because we show
 *   the full path in the feedback area.
 */
void WxBasisToolApp::projectRenamedMCB(JMSMessage& msg)
{
    EcceURL *urlPrev = new EcceURL(msg.getProperty("oldurl"));
    EcceURL *urlCurr = new EcceURL(msg.getProperty("newurl"));

    EcceURL *urlCtxt = p_basisToolFrame->getContext();

    if (urlCtxt->isChildOrMe(urlPrev->toString()))
    {
        EDSIFactory::renamePoolResource(*urlPrev, *urlCurr);
        p_basisToolFrame->updateContextURL(urlPrev, urlCurr);
        setPollContext(urlCtxt->toString());
    }

    delete urlPrev;
    delete urlCurr;
}



/**
 *   The chemical system changed.  When this happens to the current
 *   calculation, we try to make the gbs consistent with the new chem
 *   system by calling ICalcUtils::makeConsistent().  If the
 *   config cannot be corrected, it is reset to empty and the user
 *   must fix it himself.
 *
 *   If the fragment is null or has no molecular formula, then
 *   the config is cleared completely and the basis tool switches into
 *   browse mode.
 */
void WxBasisToolApp::projectFormulaChangedMCB(JMSMessage& msg)
{
    // First check to see if the fragment that was changed is
    // associated with this calculation:
    EcceURL *urlTrgt = new EcceURL(msg.getProperty("url"));
    EcceURL *urlCtxt = p_basisToolFrame->getContext();

//    bool isEqual = ((urlTrgt->toString()).compare(urlCtxt->toString()) == 0);
//    if (isEqual)

    if (*urlTrgt == *urlCtxt)
    {
        EDSIFactory::changePoolResource(*urlTrgt);
        p_basisToolFrame->refreshContextFormula();
    }

    delete urlTrgt;

}

/**
 *    The state of a calculation has changed.  If its the calculation
 *    the Basistool is working on, we have to update the state icon and if
 *    the state affects allowable operations, update the interface
 *    accordingly.  This is not likely but a user might be viewing or
 *    working on the config when the state changes to Submitted or
 *    greater.
 */
void WxBasisToolApp::projectStatusChangedMCB(JMSMessage& msg)
{
    // JMS is set up to automatically filter out messages that we sent,
    // so we know that somebody else sent this message, and we should react
    EcceURL *urlTrgt = new EcceURL(msg.getProperty("url"));

    EcceURL *urlCtxt = p_basisToolFrame->getContext();

    // Check to see if the basis set that was changed is
    // associated with this calculation:
//    if (urlCtxt == urlTrgt)

  //  bool isEqual = ((->toString()).compare(urlCtxt->toString()) == 0);
    if (*urlTrgt == *urlCtxt)
    {
        ResourceDescriptor::RUNSTATE state
                = ResourceUtils::stringToState(msg.getProperty("state"));
        EDSIFactory::changePoolResource(*urlTrgt);
        p_basisToolFrame->refreshContextStatus(state);
    }

    delete urlTrgt;
}


/**
 *  This message is sent when "other" data regarding the calculation
 *   has changed.  "other" means something other than data for which
 *   a specific message exists.  Currently that means NOT basis set and
 *   NOT chemical system.  For the basis set tool, the data of
 *   importance includes:
 *       code, theory
 *
 *   If the code changed, treat it as if the user changed the code
 *   option menu in browse mode.
 *
 *   If the theory changed, the only impact is whether the DFT tabs
 *   and columns are available.
 *
 *   Note that ICalcUtils::makeConsistent is needed since the basis set config
 *   may need to be updated.
 */
void WxBasisToolApp::projectCalcChangedMCB(JMSMessage& msg)
{
    // First check to see if the change is
    // associated with this calculation:
    EcceURL *urlTrgt = new EcceURL(msg.getProperty("url"));
    EcceURL *urlCtxt = p_basisToolFrame->getContext();

    if (*urlCtxt == *urlTrgt)
    {
        EDSIFactory::changePoolResource(*urlTrgt);
        string mesg = "The code and/or theory for this calculation was "
                      "changed by another application.  The basis set "
                      "configuration has changed to maintain consistency.";

        // Print message to notify user of change:
        p_basisToolFrame->setContext(urlTrgt, WxBasisTool::cntxtTheoryChange, mesg, "");
    }

    delete urlTrgt;
}

void WxBasisToolApp::msgAuthMCB(JMSMessage& msg)
{
    AuthCache::getCache().msgIn(msg, getMyID());
}

/**
 *   A basis configuration has changed.  (We know we didnt send this message
 *   because JMS filters them out.)
 *   If somebody changed the same config we are
 *   working on, tell the user that somebody else or some other tool
 *   has changed it.  Probably, should ask the user if they want
 *   to save or toss current work.
 */
void WxBasisToolApp::projectBasisChangedMCB(JMSMessage& msg)
{
//    cout << "WxBasisToolApp::projectBasisChangedMCB(JMSMessage& msg)" << endl;

    // Check to see if the basis set that was changed is
    // associated with this calculation:
    EcceURL *urlTrgt = new EcceURL(msg.getProperty("url"));
//    cout << "\turlTrgt:  " << urlTrgt->toString() << endl;

    EcceURL *urlCtxt = p_basisToolFrame->getContext();
//    cout << "\turlCtxt:  " << urlCtxt ->toString() << endl;

    if (*urlTrgt == *urlCtxt)
    {
        EDSIFactory::changePoolResource(*urlTrgt);
        string mesg = "The molecular basis for this calculation "
                      "has been changed by another user or tool.";
        p_basisToolFrame->setContext(urlTrgt, WxBasisTool::cntxtBasisChange, mesg, "");
    }

    delete urlTrgt;
}


/**
 *   The file chooser dialog wants us to save to the file indicated by url
 */
void WxBasisToolApp::filechooserMCB(JMSMessage& msg)
{
    EcceURL *urlTrgt = new EcceURL(msg.getProperty("url"));
    p_basisToolFrame->saveContext(urlTrgt);

    delete urlTrgt;
}


/**
 *   The file chooser dialog had an error
 */
void WxBasisToolApp::filechooserErrorMCB(JMSMessage& msg)
{
    string message(msg.getProperty("message"));
    p_basisToolFrame->showErrorMessage(message);

}


//void WxBasisToolApp::msgAuthMCB(JMSMessage& mesg)
//{
//    AuthCache& pac = AuthCache::getCache();

    // Force reload of persistent cache
//    pac.setScheme(pac.getScheme());
//}


/**
 *  Responds to msg that says the user has changed preferences for States
 *  or for miscellaneous.
*/
//void WxBasisToolApp::statePrefsChangedMCB(JMSMessage& msg)
//{
//    p_basisToolFrame->updateStatePreferences();
//}
