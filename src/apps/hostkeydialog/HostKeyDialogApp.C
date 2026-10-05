// Asks whether to trust an unknown ssh host key: hostkeydialog <host>
// <fingerprint> [keytype].  Prints "accept" and exits 0 on Accept, exits 1
// otherwise.  A separate process so any caller (GUI or not) can ask.
#include <cstdio>
#include <cstdlib>
#include <string>

#include <wx/wx.h>
#include <wx/dcscreen.h>
#include <wx/dcmemory.h>

#include "wxgui/ewxApp.H"
#include "wxgui/ewxDialog.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxWindowUtils.H"

class HostKeyDialogApp : public ewxApp
{
  public:
    virtual bool OnInit();
};

IMPLEMENT_APP(HostKeyDialogApp)

// Dev aid for headless screenshots: paints the dialog to this PNG and cancels.
static const char* SNAPSHOT_ENV = "ECCE_DIALOG_SNAPSHOT";

class HostKeyDialog : public ewxDialog
{
  public:
    HostKeyDialog(const wxString& host, const wxString& fp,
                  const wxString& keyType)
      : ewxDialog(NULL, wxID_ANY, "Unknown host key"), p_snapped(false)
    {
      SetIcon(wxIcon(ewxBitmap::pixmapFile("gateway64.xpm"), wxBITMAP_TYPE_XPM));

      // Fit small screens (#189): wrap to the display, not a fixed width.
      int wrap = wxGetDisplaySize().GetWidth() - 80;
      if (wrap > 520) wrap = 520;
      if (wrap < 280) wrap = 280;

      wxBoxSizer* top = new wxBoxSizer(wxVERTICAL);
      ewxStaticText* t1 = new ewxStaticText(this, wxID_ANY,
        "ECCE has not connected to " + host + " before, so it cannot check "
        "that this is the right machine.");
      t1->Wrap(wrap);
      top->Add(t1, 0, wxALL, 12);

      wxString kt = keyType.IsEmpty() ? wxString("Host") : keyType;
      ewxStaticText* lab = new ewxStaticText(this, wxID_ANY,
                                             kt + " key fingerprint:");
      top->Add(lab, 0, wxLEFT | wxRIGHT, 12);

      ewxTextCtrl* fpBox = new ewxTextCtrl(this, wxID_ANY, fp, wxDefaultPosition,
                                           wxDefaultSize, wxTE_READONLY);
      fpBox->SetFont(wxFont(wxFontInfo(wxSystemSettings::GetFont(
        wxSYS_DEFAULT_GUI_FONT).GetPointSize()).Family(wxFONTFAMILY_TELETYPE)));
      wxSize ext = fpBox->GetTextExtent(fp);
      int w = ext.GetWidth() + 24;
      if (w > wrap) w = wrap;
      fpBox->SetMinSize(wxSize(w, ext.GetHeight() + 10));
      top->Add(fpBox, 0, wxALL | wxEXPAND, 12);

      ewxStaticText* t2 = new ewxStaticText(this, wxID_ANY,
        "Compare it with the fingerprint your administrator gives you, or "
        "with the output of \"ssh-keygen -lf /etc/ssh/ssh_host_*_key.pub\" on the server.  "
        "If you accept, the key is saved in ~/.ssh/known_hosts and you will "
        "not be asked again.");
      t2->Wrap(wrap);
      top->Add(t2, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

      wxStdDialogButtonSizer* btns = new wxStdDialogButtonSizer;
      ewxButton* accept = new ewxButton(this, wxID_OK, "Accept and connect");
      ewxButton* cancel = new ewxButton(this, wxID_CANCEL, "Cancel");
      btns->AddButton(accept);
      btns->AddButton(cancel);
      btns->Realize();
      top->Add(btns, wxSizerFlags().Expand().Border(wxALL, 12));

      SetSizerAndFit(top);
      Centre();
      // Cancel is the default so a stray Enter never trusts a key.
      SetAffirmativeId(wxID_OK);
      SetEscapeId(wxID_CANCEL);
      cancel->SetDefault();
      cancel->SetFocus();

      if (getenv(SNAPSHOT_ENV)) Bind(wxEVT_IDLE, &HostKeyDialog::OnIdle, this);
    }

    // #120: the first UI action of a fresh process needs a server-fresh
    // timestamp or Mutter declines to give the window keyboard focus.
    int ShowModal()
    {
      Show(true);
      ewxRaiseWindow(this);
      return ewxDialog::ShowModal();
    }

  private:
    void OnIdle(wxIdleEvent& e)
    {
      e.Skip();
      if (p_snapped || !IsShown()) return;
      p_snapped = true;
      // Let the first paint land before reading the screen.
      CallAfter([this]() { wxMilliSleep(300); wxYield(); snapshot(); });
    }

    void snapshot()
    {
      wxRect r = GetScreenRect();
      wxScreenDC sdc;
      wxBitmap bmp(r.width, r.height);
      wxMemoryDC mdc(bmp);
      mdc.Blit(0, 0, r.width, r.height, &sdc, r.x, r.y);
      mdc.SelectObject(wxNullBitmap);
      bmp.SaveFile(getenv(SNAPSHOT_ENV), wxBITMAP_TYPE_PNG);
      EndModal(wxID_CANCEL);
    }

    bool p_snapped;
};

bool HostKeyDialogApp::OnInit()
{
  ewxApp::OnInit();

  if (argc < 3) {
    fprintf(stderr, "usage: hostkeydialog host fingerprint [keytype]\n");
    return false;
  }
  HostKeyDialog dlg(argv[1], argv[2], argc > 3 ? argv[3] : wxString());
  if (dlg.ShowModal() == wxID_OK && !getenv(SNAPSHOT_ENV)) {
    printf("accept\n");
    fflush(stdout);
    exit(0);
  }
  exit(1);
}
