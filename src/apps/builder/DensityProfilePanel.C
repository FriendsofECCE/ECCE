#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>

#include <wx/bitmap.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/checklst.h>
#include <wx/choice.h>
#include <wx/dcmemory.h>
#include <wx/filedlg.h>
#include <wx/image.h>
#include <wx/link.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/spinctrl.h>
#include <wx/splitter.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/utils.h>
#include <wx/wrapsizer.h>

#include "PointNote.H"
#include "wxgui/ewxConfig.H"
#include "dsm/IPropCalculation.H"
#include "tdat/PropVecString.H"

#include "tdat/Fragment.H"
#include "util/Event.H"
#include "viz/SGContainer.H"
#include "viz/SGFragment.H"
#include "wxviz/SGViewer.H"
#include "DensityProfilePanel.H"
#include "XYPlotCanvas.H"

namespace {

const wxWindowID ID_MODE = wxNewId(), ID_ATOM = wxNewId(), ID_QUANTITY = wxNewId(),
                 ID_SHOW = wxNewId(), ID_SPIN = wxNewId(), ID_UNIT = wxNewId(),
                 ID_LIST = wxNewId(), ID_COMBINE = wxNewId(), ID_CLIP = wxNewId(), ID_MARGIN = wxNewId(),
                 ID_PNG = wxNewId(), ID_CSV = wxNewId(), ID_XAXIS = wxNewId(),
                 ID_YAXIS = wxNewId(), ID_WHOLE = wxNewId(), ID_MORE = wxNewId(),
                 ID_SHIFT = wxNewId(), ID_SHIFTDIR = wxNewId();

const double BOHR_ANG = 0.52917721092;

//  Interactive accuracy: 0.0075 A steps and 302 directions resolve every
//  maximum of a first-row atom to well under 0.01 A.
const int GRID_POINTS = 801;
const int AUTO_ABOVE = 8;       // more occupied orbitals than this: Total + the largest few
const int AUTO_ORBITALS = 4;
const int ANGULAR_POINTS = 302;

wxString U(const char *utf8) { return wxString::FromUTF8(utf8); }

//  Colour-blind safe (Okabe-Ito without black, which is the total's, then
//  five more).  Past the end the colours return as dashed lines, so no two
//  curves ever look alike.
wxColour curveColour(int i)
{
  static const char *hex[] = { "#E69F00", "#56B4E9", "#009E73", "#0072B2", "#D55E00",
                               "#CC79A7", "#7F7F7F", "#8C510A", "#35978F", "#762A83",
                               "#B2182B", "#F0E442" };
  return wxColour(hex[i % 12]);
}

//  What a radial curve shows, in words.
wxString radialText(const RadialDist::Curve& c, bool radial, const wxString& length)
{
  if (!radial) {
    size_t at = 0;
    for (size_t i = 0; i < c.p.size(); i++) if (c.p[i] > c.p[at]) at = i;
    return at < 3 ? wxString("largest at the nucleus") : wxString("no maximum at the nucleus");
  }
  if (c.maxR.empty()) return "no maximum";
  if (c.maxR.size() == 1)
    return wxString::Format("most probable distance %.3g %s", c.maxR[0], length);
  wxString t = "maxima at ";
  for (size_t k = 0; k < c.maxR.size(); k++)
    t += wxString::Format("%s%.3g", k == 0 ? "" : (k + 1 == c.maxR.size() ? " and " : ", "), c.maxR[k]);
  return t + " " + length;
}

//  Where along the line s lies, by the nearest atom.
wxString whereText(double s, const std::vector<LineProfile::Mark>& marks, double tol,
                   const wxString& length)
{
  if (marks.empty()) return wxString::Format("at %.3g %s along the line", s, length);
  size_t near = 0;
  for (size_t k = 0; k < marks.size(); k++)
    if (std::fabs(s - marks[k].s) < std::fabs(s - marks[near].s)) near = k;
  const double d = std::fabs(s - marks[near].s);
  const wxString name = U(marks[near].label.c_str());
  if (d < tol) return "at " + name;
  if (marks.size() >= 2 && s > marks.front().s && s < marks.back().s) {
    //  Between the two atoms on either side of s.
    size_t lo = 0;
    for (size_t k = 0; k < marks.size(); k++) if (marks[k].s <= s) lo = k;
    const size_t hi = std::min(lo + 1, marks.size() - 1);
    return "between " + U(marks[lo].label.c_str()) + " and " + U(marks[hi].label.c_str()) +
           wxString::Format(", %.3g %s from %s", s - marks[lo].s, length, U(marks[lo].label.c_str()));
  }
  return wxString::Format("%.3g %s from ", d, length) + name;
}

//  What a line curve shows: where it is largest, and for psi where it changes sign.
wxString lineText(const RadialDist::Curve& c, const std::vector<double>& s,
                  const std::vector<LineProfile::Mark>& marks, int quantity,
                  const wxString& length)
{
  const double tol = length == "bohr" ? 0.1 / BOHR_ANG : 0.1;
  size_t at = 0;
  for (size_t i = 0; i < c.p.size(); i++)
    if (std::fabs(c.p[i]) > std::fabs(c.p[at])) at = i;
  const double top = std::fabs(c.p[at]);
  if (top <= 0) return "zero everywhere on the line";
  wxString t = (quantity == 2 ? "largest magnitude " : "largest ") + whereText(s[at], marks, tol, length);
  if (quantity == 2) {
    int found = 0;
    for (size_t i = 1; i < c.p.size() && found < 3; i++) {
      const double a = c.p[i-1], b = c.p[i];
      if (std::fabs(a) < 1e-6 * top || std::fabs(b) < 1e-6 * top || a * b >= 0) continue;
      const double x = s[i-1] + (s[i] - s[i-1]) * a / (a - b);
      t += "; changes sign " + whereText(x, marks, tol, length) + " (node)";
      found++;
    }
  }
  return t;
}

}  // namespace


IMPLEMENT_DYNAMIC_CLASS(DensityProfilePanel, VizPropertyPanel)
wxFORCE_LINK_THIS_MODULE(DensityProfilePanel)

