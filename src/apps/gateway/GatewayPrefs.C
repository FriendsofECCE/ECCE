/**
 * @file
 * @author Lisong Sun
 *
 * Gateway Preference Dialog.
 *
 * This dialog is implemented to be independent of the Gateway.
 * If there is the need, this dialog can become a standalone app.
 *
 */

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "GatewayPrefs.H"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "util/JMSMessage.H"
#include "util/JMSPublisher.H"
#include "util/Preferences.H"
#include "util/PreferenceLabels.H"
#include "util/TDateTime.H"
#include "util/UnitFactory.H"
#include "util/Ecce.H"

#include "dsm/MachineConfig.H"
#include "dsm/ResourceDescriptor.H"

#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxMessageDialog.H"

#include "StateButton.H"

#include "GatewayPrefs.H"

#ifdef EMSL
#include "EMSLAuth.H"
#endif

//#include <iostream>
//using std::cout;
//using std::cerr;
//using std::endl;

/**
 * Empty constructor, should never be used.
 */
GatewayPrefs::GatewayPrefs() {}


/**
 * Default constructor.
 *
 * Finalize the GUI by adding entries to the selection widgets and
 * state buttons. Restore settings from saved preference.
 */
GatewayPrefs::GatewayPrefs( wxWindow* parent, wxWindowID id,
                            const wxString& caption, const wxPoint& pos,
                            const wxSize& size, long style ):
  GatewayPrefsGUI(parent, id, caption, pos, size, style)
{
  SetIcon(wxIcon(ewxBitmap::pixmapFile("gateway64.xpm"), wxBITMAP_TYPE_XPM));

  // Necessary for help handler to find right url.
  SetName("Preferences");

  p_publisher = new JMSPublisher("GatewayPrefs");

  for (int i=0; i<TDateTime::NODATE; i++) {
    p_dateFormat->Append(TDateTime::getDateFormatName
                         ((TDateTime::DateFormat)i).c_str());
  }

  for (int j=0; j<=TDateTime::NOTIME; j++) {
    p_timeFormat->Append(TDateTime::getTimeFormatName
                         ((TDateTime::TimeFormat)j).c_str());
  }
  
  vector<string> unitNames = UnitFactory::getInstance().getFullFamilyNames();
  for (size_t k=0; k<unitNames.size(); k++) {
    p_unit->Append(unitNames[k]);
  }

  addState(ResourceDescriptor::STATE_CREATED);
  addState(ResourceDescriptor::STATE_LOADED);
  addState(ResourceDescriptor::STATE_READY);
  addState(ResourceDescriptor::STATE_KILLED);
  addState(ResourceDescriptor::STATE_SUBMITTED);
  addState(ResourceDescriptor::STATE_UNSUCCESSFUL);
  addState(ResourceDescriptor::STATE_RUNNING);
  addState(ResourceDescriptor::STATE_FAILED);
  addState(ResourceDescriptor::STATE_COMPLETED);
  addState(ResourceDescriptor::STATE_SYSTEM_FAILURE);

  int i = ResourceDescriptor::STATE_CREATED;
  for (; i < ResourceDescriptor::NUMBER_OF_STATES; i++) {
    p_stateButton[i][0]->SetSibling(p_stateButton[i][1]);
    p_stateButton[i][1]->SetSibling(p_stateButton[i][0]);
  }

#ifdef EMSL
  if (EMSLAuth::checkHost()) {
    p_savePasswords->Show();
  }
#endif

  restoreSettings();

#ifdef __WXMAC__
  SetSize(size);
#endif

  Fit();
  Show(false);
}


/**
 * Destructor.
 *
 * Save settings, clean memory.
 */
GatewayPrefs::~GatewayPrefs()
{
  saveSettings();
  delete p_publisher;
  p_publisher = NULL;
}


/**
 * Event handling function for close button.
 *
 * Hide the dialog when close button is clicked.
 */
void GatewayPrefs::OnCloseWindow( wxCloseEvent& event )
{
  Show(false);
}


/**
 * Event handling function for close button.
 *
 * Hide the dialog when close button is clicked.
 */
void GatewayPrefs::OnCloseButtonClick( wxCommandEvent& event )
{
  Show(false);
}


/**
 * Event handling function for reset all button.
 *
 * Restore all settings to default values. Depend on which kind of setting
 * is modified, send out different messages to other apps.
 */
