// ecce-fragorb: fragment-orbital analysis and OPDOS from results.h5 (#201, #171).
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>

#include "tdat/FragmentAnalysis.H"
#include "tdat/ResultsH5.H"

using namespace fragorb;
using resultsh5::Model;

static const double HARTREE_EV = 27.211386245988;

static const char* USAGE =
"ecce-fragorb: fragment-orbital analysis from real fragment calculations\n"
"\n"
"  ecce-fragorb analyse <complex> <fragment>... [--threshold 0.10]\n"
"               [--method mulliken|scpa] [--allow-uncovered] [--top N] [--json]\n"
"  ecce-fragorb dos <complex> [<fragment>...] [--set NAME=ATOMS]... [--pair A:B]...\n"
"               [--fwhm EV] [--shape gaussian|lorentzian] [--emin EV --emax EV]\n"
"               [--step EV] [--json]\n"
"\n"
"<complex> and <fragment> are calculation folders (their results.h5, or\n"
"Outputs/results.h5) or .h5 files.  All calculations must be at the same\n"
"geometry (atoms within 1e-3 Angstrom), with the same basis on shared atoms.\n"
"Fragments must not share atoms and must cover the complex.\n"
"\n"
"analyse  Expands every complex MO in the fragments' own orbitals (FOs).\n"
"         Per MO: energy, occupation, the FOs with a share >= --top-threshold\n"
"         (fragment, FO number, FO energy, %), and the remainder (the part\n"
"         outside the span of the FOs).  Then the connection list: every\n"
"         FO -> MO pair whose share is >= --threshold.  Energies in eV.\n"
"         --top N      list at most N FOs per MO (default 4)\n"
"dos      PDOS, TDOS and OPDOS from the complex's own S.  Atom sets come from\n"
"         the fragment calculations (named after their folder) or --set\n"
"         NAME=1,2,5-8 (1-based complex atoms).  --pair A:B picks OPDOS pairs\n"
"         (default: all).  Gaussian or Lorentzian broadening of FWHM\n"
"         --fwhm (default 0.3 eV); grid --emin/--emax/--step in eV (default\n"
"         HOMO-20 to LUMO+10, 0.05).  Closed-shell MOs count twice.\n"
"\n"
"--levels           (analyse) add the shares of degenerate FOs of one fragment (within 1e-4 Eh)\n"
"                   and list levels, e.g. the five d orbitals of an atom, instead of single FOs.\n"
"--allow-uncovered  analyse although some complex atoms are in no fragment.\n"
"--json             machine-readable output.\n";

static bool isDir(const string& p) { struct stat st; return !stat(p.c_str(), &st) && S_ISDIR(st.st_mode); }
static bool isFile(const string& p) { struct stat st; return !stat(p.c_str(), &st) && S_ISREG(st.st_mode); }
static string base(string p) {
  while (p.size() > 1 && p[p.size()-1] == '/') p.erase(p.size()-1);
  size_t k = p.find_last_of('/');
  return k == string::npos ? p : p.substr(k+1);
}

static bool load(const string& arg, Model& m, string& name, string& err) {
  string path = arg;
  name = base(arg);
  if (isDir(arg)) {
    path = arg + "/results.h5";
    if (!isFile(path)) path = arg + "/Outputs/results.h5";
    if (!isFile(path)) { err = arg + ": no results.h5 or Outputs/results.h5 in the calculation folder"; return false; }
  } else {
    if (!isFile(arg)) { err = arg + ": no such file or folder"; return false; }
    if (name == "results.h5") {
      string d = arg.substr(0, arg.size() - name.size());
      if (!d.empty()) { name = base(d); if (name == "Outputs") { size_t k = d.rfind("Outputs"); name = base(d.substr(0, k)); } }
    } else if (name.size() > 3 && name.substr(name.size()-3) == ".h5") name = name.substr(0, name.size()-3);
  }
  if (!resultsh5::readFile(path, m, err)) { err = path + ": " + err; return false; }
  if (!m.checks.empty() && m.checks.compare(0, 6, "failed") == 0) { err = path + ": results.h5 is marked " + m.checks; return false; }
  return true;
}

static string fmt(const char* f, int a, int b = 0) {
  char buf[64];
  snprintf(buf, sizeof buf, f, a, b);
  return buf;
}

static string jstr(const string& s) {
  string r = "\"";
  for (size_t i = 0; i < s.size(); i++) {
    if (s[i] == '"' || s[i] == '\\') r += '\\';
    r += s[i];
  }
  return r + "\"";
}

