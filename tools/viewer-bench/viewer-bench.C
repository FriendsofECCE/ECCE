//  Frame-time benchmark for the Builder's molecule viewer.
//
//  WHY: to decide whether rewriting moiv's immediate-mode GL drawing is
//  worth it we need numbers from the real code path on real hardware.  This
//  builds scenes exactly as the Builder does (SGContainerManager ->
//  SGContainer -> SGFragment + CSStyleCmd, shown in an SGViewer) and times
//  complete frames (rotate, paint, glFinish) with Inventor render caching
//  AUTO versus OFF.  See README.md; run it with tools/viewer-bench/run.sh.
#include <chrono>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

#include <wx/wx.h>
#include <wx/glcanvas.h>
#include <GL/gl.h>
#include <GL/glx.h>

#include "inv/SoWx/SoWx.H"
#include "inv/SoWx/SoWxRenderArea.H"
#include "inv/actions/SoSearchAction.H"
#include "inv/misc/SoChildList.H"
#include "inv/nodes/SoSeparator.H"
#include "inv/nodes/SoSwitch.H"
#include "inv/ChemKit/ChemInit.H"

#include "tdat/DisplayDescriptor.H"
#include "tdat/SingleGrid.H"

#include "viz/AtomNodesInit.H"
#include "viz/CSStyleCmd.H"
#include "viz/IsoSurfaceCmd.H"
#include "viz/NodesInit.H"
#include "viz/SGContainer.H"
#include "viz/SGFragment.H"

#include "wxviz/SGContainerManager.H"
#include "wxviz/SGSelection.H"
#include "wxviz/SGViewer.H"

using std::string;
using std::vector;
typedef std::chrono::steady_clock Clock;

