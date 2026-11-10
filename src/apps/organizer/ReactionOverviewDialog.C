#include "ReactionOverviewDialog.H"
#include <wx/display.h>
#include "wxgui/WindowShot.H"
#include <algorithm>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include "util/Filesystem.H"
#include <fstream>
#include <functional>
#include <map>
#include <sstream>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/collpane.h>
#include <wx/panel.h>
#include <wx/radiobut.h>
#include <wx/scrolwin.h>
#include <wx/settings.h>
#include <wx/spinctrl.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/html/htmlwin.h>
#include <wx/html/htmlcell.h>
#include <wx/image.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include "CompareEnergies.H"
#include "RateBox.H"
#include "WorkflowSteps.H"
#include "dsm/EDSIFactory.H"
#include "dsm/ICalculation.H"
#include "dsm/PropertyTask.H"
#include "dsm/Session.H"
#include "tdat/Fragment.H"
#include "tdat/PropVector.H"
#include "tdat/TAtm.H"
#include "tdat/TRunType.H"
#include "util/EcceURL.H"
#include "util/ThermoRun.H"
#include "util/VibFrequency.H"
#include "wxgui/WxHelpViewer.H"

using std::string;
using std::vector;

namespace {

// One CSV line; fields may be double-quoted with "" for a quote.
vector<string> csvFields(const string& line)
{
  vector<string> f;
  string cur;
  bool q = false;
  for (size_t i = 0; i < line.size(); i++) {
    const char c = line[i];
    if (q) {
      if (c == '"' && i + 1 < line.size() && line[i + 1] == '"') { cur += '"'; i++; }
      else if (c == '"') q = false;
      else cur += c;
    } else if (c == '"') q = true;
    else if (c == ',') { f.push_back(cur); cur.clear(); }
    else cur += c;
  }
  f.push_back(cur);
  return f;
}

string num(double v, const char *fmt = "%.3f")
{
  char b[64];
  snprintf(b, sizeof b, fmt, v);
  return b;
}

string trim(const string& s)
{
  const size_t b = s.find_first_not_of(" \t\r");
  return b == string::npos ? string() : s.substr(b, s.find_last_not_of(" \t\r") - b + 1);
}

string csvq(const string& s)
{
  if (s.find_first_of(",\"\n") == string::npos) return s;
  string o = "\"";
  for (char c : s) { if (c == '"') o += '"'; o += c; }
  return o + "\"";
}

// One side of a reaction as the calculation folders ecce-rate takes: no
// coefficients, each species once.  Empty with `why` set when it cannot.
vector<string> sidePaths(const string& side, string& why)
{
  vector<string> out;
  std::istringstream in(side);
  for (string item; std::getline(in, item, ','); ) {
    item = trim(item);
    if (item.empty()) continue;
    if (item.find('*') != string::npos) {
      why = "a reaction with a transition state takes each species once (no N* coefficient)";
      return vector<string>();
    }
    const string p = rateLocalPath(item);
    if (p.empty()) {
      why = "rate constants need stored local calculations (a file:// URL or a path): " + item;
      return vector<string>();
    }
    out.push_back(p);
  }
  return out;
}

// The calculation names of one side, "a + b" (sep " + ") or "a, b" (sep ", ").
string shortSide(const string& side, const string& sep)
{
  string out;
  std::istringstream in(side);
  for (string item; std::getline(in, item, ','); ) {
    item = trim(item);
    while (item.size() > 1 && item.back() == '/') item.pop_back();
    if (item.empty()) continue;
    const size_t slash = item.rfind('/');
    out += (out.empty() ? "" : sep) + (slash == string::npos ? item : item.substr(slash + 1));
  }
  return out;
}

// Reaction `a, b -> [ts] -> c` split into its parts; false when the line has
// brackets but not that form.
bool splitTs(const string& line, string& lhs, string& ts, string& rhs)
{
  const size_t a1 = line.find("->");
  const size_t a2 = a1 == string::npos ? string::npos : line.find("->", a1 + 2);
  if (a2 == string::npos || line.find("->", a2 + 2) != string::npos) return false;
  const string mid = trim(line.substr(a1 + 2, a2 - a1 - 2));
  if (mid.size() < 3 || mid[0] != '[' || mid.back() != ']') return false;
  lhs = trim(line.substr(0, a1));
  ts = trim(mid.substr(1, mid.size() - 2));
  rhs = trim(line.substr(a2 + 2));
  return !lhs.empty() && !ts.empty() && !rhs.empty();
}

// Species written as bare calculation names become base + "/" + name; URLs,
// paths, N* and [ ] stay as typed.  `why` names a name that is not a
// calculation in a folder we can look into.
string resolveNames(const string& line, const string& base, string& why)
{
  if (base.empty()) return line;
  const string dir = rateLocalPath(base);
  string out;
  size_t pos = 0;
  while (pos < line.size()) {
    const size_t arrow = line.find("->", pos);
    const string part = line.substr(pos, arrow == string::npos ? string::npos : arrow - pos);
    string rebuilt;
    std::istringstream in(part);
    string item;
    bool first = true;
    while (std::getline(in, item, ',')) {
      string lead, tail, body = trim(item);
      if (!body.empty() && body[0] == '[') { lead = "["; body = trim(body.substr(1)); }
      if (!body.empty() && body.back() == ']') { tail = "]"; body = trim(body.substr(0, body.size() - 1)); }
      string coef;
      const size_t star = body.find('*');
      if (star != string::npos) { coef = trim(body.substr(0, star + 1)); body = trim(body.substr(star + 1)); }
      if (!body.empty() && body.find('/') == string::npos && body.compare(0, 7, "file://") != 0) {
        if (!dir.empty() && !ecce::fs::is_directory(dir + "/" + body)) {
          if (why.empty()) why = "no calculation '" + body + "' in this folder";
        } else {
          body = base + (base.back() == '/' ? "" : "/") + body;
        }
      }
      rebuilt += string(first ? "" : ", ") + lead + coef + body + tail;
      first = false;
    }
    // keep the spacing around the arrows; the parts are trimmed above
    const string pre = part.empty() || part[0] != ' ' ? "" : " ";
    const string post = part.empty() || part.back() != ' ' ? "" : " ";
    out += pre + rebuilt + post;
    if (arrow == string::npos) break;
    out += "->";
    pos = arrow + 2;
  }
  return out;
}


// A reaction line as the picker shows it: coefficient and calculation name per
// species, the TS (empty for none).
struct Spec {
  vector<std::pair<int, string> > re, pr;
  string ts;
};

string lastPart(string n)
{
  while (n.size() > 1 && n.back() == '/') n.pop_back();
  const size_t slash = n.rfind('/');
  return slash == string::npos ? n : n.substr(slash + 1);
}

vector<std::pair<int, string> > parseSide(const string& side)
{
  vector<std::pair<int, string> > out;
  std::istringstream in(side);
  for (string item; std::getline(in, item, ','); ) {
    item = trim(item);
    if (item.empty()) continue;
    int n = 1;
    const size_t star = item.find('*');
    if (star != string::npos) { n = std::max(1, atoi(item.substr(0, star).c_str())); item = trim(item.substr(star + 1)); }
    out.push_back(std::make_pair(n, lastPart(item)));
  }
  return out;
}

Spec parseSpec(const string& line)
{
  Spec s;
  string lhs, ts, rhs;
  if (line.find('[') != string::npos && splitTs(line, lhs, ts, rhs)) {
    s.ts = lastPart(ts);
  } else {
    const size_t a = line.find("->");
    lhs = line.substr(0, a);
    rhs = a == string::npos ? string() : line.substr(a + 2);
  }
  s.re = parseSide(lhs);
  s.pr = parseSide(rhs);
  return s;
}

string formatSide(const vector<std::pair<int, string> >& side)
{
  string o;
  for (const std::pair<int, string>& t : side)
    o += (o.empty() ? "" : ", ") + (t.first > 1 ? std::to_string(t.first) + "*" : string()) + t.second;
  return o;
}

string formatLine(const Spec& s)
{
  return formatSide(s.re) + (s.ts.empty() ? " -> " : " -> [" + s.ts + "] -> ") + formatSide(s.pr);
}

// Cell text of one side: "2 x water + b".
string cellSide(const vector<std::pair<int, string> >& side)
{
  string o;
  for (const std::pair<int, string>& t : side)
    o += (o.empty() ? "" : " + ") + (t.first > 1 ? std::to_string(t.first) + " \xC3\x97 " : string()) + t.second;
  return o;
}

// Imaginary frequencies of a stored calculation; -1 when it has no frequencies.
int imaginaryCount(const string& url)
{
  PropertyTask *t = dynamic_cast<PropertyTask*>(EDSIFactory::getResource(EcceURL(url)));
  PropVector *v = t ? dynamic_cast<PropVector*>(t->getProperty("VIBFREQ")) : 0;
  if (!v || v->values().empty()) return -1;
  int n = 0;
  for (double f : v->values()) if (VibFrequency::isImaginary(f)) n++;
  return n;
}

// Atoms and total charge of a stored calculation's structure (cached per URL).
struct Comp { std::map<string, int> atoms; int charge = 0; bool ok = false; };

const Comp& compOf(const string& url)
{
  static std::map<string, Comp> cache;
  std::map<string, Comp>::iterator it = cache.find(url);
  if (it != cache.end()) return it->second;
  Comp c;
  ICalculation *calc = dynamic_cast<ICalculation*>(EDSIFactory::getResource(EcceURL(url)));
  Fragment *frag = calc ? calc->fragment() : 0;
  if (frag) {
    c.charge = frag->charge();
    vector<TAtm*> *atoms = frag->atoms();
    if (atoms) {
      for (TAtm *a : *atoms) if (a->atomicSymbol() != "X") c.atoms[a->atomicSymbol()]++;
      delete atoms;
    }
    c.ok = !c.atoms.empty();
    delete frag;
  }
  return cache.insert(std::make_pair(url, c)).first->second;
}

// "C2 H6 Cl2": carbon, hydrogen, then alphabetical (Hill order).
string hill(const std::map<string, int>& a)
{
  string o;
  auto put = [&](const string& sym, int n) {
    o += (o.empty() ? "" : " ") + sym + (n > 1 ? std::to_string(n) : string());
  };
  const bool hasC = a.count("C") > 0;
  if (hasC) put("C", a.at("C"));
  if (hasC && a.count("H")) put("H", a.at("H"));
  for (const std::pair<const string, int>& e : a) {
    if (hasC && (e.first == "C" || e.first == "H")) continue;
    put(e.first, e.second);
  }
  return o;
}

typedef std::function<const Comp*(const string&)> CompLookup;

// Empty when the reaction has the same atoms and total charge on both sides
// (coefficients applied), or when a species' structure is not known.
string balanceMessage(const vector<std::pair<int, string> >& re,
                      const vector<std::pair<int, string> >& pr, const CompLookup& find)
{
  std::map<string, int> a[2];
  int q[2] = {0, 0};
  for (int side = 0; side < 2; side++)
    for (const std::pair<int, string>& t : side == 0 ? re : pr) {
      const Comp *c = find(t.second);
      if (!c || !c->ok) return string();
      for (const std::pair<const string, int>& e : c->atoms) a[side][e.first] += t.first * e.second;
      q[side] += t.first * c->charge;
    }
  if (a[0] == a[1] && q[0] == q[1]) return string();
  auto fmt = [&](int s) {
    string f = hill(a[s]);
    if (q[0] || q[1]) f += " (charge " + std::to_string(q[s]) + ")";
    return f;
  };
  return "Not balanced: left " + fmt(0) + ", right " + fmt(1);
}

struct CalcInfo { string name, url; int imag; };

vector<CalcInfo> calcsOf(const string& base)
{
  vector<CalcInfo> out;
  if (base.empty()) return out;
  for (const std::pair<string, string>& c : calculationsIn(base)) {
    CalcInfo i;
    i.name = c.first; i.url = c.second; i.imag = imaginaryCount(c.second);
    out.push_back(i);
  }
  std::sort(out.begin(), out.end(), [](const CalcInfo& a, const CalcInfo& b) { return a.name < b.name; });
  return out;
}

// Add Reaction / Edit: reactants and products as tick lists with a coefficient
// each, the transition state as one optional choice (only calculations with
// exactly one imaginary frequency).
class PickerDialog : public wxDialog
{
public:
  PickerDialog(wxWindow *parent, const vector<CalcInfo>& calcs, const Spec& init, bool edit)
    : wxDialog(parent, wxID_ANY, edit ? _("Edit Reaction") : _("Add Reaction"), wxDefaultPosition,
               wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER), p_calcs(calcs)
  {
    wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
    wxStaticText *intro = new wxStaticText(this, wxID_ANY,
        _("Calculations in this folder. Tick reactants and products and set how many of each; "
          "the transition state is optional."));
    intro->Wrap(640);
    top->Add(intro, 0, wxALL, 10);
    wxBoxSizer *cols = new wxBoxSizer(wxHORIZONTAL);
    const wxString heads[3] = {_("Reactants"), _("Transition state"), _("Products")};
    wxScrolledWindow *pane[3];
    wxFlexGridSizer *grid[3];
    for (int c = 0; c < 3; c++) {
      wxBoxSizer *col = new wxBoxSizer(wxVERTICAL);
      wxStaticText *h = new wxStaticText(this, wxID_ANY, heads[c]);
      h->SetFont(h->GetFont().Bold());
      col->Add(h, 0, wxBOTTOM, 4);
      pane[c] = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxSize(230, 230), wxBORDER_THEME | wxVSCROLL);
      pane[c]->SetScrollRate(0, 10);
      grid[c] = new wxFlexGridSizer(c == 1 ? 1 : 2, 3, 6);
      pane[c]->SetSizer(grid[c]);
      col->Add(pane[c], 1, wxEXPAND);
      cols->Add(col, 1, wxEXPAND | wxLEFT | wxRIGHT, 8);
    }
    top->Add(cols, 1, wxEXPAND | wxLEFT | wxRIGHT, 4);

