#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <unistd.h>

#include "dsm/CodeFactory.H"
#include "dsm/EDSIFactory.H"
#include "dsm/ICalculation.H"
#include "dsm/MachinePreferences.H"
#include "dsm/Resource.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/ResourceType.H"
#include "dsm/Series.H"
#include "dsm/SmilesBatch.H"
#include "dsm/TaskJob.H"
#include "dsm/VDoc.H"
#include "tdat/BondOrderRule.H"
#include "tdat/ForceField.H"
#include "tdat/FragGraft.H"
#include "tdat/Fragment.H"
#include "tdat/TAtm.H"
#include "tdat/TBond.H"
#include "tdat/TRunType.H"
#include "tdat/TTheory.H"
#include "tdat/UFFDesc.H"
#include "util/Ecce.H"
#include "util/EcceURL.H"
#include "util/Filesystem.H"

using std::string;
using std::vector;

const char *SmilesBatch::kSmilesProp = "qsar.smiles";
const char *SmilesBatch::kMeasuredProp = "qsar.measured";

namespace {

const char *kTemplateName = "QSAR template";

string ns(const char *key) { return VDoc::getEcceNamespace() + ":" + key; }

string trim(const string& s)
{
  size_t a = s.find_first_not_of(" \t\r\n"), b = s.find_last_not_of(" \t\r\n");
  return a == string::npos ? string() : s.substr(a, b - a + 1);
}

string lower(string s)
{
  for (char& c : s) c = (char)tolower((unsigned char)c);
  return s;
}

bool isNumber(const string& s)
{
  const string t = trim(s);
  if (t.empty()) return false;
  char *end = 0;
  strtod(t.c_str(), &end);
  return end && *end == '\0';
}

vector<string> splitLine(const string& line, char d)
{
  vector<string> cells;
  string cur;
  bool quoted = false;
  for (size_t i = 0; i < line.size(); i++) {
    const char c = line[i];
    if (quoted) {
      if (c == '"' && i + 1 < line.size() && line[i + 1] == '"') { cur += '"'; i++; }
      else if (c == '"') quoted = false;
      else cur += c;
    } else if (c == '"' && trim(cur).empty()) { cur.clear(); quoted = true; }
    else if (c == d) { cells.push_back(trim(cur)); cur.clear(); }
    else cur += c;
  }
  cells.push_back(trim(cur));
  return cells;
}

bool parsesAsSmiles(const string& s)
{
  if (s.empty() || s.find(' ') != string::npos) return false;
  FragGraft::Mol m;
  string err;
  return FragGraft::smilesTo3D(s, m, err) && m.dummy < 0;
}

// A calculation name that the data folder takes: no slash, no leading dot.
string safeName(const string& want, int line)
{
  string n;
  for (char c : trim(want)) n += (c == '/' || c == '\\' || (unsigned char)c < 32) ? '_' : c;
  while (!n.empty() && n[0] == '.') n.erase(0, 1);
  if (n.empty()) {
    char buf[32];
    snprintf(buf, sizeof buf, "mol-%03d", line);
    n = buf;
  }
  return n;
}

string jsonString(const string& s)
{
  string o = "\"";
  for (unsigned char c : s) {
    if (c == '"' || c == '\\') { o += '\\'; o += (char)c; }
    else if (c == '\n') o += "\\n";
    else if (c == '\t') o += "\\t";
    else if (c < 32) { char b[8]; snprintf(b, sizeof b, "\\u%04x", c); o += b; }
    else o += (char)c;
  }
  return o + "\"";
}

// A number stays a JSON number so the descriptor table can use it as is.
string jsonValue(const string& s)
{
  const string t = trim(s);
  if (isNumber(t)) {
    char *end = 0;
    const double v = strtod(t.c_str(), &end);
    if (v == v && v - v == 0) return t[0] == '+' ? t.substr(1) : t;  // finite
  }
  return jsonString(t);
}

string measuredJson(const vector<std::pair<string, string> >& m)
{
  string o = "{";
  for (size_t i = 0; i < m.size(); i++)
    o += (i ? ", " : "") + jsonString(m[i].first) + ": " + jsonValue(m[i].second);
  return o + "}";
}

ResourceType *typeNamed(const string& name)
{
  vector<ResourceType*> types = ResourceDescriptor::getResourceDescriptor().getResourceTypes();
  for (ResourceType *t : types) if (t->getName() == name) return t;
  return 0;
}

}  // namespace


