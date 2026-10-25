#include <cmath>
#include <cstdio>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choicdlg.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include "tdat/PropVecString.H"
#include "dsm/EDSIFactory.H"
#include "dsm/IPropCalculation.H"
#include "dsm/Resource.H"
#include "XYPlotCanvas.H"

#include "MdAnalysisPanels.H"

namespace {
const wxWindowID ID_EXPORT = wxNewId();
const wxWindowID ID_COMPARE = wxNewId();
const wxWindowID ID_REMOVE = wxNewId();
const wxWindowID ID_FIT = wxNewId();
const wxWindowID ID_LOG = wxNewId();

wxString utf8(const std::string& s) { return wxString::FromUTF8(s.c_str()); }
}

IMPLEMENT_ABSTRACT_CLASS(MdAnalysisPanel, PropertyPanel)

MdAnalysisPanel::MdAnalysisPanel()
  : PropertyPanel(), p_plot(NULL), p_text(NULL), p_curveTotal(0), p_remove(NULL)
{
}

MdAnalysisPanel::~MdAnalysisPanel()
{
}

std::string MdAnalysisPanel::calcName() const
{
  IPropCalculation *calc = const_cast<MdAnalysisPanel*>(this)->getCalculation();
  return calc ? calc->getURL().getFilePathTail() : std::string();
}

std::vector<std::string> MdAnalysisPanel::recordsOf(IPropCalculation *calc) const
{
  PropVecString *v = calc ? dynamic_cast<PropVecString*>(calc->getProperty(propertyKey())) : 0;
  return v ? v->values() : std::vector<std::string>();
}

bool MdAnalysisPanel::build(IPropCalculation *calculation, wxWindow *parent,
        wxWindowID id, const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
{
  if (!PropertyPanel::Create(calculation, parent, id, pos, size, style, name)) {
    wxFAIL_MSG( wxT("MdAnalysisPanel creation failed") );
    return false;
  }
  SetSizer(new wxBoxSizer(wxVERTICAL));
  p_text = new wxStaticText(this, wxID_ANY, wxEmptyString);
  GetSizer()->Add(p_text, 0, wxEXPAND | wxALL, 4);

  // Two rows so the buttons fit a docked panel at 1366x768.
  wxBoxSizer *bar = new wxBoxSizer(wxHORIZONTAL);
  wxButton *compare = new wxButton(this, ID_COMPARE, "Compare...");
  compare->SetToolTip("Draw another calculation of this folder over this one");
  p_remove = new wxButton(this, ID_REMOVE, "Remove");
  p_remove->SetToolTip("Remove the compared calculation");
  p_remove->Enable(false);
  bar->Add(compare, 0, wxRIGHT, 4);
  bar->Add(p_remove, 0, wxRIGHT, 4);
  bar->Add(new wxButton(this, ID_EXPORT, "Export .csv..."), 0);
  GetSizer()->Add(bar, 0, wxEXPAND | wxLEFT | wxRIGHT, 4);
  wxBoxSizer *extra = new wxBoxSizer(wxHORIZONTAL);
  addControls(extra);
  if (!extra->IsEmpty()) GetSizer()->Add(extra, 0, wxEXPAND | wxALL, 4);
  else delete extra;

  p_plot = new XYPlotCanvas(this);
  p_plot->SetMinSize(wxSize(300, 200));
  p_plot->setShowKey(true);
  GetSizer()->Add(p_plot, 1, wxEXPAND);

  loadRecords(recordsOf(calculation), std::vector<std::string>());
  redraw();
  Bind(wxEVT_BUTTON, &MdAnalysisPanel::onCompare, this, ID_COMPARE);
  Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { compareWith(std::string()); }, ID_REMOVE);
  Bind(wxEVT_BUTTON, &MdAnalysisPanel::onExport, this, ID_EXPORT);
  return true;
}

void MdAnalysisPanel::refill()
{
  loadRecords(recordsOf(getCalculation()), p_otherRecords);
  redraw();
}

