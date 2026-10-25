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
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/utils.h>
#include <wx/wrapsizer.h>

#include "dsm/IPropCalculation.H"
#include "tdat/MoAoBasis.H"
#include "tdat/PropVecString.H"

#include "RadialDistPanel.H"
#include "XYPlotCanvas.H"

namespace {

const wxWindowID ID_CENTRE = wxNewId(), ID_SPIN = wxNewId(), ID_UNIT = wxNewId(),
                 ID_LIST = wxNewId(), ID_TOTAL = wxNewId(), ID_PER = wxNewId(),
                 ID_PNG = wxNewId(), ID_CSV = wxNewId();

const double BOHR_ANG = 0.52917721092;

//  Interactive accuracy: 0.0075 A steps and 302 directions resolve every
//  maximum of a first-row atom to well under 0.01 A.
const int GRID_POINTS = 801;
const int ANGULAR_POINTS = 302;

}  // namespace


IMPLEMENT_DYNAMIC_CLASS(RadialDistPanel, PropertyPanel)
wxFORCE_LINK_THIS_MODULE(RadialDistPanel)

RadialDistPanel::RadialDistPanel()
  : PropertyPanel(), p_ok(false), p_busy(false), p_centre(0), p_spin(0),
    p_unit(0), p_list(0), p_total(0), p_perElectron(0), p_text(0), p_plot(0)
{
}

