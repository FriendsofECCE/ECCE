#include <iomanip>
#include <sstream>

#include "tdat/SlaterBasisSet.H"
#include "tdat/SlaterExpansion.H"

using std::ostringstream;
using std::endl;
using std::setw;
using std::setprecision;

namespace {

  //  Row of the periodic table, indexed by atomic number.
  const int PERIOD[] = {
    0,
    1, 1,                                                   // H  - He
    2, 2, 2, 2, 2, 2, 2, 2,                                 // Li - Ne
    3, 3, 3, 3, 3, 3, 3, 3,                                 // Na - Ar
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,   // K  - Kr
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,   // Rb - Xe
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,   // Cs - Lu ..
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6
  };
  const int PERIOD_MAX = sizeof(PERIOD)/sizeof(PERIOD[0]) - 1;
}


void SlaterBasisSet::principalQuantumNumbers(int atomicNumber,
                                             int& nS, int& nP, int& nD)
{
  nS = nP = nD = 0;
  if (atomicNumber < 1 || atomicNumber > PERIOD_MAX) return;

  const int period = PERIOD[atomicNumber];

  nS = period;
  nP = period;
  nD = period;

  //  A noble gas has no room left in its own shell, so its valence s
  //  function belongs to the next one.
  if (atomicNumber == 10 || atomicNumber == 18 || atomicNumber == 36 ||
      atomicNumber == 54 || atomicNumber == 86) {
    nS = period + 1;
  }
  //  Helium's period has no p shell at all.
  if (atomicNumber == 2) {
    nP = 2;
  }
  //  A d function first exists one period below the one it fills into,
  //  which is why nD is the period itself and not period-1: MOPAC
  //  numbers it as the valence shell.
}


bool SlaterBasisSet::numericalBasis(const vector<Element>& elements,
                                    int nGauss, string& basis)
{
  ostringstream os;

  //  Spherical, because the five d functions a semiempirical code uses
  //  are the spherical set, not the six Cartesian ones.
  os << "NumericalBasis" << endl;
  os << "basis \"ao basis\" spherical print" << endl;

  bool any = false;

  for (size_t e = 0; e < elements.size(); e++) {
    const Element& el = elements[e];

    int nS, nP, nD;
    principalQuantumNumbers(el.atomicNumber, nS, nP, nD);
    if (nS == 0) return false;               // element out of range

    const double zetas[3] = { el.zetaS, el.zetaP, el.zetaD };
    const int    ns[3]    = { nS, nP, nD };
    const char*  labels[3] = { "S", "P", "D" };

    for (int l = 0; l < 3; l++) {
      if (zetas[l] <= 0.0) continue;         // shell not used
      if (ns[l] <= l) return false;          // n must exceed l

      vector<double> exponents, coefficients;
      double quality = SlaterExpansion::fit(ns[l], l, zetas[l], nGauss,
                                            exponents, coefficients);
      //  Refuse rather than render a poor fit: a wrong orbital still
      //  looks like an orbital, so there is nothing downstream that
      //  would notice.
      if (quality < 0.99) return false;

      os << el.symbol << "    " << labels[l] << endl;
      for (size_t i = 0; i < exponents.size(); i++) {
        os.setf(std::ios::fixed|std::ios::showpoint|std::ios::right);
        os << setw(23) << setprecision(12) << exponents[i];
        os << setw(23) << setprecision(12) << coefficients[i];
        os << endl;
      }
      any = true;
    }
  }

  if (!any) return false;

  os << "END" << endl;
  os << "EndNumericalBasis" << endl;

  basis = os.str();
  return true;
}
