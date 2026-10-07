//  Renders a SceneScript on a built-in XYZ system through the Builder's own
//  scene code (SGContainerManager/SGContainer/SGFragment in an SGViewer), so
//  the vendored-Inventor build and the Coin build can be compared.
//  Usage: viewer-scenes <outdir> <script> <water|benzene|crco6|glycine|ethanol>
//  Driven by tools/coin/compare.sh; calculation-backed scenes (MO, normal
//  modes, geometry trace) run inside the Builder instead (ECCE_VIEWER_SCENE).
#include <cmath>
#include <cstdio>
#include <sstream>
#include <vector>
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
  } else if (sys == "glycine") {
    o << "10\nglycine\nN -1.0852 0.9767 0\nC 0 0 0\nC 1.3861 -0.6238 0\n"
         "O 2.3679 0.0836 0\nO 1.5219 -1.9669 0\nH -0.3438 -0.5272 -0.8899\n"
         "H -0.3438 -0.5272 0.8899\nH -0.7923 1.8007 0.5053\n"
         "H -0.7923 1.8007 -0.5053\nH 2.2109 -2.6496 0\n";
  } else if (sys == "ethanol") {
    o << "9\nethanol\nC 0 0 0\nC 1.52 0 0\nO 1.9973 1.348 0\n"
         "H -0.3638 -1.0275 0\nH -0.3638 0.5137 -0.8898\nH -0.3638 0.5137 0.8898\n"
         "H 1.8838 0 -1.0275\nH 1.8838 0 1.0275\nH 2.1043 1.6501 -0.9049\n";
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
  else if (sys == "waterbox") {
    //  data/client/solvents/water216.xyz, 2x2x2: 5184 atoms (the file's
    //  OW/HW tags are not element symbols).
    const char *home = getenv("ECCE_HOME");
    FILE *f = fopen((string(home ? home : ".") + "/data/client/solvents/water216.xyz").c_str(), "r");
    if (!f) return "";
    char line[256];
    int n = 0;
    double box = 0;
    if (!fgets(line, sizeof line, f) || sscanf(line, "%d", &n) != 1 ||
        !fgets(line, sizeof line, f) || sscanf(line, "%lf", &box) != 1) { fclose(f); return ""; }
    struct A { char e; double x, y, z; };
    std::vector<A> a;
    while (fgets(line, sizeof line, f)) {
      char sym[16]; double x, y, z;
      if (sscanf(line, "%15s %lf %lf %lf", sym, &x, &y, &z) == 4) a.push_back({sym[0], x, y, z});
    }
    fclose(f);
    o << a.size() * 8 << "\nwater box 2x2x2\n";
    for (int i = 0; i < 2; i++) for (int j = 0; j < 2; j++) for (int k = 0; k < 2; k++)
      for (size_t m = 0; m < a.size(); m++)
        o << a[m].e << " " << a[m].x + i * box << " " << a[m].y + j * box << " " << a[m].z + k * box << "\n";
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
