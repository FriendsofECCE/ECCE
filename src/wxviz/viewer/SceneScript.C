#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <sstream>

#include <wx/wx.h>
#include <wx/glcanvas.h>
#include <GL/gl.h>

#include "inv/SoOffscreenRenderer.H"
#include "inv/SoWx/SoWxRenderArea.H"
#include "inv/nodes/SoSwitch.H"
#include "inv/misc/SoChildList.H"

#include "dsm/ICalculation.H"
#include "dsm/IPropCalculation.H"
#include "dsm/JCode.H"

#include "tdat/DisplayDescriptor.H"
#include "tdat/SingleGrid.H"
#include "tdat/TAtm.H"

#include "viz/AtomLabelsCmd.H"
#include "viz/ComputeMoCmd.H"
#include "viz/CSStyleCmd.H"
#include "viz/GTStepCmd.H"
#include "viz/IsoSurfaceCmd.H"
#include "viz/IsoValueCmd.H"
#include "viz/NModeStepCmd.H"
#include "viz/NModeTraceCmd.H"
#include "viz/NModeVectCmd.H"
#include "viz/SGContainer.H"
#include "viz/SGFragment.H"
#include "viz/SurfDisplayTypeCmd.H"

#include "inv/ChemKit/ChemDisplayPath.H"
#include "inv/ChemKit/MFVec2i.H"

#include "inv/SbViewportRegion.H"
#include "inv/SoSceneManager.H"
#include "inv/actions/SoGLRenderAction.H"
#include "inv/events/SoLocation2Event.H"
#include "inv/events/SoMouseButtonEvent.H"
#include "inv/nodes/SoCamera.H"
#include "viz/TwoDMoveCmd.H"
#include "dsm/ChemistryTask.H"
#include "dsm/EDSIFactory.H"
#include "util/SFile.H"
#include "util/TempStorage.H"
#include "wxviz/VizRender.H"

#include "wxviz/MotionListener.H"
#include "wxviz/SceneScript.H"
#include "wxviz/SGSelection.H"
#include "wxviz/SGViewer.H"

SceneScript::SceneScript(SGViewer *viewer, SGContainer *sg,
                         IPropCalculation *calc, const string& outdir)
  : p_viewer(viewer), p_sg(sg), p_calc(calc), p_outdir(outdir)
{
}

bool SceneScript::run(const string& scriptPath)
{
  std::ifstream in(scriptPath.c_str());
  if (!in) return fail("cannot open " + scriptPath);
  string line;
  int n = 0;
  while (std::getline(in, line)) {
    n++;
    size_t h = line.find('#');
    if (h != string::npos) line.erase(h);
    std::istringstream ss(line);
    vector<string> w;
    string t;
    while (ss >> t) w.push_back(t);
    if (w.empty()) continue;
    string rest;
    size_t p = line.find(w[0]) + w[0].size();
    size_t q = line.find_first_not_of(" \t", p);
    if (q != string::npos) rest = line.substr(q);
    while (!rest.empty() && isspace((unsigned char)rest.back())) rest.erase(rest.size() - 1);
    if (!exec(w, rest)) {
      char buf[32];
      snprintf(buf, sizeof buf, " (line %d)", n);
      p_msg += buf;
      return false;
    }
  }
  return true;
}

