#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>

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
#include <wx/radiobox.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/textctrl.h>
#include <wx/wrapsizer.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/stopwatch.h>

#include "PointNote.H"
#include "dsm/IPropCalculation.H"
#include "inv/nodes/SoCamera.H"
#include "tdat/Fragment.H"
#include "util/Event.H"
#include "wxgui/ewxConfig.H"
#include "DensityProfilePanel.H"
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
                 ID_OFFSET = wxNewId(), ID_VIEW = wxNewId(),
                 ID_FIELD = wxNewId(), ID_RESET = wxNewId(), ID_PLANETOGGLE = wxNewId(), ID_ZOOMRESET = wxNewId();

const char *PLANE_NAME = "DensityMapPlane";
const double MARGIN = 1.5;            // A beyond the atoms' extent
const double COARSE_STEP = 0.15;      // while the slider is dragged or the map zoomed
const int ZOOM_REST_MS = 400;          // no wheel step for this long: the full grid
const int OFFSET_MAX = 500;           // slider range, +-5.00 A in 0.01 A

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
      SetMinSize(wxSize(200, 200));
      Bind(wxEVT_PAINT, &DensityMapCanvas::onPaint, this);
      Bind(wxEVT_MOUSEWHEEL, [this](wxMouseEvent& e) {
        double u, v;
        if (!p_have || !onZoom || e.GetWheelRotation() == 0 ||
            !densityMapPointAt(GetClientSize(), p_map, e.GetX(), e.GetY(), u, v)) {
          e.Skip();
          return;
        }
        onZoom(u, v, e.GetWheelRotation() > 0 ? 1.25 : 0.8);
      });
      Bind(wxEVT_LEFT_DCLICK, [this](wxMouseEvent&) {
        if (onZoomReset) onZoomReset();
      });
    }

    std::function<void(double, double, double)> onZoom;   // u, v, factor
    std::function<void()> onZoomReset;
    void show(const Map& m) { p_map = m; p_have = true; p_message.clear(); Refresh(); }
    std::function<void()> onPaintHook;
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
      if (onPaintHook) onPaintHook();
    }

    Map p_map;
    bool p_have;
    wxString p_message;
};


IMPLEMENT_DYNAMIC_CLASS(DensityMapPanel, VizPropertyPanel)
wxFORCE_LINK_THIS_MODULE(DensityMapPanel)

DensityMapPanel::DensityMapPanel()
  : VizPropertyPanel(), p_haveMap(false), p_haveView(false), p_busy(false),
    p_inViewer(false), p_dragging(false), p_mapCoarse(false), p_show(0), p_spin(0), p_atomA(0),
    p_atomB(0), p_atomC(0), p_plane(0), p_offsetField(0), p_resetOffset(0),
    p_labelA(0), p_labelB(0), p_labelC(0), p_caption(0), p_atomRow(0),
    p_viewRow(0), p_planeToggle(0), p_planeOpen(true), p_planeAuto(false), p_zoomed(false), p_levelTop(0), p_zoomReset(0), p_mo(0), p_offset(0), p_status(0), p_useView(0), p_canvas(0)
{
}