    wxRadioButton *none = new wxRadioButton(pane[1], wxID_ANY, _("(none)"), wxDefaultPosition,
                                            wxDefaultSize, wxRB_GROUP);
    grid[1]->Add(none, 0, wxALL, 2);
    p_tsRadio.push_back(none);
    none->SetValue(init.ts.empty());
    for (size_t i = 0; i < calcs.size(); i++) {
      const wxString name = wxString::FromUTF8(calcs[i].name.c_str());
      for (int side = 0; side < 2; side++) {
        const int col = side == 0 ? 0 : 2;
        wxCheckBox *cb = new wxCheckBox(pane[col], wxID_ANY, name);
        wxSpinCtrl *sp = new wxSpinCtrl(pane[col], wxID_ANY, "1", wxDefaultPosition, wxSize(64, -1),
                                        wxSP_ARROW_KEYS, 1, 99, 1);
        sp->Hide();
        grid[col]->Add(cb, 0, wxALIGN_CENTER_VERTICAL | wxALL, 2);
        grid[col]->Add(sp, 0, wxALIGN_CENTER_VERTICAL);
        const vector<std::pair<int, string> >& have = side == 0 ? init.re : init.pr;
        for (const std::pair<int, string>& t : have)
          if (t.second == calcs[i].name) { cb->SetValue(true); sp->SetValue(t.first); sp->Show(); }
        cb->Bind(wxEVT_CHECKBOX, [this, sp](wxCommandEvent& e) {
          sp->Show(e.IsChecked());
          sp->GetParent()->Layout();
          check();
        });
        sp->Bind(wxEVT_SPINCTRL, [this](wxSpinEvent&) { check(); });
        sp->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { if (p_status) check(); });
        (side == 0 ? p_reCb : p_prCb).push_back(cb);
        (side == 0 ? p_reSp : p_prSp).push_back(sp);
      }
      wxRadioButton *rb = new wxRadioButton(pane[1], wxID_ANY, name);
      const bool ok = calcs[i].imag == 1;
      if (!ok) {
        rb->Enable(false);
        rb->SetToolTip(calcs[i].imag < 0 ? wxString(_("no frequencies stored"))
                       : wxString::Format(_("%d imaginary frequencies"), calcs[i].imag));
      }
      rb->SetValue(ok && init.ts == calcs[i].name);
      grid[1]->Add(rb, 0, wxALL, 2);
      p_tsRadio.push_back(rb);
    }
    for (int c = 0; c < 3; c++) pane[c]->FitInside();

