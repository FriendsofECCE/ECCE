/*
 * resourceTest <scratch-dir>
 *
 * The least-invasive higher-level check: a project and a calculation made
 * through the classes the apps use (EDSIFactory::getResource and
 * Resource::createChild) under a file:// URL, with no data server, then
 * read back.  Prints "PASS name" / "FAIL name: why"; exit 1 on any FAIL.
 */
#include <iostream>
#include <string>
#include <vector>
using namespace std;

#include "dsm/EDSIFactory.H"
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
  if (argc < 2) return 2;
  string root = argv[1];
  string ns = VDoc::getEcceNamespace();

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
