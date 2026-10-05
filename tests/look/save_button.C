//  Lay out the editors' feedback bar and ewxTool's status bar in the
//  "unsaved changes" state at a given window width, and print where
//  everything landed (the driver, save_button.py, screenshots it).
//
//      save_button WIDTH MODIFIED|READONLY|EDIT [HOLD_MS]   (needs a DISPLAY)
#include <cstdio>
#include <cstdlib>
#include <string>

#include <wx/wx.h>

#include "wxgui/WxFeedback.H"
#include "wxgui/ewxStatusBar.H"

class App : public wxApp
{
  public:
    virtual bool OnInit() { return true; }
    // An assert dialog would block a headless run forever.
    virtual void OnAssertFailure(const wxChar*, int, const wxChar*,
                                 const wxChar*, const wxChar*) {}
};

IMPLEMENT_APP_NO_MAIN(App)

static void dump(const char* tag, wxWindow* w)
{
  wxRect r = w->GetRect();
  printf("%s %s \"%s\" shown=%d x=%d y=%d w=%d h=%d best_w=%d\n", tag,
         (const char*) w->GetClassInfo()->GetClassName(),
         (const char*) w->GetLabel().utf8_str(),
         (int) w->IsShown(), r.x, r.y, r.width, r.height, w->GetBestSize().x);
}

int main(int argc, char** argv)
{
  wxEntryStart(argc, argv);
  if (!wxApp::GetInstance()->CallOnInit() || argc < 3) return 77;
  int width = atoi(argv[1]);
  std::string mode = argv[2];

  wxFrame* frame = new wxFrame(NULL, wxID_ANY, "savebutton", wxPoint(0, 0),
                               wxSize(width, 150));
  WxFeedback* fb = new WxFeedback(frame);
  fb->setContextLabel("Project/Calculation/Water");
  WxFeedback::EditStatus st = mode == "MODIFIED" ? WxFeedback::MODIFIED
                            : mode == "READONLY" ? WxFeedback::READONLY
                                                 : WxFeedback::EDIT;
  fb->setEditStatus(st);
  fb->setMessage("Input file generated.", WxFeedback::INFO);

  ewxStatusBar* bar = new ewxStatusBar(frame);
  frame->SetStatusBar(bar);
  bar->SetStatusText("Calculation saved", ewxStatusBar::FIELD_LOG);
  bar->setContext("http://localhost:8096/Ecce/users/andy/Project/Calculation/Water");
  bar->SetStatusText(mode, ewxStatusBar::FIELD_SAVE);

  wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
  sizer->Add(fb, 1, wxGROW);
  frame->SetSizer(sizer);
  frame->Show(true);
  frame->Layout();
  for (int i = 0; i < 20; ++i) wxYield();

  printf("frame w=%d h=%d\n", frame->GetSize().x, frame->GetSize().y);
  wxWindowList kids = fb->GetChildren();
  for (wxWindowList::iterator it = kids.begin(); it != kids.end(); ++it)
    dump("feedback", *it);
  wxWindowList bk = bar->GetChildren();
  for (wxWindowList::iterator it = bk.begin(); it != bk.end(); ++it)
    dump("statusbar", *it);
  printf("statusbar h=%d\n", bar->GetSize().y);
  for (int i = 0; i < ewxStatusBar::FIELD_MAX; ++i) {
    wxRect r;
    bar->GetFieldRect(i, r);
    wxString t = bar->GetStatusText(i);
    int tw = 0, th = 0;
    bar->GetTextExtent(t, &tw, &th);
    printf("field %d x=%d w=%d text_w=%d\n", i, r.x, r.width, tw);
  }
  fflush(stdout);
  if (argc > 3) {   // leave the window up for the screenshot
    wxMilliSleep(atoi(argv[3]));
  }
  return 0;
}
