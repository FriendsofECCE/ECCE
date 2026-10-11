//  The Calculation Editor's row of code icons (wxCustomButton) enabled and
//  disabled, as a frame is while a modal question is open over it.  Saves one
//  PNG of the row for each state, for codebuttons.py.
//
//      codebuttons OUTDIR    (needs a DISPLAY)
#include <cstdio>
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
                          "calcgaussian.xpm" };
  wxFrame *frame = new wxFrame(NULL, wxID_ANY, "codebuttons",
                               wxDefaultPosition, wxSize(400, 90));
  wxPanel *panel = new wxPanel(frame);
  wxBoxSizer *row = new wxBoxSizer(wxHORIZONTAL);
  for (int i = 0; i < 4; i++) {
    wxCustomButton *b = new wxCustomButton(panel, wxID_ANY,
        ewxBitmap(icons[i], wxBITMAP_TYPE_XPM), wxDefaultPosition,
        wxDefaultSize, wxCUSTBUT_BUT_DCLICK_TOG);
    b->SetMargins(wxSize(5, 5));
    row->Add(b, 0, wxALL, 3);
  }
  panel->SetSizer(row);
  frame->Show();
  shoot(panel, std::string(argv[1]) + "/enabled.png");
  panel->Disable();
  shoot(panel, std::string(argv[1]) + "/disabled.png");
  _exit(0);
}