bool SmilesBatch::parseTable(const string& text, Table& table, string& why,
                             int header)
{
  table = Table();
  std::istringstream in(text);
  string line;
  vector<std::pair<int, string> > lines;
  int n = 0;
  while (std::getline(in, line)) {
    n++;
    if (!line.empty() && line.back() == '\r') line.pop_back();
    // A UTF-8 byte-order mark, as spreadsheets write it.
    if (n == 1 && line.compare(0, 3, "\xEF\xBB\xBF") == 0) line.erase(0, 3);
    if (trim(line).empty() || trim(line)[0] == '#') continue;
    lines.push_back(std::make_pair(n, line));
  }
  if (lines.empty()) { why = "the table is empty"; return false; }
  const string& first = lines[0].second;
  const size_t tabs = std::count(first.begin(), first.end(), '\t');
  const size_t semis = std::count(first.begin(), first.end(), ';');
  const size_t commas = std::count(first.begin(), first.end(), ',');
  table.delimiter = tabs ? '\t' : semis > commas ? ';' : commas ? ',' : '\t';
  if (!tabs && !semis && !commas) {
    // One column, or name and SMILES separated by blanks (a .smi file).
    table.delimiter = ' ';
  }
  vector<vector<string> > cells;
  size_t width = 0;
  for (auto& l : lines) {
    vector<string> c;
    if (table.delimiter == ' ') {
      std::istringstream ws(l.second);
      string w;
      while (ws >> w) c.push_back(w);
    } else c = splitLine(l.second, table.delimiter);
    width = std::max(width, c.size());
    cells.push_back(c);
  }
  for (auto& c : cells) c.resize(width);
  bool hasHeader = header == 1;
  if (header < 0) {
    hasHeader = true;
    for (const string& c : cells[0])
      if (isNumber(c) || parsesAsSmiles(c)) {
        // "C", "N" or "CO" read as SMILES too: count them only when a
        // header word is absent from the whole line.
        hasHeader = false;
        for (const string& h : cells[0]) {
          const string l = lower(h);
          if (l.find("smiles") != string::npos || l == "name" || l == "id") hasHeader = true;
        }
        break;
      }
  }
  size_t start = 0;
  if (hasHeader) { table.header = cells[0]; start = 1; }
  else
    for (size_t i = 0; i < width; i++) table.header.push_back("column " + std::to_string(i + 1));
  for (size_t i = 0; i < table.header.size(); i++)
    if (table.header[i].empty()) table.header[i] = "column " + std::to_string(i + 1);
  for (size_t i = start; i < cells.size(); i++) {
    table.rows.push_back(cells[i]);
    table.lines.push_back(lines[i].first);
  }
  if (table.rows.empty()) { why = "the table has a header and no rows"; return false; }
  return true;
}


int SmilesBatch::findColumn(const Table& table, const string& key)
{
  for (size_t i = 0; i < table.header.size(); i++)
    if (lower(table.header[i]) == lower(trim(key))) return (int)i;
  if (!key.empty() && std::all_of(key.begin(), key.end(), ::isdigit)) {
    const int k = atoi(key.c_str()) - 1;
    if (k >= 0 && k < (int)table.header.size()) return k;
  }
  return -1;
}


