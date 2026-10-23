//  A group holding the same orbital shells twice (a named def2 set and an
//  "Imported" copy, as script-made calculations store them) counts and
//  draws one basis, while a genuinely different second set (polarization
//  functions appended) is still added.
#include <iostream>
#include <string>
#include <vector>

#include "dsm/TGBSConfig.H"
#include "dsm/TGBSGroup.H"
#include "dsm/TGaussianBasisSet.H"

using namespace std;

static int failures = 0;

static void check(bool ok, const string& what)
{
  cout << (ok ? "  ok:   " : "  FAIL: ") << what << endl;
  if (!ok) failures++;
}

static TGaussianBasisSet *hydrogenSet(const string& name, double exponent,
                                      char shell)
{
  TGaussianBasisSet *gbs = new TGaussianBasisSet();
  gbs->p_name = name;
  gbs->p_type = TGaussianBasisSet::UnknownGBSType;
  gbs->p_coordSys = TGaussianBasisSet::Spherical;
  ContractionSet *set = new ContractionSet();
  Contraction_ *c = new Contraction_();
  c->num_exponents = c->num_coefficients = 1;
  c->exponents = new double[1];
  c->coefficients = new double[1];
  c->exponents[0] = exponent;
  c->coefficients[0] = 1.0;
  c->shells.push_back(TGaussianBasisSet::charToShell(shell));
  set->push_back(c);
  gbs->p_contractions["H"] = set;
  return gbs;
}

int main()
{
  TGBSGroup *group = new TGBSGroup();
  group->insertGBS(hydrogenSet("def2-tzvp", 0.8, 'S'));
  group->insertGBS(hydrogenSet("Imported", 0.8, 'S'));
  TGBSConfig *config = new TGBSConfig();
  config->insertGBSGroup(string("H"), group);
  check(config->getGBSList("H").size() == 1,
        "identical named and Imported sets give one basis set");

  group->insertGBS(hydrogenSet("pol", 1.1, 'P'));
  check(config->getGBSList("H").size() == 2,
        "a different appended set is kept");
  cout << (failures ? "FAILED" : "PASSED") << endl;
  return failures ? 1 : 0;
}
