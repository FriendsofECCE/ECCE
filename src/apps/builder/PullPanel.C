#include <wx/tokenzr.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/image.h>
#include <wx/dcmemory.h>
#include <wx/listctrl.h>
#include <wx/simplebook.h>
#include <wx/scrolwin.h>
#include <wx/tglbtn.h>
#include <wx/wrapsizer.h>
#include <wx/textctrl.h>
#include <wx/utils.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/dialog.h>
#include <wx/font.h>
#include <wx/stattext.h>

#include "tdat/PropVector.H"
#include "dsm/IPropCalculation.H"
#include "dsm/GromacsMDModel.H"
#include "dsm/MdTask.H"
#include "dsm/UmbrellaWindows.H"
#include "tdat/DynamicsModel.H"
#include "util/Ecce.H"
#include "util/Filesystem.H"
#include "wxgui/ewxWindowUtils.H"
#include "XYPlotCanvas.H"

#include "PullPanel.H"
#include "Builder.H"
#include "GmxAnalysisPanel.H"

namespace {
const wxWindowID ID_EXPORT = wxNewId();
const wxWindowID ID_FIND = wxNewId();
const wxWindowID ID_CREATE = wxNewId();
const wxWindowID ID_WHAM = wxNewId();
const wxWindowID ID_UEXPORT = wxNewId();
const wxWindowID ID_UDETAILS = wxNewId();
const wxWindowID ID_RMSD = wxNewId();
const wxWindowID ID_GYRATE = wxNewId();
const wxWindowID ID_TAB0 = wxNewId();
const wxWindowID ID_TAB1 = wxNewId();

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
    p_prod(NULL), p_noise(NULL), p_detailsDlg(NULL), p_detailsText(NULL), p_info(NULL), p_jacobian(NULL), p_windows(NULL),
    p_pmf(NULL), p_hist(NULL), p_haveWham(false)
{
}

PullPanel::PullPanel(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
  : PropertyPanel(), p_force(NULL), p_dist(NULL), p_text(NULL),
    p_book(NULL), p_spacing(NULL), p_from(NULL), p_to(NULL), p_eq(NULL),
    p_prod(NULL), p_noise(NULL), p_detailsDlg(NULL), p_detailsText(NULL), p_info(NULL), p_jacobian(NULL), p_windows(NULL),
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

  //  Two pages behind two toggle buttons.  A wxSimplebook shows and hides its
  //  pages itself; with GTK's native notebook the Pull run page's Export
  //  button stayed drawn over the docked Umbrella page.
  wxBoxSizer *tabs = new wxBoxSizer(wxHORIZONTAL);
  p_tab[0] = new wxToggleButton(this, ID_TAB0, "Pull run");
  p_tab[1] = new wxToggleButton(this, ID_TAB1, "Umbrella sampling");
  tabs->Add(p_tab[0], 0, wxRIGHT, 4);
  tabs->Add(p_tab[1], 0);
  GetSizer()->Add(tabs, 0, wxALL, 4);
  p_book = new wxSimplebook(this, wxID_ANY);
  GetSizer()->Add(p_book, 1, wxEXPAND);
  wxScrolledWindow *page = new wxScrolledWindow(p_book, wxID_ANY);
  page->SetScrollRate(0, 10);
  page->SetSizer(new wxBoxSizer(wxVERTICAL));
  p_book->AddPage(page, "Pull run");

  p_text = new wxStaticText(page, wxID_ANY, wxEmptyString);
  page->GetSizer()->Add(p_text, 0, wxEXPAND | wxALL, 4);
  //  wraps in a narrow dock
  wxWrapSizer *bar = new wxWrapSizer(wxHORIZONTAL);
  bar->Add(new wxButton(page, ID_EXPORT, "Export .csv..."), 0, wxRIGHT | wxBOTTOM, 6);
  //  The structural checks a pull run needs (did the protein unfold, did
  //  the chain stay compact) are MD Analysis's; these open it on them.
  bar->Add(new wxButton(page, ID_RMSD, "RMSD..."), 0, wxRIGHT | wxBOTTOM, 6);
  bar->Add(new wxButton(page, ID_GYRATE, "Radius of gyration..."), 0, wxBOTTOM, 6);
  page->GetSizer()->Add(bar, 0, wxEXPAND | wxLEFT | wxRIGHT, 4);

  p_force = new XYPlotCanvas(page);
  p_force->SetMinSize(wxSize(260, 160));
  p_force->setXAxisLabel("Time (ps)");
  p_force->setYAxisLabel("Pull force (kJ/mol/nm)");
  page->GetSizer()->Add(p_force, 1, wxEXPAND);
  p_dist = new XYPlotCanvas(page);
  p_dist->SetMinSize(wxSize(260, 160));
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
  Bind(wxEVT_BUTTON, &PullPanel::OnAnalysis, this, ID_RMSD);
  Bind(wxEVT_BUTTON, &PullPanel::OnAnalysis, this, ID_GYRATE);
  Bind(wxEVT_TOGGLEBUTTON, &PullPanel::OnTab, this, ID_TAB0);
  Bind(wxEVT_TOGGLEBUTTON, &PullPanel::OnTab, this, ID_TAB1);

  //  Umbrella sampling: windows from this run, then the PMF from their runs.
  //  Scrolls rather than squeezing the plots in a short dock.
  wxScrolledWindow *u = new wxScrolledWindow(p_book, wxID_ANY);
  u->SetScrollRate(0, 10);
  wxBoxSizer *us = new wxBoxSizer(wxVERTICAL);
  u->SetSizer(us);
  p_book->AddPage(u, "Umbrella sampling");
  wxWrapSizer *row = new wxWrapSizer(wxHORIZONTAL);
  auto field = [&](const char *label, const char *value, wxTextCtrl **out) {
    //  label and field wrap together
    wxBoxSizer *pair = new wxBoxSizer(wxHORIZONTAL);
    pair->Add(new wxStaticText(u, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL);
    *out = new wxTextCtrl(u, wxID_ANY, value);
    (*out)->SetMinSize(wxSize((*out)->GetTextExtent("0000000").x + 20, -1));
    pair->Add(*out, 0, wxLEFT, 3);
    row->Add(pair, 0, wxRIGHT | wxBOTTOM, 10);
  };
  field("Spacing (nm)", "0.1", &p_spacing);
  field("From (nm)", "", &p_from);
  field("To (nm)", "", &p_to);
  field("Equilibration steps", "50000", &p_eq);
  field("Production steps", "250000", &p_prod);
  //  empty = kT at the run temperature; the hint says so
  field("Noise (kJ/mol)", "", &p_noise);
  p_noise->SetHint("kT");
  p_noise->SetToolTip("Minima and maxima that differ from their neighbour by less than "
                      "this are not listed (empty: kT, 2.5 kJ/mol at 300 K)");
  us->Add(row, 0, wxEXPAND | wxALL, 4);
  wxWrapSizer *b1 = new wxWrapSizer(wxHORIZONTAL);
  b1->Add(new wxButton(u, ID_FIND, "Find windows"), 0, wxRIGHT | wxBOTTOM, 6);
  b1->Add(new wxButton(u, ID_CREATE, "Create window calculations"), 0, wxRIGHT | wxBOTTOM, 6);
  b1->Add(new wxButton(u, ID_WHAM, "Run WHAM"), 0, wxRIGHT | wxBOTTOM, 6);
  b1->Add(new wxButton(u, ID_UEXPORT, "Export .csv..."), 0, wxRIGHT | wxBOTTOM, 6);
  b1->Add(new wxButton(u, ID_UDETAILS, "Details..."), 0, wxRIGHT | wxBOTTOM, 6);
  p_jacobian = new wxCheckBox(u, wxID_ANY, "Remove the 2RT ln r volume term");
  b1->Add(p_jacobian, 0, wxALIGN_CENTER_VERTICAL | wxBOTTOM, 6);
  us->Add(b1, 0, wxEXPAND | wxLEFT | wxRIGHT, 4);
  p_windows = new wxListCtrl(u, wxID_ANY, wxDefaultPosition, wxSize(-1, 110),
                             wxLC_REPORT | wxLC_SINGLE_SEL);
  for (const char *c : { "Window", "Frame", "Time (ps)", "Distance (nm)" })
    p_windows->AppendColumn(c, wxLIST_FORMAT_LEFT, wxLIST_AUTOSIZE_USEHEADER);
  us->Add(p_windows, 0, wxEXPAND | wxALL, 4);
  p_pmf = new XYPlotCanvas(u);
  p_pmf->SetMinSize(wxSize(260, 220));
  p_pmf->setXAxisLabel("COM distance (nm)");
  p_pmf->setYAxisLabel("PMF (kJ/mol)");
  us->Add(p_pmf, 2, wxEXPAND);
  p_hist = new XYPlotCanvas(u);
  p_hist->SetMinSize(wxSize(260, 150));
  p_hist->setXAxisLabel("COM distance (nm)");
  p_hist->setYAxisLabel("Window histograms (counts)");
  p_hist->setShowKey(false);
  us->Add(p_hist, 1, wxEXPAND);
  //  Messages have a panel of their own, not a line among the controls; it
  //  is tall enough for the minima and maxima and scrolls beyond that.
  p_info = new wxTextCtrl(u, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                          wxTE_MULTILINE | wxTE_READONLY);
  p_info->SetMinSize(wxSize(-1, 9 * p_info->GetCharHeight() + 12));
  us->Add(p_info, 0, wxEXPAND | wxALL, 4);
  umbrellaSay("Find windows along this pull run, create a calculation for "
              "each, and run WHAM when the production runs are Complete.");
  Bind(wxEVT_BUTTON, &PullPanel::OnFind, this, ID_FIND);
  Bind(wxEVT_BUTTON, &PullPanel::OnCreate, this, ID_CREATE);
  Bind(wxEVT_BUTTON, &PullPanel::OnWham, this, ID_WHAM);
  Bind(wxEVT_BUTTON, &PullPanel::OnUmbrellaExport, this, ID_UEXPORT);
  Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { umbrellaDetails(true); }, ID_UDETAILS);
  showPage(0);
  return true;
}

void PullPanel::clipAudit()
{
  //  each page in turn: the hidden one is not laid out until shown
  for (int n = 0; n < 2; n++) {
    showPage(n);
    for (int i = 0; i < 10; i++) { wxTheApp->Yield(true); wxMilliSleep(30); }
    ewxWindowUtils::clipAuditReport(this, n ? "PullForce-umbrella" : "PullForce-pull");
  }
  showPage(0);
}

double PullPanel::noiseValue() const
{
  double v = 0;
  if (!p_noise || !p_noise->GetValue().Trim().ToDouble(&v) || v < 0) return 0;
  return v;
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
    //  n is read once: InsertItem(n++, Format(n)) left the order to the compiler
    const long row = p_windows->InsertItem(n, wxString::Format("%d", n + 1));
    n++;
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
  double spacing = 0.1;
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
  if (!UmbrellaWindows::wham(t, temp, jacobian, noiseValue(), false, &j, 0, &used, &why)) {
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
  //  The panel says only what a student reads off: the deepest minimum, the
  //  extrema above the noise, the barriers. Everything else is in Details.
  wxString text = wxString::Format("%d windows, %g K.", used, temp);
  const double gmin = j.get("global_minimum").isNull() ? 0
                    : numOf(j.get("global_minimum").get("distance"));
  for (const MiniJson& m : j.get("minima").items())
    text += wxString::Format("\nminimum %.3f nm  %.2f kJ/mol%s",
                             numOf(m.get("distance")), numOf(m.get("pmf")),
                             numOf(m.get("distance")) == gmin ? "  (deepest)" : "");
  for (const MiniJson& m : j.get("barriers").items())
    text += wxString::Format("\nmaximum %.3f nm  %.2f kJ/mol: barrier %.2f kJ/mol "
                             "above the deepest minimum", numOf(m.get("distance")),
                             numOf(m.get("pmf")), numOf(m.get("height")));
  if (!j.get("depth").isNull())
    text += wxString::Format("\nhighest - lowest: %.2f kJ/mol", numOf(j.get("depth")));
  const int nwarn = (int)j.get("warnings").items().size();
  if (nwarn)
    text += wxString::Format("\n%d gap%s between windows: see Details...", nwarn,
                             nwarn == 1 ? "" : "s");

  wxString d = wxString::Format("PMF from %d windows at %g K%s.\nProfile shown from %.3f "
      "to %.3f nm (the ends without histogram data are cut).\nExtrema are significant "
      "when they differ from their neighbour by more than %.2f kJ/mol.\n\n"
      "Every minimum and maximum (* significant):", used, temp,
      p_jacobian->GetValue() ? ", with the 2RT ln r term" : "",
      numOf(j.get("supported").get("from")), numOf(j.get("supported").get("to")),
      numOf(j.get("noise")));
  for (const MiniJson& m : j.get("extrema").items())
    d += wxString::Format("\n%s %s %.3f nm  %8.2f kJ/mol",
                          m.get("significant").boolean() ? "*" : " ",
                          U8(m.get("kind").str()), numOf(m.get("distance")),
                          numOf(m.get("pmf")));
  d += "\n\nOverlap of neighbouring windows (by mean distance):";
  for (const MiniJson& o : j.get("overlap").items())
    d += wxString::Format("\nwindows %2s and %2s near %.3f nm: %5.1f %%",
                          U8(o.get("from").str()), U8(o.get("to").str()),
                          numOf(o.get("at")), 100 * numOf(o.get("overlap")));
  for (const MiniJson& m : j.get("warnings").items()) d += "\nwarning: " + U8(m.str());
  d += "\n\nWindows:";
  int wn = 0;
  for (const MiniJson& w : j.get("windows").items())
    d += wxString::Format("\n%2d  %s", ++wn, U8(w.str()).AfterLast('/'));
  d += "\n\nCommands:";
  for (const MiniJson& c : j.get("commands").items()) d += "\n" + U8(c.str());
  p_details = d;
  if (p_detailsDlg) { p_detailsText->SetValue(p_details); p_detailsText->ShowPosition(0); }
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
  if (!UmbrellaWindows::wham(t, temp, p_jacobian->GetValue(), noiseValue(), true, 0,
                             &text, 0, &why)) {
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
  //  PMF, window histograms and the text under it, on white, as one picture.
  //  The text gets the height its lines need, below the plots' `height`.
  wxString text = p_info->GetValue();
  int lines = 1;
  for (wxUniChar c : text) if (c == '\n') lines++;
  int lineH = 0;
  {
    wxMemoryDC probe;
    wxBitmap one(1, 1);
    probe.SelectObject(one);
    lineH = probe.GetCharHeight() + 1;
  }
  const int textH = lines * lineH + 12;
  wxBitmap bmp(width, height + textH);
  {
    wxMemoryDC dc(bmp);
    dc.SetBackground(*wxWHITE_BRUSH);
    dc.Clear();
    const int pmfH = height * 11 / 16, histH = height * 5 / 16;
    //  paintOnto clears the whole surface, so each plot gets its own clip.
    dc.SetClippingRegion(0, 0, width, pmfH);
    p_pmf->paintOnto(dc, wxSize(width, pmfH));
    dc.DestroyClippingRegion();
    dc.SetDeviceOrigin(0, pmfH);
    dc.SetClippingRegion(0, 0, width, histH);
    p_hist->paintOnto(dc, wxSize(width, histH));
    dc.DestroyClippingRegion();
    dc.SetDeviceOrigin(0, pmfH + histH);
    dc.SetTextForeground(*wxBLACK);
    int y = 4;
    wxStringTokenizer tok(text, "\n");
    while (tok.HasMoreTokens()) {
      dc.DrawText(tok.GetNextToken(), 8, y);
      y += lineH;
    }
    dc.SetDeviceOrigin(0, 0);
  }
  return bmp.SaveFile(path, wxBITMAP_TYPE_PNG);
}

void PullPanel::showUmbrella()
{
  showPage(1);
}

void PullPanel::showPage(int n)
{
  p_book->SetSelection(n);
  for (int i = 0; i < 2; i++) p_tab[i]->SetValue(i == n);
  if (wxScrolledWindow *w = dynamic_cast<wxScrolledWindow*>(p_book->GetCurrentPage()))
    w->FitInside();
  p_book->GetCurrentPage()->Layout();
}

void PullPanel::OnTab(wxCommandEvent& event)
{
  showPage(event.GetId() == ID_TAB1 ? 1 : 0);
}

bool PullPanel::openAnalysis(const std::string& key)
{
  IPropCalculation *calc = getCalculation();
  Builder *b = dynamic_cast<Builder*>(wxTheApp->GetTopWindow());
  if (!calc || !b || !b->openPropertyPanel("MD Analysis")) {
    //  said under the run's summary: a dialog would stop a scripted run
    const wxString why = "MD Analysis is not offered for this run (it needs the stored trajectory).";
    if (!p_text->GetLabel().Contains(why)) p_text->SetLabel(p_text->GetLabel() + "\n" + why);
    p_text->GetParent()->Layout();
    return false;
  }
  for (PropertyPanel *p : PropertyPanel::getPanels(calc->getURL().toString()))
    if (GmxAnalysisPanel *g = dynamic_cast<GmxAnalysisPanel*>(p))
      return g->setAnalysis(key);
  return false;
}

void PullPanel::OnAnalysis(wxCommandEvent& event)
{
  openAnalysis(event.GetId() == ID_RMSD ? "rms" : "gyrate");
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

bool PullPanel::umbrellaDetails(bool show)
{
  if (!show) {
    if (p_detailsDlg) p_detailsDlg->Destroy();
    p_detailsDlg = 0;
    return true;
  }
  if (p_details.empty()) { umbrellaSay("Run WHAM first."); return false; }
  if (!p_detailsDlg) {
    //  own window, resizable, text selectable for copying
    p_detailsDlg = new wxDialog(this, wxID_ANY, "Umbrella sampling: WHAM details",
                                wxDefaultPosition, wxSize(640, 520),
                                wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
    wxBoxSizer *s = new wxBoxSizer(wxVERTICAL);
    p_detailsText = new wxTextCtrl(p_detailsDlg, wxID_ANY, p_details, wxDefaultPosition,
                                   wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxHSCROLL);
    p_detailsText->SetFont(wxFont(wxFontInfo().Family(wxFONTFAMILY_TELETYPE)));
    s->Add(p_detailsText, 1, wxEXPAND | wxALL, 6);
    wxBoxSizer *b = new wxBoxSizer(wxHORIZONTAL);
    wxButton *ex = new wxButton(p_detailsDlg, wxID_ANY, "Export .csv...");
    wxButton *cl = new wxButton(p_detailsDlg, wxID_CLOSE, "Close");
    b->Add(ex, 0, wxRIGHT, 6);
    b->AddStretchSpacer();
    b->Add(cl, 0);
    s->Add(b, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
    p_detailsDlg->SetSizer(s);
    ex->Bind(wxEVT_BUTTON, [this](wxCommandEvent& e) { OnUmbrellaExport(e); });
    cl->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { umbrellaDetails(false); });
    p_detailsDlg->Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent&) { umbrellaDetails(false); });
  }
  p_detailsDlg->Show();
  p_detailsDlg->Raise();
  //  start at the top: SetValue leaves GTK scrolled to the end
  p_detailsText->SetInsertionPoint(0);
  p_detailsText->ShowPosition(0);
  return true;
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
