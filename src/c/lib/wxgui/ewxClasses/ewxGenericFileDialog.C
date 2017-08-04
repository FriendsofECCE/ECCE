/**
 * @file
 *
 */

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "ewxGenericFileDialog.H"
#endif

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#if wxUSE_FILEDLG

#include <algorithm>
using std::replace;

#include "wx/generic/dirctrlg.h" // for wxFileIconsTable
#include "wx/tokenzr.h" // for wxStringTokenizer

#include "dsm/BookmarkList.H"
#include "dsm/EDSIFactory.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/Resource.H"

#include "wxgui/ewxChoice.H"
#include "wxgui/ewxConfig.H"
#include "wxgui/ewxFileCtrl.H"
#include "wxgui/ewxFileData.H"
#include "wxgui/ewxGenericFileDialog.H"
#include "wxgui/ewxWindowUtils.H"
#include "wxgui/WxResourceImageList.H"

#define ID_PARENT_DIR  (wxID_FILEDLGG + 6)
#define ID_LIST_CTRL  (wxID_FILEDLGG + 10)
#define ID_SERVER_CHOICE (wxID_FILEDLGG + 101)

IMPLEMENT_DYNAMIC_CLASS(ewxGenericFileDialog, wxGenericFileDialog)

BEGIN_EVENT_TABLE(ewxGenericFileDialog,wxGenericFileDialog)
  EVT_CHOICE(ID_SERVER_CHOICE,ewxGenericFileDialog::onServerChoice)
  EVT_LIST_ITEM_SELECTED(ID_LIST_CTRL, ewxGenericFileDialog::OnSelected)
  EVT_BUTTON(ID_PARENT_DIR, ewxGenericFileDialog::OnHome)
END_EVENT_TABLE()

/**
 *
 */
ewxGenericFileDialog::ewxGenericFileDialog() : 
  wxGenericFileDialog()
{
  init();
}



/**
 *
 */
ewxGenericFileDialog::ewxGenericFileDialog( wxWindow * parent,
        const wxString& message, const EcceURL& defaultUrl,
        const wxString& wildCard, long style, const wxPoint& pos,
        bool bypassGenericImpl) : 
  wxGenericFileDialog()
{
  init();
  Create(parent, message, defaultUrl.getParent().toString(),
         defaultUrl.getFilePathTail(), wildCard, style, pos, bypassGenericImpl);
  restoreSettings();
}



/**
 *
 */
bool ewxGenericFileDialog::Create( wxWindow *parent,
                                   const wxString& message,
                                   const wxString& defaultDir,
                                   const wxString& defaultFile,
                                   const wxString& wildCard,
                                   long style,
                                   const wxPoint& pos,
                                   bool bypassGenericImpl)
{
  if (!wxGenericFileDialog::Create(parent, message, wxEmptyString,
          wxEmptyString, wildCard, style, pos, wxDefaultSize,
          wxFileDialogNameStr, bypassGenericImpl)) {
    return false;
  }

  SetName("FileDialog");

  // replace wxFileCtrl with ewxFileCtrl
  long style2 = ms_lastViewStyle;

  if ( !HasFdFlag(wxFD_MULTIPLE) )
      style2 |= wxLC_SINGLE_SEL;

#ifdef __WXWINCE__
  style2 |= wxSIMPLE_BORDER;
#else
  style2 |= wxSUNKEN_BORDER;
#endif

  wxSize list_size(500,240);
  if (wxSystemSettings::GetScreenType() <= wxSYS_SCREEN_PDA)
    list_size = wxSize(50,80);

  wxFileCtrl  * oldList = m_list;
  ewxFileCtrl * newList;
  GetSizer()->Detach(oldList);
  oldList->Destroy();
  newList = new ewxFileCtrl(this, ID_LIST_CTRL, wxEmptyString,
                            ms_lastShowHidden, wxDefaultPosition,
                            list_size, style2);
  m_list = newList;
  GetSizer()->Insert( 2, m_list, 1, wxEXPAND | wxLEFT|wxRIGHT, 10 );
  ignoreChanges.setOtherBool(&(newList->ignoreChanges));
  local.setOtherBool(&(newList->local));

  ignoreChanges = true;

  SetFilterIndex(0);

  // add server choicebox
  p_serverChoice = new ewxChoice(this, ID_SERVER_CHOICE);
  wxSizer * staticsizer = m_static->GetContainingSizer();
  staticsizer->Prepend(p_serverChoice, 0, wxRIGHT|wxALIGN_CENTER, 10);
  initServerChoices();

  // realign directory path to center vertically
  staticsizer->GetItem(m_static)->SetFlag(wxALIGN_CENTER);

  SetAutoLayout( true );
  GetSizer()->Fit( this );
  GetSizer()->SetSizeHints( this );

  ignoreChanges = false;

  m_dir = defaultDir;
  m_fileName = defaultFile;

  return true;
}



