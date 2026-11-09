#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include "util/Filesystem.H"
#include <fstream>
#include <sstream>
#include <thread>

#include <unistd.h>

#include <wx/app.h>
#include <wx/button.h>
#include <wx/checklst.h>
#include <wx/choice.h>
#include <wx/combobox.h>
#include <wx/filedlg.h>
#include <wx/gauge.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/timer.h>
#include <wx/utils.h>

#include "util/Ecce.H"
#include "util/EcceURL.H"
#include "util/SFile.H"
#include "dsm/IPropCalculation.H"
#include "dsm/MdTask.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/TaskJob.H"
#include "XYPlotCanvas.H"

#include "GmxAnalysisPanel.H"

namespace {

const wxWindowID ID_ANALYSIS = wxNewId();
const wxWindowID ID_RUN = wxNewId();
const wxWindowID ID_EXPORT = wxNewId();
const wxWindowID ID_SHOW = wxNewId();

//  Choice order of the Analysis control, and the CLI's names for them.
const char *KEYS[] = { "rdf", "energy", "rms", "gyrate", "density", "hbond" };
const char *LABELS[] = { "Radial distribution g(r)", "Energy terms",
                         "RMSD from the start", "Radius of gyration",
                         "Density profile", "Hydrogen bonds" };
const int NKEYS = 6;
enum { JOB_GROUPS, JOB_TERMS, JOB_RUN };

wxString U(const std::string& s) { return wxString::FromUTF8(s.c_str()); }
std::string S(const wxString& w) { return std::string(w.ToUTF8()); }

double num(const MiniJson& v)
{
  return v.type() == MiniJson::Number ? atof(v.str().c_str()) : NAN;
}

std::string fmt(double v)
{
  if (std::isnan(v)) return "-";
  char b[40];
  snprintf(b, sizeof b, "%.6g", v);
  return b;
}

std::string quote(const std::string& s)
{
#ifdef _WIN32
  return Ecce::winArg(s);
#else
  std::string o = "'";
  for (char c : s) o += c == '\'' ? std::string("'\\''") : std::string(1, c);
  return o + "'";
#endif
}

}  // namespace

struct GmxAnalysisPanel::Job {
  std::string cmd, out;
  int status = -1;
  int kind = JOB_RUN;
};

IMPLEMENT_DYNAMIC_CLASS(GmxAnalysisPanel, PropertyPanel)

GmxAnalysisPanel::GmxAnalysisPanel()
  : PropertyPanel(), p_analysis(0), p_fields(0), p_ref(0), p_sel(0), p_group(0),
    p_rmax(0), p_bin(0), p_slices(0), p_begin(0), p_end(0), p_axis(0), p_dens(0),
    p_show(0), p_terms(0), p_run(0), p_export(0), p_plot(0), p_results(0),
    p_status(0), p_showLabel(0), p_gauge(0), p_haveResult(false), p_started(0)
{
  p_alive = std::make_shared<GmxAnalysisPanel*>(this);
}