DensityProfilePanel::DensityProfilePanel()
  : VizPropertyPanel(), p_sumError(-1.0), p_sumPeak(0.0), p_ok(false), p_busy(false),
    p_atomsOnly(false), p_autoActive(false), p_showDensityList(true), p_moreOpen(false), p_mode(0), p_atomA(0),
    p_atomB(0), p_centre(0), p_quantity(0),
    p_show(0), p_spin(0), p_unit(0), p_xaxis(0), p_yaxis(0), p_shiftDir(0), p_margin(0),
    p_shift(0), p_more(0), p_moreButton(0), p_outer(0), p_inner(0), p_labelXAxis(0),
    p_labelYAxis(0), p_labelA(0), p_labelB(0),
    p_labelCentre(0), p_labelMargin(0), p_list(0), p_combine(0), p_clip(0), p_text(0), p_plot(0)
{
}

DensityProfilePanel::DensityProfilePanel(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
  : DensityProfilePanel()
{
  Create(calculation, parent, id, pos, size, style, name);
}

DensityProfilePanel::~DensityProfilePanel()
{
  unsubscribe();
}

bool DensityProfilePanel::Create(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
{
  if (!VizPropertyPanel::Create(calculation, parent, id, pos, size, style, name)) {
    wxFAIL_MSG( wxT("DensityProfilePanel creation failed") );
    return false;
  }
  SetSizer(new wxBoxSizer(wxVERTICAL));
  subscribe();
  return true;
}

bool DensityProfilePanel::isRelevant(IPropCalculation *propCalc) const
{
  return propCalc != 0 && propCalc->getProperty("MO") != 0 &&
         propCalc->getProperty("ORBOCC") != 0;
}

void DensityProfilePanel::initialize()
{
  build();
}

void DensityProfilePanel::build()
{
  wxSizer *top = GetSizer();
  p_eval.reset(new DensityPlane::Evaluator);
  p_ok = p_eval->build(getCalculation());
  const int RIGHT = wxALIGN_CENTER_VERTICAL | wxRIGHT;
  addOrbitalPointNote(this, top, getCalculation());

  //  Orbital list, plot and notes in splitters, so each can be given more room.
  if (!p_ok) {
    top->Add(new wxStaticText(this, wxID_ANY,
             wxString::FromUTF8(p_eval->complaint.c_str())), 0, wxALL, 8);
    return;
  }
  p_outer = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                 wxSP_LIVE_UPDATE | wxSP_3DSASH);
  p_inner = new wxSplitterWindow(p_outer, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                 wxSP_LIVE_UPDATE | wxSP_3DSASH);
  p_list = new wxCheckListBox(p_outer, ID_LIST, wxDefaultPosition, wxSize(-1, 46));
  p_plot = new XYPlotCanvas(p_inner);
  p_plot->SetMinSize(wxSize(240, 160));
  p_text = new wxTextCtrl(p_inner, wxID_ANY, wxEmptyString, wxDefaultPosition,
                          wxSize(-1, 44), wxTE_MULTILINE | wxTE_READONLY);
  const int natoms = (int)p_eval->symbols.size();

  //  Two short rows that wrap instead of widening the panel, then the
  //  rarely needed controls folded under "More".
  wxWrapSizer *row1 = new wxWrapSizer(wxHORIZONTAL);
  wxWrapSizer *row2 = new wxWrapSizer(wxHORIZONTAL);
  p_mode = new wxChoice(this, ID_MODE);
  p_mode->Append("Along a line");
  p_mode->Append("Around an atom");
  p_mode->SetToolTip("A line through two atoms, or the average over spheres about one atom");
  row1->Add(p_mode, 0, RIGHT, 12);

  const std::vector<std::string> labels = p_eval->atomLabels();
  auto atomChoice = [&](wxChoice *&c, wxStaticText *&l, const char *text) {
    c = new wxChoice(this, ID_ATOM);
    for (const std::string& a : labels) c->Append(U(a.c_str()));
    l = new wxStaticText(this, wxID_ANY, text);
    wxBoxSizer *pair = new wxBoxSizer(wxHORIZONTAL);
    pair->Add(l, 0, RIGHT, 4);
    pair->Add(c, 0, wxALIGN_CENTER_VERTICAL);
    row1->Add(pair, 0, RIGHT, 10);
  };
  atomChoice(p_atomA, p_labelA, "From");
  atomChoice(p_atomB, p_labelB, "to");
  atomChoice(p_centre, p_labelCentre, "Centre");

  p_quantity = new wxChoice(this, ID_QUANTITY);
  row1->Add(p_quantity, 0, RIGHT, 12);
  p_show = new wxChoice(this, ID_SHOW);
  row1->Add(p_show, 0, RIGHT, 12);
  p_spin = new wxChoice(this, ID_SPIN);
  p_spin->Append("alpha");
  p_spin->Append("beta");
  p_spin->Append(U("alpha + beta"));
  p_spin->SetSelection(2);
  p_spin->Show(p_eval->unrestricted);
  row1->Add(p_spin, 0, RIGHT, 12);

  p_xaxis = new wxChoice(this, ID_XAXIS);
  p_xaxis->Append("Linear");
  p_xaxis->Append(U("\xE2\x88\x9A" "r"));
  p_xaxis->Append("log r");
  p_xaxis->SetSelection(0);
  p_xaxis->SetToolTip(U("The r axis: linear, square root (spreads the region near the nucleus) or logarithmic"));
  p_labelXAxis = new wxStaticText(this, wxID_ANY, "r axis");
  wxBoxSizer *xPair = new wxBoxSizer(wxHORIZONTAL);
  xPair->Add(p_labelXAxis, 0, RIGHT, 4);
  xPair->Add(p_xaxis, 0, wxALIGN_CENTER_VERTICAL);
  row2->Add(xPair, 0, RIGHT, 12);
  p_yaxis = new wxChoice(this, ID_YAXIS);
  p_yaxis->Append("Linear");
  p_yaxis->Append("Log");
  p_yaxis->SetSelection(0);
  p_yaxis->SetToolTip(U("Log shows the weak tails; only for quantities that are never negative"));
  p_labelYAxis = new wxStaticText(this, wxID_ANY, "y axis");
  wxBoxSizer *yPair = new wxBoxSizer(wxHORIZONTAL);
  yPair->Add(p_labelYAxis, 0, RIGHT, 4);
  yPair->Add(p_yaxis, 0, wxALIGN_CENTER_VERTICAL);
  row2->Add(yPair, 0, RIGHT, 12);
  wxButton *whole = new wxButton(this, ID_WHOLE, "Show all");
  whole->SetToolTip("Reset the zoom to show the whole curves, including peaks that clip peaks cut off at the nuclei (mouse wheel zooms, Shift+wheel zooms the y axis only, drag pans)");
  row2->Add(whole, 0, RIGHT, 12);
  p_moreButton = new wxButton(this, ID_MORE, U("More \xE2\x96\xB8"));
  row2->Add(p_moreButton, 0, RIGHT, 12);

  //  The folded controls.
  p_more = new wxPanel(this);
  wxWrapSizer *rowMore = new wxWrapSizer(wxHORIZONTAL);
  p_more->SetSizer(rowMore);
  //  Wide enough for both arrows (GTK hides the "+" of a narrow one).
  p_margin = new wxSpinCtrlDouble(p_more, ID_MARGIN, "2.0", wxDefaultPosition,
                                  wxSize(130, -1), wxSP_ARROW_KEYS, 0.0, 8.0, 2.0, 0.5);
  p_margin->SetMinSize(wxSize(130, -1));
  p_margin->SetToolTip(U("How far the line goes beyond the two atoms, in \xC3\x85"));
  p_labelMargin = new wxStaticText(p_more, wxID_ANY, "Margin");
  wxBoxSizer *marginPair = new wxBoxSizer(wxHORIZONTAL);
  marginPair->Add(p_labelMargin, 0, RIGHT, 4);
  marginPair->Add(p_margin, 0, wxALIGN_CENTER_VERTICAL);
  rowMore->Add(marginPair, 0, RIGHT, 12);
  p_shift = new wxSpinCtrlDouble(p_more, ID_SHIFT, "0.0", wxDefaultPosition,
                                 wxSize(130, -1), wxSP_ARROW_KEYS, -4.0, 4.0, 0.0, 0.25);
  p_shift->SetMinSize(wxSize(130, -1));
  p_shift->SetToolTip(U("Move the line sideways, in \xC3\x85, to profile a \xCF\x80 orbital above the bond (zero on the axis itself)"));
  wxStaticText *shiftLabel = new wxStaticText(p_more, wxID_ANY, "Shift line sideways");
  p_shiftDir = new wxChoice(p_more, ID_SHIFTDIR);
  p_shiftDir->Append("out of the plane");
  p_shiftDir->Append("in the plane");
  p_shiftDir->SetSelection(0);
  p_shiftDir->SetToolTip("The molecular plane holds the atoms and the line; any perpendicular for a linear molecule");
  wxBoxSizer *shiftPair = new wxBoxSizer(wxHORIZONTAL);
  shiftPair->Add(shiftLabel, 0, RIGHT, 4);
  shiftPair->Add(p_shift, 0, RIGHT, 4);
  shiftPair->Add(p_shiftDir, 0, wxALIGN_CENTER_VERTICAL);
  rowMore->Add(shiftPair, 0, RIGHT, 12);

  p_unit = new wxChoice(p_more, ID_UNIT);
  p_unit->Append(U("\xC3\x85"));
  p_unit->Append("bohr");
  p_unit->SetSelection(0);
  wxBoxSizer *unitPair = new wxBoxSizer(wxHORIZONTAL);
  unitPair->Add(new wxStaticText(p_more, wxID_ANY, "Unit"), 0, RIGHT, 6);
  unitPair->Add(p_unit, 0, wxALIGN_CENTER_VERTICAL);
  rowMore->Add(unitPair, 0, RIGHT, 12);

  p_combine = new wxCheckBox(p_more, ID_COMBINE, "combine degenerate");
  p_combine->SetToolTip("One curve for each shell (2p) or set of orbitals of equal energy");
  p_combine->SetValue(true);
  rowMore->Add(p_combine, 0, RIGHT, 12);
  p_clip = new wxCheckBox(p_more, ID_CLIP, "clip peaks");
  p_clip->SetToolTip("Clip nuclear peaks: cut the vertical axis above the highest value away from the nuclei, "
                     "so the bond region is not flattened by the peaks at the nuclei");
  p_clip->SetValue(true);
  rowMore->Add(p_clip, 0, RIGHT, 12);
  rowMore->Add(new wxButton(p_more, ID_PNG, "Export .png..."), 0, wxLEFT | wxRIGHT, 4);
  rowMore->Add(new wxButton(p_more, ID_CSV, "Export .csv..."), 0);

  p_list->SetToolTip("Tick the orbitals for the selected views");
  p_inner->SplitHorizontally(p_plot, p_text, -70);
  p_inner->SetMinimumPaneSize(40);
  p_inner->SetSashGravity(1.0);
  p_outer->SplitHorizontally(p_list, p_inner, 70);
  p_outer->SetMinimumPaneSize(34);
  p_outer->SetSashGravity(0.0);

  top->Add(row1, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 6);
  top->Add(row2, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 6);
  top->Add(p_more, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 6);
  top->Add(p_outer, 1, wxEXPAND | wxALL, 4);
  {
    bool open = false;
    ewxConfig *cfg = ewxConfig::getConfig("wxbuilder.ini");
    if (cfg) cfg->Read("/DensityProfile/MoreOpen", &open, false);
    p_moreOpen = open;
  }

  Bind(wxEVT_CHOICE, &DensityProfilePanel::onControl, this);
  Bind(wxEVT_CHECKBOX, &DensityProfilePanel::onControl, this);
  Bind(wxEVT_CHECKLISTBOX, &DensityProfilePanel::onControl, this);
  Bind(wxEVT_SPINCTRLDOUBLE, &DensityProfilePanel::onControl, this);
  Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { zoom("whole"); }, ID_WHOLE);
  Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { setMoreOpen(!p_moreOpen); }, ID_MORE);
  Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    wxFileDialog dlg(this, "Export plot", wxEmptyString, "density-profile.png",
                     "PNG files (*.png)|*.png", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() == wxID_OK && !savePng(dlg.GetPath(), 1100, 700))
      wxMessageBox("The file could not be written.", "Export plot", wxOK | wxICON_ERROR, this);
  }, ID_PNG);
  Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    wxFileDialog dlg(this, "Export curves", wxEmptyString, "density-profile.csv",
                     "CSV files (*.csv)|*.csv", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() == wxID_OK && !exportCsv(dlg.GetPath()))
      wxMessageBox("The file could not be written.", "Export curves", wxOK | wxICON_ERROR, this);
  }, ID_CSV);

  p_shell = RadialDist::shellLabels(getCalculation(), *p_eval, false);
  //  The symmetry label of each orbital, only if it lines up with them
  //  (the MOs panel's rule).
  {
    PropVecString *pv = (PropVecString*) getCalculation()->getProperty("ORBSYM");
    if (pv != 0 && pv->values().size() == (size_t)p_eval->orbitals(false) && p_shell.empty())
      p_sym = pv->values();
  }
  p_occupied.clear();
  for (int m = 1; m <= p_eval->orbitals(false); m++) {
    const bool a = (size_t)m <= p_eval->occ.size() && p_eval->occ[m-1] > 0.0;
    const bool b = p_eval->unrestricted && (size_t)m <= p_eval->occBeta.size() &&
                   p_eval->occBeta[m-1] > 0.0;
    if (a || b) p_occupied.push_back(m);
  }

  //  An atom: shells about it.  A molecule: along its first bond.
  if (natoms == 1) {
    p_mode->SetSelection(1);
  } else {
    p_mode->SetSelection(0);
  }
  p_atomA->SetSelection(0);
  int nearest = natoms > 1 ? 1 : 0;
  double best = 1e300;
  for (int a = 1; a < natoms; a++) {
    double d2 = 0;
    for (int d = 0; d < 3; d++) {
      const double x = p_eval->coordsAng[3*a + d] - p_eval->coordsAng[d];
      d2 += x*x;
    }
    if (d2 < best) { best = d2; nearest = a; }
  }
  p_atomB->SetSelection(nearest);
  p_centre->SetSelection(RadialDist::defaultCentre(*p_eval));

  fillOrbitals();
  chooseDefaults();
  fillQuantities();
  syncControls();
  recompute();
  Layout();
}

