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
#include <wx/timer.h>

#include "PointNote.H"
#include "dsm/IPropCalculation.H"
#include "inv/nodes/SoCamera.H"
#include "tdat/Fragment.H"
#include "util/Event.H"
#include "wxgui/ewxConfig.H"
#include "DensityFragments.H"
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
                 ID_FIELD = wxNewId(), ID_RESET = wxNewId(), ID_PLANETOGGLE = wxNewId(), ID_ZOOMRESET = wxNewId(), ID_DIFF = wxNewId();

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
        p_have(false), p_wheelTimer(this)
    {
      SetBackgroundStyle(wxBG_STYLE_PAINT);
      SetMinSize(wxSize(200, 200));
      Bind(wxEVT_PAINT, &DensityMapCanvas::onPaint, this);
      Bind(wxEVT_MOUSEWHEEL, [this](wxMouseEvent& e) {
        double u, v;
        if (e.GetWheelAxis() == wxMOUSE_WHEEL_HORIZONTAL) return;
        if (!p_have || !onZoom || e.GetWheelRotation() == 0 ||
            !densityMapPointAt(GetClientSize(), p_map, e.GetX(), e.GetY(), u, v)) {
          e.Skip();
          return;
        }
        if (!p_wheelTimer.IsRunning()) {
          p_wheelAnchor = wxRealPoint(u, v);
          p_wheelTimer.StartOnce(16);
        }
        const int delta = e.GetWheelDelta() > 0 ? e.GetWheelDelta() : 120;
        p_wheelSteps += e.GetWheelRotation() / (double)delta;
      });
      Bind(wxEVT_TIMER, [this](wxTimerEvent&) {
        const double steps = p_wheelSteps;
        p_wheelSteps = 0;
        if (p_have && onZoom && steps != 0)
          onZoom(p_wheelAnchor.x, p_wheelAnchor.y, std::pow(1.25, steps));
      }, p_wheelTimer.GetId());
      Bind(wxEVT_LEFT_DCLICK, [this](wxMouseEvent&) {
        if (onZoomReset) onZoomReset();
      });
      //  Right-drag moves a zoomed map: the plane point under the pointer
      //  follows the pointer.
      Bind(wxEVT_RIGHT_DOWN, [this](wxMouseEvent& e) {
        p_drag = e.GetPosition();
        p_panning = true;
        if (!HasCapture()) CaptureMouse();
      });
      Bind(wxEVT_RIGHT_UP, [this](wxMouseEvent&) { endPan(); });
      Bind(wxEVT_MOUSE_CAPTURE_LOST, [this](wxMouseCaptureLostEvent&) { p_panning = false; });
      Bind(wxEVT_MOTION, [this](wxMouseEvent& e) {
        double u0, v0, u1, v1;
        if (!p_panning || !e.RightIsDown() || !p_have || !onPan) { e.Skip(); return; }
        const wxPoint at = e.GetPosition();
        if (at == p_drag) return;
        if (densityMapPointAt(GetClientSize(), p_map, p_drag.x, p_drag.y, u0, v0) &&
            densityMapPointAt(GetClientSize(), p_map, at.x, at.y, u1, v1)) {
          p_drag = at;
          onPan(u0 - u1, v0 - v1);
        }
      });
    }

    ~DensityMapCanvas() { if (HasCapture()) ReleaseMouse(); }

    std::function<void(double, double, double)> onZoom;   // u, v, factor
    std::function<void()> onZoomReset;
    std::function<void(double, double)> onPan;   // du, dv
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

    void endPan()
    {
      p_panning = false;
      if (HasCapture()) ReleaseMouse();
    }

    Map p_map;
    bool p_have;
    wxString p_message;
    bool p_panning = false;
    wxPoint p_drag;
    wxTimer p_wheelTimer;
    double p_wheelSteps = 0;
    wxRealPoint p_wheelAnchor;
};