    p_status = new wxStaticText(this, wxID_ANY, " ");
    p_status->SetForegroundColour(wxColour(150, 30, 30));
    p_status->SetMinSize(wxSize(640, 2 * p_status->GetCharHeight()));
    top->Add(p_status, 0, wxLEFT | wxRIGHT | wxTOP, 10);
    wxBoxSizer *bar = new wxBoxSizer(wxHORIZONTAL);
    wxButton *help = new wxButton(this, wxID_HELP, _("Help"));
    help->Bind(wxEVT_BUTTON, [](wxCommandEvent&) { WxHelpViewer::show("energy-calculator.html"); });
    bar->Add(help, 0);
    bar->AddStretchSpacer();
    bar->Add(new wxButton(this, wxID_CANCEL, _("Cancel")), 0, wxRIGHT, 8);
    p_ok = new wxButton(this, wxID_OK, _("OK"));
    bar->Add(p_ok, 0);
    top->Add(bar, 0, wxEXPAND | wxALL, 10);
    SetSizerAndFit(top);
    check();
  }

  Spec result() const
  {
    Spec s;
    for (size_t i = 0; i < p_calcs.size(); i++) {
      if (p_reCb[i]->GetValue()) s.re.push_back(std::make_pair(p_reSp[i]->GetValue(), p_calcs[i].name));
      if (p_prCb[i]->GetValue()) s.pr.push_back(std::make_pair(p_prSp[i]->GetValue(), p_calcs[i].name));
      if (p_tsRadio[i + 1]->GetValue()) s.ts = p_calcs[i].name;
    }
    return s;
  }

private:
  void check()
  {
    bool re = false, pr = false;
    for (size_t i = 0; i < p_calcs.size(); i++) {
      re = re || p_reCb[i]->GetValue();
      pr = pr || p_prCb[i]->GetValue();
    }
    string msg;
    if (re && pr) {
      const Spec s = result();
      msg = balanceMessage(s.re, s.pr, [this](const string& n) -> const Comp* {
        for (const CalcInfo& c : p_calcs) if (c.name == n) return &compOf(c.url);
        return 0;
      });
    }
    p_ok->Enable(re && pr && msg.empty());
    p_status->SetLabel(msg.empty() ? wxString(" ") : wxString::FromUTF8(msg.c_str()));
    p_status->Wrap(640);
  }
  vector<CalcInfo> p_calcs;
  vector<wxCheckBox*> p_reCb, p_prCb;
  vector<wxSpinCtrl*> p_reSp, p_prSp;
  vector<wxRadioButton*> p_tsRadio;
  wxButton *p_ok;
  wxStaticText *p_status = 0;
};

}

// The reaction a workflow stands for: its reactant, ts and product steps
// (recipe roles), else an IRC step with the TS it starts from and that TS's
// reactant and product sources.  Empty when the folder is no workflow.
static string workflowReaction(const string& base)
{
  Session *wf = dynamic_cast<Session*>(EDSIFactory::getResource(EcceURL(base)));
  if (!wf) return string();
  Spec s;
  for (const KeyStep& k : WorkflowSteps::keySteps(wf)) {
    if (k.keyRole == "reactant") s.re.push_back(std::make_pair(1, k.name));
    else if (k.keyRole == "product") s.pr.push_back(std::make_pair(1, k.name));
    else if (k.keyRole == "ts" && s.ts.empty()) s.ts = k.name;
  }
  if (!s.re.empty() && !s.pr.empty()) return formatLine(s);
  s = Spec();
  const vector<WorkflowSteps::Node> nodes = WorkflowSteps::tree(wf);
  for (const WorkflowSteps::Node& n : nodes) {
    ICalculation *irc = dynamic_cast<ICalculation*>(n.step);
    if (!irc || irc->runtype().name().find("IRC") == string::npos || n.in.size() != 1) continue;
    Resource *ts = n.in[0].source;
    if (imaginaryCount(ts->getURL().toString()) != 1) continue;
    for (const WorkflowSteps::Node& t : nodes) {
      if (t.step != ts) continue;
      for (const WorkflowSteps::Incoming& in : t.in) {
        if (in.name == "reactant") s.re.push_back(std::make_pair(1, in.source->getName()));
        else if (in.name == "product") s.pr.push_back(std::make_pair(1, in.source->getName()));
      }
    }
    if (!s.re.empty() && !s.pr.empty()) { s.ts = ts->getName(); return formatLine(s); }
    s = Spec();
  }
  return string();
}

// ---- the formatted Details page (display only; reactionText stays the copyable text)

static string htmlEsc(const string& s)
{
  string o;
  for (char c : s) {
    if (c == '&') o += "&amp;";
    else if (c == '<') o += "&lt;";
    else if (c == '>') o += "&gt;";
    else o += c;
  }
  return o;
}

// The G treatment in words; the internal key is not shown to the user.
static string treatmentWords(const string& cli)
{
  for (const ThermoTreatmentInfo& t : thermoTreatments())
    if (cli == t.cli) {
      if (cli == "rrho") return "Rigid-rotor harmonic-oscillator (RRHO), no low-mode correction";
      if (cli.compare(0, 13, "qrrho-grimme:") == 0)
        return "Grimme quasi-RRHO, \xCF\x89\xE2\x82\x80 = " + cli.substr(13) + " cm\xE2\x81\xBB\xC2\xB9";
      if (cli == "truhlar") return "Truhlar quasi-harmonic (modes below 100 cm\xE2\x81\xBB\xC2\xB9 raised to 100 cm\xE2\x81\xBB\xC2\xB9)";
      if (cli == "head-gordon") return "Head-Gordon quasi-RRHO (enthalpy and entropy)";
      return t.label;
    }
  return cli;
}

// k with the exponent as a superscript: 1.23e-05 -> 1.23 x 10<sup>-5</sup>.
static string htmlK(const string& plain)
{
  const size_t e = plain.find_first_of("eE");
  if (e == string::npos || e == 0) return htmlEsc(plain);
  int ex = atoi(plain.c_str() + e + 1);
  return htmlEsc(plain.substr(0, e)) + " \xC3\x97 10<sup>" + std::to_string(ex) + "</sup>";
}

static string citation(const char *authors, const char *ref)
{
  return string("<li>") + authors + ", <i>" + ref + "</li>\n";
}

void ReactionOverviewDialog::showDetail()
{
  p_detailW = p_detail->GetSize().x;
  p_detail->SetPage(wxString::FromUTF8(p_detailHtml.c_str()));
  p_detail->Refresh();
}