void DensityProfilePanel::fillOrbitals()
{
  const std::vector<double>& energy = p_eval->energy;
  const std::vector<double>& occ = p_eval->occ;
  p_list->Clear();
  for (int m = 1; m <= p_eval->orbitals(false); m++) {
    wxString item = wxString::Format("%3d", m);
    if ((size_t)m <= tags().size() && !tags()[m-1].empty())
      item += "  " + U(tags()[m-1].c_str());
    if ((size_t)m <= energy.size()) item += wxString::Format("   E %.4f Ha", energy[m-1]);
    if ((size_t)m <= occ.size()) {
      if (p_eval->unrestricted && (size_t)m <= p_eval->occBeta.size())
        item += wxString::Format("   occ %.1f / %.1f", occ[m-1], p_eval->occBeta[m-1]);
      else
        item += wxString::Format("   occ %.1f", occ[m-1]);
    }
    p_list->Append(item);
  }
}

//  An atom shows its occupied orbitals; a molecule the HOMO and the next one.
void DensityProfilePanel::chooseDefaults()
{
  if (p_eval->symbols.size() == 1) {
    for (int m : p_occupied) p_list->Check((unsigned)m - 1, true);
  } else {
    const int homo = p_eval->homo();
    if (homo >= 1 && homo <= (int)p_list->GetCount()) p_list->Check(homo - 1, true);
    if (homo >= 1 && homo < (int)p_list->GetCount()) p_list->Check(homo, true);
    p_autoActive = (int)p_occupied.size() > AUTO_ABOVE;
  }
}