RadialDistPanel::RadialDistPanel(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
  : RadialDistPanel()
{
  Create(calculation, parent, id, pos, size, style, name);
}

RadialDistPanel::~RadialDistPanel()
{
}

bool RadialDistPanel::Create(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
{
  if (!PropertyPanel::Create(calculation, parent, id, pos, size, style, name)) {
    wxFAIL_MSG( wxT("RadialDistPanel creation failed") );
    return false;
  }
  SetSizer(new wxBoxSizer(wxVERTICAL));
  return true;
}

bool RadialDistPanel::isRelevant(IPropCalculation *propCalc) const
{
  return propCalc != 0 && propCalc->getProperty("MO") != 0 &&
         propCalc->getProperty("ORBOCC") != 0;
}

void RadialDistPanel::initialize()
{
  build();
}

void RadialDistPanel::build()
{
  wxSizer *top = GetSizer();
  p_eval.reset(new DensityPlane::Evaluator);
  p_ok = p_eval->build(getCalculation());
  const int RIGHT = wxALIGN_CENTER_VERTICAL | wxRIGHT;

  p_plot = new XYPlotCanvas(this);
  p_plot->SetMinSize(wxSize(240, 160));
  p_text = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
                          wxSize(-1, 84), wxTE_MULTILINE | wxTE_READONLY);
  if (!p_ok) {
    top->Add(new wxStaticText(this, wxID_ANY,
             wxString::FromUTF8(p_eval->complaint.c_str())), 0, wxALL, 8);
    return;
  }

  //  Rows wrap instead of widening the panel: none is wider than the dock.
  wxWrapSizer *row1 = new wxWrapSizer(wxHORIZONTAL);
  p_centre = new wxChoice(this, ID_CENTRE);
  const std::vector<std::string> labels = p_eval->atomLabels();
  for (size_t i = 0; i < labels.size(); i++)
    p_centre->Append(wxString::FromUTF8(labels[i].c_str()));
  p_centre->SetSelection(RadialDist::defaultCentre(*p_eval));
  p_centre->SetToolTip("Distances are measured from this atom");
  wxBoxSizer *centrePair = new wxBoxSizer(wxHORIZONTAL);
  centrePair->Add(new wxStaticText(this, wxID_ANY, "Centre"), 0, RIGHT, 6);
  centrePair->Add(p_centre, 0, wxALIGN_CENTER_VERTICAL);
  row1->Add(centrePair, 0, RIGHT, 12);

  p_spin = new wxChoice(this, ID_SPIN);
  p_spin->Append("alpha");
  p_spin->Append("beta");
  p_spin->SetSelection(0);
  p_spin->Show(p_eval->unrestricted);
  row1->Add(p_spin, 0, RIGHT, 12);

  p_unit = new wxChoice(this, ID_UNIT);
  p_unit->Append(wxString::FromUTF8("\xC3\x85"));
  p_unit->Append("bohr");
  p_unit->SetSelection(0);
  wxBoxSizer *unitPair = new wxBoxSizer(wxHORIZONTAL);
  unitPair->Add(new wxStaticText(this, wxID_ANY, "Unit"), 0, RIGHT, 6);
  unitPair->Add(p_unit, 0, wxALIGN_CENTER_VERTICAL);
  row1->Add(unitPair, 0, RIGHT, 12);

  p_total = new wxCheckBox(this, ID_TOTAL, "Total density");
  p_total->SetToolTip(wxString::FromUTF8(
      "4\xCF\x80r\xC2\xB2\xCF\x81(r) of all the electrons; it integrates to their number"));
  row1->Add(p_total, 0, RIGHT, 8);
  p_perElectron = new wxCheckBox(this, ID_PER, "per electron");
  p_perElectron->SetToolTip("Divide the total by the number of electrons, "
                            "to draw it on the scale of one orbital");
  row1->Add(p_perElectron, 0, RIGHT, 12);
  row1->Add(new wxButton(this, ID_PNG, "Export .png..."), 0, wxLEFT | wxRIGHT, 4);
  row1->Add(new wxButton(this, ID_CSV, "Export .csv..."), 0);

  p_list = new wxCheckListBox(this, ID_LIST, wxDefaultPosition, wxSize(-1, 90));
  p_list->SetToolTip("Tick the orbitals to draw");

  top->Add(row1, 0, wxEXPAND | wxALL, 6);
  top->Add(p_list, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
  top->Add(p_plot, 1, wxEXPAND);
  top->Add(p_text, 0, wxEXPAND | wxALL, 6);

  Bind(wxEVT_CHOICE, &RadialDistPanel::onControl, this);
  Bind(wxEVT_CHECKBOX, &RadialDistPanel::onControl, this);
  Bind(wxEVT_CHECKLISTBOX, &RadialDistPanel::onControl, this);
  Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    wxFileDialog dlg(this, "Export plot", wxEmptyString, "radial-distribution.png",
                     "PNG files (*.png)|*.png", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() == wxID_OK && !savePng(dlg.GetPath(), 1100, 700))
      wxMessageBox("The file could not be written.", "Export plot", wxOK | wxICON_ERROR, this);
  }, ID_PNG);
  Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    wxFileDialog dlg(this, "Export curves", wxEmptyString, "radial-distribution.csv",
                     "CSV files (*.csv)|*.csv", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() == wxID_OK && !exportCsv(dlg.GetPath()))
      wxMessageBox("The file could not be written.", "Export curves", wxOK | wxICON_ERROR, this);
  }, ID_CSV);

  //  The symmetry label of each orbital, only if it lines up with them.
  auto symbols = [&](const char *key, std::vector<std::string>& out) {
    PropVecString *pv = (PropVecString*) getCalculation()->getProperty(key);
    out.clear();
    if (pv != 0) out = pv->values();
  };
  symbols("ORBSYM", p_sym);
  symbols("ORBSYMBETA", p_symBeta);
  if (p_sym.size() != (size_t)p_eval->orbitals(false)) p_sym.clear();
  if (p_symBeta.size() != (size_t)p_eval->orbitals(true)) p_symBeta.clear();

  p_shell = shellLabels(false);
  p_shellBeta = p_eval->unrestricted ? shellLabels(true) : std::vector<std::string>();
  fillOrbitals();
  chooseDefaults();
  recompute();
  Layout();
}

