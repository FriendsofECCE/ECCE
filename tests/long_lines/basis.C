#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include "dsm/ICalcUtils.H"
#include "dsm/TGBSConfig.H"
#include "dsm/TGBSGroup.H"

int main()
{
  const std::string pad(600, ' ');
  const std::string numerical = "NumericalBasis\nbasis \"ao basis\" cartesian print" +
      pad + "\nH S\n1.25" + pad + "0.75\nEND\nEndNumericalBasis\n";
  std::istringstream input(pad + "\n" + numerical);
  std::unique_ptr<TGBSConfig> config(ICalcUtils::importConfig(input));
  TGBSGroup* group = config->findGroup("H");
  if (!group || group->empty()) return 1;
  Contraction_* contraction = group->begin()->second->p_contractions["H"]->at(0);
  if (contraction->exponents[0] != 1.25 || contraction->coefficients[0] != 0.75)
    return 1;

  // Skipping an unsupported ECP must reach the following basis or EOF.
  for (const std::string& ending : {std::string(), "END\n" + numerical}) {
    std::istringstream ecp("NumericalBasis\nECP\n" + pad + "\n" + ending);
    std::unique_ptr<TGBSConfig> result(ICalcUtils::importConfig(ecp));
    if (!ending.empty() && !result->findGroup("H")) return 1;
  }
  for (const std::string section : {"NumericalBasis", "NameBasis"}) {
    std::istringstream truncated(section + "\nbasis unsupported\n" + pad);
    std::unique_ptr<TGBSConfig> result(ICalcUtils::importConfig(truncated));
  }
  std::istringstream failed(numerical);
  failed.setstate(std::ios::failbit);
  std::unique_ptr<TGBSConfig> empty(ICalcUtils::importConfig(failed));
  if (!empty->empty()) return 1;
  std::cout << "PASS basis long lines and failed reads\n";
}