//  The Show choice of the current list, by name.
std::string DensityProfilePanel::showName() const
{
  const int i = p_show->GetSelection();
  if (p_showDensityList) {
    static const char *n[] = { "all", "totalselected", "selected", "total" };
    return i >= 0 && i < 4 ? n[i] : "all";
  }
  return i == 1 ? "all" : "selected";
}

void DensityProfilePanel::selectShow(const std::string& name)
{
  int i = 0;
  if (p_showDensityList)
    i = name == "all" ? 0 : name == "totalselected" ? 1 : name == "total" ? 3 : 2;
  else
    i = name == "all" ? 1 : 0;
  p_show->SetSelection(i);
}

//  The quantity and show choices that apply to the mode and quantity.  A
//  change of quantity keeps what the user is looking at: the same kind of
//  show, so the ticked orbitals stay the ones plotted.
void DensityProfilePanel::fillQuantities(bool resetShow)
{
  const bool atom = p_mode->GetSelection() == 1;
  const int q = p_quantity->GetSelection();
  const std::string was = resetShow ? std::string("all") : showName();
  p_quantity->Clear();
  if (atom) {
    p_quantity->Append("Radial distribution");
    p_quantity->Append(U("Spherical average of \xCF\x88\xC2\xB2"));
    p_quantity->SetToolTip(U("r\xC2\xB2 \xE2\x88\xAE\xCF\x88\xC2\xB2 d\xCE\xA9 against the plain average \xE2\x88\xAE\xCF\x88\xC2\xB2 d\xCE\xA9 / 4\xCF\x80"));
  } else {
    p_quantity->Append(U("Density, n\xC2\xB7\xCF\x88\xC2\xB2 per orbital"));
    p_quantity->Append(U("\xCF\x88\xC2\xB2 of each orbital"));
    p_quantity->Append(U("\xCF\x88, signed"));
    p_quantity->SetToolTip(U("\xCF\x88 shows the sign (phase) and the nodes of an orbital"));
  }
  p_quantity->SetSelection(q >= 0 && q < (int)p_quantity->GetCount() ? q : 0);
  p_show->Clear();
  p_showDensityList = densityLike();
  if (p_showDensityList) {
    p_show->Append("Total + all occupied");
    p_show->Append("Total + selected orbitals");
    p_show->Append("Selected orbitals");
    p_show->Append("Total only");
  } else {
    p_show->Append("Selected orbitals");
    p_show->Append("All occupied orbitals");
  }
  selectShow(was);
}

//  "Selected orbitals" with nothing ticked would plot nothing: the HOMO.
//  The list then scrolls to the first ticked orbital, so what is plotted
//  is what is in view.
void DensityProfilePanel::ensureSelection()
{
  if (showName() == "selected") {
    bool any = false;
    for (unsigned i = 0; i < p_list->GetCount() && !any; i++) any = p_list->IsChecked(i);
    const int homo = p_eval->homo();
    if (!any && homo >= 1 && homo <= (int)p_list->GetCount()) p_list->Check(homo - 1, true);
  }
  for (unsigned i = 0; i < p_list->GetCount(); i++)
    if (p_list->IsChecked(i)) { p_list->EnsureVisible((int)i); break; }
}

bool DensityProfilePanel::densityLike() const
{
  return p_mode->GetSelection() == 1 || p_quantity->GetSelection() <= 0;
}