void GatewayPrefs::OnResetAllButtonClick( wxCommandEvent& event )
{
  if (!isDefaultGlobalPref()) {
    p_dateFormat->SetSelection(p_dateFormatDefault);
    p_timeFormat->SetSelection(p_timeFormatDefault);
    p_unit->SetSelection(p_unitDefault);
    p_beepError->SetValue(p_beepErrorDefault);
    p_beepWarn->SetValue(p_beepWarnDefault);
    p_focus->SetValue(p_focusDefault);
    OnGlobalPrefChange(event);
  }

  if (!isDefaultGatewayPref()) {
    p_confirmExit->SetValue(p_confirmExitDefault);
    p_showBusy->SetValue(p_showBusyDefault);
    p_alwaysOnTop->SetValue(p_alwaysOnTopDefault);
    p_leftClickNewApp->SetValue(p_leftClickNewAppDefault);
    p_closeShells->SetValue(p_closeShellsDefault);
    p_orientation->SetSelection(p_orientationDefault);
    OnGatewayPrefChange(event);
  }

  if (!isDefaultStatePref()) {
    resetAllStateColors();
    WxState::resetToSystemDefault();
    int i = ResourceDescriptor::STATE_CREATED;
    for (; i < ResourceDescriptor::NUMBER_OF_STATES; i++) {
      p_stateButton[i][0]->SetStatus(StateButton::FLATBUTTON);
      p_stateButton[i][1]->SetStatus(StateButton::RAISENBUTTON);
    }
  }

  Refresh();
}


/**
 * Save settings and send out ecce_preferences_misc message to other apps.
 */
void GatewayPrefs::OnGlobalPrefChange( wxCommandEvent& event )
{
  saveSettings();
  JMSMessage * blankMsg = p_publisher->newMessage();
  p_publisher->publish("ecce_preferences_misc", *blankMsg);
  delete blankMsg;
}


/**
 * Save settings and send out ecce_preferences_gateway message to gateway.
 *
 * This function also provide a popup to show that "Always on Top" and
 * "Orientation" change won't be affective till a restart.
 */
void GatewayPrefs::OnGatewayPrefChange( wxCommandEvent& event )
{
  if (event.GetId() == ID_ALWAYS_ON_TOP ||
      event.GetId() == ID_ORIENTATION) {
    ewxMessageDialog dlg(this,
                         "This change will take effect when you restart ECCE.",
                         "Restart Needed", wxOK|wxICON_INFORMATION,
                         wxDefaultPosition);
    dlg.ShowModal();
  }

#ifdef EMSL
  if (EMSLAuth::checkHost() && !p_savePasswords->GetValue()) {
    // Delete any currently saved passwords
    string prefPath = Ecce::realUserPrefPath();
    string configFile = prefPath + "/EMSLAuth";
    unlink(configFile.c_str());
    string keyFile = prefPath + "/.EMSLAuthKey";
    unlink(keyFile.c_str());
  }
#endif

  saveSettings();
  Target gateway(GATEWAY, "");
  JMSMessage * blankMsg = p_publisher->newMessage(gateway);
  p_publisher->publish("ecce_preferences_gateway", *blankMsg);
  delete blankMsg;
}


/**
 * Send out ecce_preferences_states message to other apps.
 */
void GatewayPrefs::OnStatePrefChange()
{
  JMSMessage * blankMsg = p_publisher->newMessage();
  p_publisher->publish("ecce_preferences_states", *blankMsg);
  delete blankMsg;
}


/**
 * Save all settings to preference files.
 */
void GatewayPrefs::saveSettings()
{
  // Small Hack
  // We assume that the indices returned from getFamilyNames match up 
  // with the indices from getFullFamily Names so that we can convert 
  // the label used in the gui to the actual family name.
  vector<string> unitNames = UnitFactory::getInstance().getFamilyNames();
  string family = unitNames[p_unit->GetSelection()];

  // Save global preferences.
  Preferences eccePref = Preferences(PrefLabels::GLOBALPREFFILE);
  eccePref.setInt(PrefLabels::FONTSIZE, p_fontSize->GetSelection());
  eccePref.setInt(PrefLabels::DATEFORMAT, p_dateFormat->GetSelection());
  eccePref.setInt(PrefLabels::TIMEFORMAT, p_timeFormat->GetSelection());
  eccePref.setString(PrefLabels::UNITFAMILY, family);
  eccePref.setBool(PrefLabels::ERRORBEEP, p_beepError->GetValue());
  eccePref.setBool(PrefLabels::WARNINGBEEP, p_beepWarn->GetValue());
  eccePref.setBool(PrefLabels::FOCUSFOLLOWMOUSE, p_focus->GetValue());
  eccePref.saveFile();

  // Save the color theme preference.
  Preferences colorPref = Preferences(PrefLabels::COLORPREFFILE);
  colorPref.setString(PrefLabels::COLORTHEME,
                      p_colorTheme->GetStringSelection().c_str());
  colorPref.saveFile();

  // Save gateway preferences.
  Preferences gwPref = Preferences(PrefLabels::GATEWAYPREFFILE);
  saveWindowSettings("GatewayPrefs", gwPref, false);
  gwPref.setBool(PrefLabels::CONFIRMEXIT, p_confirmExit->GetValue());
  gwPref.setBool(PrefLabels::SHOWBUSY, p_showBusy->GetValue());
  gwPref.setBool(PrefLabels::ALWAYSONTOP, p_alwaysOnTop->GetValue());
  gwPref.setBool(PrefLabels::LEFTCLICKNEWAPP, p_leftClickNewApp->GetValue());
  gwPref.setBool(PrefLabels::CLOSESHELLS, p_closeShells->GetValue());
  gwPref.setBool(PrefLabels::SAVEPASSWORDS, p_savePasswords->GetValue());
  gwPref.setInt(PrefLabels::ORIENTATION, p_orientation->GetSelection());
  gwPref.saveFile();

  // Set enabled|disabled state of "Reset All" button.
  p_resetAll->Enable(!isDefaultGlobalPref() ||
                     !isDefaultGatewayPref() ||
                     !isDefaultStatePref());
}


