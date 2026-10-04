//  Renders a SceneScript on a built-in XYZ system through the Builder's own
//  scene code (SGContainerManager/SGContainer/SGFragment in an SGViewer), so
//  the vendored-Inventor build and the Coin build can be compared.
//  Usage: viewer-scenes <outdir> <script> <water|benzene|crco6>
//  Driven by tools/coin/compare.sh; calculation-backed scenes (MO, normal
//  modes, geometry trace) run inside the Builder instead (ECCE_VIEWER_SCENE).
#include <cmath>
#include <cstdio>
#include <sstream>
#include <string>
#include <unistd.h>

#include <wx/wx.h>

#include "inv/SoWx/SoWx.H"
#include "inv/ChemKit/ChemInit.H"

#include "viz/AtomNodesInit.H"
#include "viz/ForegroundCmd.H"
#include "viz/NodesInit.H"
#include "viz/SGContainer.H"
#include "viz/SGFragment.H"

#include "wxviz/SGContainerManager.H"
#include "wxviz/SGSelection.H"
#include "wxviz/SGViewer.H"
#include "wxviz/SceneScript.H"
#include "wxviz/VizRender.H"

using std::string;

namespace {

//  The protected loaders are what the thumbnail path uses to get the same
//  element colours and radii as the Builder.
struct Loaders : public VizRender {
  static void apply(SGContainer *sg) { loadAtomColors(sg); loadAtomRadii(sg); }
};

string xyzFor(const string& sys)
{
  std::ostringstream o;
  if (sys == "water") {
    o << "3\nwater\nO 0 0 0.1173\nH 0 0.7572 -0.4692\nH 0 -0.7572 -0.4692\n";
  } else if (sys == "benzene") {
    o << "12\nbenzene\n";
    const double rc = 1.397, rh = 1.397 + 1.087;
    for (int i = 0; i < 6; i++)
      o << "C " << rc * cos(i * M_PI / 3) << " " << rc * sin(i * M_PI / 3) << " 0\n";
    for (int i = 0; i < 6; i++)
      o << "H " << rh * cos(i * M_PI / 3) << " " << rh * sin(i * M_PI / 3) << " 0\n";
  } else if (sys == "crco6") {
    o << "13\nCr(CO)6\nCr 0 0 0\n";
    const int ax[6][3] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
    for (int i = 0; i < 6; i++)
      o << "C " << 1.91 * ax[i][0] << " " << 1.91 * ax[i][1] << " " << 1.91 * ax[i][2] << "\n";
    for (int i = 0; i < 6; i++)
      o << "O " << 3.05 * ax[i][0] << " " << 3.05 * ax[i][1] << " " << 3.05 * ax[i][2] << "\n";
  }
  return o.str();
}

}  // namespace

class ScenesApp : public wxApp
{
public:
  virtual bool OnInit();
  virtual int OnRun();
private:
  wxFrame *p_frame;
  SGViewer *p_viewer;
  SGContainerManager *p_mgr;
};

IMPLEMENT_APP(ScenesApp)

bool ScenesApp::OnInit()
{
  p_frame = new wxFrame(NULL, wxID_ANY, "viewer-scenes");
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

int ScenesApp::OnRun()
{
  if (argc != 4) {
    fprintf(stderr, "usage: viewer-scenes <outdir> <script> <system>\n");
    return 2;
  }
  string outdir = argv[1].ToStdString(), script = argv[2].ToStdString();
  string sys = argv[3].ToStdString();
  string xyz = xyzFor(sys);
  if (xyz.empty()) { fprintf(stderr, "unknown system %s\n", sys.c_str()); return 2; }

  for (int i = 0; i < 30; i++) { Yield(true); wxMilliSleep(10); }

  p_mgr->setSceneGraph("scenes");
  SGContainer *sg = p_mgr->getSceneGraph();
  Loaders::apply(sg);
  SGFragment *frag = sg->getFragment();
  std::istringstream in(xyz);
  if (!frag->restoreXYZ(in, 1.0, true)) { fprintf(stderr, "bad xyz\n"); return 1; }
  frag->touchNumbers();
  sg->touchChemDisplay();
  p_viewer->viewAll();

  //  Labels are drawn in the foreground colour, which the Builder sets from
  //  the background (VizRender::loadDisplayStyle does the same).
  SbColor fg = p_viewer->calculateContrastingColor();
  ForegroundCmd fgCmd("Foreground", sg);
  fgCmd.getParameter("red")->setDouble(fg[0]);
  fgCmd.getParameter("green")->setDouble(fg[1]);
  fgCmd.getParameter("blue")->setDouble(fg[2]);
  fgCmd.execute();

  SceneScript run(p_viewer, sg, 0, outdir);
  if (!run.run(script)) {
    fprintf(stderr, "scene script failed: %s\n", run.message().c_str());
    return 1;
  }
  //  Skip teardown: the vendored offscreen renderer crashes in Mesa on exit.
  fflush(NULL);
  _exit(0);
}