void DensityProfilePanel::syncControls()
{
  const bool atom = p_mode->GetSelection() == 1;
  for (wxWindow *w : std::initializer_list<wxWindow*>{ p_atomA, p_atomB, p_labelA,
       p_labelB, p_margin, p_labelMargin, p_shift, p_shiftDir })
    w->Show(!atom);
  p_centre->Show(atom);
  p_labelCentre->Show(atom);
  p_combine->Enable(!(!atom && p_quantity->GetSelection() == 2));
  p_clip->Show(!atom);
  //  The r axis is only a choice about an atom; a line has one length.
  if (!atom) p_xaxis->SetSelection(0);
  p_xaxis->Show(atom);
  p_labelXAxis->Show(atom);
  //  Log needs values that are never negative.
  const bool signedPsi = !atom && p_quantity->GetSelection() == 2;
  if (signedPsi) p_yaxis->SetSelection(0);
  p_yaxis->Enable(!signedPsi);
  p_moreButton->SetLabel(p_moreOpen ? U("More \xE2\x96\xBE") : U("More \xE2\x96\xB8"));
  p_more->Show(p_moreOpen);
  Layout();
}

void DensityProfilePanel::remember() const
{
  ewxConfig *cfg = ewxConfig::getConfig("wxbuilder.ini");
  if (!cfg) return;
  cfg->Write("/DensityProfile/MoreOpen", p_moreOpen);
  cfg->Flush();
}

void DensityProfilePanel::recompute()
{
  if (!p_ok || p_busy) return;
  //  Cleared on every way out, an exception included: a stuck flag would
  //  make every later control do nothing.
  struct Busy {
    bool& f;
    explicit Busy(bool& b) : f(b) { f = true; }
    ~Busy() { f = false; }
  } busyFlag(p_busy);
  wxBusyCursor busy;
  const bool atom = p_mode->GetSelection() == 1;
  const bool bohr = p_unit->GetSelection() == 1;
  const bool like = densityLike();
  const int quantity = p_quantity->GetSelection();
  const bool bohrAuto = bohr;
  if (p_autoActive && !atom && like && p_show->GetSelection() <= 1) rankLargest(bohrAuto);
  const int showIdx = p_show->GetSelection();
  //  Density-like: 0 total + all, 1 total + selected, 2 selected, 3 total only.
  //  Otherwise: 0 selected, 1 all occupied.
  const bool wantTotal = like && (showIdx == 0 || showIdx == 1 || showIdx == 3);
  const bool allOccupied = like ? showIdx == 0 : showIdx == 1;
  const bool noOrbitals = like && showIdx == 3;
  const bool weighted = like && wantTotal;
  const RadialDist::Spin spin = !p_eval->unrestricted ? RadialDist::ALPHA
      : p_spin->GetSelection() == 0 ? RadialDist::ALPHA
      : p_spin->GetSelection() == 1 ? RadialDist::BETA : RadialDist::BOTH;

  std::vector<int> mos;
  if (!noOrbitals) {
    if (allOccupied) mos = p_occupied;
    else for (unsigned i = 0; i < p_list->GetCount(); i++)
      if (p_list->IsChecked(i)) mos.push_back((int)i + 1);
  }
  std::vector<RadialDist::Spec> specs = RadialDist::orbitalSpecs(
      *p_eval, mos, spin, weighted, p_combine->GetValue() && !(!atom && quantity == 2),
      tags(), tagsUnique());
  if (wantTotal) {
    RadialDist::Spec t;
    t.label = "Total density";
    specs.push_back(t);
  }

  std::vector<RadialDist::Curve> curves;
  std::vector<double> axis;
  p_marks.clear();
  //  Short labels that name the quantity, so they fit a small plot.
  const wxString length = bohr ? wxString("bohr") : U("Å");
  const wxString perVol = length + U("⁻³");
  wxString yLabel, xLabel;
  if (atom) {
    RadialDist::Options opt;
    opt.bohr = bohr;
    opt.radial = quantity == 0;
    opt.rMax = bohr ? 6.0 / BOHR_ANG : 6.0;
    opt.points = GRID_POINTS;
    opt.lebedev = ANGULAR_POINTS;
    //  Points even along the axis as drawn.
    const int xsel = p_xaxis->GetSelection();
    opt.spacing = xsel == 1 ? RadialDist::Options::SQRT
                : xsel == 2 ? RadialDist::Options::LOG : RadialDist::Options::EVEN;
    const int a = std::max(0, p_centre->GetSelection());
    const double c[3] = { p_eval->coordsAng[3*a], p_eval->coordsAng[3*a+1],
                          p_eval->coordsAng[3*a+2] };
    if (!specs.empty()) RadialDist::compute(*p_eval, c, opt, specs, curves);
    axis = RadialDist::grid(opt);
    xLabel = "r (" + length + ")";
    yLabel = quantity == 0 ? U("r²ψ², radial distribution (") + length + U("⁻¹)")
                           : U("ψ², averaged over directions (") + perVol + ")";
  } else {
    const int a = std::max(0, p_atomA->GetSelection()), b = std::max(0, p_atomB->GetSelection());
    if (a == b) {
      p_plot->clearCurves();
      p_plot->clearVLines();
      p_text->SetValue("Choose two different atoms.");
      return;
    }
    const LineProfile::Line line = LineProfile::throughAtoms(*p_eval, a, b, p_margin->GetValue(),
        p_shift->GetValue(),
        p_shiftDir->GetSelection() == 0);
    LineProfile::Result r;
    const LineProfile::Quantity q = quantity == 2 ? LineProfile::PSI
        : quantity == 1 ? LineProfile::PSI2 : LineProfile::DENSITY;
    if (!specs.empty()) LineProfile::compute(*p_eval, line, 801, bohr, q, specs, r);
    curves = r.curves;
    axis = r.s;
    p_marks = r.marks;
    xLabel = "position along the line (" + length + ")";
    yLabel = quantity == 2 ? U("ψ (") + length + U("⁻³/²)")
           : quantity == 1 ? U("ψ² (") + perVol + ")"
                           : U("density ρ (") + perVol + ")";
  }

  p_axis = axis;
  p_shown = curves;
  p_sumError = -1.0;
  p_sumPeak = 0.0;
  if (weighted && allOccupied && (spin == RadialDist::BOTH || !p_eval->unrestricted)) {
    p_sumError = RadialDist::maxSumError(curves);
    for (const RadialDist::Curve& c : curves)
      if (c.spec.total) for (double v : c.p) p_sumPeak = std::max(p_sumPeak, v);
  }

  const int xs = atom ? p_xaxis->GetSelection() : 0;
  const bool yLog = p_yaxis->IsEnabled() && p_yaxis->GetSelection() == 1;
  p_plot->clearCurves();
  p_plot->clearVLines();
  p_plot->setXScale(xs == 1 ? XYPlotCanvas::X_SQRT : xs == 2 ? XYPlotCanvas::X_LOG
                                                             : XYPlotCanvas::X_LINEAR);
  p_plot->setYLog(yLog);
  p_plot->setXAxisLabel(xs == 1 ? U("√r, labelled in r (") + length + ")"
                      : xs == 2 ? "log r, labelled in r (" + length + ")" : xLabel);
  p_plot->setYAxisLabel(yLabel);
  p_plot->setShowKey(true);
  p_plot->setKeyMaxRows(2);
  p_plot->setZeroLine(!atom && quantity == 2);
  //  Zero everywhere: a node, not a curve to draw ticks for.
  double biggest = 0;
  for (const RadialDist::Curve& c : curves)
    for (double v : c.p) biggest = std::max(biggest, std::fabs(v));
  const bool allZero = !atom && !curves.empty() && biggest < 1e-9;
  if (allZero)
    p_plot->setMessage(p_shift->GetValue() == 0.0
        ? U("zero on this line — π orbital: shift the line off the axis")
        : U("zero on this line — try another shift or direction"));
  else
    p_plot->setMessage(wxString());
  //  Line: the cut is the highest value 0.35 A or more from every atom.
  double limit = 0.0;
  if (!atom && !yLog && p_clip->GetValue() && quantity != 2 && !p_marks.empty()) {
    const double away = 0.35 / (bohr ? BOHR_ANG : 1.0);
    double top = 0, all = 0;
    for (const RadialDist::Curve& c : curves)
      for (size_t i = 0; i < c.p.size(); i++) {
        all = std::max(all, c.p[i]);
        bool near = false;
        for (const LineProfile::Mark& m : p_marks)
          if (std::fabs(axis[i] - m.s) < away) near = true;
        if (!near) top = std::max(top, c.p[i]);
      }
    if (top > 0 && all > 3.0 * top) limit = 1.5 * top;   // only a tall nuclear peak
  }
  p_plot->setYLimit(limit);
  for (const LineProfile::Mark& m : p_marks) p_plot->addVLine(m.s, U(m.label.c_str()));
  wxString text;
  if (p_autoActive && !atom && like && showIdx == 1) text += p_autoNote;
  if (xs != 0 && quantity == 0)
    text += U("On this r axis the area under a curve is no longer the electron count.\n");
  if (limit > 0)
    text += U("Peaks at the nuclei are cut off; \"Show all\" shows them.\n");
  int series = 0, colourIndex = 0;
  for (const RadialDist::Curve& c : curves) {
    std::vector<double> plotR(axis), plotP(c.p);
    if (atom) {
      //  Only as far out as the curve is visible.
      double topV = 0;
      for (double v : c.p) topV = std::max(topV, v);
      size_t last = 1;
      const double visible = yLog ? 1e-9 : 0.002;    // a log axis shows the tail
      for (size_t i = 0; i < c.p.size(); i++) if (c.p[i] > visible * topV) last = i;
      last = std::min(c.p.size() - 1, last + last / 8 + 2);
      plotR.resize(last + 1);
      plotP.resize(last + 1);
    }
    //  The total is black and heavier; no two others share a colour.
    const bool total = c.spec.total;
    p_plot->addCurve(plotR, plotP, U(c.spec.label.c_str()), series, XYPlotCanvas::LINE);
    const int idx = p_plot->curveCount() - 1;
    wxColour colour;
    if (total) {
      colour = p_plot->palette().text;
      p_plot->setCurveStyle(idx, false, 3);
    } else {
      colour = curveColour(colourIndex);
      p_plot->setCurveStyle(idx, colourIndex >= 12, 2);
      colourIndex++;
    }
    p_plot->setCurveColour(idx, colour);
    if (atom && !c.maxR.empty()) {
      const int m = p_plot->addCurve(c.maxR, c.maxP, wxString(), series, XYPlotCanvas::MARKERS);
      p_plot->setCurveColour(m, colour);
    }
    text += U(c.spec.label.c_str()) + ": ";
    text += atom ? radialText(c, quantity == 0, length) : lineText(c, axis, p_marks, quantity, length);
    text += "\n";
    series++;
  }
  if (p_sumError >= 0)
    text += wxString::Format("contributions sum to the total: max deviation %.1e (total peak %.4g)\n",
                             p_sumError, p_sumPeak);
  p_text->SetValue(text);
  p_plot->Refresh();
}