GmxAnalysisPanel::GmxAnalysisPanel(IPropCalculation *calculation, wxWindow *parent,
        wxWindowID id, const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
  : GmxAnalysisPanel()
{
  Create(calculation, parent, id, pos, size, style, name);
}

GmxAnalysisPanel::~GmxAnalysisPanel()
{
  *p_alive = 0;
  if (p_timer) p_timer->Stop();
  std::error_code ec;
  if (!p_scratch.empty()) ecce::fs::remove_all(p_scratch, ec);
}

bool GmxAnalysisPanel::Create(IPropCalculation *calculation, wxWindow *parent,
        wxWindowID id, const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
{
  if (!PropertyPanel::Create(calculation, parent, id, pos, size, style, name)) {
    wxFAIL_MSG( wxT("GmxAnalysisPanel creation failed") );
    return false;
  }
  build();
  return true;
}

bool GmxAnalysisPanel::isRelevant(IPropCalculation *propCalc) const
{
  MdTask *md = dynamic_cast<MdTask*>(propCalc);
  return md && md->getApplicationType() == ResourceDescriptor::AT_GROMACS;
}

void GmxAnalysisPanel::build()
{
  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
  SetSizer(top);
  const int MID = wxALIGN_CENTER_VERTICAL;

  wxBoxSizer *row = new wxBoxSizer(wxHORIZONTAL);
  row->Add(new wxStaticText(this, wxID_ANY, "Analysis:"), 0, MID | wxRIGHT, 4);
  wxArrayString labels;
  for (int i = 0; i < NKEYS; i++) labels.Add(LABELS[i]);
  p_analysis = new wxChoice(this, ID_ANALYSIS, wxDefaultPosition, wxDefaultSize, labels);
  p_analysis->SetSelection(0);
  row->Add(p_analysis, 0, MID);
  top->Add(row, 0, wxALL, 4);

  //  Every parameter has a row; showFields() shows those of the analysis.
  p_fields = new wxFlexGridSizer(2, 4, 6);
  p_fields->AddGrowableCol(1);
  auto label = [this](const char *text) {
    p_fields->Add(new wxStaticText(this, wxID_ANY, text), 0, wxALIGN_CENTER_VERTICAL);
  };
  const wxString selTip = "Atoms by molecule, residue, element or atom name, for "
      "example residue:NA, element:O or residue:SOL & element:O (both must hold). "
      "Several names: element:Na,K.";
  label("Reference:");
  p_ref = new wxComboBox(this, wxID_ANY);
  p_ref->SetToolTip(selTip);
  p_fields->Add(p_ref, 1, wxEXPAND);
  label("Selection:");
  p_sel = new wxComboBox(this, wxID_ANY);
  p_sel->SetToolTip(selTip);
  p_fields->Add(p_sel, 1, wxEXPAND);
  label("Group:");
  p_group = new wxComboBox(this, wxID_ANY);
  p_group->SetToolTip(selTip);
  p_fields->Add(p_group, 1, wxEXPAND);
  //  Small fields share a row, so the plot keeps its height in a dock.
  const wxSize NUM(70, -1);
  label("Cut-off (nm):");
  wxBoxSizer *rdfRow = new wxBoxSizer(wxHORIZONTAL);
  p_rmax = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, NUM);
  p_rmax->SetToolTip("Largest distance of g(r); empty: half the shortest box edge.");
  rdfRow->Add(p_rmax, 0, MID | wxRIGHT, 8);
  rdfRow->Add(new wxStaticText(this, wxID_ANY, "Bin width (nm):"), 0, MID | wxRIGHT, 4);
  p_bin = new wxTextCtrl(this, wxID_ANY, "0.002", wxDefaultPosition, NUM);
  rdfRow->Add(p_bin, 0, MID);
  p_fields->Add(rdfRow, 0);
  label("Axis:");
  wxBoxSizer *densRow = new wxBoxSizer(wxHORIZONTAL);
  wxArrayString axes;
  axes.Add("x");
  axes.Add("y");
  axes.Add("z");
  p_axis = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, axes);
  p_axis->SetSelection(2);
  densRow->Add(p_axis, 0, MID | wxRIGHT, 8);
  densRow->Add(new wxStaticText(this, wxID_ANY, "Slices:"), 0, MID | wxRIGHT, 4);
  p_slices = new wxTextCtrl(this, wxID_ANY, "50", wxDefaultPosition, NUM);
  densRow->Add(p_slices, 0, MID | wxRIGHT, 8);
  wxArrayString dens;
  dens.Add("number");
  dens.Add("mass");
  dens.Add("charge");
  p_dens = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, dens);
  p_dens->SetSelection(0);
  p_dens->SetToolTip("Number density (nm⁻³), mass density (kg m⁻³) or charge density (e nm⁻³)");
  densRow->Add(p_dens, 0, MID);
  p_fields->Add(densRow, 0);
  label("Terms:");
  p_terms = new wxCheckListBox(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 90));
  p_fields->Add(p_terms, 1, wxEXPAND);
  label("From (ps):");
  wxBoxSizer *times = new wxBoxSizer(wxHORIZONTAL);
  p_begin = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, NUM);
  p_begin->SetToolTip("First time used; empty: the start of the run.");
  times->Add(p_begin, 0, MID | wxRIGHT, 4);
  times->Add(new wxStaticText(this, wxID_ANY, "to"), 0, MID | wxRIGHT, 4);
  p_end = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, NUM);
  p_end->SetToolTip("Last time used; empty: the end of the run.");
  times->Add(p_end, 0, MID);
  p_fields->Add(times, 0);
  top->Add(p_fields, 0, wxEXPAND | wxLEFT | wxRIGHT, 6);

  wxBoxSizer *bar = new wxBoxSizer(wxHORIZONTAL);
  p_run = new wxButton(this, ID_RUN, "Run");
  p_run->SetToolTip("Run the GROMACS tool on the stored trajectory and energies");
  p_export = new wxButton(this, ID_EXPORT, "Export .csv...");
  p_export->Enable(false);
  bar->Add(p_run, 0, wxRIGHT, 4);
  bar->Add(p_export, 0, wxRIGHT, 12);
  p_showLabel = new wxStaticText(this, wxID_ANY, "Plot:");
  bar->Add(p_showLabel, 0, MID | wxRIGHT, 4);
  wxArrayString show;
  show.Add("g(r)");
  show.Add("cn(r), coordination number");
  p_show = new wxChoice(this, ID_SHOW, wxDefaultPosition, wxDefaultSize, show);
  p_show->SetSelection(0);
  bar->Add(p_show, 0, MID);
  top->Add(bar, 0, wxALL, 4);

  p_plot = new XYPlotCanvas(this);
  p_plot->SetMinSize(wxSize(300, 160));
  p_plot->setShowKey(true);
  top->Add(p_plot, 3, wxEXPAND);

  p_results = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
                             wxSize(-1, 70), wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP);
  p_results->SetMinSize(wxSize(-1, 60));
  top->Add(p_results, 1, wxEXPAND | wxTOP, 4);

  //  Status and notes have their own line below the results.
  wxBoxSizer *st = new wxBoxSizer(wxHORIZONTAL);
  p_gauge = new wxGauge(this, wxID_ANY, 100, wxDefaultPosition, wxSize(80, 12));
  p_gauge->Hide();
  st->Add(p_gauge, 0, MID | wxRIGHT, 6);
  p_status = new wxStaticText(this, wxID_ANY, wxEmptyString);
  st->Add(p_status, 1, MID);
  top->Add(st, 0, wxEXPAND | wxALL, 4);

  Bind(wxEVT_CHOICE, &GmxAnalysisPanel::onAnalysis, this, ID_ANALYSIS);
  Bind(wxEVT_CHOICE, &GmxAnalysisPanel::onPlotChoice, this, ID_SHOW);
  Bind(wxEVT_BUTTON, &GmxAnalysisPanel::onRun, this, ID_RUN);
  Bind(wxEVT_BUTTON, &GmxAnalysisPanel::onExport, this, ID_EXPORT);
  p_timer.reset(new wxTimer(this));
  Bind(wxEVT_TIMER, &GmxAnalysisPanel::onTimer, this);

  showFields();
  //  The group names come from the topology: gmx dump of topol.tpr.
  startJob({ "groups" }, "Reading the topology...", JOB_GROUPS);
}

