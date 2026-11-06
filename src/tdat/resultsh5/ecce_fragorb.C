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
"  ecce-fragorb diagram <complex> <fragment>... [--threshold 0.10]\n"
"               [--method mulliken|scpa] [--window OCC[,VIRT]] [--name NAME]...\n"
"               [--json]\n"
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
"diagram  The interaction diagram: the first fragment's FO levels (column 0),\n"
"         the complex's MO levels (column 1), the other fragments' FO levels\n"
"         (column 2, one lane per fragment), and a line from an FO level to an\n"
"         MO level where its share is >= --threshold.  Degenerate orbitals\n"
"         form one level.  Drawn: OCC complex levels from the HOMO level down\n"
"         and VIRT from the LUMO level up (--window, default 4,4); then, until\n"
"         nothing changes, every MO level sharing an FO level with a drawn\n"
"         occupied MO level; then every FO level with a line to a drawn MO.\n"
"         Per MO level: the part outside all FOs, the sum of its lines'\n"
"         shares (under 90 %: not representative), the fragment-fragment\n"
"         overlap population per electron, and bonding/antibonding (its sign)\n"
"         when lines come from two fragments.\n"
"dos      PDOS, TDOS and OPDOS from the complex's own S.  Atom sets come from\n"
"         the fragment calculations (named after their folder) or --set\n"
"         NAME=1,2,5-8 (1-based complex atoms).  --pair A:B picks OPDOS pairs\n"
"         (default: all).  Gaussian or Lorentzian broadening of FWHM\n"
"         --fwhm (default 0.3 eV); grid --emin/--emax/--step in eV (default\n"
"         HOMO-20 to LUMO+10, 0.05).  Closed-shell MOs count twice.\n"
"\n"
"--levels           (analyse) add the shares of degenerate FOs of one fragment (within 1e-4 Eh)\n"
"                   and list levels, e.g. the five d orbitals of an atom, instead of single FOs.\n"
"--name NAME        (diagram) the name of the next fragment in the output (default: its folder)\n"
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

static string numbers(const vector<int>& n) {
  string o = "[";
  for (size_t i = 0; i < n.size(); i++) o += (i ? "," : "") + std::to_string(n[i]);
  return o + "]";
}

static string levelName(const char* what, const vector<int>& n) {
  if (n.size() == 1) return string(what) + " " + std::to_string(n[0]);
  return string(what) + " " + std::to_string(n.front()) + "-" + std::to_string(n.back());
}