//  A molecule with many occupied orbitals: all of them in one plot is
//  unreadable, so show the total and the few that contribute most along the line.
void DensityProfilePanel::rankLargest(bool bohr)
{
  const int a = std::max(0, p_atomA->GetSelection()), b = std::max(0, p_atomB->GetSelection());
  if (a == b) return;
  const RadialDist::Spin spin = !p_eval->unrestricted ? RadialDist::ALPHA
      : p_spin->GetSelection() == 0 ? RadialDist::ALPHA
      : p_spin->GetSelection() == 1 ? RadialDist::BETA : RadialDist::BOTH;
  std::vector<RadialDist::Spec> specs = RadialDist::orbitalSpecs(
      *p_eval, p_occupied, spin, true, false, tags(), tagsUnique());
  const LineProfile::Line line = LineProfile::throughAtoms(*p_eval, a, b, p_margin->GetValue(),
      p_shift->GetValue(), p_shiftDir->GetSelection() == 0);
  LineProfile::Result r;
  LineProfile::compute(*p_eval, line, 201, bohr, LineProfile::DENSITY, specs, r);
  std::vector<std::pair<double, int> > rank;   // integral along the line, MO
  for (size_t k = 0; k < r.curves.size() && k < p_occupied.size(); k++) {
    double sum = 0;
    for (size_t i = 1; i < r.s.size() && i < r.curves[k].p.size(); i++)
      sum += 0.5 * (r.curves[k].p[i] + r.curves[k].p[i-1]) * (r.s[i] - r.s[i-1]);
    rank.push_back(std::make_pair(sum, p_occupied[k]));
  }
  std::sort(rank.begin(), rank.end(), std::greater<std::pair<double, int> >());
  for (unsigned i = 0; i < p_list->GetCount(); i++) p_list->Check(i, false);
  const size_t keep = std::min<size_t>(AUTO_ORBITALS, rank.size());
  for (size_t i = 0; i < keep; i++) p_list->Check((unsigned)rank[i].second - 1, true);
  selectShow("totalselected");
  p_autoNote = wxString::Format(
      "Total and the %d largest contributions along the line (of %d occupied); "
      "\"All occupied orbitals\" shows every one.\n", (int)keep, (int)p_occupied.size());
}

