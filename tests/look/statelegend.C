//  Show the Organizer's run-state legend (WxState::createLegend) and save it
//  as a PNG, for statelegend.py.
//
//      statelegend OUT.png        (needs a DISPLAY; GTK_THEME picks the theme)
#include <cstdio>
#include <unistd.h>

#include <wx/wx.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>

#include "wxgui/WxState.H"
#include "wxgui/ewxPanel.H"

class App : public wxApp
{
  public:
    virtual bool OnInit() { return true; }
    // An assert dialog would block a headless run forever.
    virtual void OnAssertFailure(const wxChar*, int, const wxChar*,
                                 const wxChar*, const wxChar*) {}
};

IMPLEMENT_APP_NO_MAIN(App)

int main(int argc, char** argv)
{
  wxEntryStart(argc, argv);
  if (!wxApp::GetInstance()->CallOnInit() || argc < 2) return 77;
  wxInitAllImageHandlers();

  wxFrame* frame = new wxFrame(NULL, wxID_ANY, "statelegend", wxPoint(0, 0),
                               wxDefaultSize);
  ewxPanel* legend = WxState::createLegend(frame);
  wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
  sizer->Add(legend, 0, wxGROW);
  frame->SetSizerAndFit(sizer);
  frame->Show(true);
  for (int i = 0; i < 40; ++i) {
    wxYield();
    wxMilliSleep(25);
  }

  // A wxClientDC of the shown window; a wxScreenDC goes stale under Xvfb.
  wxSize size = legend->GetClientSize();
  wxBitmap bmp(size.x, size.y);
  {
    wxClientDC src(legend);
    wxMemoryDC mem(bmp);
    mem.Blit(0, 0, size.x, size.y, &src, 0, 0);
  }
  bool ok = bmp.SaveFile(argv[1], wxBITMAP_TYPE_PNG);
  printf("%s %dx%d %s\n", argv[1], size.x, size.y, ok ? "saved" : "FAILED");
  fflush(stdout);
  // Skip static teardown: the ECCE singletons crash in it after wx is gone.
  _exit(ok ? 0 : 1);
}
