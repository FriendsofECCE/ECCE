#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/image.h>
#include <wx/dcmemory.h>
#include <wx/listctrl.h>
#include <wx/notebook.h>
#include <wx/textctrl.h>
#include <wx/utils.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include "tdat/PropVector.H"
#include "dsm/IPropCalculation.H"
#include "dsm/GromacsMDModel.H"
#include "dsm/MdTask.H"
#include "dsm/UmbrellaWindows.H"
#include "tdat/DynamicsModel.H"
#include "util/Ecce.H"
#include "util/Filesystem.H"
#include "XYPlotCanvas.H"

#include "PullPanel.H"

namespace {
const wxWindowID ID_EXPORT = wxNewId();
const wxWindowID ID_FIND = wxNewId();
const wxWindowID ID_CREATE = wxNewId();
const wxWindowID ID_WHAM = wxNewId();
const wxWindowID ID_UEXPORT = wxNewId();

wxString U8(const std::string& s) { return wxString::FromUTF8(s.c_str()); }
double numOf(const MiniJson& v) { return atof(v.str().c_str()); }

const std::vector<double> *valuesOf(IPropCalculation *calc, const char *key)
{
  PropVector *v = calc ? dynamic_cast<PropVector*>(calc->getProperty(key)) : 0;
  return v ? &v->values() : 0;
}
}

IMPLEMENT_DYNAMIC_CLASS(PullPanel, PropertyPanel)

PullPanel::PullPanel()
  : PropertyPanel(), p_force(NULL), p_dist(NULL), p_text(NULL),
    p_book(NULL), p_spacing(NULL), p_from(NULL), p_to(NULL), p_eq(NULL),
    p_prod(NULL), p_info(NULL), p_jacobian(NULL), p_windows(NULL),
    p_pmf(NULL), p_hist(NULL), p_haveWham(false)
{
}

