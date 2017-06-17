/**
 * @file
 * @author Lisong Sun
 *
 * Gateway Application.
 *
 * This file implements the Ecce Gateway Application.
 *
 */

#include <signal.h>

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "Gateway.H"
#endif

#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/timer.h"

#include "util/BrowserHelp.H"
#include "util/Preferences.H"
#include "util/PreferenceLabels.H"
#include "util/JMSPublisher.H"
#include "util/TempStorage.H"

#include "dsm/EDSIServerCentral.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/ResourceTool.H"

#include "wxgui/EcceTool.H"
#include "wxgui/ewxMenu.H"
#include "wxgui/ewxMessageDialog.H"

#include "Gateway.H"
#include "GatewayApp.H"
#include "GatewayPrefs.H"


IMPLEMENT_CLASS( Gateway, wxFrame )

BEGIN_EVENT_TABLE( Gateway, wxFrame )

  EVT_CLOSE( Gateway::OnCloseWindow )
  EVT_ICONIZE( Gateway::OnIconize )
  EVT_TIMER( wxID_ANY, Gateway::OnTimer )

END_EVENT_TABLE()


/**
 * Empty constructor, should never be used.
 */
Gateway::Gateway() 
{
  p_isLastIconifyHack = false;
}


/**
 * Default constructor.
 *
 * Initialize all the data members.
 * Finalize the GUI.
 * Restore settings from saved preference.
 */
Gateway::Gateway( GatewayApp* app, wxWindow* parent,
                  wxWindowID id, const wxString& caption,
                  const wxPoint& pos, const wxSize& size, long style )
{
  p_isLastIconifyHack = false;
  p_app = app;
  
  int i;
  char fname[12] = "gwmain0.xpm";
  for (i=0; i<10; i++) {
    p_iconmain[i] = ewxBitmap(fname, wxBITMAP_TYPE_XPM);
    fname[6]=fname[6]+1;
  }

  p_iconhelp = ewxBitmap("gwhelp2.xpm", wxBITMAP_TYPE_XPM);
  p_iconfeedback = ewxBitmap("gwfeedback2.xpm", wxBITMAP_TYPE_XPM);
  p_iconprefs = ewxBitmap("gwprefs2.xpm", wxBITMAP_TYPE_XPM);
  p_iconwindows = ewxBitmap("gwwindows2.xpm", wxBITMAP_TYPE_XPM);
  
  p_numActivities = 0;
  p_pixmapIndex = 0;
  p_winMenu = NULL;
  p_prefsDlg = new GatewayPrefs(NULL);
  p_app->registerTopShell(p_prefsDlg);
  p_timer = new wxTimer(this);
  p_targetList.clear();
  p_preference = new Preferences(PrefLabels::GATEWAYPREFFILE);

  Create( parent, id, caption, pos, size, style );

  SetIcon(wxIcon(ewxBitmap::pixmapFile("gateway64.xpm"), wxBITMAP_TYPE_XPM));

  restoreWindowSettings(GATEWAY, *p_preference, false);
}


/**
 * Destructor.
 *
 * Using quit() function to clean up the memory.
 */
Gateway::~Gateway( ) 
{
}


/**
 * Create GUI components.
 *
 * Based on the preference setting, set the "always on top" flag.
 */
bool Gateway::Create( wxWindow* parent, wxWindowID id, const wxString& caption,
                      const wxPoint& pos, const wxSize& size, long style )
{
  bool boolBuf;
  if (!p_preference->getBool(PrefLabels::ALWAYSONTOP, boolBuf))
    boolBuf = false;
  if (boolBuf)
    style = style | wxSTAY_ON_TOP;
  else
    style = style & (~wxSTAY_ON_TOP);

  ewxFrame::Create( parent, id, caption, pos, size, style , "ECCE");
  
  CreateControls();
  return true;
}


/**
 * Create GUI components.
 *
 * Create all the EcceTools and add them to the gateway panel.
 */
