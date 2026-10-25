#include <algorithm>
#include <cmath>
#include <cstdio>

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
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/utils.h>
#include <wx/wrapsizer.h>

#include "dsm/IPropCalculation.H"
#include "tdat/PropVecString.H"

#include "DensityProfilePanel.H"
#include "XYPlotCanvas.H"

namespace {

const wxWindowID ID_MODE = wxNewId(), ID_ATOM = wxNewId(), ID_QUANTITY = wxNewId(),
                 ID_SHOW = wxNewId(), ID_SPIN = wxNewId(), ID_UNIT = wxNewId(),
                 ID_LIST = wxNewId(), ID_COMBINE = wxNewId(), ID_CLIP = wxNewId(), ID_MARGIN = wxNewId(),
                 ID_PNG = wxNewId(), ID_CSV = wxNewId();

const double BOHR_ANG = 0.52917721092;

//  Interactive accuracy: 0.0075 A steps and 302 directions resolve every
//  maximum of a first-row atom to well under 0.01 A.
const int GRID_POINTS = 801;
const int ANGULAR_POINTS = 302;

wxString U(const char *utf8) { return wxString::FromUTF8(utf8); }

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


IMPLEMENT_DYNAMIC_CLASS(DensityProfilePanel, PropertyPanel)
wxFORCE_LINK_THIS_MODULE(DensityProfilePanel)

DensityProfilePanel::DensityProfilePanel()
  : PropertyPanel(), p_sumError(-1.0), p_sumPeak(0.0), p_ok(false), p_busy(false),
    p_atomsOnly(false), p_mode(0), p_atomA(0), p_atomB(0), p_centre(0), p_quantity(0),
    p_show(0), p_spin(0), p_unit(0), p_margin(0), p_labelA(0), p_labelB(0),
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
}

bool DensityProfilePanel::Create(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
{
  if (!PropertyPanel::Create(calculation, parent, id, pos, size, style, name)) {
    wxFAIL_MSG( wxT("DensityProfilePanel creation failed") );
    return false;
  }
  SetSizer(new wxBoxSizer(wxVERTICAL));
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

  p_plot = new XYPlotCanvas(this);
  p_plot->SetMinSize(wxSize(240, 160));
  p_text = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
                          wxSize(-1, 44), wxTE_MULTILINE | wxTE_READONLY);
  if (!p_ok) {
    top->Add(new wxStaticText(this, wxID_ANY,
             wxString::FromUTF8(p_eval->complaint.c_str())), 0, wxALL, 8);
    return;
  }
  const int natoms = (int)p_eval->symbols.size();

  //  Rows wrap instead of widening the panel: none is wider than the dock.
  wxWrapSizer *row1 = new wxWrapSizer(wxHORIZONTAL);
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
  p_margin = new wxSpinCtrlDouble(this, ID_MARGIN, "2.0", wxDefaultPosition,
                                  wxSize(90, -1), wxSP_ARROW_KEYS, 0.0, 8.0, 2.0, 0.5);
  p_margin->SetMinSize(wxSize(90, -1));
  p_margin->SetToolTip(U("How far the line goes beyond the two atoms, in \xC3\x85"));
  p_labelMargin = new wxStaticText(this, wxID_ANY, "Margin");
  wxBoxSizer *marginPair = new wxBoxSizer(wxHORIZONTAL);
  marginPair->Add(p_labelMargin, 0, RIGHT, 4);
  marginPair->Add(p_margin, 0, wxALIGN_CENTER_VERTICAL);
  row1->Add(marginPair, 0, RIGHT, 10);

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

  p_unit = new wxChoice(this, ID_UNIT);
  p_unit->Append(U("\xC3\x85"));
  p_unit->Append("bohr");
  p_unit->SetSelection(0);
  wxBoxSizer *unitPair = new wxBoxSizer(wxHORIZONTAL);
  unitPair->Add(new wxStaticText(this, wxID_ANY, "Unit"), 0, RIGHT, 6);
  unitPair->Add(p_unit, 0, wxALIGN_CENTER_VERTICAL);
  row1->Add(unitPair, 0, RIGHT, 12);

  p_combine = new wxCheckBox(this, ID_COMBINE, "combine degenerate");
  p_combine->SetToolTip("One curve for each shell (2p) or set of orbitals of equal energy");
  p_combine->SetValue(true);
  row1->Add(p_combine, 0, RIGHT, 12);
  p_clip = new wxCheckBox(this, ID_CLIP, "clip peaks");
  p_clip->SetToolTip("Clip nuclear peaks: cut the vertical axis above the highest value away from the nuclei, "
                     "so the bond region is not flattened by the peaks at the nuclei");
  p_clip->SetValue(true);
  row1->Add(p_clip, 0, RIGHT, 12);
  row1->Add(new wxButton(this, ID_PNG, "Export .png..."), 0, wxLEFT | wxRIGHT, 4);
  row1->Add(new wxButton(this, ID_CSV, "Export .csv..."), 0);

  p_list = new wxCheckListBox(this, ID_LIST, wxDefaultPosition, wxSize(-1, 46));
  p_list->SetToolTip("Tick the orbitals for the selected views");

  top->Add(row1, 0, wxEXPAND | wxALL, 6);
  top->Add(p_list, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
  top->Add(p_plot, 1, wxEXPAND);
  top->Add(p_text, 0, wxEXPAND | wxALL, 6);

  Bind(wxEVT_CHOICE, &DensityProfilePanel::onControl, this);
  Bind(wxEVT_CHECKBOX, &DensityProfilePanel::onControl, this);
  Bind(wxEVT_CHECKLISTBOX, &DensityProfilePanel::onControl, this);
  Bind(wxEVT_SPINCTRLDOUBLE, &DensityProfilePanel::onControl, this);
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
  }
}