/**
 *
 */
ewxGenericFileDialog::~ewxGenericFileDialog()
{
  ignoreChanges = true;
  cleanupChoices(p_serverChoice);
}



/**
 * Not only set the full path, but also update filename and dir.
 * This function is called immediately before sending the wxOK command event
 * (which closes the dialog) so that the filename and filepath returned by
 * GetFilename and GetPath are set here.
 */
void ewxGenericFileDialog::SetPath(const wxString& path)
{
  if (local) return wxGenericFileDialog::SetPath(path);

  m_path = path;
  EcceURL url = path.c_str();
  m_fileName = url.getFilePathTail();
  m_dir = url.getParent().toString();
}



/**
 * Clears the current wildcard choices and replaces with the given string of the
 * form "All files (*.*)|*.*|JPEG Files (*.jpeg)|*.jpg".
 */
void ewxGenericFileDialog::SetWildcard(const wxString& wildCard)
{
    wxFileDialogBase::SetWildcard(wildCard);

    wxArrayString wildDescriptions, wildFilters;
    const size_t count = wxParseCommonDialogsFilter(m_wildCard,
                                                    wildDescriptions,
                                                    wildFilters);
    wxCHECK_RET( count, wxT("wxFileDialog: bad wildcard string") );

    clearChoices(m_choice);

    for ( size_t n = 0; n < count; n++ )
    {
        m_choice->Append( wildDescriptions[n], new wxString( wildFilters[n] ) );
    }

    SetFilterIndex( 0 );
}



/**
 *
 */
void ewxGenericFileDialog::setServerChoice( int serverIndex )
{
  p_serverChoice->SetSelection( serverIndex );

  doSetServerChoice( serverIndex );
}



/**
 * If the given dir is a child of one of the server choices, update the last
 * dir for the current server and select the new server.
 */
void ewxGenericFileDialog::setServerChoice( string dir )
{
  const int count = p_serverChoice->GetCount();
  int i = 0;
  if (dir != "") {
    for (i = 0; i < count; i++) {
      if (dir.find(p_mountDir[i]) == 0) {
        string oldValue = p_lastDir[i];
        vector<string>::iterator it;
        it = find(p_lastDir.begin(), p_lastDir.end(), oldValue);
        replace(it, it + 1, oldValue, dir);
        break;
      }
    }
  }
  if (i >= count) {
    i = 0;
  }
  setServerChoice( i );
}



/**
 * Returns the name of the selected filter (name, not extension).
 * Can return wxEmptyString if error occurs.
 */
wxString ewxGenericFileDialog::getType() const
{
  wxString ret;

  wxStringTokenizer tokenizer(GetWildcard(), "|");
  if (GetFilterIndex() <= tokenizer.CountTokens()) {
    for (int i = 0; i < GetFilterIndex(); i++) {
      tokenizer.GetNextToken(); // throw away
      tokenizer.GetNextToken(); // throw away
    }
    ret = tokenizer.GetNextToken();
  }

  return ret;
}



/**
 * Returns the extension of the selected file or filter (extension, not name).
 * If the selected file has an extension (*.ext), return it. Otherwise, if 
 * more than one extension exists in filter, always returns the first listed.
 * Can return wxEmptyString if error occurs.
 */
wxString ewxGenericFileDialog::getExt() const
{
  wxString ret;

  wxString filename = GetFilename();
  ret = filename.AfterLast('.');
  if (ret.Len() > 6) { // we don't support extensions greater than 6 characters
    wxStringTokenizer tokenizer(GetWildcard(), "|");
    if (GetFilterIndex() <= tokenizer.CountTokens()) {
      for (int i = 0; i < GetFilterIndex(); i++) {
        tokenizer.GetNextToken(); // throw away
        tokenizer.GetNextToken(); // throw away
      }
      tokenizer.GetNextToken(); // throw away
      wxStringTokenizer wildTokenizer(tokenizer.GetNextToken(), ";");
      ret = wildTokenizer.GetNextToken().AfterLast('.');
    }
  }

  return ret;
}


/**
 * synonym for getType()
 * */
wxString ewxGenericFileDialog::getSelectedWildcardLabel() const
{
  return getType();
}


