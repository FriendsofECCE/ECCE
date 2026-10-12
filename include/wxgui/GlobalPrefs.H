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

class GlobalPrefs: public ewxFrame
{
public:
  GlobalPrefs(wxWindow* parent);
  ~GlobalPrefs();

  void restorePasswordPref();

  // Test hooks (the Organizer's ECCE_TEST_ORGANIZER "prefs" command): show the
  // Colours tab, choose a scheme as a click does, say which one is shown.
  void showColoursPage();
  bool chooseColourScheme(const string& scheme);
  string colourSchemeShown() const;
  bool tickAdvanced(const string& key, bool on);   // as a click does

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
  void createColoursPage(wxWindow* page);
  void OnColourScheme(wxCommandEvent& event);
  void createProgramsPage(wxWindow* page);
  void createGatewayPage(wxWindow* page);
  void createDataPage(wxWindow* page);
  void createAdvancedPage(wxWindow* page);
  void OnChooseAdvancedFile(wxCommandEvent& event);
  void updateDataPage();
  void OnLocalDataToggle(wxCommandEvent& event);
  void OnChangeDataFolder(wxCommandEvent& event);
  void OnOpenDataFolder(wxCommandEvent& event);
  void OnChangeServer(wxCommandEvent& event);
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

  void saveSettings();
  void restoreSettings();

  bool isDefaultGlobalPref();
  bool isDefaultAdvanced();
  bool isDefaultGatewayPref();
  bool isDefaultPrograms();
  void updateResetButton();


  JMSPublisher* p_publisher;
  ewxNotebook* p_book;

  ewxChoice* p_fontSize;
  ewxChoice* p_dateFormat;
  ewxChoice* p_timeFormat;
  ewxChoice* p_unit;
  ewxChoice* p_elementColours;
  ewxCheckBox* p_systemFont;
  ewxCheckBox* p_beepError;
  ewxCheckBox* p_beepWarn;
  ewxCheckBox* p_focus;
  ewxCheckBox* p_quickTransp;
  ewxCheckBox* p_ecceQmMoDiagram;
  ewxCheckBox* p_updateCheck;
  ewxCheckBox* p_updatePreviews;
  bool p_updateLocked;
  void updateUpdateControls();
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
  ewxButton* p_changeFolder;
  ewxButton* p_openFolder;
  ewxStaticText* p_localNote;

  // Advanced page (#307): one row per EnvSwitches::all() entry.
  struct AdvancedRow {
    ewxCheckBox* box;
    ewxTextCtrl* file;     // NULL unless the switch takes a file
    ewxButton* choose;
  };
  vector<AdvancedRow> p_advanced;

  ProgramRow p_editor;
  ProgramRow p_terminal;
  ProgramRow p_browser;

  ewxButton* p_resetAll;

  bool p_restoring;

  const static int p_fontSizeDefault = 1;  // Medium
  const static int p_dateFormatDefault = 0;
  const static int p_timeFormatDefault = 0;
  // Default units family is DEFAULT_UNIT_FAMILY which is the 3rd one.
  const static int p_unitDefault = 2;
  const static int p_elementColoursDefault = 0;  // Jmol
  const static bool p_systemFontDefault = false;
  const static bool p_beepErrorDefault = true;
  const static bool p_beepWarnDefault = true;
  const static bool p_focusDefault = false;
  const static bool p_quickTranspDefault = false;
  const static bool p_ecceQmMoDiagramDefault = true;
  const static bool p_updateCheckDefault = true;
  const static bool p_updatePreviewsDefault = false;
  const static bool p_confirmExitDefault = true;
  const static bool p_showBusyDefault = true;
  const static bool p_alwaysOnTopDefault = false;
  const static bool p_leftClickNewAppDefault = false;
  const static bool p_closeShellsDefault = false;
  const static bool p_savePasswordsDefault = false;
  const static int p_orientationDefault = 0;

};

#endif
