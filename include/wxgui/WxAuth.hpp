/**
 * @file
 *
 *
 */

#ifndef WXAUTH_H
#define WXAUTH_H


#include <wx/wx.h>

#include <string>
using std::string;

#include "wxgui/WxAuthGUI.H"

/**
 * The implementation of wxAUthGUI.
 * User is prompted for username and password if info can't be 
 * retrieved from cache.
 */
class WxAuth : public WxAuthGUI
{
public:
      
  WxAuth( );
  WxAuth( wxWindow* parent, wxWindowID id = SYMBOL_WXAUTHGUI_IDNAME,
          const wxString& caption = SYMBOL_WXAUTHGUI_TITLE,
          const wxPoint& pos = SYMBOL_WXAUTHGUI_POSITION,
          const wxSize& size = SYMBOL_WXAUTHGUI_SIZE,
          long style = SYMBOL_WXAUTHGUI_STYLE );
  virtual ~WxAuth();

  virtual void OnCloseWindow( wxCloseEvent& event );
  virtual void OnTextctrlAuthUserEnter( wxCommandEvent& event );
  virtual void OnTextctrlAuthPasswordEnter( wxCommandEvent& event );
  virtual void OnOK(wxCommandEvent& event);
  virtual void OnChange(wxCommandEvent& event);
  virtual void OnSavePasswordsClick(wxCommandEvent& event);

  void setPrompt(const string& prompt);
  void setServer(const string& server);
  void setServerLabel(const string& serverLabel);
  void setUser(const string& user);
  void disableUser();
  void setPassword(const string& password);
  void setNewPassword(const string& newPassword);
  void setPasswordLabel(const string& passwordLabel);
  void setProtocol(const string& protocol);
  void showChangeBtn(const bool& show);

  string getUser();
  string getPassword();
  string getNewPassword();

};

#endif