void GmxAnalysisPanel::showFields()
{
  const std::string a = analysis();
  const bool rdf = a == "rdf", hb = a == "hbond", en = a == "energy";
  const bool grp = a == "rms" || a == "gyrate" || a == "density";
  //  Rows in Add order: label then control; index 2k and 2k+1.
  const bool rows[] = { rdf || hb, rdf || hb, grp, rdf, a == "density", en, true };
  for (size_t k = 0; k < sizeof rows / sizeof rows[0]; k++) {
    p_fields->Show(2 * k, rows[k]);
    p_fields->Show(2 * k + 1, rows[k]);
  }
  wxSizerItem *refLabel = p_fields->GetItem((size_t)0);
  wxSizerItem *selLabel = p_fields->GetItem((size_t)2);
  if (refLabel && refLabel->GetWindow())
    refLabel->GetWindow()->SetLabel(hb ? "Group:" : "Reference:");
  if (selLabel && selLabel->GetWindow())
    selLabel->GetWindow()->SetLabel(hb ? "Second group (optional):" : "Selection:");
  p_showLabel->Show(rdf);
  p_show->Show(rdf);
  if (en && p_terms->IsEmpty() && !p_job)
    startJob({ "energy" }, "Reading the energy terms...", JOB_TERMS);
  Layout();
}

