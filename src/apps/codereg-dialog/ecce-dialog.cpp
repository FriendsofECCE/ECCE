// ecce-dialog: a data-driven C++ replacement for the wxPython code-
// registration "Details" dialogs (scripts/codereg/*.py).  It takes the same
// argv, reads the same restore file and sends the same "name|value|unit|
// enabled|export|kind" lines over UDP, so the Calculation Editor cannot tell
// it from the Python dialog.  The dialog itself is a .dlg file next to the
// Python one (see scripts/codereg/ecceqmtheory.dlg).  Pilot: only wx is
// linked, nothing from ECCE, so the Windows client needs no Python for it.

#include <wx/wx.h>
#include <wx/socket.h>
#include <wx/spinctrl.h>
#include <wx/statline.h>
#include <wx/display.h>
#include <wx/tokenzr.h>
#include <wx/utils.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#ifndef __WXMSW__
#include <unistd.h>
#endif

using std::string;
using std::vector;

namespace {

string trim(const string& s)
{
  size_t a = s.find_first_not_of(" \t\r\n");
  if (a == string::npos) return "";
  return s.substr(a, s.find_last_not_of(" \t\r\n") - a + 1);
}

// Everything the editor passes on the command line, by the name the .dlg
// expressions use.
std::map<string, string> g_vars;
string g_restoreFile, g_restoreFlag, g_readOnlyFlag;
long g_port = 0;
bool g_debug = false;
wxDatagramSocket *g_sock = 0;
wxIPV4address *g_peer = 0;
bool g_loopStarted = false;
vector<string> g_batch;

void debug(const string& m)
{
  if (g_debug) fprintf(stderr, "[ecce-dialog] %s\n", m.c_str());
}

void sendRaw(const string& s)
{
  if (g_sock) g_sock->SendTo(*g_peer, s.data(), s.size());
}

bool numeric(const string& s, double& v)
{
  if (s.empty()) return false;
  char *end = 0;
  v = strtod(s.c_str(), &end);
  return end && *end == 0;
}

struct Ctl;
vector<Ctl*> g_ctls;
string valueOf(const string& name);

// "a==b && c>1 || !d": comparison of a variable (argv value or another
// control's value) with a literal; numbers compare as numbers.
bool evalTerm(string t)
{
  t = trim(t);
  bool neg = false;
  if (!t.empty() && t[0] == '!') { neg = true; t = trim(t.substr(1)); }
  static const char *ops[] = {"==", "!=", "<=", ">=", "<", ">"};
  bool r = false, found = false;
  for (const char *op : ops) {
    size_t p = t.find(op);
    if (p == string::npos) continue;
    string l = valueOf(trim(t.substr(0, p))), rr = trim(t.substr(p + strlen(op)));
    double a, b;
    int c;
    if (numeric(l, a) && numeric(rr, b)) c = a < b ? -1 : a > b ? 1 : 0;
    else c = l.compare(rr);
    string o = op;
    r = o == "==" ? c == 0 : o == "!=" ? c != 0 : o == "<=" ? c <= 0 :
        o == ">=" ? c >= 0 : o == "<" ? c < 0 : c > 0;
    found = true;
    break;
  }
  if (!found) {
    string v = valueOf(t);
    double a;
    r = numeric(v, a) ? a != 0 : !v.empty();
  }
  return neg ? !r : r;
}

bool evalExpr(const string& e)
{
  if (trim(e).empty()) return true;
  size_t from = 0;
  bool any = false;
  while (from <= e.size()) {
    size_t q = e.find("||", from);
    string orPart = e.substr(from, q == string::npos ? string::npos : q - from);
    bool all = true;
    size_t f2 = 0;
    while (f2 <= orPart.size()) {
      size_t a = orPart.find("&&", f2);
      all = evalTerm(orPart.substr(f2, a == string::npos ? string::npos
                                                          : a - f2)) && all;
      if (a == string::npos) break;
      f2 = a + 2;
    }
    any = any || all;
    if (q == string::npos) break;
    from = q + 2;
  }
  return any;
}

struct Ctl {
  string kind, name, label, unit, def, enableIf, showIf;
  vector<string> choices;
  int exportFlag = 0;
  double mn = -1e100, mx = 1e100;
  wxWindow *w = 0, *w2 = 0;       // w2: the second radio of a radiopair
  wxStaticText *lab = 0;
  string saved;