std::vector<IPropCalculation*> MdAnalysisPanel::comparable() const
{
  std::vector<IPropCalculation*> out;
  IPropCalculation *calc = const_cast<MdAnalysisPanel*>(this)->getCalculation();
  if (!calc) return out;
  Resource *folder = EDSIFactory::getResource(calc->getURL().getParent());
  std::vector<Resource*> *kids = folder ? folder->getChildren() : 0;
  if (!kids) return out;
  const std::string own = calc->getURL().getFilePathTail();
  for (size_t i = 0; i < kids->size(); i++) {
    IPropCalculation *other = dynamic_cast<IPropCalculation*>((*kids)[i]);
    if (!other || (*kids)[i]->getName() == own) continue;
    if (other->getProperty(propertyKey()) != 0) out.push_back(other);
  }
  return out;
}

bool MdAnalysisPanel::compareWith(const std::string& name)
{
  if (name.empty()) {
    p_compareName.clear();
    p_otherRecords.clear();
    if (p_remove) p_remove->Enable(false);
    refill();
    return true;
  }
  const std::vector<IPropCalculation*> found = comparable();
  for (size_t i = 0; i < found.size(); i++) {
    if (found[i]->getURL().getFilePathTail() != name) continue;
    p_compareName = name;
    p_otherRecords = recordsOf(found[i]);
    if (p_remove) p_remove->Enable(true);
    refill();
    return true;
  }
  return false;
}

void MdAnalysisPanel::onCompare(wxCommandEvent&)
{
  const std::vector<IPropCalculation*> found = comparable();
  if (found.empty()) {
    wxMessageBox("No other calculation in this folder has this analysis.",
                 "Compare", wxOK | wxICON_INFORMATION, this);
    return;
  }
  wxArrayString names;
  for (size_t i = 0; i < found.size(); i++)
    names.Add(utf8(found[i]->getURL().getFilePathTail()));
  const int at = wxGetSingleChoiceIndex(
      "Draw this calculation as well, in the same plot:", "Compare", names, this);
  if (at >= 0) compareWith(found[at]->getURL().getFilePathTail());
}

bool MdAnalysisPanel::exportCsv(const wxString& path)
{
  const std::string text = csvText();
  FILE *out = fopen(path.utf8_str(), "wb");
  if (out == 0) return false;
  const bool ok = fwrite(text.data(), 1, text.size(), out) == text.size();
  return fclose(out) == 0 && ok;
}

