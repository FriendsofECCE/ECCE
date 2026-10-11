//  The Calculation Editor's row of code icons (wxCustomButton) enabled and
//  disabled, as a frame is while a modal question is open over it.  Saves one
//  PNG of the row for each state, for codebuttons.py.
//
//      codebuttons OUTDIR    (needs a DISPLAY)
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>

#include <wx/wx.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>

#include "wxgui/ThingToggle.H"
#include "wxgui/ewxBitmap.H"

class App : public wxApp
{
  public:
    virtual bool OnInit() { return true; }
    virtual void OnAssertFailure(const wxChar*, int, const wxChar*,
                                 const wxChar*, const wxChar*) {}
};

IMPLEMENT_APP_NO_MAIN(App)

static void shoot(wxWindow *w, const std::string& file)
{
  for (int i = 0; i < 20; i++) wxYield();
  w->Refresh();
  w->Update();
  for (int i = 0; i < 20; i++) wxYield();
  wxSize s = w->GetClientSize();
  wxBitmap bmp(s.x, s.y);
  {
    wxMemoryDC mem(bmp);
    wxClientDC dc(w);
    mem.Blit(0, 0, s.x, s.y, &dc, 0, 0);
  }
  bmp.SaveFile(file, wxBITMAP_TYPE_PNG);
}

int main(int argc, char** argv)
{
  wxEntryStart(argc, argv);
  if (!wxApp::GetInstance()->CallOnInit() || argc < 2) return 77;
  wxInitAllImageHandlers();
  const char *icons[] = { "calcorca.xpm", "calcecceqm.xpm", "calcnwchem.xpm",
                          "calcgaussian.xpm", "calcgaussian16.xpm",
                          "calcmopac.xpm", "calcqe.xpm", "calcnwchemmd.xpm",
                          "calcamica.xpm", "calcgamessuk.xpm" };
  wxFrame *frame = new wxFrame(NULL, wxID_ANY, "codebuttons",
                               wxDefaultPosition, wxSize(600, 90));
  wxPanel *panel = new wxPanel(frame);
  wxBoxSizer *row = new wxBoxSizer(wxHORIZONTAL);
  for (int i = 0; i < 10; i++) {
    wxCustomButton *b = new wxCustomButton(panel, wxID_ANY,
        ewxBitmap(icons[i], wxBITMAP_TYPE_XPM), wxDefaultPosition,
        wxDefaultSize, wxCUSTBUT_BUT_DCLICK_TOG);
    b->SetMargins(wxSize(5, 5));
    //  What ewxStyledWindow does to a control that is not a text box.
    if (getenv("NULL_BG")) b->SetBackgroundColour(wxNullColour);
    row->Add(b, 0, wxALL, 3);
  }
  panel->SetSizer(row);
  frame->Show();
  shoot(panel, std::string(argv[1]) + "/enabled.png");
  panel->Disable();
  shoot(panel, std::string(argv[1]) + "/disabled.png");
  panel->Enable();
  //  The real case: the frame is disabled by a modal question over it.
  wxDialog dlg(frame, wxID_ANY, "Code Change");
  wxBoxSizer *dsz = new wxBoxSizer(wxVERTICAL);
  dsz->Add(new wxStaticText(&dlg, wxID_ANY, "Change it to ECCE-QM?"), 0,
           wxALL, 10);
  dsz->Add(dlg.CreateButtonSizer(wxYES_NO), 0, wxALL, 10);
  dlg.SetSizerAndFit(dsz);
  wxTheApp->CallAfter([&]() {
    for (int i = 0; i < 40; i++) { wxYieldIfNeeded(); wxMilliSleep(25); }
    shoot(panel, std::string(argv[1]) + "/modal.png");
    dlg.EndModal(wxID_NO);
  });
  dlg.ShowModal();
  _exit(0);
}
