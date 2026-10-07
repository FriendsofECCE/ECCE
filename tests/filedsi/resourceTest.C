/*
 * resourceTest create|reopen|mopac|g16 <scratch-dir>
 *
 * A project and a calculation made through the classes the apps use
 * (EDSIFactory::getResource, Resource::createChild) under a file:// URL
 * with no data server ("create"); a SEPARATE process then re-opens them
 * from disk alone ("reopen"), so nothing can come from a cache.  Prints "PASS name" / "FAIL name: why"; exit 1 on any FAIL.
 * "mopac" makes proj-mopac/ch4, a MOPAC calculation holding only a methane
 * molecule, which is what the Builder leaves for CalcEd to set up and save.
 * "g16" makes proj-g16/w-c1 and proj-g16/w-c2v, Gaussian-16 calculations
 * holding a distorted (C1) and a symmetric (C2v) water.
 * "summary" reads <scratch-dir>/proj-sum/water-opt (a copy of
 * tests/apps/fixtures/calc-water-opt): the molecule and basis properties,
 * stored on its Parameters documents, must be the calculation's, as the
 * Organizer's summary panel reads them.
 */
#include <iostream>
#include <string>
#include <vector>
using namespace std;

#include "dsm/EDSIFactory.H"
#include "dsm/ICalculation.H"
#include "tdat/Fragment.H"
#include "dsm/Resource.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/ResourceType.H"
#include "dsm/VDoc.H"
#include "util/EcceURL.H"

static int failures = 0;
static void check(bool ok, const string& name, const string& why = "")
{
  if (ok) cout << "PASS " << name << endl;
  else { cout << "FAIL " << name << ": " << why << endl; failures++; }
}