IMPLEMENT_DYNAMIC_CLASS(DensityMapPanel, VizPropertyPanel)
wxFORCE_LINK_THIS_MODULE(DensityMapPanel)

DensityMapPanel::DensityMapPanel()
  : VizPropertyPanel(), p_haveMap(false), p_haveView(false), p_busy(false),
    p_inViewer(false), p_dragging(false), p_mapCoarse(false), p_show(0), p_spin(0), p_atomA(0),
    p_atomB(0), p_atomC(0), p_plane(0), p_principal(0), p_offsetField(0), p_resetOffset(0),
    p_labelA(0), p_labelB(0), p_labelC(0), p_caption(0), p_atomRow(0),
    p_viewRow(0), p_moreRow(0), p_planeToggle(0), p_planeOpen(true), p_planeAuto(false), p_diff(0), p_zoomed(false), p_levelTop(0), p_zoomReset(0), p_mo(0), p_offset(0), p_status(0), p_useView(0), p_canvas(0)
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
    //  Atoms selected before the panel opened define the plane too.
    applySelection();
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

  //  Labels share one column width, so Show, MO and Offset line up; the
  //  controls beside them stretch, which keeps the panel no wider than the dock.
  const int labelW = GetTextExtent("Offset").GetWidth() + 8;
  auto label = [&](const wxString &t) {
    wxStaticText *l = new wxStaticText(this, wxID_ANY, t);
    l->SetMinSize(wxSize(labelW, -1));
    return l;
  };
  wxFlexGridSizer *grid = new wxFlexGridSizer(2, 4, 6);
  grid->AddGrowableCol(1);
  p_show = new wxChoice(this, ID_SHOW);
  p_show->Append("Total density");
  p_show->Append("One orbital");
  p_show->Append("Sum of the degenerate set");
  p_show->SetSelection(0);
  p_show->SetToolTip("The orbital is the one selected in the MOs panel");
  grid->Add(label("Show"), 0, wxALIGN_CENTER_VERTICAL);
  grid->Add(p_show, 1, wxEXPAND);
  p_mo = new wxSpinCtrl(this, ID_MO, "1", wxDefaultPosition, wxSize(110, -1),
                        wxSP_ARROW_KEYS, 1, std::max(1, ok ? p_eval->orbitals(false) : 1),
                        std::max(1, ok ? p_eval->homo() : 1));
  p_mo->SetMinSize(wxSize(110, -1));   // narrower hides the + button
  p_spin = new wxChoice(this, ID_SPIN);
  p_spin->Append("alpha");
  p_spin->Append("beta");
  p_spin->SetSelection(0);
  wxBoxSizer *moPair = new wxBoxSizer(wxHORIZONTAL);
  moPair->Add(p_mo, 0, RIGHT, 8);
  moPair->Add(p_spin, 0, wxALIGN_CENTER_VERTICAL);
  grid->Add(label("MO"), 0, wxALIGN_CENTER_VERTICAL);
  grid->Add(moPair, 1, wxEXPAND);
  p_diff = new wxButton(this, ID_DIFF, "Difference...");
  p_diff->SetToolTip("Show this density minus the density of other calculations of "
                     "some of the same atoms (the fragments of a complex)");

  wxArrayString planes;
  planes.Add("Molecular plane");
  planes.Add("Through three atoms A, B, C");
  planes.Add(wxString::FromUTF8("Containing the line A\xE2\x80\x93" "B, \xE2\x8A\xA5 molecular plane"));
  planes.Add("This view (parallel to the screen)");
  p_plane = new wxRadioBox(this, ID_PLANE, "Plane", wxDefaultPosition,
                           wxDefaultSize, planes, 1, wxRA_SPECIFY_COLS);
  p_principal = new wxChoice(this, wxID_ANY);
  p_principal->SetToolTip(wxString::FromUTF8("Principal plane through the centroid; RMS distance of all atoms in Å"));
  if (ok) updatePrincipalPlanes();

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
    p_plane->Enable(0, flat || standing);
    p_plane->Enable(2, standing);
    const wxString why = labels.size() == 1
        ? wxString("One atom has no molecular plane: the map lies in the plane of the screen; turn the molecule and press \"Update from view\".")
        : wxString("This molecule is not planar: the molecular plane is the best-fit plane through all atoms.");
    if (!flat) p_plane->SetItemToolTip(0, why);
    if (!standing) p_plane->SetItemToolTip(2, flat
        ? wxString("A linear molecule has no molecular plane to stand on: use \"Through three atoms\" or the molecular plane.")
        : why);
    p_plane->SetSelection(flat || standing ? 0 : 1);
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
  rowOff->Add(label("Offset"), 0, RIGHT, 6);
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
  //  The four actions in one grid of equal widths (as "Mode in the viewer"
  //  in the Vibrational Frequencies panel); height is what the map lacks.
  //  One Options toggle; the rarely used actions sit inside it with the
  //  plane controls, so the map keeps the height.
  wxBoxSizer *buttons = new wxBoxSizer(wxHORIZONTAL);
  p_moreRow = new wxBoxSizer(wxHORIZONTAL);
  p_moreRow->Add(p_diff, 0, RIGHT, 4);
  p_moreRow->Add(new wxButton(this, ID_PNG, "Export .png..."), 0, RIGHT, 4);
  p_moreRow->Add(new wxButton(this, ID_CSV, "Export .csv..."), 0, 0);

  top->Add(grid, 0, wxEXPAND | wxALL, 6);
  top->Add(buttons, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
  //  The plane controls take most of the height; the choice is remembered.
  p_planeToggle = new wxButton(this, ID_PLANETOGGLE, wxEmptyString,
                               wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
  p_planeToggle->SetToolTip("Show or hide the plane choice, Difference... and the exports");
  {
    bool open = true;
    ewxConfig *cfg = ewxConfig::getConfig("wxbuilder.ini");
    if (cfg) cfg->Read("/DensityMap/PlaneControlsOpen", &open, true);
    p_planeOpen = open;
    //  Nothing remembered: a short panel starts folded (see relayout).
    p_planeAuto = !(cfg && cfg->Exists("/DensityMap/PlaneControlsOpen"));
  }
  buttons->Add(p_planeToggle, 0, 0);
  //  Shown only while zoomed, so the row stays one line otherwise.
  p_zoomReset = new wxButton(this, ID_ZOOMRESET, "Whole plane");
  p_zoomReset->SetToolTip("Mouse wheel zooms around the pointer; double-click also resets");
  p_zoomReset->Hide();
  top->Add(p_zoomReset, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
  p_canvas->onZoom = [this](double u, double v, double f) { zoomAt(u, v, f, true); };
  p_canvas->onPan = [this](double du, double dv) { panBy(du, dv, true); };
  p_zoomRest.SetOwner(this);
  Bind(wxEVT_TIMER, [this](wxTimerEvent&) { refineZoom(); }, p_zoomRest.GetId());
  p_canvas->onZoomReset = [this]() { resetZoom(); };
  Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { resetZoom(); }, ID_ZOOMRESET);
  Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { chooseFragments(); }, ID_DIFF);
  addOrbitalPointNote(this, top, getCalculation());
  top->Add(p_moreRow, 0, wxLEFT | wxRIGHT | wxBOTTOM, 6);
  top->Add(p_plane, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
  top->Add(p_principal, 0, wxLEFT | wxRIGHT | wxBOTTOM, 6);
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
         p_plane, p_principal, p_atomA, p_atomB, p_atomC, p_offset, p_offsetField,
         p_resetOffset, p_useView })
      w->Enable(false);
    p_principal->Hide();
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