bool SceneScript::exec(const vector<string>& w, const string& rest)
{
  const string& c = w[0];
  if (getenv("SCENE_DEBUG")) fprintf(stderr, "SCENE exec: %s\n", rest.empty() ? c.c_str() : (c + " " + rest).c_str());
  SGFragment *frag = p_sg->getFragment();

  if (c == "style") {
    CSStyleCmd cmd("Style", p_sg);
    DisplayDescriptor dd("default", rest, "Element");
    cmd.getParameter("descriptor")->setString(dd.toString());
    cmd.getParameter("all")->setBoolean(true);
    cmd.execute();
  } else if (c == "labels") {
    int type = AtomLabelsCmd::NONE;
    if (rest == "element") type = AtomLabelsCmd::ELEMENT;
    else if (rest == "name") type = AtomLabelsCmd::ATOMNAME;
    else if (rest != "none") return fail("labels: unknown type " + rest);
    //  Labels apply to the selection if there is one; clear it first.
    frag->m_atomHighLight.clear();
    frag->m_bondHighLight.clear();
    AtomLabelsCmd cmd("AtomLabel", p_sg);
    cmd.getParameter("type")->setInteger(type);
    cmd.execute();
    p_sg->touchChemDisplay();
  } else if (c == "select") {
    //  Same route as WxVizTool::setSelection: the atoms go into the
    //  fragment's highlight list and into the viewer's selection node, which
    //  is what draws the highlight.
    vector<int> atoms;
    for (size_t i = 1; i < w.size(); i++) atoms.push_back(atoi(w[i].c_str()) - 1);
    if (atoms.empty()) return fail("select: no atoms");
    frag->m_atomHighLight = atoms;
    frag->m_bondHighLight.clear();
    SGSelection *esel = p_viewer->getSel();
    esel->deselectAll();
    vector<string> names = p_sg->getDisplayStyleNames();
    int found = 0;
    for (size_t ds = 0; ds < names.size(); ds++) {
      SoPath *pathCD = p_sg->getSelectionPath(esel, ds);
      if (!pathCD) continue;
      MFVec2i lAtoms;
      int n = 0;
      for (size_t j = 0; j < atoms.size(); j++)
        if (frag->atomRef(atoms[j])->displayStyle().getName() == names[ds])
          lAtoms.set1Value(n++, SbVec2i(atoms[j], 1));
      if (n == 0) continue;
      ChemDisplayPath *cpt = new ChemDisplayPath;
      cpt->ref();
      cpt->setPath(pathCD, &lAtoms, NULL, NULL, NULL);
      esel->merge(cpt);
      found += n;
    }
    if (!found) return fail("select: atoms not in any display");
  } else if (c == "deselect") {
    frag->m_atomHighLight.clear();
    frag->m_bondHighLight.clear();
    p_viewer->getSel()->deselectAll();
  } else if (c == "mo") {
    ICalculation *ic = dynamic_cast<ICalculation*>(p_calc);
    if (!ic || w.size() < 2) return fail("mo: needs a calculation and an MO number");
    double iso = w.size() > 2 ? atof(w[2].c_str()) : 0.05;
    int res = w.size() > 3 ? atoi(w[3].c_str()) : 40;
    //  Grid box: atom extent plus 3.5 Angstrom.
    double lo[3] = {1e9, 1e9, 1e9}, hi[3] = {-1e9, -1e9, -1e9};
    for (int i = 0; i < frag->numAtoms(); i++) {
      const double *x = frag->atomRef(i)->coordinates();
      for (int k = 0; k < 3; k++) {
        if (x[k] < lo[k]) lo[k] = x[k];
        if (x[k] > hi[k]) hi[k] = x[k];
      }
    }
    ComputeMoCmd mo("Compute Mo", p_sg, p_calc, 0);
    mo.getParameter("FieldType")->setString("MO");
    mo.getParameter("CoefCutoff")->setDouble(0.0);
    mo.getParameter("SelectedMO")->setInteger(atoi(w[1].c_str()));
    mo.getParameter("Type")->setString("alpha");
    mo.getParameter("Code")->setString(ic->application()->name());
    const char *ax[3] = {"X", "Y", "Z"};
    for (int k = 0; k < 3; k++) {
      mo.getParameter(string("res") + ax[k])->setInteger(res);
      mo.getParameter(string("from") + ax[k])->setDouble(lo[k] - 3.5);
      mo.getParameter(string("to") + ax[k])->setDouble(hi[k] + 3.5);
    }
    mo.execute();
    SingleGrid *grid = p_sg->getCurrentGrid();
    if (!grid || !(grid->fieldMax() > grid->fieldMin()))
      return fail("mo: no field computed");
    IsoSurfaceCmd surf("Iso Surface", p_sg, p_calc);
    surf.getParameter("transparency")->setDouble(0.5);
    surf.getParameter("positiveRed")->setDouble(1.0);
    surf.getParameter("positiveGreen")->setDouble(0.0);
    surf.getParameter("positiveBlue")->setDouble(0.0);
    surf.getParameter("negativeRed")->setDouble(0.0);
    surf.getParameter("negativeGreen")->setDouble(1.0);
    surf.getParameter("negativeBlue")->setDouble(0.0);
    surf.execute();
    SurfDisplayTypeCmd type("Surface Type", p_sg, p_calc);
    type.getParameter("IsosurfStyle")->setString("Solid");
    type.execute();
    IsoValueCmd val("Iso Value", p_sg, p_calc);
    val.getParameter("Value")->setDouble(log10(iso));   // a log10 slider value
    val.getParameter("transparency")->setDouble(0.5);
    val.getParameter("positiveRed")->setDouble(1.0);
    val.getParameter("positiveGreen")->setDouble(0.0);
    val.getParameter("positiveBlue")->setDouble(0.0);
    val.getParameter("negativeRed")->setDouble(0.0);
    val.getParameter("negativeGreen")->setDouble(1.0);
    val.getParameter("negativeBlue")->setDouble(0.0);
    val.execute();
  } else if (c == "isotest") {
    //  Synthetic pi-type field over benzene (alternating p_z Gaussians), for
    //  the isosurface path without a calculation.
    const int N = 48;
    const double L = 6.0, rc = 1.397;
    SingleGrid *g = new SingleGrid();
    g->type("MO");
    g->name("synthetic pi");
    g->dimensions(N, N, N);
    g->origin(-L, -L, -L);
    g->corner(L, L, L);
    float *f = new float[N * N * N];
    for (int ix = 0; ix < N; ix++)
      for (int iy = 0; iy < N; iy++)
        for (int iz = 0; iz < N; iz++) {
          double x = -L + 2 * L * ix / (N - 1), y = -L + 2 * L * iy / (N - 1),
                 z = -L + 2 * L * iz / (N - 1), v = 0;
          for (int a = 0; a < 6; a++) {
            double cx = rc * cos(a * M_PI / 3), cy = rc * sin(a * M_PI / 3);
            double r2 = (x - cx) * (x - cx) + (y - cy) * (y - cy) + z * z;
            v += ((a % 2) ? -1 : 1) * 0.5 * z * exp(-0.9 * r2);
          }
          f[(ix * N + iy) * N + iz] = (float)v;
        }
    g->setFieldData(f);
    g->findMinMax();
    p_sg->setCurrentGrid(g);
    IsoSurfaceCmd surf("Iso Surface", p_sg, 0);
    surf.getParameter("transparency")->setDouble(0.5);
    surf.getParameter("positiveRed")->setDouble(1.0);
    surf.getParameter("positiveGreen")->setDouble(0.0);
    surf.getParameter("positiveBlue")->setDouble(0.0);
    surf.getParameter("negativeRed")->setDouble(0.0);
    surf.getParameter("negativeGreen")->setDouble(1.0);
    surf.getParameter("negativeBlue")->setDouble(0.0);
    surf.execute();
    IsoValueCmd val("Iso Value", p_sg, 0);
    val.getParameter("Value")->setDouble(log10(0.05));   // log10 slider value
    val.getParameter("transparency")->setDouble(0.5);
    val.execute();
  } else if (c == "nmvect" || c == "nmstep") {
    if (!p_calc || w.size() < 2) return fail(c + ": needs a calculation and a mode");
    int mode = atoi(w[1].c_str());
    if (c == "nmvect") {
      NModeVectCmd v("Normal Mode Vectors", p_sg, p_calc);
      v.getParameter("Mode")->setInteger(mode);
      v.getParameter("Color")->setString("#cccc00");
      v.getParameter("Sign")->setBoolean(false);
      if (!v.execute()) return fail("nmvect: no VIB data");
      p_sg->getNMRoot()->whichChild.setValue(SO_SWITCH_NONE);
      p_sg->getNMVecRoot()->whichChild.setValue(SO_SWITCH_ALL);
    } else {
      NModeTraceCmd t("Normal Mode Animation", p_sg, p_calc);
      t.getParameter("Mode")->setInteger(mode);
      t.execute();
      NModeStepCmd s("Normal Mode Step", p_sg, p_calc);
      s.getParameter("Index")->setInteger(w.size() > 2 ? atoi(w[2].c_str()) : 0);
      s.execute();
      p_sg->getNMVecRoot()->whichChild.setValue(SO_SWITCH_NONE);
    }
  } else if (c == "gtstep") {
    if (!p_calc || w.size() < 2) return fail("gtstep: needs a calculation and a step");
    GTStepCmd g("Trace Step", p_sg, p_calc);
    g.getParameter("Index")->setInteger(atoi(w[1].c_str()));
    g.getParameter("PropKey")->setString("GEOMTRACE");
    g.execute();
    p_sg->touchChemDisplay();
  } else if (c == "transparency" && w.size() == 2) {
    SoGLRenderAction::TransparencyType t;
    if (w[1] == "SCREEN_DOOR") t = SoGLRenderAction::SCREEN_DOOR;
    else if (w[1] == "DELAYED_ADD") t = SoGLRenderAction::DELAYED_ADD;
    else if (w[1] == "SORTED_OBJECT_BLEND") t = SoGLRenderAction::SORTED_OBJECT_BLEND;
    else return fail("transparency: unknown mode " + w[1]);
    p_viewer->setTransparencyType(t);
  } else if (c == "pick" && w.size() >= 3) {
    return pickAtoms(w[1], vector<string>(w.begin() + 2, w.end()));
  } else if (c == "drag" && w.size() == 5) {
    return dragAtom(w[1], atoi(w[2].c_str()), atoi(w[3].c_str()), atoi(w[4].c_str()));
  } else if (c == "vizthumb" && w.size() == 4) {
    //  vizthumbnail's own entry point: VizRender::thumbnail(url, ...) builds
    //  the container from the stored calculation, renders offscreen, stores
    //  the JPEG on the task; read it back from the data server.
    if (!p_calc) return fail("vizthumb: needs a calculation");
    string url = p_calc->getURL().toString();
    int tw = atoi(w[2].c_str()), th = atoi(w[3].c_str());
    if (!VizRender::thumbnail(url, tw, th, 0.0, 0.0, 0.0)) {
      FILE *u = fopen((p_outdir + "/" + w[1] + ".UNAVAILABLE").c_str(), "w");
      if (u) { fprintf(u, "%s\n", VizRender::msg().c_str()); fclose(u); }
      return true;
    }
    ChemistryTask *task = dynamic_cast<ChemistryTask *>(EDSIFactory::getResource(url));
    SFile *tmp = TempStorage::getTempFile();
    if (!task || !task->getThumbnail(tmp)) return fail("vizthumb: thumbnail not stored");
    string cmd = "cp '" + tmp->path() + "' '" + p_outdir + "/" + w[1] + ".jpg'";
    if (system(cmd.c_str()) != 0) return fail("vizthumb: copy failed");
  } else if (c == "redraws" && w.size() >= 3) {
    return countRedraws(w[1], vector<string>(w.begin() + 2, w.end()));
  } else if (c == "viewall") {
    p_viewer->viewAll();
  } else if (c == "clear") {
    p_sg->clearGridScene();
    p_sg->getNMVecRoot()->whichChild.setValue(SO_SWITCH_NONE);
    p_sg->getNMRoot()->whichChild.setValue(SO_SWITCH_NONE);
  } else if (c == "snap" && w.size() == 2) {
    return snapshot(w[1], 480, 0.2f, 0.3f, 0.4f);
  } else if (c == "thumb" && w.size() == 2) {
    return snapshot(w[1], 64, 0.0f, 0.0f, 0.0f);
  } else {
    return fail("unknown command: " + c);
  }
  return true;
}