DensityMapPanel::DensityMapPanel(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
  : DensityMapPanel()
{
  Create(calculation, parent, id, pos, size, style, name);
}

DensityMapPanel::~DensityMapPanel()
{
  p_zoomRest.Stop();
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
  const int RIGHT = wxALIGN_CENTER_VERTICAL | wxRIGHT;

  //  Rows wrap instead of widening the panel: none is wider than the dock.
  wxWrapSizer *row1 = new wxWrapSizer(wxHORIZONTAL);
  p_show = new wxChoice(this, ID_SHOW);
  p_show->Append("Total density");
  p_show->Append("One orbital");
  p_show->Append("Sum of the degenerate set");
  p_show->SetSelection(0);
  p_show->SetToolTip("The orbital is the one selected in the MOs panel");
  //  A label and its control wrap together, never apart.
  wxBoxSizer *showPair = new wxBoxSizer(wxHORIZONTAL);
  showPair->Add(new wxStaticText(this, wxID_ANY, "Show"), 0, RIGHT, 6);
  showPair->Add(p_show, 0, wxALIGN_CENTER_VERTICAL);
  row1->Add(showPair, 0, RIGHT, 12);
  p_mo = new wxSpinCtrl(this, ID_MO, "1", wxDefaultPosition, wxSize(110, -1),
                        wxSP_ARROW_KEYS, 1, std::max(1, ok ? p_eval->orbitals(false) : 1),
                        std::max(1, ok ? p_eval->homo() : 1));
  p_mo->SetMinSize(wxSize(110, -1));   // narrower hides the + button
  p_spin = new wxChoice(this, ID_SPIN);
  p_spin->Append("alpha");
  p_spin->Append("beta");
  p_spin->SetSelection(0);
  wxBoxSizer *moPair = new wxBoxSizer(wxHORIZONTAL);
  moPair->Add(new wxStaticText(this, wxID_ANY, "MO"), 0, RIGHT, 6);
  moPair->Add(p_mo, 0, RIGHT, 8);
  moPair->Add(p_spin, 0, wxALIGN_CENTER_VERTICAL);
  row1->Add(moPair, 0);

  wxArrayString planes;
  planes.Add("Molecular plane");
  planes.Add("Through three atoms A, B, C");
  planes.Add(wxString::FromUTF8("Containing the line A\xE2\x80\x93" "B, \xE2\x8A\xA5 molecular plane"));
  planes.Add("This view (parallel to the screen)");
  p_plane = new wxRadioBox(this, ID_PLANE, "Plane", wxDefaultPosition,
                           wxDefaultSize, planes, 1, wxRA_SPECIFY_COLS);

  p_atomRow = new wxBoxSizer(wxHORIZONTAL);
  wxWrapSizer *atoms = new wxWrapSizer(wxHORIZONTAL);
  p_atomA = new wxChoice(this, ID_ATOM);
  p_atomB = new wxChoice(this, ID_ATOM);
  p_atomC = new wxChoice(this, ID_ATOM);
  for (wxChoice *c : { p_atomA, p_atomB, p_atomC }) c->SetMinSize(wxSize(70, -1));
  p_labelA = new wxStaticText(this, wxID_ANY, "A");
  p_labelB = new wxStaticText(this, wxID_ANY, "B");
  p_labelC = new wxStaticText(this, wxID_ANY, "C");
  atoms->Add(p_labelA, 0, RIGHT, 4);
  atoms->Add(p_atomA, 0, RIGHT, 12);
  atoms->Add(p_labelB, 0, RIGHT, 4);
  atoms->Add(p_atomB, 0, RIGHT, 12);
  atoms->Add(p_labelC, 0, RIGHT, 4);
  atoms->Add(p_atomC, 0, RIGHT, 0);
  p_atomRow->Add(atoms, 1, wxEXPAND);
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
    chooseBondedB();
    //  A plane that cannot be defined is greyed out, not left to fail.
    const bool flat = isPlanar(p_eval->coordsAng);
    const bool standing = hasMolecularPlane(p_eval->coordsAng);
    p_plane->Enable(0, flat);
    p_plane->Enable(2, standing);
    const wxString why = "This molecule is not planar: use \"Through three atoms\".";
    if (!flat) p_plane->SetItemToolTip(0, why);
    if (!standing) p_plane->SetItemToolTip(2, flat
        ? wxString("A linear molecule has no molecular plane to stand on: use \"Through three atoms\" or the molecular plane.")
        : why);
    p_plane->SetSelection(flat ? 0 : 1);
    //  A lone atom has no molecular plane: start from the screen plane (the
    //  unrotated camera when the viewer is not there yet).
    if (startsOnViewPlane(labels.size())) {
      double right[3] = { 1, 0, 0 }, up[3] = { 0, 1, 0 };
      SoCamera *cam = p_vizfw ? getFW().getViewer().getCamera() : 0;
      if (cam) {
        SbVec3f r, u;
        cam->orientation.getValue().multVec(SbVec3f(1, 0, 0), r);
        cam->orientation.getValue().multVec(SbVec3f(0, 1, 0), u);
        for (int k = 0; k < 3; k++) { right[k] = r[k]; up[k] = u[k]; }
      }
      string why;
      if (makeViewPlane(p_eval->coordsAng, right, up, p_viewPlane, why)) {
        p_haveView = true;
        p_plane->SetSelection(3);
      }
    }
  }

  p_viewRow = new wxBoxSizer(wxHORIZONTAL);
  p_useView = new wxButton(this, ID_VIEW, "Update from view");
  p_useView->SetToolTip("Take the plane parallel to the screen, through the "
                        "molecule's centre, from the view as it is now");
  p_viewRow->Add(p_useView, 0);

  wxBoxSizer *rowOff = new wxBoxSizer(wxHORIZONTAL);
  p_offset = new wxSlider(this, ID_OFFSET, 0, -OFFSET_MAX, OFFSET_MAX);
  p_offset->SetMinSize(wxSize(80, -1));
  p_offsetField = new wxTextCtrl(this, ID_FIELD, "0.00", wxDefaultPosition,
                                 wxSize(64, -1), wxTE_PROCESS_ENTER | wxTE_RIGHT);
  p_offsetField->SetToolTip(wxString::FromUTF8("Distance along the plane's normal, in Å (0.01)"));
  p_resetOffset = new wxButton(this, ID_RESET, "Reset");
  rowOff->Add(new wxStaticText(this, wxID_ANY, "Offset"), 0, RIGHT, 6);
  rowOff->Add(p_offset, 1, RIGHT, 6);
  rowOff->Add(p_offsetField, 0, RIGHT, 2);
  rowOff->Add(new wxStaticText(this, wxID_ANY, wxString::FromUTF8("\xC3\x85")), 0, RIGHT, 6);
  rowOff->Add(p_resetOffset, 0, wxALIGN_CENTER_VERTICAL);

  p_canvas = new DensityMapCanvas(this);
  p_canvas->onPaintHook = [this]() { ensurePlane(); };
  //  One ellipsized line each, full text in the tooltip: wrapped text took
  //  the canvas's height.
  p_caption = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
                               wxDefaultSize, wxST_ELLIPSIZE_END);
  p_status = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
                              wxDefaultSize, wxST_ELLIPSIZE_END);
  //  The export buttons share the first row: height is what the map lacks.
  row1->Add(new wxButton(this, ID_PNG, "Export .png..."), 0, wxLEFT | wxRIGHT, 4);
  row1->Add(new wxButton(this, ID_CSV, "Export .csv..."), 0, wxRIGHT, 4);

  top->Add(row1, 0, wxEXPAND | wxALL, 6);
  //  The plane controls take most of the height; the choice is remembered.
  p_planeToggle = new wxButton(this, ID_PLANETOGGLE, wxEmptyString,
                               wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
  p_planeToggle->SetToolTip("Show or hide the plane choice and its atoms");
  {
    bool open = true;
    ewxConfig *cfg = ewxConfig::getConfig("wxbuilder.ini");
    if (cfg) cfg->Read("/DensityMap/PlaneControlsOpen", &open, true);
    p_planeOpen = open;
    //  Nothing remembered: a short panel starts folded (see relayout).
    p_planeAuto = !(cfg && cfg->Exists("/DensityMap/PlaneControlsOpen"));
  }
  row1->Add(p_planeToggle, 0, wxRIGHT, 4);
  //  Shown only while zoomed, so the row stays one line otherwise.
  p_zoomReset = new wxButton(this, ID_ZOOMRESET, "Whole plane");
  p_zoomReset->SetToolTip("Mouse wheel zooms around the pointer; double-click also resets");
  p_zoomReset->Hide();
  row1->Add(p_zoomReset, 0);
  p_canvas->onZoom = [this](double u, double v, double f) { zoomAt(u, v, f, true); };
  p_zoomRest.SetOwner(this);
  Bind(wxEVT_TIMER, [this](wxTimerEvent&) { refineZoom(); }, p_zoomRest.GetId());
  p_canvas->onZoomReset = [this]() { resetZoom(); };
  Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { resetZoom(); }, ID_ZOOMRESET);
  addOrbitalPointNote(this, top, getCalculation());
  top->Add(p_plane, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
  top->Add(p_atomRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
  top->Add(p_viewRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
  top->Add(rowOff, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
  top->Add(p_canvas, 1, wxEXPAND);
  top->Add(p_caption, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 6);
  top->Add(p_status, 0, wxEXPAND | wxLEFT | wxRIGHT, 6);

  Bind(wxEVT_CHOICE, &DensityMapPanel::onControl, this);
  Bind(wxEVT_RADIOBOX, &DensityMapPanel::onControl, this);
  Bind(wxEVT_SPINCTRL, &DensityMapPanel::onControl, this);
  Bind(wxEVT_SIZE, [this](wxSizeEvent& e) { relayout(); e.Skip(); });
  Bind(wxEVT_SHOW, [this](wxShowEvent& e) {
    if (e.IsShown()) drawPlaneInViewer();
    e.Skip();
  });
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
  p_offsetField->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent&) { commitOffsetField(); });
  p_offsetField->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) {
    commitOffsetField();
    e.Skip();
  });
  Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { setPlaneControlsOpen(!p_planeOpen); },
       ID_PLANETOGGLE);
  Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { setOffset(0.0); }, ID_RESET);
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
    for (wxWindow *w : std::initializer_list<wxWindow*>{ p_show, p_mo, p_spin,
         p_plane, p_atomA, p_atomB, p_atomC, p_offset, p_offsetField,
         p_resetOffset, p_useView })
      w->Enable(false);
    return;
  }
  p_spin->Show(p_eval->unrestricted);
  fillAtomLists();
  syncControls();
  compute();
  relayout();
  if (IsShown()) drawPlaneInViewer();
}