void DensityProfilePanel::onControl(wxCommandEvent& event)
{
  if (!p_ok) return;
  const wxWindow *w = dynamic_cast<wxWindow*>(event.GetEventObject());
  const wxWindowID id = w ? w->GetId() : 0;
  if (id == ID_MODE || id == ID_SHOW || id == ID_LIST) p_autoActive = false;
  if (id == ID_MODE) {
    p_quantity->SetSelection(0);
    fillQuantities(true);
    syncControls();
  } else if (id == ID_QUANTITY) {
    fillQuantities();
    ensureSelection();
    syncControls();
  } else if (id == ID_SHOW) {
    ensureSelection();
  } else if (id == ID_XAXIS || id == ID_YAXIS) {
    syncControls();
  }
  recompute();
}

bool DensityProfilePanel::setMode(bool atomMode)
{
  if (!p_ok) return false;
  if (atomMode) p_autoActive = false;
  p_mode->SetSelection(atomMode ? 1 : 0);
  p_quantity->SetSelection(0);
  fillQuantities(true);
  syncControls();
  recompute();
  return true;
}

bool DensityProfilePanel::setAtoms(int a, int b)
{
  const int n = (int)p_eval->symbols.size();
  if (!p_ok || a < 1 || b < 1 || a > n || b > n || a == b) return false;
  p_atomA->SetSelection(a - 1);
  p_atomB->SetSelection(b - 1);
  recompute();
  return true;
}

bool DensityProfilePanel::pickAtoms(const std::vector<int>& atoms)
{
  ensureInitialized();
  if (!p_ok || (atoms.size() != 1 && atoms.size() != 2)) return false;
  const int n = (int)p_eval->symbols.size();
  for (size_t i = 0; i < atoms.size(); i++)
    if (atoms[i] < 1 || atoms[i] > n) return false;
  if (atoms.size() == 2 && atoms[0] == atoms[1]) return false;
  if (atoms.size() == 2) {
    p_atomA->SetSelection(atoms[0] - 1);
    p_atomB->SetSelection(atoms[1] - 1);
  } else {
    p_centre->SetSelection(atoms[0] - 1);
  }
  const bool atomMode = atoms.size() == 1;
  if ((p_mode->GetSelection() == 1) != atomMode) return setMode(atomMode);
  recompute();
  return true;
}

void DensityProfilePanel::eventMCB(const ecce::Event& event)
{
  if (event.getName() != "SelectionChanged" || !p_ok || !IsShown() || !p_vizfw)
    return;
  SGFragment *frag = getFW().getSceneGraph().getFragment();
  if (!frag) return;
  std::vector<int> picked;
  for (size_t i = 0; i < frag->m_atomHighLight.size(); i++) {
    const int a = frag->m_atomHighLight[i];
    if (a >= 0 && a < (int)p_eval->symbols.size() &&
        frag->atomRef(a)->atomicSymbol() != "Nub")
      picked.push_back(a + 1);
  }
  if (picked.size() == 1 || picked.size() == 2) pickAtoms(picked);
}

bool DensityProfilePanel::setMargin(double angstrom)
{
  if (!p_ok || angstrom < 0 || angstrom > 8) return false;
  p_margin->SetValue(angstrom);
  recompute();
  return true;
}

bool DensityProfilePanel::setCentre(int atom)
{
  if (!p_ok || atom < 1 || atom > (int)p_centre->GetCount()) return false;
  p_centre->SetSelection(atom - 1);
  recompute();
  return true;
}

bool DensityProfilePanel::setQuantity(const std::string& name)
{
  if (!p_ok) return false;
  const bool atom = p_mode->GetSelection() == 1;
  static const char *lineNames[] = { "density", "psi2", "psi" };
  static const char *atomNames[] = { "radial", "avg" };
  const int n = atom ? 2 : 3;
  for (int i = 0; i < n; i++) {
    if (name == (atom ? atomNames[i] : lineNames[i])) {
      p_quantity->SetSelection(i);
      fillQuantities();
      ensureSelection();
      syncControls();
      recompute();
      return true;
    }
  }
  return false;
}

bool DensityProfilePanel::setShow(const std::string& name)
{
  if (!p_ok) return false;
  int i = -1;
  if (densityLike()) {
    i = name == "all" ? 0 : name == "totalselected" ? 1 : name == "selected" ? 2
        : name == "total" ? 3 : -1;
  } else {
    i = name == "selected" ? 0 : name == "all" ? 1 : -1;
  }
  if (i < 0) return false;
  p_autoActive = false;
  p_show->SetSelection(i);
  ensureSelection();
  recompute();
  return true;
}

bool DensityProfilePanel::setOrbitals(const std::vector<int>& mos)
{
  if (!p_ok) return false;
  for (int m : mos) if (m < 1 || m > (int)p_list->GetCount()) return false;
  p_autoActive = false;
  for (unsigned i = 0; i < p_list->GetCount(); i++) p_list->Check(i, false);
  for (int m : mos) p_list->Check((unsigned)m - 1, true);
  recompute();
  return true;
}

bool DensityProfilePanel::setSpin(const std::string& name)
{
  if (!p_ok || !p_eval->unrestricted) return false;
  const int i = name == "alpha" ? 0 : name == "beta" ? 1 : name == "both" ? 2 : -1;
  if (i < 0) return false;
  p_spin->SetSelection(i);
  recompute();
  return true;
}

bool DensityProfilePanel::setBohr(bool bohr)
{
  if (!p_ok) return false;
  p_unit->SetSelection(bohr ? 1 : 0);
  recompute();
  return true;
}

bool DensityProfilePanel::setCombine(bool on)
{
  if (!p_ok) return false;
  p_combine->SetValue(on);
  recompute();
  return true;
}

bool DensityProfilePanel::setClip(bool on)
{
  if (!p_ok) return false;
  p_clip->SetValue(on);
  recompute();
  return true;
}

bool DensityProfilePanel::setXAxis(const std::string& name)
{
  if (!p_ok || p_mode->GetSelection() != 1) return false;
  const int i = name == "linear" ? 0 : name == "sqrt" ? 1 : name == "log" ? 2 : -1;
  if (i < 0) return false;
  p_xaxis->SetSelection(i);
  syncControls();
  recompute();
  return true;
}