namespace {

struct Frame {
  vector<unsigned char> px;
  int w, h;
  bool got;
  Frame() : w(0), h(0), got(false) {}
};

//  Runs inside the render area's redraw, after the scene is drawn and
//  before the swap, so the back buffer is the finished frame.
void grabFrame(void *p)
{
  Frame *f = (Frame *)p;
  GLint vp[4];
  glGetIntegerv(GL_VIEWPORT, vp);
  f->w = vp[2];
  f->h = vp[3];
  f->px.resize((size_t)f->w * f->h * 3);
  glFinish();
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(vp[0], vp[1], f->w, f->h, GL_RGB, GL_UNSIGNED_BYTE, &f->px[0]);
  f->got = true;
}

SoWxRenderArea *findRenderArea(wxWindow *w)
{
  if (SoWxRenderArea *a = dynamic_cast<SoWxRenderArea *>(w)) return a;
  for (wxWindowList::iterator it = w->GetChildren().begin();
       it != w->GetChildren().end(); ++it)
    if (SoWxRenderArea *a = findRenderArea(*it)) return a;
  return 0;
}

void writePpm(const string& path, int w, int h, const unsigned char *rgb)
{
  FILE *f = fopen(path.c_str(), "wb");
  if (!f) return;
  fprintf(f, "P6\n%d %d\n255\n", w, h);
  //  GL rows are bottom-up
  for (int y = h - 1; y >= 0; y--) fwrite(rgb + (size_t)y * w * 3, 1, (size_t)w * 3, f);
  fclose(f);
}

}  // namespace