void RadialDistPanel::fillOrbitals()
{
  const bool beta = p_spin->GetSelection() == 1;
  const std::vector<double>& energy = beta ? p_eval->energyBeta : p_eval->energy;
  const std::vector<double>& occ = beta ? p_eval->occBeta : p_eval->occ;
  const std::vector<std::string>& sym = beta ? p_symBeta : p_sym;
  p_list->Clear();
  p_rows.clear();
  for (int m = 1; m <= p_eval->orbitals(beta); m++) {
    Row row;
    row.mo = m;
    row.label = "MO " + std::to_string(m);
    //  The code's own symmetry label, else the shell of an atom's occupied MO.
    std::string tag;
    if ((size_t)m <= sym.size()) tag = sym[m-1];
    const std::vector<std::string>& shell = beta ? p_shellBeta : p_shell;
    if (tag.empty() && (size_t)m <= shell.size()) tag = shell[m-1];
    if (!tag.empty()) row.label += " (" + tag + ")";
    wxString item = wxString::Format("%3d", m);
    if (!tag.empty()) item += "  " + wxString::FromUTF8(tag.c_str());
    if ((size_t)m <= energy.size()) item += wxString::Format("   E %.4f Ha", energy[m-1]);
    if ((size_t)m <= occ.size()) item += wxString::Format("   occ %.1f", occ[m-1]);
    p_list->Append(item);
    p_rows.push_back(row);
  }
}

//  An atom shows its occupied orbitals (the shells); a molecule the HOMO.
void RadialDistPanel::chooseDefaults()
{
  const bool beta = p_spin->GetSelection() == 1;
  const std::vector<double>& occ = beta ? p_eval->occBeta : p_eval->occ;
  if (p_eval->symbols.size() == 1) {
    for (size_t m = 0; m < occ.size() && m < p_rows.size(); m++)
      if (occ[m] > 0.0) p_list->Check((unsigned)m, true);
  } else {
    int homo = 0;
    for (size_t m = 0; m < occ.size(); m++) if (occ[m] > 0.5) homo = (int)m + 1;
    if (homo >= 1 && homo <= (int)p_rows.size()) p_list->Check(homo - 1, true);
  }
}

//  An atom's occupied orbitals named by shell: the dominant angular momentum
//  of each, counted up in energy order within it (1s, 2s, 2p, ...).
std::vector<std::string> RadialDistPanel::shellLabels(bool beta) const
{
  std::vector<std::string> out;
  if (p_eval->symbols.size() != 1) return out;
  std::vector< std::vector<double> > rows;
  if (!MoAoBasis::coefficients(getCalculation(), p_eval->symbols, beta, rows) ||
      rows.empty())
    return out;
  MoAoBasis basis;
  if (!basis.build(getCalculation(), p_eval->symbols, p_eval->coordsAng,
                   (int)rows[0].size()))
    return out;
  const std::vector<double>& occ = beta ? p_eval->occBeta : p_eval->occ;
  static const char *letters = "spdfgh";
  const std::vector<double>& energy = beta ? p_eval->energyBeta : p_eval->energy;
  int seen[6] = { 0, 0, 0, 0, 0, 0 };
  double last[6] = { 1e9, 1e9, 1e9, 1e9, 1e9, 1e9 };   // energy of the l's latest shell
  out.assign(rows.size(), std::string());
  for (size_t m = 0; m < rows.size() && m < occ.size(); m++) {
    MoAoBasis::Composition comp;
    if (occ[m] <= 0.0 || !basis.analyse(rows[m], MoAoBasis::MULLIKEN, comp)) continue;
    const MoAoBasis::Share *best = 0;
    for (const MoAoBasis::Share& t : comp.types)
      if (t.l >= 0 && t.l < 6 && (!best || t.share > best->share)) best = &t;
    if (best && best->share >= 0.9) {
      //  The members of a degenerate set (2px, 2py, 2pz) are one shell.
      const int l = best->l;
      const double e = m < energy.size() ? energy[m] : 0.0;
      if (seen[l] == 0 || std::fabs(e - last[l]) > 1e-4) seen[l]++;
      last[l] = e;
      out[m] = std::to_string(l + seen[l]) + letters[l];
    }
  }
  return out;
}

void RadialDistPanel::centreOf(double c[3]) const
{
  const int a = std::max(0, p_centre->GetSelection());
  for (int d = 0; d < 3; d++) c[d] = p_eval->coordsAng[3*a + d];
}

