
#include "util/Ecce.H"
#include "util/Host.H"
#include "util/Preferences.H"
#include "util/PreferenceLabels.H"
#include "util/JMSMessage.H"
#include "util/JMSPublisher.H"

#include "dsm/ResourceDescriptor.H"

#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxCheckBox.H"

#include "wxgui/WxAuth.H"


/**
 * Constructor.
 */
WxAuth::WxAuth() : WxAuthGUI()
{
   setStyles(this,false);
}



/**
 * Constructor.
 */
WxAuth::WxAuth( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
    : WxAuthGUI(parent, id, caption, pos, size, style)
{
   SetIcon(wxIcon(ewxBitmap::pixmapFile("gateway64.xpm"), wxBITMAP_TYPE_XPM));

   setStyles(this,false);

   ewxStaticText *l = ((ewxStaticText*)FindWindow(wxID_STATIC_AUTH_NEWPASSWD));
   l->Show(false);
   ewxTextCtrl *t = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_AUTH_NEWPASSWORD));
   t->Show(false);

#ifdef EMSL
   ewxCheckBox *c =((ewxCheckBox*)FindWindow(ID_CHECKBOX_AUTH_SAVEPASSWORDS));
   c->Show(true);

   Preferences gwPref = Preferences(PrefLabels::GATEWAYPREFFILE);
   bool boolBuf = false;
   gwPref.getBool(PrefLabels::SAVEPASSWORDS, boolBuf);
   c->SetValue(boolBuf);
#endif

   ewxTextCtrl * passInput = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_AUTH_PASSWORD));
   passInput->setLeaveAsEnter(false);
   passInput->SetFocus();
}



/**
 * Destructor.
 */
WxAuth::~WxAuth()
{
}



void WxAuth::OnCloseWindow( wxCloseEvent& event )
{
  event.Skip();
}



/**
 * Returns control to caller if both username and password fields are filled
 * in.  Otherwise beeps at the user.
 */
void WxAuth::OnOK( wxCommandEvent& event )
{
  if (getUser() != "" && getPassword() != "") {
    event.Skip();
  } else {
    wxBell();
  }
}



void WxAuth::OnTextctrlAuthUserEnter( wxCommandEvent& event )
{
    // Insert custom code here
    event.Skip();
}




void WxAuth::OnTextctrlAuthPasswordEnter( wxCommandEvent& event )
{
    // Insert custom code here
  OnOK(event);
}



void WxAuth::OnChange( wxCommandEvent& event )
{
  setPrompt("To change your data server password, please enter\nyour current and newly chosen password before hitting OK:");
  setPasswordLabel("Current Password:");
  ewxStaticText *l = ((ewxStaticText*)FindWindow(wxID_STATIC_AUTH_NEWPASSWD));
  l->Show(true);
  ewxTextCtrl *t = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_AUTH_NEWPASSWORD));
  t->Show(true);

  // wxWidgets is really picky about dynamic changes to the GUI.  Must call
  // the Layout() method even after minor changes like showing/hiding fields
  // else things will look really bad
  Layout();
}



void WxAuth::OnSavePasswordsClick( wxCommandEvent& event )
{
  ewxCheckBox *c =((ewxCheckBox*)FindWindow(ID_CHECKBOX_AUTH_SAVEPASSWORDS));
  Preferences gwPref = Preferences(PrefLabels::GATEWAYPREFFILE);
  gwPref.setBool(PrefLabels::SAVEPASSWORDS, c->GetValue());
  gwPref.saveFile();

  // Goodbye to any saved passwords 
  if (!c->GetValue()) {
    string configFile = Ecce::realUserPrefPath();
    configFile += "EMSLAuth";
    unlink(configFile.c_str());
  }

  JMSPublisher publisher("WxAuth");
  Target gateway(GATEWAY, "");
  JMSMessage *blankMsg = publisher.newMessage(gateway);
  publisher.publish("ecce_preferences_gateway", *blankMsg);
  delete blankMsg;
}



void WxAuth::setPrompt(const string& prompt)
{
   ewxStaticText *l = ((ewxStaticText*)FindWindow(wxID_STATIC_AUTH_PROMPT_LABEL));

   l->SetLabel(prompt.c_str());
}



void WxAuth::setServer(const string& server)
{
   ewxStaticText *l = ((ewxStaticText*)FindWindow(wxID_STATIC_AUTH_SERVER_VALUE));
   l->SetLabel(server.c_str());
}



void WxAuth::setServerLabel(const string& serverLabel)
{
   ewxStaticText *l = ((ewxStaticText*)FindWindow(wxID_STATIC_AUTH_SERVER_LABEL));
   l->SetLabel(serverLabel.c_str());
}



void WxAuth::setUser(const string& user)
{
   ewxTextCtrl *t = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_AUTH_USER));
   t->SetValue(user.c_str());
}



void WxAuth::disableUser()
{
   ewxTextCtrl *t = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_AUTH_USER));
   t->setCustomDisabledStyle(false);
}


void WxAuth::setPassword(const string& password)
{
   ewxTextCtrl *t = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_AUTH_PASSWORD));
   t->SetValue(password.c_str());
}


void WxAuth::setNewPassword(const string& newPassword)
{
   ewxTextCtrl *t = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_AUTH_NEWPASSWORD));
   t->SetValue(newPassword.c_str());
}


void WxAuth::setPasswordLabel(const string& passwordLabel)
{
   ewxStaticText *l = ((ewxStaticText*)FindWindow(wxID_STATIC_AUTH_PASSWD));
   l->SetLabel(passwordLabel.c_str());
}



void WxAuth::setProtocol(const string& protocol)
{
  ewxStaticText *l = ((ewxStaticText*)FindWindow(wxID_STATIC_AUTH_PROMPT_LABEL));
  if (protocol != "http") {
    l->SetLabel("Please enter your compute server password:");
    ewxTextCtrl *t = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_AUTH_USER));
    t->setCustomDisabledStyle(false);   
  }
}



void WxAuth::showChangeBtn(const bool& show)
{
  ewxButton *b = ((ewxButton*)FindWindow(wxID_CHANGE));
  b->Show(show);
}


/**
 * Returns the value from the username field.
 */
string WxAuth::getUser() 
{
   ewxTextCtrl *t = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_AUTH_USER));
   return t->GetValue().c_str();
}


/**
 * Returns the value from the password field.
 */
string WxAuth::getPassword()
{
   ewxTextCtrl *t = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_AUTH_PASSWORD));
   return t->GetValue().c_str();
}


/**
 * Returns the value from the new password field.
 */
string WxAuth::getNewPassword()
{
   ewxTextCtrl *t = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_AUTH_NEWPASSWORD));
   return t->GetValue().c_str();
}