namespace {

SbVec2s atomPixel(SGViewer *v, SGFragment *frag, int atom)
{
  SbVec3f p = frag->getAtomCoordinates(atom), d;
  const SbViewportRegion& vpr = v->getViewportRegion();
  SbViewVolume vv = v->getCamera()->getViewVolume(vpr.getViewportAspectRatio());
  vv.projectToScreen(p, d);
  SbVec2s sz = vpr.getViewportSizePixels();
  return SbVec2s((short)(d[0] * sz[0] + 0.5f), (short)(d[1] * sz[1] + 0.5f));
}

void sendMouse(SoSceneManager *m, bool down, SbVec2s pos, double t)
{
  SoMouseButtonEvent e;
  e.setButton(SoMouseButtonEvent::BUTTON1);
  e.setState(down ? SoButtonEvent::DOWN : SoButtonEvent::UP);
  e.setPosition(pos);
  e.setTime(SbTime(t));
  m->processEvent(&e);
}

void sendMove(SoSceneManager *m, SbVec2s pos, double t)
{
  SoLocation2Event e;
  e.setPosition(pos);
  e.setTime(SbTime(t));
  m->processEvent(&e);
}

//  What the Builder does on a plain selection pick (Builder::
//  selectionChangeCB reads the selection back into the fragment).
void finishCB(void *data, ChemSelection *sel)
{
  SGFragment *frag = (SGFragment *)data;
  ((SGSelection *)sel)->readSelection(frag);
}

//  Builder::motionChanged, minus the command manager.
class DragListener : public MotionListener
{
 public:
  DragListener(SGContainer *sg) : p_sg(sg) {}
  virtual void motionChanged(const MotionData& d)
  {
    SGFragment *frag = p_sg->getFragment();
    if (d.isButton1() && frag && frag->m_atomHighLight.size() > 0) {
      TwoDMoveCmd cmd("Translate", p_sg);
      cmd.getParameter("deltax")->setDouble(d.getDeltaX());
      cmd.getParameter("deltay")->setDouble(d.getDeltaY());
      cmd.getParameter("deltaz")->setDouble(d.getDeltaZ());
      cmd.getParameter("movez")->setBoolean(d.wasShiftDown());
      cmd.getParameter("doundo")->setBoolean(d.wasStartMotion());
      cmd.execute();
    }
    p_sg->adjustAtomContainers();
  }
 private:
  SGContainer *p_sg;
};

}  // namespace