double DensityMapPanel::offsetAngstrom() const
{
  return p_offset ? 0.01 * p_offset->GetValue() : 0.0;
}

void DensityMapPanel::updateOffsetLabel()
{
  if (p_offsetField) p_offsetField->ChangeValue(wxString::Format("%.2f", offsetAngstrom()));
}

void DensityMapPanel::commitOffsetField()
{
  if (!p_offsetField || !p_eval) return;
  double v;
  if (!p_offsetField->GetValue().ToDouble(&v)) { updateOffsetLabel(); return; }
  if (fabs(v - offsetAngstrom()) > 0.0049) setOffset(v);
  else updateOffsetLabel();
}

void DensityMapPanel::newPlane()
{
  p_offset->SetValue(0);
  updateOffsetLabel();
}

void DensityMapPanel::fillAtomLists()
{
  if (!p_eval || p_atomB->GetCount() == 0) return;
  const vector<string> labels = p_eval->atomLabels();
  const int a = p_atomA->GetSelection();
  const int b = p_atomB->GetSelection();
  const bool line = p_plane->GetSelection() == 2;
  for (size_t i = 0; i < labels.size(); i++) {
    wxString text = wxString::FromUTF8(labels[i].c_str());
    if (line && (int)i != a && !bonded(p_eval->symbols, p_eval->coordsAng, a, (int)i))
      text += " (not bonded)";
    p_atomB->SetString((unsigned)i, text);
  }
  p_atomB->SetSelection(b);
}