SmilesBatch::Columns SmilesBatch::guessColumns(const Table& table)
{
  Columns c;
  const int w = (int)table.header.size();
  const size_t probe = std::min<size_t>(table.rows.size(), 5);
  for (int i = 0; i < w && c.smiles < 0; i++)
    if (lower(table.header[i]).find("smiles") != string::npos) c.smiles = i;
  for (int i = 0; i < w && c.smiles < 0; i++) {
    bool all = true, numeric = true;
    for (size_t r = 0; r < probe && all; r++) {
      all = parsesAsSmiles(table.rows[r][i]);
      numeric = numeric && isNumber(table.rows[r][i]);
    }
    if (all && !numeric) c.smiles = i;
  }
  static const char *names[] = {"name", "id", "compound", "molecule", "title",
                                "label", "mol", "cmpd", 0};
  for (int k = 0; names[k] && c.name < 0; k++)
    for (int i = 0; i < w; i++)
      if (i != c.smiles && lower(table.header[i]) == names[k]) { c.name = i; break; }
  vector<int> numericCount(w, 0), filled(w, 0);
  for (const auto& row : table.rows)
    for (int i = 0; i < w; i++)
      if (!trim(row[i]).empty()) { filled[i]++; if (isNumber(row[i])) numericCount[i]++; }
  for (int i = 0; i < w && c.name < 0; i++)
    if (i != c.smiles && filled[i] > 0 && numericCount[i] * 2 < filled[i]) c.name = i;
  // A descriptor table (MOE writes a hundred number columns) must not
  // become a hundred measured values: prefer activity-like headers, else
  // all number columns only when there are few.
  vector<int> numeric, activity;
  for (int i = 0; i < w; i++) {
    if (i == c.smiles || i == c.name || filled[i] == 0 || numericCount[i] * 2 < filled[i]) continue;
    numeric.push_back(i);
    const string h = lower(table.header[i]);
    static const char *words[] = {"ic50", "ec50", "ki", "kd", "activity", "response",
                                  "measured", "pic50", "pki", 0};
    // A concentration or time unit in parentheses: "IC50 (uM)", not "logP(o/w)".
    bool act = false;
    const size_t lp = h.find('('), rp = h.find(')', lp == string::npos ? 0 : lp);
    if (lp != string::npos && rp != string::npos) {
      static const std::set<string> units = {"m", "mm", "um", "\xc2\xb5m", "nm", "pm",
          "mol/l", "mg/l", "ug/ml", "mg/ml", "%", "s", "min", "h"};
      act = units.count(trim(h.substr(lp + 1, rp - lp - 1))) > 0;
    }
    for (int k = 0; words[k] && !act; k++) {
      const size_t p = h.find(words[k]);
      const size_t e = p + strlen(words[k]);
      act = p != string::npos && (p == 0 || !isalpha((unsigned char)h[p - 1])) &&
            (e == h.size() || !isalpha((unsigned char)h[e]));
    }
    if (act) activity.push_back(i);
  }
  c.responses = !activity.empty() ? activity : numeric.size() <= 3 ? numeric : vector<int>();
  return c;
}


vector<SmilesBatch::Row> SmilesBatch::preview(const Table& table, const Columns& columns)
{
  vector<Row> rows;
  std::set<string> seen;
  for (size_t r = 0; r < table.rows.size(); r++) {
    const vector<string>& cells = table.rows[r];
    Row row;
    row.line = table.lines[r];
    row.smiles = columns.smiles >= 0 ? trim(cells[columns.smiles]) : string();
    row.name = safeName(columns.name >= 0 ? cells[columns.name] : string(), row.line);
    for (int k : columns.responses)
      if (k >= 0 && k < (int)cells.size())
        row.measured.push_back(std::make_pair(table.header[k], trim(cells[k])));
    FragGraft::Mol m;
    string err;
    if (columns.smiles < 0) row.problem = "no SMILES column chosen";
    else if (row.smiles.empty()) row.problem = "no SMILES";
    else if (!FragGraft::smilesTo3D(row.smiles, m, err)) row.problem = "SMILES not read: " + err;
    else if (m.dummy >= 0) row.problem = "[*] is not a molecule";
    else {
      row.atoms = (int)m.el.size();
      for (int q : m.charge) row.charge += q;
    }
    if (row.problem.empty() && !seen.insert(row.name).second)
      row.problem = "the name " + row.name + " is used twice";
    rows.push_back(row);
  }
  return rows;
}


