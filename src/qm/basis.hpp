// Gaussian basis set for a molecule, in the layout libcint expects.
//
// AO order (documented because MO coefficients are exported):
//  * atoms in input order; within an atom, shells sorted by angular momentum
//    (stable, so the library order is kept within one l) -- the order ECCE's
//    own basis storage uses;
//  * spherical shells: s; p = x,y,z; l>=2 real solid harmonics m = -l..+l as
//    in libcint (d: xy, yz, z2, xz, x2-y2);
//  * Cartesian shells: libcint order (d: xx, xy, xz, yy, yz, zz).
// Contracted functions are normalised to 1 (spherical).  Cartesian d and
// higher are normalised like x^l (so xy has norm 1/sqrt(3) of xx's).
#pragma once
#include <string>
#include <vector>
#include "basislib.hpp"
#include "molecule.hpp"

namespace qm {

struct Shell {
  int atom = 0;
  int l = 0;
  std::vector<double> exps;
  std::vector<double> raw_coefs;  // as in the library file (for Molden)
  std::vector<double> coefs;  // include primitive and contraction normalisation
  int ao0 = 0;                // first AO index
  int nao = 0;
  double center[3] = {0, 0, 0};
  double extent = 0;          // radius beyond which |phi| < 1e-10 (bohr)
};

class Basis {
 public:
  // spherical: -1 = the basis set's own convention, 0 = Cartesian, 1 = spherical.
  Basis(const Molecule& mol, BasisLibrary& lib, const std::string& name, int spherical = -1);

  // True when the named basis is conventionally used with spherical d/f.
  static bool default_spherical(const std::string& name);

  int nbf() const { return nbf_; }
  const std::string& name() const { return name_; }
  BasisLibrary& library() const { return *lib_; }
  bool spherical() const { return sph_; }
  const std::vector<Shell>& shells() const { return shells_; }
  const Molecule& molecule() const { return mol_; }
  std::string ao_label(int ao) const;  // e.g. "O 1s" style: "0O  2pz"

  // libcint arrays (valid for the lifetime of this object).
  std::vector<int>& atm() { return atm_; }
  std::vector<int>& bas() { return bas_; }
  std::vector<double>& env() { return env_; }
  int natm() const { return static_cast<int>(atm_.size() / 6); }
  int nshl() const { return static_cast<int>(shells_.size()); }

  static int nao_of(int l, bool sph) { return sph ? 2 * l + 1 : (l + 1) * (l + 2) / 2; }

 private:
  Molecule mol_;
  std::string name_;
  BasisLibrary* lib_ = nullptr;
  bool sph_ = true;
  int nbf_ = 0;
  std::vector<Shell> shells_;
  std::vector<int> atm_, bas_;
  std::vector<double> env_;
};

}  // namespace qm