void Gateway::CreateControls()
{
  int intBuf;
  if (!p_preference->getInt(PrefLabels::ORIENTATION, intBuf))
    intBuf = 0;

  wxBoxSizer * toolSizer;

  if (intBuf == 0)
    toolSizer = new wxBoxSizer(wxHORIZONTAL);
  else
    toolSizer = new wxBoxSizer(wxVERTICAL);
  
  this->SetSizer(toolSizer);

  p_main = new EcceTool(this, wxID_EXIT, wxT("exit"), p_iconmain[0]);
  toolSizer->Add(p_main, 1, wxGROW|wxALL, 0);
  Connect(wxID_EXIT, wxEVT_COMMAND_BUTTON_CLICKED,
          wxCommandEventHandler(Gateway::toolActivate));

  ResourceDescriptor descriptor = ResourceDescriptor::getResourceDescriptor();

  int i = 0;
  for(; true; i++) {
    ResourceTool * resTool = descriptor.getGatewayTool(i);
    if (resTool == 0) break;
    EcceTool * tool = new EcceTool(this, resTool);
    tool->SetSize(68, 68);
    toolSizer->Add(tool, 1, wxGROW|wxALL, 0);
    Connect(resTool->getId(), wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(Gateway::toolActivate));
  }

  // Calculate proper size of the frame
  if (intBuf == 0)
    SetSize(wxSize(68*(i+2), 68));
  else
    SetSize(wxSize(68, 68*(i+2)));

  wxBoxSizer* buttonSizer = new wxBoxSizer(wxVERTICAL);
  toolSizer->Add(buttonSizer, 1, wxGROW|wxALL, 0);

  p_help = new EcceTool(this, wxID_HELP, wxT("help"), p_iconhelp);
  p_help->SetSize(68, 17);
  buttonSizer->Add(p_help, 1, wxGROW|wxALL, 0);
  Connect(wxID_HELP, wxEVT_COMMAND_BUTTON_CLICKED,
          wxCommandEventHandler(Gateway::toolActivate));

  p_feedback = new EcceTool(this, wxID_FEEDBACK,
                            wxT("feedback"), p_iconfeedback);
  p_feedback->SetSize(68, 17);
  buttonSizer->Add(p_feedback, 1, wxGROW|wxALL, 0);
  Connect(wxID_FEEDBACK, wxEVT_COMMAND_BUTTON_CLICKED,
          wxCommandEventHandler(Gateway::toolActivate));

  p_prefs = new EcceTool(this, wxID_PREFERENCE, wxT("preference"), p_iconprefs);
  p_prefs->SetSize(68, 17);
  buttonSizer->Add(p_prefs, 1, wxGROW|wxALL, 0);
  Connect(wxID_PREFERENCE, wxEVT_COMMAND_BUTTON_CLICKED,
          wxCommandEventHandler(Gateway::toolActivate));

  p_win = new EcceTool(this, wxID_WINLIST, wxT("wins"), p_iconwindows);
  p_win->SetSize(68, 17);
  buttonSizer->Add(p_win, 1, wxGROW|wxALL, 0);
  Connect(wxID_WINLIST, wxEVT_COMMAND_BUTTON_CLICKED,
          wxCommandEventHandler(Gateway::toolActivate));
}


/**
 * Clear target list.
 *
 * p_targetList is used by the windows button to save identify message senders
 */
void Gateway::clearTargetList()
{
  Disconnect(ID_GATEWAY_APPSBASE,
             ID_GATEWAY_APPSBASE + p_targetList.size()-1,
             wxEVT_COMMAND_MENU_SELECTED,
             wxCommandEventHandler(Gateway::OnMenuClick));
  vector<Target *>::iterator targetItor = p_targetList.begin();
  for (; targetItor != p_targetList.end(); targetItor++)
    delete (*targetItor);
  p_targetList.clear();
}


/**
 * Save settings, clean up memory and quit.
 *
 * There are options for byppassing saving settings and disconnecting
 * from the session list.
 */
void Gateway::quit(bool doSave)
{
  // delete the ejs session info file so if jobstore is restarted then it
  // knows not to try to connect to tooltalk when ecce apps aren't running
  string sessName = TempStorage::getTempRootPath();
  sessName += "/.eccejobstore.session";
  (void)unlink(sessName.c_str());
  if (doSave) {
    Preferences pref = Preferences(PrefLabels::GATEWAYPREFFILE);
    saveWindowSettings(GATEWAY, pref, false);
  }
  p_app->notifyExit();

  clearTargetList();
  p_timer->Stop();
  delete p_timer;

  // Close all ecmd processes that have been started in this session based
  // on user preference and delete the preferences file containing these pids
  string pidFile = TempStorage::getTempRootPath();
  pidFile += "/ecmdProcesses";

  bool closeShellsFlag;
  if (!p_preference->getBool(PrefLabels::CLOSESHELLS, closeShellsFlag))
    closeShellsFlag = false;

  if (closeShellsFlag) {
    Preferences *pidPrefs = new Preferences(pidFile, true);
    vector<int> pidList;
    (void)pidPrefs->getIntList("pids", pidList);
    for (int it=0; it < pidList.size(); it++) {
      (void)kill((pid_t)pidList[it], SIGTERM);
    }
  }

  // Always delete the preferences format file that holds the process ids
  (void)unlink(pidFile.c_str());

  p_timer = NULL;
  p_app->unregisterTopShell(p_prefsDlg);
  p_prefsDlg->Destroy();
  p_prefsDlg = NULL;
  delete p_preference;
  p_preference = NULL;

  // Prevent processing of wxIconizeEvent that gets generated during Destroy
  p_isLastIconifyHack = true;

  Destroy();
}


