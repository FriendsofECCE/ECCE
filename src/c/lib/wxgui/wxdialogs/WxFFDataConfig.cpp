/**
 * @file
 *
 *
 */

#include "wx/ctrlsub.h" // for wxControlWithItems
#include "wx/dirdlg.h"
#include "wx/filedlg.h"

#include "dsm/EDSIServerCentral.H"

#include "util/Ecce.H"
#include "util/SFile.H"

#include "wxgui/ewxMessageDialog.H"
#include "wxgui/ewxWindowUtils.H"
#include "wxgui/WxFFDataConfig.H"


typedef map<string, vector<string>, less<string> >::value_type PathMapPair;
typedef map<string, vector<string>, less<string> >::const_iterator PathIterator;


// The name of this dialog (useful to preferences)
string WxFFDataConfig::p_appName = "WxFFDataConfig";


/**
 * Constructor.
 */
WxFFDataConfig::WxFFDataConfig(wxWindow * parent)
  :WxFFDataConfigGUI(parent)
{
  loadPreferences();
  SetEscapeId(wxID_CLOSE);
}



/**
 * Destructor.
 */
WxFFDataConfig::~WxFFDataConfig()
{
}



/**
 * Retrieves the name for the selected force field type.
 */
string WxFFDataConfig::getFFName()
{
  wxControlWithItems* ffChoice = 
          dynamic_cast<wxControlWithItems*>(FindWindow(ID_CHOICE_FORCE_FIELD));
  return ffChoice->GetStringSelection().c_str();
}



/**
 * Retrieves all available force fields types from the choice box.
 */
vector<string> WxFFDataConfig::getFFNames()
{
  vector<string> ret;

  wxControlWithItems* ffChoice = (wxControlWithItems*) FindWindow(ID_CHOICE_FORCE_FIELD);

  for (int i = 0; i < ffChoice->GetCount(); i++) {
    ret.push_back(ffChoice->GetString(i).c_str());
  }

  return ret;
}



/**
 * Retrieves all force field paths for the currently selected force field type.
 */
vector<string> WxFFDataConfig::getFFPaths()
{
  return getFFPaths(getFFName());
}



/**
 * Retrieves all force field paths for the given force field type.
 */
vector<string> WxFFDataConfig::getFFPaths(string ffName)
{
  return p_ffPaths[ffName];
}



/**
 * Retrieves all segment paths for the currently selected force field type.
 */
vector<string> WxFFDataConfig::getSegPaths()
{
  return getSegPaths(getFFName());
}



/**
 * Retrieves all segment paths for the given force field type.
 */
vector<string> WxFFDataConfig::getSegPaths(string ffName)
{
  return p_segPaths[ffName];
}



/**
 * Given the force field selection, updates the path lists.
 */
void WxFFDataConfig::doChoiceForceFieldSelected(string selection)
{
  wxControlWithItems * control;
  vector<string> paths;
  vector<string>::iterator pathsIter;

  control = dynamic_cast<wxControlWithItems*>(FindWindow(ID_LISTBOX_FF));
  control->Clear();
  paths = p_ffPaths[selection];
  for (pathsIter = paths.begin(); pathsIter != paths.end(); pathsIter++) {
    control->Append(pathsIter->c_str());
  }

  control = dynamic_cast<wxControlWithItems*>(FindWindow(ID_LISTBOX_SEG));
  control->Clear();
  paths = p_segPaths[selection];
  for (pathsIter = paths.begin(); pathsIter != paths.end(); pathsIter++) {
    control->Append(pathsIter->c_str());
  }
}



/**
 *
 */
void WxFFDataConfig::doButtonRemoveClick(int id)
{
  wxControlWithItems * list = dynamic_cast<wxControlWithItems*>(FindWindow(id));
  int selection = list->GetSelection();
  if (selection != wxNOT_FOUND) {
    list->Delete(selection);
    if (id == ID_LISTBOX_FF) {
      p_ffPaths[getFFName()].erase(p_ffPaths[getFFName()].begin() + selection);
    } else {
      p_segPaths[getFFName()].erase(p_segPaths[getFFName()].begin() + selection);
    }
  }
}



/**
 *
 */
void WxFFDataConfig::doButtonUpClick(int id)
{
  wxControlWithItems * list = dynamic_cast<wxControlWithItems*>(FindWindow(id));
  int selection = list->GetSelection();
  if (selection == wxNOT_FOUND || selection == 0) {
    return;
  }

  // swap the strings
  int other = selection - 1;
  wxString selectionString = list->GetString(selection);
  wxString otherString = list->GetString(other);
  list->SetString(selection, otherString);
  list->SetString(other, selectionString);

  // make sure the select property moved with it
  list->Select(other);

  // now we need to update the underlying data structure
  if (id == ID_LISTBOX_FF) {
    p_ffPaths[getFFName()][selection] = otherString;
    p_ffPaths[getFFName()][other] = selectionString;
  } else {
    p_segPaths[getFFName()][selection] = otherString;
    p_segPaths[getFFName()][other] = selectionString;
  }
}



/**
 *
 */
