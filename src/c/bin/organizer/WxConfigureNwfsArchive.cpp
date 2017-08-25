/**
 * @file
 *
 * @author Jack Chatterton
 *
 */

#include <iostream>
using std::cerr;
using std::endl;

#include "tdat/AuthCache.H"

#include "dsm/MachinePreferences.H"
#include "dsm/EDSIFactory.H"

#include "wxgui/ewxTextCtrl.H"

#include "CalcMgr.H"
#include "WxConfigureNwfsArchive.H"


/**
 * Default constructor which should never be used.
 */
WxConfigureNwfsArchive::WxConfigureNwfsArchive( )
{

}


/**
 * Constructor.
 * All the default parameters are defined in WxConfigureNwfsArchiveGUI class.
 */
WxConfigureNwfsArchive::WxConfigureNwfsArchive( wxWindow* parent, 
                wxWindowID id,
                const wxString& caption,
                const wxPoint& pos,
                const wxSize& size,
                long style ):
  WxConfigureNwfsArchiveGUI(parent, id, caption, pos, size, style) {

  setTextCtrlValues();

}



/**
 * Destructor.
 * Does nothing.  Parent window destroys.
 */
WxConfigureNwfsArchive::~WxConfigureNwfsArchive( ) {
  // do nothing, parent window uses Destroy()
}

/**
 * Saves information in window.
 */
void WxConfigureNwfsArchive::OnOkClick( wxCommandEvent& event ) {

  MachinePreferences *prefs = NULL;
  prefs = MachinePreferences::create("nwfs");

  ewxTextCtrl* usernameCtrl = (ewxTextCtrl *)(FindWindowById(ID_USERNAME, this));
  ewxTextCtrl* passwordCtrl = (ewxTextCtrl *)(FindWindowById(ID_PASSWORD, this));
  ewxTextCtrl* rootdirCtrl = (ewxTextCtrl *)(FindWindowById(ID_ROOTDIR, this));

  string username = (string)usernameCtrl->GetValue();
  string password = (string)passwordCtrl->GetValue();
  string rootdir = (string)rootdirCtrl->GetValue();

  prefs->setUsername(username);
  prefs->setRemoteDirectory(rootdir);

  // Add password to the cache even though it hasn't been verified
  AuthCache::getCache().addAuthentication("scp://nwfs", username,
                                          password, "", true);

  MachinePreferences::saveChanges();

  Show(false);

}

/**
 * Closes window without saving changes.
 */
void WxConfigureNwfsArchive::OnCancelClick( wxCommandEvent& event ) {
  
  Show(false);

}

/** 
 * Make sure values are correct when Show is called.
 * This looks weird because if we do it when the dialog is first shown,
 * it is called before the widgets have been instantiated, and the program
 * segfaults.
 */
bool WxConfigureNwfsArchive::Show(const bool show) {

  if(false == show)
    setTextCtrlValues();

  return wxDialog::Show(show);

}

/**
 * Utility function. Sets the control values to the values stored, or to
 * blank if none are stored.
 */
void WxConfigureNwfsArchive::setTextCtrlValues() {
  
  MachinePreferences *prefs = NULL;
  prefs = MachinePreferences::create("nwfs");

  string username = prefs->getUsername();
  string rootdir = prefs->getRemoteDirectory();

  ewxTextCtrl* usernameCtrl = (ewxTextCtrl *)(FindWindowById(ID_USERNAME,this));
  usernameCtrl->SetValue(username);

  string password = "";
  BasicAuth *ba = AuthCache::getCache().getAuthentication("scp://nwfs",
                                                          username, "", 1);
  if (ba != NULL) {
    password = ba->m_pass;
    delete ba;
  }

  ewxTextCtrl* passwordCtrl = (ewxTextCtrl *)(FindWindowById(ID_PASSWORD,this));
  passwordCtrl->SetValue(password);

  ewxTextCtrl* rootdirCtrl = (ewxTextCtrl *)(FindWindowById(ID_ROOTDIR, this));
  rootdirCtrl->SetValue(rootdir);

}