bool DensityProfilePanel::setYAxis(const std::string& name)
{
  if (!p_ok) return false;
  const int i = name == "linear" ? 0 : name == "log" ? 1 : -1;
  if (i < 0 || (i == 1 && !p_yaxis->IsEnabled())) return false;
  p_yaxis->SetSelection(i);
  recompute();
  return true;
}

bool DensityProfilePanel::setShift(double angstrom, bool outOfPlane)
{
  if (!p_ok || p_mode->GetSelection() == 1 || angstrom < -4 || angstrom > 4) return false;
  p_shift->SetValue(angstrom);
  p_shiftDir->SetSelection(outOfPlane ? 0 : 1);
  recompute();
  return true;
}

//  The canvas lays itself out when painted; a scripted zoom needs a view.
bool DensityProfilePanel::zoom(const std::string& what)
{
  if (!p_ok) return false;
  wxBitmap bmp(800, 500);
  { wxMemoryDC dc(bmp); p_plot->paintOnto(dc, wxSize(800, 500)); }
  if (what == "whole") p_plot->showWholePlot();
  else if (what == "fit") p_plot->fitAll();
  else if (what == "in") p_plot->zoomBy(0.5);
  else if (what == "out") p_plot->zoomBy(2.0);
  else return false;
  p_plot->Refresh();
  return true;
}

bool DensityProfilePanel::setMoreOpen(bool open)
{
  if (!p_ok) return false;
  p_moreOpen = open;
  syncControls();
  remember();
  return true;
}

void DensityProfilePanel::followMo(int mo, bool beta)
{
  if (!p_ok || !IsShown() || mo < 1 || mo > (int)p_list->GetCount()) return;
  p_autoActive = false;
  if (p_eval->unrestricted) p_spin->SetSelection(beta ? 1 : 0);
  for (unsigned i = 0; i < p_list->GetCount(); i++) p_list->Check(i, (int)i + 1 == mo);
  selectShow("selected");
  ensureSelection();
  if (IsShown()) recompute();
}

bool DensityProfilePanel::exportCsv(const wxString& path)
{
  if (!p_ok || p_shown.empty()) return false;
  FILE *f = fopen((const char*)path.utf8_str(), "w");
  if (!f) return false;
  const bool bohr = p_unit->GetSelection() == 1;
  std::string text;
  if (p_mode->GetSelection() == 1) {
    text = RadialDist::csv(p_axis, p_shown, bohr);
  } else {
    LineProfile::Result r;
    r.s = p_axis;
    r.curves = p_shown;
    r.marks = p_marks;
    text = LineProfile::csv(r, bohr);
  }
  fputs(text.c_str(), f);
  return fclose(f) == 0;
}

bool DensityProfilePanel::savePng(const wxString& path, int width, int height)
{
  if (!p_ok || !p_plot) return false;
  static bool handlers = false;
  if (!handlers) { wxInitAllImageHandlers(); handlers = true; }
  wxBitmap bmp(width, height);
  {
    wxMemoryDC dc(bmp);
    p_plot->paintOnto(dc, wxSize(width, height));
  }
  return bmp.SaveFile(path, wxBITMAP_TYPE_PNG);
}

std::string DensityProfilePanel::state() const
{
  if (!p_ok) return "ok 0\n";
  std::string s = "ok 1\n";
  char buf[400];
  snprintf(buf, sizeof buf, "mode %s\nquantity %d\nshow %d\nunit %s\ncurves %d\nmarks %d\nsumerror %.3e\n",
           p_mode->GetSelection() ? "atom" : "line", p_quantity->GetSelection(),
           p_show->GetSelection(), p_unit->GetSelection() ? "bohr" : "A",
           (int)p_shown.size(), (int)p_marks.size(), p_sumError);
  s += buf;
  //  What the controls show: axes, shift, folded controls, view, ticked orbitals.
  {
    std::string ticked;
    for (unsigned i = 0; i < p_list->GetCount(); i++)
      if (p_list->IsChecked(i)) ticked += (ticked.empty() ? "" : ",") + std::to_string(i + 1);
    wxBitmap bmp(800, 500);
    { wxMemoryDC dc(bmp); p_plot->paintOnto(dc, wxSize(800, 500)); }
    double x0, x1, y0, y1;
    p_plot->viewRange(&x0, &x1, &y0, &y1);
    snprintf(buf, sizeof buf,
             "xaxis %s\nyaxis %s%s\nshift %.3f %s\nmore %d\nticked %s\nview %.6g %.6g %.6g %.6g\n"
             "message %d\nkeyrows %d\nplotheight %d\nline %d %d\ncentre %d\nauto %d\n",
             p_xaxis->GetSelection() == 1 ? "sqrt" : p_xaxis->GetSelection() == 2 ? "log" : "linear",
             p_yaxis->GetSelection() == 1 ? "log" : "linear", p_yaxis->IsEnabled() ? "" : " (disabled)",
             p_shift->GetValue(), p_shiftDir->GetSelection() == 0 ? "out" : "in",
             p_moreOpen ? 1 : 0, ticked.empty() ? "none" : ticked.c_str(), x0, x1, y0, y1,
             p_plot->message().empty() ? 0 : 1, p_plot->keyRowsDrawn(), p_plot->plotHeight(),
             p_atomA->GetSelection() + 1, p_atomB->GetSelection() + 1,
             p_centre->GetSelection() + 1, p_autoActive ? 1 : 0);
    s += buf;
    for (int c = 0; c < p_plot->curveCount(); c++)
      if (!p_plot->curveName(c).empty()) {
        const wxColour col = p_plot->curveColourOf(c);
        snprintf(buf, sizeof buf, "colour '%s' %02x%02x%02x\n", (const char*)p_plot->curveName(c).utf8_str(),
                 col.Red(), col.Green(), col.Blue());
        s += buf;
      }
  }
  for (const LineProfile::Mark& m : p_marks) {
    snprintf(buf, sizeof buf, "mark %s %.4f\n", m.label.c_str(), m.s);
    s += buf;
  }
  for (const RadialDist::Curve& c : p_shown) {
    snprintf(buf, sizeof buf, "curve '%s' integral %.6f maxima %d", c.spec.label.c_str(),
             c.integral, (int)c.maxR.size());
    s += buf;
    for (double r : c.maxR) { snprintf(buf, sizeof buf, " %.4f", r); s += buf; }
    s += "\n";
  }
  return s;
}