Fragment *SmilesBatch::structure(const string& smiles, int& charge, string& why)
{
  FragGraft::Mol m;
  if (!FragGraft::smilesTo3D(smiles, m, why)) return 0;
  if (m.dummy >= 0) { why = "[*] is not a molecule"; return 0; }
  Fragment *frag = new Fragment;
  charge = 0;
  for (size_t i = 0; i < m.el.size(); i++) {
    const double c[3] = {m.xyz[i][0], m.xyz[i][1], m.xyz[i][2]};
    frag->addAtom(m.el[i], c);
    frag->atomRef((int)i)->partialCharge(m.charge[i]);
    charge += m.charge[i];
  }
  // The orders the Builder's Clean derives (CleanCmd), so UFF types the
  // atoms as it does for Tools > Add from SMILES.
  vector<std::pair<int, int> > pairs;
  for (const FragGraft::Bond& b : m.bonds) pairs.push_back(std::make_pair(b.a, b.b));
  BondOrderRule::Result orders = BondOrderRule::perceive(m.el, pairs);
  for (size_t i = 0; i < m.bonds.size(); i++)
    new TBond(frag->atomRef(m.bonds[i].a), frag->atomRef(m.bonds[i].b),
              i < orders.orders.size() ? orders.orders[i] : m.bonds[i].order);
  if (frag->numAtoms() > 1) {
    vector<int> all;   // empty: every atom moves
    UFFDesc desc;
    ForceField ff(frag, &all, &desc);
    ff.optimize(100, 0.0001, 5.0);   // CleanCmd's defaults
  }
  frag->charge(charge);
  return frag;
}


string SmilesBatch::defaultTemplate(const EcceURL& folder, string& why)
{
  Resource *dir = EDSIFactory::getResource(folder);
  if (!dir) { why = "no such folder"; return ""; }
  const EcceURL url = folder.getChild(kTemplateName);
  if (dynamic_cast<ICalculation*>(EDSIFactory::getResource(url))) return url.toString();
  ResourceType *type = typeNamed("xtb_es");
  const JCode *code = CodeFactory::lookup("xTB");
  if (!type || !code) { why = "xTB is not registered in this installation"; return ""; }
  Resource *made = dir->createChild(kTemplateName, type);
  TaskJob *job = dynamic_cast<TaskJob*>(made);
  ICalculation *calc = dynamic_cast<ICalculation*>(made);
  if (!job || !calc) { why = "the template could not be created"; return ""; }
  job->application(code);
  TTheory theory("SE", "GFN2-xTB");
  calc->theory(&theory);
  calc->runtype(TRunType("Geometry", false));
  // This computer, as the Launcher fills it in for a first launch here.
  Launchdata l;
  l.machine = "localhost";
  l.nodes = 1;
  l.totalprocs = 1;
  l.threads = 4;
  if (MachinePreferences *p = MachinePreferences::lookup("localhost"))
    l.rundir = p->getProfile("").rundir;
  if (l.rundir.empty()) l.rundir = string(Ecce::realUserHome()) + "/ecce-runs";
  const char *user = getenv("USER");
  l.user = user ? user : "";
  l.remoteShell = "ssh";
  job->launchdata(l);
  made->addProp(ns("annotation"), "Setup of the batch made by Batch Add from SMILES. "
                "Edit it and apply it to the batch again with New Series.");
  return url.toString();
}