/**
 * Increments the number of current activities.
 *
 * If this is the first one, the busy pixmap will start doing its thing unless
 * of course, the user has turned this functionality off.
 */
void Gateway::startActivity()
{
  p_numActivities++;
  
  bool boolBuf;
  if (!p_preference->getBool(PrefLabels::SHOWBUSY, boolBuf))
    boolBuf = true;

  if (boolBuf) {
    if (p_numActivities == 1) {
      p_timer->Start(50);
    }
  }
  setBusyCursor(true);
}


/**
 * Ends one active activity.
 *
 * If the total number of activities reaches 0, the busy pixmap stops and the
 * master pixmap is restored.
 * Since the user can turn the wave on and off at will, we have to
 * handle the unlikely event of turning it on during after a start
 * was done but before the end is done.  That means we always have
 * to make sure p_timerId is not 0.
 */
void Gateway::endActivity()
{
  p_numActivities--;

  if (p_numActivities == 0) {
    bool boolBuf;
    if (!p_preference->getBool(PrefLabels::SHOWBUSY, boolBuf))
      boolBuf = true;

    if (boolBuf) {
      p_timer->Stop();
      p_pixmapIndex = 0;
      p_main->setBitMap(p_iconmain[p_pixmapIndex]);
    }
  }
  else if (p_numActivities < 0) {
    p_numActivities = 0;
  }
  setBusyCursor(false);
}


/**
 * Provides a mechanism to abort the busy activity cursor.
 *
 * The number of activities is set to 0 and any outstanding timer is canceled.
 * Because the counter is set to 0, the code the gets endActivity must be sure
 * not to decrement past 0.
 */
void Gateway::clearActivities()
{
  p_numActivities = 0;
  if (p_timer->IsRunning()) {
    p_timer->Stop();
    p_pixmapIndex = 0;
    p_main->setBitMap(p_iconmain[p_pixmapIndex]);
  }
  setBusyCursor(false);
}


/**
 * Event handling function for close window.
 */
void Gateway::OnCloseWindow(wxCloseEvent& event)
{
  exitGateway();
}


/**
 * Event handling function for minimize button.
 *
 * It will send out iconify message to all the other apps.
 */
void Gateway::OnIconize(wxIconizeEvent &event)
{
  if (p_isLastIconifyHack) return;

  p_app->sendIconify(IsIconized()?1:0);
  event.Skip();
}


/**
 * Event handling function for timer.
 *
 * It will swap the gateway pixmap icon to make wave effect.
 */
void Gateway::OnTimer(wxTimerEvent& WXUNUSED(event))
{
  p_pixmapIndex = (p_pixmapIndex+1)%10;
  p_main->setBitMap(p_iconmain[p_pixmapIndex]);
}


/**
 * Event handling function for window list menu click.
 */
void Gateway::OnMenuClick(wxCommandEvent& event)
{
  Target * target = p_targetList[event.GetId() - ID_GATEWAY_APPSBASE];
  JMSMessage* msg = p_app->newMessage(*target);
  p_app->publish("ecce_raise", *msg);
  delete msg;
}


/**
 * Set busy cursor for both this frame and globally.
 */
void Gateway::setBusyCursor(bool isBusy)
{
  const wxCursor * cursor = (isBusy?wxHOURGLASS_CURSOR:wxSTANDARD_CURSOR);
  SetCursor(*cursor);
  wxSetCursor(*cursor);
}


/**
 * Activate a tool based on the name.
 *
 * Special buttons are handled differently. Then the EcceTools.
 */
