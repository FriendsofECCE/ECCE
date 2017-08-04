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

#ifndef _GATEWAYPREFS_H_
#define _GATEWAYPREFS_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "GatewayPrefs.C"
#endif


#include "wx/frame.h"

#include "wxgui/WxState.H"

#include "GatewayPrefsGUI.H"


class JMSPublisher;
class Preferences;
class StateButton;

class GatewayPrefs: public GatewayPrefsGUI
{    
public:

  GatewayPrefs( wxWindow* parent,
                wxWindowID id = SYMBOL_GATEWAYPREFSGUI_IDNAME,
                const wxString& caption = SYMBOL_GATEWAYPREFSGUI_TITLE,
                const wxPoint& pos = SYMBOL_GATEWAYPREFSGUI_POSITION,
                const wxSize& size = SYMBOL_GATEWAYPREFSGUI_SIZE,
                long style = SYMBOL_GATEWAYPREFSGUI_STYLE );
  ~GatewayPrefs( );

  void restorePasswordPref();


protected:
  
  GatewayPrefs( );

  void OnCloseWindow( wxCloseEvent& event );
  void OnCloseButtonClick( wxCommandEvent& event );
  void OnResetAllButtonClick( wxCommandEvent& event );
  void OnGlobalPrefChange( wxCommandEvent& event );
  void OnGatewayPrefChange( wxCommandEvent& event );
  void OnStatePrefChange();

  void saveSettings();
  void restoreSettings();
  void changeStateColor(string statename, string newcolor);
  void resetStateColor(string statename);
  void resetAllStateColors();

  bool isDefaultGlobalPref();
  bool isDefaultGatewayPref();
  bool isDefaultStatePref();

  void addState(ResourceDescriptor::RUNSTATE state);

  JMSPublisher * p_publisher;
  Preferences * p_gwPref;
  Preferences * p_eccePref;

  StateButton * p_stateButton[ResourceDescriptor::NUMBER_OF_STATES][2];

  // The default values for all the options.
  const static int p_fontSizeDefault = 1;  // Medium
  const static int p_dateFormatDefault = 0;
  const static int p_timeFormatDefault = 0;
  // Default units family is DEFAULT_UNIT_FAMILY which is the 3rd one.
  const static int p_unitDefault = 2;
  const static bool p_beepErrorDefault = true;
  const static bool p_beepWarnDefault = true;
  const static bool p_focusDefault = false;
  const static bool p_confirmExitDefault = true;
  const static bool p_showBusyDefault = true;
  const static bool p_alwaysOnTopDefault = false;
  const static bool p_leftClickNewAppDefault = false;
  const static bool p_closeShellsDefault = false;
  const static bool p_savePasswordsDefault = false;
  const static int p_orientationDefault = 0;

  friend class StateButton;
};

#endif
// _GATEWAYPREFS_H_
