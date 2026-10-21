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
 * "nwchem" makes proj-nw/nh3-energy, nh3-geom (run type Geometry) and nh3-orca
 * (an ORCA calculation), nh3-qe (Quantum ESPRESSO), each holding NH3 with a dummy atom X bonded to N,
 * for the Geometry Constraints tests; empty-nw holds no structure.
 * "cp2k" makes proj-cp2k/si and si-geom (CP2K calculations, run type Energy and
 * Geometry, holding two silicon atoms in a face-centred cell), which the Calculation Editor turns into a CP2K input file.
 * "summary" reads <scratch-dir>/proj-sum/water-opt (a copy of
 * tests/apps/fixtures/calc-water-opt): the molecule and basis properties,
 * stored on its Parameters documents, must be the calculation's, as the
 * Organizer's summary panel reads them.
 */
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

#include "dsm/EDSIFactory.H"
#include "dsm/ICalculation.H"
#include "tdat/Fragment.H"
#include "tdat/LatticeDef.H"
#include "dsm/Resource.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/ResourceType.H"
#include "dsm/VDoc.H"
#include "tdat/TRunType.H"
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

  if (mode == "nwchem") {
    Resource *top = EDSIFactory::getResource(EcceURL("file://" + root));
    ResourceDescriptor& rd = ResourceDescriptor::getResourceDescriptor();
    ResourceType *projType = rd.getResourceType("collection", "ecceProject", "");
    Resource *proj = top && projType ? top->createChild("proj-nw", projType) : 0;
    vector<string> tags = {"N", "H", "H", "H", "X"};
    const int bonds[] = { 0, 1, 0, 2, 0, 3, 0, 4 };
    const double xyz[] = { 0, 0, 0.1,  0, 0.94, -0.3,  0.81, -0.47, -0.3,
                           -0.81, -0.47, -0.3,  0, 0, 1.1 };
    struct { const char *name; const char *code; const char *run; } calcs[] =
        { {"nh3-energy", "NWChem", "Energy"}, {"nh3-geom", "NWChem", "Geometry"},
          {"nh3-orca", "ORCA", "Energy"}, {"nh3-qe", "QuantumESPRESSO", "Energy"},
          {"empty-nw", "NWChem", "Energy"} };
    for (auto& c : calcs) {
      ResourceType *calcType = rd.getResourceType("virtual_document",
                                                  "ecceCalculation", c.code);
      Resource *calc = proj && calcType ? proj->createChild(c.name, calcType) : 0;
      ICalculation *icalc = dynamic_cast<ICalculation*>(calc);
      check(icalc != 0, string(c.code) + " calculation " + c.name + " created");
      if (!icalc) return 1;
      icalc->runtype(TRunType(c.run, false));
      if (string(c.name) == "empty-nw") {      // a structure still to be drawn
        cout << calc->getURL().toString() << endl;
        continue;
      }
      Fragment frag(c.name, tags, xyz, 4, bonds);
      check(icalc->fragment(&frag), string("NH3+X stored in ") + c.name);
      cout << calc->getURL().toString() << endl;
    }
    return failures ? 1 : 0;
  }

  if (mode == "cp2k") {
    Resource *top = EDSIFactory::getResource(EcceURL("file://" + root));
    ResourceDescriptor& rd = ResourceDescriptor::getResourceDescriptor();
    ResourceType *projType = rd.getResourceType("collection", "ecceProject", "");
    ResourceType *calcType = rd.getResourceType("virtual_document",
                                                "ecceCalculation", "CP2K");
    Resource *proj = top && projType ? top->createChild("proj-cp2k", projType) : 0;
    vector<string> tags = {"Si", "Si"};
    const double xyz[] = { 0, 0, 0,   0.96, 0.96, 0.96 };
    const char *names[] = { "si", "si-geom" };
    const char *runs[] = { "Energy", "Geometry" };
    for (int i = 0; i < 2; i++) {
      Resource *calc = proj && calcType ? proj->createChild(names[i], calcType) : 0;
      ICalculation *icalc = dynamic_cast<ICalculation*>(calc);
      check(icalc != 0, string("CP2K calculation ") + names[i] + " created");
      if (!icalc) return 1;
      icalc->runtype(TRunType(runs[i], false));
      Fragment frag(names[i], tags, xyz, 0, 0);
      frag.setLattice(new LatticeDef(3.84, 3.84, 3.84, 60.0, 60.0, 60.0));
      check(icalc->fragment(&frag), "two silicon atoms and a cell stored");
      cout << calc->getURL().toString() << endl;
    }
    return failures ? 1 : 0;
  }

  if (mode == "qst") {
    // qst <dir> <name>=<file.xyz> ...: Gaussian-16 calculations in proj-qst,
    // each holding the structure of its xyz file (bonds by distance), for
    // the Opt=QST2/QST3 end-to-end test (#419).
    Resource *top = EDSIFactory::getResource(EcceURL("file://" + root));
    ResourceDescriptor& rd = ResourceDescriptor::getResourceDescriptor();
    ResourceType *projType = rd.getResourceType("collection", "ecceProject", "");
    ResourceType *calcType = rd.getResourceType("virtual_document",
                                                "ecceCalculation", "Gaussian-16");
    Resource *proj = top && projType ? top->createChild("proj-qst", projType) : 0;
    for (int a = 3; a < argc; a++) {
      string spec = argv[a];
      size_t eq = spec.find('=');
      string name = spec.substr(0, eq);
      ifstream in(spec.substr(eq + 1).c_str());
      int n = 0;
      string line;
      getline(in, line);
      n = atoi(line.c_str());
      getline(in, line);
      vector<string> tags;
      vector<double> xyz;
      for (int i = 0; i < n; i++) {
        string e; double x, y, z;
        in >> e >> x >> y >> z;
        tags.push_back(e);
        xyz.push_back(x); xyz.push_back(y); xyz.push_back(z);
      }
      vector<int> bonds;
      for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n; j++) {
          double d2 = 0;
          for (int k = 0; k < 3; k++)
            d2 += (xyz[3*i+k] - xyz[3*j+k]) * (xyz[3*i+k] - xyz[3*j+k]);
          if (d2 < 1.6 * 1.6) { bonds.push_back(i); bonds.push_back(j); }
        }
      Resource *calc = proj && calcType ? proj->createChild(name, calcType) : 0;
      ICalculation *icalc = dynamic_cast<ICalculation*>(calc);
      check(icalc != 0, "Gaussian-16 calculation " + name + " created");
      if (!icalc) return 1;
      Fragment frag(name, tags, &xyz[0], (int)bonds.size() / 2, &bonds[0]);
      check(icalc->fragment(&frag), "structure stored in " + name);
      cout << calc->getURL().toString() << endl;
    }
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
