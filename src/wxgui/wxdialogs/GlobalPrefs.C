/**
 * @file
 *
 * Global preferences dialog; see wxgui/GlobalPrefs.H.
 */

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif
#include "wx/dirdlg.h"

#include <cstdlib>
#include <algorithm>

#include "util/JMSMessage.H"
#include "util/JMSPublisher.H"
#include "util/Preferences.H"
#include "util/PreferenceLabels.H"
#include "util/TDateTime.H"
#include "util/UnitFactory.H"
#include "util/Ecce.H"
#include "util/LocalData.H"

#include "dsm/ResourceDescriptor.H"

#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxMessageDialog.H"
#include "wxgui/ewxNotebook.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/StateButton.H"
#include "wxgui/WxState.H"

#include "wxgui/GlobalPrefs.H"

namespace {

  // wx3.2/GTK3: Fit() can start a wxEVT_SIZE -> Layout() cascade that never
  // converges, so size events are swallowed for the length of our own
  // Fit()/Layout() and one explicit Layout() follows.
  bool g_suppressSizeEvents = false;

  class SizeEventSuppressor : public wxEventFilter
  {
  public:
    virtual int FilterEvent(wxEvent& event) wxOVERRIDE
    {
      if (g_suppressSizeEvents && event.GetEventType() == wxEVT_SIZE)
        return Event_Processed;
      return Event_Skip;
    }
  };

  SizeEventSuppressor g_sizeEventSuppressor;
  bool g_sizeEventFilterInstalled = false;

  void ensureSizeEventFilterInstalled()
  {
    if (!g_sizeEventFilterInstalled) {
      wxEvtHandler::AddFilter(&g_sizeEventSuppressor);
      g_sizeEventFilterInstalled = true;
    }
  }

  bool gatewayWindowEnabled()
  {
    const char *v = getenv("ECCE_GATEWAY_WINDOW");
    return v != 0 && *v != '\0' && *v != '0';
  }

  string trim(const string& s)
  {
    string::size_type a = s.find_first_not_of(" \t");
    if (a == string::npos) return "";
    return s.substr(a, s.find_last_not_of(" \t") - a + 1);
  }

  const wxWindowID ID_PROGRAM_CHOICE = wxNewId();
  const wxWindowID ID_PROGRAM_TEXT = wxNewId();

  const int PAD = 5;
}


GlobalPrefs::GlobalPrefs(wxWindow* parent)
  : p_publisher(NULL), p_book(NULL),
    p_fontSize(NULL), p_dateFormat(NULL),
    p_timeFormat(NULL), p_unit(NULL), p_systemFont(NULL), p_beepError(NULL), p_beepWarn(NULL),
    p_focus(NULL), p_quickTransp(NULL), p_confirmExit(NULL), p_closeShells(NULL),
    p_savePasswords(NULL), p_showBusy(NULL), p_alwaysOnTop(NULL),
    p_leftClickNewApp(NULL), p_orientation(NULL),
    p_localData(NULL), p_localFolder(NULL), p_openFolder(NULL),
    p_localNote(NULL), p_stateIconSizer(NULL), p_resetAll(NULL), p_restoring(true)
{
  p_editor.choice = p_terminal.choice = p_browser.choice = NULL;
  p_editor.text = p_terminal.text = p_browser.text = NULL;

  ewxFrame::Create(parent, wxID_ANY, _("ECCE Preferences"), wxDefaultPosition,
                   wxDefaultSize,
                   wxCAPTION|wxSYSTEM_MENU|wxMINIMIZE_BOX|wxCLOSE_BOX);

  SetIcon(wxIcon(ewxBitmap::pixmapFile("gateway64.xpm"), wxBITMAP_TYPE_XPM));
  SetName("Preferences");

  p_publisher = new JMSPublisher("GlobalPrefs");

  wxBoxSizer* top = new wxBoxSizer(wxVERTICAL);
  p_book = new ewxNotebook(this, wxID_ANY);

  wxPanel* general = new wxPanel(p_book);
  createGeneralPage(general);
  p_book->AddPage(general, _("General"));

  wxPanel* programs = new wxPanel(p_book);
  createProgramsPage(programs);
  p_book->AddPage(programs, _("External programs"));

  wxPanel* data = new wxPanel(p_book);
  createDataPage(data);
  p_book->AddPage(data, _("Data folder"));

  wxPanel* states = new wxPanel(p_book);
  createStatesPage(states);
  p_book->AddPage(states, _("Run state icons"));

  if (gatewayWindowEnabled()) {
    wxPanel* gw = new wxPanel(p_book);
    createGatewayPage(gw);
    p_book->AddPage(gw, _("Gateway window"));
  }

  top->Add(p_book, 1, wxGROW|wxALL, PAD);

  wxBoxSizer* buttons = new wxBoxSizer(wxHORIZONTAL);
  buttons->AddStretchSpacer(1);
  ewxButton* close = new ewxButton(this, wxID_CLOSE, _("Close"));
  buttons->Add(close, 0, wxALL, PAD);
  buttons->AddStretchSpacer(1);
  p_resetAll = new ewxButton(this, wxID_RESET, _("Reset All"));
  buttons->Add(p_resetAll, 0, wxALL, PAD);
  buttons->AddStretchSpacer(1);
  top->Add(buttons, 0, wxGROW);

  Bind(wxEVT_CLOSE_WINDOW, &GlobalPrefs::OnCloseWindow, this);
  close->Bind(wxEVT_BUTTON, &GlobalPrefs::OnCloseButton, this);
  p_resetAll->Bind(wxEVT_BUTTON, &GlobalPrefs::OnResetAll, this);

  // Attached only once the tree is complete, so nothing lays out a
  // half-built sizer during construction.
  SetSizer(top);

  restoreSettings();
  p_restoring = false;
  updateResetButton();

  ensureSizeEventFilterInstalled();
  g_suppressSizeEvents = true;
  top->Fit(this);
  Layout();
  g_suppressSizeEvents = false;
  top->SetSizeHints(this);
  Centre();

  Show(false);
}


