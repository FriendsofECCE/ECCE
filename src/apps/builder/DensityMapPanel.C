#include <algorithm>
#include <cmath>
#include <cstdio>

#include <wx/bitmap.h>
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/dcbuffer.h>
#include <wx/dcmemory.h>
#include <wx/filedlg.h>
#include <wx/image.h>
#include <wx/link.h>
#include <wx/msgdlg.h>
#include <wx/progdlg.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/stopwatch.h>

#include "dsm/IPropCalculation.H"
#include "inv/nodes/SoCamera.H"
#include "tdat/Fragment.H"
#include "util/Event.H"
#include "viz/SGContainer.H"
#include "viz/SGFragment.H"
#include "viz/SGPlane.H"
#include "viz/StyledPlane.H"
#include "wxviz/SGViewer.H"

#include "DensityMapDraw.H"
#include "DensityMapPanel.H"

using namespace DensityPlane;

namespace {

const wxWindowID ID_SHOW = wxNewId(), ID_SPIN = wxNewId(), ID_MO = wxNewId(),
                 ID_PLANE = wxNewId(), ID_ATOM = wxNewId(),
                 ID_PNG = wxNewId(), ID_CSV = wxNewId(),
                 ID_OFFSET = wxNewId(), ID_VIEW = wxNewId();

const char *PLANE_NAME = "DensityMapPlane";
const double MARGIN = 1.5;            // A beyond the atoms' extent
const double COARSE_STEP = 0.15;      // while the slider is dragged
const int OFFSET_TENTHS = 50;         // slider range, +-5.0 A

}  // namespace


/** Draws a Map; knows nothing of the calculation. */
class DensityMapCanvas : public wxPanel
{
  public:
    DensityMapCanvas(wxWindow *parent)
      : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                wxFULL_REPAINT_ON_RESIZE),
        p_have(false)
    {
      SetBackgroundStyle(wxBG_STYLE_PAINT);
      SetMinSize(wxSize(300, 260));
      Bind(wxEVT_PAINT, &DensityMapCanvas::onPaint, this);
    }

    void show(const Map& m) { p_map = m; p_have = true; p_message.clear(); Refresh(); }
    void say(const wxString& msg) { p_have = false; p_message = msg; Refresh(); }

    void draw(wxDC& dc, const wxSize& size) const
    {
      paintDensityMap(dc, size, p_have, p_map, p_message);
    }

  private:
    void onPaint(wxPaintEvent&)
    {
      wxAutoBufferedPaintDC dc(this);
      draw(dc, GetClientSize());
    }

    Map p_map;
    bool p_have;
    wxString p_message;
};


IMPLEMENT_DYNAMIC_CLASS(DensityMapPanel, VizPropertyPanel)
wxFORCE_LINK_THIS_MODULE(DensityMapPanel)

DensityMapPanel::DensityMapPanel()
  : VizPropertyPanel(), p_haveMap(false), p_haveView(false), p_busy(false),
    p_inViewer(false), p_dragging(false), p_show(0), p_spin(0), p_plane(0),
    p_atomA(0), p_atomB(0), p_atomC(0), p_mo(0), p_offset(0),
    p_offsetText(0), p_status(0), p_useView(0), p_canvas(0)
{
}