//  The quantity and show choices that apply to the mode and quantity.
void DensityProfilePanel::fillQuantities()
{
  const bool atom = p_mode->GetSelection() == 1;
  const int q = p_quantity->GetSelection();
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
  const int s = p_show->GetSelection();
  p_show->Clear();
  if (densityLike()) {
    p_show->Append("Total + all occupied");
    p_show->Append("Total + selected orbitals");
    p_show->Append("Selected orbitals");
    p_show->Append("Total only");
  } else {
    p_show->Append("Selected orbitals");
    p_show->Append("All occupied orbitals");
  }
  p_show->SetSelection(s >= 0 && s < (int)p_show->GetCount() ? s : 0);
  if (!densityLike() && s < 0) p_show->SetSelection(0);
}

bool DensityProfilePanel::densityLike() const
{
  return p_mode->GetSelection() == 1 || p_quantity->GetSelection() <= 0;
}

void DensityProfilePanel::syncControls()
{
  const bool atom = p_mode->GetSelection() == 1;
  for (wxWindow *w : std::initializer_list<wxWindow*>{ p_atomA, p_atomB, p_labelA,
       p_labelB, p_margin, p_labelMargin })
    w->Show(!atom);
  p_centre->Show(atom);
  p_labelCentre->Show(atom);
  p_combine->Enable(!(!atom && p_quantity->GetSelection() == 2));
  p_clip->Show(!atom);
  Layout();
}

