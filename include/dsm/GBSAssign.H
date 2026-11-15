/**
 * @file
 *
 * Puts a library basis set on one group (element or atom tag) of a basis
 * configuration.  Shared by the Basis Set Tool and the Calculation Editor's
 * basis panel so both store the same thing for the same choice.
 */
#ifndef GBSASSIGN_HH
#define GBSASSIGN_HH

#include <string>

#include "dsm/TGaussianBasisSet.H"

class EDSIGaussianBasisSetLibrary;
class TGBSConfig;
class TGBSGroup;

class GBSAssign
{
public:
  enum Severity { NONE, WARNING, ERROR };

  struct Result {
    TGBSGroup *group = 0;       // the group that holds the set; 0: nothing done
    Severity severity = NONE;
    std::string message;        // for the user when severity != NONE
  };

  // bstype is the library section the set is listed in.  An orbital set
  // replaces the group's orbital sets unless multAllowed; an auxiliary
  // one is added to the group's existing sets.
  static Result assign(EDSIGaussianBasisSetLibrary& library,
                       TGBSConfig *config, const std::string& tag,
                       const std::string& bsName,
                       TGaussianBasisSet::GBSType bstype, bool multAllowed);
};

#endif