/**
 * Returns the extensions of the selected filter (extension, not name).
 * If more than one extension exists in filter, returns them all.
 * Should always return at least a length 1 vector.
 */
vector<wxString> ewxGenericFileDialog::getSelectedWildcard() const
{
  vector<wxString> ret;

  wxStringTokenizer tokenizer(GetWildcard(), "|");
  if (GetFilterIndex() <= tokenizer.CountTokens()) {
    for (int i = 0; i < GetFilterIndex(); i++) {
      tokenizer.GetNextToken(); // throw away
      tokenizer.GetNextToken(); // throw away
    }
    tokenizer.GetNextToken(); // throw away
    wxStringTokenizer wildTokenizer(tokenizer.GetNextToken(), ";");
    while (wildTokenizer.HasMoreTokens()) {
      ret.push_back(wildTokenizer.GetNextToken().AfterLast('.'));
    }
  }

  return ret;
}



/**
 *
 */
void ewxGenericFileDialog::OnSelected( wxListEvent &event )
{
  if (local) return wxGenericFileDialog::OnSelected(event);

  /*
  static bool inSelected = false;

  if (inSelected)
    return;

  inSelected = true;
    */
  wxString filename( event.m_item.m_text );

#ifdef __WXWINCE__
  // No double-click on most WinCE devices, so do action immediately.
  HandleAction( filename );
#else
  if (filename == wxT("..")) return;

  wxString dir = m_list->GetDir();
  dir += wxT("/");
  dir += filename;
  if (dirExists(dir)) return;

  ignoreChanges = true;
  m_text->SetValue( filename );
  ignoreChanges = false;
#endif
  //inSelected = false;
}



/**
 *
 */
void ewxGenericFileDialog::OnHome( wxCommandEvent &event )
{
  ignoreChanges = true;
  goToHomeDir();
  m_list->SetFocus();
  UpdateControls();
  ignoreChanges = false;
}



/**
 * Respond to server choicebox selection event.
 */
void ewxGenericFileDialog::onServerChoice( wxCommandEvent &event )
{
  doSetServerChoice((int)event.GetInt());
}



/**
 *
 */
void ewxGenericFileDialog::HandleAction( const wxString &fn )
{
  if (local) return wxGenericFileDialog::HandleAction( fn );
  
    if (ignoreChanges)
        return;

    wxString filename( fn );
    wxString dir = m_list->GetDir();
    EcceURL dirUrl = dir.c_str();

    if (filename.empty()) return;
    if (filename == wxT(".")) return;

    // "some/place/" means they want to chdir not try to load "place"
    bool want_dir = filename.Last() == wxT('/');
    if (want_dir)
        filename = filename.RemoveLast();

    if (filename == wxT(".."))
    {
        ignoreChanges = true;
        m_list->GoToParentDir();
        m_list->SetFocus();
        UpdateControls();
        ignoreChanges = false;
        return;
    }

#ifdef __UNIX__
    if (filename == wxT("~"))
    {
        ignoreChanges = true;
        goToHomeDir();
        m_list->SetFocus();
        UpdateControls();
        ignoreChanges = false;
        return;
    }

    if (filename.BeforeFirst(wxT('/')) == wxT("~"))
    {
        filename = wxString(EDSIServerCentral::getUserHome(
                            EcceURL(m_dir.c_str()).getEcceRoot()).toString()) + 
                            filename.Remove(0, 1);
        dir = filename;
    }
#endif // __UNIX__

    // if a wildcard is found as well as a url separator, we have a bad url
    // string, otherwise we have a wildcard string
    if (!HasFdFlag(wxFD_SAVE))
    {
        if ((filename.Find(wxT('*')) != wxNOT_FOUND) ||
            (filename.Find(wxT('?')) != wxNOT_FOUND))
        {
            if (filename.Find(wxT('/')) != wxNOT_FOUND)
            {
                wxMessageBox(_("Illegal file specification."), _("Error"), wxOK | wxICON_EXCLAMATION );
                return;
            }
            m_list->SetWild( filename );
            return;
        }
    }

    // make sure to convert the filename into an "absolute" url
    if (filename != dir) {
      dir += "/" + filename;
      filename = dir;
    }

    if (dirExists(filename))
    {
        ignoreChanges = true;
        m_list->GoToDir( filename );
        UpdateControls();
        ignoreChanges = false;
        return;
    }

    // they really wanted a dir, but it doesn't exist
    if (want_dir)
    {
        wxMessageBox(_("Directory doesn't exist."), _("Error"),
                     wxOK | wxICON_EXCLAMATION );
        return;
    }

    // append the default extension to the filename if it doesn't have any
    //
    // VZ: the logic of testing for !wxFileExists() only for the open file
    //     dialog is not entirely clear to me, why don't we allow saving to a
    //     file without extension as well?
    if ( !HasFdFlag(wxFD_OPEN) || !fileExists(filename) )
    {
        filename = AppendExtension(filename, m_filterExtension);
    }

    // check that the file [doesn't] exist if necessary
    if ( HasFdFlag(wxFD_SAVE) &&
            HasFdFlag(wxFD_OVERWRITE_PROMPT) &&
                fileExists( filename ) )
    {
        wxString msg;
        msg.Printf( _("File '%s' already exists, do you really want to "
                      "overwrite it?"), filename.c_str() );

        if (wxMessageBox(msg, _("Confirm"), wxYES_NO) != wxYES)
            return;
    }
    else if ( HasFdFlag(wxFD_OPEN) &&
                HasFdFlag(wxFD_FILE_MUST_EXIST) &&
                    !fileExists(filename) )
    {
        wxMessageBox(_("Please choose an existing file."), _("Error"),
                     wxOK | wxICON_EXCLAMATION );
        return;
    }

    SetPath( filename );

    wxCommandEvent event;

    AcceptAndClose();
}



