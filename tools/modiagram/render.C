//  Paint a molecule's MO diagram with the canvas ECCE itself uses.
//
//  WHY THIS EXISTS.  tools/modiagram/draw.py renders the model with
//  matplotlib, which was how the layout was argued about -- but it is
//  a SECOND IMPLEMENTATION of the picture, and the two drifted: seven
//  separate faults were fixed in draw.py and never in the canvas, so
//  the diagram looked right here and wrong in builder.
//
//  This runs the same engine and then hands the model to the REAL
//  MoDiagramCanvas, painted onto a bitmap.  What comes out is what
//  builder draws, to the pixel, without building a package or opening
//  a calculation.
//
//      render CharacterTables molecule.mol out.png [width height]
#include <cstdio>
#include <cstdlib>
#include <string>

#include <wx/wx.h>
#include <wx/image.h>

#include "tdat/MoSpec.H"
#include "MoDiagramCanvas.H"

class RenderApp : public wxApp
{
  public:
    virtual bool OnInit() { return true; }
};

IMPLEMENT_APP_NO_MAIN(RenderApp)


int main(int argc, char** argv)
{
  if (argc < 4) {
    fprintf(stderr,
            "usage: render <CharacterTables> <molecule> <out.png> "
            "[width height]\n");
    return 2;
  }
  const int width  = (argc > 4) ? atoi(argv[4]) : 1400;
  const int height = (argc > 5) ? atoi(argv[5]) : 950;

  wxEntryStart(argc, argv);
  wxApp::GetInstance()->CallOnInit();
  wxInitAllImageHandlers();

  MoModel model;
  const bool haveFragments = buildModel(argv[1], argv[2], model);

  wxFrame *frame = new wxFrame(NULL, wxID_ANY, "render",
                               wxDefaultPosition, wxSize(width, height));
  MoDiagramCanvas *canvas = new MoDiagramCanvas(frame);
  canvas->setGroup(model.group);
  canvas->setFormula(MoDiagram::formula(model.elements,
                                        model.charge));
  canvas->setDiagram(model.left, model.centre, model.right, model.links,
                     haveFragments, model.note);

  wxBitmap bitmap(width, height, 24);
  wxMemoryDC dc(bitmap);
  dc.SetBackground(*wxWHITE_BRUSH);
  dc.Clear();
  canvas->SetSize(wxSize(width, height));
  canvas->paintOnto(dc, wxSize(width, height));
  dc.SelectObject(wxNullBitmap);

  bitmap.SaveFile(wxString(argv[3], wxConvUTF8), wxBITMAP_TYPE_PNG);

  //  How much of it is not blank, which is the one thing a machine can
  //  say about a picture without a person looking at it.
  wxImage image = bitmap.ConvertToImage();
  int ink = 0;
  for (int y = 0; y < image.GetHeight(); y++)
    for (int x = 0; x < image.GetWidth(); x++)
      if (image.GetRed(x,y) < 250 || image.GetGreen(x,y) < 250 ||
          image.GetBlue(x,y) < 250) ink++;

  printf("note: %s\n", model.note.empty() ? "-" : model.note.c_str());
  printf("%s: %s, %d left %d centre %d right, %d links, %d ink\n",
         argv[2], model.group.c_str(),
         (int)model.left.levels.size(), (int)model.centre.levels.size(),
         (int)model.right.levels.size(), (int)model.links.size(), ink);

  frame->Destroy();
  wxEntryCleanup();
  return 0;
}