//  "pick <name> <atom>...": click each atom (1-based) through the viewer's
//  scene manager, the path a wx mouse event takes once translated to an
//  SoEvent (SoHandleEventAction -> SGSelection::handleEvent -> its own
//  SoRayPickAction).  Writes the selection after each click.
bool SceneScript::pickAtoms(const string& name, const vector<string>& atoms)
{
  SoWxRenderArea *area = findRenderArea(p_viewer);
  SGFragment *frag = p_sg->getFragment();
  if (!area || !frag) return fail("pick: no render area");
  static bool wired = false;
  if (!wired) {
    p_viewer->getSel()->addFinishCallback(finishCB, frag);
    wired = true;
  }
  wxTheApp->Yield(true);
  FILE *o = fopen((p_outdir + "/" + name + ".txt").c_str(), "w");
  if (!o) return fail("pick: cannot write output");
  double t = 100.0;
  for (size_t i = 0; i < atoms.size(); i++) {
    int a = atoi(atoms[i].c_str()) - 1;
    SbVec2s pos = atomPixel(p_viewer, frag, a);
    frag->m_atomHighLight.clear();
    sendMouse(area->getSceneManager(), true, pos, t);
    sendMouse(area->getSceneManager(), false, pos, t + 0.05);
    t += 1.0;
    fprintf(o, "click atom %d at (%d,%d): selected", a + 1, pos[0], pos[1]);
    for (size_t k = 0; k < frag->m_atomHighLight.size(); k++)
      fprintf(o, " %d", frag->m_atomHighLight[k] + 1);
    fprintf(o, "\n");
    p_viewer->getSel()->deselectAll();
  }
  fclose(o);
  return true;
}