int main(int argc, char **argv)
{
  if (argc < 3) return 2;
  string mode = argv[1];
  string root = argv[2];
  string ns = VDoc::getEcceNamespace();

  if (mode == "reopen") {
    Resource *proj = EDSIFactory::getResource(EcceURL("file://" + root + "/proj"));
    check(proj != 0 && proj->getDescriptor() != 0, "reopen project");
    if (!proj || !proj->getDescriptor()) return 1;
    check(proj->getDescriptor()->getFactoryCategory() == "Project",
          "project type", proj->getDescriptor()->getFactoryCategory());
    vector<Resource*> *kids = proj->getChildren(true);
    check(kids != 0 && kids->size() == 1, "project lists one child");
    if (!kids || kids->empty()) return 1;
    Resource *calc = (*kids)[0];
    check(calc->getName() == "water", "child name", calc->getName());
    check(calc->getDescriptor() != 0 &&
          calc->getDescriptor()->getName() == "nwchem_es",
          "child is an NWChem calculation (virtual_document type)",
          calc->getDescriptor() ? calc->getDescriptor()->getName() : "no type");
    check(calc->getProp(ns + ":state") == "Created", "state",
          calc->getProp(ns + ":state"));
    check(calc->getProp(ns + ":application") == "NWChem", "application",
          calc->getProp(ns + ":application"));
    cout << (failures ? "FAILED " : "ALL OK ") << failures << " failure(s)" << endl;
    return failures ? 1 : 0;
  }

  if (mode == "summary") {
    Resource *calc = EDSIFactory::getResource(
        EcceURL("file://" + root + "/proj-sum/water-opt"));
    check(calc != 0, "open water-opt");
    if (!calc) return 1;
    const char *want[][2] = {
      {"empiricalFormula", "H2O"}, {"numAtoms", "3"}, {"numElectrons", "10"},
      {"symmetrygroup", "C2v"}, {"name", "6-31G*"}, {"coordsys", "N"},
      {"numFunctions", "19"}, {"numPrimitives", "36"},
      {"theory", "SCF/RHF"}, {"state", "Complete"}};
    for (auto& w : want)
      check(calc->getProp(ns + ":" + w[0]) == w[1],
            string("calculation summary ") + w[0], calc->getProp(ns + ":" + w[0]));
    Resource *proj = EDSIFactory::getResource(
        EcceURL("file://" + root + "/proj-sum"));
    check(proj != 0 && proj->getProp(ns + ":empiricalFormula").empty(),
          "a project does not take its calculations' properties");
    cout << (failures ? "FAILED " : "ALL OK ") << failures << " failure(s)" << endl;
    return failures ? 1 : 0;
  }

  if (mode == "mopac") {
    Resource *top = EDSIFactory::getResource(EcceURL("file://" + root));
    ResourceDescriptor& rd = ResourceDescriptor::getResourceDescriptor();
    ResourceType *projType = rd.getResourceType("collection", "ecceProject", "");
    ResourceType *calcType = rd.getResourceType("virtual_document",
                                                "ecceCalculation", "MOPAC");
    Resource *proj = top && projType ? top->createChild("proj-mopac", projType) : 0;
    Resource *calc = proj && calcType ? proj->createChild("ch4", calcType) : 0;
    ICalculation *icalc = dynamic_cast<ICalculation*>(calc);
    check(icalc != 0, "MOPAC calculation created");
    if (!icalc) return 1;
    vector<string> tags = {"C", "H", "H", "H", "H"};
    const double xyz[] = { 0, 0, 0,   0.629, 0.629, 0.629,  -0.629, -0.629, 0.629,
                          -0.629, 0.629, -0.629,   0.629, -0.629, -0.629 };
    const int bonds[] = { 0, 1, 0, 2, 0, 3, 0, 4 };
    Fragment frag("ch4", tags, xyz, 4, bonds);
    check(icalc->fragment(&frag), "methane stored");
    cout << calc->getURL().toString() << endl;
    return failures ? 1 : 0;
  }

  if (mode == "g16") {
    Resource *top = EDSIFactory::getResource(EcceURL("file://" + root));
    ResourceDescriptor& rd = ResourceDescriptor::getResourceDescriptor();
    ResourceType *projType = rd.getResourceType("collection", "ecceProject", "");
    ResourceType *calcType = rd.getResourceType("virtual_document",
                                                "ecceCalculation", "Gaussian-16");
    Resource *proj = top && projType ? top->createChild("proj-g16", projType) : 0;
    vector<string> tags = {"O", "H", "H"};
    const int bonds[] = { 0, 1, 0, 2 };
    const double c1[] = { 0, 0, 0.117,  0.05, 0.757, -0.469,  0, -0.790, -0.440 };
    const double c2v[] = { 0, 0, 0.117,  0, 0.757, -0.469,  0, -0.757, -0.469 };
    struct { const char *name; const double *xyz; const char *group; } calcs[] =
        { {"w-c1", c1, "C1"}, {"w-c2v", c2v, "C2v"} };
    for (auto& c : calcs) {
      Resource *calc = proj && calcType ? proj->createChild(c.name, calcType) : 0;
      ICalculation *icalc = dynamic_cast<ICalculation*>(calc);
      check(icalc != 0, string("Gaussian-16 calculation ") + c.name + " created",
            !top ? "no top" : !projType ? "no project type" : !proj ? "no project"
            : !calcType ? "no Gaussian-16 type" : "createChild failed");
      if (!icalc) return 1;
      Fragment frag(c.name, tags, c.xyz, 2, bonds);
      frag.pointGroup(c.group);
      check(icalc->fragment(&frag), string("water stored in ") + c.name);
      cout << calc->getURL().toString() << endl;
    }
    return failures ? 1 : 0;
  }

  Resource *top = EDSIFactory::getResource(EcceURL("file://" + root));
  check(top != 0, "getResource(root)");
  if (!top) return 1;

  ResourceDescriptor rd = ResourceDescriptor::getResourceDescriptor();
  ResourceType *projType = rd.getResourceType("collection", "ecceProject", "");
  check(projType != 0, "project type found");
  if (!projType) return 1;

  Resource *proj = top->createChild("proj", projType);
  check(proj != 0, "createChild project");
  if (!proj) return 1;
  check(proj->getProp(ns + ":contenttype") == "ecceProject",
        "project contenttype stored", proj->getProp(ns + ":contenttype"));

  ResourceType *calcType = rd.getResourceType("virtual_document",
                                              "ecceCalculation", "NWChem");
  check(calcType != 0, "calculation type found");
  if (!calcType) return 1;
  Resource *calc = proj->createChild("water", calcType);
  check(calc != 0, "createChild calculation");
  if (!calc) return 1;

  check(calc->getProp(ns + ":application") == "NWChem",
        "calc application", calc->getProp(ns + ":application"));
  check(calc->getProp(ns + ":state") == "Created",
        "calc state", calc->getProp(ns + ":state"));

  // Read the state back from disk, not from the cached Resource.
  EDSI *e = EDSIFactory::getEDSI(calc->getURL());
  vector<MetaDataRequest> rq(1);
  rq[0].name = ns + ":state";
  vector<MetaDataResult> res;
  e->getMetaData(rq, res);
  check(res.size() == 1 && res[0].value == "Created",
        "state read back through EDSI", res.empty() ? "absent" : res[0].value);
  delete e;

  cout << (failures ? "FAILED " : "ALL OK ") << failures << " failure(s)" << endl;
  return failures ? 1 : 0;
}
