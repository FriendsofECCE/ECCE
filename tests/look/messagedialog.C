// Shows ewxMessageDialog the way the Quit prompt and an error use it and
// saves each as a PNG (F26).  tests/look/messagedialog.py builds and runs it.
#include <unistd.h>
#include "wx/wx.h"
#include "wxgui/ewxMessageDialog.H"

class ShotApp : public wxApp
{
public:
  bool OnInit() override
  {
    wxString out = argc > 1 ? wxString(argv[1]) : wxString(".");
    wxInitAllImageHandlers();
    shoot(out + "/quit.png", "Do you really want to quit?\n\nThe data "
          "server and message broker stay running: another ECCE session "
          "or a job of yours still uses them.", "Quit ECCE",
          wxOK | wxCANCEL | wxICON_QUESTION, true);
    shoot(out + "/error.png", "The calculation could not be saved: the "
          "data server did not answer.", "ECCE",
          wxOK | wxICON_ERROR, false);
    fflush(stdout);
    _exit(0);   // returning would wait on the destroyed dialogs' events
  }

  void shoot(const wxString& png, const wxString& msg, const wxString& cap,
             long style, bool quit)
  {
    ewxMessageDialog *dlg = new ewxMessageDialog(0, msg, cap, style,
                                                 wxDefaultPosition);
    if (quit) {
      dlg->SetButtonLabel(wxID_OK, "Quit");
      dlg->AddButton(wxID_HIGHEST + 1, "Quit and Stop Server");
    }
    dlg->Move(0, 0);
    dlg->Show();
    for (int i = 0; i < 20; ++i) { wxYield(); wxMilliSleep(50); }
    wxSize sz = dlg->GetClientSize();
    wxBitmap bmp(sz);
    wxClientDC src(dlg);
    wxMemoryDC dst(bmp);
    dst.Blit(0, 0, sz.x, sz.y, &src, 0, 0);
    dst.SelectObject(wxNullBitmap);
    bmp.SaveFile(png, wxBITMAP_TYPE_PNG);
    printf("%s %dx%d\n", (const char *) png.utf8_str(), sz.x, sz.y);
    dlg->Destroy();
  }
};

wxIMPLEMENT_APP(ShotApp);
