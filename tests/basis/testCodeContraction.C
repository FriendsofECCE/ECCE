//  gbscodecontraction <BASISPATTERN text> <element>  < NumericalBasis text
//
//  Reads one element's basis (ICalcUtils::importConfig's NumericalBasis
//  format, made from ECCE's library file by the test), applies the code's contraction
//  pattern (ICalcUtils::applyContractionPattern, #942), flattens it for ORCA
//  and prints one block per shell: "SHELL <element> <l>", then
//  "<exponent> <coefficient>" for each primitive with a nonzero coefficient
//  (coefficients of unnormalised primitives, as EspField evaluates them).
//  tests/basis/code_contraction_test.py compares this with ORCA's own print.

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "dsm/CodeFactory.H"
#include "dsm/ICalcUtils.H"
#include "dsm/JCode.H"
#include "dsm/TGBSConfig.H"
#include "dsm/TGaussianBasisSet.H"
#include "tdat/BasisFlatten.H"
#include "tdat/EspField.H"
#include "tdat/TGBSAngFunc.H"

using namespace std;

int main(int argc, char **argv)
{
  if (argc != 3) {
    cerr << "usage: gbscodecontraction <pattern> <element> < basis" << endl;
    return 2;
  }
  vector<string> elements(1, argv[2]);
  TGBSConfig *config = ICalcUtils::importConfig(cin);
  const JCode *code = CodeFactory::lookup("ORCA");
  if (config == 0 || config->empty() || code == 0) {
    cerr << "no basis or no ORCA EDML" << endl;
    return 1;
  }
  const bool changed = ICalcUtils::applyContractionPattern(argv[1], config);
  cout << "changed " << (changed ? 1 : 0) << endl;

  TGBSAngFunc *af = code->getAngFunc(TGaussianBasisSet::Spherical);
  const int lenSph[7] = { 1, 3, 5, 7, 9, 11, 13 };
  const vector<double> coords(3*elements.size(), 0.0);
  vector<EspBasisFunction> basis;
  if (af == 0 || !BasisFlatten::flatten(elements, coords, config, code, af,
                                        af->maxShells(), lenSph, basis)) {
    cerr << "flatten failed" << endl;
    return 1;
  }
  //  One block per shell: its first component (spherical m = 0).
  size_t i = 0;
  cout.precision(12);
  while (i < basis.size()) {
    const EspBasisFunction& f = basis[i];
    if (f.powerX.empty()) return 1;
    const int l = f.powerX[0] + f.powerY[0] + f.powerZ[0];
    cout << "SHELL " << elements[0] << " " << l << "\n";
    for (size_t k = 0; k < f.exponent.size(); k++)
      if (f.contraction[k] != 0.0)
        cout << f.exponent[k] << " " << f.contraction[k] << "\n";
    i += 2*l + 1;
  }
  delete af;
  delete config;
  return 0;
}
