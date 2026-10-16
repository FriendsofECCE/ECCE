#include "util/JobProgress.H"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <sstream>

using std::string;
using std::vector;


string JobProgress::Stage::field(const string& key) const
{
  for (const auto& f : fields)
    if (f.first == key) return f.second;
  return "";
}


static bool number(const string& s, double& v)
{
  if (s.empty()) return false;
  char *end = 0;
  v = strtod(s.c_str(), &end);
  return end && *end == '\0' && std::isfinite(v);
}


bool JobProgress::split(const string& v, double& value, double& target,
                        bool& hasTarget)
{
  const size_t slash = v.find('/');
  hasTarget = false;
  if (!number(v.substr(0, slash), value)) return false;
  if (slash != string::npos)
    hasTarget = number(v.substr(slash + 1), target);
  return true;
}


bool JobProgress::parse(const string& text)
{
  p_stages.clear();
  p_seq = 0;
  p_at = 0;
  std::istringstream in(text);
  string line;
  while (std::getline(in, line)) {
    std::istringstream words(line);
    string head;
    if (!(words >> head)) continue;
    if (head == "#") {
      string w;
      while (words >> w)
        if (w.compare(0, 4, "seq=") == 0) p_seq = atoi(w.c_str() + 4);
        else if (w.compare(0, 3, "at=") == 0) p_at = atof(w.c_str() + 3);
      continue;
    }
    const bool hist = head.size() > 1 && head.back() == '~';
    const string name = hist ? head.substr(0, head.size() - 1) : head;
    Stage *st = 0;
    for (auto& s : p_stages)
      if (s.name == name) st = &s;
    if (!st) {
      p_stages.push_back(Stage());
      st = &p_stages.back();
      st->name = name;
    }
    string w;
    while (words >> w) {
      const size_t eq = w.find('=');
      if (eq == string::npos || eq == 0) continue;
      const string key = w.substr(0, eq), val = w.substr(eq + 1);
      if (hist) {
        st->historyKey = key;
        st->history.clear();
        std::istringstream vals(val);
        string one;
        double d;
        while (std::getline(vals, one, ','))
          if (number(one, d)) st->history.push_back(d);
      } else {
        st->fields.push_back(std::make_pair(key, val));
      }
    }
  }
  //  pw.x ends an md run with the relax "maximum number of steps" line, which
  //  arrives as an opt stage with no step; it is not an optimisation.
  p_stages.erase(std::remove_if(p_stages.begin(), p_stages.end(),
      [](const Stage& s) { return s.name == "opt" && s.field("step").empty() &&
                                  s.field("istep").empty(); }),
      p_stages.end());
  return !p_stages.empty();
}


const JobProgress::Stage* JobProgress::stage(const string& name) const
{
  for (const auto& s : p_stages)
    if (s.name == name) return &s;
  return 0;
}


string JobProgress::title(const string& stage)
{
  if (stage == "opt") return "Optimisation";
  if (stage == "scf") return "SCF";
  if (stage == "freq") return "Frequencies";
  if (stage == "prepare") return "Prepare";
  // GROMACS names the stage by its integrator.
  if (stage == "steep" || stage == "cg" || stage == "l-bfgs" || stage == "nm")
    return "Minimisation";
  if (stage == "md" || stage == "md-vv" || stage == "md-vv-avek" ||
      stage == "sd" || stage == "bd")
    return "Molecular dynamics";
  return stage;
}


static string fmt(const char *f, double v)
{
  char b[64];
  snprintf(b, sizeof b, f, v);
  return b;
}


static string energyText(const string& v)
{
  double e;
  if (!number(v, e)) return v;
  return fmt("%.8f", e) + " hartree";
}


// The criteria labels a user reads; unknown keys keep their own name.
static string criterionLabel(const string& stage, const string& key)
{
  if (key == "maxgrad") return "max force";
  if (key == "rmsgrad") return "RMS force";
  if (key == "maxstep") return "max step";
  if (key == "rmsstep") return "RMS step";
  if (key == "dE") return "\xce\x94" "E";   // Delta E
  if (key == "scferr") return "estimated SCF error";
  if (key == "grad") return stage == "scf" ? "orbital gradient" : "gradient";
  return key;
}


string JobProgress::updatedText(double seconds)
{
  if (seconds < 0) seconds = 0;
  if (seconds < 90) return fmt("updated %.0f s ago", seconds);
  if (seconds < 5400) return fmt("updated %.0f min ago", seconds / 60);
  return fmt("updated %.1f h ago", seconds / 3600);
}


