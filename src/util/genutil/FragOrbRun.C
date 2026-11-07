#include "util/FragOrbRun.H"

#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "util/Ecce.H"
#include "util/MiniJson.H"

using std::string;
using std::vector;
using J = MiniJson;

namespace {

double num(const J& v)
{
  return v.type() == MiniJson::Number ? atof(v.str().c_str()) : NAN;
}

string quote(const string& v)
{
  string o;
  for (char c : v) o += c == '\'' ? string("'\\''") : string(1, c);
  return "'" + o + "'";
}

// Runs ecce-fragorb; the parsed JSON, or false with its message.
bool run(const string& args, J& j, string& error)
{
#ifdef _WIN32
  (void)args; (void)j;
  error = "Fragment orbitals are not available on this platform yet.";
  return false;
#else
  const char *home = getenv("ECCE_HOME");
  const string exe = (home && *home ? string(home) + "/bin/" : string()) + "ecce-fragorb";
  string text;
  const int status = Ecce::readCommand(quote(exe) + " " + args + " --json 2>&1", text);
  const size_t b = text.find('{');
  string err;
  if (status == 0 && b != string::npos && MiniJson::parse(text.substr(b), j, err) &&
      j.type() == MiniJson::Object)
    return true;
  const size_t e = text.find("ecce-fragorb: ");
  error = e == string::npos ? "ecce-fragorb could not be run."
                            : text.substr(e + 14, text.find('\n', e) - e - 14);
  return false;
#endif
}

string files(const string& complex, const vector<string>& fragments)
{
  string a = quote(complex);
  for (const string& f : fragments) a += " " + quote(f);
  return a;
}

}  // namespace


FragOrbResult runFragOrb(const string& complex, const vector<string>& fragments,
                         const string& method)
{
  FragOrbResult r;
  J j;
  if (!run("analyse " + files(complex, fragments) + " --method " + quote(method), j, r.error))
    return r;
  r.method = j.get("method").str();
  r.threshold = num(j.get("threshold"));
  for (const J& f : j.get("fragments").items()) r.fragments.push_back(f.str());
  for (const J& c : j.get("channels").items()) {
    FragOrbChannel ch;
    ch.spin = (int)num(c.get("spin"));
    for (const J& f : c.get("fo").items()) {
      FragOrbFo fo;
      fo.fragment = (int)num(f.get("fragment"));
      fo.index = (int)num(f.get("index"));
      fo.energy = num(f.get("energy_ev"));
      fo.occupation = num(f.get("occupation"));
      ch.fo.push_back(fo);
    }
    for (const J& m : c.get("mo").items()) {
      FragOrbMo mo;
      mo.index = (int)num(m.get("index"));
      mo.energy = num(m.get("energy_ev"));
      mo.occupation = num(m.get("occupation"));
      mo.remainder = num(m.get("remainder"));
      for (const J& x : m.get("composition").items()) mo.share.push_back(num(x));
      ch.mo.push_back(mo);
    }
    r.channels.push_back(ch);
  }
  r.ok = !r.channels.empty() && !r.channels[0].mo.empty();
  if (!r.ok) r.error = "ecce-fragorb gave no orbitals.";
  return r;
}


FragDosResult runFragDos(const string& complex, const vector<string>& fragments, double fwhm)
{
  FragDosResult r;
  char w[32];
  snprintf(w, sizeof w, "%g", fwhm);
  J j;
  if (!run("dos " + files(complex, fragments) + " --fwhm " + w, j, r.error)) return r;
  r.fermi = num(j.get("fermi_ev"));
  for (const J& s : j.get("sets").items()) r.sets.push_back(s.str());
  for (const J& p : j.get("pairs").items()) r.pairs.push_back(p.str());
  for (const J& x : j.get("occupied_overlap").items()) r.occupiedOverlap.push_back(num(x));
  r.pdos.assign(r.sets.size(), vector<double>());
  r.opdos.assign(r.pairs.size(), vector<double>());
  for (const J& g : j.get("grid").items()) {
    r.energy.push_back(num(g.get("e")));
    r.tdos.push_back(num(g.get("tdos")));
    const vector<J>& p = g.get("pdos").items();
    for (size_t s = 0; s < r.sets.size(); s++) r.pdos[s].push_back(s < p.size() ? num(p[s]) : 0);
    const vector<J>& o = g.get("opdos").items();
    for (size_t s = 0; s < r.pairs.size(); s++) r.opdos[s].push_back(s < o.size() ? num(o[s]) : 0);
  }
  r.ok = !r.energy.empty();
  if (!r.ok) r.error = "ecce-fragorb gave no density of states.";
  return r;
}


