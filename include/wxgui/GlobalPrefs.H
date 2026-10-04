/**
 * @file
 *
 * Global preferences dialog (Edit > Preferences in the Organizer).
 *
 * Settings apply as they are changed and are saved at once, to
 * ~/.ECCE/EcceGlobal (and ~/.ECCE/Gateway for the session items). The
 * dialog does not depend on the Gateway window; the Gateway keeps one only
 * for ECCE_GATEWAY_WINDOW=1.
 */

#ifndef _GLOBALPREFS_H_
#define _GLOBALPREFS_H_

#include <string>
#include <vector>
using std::string;
using std::vector;

#include "wx/wx.h"

#include "dsm/ResourceDescriptor.H"
#include "wxgui/ewxFrame.H"

class ewxButton;
class ewxCheckBox;
class ewxChoice;
class ewxNotebook;
class ewxTextCtrl;
class ewxStaticText;
class JMSPublisher;
class StateButton;

class GlobalPrefs: public ewxFrame
{
public:
  GlobalPrefs(wxWindow* parent);
  ~GlobalPrefs();

  void restorePasswordPref();

  // One "command" setting: a choice of well-known programs plus a text
  // field that is editable for "Other".  Stored value is the command;
  // empty means the built-in default.
  struct ProgramRow {
    ewxChoice* choice;
    ewxTextCtrl* text;
    vector<string> presets;   // presets[0] is the default (stored as "")
    const char* prefKey;
  };

  static string programValue(const ProgramRow& row);
  static string envOverrideNote(const char* var, const char* what);

private:
  void createGeneralPage(wxWindow* page);
  void createProgramsPage(wxWindow* page);
  void createStatesPage(wxWindow* page);
  void createGatewayPage(wxWindow* page);
  void createDataPage(wxWindow* page);
  void updateDataPage();
  void OnLocalDataToggle(wxCommandEvent& event);
  void OnChangeDataFolder(wxCommandEvent& event);
  void OnOpenDataFolder(wxCommandEvent& event);
  ProgramRow makeProgramRow(wxWindow* page, wxSizer* sizer,
                            const wxString& label,
                            const vector<string>& labels,
                            const vector<string>& presets,
                            const char* prefKey);
  void setProgramRow(ProgramRow& row, const string& value);

  void OnCloseWindow(wxCloseEvent& event);
  void OnCloseButton(wxCommandEvent& event);
  void OnResetAll(wxCommandEvent& event);
  void OnGlobalChange(wxCommandEvent& event);
  void OnGatewayChange(wxCommandEvent& event);
  void OnProgramChoice(wxCommandEvent& event);
  void OnProgramText(wxCommandEvent& event);
  void OnStatePrefChange();

  void saveSettings();
  void restoreSettings();
  void changeStateColor(string statename, string newcolor);
  void resetStateColor(string statename);
  void resetAllStateColors();

  bool isDefaultGlobalPref();
  bool isDefaultGatewayPref();
  bool isDefaultStatePref();
  bool isDefaultPrograms();
  void updateResetButton();

  void addState(wxWindow* page, ResourceDescriptor::RUNSTATE state);

  JMSPublisher* p_publisher;
  ewxNotebook* p_book;

  ewxChoice* p_fontSize;
  ewxChoice* p_dateFormat;
  ewxChoice* p_timeFormat;
  ewxChoice* p_unit;
  ewxCheckBox* p_systemFont;
  ewxCheckBox* p_beepError;
  ewxCheckBox* p_beepWarn;
  ewxCheckBox* p_focus;
  ewxCheckBox* p_quickTransp;
  ewxCheckBox* p_confirmExit;
  ewxCheckBox* p_closeShells;
  ewxCheckBox* p_savePasswords;

  // Gateway window page; all NULL unless ECCE_GATEWAY_WINDOW is set.
  ewxCheckBox* p_showBusy;
  ewxCheckBox* p_alwaysOnTop;
  ewxCheckBox* p_leftClickNewApp;
  ewxChoice* p_orientation;

  // Data folder page (#216); saved at once, never touched by Reset All.
  ewxCheckBox* p_localData;
  ewxTextCtrl* p_localFolder;
  ewxButton* p_openFolder;
  ewxStaticText* p_localNote;

  ProgramRow p_editor;
  ProgramRow p_terminal;
  ProgramRow p_browser;

  wxGridSizer* p_stateIconSizer;
  ewxButton* p_resetAll;
  StateButton* p_stateButton[ResourceDescriptor::NUMBER_OF_STATES][2];

  bool p_restoring;

  const static int p_fontSizeDefault = 1;  // Medium
  const static int p_dateFormatDefault = 0;
  const static int p_timeFormatDefault = 0;
  // Default units family is DEFAULT_UNIT_FAMILY which is the 3rd one.
  const static int p_unitDefault = 2;
  const static bool p_systemFontDefault = false;
  const static bool p_beepErrorDefault = true;
  const static bool p_beepWarnDefault = true;
  const static bool p_focusDefault = false;
  const static bool p_quickTranspDefault = false;
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
