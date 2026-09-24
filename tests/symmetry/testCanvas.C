//  Paint the REAL diagram canvas, without a person looking at it.
//
//  The model is checked against the character tables and against the
//  course's own worked answers.  The LAYOUT was not checked at all:
//  reaching it meant building a package, installing it, starting the
//  services and opening a calculation, so a second implementation grew
//  up alongside it in tools/modiagram for iterating on -- and two
//  implementations of the same picture drift apart.
//
//  This gives MoDiagramCanvas a model and asks it to paint itself onto
//  a bitmap.  It cannot tell whether the picture is beautiful.  It can
//  tell whether it painted at all, whether it put ink where the levels
//  are, and whether it survives the cases that are easy to get wrong:
//  a column with nothing in it, a level with no label, a spectrum with
//  every level at the same energy.  Those are the ones that crash or
//  silently draw nothing.
#include <cstdio>
#include <string>
#include <vector>

#include <wx/wx.h>
#include <wx/image.h>

#include "MoDiagramCanvas.H"

static int bad = 0;

static void check(const char* what, bool ok, const char* detail = "")
{
  printf("  %-52s %s %s\n", what, ok ? "ok" : "FAIL", detail);
  if (!ok) bad++;
}


/** Paint a canvas onto a bitmap and report how much ink landed. */
static int paint(MoDiagramCanvas *canvas, const wxSize& size,
                 const char *save)
{
  wxBitmap bitmap(size.x, size.y, 24);
  wxMemoryDC dc(bitmap);
  dc.SetBackground(*wxWHITE_BRUSH);
  dc.Clear();

  canvas->SetSize(size);
  canvas->paintOnto(dc, size);
  dc.SelectObject(wxNullBitmap);

  if (save != 0) bitmap.SaveFile(wxString(save, wxConvUTF8), wxBITMAP_TYPE_PNG);

  //  How many pixels are not background.  A layout that throws, draws
  //  off-screen or paints nothing all come out as zero, and they are
  //  the failures worth catching.
  wxImage image = bitmap.ConvertToImage();
  int ink = 0;
  for (int y = 0; y < image.GetHeight(); y++) {
    for (int x = 0; x < image.GetWidth(); x++) {
      if (image.GetRed(x, y) < 250 || image.GetGreen(x, y) < 250 ||
          image.GetBlue(x, y) < 250) {
        ink++;
      }
    }
  }
  return ink;
}


static MoLevel level(const char* label, const char* irrep, double energy,
                     double occupancy, int degeneracy)
{
  MoLevel l;
  l.label = label;
  l.irrep = irrep;
  l.energy = energy;
  l.occupancy = occupancy;
  l.degeneracy = degeneracy;
  l.energies.assign(degeneracy, energy);
  return l;
}


