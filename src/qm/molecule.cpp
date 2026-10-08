#include "molecule.hpp"
#include <cctype>
#include <cmath>

namespace qm {

static const char* kSymbols[] = {
    "X",  "H",  "He", "Li", "Be", "B",  "C",  "N",  "O",  "F",  "Ne", "Na",
    "Mg", "Al", "Si", "P",  "S",  "Cl", "Ar", "K",  "Ca", "Sc", "Ti", "V",
    "Cr", "Mn", "Fe", "Co", "Ni", "Cu", "Zn", "Ga", "Ge", "As", "Se", "Br",
    "Kr", "Rb", "Sr", "Y",  "Zr", "Nb", "Mo", "Tc", "Ru", "Rh", "Pd", "Ag",
    "Cd", "In", "Sn", "Sb", "Te", "I",  "Xe", "Cs", "Ba", "La", "Ce", "Pr", "Nd",
    "Pm", "Sm", "Eu", "Gd", "Tb", "Dy", "Ho", "Er", "Tm", "Yb", "Lu", "Hf", "Ta", "W",
    "Re", "Os", "Ir", "Pt", "Au", "Hg", "Tl", "Pb", "Bi", "Po", "At", "Rn"};
static const int kNSym = sizeof(kSymbols) / sizeof(kSymbols[0]);

const char* element_symbol(int Z) { return (Z >= 0 && Z < kNSym) ? kSymbols[Z] : "X"; }

int atomic_number(const std::string& s) {
  // Accept a bare atomic number or a symbol, optionally followed by digits
  // or an underscore label ("C1", "H_a").
  if (!s.empty() && std::isdigit(static_cast<unsigned char>(s[0]))) {
    int z = std::atoi(s.c_str());
    if (z >= 1 && z < kNSym) return z;
    throw Error("unsupported atomic number " + s);
  }
  std::string t;
  for (char c : s) {
    if (!std::isalpha(static_cast<unsigned char>(c))) break;
    t += static_cast<char>(t.empty() ? std::toupper(c) : std::tolower(c));
  }
  for (int z = 1; z < kNSym; ++z)
    if (t == kSymbols[z]) return z;
  // "CA" in a label is calcium only when the user meant it; try the first letter.
  if (t.size() > 1) {
    std::string one(1, t[0]);
    for (int z = 1; z < kNSym; ++z)
      if (one == kSymbols[z]) return z;
  }
  throw Error("unknown element '" + s + "'");
}

int Molecule::nelectrons() const {
  int n = -charge;
  for (const auto& a : atoms) n += a.Z;
  return n;
}
int Molecule::nalpha() const { return (nelectrons() + multiplicity - 1) / 2; }
int Molecule::nbeta() const { return nelectrons() - nalpha(); }

void Molecule::check() const {
  int n = nelectrons();
  if (n < 0 || multiplicity < 1 || (n + multiplicity) % 2 == 0 || multiplicity - 1 > n)
    throw Error("charge " + std::to_string(charge) + " and multiplicity " +
                std::to_string(multiplicity) + " are inconsistent with " +
                std::to_string(n) + " electrons");
}

double Molecule::nuclear_repulsion() const {
  double e = 0;
  for (size_t i = 0; i < atoms.size(); ++i)
    for (size_t j = 0; j < i; ++j) {
      double dx = atoms[i].x[0] - atoms[j].x[0], dy = atoms[i].x[1] - atoms[j].x[1],
             dz = atoms[i].x[2] - atoms[j].x[2];
      e += atoms[i].Z * atoms[j].Z / std::sqrt(dx * dx + dy * dy + dz * dz);
    }
  return e;
}

}  // namespace qm