/**
 * Restore all settings from preference files.
 */
void GatewayPrefs::restoreSettings()
{
  int intBuf;
  string strBuf;
  bool boolBuf;

  // Restore global preferences
  Preferences eccePref = Preferences(PrefLabels::GLOBALPREFFILE);
  if (!eccePref.getInt(PrefLabels::FONTSIZE, intBuf))
    intBuf = p_fontSizeDefault;
  p_fontSize->SetSelection(intBuf);

  if (!eccePref.getInt(PrefLabels::DATEFORMAT, intBuf))
    intBuf = p_dateFormatDefault;
  p_dateFormat->SetSelection(intBuf);

  if (!eccePref.getInt(PrefLabels::TIMEFORMAT, intBuf))
    intBuf = p_timeFormatDefault;
  p_timeFormat->SetSelection(intBuf);

  if (eccePref.getString(PrefLabels::UNITFAMILY, strBuf)) {
    try {
      UnitFamily& family = UnitFactory::getInstance().getUnitFamily(strBuf);
      p_unit->SetStringSelection(family.getFullName());
    } catch (...) {
      p_unit->SetSelection(p_unitDefault);
    }
  }
  else
    p_unit->SetSelection(p_unitDefault);
  
  if (!eccePref.getBool(PrefLabels::ERRORBEEP, boolBuf))
    boolBuf = p_beepErrorDefault;
  p_beepError->SetValue(boolBuf);

  if (!eccePref.getBool(PrefLabels::WARNINGBEEP, boolBuf))
    boolBuf = p_beepWarnDefault;
  p_beepWarn->SetValue(boolBuf);

  if (!eccePref.getBool(PrefLabels::FOCUSFOLLOWMOUSE, boolBuf))
    boolBuf = p_focusDefault;
  p_focus->SetValue(boolBuf);

  // Restore color theme preference
  Preferences colorPref = Preferences(PrefLabels::COLORPREFFILE);
  if (!colorPref.getString(PrefLabels::COLORTHEME, strBuf))
    strBuf = "Site Default";
  p_colorTheme->SetStringSelection(strBuf);

  // Restore gateway preferences
  Preferences gwPref = Preferences(PrefLabels::GATEWAYPREFFILE);
  restoreWindowSettings("GatewayPrefs", gwPref, false);

  if (!gwPref.getBool(PrefLabels::CONFIRMEXIT, boolBuf))
    boolBuf = p_confirmExitDefault;
  p_confirmExit->SetValue(boolBuf);

  if (!gwPref.getBool(PrefLabels::SHOWBUSY, boolBuf))
    boolBuf = p_showBusyDefault;
  p_showBusy->SetValue(boolBuf);

  if (!gwPref.getBool(PrefLabels::ALWAYSONTOP, boolBuf))
    boolBuf = p_alwaysOnTopDefault;
  p_alwaysOnTop->SetValue(boolBuf);

  if (!gwPref.getBool(PrefLabels::LEFTCLICKNEWAPP, boolBuf))
    boolBuf = p_leftClickNewAppDefault;
  p_leftClickNewApp->SetValue(boolBuf);

  if (!gwPref.getBool(PrefLabels::CLOSESHELLS, boolBuf))
    boolBuf = p_closeShellsDefault;
  p_closeShells->SetValue(boolBuf);

  if (!gwPref.getBool(PrefLabels::SAVEPASSWORDS, boolBuf))
    boolBuf = p_savePasswordsDefault;
  p_savePasswords->SetValue(boolBuf);

  if (!gwPref.getInt(PrefLabels::ORIENTATION, intBuf))
    boolBuf = p_orientationDefault;
  p_orientation->SetSelection(intBuf);

  // Set enable|disable state of "Reset All" button.
  p_resetAll->Enable(!isDefaultGlobalPref() ||
                     !isDefaultGatewayPref() ||
                     !isDefaultStatePref());
}