PullPanel::PullPanel(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
  : PropertyPanel(), p_force(NULL), p_dist(NULL), p_text(NULL),
    p_book(NULL), p_spacing(NULL), p_from(NULL), p_to(NULL), p_eq(NULL),
    p_prod(NULL), p_info(NULL), p_jacobian(NULL), p_windows(NULL),
    p_pmf(NULL), p_hist(NULL), p_haveWham(false)
{
  Create(calculation, parent, id, pos, size, style, name);
}

PullPanel::~PullPanel()
{
}

bool PullPanel::Create(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
{
  if (!PropertyPanel::Create(calculation, parent, id, pos, size, style, name)) {
    wxFAIL_MSG( wxT("PullPanel creation failed") );
    return false;
  }
  SetSizer(new wxBoxSizer(wxVERTICAL));

  const std::vector<double> *t = valuesOf(calculation, "PULLTIME");
  const std::vector<double> *x = valuesOf(calculation, "PULLDIST");
  const std::vector<double> *f = valuesOf(calculation, "PULLFORCE");
  if (t && x && f) {
    p_series.time = *t;
    p_series.dist = *x;
    p_series.force = *f;
  }

  p_book = new wxNotebook(this, wxID_ANY);
  GetSizer()->Add(p_book, 1, wxEXPAND);
  wxPanel *page = new wxPanel(p_book);
  page->SetSizer(new wxBoxSizer(wxVERTICAL));
  p_book->AddPage(page, "Pull run");

  wxBoxSizer *bar = new wxBoxSizer(wxHORIZONTAL);
  p_text = new wxStaticText(page, wxID_ANY, wxEmptyString);
  bar->Add(p_text, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
  bar->Add(new wxButton(page, ID_EXPORT, "Export .csv..."), 0);
  page->GetSizer()->Add(bar, 0, wxEXPAND | wxALL, 4);

  p_force = new XYPlotCanvas(page);
  p_force->SetMinSize(wxSize(300, 150));
  p_force->setXAxisLabel("Time (ps)");
  p_force->setYAxisLabel("Pull force (kJ/mol/nm)");
  page->GetSizer()->Add(p_force, 1, wxEXPAND);
  p_dist = new XYPlotCanvas(page);
  p_dist->SetMinSize(wxSize(300, 150));
  p_dist->setXAxisLabel("Time (ps)");
  p_dist->setYAxisLabel("COM distance (nm)");
  page->GetSizer()->Add(p_dist, 1, wxEXPAND);

  if (p_series.size() > 1) {
    p_force->addCurve(p_series.time, p_series.force);
    p_dist->addCurve(p_series.time, p_series.dist);
    const PullSummary s = pullSummary(p_series);
    p_text->SetLabel(wxString::Format(
        "%d points, distance rising at %.4g nm/ps\nmean force %.4g, peak %.4g "
        "kJ/mol/nm at %.4g ps", (int)s.points, s.rate, s.meanForce,
        s.peakForce, s.peakTime));
  } else {
    p_text->SetLabel("No pull output in this run.");
  }

  Bind(wxEVT_BUTTON, &PullPanel::OnExport, this, ID_EXPORT);

  //  Umbrella sampling: windows from this run, then the PMF from their runs.
  wxPanel *u = new wxPanel(p_book);
  wxBoxSizer *us = new wxBoxSizer(wxVERTICAL);
  u->SetSizer(us);
  p_book->AddPage(u, "Umbrella sampling");
  wxBoxSizer *row = new wxBoxSizer(wxHORIZONTAL);
  auto field = [&](const char *label, const char *value, wxTextCtrl **out) {
    row->Add(new wxStaticText(u, wxID_ANY, label), 0,
             wxALIGN_CENTER_VERTICAL | wxLEFT, 6);
    *out = new wxTextCtrl(u, wxID_ANY, value, wxDefaultPosition, wxSize(70, -1));
    row->Add(*out, 0, wxLEFT, 3);
  };
  field("Spacing (nm)", "0.2", &p_spacing);
  field("From (nm)", "", &p_from);
  field("To (nm)", "", &p_to);
  field("Equilibration steps", "50000", &p_eq);
  field("Production steps", "250000", &p_prod);
  us->Add(row, 0, wxEXPAND | wxALL, 4);
  wxBoxSizer *b1 = new wxBoxSizer(wxHORIZONTAL);
  b1->Add(new wxButton(u, ID_FIND, "Find windows"), 0, wxRIGHT, 6);
  b1->Add(new wxButton(u, ID_CREATE, "Create window calculations"), 0, wxRIGHT, 12);
  p_jacobian = new wxCheckBox(u, wxID_ANY, "Remove the 2RT ln r volume term");
  b1->Add(p_jacobian, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
  b1->Add(new wxButton(u, ID_WHAM, "Run WHAM"), 0, wxRIGHT, 6);
  b1->Add(new wxButton(u, ID_UEXPORT, "Export .csv..."), 0);
  us->Add(b1, 0, wxEXPAND | wxALL, 4);
  p_windows = new wxListCtrl(u, wxID_ANY, wxDefaultPosition, wxSize(-1, 110),
                             wxLC_REPORT | wxLC_SINGLE_SEL);
  for (const char *c : { "Window", "Frame", "Time (ps)", "Distance (nm)" })
    p_windows->AppendColumn(c, wxLIST_FORMAT_LEFT, 100);
  us->Add(p_windows, 0, wxEXPAND | wxALL, 4);
  p_pmf = new XYPlotCanvas(u);
  p_pmf->SetMinSize(wxSize(300, 150));
  p_pmf->setXAxisLabel("COM distance (nm)");
  p_pmf->setYAxisLabel("PMF (kJ/mol)");
  us->Add(p_pmf, 1, wxEXPAND);
  p_hist = new XYPlotCanvas(u);
  p_hist->SetMinSize(wxSize(300, 120));
  p_hist->setXAxisLabel("COM distance (nm)");
  p_hist->setYAxisLabel("Window histograms (counts)");
  p_hist->setShowKey(false);
  us->Add(p_hist, 1, wxEXPAND);
  //  Messages have a panel of their own, not a line among the controls.
  p_info = new wxTextCtrl(u, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 70),
                          wxTE_MULTILINE | wxTE_READONLY);
  us->Add(p_info, 0, wxEXPAND | wxALL, 4);
  umbrellaSay("Find windows along this pull run, create a calculation for "
              "each, and run WHAM when the production runs are Complete.");
  Bind(wxEVT_BUTTON, &PullPanel::OnFind, this, ID_FIND);
  Bind(wxEVT_BUTTON, &PullPanel::OnCreate, this, ID_CREATE);
  Bind(wxEVT_BUTTON, &PullPanel::OnWham, this, ID_WHAM);
  Bind(wxEVT_BUTTON, &PullPanel::OnUmbrellaExport, this, ID_UEXPORT);
  return true;
}

void PullPanel::umbrellaSay(const wxString& text)
{
  if (p_info) p_info->SetValue(text);
}

static MdTask *pullTask(IPropCalculation *calc)
{
  MdTask *t = dynamic_cast<MdTask*>(calc);
  return t && t->getContentType() == ResourceDescriptor::CT_MDDYNAMICS ? t : 0;
}

bool PullPanel::umbrellaFind(double spacing, const wxString& from, const wxString& to)
{
  MdTask *t = pullTask(getCalculation());
  if (!t) { umbrellaSay("This is not a GROMACS Dynamics task."); return false; }
  wxBusyCursor wait;
  MiniJson j;
  std::string why;
  if (!UmbrellaWindows::find(t, spacing, from.utf8_string(), to.utf8_string(),
                             "", 0, &j, &why)) {
    umbrellaSay(U8(why));
    return false;
  }
  p_windows->DeleteAllItems();
  p_dist->clearVLines();
  int n = 0;
  for (const MiniJson& w : j.get("windows").items()) {
    const long row = p_windows->InsertItem(n++, wxString::Format("%d", n));
    p_windows->SetItem(row, 1, U8(w.get("frame").str()));
    p_windows->SetItem(row, 2, U8(w.get("time").str()));
    p_windows->SetItem(row, 3, U8(w.get("distance").str()));
    p_dist->addVLine(numOf(w.get("time")), wxString::Format("%d", n));
  }
  umbrellaSay(wxString::Format("%d windows, %g nm apart, from %d frames.", n,
              spacing, atoi(j.get("frames").str().c_str())));
  return n > 0;
}

bool PullPanel::umbrellaCreate(int eqSteps, int prodSteps)
{
  MdTask *t = pullTask(getCalculation());
  if (!t) { umbrellaSay("This is not a GROMACS Dynamics task."); return false; }
  wxBusyCursor wait;
  double spacing = 0.2;
  p_spacing->GetValue().ToDouble(&spacing);
  static int serial = 0;
  const ecce::fs::path dir = ecce::fs::temp_directory_path() /
      ("ecce-umbx-" + std::to_string((long)getpid()) + "-" + std::to_string(serial++));
  std::vector<UmbrellaWindows::Window> windows;
  std::vector<std::string> names;
  std::string why;
  bool ok = UmbrellaWindows::find(t, spacing, p_from->GetValue().utf8_string(),
                                  p_to->GetValue().utf8_string(), dir.string(),
                                  &windows, 0, &why) &&
            UmbrellaWindows::create(t, windows, eqSteps, prodSteps, &names, &why);
  std::error_code ec;
  ecce::fs::remove_all(dir, ec);
  if (!ok) {
    umbrellaSay(U8("Nothing more was made: " + why));
    return false;
  }
  umbrellaSay(wxString::Format("Made %d calculations in the study (Created: open "
              "each in the editor and Save to prepare it, then launch):\n",
              (int)names.size()) + U8([&]{ std::string s;
              for (const std::string& n : names) s += n + "  "; return s; }()));
  return true;
}

bool PullPanel::umbrellaWham(bool jacobian)
{
  MdTask *t = pullTask(getCalculation());
  if (!t) { umbrellaSay("This is not a GROMACS Dynamics task."); return false; }
  wxBusyCursor wait;
  double temp = 300;
  GromacsMDModel model(t->getContentType());
  t->getTaskModel(model);
  if (DynamicsModel *dm = model.getDynamicsModel())
    if (dm->getNVTTemperature() > 0) temp = dm->getNVTTemperature();
  MiniJson j;
  std::string why;
  int used = 0;
  if (!UmbrellaWindows::wham(t, temp, jacobian, false, &j, 0, &used, &why)) {
    umbrellaSay(U8(why));
    return false;
  }
  p_wham = j;
  p_haveWham = true;
  p_pmf->clearCurves();
  p_hist->clearCurves();
  auto column = [](const MiniJson& a) {
    std::vector<double> v;
    for (const MiniJson& x : a.items()) v.push_back(numOf(x));
    return v;
  };
  const MiniJson& pmf = j.get("pmf");
  p_pmf->addCurve(column(pmf.get("distance")), column(pmf.get("pmf")), "PMF");
  const std::vector<double> hx = column(j.get("histograms").get("distance"));
  int w = 0;
  for (const MiniJson& c : j.get("histograms").get("counts").items())
    p_hist->addCurve(hx, column(c), wxString::Format("%d", ++w));
  wxString text = wxString::Format("%d windows, %g K.", used, temp);
  for (const MiniJson& m : j.get("minima").items())
    text += wxString::Format("\nminimum %.3f nm  %.2f kJ/mol",
                             numOf(m.get("distance")), numOf(m.get("pmf")));
  for (const MiniJson& m : j.get("maxima").items())
    text += wxString::Format("\nmaximum %.3f nm  %.2f kJ/mol",
                             numOf(m.get("distance")), numOf(m.get("pmf")));
  if (!j.get("depth").isNull())
    text += "\ndepth " + U8(j.get("depth").str()) + " kJ/mol";
  for (const MiniJson& m : j.get("warnings").items()) text += "\n" + U8(m.str());
  umbrellaSay(text);
  return true;
}

bool PullPanel::umbrellaExport(const wxString& path)
{
  MdTask *t = pullTask(getCalculation());
  if (!t) return false;
  double temp = 300;
  GromacsMDModel model(t->getContentType());
  t->getTaskModel(model);
  if (DynamicsModel *dm = model.getDynamicsModel())
    if (dm->getNVTTemperature() > 0) temp = dm->getNVTTemperature();
  std::string text, why;
  if (!UmbrellaWindows::wham(t, temp, p_jacobian->GetValue(), true, 0, &text, 0, &why)) {
    umbrellaSay(U8(why));
    return false;
  }
  FILE *out = fopen(path.utf8_str(), "wb");
  if (!out) return false;
  const bool ok = fwrite(text.data(), 1, text.size(), out) == text.size();
  return fclose(out) == 0 && ok;
}

bool PullPanel::umbrellaShot(const wxString& path, int width, int height)
{
  static bool handlers = false;
  if (!handlers) { wxInitAllImageHandlers(); handlers = true; }
  wxBitmap bmp(width, height);
  { wxMemoryDC dc(bmp); p_pmf->paintOnto(dc, wxSize(width, height)); }
  return bmp.SaveFile(path, wxBITMAP_TYPE_PNG);
}

void PullPanel::showUmbrella()
{
  p_book->SetSelection(1);
}

wxString PullPanel::umbrellaState() const
{
  return wxString::Format("windows=%d pmf=%d hist=%d",
      p_windows ? p_windows->GetItemCount() : 0,
      p_pmf ? p_pmf->curveCount() : 0, p_hist ? p_hist->curveCount() : 0);
}

void PullPanel::OnFind(wxCommandEvent&)
{
  double sp = 0.2;
  p_spacing->GetValue().ToDouble(&sp);
  umbrellaFind(sp, p_from->GetValue(), p_to->GetValue());
}

void PullPanel::OnCreate(wxCommandEvent&)
{
  long eq = 0, prod = 0;
  p_eq->GetValue().ToLong(&eq);
  p_prod->GetValue().ToLong(&prod);
  umbrellaCreate((int)eq, (int)prod);
}

void PullPanel::OnWham(wxCommandEvent&)
{
  umbrellaWham(p_jacobian->GetValue());
}

void PullPanel::OnUmbrellaExport(wxCommandEvent&)
{
  wxFileDialog dlg(this, "Export the PMF", wxEmptyString, "pmf.csv",
                   "CSV files (*.csv)|*.csv", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  if (dlg.ShowModal() != wxID_OK) return;
  if (!umbrellaExport(dlg.GetPath())) umbrellaSay("The PMF could not be exported.");
}

bool PullPanel::exportCsv(const wxString& path)
{
  IPropCalculation *calc = getCalculation();
  const std::string text = pullCsv(p_series,
      calc ? calc->getURL().getFilePathTail() : std::string());
  FILE *out = fopen(path.utf8_str(), "wb");
  if (out == 0) return false;
  const bool ok = fwrite(text.data(), 1, text.size(), out) == text.size();
  return fclose(out) == 0 && ok;
}

void PullPanel::OnExport(wxCommandEvent&)
{
  IPropCalculation *calc = getCalculation();
  wxFileDialog dlg(this, "Export pull run", wxEmptyString,
                   wxString::FromUTF8(calc ? calc->getURL().getFilePathTail().c_str() : "pull") +
                   "-pull.csv", "CSV files (*.csv)|*.csv",
                   wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  if (dlg.ShowModal() != wxID_OK) return;
  if (!exportCsv(dlg.GetPath()))
    wxMessageBox("The file could not be written.", "Export pull run",
                 wxOK | wxICON_ERROR, this);
}
