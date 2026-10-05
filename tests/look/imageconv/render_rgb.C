//  Renders built-in molecules through VizRender::file (the thumbnail and
//  Save As path) to .rgb files and converts each with ImageConverter, so
//  tests/look/imageconv.py can compare against ImageMagick and PIL.
//  Usage: render-rgb <outdir>   (needs a display; run on Xvfb)
//         render-rgb <in> <out> <w> <h>   just convert one file
#include <cmath>
#include <cstdio>
#include <sstream>
#include <string>
#include <unistd.h>

#include <wx/wx.h>

#include "inv/SoWx/SoWx.H"
#include "inv/ChemKit/ChemInit.H"

#include "util/SFile.H"
#include "viz/AtomNodesInit.H"
#include "viz/NodesInit.H"
#include "viz/SGContainer.H"
#include "viz/SGFragment.H"

#include "wxviz/ImageConverter.H"
#include "wxviz/SGContainerManager.H"
#include "wxviz/SGSelection.H"
#include "wxviz/SGViewer.H"
#include "wxviz/VizRender.H"

using std::string;

namespace {

struct Loaders : public VizRender {
  static void apply(SGContainer *sg) { loadAtomColors(sg); loadAtomRadii(sg); }
};

//  Deliberately lopsided: O and N (red, blue) on one side, so a flipped or
//  mirrored image cannot match the reference.
const char *ETHANOLAMINE =
  "12\nethanolamine\n"
  "O  -2.30  0.60  0.00\nC  -1.00 -0.00  0.10\nC   0.10  1.00 -0.10\n"
  "N   1.40  0.40  0.20\nH  -2.90  0.00  0.00\nH  -0.90 -0.60  1.00\n"
  "H  -0.90 -0.70 -0.70\nH   0.10  1.60 -1.00\nH   0.00  1.70  0.80\n"
  "H   2.10  1.00 -0.10\nH   1.50 -0.20  1.00\nH   3.50  3.00  0.00\n";

string benzene()
{
  std::ostringstream o;
  o << "12\nbenzene\n";
  const double rc = 1.397, rh = 1.397 + 1.087;
  for (int i = 0; i < 6; i++)
    o << "C " << rc * cos(i * M_PI / 3) << " " << rc * sin(i * M_PI / 3) << " 0\n";
  for (int i = 0; i < 6; i++)
    o << "H " << rh * cos(i * M_PI / 3) << " " << rh * sin(i * M_PI / 3) << " 0\n";
  return o.str();
}

}  // namespace

class RgbApp : public wxApp
{
public:
  virtual bool OnInit();
  virtual int OnRun();
private:
  bool render(const string& outdir, const string& name, const string& xyz,
              int w, int h, double r, double g, double b);
  wxFrame *p_frame;
  SGViewer *p_viewer;
  SGContainerManager *p_mgr;
};

IMPLEMENT_APP(RgbApp)

bool RgbApp::OnInit()
{
  p_frame = new wxFrame(NULL, wxID_ANY, "render-rgb");
  SetTopWindow(p_frame);
  SoWx::init(p_frame);
  ChemInit::initClasses();
  SGSelection::initClass();
  NodesInit::initClasses();
  AtomNodesInit::initClasses();
  SGFragment::initClass();
  SGContainer::initClass();
  SGContainerManager::initClass();

  p_mgr = new SGContainerManager();
  p_mgr->ref();
  p_viewer = new SGViewer(p_frame, wxID_ANY);
  p_viewer->setText("", "", "", "");
  p_viewer->setSceneGraph(p_mgr);
  p_viewer->setViewing(false);
  p_viewer->setDecoration(false);
  wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
  p_viewer->SetMinSize(wxSize(480, 480));
  sizer->Add(p_viewer, 1, wxEXPAND);
  p_frame->SetSizerAndFit(sizer);
  p_frame->Show(true);
  return true;
}

bool RgbApp::render(const string& outdir, const string& name,
                    const string& xyz, int w, int h,
                    double r, double g, double b)
{
  p_mgr->setSceneGraph("scenes");
  SGContainer *sg = p_mgr->getSceneGraph();
  Loaders::apply(sg);
  SGFragment *frag = sg->getFragment();
  frag->clear();
  std::istringstream in(xyz);
  if (!frag->restoreXYZ(in, 1.0, true)) return false;
  frag->touchNumbers();
  sg->touchChemDisplay();
  p_viewer->viewAll();

  string rgb = outdir + "/" + name + ".rgb";
  SFile file(rgb);
  if (!VizRender::file(p_viewer->getTopNode(), &file, "RGB", w, h, r, g, b)) {
    fprintf(stderr, "%s: %s\n", name.c_str(), VizRender::msg().c_str());
    return false;
  }
  ImageConverter conv;
  try {
    conv.convert(rgb, outdir + "/" + name + ".conv.png", w, h, 8, false);
    conv.convert(rgb, outdir + "/" + name + ".conv.jpg", w, h, 8, false);
  } catch (EcceException& ex) {
    fprintf(stderr, "%s: %s\n", name.c_str(), ex.what());
    return false;
  }
  return true;
}

int RgbApp::OnRun()
{
  if (argc == 5) {
    try {
      ImageConverter().convert(argv[1].ToStdString(), argv[2].ToStdString(),
                               atoi(argv[3].ToStdString().c_str()),
                               atoi(argv[4].ToStdString().c_str()), 8, false);
    } catch (EcceException& ex) {
      fprintf(stderr, "%s\n", ex.what());
      return 1;
    }
    return 0;
  }
  if (argc != 2) {
    fprintf(stderr, "usage: render-rgb <outdir>\n");
    return 2;
  }
  string outdir = argv[1].ToStdString();
  for (int i = 0; i < 30; i++) { Yield(true); wxMilliSleep(10); }

  //  Non-square so a width/height mix-up shows; the coloured background
  //  exercises channel order independent of the atoms.
  bool ok = render(outdir, "ethanolamine", ETHANOLAMINE, 200, 150, 0.1, 0.3, 0.8);
  ok = render(outdir, "benzene", benzene(), 160, 160, 0, 0, 0) && ok;
  //  Skip teardown: the vendored offscreen renderer crashes in Mesa on exit.
  fflush(NULL);
  _exit(ok ? 0 : 1);
}
