//  Which convention, spherical or Cartesian, is recorded for a calculation's
//  basis when it is chosen, per code?
//
//      testCoordSys <path to data/admin/basissets>     (ECCE_HOME set)
//
//  The MO code reads the recorded convention to know how many functions
//  each d or f shell has.  Most library sets leave the "spherical" flag in
//  their .meta blank, so GBSRules::autoOptimize() recorded Cartesian for
//  them -- also for ORCA, which has no Cartesian functions, and the MOs
//  panel then expected six d functions where ORCA wrote five.  ORCA must
//  record spherical; NWChem and Gaussian, which write the recorded
//  convention into their input, must be unchanged.

#include <fstream>
#include <iostream>
#include <string>

#include "dsm/CodeFactory.H"
#include "dsm/GBSRules.H"
#include "dsm/JCode.H"
#include "dsm/TGBSConfig.H"
#include "dsm/TGBSGroup.H"
#include "dsm/TGaussianBasisSet.H"

using namespace std;

static int failures = 0;
static string basisDir;

static void check(bool ok, const string& what)
{
  cout << (ok ? "  ok:   " : "  FAIL: ") << what << endl;
  if (!ok) failures++;
}

//  The value of the "spherical" field of a library .meta sidecar, as
//  EDSIGaussianBasisSetLibrary reads it: key line, value lines, key line.
static string metaSpherical(const string& file)
{
  ifstream in((basisDir + "/" + file).c_str());
  string line, value;
  bool inField = false;
  while (getline(in, line)) {
    if (line == "spherical") {
      if (inField) break;
      inField = true;
    } else if (inField) {
      value += line;
    }
  }
  return value;
}

//  A water basis from one library set, its convention as the library
//  reports it.
static TGBSConfig *waterConfig(const string& name, const string& meta)
{
  TGaussianBasisSet *gbs = new TGaussianBasisSet();
  gbs->p_name = name;
  gbs->p_type = TGaussianBasisSet::correlation_consistent;
  gbs->p_coordSys = TGaussianBasisSet::strToCoordSys(metaSpherical(meta));
  TGBSGroup *group = new TGBSGroup();
  group->insertGBS(gbs);
  TGBSConfig *config = new TGBSConfig();
  config->insertGBSGroup(string("H O"), group);
  return config;
}

static const char *name(TGaussianBasisSet::CoordinateSystem c)
{
  return c == TGaussianBasisSet::Spherical ? "spherical" :
         c == TGaussianBasisSet::Cartesian ? "Cartesian" : "unknown";
}

static void expect(const string& codeName, const string& basis,
                   const string& meta, TGaussianBasisSet::CoordinateSystem want)
{
  const JCode *code = CodeFactory::lookup(codeName.c_str());
  if (!code) {
    check(false, codeName + ": no EDML");
    return;
  }
  TGBSConfig *config = waterConfig(basis, meta);
  GBSRules::autoOptimize(config, code, 0);
  check(config->coordsys() == want,
        codeName + " " + basis + " records " + name(config->coordsys()) +
        ", want " + name(want));
  delete config;
}

int main(int argc, char **argv)
{
  if (argc != 2) {
    cerr << "usage: testCoordSys <data/admin/basissets>" << endl;
    return 2;
  }
  basisDir = argv[1];
  const TGaussianBasisSet::CoordinateSystem S = TGaussianBasisSet::Spherical;
  const TGaussianBasisSet::CoordinateSystem C = TGaussianBasisSet::Cartesian;

  check(metaSpherical("cc-pVDZ.BAS.meta").empty() &&
        metaSpherical("def2-svp.BAS.meta").empty(),
        "cc-pVDZ and def2-SVP leave the library's spherical flag blank");

  expect("ORCA", "cc-pVDZ", "cc-pVDZ.BAS.meta", S);
  expect("ORCA", "def2-svp", "def2-svp.BAS.meta", S);
  //  Unchanged: these codes are told the recorded convention.
  expect("NWChem", "cc-pVDZ", "cc-pVDZ.BAS.meta", C);
  expect("Gaussian-16", "cc-pVDZ", "cc-pVDZ.BAS.meta", C);

  //  A basis recorded Cartesian by an older client is corrected for ORCA
  //  only.
  TGBSConfig *config = waterConfig("cc-pVDZ", "cc-pVDZ.BAS.meta");
  config->coordsys(C);
  check(!GBSRules::enforceCodeCoordSys(config,
                                       CodeFactory::lookup("NWChem")) &&
        config->coordsys() == C, "NWChem keeps a recorded Cartesian");
  check(GBSRules::enforceCodeCoordSys(config, CodeFactory::lookup("ORCA")) &&
        config->coordsys() == S, "ORCA turns a recorded Cartesian spherical");
  delete config;

  cout << (failures ? "FAILED" : "PASSED") << endl;
  return failures ? 1 : 0;
}