namespace {

const int SIZE = 800;

//  Geometry in Angstrom, plain XYZ as Fragment::restoreXYZ reads it.
string waterXyz()
{
  return "3\nwater\n"
         "O 0.000000 0.000000 0.117300\n"
         "H 0.000000 0.757200 -0.469200\n"
         "H 0.000000 -0.757200 -0.469200\n";
}

string benzeneXyz()
{
  std::ostringstream o;
  o << "12\nbenzene\n";
  const double rc = 1.397, rh = 1.397 + 1.087;
  for (int i = 0; i < 6; i++) {
    double a = i * M_PI / 3.0;
    o << "C " << rc * cos(a) << " " << rc * sin(a) << " 0.0\n";
  }
  for (int i = 0; i < 6; i++) {
    double a = i * M_PI / 3.0;
    o << "H " << rh * cos(a) << " " << rh * sin(a) << " 0.0\n";
  }
  return o.str();
}

string crco6Xyz()
{
  std::ostringstream o;
  o << "13\nCr(CO)6\nCr 0.0 0.0 0.0\n";
  const double dc = 1.91, dco = 1.91 + 1.14;
  const int ax[6][3] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
  for (int i = 0; i < 6; i++)
    o << "C " << dc * ax[i][0] << " " << dc * ax[i][1] << " "
      << dc * ax[i][2] << "\n";
  for (int i = 0; i < 6; i++)
    o << "O " << dco * ax[i][0] << " " << dco * ax[i][1] << " "
      << dco * ax[i][2] << "\n";
  return o.str();
}

//  data/client/solvents/water216.xyz replicated 2x2x2 -> 5184 atoms.  The
//  file's OW/HW tags are not element symbols, so they are rewritten.
string waterBoxXyz(string& err)
{
  const char *home = getenv("ECCE_HOME");
  string path = string(home ? home : ".") + "/data/client/solvents/water216.xyz";
  FILE *f = fopen(path.c_str(), "r");
  if (!f) { err = "cannot open " + path; return ""; }
  int n = 0;
  double box = 0;
  char line[256];
  if (!fgets(line, sizeof line, f) || sscanf(line, "%d", &n) != 1 ||
      !fgets(line, sizeof line, f) || sscanf(line, "%lf", &box) != 1) {
    fclose(f);
    err = "bad header in " + path;
    return "";
  }
  struct A { char e; double x, y, z; };
  vector<A> atoms;
  while (fgets(line, sizeof line, f)) {
    char sym[16]; double x, y, z;
    if (sscanf(line, "%15s %lf %lf %lf", sym, &x, &y, &z) == 4)
      atoms.push_back({sym[0], x, y, z});
  }
  fclose(f);
  std::ostringstream o;
  o << atoms.size() * 8 << "\nwater box 2x2x2\n";
  for (int i = 0; i < 2; i++) for (int j = 0; j < 2; j++)
    for (int k = 0; k < 2; k++)
      for (size_t a = 0; a < atoms.size(); a++)
        o << atoms[a].e << " " << atoms[a].x + i * box << " "
          << atoms[a].y + j * box << " " << atoms[a].z + k * box << "\n";
  return o.str();
}

struct Config {
  string system;
  string style;      //  DisplayStyle name, as in the Builder's style menu
  bool   iso;        //  add the synthetic MO isosurface
};

struct Result {
  Config cfg;
  int atoms;
  string caching;
  int frames;
  double mean, p95;
};

//  glXSwapIntervalEXT(0): with vsync on every number would read 16.7 ms.
string disableVsync()
{
  typedef void (*SwapFn)(Display *, GLXDrawable, int);
  Display *dpy = glXGetCurrentDisplay();
  GLXDrawable drw = glXGetCurrentDrawable();
  if (!dpy || !drw) return "unknown (no GLX context)";
  SwapFn fn = (SwapFn)glXGetProcAddressARB(
      (const GLubyte *)"glXSwapIntervalEXT");
  if (!fn) return "glXSwapIntervalEXT unavailable (set vblank_mode=0 / "
                  "__GL_SYNC_TO_VBLANK=0)";
  fn(dpy, drw, 0);
  return "swap interval set to 0";
}

wxGLCanvas *findCanvas(wxWindow *w)
{
  if (wxGLCanvas *c = dynamic_cast<wxGLCanvas *>(w)) return c;
  for (wxWindowList::iterator it = w->GetChildren().begin();
       it != w->GetChildren().end(); ++it)
    if (wxGLCanvas *c = findCanvas(*it)) return c;
  return 0;
}

//  Per-separator field, so it covers every cache in the tree (the molecule
//  hangs off child separators of SGContainer, not off one top node).
void setCaching(SoNode *root, bool on)
{
  SoSearchAction sa;
  sa.setType(SoSeparator::getClassTypeId());
  sa.setInterest(SoSearchAction::ALL);
  sa.setSearchingAll(TRUE);
  sa.apply(root);
  const SoPathList &paths = sa.getPaths();
  for (int i = 0; i < paths.getLength(); i++) {
    SoSeparator *s = (SoSeparator *)paths[i]->getTail();
    s->renderCaching = on ? SoSeparator::AUTO : SoSeparator::OFF;
  }
}

//  Stand-in for a molecular orbital: a pi-type field over benzene (p_z
//  Gaussians, alternating sign).  The repo ships no cube file or finished
//  calculation, so this exercises the same ChemIso path through
//  IsoSurfaceCmd without needing one.
SingleGrid *syntheticPiGrid()
{
  const int N = 48;
  const double L = 6.0;
  SingleGrid *g = new SingleGrid();
  g->type("MO");
  g->name("synthetic pi");
  g->dimensions(N, N, N);
  g->origin(-L, -L, -L);
  g->corner(L, L, L);
  float *f = new float[N * N * N];
  const double rc = 1.397;
  for (int ix = 0; ix < N; ix++)
    for (int iy = 0; iy < N; iy++)
      for (int iz = 0; iz < N; iz++) {
        double x = -L + 2 * L * ix / (N - 1), y = -L + 2 * L * iy / (N - 1),
               z = -L + 2 * L * iz / (N - 1), v = 0;
        for (int a = 0; a < 6; a++) {
          double cx = rc * cos(a * M_PI / 3), cy = rc * sin(a * M_PI / 3);
          double r2 = (x-cx)*(x-cx) + (y-cy)*(y-cy) + z*z;
          v += ((a % 2) ? -1 : 1) * 0.5 * z * exp(-0.9 * r2);
        }
        f[(ix * N + iy) * N + iz] = (float)v;
      }
  g->setFieldData(f);
  g->findMinMax();
  return g;
}

}  // namespace


class BenchApp : public wxApp
{
public:
  virtual bool OnInit();
  virtual int OnRun();

private:
  int runFallbackCheck();
  bool loadScene(const Config &c, string &err);
  bool measure(const Config &c, bool caching, int warm, int frames,
               double budget, Result &r);
  string xyzFor(const string &sys, string &err);

  wxFrame *p_frame;
  SGViewer *p_viewer;
  SGContainerManager *p_mgr;
  SGContainer *p_sg;
  wxGLCanvas *p_canvas;
  int p_atoms;
};

IMPLEMENT_APP(BenchApp)