void DensityMapPanel::chooseBondedB()
{
  const int n = (int)p_eval->symbols.size();
  const int a = p_atomA->GetSelection();
  int pick = -1;
  for (int j = 0; j < n && pick < 0; j++)
    if (j != a && bonded(p_eval->symbols, p_eval->coordsAng, a, j)) pick = j;
  if (pick < 0) pick = (a + 1) % std::max(1, n);
  p_atomB->SetSelection(pick);
  fillAtomLists();
}

string DensityMapPanel::zoomKey(const Plane& plane, int what, int orbital,
                                bool beta) const
{
  char b[400];
  snprintf(b, sizeof b, "%d %d %d %.5f %.5f %.5f %.5f %.5f %.5f %.5f %.5f %.5f",
           what, orbital, beta, plane.origin[0], plane.origin[1], plane.origin[2],
           plane.u[0], plane.u[1], plane.u[2], plane.v[0], plane.v[1], plane.v[2]);
  return b;
}

bool DensityMapPanel::atomPoint(int n, double& u, double& v) const
{
  if (!p_haveMap || n < 1 || n > (int)p_map.atoms.size()) return false;
  u = p_map.atoms[n - 1].u;
  v = p_map.atoms[n - 1].v;
  return true;
}

bool DensityMapPanel::zoomAt(double u, double v, double factor, bool interactive)
{
  if (!p_haveMap || !p_eval || factor <= 0) return false;
  if (!p_zoomed) {
    //  The whole plane's window and level ladder, kept while zoomed.
    p_fullWin[0] = p_map.u0;  p_fullWin[1] = p_map.u(p_map.nu - 1);
    p_fullWin[2] = p_map.v0;  p_fullWin[3] = p_map.v(p_map.nv - 1);
    std::copy(p_fullWin, p_fullWin + 4, p_zoomWin);
    p_levelTop = p_map.levelTop > 0 ? p_map.levelTop
        : p_map.quantity == ORBITAL ? std::max(fabs(p_map.zmin), fabs(p_map.zmax))
        : p_map.zmax;
    string why;
    Plane plane;
    if (!currentPlane(plane, why)) return false;
    const int what = p_show->GetSelection();
    p_zoomKey = zoomKey(plane, what, what == 0 ? 0 : p_mo->GetValue(),
                        p_spin->GetSelection() == 1);
  }
  const double fullW = p_fullWin[1] - p_fullWin[0], fullH = p_fullWin[3] - p_fullWin[2];
  double w = (p_zoomWin[1] - p_zoomWin[0]) / factor;
  double h = (p_zoomWin[3] - p_zoomWin[2]) / factor;
  if (w >= fullW * 0.999) {
    resetZoom();
    return true;
  }
  if (std::min(w, h) < 0.3) {
    const double k = 0.3 / std::min(w, h);
    w *= k;  h *= k;
  }
  //  The pointer keeps its place in the window; the window stays on the plane.
  const double fu = (u - p_zoomWin[0]) / (p_zoomWin[1] - p_zoomWin[0]);
  const double fv = (v - p_zoomWin[2]) / (p_zoomWin[3] - p_zoomWin[2]);
  double u0 = u - fu * w, v0 = v - fv * h;
  u0 = std::min(std::max(u0, p_fullWin[0]), p_fullWin[1] - w);
  v0 = std::min(std::max(v0, p_fullWin[2]), p_fullWin[3] - h);
  p_zoomWin[0] = u0;  p_zoomWin[1] = u0 + w;
  p_zoomWin[2] = v0;  p_zoomWin[3] = v0 + h;
  p_zoomed = true;
  if (interactive) {
    //  Each wheel step at full resolution made zooming crawl.
    compute(true);
    p_zoomRest.StartOnce(ZOOM_REST_MS);
  } else {
    compute();
  }
  return p_haveMap;
}