std::string GmxAnalysisPanel::analysis() const
{
  const int i = p_analysis ? p_analysis->GetSelection() : 0;
  return KEYS[i >= 0 && i < NKEYS ? i : 0];
}

bool GmxAnalysisPanel::setAnalysis(const std::string& key)
{
  for (int i = 0; i < NKEYS; i++)
    if (key == KEYS[i]) {
      p_analysis->SetSelection(i);
      showFields();
      return true;
    }
  return false;
}

std::string GmxAnalysisPanel::setField(const std::string& name, const std::string& value)
{
  const wxString v = U(value);
  if (name == "ref") p_ref->SetValue(v);
  else if (name == "sel") p_sel->SetValue(v);
  else if (name == "group") p_group->SetValue(v);
  else if (name == "rmax") p_rmax->SetValue(v);
  else if (name == "bin") p_bin->SetValue(v);
  else if (name == "slices") p_slices->SetValue(v);
  else if (name == "begin") p_begin->SetValue(v);
  else if (name == "end") p_end->SetValue(v);
  else if (name == "axis") {
    if (!p_axis->SetStringSelection(v)) return "no axis " + value;
  } else if (name == "dens") {
    if (!p_dens->SetStringSelection(v)) return "no density " + value;
  } else if (name == "plot") {
    p_show->SetSelection(value == "cn" ? 1 : 0);
    redraw();
  } else if (name == "terms") {
    //  Comma-separated; ticks exactly these.
    std::vector<std::string> want;
    std::istringstream in(value);
    for (std::string t; std::getline(in, t, ','); ) if (!t.empty()) want.push_back(t);
    for (unsigned i = 0; i < p_terms->GetCount(); i++) p_terms->Check(i, false);
    for (const std::string& t : want) {
      const int i = p_terms->FindString(U(t), true);
      if (i == wxNOT_FOUND) return "no energy term " + t;
      p_terms->Check(i, true);
    }
  } else {
    return "unknown field " + name;
  }
  return "ok";
}

std::string GmxAnalysisPanel::groupChoices() const
{
  std::string out;
  for (unsigned i = 0; i < p_ref->GetCount(); i++)
    out += (i ? "|" : "") + S(p_ref->GetString(i));
  return out;
}

std::string GmxAnalysisPanel::energyTerms() const
{
  std::string out;
  for (unsigned i = 0; i < p_terms->GetCount(); i++)
    out += (i ? "|" : "") + S(p_terms->GetString(i));
  return out;
}