void Gateway::toolActivate(wxCommandEvent &event)
{
  JMSMessage * msg;

  switch(event.GetId()) {
  case wxID_EXIT:
    if (p_numActivities == 0) {
      p_main->setStatus(false);
      exitGateway();
    }
    else
      clearActivities();
    break;
    
  case wxID_HELP:
    BrowserHelp().showPage(BrowserHelp().URL(GATEWAY));
    break;

  case wxID_FEEDBACK:
    BrowserHelp().showFeedbackPage();
    break;

  case wxID_WINLIST:
    if (p_winMenu != NULL)
      delete p_winMenu;
    p_winMenu = new ewxMenu();
    clearTargetList();
    
    //sleep(1);   this solves the unlabeled button problem but is too slow
    msg = p_app->newMessage();
    p_app->publish("ecce_identify", *msg);
    delete msg;
    // Lisong Note: May need some delay on calling this PopupMenu function
    // in case the messaging is slower than the widget generation, some menu
    // entry may be left out.
    PopupMenu(p_winMenu, p_win->GetPosition()+wxPoint(2,16));

    break;

  case wxID_PREFERENCE:
    p_prefsDlg->Show(true);
    p_prefsDlg->Raise();
    break;

  default:
    InvokeParam * param = dynamic_cast<InvokeParam *>(event.GetClientObject());
    if (!param) return;

    string name = param->name;
    int forceNew = param->forceNew;

    bool boolBuf;
    if (!p_preference->getBool(PrefLabels::LEFTCLICKNEWAPP, boolBuf))
      boolBuf = false;
    
    if (boolBuf)
      forceNew = (forceNew+1)%2;

    // If data server connection cannot be 
    // made then return w/o trying to 
    // invoke Organizer application.
    bool foundDataServer = true;
    if (name == "Organizer") {
      try {
        EDSIServerCentral servers;
        if (!servers.checkServer()) {
          foundDataServer = false;
        }
      }
      catch (EcceException ex) {
        foundDataServer = false;
      }
      if (!foundDataServer) {

        ewxMessageDialog dlg(this,
                             "ECCE Organizer startup failed.\n\nConnection to"
                             "ECCE Server could not be established "
                             "(may not be running?)  Contact your ECCE "
                             "administrator if necessary.", 
                             "ECCE Server Failure",
                             wxOK|wxICON_EXCLAMATION, wxDefaultPosition);
        if (dlg.ShowModal() == wxID_OK) {
          return;
        }
      }
    }
  
    Target myself("", p_app->getMyID());
    msg = p_app->newMessage(myself);
    msg->addProperty("appname", name);
    msg->addIntProperty("forcenew", forceNew);

    // special gateway invocation of the Calculation Viewer
    if (name == "wxCalculationViewer") {
      msg->addIntProperty("importcalc", 1);
    }

    p_app->publish("ecce_get_app", *msg);
    delete msg;

    JMSPublisher publisher("MCursor");
    msg = publisher.newMessage();
    msg->addProperty("action","start");
    publisher.publish("ecce_activity",*msg);
    delete msg;
    setBusyCursor(true);
  }
}



/**
 * The standard exit adheres to the confirm exit preference.
 *
 * If the user wants to confirm before exiting, popup a dialog asking whether
 * to continue or cancel.
 */
void Gateway::exitGateway()
{
  bool boolBuf;
  if (!p_preference->getBool(PrefLabels::CONFIRMEXIT, boolBuf))
    boolBuf = true;

  if (boolBuf) {
    Iconize(false);
    Show(true);
    ewxMessageDialog dlg(this, "Do you really want to quit?", "Quit ECCE",
                         wxOK|wxCANCEL|wxICON_QUESTION,
                         wxDefaultPosition);
    if (dlg.ShowModal() == wxID_OK)
      quit(true);
  } else {
    quit(true);
  }
}


/**
 * Add window menu entry for the message sender.
 *
 * It is called when received a identify_reply message.
 * It will place the sender into target list for future needs and add a menu
 * entry for the sender.
 */
void Gateway::addMenuEntry(JMSMessage& msg)
{
  Sender msgSender = msg.getSender();
  Target * msgTarget = new Target(msgSender.getName(),
                                  msgSender.getID());
  p_targetList.push_back(msgTarget);

  int eventId = ID_GATEWAY_APPSBASE + p_targetList.size()-1;
  p_winMenu->Append(eventId, msg.getProperty("info").c_str());

  Connect(eventId, wxEVT_COMMAND_MENU_SELECTED,
          wxCommandEventHandler(Gateway::OnMenuClick));
}


/**
 * Update gateway preference.
 *
 * It is called when Gateway received a ecce_preference_gateway message.
 * It will update the gateway preference using the preference file.
 */
void Gateway::preferenceChanged()
{
  delete p_preference;
  p_preference = new Preferences(PrefLabels::GATEWAYPREFFILE);

  // password saving preference needs to be explicitly synchronized with the
  // WxAuth dialog settings
  p_prefsDlg->restorePasswordPref();

  /*
  // Set show on top style, currently has no effect
  bool boolBuf;
  if (!p_preference->getBool(PrefLabels::ALWAYSONTOP, boolBuf))
    boolBuf = true;
  long style = GetWindowStyleFlag();
  if (boolBuf)
    style = style | wxSTAY_ON_TOP;
  else
    style = style & (~wxSTAY_ON_TOP);

  SetWindowStyle(style);
  Refresh();

  // Refresh by recreate the widgets, resize not working.
  DestroyChildren();
  CreateControls();
  */
}
