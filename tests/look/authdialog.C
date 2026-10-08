//  Show the data-server login dialog (WxAuth) as WxDavAuth fills it in.
//
//      authdialog MODE HOLD_MS     MODE: plain | tls-pinned | tls-ca | retry | change
//
//  (needs a DISPLAY).  Prints the dialog's size and the labels it shows.
//  Compiled with -DHAVE_NEW against a tree that has setEncryption() and
//  setStatus(); without it the same calls are made the way the dialog was
//  filled in before them, so one program gives the before and after pictures.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#include <wx/wx.h>

#include "wxgui/WxAuth.H"

class App : public wxApp
{
  public:
    virtual bool OnInit() { return true; }
    virtual void OnAssertFailure(const wxChar*, int, const wxChar*,
                                 const wxChar*, const wxChar*) {}
};

IMPLEMENT_APP_NO_MAIN(App)

int main(int argc, char** argv)
{
  wxEntryStart(argc, argv);
  if (!wxApp::GetInstance()->CallOnInit() || argc < 3) return 77;
  wxInitAllImageHandlers();
  const char* mode = argv[1];
  const bool tls = !strncmp(mode, "tls", 3);

  WxAuth* dlg = new WxAuth(NULL);
  dlg->showChangeBtn(false);
  dlg->setServer(tls ? "https://dataserver.example.org:8443"
                     : "dataserver.example.org");
  dlg->setProtocol("http");
  dlg->setUser("alice");
  dlg->setPassword("hunter2hunter2");
#ifdef HAVE_NEW
  if (tls)
    dlg->setEncryption(!strcmp(mode, "tls-pinned")
      ? "Encrypted connection (TLS), server certificate pinned"
      : "Encrypted connection (TLS), certificate checked by the system");
  if (!strcmp(mode, "retry")) {
    dlg->setPrompt("Please try again:");
    dlg->setStatus("The user name or password was not accepted.");
  }
#else
  if (!strcmp(mode, "retry"))
    dlg->setPrompt("The user name or password was not accepted.\nPlease try again:");
#endif
  if (!strcmp(mode, "change")) {
    dlg->showChangeBtn(true);
    wxCommandEvent ev(wxEVT_BUTTON, WxAuthGUI::wxID_CHANGE);
    dlg->OnChange(ev);
  }
  dlg->Show();
  dlg->Layout();
  printf("DIALOG %s w=%d h=%d\n", mode, dlg->GetSize().x, dlg->GetSize().y);
  fflush(stdout);
  wxStopWatch sw;
  while (sw.Time() < atol(argv[2])) { wxYield(); wxMilliSleep(20); }
  _exit(0);  // skip teardown, which segfaults headless
}