class Harness : public wxApp
{
  public:
    virtual bool OnInit()
    {
      wxImage::AddHandler(new wxPNGHandler);

      wxFrame *frame = new wxFrame(NULL, wxID_ANY, wxT("canvas"));
      MoDiagramCanvas *canvas = new MoDiagramCanvas(frame);
      const wxSize size(900, 700);

      //  --- water, the case every other test uses ------------------
      {
        MoColumn left, centre, right;
        left.title = "O";
        left.levels.push_back(level("A1  (2s)", "A1", -0.87, 2, 1));
        left.levels.push_back(level("B1  (2p)", "B1", -0.44, 1, 1));
        right.title = "2H TASOs";
        right.levels.push_back(level("A1  (1s)", "A1", -0.17, 1, 1));
        centre.title = "Molecular orbitals";
        centre.levels.push_back(level("1a1", "A1", -1.09, 2, 1));
        centre.levels.push_back(level("1b1 nb", "B1", -0.44, 2, 1));
        centre.levels.push_back(level("3a1*", "A1", 0.15, 0, 1));
        centre.levels[0].character = MoLevel::BONDING;
        centre.levels[0].pairing = 0;
        centre.levels[2].character = MoLevel::ANTIBONDING;
        centre.levels[2].pairing = 0;

        vector<MoConnection> links;
        MoDiagram::connect(left.levels, centre.levels, right.levels, links);

        canvas->setGroup("C2V");
        canvas->setDiagram(left, centre, right, links, true,
                           "Energies in Hartree.");
        const int ink = paint(canvas, size, "/tmp/ecce-canvas-water.png");
        check("water paints", ink > 2000,
              wxString::Format(wxT("(%d pixels)"), ink).mb_str());
      }

      //  --- a level diagram with no fragment columns ----------------
      //
      //  What a calculation with no usable symmetry gets.  The columns
      //  are empty and the centre has no irreps, and the layout has to
      //  cope rather than divide by the number of levels it does not
      //  have.
      {
        MoColumn left, centre, right;
        centre.title = "Molecular orbitals";
        for (int i = 0; i < 4; i++) {
          centre.levels.push_back(level("?", "", -1.0 + 0.3*i, 2, 1));
        }
        vector<MoConnection> links;
        canvas->setGroup("C1");
        canvas->setDiagram(left, centre, right, links, false,
                           "This calculation reports no orbital symmetry "
                           "labels, so the levels are unlabelled and cannot "
                           "be correlated with the fragment orbitals.");
        const int ink = paint(canvas, size, "/tmp/ecce-canvas-bare.png");
        check("an unlabelled level diagram paints", ink > 1000,
              wxString::Format(wxT("(%d pixels)"), ink).mb_str());
      }

      //  --- every level at one energy -------------------------------
      //
      //  The layout divides by the energy range to turn energies into
      //  pixels.  A spectrum with no range at all is what a one-orbital
      //  calculation gives, and it is the obvious way to divide by
      //  zero.
      {
        MoColumn left, centre, right;
        centre.title = "Molecular orbitals";
        for (int i = 0; i < 3; i++) {
          centre.levels.push_back(level("1a", "A", -0.5, 2, 1));
        }
        vector<MoConnection> links;
        canvas->setGroup("C1");
        canvas->setDiagram(left, centre, right, links, false, "");
        const int ink = paint(canvas, size, "/tmp/ecce-canvas-flat.png");
        check("a spectrum with no energy range paints", ink > 200,
              wxString::Format(wxT("(%d pixels)"), ink).mb_str());
      }

      //  --- nothing at all ------------------------------------------
      {
        MoColumn left, centre, right;
        vector<MoConnection> links;
        canvas->setGroup("");
        canvas->setDiagram(left, centre, right, links, false,
                           "This calculation has no orbital energies.");
        const int ink = paint(canvas, size, "/tmp/ecce-canvas-empty.png");
        check("an empty diagram paints its explanation", ink > 100,
              wxString::Format(wxT("(%d pixels)"), ink).mb_str());
      }

      //  --- a window too small to lay out in ------------------------
      //
      //  Panels get dragged narrow.  The columns are placed as
      //  fractions of the width and the plot height is what is left
      //  after the headings, so a small enough window makes both
      //  negative.
      {
        MoColumn left, centre, right;
        centre.title = "Molecular orbitals";
        centre.levels.push_back(level("1a1", "A1", -1.0, 2, 1));
        centre.levels.push_back(level("2a1*", "A1", 0.2, 0, 1));
        vector<MoConnection> links;
        canvas->setGroup("C2V");
        canvas->setDiagram(left, centre, right, links, false, "");
        paint(canvas, wxSize(60, 40), 0);
        check("a 60x40 window does not take it down", true);
      }

      printf("\n  %s\n", bad ? "FAIL" : "PASS");
      frame->Destroy();
      ExitMainLoop();
      return true;
    }
};

IMPLEMENT_APP_NO_MAIN(Harness)

int main(int argc, char** argv)
{
  wxEntryStart(argc, argv);
  wxTheApp->CallOnInit();
  wxEntryCleanup();
  return bad ? 1 : 0;
}