void DensityProfilePanel::recompute()
{
  if (!p_ok || p_busy) return;
  p_busy = true;
  wxBusyCursor busy;
  const bool atom = p_mode->GetSelection() == 1;
  const bool bohr = p_unit->GetSelection() == 1;
  const bool like = densityLike();
  const int quantity = p_quantity->GetSelection();
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
  const wxString length = bohr ? wxString("bohr") : U("\xC3\x85");
  const wxString perVol = bohr ? wxString("bohr^-3") : U("\xC3\x85^-3");
  wxString yLabel, xLabel;
  if (atom) {
    RadialDist::Options opt;
    opt.bohr = bohr;
    opt.radial = quantity == 0;
    opt.rMax = bohr ? 6.0 / BOHR_ANG : 6.0;
    opt.points = GRID_POINTS;
    opt.lebedev = ANGULAR_POINTS;
    const int a = std::max(0, p_centre->GetSelection());
    const double c[3] = { p_eval->coordsAng[3*a], p_eval->coordsAng[3*a+1],
                          p_eval->coordsAng[3*a+2] };
    if (!specs.empty()) RadialDist::compute(*p_eval, c, opt, specs, curves);
    axis = RadialDist::grid(opt);
    xLabel = "r (" + length + ")";
    yLabel = quantity == 0 ? "radial distribution (1/" + length + ")"
                           : U("spherical average of \xCF\x88\xC2\xB2 (") + perVol + ")";
  } else {
    const int a = std::max(0, p_atomA->GetSelection()), b = std::max(0, p_atomB->GetSelection());
    if (a == b) {
      p_plot->clearCurves();
      p_plot->clearVLines();
      p_text->SetValue("Choose two different atoms.");
      p_busy = false;
      return;
    }
    const LineProfile::Line line = LineProfile::throughAtoms(*p_eval, a, b, p_margin->GetValue());
    LineProfile::Result r;
    const LineProfile::Quantity q = quantity == 2 ? LineProfile::PSI
        : quantity == 1 ? LineProfile::PSI2 : LineProfile::DENSITY;
    if (!specs.empty()) LineProfile::compute(*p_eval, line, 801, bohr, q, specs, r);
    curves = r.curves;
    axis = r.s;
    p_marks = r.marks;
    xLabel = "position along the line (" + length + ")";
    yLabel = quantity == 2 ? U("\xCF\x88 (") + length + "^-3/2)"
           : quantity == 1 ? U("\xCF\x88\xC2\xB2 (") + perVol + ")"
                           : U("density (") + perVol + ")";
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

  p_plot->clearCurves();
  p_plot->clearVLines();
  p_plot->setXAxisLabel(xLabel);
  p_plot->setYAxisLabel(yLabel);
  p_plot->setShowKey(true);
  p_plot->setZeroLine(!atom && quantity == 2);
  //  Line: the cut is the highest value 0.35 A or more from every atom.
  double limit = 0.0;
  if (!atom && p_clip->GetValue() && quantity != 2 && !p_marks.empty()) {
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
  int series = 0;
  for (const RadialDist::Curve& c : curves) {
    std::vector<double> plotR(axis), plotP(c.p);
    if (atom) {
      //  Only as far out as the curve is visible.
      double topV = 0;
      for (double v : c.p) topV = std::max(topV, v);
      size_t last = 1;
      for (size_t i = 0; i < c.p.size(); i++) if (c.p[i] > 0.002 * topV) last = i;
      last = std::min(c.p.size() - 1, last + last / 8 + 2);
      plotR.resize(last + 1);
      plotP.resize(last + 1);
    }
    p_plot->addCurve(plotR, plotP, U(c.spec.label.c_str()), series, XYPlotCanvas::LINE);
    if (atom && !c.maxR.empty()) {
      const int m = p_plot->addCurve(c.maxR, c.maxP, wxString(), series, XYPlotCanvas::MARKERS);
      p_plot->setCurveColour(m, p_plot->seriesColour(series));
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
  p_busy = false;
}

void DensityProfilePanel::onControl(wxCommandEvent& event)
{
  if (!p_ok) return;
  const wxWindow *w = dynamic_cast<wxWindow*>(event.GetEventObject());
  const wxWindowID id = w ? w->GetId() : 0;
  if (id == ID_MODE) {
    p_quantity->SetSelection(0);
    p_show->SetSelection(0);
    fillQuantities();
    syncControls();
  } else if (id == ID_QUANTITY) {
    fillQuantities();
    syncControls();
  }
  recompute();
}

bool DensityProfilePanel::setMode(bool atomMode)
{
  if (!p_ok) return false;
  p_mode->SetSelection(atomMode ? 1 : 0);
  p_quantity->SetSelection(0);
  p_show->SetSelection(0);
  fillQuantities();
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
  p_show->SetSelection(i);
  recompute();
  return true;
}

bool DensityProfilePanel::setOrbitals(const std::vector<int>& mos)
{
  if (!p_ok) return false;
  for (int m : mos) if (m < 1 || m > (int)p_list->GetCount()) return false;
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
  char buf[320];
  snprintf(buf, sizeof buf, "mode %s\nquantity %d\nshow %d\nunit %s\ncurves %d\nmarks %d\nsumerror %.3e\n",
           p_mode->GetSelection() ? "atom" : "line", p_quantity->GetSelection(),
           p_show->GetSelection(), p_unit->GetSelection() ? "bohr" : "A",
           (int)p_shown.size(), (int)p_marks.size(), p_sumError);
  s += buf;
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
