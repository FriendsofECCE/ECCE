//  Two top-level windows with the usual kinds of text, for comparing a
//  focused window with one in GTK's :backdrop state.
//
//      backdrop APPLY_FIX HOLD_MS     (needs a DISPLAY; APPLY_FIX 0 or 1)
//
//  Window "Left" is at x=0 and active, "Right" at x=460 and in :backdrop.  Compiled with -DHAVE_NEW it
//  calls ewxApp::applyBackdropStyle() when APPLY_FIX is 1; without it the
//  program runs against a tree that does not have the function.
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#include <wx/wx.h>
#include <wx/listctrl.h>
#include <gtk/gtk.h>

#ifdef HAVE_NEW
#include "wxgui/ewxApp.H"
#endif

class App : public wxApp
{
  public:
    virtual bool OnInit() { return true; }
    virtual void OnAssertFailure(const wxChar*, int, const wxChar*,
                                 const wxChar*, const wxChar*) {}
};

IMPLEMENT_APP_NO_MAIN(App)

static void setBackdrop(wxFrame* f, bool backdrop)
{
  if (backdrop) gtk_widget_set_state_flags(f->GetHandle(), GTK_STATE_FLAG_BACKDROP, FALSE);
  else gtk_widget_unset_state_flags(f->GetHandle(), GTK_STATE_FLAG_BACKDROP);
}

static wxFrame* makeFrame(const wxString& title, int x, bool backdrop)
{
  wxFrame* f = new wxFrame(NULL, wxID_ANY, title, wxPoint(x, 0), wxSize(440, 330));
  wxPanel* p = new wxPanel(f);
  wxBoxSizer* col = new wxBoxSizer(wxVERTICAL);
  col->Add(new wxStaticText(p, wxID_ANY, "Normal label: Energy (Hartree)"), wxSizerFlags().Border());
  wxStaticText* dis = new wxStaticText(p, wxID_ANY, "Disabled label: Basis set");
  dis->Enable(false);
  col->Add(dis, wxSizerFlags().Border());
  col->Add(new wxCheckBox(p, wxID_ANY, "A checkbox label"), wxSizerFlags().Border());
  wxTextCtrl* t = new wxTextCtrl(p, wxID_ANY, "Entry text");
  col->Add(t, wxSizerFlags().Expand().Border());
  wxTextCtrl* td = new wxTextCtrl(p, wxID_ANY, "Disabled entry");
  td->Enable(false);
  col->Add(td, wxSizerFlags().Expand().Border());
  col->Add(new wxButton(p, wxID_ANY, "A button"), wxSizerFlags().Border());
  wxListCtrl* l = new wxListCtrl(p, wxID_ANY, wxDefaultPosition, wxSize(-1, 70), wxLC_REPORT);
  l->InsertColumn(0, "Name");
  l->InsertItem(0, "benzene");
  l->InsertItem(1, "water");
  col->Add(l, wxSizerFlags(1).Expand().Border());
  p->SetSizer(col);
  f->Show();
  // No window manager on the headless display, so no focus ever moves and
  // GTK keeps every window active; set the state a manager would.
  setBackdrop(f, backdrop);
  f->Layout();
  wxYield();
  struct { const char* name; wxWindow* w; } rects[] = {
    {"label", NULL}, {"disabled-label", dis}, {"entry", t}, {"disabled-entry", td}};
  rects[0].w = p->GetChildren()[0];
  for (auto& r : rects) {
    wxRect q = r.w->GetScreenRect();
    printf("RECT %s %s %d %d %d %d\n", (const char*) title.utf8_str(), r.name,
           q.x, q.y, q.width, q.height);
  }
  return f;
}

int main(int argc, char** argv)
{
  wxEntryStart(argc, argv);
  if (!wxApp::GetInstance()->CallOnInit() || argc < 3) return 77;
#ifdef HAVE_NEW
  if (atoi(argv[1])) ewxApp::applyBackdropStyle();
#endif
  wxFrame* left = makeFrame("Left", 0, false);
  wxFrame* right = makeFrame("Right", 460, true);
  fflush(stdout);
  wxStopWatch sw;
  while (sw.Time() < atol(argv[2])) {
    wxYield();
    setBackdrop(left, false);   // GTK may reassert its own idea of focus
    setBackdrop(right, true);
    wxMilliSleep(20);
  }
  _exit(0);
}