/**
 *
 */
void ewxGenericFileDialog::UpdateControls()
{
  wxGenericFileDialog::UpdateControls();

  string dir = m_list->GetDir().c_str();
  int n = p_serverChoice->GetSelection();
  if (n != wxNOT_FOUND) {
    string oldValue = p_lastDir[n];
    vector<string>::iterator it;
    it = find(p_lastDir.begin(), p_lastDir.end(), oldValue);
    replace(it, it + 1, oldValue, dir);
  }

  m_upDirButton->Enable(dir != p_mountDir[n]);

  wxString label = m_static->GetLabel();
  label.Replace(wxString(p_mountDir[n]), "", false);
#ifdef __UNIX__
  if (!label.StartsWith("/")) {
    label = label.Prepend("/");
  }
#endif // __UNIX__
  m_static->SetLabel(label);
}



/**
 * We only want to save settings if the retCode is wxID_OK.
 */
void ewxGenericFileDialog::EndModal(int retCode)
{
  if (retCode == wxID_OK) {
    saveSettings();
  }
  wxGenericFileDialog::EndModal(retCode);
}


/**
 * Use the server with the given index.
 */
void ewxGenericFileDialog::doSetServerChoice( int serverIndex )
{
  // check for local filesystem
  local = EcceURL(p_mountDir[serverIndex]).isLocal();

  if (local) {
    m_list->SetImageList(wxTheFileIconsTable->GetSmallImageList(),
                         wxIMAGE_LIST_SMALL);
  } else {
    m_list->SetImageList(WxResourceImageList::getImageList(),
                         wxIMAGE_LIST_SMALL);
  }

  m_list->GoToDir(p_lastDir[serverIndex]);
  UpdateControls();
}



/**
 *
 */
void ewxGenericFileDialog::goToHomeDir()
{
  int n = p_serverChoice->GetSelection();
  if (local) {
    string home = wxGetUserHome( wxString() );
    if (home.find(p_mountDir[n]) != 0) {
      m_list->GoToDir(p_mountDir[n]);
    } else {
      m_list->GoToHomeDir();
    }
  } else {
    EcceURL url = p_mountDir[n];
    if (EDSIServerCentral::getUserHome(url.getEcceRoot()).isChildOf(url)) {
      m_list->GoToHomeDir();
    } else {
      m_list->GoToDir(p_mountDir[n]);
    }
  }
}



/**
 *
 */
void ewxGenericFileDialog::init()
{
  p_serverChoice = NULL;
  local = false;
  ignoreChanges = false;
}



/**
 *
 */
