
#include <iostream>
  using std::cout;
  using std::endl;

#include "wxgui/ewxApp.H"
#include "wxgui/WxAuth.H"


class PasswordDialogApp : public ewxApp
{
  public:
    virtual bool OnInit();
};

IMPLEMENT_APP(PasswordDialogApp)

bool PasswordDialogApp::OnInit()
{
  ewxApp::OnInit();

  char* type = argv[1];
  char* host = argv[2];
  char* user = argv[3];

  WxAuth authDlg(NULL);

  authDlg.setPrompt("");
  authDlg.setServerLabel("Host:");
  authDlg.setServer(host);

  authDlg.disableUser();
  authDlg.setUser(user);

  type[0] = toupper(type[0]); // old school :)
  string label = type;
  label += ":";
  authDlg.setPasswordLabel(label);

  if (authDlg.ShowModal() == wxID_OK) {
    cout << authDlg.getPassword() << endl;
  }

  return false; // exits program immediately
}

