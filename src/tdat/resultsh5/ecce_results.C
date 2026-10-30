// ecce-results: convert, export, show, check, selftest for results.h5.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sys/stat.h>
#include "tdat/ResultsH5.H"

using namespace resultsh5;

static bool ends(const string& s, const char* e) {
  size_t n = strlen(e);
  return s.size() >= n && s.compare(s.size()-n, n, e) == 0;
}
static long fsize(const string& p) { struct stat st; return stat(p.c_str(), &st) ? -1 : long(st.st_size); }
static string tail(const string& p) { size_t k = p.find_last_of('/'); return k == string::npos ? p : p.substr(k+1); }

static bool readAny(const string& in, Model& m, string& err) {
  if (ends(in, ".molden") || ends(in, ".molden.input") || ends(in, ".input")) return readMolden(in, m, err);
  if (ends(in, ".fchk") || ends(in, ".fch")) return readFchk(in, m, err);
  if (ends(in, ".hess")) return readOrcaHess(in, m, err);
  err = "unknown input type (expected .molden, .fchk, .hess): " + in;
  return false;
}

static int show(const Model& m) {
  printf("format_version %d\n", m.formatVersion);
  for (std::map<string,string>::const_iterator i = m.provenance.begin(); i != m.provenance.end(); ++i)
    printf("  %s = %s\n", i->first.c_str(), i->second.c_str());
  printf("atoms %zu, geometries %zu\n", m.natoms(), m.nframes());
  printf("basis %s%s: %zu shells, %zu AOs\n", m.basis.type.c_str(), m.basis.orthonormal ? " (orthonormal)" : "",
         m.basis.shells.size(), m.basis.nao());
  for (size_t k = 0; k < m.mos.size(); k++)
    printf("mo %zu: spin %d, %zu MOs x %zu AOs, geometry %d%s\n", k, m.mos[k].spin, m.mos[k].nmo, m.mos[k].nao,
           m.mos[k].geometry, m.mos[k].symmetry.empty() ? "" : ", with symmetry labels");
  for (size_t k = 0; k < m.hessian.size(); k++) printf("hessian %zu: geometry %d, %s\n", k, m.hessian[k].geometry, m.hessian[k].method.c_str());
  for (size_t k = 0; k < m.gradient.size(); k++) printf("gradient %zu\n", k);
  for (size_t k = 0; k < m.charges.size(); k++) printf("charges: %s\n", m.charges[k].scheme.c_str());
  for (size_t k = 0; k < m.vib.size(); k++) printf("vib %zu: %zu modes\n", k, m.vib[k].frequency.size());
  if (m.bands.nk) printf("bands: %zu k-points, %zu bands\n", m.bands.nk, m.bands.nband);
  return 0;
}

static double maxDiff(const vector<double>& a, const vector<double>& b) {
  if (a.size() != b.size()) return 1e30;
  double w = 0;
  for (size_t i = 0; i < a.size(); i++) w = std::max(w, std::fabs(a[i]-b[i]));
  return w;
}

