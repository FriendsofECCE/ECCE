
#include <cstdlib>
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

  std::string type = argv[1].ToStdString();
  std::string host = argv[2].ToStdString();
  std::string user = argv[3].ToStdString();

  WxAuth authDlg(NULL);

  // A fourth argument "remember" shows the checkbox (ssh passwords only).
  const bool canRemember = argc > 4 && argv[4].ToStdString() == "remember" &&
                           type == "password";
  authDlg.enableRemember(canRemember);

  authDlg.setPrompt("");
  authDlg.setServerLabel("Host:");
  authDlg.setServer(host);

  authDlg.disableUser();
  authDlg.setUser(user);

  type[0] = toupper(type[0]); // old school :)
  string label = type;
  label += ":";
  authDlg.setPasswordLabel(label);

  // Returning false from OnInit makes wx exit with 255, which a caller
  // (ecce-askpass) cannot tell from a failure: say OK or cancel instead.
  // ECCE_TEST_AUTH_ANSWER: the answer without the dialog ("cancel" cancels).
  const char* test = getenv("ECCE_TEST_AUTH_ANSWER");
  const bool ok = test ? std::string(test) != "cancel"
                       : authDlg.ShowModal() == wxID_OK;
  if (ok) {
    cout << (test ? std::string(test) : authDlg.getPassword()) << endl;
    // ECCE_TEST_AUTH_REMEMBER: the checkbox ticked, without the dialog.
    if (canRemember && (test ? getenv("ECCE_TEST_AUTH_REMEMBER") != NULL
                             : authDlg.remembered()))
      cout << "remember" << endl;
    exit(0);
  }
  exit(1);
}