  string get() const
  {
    if (kind == "check") return ((wxCheckBox*)w)->GetValue() ? "1" : "0";
    if (kind == "radiopair") return ((wxRadioButton*)w2)->GetValue() ? "1" : "0";
    if (kind == "combo") return ((wxChoice*)w)->GetStringSelection().ToStdString();
    if (kind == "int") return std::to_string(((wxSpinCtrl*)w)->GetValue());
    if (kind == "note") return "";
    return ((wxTextCtrl*)w)->GetValue().ToStdString();
  }
  void set(const string& v)
  {
    if (kind == "check") ((wxCheckBox*)w)->SetValue(v == "1");
    else if (kind == "radiopair") {
      ((wxRadioButton*)w2)->SetValue(v == "1");
      ((wxRadioButton*)w)->SetValue(v != "1");
    } else if (kind == "combo") ((wxChoice*)w)->SetStringSelection(v);
    else if (kind == "int") ((wxSpinCtrl*)w)->SetValue(atoi(v.c_str()));
    else if (kind != "note") ((wxTextCtrl*)w)->SetValue(v);
  }
  bool notDefault() const
  {
    double a, b;
    string v = get();
    if ((kind == "float" || kind == "int") && numeric(v, a) && numeric(def, b))
      return a != b;
    return v != def;
  }
  string typeName() const
  {
    return kind == "check" || kind == "radiopair" ? "toggle_input" :
           kind == "combo" ? "menu_input" : kind == "int" ? "integer_input" :
           kind == "float" ? "float_input" : "text_input";
  }
  // Same line the Python widget's ExportSetting() builds; a radiopair always
  // exports, as the Python radio does.
  string exportLine() const
  {
    int need = kind == "radiopair" ? 1 : exportFlag;
    if (notDefault()) need = 1;
    string u = kind == "int" || kind == "float" ? unit : "";
    return name + "|" + get() + "|" + u + "|" + (w->IsEnabled() ? "1" : "0") +
           "|" + std::to_string(need) + "|" + typeName();
  }
  bool sendsOnEnable() const
  {
    return kind == "combo" || kind == "int" || kind == "float";
  }
};

string valueOf(const string& name)
{
  auto it = g_vars.find(name);
  if (it != g_vars.end()) return it->second;
  for (Ctl *c : g_ctls)
    if (c->name == name) return c->get();
  return "";
}

// One line of a .dlg: a command word, then key=value pairs (values may be
// "quoted").
struct Line {
  string cmd;
  std::map<string, string> kv;
  string get(const string& k, const string& d = "") const
  {
    auto it = kv.find(k);
    return it == kv.end() ? d : it->second;
  }
};

bool parseLine(const string& raw, Line& out)
{
  string s = raw;
  bool inq = false;
  for (size_t i = 0; i < s.size(); i++) {      // strip a trailing # comment
    if (s[i] == '"') inq = !inq;
    if (s[i] == '#' && !inq) { s.erase(i); break; }
  }
  s = trim(s);
  if (s.empty()) return false;
  vector<string> tok;
  string cur;
  inq = false;
  for (char c : s) {
    if (c == '"') { inq = !inq; cur += c; continue; }
    if ((c == ' ' || c == '\t') && !inq) {
      if (!cur.empty()) tok.push_back(cur);
      cur.clear();
    } else cur += c;
  }
  if (!cur.empty()) tok.push_back(cur);
  out = Line();
  out.cmd = tok[0];
  for (size_t i = 1; i < tok.size(); i++) {
    size_t eq = tok[i].find('=');
    if (eq == string::npos) continue;
    string v = tok[i].substr(eq + 1);
    v.erase(std::remove(v.begin(), v.end(), '"'), v.end());
    out.kv[tok[i].substr(0, eq)] = v;
  }
  return true;
}

wxFont themeFont()
{
  // The Python dialogs' rule: 10 pt (or the system size) moved by the
  // preference step, read from the same EcceGlobal file.
  int base = 10, fs = 1;
  wxString home;
  if (!wxGetEnv("ECCE_REALUSERHOME", &home)) home = wxGetHomeDir();
  std::ifstream in((home + "/.ECCE/EcceGlobal").ToStdString());
  string line;
  bool sys = false;
  while (std::getline(in, line)) {
    size_t c = line.find(':');
    if (c == string::npos) continue;
    string k = trim(line.substr(0, c)), v = trim(line.substr(c + 1));
    for (auto& ch : k) ch = tolower(ch);
    for (auto& ch : v) ch = tolower(ch);
    if (k == "usesystemfont") sys = v == "true" || v == "1";
    if (k == "fontsize") fs = atoi(v.c_str());
  }
  wxFont f = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
  if (sys) base = f.GetPointSize();
  int step = fs == 0 ? -1 : fs == 2 ? 2 : fs == 3 ? 4 : 0;
  f.SetPointSize(base + step);
  return f;
}

class DialogPanel : public wxPanel {
public:
  DialogPanel(wxFrame *frame, const string& dlgFile);
  void finalizeSettings();
  void checkDependency();
  void close();
  vector<Ctl*> settings;
  wxPanel *footer = 0;
  wxTextCtrl *msg = 0;
  wxButton *resetBtn = 0;
  string helpURL;

private:
  void onChange(wxCommandEvent& e);
  void changed(Ctl *c);
  void onReset(wxCommandEvent&);
  void onClose(wxCommandEvent&) { close(); }
  void checkButtonEnables();
  void validate(Ctl *c);
  void enable(Ctl *c, bool on);
  wxFrame *p_frame;
};

DialogPanel::DialogPanel(wxFrame *frame, const string& dlgFile)
    : wxPanel(frame, wxID_ANY, wxDefaultPosition, wxDefaultSize,
              wxTAB_TRAVERSAL),
      p_frame(frame)
{
  wxFont font = themeFont();
  SetFont(font);
  std::ifstream in(dlgFile);
  if (!in) {
    fprintf(stderr, "ecce-dialog: cannot read %s\n", dlgFile.c_str());
    exit(2);
  }

  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
  wxFlexGridSizer *grid = 0;
  bool boxOn = true, hideMsg = false;
  int minWidth = 0;
  string title, raw;
  const int fl = wxALL | wxALIGN_CENTER_VERTICAL;
  while (std::getline(in, raw)) {
    Line L;
    if (!parseLine(raw, L)) continue;
    if (L.cmd == "dialog") {
      title = L.get("title");
      helpURL = L.get("help");
      minWidth = atoi(L.get("minwidth", "0").c_str());
      hideMsg = L.get("message") == "hide";
    } else if (L.cmd == "box") {
      boxOn = evalExpr(L.get("when"));
      if (!boxOn) continue;
      // The Python EcceBoxSizer: a rule, the label, a rule.
      wxBoxSizer *bar = new wxBoxSizer(wxHORIZONTAL);
      string lbl = L.get("label");
      if (lbl.empty()) bar->Add(new wxStaticLine(this), 1);
      else {
        bar->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition,
                                  wxSize(10, -1)), 0, wxALIGN_CENTER, 0);
        bar->Add(new wxStaticText(this, wxID_ANY, lbl), 0,
                 wxALIGN_CENTER | wxALL, 2);
        bar->Add(new wxStaticLine(this), 1, wxALIGN_CENTER, 0);
      }
      wxBoxSizer *box = new wxBoxSizer(wxVERTICAL);
      box->Add(bar, 0, wxEXPAND, 0);
      grid = new wxFlexGridSizer(0, atoi(L.get("cols", "1").c_str()), 0, 3);
      box->Add(grid, 1, wxALL | wxEXPAND, 0);
      top->Add(box, 0, wxALL | wxEXPAND, 1);
    } else if (boxOn && grid && evalExpr(L.get("when"))) {
      Ctl *c = new Ctl();
      c->kind = L.cmd;
      c->name = L.get("name");
      c->label = L.get("label");
      c->unit = L.get("unit");
      c->def = L.get("default");
      c->exportFlag = atoi(L.get("export", "0").c_str());
      c->enableIf = L.get("enable_if");
      c->showIf = L.get("show_if");
      if (!L.get("min").empty()) c->mn = atof(L.get("min").c_str());
      if (!L.get("max").empty()) c->mx = atof(L.get("max").c_str());
      wxString nm(c->name);
      if (c->kind == "note") {
        wxStaticText *t = new wxStaticText(this, wxID_ANY, c->label);
        c->w = t;
        grid->Add(t, 0, fl, 3);
        g_ctls.push_back(c);
        continue;
      }
      if (c->kind == "check") {
        if (c->def.empty()) c->def = "0";
        wxCheckBox *b = new wxCheckBox(this, wxID_ANY, c->label,
            wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, nm);
        c->w = b;
        grid->Add(b, 0, fl, 3);
        b->Bind(wxEVT_CHECKBOX, &DialogPanel::onChange, this);
      } else if (c->kind == "radiopair") {
        // first = the default (false) choice, second = the stored setting.
        wxRadioButton *a = new wxRadioButton(this, wxID_ANY, L.get("first"),
            wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
        wxRadioButton *b = new wxRadioButton(this, wxID_ANY, L.get("second"),
            wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, nm);
        a->SetValue(true);
        c->def = "0";
        c->w = a;
        c->w2 = b;
        grid->Add(a, 0, fl, 3);
        grid->Add(b, 0, fl, 3);
        a->Bind(wxEVT_RADIOBUTTON, &DialogPanel::onChange, this);
        b->Bind(wxEVT_RADIOBUTTON, &DialogPanel::onChange, this);
        b->SetToolTip(nm);
      } else if (c->kind == "combo") {
        wxStringTokenizer tk(L.get("choices"), "|");
        wxArrayString ch;
        while (tk.HasMoreTokens()) {
          ch.Add(tk.GetNextToken());
          c->choices.push_back(ch.Last().ToStdString());
        }
        wxBoxSizer *row = new wxBoxSizer(wxHORIZONTAL);
        if (!c->label.empty()) {
          c->lab = new wxStaticText(this, wxID_ANY, c->label + "   ");
          row->Add(c->lab, 0, fl, 0);
        }
        wxChoice *ch2 = new wxChoice(this, wxID_ANY, wxDefaultPosition,
            wxDefaultSize, ch, 0, wxDefaultValidator, nm);
        c->w = ch2;
        row->Add(ch2, 0, fl, 0);
        grid->Add(row, 0, fl, 3);
        if (c->def.empty() && !c->choices.empty()) c->def = c->choices[0];
        ch2->SetStringSelection(c->def);
        ch2->Bind(wxEVT_CHOICE, &DialogPanel::onChange, this);
      } else {   // text, float, int
        wxBoxSizer *row = new wxBoxSizer(wxHORIZONTAL);
        if (!c->label.empty()) {
          c->lab = new wxStaticText(this, wxID_ANY, c->label + "   ");
          row->Add(c->lab, 0, fl, 0);
        }
        if (c->kind == "int") {
          wxSpinCtrl *s = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition,
              wxSize(80, -1), wxSP_ARROW_KEYS, (int)std::max(c->mn, -1e6),
              (int)std::min(c->mx, 1e6), atoi(c->def.c_str()));
          s->SetName(nm);
          c->w = s;
          s->Bind(wxEVT_SPINCTRL, &DialogPanel::onChange, this);
        } else {
          wxTextCtrl *t = new wxTextCtrl(this, wxID_ANY, c->def,
              wxDefaultPosition, wxSize(80, -1), wxTE_PROCESS_ENTER,
              wxDefaultValidator, nm);
          c->w = t;
          t->Bind(wxEVT_KILL_FOCUS, [this, c](wxFocusEvent& e) {
            validate(c);
            e.Skip();
          });
          t->Bind(wxEVT_TEXT_ENTER, [this, c](wxCommandEvent&) { validate(c); });
        }
        row->Add(c->w, 0, fl, 0);
        if (!c->unit.empty()) {
          string ur = "  " + c->unit;
          if (c->kind != "text" && (c->mn > -1e99 || c->mx < 1e99)) {
            ur += "\n   [" + (c->mn > -1e99 ? std::to_string((long)c->mn) : "") +
                  ", " + (c->mx < 1e99 ? std::to_string((long)c->mx) : "") + "]";
          }
          row->Add(new wxStaticText(this, wxID_ANY, ur), 0, fl, 0);
        }
        grid->Add(row, 0, fl, 3);
      }
      c->w->SetToolTip(nm);
      c->saved = c->get();
      settings.push_back(c);
      g_ctls.push_back(c);
    }
  }
  if (minWidth) top->SetMinSize(wxSize(minWidth, -1));
  SetSizer(top);

  // The footer belongs to the frame, as in the Python dialogs, so it stays put
  // when the settings area changes size.
  footer = new wxPanel(frame, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                       wxTAB_TRAVERSAL);
  footer->SetFont(font);
  wxBoxSizer *fs = new wxBoxSizer(wxVERTICAL);
  footer->SetSizer(fs);
  wxBoxSizer *fr = new wxBoxSizer(wxVERTICAL);
  frame->SetSizer(fr);
  fr->Add(this, 1, wxEXPAND);
  fr->Add(footer, 0, wxEXPAND);

  msg = new wxTextCtrl(footer, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 65),
                       wxTE_MULTILINE | wxTE_WORDWRAP | wxTE_READONLY);
  msg->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
  fs->Add(msg, 0, wxALL | wxEXPAND, 1);

  wxBoxSizer *bs = new wxBoxSizer(wxHORIZONTAL);
  bs->AddStretchSpacer(1);
  wxButton *closeB = new wxButton(footer, wxID_ANY, "Close");
  closeB->SetToolTip("close window");
  closeB->Bind(wxEVT_BUTTON, &DialogPanel::onClose, this);
  resetBtn = new wxButton(footer, wxID_ANY, "Reset");
  resetBtn->SetToolTip("Restore default settings");
  resetBtn->Bind(wxEVT_BUTTON, &DialogPanel::onReset, this);
  wxButton *helpB = 0;
  if (!helpURL.empty()) {
    helpB = new wxButton(footer, wxID_ANY, "Help");
    helpB->Bind(wxEVT_BUTTON,
                [this](wxCommandEvent&) { wxLaunchDefaultBrowser(helpURL); });
  }
  // One sizer item per button on every platform; the stretch spacers are
  // not added for Windows, so the row cannot be laid out twice there.
  for (wxButton *b : {closeB, resetBtn, helpB}) {
    if (!b) continue;
#ifdef __WXMSW__
    bs->Add(b, 0, wxALL, 3);
#else
    bs->Add(b, 0, wxTOP, 3);
    bs->AddStretchSpacer(1);
#endif
  }
  fs->Add(bs, 0, wxEXPAND, 0);
  if (hideMsg) msg->Hide();
  frame->SetTitle(title);
}