string ReactionOverviewDialog::detailReport(const string& dialogShot)
{
  // cells: the laid-out page; px: non-white pixels of the pane as painted, read back from the screen.
  auto count = [](const wxImage& img, const wxRect& r) {
    long n = 0;
    for (int y = r.y; y < r.y + r.height && y < img.GetHeight(); y++)
      for (int x = r.x; x < r.x + r.width && x < img.GetWidth(); x++)
        if (x >= 0 && y >= 0 && (img.GetRed(x, y) < 235 || img.GetGreen(x, y) < 235 || img.GetBlue(x, y) < 235)) n++;
    return n;
  };
  wxHtmlContainerCell *cells = p_detail->GetInternalRepresentation();
  string r = "cells=" + std::to_string(cells ? cells->GetWidth() : 0) + "x" + std::to_string(cells ? cells->GetHeight() : 0) +
             " pane=" + std::to_string(p_detail->GetSize().x) + "x" + std::to_string(p_detail->GetSize().y) +
             " shown=" + (p_detail->IsShownOnScreen() ? "1" : "0");
  p_detail->Refresh();
  p_detail->Update();
  const string own = string(wxFileName::CreateTempFileName("detail").ToUTF8());
  long px = -1, pxd = -1;
  if (ecceClientShot(p_detail, wxString::FromUTF8(own.c_str()))) {
    wxImage img(wxString::FromUTF8(own.c_str()), wxBITMAP_TYPE_PNG);
    if (img.IsOk()) px = count(img, wxRect(0, 0, img.GetWidth(), img.GetHeight()));
  }
  std::remove(own.c_str());
  if (!dialogShot.empty() && dialogShot != "-") {
    wxImage img(wxString::FromUTF8(dialogShot.c_str()), wxBITMAP_TYPE_PNG);
    if (img.IsOk()) {
      const wxPoint o = p_detail->GetScreenPosition() - ClientToScreen(wxPoint(0, 0));
      pxd = count(img, wxRect(o.x + 10, o.y + 10, p_detail->GetSize().x - 20, p_detail->GetSize().y - 20));
    }
  }
  return r + " px=" + std::to_string(px) + " pxdialog=" + std::to_string(pxd);
}

string ReactionOverviewDialog::buildDetailHtml(const string& treatment, const string& tText) const
{
  const string warn = "<font color=\"#a02020\">";
  string h = "<body>\n<p><b>G treatment:</b> " + treatmentWords(treatment) + ".  <b>T</b> = " + htmlEsc(tText) +
             " K.  Energies in kJ/mol.</p>\n";
  bool anyTs = false;
  for (size_t i = 0; i < p_rows.size(); i++) {
    const Row& r = p_rows[i];
    const Spec sp = parseSpec(r.text);
    const string head = std::to_string(i + 1) + ": " + htmlEsc(cellSide(sp.re) + " \xE2\x86\x92 " + cellSide(sp.pr));
    h += "<p><font size=\"+1\"><b>Reaction " + head + "</b></font></p>\n";
    if (r.status != "ok") {
      h += "<p>" + warn + "\xE2\x9A\xA0 " + htmlEsc(r.reason) + "</font></p>\n";
      continue;
    }
    h += "<p>\xCE\x94" "E<sub>el</sub> = " + num(r.dE, "%.2f") + ",  \xCE\x94G = " + num(r.dG, "%.2f") +
         ",  spread " + num(r.spread, "%.2f") + " (\xCE\x94G from " + num(r.lo, "%.2f") + " to " +
         num(r.hi, "%.2f") + " over the six treatments)</p>\n";
    h += "<p><b>Soft modes</b> (&lt; 100 cm<sup>-1</sup>)</p>\n";
    if (r.modes.empty()) h += "<p>none</p>\n";
    else {
      h += "<table border=\"1\" cellspacing=\"0\" cellpadding=\"4\">\n<tr><th align=\"left\">Calculation</th>"
           "<th>\xCE\xBD / cm<sup>-1</sup></th><th>S / J mol<sup>-1</sup> K<sup>-1</sup></th>"
           "<th>share of S</th><th>G correction / kJ mol<sup>-1</sup></th><th>share</th></tr>\n";
      for (const Mode& m : r.modes)
        h += "<tr><td>" + htmlEsc(m.species) + "</td><td align=\"right\">" + num(m.freq, "%.1f") +
             "</td><td align=\"right\">" + num(m.S) + "</td><td align=\"right\">" + num(100 * m.Sshare, "%.1f") +
             " %</td><td align=\"right\">" + num(m.G) + "</td><td align=\"right\">" + num(100 * m.Gshare, "%.1f") +
             " %</td></tr>\n";
      h += "</table>\n";
    }
    if (!r.hasTs) continue;
    anyTs = true;
    h += "<p><b>Tunnelling</b></p>\n";
    if (!r.rateError.empty()) { h += "<p>" + warn + "no rate: " + htmlEsc(r.rateError) + "</font></p>\n"; continue; }
    const RateRow& w = r.rate.rows[0];
    const string u = r.rate.unit.empty() ? string() : htmlEsc(r.rate.unit);
    h += "<p>\xCE\x94" "E<sup>\xE2\x80\xA1</sup> = " + num(r.dEact, "%.2f") + ",  \xCE\x94G<sup>\xE2\x80\xA1</sup> = " +
         num(r.dGact, "%.2f") + " kJ/mol at " + num(w.T, "%.2f") + " K.  k in " + (u.empty() ? string("the rate unit") : u) + ".</p>\n";
    h += "<table border=\"1\" cellspacing=\"0\" cellpadding=\"4\">\n<tr><th align=\"left\">Rate</th><th>\xCE\xBA</th><th>k</th></tr>\n";
    h += "<tr><td>Transition-state theory</td><td align=\"right\">1</td><td align=\"right\">" + htmlK(rateNum(w.k)) + "</td></tr>\n";
    h += "<tr><td>Wigner</td><td align=\"right\">" + num(w.kappaWigner) + "</td><td align=\"right\">" +
         htmlK(rateNum(w.kWigner)) + "</td></tr>\n";
    const bool noE = w.kappaEckart != w.kappaEckart;
    h += "<tr><td>Eckart</td><td align=\"right\">" + (noE ? string("\xE2\x80\x94") : num(w.kappaEckart)) +
         "</td><td align=\"right\">" + (noE ? string("\xE2\x80\x94") : htmlK(rateNum(w.kEckart))) + "</td></tr>\n</table>\n";
    for (const string& note : r.rate.notes) h += "<p><i>" + htmlEsc(note) + "</i></p>\n";
  }

  h += "<p><font size=\"+1\"><b>References</b></font></p>\n<p>Methods used in the results above.</p>\n<ol>\n";
  if (anyTs) {
    h += citation("H. Eyring", "J. Chem. Phys.</i> 1935, <b>3</b>, 107\xE2\x80\x93" "115");
    h += citation("E. Wigner", "Z. Phys. Chem. B</i> 1932, <b>19</b>, 203\xE2\x80\x93" "216");
    h += citation("C. Eckart", "Phys. Rev.</i> 1930, <b>35</b>, 1303\xE2\x80\x93" "1309");
  }
  if (treatment.compare(0, 13, "qrrho-grimme:") == 0)
    h += citation("S. Grimme", "Chem. Eur. J.</i> 2012, <b>18</b>, 9955\xE2\x80\x93" "9964");
  else if (treatment == "truhlar")
    h += citation("R. F. Ribeiro, A. V. Marenich, C. J. Cramer, D. G. Truhlar",
                  "J. Phys. Chem. B</i> 2011, <b>115</b>, 14556\xE2\x80\x93" "14562");
  else if (treatment == "head-gordon")
    h += citation("Y.-P. Li, J. Gomes, S. M. Sharada, A. T. Bell, M. Head-Gordon",
                  "J. Phys. Chem. C</i> 2015, <b>119</b>, 1840\xE2\x80\x93" "1850");
  h += "</ol>\n</body>";
  return h;
}

