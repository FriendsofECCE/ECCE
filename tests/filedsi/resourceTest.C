/*
 * resourceTest create|reopen|mopac <scratch-dir>
 *
 * A project and a calculation made through the classes the apps use
 * (EDSIFactory::getResource, Resource::createChild) under a file:// URL
 * with no data server ("create"); a SEPARATE process then re-opens them
 * from disk alone ("reopen"), so nothing can come from a cache.  Prints "PASS name" / "FAIL name: why"; exit 1 on any FAIL.
 * "mopac" makes proj-mopac/ch4, a MOPAC calculation holding only a methane
 * molecule, which is what the Builder leaves for CalcEd to set up and save.
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