void DialogPanel::validate(Ctl *c)
{
  if (c->kind != "float") return;
  double v;
  string s = c->get();
  if (!numeric(s, v) || v < c->mn || v > c->mx) {
    msg->SetValue("Invalid value for " + c->name + ": out of range");
    c->set(c->saved);
    return;
  }
  changed(c);
}

void DialogPanel::enable(Ctl *c, bool on)
{
  bool was = c->w->IsEnabled();
  c->w->Enable(on);
  if (c->w2) c->w2->Enable(on);
  if (c->lab) c->lab->Enable(on);
  // The Python combo and number widgets report an enable change as an edit.
  if (was != on && c->sendsOnEnable() && g_port &&
      g_readOnlyFlag == "Writable") {
    string l = c->exportLine() + "\n";
    if (g_restoreFlag == "NO_GUIValues") g_batch.push_back(l);
    else sendRaw(l);
  }
}

void DialogPanel::checkDependency()
{
  for (Ctl *c : g_ctls) {
    if (!c->showIf.empty()) c->w->Show(evalExpr(c->showIf));
    if (!c->enableIf.empty() && c->kind != "note")
      enable(c, evalExpr(c->enableIf));
  }
  Layout();
}

void DialogPanel::checkButtonEnables()
{
  if (!g_loopStarted) return;
  bool any = false;
  for (Ctl *c : settings) if (c->notDefault()) { any = true; break; }
  resetBtn->Enable(any);
}