ReactionOverviewDialog::ReactionOverviewDialog(wxWindow *parent, const string& lines, const string& base)
  : wxDialog(parent, wxID_ANY, _("Reaction overview"), wxDefaultPosition,
             wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
  p_base = base;
  // Everything but the button row scrolls: the natural size (table, results, details, chart)
  // is taller than a 768 px display, and a plain sizer squeezes the chart to a strip.
  p_body = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                wxVSCROLL | wxHSCROLL | wxTAB_TRAVERSAL);
  p_body->SetScrollRate(10, 10);
  wxScrolledWindow *body = p_body;
  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
  wxStaticText *head = new wxStaticText(body, wxID_ANY, _("Reactions"));
  head->SetFont(head->GetFont().Bold());
  top->Add(head, 0, wxLEFT | wxRIGHT | wxTOP, 8);
  p_note = new wxStaticText(body, wxID_ANY, "");
  p_note->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
  top->Add(p_note, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
  p_table = new wxPanel(body, wxID_ANY, wxDefaultPosition, wxSize(720, -1), wxBORDER_THEME);
  p_tableSizer = new wxFlexGridSizer(6, 4, 14);
  p_tableSizer->AddGrowableCol(0, 1);
  p_tableSizer->AddGrowableCol(2, 1);
  p_tableSizer->AddGrowableCol(4, 1);
  p_table->SetSizer(p_tableSizer);
  top->Add(p_table, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
  wxBoxSizer *rb = new wxBoxSizer(wxHORIZONTAL);
  wxButton *add = new wxButton(body, wxID_ANY, _("Add Reaction..."));
  add->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { pick(-1); });
  add->Enable(!calcsOf(base).empty());
  p_editButton = new wxButton(body, wxID_ANY, _("Edit..."));
  p_editButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { pick(p_sel); });
  rb->Add(add, 0, wxRIGHT, 8);
  rb->Add(p_editButton, 0);
  top->Add(rb, 0, wxALL, 8);

  bool filled = false;
  {
    std::istringstream in(lines);
    for (string l; std::getline(in, l); ) {
      l = trim(l);
      if (!l.empty() && l[0] != '#') p_lines.push_back(l);
    }
  }
  if (p_lines.empty() && !base.empty()) {
    const string auto_ = workflowReaction(base);
    if (!auto_.empty() && balanceOf(auto_).empty()) { p_lines.push_back(auto_); filled = true; }
  }
  p_note->SetLabel(filled
      ? _("Filled in from the workflow's reactant, TS and product steps. Add or change a reaction by picking calculations.")
      : _("Add or change a reaction by picking calculations."));
  p_sel = p_lines.empty() ? -1 : 0;
  rebuildTable();

  wxBoxSizer *bar = new wxBoxSizer(wxHORIZONTAL);
  bar->Add(new wxStaticText(body, wxID_ANY, _("G treatment:")), 0,
           wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
  p_treatment = new wxChoice(body, wxID_ANY);
  for (const ThermoTreatmentInfo& t : thermoTreatments())
    p_treatment->Append(wxString::FromUTF8(t.label));
  p_treatment->SetSelection(2);   // Grimme qRRHO, 100 cm-1
  p_treatment->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { update(); });
  bar->Add(p_treatment, 0, wxRIGHT, 12);
  wxButton *run = new wxButton(body, wxID_ANY, _("Update"));
  run->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { update(); });
  bar->Add(new wxStaticText(body, wxID_ANY, _("T / K:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
  p_T = new wxTextCtrl(body, wxID_ANY, "298.15", wxDefaultPosition, wxSize(80, -1),
                       wxTE_PROCESS_ENTER);
  p_T->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent&) { update(); });
  bar->Add(p_T, 0, wxRIGHT, 12);
  bar->Add(run, 0);
  top->Add(bar, 0, wxALL, 8);

  p_results = new wxPanel(body, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_THEME);
  p_resSizer = new wxFlexGridSizer(9, 4, 16);
  p_resSizer->AddGrowableCol(1, 1);
  p_results->SetSizer(p_resSizer);
  top->Add(p_results, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
  p_units = new wxStaticText(body, wxID_ANY, "");
  p_units->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
  top->Add(p_units, 0, wxALL, 8);

  p_pane = new wxCollapsiblePane(body, wxID_ANY, _("Details: soft modes and tunnelling"));
  p_detail = new wxHtmlWindow(p_pane->GetPane(), wxID_ANY, wxDefaultPosition, wxSize(720, 180),
                              wxHW_SCROLLBAR_AUTO | wxBORDER_SUNKEN);
  p_detail->SetMinSize(wxSize(300, 150));
  p_detail->SetBorders(8);
  p_detail->SetBackgroundColour(*wxWHITE);
  // The page is laid out for the width it had when set; lay it out again when the pane gets its real size.
  p_detail->Bind(wxEVT_SIZE, [this](wxSizeEvent& e) {
    e.Skip();
    if (e.GetSize().x > 50 && e.GetSize().x != p_detailW) CallAfter([this]() { showDetail(); });
  });
  wxBoxSizer *ps = new wxBoxSizer(wxVERTICAL);
  ps->Add(p_detail, 1, wxEXPAND);
  p_pane->GetPane()->SetSizer(ps);
  p_pane->Expand();
  p_pane->Bind(wxEVT_COLLAPSIBLEPANE_CHANGED, [this](wxCollapsiblePaneEvent&) {
    p_body->Layout();
    p_body->FitInside();
  });
  top->Add(p_pane, 1, wxEXPAND | wxALL, 8);

  wxBoxSizer *pick = new wxBoxSizer(wxHORIZONTAL);
  p_profileLabel = new wxStaticText(body, wxID_ANY, _("Energy profile of:"));
  pick->Add(p_profileLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
  p_profileChoice = new wxChoice(body, wxID_ANY);
  p_profileChoice->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) {
    const int i = p_profileChoice->GetSelection();
    if (i >= 0 && i < (int)p_profiles.size()) p_plot->setProfile(p_profiles[i]);
  });
  pick->Add(p_profileChoice, 1);
  top->Add(pick, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
  p_plot = new EnergyProfileCanvas(body);
  p_plot->SetMinSize(wxSize(720, 380));
  p_plot->SetToolTip(_("Right-click to copy the numbers"));
  top->Add(p_plot, 2, wxEXPAND | wxLEFT | wxRIGHT, 8);

  wxBoxSizer *buttons = new wxBoxSizer(wxHORIZONTAL);
  wxButton *csv = new wxButton(this, wxID_ANY, _("Export CSV..."));
  csv->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    wxFileDialog d(this, _("Export chart data"), "", "reactions.csv", "*.csv",
                   wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (d.ShowModal() == wxID_OK) saveCsv(string(d.GetPath().ToUTF8()));
  });
  wxButton *png = new wxButton(this, wxID_ANY, _("Export PNG..."));
  png->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    wxFileDialog d(this, _("Export energy profiles"), "", "reactions.png", "*.png",
                   wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (d.ShowModal() == wxID_OK) savePng(string(d.GetPath().ToUTF8()));
  });
  buttons->Add(csv, 0, wxRIGHT, 8);
  buttons->Add(png, 0);
  p_pngButton = png;
  buttons->AddStretchSpacer();
  buttons->Add(new wxButton(this, wxID_CLOSE, _("Close")));
  body->SetSizer(top);
  wxBoxSizer *outer = new wxBoxSizer(wxVERTICAL);
  outer->Add(body, 1, wxEXPAND);
  outer->Add(buttons, 0, wxEXPAND | wxALL, 8);
  // The scroller's own minimum is small; the dialog's size comes from fitContent().
  body->SetMinSize(wxSize(320, 200));
  SetSizer(outer);
  update();
  SetEscapeId(wxID_CLOSE);
  Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    if (IsModal()) EndModal(wxID_CLOSE); else Hide();
  }, wxID_CLOSE);
}