//  "drag <name> <atom> <dx> <dy>": click the atom, then press on it, hold
//  past the drag delay, move by (dx,dy) pixels in 5 steps and release; the
//  motion goes to a listener that does what Builder::motionChanged does.
//  Writes the atom's coordinates before and after.
bool SceneScript::dragAtom(const string& name, int atom1, int dx, int dy)
{
  SoWxRenderArea *area = findRenderArea(p_viewer);
  SGFragment *frag = p_sg->getFragment();
  if (!area || !frag) return fail("drag: no render area");
  static DragListener *listener = 0;
  static bool wired = false;
  if (!wired) {
    listener = new DragListener(p_sg);
    p_viewer->getSel()->addFinishCallback(finishCB, frag);
    p_viewer->getSel()->addMotionListener(listener);
    wired = true;
  }
  p_viewer->setSelectModeDrag(true);
  wxTheApp->Yield(true);
  SoSceneManager *m = area->getSceneManager();
  int a = atom1 - 1;
  SbVec2s pos = atomPixel(p_viewer, frag, a);
  FILE *o = fopen((p_outdir + "/" + name + ".txt").c_str(), "w");
  if (!o) return fail("drag: cannot write output");
  const double *x = frag->atomRef(a)->coordinates();
  fprintf(o, "atom %d before %.3f %.3f %.3f at (%d,%d)\n", atom1, x[0], x[1], x[2], pos[0], pos[1]);

  frag->m_atomHighLight.clear();
  sendMouse(m, true, pos, 200.0);
  sendMouse(m, false, pos, 200.05);
  fprintf(o, "selected:");
  for (size_t k = 0; k < frag->m_atomHighLight.size(); k++) fprintf(o, " %d", frag->m_atomHighLight[k] + 1);
  fprintf(o, "\n");

  double t = 300.0;
  sendMouse(m, true, pos, t);
  t += 0.5;                                   // past the 333 ms delay
  sendMove(m, pos, t);
  for (int i = 1; i <= 5; i++) {
    t += 0.05;
    sendMove(m, SbVec2s(pos[0] + dx * i / 5, pos[1] + dy * i / 5), t);
  }
  sendMouse(m, false, SbVec2s(pos[0] + dx, pos[1] + dy), t + 0.05);
  wxTheApp->Yield(true);
  x = frag->atomRef(a)->coordinates();
  fprintf(o, "atom %d after %.3f %.3f %.3f\n", atom1, x[0], x[1], x[2]);
  fclose(o);
  return true;
}