void WxFFDataConfig::doButtonDownClick(int id)
{
  wxControlWithItems * list = dynamic_cast<wxControlWithItems*>(FindWindow(id));
  int selection = list->GetSelection();
  if (selection == wxNOT_FOUND || selection == list->GetCount() - 1) {
    return;
  }

  // swap the strings
  int other = selection + 1;
  wxString selectionString = list->GetString(selection);
  wxString otherString = list->GetString(other);
  list->SetString(selection, otherString);
  list->SetString(other, selectionString);

  // make sure the select property moved with it
  list->Select(other);

  // now we need to update the underlying data structure
  if (id == ID_LISTBOX_FF) {
    p_ffPaths[getFFName()][selection] = otherString;
    p_ffPaths[getFFName()][other] = selectionString;
  } else {
    p_segPaths[getFFName()][selection] = otherString;
    p_segPaths[getFFName()][other] = selectionString;
  }
}



/**
 *
 */
void WxFFDataConfig::OnCloseWindow( wxCloseEvent& event )
{
  savePreferences();
  this->Destroy();
}



/**
 *
 */
void WxFFDataConfig::OnChoiceForceFieldSelected( wxCommandEvent& event )
{
  doChoiceForceFieldSelected(event.GetString().c_str());
}



/**
 *
 */
void WxFFDataConfig::OnButtonFfBrowseClick( wxCommandEvent& event )
{
  Preferences prefs(p_appName);

  string lastDir;
  if (!prefs.getString("LAST_DIR", lastDir)) {
    lastDir = Ecce::realUserHome();
  }
  
  wxFileDialog dialog(this, "Choose a file", lastDir);
  
  if (dialog.ShowModal() != wxID_CANCEL) {
    wxControlWithItems * control = 
            dynamic_cast<wxControlWithItems*>(FindWindow(ID_LISTBOX_FF));
    control->Append(dialog.GetPath());
    string path = dialog.GetPath().c_str();
    string dir = dialog.GetDirectory().c_str();
    p_ffPaths[getFFName()].push_back(path);

    prefs.setString("LAST_DIR", dir);
    prefs.saveFile();
  }
}



/**
 *
 */
void WxFFDataConfig::OnButtonFfRemoveClick( wxCommandEvent& event )
{
  doButtonRemoveClick(ID_LISTBOX_FF);
}



/**
 *
 */
void WxFFDataConfig::OnButtonFfUpClick( wxCommandEvent& event )
{
  doButtonUpClick(ID_LISTBOX_FF);
}



/**
 *
 */
void WxFFDataConfig::OnButtonFfDownClick( wxCommandEvent& event )
{
  doButtonDownClick(ID_LISTBOX_FF);
}



/**
 *
 */
void WxFFDataConfig::OnButtonSegBrowseClick( wxCommandEvent& event )
{
  Preferences prefs(p_appName);

  string lastDir;
  if (!prefs.getString("LAST_DIR", lastDir)) {
    lastDir = Ecce::realUserHome();
  }
  
  wxDirDialog dialog(this, "Choose a directory", lastDir);
  
  if (dialog.ShowModal() != wxID_CANCEL) {
    wxControlWithItems * control = 
            dynamic_cast<wxControlWithItems*>(FindWindow(ID_LISTBOX_SEG));
    control->Append(dialog.GetPath());
    string path = dialog.GetPath().c_str();
    p_segPaths[getFFName()].push_back(path);

    prefs.setString("LAST_DIR", path);
    prefs.saveFile();
  }
}



/**
 *
 */
void WxFFDataConfig::OnButtonSegRemoveClick( wxCommandEvent& event )
{
  doButtonRemoveClick(ID_LISTBOX_SEG);
}



/**
 *
 */
void WxFFDataConfig::OnButtonSegUpClick( wxCommandEvent& event )
{
  doButtonUpClick(ID_LISTBOX_SEG);
}



/**
 *
 */
void WxFFDataConfig::OnButtonSegDownClick( wxCommandEvent& event )
{
  doButtonDownClick(ID_LISTBOX_SEG);
}



/**
 *
 */
void WxFFDataConfig::OnButtonDefaultsClick( wxCommandEvent& event )
{
  // Clear any values that might exist
  p_ffPaths.clear();
  p_segPaths.clear();
  loadPreferences(false);
}



/**
 *
 */
void WxFFDataConfig::OnCloseClick( wxCommandEvent& event )
{
  Close(true);
}



/**
 *
 */
void WxFFDataConfig::OnHelpClick( wxCommandEvent& event )
{

}



/**
 * Loads the force field parameter file and segment directories from the
 * preferences.
 *
 * The old design used a shared preferences with the Builder.  This dialog
 * in reality is used in multiple applications (Builder, MDEditor, etc), 
 * so it should store its own preferences.
 */