std::vector<RadialDist::Spec> RadialDistPanel::wanted() const
{
  std::vector<RadialDist::Spec> specs;
  const bool beta = p_spin->GetSelection() == 1;
  for (size_t i = 0; i < p_rows.size(); i++) {
    if (!p_list->IsChecked((unsigned)i)) continue;
    RadialDist::Spec s;
    s.total = false;
    s.mo = p_rows[i].mo;
    s.beta = beta;
    s.label = p_rows[i].label + (beta ? " beta" : "");
    specs.push_back(s);
  }
  if (p_total->GetValue()) {
    RadialDist::Spec s;
    s.label = "Total density";
    specs.push_back(s);
  }
  return specs;
}

std::string RadialDistPanel::key(const RadialDist::Spec& s) const
{
  return s.total ? "total" : std::to_string(s.mo) + (s.beta ? "b" : "a");
}

void RadialDistPanel::recompute()
{
  if (!p_ok || p_busy) return;
  p_busy = true;
  const bool bohr = p_unit->GetSelection() == 1;
  RadialDist::Options opt;
  opt.bohr = bohr;
  opt.rMax = bohr ? 6.0 / BOHR_ANG : 6.0;
  opt.points = GRID_POINTS;
  opt.lebedev = ANGULAR_POINTS;
  p_r = RadialDist::grid(opt);

  const std::vector<RadialDist::Spec> specs = wanted();
  std::vector<RadialDist::Spec> missing;
  for (const RadialDist::Spec& s : specs)
    if (!p_cache.count(key(s))) missing.push_back(s);
  if (!missing.empty()) {
    wxBusyCursor busy;
    double c[3];
    centreOf(c);
    std::vector<RadialDist::Curve> out;
    RadialDist::compute(*p_eval, c, opt, missing, out);
    for (size_t i = 0; i < missing.size(); i++) p_cache[key(missing[i])] = out[i];
  }

  const bool bohrAxis = bohr;
  const double electrons = [&]() {
    double n = 0;
    for (double o : p_eval->occ) n += o;
    for (double o : p_eval->occBeta) n += o;
    return n;
  }();
  const bool perElectron = p_perElectron->GetValue() && electrons > 0;

  p_shown.clear();
  p_plot->clearCurves();
  p_plot->setXAxisLabel(bohrAxis ? "r (bohr)" : wxString::FromUTF8("r (\xC3\x85)"));
  p_plot->setYAxisLabel(bohrAxis ? wxString::FromUTF8("radial distribution (1/bohr)")
                                 : wxString::FromUTF8("radial distribution (1/\xC3\x85)"));
  p_plot->setShowKey(true);
  wxString text;
  int series = 0;
  for (const RadialDist::Spec& s : specs) {
    RadialDist::Curve c = p_cache[key(s)];
    c.spec = s;
    if (s.total && perElectron) {
      for (double& v : c.p) v /= electrons;
      for (double& v : c.maxP) v /= electrons;
      c.integral /= electrons;
      c.spec.label = "Total density / " + std::to_string((int)std::lround(electrons));
    }
    std::vector<double> plotR, plotP;   // only as far out as the curve is visible
    {
      double top = 0;
      for (double v : c.p) top = std::max(top, v);
      size_t last = 1;
      for (size_t i = 0; i < c.p.size(); i++) if (c.p[i] > 0.002 * top) last = i;
      last = std::min(c.p.size() - 1, last + last / 8 + 2);
      plotR.assign(p_r.begin(), p_r.begin() + last + 1);
      plotP.assign(c.p.begin(), c.p.begin() + last + 1);
    }
    p_plot->addCurve(plotR, plotP, wxString::FromUTF8(c.spec.label.c_str()),
                     series, XYPlotCanvas::LINE);
    //  Maxima: an unnamed marker series in the curve's colour.
    if (!c.maxR.empty()) {
      const int m = p_plot->addCurve(c.maxR, c.maxP, wxString(), series,
                                     XYPlotCanvas::MARKERS);
      p_plot->setCurveColour(m, p_plot->seriesColour(series));
    }
    text += wxString::FromUTF8(c.spec.label.c_str()) + ": ";
    for (size_t k = 0; k < c.maxR.size(); k++)
      text += wxString::Format("%s%.3f", k ? ", " : "max at ", c.maxR[k]);
    if (c.maxR.empty()) text += "no maximum";
    text += wxString::Format(" %s;  integral %.3f\n",
                             bohrAxis ? wxString("bohr") : wxString::FromUTF8("\xC3\x85"),
                             c.integral);
    p_shown.push_back(c);
    series++;
  }
  p_text->SetValue(text);
  p_plot->Refresh();
  p_busy = false;
}