void ReactionOverviewDialog::rebuildTable()
{
  p_table->DestroyChildren();
  p_tableSizer->Clear(false);
  auto cell = [this](const wxString& t, bool bold) {
    wxStaticText *w = new wxStaticText(p_table, wxID_ANY, t);
    if (bold) w->SetFont(w->GetFont().Bold());
    return w;
  };
  const wxString heads[6] = {_("Reactants"), "", _("Transition state"), "", _("Products"), ""};
  for (int c = 0; c < 6; c++) p_tableSizer->Add(cell(heads[c], true), 0, wxALL, 4);
  for (size_t i = 0; i < p_lines.size(); i++) {
    const Spec s = parseSpec(p_lines[i]);
    const string unbal = balanceOf(p_lines[i]);
    const wxColour bg = (int)i == p_sel ? wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT).ChangeLightness(170)
                                        : p_table->GetBackgroundColour();
    wxWindow *w[6] = {
      cell(wxString::FromUTF8(((unbal.empty() ? "" : "\xE2\x9A\xA0 ") + cellSide(s.re)).c_str()), false), cell(wxString::FromUTF8("\xE2\x86\x92"), false),
      cell(s.ts.empty() ? wxString::FromUTF8("\xE2\x80\x94") : wxString::FromUTF8(s.ts.c_str()), false),
      cell(wxString::FromUTF8("\xE2\x86\x92"), false),
      cell(wxString::FromUTF8(cellSide(s.pr).c_str()), false), 0};
    wxButton *x = new wxButton(p_table, wxID_ANY, wxString::FromUTF8("\xC3\x97"), wxDefaultPosition,
                               wxDefaultSize, wxBU_EXACTFIT);
    x->SetToolTip(_("Remove this reaction"));
    const int row = (int)i;
    x->Bind(wxEVT_BUTTON, [this, row](wxCommandEvent&) { CallAfter([this, row]() { removeReaction(row + 1); }); });
    w[5] = x;
    if (!unbal.empty()) w[0]->SetToolTip(wxString::FromUTF8(unbal.c_str()));
    for (int c = 0; c < 6; c++) {
      if (c < 5) {
        w[c]->SetBackgroundColour(bg);
        w[c]->Bind(wxEVT_LEFT_DOWN, [this, row](wxMouseEvent&) { p_sel = row; rebuildTable(); });
      }
      p_tableSizer->Add(w[c], 0, wxALL | wxEXPAND, 4);
    }
  }
  p_editButton->Enable(p_sel >= 0 && p_sel < (int)p_lines.size() && !calcsOf(p_base).empty());
  p_table->Layout();
  Layout();
}

string ReactionOverviewDialog::balanceOf(const string& line) const
{
  if (p_base.empty()) return string();
  const Spec s = parseSpec(line);
  if (s.re.empty() || s.pr.empty()) return string();
  if (p_calcUrls.empty())
    for (const std::pair<string, string>& c : calculationsIn(p_base)) p_calcUrls[c.first] = c.second;
  return balanceMessage(s.re, s.pr, [this](const string& n) -> const Comp* {
    std::map<string, string>::const_iterator it = p_calcUrls.find(n);
    return it == p_calcUrls.end() ? 0 : &compOf(it->second);
  });
}

string ReactionOverviewDialog::balanceText() const
{
  string o;
  for (size_t i = 0; i < p_lines.size(); i++) {
    const string m = balanceOf(p_lines[i]);
    o += std::to_string(i + 1) + " " + (m.empty() ? string("balanced") : m) + "\n";
  }
  return o;
}

void ReactionOverviewDialog::pick(int row)
{
  const vector<CalcInfo> calcs = calcsOf(p_base);
  if (calcs.empty()) return;
  const bool edit = row >= 0 && row < (int)p_lines.size();
  PickerDialog d(this, calcs, edit ? parseSpec(p_lines[row]) : Spec(), edit);
  if (d.ShowModal() != wxID_OK) return;
  const string line = formatLine(d.result());
  if (edit) p_lines[row] = line;
  else { p_lines.push_back(line); p_sel = (int)p_lines.size() - 1; }
  rebuildTable();
  update();
}

string ReactionOverviewDialog::reactionLines() const
{
  string o;
  for (const string& l : p_lines) o += l + "\n";
  return o;
}

void ReactionOverviewDialog::addReactionLine(const string& line)
{
  // Store the canonical spelling, as the picker does, whatever spacing the hook got.
  const Spec s = parseSpec(line);
  p_lines.push_back(s.re.empty() || s.pr.empty() ? line : formatLine(s));
  p_sel = (int)p_lines.size() - 1;
  rebuildTable();
  update();
}

void ReactionOverviewDialog::removeReaction(int n)
{
  if (n < 1 || n > (int)p_lines.size()) return;
  p_lines.erase(p_lines.begin() + (n - 1));
  p_sel = p_lines.empty() ? -1 : std::min(p_sel, (int)p_lines.size() - 1);
  rebuildTable();
  update();
}

string ReactionOverviewDialog::tsChoices() const
{
  string o;
  for (const CalcInfo& c : calcsOf(p_base))
    o += c.name + ":" + std::to_string(c.imag) + ":" + (c.imag == 1 ? "1" : "0") + "\n";
  return o;
}

void ReactionOverviewDialog::setTreatment(const string& cli)
{
  const vector<ThermoTreatmentInfo>& t = thermoTreatments();
  for (size_t i = 0; i < t.size(); i++)
    if (cli == t[i].cli) p_treatment->SetSelection((int)i);
  update();
}

void ReactionOverviewDialog::setTemperature(const string& kelvin)
{
  p_T->SetValue(wxString::FromUTF8(kelvin.c_str()));
  update();
}