void DensityMapPanel::refineZoom()
{
  p_zoomRest.Stop();
  if (p_haveMap && p_mapCoarse && !p_dragging) compute(false, true);
}

void DensityMapPanel::resetZoom()
{
  p_zoomRest.Stop();
  if (!p_zoomed) return;
  p_zoomed = false;
  compute();
}

void DensityMapPanel::relayout()
{
  if (!p_caption) return;
  if (p_planeAuto && p_show) {
    const bool open = GetClientSize().y >= 560;
    if (open != p_planeOpen) { p_planeOpen = open; syncControls(); }
  }
  setCaption();
  p_status->SetLabel(wxString::FromUTF8(p_statusText.c_str()));
  p_status->SetToolTip(wxString::FromUTF8(p_statusText.c_str()));
  Layout();
}

void DensityMapPanel::setCaption()
{
  if (!p_caption) return;
  wxString text;
  if (p_haveMap)
    text = wxString::FromUTF8((p_map.title + "; " + p_map.plane.description).c_str());
  if (p_haveMap && p_zoomed)
    text += wxString::Format(" (zoom x%.1f)", (p_fullWin[1] - p_fullWin[0]) /
                                              (p_zoomWin[1] - p_zoomWin[0]));
  p_caption->SetLabel(text);
  p_caption->SetToolTip(text);
}