static string agoText(double seconds)
{
  if (seconds < 0) seconds = 0;
  if (seconds < 90) return fmt("%.0f s", seconds);
  if (seconds < 5400) return fmt("%.0f min", std::floor(seconds / 60));
  return fmt("%.1f h", seconds / 3600);
}


string JobProgress::noContactText(double seconds)
{
  return "\xe2\x9a\xa0 no contact for " + agoText(seconds);   // U+26A0
}


string JobProgress::lastHeardText(double when, double now)
{
  time_t t = (time_t)when;
  struct tm tms;
  localtime_r(&t, &tms);
  char hm[16];
  strftime(hm, sizeof hm, "%H:%M", &tms);
  return string(hm) + " (" + agoText(now - when) + " ago)";
}


// The Prepare script announces gmx tool names; the box says what each makes.
static string prepareStepName(const string& tool)
{
  if (tool == "editconf") return "box";
  if (tool == "genion") return "ions";
  return tool;
}


static string prepareBoxName(const string& box)
{
  if (box == "dodecahedron") return "rhombic dodecahedron";
  if (box == "octahedron") return "truncated octahedron";
  return box;
}


// The Prepare outcome: one line for the system, one for charge and box.
static vector<string> prepareSummary(const JobProgress::Stage& st, bool completed)
{
  vector<string> out;
  double k = 0, n = 0;
  bool hasN = false;
  JobProgress::split(st.field("point"), k, n, hasN);
  if (st.field("done") != "yes") {
    string line = completed ? "Prepare stopped" : "Prepare";
    if (hasN)
      line += fmt(" at step %.0f", k) + fmt(" of %.0f", n) + " (" +
              prepareStepName(st.field("tool")) + ")";
    out.push_back(line);
    return out;
  }
  const string dot = " \xc2\xb7 ";
  string a = "Prepare complete";
  if (!st.field("atoms").empty()) a += dot + st.field("atoms") + " atoms";
  if (!st.field("residues").empty())
    a += dot + st.field("residues") + " protein residues";
  if (!st.field("waters").empty()) a += dot + st.field("waters") + " waters";
  if (!st.field("pos").empty())
    a += dot + st.field("pos") + " " + st.field("posname") + ", " +
         st.field("neg") + " " + st.field("negname");
  out.push_back(a);
  string b;
  double c0, c1;
  if (number(st.field("charge0"), c0) && number(st.field("charge1"), c1))
    b = fmt("Net charge %.0f", c0) + fmt(" to %.0f", c1);
  if (!st.field("box").empty()) {
    b += string(b.empty() ? "" : dot) + prepareBoxName(st.field("box")) + " box " +
         st.field("a") + " \xc3\x97 " + st.field("b") + " \xc3\x97 " +
         st.field("c") + " nm, angles " + st.field("alpha") + "/" +
         st.field("beta") + "/" + st.field("gamma");
  }
  if (!b.empty()) out.push_back(b);
  return out;
}