namespace {
void countFrame(void *p) { ++*(int *)p; }
}

//  "redraws <name> <step>...": N geometry-trace steps, one scene change
//  each, letting the wx loop idle between them but never forcing a paint
//  (no Refresh/Update, unlike snap); writes how many frames each produced.
bool SceneScript::countRedraws(const string& name, const vector<string>& steps)
{
  SoWxRenderArea *area = findRenderArea(p_viewer);
  if (!area || !p_calc) return fail("redraws: needs a calculation");
  int frames = 0;
  for (int i = 0; i < 20; i++) wxTheApp->Yield(true);     // settle
  area->setFrameCallback(countFrame, &frames);
  FILE *o = fopen((p_outdir + "/" + name + ".txt").c_str(), "w");
  if (!o) return fail("redraws: cannot write output");
  int total = 0;
  for (size_t i = 0; i < steps.size(); i++) {
    int before = frames;
    GTStepCmd g("Trace Step", p_sg, p_calc);
    g.getParameter("Index")->setInteger(atoi(steps[i].c_str()));
    g.getParameter("PropKey")->setString("GEOMTRACE");
    g.execute();
    p_sg->touchChemDisplay();
    for (int k = 0; k < 60 && frames == before; k++) {
      wxTheApp->Yield(true);
      wxMilliSleep(5);
    }
    wxTheApp->Yield(true);
    fprintf(o, "step %s: %d frame(s)\n", steps[i].c_str(), frames - before);
    if (frames > before) total++;
  }
  area->setFrameCallback(0, 0);
  fprintf(o, "changes %d, changes that produced a render %d\n", (int)steps.size(), total);
  fclose(o);
  fprintf(stderr, "redraws %s: %d changes, %d rendered\n", name.c_str(), (int)steps.size(), total);
  return true;
}

//  "snap": the real canvas, read back with glReadPixels at its own size.
//  "thumb": the offscreen renderer the calculation thumbnail uses; where it
//  cannot make a context (Coin under Xvfb) a .UNAVAILABLE note is written
//  instead of failing the run.
bool SceneScript::snapshot(const string& name, int size, float r, float g,
                           float b)
{
  if (size == 480) {
    SoWxRenderArea *area = findRenderArea(p_viewer);
    if (!area) return fail("no render area");
    Frame f;
    area->setFrameCallback(grabFrame, &f);
    for (int i = 0; i < 20 && !f.got; i++) {
      wxTheApp->Yield(true);
      area->Refresh(false);
      area->Update();
    }
    area->setFrameCallback(0, 0);
    if (!f.got) return fail("canvas never painted: " + name);
    writePpm(p_outdir + "/" + name + ".ppm", f.w, f.h, &f.px[0]);
    return true;
  }

  //  Kept alive for the process: the vendored class crashes in Mesa when
  //  it is destroyed, and the thumbnail path holds one for the same reason.
  static SoOffscreenRenderer *rend = 0;
  SbViewportRegion vp;
  vp.setWindowSize(SbVec2s(size, size));
  if (!rend) rend = new SoOffscreenRenderer(vp);
  rend->setComponents(SoOffscreenRenderer::RGB);
  rend->setBackgroundColor(SbColor(r, g, b));
  if (!rend->render(p_viewer->getTopNode()) || !rend->getBuffer()) {
    std::ofstream(p_outdir + "/" + name + ".UNAVAILABLE")
        << "SoOffscreenRenderer::render failed\n";
    return true;
  }
  writePpm(p_outdir + "/" + name + ".ppm", size, size, rend->getBuffer());
  return true;
}