void DialogPanel::changed(Ctl *c)
{
  checkDependency();
  if (g_readOnlyFlag == "Writable") {
    if (g_port) {
      string l = c->exportLine() + "\n";
      if (g_restoreFlag == "NO_GUIValues") g_batch.push_back(l);
      else sendRaw(l);
      checkButtonEnables();
      c->saved = c->get();
    } else {
      msg->SetValue("Calculation is read only.  Changes cannot be saved!");
    }
  }
}

void DialogPanel::onChange(wxCommandEvent& e)
{
  wxObject *o = e.GetEventObject();
  for (Ctl *c : settings)
    if (c->w == o || c->w2 == o) { changed(c); break; }
  e.Skip();
}

void DialogPanel::onReset(wxCommandEvent& e)
{
  wxMessageDialog d(this, "Hitting OK will set all parameters to their "
      "default values.\nDo you want to continue?", "Reset Defaults?",
      wxOK | wxCANCEL | wxICON_QUESTION | wxSTAY_ON_TOP);
  if (d.ShowModal() == wxID_OK) {
    for (Ctl *c : settings) {
      c->set(c->def);
      changed(c);          // the Python Reset() reports every setting
    }
    checkButtonEnables();
    checkDependency();
  }
  e.Skip();
}

void DialogPanel::close()
{
  if (g_readOnlyFlag == "Writable" && g_port) sendRaw("#CLOSING\n");
  p_frame->Destroy();
}

