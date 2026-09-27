
#include <iostream>
  using std::cout;
  using std::endl;

#include "wxgui/ewxApp.H"
#include "wxgui/ewxTextEntryDialog.H"
#include "wxgui/ewxMessageDialog.H"


class MessageDialogApp : public ewxApp
{
  public:
    virtual bool OnInit();
};

IMPLEMENT_APP(MessageDialogApp)

bool MessageDialogApp::OnInit()
{
  ewxApp::OnInit();

  // The test suite runs this app with no arguments; bail out rather than
  // reading argv[1]/argv[2] unchecked.
  if (argc < 3) {
    return false;
  }

  std::string type = argv[1].ToStdString();

  std::string caption = argv[2].ToStdString();

  std::string message;
  for (int i = 3; i < argc; i++) {
    message += argv[i].ToStdString();
    message += "\n";
  }

  if (strncmp(type.c_str(), "pass", 4) == 0) {
    wxPasswordEntryDialog * pass = new wxPasswordEntryDialog(NULL,
        message, caption.c_str(), "", wxOK|wxCANCEL);
    SetTopWindow(pass);
    pass->ShowModal();
    cout << pass->GetValue() << endl;
  } else if (strncmp(type.c_str(), "prompt", 6) == 0) {
    ewxTextEntryDialog * prompt = new ewxTextEntryDialog(NULL,
        message, caption.c_str(), "", wxOK|wxCANCEL);
    SetTopWindow(prompt);
    prompt->ShowModal();
    cout << prompt->GetValue() << endl;
  } else {
    ewxMessageDialog * msg = new ewxMessageDialog(NULL,
        message, caption.c_str(), wxOK|wxICON_EXCLAMATION);
    SetTopWindow(msg);
    msg->ShowModal();
  }

  return false; // exits program immediately
}

