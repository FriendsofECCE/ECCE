// The Calculation Summary's grid with one long value, painted with and
// without wrapping (#490): summary_wrap_shot OUT.png wrap|plain [width].
#include <wx/app.h>
#include <wx/frame.h>
#include <wx/image.h>
#include <wx/dcmemory.h>
#include <wx/dcscreen.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/utils.h>
#include <unistd.h>
extern "C" {
  #include <gtk/gtk.h>
}
#include "wxgui/ewxNonBoldLabel.H"
#include "wxgui/ewxSelectableText.H"
#include "wxgui/ewxStaticText.H"

class ShotApp : public wxApp {
public:
  bool OnInit() override {
    wxInitAllImageHandlers();
    const wxString out = argv[1];
    const bool wrap = argc > 2 && argv[2] == "wrap";
    const long width = argc > 3 ? atol(argv[3].ToStdString().c_str()) : 360;
    const wxString path = "/proj/nobackup/ecce/Kebnekaise-opt-2026-10/"
                          "A04_mu-OH-mu-SO4-Co2_NH3_8_3plus-1";
    wxFrame *f = new wxFrame(NULL, wxID_ANY, "Summary");
    wxScrolledWindow *body = new wxScrolledWindow(f);
    body->SetScrollRate(0, 10);
    wxFlexGridSizer *g = new wxFlexGridSizer(0, 2, 1, 5);
    body->SetSizer(g);
    const wxString rows[][2] = {{"Machine:", "kebnekaise"}, {"# Processors:", "28"},
        {"Run Directory:", path}, {"Start Date:", "2026-10-20"}};
    for (auto &r : rows) {
      g->Add(new ewxNonBoldLabel(body, wxID_ANY, r[0]), 0, wxALIGN_RIGHT);
      wxWindow *v = new ewxStaticText(body, wxID_ANY, r[1]);
      g->Add(v);
      ewxMakeSelectable(v);
    }
    if (wrap) ewxWrapValueColumn(body);
    wxBoxSizer *s = new wxBoxSizer(wxVERTICAL);
    s->Add(body, 1, wxEXPAND | wxALL, 8);
    f->SetSizer(s);
    f->SetClientSize(width, 200);
    f->Show();
    for (int i = 0; i < 30; i++) { Yield(true); wxMilliSleep(30); }
    // The text a copy would take must be the whole path, unbroken.
    for (auto &r : rows)
      if (r[0] == "Run Directory:")
        for (wxSizerItem *it : g->GetChildren())
          if (it->IsWindow() && GTK_IS_LABEL(it->GetWindow()->GetHandle()) &&
              wxString::FromUTF8(gtk_label_get_text(
                  GTK_LABEL(it->GetWindow()->GetHandle()))) == path)
            printf("copy text intact, label %dx%d\n",
                   it->GetWindow()->GetSize().x, it->GetWindow()->GetSize().y);
    wxScreenDC sdc;
    wxPoint p = f->GetScreenPosition();
    wxSize sz = f->GetSize();
    wxBitmap bmp(sz.x, sz.y);
    wxMemoryDC m(bmp);
    m.Blit(0, 0, sz.x, sz.y, &sdc, p.x, p.y);
    m.SelectObject(wxNullBitmap);
    bmp.SaveFile(out, wxBITMAP_TYPE_PNG);
    printf("wrote %s %dx%d\n", (const char *)out.c_str(), sz.x, sz.y);
    fflush(stdout); _exit(0);  // teardown without the services crashes, after the shot
    return false;
  }
};
wxIMPLEMENT_APP_NO_MAIN(ShotApp);
int main(int argc, char **argv) { return wxEntry(argc, argv); }