static int analyseCmd(const vector<string>& pos, double threshold, string method,
                      bool allow, bool json, int top, double topThreshold, bool levels) {
  if (pos.size() < 2) { fprintf(stderr, "analyse needs a complex and at least one fragment\n"); return 2; }
  Model cx;
  vector<Model> fm(pos.size() - 1);
  vector<NamedModel> frags;
  string name, err;
  if (!load(pos[0], cx, name, err)) { fprintf(stderr, "ecce-fragorb: %s\n", err.c_str()); return 1; }
  for (size_t i = 1; i < pos.size(); i++) {
    NamedModel nm;
    if (!load(pos[i], fm[i-1], nm.name, err)) { fprintf(stderr, "ecce-fragorb: %s\n", err.c_str()); return 1; }
    nm.model = &fm[i-1];
    frags.push_back(nm);
  }
  Options opt;
  opt.allowUncovered = allow;
  Analysis a;
  if (!analyse(cx, frags, opt, a, err)) { fprintf(stderr, "ecce-fragorb: %s\n", err.c_str()); return 1; }
  const bool mull = method != "scpa";
  vector<Connection> con = connections(a, threshold, method);
  if (json) {
    printf("{\"method\":%s,\"threshold\":%g,\"fragments\":[", jstr(method).c_str(), threshold);
    for (size_t f = 0; f < a.fragmentNames.size(); f++) printf("%s%s", f ? "," : "", jstr(a.fragmentNames[f]).c_str());
    printf("],\"channels\":[");
    for (size_t c = 0; c < a.channel.size(); c++) {
      const Channel& ch = a.channel[c];
      printf("%s{\"spin\":%d,\"fo\":[", c ? "," : "", ch.spin);
      for (size_t k = 0; k < ch.fo.size(); k++)
        printf("%s{\"fragment\":%d,\"index\":%d,\"energy_ev\":%.8f,\"occupation\":%.6f}", k ? "," : "",
               ch.fo[k].fragment, ch.fo[k].index, ch.fo[k].energy * HARTREE_EV, ch.fo[k].occupation);
      printf("],\"mo\":[");
      const vector<double>& comp = mull ? ch.mulliken : ch.scpa;
      for (size_t i = 0; i < ch.nmo; i++) {
        printf("%s{\"index\":%zu,\"energy_ev\":%.8f,\"occupation\":%.6f,\"remainder\":%.8f,\"composition\":[", i ? "," : "",
               i + 1, ch.energy[i] * HARTREE_EV, ch.occupation[i], ch.remainder[i]);
        for (size_t k = 0; k < ch.fo.size(); k++) printf("%s%.8f", k ? "," : "", comp[k * ch.nmo + i]);
        printf("]}");
      }
      printf("]}");
    }
    printf("],\"connections\":[");
    for (size_t k = 0; k < con.size(); k++)
      printf("%s{\"spin\":%d,\"mo\":%d,\"fo\":%d,\"share\":%.8f}", k ? "," : "", con[k].spin, con[k].mo + 1, con[k].fo + 1, con[k].share);
    printf("]}\n");
    return 0;
  }
  printf("complex %s, %zu fragments, %s composition, FO energies and MO energies in eV\n", name.c_str(),
         a.fragmentNames.size(), mull ? "Mulliken" : "SCPA");
  for (size_t f = 0; f < a.fragmentNames.size(); f++) printf("  fragment %zu: %s (%zu atoms)\n", f + 1, a.fragmentNames[f].c_str(), a.atomMap[f].size());
  for (size_t c = 0; c < a.channel.size(); c++) {
    const Channel& ch = a.channel[c];
    const vector<double>& comp = mull ? ch.mulliken : ch.scpa;
    printf("\n%s: %zu MOs expanded in %zu FOs\n", a.restricted ? "orbitals" : (ch.spin ? "beta orbitals" : "alpha orbitals"), ch.nmo, ch.fo.size());
    for (size_t i = 0; i < ch.nmo; i++) {
      // a level is one FO, or with --levels a run of degenerate FOs of one fragment
      vector<int> lo(ch.fo.size()), hi(ch.fo.size());
      for (size_t k = 0; k < ch.fo.size(); k++) {
        lo[k] = hi[k] = int(k);
        if (!levels) continue;
        while (lo[k] > 0 && ch.fo[lo[k]-1].fragment == ch.fo[k].fragment && std::fabs(ch.fo[lo[k]-1].energy - ch.fo[k].energy) < 1e-4) lo[k]--;
        while (size_t(hi[k]) + 1 < ch.fo.size() && ch.fo[hi[k]+1].fragment == ch.fo[k].fragment && std::fabs(ch.fo[hi[k]+1].energy - ch.fo[k].energy) < 1e-4) hi[k]++;
      }
      vector<std::pair<double,size_t> > v;
      for (size_t k = 0; k < ch.fo.size(); k++) {
        if (size_t(lo[k]) != k) continue;
        double sum = 0;
        for (int q = lo[k]; q <= hi[k]; q++) sum += comp[size_t(q) * ch.nmo + i];
        if (sum >= topThreshold) v.push_back(std::make_pair(-sum, k));
      }
      std::sort(v.begin(), v.end());
      printf("MO %4zu  E %10.4f  occ %.3f  remainder %6.2f%%\n", i + 1, ch.energy[i] * HARTREE_EV, ch.occupation[i], ch.remainder[i] * 100);
      for (size_t t = 0; t < v.size() && int(t) < top; t++) {
        size_t k = v[t].second;
        const Fo& fo = ch.fo[k];
        string idx = lo[k] == hi[k] ? fmt("%3d", fo.index) : fmt("%d-%d", fo.index, ch.fo[hi[k]].index);
        printf("      %-12s FO %-7s E %10.4f  occ %.3f  %7.2f%%\n", a.fragmentNames[fo.fragment].c_str(), idx.c_str(),
               fo.energy * HARTREE_EV, fo.occupation, -v[t].first * 100);
      }
    }
    printf("\nconnections (share >= %.0f%%): FO -> MO\n", threshold * 100);
    for (size_t k = 0; k < con.size(); k++) {
      if (con[k].spin != ch.spin) continue;
      const Fo& fo = ch.fo[con[k].fo];
      printf("  %s FO %d (%.4f eV) -> MO %d (%.4f eV)  %.2f%%\n", a.fragmentNames[fo.fragment].c_str(), fo.index,
             fo.energy * HARTREE_EV, con[k].mo + 1, ch.energy[con[k].mo] * HARTREE_EV, con[k].share * 100);
    }
  }
  return 0;
}