JobProgress::Block JobProgress::stageBlock(const Stage& st, double age,
                                           bool finished) const
{
  Block b;
  b.title = title(st.name);
  const string rec = !st.field("cycle").empty() ? "cycle"
                   : !st.field("step").empty() ? "step"
                   : !st.field("point").empty() ? "point"
                   // a step number printed after its criteria (QE): the
                   // monitor cannot take it as a record, the box shows it
                   : !st.field("istep").empty() ? "istep" : "";
  const string recName = rec == "istep" ? "step" : rec;
  string first;
  if (rec == "point") {
    double k = 0, n = 0;
    bool hasN;
    if (split(st.field("point"), k, n, hasN) && hasN && n > 0) {
      b.bar = true;
      b.fraction = std::min(1.0, std::max(0.0, k / n));
      first = string(st.name == "freq" ? "point" : "step") + fmt(" %.0f", k) +
              fmt(" of %.0f", n);
      if (st.name == "prepare") {
        // step k is under way, so the bar counts the finished ones
        b.fraction = st.field("done") == "yes" ? 1.0 : (k - 1) / n;
        if (!st.field("tool").empty())
          first += " \xc2\xb7 " + prepareStepName(st.field("tool"));
        if (st.field("done") == "yes") first += " \xc2\xb7 done";
      }
      double eta;
      if (st.field("done") == "yes") {
        double real, imag;
        if (number(st.field("real"), real) &&
            number(st.field("imaginary"), imag))
          first += fmt(" \xc2\xb7 %.0f real", real) +
                   fmt(", %.0f imaginary", imag);
      } else if (st.name != "prepare" && !finished &&
                 number(st.field("eta"), eta) && eta > 0) {
        const double left = std::max(0.0, eta - age);
        first += left < 90 ? fmt(" \xc2\xb7 about %.0f s left", left)
                           : fmt(" \xc2\xb7 about %.0f min left", left / 60);
      }
    }
  } else if (!rec.empty()) {
    first = recName + " " + st.field(rec);
  }
  if (!st.field("energy").empty())
    first += string(first.empty() ? "" : " \xc2\xb7 ") + "E = " +
             energyText(st.field("energy"));
  //  The step limit is a ceiling, not a target: a converged stage is
  //  finished, so its bar is full whatever step it stopped at.
  if (st.field("converged") == "yes") {
    if (b.bar) b.fraction = 1.0;
    first += string(first.empty() ? "" : " \xc2\xb7 ") + "converged";
  } else if (st.field("converged") == "no")
    first += string(first.empty() ? "" : " \xc2\xb7 ") + "not converged";
  if (!first.empty()) b.lines.push_back(first);

  for (const auto& f : st.fields) {
    double v, t;
    bool hasT;
    if (f.first == rec || f.first == "energy" || f.first == "eta") continue;
    if (!split(f.second, v, t, hasT)) continue;
    if (!hasT) {
      // A value whose target the code never prints: shown as a plain line.
      const string label = criterionLabel(st.name, f.first);
      if (label != f.first)
        b.lines.push_back(label + " " + fmt("%.1e", v));
      continue;
    }
    Criterion c;
    c.label = criterionLabel(st.name, f.first);
    c.value = fmt("%.1e", v) + " / " + fmt("%.0e", t);
    c.met = std::fabs(v) <= t ? 1 : 0;
    b.criteria.push_back(c);
  }

  if (st.history.size() >= 2) {
    if (st.historyKey == "energy") {
      b.plot = st.history;
      b.plotLabel = "energy per step";
    } else {
      for (double v : st.history)
        b.plot.push_back(std::log10(std::max(std::fabs(v), 1e-16)));
      b.plotLog = true;
      b.plotLabel = "log |" + criterionLabel(st.name, st.historyKey) +
                    "| per " + (rec.empty() ? "record" : recName);
    }
  }
  return b;
}


vector<JobProgress::Block> JobProgress::running(double age, bool finished) const
{
  // The outer loops first: optimisation, then frequencies, then the SCF
  // inside them; other stages in the order they appeared.
  vector<Block> out;
  static const char *order[] = {"opt", "freq", "scf"};
  for (const char *name : order)
    if (const Stage *st = stage(name)) out.push_back(stageBlock(*st, age, finished));
  for (const auto& st : p_stages)
    if (st.name != "opt" && st.name != "freq" && st.name != "scf")
      out.push_back(stageBlock(st, age, finished));
  return out;
}