bool BenchApp::OnInit()
{
  p_frame = new wxFrame(NULL, wxID_ANY, "viewer-bench", wxDefaultPosition,
                        wxDefaultSize,
                        wxDEFAULT_FRAME_STYLE & ~(wxRESIZE_BORDER |
                                                  wxMAXIMIZE_BOX));
  SetTopWindow(p_frame);

  //  Same initialisation order as Builder::OnInit.
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
  p_viewer->SetMinSize(wxSize(SIZE, SIZE));
  sizer->Add(p_viewer, 1, wxEXPAND);
  p_frame->SetSizerAndFit(sizer);
  p_frame->Show(true);

  p_mgr->setSceneGraph("bench");
  p_sg = p_mgr->getSceneGraph();
  return p_sg != 0;
}


string BenchApp::xyzFor(const string &sys, string &err)
{
  if (sys == "water") return waterXyz();
  if (sys == "benzene") return benzeneXyz();
  if (sys == "Cr(CO)6") return crco6Xyz();
  return waterBoxXyz(err);
}


bool BenchApp::loadScene(const Config &c, string &err)
{
  string xyz = xyzFor(c.system, err);
  if (xyz.empty()) return false;

  //  A fresh container per scene: clearing the fragment in place while
  //  ChemDisplay still holds the old bond list crashes in generateIndices.
  static int serial = 0;
  p_mgr->removeSceneGraph();
  char name[32];
  snprintf(name, sizeof name, "bench%d", ++serial);
  p_mgr->setSceneGraph(name);
  p_sg = p_mgr->getSceneGraph();
  SGFragment *frag = p_sg->getFragment();

  std::istringstream in(xyz);
  if (!frag->restoreXYZ(in, 1.0, true)) {
    err = "restoreXYZ failed for " + c.system;
    return false;
  }
  frag->touchNumbers();
  p_sg->touchChemDisplay();
  p_atoms = frag->numAtoms();

  CSStyleCmd style("Style", p_sg);
  DisplayDescriptor dd("default", c.style, "Element");
  style.getParameter("descriptor")->setString(dd.toString());
  style.getParameter("all")->setBoolean(true);
  style.execute();

  if (c.iso) {
    p_sg->setCurrentGrid(syntheticPiGrid());
    IsoSurfaceCmd iso("Iso Surface", p_sg, 0);
    iso.execute();
  }

  p_viewer->viewAll();
  return true;
}


//  One timed frame = rotate camera, synchronous paint, glFinish.
bool BenchApp::measure(const Config &c, bool caching, int warm, int frames,
                       double budget, Result &r)
{
  setCaching(p_viewer->getTopNode(), caching);
  const SbRotation step(SbVec3f(0, 1, 0), 2.0f * (float)M_PI / frames);

  vector<double> ms;
  Clock::time_point t0 = Clock::now();
  for (int i = 0; i < warm + frames; i++) {
    Clock::time_point a = Clock::now();
    p_viewer->rotateCamera(step);
    p_canvas->Refresh(false);
    p_canvas->Update();
    glFinish();
    Clock::time_point b = Clock::now();
    if (i >= warm) {
      ms.push_back(std::chrono::duration<double, std::milli>(b - a).count());
      //  Stop early on slow (software) GL so the whole run stays short.
      if (ms.size() >= 20 &&
          std::chrono::duration<double>(b - t0).count() > budget)
        break;
    }
  }
  if (ms.empty()) return false;

  double sum = 0;
  for (size_t i = 0; i < ms.size(); i++) sum += ms[i];
  std::sort(ms.begin(), ms.end());
  r.cfg = c;
  r.atoms = p_atoms;
  r.caching = caching ? "AUTO" : "OFF";
  r.frames = (int)ms.size();
  r.mean = sum / ms.size();
  r.p95 = ms[std::min(ms.size() - 1, (size_t)ceil(0.95 * ms.size()) - 1)];
  return true;
}