// "1,2,5-8" -> 0-based atoms
static bool parseAtoms(const string& s, vector<int>& out) {
  size_t i = 0;
  while (i < s.size()) {
    size_t j = s.find(',', i);
    string t = s.substr(i, j == string::npos ? string::npos : j - i);
    int a = atoi(t.c_str()), b = a;
    size_t d = t.find('-');
    if (d != string::npos) b = atoi(t.c_str() + d + 1);
    if (a < 1 || b < a) return false;
    for (int x = a; x <= b; x++) out.push_back(x - 1);
    if (j == string::npos) break;
    i = j + 1;
  }
  return !out.empty();
}

static int dosCmd(const vector<string>& pos, const vector<string>& sets, const vector<string>& pairs, DosOptions opt, bool json) {
  if (pos.empty()) { fprintf(stderr, "dos needs a complex\n"); return 2; }
  Model cx;
  string name, err;
  if (!load(pos[0], cx, name, err)) { fprintf(stderr, "ecce-fragorb: %s\n", err.c_str()); return 1; }
  for (size_t i = 1; i < pos.size(); i++) {
    Model fr;
    AtomSet as;
    if (!load(pos[i], fr, as.name, err)) { fprintf(stderr, "ecce-fragorb: %s\n", err.c_str()); return 1; }
    if (!mapAtoms(cx, fr, 1e-3, as.atoms, err)) { fprintf(stderr, "ecce-fragorb: fragment %s: %s\n", as.name.c_str(), err.c_str()); return 1; }
    opt.sets.push_back(as);
  }
  for (size_t i = 0; i < sets.size(); i++) {
    size_t e = sets[i].find('=');
    AtomSet as;
    if (e == string::npos || !parseAtoms(sets[i].substr(e + 1), as.atoms)) { fprintf(stderr, "--set expects NAME=1,2,5-8, got '%s'\n", sets[i].c_str()); return 2; }
    as.name = sets[i].substr(0, e);
    opt.sets.push_back(as);
  }
  for (size_t i = 0; i < pairs.size(); i++) {
    size_t e = pairs[i].find(':');
    int a = -1, b = -1;
    for (size_t s = 0; s < opt.sets.size(); s++) {
      if (e != string::npos && opt.sets[s].name == pairs[i].substr(0, e)) a = int(s);
      if (e != string::npos && opt.sets[s].name == pairs[i].substr(e + 1)) b = int(s);
    }
    if (a < 0 || b < 0 || a == b) { fprintf(stderr, "--pair expects two different set names as A:B, got '%s'\n", pairs[i].c_str()); return 2; }
    opt.pairs.push_back(std::make_pair(a, b));
  }
  DosResult r;
  if (!dos(cx, opt, r, err)) { fprintf(stderr, "ecce-fragorb: %s\n", err.c_str()); return 1; }
  size_t ns = r.setNames.size(), np = r.pairNames.size();
  if (json) {
    printf("{\"fermi_ev\":%.6f,\"sets\":[", r.fermiEv);
    for (size_t s = 0; s < ns; s++) printf("%s%s", s ? "," : "", jstr(r.setNames[s]).c_str());
    printf("],\"pairs\":[");
    for (size_t p = 0; p < np; p++) printf("%s%s", p ? "," : "", jstr(r.pairNames[p]).c_str());
    printf("],\"occupied_overlap\":[");
    for (size_t p = 0; p < np; p++) printf("%s%.8f", p ? "," : "", r.occupiedOverlap[p]);
    printf("],\"grid\":[");
    for (size_t g = 0; g < r.energy.size(); g++) {
      printf("%s{\"e\":%.4f,\"tdos\":%.8g,\"pdos\":[", g ? "," : "", r.energy[g], r.tdos[g]);
      for (size_t s = 0; s < ns; s++) printf("%s%.8g", s ? "," : "", r.pdos[g * ns + s]);
      printf("],\"opdos\":[");
      for (size_t p = 0; p < np; p++) printf("%s%.8g", p ? "," : "", r.opdos[g * np + p]);
      printf("],\"int_opdos\":[");
      for (size_t p = 0; p < np; p++) printf("%s%.8g", p ? "," : "", r.intOpdos[g * np + p]);
      printf("]}");
    }
    printf("]}\n");
    return 0;
  }
  printf("# complex %s; Fermi level (HOMO/LUMO midpoint) %.4f eV; %s, FWHM %.3f eV; states/eV, closed-shell MOs count twice\n",
         name.c_str(), r.fermiEv, opt.lorentzian ? "Lorentzian" : "Gaussian", opt.fwhmEv);
  printf("# occupied overlap population (sum n_i OP_i):");
  for (size_t p = 0; p < np; p++) printf("  %s %.4f", r.pairNames[p].c_str(), r.occupiedOverlap[p]);
  printf("\n# E_eV  TDOS");
  for (size_t s = 0; s < ns; s++) printf("  PDOS(%s)", r.setNames[s].c_str());
  for (size_t p = 0; p < np; p++) printf("  OPDOS(%s)", r.pairNames[p].c_str());
  for (size_t p = 0; p < np; p++) printf("  intOPDOS(%s)", r.pairNames[p].c_str());
  printf("\n");
  for (size_t g = 0; g < r.energy.size(); g++) {
    printf("%.4f %.6g", r.energy[g], r.tdos[g]);
    for (size_t s = 0; s < ns; s++) printf(" %.6g", r.pdos[g * ns + s]);
    for (size_t p = 0; p < np; p++) printf(" %.6g", r.opdos[g * np + p]);
    for (size_t p = 0; p < np; p++) printf(" %.6g", r.intOpdos[g * np + p]);
    printf("\n");
  }
  return 0;
}

