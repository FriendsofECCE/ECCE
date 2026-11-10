#include "results_h5.hpp"

#include <algorithm>
#include <cstdio>

#ifdef ECCE_QM_RESULTS_H5
#include "tdat/ResultsH5.H"
#include "Internal.H"   // normaliseSource
#endif

namespace qm {

#ifndef ECCE_QM_RESULTS_H5
bool write_results_h5(const std::string&, const Input&, const Basis&, const Molecule&, const ScfResult&,
                      const FrequencyOutput*, const ResultsTraces&, std::string& why) {
  why = "ecce-qm was built without HDF5";
  return false;
}
#else

namespace {

// Basis AO index of each canonical AO: spherical m = 0,+1,-1,... (libcint
// stores m = -l..l); p and Cartesian shells are already in canonical order.
std::vector<int> canonicalOrder(const Basis& b) {
  std::vector<int> perm(b.nbf());
  for (const auto& s : b.shells()) {
    const int n = Basis::nao_of(s.l, b.spherical());
    if (s.l <= 1 || !b.spherical()) {
      for (int k = 0; k < n; ++k) perm[s.ao0 + k] = s.ao0 + k;
      continue;
    }
    int pos = 0;
    for (int m = 0; m <= s.l; ++m) {
      perm[s.ao0 + pos++] = s.ao0 + s.l + m;
      if (m > 0) perm[s.ao0 + pos++] = s.ao0 + s.l - m;
    }
  }
  return perm;
}

resultsh5::Mos mosOf(int spin, const Mat& C, const Vec& eps, const Vec& occ, const std::vector<int>& perm) {
  resultsh5::Mos mo;
  mo.spin = spin;
  mo.nao = C.rows();
  mo.nmo = C.cols();
  mo.coef.resize(mo.nmo * mo.nao);
  for (size_t i = 0; i < mo.nmo; ++i)
    for (size_t k = 0; k < mo.nao; ++k) mo.coef[i * mo.nao + k] = C(perm[k], i);
  mo.energy.assign(eps.data(), eps.data() + eps.size());
  mo.occupation.assign(occ.data(), occ.data() + occ.size());
  return mo;
}

resultsh5::Trace traceOf(const std::string& name, const std::vector<TracePoint>& pts, size_t natoms) {
  resultsh5::Trace t;
  t.name = name;
  bool grad = true;
  for (const TracePoint& p : pts) {
    const std::vector<int> idx = p.mol.real_atoms();
    if (idx.size() != natoms) return resultsh5::Trace();
    for (int a : idx)
      for (int k = 0; k < 3; ++k) t.coord.push_back(p.mol.atoms[a].x[k]);
    t.energy.push_back(p.energy);
    t.value.push_back(p.value);
    grad = grad && p.grad.size() == 3 * p.mol.atoms.size();
    if (grad)
      for (int a : idx)
        for (int k = 0; k < 3; ++k) t.gradient.push_back(p.grad[3 * a + k]);
  }
  if (!grad) t.gradient.clear();
  return t;
}

}  // namespace

bool write_results_h5(const std::string& path, const Input& in, const Basis& b, const Molecule& at,
                      const ScfResult& r, const FrequencyOutput* fo, const ResultsTraces& traces,
                      std::string& why) {
  resultsh5::Model m;
  for (const Atom& a : at.atoms) {
    m.atomicNumber.push_back(a.Z);
    m.label.push_back(element_symbol(a.Z));
    for (int k = 0; k < 3; ++k) m.coord.push_back(a.x[k]);
  }
  m.geomEnergy.push_back(r.energy);
  m.nelectron[0] = at.nalpha();
  m.nelectron[1] = at.nbeta();
  m.basis.name = in.basis;
  for (const auto& s : b.shells()) {
    resultsh5::Shell sh;
    sh.atom = s.atom;
    sh.l = s.l;
    sh.cartesian = !b.spherical();
    sh.exponent = s.exps;
    sh.coef = s.raw_coefs;
    m.basis.shells.push_back(sh);
  }
  const std::vector<int> perm = canonicalOrder(b);
  // As in ECCE's parsed properties: one spin-0 set with occupations 0..2
  // for a restricted calculation, alpha and beta otherwise.
  if (r.reference == Reference::Restricted) {
    m.mos.push_back(mosOf(0, r.C_a, r.eps_a, r.occ_a + r.occ_b, perm));
  } else {
    m.mos.push_back(mosOf(0, r.C_a, r.eps_a, r.occ_a, perm));
    m.mos.push_back(mosOf(1, r.C_b, r.eps_b, r.occ_b, perm));
  }
  std::string note, err;
  if (!resultsh5::normaliseSource(m, note, err)) {
    why = "orbitals not expressible in the canonical basis: " + err;
    return false;
  }
  m.provenance["code"] = "ECCE-QM";
  m.provenance["writer"] = "ecce-qm";
  m.provenance["method"] = in.scf.method;
  m.provenance["basis_name"] = in.basis;
  m.provenance["ao_convention"] = note;
  m.provenance["converter_version"] = resultsh5::CONVERTER_VERSION;

  const size_t n3 = 3 * at.atoms.size();
  if (fo != nullptr) {
    if (static_cast<size_t>(fo->hessian.rows()) == n3 && static_cast<size_t>(fo->hessian.cols()) == n3) {
      resultsh5::Hessian h;
      h.method = in.scf.method;
      for (size_t i = 0; i < n3; ++i)
        for (size_t j = i; j < n3; ++j) h.upper.push_back(fo->hessian(i, j));
      m.hessian.push_back(h);
    }
    resultsh5::Vib v;
    v.frequency = fo->vib.freq;
    v.irIntensity = fo->vib.ir;
    v.ramanActivity = fo->vib.raman;
    if (static_cast<size_t>(fo->vib.modes.rows()) == n3)
      for (int k = 0; k < fo->vib.modes.cols(); ++k)
        for (size_t i = 0; i < n3; ++i) v.mode.push_back(fo->vib.modes(i, k));
    if (!v.frequency.empty()) m.vib.push_back(v);
  }
  if (traces.scan.size() > 0) {
    resultsh5::Trace t = traceOf("scan", traces.scan, at.atoms.size());
    t.valueUnit = traces.scan_unit;
    if (!t.coord.empty()) m.trace.push_back(t);
  }
  if (traces.optimization.size() > 1) {
    resultsh5::Trace t = traceOf("optimization", traces.optimization, at.atoms.size());
    t.value.clear();
    if (!t.coord.empty()) m.trace.push_back(t);
  }

  // The job store's limits (tr(PS) = N to 1e-4 relative, C'SC = 1 to 1e-5);
  // the Hessian's frequencies against ours are reported, not judged.
  const resultsh5::Invariants inv = resultsh5::check(m);
  double rel = 0;
  for (const resultsh5::Mos& mo : m.mos) {
    double n = 0;
    for (double o : mo.occupation) n += o;
    if (n > 0) rel = std::max(rel, inv.traceErr[mo.spin & 1] / n);
  }
  const bool ok = inv.haveMos && rel <= 1e-4 && inv.orthoErr <= 1e-5;
  char buf[200];
  snprintf(buf, sizeof buf, "tr(PS) rel err %.1e, max |C'SC-1| %.1e, %zu AOs (ecce-qm)", rel, inv.orthoErr,
           m.basis.nao());
  std::string line = buf;
  if (inv.haveFreq) {
    snprintf(buf, sizeof buf, "; frequencies from the Hessian within %.2f cm-1", inv.freqDiff);
    line += buf;
  }
  m.checks = (ok ? "ok: " : "failed: ") + line;
  if (!resultsh5::writeFile(path, m, err)) {
    why = err;
    return false;
  }
  return true;
}

#endif

}  // namespace qm
