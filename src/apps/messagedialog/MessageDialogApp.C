
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

  char* type = argv[1];

  char* caption = argv[2];

  char message[1024];
  strcpy(message, "");

  for (int i = 3; i < argc; i++) {
    strcat(message, argv[i]);
    strcat(message, "\n");
  }

  if (strncmp(type, "pass", 4) == 0) {
    wxPasswordEntryDialog * pass = new wxPasswordEntryDialog(NULL,
        message, caption, "", wxOK|wxCANCEL);
    SetTopWindow(pass);
    pass->ShowModal();
    cout << pass->GetValue() << endl;
  } else if (strncmp(type, "prompt", 6) == 0) {
    ewxTextEntryDialog * prompt = new ewxTextEntryDialog(NULL,
        message, caption, "", wxOK|wxCANCEL);
    SetTopWindow(prompt);
    prompt->ShowModal();
    cout << prompt->GetValue() << endl;
  } else {
    ewxMessageDialog * msg = new ewxMessageDialog(NULL,
        message, caption, wxOK|wxICON_EXCLAMATION);
    SetTopWindow(msg);
    msg->ShowModal();
  }

  return false; // exits program immediately
}