int main(int argc, char** argv) {
  if (argc < 2 || !strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) {
    fputs(USAGE, argc < 2 ? stderr : stdout);
    return argc < 2 ? 2 : 0;
  }
  string cmd = argv[1];
  vector<string> pos, sets, pairs;
  double threshold = 0.10, topThreshold = 0.02;
  string method = "mulliken";
  bool allow = false, json = false, levels = false;
  int top = 4;
  DosOptions dopt;
  for (int i = 2; i < argc; i++) {
    string a = argv[i];
    bool more = i + 1 < argc;
    if (a == "--threshold" && more) { threshold = atof(argv[++i]); topThreshold = std::min(topThreshold, threshold); }
    else if (a == "--method" && more) method = argv[++i];
    else if (a == "--top" && more) top = atoi(argv[++i]);
    else if (a == "--allow-uncovered") allow = true;
    else if (a == "--json") json = true;
    else if (a == "--levels") levels = true;
    else if (a == "--set" && more) sets.push_back(argv[++i]);
    else if (a == "--pair" && more) pairs.push_back(argv[++i]);
    else if (a == "--fwhm" && more) dopt.fwhmEv = atof(argv[++i]);
    else if (a == "--shape" && more) dopt.lorentzian = !strcmp(argv[++i], "lorentzian");
    else if (a == "--emin" && more) dopt.emin = atof(argv[++i]);
    else if (a == "--emax" && more) dopt.emax = atof(argv[++i]);
    else if (a == "--step" && more) dopt.step = atof(argv[++i]);
    else if (a.size() > 1 && a[0] == '-' && a[1] == '-') { fprintf(stderr, "ecce-fragorb: unknown option %s (see --help)\n", a.c_str()); return 2; }
    else pos.push_back(a);
  }
  if (method != "mulliken" && method != "scpa") { fprintf(stderr, "--method is mulliken or scpa\n"); return 2; }
  if (cmd == "analyse" || cmd == "analyze") return analyseCmd(pos, threshold, method, allow, json, top, topThreshold, levels);
  if (cmd == "dos") return dosCmd(pos, sets, pairs, dopt, json);
  fprintf(stderr, "ecce-fragorb: unknown command %s (analyse, dos; see --help)\n", cmd.c_str());
  return 2;
}