// Applies the stored values in the restore file, then reports every setting.
void DialogPanel::finalizeSettings()
{
  vector<Ctl*> notRestored;
  if (g_restoreFlag == "GUIValues") {
    std::ifstream f(g_restoreFile);
    string line;
    while (std::getline(f, line)) {
      vector<string> part(1);
      for (char ch : line) {
        if (ch == '|') part.push_back("");
        else part.back() += ch;
      }
      if (part.size() != 6) continue;
      const string &name = part[0], &value = part[1], &unit = part[2],
                   &active = part[3], &type = part[5];
      bool known = false;
      for (Ctl *c : settings) {
        if (c->name != name) continue;
        known = true;
        string stored = trim(unit), widget = trim(c->unit);
        auto head = [](string s) {
          size_t p = s.find(" / ");
          return p == string::npos ? s : s.substr(0, p);
        };
        if (head(stored) == head(widget)) stored = widget;
        // A number saved in another unit would be silently wrong here.
        if (!stored.empty() && !widget.empty() && stored != widget) {
          debug("restore " + name + ": skipped, the stored unit differs");
          notRestored.push_back(c);
          continue;
        }
        if (type == "toggle_input") c->set(value == "1" ? "1" : "0");
        else c->set(value);
        enable(c, active == "1");
      }
      if (!known) debug("restore " + name + ": no widget of that name, ignored");
    }
  }
  for (Ctl *c : settings) {
    c->saved = c->get();
    if (g_port) {
      string l = c->exportLine() + "\n";
      if (g_restoreFlag == "NO_GUIValues") g_batch.push_back(l);
      else sendRaw(l);
    }
  }
  checkDependency();
  if (g_port) {
    if (g_restoreFlag == "NO_GUIValues") g_batch.push_back("#INITIALIZED\n");
    else {
      sendRaw("#INITIALIZED\n");
      for (Ctl *c : notRestored) sendRaw(c->exportLine() + "\n");
    }
  }
  if (g_restoreFlag == "NO_GUIValues") {
    string all;
    for (auto& s : g_batch) all += s;
    sendRaw(all);
    close();
  } else {
    g_loopStarted = true;
    checkButtonEnables();
    Layout();
  }
}