void MdAnalysisPanel::onExport(wxCommandEvent&)
{
  wxFileDialog dlg(this, "Export analysis", wxEmptyString,
                   utf8(calcName() + "-" + exportName() + ".csv"),
                   "CSV files (*.csv)|*.csv", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  if (dlg.ShowModal() != wxID_OK) return;
  if (!exportCsv(dlg.GetPath()))
    wxMessageBox("The file could not be written.", "Export analysis",
                 wxOK | wxICON_ERROR, this);
}

wxString MdAnalysisPanel::summary() const
{
  return p_text ? p_text->GetLabel() : wxString();
}

// ---------------------------------------------------------------- speeds

IMPLEMENT_DYNAMIC_CLASS(SpeedPanel, MdAnalysisPanel)

SpeedPanel::SpeedPanel() : MdAnalysisPanel() {}

SpeedPanel::SpeedPanel(IPropCalculation *calculation, wxWindow *parent,
        wxWindowID id, const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
  : MdAnalysisPanel()
{
  Create(calculation, parent, id, pos, size, style, name);
}

bool SpeedPanel::Create(IPropCalculation *calculation, wxWindow *parent,
        wxWindowID id, const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
{
  if (!build(calculation, parent, id, pos, size, style, name)) return false;
  p_plot->setXAxisLabel("Speed (m/s)");
  p_plot->setYAxisLabel("Probability density (10⁻³ s/m)");
  return true;
}

void SpeedPanel::loadRecords(const std::vector<std::string>& own,
                             const std::vector<std::string>& other)
{
  if (!decodeSpeed(own, p_own)) p_own.clear();
  if (!decodeSpeed(other, p_other)) p_other.clear();
}

void SpeedPanel::redraw()
{
  p_plot->clearCurves();
  p_plot->clearVLines();
  p_curveTotal = 0;
  wxString text;
  int series = 0;
  for (int pass = 0; pass < 2; pass++) {
    const std::vector<SpeedCurve>& set = pass ? p_other : p_own;
    const std::string run = pass ? p_compareName : calcName();
    for (const SpeedCurve& c : set) {
      const size_t n = c.density.size();
      std::vector<double> v(n), meas(n), mb(n);
      for (size_t b = 0; b < n; b++) {
        v[b] = (b + 0.5) * c.vmax / n;
        meas[b] = 1000 * c.density[b];
        mb[b] = 1000 * mbDensity(v[b], c.mass, c.refT);
      }
      const wxString who = utf8(c.label) + " (" + utf8(run) + ")";
      const int a = p_plot->addCurve(v, meas, who + " simulation", series, XYPlotCanvas::LINE);
      p_plot->setCurveColour(a, p_plot->seriesColour(series));
      const int b = p_plot->addCurve(v, mb, utf8(c.label) +
          wxString::Format(" Maxwell-Boltzmann, %.0f K", c.refT), series, XYPlotCanvas::LINE);
      p_plot->setCurveColour(b, p_plot->seriesColour(series));
      p_plot->setCurveStyle(b, true, 1);
      p_plot->addVLine(mbMostProbable(c.mass, c.refT), "v_p " + utf8(c.label));
      p_plot->addVLine(mbMean(c.mass, c.refT), "mean " + utf8(c.label));
      p_plot->addVLine(mbRms(c.mass, c.refT), "rms " + utf8(c.label));
      series++;
      p_curveTotal += 2;
      text += wxString::Format("%s, %s: mass %.4g u, %.0f K. Mean speed %.0f m/s (Maxwell-Boltzmann %.0f), "
                               "rms %.0f m/s (%.0f), most probable %.0f m/s.\n",
                               utf8(c.label), utf8(run), c.mass, c.refT, c.mean,
                               mbMean(c.mass, c.refT), c.rms, mbRms(c.mass, c.refT),
                               mbMostProbable(c.mass, c.refT));
    }
  }
  if (text.IsEmpty()) text = "No velocities were saved in this run (nstvout), so there is no speed distribution.";
  p_text->SetLabel(text.Trim());
  p_text->Wrap(GetSize().x > 100 ? GetSize().x - 16 : 360);
  Layout();
}

std::string SpeedPanel::csvText()
{
  std::vector<SpeedCurve> all = p_own;
  for (SpeedCurve c : p_other) { c.label += "_" + p_compareName; all.push_back(c); }
  return speedCsv(all);
}

// ---------------------------------------------------------------- MSD

IMPLEMENT_DYNAMIC_CLASS(MsdPanel, MdAnalysisPanel)

MsdPanel::MsdPanel()
  : MdAnalysisPanel(), p_from(0), p_to(0), p_fromBox(NULL), p_toBox(NULL), p_log(NULL)
{
}

MsdPanel::MsdPanel(IPropCalculation *calculation, wxWindow *parent,
        wxWindowID id, const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
  : MdAnalysisPanel(), p_from(0), p_to(0), p_fromBox(NULL), p_toBox(NULL), p_log(NULL)
{
  Create(calculation, parent, id, pos, size, style, name);
}

bool MsdPanel::Create(IPropCalculation *calculation, wxWindow *parent,
        wxWindowID id, const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
{
  if (!build(calculation, parent, id, pos, size, style, name)) return false;
  p_plot->setXAxisLabel("Lag time (ps)");
  p_plot->setYAxisLabel("Mean-square displacement (nm²)");
  Bind(wxEVT_BUTTON, &MsdPanel::onFit, this, ID_FIT);
  Bind(wxEVT_CHECKBOX, &MsdPanel::onLogLog, this, ID_LOG);
  return true;
}

void MsdPanel::addControls(wxSizer *bar)
{
  p_fromBox = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(60, -1),
                             wxTE_PROCESS_ENTER);
  p_toBox = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(60, -1),
                           wxTE_PROCESS_ENTER);
  p_log = new wxCheckBox(this, ID_LOG, "Log-log");
  p_log->SetToolTip("Both axes logarithmic: slope 2 is free flight, slope 1 diffusion");
  bar->Add(new wxStaticText(this, wxID_ANY, "Fit from"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
  bar->Add(p_fromBox, 0, wxRIGHT, 4);
  bar->Add(new wxStaticText(this, wxID_ANY, "to"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
  bar->Add(p_toBox, 0, wxRIGHT, 4);
  bar->Add(new wxStaticText(this, wxID_ANY, "ps"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
  bar->Add(new wxButton(this, ID_FIT, "Fit"), 0, wxRIGHT, 8);
  bar->Add(p_log, 0, wxALIGN_CENTER_VERTICAL);
}

void MsdPanel::loadRecords(const std::vector<std::string>& own,
                           const std::vector<std::string>& other)
{
  if (!decodeMsd(own, p_own)) p_own.clear();
  if (!decodeMsd(other, p_other)) p_other.clear();
}

void MsdPanel::setFit(double from, double to)
{
  p_from = from;
  p_to = to;
  redraw();
}

void MsdPanel::setLogLog(bool on)
{
  if (p_log) p_log->SetValue(on);
  p_plot->setXScale(on ? XYPlotCanvas::X_LOG : XYPlotCanvas::X_LINEAR);
  p_plot->setYLog(on);
}

double MsdPanel::diffusion(int i) const
{
  if (i < 0 || i >= (int)p_own.size()) return -1;
  const DiffusionFit f = fitDiffusion(p_own[i].time, p_own[i].msd, p_from, p_to);
  return f.ok ? toE5cm2s(f.D) : -1;
}

void MsdPanel::redraw()
{
  p_plot->clearCurves();
  p_plot->clearVLines();
  p_curveTotal = 0;
  wxString text;
  int series = 0;
  bool first = true;
  for (int pass = 0; pass < 2; pass++) {
    const std::vector<MsdCurve>& set = pass ? p_other : p_own;
    const std::string run = pass ? p_compareName : calcName();
    for (const MsdCurve& c : set) {
      const wxString who = utf8(c.label) + " (" + utf8(run) + ")";
      const int a = p_plot->addCurve(c.time, c.msd, who, series, XYPlotCanvas::LINE);
      p_plot->setCurveColour(a, p_plot->seriesColour(series));
      p_curveTotal++;
      const DiffusionFit f = fitDiffusion(c.time, c.msd, p_from, p_to);
      if (f.ok) {
        const std::vector<double> t = {f.from, f.to};
        const std::vector<double> y = {f.intercept + f.slope * f.from, f.intercept + f.slope * f.to};
        const int l = p_plot->addCurve(t, y, "fit " + utf8(c.label), series, XYPlotCanvas::LINE);
        p_plot->setCurveColour(l, p_plot->seriesColour(series));
        p_plot->setCurveStyle(l, true, 2);
        p_curveTotal++;
        if (first) {
          p_plot->addVLine(f.from, "fit from");
          p_plot->addVLine(f.to, "fit to");
          if (p_fromBox && !p_fromBox->HasFocus()) p_fromBox->ChangeValue(wxString::Format("%.4g", f.from));
          if (p_toBox && !p_toBox->HasFocus()) p_toBox->ChangeValue(wxString::Format("%.4g", f.to));
          first = false;
        }
        text += wxString::Format("%s, %s (%d molecules): D = %.4g × 10⁻⁵ cm²/s "
                                 "(%.4g × 10⁻⁹ m²/s), fit %.4g–%.4g ps, %d points.\n",
                                 utf8(c.label), utf8(run), c.molecules, toE5cm2s(f.D),
                                 toE5cm2s(f.D), f.from, f.to, f.points);
      }
      series++;
    }
  }
  if (text.IsEmpty()) text = "No displacement analysis was made for this run.";
  p_summary = text.Trim();
  p_text->SetLabel(p_summary);
  p_text->Wrap(GetSize().x > 100 ? GetSize().x - 16 : 360);
  Layout();
}

void MsdPanel::onFit(wxCommandEvent&)
{
  double a = 0, b = 0;
  if (p_fromBox && p_toBox && p_fromBox->GetValue().ToDouble(&a) &&
      p_toBox->GetValue().ToDouble(&b) && a < b)
    setFit(a, b);
}

void MsdPanel::onLogLog(wxCommandEvent&)
{
  setLogLog(p_log->GetValue());
}

std::string MsdPanel::csvText()
{
  std::vector<MsdCurve> all = p_own;
  for (MsdCurve c : p_other) { c.label += "_" + p_compareName; all.push_back(c); }
  return msdCsv(all, p_from, p_to);
}