//  The folder ecce-gmxanalysis reads: the calculation's own in local mode;
//  from a data server, a scratch copy of the files it uses.
std::string GmxAnalysisPanel::calcFolder()
{
  if (!p_folder.empty()) return p_folder;
  IPropCalculation *calc = getCalculation();
  if (!calc) return "";
  const std::string url = calc->getURL().toString();
  if (url.compare(0, 7, "file://") == 0) {
    p_folder = url.substr(7);
    while (p_folder.size() > 1 && p_folder.back() == '/') p_folder.pop_back();
    return p_folder;
  }
  TaskJob *job = dynamic_cast<TaskJob*>(calc);
  if (!job) return "";
  static int serial = 0;
  const ecce::fs::path dir = ecce::fs::temp_directory_path() /
      ("ecce-gmxa-" + std::to_string((long)getpid()) + "-" + std::to_string(serial++));
  std::error_code ec;
  ecce::fs::create_directories(dir / "Outputs", ec);
  p_scratch = dir.string();
  wxBusyCursor wait;
  for (const char *f : { "traj.xtc", "ener.edr", "topol.tpr", "ecce_pp.top" }) {
    SFile *dest = new SFile((dir / "Outputs" / f).string());
    SFile *got = job->getAnyFile(calc->getURL().getChild("Outputs").getChild(f), dest);
    if (got != dest) delete got;
    delete dest;
  }
  p_folder = p_scratch;
  return p_folder;
}

std::string GmxAnalysisPanel::command(const std::vector<std::string>& args) const
{
  const char *home = getenv("ECCE_HOME");
  const std::string script = home && *home ? std::string(home) + "/bin/ecce-gmxanalysis"
                                           : std::string("ecce-gmxanalysis");
  std::string cmd;
#ifdef _WIN32
  ecce::fs::path py = ecce::fs::path(Ecce::ecceHome()) / "python" / "python.exe";
  cmd = quote(ecce::fs::exists(py) ? py.string() : std::string("python.exe")) + " " +
        quote(script);
#else
  cmd = quote(script);
#endif
  for (const std::string& a : args) cmd += " " + quote(a);
#ifndef _WIN32
  cmd += " 2>/dev/null";
#endif
  return cmd;
}

void GmxAnalysisPanel::setStatus(const wxString& text)
{
  //  A static text reads "&" as a mnemonic: "residue:SOL & element:O".
  wxString shown = text;
  shown.Replace("&", "&&");
  p_status->SetLabel(shown);
  p_status->SetToolTip(text);
  Layout();
}

void GmxAnalysisPanel::startJob(const std::vector<std::string>& args,
                                const wxString& what, int kind)
{
  const std::string folder = calcFolder();
  if (folder.empty()) {
    setStatus("This calculation has no stored files to analyse.");
    return;
  }
  std::vector<std::string> full = { folder };
  full.insert(full.end(), args.begin(), args.end());
  full.push_back("--json");
  std::shared_ptr<Job> job = std::make_shared<Job>();
  job->cmd = command(full);
  job->kind = kind;
  p_job = job;
  p_started = (long)time(0);
  p_run->Enable(false);
  setStatus(what);
  p_gauge->Show();
  p_gauge->Pulse();
  Layout();
  p_timer->Start(150);
  std::shared_ptr<GmxAnalysisPanel*> alive = p_alive;
  //  gmx may take minutes on a long trajectory: never on the GUI thread.
  std::thread([job, alive]() {
    job->status = Ecce::readCommand(job->cmd, job->out);
    if (wxTheApp) wxTheApp->CallAfter([alive, job]() { deliver(alive, job); });
  }).detach();
}

void GmxAnalysisPanel::deliver(std::shared_ptr<GmxAnalysisPanel*> alive,
                               std::shared_ptr<Job> job)
{
  if (GmxAnalysisPanel *panel = *alive) panel->finish(job);
}

void GmxAnalysisPanel::onTimer(wxTimerEvent&)
{
  if (!p_job) return;
  p_gauge->Pulse();
}

