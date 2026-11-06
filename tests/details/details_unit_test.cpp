// wx-free tests of the Details engine: parser, expressions, ranges, number
// formatting, and each .dlg construct through the Model with a recording
// transport (tests/dialogs/grammar_test.dlg).
#include "DetailsModel.h"

#include <cstdio>
#include <fstream>
#include <sstream>

using namespace details;
using std::string;

static int failures = 0;
#define CHECK(c) do { if (!(c)) { failures++; \
  fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); } } while (0)
#define CHECK_EQ(a, b) do { string x_ = (a), y_ = (b); if (x_ != y_) { \
  failures++; fprintf(stderr, "FAIL line %d: %s\n  got  [%s]\n  want [%s]\n", \
  __LINE__, #a, x_.c_str(), y_.c_str()); } } while (0)

struct Recorder : Transport, View {
  Strings lines, statuses;
  bool closed = false;
  void send(const string& d) override
  {
    for (const string& l : split(d, '\n'))
      if (!l.empty()) lines.push_back(l);
  }
  void itemUpdated(int) override {}
  void status(const string& t, Severity s) override
  {
    statuses.push_back(std::to_string(s) + ":" + t);
  }
  void resetEnabled(bool) override {}
  void layoutChanged() override {}
  void closeRequested() override { closed = true; }
};

static Params params(const string& category = "DFT", const string& runtype = "Energy")
{
  Params p;
  p.vars = {{"category", category}, {"theory", "HF"}, {"runtype", runtype},
            {"mult", "1"}, {"nfrozen", "3"}, {"nocc", "5"}};
  p.restoreFlag = "GUIValues";
  p.readOnly = "Writable";
  return p;
}

static Description load()
{
  Description d;
  if (!d.load("../dialogs/grammar_test.dlg")) {
    fprintf(stderr, "%s\n", d.error.c_str());
    exit(1);
  }
  return d;
}

static void expressions()
{
  Lists lists = {{"L", {"a", "b"}}};
  std::map<string, string> v = {{"x", "5"}, {"s", "DFT"}, {"e", ""}, {"T.y", "1"}};
  Lookup lk = [&v](const string& n, string& out) {
    if (!v.count(n)) return false;
    out = v[n];
    return true;
  };
  auto E = [&](const string& e) { return evalExpr(e, lk, lists); };
  CHECK(E(""));
  CHECK(E("s==DFT"));
  CHECK(!E("s!=DFT"));
  CHECK(E("x>4 && x<=5"));
  CHECK(E("x>10 || s==DFT"));
  CHECK(E("x == 5.0"));                     // numbers compare as numbers
  CHECK(E("!e"));
  CHECK(E("T.y"));
  CHECK(E("(x>10 || x<6) && s==DFT"));
  CHECK(!E("x>10 || x<6 && s==SCF"));       // && binds tighter than ||
  v["s"] = "a";
  CHECK(E("s in @L"));
  CHECK(!E("s not in @L"));
  v["s"] = "z";
  CHECK(!E("s in @L"));
  CHECK(E("s not in @L"));
  CHECK(E("x == $x"));                      // $ on the right is a variable
  string err;
  evalExpr("s in @NOPE", lk, lists, &err);
  CHECK(!err.empty());
  err.clear();
  evalExpr("x = 5", lk, lists, &err);
  CHECK(!err.empty());
  CHECK_EQ(substitute("[0..$x] $nope", lk), "[0..5] $nope");
}

static void numbers()
{
  CHECK_EQ(pyFloatStr(1.0), "1.0");
  CHECK_EQ(pyFloatStr(0.1), "0.1");
  CHECK_EQ(pyFloatStr(-2.5), "-2.5");
  CHECK_EQ(pyFloatStr(100.0), "100.0");
  CHECK_EQ(pyFloatStr(0.0001), "0.0001");
  CHECK_EQ(pyFloatStr(0.00001), "1e-05");
  CHECK_EQ(pyFloatStr(1e-6), "1e-06");
  CHECK_EQ(pyFloatStr(1e16), "1e+16");
  CHECK_EQ(pyFloatStr(1e15), "1000000000000000.0");
  CHECK_EQ(pyFloatStr(1.5e-7), "1.5e-07");
  CHECK_EQ(pyFloatStr(0.0), "0.0");

  bool hl, hh;
  double lo, hi;
  CHECK(rangeBounds("[0..5]", hl, lo, hh, hi) && hl && hh && lo == 0 && hi == 5);
  CHECK(rangeBounds("(0..)", hl, lo, hh, hi) && hl && !hh);
  CHECK(rangeBounds("", hl, lo, hh, hi) && !hl && !hh);
  CHECK(!rangeBounds("5", hl, lo, hh, hi));
  string m;
  CHECK(inRange("[0..5]", 5, m));
  CHECK(!inRange("[0..5]", 6, m)); CHECK_EQ(m, "less than or equal to 5.");
  CHECK(!inRange("(0..5]", 0, m)); CHECK_EQ(m, "greater than 0.");
  CHECK(!inRange("[1..5)", 5, m)); CHECK_EQ(m, "less than 5.");
  CHECK(!inRange("[1..5]", -1, m)); CHECK_EQ(m, "greater than or equal to 1.");
  CHECK(!inRange("[1.0..)", 0, m)); CHECK_EQ(m, "greater than or equal to 1.0.");
  CHECK(!inRange("(0..0.5]", 1, m)); CHECK_EQ(m, "less than or equal to 0.5.");
  CHECK(inRange("", 1e9, m));
}

static void parser()
{
  Description d;
  std::istringstream in("# c\nlist A = x | y z|w   # trailing\n"
                        "combo name=N choices=@A label=\"a b # not a comment\"\n");
  CHECK(d.parse(in));
  CHECK(d.lists["A"].size() == 3 && d.lists["A"][1] == "y z");
  CHECK(d.lines.size() == 1);
  CHECK_EQ(d.lines[0].get("label"), "a b # not a comment");
  std::istringstream bad("list = a|b\n");
  Description e;
  CHECK(!e.parse(bad));
}

static void build()
{
  Description d = load();
  Params p = params();
  Model m(d, p);
  string err;
  CHECK(m.build(err));
  CHECK_EQ(m.items()[0].name, "T.Func");
  CHECK(m.items()[0].choices.size() == 4);                 // choices=@FUNCS
  CHECK_EQ(m.items()[0].def, "PBE");                       // default by name
  CHECK(m.find("T.Hidden") < 0);                           // box when= false
  CHECK_EQ(m.items()[m.find("T.Core")].soft, "[0..3]");    // $nfrozen
  CHECK_EQ(m.items()[m.find("T.Core")].hard, "[0..5]");    // $nocc
  CHECK_EQ(m.boxes()[0].label, "Functional for HF");
  Params pn = params();
  pn.vars["nocc"] = "-1";                                  // hard_if
  Model mn(d, pn);
  CHECK(mn.build(err));
  CHECK_EQ(mn.items()[mn.find("T.Core")].hard, "[0..0]");

  Model m2(d, params("DFT", "Opt"));
  CHECK(m2.build(err));
  CHECK_EQ(m2.items()[0].def, "B3LYP");                    // default_if, 1st
  Model m3(d, params("UHF"));
  CHECK(m3.build(err));
  CHECK_EQ(m3.items()[0].def, "SVWN");                     // default_if, 2nd
  Model m4(d, params("SCF"));                              // not in @OPENSHELL
  CHECK(m4.build(err));
  CHECK(m4.find("T.Func") < 0 && m4.find("T.Flag") < 0 && m4.find("T.Core") >= 0);
  Params p5 = params();
  p5.vars["mult"] = "0";                                   // mult >= 1 false
  Model m5(d, p5);
  CHECK(m5.build(err));
  CHECK(m5.find("T.Func") < 0);
}

static void live()
{
  Description d = load();
  Recorder r;
  Model m(d, params());
  string err;
  CHECK(m.build(err));
  m.attach(&r, &r);
  m.initialize();
  // every setting, then the dependency pass reports Core and Pair becoming
  // disabled (combo/number widgets report that, toggles do not), then done
  CHECK_EQ(r.lines[0], "T.Func|PBE||1|1|menu_input");     // export_if: default!=@0
  CHECK_EQ(r.lines[2], "T.Pair|0||1|1|toggle_input");
  CHECK_EQ(r.lines[3], "T.Core|0|orbitals|1|0|integer_input");
  CHECK_EQ(r.lines[4], "T.Conv|1.0|Hartree|1|0|float_input");
  CHECK_EQ(r.lines[r.lines.size() - 2], "T.Core|0|orbitals|0|0|integer_input");
  CHECK_EQ(r.lines.back(), "#INITIALIZED");
  CHECK(!m.items()[m.find("T.Pair")].enabled);
  CHECK(m.items()[m.find("T.Guess")].choices.size() == 1);  // "(none)" only

  r.lines.clear();
  m.userEdit(m.find("T.Flag"), "1");                       // enable_if
  // Core is enabled during the nested pass, so its line precedes Flag's
  CHECK(r.lines.size() == 2);
  CHECK_EQ(r.lines[0], "T.Core|0|orbitals|1|0|integer_input");
  CHECK_EQ(r.lines[1], "T.Flag|1||1|1|toggle_input");
  CHECK(m.items()[m.find("T.Pair")].enabled);

  r.lines.clear();
  m.userEdit(m.find("T.Core"), "9");                       // hard clamp
  CHECK_EQ(m.items()[m.find("T.Core")].value, "5");
  CHECK_EQ(r.lines.back(), "T.Core|5|orbitals|1|1|integer_input");
  bool warned = false, said = false;
  for (const string& s : r.statuses) {
    if (s.compare(0, 2, "1:") == 0) warned = true;        // soft [0..3]
    if (s.find("Frozen core above 3 of 5") != string::npos) said = true;
  }
  CHECK(warned && said);                                    // status text, $nocc
  // the non-silent force of Conv=2.5 sent its own line, before Core's
  CHECK_EQ(r.lines[0], "T.Conv|2.5|Hartree|1|1|float_input");
}

static void floats()
{
  Description d = load();
  Recorder r;
  Model m(d, params());
  string err;
  m.build(err);
  m.attach(&r, &r);
  m.initialize();
  r.lines.clear();
  r.statuses.clear();
  int c = m.find("T.Conv");
  m.userEdit(c, "20");                                     // hard [0..10]
  CHECK(r.lines.empty());
  CHECK_EQ(m.items()[c].value, "1.0");
  CHECK(r.statuses.size() == 1 && r.statuses[0].compare(0, 8, "2:Error:") == 0);
  m.userEdit(c, "abc");
  CHECK(r.lines.empty() && m.items()[c].value == "1.0");
  m.userEdit(c, "1.5");                                    // soft (0..1]: warns
  CHECK(r.statuses.back().compare(0, 10, "1:Warning:") == 0);
  CHECK_EQ(r.lines.size() ? r.lines[0] : "", "T.Conv|1.5|Hartree|1|1|float_input");
  r.lines.clear();
  m.userEdit(c, "1.50");                                   // reformatted, same
  CHECK_EQ(m.items()[c].value, "1.5");
  CHECK(r.lines.empty());
  int e = m.find("T.Exp");                                 // exp=1: power of ten
  r.lines.clear();
  m.userEdit(e, "3e-06");
  CHECK(r.lines.empty() && m.items()[e].value == "1e-08");
  CHECK(r.statuses.back().find("power of 10") != string::npos);
  m.userEdit(e, "1e-06");
  CHECK_EQ(r.lines.size() ? r.lines[0] : "", "T.Exp|1e-06||1|1|float_input");
}

static void silentForce()
{
  Description d = load();
  Recorder r;
  Model m(d, params());
  string err;
  m.build(err);
  m.attach(&r, &r);
  m.initialize();
  r.lines.clear();
  m.userEdit(m.find("T.Func"), "PBE0");
  CHECK_EQ(m.items()[m.find("T.Flag")].value, "1");        // forced ...
  bool flagSent = false;
  for (const string& l : r.lines)
    if (l.compare(0, 7, "T.Flag|") == 0) flagSent = true;
  CHECK(!flagSent);                                        // ... silently
  CHECK_EQ(r.lines.back(), "T.Func|PBE0||1|1|menu_input");
}

static void calcpick()
{
  string cand = "grammar_candidates.tmp";
  { std::ofstream f(cand); f << "opt1\tfile:///p/opt1\toptimised\nbad line\n"
                              << "in1\tfile:///p/in1\tinput\n"; }
  Description d = load();
  Params p = params();
  p.candidatesFile = cand;
  Recorder r;
  Model m(d, p);
  string err;
  m.build(err);
  m.attach(&r, &r);
  int g = m.find("T.Guess");
  const Item& it = m.items()[g];
  CHECK(it.choices.size() == 3);
  CHECK_EQ(it.choices[0], "(none)");
  CHECK_EQ(it.choices[1], "opt1 (optimised structure)");
  CHECK_EQ(it.urls[2], "file:///p/in1");
  m.initialize();
  r.lines.clear();
  m.userEdit(g, "file:///p/in1");
  CHECK_EQ(r.lines.back(), "T.Guess|file:///p/in1||1|1|menu_input");
  m.userEdit(g, "file:///nope");                           // unknown -> (none)
  CHECK_EQ(m.items()[g].value, "");
  remove(cand.c_str());
}

static void restore()
{
  string rf = "grammar_restore.tmp";
  { std::ofstream f(rf);
    f << "T.Func|PBE0||1|1|menu_input\n"
      << "T.Flag|1||0|1|toggle_input\n"          // stored disabled flag
      << "T.Conv|7.5|Bohr|1|1|float_input\n"     // stale unit
      << "T.Nothing|1||1|1|text_input\n"         // unknown key
      << "T.Text|xyz||1|1|text_input\n"
      << "END_GUIValues\n"; }
  Description d = load();
  Params p = params();
  p.restoreFile = rf;
  Recorder r;
  Model m(d, p);
  string err;
  m.build(err);
  m.attach(&r, &r);
  m.initialize();
  CHECK_EQ(m.items()[m.find("T.Func")].value, "PBE0");
  CHECK_EQ(m.items()[m.find("T.Conv")].value, "1.0");      // not applied
  CHECK_EQ(m.items()[m.find("T.Text")].value, "xyz");
  CHECK_EQ(r.lines.back(), "T.Conv|1.0|Hartree|1|0|float_input");  // re-sent
  CHECK_EQ(r.lines[r.lines.size() - 2], "#INITIALIZED");

  // the headless defaults pass: one batch, then #CLOSING
  Params q = params();
  q.restoreFlag = "NO_GUIValues";
  Recorder r2;
  Model m2(d, q);
  m2.build(err);
  m2.attach(&r2, &r2);
  m2.initialize();
  CHECK(r2.closed);
  CHECK_EQ(r2.lines.back(), "#CLOSING");
  CHECK_EQ(r2.lines[r2.lines.size() - 2], "#INITIALIZED");

  // reset reports every setting and restores the defaults
  r.lines.clear();
  m.reset();
  CHECK_EQ(m.items()[m.find("T.Func")].value, "PBE");
  CHECK(r.lines.size() >= 6);
  remove(rf.c_str());
}

static void inventory()
{
  Description d = load();
  Model m(d, params());
  string err;
  m.build(err);
  string j = m.inventoryJson();
  CHECK(j.find("\"name\": \"T.Core\"") != string::npos);
  CHECK(j.find("\"hardRange\": \"[0..5]\"") != string::npos);
  CHECK(j.find("\"choices\": [\"SVWN\", \"PBE\", \"B3LYP\", \"PBE0\"]") != string::npos);
  CHECK(j.find("\"enabled\": false") != string::npos);   // Core, before Flag
}

int main()
{
  expressions();
  numbers();
  parser();
  build();
  live();
  floats();
  silentForce();
  calcpick();
  restore();
  inventory();
  if (failures) { fprintf(stderr, "%d failure(s)\n", failures); return 1; }
  printf("details_unit: ok\n");
  return 0;
}
