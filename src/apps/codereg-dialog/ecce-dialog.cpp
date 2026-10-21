// ecce-dialog: a data-driven C++ replacement for the wxPython code-
// registration "Details" dialogs (scripts/codereg/*.py).  It takes the same
// argv, reads the same restore file and sends the same "name|value|unit|
// enabled|export|kind" lines over UDP, so the Calculation Editor cannot tell
// it from the Python dialog.  The engine is src/wxgui/details; the dialog
// itself is a .dlg file next to the Python one.
//
//   ecce-dialog <file.dlg> <the 17 argv fields>
//   ecce-dialog --inventory <file.dlg> <the 17 argv fields>   (JSON, no GUI)

#include "DetailsPanel.h"

#include <wx/display.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iostream>

using namespace details;
using std::string;
using std::vector;

namespace {

// argv: <restore> <port> <flag> <rw> <debug> <category> <theory> <runtype>
//       <calc> <rxn> <sym> <nelec> <mult> <nfrozen> <nocc> <nvirt> <nmodes>
bool makeParams(const vector<string>& a, Params& p, long& port)
{
  if (a.size() < 17) return false;
  static const char *names[] = {"restore", "port", "restoreflag", "rw",
    "debug", "category", "theory", "runtype", "calc", "rxn", "symmetry",
    "nelec", "mult", "nfrozen", "nocc", "nvirt", "nmodes"};
  for (int i = 0; i < 17; i++) p.vars[names[i]] = a[i];
  p.vars["pointgroup"] = a[10];
  p.vars["calcname"] = a[8];
  p.restoreFile = a[0];
  port = atol(a[1].c_str());
  p.restoreFlag = a[2];
  p.readOnly = a[3];
  p.debug = a[4] == "DebugOn";
  const char *c = getenv("ECCE_QST_CANDIDATES");
  p.candidatesFile = c ? c : "";
  return true;
}

int inventory(const vector<string>& a)
{
  Params p;
  long port;
  Description d;
  if (a.size() < 18 || !makeParams(vector<string>(a.begin() + 1, a.end()), p, port)) {
    fprintf(stderr, "usage: ecce-dialog --inventory X.dlg <17 argv fields>\n");
    return 2;
  }
  string err;
  if (!d.load(a[0])) { fprintf(stderr, "ecce-dialog: %s\n", d.error.c_str()); return 2; }
  Model m(d, p);
  if (!m.build(err)) { fprintf(stderr, "ecce-dialog: %s\n", err.c_str()); return 2; }
  std::cout << m.inventoryJson();
  return 0;
}

class DialogApp : public wxApp {
public:
  bool OnInit() override
  {
    vector<string> a;
    for (int i = 1; i < argc; i++) a.push_back(string(argv[i].ToUTF8()));
#ifdef __WXMSW__
    if (!a.empty() && a[0] == "--inventory") {
      inventory(vector<string>(a.begin() + 1, a.end()));
      return false;
    }
#endif
    if (a.size() < 18 || !makeParams(vector<string>(a.begin() + 1, a.end()),
                                     par_, port_)) {
      fprintf(stderr, "ecce-dialog: details dialog command line parameter "
              "missing!\n");
      return false;
    }
    if (!desc_.load(a[0])) {
      fprintf(stderr, "ecce-dialog: %s\n", desc_.error.c_str());
      exit(2);
    }
    // Sockets need the running app, so the rest waits for the event loop.
    CallAfter([this] { start(); });
    return true;
  }

private:
  Description desc_;
  Params par_;
  long port_ = 0;
  Model *model_ = 0;
  UdpTransport *udp_ = 0;

  void start()
  {
    if (port_) {
      udp_ = new UdpTransport(port_);
      udp_->send("#STARTED " + (par_.restoreFlag == "NO_GUIValues"
                                    ? string("0")
                                    : std::to_string((long)wxGetProcessId())) +
                 "\n");
    }
    model_ = new Model(desc_, par_);
    string err;
    if (!model_->build(err)) {
      fprintf(stderr, "ecce-dialog: %s\n", err.c_str());
      exit(2);
    }
    wxFrame *f = new wxFrame(0, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
        wxDEFAULT_FRAME_STYLE & ~(wxRESIZE_BORDER | wxMAXIMIZE_BOX));
    f->SetFont(themeFont());
    DetailsPanel *p = new DetailsPanel(f, *model_);
    model_->attach(udp_, p);
    f->Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent&) { model_->close(); });
    model_->initialize();
    if (par_.restoreFlag == "NO_GUIValues") return;   // closed already
    f->Fit();
    placeOverEditor(f);
    f->Show(true);
  }

  static void placeOverEditor(wxFrame *f)
  {
    wxString r;
    int x, y, w, h;
    if (!wxGetEnv("ECCE_EDITOR_RECT", &r) ||
        sscanf(r.ToStdString().c_str(), "%d,%d,%d,%d", &x, &y, &w, &h) != 4)
      return;
    wxSize sz = f->GetSize();
    int px = x + (w - sz.x) / 2, py = y + (h - sz.y) / 2;
    int n = wxDisplay::GetFromPoint(wxPoint(x + w / 2, y + h / 2));
    if (n != wxNOT_FOUND) {
      wxRect area = wxDisplay(n).GetClientArea();
      px = std::max(area.x, std::min(px, area.x + area.width - sz.x));
      py = std::max(area.y, std::min(py, area.y + area.height - sz.y));
    }
    f->SetPosition(wxPoint(px, py));
  }
};

}  // namespace

#ifdef __WXMSW__
wxIMPLEMENT_APP(DialogApp);
#else
wxIMPLEMENT_APP_NO_MAIN(DialogApp);

// --inventory needs no display, so it is answered before wx starts GTK.
int main(int argc, char **argv)
{
  if (argc > 2 && string(argv[1]) == "--inventory")
    return inventory(vector<string>(argv + 2, argv + argc));
  return wxEntry(argc, argv);
}
#endif
