//  The real MoDiagramCanvas painted onto a bitmap at a given width, with the
//  three column headings given on the command line (an empty one is left
//  out), for modiagramheads.py: do the headings overlap when it is narrow?
//
//      modiagramheads OUT.png WIDTH LEFT CENTRE RIGHT
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>

#include <wx/wx.h>
#include <wx/dcmemory.h>
#include <wx/image.h>

#include "tdat/MoDiagram.H"
#include "MoDiagramCanvas.H"

class App : public wxApp
{
  public:
    virtual bool OnInit() { return true; }
    virtual void OnAssertFailure(const wxChar*, int, const wxChar*,
                                 const wxChar*, const wxChar*) {}
};

IMPLEMENT_APP_NO_MAIN(App)

static MoLevel level(double e, const char *label, int index, double occ)
{
  MoLevel l;
  l.energy = e;
  l.label = label;
  l.irrep = label;
  l.degeneracy = 1;
  l.orbitals.push_back(index);
  l.energies.push_back(e);
  l.occupancy = occ;
  return l;
}

//  Answers the canvas's request for a composition, as the MOs panel does.
class Owner : public MoDiagramClickTarget
{
  public:
    virtual void orbitalClicked(int) {}
    virtual std::string orbitalComposition(const std::vector<int>& o)
    {
      return std::string(o.size() > 1 ? "MOs" : "MO") +
             " composition (Mulliken population):\n"
             "F1 p 71.2%   H2 s 20.0%   F1 s 6.1%   other (2) 2.7%";
    }
};

int main(int argc, char** argv)
{
  if (argc < 6) return 2;
  wxEntryStart(argc, argv);
  wxApp::GetInstance()->CallOnInit();
  wxInitAllImageHandlers();

  MoColumn left, centre, right;
  //  "~" is a heading left blank while its column stays.
  left.title = argv[3];
  centre.title = argv[4];
  right.title = argv[5];
  if (left.title.size()) left.levels.push_back(level(-0.5, "1s", 0, 1));
  centre.levels.push_back(level(-1.1, "1a1", 1, 2));
  centre.levels.push_back(level(-0.6, "2a1", 2, 2));
  centre.levels.push_back(level(-0.3, "1e", 3, 4));
  if (right.title.size()) {
    right.levels.push_back(level(-1.2, "2s", 4, 2));
    right.levels.push_back(level(-0.4, "2p", 5, 5));
  }

  if (left.title == "~") left.title = "";
  if (centre.title == "~") centre.title = "";
  if (right.title == "~") right.title = "";
  const int width = atoi(argv[2]);
  wxFrame *frame = new wxFrame(NULL, wxID_ANY, "heads");
  MoDiagramCanvas *canvas = new MoDiagramCanvas(frame);
  canvas->setGroup("C4V");
  canvas->setFormula("HF");
  canvas->setDiagram(left, centre, right, std::vector<MoConnection>(),
                     true, "");
  Owner owner;
  if (argc > 6) {
    //  "click": a click on a centre level, found by trying points down the
    //  middle column, then painted again with its text shown.
    canvas->setClickHandler(&owner);
    {
      wxBitmap warm(width, 500);
      wxMemoryDC dc(warm);
      canvas->paintOnto(dc, wxSize(width, 500));
    }
    for (int y = 60; y < 480 && canvas->infoText().empty(); y += 3) {
      wxMouseEvent ev(wxEVT_LEFT_DOWN);
      ev.SetPosition(wxPoint(width / 2, y));
      canvas->GetEventHandler()->ProcessEvent(ev);
    }
  }
  wxBitmap bmp(width, 500);
  {
    wxMemoryDC dc(bmp);
    dc.SetBackground(*wxWHITE_BRUSH);
    dc.Clear();
    canvas->paintOnto(dc, wxSize(width, 500));
  }
  bmp.SaveFile(argv[1], wxBITMAP_TYPE_PNG);
  _exit(0);
}