void GmxAnalysisPanel::finish(std::shared_ptr<Job> job)
{
  if (job != p_job) return;
  p_job.reset();
  p_timer->Stop();
  p_gauge->Hide();
  p_run->Enable(true);
  MiniJson res;
  std::string err;
  //  The JSON is the last line; Python may warn above it.
  std::string text = job->out;
  while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) text.pop_back();
  const size_t nl = text.rfind('\n');
  const std::string last = nl == std::string::npos ? text : text.substr(nl + 1);
  if (!MiniJson::parse(last, res, err) || res.type() != MiniJson::Object) {
    setStatus("ecce-gmxanalysis could not be run (is it installed?)");
    return;
  }
  const std::string error = res.get("error").str();
  if (job->kind == JOB_GROUPS) {
    if (!error.empty()) setStatus(U(error));
    else {
      fillPickers(res.get("groups"));
      std::string notes;
      for (const MiniJson& n : res.get("notes").items()) notes += (notes.empty() ? "" : " ") + n.str();
      setStatus(notes.empty() ? wxString("Choose the groups and press Run.") : U(notes));
    }
    if (analysis() == "energy" && p_terms->IsEmpty())
      startJob({ "energy" }, "Reading the energy terms...", JOB_TERMS);
    return;
  }
  if (job->kind == JOB_TERMS) {
    if (!error.empty()) { setStatus(U(error)); return; }
    p_terms->Clear();
    for (const MiniJson& t : res.get("terms").items()) p_terms->Append(U(t.str()));
    for (const char *d : { "Potential", "Temperature" }) {
      const int i = p_terms->FindString(d, true);
      if (i != wxNOT_FOUND) p_terms->Check(i, true);
    }
    setStatus(wxString::Format("%u energy terms; tick the ones to plot.",
                               p_terms->GetCount()));
    Layout();
    return;
  }
  if (!error.empty()) {
    setStatus(U(error));
    return;
  }
  p_lastJson = last;
  p_result = res;
  p_haveResult = true;
  p_export->Enable(true);
  std::string notes;
  for (const MiniJson& n : res.get("notes").items()) notes += (notes.empty() ? "" : " ") + n.str();
  setStatus(wxString::Format("Done in %ld s. ", (long)time(0) - p_started) + U(notes));
  redraw();
}

void GmxAnalysisPanel::fillPickers(const MiniJson& groups)
{
  wxArrayString items;
  items.Add("all");
  const char *kinds[][2] = { { "molecules", "molecule" }, { "residues", "residue" },
                             { "elements", "element" }, { "atomnames", "atom" } };
  for (auto& k : kinds) {
    int n = 0;
    for (const MiniJson& g : groups.get(k[0]).items())
      if (n++ < 200) items.Add(U(std::string(k[1]) + ":" + g.get("name").str()));
  }
  for (wxComboBox *c : { p_ref, p_sel, p_group }) {
    const wxString keep = c->GetValue();
    c->Set(items);
    c->SetValue(keep);
  }
  //  Sensible first choices: the first two residue names, and the first.
  std::vector<std::string> res;
  for (const MiniJson& g : groups.get("residues").items()) res.push_back(g.get("name").str());
  if (p_ref->GetValue().IsEmpty() && !res.empty()) p_ref->SetValue(U("residue:" + res[0]));
  if (p_sel->GetValue().IsEmpty() && res.size() > 1)
    p_sel->SetValue(U("residue:" + res[res.size() > 2 ? 2 : 1]));
  if (p_group->GetValue().IsEmpty() && !res.empty()) p_group->SetValue(U("residue:" + res[0]));
}