GlobalPrefs::~GlobalPrefs()
{
  saveSettings();
  delete p_publisher;
  p_publisher = NULL;
}


void GlobalPrefs::createGeneralPage(wxWindow* page)
{
  wxBoxSizer* outer = new wxBoxSizer(wxVERTICAL);

  wxStaticBoxSizer* box = new wxStaticBoxSizer(wxVERTICAL, page, _("Appearance and units"));
  wxFlexGridSizer* grid = new wxFlexGridSizer(2, PAD, PAD);
  box->Add(grid, 0, wxGROW|wxALL, PAD);

  wxWindow* sb = box->GetStaticBox();
  int c;
  // No colour theme: windows take the desktop's GTK theme (#210).
  ewxChoice** choices[] = {&p_fontSize, &p_dateFormat,
                           &p_timeFormat, &p_unit};
  const wxChar* labels[] = {_("Font Size:"),
                            _("Date Format:"), _("Time Format:"), _("Units:")};
  for (c = 0; c < 4; c++) {
    grid->Add(new ewxStaticText(sb, wxID_ANY, labels[c]), 0,
              wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
    *choices[c] = new ewxChoice(sb, wxID_ANY);
    grid->Add(*choices[c], 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
    (*choices[c])->Bind(wxEVT_CHOICE, &GlobalPrefs::OnGlobalChange, this);
  }

  p_fontSize->Append(_("Small"));
  p_fontSize->Append(_("Medium"));
  p_fontSize->Append(_("Large"));
  p_fontSize->Append(_("Extra Large"));

  for (int i = 0; i < TDateTime::NODATE; i++) {
    p_dateFormat->Append(TDateTime::getDateFormatName
                         ((TDateTime::DateFormat)i).c_str());
  }
  for (int j = 0; j <= TDateTime::NOTIME; j++) {
    p_timeFormat->Append(TDateTime::getTimeFormatName
                         ((TDateTime::TimeFormat)j).c_str());
  }
  vector<string> unitNames = UnitFactory::getInstance().getFullFamilyNames();
  for (size_t k = 0; k < unitNames.size(); k++) {
    p_unit->Append(unitNames[k]);
  }
  // ECCE's windows are dense, so they use 10 pt unless the user asks for
  // the desktop's size.  Fonts are set when a window is created.
  p_systemFont = new ewxCheckBox(sb, wxID_ANY,
      _("Use the system font size (takes effect when applications are next started)"),
      wxDefaultPosition, wxDefaultSize, wxCHK_2STATE);
  p_systemFont->Bind(wxEVT_CHECKBOX, &GlobalPrefs::OnGlobalChange, this);
  box->Add(p_systemFont, 0, wxLEFT|wxRIGHT|wxBOTTOM, PAD);
  outer->Add(box, 0, wxGROW|wxALL, PAD);

  wxStaticBoxSizer* box2 = new wxStaticBoxSizer(wxVERTICAL, page, _("Behavior"));
  wxWindow* sb2 = box2->GetStaticBox();
  ewxCheckBox** checks[] = {&p_beepError, &p_beepWarn, &p_focus,
                            &p_confirmExit, &p_closeShells, &p_savePasswords};
  const wxChar* clabels[] = {
    _("Beep on errors"), _("Beep on warnings"),
    _("Focus follows mouse over input fields"),
    _("Ask for confirmation on exit"),
    _("Close remote shells on exit"),
    _("Save data and compute server passwords between sessions")};
  for (c = 0; c < 6; c++) {
    *checks[c] = new ewxCheckBox(sb2, wxID_ANY, clabels[c],
                                 wxDefaultPosition, wxDefaultSize, wxCHK_2STATE);
    box2->Add(*checks[c], 0, wxALL, 3);
    // Exit and password items belong to the session, not to every app.
    (*checks[c])->Bind(wxEVT_CHECKBOX,
                       c < 3 ? &GlobalPrefs::OnGlobalChange
                             : &GlobalPrefs::OnGatewayChange, this);
  }
  outer->Add(box2, 0, wxGROW|wxALL, PAD);

  wxStaticBoxSizer* box3 = new wxStaticBoxSizer(wxVERTICAL, page, _("3D viewer"));
  p_quickTransp = new ewxCheckBox(box3->GetStaticBox(), wxID_ANY,
      _("Quick transparency (faster, less accurate where orbital lobes overlap)"),
      wxDefaultPosition, wxDefaultSize, wxCHK_2STATE);
  p_quickTransp->SetToolTip(_("Applies the next time an orbital or isosurface "
      "is shown in the Builder. Large scenes switch to this automatically."));
  p_quickTransp->Bind(wxEVT_CHECKBOX, &GlobalPrefs::OnGlobalChange, this);
  box3->Add(p_quickTransp, 0, wxALL, 3);
  box3->Add(new ewxStaticText(box3->GetStaticBox(), wxID_ANY,
      _("Applies the next time an orbital or isosurface is shown in the Builder.")),
      0, wxLEFT|wxBOTTOM, PAD);
  outer->Add(box3, 0, wxGROW|wxALL, PAD);

  page->SetSizer(outer);
}


GlobalPrefs::ProgramRow GlobalPrefs::makeProgramRow(wxWindow* page,
    wxSizer* sizer, const wxString& label, const vector<string>& labels,
    const vector<string>& presets, const char* prefKey)
{
  ProgramRow row;
  row.presets = presets;
  row.prefKey = prefKey;

  sizer->Add(new ewxStaticText(page, wxID_ANY, label), 0,
             wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
  wxBoxSizer* line = new wxBoxSizer(wxHORIZONTAL);
  row.choice = new ewxChoice(page, ID_PROGRAM_CHOICE);
  for (size_t i = 0; i < labels.size(); i++) row.choice->Append(labels[i]);
  row.text = new ewxTextCtrl(page, ID_PROGRAM_TEXT, wxEmptyString,
                             wxDefaultPosition, wxSize(220, -1));
  line->Add(row.choice, 0, wxALIGN_CENTER_VERTICAL);
  line->Add(row.text, 1, wxLEFT|wxALIGN_CENTER_VERTICAL, PAD);
  sizer->Add(line, 1, wxGROW);

  row.choice->Bind(wxEVT_CHOICE, &GlobalPrefs::OnProgramChoice, this);
  row.text->Bind(wxEVT_TEXT, &GlobalPrefs::OnProgramText, this);
  return row;
}


void GlobalPrefs::createProgramsPage(wxWindow* page)
{
  wxBoxSizer* outer = new wxBoxSizer(wxVERTICAL);
  wxStaticBoxSizer* box = new wxStaticBoxSizer(wxVERTICAL, page, _("External programs"));
  wxWindow* sb = box->GetStaticBox();
  wxFlexGridSizer* grid = new wxFlexGridSizer(2, PAD, PAD);
  grid->AddGrowableCol(1);
  box->Add(grid, 0, wxGROW|wxALL, PAD);

  const char* edPresets[] = {"", "vi", "vim", "nano", "emacs", "gedit",
                             "geany", "kate"};
  vector<string> edVals(edPresets, edPresets + 8);
  vector<string> edLabels;
  edLabels.push_back("Default (vi)");
  for (int i = 1; i < 8; i++) edLabels.push_back(edPresets[i]);
  edLabels.push_back("Other");
  p_editor = makeProgramRow(sb, grid, _("Editor:"), edLabels, edVals,
                            PrefLabels::EDITOR);
  p_editor.choice->SetToolTip(_("Default uses VISUAL or EDITOR when set, otherwise vi"));

  const char* termPresets[] = {"", "xterm", "x-terminal-emulator", "konsole"};
  vector<string> termVals(termPresets, termPresets + 4);
  vector<string> termLabels;
  termLabels.push_back("Default (xterm)");
  for (int i = 1; i < 4; i++) termLabels.push_back(termPresets[i]);
  termLabels.push_back("Other");
  p_terminal = makeProgramRow(sb, grid, _("Terminal:"), termLabels, termVals,
                              PrefLabels::TERMINAL);
  p_terminal.choice->SetToolTip(_("Used only to run terminal editors such as vi, started as: terminal -e editor file. Open Shell and Tail on a remote machine use it too."));

  const char* brPresets[] = {"", "firefox", "firefox-esr", "chromium"};
  vector<string> brVals(brPresets, brPresets + 4);
  vector<string> brLabels;
  brLabels.push_back("Automatic");
  for (int i = 1; i < 4; i++) brLabels.push_back(brPresets[i]);
  brLabels.push_back("Other");
  p_browser = makeProgramRow(sb, grid, _("Web browser:"), brLabels, brVals,
                             PrefLabels::BROWSER);
  p_browser.choice->SetToolTip(_("Used for Help. Automatic uses xdg-open, x-www-browser or sensible-browser, whichever is found first"));

  outer->Add(box, 0, wxGROW|wxALL, PAD);

  outer->Add(new ewxStaticText(page, wxID_ANY,
      _("Default editor: VISUAL or EDITOR if set, otherwise vi.\n"
        "Terminal editors (vi, vim, nano, emacs -nw) run in this terminal;\n"
        "Open Shell and Tail use this terminal on a remote machine, xterm locally.\n"
        "Choose Other to type a command; it may include arguments.")),
      0, wxLEFT|wxRIGHT|wxBOTTOM, PAD*2);

  // Environment variables win over the settings above.
  string note = envOverrideNote("ECCE_EDITOR", "the editor");
  string b = envOverrideNote("ECCE_BROWSER", "the browser");
  if (!note.empty() && !b.empty()) note += "\n";
  note += b;
  if (!note.empty()) {
    ewxStaticText* label = new ewxStaticText(page, wxID_ANY, note);
    outer->Add(label, 0, wxLEFT|wxRIGHT|wxBOTTOM, PAD*2);
  }

  page->SetSizer(outer);
}


// One line for a variable that is set and so overrides its preference.
string GlobalPrefs::envOverrideNote(const char* var, const char* what)
{
  const char* v = getenv(var);
  if (v == NULL || *v == '\0') return "";
  return string(var) + "=" + v + " is set and overrides " + what +
         " setting.";
}


void GlobalPrefs::createDataPage(wxWindow* page)
{
  wxBoxSizer* outer = new wxBoxSizer(wxVERTICAL);
  wxStaticBoxSizer* box = new wxStaticBoxSizer(wxVERTICAL, page,
                                               _("Where calculations are kept"));
  wxWindow* sb = box->GetStaticBox();
  p_localData = new ewxCheckBox(sb, wxID_ANY,
      _("Keep calculations in a folder on this computer, not on a data server"));
  box->Add(p_localData, 0, wxALL, PAD);

  wxBoxSizer* row = new wxBoxSizer(wxHORIZONTAL);
  row->Add(new ewxStaticText(sb, wxID_ANY, _("Folder:")), 0,
           wxALIGN_CENTER_VERTICAL|wxRIGHT, PAD);
  p_localFolder = new ewxTextCtrl(sb, wxID_ANY, wxEmptyString,
                                  wxDefaultPosition, wxSize(320, -1),
                                  wxTE_READONLY);
  row->Add(p_localFolder, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT, PAD);
  ewxButton* change = new ewxButton(sb, wxID_ANY, _("Change..."));
  row->Add(change, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, PAD);
  p_openFolder = new ewxButton(sb, wxID_ANY, _("Open data folder"));
  row->Add(p_openFolder, 0, wxALIGN_CENTER_VERTICAL);
  box->Add(row, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, PAD);
  outer->Add(box, 0, wxGROW|wxALL, PAD);

  p_localNote = new ewxStaticText(page, wxID_ANY, wxEmptyString);
  outer->Add(p_localNote, 0, wxLEFT|wxRIGHT|wxBOTTOM, PAD*2);
  page->SetSizer(outer);

  p_localData->Bind(wxEVT_CHECKBOX, &GlobalPrefs::OnLocalDataToggle, this);
  change->Bind(wxEVT_BUTTON, &GlobalPrefs::OnChangeDataFolder, this);
  p_openFolder->Bind(wxEVT_BUTTON, &GlobalPrefs::OnOpenDataFolder, this);
  updateDataPage();
}


// The page shows the preference, which is what the NEXT start uses; the
// running session keeps the mode it started in.
void GlobalPrefs::updateDataPage()
{
  bool on = LocalData::prefEnabled();
  string folder = LocalData::prefFolder();
  string moveTo = LocalData::prefMoveTo();
  p_localData->SetValue(on);
  p_localFolder->SetValue(folder);
  p_openFolder->Enable(wxDirExists(folder));

  string note =
      "The folder holds your projects and calculations. A change takes\n"
      "effect the next time ECCE starts. Switching between a data server\n"
      "and a folder copies nothing: the calculations of the other mode stay\n"
      "where they are, and the new mode starts empty.";
  if (!moveTo.empty())
    note += "\n\nAt the next start ECCE will offer to move your calculations\n"
            "from " + folder + " to " + moveTo + ".";
  string now = LocalData::dir();
  note += "\n\nThis session keeps its calculations " +
          (now.empty() ? string("on a data server.") : "in " + now + ".");
  const char* env = getenv("ECCE_LOCAL_DATA");
  if (env && *env && !getenv("ECCE_LOCAL_DATA_FROM_PREF"))
    note += "\n" + envOverrideNote("ECCE_LOCAL_DATA", "the data folder");
  p_localNote->SetLabel(note);
  Layout();
}


void GlobalPrefs::OnLocalDataToggle(wxCommandEvent& event)
{
  bool on = p_localData->GetValue();
  // Turning the folder off also drops a move that was waiting for it.
  LocalData::setPref(on, LocalData::prefFolder(),
                     on ? LocalData::prefMoveTo() : "");
  string msg = on ?
      "From the next start, ECCE keeps your calculations in\n    " +
      LocalData::prefFolder() + "\n\nCalculations on the data server stay "
      "there; the folder starts empty." :
      string("From the next start, ECCE keeps your calculations on a data "
             "server.\n\nCalculations in the folder stay there; they are "
             "not copied to the server.");
  ewxMessageDialog dlg(this, msg, "ECCE data folder", wxOK|wxICON_INFORMATION,
                       wxDefaultPosition);
  dlg.ShowModal();
  updateDataPage();
}


void GlobalPrefs::OnChangeDataFolder(wxCommandEvent& event)
{
  string folder = LocalData::prefFolder();
  wxDirDialog dlg(this, _("Folder for ECCE calculations"), folder,
                  wxDD_DEFAULT_STYLE);
  if (dlg.ShowModal() != wxID_OK) return;
  string chosen = dlg.GetPath().ToStdString();
  if (chosen.empty()) return;
  // A folder already holding calculations is moved at the next start,
  // after asking; one that holds nothing yet is simply replaced.
  if (chosen == folder)
    LocalData::setPref(LocalData::prefEnabled(), folder, "");
  else if (LocalData::isEmptyOrMissing(folder))
    LocalData::setPref(LocalData::prefEnabled(), chosen, "");
  else
    LocalData::setPref(LocalData::prefEnabled(), folder, chosen);
  updateDataPage();
}


void GlobalPrefs::OnOpenDataFolder(wxCommandEvent& event)
{
  string folder = LocalData::prefFolder();
  const char* argv[] = { "xdg-open", folder.c_str(), NULL };
  wxExecute(const_cast<char**>(argv), wxEXEC_ASYNC);
}


void GlobalPrefs::createStatesPage(wxWindow* page)
{
  wxBoxSizer* outer = new wxBoxSizer(wxVERTICAL);
  p_stateIconSizer = new wxGridSizer(6, PAD, PAD);
  for (int i = 0; i < 2; i++) {
    p_stateIconSizer->Add(new ewxStaticText(page, wxID_ANY, _("State")), 0,
                          wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL);
    p_stateIconSizer->Add(new ewxStaticText(page, wxID_ANY, _("Default")), 0,
                          wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL);
    p_stateIconSizer->Add(new ewxStaticText(page, wxID_ANY, _("Custom")), 0,
                          wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL);
  }

  // The order matters: the grid fills row by row, two states per row.
  addState(page, ResourceDescriptor::STATE_CREATED);
  addState(page, ResourceDescriptor::STATE_LOADED);
  addState(page, ResourceDescriptor::STATE_READY);
  addState(page, ResourceDescriptor::STATE_KILLED);
  addState(page, ResourceDescriptor::STATE_SUBMITTED);
  addState(page, ResourceDescriptor::STATE_UNSUCCESSFUL);
  addState(page, ResourceDescriptor::STATE_RUNNING);
  addState(page, ResourceDescriptor::STATE_FAILED);
  addState(page, ResourceDescriptor::STATE_COMPLETED);
  addState(page, ResourceDescriptor::STATE_SYSTEM_FAILURE);

  for (int i = ResourceDescriptor::STATE_CREATED;
       i < ResourceDescriptor::NUMBER_OF_STATES; i++) {
    p_stateButton[i][0]->SetSibling(p_stateButton[i][1]);
    p_stateButton[i][1]->SetSibling(p_stateButton[i][0]);
  }

  outer->Add(p_stateIconSizer, 1, wxGROW|wxALL, PAD*2);
  page->SetSizer(outer);
}


void GlobalPrefs::createGatewayPage(wxWindow* page)
{
  wxBoxSizer* outer = new wxBoxSizer(wxVERTICAL);
  p_showBusy = new ewxCheckBox(page, wxID_ANY, _("Show system busy icon"),
                               wxDefaultPosition, wxDefaultSize, wxCHK_2STATE);
  p_alwaysOnTop = new ewxCheckBox(page, wxID_ANY, _("Always on top"),
                               wxDefaultPosition, wxDefaultSize, wxCHK_2STATE);
  p_alwaysOnTop->SetToolTip(_("Your window manager may override this setting"));
  p_leftClickNewApp = new ewxCheckBox(page, wxID_ANY,
                               _("Left click invokes new tool"),
                               wxDefaultPosition, wxDefaultSize, wxCHK_2STATE);
  p_orientation = new ewxChoice(page, wxID_ANY);
  p_orientation->Append(_("Horizontal Orientation"));
  p_orientation->Append(_("Vertical Orientation"));

  ewxCheckBox* checks[] = {p_showBusy, p_alwaysOnTop, p_leftClickNewApp};
  for (int i = 0; i < 3; i++) {
    outer->Add(checks[i], 0, wxALL, 3);
    checks[i]->Bind(wxEVT_CHECKBOX, &GlobalPrefs::OnGatewayChange, this);
  }
  outer->Add(p_orientation, 0, wxALL, 3);
  p_orientation->Bind(wxEVT_CHOICE, &GlobalPrefs::OnGatewayChange, this);
  page->SetSizer(outer);
}


void GlobalPrefs::addState(wxWindow* page, ResourceDescriptor::RUNSTATE state)
{
  p_stateIconSizer->Add(
      new ewxStaticText(page, wxID_STATIC, WxState::getName(state, true)), 0,
      wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL);

  p_stateButton[state][0] = new StateButton(state, true, this, page);
  p_stateIconSizer->Add(p_stateButton[state][0], 0,
                        wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL);

  p_stateButton[state][1] = new StateButton(state, false, this, page);
  p_stateIconSizer->Add(p_stateButton[state][1], 0,
                        wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL);
}


// The stored command for a row: "" for the default entry, the preset's own
// name, or the typed text for Other.
string GlobalPrefs::programValue(const ProgramRow& row)
{
  int sel = row.choice->GetSelection();
  if (sel < 0) return "";
  if (sel < (int)row.presets.size()) return row.presets[sel];
  return trim(row.text->GetValue().ToStdString());
}


void GlobalPrefs::setProgramRow(ProgramRow& row, const string& value)
{
  size_t found = row.presets.size();   // Other
  for (size_t i = 0; i < row.presets.size(); i++) {
    if (row.presets[i] == value) { found = i; break; }
  }
  row.choice->SetSelection((int)found);
  row.text->ChangeValue(found == row.presets.size() ? value : string(row.presets[found]));
  row.text->Enable(found == row.presets.size());
  row.text->setCustomDisabledStyle(found == row.presets.size());
}


void GlobalPrefs::OnProgramChoice(wxCommandEvent& event)
{
  ProgramRow* rows[] = {&p_editor, &p_terminal, &p_browser};
  for (int i = 0; i < 3; i++) {
    ProgramRow& row = *rows[i];
    if (event.GetEventObject() != row.choice) continue;
    int sel = row.choice->GetSelection();
    bool other = sel >= (int)row.presets.size();
    row.text->Enable(other);
    row.text->setCustomDisabledStyle(other);
    if (!other) row.text->ChangeValue(row.presets[sel]);
    else row.text->SetFocus();
  }
  saveSettings();
}


void GlobalPrefs::OnProgramText(wxCommandEvent& event)
{
  if (!p_restoring) saveSettings();
}


void GlobalPrefs::OnCloseWindow(wxCloseEvent& event)
{
  Show(false);
}


void GlobalPrefs::OnCloseButton(wxCommandEvent& event)
{
  Show(false);
}


void GlobalPrefs::OnResetAll(wxCommandEvent& event)
{
  p_restoring = true;
  if (!isDefaultGlobalPref()) {
    p_fontSize->SetSelection(p_fontSizeDefault);
    p_systemFont->SetValue(p_systemFontDefault);
    p_dateFormat->SetSelection(p_dateFormatDefault);
    p_timeFormat->SetSelection(p_timeFormatDefault);
    p_unit->SetSelection(p_unitDefault);
    p_beepError->SetValue(p_beepErrorDefault);
    p_beepWarn->SetValue(p_beepWarnDefault);
    p_focus->SetValue(p_focusDefault);
    p_quickTransp->SetValue(p_quickTranspDefault);
    OnGlobalChange(event);
  }

  if (!isDefaultGatewayPref()) {
    p_confirmExit->SetValue(p_confirmExitDefault);
    p_closeShells->SetValue(p_closeShellsDefault);
    p_savePasswords->SetValue(p_savePasswordsDefault);
    if (p_showBusy != NULL) {
      p_showBusy->SetValue(p_showBusyDefault);
      p_alwaysOnTop->SetValue(p_alwaysOnTopDefault);
      p_leftClickNewApp->SetValue(p_leftClickNewAppDefault);
      p_orientation->SetSelection(p_orientationDefault);
    }
    OnGatewayChange(event);
  }

  if (!isDefaultPrograms()) {
    setProgramRow(p_editor, "");
    setProgramRow(p_terminal, "");
    setProgramRow(p_browser, "");
  }

  if (!isDefaultStatePref()) {
    resetAllStateColors();
    WxState::resetToSystemDefault();
    for (int i = ResourceDescriptor::STATE_CREATED;
         i < ResourceDescriptor::NUMBER_OF_STATES; i++) {
      p_stateButton[i][0]->SetStatus(StateButton::FLATBUTTON);
      p_stateButton[i][1]->SetStatus(StateButton::RAISENBUTTON);
    }
  }
  p_restoring = false;
  saveSettings();
  Refresh();
}


// Tell the other apps of this session that a global setting changed.
void GlobalPrefs::OnGlobalChange(wxCommandEvent& event)
{
  saveSettings();
  JMSMessage * blankMsg = p_publisher->newMessage();
  p_publisher->publish("ecce_preferences_misc", *blankMsg);
  delete blankMsg;
}


// Session settings are read by the Gateway process, which is told here.
void GlobalPrefs::OnGatewayChange(wxCommandEvent& event)
{
  if (p_orientation != NULL && event.GetEventObject() == p_orientation) {
    ewxMessageDialog dlg(this,
                         "This change will take effect when you restart ECCE.",
                         "Restart Needed", wxOK|wxICON_INFORMATION,
                         wxDefaultPosition);
    dlg.ShowModal();
  } else if (p_alwaysOnTop != NULL &&
             event.GetEventObject() == p_alwaysOnTop) {
    ewxMessageDialog dlg(this,
                         "This change will take effect when you restart ECCE.",
                         "Restart Needed", wxOK|wxICON_INFORMATION,
                         wxDefaultPosition);
    dlg.ShowModal();
  }

#ifdef EMSL
  if (EMSLAuth::checkHost() && !p_savePasswords->GetValue()) {
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


void GlobalPrefs::OnStatePrefChange()
{
  JMSMessage * blankMsg = p_publisher->newMessage();
  p_publisher->publish("ecce_preferences_states", *blankMsg);
  delete blankMsg;
}


void GlobalPrefs::saveSettings()
{
  if (p_restoring) return;

  vector<string> unitNames = UnitFactory::getInstance().getFamilyNames();
  int unitSel = p_unit->GetSelection();
  string family = unitNames[unitSel < 0 ? p_unitDefault : unitSel];

  Preferences eccePref = Preferences(PrefLabels::GLOBALPREFFILE);
  eccePref.setInt(PrefLabels::FONTSIZE, p_fontSize->GetSelection());
  eccePref.setBool(PrefLabels::USESYSTEMFONT, p_systemFont->GetValue());
  eccePref.setInt(PrefLabels::DATEFORMAT, p_dateFormat->GetSelection());
  eccePref.setInt(PrefLabels::TIMEFORMAT, p_timeFormat->GetSelection());
  eccePref.setString(PrefLabels::UNITFAMILY, family);
  eccePref.setBool(PrefLabels::ERRORBEEP, p_beepError->GetValue());
  eccePref.setBool(PrefLabels::WARNINGBEEP, p_beepWarn->GetValue());
  eccePref.setBool(PrefLabels::FOCUSFOLLOWMOUSE, p_focus->GetValue());
  eccePref.setBool(PrefLabels::QUICKTRANSPARENCY, p_quickTransp->GetValue());
  eccePref.setString(PrefLabels::EDITOR, programValue(p_editor));
  eccePref.setString(PrefLabels::TERMINAL, programValue(p_terminal));
  eccePref.setString(PrefLabels::BROWSER, programValue(p_browser));
  eccePref.saveFile();

  Preferences gwPref = Preferences(PrefLabels::GATEWAYPREFFILE);
  saveWindowSettings("GatewayPrefs", gwPref, false);
  gwPref.setBool(PrefLabels::CONFIRMEXIT, p_confirmExit->GetValue());
  gwPref.setBool(PrefLabels::CLOSESHELLS, p_closeShells->GetValue());
  gwPref.setBool(PrefLabels::SAVEPASSWORDS, p_savePasswords->GetValue());
  if (p_showBusy != NULL) {
    gwPref.setBool(PrefLabels::SHOWBUSY, p_showBusy->GetValue());
    gwPref.setBool(PrefLabels::ALWAYSONTOP, p_alwaysOnTop->GetValue());
    gwPref.setBool(PrefLabels::LEFTCLICKNEWAPP, p_leftClickNewApp->GetValue());
    gwPref.setInt(PrefLabels::ORIENTATION, p_orientation->GetSelection());
  }
  gwPref.saveFile();

  updateResetButton();
}


void GlobalPrefs::restoreSettings()
{
  int intBuf;
  string strBuf;
  bool boolBuf;

  Preferences eccePref = Preferences(PrefLabels::GLOBALPREFFILE);
  if (!eccePref.getInt(PrefLabels::FONTSIZE, intBuf))
    intBuf = p_fontSizeDefault;
  p_fontSize->SetSelection(intBuf);

  if (!eccePref.getBool(PrefLabels::USESYSTEMFONT, boolBuf))
    boolBuf = p_systemFontDefault;
  p_systemFont->SetValue(boolBuf);

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

  if (!eccePref.getBool(PrefLabels::QUICKTRANSPARENCY, boolBuf))
    boolBuf = p_quickTranspDefault;
  p_quickTransp->SetValue(boolBuf);

  strBuf = "";
  eccePref.getString(PrefLabels::EDITOR, strBuf);
  setProgramRow(p_editor, strBuf);
  strBuf = "";
  eccePref.getString(PrefLabels::TERMINAL, strBuf);
  setProgramRow(p_terminal, strBuf);
  strBuf = "";
  eccePref.getString(PrefLabels::BROWSER, strBuf);
  setProgramRow(p_browser, strBuf);

  Preferences gwPref = Preferences(PrefLabels::GATEWAYPREFFILE);
  restoreWindowSettings("GatewayPrefs", gwPref, false);

  if (!gwPref.getBool(PrefLabels::CONFIRMEXIT, boolBuf))
    boolBuf = p_confirmExitDefault;
  p_confirmExit->SetValue(boolBuf);

  if (!gwPref.getBool(PrefLabels::CLOSESHELLS, boolBuf))
    boolBuf = p_closeShellsDefault;
  p_closeShells->SetValue(boolBuf);

  if (!gwPref.getBool(PrefLabels::SAVEPASSWORDS, boolBuf))
    boolBuf = p_savePasswordsDefault;
  p_savePasswords->SetValue(boolBuf);

  if (p_showBusy != NULL) {
    if (!gwPref.getBool(PrefLabels::SHOWBUSY, boolBuf))
      boolBuf = p_showBusyDefault;
    p_showBusy->SetValue(boolBuf);

    if (!gwPref.getBool(PrefLabels::ALWAYSONTOP, boolBuf))
      boolBuf = p_alwaysOnTopDefault;
    p_alwaysOnTop->SetValue(boolBuf);

    if (!gwPref.getBool(PrefLabels::LEFTCLICKNEWAPP, boolBuf))
      boolBuf = p_leftClickNewAppDefault;
    p_leftClickNewApp->SetValue(boolBuf);

    if (!gwPref.getInt(PrefLabels::ORIENTATION, intBuf))
      intBuf = p_orientationDefault;
    p_orientation->SetSelection(intBuf);
  }
}


void GlobalPrefs::restorePasswordPref()
{
  bool boolBuf;
  Preferences gwPref = Preferences(PrefLabels::GATEWAYPREFFILE);
  if (!gwPref.getBool(PrefLabels::SAVEPASSWORDS, boolBuf))
    boolBuf = p_savePasswordsDefault;
  p_savePasswords->SetValue(boolBuf);
}


void GlobalPrefs::updateResetButton()
{
  p_resetAll->Enable(!isDefaultGlobalPref() || !isDefaultGatewayPref() ||
                     !isDefaultPrograms() || !isDefaultStatePref());
}


bool GlobalPrefs::isDefaultGlobalPref()
{
  return p_fontSize->GetSelection() == p_fontSizeDefault &&
         p_systemFont->GetValue() == p_systemFontDefault &&
         p_dateFormat->GetSelection() == p_dateFormatDefault &&
         p_timeFormat->GetSelection() == p_timeFormatDefault &&
         p_unit->GetSelection() == p_unitDefault &&
         p_beepError->GetValue() == p_beepErrorDefault &&
         p_beepWarn->GetValue() == p_beepWarnDefault &&
         p_focus->GetValue() == p_focusDefault &&
         p_quickTransp->GetValue() == p_quickTranspDefault;
}


bool GlobalPrefs::isDefaultGatewayPref()
{
  if (p_confirmExit->GetValue() != p_confirmExitDefault ||
      p_closeShells->GetValue() != p_closeShellsDefault ||
      p_savePasswords->GetValue() != p_savePasswordsDefault)
    return false;
  if (p_showBusy != NULL &&
      (p_showBusy->GetValue() != p_showBusyDefault ||
       p_alwaysOnTop->GetValue() != p_alwaysOnTopDefault ||
       p_leftClickNewApp->GetValue() != p_leftClickNewAppDefault ||
       p_orientation->GetSelection() != p_orientationDefault))
    return false;
  return true;
}


bool GlobalPrefs::isDefaultPrograms()
{
  return programValue(p_editor).empty() && programValue(p_terminal).empty() &&
         programValue(p_browser).empty();
}


bool GlobalPrefs::isDefaultStatePref()
{
  for (int i = ResourceDescriptor::STATE_CREATED;
       i < ResourceDescriptor::NUMBER_OF_STATES; i++) {
    if (p_stateButton[i][0]->CanBeReset())
      return false;
  }
  return true;
}


void GlobalPrefs::changeStateColor(string statename, string newcolor)
{
  Preferences eccePref = Preferences(PrefLabels::GLOBALPREFFILE);
  eccePref.setString(statename, newcolor);
  eccePref.saveFile();
  OnStatePrefChange();
  updateResetButton();
}


void GlobalPrefs::resetStateColor(string statename)
{
  Preferences eccePref = Preferences(PrefLabels::GLOBALPREFFILE, true);
  string defcolor;
  eccePref.getString(statename, defcolor);
  eccePref = Preferences(PrefLabels::GLOBALPREFFILE);
  eccePref.setString(statename, defcolor);
  eccePref.saveFile();
  OnStatePrefChange();
  updateResetButton();
}


void GlobalPrefs::resetAllStateColors()
{
  Preferences sysPrefs(PrefLabels::GLOBALPREFFILE, true);
  Preferences eccePref = Preferences(PrefLabels::GLOBALPREFFILE);
  string defaultColor;
  string prefString;

  for (int i = ResourceDescriptor::STATE_CREATED;
       i < ResourceDescriptor::NUMBER_OF_STATES; i++) {
    prefString = WxState::getPrefString(i);
    sysPrefs.getString(prefString, defaultColor);
    eccePref.setString(prefString, defaultColor);
  }

  eccePref.saveFile();
  OnStatePrefChange();
}