void WxFFDataConfig::loadPreferences(const bool& usePrefs) throw (IOException)
{
  vector<string> ffNames = getFFNames();
  vector<string> tmp;
  string exmsg;
  string keytag;
  EDSIServerCentral dataInfo;

  Preferences * prefs = new Preferences(p_appName);

  // Builder used to house these preferences, so if the user is migrating to
  // this dialog for the first time, load from Builder
  if (! prefs->isValid()) {
    delete prefs;
    prefs = new Preferences("Builder");
  } else {
    ewxWindowUtils::restoreWindowSettings(this, p_appName, *prefs, true);
  }

  if (usePrefs) {
    for (int i = 0; i < ffNames.size(); i++) {
      // get the force field paths
      keytag = ffNames[i] + "FF";
      tmp.clear();
      if (prefs->getStringList(keytag.c_str(), tmp)) {
        if (tmp.size() > 0) {
          exmsg += validate(ffNames[i], tmp);
          p_ffPaths.insert(PathMapPair(ffNames[i], tmp));
        }
      }
  
      // get the segment paths
      keytag = ffNames[i] + "SEG";
      tmp.clear();
      if (prefs->getStringList(keytag.c_str(), tmp)) {
        if (tmp.size() > 0) {
          exmsg += validate(ffNames[i], tmp);
          p_segPaths.insert(PathMapPair(ffNames[i], tmp));
        }
      }
    }
  }

  for (int i = 0; i < ffNames.size(); i++) {
    // get the force field paths
    if (p_ffPaths.find(ffNames[i]) == p_ffPaths.end()) {
      // Use system defaults
      vector<string> *params = dataInfo.getDefaultParamFiles(ffNames[i]);
      if (params->size() > 0) {
        exmsg += validate(ffNames[i], *params);
        p_ffPaths.insert(PathMapPair(ffNames[i], *params));
      }
      delete params;
    }

    // get the segment paths
    if (p_segPaths.find(ffNames[i]) == p_segPaths.end()) {
      // Use system defaults
      vector<string> *segs = dataInfo.getDefaultSegmentDirs(ffNames[i]);
      if (segs->size() > 0) {
        exmsg += validate(ffNames[i], *segs);
        p_segPaths.insert(PathMapPair(ffNames[i], *segs));
      }
      delete segs;
    }
  }

  if (!exmsg.empty()) {
    ewxMessageDialog(this, "One or more force field files or directories "
            "do not exist!", "Force Field Config Error",
            wxDEFAULT_DIALOG_STYLE|wxOK).ShowModal();
  }

  // now that the paths are all loaded, restore the type of force field
  string ffname;
  wxControlWithItems* control = 
          dynamic_cast<wxControlWithItems*>(FindWindow(ID_CHOICE_FORCE_FIELD));
  if (usePrefs && prefs->getString("FFTYPE", ffname)) {
    // update the data lists based on the preference
    if (control->FindString(ffname) != wxNOT_FOUND) {
      control->SetStringSelection(ffname); // does not fire event
      doChoiceForceFieldSelected(ffname);
    } else {
      // if force field from pref does not match any in list, select first
      control->SetSelection(0); // does not fire event
      doChoiceForceFieldSelected(control->GetStringSelection().c_str());
    }
  } else {
    // if no force field in prefs, select first in list
    control->SetSelection(0); // does not fire event
    doChoiceForceFieldSelected(control->GetStringSelection().c_str());
  }

  delete prefs;
}



/**
 * Saves the force field parameter files and segment directories
 * currently in use to use preferences UNLESS they are the same as the
 * system defaults.
 *
 * The old design shared preferences with the Builder.  This dialog
 * in reality is used in multiple applications (Builder, MDEditor, etc),
 * so it should store its own preferences.
 */
void WxFFDataConfig::savePreferences()
{
  Preferences newPrefs(p_appName);
  ewxWindowUtils::saveWindowSettings(this, p_appName, newPrefs, true);
  savePreferences(newPrefs);
  Preferences oldPrefs("Builder");
  savePreferences(oldPrefs);
}


void WxFFDataConfig::savePreferences(Preferences& prefs)
{
  vector<string> stltmp;
  PathIterator paths;
  string keytag;

  // save the current force field name
  prefs.setString("FFTYPE", getFFName());

  // for each force field name, save the corresponding paths
  for (paths = p_ffPaths.begin(); paths != p_ffPaths.end(); paths++) {
    // Get ff paths from model
    stltmp.clear();
    stltmp = paths->second;

    keytag = paths->first + "FF";
    if (stltmp.size() == 0)
      prefs.remove_entry(keytag.c_str());
    else
      prefs.setStringList(keytag.c_str(), stltmp);
  }
  
  // for each force field name, save the corresponding paths
  for (paths = p_segPaths.begin(); paths != p_segPaths.end(); paths++) {
    // Get seg paths from model
    stltmp.clear();
    stltmp = paths->second;

    keytag = paths->first + "SEG";
    if (stltmp.size() == 0)
      prefs.remove_entry(keytag.c_str());
    else
      prefs.setStringList(keytag.c_str(), stltmp);
  }

  prefs.saveFile();
}



/**
 *
 */
string WxFFDataConfig::validate(const string& name, const vector<string>& paths)
{
  string ret;
  for (int i = 0; i < paths.size(); i++) {
    SFile file(paths[i]);
    if (!file.exists()) {
      ret.append(paths[i] + " ");
    }
  }
  return ret;
}