void ReactionOverviewDialog::update()
{
  p_rows.clear();
  p_profiles.clear();
  p_profileRow.clear();
  p_csv.clear();
  p_anyTs = false;
  const int sel = p_treatment->GetSelection();
  const string treatment = thermoTreatments()[sel < 0 ? 2 : sel].cli;
  double kelvin = atof(string(p_T->GetValue().ToUTF8()).c_str());
  if (kelvin <= 0) kelvin = 298.15;
  const string tText = num(kelvin, "%.2f");

  // One entry per line.  A TS line goes to ecce-thermo twice: 'a -> c' for the
  // reaction and 'a -> ts' for the barrier, so both come from its numbers.
  struct Entry { string user, main, aux; int mainIdx = -1, auxIdx = -1; string refused; vector<string> re, ts, pr; string names[3], title; };
  vector<Entry> entries;
  string joined;
  for (const string& l : p_lines) joined += l + "\n";
  std::istringstream lines(joined);
  string thermoFile;
  int nth = 0;
  for (string raw; std::getline(lines, raw); ) {
    const string l = trim(raw);
    if (l.empty() || l[0] == '#') continue;
    Entry e;
    e.user = l;
    string nameErr;
    const string full = resolveNames(l, p_base, nameErr);
    if (!nameErr.empty()) {
      e.refused = nameErr;
    } else if (full.find('[') == string::npos) {
      e.main = full;
    } else {
      string lhs, ts, rhs;
      if (!splitTs(full, lhs, ts, rhs)) {
        e.refused = "cannot parse; the form is 'a, 2*b -> [ts] -> c'";
      } else {
        string why;
        e.re = sidePaths(lhs, why);
        e.ts = sidePaths(ts, why);
        e.pr = sidePaths(rhs, why);
        if (!why.empty()) e.refused = why;
        else if (e.ts.size() != 1) e.refused = "name one transition state in [ ]";
        e.main = lhs + " -> " + rhs;
        e.aux = lhs + " -> " + ts;
        e.names[0] = shortSide(lhs, " + ");
        e.names[1] = shortSide(ts, " + ");
        e.names[2] = shortSide(rhs, " + ");
        e.title = shortSide(lhs, ", ") + " -> [" + e.names[1] + "] -> " + shortSide(rhs, ", ");
      }
    }
    if (e.refused.empty()) e.refused = balanceOf(l);
    if (e.refused.empty()) {
      e.mainIdx = nth++;
      thermoFile += e.main + "\n";
      if (!e.aux.empty()) { e.auxIdx = nth++; thermoFile += e.aux + "\n"; }
    }
    entries.push_back(e);
  }
  const string tmp = string(wxFileName::CreateTempFileName("reactions").ToUTF8());
  { std::ofstream(tmp.c_str()) << thermoFile; }
  string csv, err;
  const bool ok = thermoFile.empty() ? true : runThermoReactions(tmp, treatment, csv, err, kelvin);
  std::remove(tmp.c_str());
  if (!ok) {
    p_error = err;
    p_detailHtml = "<body><font color=\"#a02020\">" + htmlEsc(err) + "</font></body>";
    showDetail();
    showResults();
    showProfiles();
    return;
  }
  p_error.clear();

  vector<Row> thermo;
  vector<string> rawLines;
  string header;
  {
    std::istringstream in(csv);
    string line;
    std::getline(in, header);
    bool modes = false;
    while (std::getline(in, line)) {
      if (line.empty()) { modes = true; std::getline(in, line); continue; }
      const vector<string> f = csvFields(line);
      if (!modes && f.size() >= 8) {
        Row r;
        r.text = f[0]; r.status = f[1]; r.reason = f[7];
        if (r.status == "ok") {
          r.dE = atof(f[2].c_str()); r.dG = atof(f[3].c_str()); r.spread = atof(f[4].c_str());
          r.lo = atof(f[5].c_str()); r.hi = atof(f[6].c_str());
        }
        thermo.push_back(r);
        rawLines.push_back(line);
      } else if (modes && f.size() >= 7) {
        for (Row& r : thermo)
          if (r.text == f[0] && r.status == "ok") {
            r.modes.push_back({f[1], atof(f[2].c_str()), atof(f[3].c_str()), atof(f[4].c_str()),
                               atof(f[5].c_str()), atof(f[6].c_str())});
            break;
          }
      }
    }
  }
  for (const Entry& e : entries) {
    Row r;
    if (e.mainIdx >= 0 && e.mainIdx < (int)thermo.size()) r = thermo[e.mainIdx];
    else { r.status = "refused"; r.reason = e.refused.empty() ? "no result" : e.refused; }
    r.thermoText = r.text;
    r.text = e.user;
    if (e.mainIdx >= 0 && e.mainIdx < (int)rawLines.size()) r.rawCsv = rawLines[e.mainIdx];
    if (!e.aux.empty() && r.status == "ok") {
      p_anyTs = true;
      r.hasTs = true;
      const Row *a = e.auxIdx < (int)thermo.size() ? &thermo[e.auxIdx] : 0;
      if (!a || a->status != "ok") {
        r.rateError = "barrier: " + (a ? a->reason : string("no result"));
      } else {
        r.dEact = a->dE;
        r.dGact = a->dG;
        RateRequest req;
        req.reactants = e.re; req.ts = e.ts[0]; req.products = e.pr;
        req.temperatures = tText;
        req.treatment = treatment;
        r.rate = runRate(req);
        if (!r.rate.ok) r.rateError = r.rate.error;
        EnergyProfileData pd;
        pd.title = e.title;
        for (int k = 0; k < 3; k++) pd.names[k] = e.names[k];
        pd.E[1] = r.dEact; pd.G[1] = r.dGact;
        pd.E[2] = r.dE; pd.G[2] = r.dG;
        pd.kelvin = kelvin;
        if (r.rate.ok && !r.rate.rows.empty()) {
          const RateRow& w = r.rate.rows[0];
          const string u = r.rate.unit.empty() ? string() : " " + r.rate.unit;
          pd.rateLines.push_back("k (TST) = " + rateNum(w.k) + u);
          pd.rateLines.push_back("k (Eckart) = " + rateNum(w.kEckart) + u);
        } else {
          pd.rateLines.push_back("k: not available (see the text below)");
        }
        p_profiles.push_back(pd);
        p_profileRow.push_back((int)p_rows.size());
      }
    }
    p_rows.push_back(r);
  }
  // The exported table: ecce-thermo's rows, as before, plus rate columns when a reaction has a TS.
  string table = header;
  if (p_anyTs)
    table += ",dE_act_kJ_mol,dG_act_kJ_mol,T_K,kappa_wigner,kappa_eckart,k_tst,k_wigner,k_eckart,k_unit";
  for (const Row& r : p_rows) {
    string line = r.rawCsv;
    if (line.empty()) line = csvq(r.text) + ",refused,,,,,," + csvq(r.reason);
    else if (r.text != r.thermoText) line = csvq(r.text) + line.substr(csvq(r.thermoText).size());
    if (p_anyTs) {
      if (r.hasTs && r.rate.ok) {
        const RateRow& w = r.rate.rows[0];
        line += "," + num(r.dEact) + "," + num(r.dGact) + "," + num(w.T, "%.2f") + "," +
                num(w.kappaWigner) + "," + (w.kappaEckart != w.kappaEckart ? string() : num(w.kappaEckart)) + "," +
                num(w.k, "%.4e") + "," + num(w.kWigner, "%.4e") + "," +
                (w.kEckart != w.kEckart ? string() : num(w.kEckart, "%.4e")) + "," + csvq(r.rate.unit);
      } else if (r.hasTs) {
        line += "," + num(r.dEact) + "," + num(r.dGact) + "," + num(kelvin, "%.2f") + ",,,,,," + csvq(r.rateError);
      } else line += ",,,,,,,,,";
    }
    table += "\n" + line;
  }
  p_csv = table;

  p_detailHtml = buildDetailHtml(treatment, tText);
  showDetail();
  showResults();
  showProfiles();
}

// The numbers of every reaction in one table: kJ/mol, and k in the rate unit.
void ReactionOverviewDialog::showResults()
{
  p_results->DestroyChildren();
  p_resSizer->Clear(false);
  auto cell = [this](const string& t, bool bold, bool right) {
    wxStaticText *w = new wxStaticText(p_results, wxID_ANY, wxString::FromUTF8(t.c_str()),
                                       wxDefaultPosition, wxDefaultSize,
                                       right ? wxALIGN_RIGHT : wxALIGN_LEFT);
    if (bold) w->SetFont(w->GetFont().Bold());
    p_resSizer->Add(w, 0, wxALL | (right ? wxALIGN_RIGHT : 0), 4);
    return w;
  };
  auto kj = [](double v) {   // typographic minus
    string o = num(v, "%.2f");
    return o[0] == '-' ? string("\xE2\x88\x92") + o.substr(1) : o;
  };
  const string dash = "\xE2\x80\x94";
  // Units in the headers: the note under the table was easy to miss.
  string unit = "s-1";
  for (const Row& r : p_rows)
    if (r.status == "ok" && r.hasTs && r.rate.ok && !r.rate.rows.empty() &&
        !r.rate.unit.empty()) { unit = r.rate.unit; break; }
  const string kU = unit == "s-1" ? string("s\xE2\x81\xBB\xC2\xB9") : unit;
  const string eU = " / kJ mol\xE2\x81\xBB\xC2\xB9";
  const string heads[9] = {"#", "Reaction", "\xCE\x94" "E" + eU, "\xCE\x94" "G" + eU, "Spread" + eU,
                          "\xCE\x94" "E\xE2\x80\xA1" + eU, "\xCE\x94" "G\xE2\x80\xA1" + eU,
                          "k (TST) / " + kU, "k (Eckart) / " + kU};
  for (int c = 0; c < 9; c++) cell(heads[c], true, c != 1);
  for (size_t i = 0; i < p_rows.size(); i++) {
    const Row& r = p_rows[i];
    const Spec sp = parseSpec(r.text);
    const bool unbal = r.status != "ok" && r.reason.compare(0, 12, "Not balanced") == 0;
    cell(std::to_string(i + 1), false, true);
    wxStaticText *name = cell((unbal ? "\xE2\x9A\xA0 " : "") + cellSide(sp.re) + " \xE2\x86\x92 " + cellSide(sp.pr), false, false);
    if (r.status != "ok") {
      name->SetToolTip(wxString::FromUTF8(r.reason.c_str()));
      cell(unbal ? "not balanced" : "refused", false, true);
      for (int c = 3; c < 9; c++) cell(dash, false, true);
      continue;
    }
    cell(kj(r.dE), false, true);
    cell(kj(r.dG), false, true);
    cell(num(r.spread, "%.2f"), false, true);
    const bool rate = r.hasTs && r.rate.ok && !r.rate.rows.empty();
    cell(rate ? kj(r.dEact) : dash, false, true);
    cell(rate ? kj(r.dGact) : dash, false, true);
    cell(rate ? rateNum(r.rate.rows[0].k) : dash, false, true);
    const double ke = rate ? r.rate.rows[0].kEckart : 0;
    cell(rate && ke == ke ? rateNum(ke) : dash, false, true);
  }
  p_units->SetLabel(wxString::FromUTF8(("k at the T above. Spread: how much \xCE\x94G changes with the "
      "G treatment (largest minus smallest over the six treatments in the G treatment list).")));
  p_results->Layout();
}