vector<SmilesBatch::Outcome> SmilesBatch::make(
    const EcceURL& folder, const string& templ, const vector<Row>& rows,
    string& templateUrl, string& why, bool replace,
    std::function<void(int, int, const string&)> progress)
{
  vector<Outcome> out;
  templateUrl = templ.empty() ? defaultTemplate(folder, why) : templ;
  if (templateUrl.empty()) return out;
  std::error_code ec;
  const ecce::fs::path tmp = ecce::fs::temp_directory_path() /
      ("ecce-batch-smiles-" + std::to_string((long)getpid()) + "-" +
       std::to_string((long)ecce::fs::file_time_type::clock::now().time_since_epoch().count()));
  ecce::fs::create_directories(tmp, ec);
  if (ec) { why = "cannot make a temporary folder: " + ec.message(); return out; }
  for (size_t i = 0; i < rows.size(); i++) {
    const Row& row = rows[i];
    if (progress) progress((int)i, (int)rows.size(), row.name);
    Outcome o;
    o.name = row.name;
    o.smiles = row.smiles;
    o.line = row.line;
    o.note = row.problem;
    if (row.problem.empty()) {
      string w;
      Fragment *frag = structure(row.smiles, o.charge, w);
      if (!frag) o.note = "no structure: " + w;
      else {
        // Series reads charge= from the xyz comment and names the
        // calculation after the file.
        const string xyz = (tmp / (row.name + ".xyz")).string();
        std::ofstream f(xyz.c_str());
        f << frag->numAtoms() << "\ncharge=" << o.charge << "\n";
        char buf[160];
        for (size_t a = 0; a < frag->numAtoms(); a++) {
          const double *c = frag->atomRef((int)a)->coordinates();
          snprintf(buf, sizeof buf, "%-3s %14.8f %14.8f %14.8f\n",
                   frag->atomRef((int)a)->atomicSymbol().c_str(), c[0], c[1], c[2]);
          f << buf;
        }
        f.close();
        delete frag;
        const EcceURL target = folder.getChild(row.name);
        Resource *old = replace && target.toString() != templateUrl
                            ? EDSIFactory::getResource(target) : 0;
        TaskJob *oldJob = dynamic_cast<TaskJob*>(old);
        if (oldJob && oldJob->getState() < ResourceDescriptor::STATE_SUBMITTED)
          old->remove();
        vector<Series::Outcome> r = Series::make(EcceURL(templateUrl), folder, {xyz});
        if (r.empty()) o.note = "not made";
        else if (!r[0].ok) o.note = r[0].note;
        else {
          o.ok = true;
          o.url = r[0].url;
          o.note = r[0].note;
          if (Resource *c = EDSIFactory::getResource(EcceURL(o.url))) {
            c->addProp(ns(kSmilesProp), row.smiles);
            c->addProp(ns(kMeasuredProp), measuredJson(row.measured));
            string note;
            for (const auto& m : row.measured)
              if (!m.second.empty()) note += (note.empty() ? "" : "; ") + m.first + " = " + m.second;
            c->addProp(ns("annotation"), note.empty() ? row.smiles : note);
          }
        }
      }
    }
    out.push_back(o);
  }
  if (progress) progress((int)rows.size(), (int)rows.size(), "");
  ecce::fs::remove_all(tmp, ec);
  return out;
}


string SmilesBatch::json(const string& folder, const string& templ,
                         const vector<Outcome>& outcomes)
{
  int made = 0;
  for (const Outcome& o : outcomes) made += o.ok;
  std::ostringstream os;
  os << "{\"folder\": " << jsonString(folder) << ", \"template\": " << jsonString(templ)
     << ", \"made\": " << made << ", \"failed\": " << (int)outcomes.size() - made
     << ", \"rows\": [";
  for (size_t i = 0; i < outcomes.size(); i++) {
    const Outcome& o = outcomes[i];
    os << (i ? ", " : "") << "{\"line\": " << o.line << ", \"name\": " << jsonString(o.name)
       << ", \"smiles\": " << jsonString(o.smiles) << ", \"made\": " << (o.ok ? "true" : "false")
       << ", \"charge\": " << o.charge << ", \"url\": " << jsonString(o.url)
       << ", \"note\": " << jsonString(o.note) << "}";
  }
  os << "]}";
  return os.str();
}