class DialogApp : public wxApp {
public:
  bool OnInit() override
  {
    // argv: <dlg> <restore> <port> <flag> <rw> <debug> <category> <theory>
    //       <runtype> <name> <rxn> <sym> <nelec> <mult> <nfrozen> <nocc>
    //       <nvirt> <nmodes>
    vector<string> a;
    for (int i = 1; i < argc; i++) a.push_back(string(argv[i].ToUTF8()));
    if (a.size() < 18) {
      fprintf(stderr, "ecce-dialog: details dialog command line parameter "
              "missing!\n");
      return false;
    }
    static const char *names[] = {"restore", "port", "restoreflag", "rw",
      "debug", "category", "theory", "runtype", "calc", "rxn", "symmetry",
      "nelec", "mult", "nfrozen", "nocc", "nvirt", "nmodes"};
    for (int i = 0; i < 17; i++) g_vars[names[i]] = a[i + 1];
    g_restoreFile = a[1];
    g_port = atol(a[2].c_str());
    g_restoreFlag = a[3];
    g_readOnlyFlag = a[4];
    g_debug = a[5] == "DebugOn";
    p_dlg = a[0];
    // Sockets need the running app, so the rest waits for the event loop.
    CallAfter([this] { start(); });
    return true;
  }

private:
  string p_dlg;