std::vector<std::string> GmxAnalysisPanel::runArgs(std::string& problem)
{
  const std::string a = analysis();
  std::vector<std::string> args = { a };
  auto value = [](wxWindow *w) {
    wxComboBox *c = dynamic_cast<wxComboBox*>(w);
    wxTextCtrl *t = dynamic_cast<wxTextCtrl*>(w);
    return S(c ? c->GetValue() : t ? t->GetValue() : wxString());
  };
  auto trim = [](std::string s) {
    while (!s.empty() && isspace((unsigned char)s.back())) s.pop_back();
    while (!s.empty() && isspace((unsigned char)s[0])) s.erase(0, 1);
    return s;
  };
  auto number = [&](wxTextCtrl *t, const char *opt, const char *what) {
    const std::string v = trim(value(t));
    if (v.empty()) return;
    char *end = 0;
    strtod(v.c_str(), &end);
    if (!end || *end) { problem = std::string(what) + " is not a number: " + v; return; }
    args.push_back(opt);
    args.push_back(v);
  };
  if (a == "rdf" || a == "hbond") {
    const std::string r = trim(value(p_ref)), s = trim(value(p_sel));
    if (r.empty()) problem = "Choose the reference group.";
    else if (a == "rdf" && s.empty()) problem = "Choose the selection.";
    args.push_back("--ref"); args.push_back(r);
    if (!s.empty()) { args.push_back("--sel"); args.push_back(s); }
  }
  if (a == "rdf") {
    number(p_rmax, "--rmax", "The cut-off");
    number(p_bin, "--bin", "The bin width");
  }
  if (a == "rms" || a == "gyrate" || a == "density") {
    const std::string g = trim(value(p_group));
    if (g.empty()) problem = "Choose the group.";
    args.push_back("--group"); args.push_back(g);
  }
  if (a == "density") {
    args.push_back("--axis"); args.push_back(S(p_axis->GetStringSelection()));
    args.push_back("--dens"); args.push_back(S(p_dens->GetStringSelection()));
    number(p_slices, "--slices", "The number of slices");
  }
  if (a == "energy") {
    std::string terms;
    for (unsigned i = 0; i < p_terms->GetCount(); i++)
      if (p_terms->IsChecked(i)) terms += (terms.empty() ? "" : ",") + S(p_terms->GetString(i));
    if (terms.empty()) problem = p_terms->IsEmpty() ? "The energy terms are not read yet."
                                                    : "Tick at least one energy term.";
    args.push_back("--terms"); args.push_back(terms);
  }
  number(p_begin, "--begin", "The start time");
  number(p_end, "--end", "The end time");
  return args;
}

bool GmxAnalysisPanel::run()
{
  if (p_job) {
    setStatus("Still running the last analysis.");
    return false;
  }
  std::string problem;
  std::vector<std::string> args = runArgs(problem);
  if (!problem.empty()) {
    setStatus(U(problem));
    return false;
  }
  startJob(args, U("Running gmx " + args[0] + "..."), JOB_RUN);
  return (bool)p_job;
}

void GmxAnalysisPanel::redraw()
{
  p_plot->clearCurves();
  p_plot->clearVLines();
  p_results->Clear();
  if (!p_haveResult) { p_plot->Refresh(); return; }
  const MiniJson& x = p_result.get("x");
  std::vector<double> xs;
  for (const MiniJson& v : x.get("values").items()) xs.push_back(num(v));
  const std::string a = p_result.get("analysis").str();
  const std::string xunit = x.get("unit").str();
  std::string xname = x.get("name").str();
  if (xname == "time") xname = "Time";
  p_plot->setXAxisLabel(U(xname + (xunit.empty() ? "" : " (" + xunit + ")")));
  std::vector<std::string> units;
  int drawn = 0;
  const auto& series = p_result.get("series").items();
  for (size_t k = 0; k < series.size(); k++) {
    //  g(r) and cn(r) differ by orders of magnitude: one at a time.
    if (a == "rdf" && (int)k != p_show->GetSelection()) continue;
    std::vector<double> ys;
    for (const MiniJson& v : series[k].get("values").items()) ys.push_back(num(v));
    const int c = p_plot->addCurve(xs, ys, U(series[k].get("name").str()), drawn,
                                   XYPlotCanvas::LINE);
    p_plot->setCurveColour(c, p_plot->palette().series(drawn));
    p_plot->setCurveStyle(c, false, 2);
    const std::string u = series[k].get("unit").str();
    if (std::find(units.begin(), units.end(), u) == units.end()) units.push_back(u);
    drawn++;
  }
  std::string ylab;
  if (a == "rdf") ylab = p_show->GetSelection() == 1 ? "cn(r)" : "g(r)";
  else if (drawn == 1) ylab = series.empty() ? "" : series[0].get("name").str();
  else ylab = a == "energy" ? "Energy terms" : "Value";
  std::string ustr;
  for (const std::string& u : units) if (!u.empty()) ustr += (ustr.empty() ? "" : ", ") + u;
  if (!ustr.empty()) ylab += " (" + ustr + ")";
  p_plot->setYAxisLabel(U(ylab));
  p_plot->fitAll();

  std::string text;
  for (const MiniJson& s : p_result.get("summary").items()) {
    const std::string unit = s.get("unit").str();
    if (s.get("value").type() == MiniJson::Number)
      text += s.get("name").str() + ": " + fmt(num(s.get("value"))) +
              (unit.empty() ? "" : " " + unit) + "\n";
    else
      text += s.get("name").str() + ": average " + fmt(num(s.get("average"))) +
              ", error estimate " + fmt(num(s.get("error estimate"))) +
              ", RMSD " + fmt(num(s.get("rmsd"))) + ", drift " +
              fmt(num(s.get("drift"))) + (unit.empty() ? "" : " " + unit) + "\n";
  }
  for (const MiniJson& g : p_result.get("groups").items())
    text += g.get("selection").str() + ": " + g.get("atoms").str() + " atoms\n";
  for (const MiniJson& c : p_result.get("commands").items())
    text += c.str() + "\n";
  p_results->ChangeValue(U(text));
  p_plot->Refresh();
}