void RadialDistPanel::onControl(wxCommandEvent& event)
{
  if (!p_ok) return;
  const wxWindow *w = dynamic_cast<wxWindow*>(event.GetEventObject());
  const wxWindowID id = w ? w->GetId() : 0;
  if (id == ID_SPIN) {
    p_cache.clear();
    fillOrbitals();
    chooseDefaults();
  } else if (id == ID_CENTRE || id == ID_UNIT) {
    p_cache.clear();
  } else if (id == ID_TOTAL) {
    p_perElectron->Enable(p_total->GetValue());
  }
  recompute();
}

bool RadialDistPanel::setCentre(int atom)
{
  if (!p_ok || atom < 1 || atom > (int)p_centre->GetCount()) return false;
  p_centre->SetSelection(atom - 1);
  p_cache.clear();
  recompute();
  return true;
}

bool RadialDistPanel::setOrbitals(const std::vector<int>& mos)
{
  if (!p_ok) return false;
  for (int m : mos) if (m < 1 || m > (int)p_rows.size()) return false;
  for (unsigned i = 0; i < p_list->GetCount(); i++) p_list->Check(i, false);
  for (int m : mos) p_list->Check((unsigned)m - 1, true);
  recompute();
  return true;
}

bool RadialDistPanel::setTotal(bool on, bool perElectron)
{
  if (!p_ok) return false;
  p_total->SetValue(on);
  p_perElectron->SetValue(perElectron);
  p_perElectron->Enable(on);
  recompute();
  return true;
}

bool RadialDistPanel::setBeta(bool beta)
{
  if (!p_ok || (beta && !p_eval->unrestricted)) return false;
  p_spin->SetSelection(beta ? 1 : 0);
  p_cache.clear();
  fillOrbitals();
  chooseDefaults();
  recompute();
  return true;
}

bool RadialDistPanel::setBohr(bool bohr)
{
  if (!p_ok) return false;
  p_unit->SetSelection(bohr ? 1 : 0);
  p_cache.clear();
  recompute();
  return true;
}

bool RadialDistPanel::exportCsv(const wxString& path)
{
  if (!p_ok || p_shown.empty()) return false;
  FILE *f = fopen((const char*)path.utf8_str(), "w");
  if (!f) return false;
  const std::string text = RadialDist::csv(p_r, p_shown, p_unit->GetSelection() == 1);
  fputs(text.c_str(), f);
  return fclose(f) == 0;
}

bool RadialDistPanel::savePng(const wxString& path, int width, int height)
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

std::string RadialDistPanel::state() const
{
  if (!p_ok) return "ok 0\n";
  std::string s = "ok 1\n";
  char buf[256];
  snprintf(buf, sizeof buf, "centre %d\nunit %s\nspin %s\ncurves %d\n",
           p_centre->GetSelection() + 1, p_unit->GetSelection() ? "bohr" : "A",
           p_spin->GetSelection() ? "beta" : "alpha", (int)p_shown.size());
  s += buf;
  for (const RadialDist::Curve& c : p_shown) {
    snprintf(buf, sizeof buf, "curve '%s' integral %.6f maxima %d", c.spec.label.c_str(),
             c.integral, (int)c.maxR.size());
    s += buf;
    for (double r : c.maxR) { snprintf(buf, sizeof buf, " %.4f", r); s += buf; }
    s += "\n";
  }
  return s;
}