void DensityMapPanel::onControl(wxCommandEvent& event)
{
  if (!p_eval || !p_show || !p_show->IsEnabled()) return;
  const int id = event.GetId();
  if (id == ID_PLANE) {
    newPlane();
    if (p_plane->GetSelection() == 2) chooseBondedB();
    else fillAtomLists();
    if (p_plane->GetSelection() == 3) { useCurrentView(); return; }
  } else if (id == ID_ATOM) {
    newPlane();
    if (p_plane->GetSelection() == 2 && event.GetEventObject() == p_atomA)
      chooseBondedB();
    else fillAtomLists();
  }
  syncControls();
  compute();
}

void DensityMapPanel::syncControls()
{
  const bool orbital = p_show->GetSelection() != 0;
  p_mo->Enable(orbital);
  p_spin->Enable(orbital && p_eval->unrestricted);
  const int plane = p_plane->GetSelection();
  const bool two = plane == 2, three = plane == 1;
  p_labelA->Show(two || three);  p_atomA->Show(two || three);
  p_labelB->Show(two || three);  p_atomB->Show(two || three);
  p_labelC->Show(three);         p_atomC->Show(three);
  p_planeToggle->SetLabel(p_planeOpen ? "Hide plane controls" : "Show plane controls");
  GetSizer()->Show(p_plane, p_planeOpen, true);
  GetSizer()->Show(p_atomRow, p_planeOpen && (two || three), true);
  GetSizer()->Show(p_viewRow, p_planeOpen && plane == 3, true);
  Layout();
}