vector<string> JobProgress::summary(bool completed) const
{
  vector<string> out;
  const Stage *opt = stage("opt"), *freq = stage("freq"), *scf = stage("scf");
  if (opt) {
    const string step = !opt->field("step").empty() ? opt->field("step")
                                                   : opt->field("istep");
    string line;
    if (opt->field("converged") == "yes")
      line = "Optimisation converged in " + step + " steps";
    else if (opt->field("converged") == "no")
      line = "Optimisation not converged after " + step + " steps";
    else
      line = "Optimisation stopped at step " + step;
    string e = opt->field("energy");
    // MOPAC's history is the gradient, not an energy.
    if (e.empty() && !opt->history.empty() && opt->historyKey == "energy")
      e = fmt("%.10f", opt->history.back());
    if (!e.empty()) line += " \xc2\xb7 E = " + energyText(e);
    out.push_back(line);
  }
  if (freq) {
    double k = 0, n = 0, real, imag, zpe;
    bool hasN = false;
    split(freq->field("point"), k, n, hasN);
    if (freq->field("done") == "yes" && number(freq->field("real"), real) &&
        number(freq->field("imaginary"), imag)) {
      string line = fmt("Frequencies: %.0f real", real) +
                    fmt(", %.0f imaginary", imag);
      if (number(freq->field("zpe"), zpe))
        line += " \xc2\xb7 ZPE " + fmt("%.6f", zpe) + " hartree";
      out.push_back(line);
    } else if (freq->field("done") == "yes") {
      // Codes that do not say how many modes are imaginary.
      string line = "Frequencies: done";
      if (number(freq->field("zpe"), zpe))
        line += " \xc2\xb7 ZPE " + fmt("%.6f", zpe) + " hartree";
      out.push_back(line);
    } else if (hasN) {
      out.push_back(fmt("Frequencies stopped at point %.0f", k) +
                    fmt(" of %.0f", n));
    }
  }
  if (const Stage *prep = stage("prepare")) {
    vector<string> p = prepareSummary(*prep, completed);
    out.insert(out.end(), p.begin(), p.end());
  }
  // Runs counted in steps (GROMACS): where they got to.
  for (const auto& st : p_stages) {
    if (st.name == "opt" || st.name == "freq" || st.name == "scf" ||
        st.name == "prepare") continue;
    double k = 0, n = 0;
    bool hasN = false;
    if (!split(st.field("point"), k, n, hasN)) continue;
    string line;
    if (st.field("converged") == "yes")
      line = title(st.name) + " converged in " + fmt("%.0f", k) + " steps";
    else if (st.field("converged") == "no")
      line = title(st.name) + " did not converge in " + fmt("%.0f", k) + " steps";
    else
      line = title(st.name) + ": " + fmt("%.0f", k) +
             (hasN ? fmt(" of %.0f", n) : string()) + " steps";
    out.push_back(line);
  }
  if (!opt && !freq && scf) {
    const string cyc = scf->field("cycle");
    string line;
    // MOPAC prints no SCF cycles unless asked to.
    if (scf->field("converged") == "yes" && cyc.empty())
      line = "SCF converged";
    else if (scf->field("converged") == "yes")
      line = "SCF converged in " + cyc + " cycles";
    else if (scf->field("converged") == "no")
      line = "SCF not converged after " + cyc + " cycles";
    else
      line = "SCF stopped at cycle " + cyc;
    if (!scf->field("energy").empty())
      line += " \xc2\xb7 E = " + energyText(scf->field("energy"));
    out.push_back(line);
  }
  if (out.empty() && !completed) {
    for (const auto& b : running(0))
      if (!b.lines.empty())
        out.push_back(b.title + ": " + b.lines.front());
  }
  return out;
}


// 4.1e-05 -> "4e\xe2\x88\x925": one digit and the exponent, as a cell has room for.
static string shortNumber(const string& text)
{
  double v;
  if (!number(text.substr(0, text.find('/')), v) || v == 0) return "0";
  char b[32];
  snprintf(b, sizeof b, "%.0e", fabs(v));
  string out = b;                       // "4e-05"
  const size_t e = out.find('e');
  const bool neg = out[e + 1] == '-';
  const string digits = out.substr(e + 2);
  const int exp = atoi(digits.c_str());
  return out.substr(0, e) + "e" + (neg ? "\xe2\x88\x92" : "") + std::to_string(exp);
}


string JobProgress::shortLine() const
{
  const Stage *current = 0;
  for (const auto& st : p_stages)
    if (st.name == "opt" || st.name == "freq") current = &st;
  const Stage *scf = stage("scf");
  const string dot = " \xc2\xb7 ";

  string scfPart;
  if (scf && !scf->field("cycle").empty()) {
    scfPart = "SCF " + scf->field("cycle");
    if (!current && !scf->field("dE").empty())
      scfPart += dot + "\xce\x94" "E " + shortNumber(scf->field("dE"));
  }

  if (current && current->name == "opt") {
    const string step = !current->field("step").empty() ? current->field("step")
                                                         : current->field("istep");
    string line = "Opt";
    if (!step.empty()) line += " step " + step;
    if (current->field("converged") == "yes") return line + dot + "converged";
    if (!scfPart.empty()) line += dot + scfPart;
    return line;
  }
  if (current) {                          // freq
    double k, n;
    bool hasN;
    if (split(current->field("point"), k, n, hasN) && hasN && n > 0)
      return fmt("Freq %.0f", k) + fmt("/%.0f", n);
    if (current->field("done") == "yes") return "Freq done";
    if (!current->field("step").empty()) return "Freq step " + current->field("step");
    return scfPart.empty() ? "Freq" : "Freq" + dot + scfPart;
  }
  if (!scfPart.empty()) return scfPart;

  for (const auto& st : p_stages) {       // runs counted in steps (GROMACS)
    double k, n;
    bool hasN;
    if (st.name == "prepare" && split(st.field("point"), k, n, hasN))
      return st.field("done") == "yes" ? string("Prepare done")
                                       : "Prepare" + fmt(" %.0f", k) +
                                         (hasN ? fmt("/%.0f", n) : string());
    if (st.name != "scf" && split(st.field("point"), k, n, hasN))
      return (title(st.name) == "Molecular dynamics" ? string("MD") : title(st.name)) +
             fmt(" %.0f", k) + (hasN ? fmt("/%.0f", n) : string());
  }
  return "";
}