static int diagramCmd(const vector<string>& pos, const vector<string>& names, const DiagramOptions& dopt, bool allow, bool json) {
  if (pos.size() < 2) { fprintf(stderr, "diagram needs a complex and at least one fragment\n"); return 2; }
  Model cx;
  vector<Model> fm(pos.size() - 1);
  vector<NamedModel> frags;
  string name, err;
  if (!load(pos[0], cx, name, err)) { fprintf(stderr, "ecce-fragorb: %s\n", err.c_str()); return 1; }
  for (size_t i = 1; i < pos.size(); i++) {
    NamedModel nm;
    if (!load(pos[i], fm[i-1], nm.name, err)) { fprintf(stderr, "ecce-fragorb: %s\n", err.c_str()); return 1; }
    nm.model = &fm[i-1];
    if (i - 1 < names.size()) nm.name = names[i - 1];
    frags.push_back(nm);
  }
  Options opt;
  opt.allowUncovered = allow;
  Analysis a;
  Diagram d;
  if (!analyse(cx, frags, opt, a, err) || !diagram(cx, a, dopt, d, err)) {
    fprintf(stderr, "ecce-fragorb: %s\n", err.c_str());
    return 1;
  }
  if (json) {
    printf("{\"method\":%s,\"threshold\":%g,\"covered\":%g,\"window\":[%d,%d],\"fragments\":[",
           jstr(dopt.method).c_str(), dopt.threshold, dopt.covered, dopt.occupiedLevels, dopt.virtualLevels);
    for (size_t f = 0; f < d.fragmentNames.size(); f++) printf("%s%s", f ? "," : "", jstr(d.fragmentNames[f]).c_str());
    printf("],\"channels\":[");
    for (size_t c = 0; c < d.channel.size(); c++) {
      const DiagramChannel& ch = d.channel[c];
      printf("%s{\"spin\":%d,\"homo\":%d,\"lumo\":%d,\"gap_ev\":%.6f,\"representative\":%s,\"window_representative\":%s,\"verdict\":%s,\"notes\":[",
             c ? "," : "", ch.spin, ch.homo, ch.lumo, ch.gapEv, ch.representative ? "true" : "false",
             ch.windowRepresentative ? "true" : "false",
             jstr(ch.verdict).c_str());
      for (size_t k = 0; k < ch.notes.size(); k++) printf("%s%s", k ? "," : "", jstr(ch.notes[k]).c_str());
      printf("],\"hidden_fo_levels\":%s,\"levels\":[", numbers(ch.hiddenFoLevels).c_str());
      for (size_t k = 0; k < ch.levels.size(); k++) {
        const DiagramLevel& l = ch.levels[k];
        printf("%s{\"id\":%d,\"column\":%d,\"fragment\":%d,\"numbers\":%s,\"energy_ev\":%.6f,\"occupation\":[",
               k ? "," : "", l.id, l.column, l.fragment, numbers(l.numbers).c_str(), l.energyEv);
        for (size_t i = 0; i < l.occupation.size(); i++) printf("%s%.6f", i ? "," : "", l.occupation[i]);
        printf("],\"position\":%s,\"kind\":%s", jstr(l.position).c_str(), jstr(l.kind).c_str());
        if (l.fragment < 0)
          printf(",\"partner\":%s,\"remainder\":%.8f,\"covered\":%.8f,\"representative\":%s,"
                 "\"overlap_population\":%.8f,\"fragments\":%s",
                 l.partner ? "true" : "false", l.remainder, l.covered, l.representative ? "true" : "false",
                 l.overlapPopulation, numbers(l.fragments).c_str());
        printf(",\"shares\":[");
        for (size_t i = 0; i < l.shares.size(); i++) {
          const DiagramShare& sh = l.shares[i];
          printf("%s{\"level\":%d,\"fragment\":%d,\"numbers\":%s,\"energy_ev\":%.6f,\"share\":%.8f,\"position\":%s}",
                 i ? "," : "", sh.level, sh.fragment, numbers(sh.numbers).c_str(), sh.energyEv, sh.share,
                 jstr(sh.position).c_str());
        }
        printf("]}");
      }
      printf("],\"lines\":[");
      for (size_t k = 0; k < ch.lines.size(); k++)
        printf("%s{\"fo\":%d,\"mo\":%d,\"fragment\":%d,\"share\":%.8f}", k ? "," : "",
               ch.lines[k].fo, ch.lines[k].mo, ch.lines[k].fragment, ch.lines[k].share);
      printf("]}");
    }
    printf("]}\n");
    return 0;
  }
  printf("complex %s; columns: %s | complex", name.c_str(), d.fragmentNames[0].c_str());
  for (size_t f = 1; f < d.fragmentNames.size(); f++) printf("%s %s", f == 1 ? " |" : ",", d.fragmentNames[f].c_str());
  printf("\n%s shares, lines for >= %.0f %%, window %d occupied + %d virtual MO levels; energies in eV\n",
         dopt.method == "scpa" ? "SCPA" : "Mulliken", 100 * dopt.threshold, dopt.occupiedLevels, dopt.virtualLevels);
  for (size_t c = 0; c < d.channel.size(); c++) {
    const DiagramChannel& ch = d.channel[c];
    if (d.channel.size() > 1) printf("\n%s orbitals\n", ch.spin ? "beta" : "alpha");
    printf("%s\n", ch.verdict.c_str());
    if (ch.homo >= 0 && ch.lumo >= 0)
      printf("HOMO-LUMO orbital-energy gap %.3f eV\n", ch.gapEv);
    printf("\ncomplex MO levels (highest first): energy, occupation, outside FOs, covered by lines, OP, kind\n");
    for (size_t k = ch.levels.size(); k-- > 0;) {
      const DiagramLevel& l = ch.levels[k];
      if (l.fragment >= 0) continue;
      string kind = l.kind == "fragment" ? d.fragmentNames[l.fragments[0]] : l.kind;
      printf("  %-10s %9.3f  occ %.3f  %6.1f %%  %6.1f %%  OP %+.4f  %s%s%s\n", levelName("MO", l.numbers).c_str(),
             l.energyEv, l.occupation[0], 100 * l.remainder, 100 * l.covered, l.overlapPopulation, kind.c_str(),
             l.partner ? "  (partner)" : "", l.representative ? "" : "  NOT REPRESENTATIVE");
    }
    printf("\nFO levels drawn\n");
    for (size_t k = 0; k < ch.levels.size(); k++) {
      const DiagramLevel& l = ch.levels[k];
      if (l.fragment < 0) continue;
      printf("  %-12s %-10s %-9s %9.3f  occ %.3f\n", d.fragmentNames[l.fragment].c_str(),
             levelName("FO", l.numbers).c_str(), l.position.c_str(), l.energyEv, l.occupation[0]);
    }
    printf("\nlines: FO level -> MO level, share\n");
    for (size_t k = 0; k < ch.lines.size(); k++) {
      const DiagramLevel& f = ch.levels[ch.lines[k].fo];
      const DiagramLevel& m = ch.levels[ch.lines[k].mo];
      printf("  %-12s %-10s -> %-10s %6.1f %%\n", d.fragmentNames[f.fragment].c_str(), levelName("FO", f.numbers).c_str(),
             levelName("MO", m.numbers).c_str(), 100 * ch.lines[k].share);
    }
    for (size_t k = 0; k < ch.notes.size(); k++) printf("note: %s\n", ch.notes[k].c_str());
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
  vector<string> pos, sets, pairs, names;
  double threshold = 0.10, topThreshold = 0.02;
  string method = "mulliken";
  bool allow = false, json = false, levels = false;
  int top = 4;
  DosOptions dopt;
  DiagramOptions gopt;
  for (int i = 2; i < argc; i++) {
    string a = argv[i];
    bool more = i + 1 < argc;
    if (a == "--threshold" && more) { threshold = atof(argv[++i]); topThreshold = std::min(topThreshold, threshold); }
    else if (a == "--method" && more) method = argv[++i];
    else if (a == "--window" && more) {
      const char* w = argv[++i];
      gopt.occupiedLevels = gopt.virtualLevels = atoi(w);
      if (strchr(w, ',')) gopt.virtualLevels = atoi(strchr(w, ',') + 1);
      if (gopt.occupiedLevels < 1 || gopt.virtualLevels < 0) { fprintf(stderr, "--window expects OCC[,VIRT], OCC >= 1\n"); return 2; }
    }
    else if (a == "--top" && more) top = atoi(argv[++i]);
    else if (a == "--allow-uncovered") allow = true;
    else if (a == "--json") json = true;
    else if (a == "--levels") levels = true;
    else if (a == "--name" && more) names.push_back(argv[++i]);
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
  if (cmd == "diagram") {
    gopt.threshold = threshold;
    gopt.method = method;
    return diagramCmd(pos, names, gopt, allow, json);
  }
  if (cmd == "dos") return dosCmd(pos, sets, pairs, dopt, json);
  fprintf(stderr, "ecce-fragorb: unknown command %s (analyse, diagram, dos; see --help)\n", cmd.c_str());
  return 2;
}