// convert -> check -> h5 round trip -> molden round trip -> size.
static int selftest(int argc, char** argv) {
  if (argc < 5) { fprintf(stderr, "selftest <workdir> <tol-bytes> <input>... [-- <size-only files>]\n"); return 2; }
  string dir = argv[2];
  long tol = atol(argv[3]);
  vector<string> in, ref;
  bool refs = false;
  for (int i = 4; i < argc; i++) {
    if (!strcmp(argv[i], "--")) { refs = true; continue; }
    (refs ? ref : in).push_back(argv[i]);
  }
  Model m; string err;
  long srcSum = 0;
  for (size_t i = 0; i < in.size(); i++) {
    if (!readAny(in[i], m, err)) { fprintf(stderr, "FAIL read %s: %s\n", in[i].c_str(), err.c_str()); return 1; }
    srcSum += fsize(in[i]);
  }
  int fails = 0;
  Invariants inv = check(m);
  printf("%s", inv.report.c_str());
  if (!inv.ok) { printf("FAIL invariants\n"); fails++; }
  if (m.mos.size() && !inv.haveMos && !m.basis.orthonormal) { printf("FAIL no MO invariants computed\n"); fails++; }

  string h5 = dir + "/results.h5", h5n = dir + "/results-nocompress.h5";
  compression = false; writeFile(h5n, m, err); compression = true;
  if (!writeFile(h5, m, err)) { printf("FAIL write: %s\n", err.c_str()); return 1; }
  Model r;
  if (!readFile(h5, r, err)) { printf("FAIL read back: %s\n", err.c_str()); return 1; }
  double d = 0;
  for (size_t k = 0; k < m.mos.size() && k < r.mos.size(); k++) d = std::max(d, maxDiff(m.mos[k].coef, r.mos[k].coef));
  if (m.mos.size() != r.mos.size() || d != 0 || maxDiff(m.coord, r.coord) != 0) { printf("FAIL h5 round trip (diff %g)\n", d); fails++; }
  for (size_t k = 0; k < m.hessian.size(); k++)
    if (k >= r.hessian.size() || maxDiff(m.hessian[k].upper, r.hessian[k].upper) != 0) { printf("FAIL hessian round trip\n"); fails++; }
  printf("h5 round trip: MO coefficients identical, geometry identical\n");

  if (!m.mos.empty() && !m.basis.orthonormal && m.basis.type == "GTO") {
    string md = dir + "/export.molden";
    Model b;
    if (!writeMolden(md, r, err) || !readMolden(md, b, err)) { printf("FAIL molden export/import: %s\n", err.c_str()); return 1; }
    double dc = 0;
    for (size_t k = 0; k < m.mos.size() && k < b.mos.size(); k++) dc = std::max(dc, maxDiff(m.mos[k].coef, b.mos[k].coef));
    if (m.mos.size() != b.mos.size()) dc = 1e30;
    double de = 0;
    for (size_t k = 0; k < m.basis.shells.size() && k < b.basis.shells.size(); k++) {
      de = std::max(de, maxDiff(m.basis.shells[k].exponent, b.basis.shells[k].exponent));
      const vector<double>&x = m.basis.shells[k].coef, &y = b.basis.shells[k].coef;
      for (size_t i = 0; i < x.size() && i < y.size(); i++) de = std::max(de, std::fabs(x[i]/x[0] - y[i]/y[0]));
    }
    if (m.basis.shells.size() != b.basis.shells.size()) de = 1e30;
    Invariants ib = check(b);
    printf("molden round trip: max |C' - C'_re-read| = %.2e, shell max diff %.2e, re-read invariants: tr(PS) err %.1e %.1e, ortho %.1e\n",
           dc, de, ib.traceErr[0], ib.traceErr[1], ib.orthoErr);
    if (dc > 1e-10 || de > 1e-10 || !ib.ok) { printf("FAIL molden round trip\n"); fails++; }
  }
  long sh5 = fsize(h5), sn = fsize(h5n);
  printf("sizes (bytes): results.h5 %ld, uncompressed %ld, parsed sources %ld", sh5, sn, srcSum);
  for (size_t i = 0; i < in.size(); i++) printf(", %s %ld", tail(in[i]).c_str(), fsize(in[i]));
  for (size_t i = 0; i < ref.size(); i++) printf(", %s %ld", tail(ref[i]).c_str(), fsize(ref[i]));
  printf("\n");
  if (sh5 > srcSum + tol) { printf("FAIL results.h5 (%ld) larger than the parsed sources (%ld) + tolerance %ld\n", sh5, srcSum, tol); fails++; }
  for (size_t i = 0; i < ref.size(); i++)
    if (sh5 > fsize(ref[i]) + tol) { printf("FAIL results.h5 larger than %s\n", ref[i].c_str()); fails++; }
  printf(fails ? "RESULT FAIL\n" : "RESULT OK\n");
  return fails ? 1 : 0;
}

int main(int argc, char** argv) {
  string err;
  if (argc >= 2 && !strcmp(argv[1], "selftest")) return selftest(argc, argv);
  if (argc >= 4 && !strcmp(argv[1], "convert")) {
    string in = argv[2], out = argv[3];
    Model m;
    if (ends(in, ".hess") && (fsize(out) < 0 || !readFile(out, m, err))) {
      fprintf(stderr, ".hess merges into an existing results.h5 (convert the .molden or .fchk first): %s\n", err.c_str());
      return 1;
    }
    if (!readAny(in, m, err) || !writeFile(out, m, err)) { fprintf(stderr, "ecce-results: %s\n", err.c_str()); return 1; }
    printf("wrote %s (%ld bytes) from %s (%ld bytes)\n", out.c_str(), fsize(out), in.c_str(), fsize(in));
    return 0;
  }
  if (argc >= 5 && !strcmp(argv[1], "export") && !strcmp(argv[2], "molden")) {
    Model m;
    if (!readFile(argv[3], m, err) || !writeMolden(argv[4], m, err)) { fprintf(stderr, "ecce-results: %s\n", err.c_str()); return 1; }
    return 0;
  }
  if (argc >= 3 && (!strcmp(argv[1], "show") || !strcmp(argv[1], "check"))) {
    Model m;
    if (!readFile(argv[2], m, err)) { fprintf(stderr, "ecce-results: %s\n", err.c_str()); return 1; }
    if (!strcmp(argv[1], "show")) return show(m);
    Invariants inv = check(m);
    printf("%s%s\n", inv.report.c_str(), inv.ok ? "OK" : "FAILED");
    return inv.ok ? 0 : 1;
  }
  fprintf(stderr, "usage: ecce-results convert <in.molden|in.fchk|in.hess> <results.h5>\n"
                  "       ecce-results export molden <results.h5> <out.molden>\n"
                  "       ecce-results show|check <results.h5>\n");
  return 2;
}