// Only reactions with a [ts] get a chart; without any, the text alone.
void ReactionOverviewDialog::showProfiles()
{
  const bool any = !p_profiles.empty();
  p_profileChoice->Clear();
  for (size_t i = 0; i < p_profiles.size(); i++)
    p_profileChoice->Append(wxString::FromUTF8(
        (std::to_string(p_profileRow[i] + 1) + ": " + p_profiles[i].title).c_str()));
  if (any) {
    p_profileChoice->SetSelection(0);
    p_plot->setProfile(p_profiles[0]);
  }
  p_profileLabel->Show(p_profiles.size() > 1);
  p_profileChoice->Show(p_profiles.size() > 1);
  p_plot->Show(any);
  p_pngButton->Enable(any);
  fitContent(any);
}

// Size the dialog to the content, capped to the display; what does not fit scrolls.
// A chart shown after the first fit makes the dialog grow rather than squeeze to a strip.
void ReactionOverviewDialog::fitContent(bool wantChart)
{
  if (!p_body->GetSizer()) return;
  p_body->Layout();
  const wxSize need = p_body->GetSizer()->GetMinSize();
  const wxSize foot = GetSizer()->GetMinSize() - p_body->GetMinSize();   // the button row
  wxSize want = GetClientSize();
  want.x = std::max(want.x, need.x + 16);
  want.y = std::max(want.y, need.y + foot.y);
  if (wantChart) {
    want.x = std::max(want.x, 900);
    want.y = std::max(want.y, 800);
  }
  const int d = wxDisplay::GetFromWindow(this);
  const wxRect area = wxDisplay(d < 0 ? 0 : d).GetClientArea();
  want.x = std::min(want.x, area.width - 80);
  want.y = std::min(want.y, area.height - 120);
  if (want != GetClientSize()) SetClientSize(want);
  Layout();
  p_body->FitInside();
}

string ReactionOverviewDialog::rateLines(int n) const
{
  const Row& r = p_rows[n - 1];
  if (!r.hasTs) return string();
  const string head = std::to_string(n) + ": ";
  if (!r.rateError.empty()) return head + "no rate: " + r.rateError + "\n";
  const RateRow& w = r.rate.rows[0];
  const string u = r.rate.unit.empty() ? string() : " " + r.rate.unit;
  string o = head + "dE\xE2\x80\xA1 " + num(r.dEact, "%.2f") + "  dG\xE2\x80\xA1 " + num(r.dGact, "%.2f") +
             " kJ/mol  T " + num(w.T, "%.2f") + " K\n";
  o += "   k TST " + rateNum(w.k) + u + "\n";
  o += "   kappa Wigner " + num(w.kappaWigner) + "  k " + rateNum(w.kWigner) + u + "\n";
  o += "   kappa Eckart " + (w.kappaEckart != w.kappaEckart ? string("-") : num(w.kappaEckart)) +
       "  k " + rateNum(w.kEckart) + u + "\n";
  for (const string& note : r.rate.notes) o += "   note: " + note + "\n";
  return o;
}

string ReactionOverviewDialog::rateText() const
{
  string o;
  for (size_t i = 0; i < p_rows.size(); i++)
    if (p_rows[i].hasTs) o += rateLines((int)i + 1);
  return o;
}

string ReactionOverviewDialog::tableText() const
{
  string out = p_error.empty() ? string() : "error: " + p_error + "\n";
  for (size_t i = 0; i < p_rows.size(); i++) {
    const Row& r = p_rows[i];
    if (r.status != "ok") continue;
    out += "reaction " + std::to_string(i + 1) + " " + num(r.dE) + " " + num(r.dG) + " " +
           num(r.lo) + " " + num(r.hi) + "\n";
  }
  return out;
}

string ReactionOverviewDialog::profileText() const
{
  string out;
  const char *lv[3] = {"reactant", "ts", "product"};
  for (size_t p = 0; p < p_profiles.size(); p++)
    for (int k = 0; k < 3; k++)
      out += "profile " + std::to_string(p + 1) + " " + std::to_string(p_profileRow[p] + 1) + " " +
             lv[k] + " " + num(p_profiles[p].E[k]) + " " + num(p_profiles[p].G[k]) + "\n";
  return out;
}

string ReactionOverviewDialog::layoutText() const
{
  string out;
  for (size_t p = 0; p < p_profiles.size(); p++) {
    const EnergyProfileLayout l = exportLayout(p_profiles[p]);
    std::istringstream in(l.dump());
    for (string line; std::getline(in, line); )
      out += "layout " + std::to_string(p + 1) + " " + line + "\n";
    if (!l.placed) out += "layout " + std::to_string(p + 1) + " unplaced\n";
  }
  return out;
}

string ReactionOverviewDialog::reactionText(int n) const
{
  string d;
  if (n < 1 || n > (int)p_rows.size()) return d;
  const Row& r = p_rows[n - 1];
  d += std::to_string(n) + ": " + r.text + "\n";
  if (r.status != "ok") return d + "refused: " + r.reason + "\n";
  d += "dE_el " + num(r.dE, "%.2f") + "   dG " + num(r.dG, "%.2f") + "   spread " +
       num(r.spread, "%.2f") + " kJ/mol (dG over the treatments " + num(r.lo, "%.2f") + " to " +
       num(r.hi, "%.2f") + ")\n";
  d += "Soft modes (< 100 cm-1), share of the species' S and of its G correction:\n";
  if (r.modes.empty()) d += "  none\n";
  for (const Mode& m : r.modes)
    d += "  " + m.species + "  " + num(m.freq, "%.1f") + " cm-1  S " + num(m.S) + " J/mol/K (" +
         num(100 * m.Sshare, "%.1f") + "%)  G " + num(m.G) + " kJ/mol (" +
         num(100 * m.Gshare, "%.1f") + "%)\n";
  for (size_t p = 0; p < p_profiles.size(); p++)
    if (p_profileRow[p] == n - 1) {
      std::istringstream in(p_profiles[p].text());
      string line;
      std::getline(in, line);   // the title repeats the reaction
      d += "Energy profile:\n";
      while (std::getline(in, line))
        if (line.compare(0, 2, "k ") && line.compare(0, 2, "k:")) d += "  " + line + "\n";
    }
  d += rateLines(n);
  return d;
}

string ReactionOverviewDialog::clickText(int n)
{
  return reactionText(n);
}

bool ReactionOverviewDialog::savePng(const string& file) const
{
  return saveEnergyProfilesPng(p_profiles, file);
}

bool ReactionOverviewDialog::saveCsv(const string& file) const
{
  std::ofstream f(file.c_str());
  if (!f) return false;
  f << p_csv << "\n";
  return (bool)f;
}

string ReactionOverviewDialog::layoutReport(const string& shot)
{
  auto sz = [](wxWindow *w) { return std::to_string(w->GetSize().x) + "x" + std::to_string(w->GetSize().y); };
  string r = "dialog=" + sz(this) + " plot=" + sz(p_plot) + " plotshown=" + (p_plot->IsShownOnScreen() ? "1" : "0") +
             " choice=" + sz(p_profileChoice) + " choiceshown=" + (p_profileChoice->IsShownOnScreen() ? "1" : "0") +
             " treatment=" + sz(p_treatment) + " treatmentenabled=" + (p_treatment->IsEnabled() ? "1" : "0") +
             " detailenabled=" + (p_detail->IsEnabled() ? "1" : "0") + " detail=" + sz(p_detail) +
             " body=" + sz(p_body) + " virtual=" + std::to_string(p_body->GetVirtualSize().x) + "x" +
             std::to_string(p_body->GetVirtualSize().y) + " screen=" +
             std::to_string(wxDisplay(std::max(0, wxDisplay::GetFromWindow(this))).GetClientArea().height);
  if (!shot.empty() && shot != "-") r += ecceWindowShot(this, wxString::FromUTF8(shot.c_str())) ? " shot=saved" : " shot=failed";
  return r;
}