void ewxGenericFileDialog::initServerChoices()
{
  if (!p_serverChoice) return;

  clearChoices(p_serverChoice);
  p_mountDir.clear();

  // always allow local filesystem
  p_serverChoice->Append("Local Filesystem");
  // Guessing here.  Mount for local filesystem should be root dir?
  p_mountDir.push_back("/");

  // HACK - (standalone) ebuilder sets this env var
  if (getenv("ECCE_NO_MESSAGING") == NULL) {
    // standard edsi providers
    EDSIServerCentral servers;
    vector<Bookmark> * mount = servers.getEDSIProviders();

    vector<Bookmark>::iterator mountItor = mount->begin();
    for (;mountItor!= mount->end(); mountItor++) {
      p_serverChoice->Append(mountItor->getName());
      p_mountDir.push_back(mountItor->getUrl());
    }
    delete mount;
  }

  // lastly, add any user-defined mount points
  BookmarkList * serverList = new BookmarkList("ServerList");
  if (serverList) {
    vector<Bookmark *> * bookmarks = serverList->getBookmarks();
    vector<Bookmark *>::iterator bookmark = bookmarks->begin();
    for (; bookmark != bookmarks->end(); bookmark++) {
      p_serverChoice->Append((*bookmark)->getName());
      p_mountDir.push_back((*bookmark)->getUrl());
    }
    delete serverList;
  }
}



/**
 * Deletes the client data associated with each selection for the given 
 * choicebox.  Only necessary to call if using void* client data rather than
 * wxClientData or its derived classes.
 */
void ewxGenericFileDialog::cleanupChoices(wxChoice * choice)
{
  if (!choice) return;

  const int count = choice->GetCount();
  for (int i = 0; i < count; i++) {
    if (wxString * str = (wxString*) choice->GetClientData(i)) {
      delete str;
    }
  }
}



/**
 * Calls cleanupChoices before clearing the choicebox selections.
 */
void ewxGenericFileDialog::clearChoices(wxChoice * choice)
{
  if (!choice) return;

  cleanupChoices(choice);
  choice->Clear();
}



/**
 *
 */
bool ewxGenericFileDialog::fileExists(wxString filename)
{
  bool ret = false;
  if (local) {
    ret = wxFileExists(filename);
  } else {
    Resource *resource = EDSIFactory::getResource(EcceURL(filename));
    ret = (resource && resource->isValid());
  }
  return ret;
}



/**
 *
 */
bool ewxGenericFileDialog::dirExists(wxString filename)
{
  bool ret = false;
  if (local) {
    ret = wxDirExists(filename);
  } else {
    Resource *resource = EDSIFactory::getResource(EcceURL(filename.c_str()));
    if (!resource) ret = false;
    else ret = ewxFileData(resource).IsDir();
  }
  return ret;
}



/**
 */
void ewxGenericFileDialog::saveSettings()
{
  ewxConfig *config = ewxConfig::getConfig(string(GetName() + ".ini"));

  //ewxWindowUtils::saveWindowSettings(this, config, true);

  // Don't save window size because with some window managers it
  // won't return the correct size and it will be the incredible
  // growing window.  This change was made on 5/2/08 by Gary.
  // Don't revert it without figuring out a real solution or some
  // other workaround because it was an external user who had
  // this problem (running via a Windows X server--Xming).
  ewxWindowUtils::saveWindowSettings(this, config, false);

  config->Write("DIR", m_dir);
  config->Write("FILENAME", m_fileName);
  config->Write("DIRS", p_lastDir);

  // MOUNTS only stored to check whether our server list is the same as last
  // time.  If it is, we can use the stored most-recent-directory per mount.
  config->Write("MOUNTS", p_mountDir);

  config->Write("FILTER", GetFilterIndex());

  ewxConfig::closeConfigs();
}



/**
 */
void ewxGenericFileDialog::restoreSettings()
{
  ewxConfig *config = ewxConfig::getConfig(string(GetName() + ".ini"));

  int filter = 0;
  vector<string> mounts;

  //ewxWindowUtils::restoreWindowSettings(this, config, true);

  // See comment above in saveSettings about no longer saving the size
  ewxWindowUtils::restoreWindowSettings(this, config, false);

  if (m_dir == wxEmptyString)
    config->Read("DIR", &m_dir, p_mountDir[0]);
  if (m_fileName == wxEmptyString)
    config->Read("FILENAME", &m_fileName);

  // if saved MOUNTS is same as our current server list we can use the stored
  // most-recent-directory per mount
  // if not (i.e. empty dirs), the most-recent-directory IS the mount dir
  p_lastDir.clear();
  if (config->Read("MOUNTS", &mounts)) {
    if (mounts == p_mountDir) {
      config->Read("DIRS", &p_lastDir);
    }
  }
  if (p_lastDir.empty()) {
    p_lastDir = p_mountDir;
  }

  if (config->Read("FILTER", &filter)) {
    if (filter < m_choice->GetCount()) {
      SetFilterIndex(filter);
    }
  }

  ewxConfig::closeConfigs();

  setServerChoice(m_dir.c_str());
}

#endif // wxUSE_FILEDLG