DensityMapPanel::DensityMapPanel(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
  : VizPropertyPanel(), p_haveMap(false), p_haveView(false), p_busy(false),
    p_inViewer(false), p_dragging(false), p_show(0), p_spin(0), p_plane(0),
    p_atomA(0), p_atomB(0), p_atomC(0), p_mo(0), p_offset(0),
    p_offsetText(0), p_status(0), p_useView(0), p_canvas(0)
{
  Create(calculation, parent, id, pos, size, style, name);
}

DensityMapPanel::~DensityMapPanel()
{
  removePlaneFromViewer();
  unsubscribe();
}

bool DensityMapPanel::Create(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
{
  if (!VizPropertyPanel::Create(calculation, parent, id, pos, size, style, name)) {
    wxFAIL_MSG( wxT("DensityMapPanel creation failed") );
    return false;
  }
  SetSizer(new wxBoxSizer(wxVERTICAL));
  subscribe();
  return true;
}

bool DensityMapPanel::isRelevant(IPropCalculation *propCalc) const
{
  return propCalc != 0 && propCalc->getProperty("MO") != 0 &&
         propCalc->getProperty("ORBOCC") != 0;
}

void DensityMapPanel::initialize()
{
  build();
}

bool DensityMapPanel::Show(bool show)
{
  const bool ret = VizPropertyPanel::Show(show);
  if (show) {
    ensureInitialized();
    drawPlaneInViewer();
  } else {
    removePlaneFromViewer();
  }
  return ret;
}

void DensityMapPanel::build()
{
  wxSizer *top = GetSizer();
  p_eval.reset(new Evaluator);
  const bool ok = p_eval->build(getCalculation());

  wxBoxSizer *row1 = new wxBoxSizer(wxHORIZONTAL);
  p_show = new wxChoice(this, ID_SHOW);
  p_show->Append("Total density");
  p_show->Append("One orbital");
  p_show->Append("Sum of the degenerate set");
  p_show->SetSelection(0);
  p_show->SetToolTip("The orbital is the one selected in the MOs panel");
  row1->Add(new wxStaticText(this, wxID_ANY, "Show"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
  row1->Add(p_show, 0, wxRIGHT, 12);
  p_mo = new wxSpinCtrl(this, ID_MO, "1", wxDefaultPosition, wxSize(80, -1),
                        wxSP_ARROW_KEYS, 1, std::max(1, ok ? p_eval->orbitals(false) : 1),
                        std::max(1, ok ? p_eval->homo() : 1));
  row1->Add(new wxStaticText(this, wxID_ANY, "MO"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
  row1->Add(p_mo, 0, wxRIGHT, 12);
  p_spin = new wxChoice(this, ID_SPIN);
  p_spin->Append("alpha");
  p_spin->Append("beta");
  p_spin->SetSelection(0);
  row1->Add(p_spin, 0);

  wxBoxSizer *row2 = new wxBoxSizer(wxHORIZONTAL);
  p_plane = new wxChoice(this, ID_PLANE);
  p_plane->Append("Molecular plane");
  p_plane->Append("Through three atoms");
  p_plane->Append(wxString::FromUTF8("Through two atoms, \xE2\x8A\xA5 molecular plane"));
  p_plane->Append("This view (screen plane)");
  p_plane->SetToolTip("Pick three atoms in the 3-D view for a plane through "
                      "them, or two for a plane through that bond standing "
                      "on the molecular plane");
  row2->Add(new wxStaticText(this, wxID_ANY, "Plane"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
  row2->Add(p_plane, 0, wxRIGHT, 12);
  p_atomA = new wxChoice(this, ID_ATOM);
  p_atomB = new wxChoice(this, ID_ATOM);
  p_atomC = new wxChoice(this, ID_ATOM);
  if (ok) {
    const vector<string> labels = p_eval->atomLabels();
    for (size_t i = 0; i < labels.size(); i++) {
      p_atomA->Append(wxString::FromUTF8(labels[i].c_str()));
      p_atomB->Append(wxString::FromUTF8(labels[i].c_str()));
      p_atomC->Append(wxString::FromUTF8(labels[i].c_str()));
    }
    const int n = (int)labels.size();
    p_atomA->SetSelection(0);
    p_atomB->SetSelection(std::min(1, n - 1));
    p_atomC->SetSelection(std::min(2, n - 1));
    //  The molecular plane is the first guess; picks replace it.
    p_plane->SetSelection(isPlanar(p_eval->coordsAng) ? 0 : 1);
  } else {
    p_plane->SetSelection(0);
  }
  row2->Add(p_atomA, 0, wxRIGHT, 4);
  row2->Add(p_atomB, 0, wxRIGHT, 4);
  row2->Add(p_atomC, 0, wxRIGHT, 8);
  p_useView = new wxButton(this, ID_VIEW, "Use this view");
  p_useView->SetToolTip("Take the plane parallel to the screen through the "
                        "molecule's centre; rotate the molecule to change "
                        "the cut");
  row2->Add(p_useView, 0);

  wxBoxSizer *rowOff = new wxBoxSizer(wxHORIZONTAL);
  p_offset = new wxSlider(this, ID_OFFSET, 0, -OFFSET_TENTHS, OFFSET_TENTHS);
  p_offsetText = new wxStaticText(this, wxID_ANY, wxEmptyString,
                                  wxDefaultPosition, wxSize(120, -1));
  rowOff->Add(new wxStaticText(this, wxID_ANY, "Offset along the normal"), 0,
              wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
  rowOff->Add(p_offset, 1, wxRIGHT, 8);
  rowOff->Add(p_offsetText, 0, wxALIGN_CENTER_VERTICAL);

  wxBoxSizer *row3 = new wxBoxSizer(wxHORIZONTAL);
  p_status = new wxStaticText(this, wxID_ANY, wxEmptyString);
  row3->Add(p_status, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
  row3->Add(new wxButton(this, ID_PNG, "Export .png..."), 0, wxRIGHT, 4);
  row3->Add(new wxButton(this, ID_CSV, "Export .csv..."), 0);

  p_canvas = new DensityMapCanvas(this);
  top->Add(row1, 0, wxEXPAND | wxALL, 6);
  top->Add(row2, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
  top->Add(rowOff, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
  top->Add(p_canvas, 1, wxEXPAND);
  top->Add(row3, 0, wxEXPAND | wxALL, 6);

  Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { onChanged(); });
  Bind(wxEVT_SPINCTRL, [this](wxSpinEvent&) { onChanged(); });
  //  Dragging computes the coarse grid; releasing, or any other change of
  //  the slider, the full one.
  p_offset->Bind(wxEVT_SCROLL_THUMBTRACK, [this](wxScrollEvent&) {
    p_dragging = true;
    updateOffsetLabel();
    compute(true);
  });
  p_offset->Bind(wxEVT_SCROLL_THUMBRELEASE, [this](wxScrollEvent&) {
    p_dragging = false;
    compute(false);
  });
  p_offset->Bind(wxEVT_SLIDER, [this](wxCommandEvent&) {
    updateOffsetLabel();
    if (!p_dragging) compute(false);
  });
  Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { useCurrentView(); }, ID_VIEW);
  Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    wxFileDialog dlg(this, "Export map", wxEmptyString, "density-map.png",
                     "PNG files (*.png)|*.png", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() == wxID_OK && !savePng(dlg.GetPath(), 1100, 820))
      wxMessageBox("The file could not be written.", "Export map", wxOK | wxICON_ERROR, this);
  }, ID_PNG);
  Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    wxFileDialog dlg(this, "Export map values", wxEmptyString, "density-map.csv",
                     "CSV files (*.csv)|*.csv", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() == wxID_OK && !exportCsv(dlg.GetPath()))
      wxMessageBox("The file could not be written.", "Export map values", wxOK | wxICON_ERROR, this);
  }, ID_CSV);

  updateOffsetLabel();
  if (!ok) {
    p_canvas->say(wxString::FromUTF8(p_eval->complaint.c_str()));
    p_show->Enable(false);  p_mo->Enable(false);  p_spin->Enable(false);
    p_plane->Enable(false); p_atomA->Enable(false);
    p_atomB->Enable(false); p_atomC->Enable(false);
    p_offset->Enable(false); p_useView->Enable(false);
    return;
  }
  p_spin->Show(p_eval->unrestricted);
  syncControls();
  compute();
  Layout();
  if (IsShown()) drawPlaneInViewer();
}

double DensityMapPanel::offsetAngstrom() const
{
  return p_offset ? 0.1 * p_offset->GetValue() : 0.0;
}

void DensityMapPanel::updateOffsetLabel()
{
  p_offsetText->SetLabel(wxString::FromUTF8(
      wxString::Format("%+.1f \xC3\x85", offsetAngstrom()).utf8_str()));
}

void DensityMapPanel::syncControls()
{
  const bool orbital = p_show->GetSelection() != 0;
  p_mo->Enable(orbital);
  p_spin->Enable(orbital && p_eval->unrestricted);
  const int plane = p_plane->GetSelection();
  p_atomA->Enable(plane == 1 || plane == 2);
  p_atomB->Enable(plane == 1 || plane == 2);
  p_atomC->Enable(plane == 1);
}

void DensityMapPanel::onChanged()
{
  syncControls();
  compute();
}

bool DensityMapPanel::currentPlane(Plane& plane, string& why)
{
  const int sel = p_plane->GetSelection();
  if (sel == 3) {
    if (!p_haveView) {
      why = "Press \"Use this view\" to take the plane parallel to the screen.";
      return false;
    }
    plane = p_viewPlane;
  } else {
    const PlaneKind kind = sel == 0 ? MOLECULAR : sel == 1 ? THREE_ATOMS : AXIAL;
    if (!makePlane(kind, p_eval->coordsAng, p_eval->atomLabels(),
                   p_atomA->GetSelection(), p_atomB->GetSelection(),
                   p_atomC->GetSelection(), plane, why))
      return false;
  }
  plane = offsetPlane(plane, offsetAngstrom());
  return true;
}

void DensityMapPanel::compute(bool coarse)
{
  if (p_busy || !p_eval || !p_canvas) return;
  string why;
  Plane plane;
  if (!currentPlane(plane, why)) {
    p_canvas->say(wxString::FromUTF8(why.c_str()));
    p_status->SetLabel(wxEmptyString);
    p_haveMap = false;
    removePlaneFromViewer();
    return;
  }
  const int what = p_show->GetSelection();
  const int orbital = what == 0 ? 0 : p_mo->GetValue();
  const bool beta = p_spin->GetSelection() == 1;
  if (orbital > p_eval->orbitals(beta)) {
    p_canvas->say("No such orbital.");
    p_haveMap = false;
    return;
  }
  //  Larger molecules get a coarser grid so a map stays interactive.
  const double full = p_eval->symbols.size() > 20 ? 0.1 : 0.05;
  const double step = coarse ? std::max(COARSE_STEP, 2*full) : full;

  //  Shown only if the map takes long enough to be noticed.
  wxStopWatch clock;
  std::unique_ptr<wxProgressDialog> progress;
  p_busy = true;
  Map out;
  const bool done = p_eval->map(plane,
      what == 0 ? DENSITY : what == 1 ? ORBITAL : ORBITAL_SET, orbital, beta,
      step, MARGIN, out, [&](double fraction) {
        if (!progress && !coarse && clock.Time() > 500 && fraction < 0.7)
          progress.reset(new wxProgressDialog("Density map", "Evaluating...",
              100, this, wxPD_CAN_ABORT | wxPD_AUTO_HIDE | wxPD_APP_MODAL));
        return progress ? progress->Update((int)(100*fraction)) : true;
      });
  p_busy = false;
  if (!done) return;
  p_map = out;
  p_haveMap = true;
  p_canvas->show(p_map);
  p_status->SetLabel(coarse ? "Coarse grid while dragging"
      : p_map.quantity == ORBITAL
          ? "Contours from +-0.02, each level double the last; dashed is negative"
          : "Contours from 0.001, each level double the last");
  drawPlaneInViewer();
}

bool DensityMapPanel::setView(int orbital, bool beta, int plane, int a, int b,
                              int c, bool sum)
{
  ensureInitialized();
  if (!p_eval || !p_canvas || !p_show || !p_show->IsEnabled()) return false;
  if (orbital > 0) { p_show->SetSelection(sum ? 2 : 1); p_mo->SetValue(orbital); }
  else p_show->SetSelection(0);
  p_spin->SetSelection(beta ? 1 : 0);
  p_plane->SetSelection(plane);
  if (a > 0) p_atomA->SetSelection(a - 1);
  if (b > 0) p_atomB->SetSelection(b - 1);
  if (c > 0) p_atomC->SetSelection(c - 1);
  syncControls();
  compute();
  return p_haveMap;
}

bool DensityMapPanel::pickAtoms(const vector<int>& atoms)
{
  ensureInitialized();
  if (!p_eval || (atoms.size() != 2 && atoms.size() != 3)) return false;
  const int n = (int)p_eval->symbols.size();
  for (size_t i = 0; i < atoms.size(); i++)
    if (atoms[i] < 1 || atoms[i] > n) return false;
  p_atomA->SetSelection(atoms[0] - 1);
  p_atomB->SetSelection(atoms[1] - 1);
  if (atoms.size() == 3) p_atomC->SetSelection(atoms[2] - 1);
  p_plane->SetSelection(atoms.size() == 3 ? 1 : 2);
  syncControls();
  compute();
  return p_haveMap;
}

bool DensityMapPanel::setOffset(double angstrom, bool drag)
{
  ensureInitialized();
  if (!p_eval || !p_offset) return false;
  p_offset->SetValue(std::max(-OFFSET_TENTHS,
                              std::min(OFFSET_TENTHS, (int)lround(10*angstrom))));
  updateOffsetLabel();
  compute(drag);
  return p_haveMap;
}

bool DensityMapPanel::useView(const double right[3], const double up[3])
{
  ensureInitialized();
  if (!p_eval) return false;
  string why;
  if (!makeViewPlane(p_eval->coordsAng, right, up, p_viewPlane, why)) return false;
  p_haveView = true;
  p_plane->SetSelection(3);
  syncControls();
  compute();
  return p_haveMap;
}

bool DensityMapPanel::useCurrentView()
{
  ensureInitialized();
  SoCamera *cam = p_vizfw ? getFW().getViewer().getCamera() : 0;
  if (!cam) return false;
  const SbRotation r = cam->orientation.getValue();
  SbVec3f right, up;
  r.multVec(SbVec3f(1, 0, 0), right);
  r.multVec(SbVec3f(0, 1, 0), up);
  const double rv[3] = { right[0], right[1], right[2] };
  const double uv[3] = { up[0], up[1], up[2] };
  return useView(rv, uv);
}

void DensityMapPanel::followMo(int mo, bool beta)
{
  if (!p_eval || !p_show || !p_show->IsEnabled() || mo < 1) return;
  if (mo > p_eval->orbitals(beta)) return;
  if (p_show->GetSelection() == 0) p_show->SetSelection(1);
  p_mo->SetValue(mo);
  p_spin->SetSelection(beta ? 1 : 0);
  syncControls();
  if (IsShown()) compute();
}

void DensityMapPanel::eventMCB(const ecce::Event& event)
{
  if (event.getName() != "SelectionChanged" || !p_eval || !IsShown() ||
      !p_vizfw || !p_canvas)
    return;
  SGFragment *frag = getFW().getSceneGraph().getFragment();
  if (!frag) return;
  vector<int> picked;
  for (size_t i = 0; i < frag->m_atomHighLight.size(); i++) {
    const int a = frag->m_atomHighLight[i];
    if (a >= 0 && a < (int)p_eval->symbols.size() &&
        frag->atomRef(a)->atomicSymbol() != "Nub")
      picked.push_back(a + 1);
  }
  if (picked.size() == 2 || picked.size() == 3) pickAtoms(picked);
}

void DensityMapPanel::drawPlaneInViewer()
{
  if (!p_vizfw || !p_haveMap || !p_eval) return;
  SGContainer& sg = getFW().getSceneGraph();
  double u0, u1, v0, v1;
  window(p_map.plane, p_eval->coordsAng, MARGIN, u0, u1, v0, v1);
  const double us[4] = { u0, u1, u1, u0 }, vs[4] = { v0, v0, v1, v1 };
  vector<MPoint> corners;
  for (int k = 0; k < 4; k++) {
    double p[3];
    pointOn(p_map.plane, us[k], vs[k], p);
    corners.push_back(MPoint(p[0], p[1], p[2]));
  }
  StyledPlane *sp = sg.getPlane(PLANE_NAME);
  if (!sp) sp = sg.addPlane(new SGPlane(), PLANE_NAME);
  if (!sp) return;
  SGPlane *plane = sp->getPlane();
  plane->setLabel("");
  plane->setCorners(corners);
  sp->setColor(0.0f, 0.45f, 0.7f);
  sp->setTransparent(true);
  sp->show(true);
  plane->touch();
  p_inViewer = true;
  getFW().getViewer().refreshRenderArea();
}

void DensityMapPanel::removePlaneFromViewer()
{
  if (!p_inViewer || !p_vizfw) return;
  p_inViewer = false;
  SGContainer& sg = getFW().getSceneGraph();
  if (sg.getPlane(PLANE_NAME)) sg.deletePlane(PLANE_NAME);
  getFW().getViewer().refreshRenderArea();
}

string DensityMapPanel::state() const
{
  char buf[256];
  string s;
  s += "have " + string(p_haveMap ? "1" : "0") + "\n";
  s += "shown " + string(IsShown() ? "1" : "0") + "\n";
  s += "planeinviewer " + string(p_inViewer ? "1" : "0") + "\n";
  if (!p_haveMap) return s;
  s += "title " + p_map.title + "\n";
  s += "plane " + p_map.plane.description + "\n";
  snprintf(buf, sizeof buf, "offset %.3f\nnormal %.4f %.4f %.4f\norigin %.4f %.4f %.4f\n"
           "grid %d %d %.3f\nrange %.6g %.6g\n",
           p_map.plane.offset, p_map.plane.n[0], p_map.plane.n[1],
           p_map.plane.n[2], p_map.plane.origin[0], p_map.plane.origin[1],
           p_map.plane.origin[2], p_map.nu, p_map.nv, p_map.step, p_map.zmin,
           p_map.zmax);
  return s + buf;
}

bool DensityMapPanel::savePng(const wxString& path, int width, int height)
{
  if (!p_canvas) return false;
  wxInitAllImageHandlers();
  wxBitmap bmp(width, height);
  wxMemoryDC dc(bmp);
  p_canvas->draw(dc, wxSize(width, height));
  dc.SelectObject(wxNullBitmap);
  return bmp.SaveFile(path, wxBITMAP_TYPE_PNG);
}

bool DensityMapPanel::exportCsv(const wxString& path)
{
  if (!p_haveMap) return false;
  IPropCalculation *calc = getCalculation();
  const string text = csv(p_map, calc ? calc->getURL().getFilePathTail() : string());
  FILE *out = fopen(path.utf8_str(), "wb");
  if (out == 0) return false;
  const bool ok = fwrite(text.data(), 1, text.size(), out) == text.size();
  return fclose(out) == 0 && ok;
}