//  The farthest a zoomed or panned window may reach: the whole plane grown
//  by one and a half of its widths on every side.
void DensityMapPanel::outerWindow(double out[4]) const
{
  const double gw = 1.5 * (p_fullWin[1] - p_fullWin[0]);
  const double gh = 1.5 * (p_fullWin[3] - p_fullWin[2]);
  out[0] = p_fullWin[0] - gw;  out[1] = p_fullWin[1] + gw;
  out[2] = p_fullWin[2] - gh;  out[3] = p_fullWin[3] + gh;
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
  double w = (p_zoomWin[1] - p_zoomWin[0]) / factor;
  double h = (p_zoomWin[3] - p_zoomWin[2]) / factor;
  //  Zooming out may go past the whole plane: its margin is a fixed 1.5 A,
  //  too small for the diffuse orbitals of an atom or a small molecule.
  double outer[4];
  outerWindow(outer);
  const double outW = outer[1] - outer[0], outH = outer[3] - outer[2];
  if (w > outW || h > outH) {
    const double k = std::min(outW / w, outH / h);
    w *= k;  h *= k;
  }
  if (std::min(w, h) < 0.3) {
    const double k = 0.3 / std::min(w, h);
    w *= k;  h *= k;
  }
  //  The pointer keeps its place in the window; the window stays on the plane.
  const double fu = (u - p_zoomWin[0]) / (p_zoomWin[1] - p_zoomWin[0]);
  const double fv = (v - p_zoomWin[2]) / (p_zoomWin[3] - p_zoomWin[2]);
  double u0 = u - fu * w, v0 = v - fv * h;
  u0 = std::min(std::max(u0, outer[0]), outer[1] - w);
  v0 = std::min(std::max(v0, outer[2]), outer[3] - h);
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

bool DensityMapPanel::panBy(double du, double dv, bool interactive)
{
  if (!p_haveMap || !p_eval || !p_zoomed) return false;
  const double w = p_zoomWin[1] - p_zoomWin[0], h = p_zoomWin[3] - p_zoomWin[2];
  double outer[4];
  outerWindow(outer);
  const double u0 = std::min(std::max(p_zoomWin[0] + du, outer[0]), outer[1] - w);
  const double v0 = std::min(std::max(p_zoomWin[2] + dv, outer[2]), outer[3] - h);
  if (u0 == p_zoomWin[0] && v0 == p_zoomWin[2]) return true;
  p_zoomWin[0] = u0;  p_zoomWin[1] = u0 + w;
  p_zoomWin[2] = v0;  p_zoomWin[3] = v0 + h;
  if (interactive) {
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
  } else if (event.GetEventObject() == p_principal) {
    newPlane();
  } else if (id == ID_ATOM) {
    newPlane();
    if (p_plane->GetSelection() == 2 && event.GetEventObject() == p_atomA)
      chooseBondedB();
    else fillAtomLists();
  }
  syncControls();
  compute();
}

void DensityMapPanel::updatePrincipalPlanes()
{
  const int selected = std::max(0, p_principal->GetSelection());
  p_principalPlanes.clear();
  p_principal->Clear();
  const bool flat = isPlanar(p_eval->coordsAng);
  const bool standing = hasMolecularPlane(p_eval->coordsAng);
  if (!flat) p_principalPlanes = principalPlanes(p_eval->coordsAng);
  for (size_t i = 0; i < p_principalPlanes.size(); ++i)
    p_principal->Append(wxString::Format(wxString::FromUTF8("%d: %s(RMS %.2f Å)"), int(i + 1),
        i == 0 ? "best fit " : "", p_principalPlanes[i].rms));
  if (!p_principalPlanes.empty()) p_principal->SetSelection(std::min(selected, 2));
  p_plane->Enable(0, flat || standing);
  p_plane->Enable(2, standing);
  if (flat) p_plane->SetItemToolTip(0, wxEmptyString);
  else p_plane->SetItemToolTip(0, standing
      ? "This molecule is not planar: the molecular plane is the best-fit plane through all atoms."
      : "One atom has no molecular plane: use the screen plane.");
}

void DensityMapPanel::syncControls()
{
  const bool orbital = p_show->GetSelection() != 0;
  p_mo->Enable(orbital);
  p_diff->Enable(!orbital);
  p_spin->Enable(orbital && p_eval->unrestricted);
  const int plane = p_plane->GetSelection();
  const bool two = plane == 2, three = plane == 1;
  p_labelA->Show(two || three);  p_atomA->Show(two || three);
  p_labelB->Show(two || three);  p_atomB->Show(two || three);
  p_labelC->Show(three);         p_atomC->Show(three);
  p_planeToggle->SetLabel(wxString::FromUTF8(p_planeOpen ? "Options \xE2\x96\xBE" : "Options \xE2\x96\xB8"));
  GetSizer()->Show(p_moreRow, p_planeOpen, true);
  GetSizer()->Show(p_plane, p_planeOpen, true);
  GetSizer()->Show(p_principal, p_planeOpen && plane == 0 && !p_principalPlanes.empty());
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
  } else if (sel == 0 && !p_principalPlanes.empty()) {
    plane = p_principalPlanes[p_principal->GetSelection()];
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
  if (p_map.difference) {
    char b[160];
    snprintf(b, sizeof b, "Blue: density gained, red: lost, white: unchanged; full "
             "colour at \xC2\xB1%.3g e/bohr\xC2\xB3 (the 99th percentile of the plane)",
             p_map.colourLimit);
    p_statusText = coarse ? "Coarse grid while dragging" : b;
  } else
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
  if (event.getName() != "SelectionChanged" || !IsShown()) return;
  applySelection();
}

void DensityMapPanel::applySelection()
{
  if (!p_eval || !p_vizfw || !p_canvas) return;
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
  s += "orbitals " + string(p_eval && p_eval->fromResults ? "results.h5" : "properties") + "\n";
  snprintf(buf, sizeof buf, "difference %d fragments %d colourlimit %.6g\n",
           p_map.difference ? 1 : 0, (int)p_fragments.size(), p_map.colourLimit);
  s += buf;
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

bool DensityMapPanel::setFragments(const std::vector<std::string>& urls, string& why)
{
  ensureInitialized();
  if (!p_eval || !p_canvas || !p_show || !p_show->IsEnabled()) {
    why = "The Density Map is not available for this calculation.";
    return false;
  }
  if (!DensityFragments::attach(*p_eval, urls, why)) {
    p_fragments.clear();
    updateDiffButton();
    compute();
    return false;
  }
  p_fragments = urls;
  updateDiffButton();
  p_zoomed = false;
  compute();
  return true;
}

void DensityMapPanel::updateDiffButton()
{
  p_diff->SetLabel(p_fragments.empty() ? wxString("Difference...")
      : wxString::Format("Difference: %d fragment%s...", (int)p_fragments.size(),
                         p_fragments.size() == 1 ? "" : "s"));
  relayout();
}

void DensityMapPanel::chooseFragments()
{
  std::vector<std::string> chosen;
  if (!DensityFragments::choose(this, getCalculation(), p_fragments, chosen)) return;
  string why;
  if (!setFragments(chosen, why))
    wxMessageBox(wxString::FromUTF8(why.c_str()), "Density difference", wxOK | wxICON_WARNING, this);
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
  string ignored;
  if (!DensityFragments::attach(*p_eval, p_fragments, ignored)) p_fragments.clear();
  const int n = std::max(1, p_eval->orbitals(false));
  p_mo->SetRange(1, n);
  p_mo->SetValue(std::min(p_mo->GetValue(), n));
  p_spin->Show(p_eval->unrestricted);
  updatePrincipalPlanes();
  fillAtomLists();
  syncControls();
  compute();
  Layout();
  if (IsShown()) drawPlaneInViewer();
}