void GmxAnalysisPanel::onAnalysis(wxCommandEvent&)
{
  showFields();
}

void GmxAnalysisPanel::onRun(wxCommandEvent&)
{
  run();
}

void GmxAnalysisPanel::onPlotChoice(wxCommandEvent&)
{
  redraw();
}

bool GmxAnalysisPanel::exportCsv(const wxString& path)
{
  if (!p_haveResult) return false;
  std::ofstream out(path.ToStdString().c_str());
  if (!out) return false;
  const MiniJson& x = p_result.get("x");
  auto head = [](const MiniJson& s) {
    const std::string u = s.get("unit").str(), n = s.get("name").str();
    return u.empty() ? n : n + " (" + u + ")";
  };
  out << "# " << p_result.get("analysis").str() << " of " << p_result.get("calc").str() << "\n";
  for (const MiniJson& s : p_result.get("summary").items()) {
    out << "# " << s.get("name").str();
    for (const char *k : { "value", "average", "error estimate", "rmsd", "drift" })
      if (s.get(k).type() == MiniJson::Number) out << " " << k << "=" << s.get(k).str();
    out << " " << s.get("unit").str() << "\n";
  }
  out << head(x);
  const auto& series = p_result.get("series").items();
  for (const MiniJson& s : series) out << "," << head(s);
  out << "\n";
  const auto& xs = x.get("values").items();
  for (size_t i = 0; i < xs.size(); i++) {
    out << xs[i].str();
    for (const MiniJson& s : series) {
      const auto& v = s.get("values").items();
      out << "," << (i < v.size() ? v[i].str() : std::string());
    }
    out << "\n";
  }
  return (bool)out;
}

void GmxAnalysisPanel::onExport(wxCommandEvent&)
{
  wxFileDialog dlg(this, "Export .csv", wxEmptyString,
                   U(p_result.get("analysis").str() + ".csv"),
                   "CSV files (*.csv)|*.csv", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  if (dlg.ShowModal() != wxID_OK) return;
  if (!exportCsv(dlg.GetPath())) setStatus("Could not write " + dlg.GetPath());
}

std::string GmxAnalysisPanel::plotDump() const
{
  std::string out;
  char b[64];
  for (int c = 0; c < p_plot->curveCount(); c++) {
    out += S(p_plot->curveName(c)) + "|";
    for (int i = 0; i < p_plot->pointCount(c); i++) {
      snprintf(b, sizeof b, "%s%.9g %.9g", i ? ";" : "", p_plot->xAt(c, i), p_plot->yAt(c, i));
      out += b;
    }
    out += "\n";
  }
  return out;
}

wxString GmxAnalysisPanel::status() const
{
  return p_status->GetLabelText();
}

wxString GmxAnalysisPanel::resultsText() const
{
  return p_results->GetValue();
}
