#include "DetailsModel.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>

namespace details {

using std::string;

namespace {

const double kMinInt = -1000000, kMaxInt = 1000000;   // globals.py defaults

bool isIn(const Strings& v, const string& s)
{
  return std::find(v.begin(), v.end(), s) != v.end();
}

string jstr(const string& s)
{
  string o = "\"";
  for (char c : s) {
    if (c == '"' || c == '\\') { o += '\\'; o += c; }
    else if (c == '\n') o += "\\n";
    else if (c == '\t') o += "\\t";
    else o += c;
  }
  return o + "\"";
}

string jlist(const Strings& v)
{
  string o = "[";
  for (size_t i = 0; i < v.size(); i++) o += (i ? ", " : "") + jstr(v[i]);
  return o + "]";
}

string head(const string& s)
{
  size_t p = s.find(" / ");
  return p == string::npos ? s : s.substr(0, p);
}

}  // namespace

void Model::dbg(const string& m) const
{
  if (par_.debug) fprintf(stderr, "[ecce-dialog] %s\n", m.c_str());
}

bool Model::lookupVar(const string& n, string& v) const
{
  std::map<string, string>::const_iterator it = par_.vars.find(n);
  if (it == par_.vars.end()) return false;
  v = it->second;
  return true;
}

bool Model::lookupAny(const string& n, string& v) const
{
  if (lookupVar(n, v)) return true;
  int i = find(n);
  if (i < 0) return false;
  v = items_[i].value;
  return true;
}

bool Model::lookupItem(const Item& it, const string& n, string& v) const
{
  if (n == "default") { v = it.def; return true; }
  if (n == "value") { v = it.value; return true; }
  if (n.size() > 1 && n[0] == '@' && isdigit((unsigned char)n[1])) {
    size_t k = atoi(n.c_str() + 1);
    v = k < it.choices.size() ? it.choices[k] : "";
    return true;
  }
  return lookupAny(n, v);
}

bool Model::eval(const string& e, bool live, const Item *self)
{
  string err;
  Lookup lk = [this, live, self](const string& n, string& v) {
    if (self) return lookupItem(*self, n, v);
    return live ? lookupAny(n, v) : lookupVar(n, v);
  };
  bool r = evalExpr(e, lk, desc_.lists, &err);
  if (!err.empty()) warnings_.push_back(err);
  return r;
}

int Model::find(const string& name) const
{
  for (size_t i = 0; i < items_.size(); i++)
    if (items_[i].name == name && items_[i].isSetting()) return (int)i;
  return -1;
}

string Model::normalize(const Item& it, const string& v) const
{
  const string& k = it.kind;
  if (k == "check" || k == "radiopair") return v == "1" ? "1" : "0";
  if (k == "combo") return isIn(it.choices, v) ? v : it.value;
  if (k == "calcpick") return isIn(it.urls, v) ? v : "";
  if (k == "int") {
    bool hl, hh;
    double lo = kMinInt, hi = kMaxInt, a, b;
    if (rangeBounds(it.hard, hl, a, hh, b)) {
      if (hl) lo = a;
      if (hh) hi = b;
    }
    long x = atol(v.c_str());
    x = std::max((long)lo, std::min((long)hi, x));
    return std::to_string(x);
  }
  return v;
}

string Model::typeName(const Item& it) const
{
  const string& k = it.kind;
  return k == "check" || k == "radiopair" ? "toggle_input" :
         k == "combo" || k == "calcpick" ? "menu_input" :
         k == "int" ? "integer_input" : k == "float" ? "float_input" :
         "text_input";
}

string Model::unitOf(const Item& it) const
{
  return it.kind == "int" || it.kind == "float" ? it.unit : "";
}

bool Model::notDefault(const Item& it) const
{
  double a, b;
  if ((it.kind == "float" || it.kind == "int")) {
    if (!toNumber(it.value, a)) return true;
    return toNumber(it.def, b) ? a != b : true;
  }
  return it.value != it.def;
}

bool Model::anyNotDefault() const
{
  for (const Item& it : items_)
    if (it.isSetting() && notDefault(it)) return true;
  return false;
}

bool Model::sendsOnEnable(const Item& it) const
{
  const string& k = it.kind;
  return k == "combo" || k == "calcpick" || k == "int" || k == "float" ||
         k == "text";
}

string Model::exportLine(int index) const
{
  const Item& it = items_[index];
  int need = it.kind == "radiopair" ? 1 : it.exportFlag;
  if (!need && !it.exportIf.empty() &&
      const_cast<Model*>(this)->eval(it.exportIf, true, &it))
    need = 1;
  if (notDefault(it)) need = 1;
  return it.name + "|" + it.value + "|" + unitOf(it) + "|" +
         (it.enabled ? "1" : "0") + "|" + std::to_string(need) + "|" +
         typeName(it);
}

void Model::save(Item& it)
{
  it.saved = it.value;
  double v;
  if (toNumber(it.value, v)) it.savedNum = v;
}

void Model::say(const string& t, Severity s)
{
  if (view_) view_->status(t, s);
}

void Model::touch(int index)
{
  if (view_) view_->itemUpdated(index);
}

void Model::setValue(int index, const string& text)
{
  items_[index].value = normalize(items_[index], text);
  touch(index);
}

void Model::emit(int index)
{
  string l = exportLine(index) + "\n";
  if (par_.restoreFlag == "NO_GUIValues") batch_.push_back(l);
  else if (transport_) transport_->send(l);
}

void Model::changed(int index)
{
  checkDependency();
  if (!writable()) return;
  if (transport_) {
    emit(index);
    if (loopStarted_ && view_) view_->resetEnabled(anyNotDefault());
    save(items_[index]);
  } else {
    say("Calculation is read only.  Changes cannot be saved!", WARNING);
  }
}

// The wx Enable() of the combo, number and text widgets reports a real
// change of state as an edit; toggles do not.
void Model::enable(int index, bool on)
{
  Item& it = items_[index];
  bool was = it.enabled;
  it.enabled = on;
  touch(index);
  if (was != on && sendsOnEnable(it)) changed(index);
}

void Model::checkDependency()
{
  if (depth_ > 8) return;
  depth_++;
  for (size_t s = 0; s < steps_.size(); s++) {
    Step& st = steps_[s];
    if (st.type == Step::ItemRule) {
      Item& it = items_[st.item];
      if (!it.showIf.empty()) {
        bool on = eval(it.showIf, true);
        if (on != it.shown) { it.shown = on; touch(st.item); }
      }
      if (!it.enableIf.empty() && it.isSetting())
        enable(st.item, eval(it.enableIf, true));
      else if (!it.enableIf.empty()) {
        bool on = eval(it.enableIf, true);
        if (on != it.enabled) { it.enabled = on; touch(st.item); }
      }
    } else if (st.type == Step::Force) {
      if (!eval(st.when, true)) continue;
      int t = find(st.target);
      if (t < 0) continue;
      string nv = normalize(items_[t],
          substitute(st.value, [this](const string& n, string& v) {
            return lookupAny(n, v); }));
      if (nv == items_[t].value) continue;
      items_[t].value = nv;
      touch(t);
      if (!st.silent) changed(t);
    } else {
      bool on = eval(st.when, true);
      if (on && !st.on)
        say(substitute(st.text, [this](const string& n, string& v) {
              return lookupAny(n, v); }), NORMAL);
      st.on = on;
    }
  }
  if (--depth_ == 0 && view_) view_->layoutChanged();
}

void Model::userEdit(int index, const string& text)
{
  Item& it = items_[index];
  if (it.kind == "float") {
    double v;
    if (!toNumber(text, v)) {
      say("Error: " + text + " is not a valid floating point number!", ERROR);
      it.value = pyFloatStr(it.savedNum);
      touch(index);
      return;
    }
    it.value = pyFloatStr(v);
    touch(index);
    if (loopStarted_ && view_) view_->resetEnabled(anyNotDefault());
    string msg;
    if (!inRange(it.hard, v, msg)) {
      say("Error: Value outside allowed range--must be " + msg, ERROR);
      it.value = pyFloatStr(it.savedNum);
      touch(index);
      return;
    }
    if (!inRange(it.soft, v, msg))
      say("Warning: Value outside recommended range--typically " + msg,
          WARNING);
    if (v != it.savedNum) changed(index);
  } else if (it.kind == "int") {
    it.value = normalize(it, text);
    touch(index);
    string msg;
    if (!inRange(it.soft, atof(it.value.c_str()), msg))
      say("Warning: Value outside recommended range " + it.soft + "!",
          WARNING);
    if (it.value != it.saved) changed(index);
  } else if (it.kind == "text") {
    it.value = text;
    if (it.value != it.saved) changed(index);
  } else {
    it.value = normalize(it, text);
    touch(index);
    changed(index);
  }
}

void Model::reset()
{
  for (size_t i = 0; i < items_.size(); i++) {
    if (!items_[i].isSetting()) continue;
    items_[i].value = items_[i].def;
    items_[i].value = normalize(items_[i], items_[i].def);
    touch((int)i);
    changed((int)i);        // the Python Reset() reports every setting
  }
  if (loopStarted_ && view_) view_->resetEnabled(anyNotDefault());
  checkDependency();
}

void Model::close()
{
  if (writable() && transport_) transport_->send("#CLOSING\n");
  if (view_) view_->closeRequested();
}

void Model::initialize()
{
  std::vector<int> notRestored;
  bool legacyDispersion = false, legacyGrid = false;
  if (par_.restoreFlag == "GUIValues") {
    std::ifstream f(par_.restoreFile);
    string line;
    while (std::getline(f, line)) {
      Strings part = split(line, '|');
      if (part.size() != 6) continue;
      const string &name = part[0], &value0 = part[1], &unit = part[2],
                   &active = part[3];
      int i = find(name);
      if (i < 0) {
        dbg("restore " + name + ": no widget of that name, ignored");
        continue;
      }
      if (name == "ES.Theory.SCF.Dispersion" && legacyDispersion) continue;
      if (name == "ES.Theory.DFT.Grid" && legacyGrid) continue;
      Item& it = items_[i];
      // ORCA: a functional stored as "PBE0 D3BJ" or "B3LYP-D3" is the
      // functional plus the Dispersion (and Grid) choice; templates.py
      // does the same.
      string value = value0;
      if (name == "ES.Theory.DFT.XCFunctionals" &&
          std::find(it.choices.begin(), it.choices.end(), value) ==
              it.choices.end()) {
        // Tails are peeled off in either order, each at most once.
        int d = find("ES.Theory.SCF.Dispersion");
        int g = find("ES.Theory.DFT.Grid");
        string base = value, disp, grid;
        // A known name ends the peeling, so B97-D3 keeps its D3.
        while (std::find(it.choices.begin(), it.choices.end(), base) ==
               it.choices.end()) {
          size_t cut = base.find_last_of(" -");
          if (cut == string::npos || cut == 0) break;
          string tail = base.substr(cut + 1);
          for (char& c : tail) c = (char)toupper((unsigned char)c);
          if (tail == "D3" || tail == "D3ZERO" || tail == "D3(0)") tail = "D3(0)";
          bool isGrid = tail == "DEFGRID1" || tail == "DEFGRID2" ||
                        tail == "DEFGRID3";
          bool isDisp = tail == "D3(0)" || tail == "D3BJ" || tail == "D4";
          if (isGrid && grid.empty()) grid = tail;
          else if (isDisp && disp.empty()) disp = tail;
          else break;
          base = base.substr(0, cut);
        }
        if (base != value &&
            std::find(it.choices.begin(), it.choices.end(), base) !=
                it.choices.end()) {
          if (!disp.empty() && d >= 0) {
            items_[d].value = normalize(items_[d], disp);
            touch(d);
            legacyDispersion = true;
          }
          if (!grid.empty() && g >= 0) {
            items_[g].value = normalize(items_[g], "DefGrid" + grid.substr(7));
            touch(g);
            legacyGrid = true;
          }
          value = base;
        }
      }
      string stored = trim(unit), widget = trim(unitOf(it));
      if (head(stored) == head(widget)) stored = widget;
      // A number saved in another unit would be silently wrong here.
      if (!stored.empty() && !widget.empty() && stored != widget) {
        dbg("restore " + name + ": skipped, the stored unit differs");
        notRestored.push_back(i);
        continue;
      }
      it.value = normalize(it, value);
      touch(i);
      enable(i, active == "1");
    }
  }
  for (size_t i = 0; i < items_.size(); i++) {
    if (!items_[i].isSetting()) continue;
    save(items_[i]);
    if (transport_) emit((int)i);
  }
  checkDependency();
  if (transport_) {
    if (par_.restoreFlag == "NO_GUIValues") batch_.push_back("#INITIALIZED\n");
    else {
      transport_->send("#INITIALIZED\n");
      for (int i : notRestored) transport_->send(exportLine(i) + "\n");
    }
  }
  if (par_.restoreFlag == "NO_GUIValues") {
    string all;
    for (const string& s : batch_) all += s;
    if (transport_) transport_->send(all);
    close();
  } else {
    loopStarted_ = true;
    if (view_) { view_->resetEnabled(anyNotDefault()); view_->layoutChanged(); }
  }
}

bool Model::build(string& error)
{
  Lookup argv = [this](const string& n, string& v) { return lookupVar(n, v); };
  static const char *kinds[] = {"check", "radiopair", "combo", "calcpick",
                                "int", "float", "text", "note"};
  int curBox = -1;
  bool boxOn = false;
  for (const Line& L : desc_.lines) {
    const string& c = L.cmd;
    if (c == "dialog") {
      title = L.get("title");
      helpURL = L.get("help");
      minWidth = atoi(L.get("minwidth", "0").c_str());
      hideMessage = L.get("message") == "hide";
    } else if (c == "box") {
      boxOn = eval(L.get("when"), false);
      if (!boxOn) continue;
      Box b;
      b.label = substitute(L.get("label"), argv);
      b.cols = atoi(L.get("cols", "1").c_str());
      boxes_.push_back(b);
      curBox = (int)boxes_.size() - 1;
      entries_.push_back({'B', curBox});
    } else if (c == "force") {
      Step st;
      st.type = Step::Force;
      st.target = L.get("name");
      st.value = L.get("value");
      st.when = L.get("when");
      st.silent = L.get("silent") == "1";
      steps_.push_back(st);
    } else if (c == "status") {
      Step st;
      st.type = Step::StatusMsg;
      st.text = L.get("text");
      st.when = L.get("when");
      steps_.push_back(st);
    } else if (std::find(kinds, kinds + 8, c) != kinds + 8) {
      if (curBox < 0 || !boxOn || !eval(L.get("when"), false)) continue;
      Item it;
      it.kind = c;
      it.box = curBox;
      it.name = L.get("name");
      it.label = substitute(L.get("label"), argv);
      it.unit = L.get("unit");
      it.soft = substitute(L.get("soft"), argv);
      it.hard = substitute(L.get("hard"), argv);
      it.exportFlag = atoi(L.get("export", "0").c_str());
      it.enableIf = L.get("enable_if");
      it.showIf = L.get("show_if");
      it.exportIf = L.get("export_if");
      it.first = L.get("first");
      it.second = L.get("second");
      it.def = L.get("default");
      Strings clauses = split(L.get("default_if"), ';');
      for (const string& cl : clauses) {
        size_t arrow = cl.find("=>");
        if (arrow == string::npos) continue;
        if (eval(cl.substr(0, arrow), false)) {
          it.def = trim(cl.substr(arrow + 2));
          break;
        }
      }
      it.def = substitute(it.def, argv);
      if (c == "combo") {
        string ch = L.get("choices");
        if (!ch.empty() && ch[0] == '@') {
          Lists::const_iterator li = desc_.lists.find(ch.substr(1));
          if (li == desc_.lists.end()) {
            error = "unknown list " + ch;
            return false;
          }
          it.choices = li->second;
        } else it.choices = split(ch, '|');
        if (ch.empty()) it.choices.clear();
        if (!isIn(it.choices, it.def)) {
          if (!it.def.empty() && !it.choices.empty())
            warnings_.push_back(it.name + ": default \"" + it.def +
                                "\" is not one of the choices");
          it.def = it.choices.empty() ? "" : it.choices[0];
        }
      } else if (c == "calcpick") {
        it.urls.push_back("");
        it.choices.push_back("(none)");
        std::ifstream f(par_.candidatesFile);
        string line;
        while (std::getline(f, line)) {
          Strings p = split(line, '\t');
          if (p.size() != 3) continue;
          it.urls.push_back(p[1]);
          it.choices.push_back(p[0] + " (" + p[2] + " structure)");
        }
        it.def = "";
      } else if (c == "check") {
        it.def = it.def == "1" ? "1" : "0";
      } else if (c == "radiopair") {
        it.def = "0";
      } else if (c == "float" && it.def.empty()) {
        it.def = "0.0";
      } else if (c == "int" && it.def.empty()) {
        it.def = "0";
      }
      it.value = it.def;
      if (c == "int") it.value = normalize(it, it.def);
      save(it);
      items_.push_back(it);
      entries_.push_back({'I', (int)items_.size() - 1});
      if (!it.enableIf.empty() || !it.showIf.empty()) {
        Step st;
        st.type = Step::ItemRule;
        st.item = (int)items_.size() - 1;
        steps_.push_back(st);
      }
    } else {
      error = "unknown command: " + c;
      return false;
    }
  }
  return true;
}

string Model::inventoryJson()
{
  transport_ = 0;
  view_ = 0;
  par_.readOnly = "ReadOnly";
  checkDependency();
  string o = "{\n \"title\": " + jstr(title) + ",\n \"widgets\": [";
  bool first = true;
  for (const Item& it : items_) {
    if (!it.isSetting()) continue;
    o += first ? "\n  " : ",\n  ";
    first = false;
    o += "{\"kind\": " + jstr(it.kind) + ", \"name\": " + jstr(it.name) +
         ", \"label\": " + jstr(it.label) + ", \"type\": " +
         jstr(typeName(it)) + ", \"choices\": " + jlist(it.choices) +
         ", \"default\": " + jstr(it.def) + ", \"value\": " +
         jstr(it.value) + ", \"export\": " +
         std::to_string(it.kind == "radiopair" ? 1 : it.exportFlag) +
         ", \"unit\": " + jstr(unitOf(it)) + ", \"softRange\": " +
         jstr(it.soft) + ", \"hardRange\": " + jstr(it.hard) +
         ", \"enabled\": " + (it.enabled ? "true" : "false") + "}";
  }
  o += "\n ],\n \"warnings\": " + jlist(warnings_) + "\n}\n";
  return o;
}

}  // namespace details
