// Converters: Molden (read/write), Gaussian formatted checkpoint, ORCA .hess.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include "tdat/EspField.H"
#include "Internal.H"

namespace resultsh5 {
namespace {

const double BOHR = 0.529177210903;
const char* const SYM[] = {"X","H","He","Li","Be","B","C","N","O","F","Ne","Na","Mg","Al","Si","P","S","Cl","Ar",
  "K","Ca","Sc","Ti","V","Cr","Mn","Fe","Co","Ni","Cu","Zn","Ga","Ge","As","Se","Br","Kr",
  "Rb","Sr","Y","Zr","Nb","Mo","Tc","Ru","Rh","Pd","Ag","Cd","In","Sn","Sb","Te","I","Xe"};

string baseName(const string& p) { size_t k = p.find_last_of("/\\"); return k == string::npos ? p : p.substr(k+1); }
string lower(string s) { for (size_t i = 0; i < s.size(); i++) s[i] = tolower(s[i]); return s; }
string trim(const string& s) {
  size_t a = s.find_first_not_of(" \t\r\n"), b = s.find_last_not_of(" \t\r\n");
  return a == string::npos ? "" : s.substr(a, b-a+1);
}
vector<string> words(const string& s) {
  vector<string> w; std::istringstream is(s); string t;
  while (is >> t) w.push_back(t);
  return w;
}
double dnum(string s) {
  for (size_t i = 0; i < s.size(); i++) if (s[i] == 'D' || s[i] == 'd') s[i] = 'E';
  return atof(s.c_str());
}
const char* lchar = "spdfgh";

void stamp(Model& m, const string& path, const string& code) {
  m.provenance["source_file"] = baseName(path);
  m.provenance["source_sha256"] = sha256File(path);
  m.provenance["converter_version"] = CONVERTER_VERSION;
  if (!code.empty()) m.provenance["code"] = code;
}

}  // namespace

// ---- SHA-256 ---------------------------------------------------------------
string sha256File(const string& path) {
  static const unsigned K[64] = {
  0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,0xd807aa98,0x12835b01,
  0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,
  0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,
  0x06ca6351,0x14292967,0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
  0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,0x19a4c116,0x1e376c08,
  0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,
  0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
  std::ifstream in(path.c_str(), std::ios::binary);
  if (!in) return "";
  std::string data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  unsigned long long bits = (unsigned long long)data.size()*8;
  data += char(0x80);
  while (data.size() % 64 != 56) data += char(0);
  for (int i = 7; i >= 0; i--) data += char((bits >> (8*i)) & 255);
  unsigned h[8] = {0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
  #define ROR(x,n) (((x) >> (n)) | ((x) << (32-(n))))
  for (size_t off = 0; off < data.size(); off += 64) {
    unsigned w[64];
    for (int i = 0; i < 16; i++)
      w[i] = ((unsigned char)data[off+4*i] << 24) | ((unsigned char)data[off+4*i+1] << 16) |
             ((unsigned char)data[off+4*i+2] << 8) | (unsigned char)data[off+4*i+3];
    for (int i = 16; i < 64; i++) {
      unsigned s0 = ROR(w[i-15],7) ^ ROR(w[i-15],18) ^ (w[i-15] >> 3);
      unsigned s1 = ROR(w[i-2],17) ^ ROR(w[i-2],19) ^ (w[i-2] >> 10);
      w[i] = w[i-16] + s0 + w[i-7] + s1;
    }
    unsigned a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],hh=h[7];
    for (int i = 0; i < 64; i++) {
      unsigned S1 = ROR(e,6) ^ ROR(e,11) ^ ROR(e,25), ch = (e & f) ^ (~e & g);
      unsigned t1 = hh + S1 + ch + K[i] + w[i];
      unsigned S0 = ROR(a,2) ^ ROR(a,13) ^ ROR(a,22), mj = (a & b) ^ (a & c) ^ (b & c);
      unsigned t2 = S0 + mj;
      hh = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
    }
    h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d; h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=hh;
  }
  char out[65];
  for (int i = 0; i < 8; i++) snprintf(out+8*i, 9, "%08x", h[i]);
  return out;
}

// ---- Molden ----------------------------------------------------------------
bool readMolden(const string& path, Model& m, string& err) {
  std::ifstream in(path.c_str());
  if (!in) { err = "cannot open " + path; return false; }
  m = Model();
  string line, sect, title;
  bool ang = true, sph[8] = {false,false,false,false,false,false,false,false};
  bool sawGto = false;
  vector<vector<double> > rows[2];      // raw MO rows by spin, source order
  vector<double> ene[2], occ[2];
  vector<string> sym[2];
  vector<double> cur; int curSpin = 0; bool inMo = false;
  string pendSym = "", pendSpin = "Alpha"; double pendEne = 0, pendOcc = 0;
  vector<double> coord;
  int atomForShell = -1;
  auto flush = [&]() {
    if (!inMo) return;
    int s = lower(pendSpin).compare(0, 1, "b") == 0 ? 1 : 0;
    rows[s].push_back(cur); ene[s].push_back(pendEne); occ[s].push_back(pendOcc); sym[s].push_back(pendSym);
    cur.clear(); inMo = false;
  };
  while (std::getline(in, line)) {
    string t = trim(line);
    if (t.empty()) { if (sect == "gto") atomForShell = -1; continue; }
    if (t[0] == '[') {
      if (sect == "mo") flush();
      size_t e = t.find(']');
      sect = lower(t.substr(1, e-1));
      string rest = lower(trim(t.substr(e+1)));
      if (sect == "atoms") ang = rest.find("au") == string::npos && rest.find("bohr") == string::npos;
      else if (sect == "5d") { sph[2] = true; }
      else if (sect == "5d10f") { sph[2] = true; sph[3] = false; }
      else if (sect == "5d7f") { sph[2] = sph[3] = true; }
      else if (sect == "7f") sph[3] = true;
      else if (sect == "9g") sph[4] = true;
      else if (sect == "gto") sawGto = true;
      continue;
    }
    if (sect == "title") title += t + " ";
    else if (sect == "atoms") {
      vector<string> w = words(t);
      if (w.size() < 6) { err = "bad [Atoms] line"; return false; }
      m.label.push_back(w[0]);
      m.atomicNumber.push_back(atoi(w[2].c_str()));
      for (int c = 0; c < 3; c++) coord.push_back(dnum(w[3+c]) / (ang ? BOHR : 1.0));
    } else if (sect == "gto") {
      vector<string> w = words(t);
      if (atomForShell < 0) { atomForShell = atoi(w[0].c_str()) - 1; continue; }
      if (w.size() < 2) continue;
      const char* p = strchr(lchar, tolower(w[0][0]));
      if (w[0].size() != 1 || !p) { err = "unsupported shell type '" + w[0] + "' in [GTO]"; return false; }
      Shell s; s.atom = atomForShell; s.l = int(p - lchar);
      int np = atoi(w[1].c_str());
      for (int i = 0; i < np; i++) {
        string pl; std::getline(in, pl);
        vector<string> pw = words(pl);
        if (pw.size() < 2) { err = "bad primitive line"; return false; }
        s.exponent.push_back(dnum(pw[0])); s.coef.push_back(dnum(pw[1]));
      }
      m.basis.shells.push_back(s);
    } else if (sect == "mo") {
      size_t eq = t.find('=');
      if (eq != string::npos && !isdigit((unsigned char)t[0])) {
        string k = lower(trim(t.substr(0, eq))), v = trim(t.substr(eq+1));
        if (k == "sym") { flush(); inMo = true; pendSym = v; pendEne = 0; pendOcc = 0; pendSpin = "Alpha"; }
        else if (k == "ene") pendEne = dnum(v);
        else if (k == "spin") pendSpin = v;
        else if (k == "occup") pendOcc = dnum(v);
        continue;
      }
      vector<string> w = words(t);
      if (w.size() >= 2) {
        size_t idx = atoi(w[0].c_str());
        if (cur.size() < idx) cur.resize(idx, 0.0);
        cur[idx-1] = dnum(w[1]);
      }
    }
  }
  if (sect == "mo") flush();
  if (m.atomicNumber.empty() || !sawGto) { err = "no [Atoms]/[GTO] in " + path; return false; }
  m.coord = coord;
  for (size_t i = 0; i < m.basis.shells.size(); i++) {
    Shell& s = m.basis.shells[i];
    s.cartesian = s.l >= 2 && !sph[s.l];
  }
  size_t nao = m.basis.nao();
  for (int sp = 0; sp < 2; sp++) {
    if (rows[sp].empty()) continue;
    Mos mo; mo.spin = sp; mo.nao = nao; mo.nmo = rows[sp].size();
    mo.coef.assign(mo.nmo*nao, 0.0);
    for (size_t i = 0; i < mo.nmo; i++) {
      vector<double>& r = rows[sp][i];
      if (r.size() > nao) { err = "MO longer than the basis"; return false; }
      r.resize(nao, 0.0);
      size_t off = 0;
      for (size_t k = 0; k < m.basis.shells.size(); k++) {
        const Shell& s = m.basis.shells[k];
        int n = shellSize(s);
        vector<int> perm;
        if (s.cartesian) perm = cartPermFromMolden(s.l);
        for (int j = 0; j < n; j++)
          mo.coef[i*nao + off + (s.cartesian ? perm[j] : j)] = r[off + j];
        off += n;
      }
    }
    mo.energy = ene[sp]; mo.occupation = occ[sp];
    bool anySym = false;
    for (size_t i = 0; i < sym[sp].size(); i++) anySym |= !sym[sp][i].empty() && sym[sp][i] != "?";
    if (anySym) mo.symmetry = sym[sp];
    m.mos.push_back(mo);
  }
  stamp(m, path, title.find("orca_2mkl") != string::npos ? "ORCA" : "");
  if (!title.empty()) m.provenance["molden_title"] = trim(title);
  string note;
  if (!normaliseSource(m, note, err)) return false;
  if (!note.empty()) m.provenance["ao_convention"] = note;
  for (size_t k = 0; k < m.mos.size(); k++) {
    double n = 0;
    for (size_t i = 0; i < m.mos[k].occupation.size(); i++) n += m.mos[k].occupation[i];
    m.nelectron[m.mos[k].spin] = int(n + 0.5);
  }
  return true;
}

bool writeMolden(const string& path, const Model& m, string& err) {
  if (m.basis.type != "GTO") { err = "Molden needs a GTO basis"; return false; }
  size_t N = m.natoms(), nao = m.basis.nao();
  vector<double> xyz;
  if (N == 0 || m.nframes() == 0) { err = "no geometry"; return false; }
  xyz.assign(m.coord.begin(), m.coord.begin() + 3*N);
  // shells normalised so the first component has unit raw self-overlap
  Model t = m;
  vector<size_t> shellOff(t.basis.shells.size());
  size_t off = 0;
  bool sph[8] = {false}, cart[8] = {false};
  for (size_t k = 0; k < t.basis.shells.size(); k++) {
    Shell& s = t.basis.shells[k];
    shellOff[k] = off; off += shellSize(s);
    EspBasisFunction f = rawAo(s, &xyz[3*s.atom], 0);
    double n0 = std::sqrt(EspField::overlapOf(f, f));
    for (size_t i = 0; i < s.coef.size(); i++) s.coef[i] /= n0;
    (s.cartesian ? cart : sph)[s.l] = true;
  }
  for (int l = 2; l < 5; l++)
    if (sph[l] && cart[l]) { err = "mixed Cartesian/spherical shells of one l"; return false; }
  vector<EspBasisFunction> raw;
  for (size_t k = 0; k < t.basis.shells.size(); k++)
    for (int c = 0; c < shellSize(t.basis.shells[k]); c++)
      raw.push_back(rawAo(t.basis.shells[k], &xyz[3*t.basis.shells[k].atom], c));
  vector<double> Nb(nao);
  for (size_t i = 0; i < nao; i++) Nb[i] = std::sqrt(EspField::overlapOf(raw[i], raw[i]));

  FILE* f = fopen(path.c_str(), "w");
  if (!f) { err = "cannot write " + path; return false; }
  fprintf(f, "[Molden Format]\n[Title]\n results.h5 export (resultsh5)\n\n[Atoms] AU\n");
  for (size_t a = 0; a < N; a++)
    fprintf(f, "%s %zu %d %.12f %.12f %.12f\n", a < m.label.size() ? m.label[a].c_str() : "X", a+1,
            m.atomicNumber[a], xyz[3*a], xyz[3*a+1], xyz[3*a+2]);
  fprintf(f, "[GTO]\n");
  vector<size_t> order;     // output AO k -> canonical index
  vector<int> orderShell;
  for (size_t a = 0; a < N; a++) {
    fprintf(f, "%zu 0\n", a+1);
    for (size_t k = 0; k < t.basis.shells.size(); k++) {
      const Shell& s = t.basis.shells[k];
      if (size_t(s.atom) != a) continue;
      fprintf(f, "%c %zu 1.00\n", lchar[s.l], s.exponent.size());
      for (size_t i = 0; i < s.exponent.size(); i++)
        fprintf(f, "%.12e %.12e\n", s.exponent[i], s.coef[i] / nrad(s.exponent[i], s.l));
      vector<int> perm;
      if (s.cartesian) perm = cartPermFromMolden(s.l);
      for (int c = 0; c < shellSize(s); c++) order.push_back(shellOff[k] + (s.cartesian ? perm[c] : c));
    }
    fprintf(f, "\n");
  }
  if (sph[2] && !sph[3] && !cart[3]) fprintf(f, "[5D]\n");
  else if (sph[2] && cart[3]) fprintf(f, "[5D10F]\n");
  else { if (sph[2]) fprintf(f, "[5D]\n"); if (sph[3]) fprintf(f, "[7F]\n"); }
  if (sph[4]) fprintf(f, "[9G]\n");
  fprintf(f, "[MO]\n");
  for (size_t k = 0; k < m.mos.size(); k++) {
    const Mos& mo = m.mos[k];
    for (size_t i = 0; i < mo.nmo; i++) {
      fprintf(f, " Sym= %s\n Ene= %.14e\n Spin= %s\n Occup= %.8f\n",
              mo.symmetry.empty() ? "a" : mo.symmetry[i].c_str(), mo.energy.empty() ? 0.0 : mo.energy[i],
              mo.spin ? "Beta" : "Alpha", mo.occupation.empty() ? 0.0 : mo.occupation[i]);
      for (size_t j = 0; j < order.size(); j++)
        fprintf(f, "%zu %.14e\n", j+1, mo.coef[i*nao + order[j]] / Nb[order[j]]);
    }
  }
  fclose(f);
  return true;
}

// ---- Gaussian fchk -----------------------------------------------------------
bool readFchk(const string& path, Model& m, string& err) {
  std::ifstream in(path.c_str());
  if (!in) { err = "cannot open " + path; return false; }
  m = Model();
  std::map<string, vector<double> > sec;
  string line, l1, l2;
  std::getline(in, l1); std::getline(in, l2);
  while (std::getline(in, line)) {
    if (line.size() < 44 || line[0] == ' ') continue;
    string name = trim(line.substr(0, 43)), rest = trim(line.substr(43));
    if (rest.size() < 1) continue;
    char type = rest[0];
    string tail = trim(rest.substr(1));
    vector<double>& v = sec[name];
    if (tail.compare(0, 2, "N=") == 0) {
      size_t n = atol(tail.c_str() + 2);
      v.reserve(n);
      string tok;
      while (v.size() < n && in >> tok) v.push_back(dnum(tok));
      std::getline(in, line);
    } else if (type == 'I' || type == 'R') {
      v.push_back(dnum(tail));
    }
  }
  auto S = [&](const char* k) -> vector<double>& { return sec[k]; };
  size_t N = S("Atomic numbers").size();
  if (!N || S("Shell types").empty()) { err = "fchk without geometry or basis"; return false; }
  for (size_t i = 0; i < N; i++) m.atomicNumber.push_back(int(S("Atomic numbers")[i]));
  vector<double>& chg = S("Nuclear charges");
  for (size_t i = 0; i < N && i < chg.size(); i++)
    if (std::fabs(chg[i] - m.atomicNumber[i]) > 1e-6) { m.nuclearCharge = chg; break; }
  for (size_t i = 0; i < N; i++) m.label.push_back(m.atomicNumber[i] < 55 ? SYM[m.atomicNumber[i]] : "X");
  m.coord = S("Current cartesian coordinates");
  if (m.coord.size() != 3*N) { err = "bad coordinates"; return false; }
  if (!S("Total Energy").empty()) m.geomEnergy.push_back(S("Total Energy")[0]);
  vector<double> &st = S("Shell types"), &np = S("Number of primitives per shell"),
                 &sa = S("Shell to atom map"), &ex = S("Primitive exponents"),
                 &cc = S("Contraction coefficients"), &cp = S("P(S=P) Contraction coefficients");
  size_t off = 0;
  for (size_t i = 0; i < st.size(); i++) {
    int t = int(st[i]), n = int(np[i]);
    vector<double> e(ex.begin()+off, ex.begin()+off+n), c(cc.begin()+off, cc.begin()+off+n);
    if (t == -1) {      // SP shell: split
      Shell s0, p0;
      s0.atom = p0.atom = int(sa[i]) - 1;
      s0.l = 0; p0.l = 1;
      s0.exponent = p0.exponent = e; s0.coef = c;
      if (cp.size() < off+n) { err = "SP shell without P coefficients"; return false; }
      p0.coef.assign(cp.begin()+off, cp.begin()+off+n);
      m.basis.shells.push_back(s0); m.basis.shells.push_back(p0);
    } else {
      Shell s; s.atom = int(sa[i]) - 1; s.l = std::abs(t); s.cartesian = t >= 2;
      if (s.cartesian && s.l > 3) { err = "Cartesian g shells in fchk: order not implemented"; return false; }
      s.exponent = e; s.coef = c;
      m.basis.shells.push_back(s);
    }
    off += n;
  }
  vector<string> r = words(l2);
  if (r.size() >= 3) { m.provenance["method"] = r[1]; m.basis.name = r[2]; m.provenance["basis_name"] = r[2]; }
  size_t nao = m.basis.nao();
  int nel[2] = {int(S("Number of alpha electrons").empty() ? 0 : S("Number of alpha electrons")[0]),
                int(S("Number of beta electrons").empty() ? 0 : S("Number of beta electrons")[0])};
  bool uhf = !S("Beta MO coefficients").empty();
  for (int sp = 0; sp < (uhf ? 2 : 1); sp++) {
    vector<double>& C = S(sp ? "Beta MO coefficients" : "Alpha MO coefficients");
    vector<double>& E = S(sp ? "Beta Orbital Energies" : "Alpha Orbital Energies");
    if (C.empty() || E.empty() || C.size() != E.size()*nao) { err = "MO coefficient size mismatch"; return false; }
    Mos mo; mo.spin = sp; mo.nao = nao; mo.nmo = E.size();
    mo.coef = C; mo.energy = E;
    for (size_t i = 0; i < mo.nmo; i++) mo.occupation.push_back(int(i) < nel[sp] ? (uhf ? 1.0 : 2.0) : 0.0);
    size_t o = 0;
    for (size_t k = 0; k < m.basis.shells.size(); k++) {
      const Shell& s = m.basis.shells[k];
      int n = shellSize(s);
      if (s.cartesian) {
        vector<int> perm = cartPermFromMolden(s.l);
        for (size_t i = 0; i < mo.nmo; i++)
          for (int j = 0; j < n; j++) mo.coef[i*nao + o + perm[j]] = C[i*nao + o + j];
      }
      o += n;
    }
    m.mos.push_back(mo);
  }
  m.nelectron[0] = nel[0]; m.nelectron[1] = uhf ? nel[1] : nel[0];
  vector<double>& fc = S("Cartesian Force Constants");
  size_t n3 = 3*N;
  if (fc.size() == n3*(n3+1)/2) {
    Hessian h; h.method = r.size() > 1 ? r[1] : "";
    for (size_t i = 0; i < n3; i++)
      for (size_t j = i; j < n3; j++) h.upper.push_back(fc[j*(j+1)/2 + i]);
    m.hessian.push_back(h);
  }
  vector<double>& gr = S("Cartesian Gradient");
  if (gr.size() == n3) { Gradient g; g.g = gr; m.gradient.push_back(g); }
  vector<double>& modes = S("Vib-Modes");
  vector<double>& ve2 = S("Vib-E2");
  if (!modes.empty() && modes.size() % n3 == 0 && ve2.size() >= 2*(modes.size()/n3)) {
    Vib v; size_t nf = modes.size()/n3;
    v.frequency.assign(ve2.begin(), ve2.begin()+nf);
    v.irIntensity.assign(ve2.begin()+nf, ve2.begin()+2*nf);
    v.mode = modes;
    m.vib.push_back(v);
  }
  const char* cn[2][2] = {{"Mulliken Charges", "mulliken"}, {"ESP Charges", "esp"}};
  for (int i = 0; i < 2; i++)
    if (S(cn[i][0]).size() == N) { Charges c; c.scheme = cn[i][1]; c.q = S(cn[i][0]); m.charges.push_back(c); }
  if (S("Dipole Moment").size() == 3) { Multipoles p; p.dipole = S("Dipole Moment"); m.multipoles.push_back(p); }
  stamp(m, path, "Gaussian");
  string note;
  if (!normaliseSource(m, note, err)) return false;
  m.provenance["ao_convention"] = note;
  return true;
}

// ---- ORCA .hess ----------------------------------------------------------------
bool readOrcaHess(const string& path, Model& m, string& err) {
  std::ifstream in(path.c_str());
  if (!in) { err = "cannot open " + path; return false; }
  string line;
  vector<string> toks;
  vector<double> H, freq, ir;
  vector<double> modes;
  size_t n = 0;
  auto next = [&]() { return bool(std::getline(in, line)); };
  while (next()) {
    string t = trim(line);
    if (t == "$hessian") {
      next(); n = atoi(trim(line).c_str());
      H.assign(n*n, 0.0);
      size_t done = 0;
      while (done < n) {
        next();
        vector<string> cols = words(line);
        for (size_t r = 0; r < n; r++) {
          next();
          vector<string> w = words(line);
          for (size_t c = 0; c < cols.size(); c++) H[r*n + atoi(cols[c].c_str())] = dnum(w[c+1]);
        }
        done += cols.size();
      }
    } else if (t == "$vibrational_frequencies") {
      next(); size_t k = atoi(trim(line).c_str());
      for (size_t i = 0; i < k; i++) { next(); vector<string> w = words(line); freq.push_back(dnum(w[1])); }
    } else if (t == "$normal_modes") {
      next(); vector<string> hd = words(line);
      size_t rows = atoi(hd[0].c_str()), cols = atoi(hd[1].c_str());
      modes.assign(rows*cols, 0.0);
      size_t done = 0;
      while (done < cols) {
        next(); vector<string> c = words(line);
        for (size_t r = 0; r < rows; r++) {
          next(); vector<string> w = words(line);
          for (size_t q = 0; q < c.size(); q++) modes[r*cols + atoi(c[q].c_str())] = dnum(w[q+1]);
        }
        done += c.size();
      }
    } else if (t == "$ir_spectrum") {
      next(); size_t k = atoi(trim(line).c_str());
      for (size_t i = 0; i < k; i++) { next(); vector<string> w = words(line); ir.push_back(dnum(w[2])); }
    }
  }
  if (!n) { err = "no $hessian in " + path; return false; }
  if (m.natoms() && 3*m.natoms() != n) { err = "Hessian size does not match the geometry in the file"; return false; }
  Hessian h; h.method = "ORCA";
  for (size_t i = 0; i < n; i++)
    for (size_t j = i; j < n; j++) h.upper.push_back(0.5*(H[i*n+j] + H[j*n+i]));
  m.hessian.push_back(h);
  if (!freq.empty() && freq.size() == n) {
    Vib v;
    for (size_t i = 0; i < n; i++) {
      if (std::fabs(freq[i]) < 1e-6) continue;
      v.frequency.push_back(freq[i]);
      if (ir.size() == n) v.irIntensity.push_back(ir[i]);
      if (modes.size() == n*n) for (size_t r = 0; r < n; r++) v.mode.push_back(modes[r*n + i]);
    }
    m.vib.push_back(v);
  }
  m.provenance["hessian_source_file"] = baseName(path);
  m.provenance["hessian_source_sha256"] = sha256File(path);
  if (!m.provenance.count("code")) m.provenance["code"] = "ORCA";
  return true;
}

}  // namespace resultsh5
