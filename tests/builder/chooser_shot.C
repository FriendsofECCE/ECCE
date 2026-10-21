// Paints the element chooser alone in a frame and saves PNGs (#222), for a
// display with no ECCE services: chooser_shot OUTDIR [search-text].
#include <wx/app.h>
#include <wx/frame.h>
#include <wx/image.h>
#include <wx/dcmemory.h>
#include <wx/dcscreen.h>
#include <wx/sizer.h>
#include <wx/utils.h>

#include "ElementChooser.H"

class ShotApp : public wxApp {
public:
  bool OnInit() override {
    wxInitAllImageHandlers();
    const wxString out = argv[1];
    wxFrame *f = new wxFrame(NULL, wxID_ANY, "Elements");
    ElementChooser *c = new ElementChooser(f, "C");
    wxBoxSizer *s = new wxBoxSizer(wxVERTICAL);
    s->Add(c, 1, wxEXPAND);
    f->SetSizer(s); f->SetClientSize(760, 640);
    f->Show();
    snap(c, out + "/chooser-initial.png");
    c->search(argc > 2 ? argv[2].ToStdString() : "iron", false);
    snap(c, out + "/chooser-search.png");
    c->pick("Fe");
    f->SetClientSize(560, 520);
    snap(c, out + "/chooser-small.png");
    return false;
  }
  void snap(wxWindow *c, const wxString& path) {
    for (int i = 0; i < 20; i++) { Yield(true); wxMilliSleep(30); }
    wxSize sz = c->GetSize();
    wxBitmap bmp = ((ElementChooser *)c)->render(sz);
    bmp.SaveFile(path, wxBITMAP_TYPE_PNG);
    printf("wrote %s %dx%d\n", (const char *)path.c_str(), sz.x, sz.y);
  }
};
wxIMPLEMENT_APP_NO_MAIN(ShotApp);
int main(int argc, char **argv) { return wxEntry(argc, argv); }
