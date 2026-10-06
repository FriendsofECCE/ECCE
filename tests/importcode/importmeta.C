// Import Calculation from Output File, minus the GUI and the job store:
// runs the code's importer script on the output and TaskJob::import() on
// what it wrote, into a calculation in local mode (ECCE_LOCAL_DATA, #216),
// then prints the chemical-system and basis metadata read back from it --
// what the Organizer and the viewer's Calculation Summary show.
// Usage: importmeta <file> <code>   (ECCE_HOME, ECCE_LOCAL_DATA set)
#include <cstdlib>
#include <iostream>
#include <string>
#include <unistd.h>

#include "util/EcceURL.H"
#include "util/SDirectory.H"
#include "tdat/Fragment.H"
#include "tdat/SpinMult.H"
#include "dsm/ChemistryTask.H"
#include "dsm/CodeFactory.H"
#include "dsm/EDSIFactory.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/ICalculation.H"
#include "dsm/JCode.H"
#include "dsm/Resource.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/ResourceType.H"
#include "dsm/TGBSConfig.H"
#include "dsm/TaskJob.H"

using namespace std;

static string shellQuote(const string& s)
{
  string q = "'";
  for (char c : s) q += (c == '\'') ? string("'\\''") : string(1, c);
  return q + "'";
}

int main(int argc, char **argv)
{
  if (argc != 3) {
    cerr << "usage: importmeta <file> <code>" << endl;
    return 2;
  }
  string path = argv[1], code = argv[2];
  string base = path.substr(path.find_last_of('/') + 1);
  string stem = base.substr(0, base.find_last_of('.'));

  const JCode *jcode = CodeFactory::lookup(code.c_str());
  if (!jcode) { cerr << "no code " << code << endl; return 2; }
  string script = jcode->getScript("CalcImport");
  script.erase(script.find_last_not_of(" \t") + 1);

  EDSIServerCentral central;
  Resource *home = EDSIFactory::getResource(central.getDefaultUserHome());
  ResourceDescriptor& rd = ResourceDescriptor::getResourceDescriptor();
  ResourceType *projType = rd.getResourceType("collection", "ecceProject", "");
  ResourceType *calcType = rd.getResourceType("virtual_document",
                                              "ecceCalculation", code);
  Resource *proj = 0;
  if (home && projType) {
    proj = EDSIFactory::getResource(home->getURL().getChild("import"));
    if (!proj) proj = home->createChild("import", projType);
  }
  Resource *calc = proj && calcType ? proj->createChild(stem, calcType) : 0;
  TaskJob *task = dynamic_cast<TaskJob*>(calc);
  if (!task) { cerr << "could not create a " << code << " calculation" << endl;
               return 1; }
  task->application(jcode);

  // The importer runs in a directory of its own, on a link to the output,
  // as JobParser::importCalculation runs it.
  string work = string(getenv("ECCE_LOCAL_DATA")) + "/work-" + stem;
  string cmd = "mkdir -p " + shellQuote(work) + " && cd " + shellQuote(work) +
               " && ln -sf " + shellQuote(path) + " " + shellQuote(base) +
               " && " + shellQuote(string(getenv("ECCE_HOME")) +
                                   "/scripts/parsers/" + script) +
               " " + shellQuote(base) + " >importer.log 2>&1";
  if (system(cmd.c_str()) != 0) {
    cerr << "importer failed: " << cmd << endl;
    return 1;
  }
  string message;
  try {
    message = task->import(work, base);
  } catch (EcceException& ex) {
    cerr << "import failed: " << ex.what() << endl;
    return 1;
  }

  // fragment() and gbsConfig() read the stored documents back.
  ChemistryTask *chem = dynamic_cast<ChemistryTask*>(calc);
  ICalculation *icalc = dynamic_cast<ICalculation*>(calc);
  Fragment *frag = chem ? chem->fragment() : 0;
  TGBSConfig *config = icalc ? icalc->gbsConfig() : 0;
  cout << "message: " << message << endl;
  cout << "pointgroup: " << (frag ? frag->pointGroup() : "-") << endl;
  cout << "multiplicity: "
       << (icalc ? (int)icalc->spinMultiplicity() : -1) << " "
       << (icalc ? SpinMult::toString(icalc->spinMultiplicity()) : "-") << endl;
  cout << "basis: " << (config ? config->name() : "-") << endl;
  cout << "ecp: " << (config ? config->ecpName() : "-") << endl;
  cout << "spherical: "
       << (config ? (config->coordsys() == TGaussianBasisSet::Spherical
                         ? "yes" : "no") : "-") << endl;
  return 0;
}