  void start()
  {
    if (g_port) {
      g_peer = new wxIPV4address; g_peer->Hostname("127.0.0.1");
      g_peer->Service((unsigned short)g_port);
      wxIPV4address local;
      local.Service(0);
      g_sock = new wxDatagramSocket(local);
      long pid = (long)wxGetProcessId();
      sendRaw("#STARTED " + (g_restoreFlag == "NO_GUIValues" ? string("0")
                                : std::to_string(pid)) + "\n");
    }

    wxFrame *f = new wxFrame(0, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
        wxDEFAULT_FRAME_STYLE & ~(wxRESIZE_BORDER | wxMAXIMIZE_BOX));
    f->SetFont(themeFont());
    DialogPanel *p = new DialogPanel(f, p_dlg);
    f->Bind(wxEVT_CLOSE_WINDOW, [p](wxCloseEvent&) { p->close(); });
    p->finalizeSettings();
    if (g_restoreFlag == "NO_GUIValues") return;   // closed already
    f->Fit();
    placeOverEditor(f);
    f->Show(true);
  }

  static void placeOverEditor(wxFrame *f)
  {
    wxString r;
    int x, y, w, h;
    if (!wxGetEnv("ECCE_EDITOR_RECT", &r) ||
        sscanf(r.ToStdString().c_str(), "%d,%d,%d,%d", &x, &y, &w, &h) != 4)
      return;
    wxSize sz = f->GetSize();
    int px = x + (w - sz.x) / 2, py = y + (h - sz.y) / 2;
    int n = wxDisplay::GetFromPoint(wxPoint(x + w / 2, y + h / 2));
    if (n != wxNOT_FOUND) {
      wxRect area = wxDisplay(n).GetClientArea();
      px = std::max(area.x, std::min(px, area.x + area.width - sz.x));
      py = std::max(area.y, std::min(py, area.y + area.height - sz.y));
    }
    f->SetPosition(wxPoint(px, py));
  }
};

}  // namespace

wxIMPLEMENT_APP(DialogApp);