FragDiagramResult runFragDiagram(const string& complex, const vector<string>& fragments,
                                 const vector<string>& names, int occupied, int virtuals,
                                 double threshold)
{
  FragDiagramResult r;
  string args = "diagram " + files(complex, fragments);
  for (const string& n : names) args += " --name " + quote(n);
  char w[64];
  snprintf(w, sizeof w, " --window %d,%d --threshold %g", occupied, virtuals, threshold);
  J j;
  if (!run(args + w, j, r.error)) return r;
  r.method = j.get("method").str();
  r.threshold = num(j.get("threshold"));
  r.covered = num(j.get("covered"));
  const vector<J>& win = j.get("window").items();
  if (win.size() == 2) { r.occupied = (int)num(win[0]); r.virtuals = (int)num(win[1]); }
  for (const J& f : j.get("fragments").items()) r.fragments.push_back(f.str());
  for (const J& g : j.get("groups").items()) r.groups.push_back(g.str());
  auto ints = [](const J& a) {
    vector<int> v;
    for (const J& x : a.items()) v.push_back((int)num(x));
    return v;
  };
  for (const J& c : j.get("channels").items()) {
    FragDiagramChannel ch;
    ch.spin = (int)num(c.get("spin"));
    ch.homo = (int)num(c.get("homo"));
    ch.lumo = (int)num(c.get("lumo"));
    ch.gap = num(c.get("gap_ev"));
    ch.representative = c.get("representative").boolean();
    ch.verdict = c.get("verdict").str();
    for (const J& n : c.get("notes").items()) ch.notes.push_back(n.str());
    for (const J& l : c.get("levels").items()) {
      FragDiagramLevel lv;
      lv.id = (int)num(l.get("id"));
      lv.column = (int)num(l.get("column"));
      lv.fragment = (int)num(l.get("fragment"));
      lv.numbers = ints(l.get("numbers"));
      lv.energy = num(l.get("energy_ev"));
      for (const J& o : l.get("occupation").items()) lv.occupation.push_back(num(o));
      lv.position = l.get("position").str();
      lv.kind = l.get("kind").str();
      lv.irrep = l.get("irrep").str();
      lv.partner = l.get("partner").boolean();
      lv.representative = l.get("representative").isNull() || l.get("representative").boolean();
      lv.remainder = l.get("remainder").isNull() ? 0 : num(l.get("remainder"));
      lv.covered = l.get("covered").isNull() ? 0 : num(l.get("covered"));
      lv.overlapPopulation = l.get("overlap_population").isNull() ? 0 : num(l.get("overlap_population"));
      lv.fragments = ints(l.get("fragments"));
      for (const J& s : l.get("shares").items()) {
        FragDiagramShare sh;
        sh.level = (int)num(s.get("level"));
        sh.fragment = (int)num(s.get("fragment"));
        sh.numbers = ints(s.get("numbers"));
        sh.energy = num(s.get("energy_ev"));
        sh.share = num(s.get("share"));
        sh.position = s.get("position").str();
        sh.irrep = s.get("irrep").str();
        lv.shares.push_back(sh);
      }
      ch.levels.push_back(lv);
    }
    for (const J& l : c.get("lines").items()) {
      FragDiagramLine ln;
      ln.fo = (int)num(l.get("fo"));
      ln.mo = (int)num(l.get("mo"));
      ln.fragment = (int)num(l.get("fragment"));
      ln.share = num(l.get("share"));
      ch.lines.push_back(ln);
    }
    r.channels.push_back(ch);
  }
  r.ok = !r.channels.empty() && !r.channels[0].levels.empty();
  if (!r.ok) r.error = "ecce-fragorb drew no levels.";
  return r;
}