/**
 * Restore just the save password preference
 */
void GatewayPrefs::restorePasswordPref()
{
  bool boolBuf;

  Preferences gwPref = Preferences(PrefLabels::GATEWAYPREFFILE);

  if (!gwPref.getBool(PrefLabels::SAVEPASSWORDS, boolBuf))
    boolBuf = p_savePasswordsDefault;
  p_savePasswords->SetValue(boolBuf);
}


/**
 * Check if all global preferences are default settings.
 */
bool GatewayPrefs::isDefaultGlobalPref()
{
  if (p_dateFormat->GetSelection() != p_dateFormatDefault ||
      p_timeFormat->GetSelection() != p_timeFormatDefault ||
      p_unit->GetSelection() != p_unitDefault ||
      p_beepError->GetValue() != p_beepErrorDefault ||
      p_beepWarn->GetValue() != p_beepWarnDefault ||
      p_focus->GetValue() != p_focusDefault)
    return false;
  return true;
}


/**
 * Check if all gateway preferences are default settings.
 */
bool GatewayPrefs::isDefaultGatewayPref()
{
  if (p_confirmExit->GetValue() != p_confirmExitDefault ||
      p_showBusy->GetValue() != p_showBusyDefault ||
      p_alwaysOnTop->GetValue() != p_alwaysOnTopDefault ||
      p_leftClickNewApp->GetValue() != p_leftClickNewAppDefault ||
      p_closeShells->GetValue() != p_closeShellsDefault ||
      p_savePasswords->GetValue() != p_savePasswordsDefault ||
      p_orientation->GetSelection() != p_orientationDefault)
    return false;
  return true;
}


/**
 * Check if all state preferences are default settings.
 */
bool GatewayPrefs::isDefaultStatePref()
{
  int i = ResourceDescriptor::STATE_CREATED;
  for (; i < ResourceDescriptor::NUMBER_OF_STATES; i++) {
    if (p_stateButton[i][0]->CanBeReset())
      return false;
  }
  return true;
}


/**
 * Change the statename to use a newcolor.
 */
void GatewayPrefs::changeStateColor(string statename, string newcolor)
{
  Preferences eccePref = Preferences(PrefLabels::GLOBALPREFFILE);
  eccePref.setString(statename, newcolor);
  eccePref.saveFile();
  OnStatePrefChange();
}


/**
 * Reset the statename to use the default color.
 */
void GatewayPrefs::resetStateColor(string statename)
{
  Preferences eccePref = Preferences(PrefLabels::GLOBALPREFFILE, true);
  string defcolor;
  eccePref.getString(statename, defcolor);
  eccePref = Preferences(PrefLabels::GLOBALPREFFILE);
  eccePref.setString(statename, defcolor);
  eccePref.saveFile();
  OnStatePrefChange();
}


/**
 * Reset all the states to use the default colors.
 */
void GatewayPrefs::resetAllStateColors()
{
  Preferences sysPrefs(PrefLabels::GLOBALPREFFILE, true);
  Preferences eccePref = Preferences(PrefLabels::GLOBALPREFFILE);
  string defaultColor;
  string prefString;
    
  int i = ResourceDescriptor::STATE_CREATED;
  for (; i < ResourceDescriptor::NUMBER_OF_STATES; i++) {
    prefString = WxState::getPrefString(i);
    sysPrefs.getString(prefString, defaultColor);
    eccePref.setString(prefString, defaultColor);
  }

  eccePref.saveFile();
  OnStatePrefChange();
}


/**
 * Utility function to add the state button to the dialog.
 *
 * Called in the constructor. The order matters.
 */
void GatewayPrefs::addState(ResourceDescriptor::RUNSTATE state)
{
  ewxStaticText* label;
  
  label = new ewxStaticText(this, wxID_STATIC, WxState::getName(state, true), wxDefaultPosition, wxDefaultSize, 0 );
  p_stateIconSizer->Add(label, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);
  
  p_stateButton[state][0] = new StateButton(state, true, this);
  p_stateIconSizer->Add(p_stateButton[state][0], 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);

  p_stateButton[state][1] = new StateButton(state, false, this);
  p_stateIconSizer->Add(p_stateButton[state][1], 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);
}
