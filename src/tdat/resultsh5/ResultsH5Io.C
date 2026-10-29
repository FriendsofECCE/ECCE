// results.h5 reader/writer (HDF5 C API).  Layout: docs/claude/codereg/results-h5-format.md
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <hdf5.h>
#include "Internal.H"

namespace resultsh5 {
bool compression = true;
namespace {

//  Small arrays go compact (data inside the object header: no heap or
//  chunk index), larger ones chunked with shuffle+deflate.  HDF5's fixed
//  cost per dataset is what decides whether tiny molecules beat their
//  source files, so none of it is spent on empty or unneeded objects.
const size_t COMPACT_MAX = 1024;

hid_t dcplFor(int rank, const hsize_t* dims, size_t esize) {
  hid_t p = H5Pcreate(H5P_DATASET_CREATE);
  H5Pset_obj_track_times(p, 0);
  size_t total = esize;
  for (int i = 0; i < rank; i++) total *= dims[i];
  if (total <= COMPACT_MAX) {
    H5Pset_layout(p, H5D_COMPACT);
  } else {
    hsize_t ch[4];
    size_t cnt = 1;
    for (int i = 0; i < rank; i++) { ch[i] = dims[i]; cnt *= dims[i]; }
    const size_t maxElem = 1 << 17;
    if (cnt > maxElem) ch[0] = std::max<hsize_t>(1, ch[0]*maxElem/cnt);
    H5Pset_chunk(p, rank, ch);
    if (compression) { H5Pset_shuffle(p); H5Pset_deflate(p, 9); }
  }
  return p;
}

void putArr(hid_t loc, const char* name, hid_t ftype, hid_t mtype, size_t esize,
            int rank, const hsize_t* dims, const void* data) {
  hid_t sp = H5Screate_simple(rank, dims, 0);
  hid_t dcpl = dcplFor(rank, dims, esize);
  hid_t d = H5Dcreate2(loc, name, ftype, sp, H5P_DEFAULT, dcpl, H5P_DEFAULT);
  H5Dwrite(d, mtype, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
  H5Dclose(d); H5Pclose(dcpl); H5Sclose(sp);
}

hid_t putD(hid_t loc, const char* name, const vector<double>& v, int rank = 1,
           const hsize_t* dims = 0, bool keep = false) {
  if (v.empty()) return -1;
  hsize_t d1 = v.size();
  putArr(loc, name, H5T_IEEE_F64LE, H5T_NATIVE_DOUBLE, 8, rank, dims ? dims : &d1, &v[0]);
  return keep ? H5Dopen2(loc, name, H5P_DEFAULT) : -1;
}

void putI(hid_t loc, const char* name, const vector<int>& v) {
  if (v.empty()) return;
  hsize_t d = v.size();
  putArr(loc, name, H5T_STD_I32LE, H5T_NATIVE_INT, 4, 1, &d, &v[0]);
}

void putStrs(hid_t loc, const char* name, const vector<string>& v) {
  if (v.empty()) return;
  size_t w = 1;
  for (size_t i = 0; i < v.size(); i++) w = std::max(w, v[i].size());
  vector<char> buf(v.size()*w, 0);
  for (size_t i = 0; i < v.size(); i++) memcpy(&buf[i*w], v[i].data(), v[i].size());
  hid_t t = H5Tcopy(H5T_C_S1);
  H5Tset_size(t, w);
  H5Tset_strpad(t, H5T_STR_NULLPAD);
  hsize_t d = v.size();
  putArr(loc, name, t, t, w, 1, &d, &buf[0]);
  H5Tclose(t);
}

void attrStr(hid_t o, const char* name, const string& s) {
  hid_t t = H5Tcopy(H5T_C_S1);
  H5Tset_size(t, std::max<size_t>(1, s.size()));
  H5Tset_strpad(t, H5T_STR_NULLPAD);
  hid_t sp = H5Screate(H5S_SCALAR);
  hid_t a = H5Acreate2(o, name, t, sp, H5P_DEFAULT, H5P_DEFAULT);
  H5Awrite(a, t, s.data());
  H5Aclose(a); H5Sclose(sp); H5Tclose(t);
}
void attrInt(hid_t o, const char* name, long long v) {
  hid_t sp = H5Screate(H5S_SCALAR);
  hid_t a = H5Acreate2(o, name, H5T_STD_I64LE, sp, H5P_DEFAULT, H5P_DEFAULT);
  H5Awrite(a, H5T_NATIVE_LLONG, &v);
  H5Aclose(a); H5Sclose(sp);
}
void attrDbl(hid_t o, const char* name, double v) {
  hid_t sp = H5Screate(H5S_SCALAR);
  hid_t a = H5Acreate2(o, name, H5T_IEEE_F64LE, sp, H5P_DEFAULT, H5P_DEFAULT);
  H5Awrite(a, H5T_NATIVE_DOUBLE, &v);
  H5Aclose(a); H5Sclose(sp);
}

hid_t grp(hid_t loc, const char* name) {
  hid_t gcpl = H5Pcreate(H5P_GROUP_CREATE);
  H5Pset_obj_track_times(gcpl, 0);
  hid_t g = H5Gcreate2(loc, name, H5P_DEFAULT, gcpl, H5P_DEFAULT);
  H5Pclose(gcpl);
  return g;
}

string num(const char* base, size_t i) { char b[64]; snprintf(b, sizeof b, "%s/%zu", base, i); return b; }

// ---- reading ----
bool getD(hid_t loc, const char* name, vector<double>& v, vector<hsize_t>* dims = 0) {
  v.clear();
  if (H5Lexists(loc, name, H5P_DEFAULT) <= 0) return false;
  hid_t d = H5Dopen2(loc, name, H5P_DEFAULT);
  hid_t sp = H5Dget_space(d);
  int r = H5Sget_simple_extent_ndims(sp);
  vector<hsize_t> dm(r);
  H5Sget_simple_extent_dims(sp, &dm[0], 0);
  hssize_t n = H5Sget_simple_extent_npoints(sp);
  v.resize(n);
  if (n) H5Dread(d, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, &v[0]);
  if (dims) *dims = dm;
  H5Sclose(sp); H5Dclose(d);
  return true;
}
bool getI(hid_t loc, const char* name, vector<int>& v) {
  v.clear();
  if (H5Lexists(loc, name, H5P_DEFAULT) <= 0) return false;
  hid_t d = H5Dopen2(loc, name, H5P_DEFAULT);
  hid_t sp = H5Dget_space(d);
  hssize_t n = H5Sget_simple_extent_npoints(sp);
  v.resize(n);
  if (n) H5Dread(d, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &v[0]);
  H5Sclose(sp); H5Dclose(d);
  return true;
}
bool getStrs(hid_t loc, const char* name, vector<string>& v) {
  v.clear();
  if (H5Lexists(loc, name, H5P_DEFAULT) <= 0) return false;
  hid_t d = H5Dopen2(loc, name, H5P_DEFAULT);
  hid_t t = H5Dget_type(d);
  size_t w = H5Tget_size(t);
  hid_t sp = H5Dget_space(d);
  hssize_t n = H5Sget_simple_extent_npoints(sp);
  vector<char> buf(n*w + 1, 0);
  H5Dread(d, t, H5S_ALL, H5S_ALL, H5P_DEFAULT, &buf[0]);
  for (hssize_t i = 0; i < n; i++) v.push_back(string(&buf[i*w], strnlen(&buf[i*w], w)));
  H5Sclose(sp); H5Tclose(t); H5Dclose(d);
  return true;
}
string rdStr(hid_t o, const char* name) {
  if (H5Aexists(o, name) <= 0) return "";
  hid_t a = H5Aopen(o, name, H5P_DEFAULT);
  hid_t t = H5Aget_type(a);
  size_t w = H5Tget_size(t);
  vector<char> b(w + 1, 0);
  H5Aread(a, t, &b[0]);
  H5Tclose(t); H5Aclose(a);
  return string(&b[0], strnlen(&b[0], w));
}
long long rdInt(hid_t o, const char* name, long long dflt = 0) {
  if (H5Aexists(o, name) <= 0) return dflt;
  hid_t a = H5Aopen(o, name, H5P_DEFAULT);
  long long v = dflt;
  H5Aread(a, H5T_NATIVE_LLONG, &v);
  H5Aclose(a);
  return v;
}
double rdDbl(hid_t o, const char* name) {
  if (H5Aexists(o, name) <= 0) return 0;
  hid_t a = H5Aopen(o, name, H5P_DEFAULT);
  double v = 0;
  H5Aread(a, H5T_NATIVE_DOUBLE, &v);
  H5Aclose(a);
  return v;
}
vector<string> members(hid_t g) {
  vector<string> out;
  H5G_info_t gi;
  H5Gget_info(g, &gi);
  for (hsize_t i = 0; i < gi.nlinks; i++) {
    char nm[256];
    H5Lget_name_by_idx(g, ".", H5_INDEX_NAME, H5_ITER_INC, i, nm, sizeof nm, H5P_DEFAULT);
    out.push_back(nm);
  }
  return out;
}
bool has(hid_t f, const char* path) { return H5Lexists(f, path, H5P_DEFAULT) > 0; }
// attribute/dataset on an object addressed by path
hid_t openObj(hid_t f, const string& p) { return H5Oopen(f, p.c_str(), H5P_DEFAULT); }

}  // namespace

bool writeFile(const string& path, const Model& m, string& err) {
  hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
  H5Pset_libver_bounds(fapl, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST);
  // The default 2 kB metadata/small-data blocks are most of a small file.
  H5Pset_meta_block_size(fapl, 1);
  H5Pset_small_data_block_size(fapl, 1);
  hid_t f = H5Fcreate(path.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, fapl);
  H5Pclose(fapl);
  if (f < 0) { err = "cannot create " + path; return false; }
  size_t N = m.natoms();
  attrInt(f, "format_version", FORMAT_VERSION);
  if (m.nelectron[0] >= 0) { attrInt(f, "nelectron_alpha", m.nelectron[0]); attrInt(f, "nelectron_beta", m.nelectron[1]); }

  if (!m.provenance.empty()) {
    hid_t g = grp(f, "provenance");
    for (std::map<string,string>::const_iterator i = m.provenance.begin(); i != m.provenance.end(); ++i)
      attrStr(g, i->first.c_str(), i->second);
    H5Gclose(g);
  }
  if (N) {
    hid_t g = grp(f, "nucleus");
    putI(g, "atomic_number", m.atomicNumber);
    putD(g, "charge", m.nuclearCharge);
    putStrs(g, "label", m.label);
    H5Gclose(g);
    g = grp(f, "geometry");
    hsize_t d[3] = {m.nframes(), N, 3};
    putD(g, "coord", m.coord, 3, d);
    putD(g, "energy", m.geomEnergy);
    H5Gclose(g);
  }
  if (m.hasCell) {
    hid_t g = grp(f, "cell");
    vector<double> l(m.lattice, m.lattice+9);
    hsize_t d[2] = {3, 3};
    putD(g, "lattice", l, 2, d);
    attrInt(g, "pbc", m.pbc[0] | (m.pbc[1] << 1) | (m.pbc[2] << 2));
    H5Gclose(g);
  }
  if (!m.basis.shells.empty() || m.basis.orthonormal || m.basis.type != "GTO") {
    hid_t g = grp(f, "basis");
    attrStr(g, "type", m.basis.type);
    attrInt(g, "orthonormal", m.basis.orthonormal);
    if (!m.basis.name.empty()) attrStr(g, "name", m.basis.name);
    vector<int> atom, ang, cart, nq, nprim;
    vector<double> e, c;
    bool anyCart = false, anyN = false;
    for (size_t i = 0; i < m.basis.shells.size(); i++) {
      const Shell& s = m.basis.shells[i];
      atom.push_back(s.atom); ang.push_back(s.l); cart.push_back(s.cartesian);
      nq.push_back(s.n); nprim.push_back(int(s.exponent.size()));
      anyCart |= s.cartesian; anyN |= s.n != 0;
      e.insert(e.end(), s.exponent.begin(), s.exponent.end());
      c.insert(c.end(), s.coef.begin(), s.coef.end());
    }
    putI(g, "shell_nucleus", atom);
    putI(g, "shell_ang_mom", ang);
    if (anyCart) putI(g, "shell_cartesian", cart);
    if (anyN) putI(g, "shell_n", nq);
    putI(g, "shell_nprim", nprim);
    putD(g, "exponent", e);
    putD(g, "coefficient", c);
    H5Gclose(g);
  }
  if (!m.ecp.kind.empty()) {
    hid_t g = grp(f, "ecp");
    attrStr(g, "kind", m.ecp.kind);
    putI(g, "nucleus", m.ecp.atom);
    putStrs(g, "name", m.ecp.name);
    putI(g, "ncore", m.ecp.ncore);
    putD(g, "parameters", m.ecp.params);
    H5Gclose(g);
  }
  if (!m.mos.empty()) {
    hid_t g0 = grp(f, "mo");
    for (size_t k = 0; k < m.mos.size(); k++) {
      const Mos& mo = m.mos[k];
      char nm[32]; snprintf(nm, sizeof nm, "%zu", k);
      hid_t g = grp(g0, nm);
      attrInt(g, "spin", mo.spin); attrInt(g, "geometry", mo.geometry);
      hsize_t d[2] = {mo.nmo, mo.nao};
      putD(g, "coefficient", mo.coef, 2, d);
      putD(g, "energy", mo.energy);
      putD(g, "occupation", mo.occupation);
      putStrs(g, "symmetry", mo.symmetry);
      H5Gclose(g);
    }
    H5Gclose(g0);
  }
  if (!m.hessian.empty()) {
    hid_t g = grp(f, "hessian");
    for (size_t k = 0; k < m.hessian.size(); k++) {
      char nm[32]; snprintf(nm, sizeof nm, "%zu", k);
      hid_t o = putD(g, nm, m.hessian[k].upper, 1, 0, true);
      if (o < 0) continue;
      attrInt(o, "geometry", m.hessian[k].geometry);
      attrStr(o, "method", m.hessian[k].method);
      H5Dclose(o);
    }
    H5Gclose(g);
  }
  if (!m.gradient.empty()) {
    hid_t g = grp(f, "gradient");
    for (size_t k = 0; k < m.gradient.size(); k++) {
      char nm[32]; snprintf(nm, sizeof nm, "%zu", k);
      hid_t o = putD(g, nm, m.gradient[k].g, 1, 0, true);
      if (o < 0) continue;
      attrInt(o, "geometry", m.gradient[k].geometry);
      H5Dclose(o);
    }
    H5Gclose(g);
  }
  if (!m.charges.empty()) {
    hid_t g = grp(f, "charges");
    for (size_t k = 0; k < m.charges.size(); k++) {
      string nm = m.charges[k].scheme;
      if (m.charges[k].geometry) nm += "." + std::to_string(m.charges[k].geometry);
      hid_t o = putD(g, nm.c_str(), m.charges[k].q, 1, 0, true);
      if (o < 0) continue;
      attrStr(o, "scheme", m.charges[k].scheme);
      attrInt(o, "geometry", m.charges[k].geometry);
      H5Dclose(o);
    }
    H5Gclose(g);
  }
  if (!m.multipoles.empty()) {
    hid_t g = grp(f, "multipoles");
    for (size_t k = 0; k < m.multipoles.size(); k++) {
      char nm[32]; snprintf(nm, sizeof nm, "%zu", k);
      hid_t s = grp(g, nm);
      attrInt(s, "geometry", m.multipoles[k].geometry);
      putD(s, "dipole", m.multipoles[k].dipole);
      putD(s, "quadrupole", m.multipoles[k].quadrupole);
      H5Gclose(s);
    }
    H5Gclose(g);
  }
  if (!m.vib.empty()) {
    hid_t g0 = grp(f, "vib");
    for (size_t k = 0; k < m.vib.size(); k++) {
      char nm[32]; snprintf(nm, sizeof nm, "%zu", k);
      hid_t g = grp(g0, nm);
      const Vib& v = m.vib[k];
      attrInt(g, "geometry", v.geometry);
      putD(g, "frequency", v.frequency);
      putD(g, "ir_intensity", v.irIntensity);
      putD(g, "reduced_mass", v.reducedMass);
      if (!v.mode.empty() && !v.frequency.empty()) {
        hsize_t d[2] = {v.frequency.size(), v.mode.size()/v.frequency.size()};
        putD(g, "mode", v.mode, 2, d);
      }
      H5Gclose(g);
    }
    H5Gclose(g0);
  }
  if (!m.tensor.empty()) {
    hid_t g = grp(f, "tensors");
    for (size_t k = 0; k < m.tensor.size(); k++) {
      const Tensor& t = m.tensor[k];
      string nm = t.name + (t.geometry ? "." + std::to_string(t.geometry) : "");
      vector<hsize_t> d(t.dims.begin(), t.dims.end());
      hid_t o = putD(g, nm.c_str(), t.data, int(d.size()), &d[0], true);
      if (o < 0) continue;
      attrStr(o, "name", t.name); attrInt(o, "geometry", t.geometry);
      H5Dclose(o);
    }
    H5Gclose(g);
  }
  if (m.bands.nk) {
    hid_t g = grp(f, "bands");
    const Bands& b = m.bands;
    attrInt(g, "nspin", b.nspin);
    if (b.hasFermi) attrDbl(g, "fermi_energy", b.fermi);
    hsize_t d3[3] = {b.nspin, b.nk, b.nband}, dk[2] = {b.nk, 3};
    putD(g, "kpoint", b.kpoint, 2, dk);
    putD(g, "weight", b.weight);
    putD(g, "energy", b.energy, 3, d3);
    putD(g, "occupation", b.occupation, 3, d3);
    H5Gclose(g);
  }
  if (!m.trajectory.empty()) {
    hid_t g0 = grp(f, "trajectory");
    for (size_t k = 0; k < m.trajectory.size(); k++) {
      char nm[32]; snprintf(nm, sizeof nm, "%zu", k);
      hid_t g = grp(g0, nm);
      attrStr(g, "file", m.trajectory[k].file); attrStr(g, "sha256", m.trajectory[k].sha256);
      attrInt(g, "nframes", m.trajectory[k].nframes); attrDbl(g, "dt", m.trajectory[k].dt);
      H5Gclose(g);
    }
    H5Gclose(g0);
  }
  if (!m.series.empty()) {
    hid_t g0 = grp(f, "series");
    for (size_t k = 0; k < m.series.size(); k++) {
      char nm[32]; snprintf(nm, sizeof nm, "%zu", k);
      hid_t g = grp(g0, nm);
      attrStr(g, "name", m.series[k].name); attrStr(g, "unit", m.series[k].unit);
      putD(g, "time", m.series[k].time); putD(g, "value", m.series[k].value);
      H5Gclose(g);
    }
    H5Gclose(g0);
  }
  H5Fclose(f);
  return true;
}

bool readFile(const string& path, Model& m, string& err) {
  m = Model();
  H5E_auto2_t oldf; void* oldd;
  H5Eget_auto2(H5E_DEFAULT, &oldf, &oldd);
  H5Eset_auto2(H5E_DEFAULT, 0, 0);
  hid_t f = H5Fopen(path.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
  H5Eset_auto2(H5E_DEFAULT, oldf, oldd);
  if (f < 0) { err = "cannot open " + path; return false; }
  m.formatVersion = int(rdInt(f, "format_version", 0));
  if (m.formatVersion != FORMAT_VERSION) {
    err = "unsupported format_version " + std::to_string(m.formatVersion);
    H5Fclose(f); return false;
  }
  m.nelectron[0] = int(rdInt(f, "nelectron_alpha", -1));
  m.nelectron[1] = int(rdInt(f, "nelectron_beta", -1));
  if (has(f, "provenance")) {
    hid_t g = H5Gopen2(f, "provenance", H5P_DEFAULT);
    // H5Oget_info changed signature between HDF5 1.10 and 1.12; walk by index instead.
    for (hsize_t i = 0;; i++) {
      char nm[256];
      if (H5Aget_name_by_idx(g, ".", H5_INDEX_NAME, H5_ITER_INC, i, nm, sizeof nm, H5P_DEFAULT) < 0) break;
      m.provenance[nm] = rdStr(g, nm);
    }
    H5Gclose(g);
  }
  if (has(f, "nucleus")) {
    hid_t g = H5Gopen2(f, "nucleus", H5P_DEFAULT);
    getI(g, "atomic_number", m.atomicNumber);
    getD(g, "charge", m.nuclearCharge);
    getStrs(g, "label", m.label);
    H5Gclose(g);
    g = H5Gopen2(f, "geometry", H5P_DEFAULT);
    getD(g, "coord", m.coord);
    getD(g, "energy", m.geomEnergy);
    H5Gclose(g);
  }
  if (has(f, "cell")) {
    hid_t g = H5Gopen2(f, "cell", H5P_DEFAULT);
    vector<double> l;
    getD(g, "lattice", l);
    if (l.size() == 9) { m.hasCell = true; std::copy(l.begin(), l.end(), m.lattice); }
    int p = int(rdInt(g, "pbc"));
    for (int i = 0; i < 3; i++) m.pbc[i] = (p >> i) & 1;
    H5Gclose(g);
  }
  if (has(f, "basis")) {
    hid_t g = H5Gopen2(f, "basis", H5P_DEFAULT);
    m.basis.type = rdStr(g, "type");
    m.basis.orthonormal = rdInt(g, "orthonormal") != 0;
    m.basis.name = rdStr(g, "name");
    vector<int> atom, ang, cart, nq, nprim;
    vector<double> e, c;
    getI(g, "shell_nucleus", atom); getI(g, "shell_ang_mom", ang);
    getI(g, "shell_cartesian", cart); getI(g, "shell_n", nq); getI(g, "shell_nprim", nprim);
    getD(g, "exponent", e); getD(g, "coefficient", c);
    size_t off = 0;
    for (size_t i = 0; i < atom.size(); i++) {
      Shell s;
      s.atom = atom[i]; s.l = ang[i];
      s.cartesian = !cart.empty() && cart[i];
      s.n = nq.empty() ? 0 : nq[i];
      if (off + nprim[i] > e.size() || off + nprim[i] > c.size()) { err = "truncated basis"; H5Gclose(g); H5Fclose(f); return false; }
      s.exponent.assign(e.begin()+off, e.begin()+off+nprim[i]);
      s.coef.assign(c.begin()+off, c.begin()+off+nprim[i]);
      off += nprim[i];
      m.basis.shells.push_back(s);
    }
    H5Gclose(g);
  }
  if (has(f, "ecp")) {
    hid_t g = H5Gopen2(f, "ecp", H5P_DEFAULT);
    m.ecp.kind = rdStr(g, "kind");
    getI(g, "nucleus", m.ecp.atom); getStrs(g, "name", m.ecp.name);
    getI(g, "ncore", m.ecp.ncore); getD(g, "parameters", m.ecp.params);
    H5Gclose(g);
  }
  if (has(f, "mo")) {
    hid_t g0 = H5Gopen2(f, "mo", H5P_DEFAULT);
    vector<string> names = members(g0);
    std::sort(names.begin(), names.end(), [](const string& a, const string& b) { return atoi(a.c_str()) < atoi(b.c_str()); });
    for (size_t k = 0; k < names.size(); k++) {
      hid_t g = H5Gopen2(g0, names[k].c_str(), H5P_DEFAULT);
      Mos mo;
      mo.spin = int(rdInt(g, "spin")); mo.geometry = int(rdInt(g, "geometry"));
      vector<hsize_t> d;
      getD(g, "coefficient", mo.coef, &d);
      if (d.size() == 2) { mo.nmo = d[0]; mo.nao = d[1]; }
      getD(g, "energy", mo.energy); getD(g, "occupation", mo.occupation);
      getStrs(g, "symmetry", mo.symmetry);
      m.mos.push_back(mo);
      H5Gclose(g);
    }
    H5Gclose(g0);
  }
  struct Arrays {
    static void each(hid_t f, const char* grp_, std::function<void(hid_t, const string&)> fn) {
      if (!has(f, grp_)) return;
      hid_t g = H5Gopen2(f, grp_, H5P_DEFAULT);
      vector<string> n = members(g);
      std::sort(n.begin(), n.end(), [](const string& a, const string& b) { return atoi(a.c_str()) < atoi(b.c_str()); });
      for (size_t i = 0; i < n.size(); i++) fn(g, n[i]);
      H5Gclose(g);
    }
  };
  Arrays::each(f, "hessian", [&](hid_t g, const string& nm) {
    Hessian h; getD(g, nm.c_str(), h.upper);
    hid_t o = openObj(g, nm);
    h.geometry = int(rdInt(o, "geometry")); h.method = rdStr(o, "method");
    H5Oclose(o);
    m.hessian.push_back(h);
  });
  Arrays::each(f, "gradient", [&](hid_t g, const string& nm) {
    Gradient h; getD(g, nm.c_str(), h.g);
    hid_t o = openObj(g, nm); h.geometry = int(rdInt(o, "geometry")); H5Oclose(o);
    m.gradient.push_back(h);
  });
  Arrays::each(f, "charges", [&](hid_t g, const string& nm) {
    Charges c; getD(g, nm.c_str(), c.q);
    hid_t o = openObj(g, nm);
    c.scheme = rdStr(o, "scheme"); c.geometry = int(rdInt(o, "geometry"));
    H5Oclose(o);
    m.charges.push_back(c);
  });
  Arrays::each(f, "multipoles", [&](hid_t g, const string& nm) {
    hid_t s = H5Gopen2(g, nm.c_str(), H5P_DEFAULT);
    Multipoles p; p.geometry = int(rdInt(s, "geometry"));
    getD(s, "dipole", p.dipole); getD(s, "quadrupole", p.quadrupole);
    H5Gclose(s); m.multipoles.push_back(p);
  });
  Arrays::each(f, "vib", [&](hid_t g, const string& nm) {
    hid_t s = H5Gopen2(g, nm.c_str(), H5P_DEFAULT);
    Vib v; v.geometry = int(rdInt(s, "geometry"));
    getD(s, "frequency", v.frequency); getD(s, "ir_intensity", v.irIntensity);
    getD(s, "reduced_mass", v.reducedMass); getD(s, "mode", v.mode);
    H5Gclose(s); m.vib.push_back(v);
  });
  Arrays::each(f, "tensors", [&](hid_t g, const string& nm) {
    Tensor t; vector<hsize_t> d;
    getD(g, nm.c_str(), t.data, &d);
    t.dims.assign(d.begin(), d.end());
    hid_t o = openObj(g, nm);
    t.name = rdStr(o, "name"); t.geometry = int(rdInt(o, "geometry"));
    H5Oclose(o);
    m.tensor.push_back(t);
  });
  if (has(f, "bands")) {
    hid_t g = H5Gopen2(f, "bands", H5P_DEFAULT);
    Bands& b = m.bands;
    b.nspin = rdInt(g, "nspin", 1);
    if (H5Aexists(g, "fermi_energy") > 0) { b.hasFermi = true; b.fermi = rdDbl(g, "fermi_energy"); }
    vector<hsize_t> d;
    getD(g, "kpoint", b.kpoint); getD(g, "weight", b.weight);
    getD(g, "energy", b.energy, &d); getD(g, "occupation", b.occupation);
    if (d.size() == 3) { b.nk = d[1]; b.nband = d[2]; }
    H5Gclose(g);
  }
  Arrays::each(f, "trajectory", [&](hid_t g, const string& nm) {
    hid_t s = H5Gopen2(g, nm.c_str(), H5P_DEFAULT);
    TrajectoryRef t; t.file = rdStr(s, "file"); t.sha256 = rdStr(s, "sha256");
    t.nframes = rdInt(s, "nframes"); t.dt = rdDbl(s, "dt");
    H5Gclose(s); m.trajectory.push_back(t);
  });
  Arrays::each(f, "series", [&](hid_t g, const string& nm) {
    hid_t s = H5Gopen2(g, nm.c_str(), H5P_DEFAULT);
    Series t; t.name = rdStr(s, "name"); t.unit = rdStr(s, "unit");
    getD(s, "time", t.time); getD(s, "value", t.value);
    H5Gclose(s); m.series.push_back(t);
  });
  H5Fclose(f);
  return true;
}

}  // namespace resultsh5