//  BENCH_FALLBACK=1: for each scene ask for the lobe transparency mode the
//  way the MO panel does and report whether the render area fell back to
//  quick mode.  ECCE_TRANSPARENCY_FALLBACK_MS moves the threshold.
int BenchApp::runFallbackCheck()
{
  SoWxRenderArea *ra = dynamic_cast<SoWxRenderArea *>(p_canvas);
  if (!ra) { fprintf(stderr, "canvas is not a SoWxRenderArea\n"); return 2; }
  const Config configs[] = {
    {"water", "Ball And Stick", true}, {"benzene", "Ball And Stick", true},
    {"Cr(CO)6", "Ball And Stick", true}, {"water box", "Ball And Stick", true},
  };
  int bad = 0;
  const char *lim = getenv("ECCE_TRANSPARENCY_FALLBACK_MS");
  GLint ab = 0;
  glGetIntegerv(GL_ALPHA_BITS, &ab);
  printf("alpha bits %d, threshold %s ms\n", (int)ab, lim ? lim : "100 (default)");
  for (size_t i = 0; i < sizeof configs / sizeof configs[0]; i++) {
    string err;
    if (!loadScene(configs[i], err)) { fprintf(stderr, "%s\n", err.c_str()); return 2; }
    p_viewer->setTransparencyType(SoGLRenderAction::SCREEN_DOOR);
    const SbRotation step(SbVec3f(0, 1, 0), 0.1f);
    Clock::time_point t0 = Clock::now();
    int n = 0;
    for (; n < 12; n++) {
      p_viewer->rotateCamera(step);
      p_canvas->Refresh(false);
      p_canvas->Update();
      glFinish();
    }
    double ms = std::chrono::duration<double, std::milli>(Clock::now() - t0).count() / n;
    bool quick = ra->getTransparencyType() != SoGLRenderAction::SORTED_LAYERS_BLEND;
    printf("%-10s %6d atoms +iso: %7.1f ms/frame  mode %s\n",
           configs[i].system.c_str(), p_atoms, ms,
           quick ? "QUICK (fell back)" : "ACCURATE");
  }
  return bad;
}


int BenchApp::OnRun()
{
  const char *e = getenv("BENCH_FRAMES");
  int frames = e ? atoi(e) : 180;
  e = getenv("BENCH_WARMUP");
  int warm = e ? atoi(e) : 10;
  e = getenv("BENCH_SECONDS");
  double budget = e ? atof(e) : 4.0;

  //  Let the window map and the first paint create the GL context.
  for (int i = 0; i < 50; i++) { Yield(true); wxMilliSleep(10); }
  p_canvas = findCanvas(p_viewer);
  if (!p_canvas) { fprintf(stderr, "no GL canvas found\n"); return 2; }
  p_canvas->Refresh(false);
  p_canvas->Update();
  Yield(true);

  if (getenv("BENCH_FALLBACK")) return runFallbackCheck();

  const char *vendor = (const char *)glGetString(GL_VENDOR);
  const char *renderer = (const char *)glGetString(GL_RENDERER);
  const char *version = (const char *)glGetString(GL_VERSION);
  string vsync = disableVsync();
  wxSize cs = p_canvas->GetClientSize();

  const Config configs[] = {
    {"water",   "Ball And Stick", false}, {"water",   "CPK", false},
    {"benzene", "Ball And Stick", false}, {"benzene", "CPK", false},
    {"Cr(CO)6", "Ball And Stick", false}, {"Cr(CO)6", "CPK", false},
    {"benzene", "Ball And Stick", true},
    {"water box", "Ball And Stick", false}, {"water box", "CPK", false},
  };

  //  BENCH_STYLES="CPK,Stick,...": only the water box, in those styles,
  //  caching AUTO (for profiling one style at a time).
  vector<Config> todo(configs, configs + sizeof configs / sizeof configs[0]);
  const char *only = getenv("BENCH_STYLES");
  if (only) {
    todo.clear();
    std::istringstream ss(only);
    string st;
    while (std::getline(ss, st, ','))
      if (!st.empty()) todo.push_back(Config{"water box", st, false});
  }

  vector<Result> results;
  for (size_t i = 0; i < todo.size(); i++) {
    string err;
    if (!loadScene(todo[i], err)) {
      fprintf(stderr, "skipping %s: %s\n", todo[i].system.c_str(),
              err.c_str());
      continue;
    }
    for (int cache = 1; cache >= (only ? 1 : 0); cache--) {
      Result r;
      if (measure(todo[i], cache != 0, warm, frames, budget, r))
        results.push_back(r);
      Yield(true);
    }
  }

  printf("GL_VENDOR   : %s\nGL_RENDERER : %s\nGL_VERSION  : %s\n",
         vendor ? vendor : "?", renderer ? renderer : "?",
         version ? version : "?");
  printf("canvas      : %dx%d   vsync: %s\n", cs.x, cs.y, vsync.c_str());
  printf("frames/run  : up to %d after %d warm-up (rotation 360 deg over "
         "that many frames), %.0f s cap\n\n", frames, warm, budget);
  printf("%-10s %6s  %-15s %-7s %6s %10s %10s\n", "system", "atoms",
         "style", "caching", "frames", "mean ms", "p95 ms");
  for (size_t i = 0; i < results.size(); i++) {
    const Result &r = results[i];
    string sys = r.cfg.system + (r.cfg.iso ? "+MO" : "");
    printf("%-10s %6d  %-15s %-7s %6d %10.3f %10.3f\n", sys.c_str(), r.atoms,
           r.cfg.style.c_str(), r.caching.c_str(), r.frames, r.mean, r.p95);
  }
  fflush(stdout);
  return 0;
}