void DensityMapPanel::setPlaneControlsOpen(bool open)
{
  p_planeOpen = open;
  p_planeAuto = false;
  ewxConfig *cfg = ewxConfig::getConfig("wxbuilder.ini");
  if (cfg) {
    cfg->Write("/DensityMap/PlaneControlsOpen", open);
    cfg->Flush();
  }
  if (p_eval) syncControls();
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
      why = "No view taken yet.";
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

void DensityMapPanel::compute(bool coarse, bool progressInPanel)
{
  if (p_busy || !p_eval || !p_canvas) return;
  string why;
  Plane plane;
  if (!currentPlane(plane, why)) {
    p_canvas->say(wxString::FromUTF8(why.c_str()));
    p_statusText.clear();
    p_haveMap = false;
    removePlaneFromViewer();
    relayout();
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

  const string key = zoomKey(plane, what, orbital, beta);
  if (p_zoomed && key != p_zoomKey) p_zoomed = false;
  double zstep = step;
  if (p_zoomed)
    zstep = std::max(0.01, step * (p_zoomWin[1] - p_zoomWin[0]) /
                     (p_fullWin[1] - p_fullWin[0]));
  p_zoomReset->Show(p_zoomed);

  //  Shown only if the map takes long enough to be noticed.
  wxStopWatch clock;
  std::unique_ptr<wxProgressDialog> progress;
  int shownPercent = -1;
  p_busy = true;
  Map out;
  const bool done = p_eval->map(plane,
      what == 0 ? DENSITY : what == 1 ? ORBITAL : ORBITAL_SET, orbital, beta,
      zstep, MARGIN, out, [&](double fraction) {
        if (progressInPanel) {
          //  After a zoom: the status line, not a dialog over the map.
          const int percent = (int)(100*fraction) / 10 * 10;
          if (clock.Time() > 300 && percent != shownPercent && p_status) {
            shownPercent = percent;
            p_status->SetLabel(wxString::Format("Full grid: %d %%", percent));
            p_status->Update();
          }
          return true;
        }
        if (!progress && !coarse && clock.Time() > 500 && fraction < 0.7)
          progress.reset(new wxProgressDialog("Density map", "Evaluating...",
              100, this, wxPD_CAN_ABORT | wxPD_AUTO_HIDE | wxPD_APP_MODAL));
        return progress ? progress->Update((int)(100*fraction)) : true;
      }, p_zoomed ? p_zoomWin : nullptr);
  p_busy = false;
  if (!done) return;
  if (p_zoomed) out.levelTop = p_levelTop;
  p_map = out;
  p_haveMap = true;
  p_mapCoarse = coarse;
  p_canvas->show(p_map);
  std::string lev;
  {
    bool scaled = false;
    vector<double> lv = mapLevels(p_map, scaled);
    std::sort(lv.begin(), lv.end());   // the ladder is extended downwards last
    const bool orb = p_map.quantity == ORBITAL;
    for (size_t k = 0; k < lv.size() && k < 4; k++) {
      if (k) lev += ", ";
      char b[32];
      snprintf(b, sizeof b, orb ? "\xC2\xB1%g" : "%g", lv[k]);
      lev += b;
    }
    if (lv.size() > 4) lev += " ...";
    if (!lv.empty())
      lev += p_map.quantity == DENSITY ? " e/bohr\xC2\xB3"
           : p_map.quantity == ORBITAL_SET ? " bohr\xE2\x81\xBB\xC2\xB3"
           : " bohr\xE2\x81\xBB\xC2\xB3\xE2\x81\x84\xC2\xB2";
  }
  p_statusText = coarse ? (p_dragging ? "Coarse grid while dragging"
                                       : "Coarse grid while zooming")
      : lev.empty() ? std::string("No contour level is reached in this plane")
      : "Contours at " + lev + " (each twice the previous)" +
        (p_map.quantity == ORBITAL ? "; dashed: negative" : "");
  relayout();
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
  const int before[4] = { p_plane->GetSelection(), p_atomA->GetSelection(),
                          p_atomB->GetSelection(), p_atomC->GetSelection() };
  p_plane->SetSelection(plane);
  if (a > 0) p_atomA->SetSelection(a - 1);
  if (b > 0) p_atomB->SetSelection(b - 1);
  if (c > 0) p_atomC->SetSelection(c - 1);
  if (before[0] != plane || before[1] != p_atomA->GetSelection() ||
      before[2] != p_atomB->GetSelection() || before[3] != p_atomC->GetSelection())
    newPlane();
  if (plane == 3 && !p_haveView) return useCurrentView();
  fillAtomLists();
  syncControls();
  compute();
  return p_haveMap;
}

bool DensityMapPanel::choosePlane(int n)
{
  ensureInitialized();
  if (!p_eval || n < 0 || n > 3 || !p_plane->IsItemEnabled(n)) return false;
  p_plane->SetSelection(n);
  wxCommandEvent e(wxEVT_RADIOBOX, ID_PLANE);
  e.SetEventObject(p_plane);
  onControl(e);
  return n == 3 ? p_haveView : p_haveMap;
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
  newPlane();
  fillAtomLists();
  syncControls();
  compute();
  return p_haveMap;
}

bool DensityMapPanel::setOffset(double angstrom, bool drag)
{
  ensureInitialized();
  if (!p_eval || !p_offset) return false;
  p_offset->SetValue(std::max(-OFFSET_MAX,
                              std::min(OFFSET_MAX, (int)lround(100*angstrom))));
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
  fillAtomLists();
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
  //  The MO list and the MO Diagram tell this panel; the Density Profile
  //  follows the same selection through it.
  if (getCalculation()) {
    std::set<PropertyPanel*> panels = PropertyPanel::getPanels(getCalculation()->getURL().toString());
    for (std::set<PropertyPanel*>::iterator it = panels.begin(); it != panels.end(); ++it) {
      DensityProfilePanel *dp = dynamic_cast<DensityProfilePanel*>(*it);
      if (dp) dp->followMo(mo, beta);
    }
  }
  if (!p_eval || !p_show || !p_show->IsEnabled() || mo < 1) return;
  if (mo > p_eval->orbitals(beta)) return;
  if (p_show->GetSelection() == 0) p_show->SetSelection(1);
  p_mo->SetValue(mo);
  p_spin->SetSelection(beta ? 1 : 0);
  syncControls();
  if (IsShown()) compute();
  ensurePlane();
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

//  The scene graph can lose the plane behind the panel's back (another
//  panel taking the viewer), so every repaint of the map checks it.
void DensityMapPanel::ensurePlane()
{
  if (!p_vizfw || !p_haveMap || !IsShownOnScreen()) return;
  if (!getFW().getSceneGraph().getPlane(PLANE_NAME)) drawPlaneInViewer();
}

string DensityMapPanel::layoutReport(int width)
{
  ensureInitialized();
  GetSizer()->SetDimension(0, 0, width, 760);
  string out = "layout width " + std::to_string(width) + "\n";
  bool clipped = false;
  for (wxWindowList::compatibility_iterator n = GetChildren().GetFirst(); n;
       n = n->GetNext()) {
    wxWindow *w = n->GetData();
    if (!w->IsShown()) continue;
    const wxRect r = w->GetRect();
    const bool bad = r.x < 0 || r.GetRight() >= width;
    clipped = clipped || bad;
    char buf[200];
    snprintf(buf, sizeof buf, "ctl %s x=%d y=%d w=%d h=%d %s\n",
             (const char*)wxString(w->GetClassInfo()->GetClassName()).utf8_str(),
             r.x, r.y, r.width, r.height, bad ? "clipped" : "ok");
    out += buf;
  }
  out += clipped ? "layout clipped\n" : "layout ok\n";
  Layout();
  return out;
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
  snprintf(buf, sizeof buf, "panel %d %d\ncanvas %d %d\n", GetSize().x, GetSize().y,
           p_canvas ? p_canvas->GetSize().x : 0, p_canvas ? p_canvas->GetSize().y : 0);
  s += buf;
  if (!p_haveMap) return s;
  snprintf(buf, sizeof buf, "zoom %d window %.3f %.3f %.3f %.3f\n", p_zoomed ? 1 : 0,
           p_map.u0, p_map.u(p_map.nu - 1), p_map.v0, p_map.v(p_map.nv - 1));
  s += buf;
  s += "title " + p_map.title + "\n";
  s += "plane " + p_map.plane.description + "\n";
  s += "labelled";
  for (const string& l : densityMapLabelled(p_map)) s += " " + l;
  s += "\n";
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

void DensityMapPanel::orbitalPointChanged()
{
  if (!p_eval || !p_canvas || !p_show || !p_show->IsEnabled()) return;
  p_eval.reset(new Evaluator);
  if (!p_eval->build(getCalculation())) {
    p_canvas->say(wxString::FromUTF8(p_eval->complaint.c_str()));
    p_haveMap = false;
    removePlaneFromViewer();
    return;
  }
  const int n = std::max(1, p_eval->orbitals(false));
  p_mo->SetRange(1, n);
  p_mo->SetValue(std::min(p_mo->GetValue(), n));
  p_spin->Show(p_eval->unrestricted);
  fillAtomLists();
  syncControls();
  compute();
  Layout();
  if (IsShown()) drawPlaneInViewer();
}
