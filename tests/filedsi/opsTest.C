/*
 * opsTest <scratch-dir>
 *
 * The Organizer's operations on a file:// store, through the calls the
 * Organizer makes (Resource::move for rename, cut-paste and drag;
 * Resource::copy for paste; Resource::remove; Resource::descendantSearch
 * for Find; EDSI::isLocked), then the same directory changed by several
 * processes at once.  Prints "PASS name" / "FAIL name: why"; exit 1 on any
 * FAIL.
 */
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "dsm/EDSI.H"
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

static bool onDisk(const string& path)
{
  struct stat st;
  return stat(path.c_str(), &st) == 0;
}

static string slurp(const string& path)
{
  ifstream in(path.c_str());
  stringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

static EcceURL url(const string& path) { return EcceURL("file://" + path); }

// A property read back through a fresh EDSI, not from any cached Resource.
static string diskProp(const string& path, const string& name)
{
  EDSI *e = EDSIFactory::getEDSI(url(path));
  vector<MetaDataRequest> rq(1);
  rq[0].name = name;
  vector<MetaDataResult> res;
  string ret;
  if (e->getMetaData(rq, res))
    for (size_t i = 0; i < res.size(); i++)
      if (res[i].name == name) ret = res[i].value;
  delete e;
  return ret;
}

static bool putProp(const string& path, const string& name, const string& v)
{
  EDSI *e = EDSIFactory::getEDSI(url(path));
  MetaDataResult m;
  m.name = name;
  m.type = "string";
  m.value = v;
  bool ok = e->putMetaData(vector<MetaDataResult>(1, m));
  delete e;
  return ok;
}

static bool hasChild(Resource *parent, const string& name)
{
  vector<Resource*> *kids = parent->getChildren(true);
  if (!kids) return false;
  for (size_t i = 0; i < kids->size(); i++)
    if ((*kids)[i]->getName() == name) return true;
  return false;
}

// Several processes rename files and write properties in one directory
// at once; every property must survive and every file keep its own.
static void concurrent(const string& root)
{
  const int NPROC = 4, N = 40;
  string dir = root + "/busy";
  mkdir(dir.c_str(), 0755);
  for (int p = 0; p < NPROC; p++)
    for (int i = 0; i < N; i++) {
      char n[64];
      sprintf(n, "/f%d_%d", p, i);
      ofstream(dir + n) << n;
      putProp(dir + n, "ecce:owner", n + 1);
    }
  vector<pid_t> kids;
  for (int p = 0; p < NPROC; p++) {
    pid_t pid = fork();
    if (pid == 0) {
      int bad = 0;
      for (int i = 0; i < N; i++) {
        char from[64], to[64], prop[64];
        sprintf(from, "/f%d_%d", p, i);
        sprintf(to, "/g%d_%d", p, i);
        sprintf(prop, "ecce:extra%d", i);
        EDSI *e = EDSIFactory::getEDSI(url(dir + from));
        EcceURL target = url(dir + to);
        if (!e->moveResource(target)) bad++;
        delete e;
        if (!putProp(dir + to, prop, "x")) bad++;
      }
      _exit(bad ? 1 : 0);
    }
    kids.push_back(pid);
  }
  bool exits = true;
  for (size_t k = 0; k < kids.size(); k++) {
    int st = 0;
    waitpid(kids[k], &st, 0);
    if (!WIFEXITED(st) || WEXITSTATUS(st) != 0) exits = false;
  }
  check(exits, "concurrent renames and property writes all succeeded");
  int lost = 0;
  for (int p = 0; p < NPROC; p++)
    for (int i = 0; i < N; i++) {
      char g[64], owner[64], prop[64];
      sprintf(g, "/g%d_%d", p, i);
      sprintf(owner, "f%d_%d", p, i);
      sprintf(prop, "ecce:extra%d", i);
      if (diskProp(dir + g, "ecce:owner") != owner) lost++;
      if (diskProp(dir + g, prop) != "x") lost++;
    }
  char why[64];
  sprintf(why, "%d properties lost or misplaced", lost);
  check(lost == 0, "concurrent renames keep every file's properties", why);
}

int main(int argc, char **argv)
{
  if (argc < 2) return 2;
  string root = argv[1];
  string ns = VDoc::getEcceNamespace();
  string state = ns + ":state";

  ResourceDescriptor& rd = ResourceDescriptor::getResourceDescriptor();
  ResourceType *projType = rd.getResourceType("collection", "ecceProject", "");
  ResourceType *calcType = rd.getResourceType("virtual_document",
                                              "ecceCalculation", "NWChem");
  Resource *top = EDSIFactory::getResource(url(root));
  Resource *a = top ? top->createChild("projA", projType) : 0;
  Resource *b = top ? top->createChild("projB", projType) : 0;
  Resource *water = a ? a->createChild("water", calcType) : 0;
  Resource *ethane = a ? a->createChild("ethane", calcType) : 0;
  check(water && ethane && b, "projects and calculations created");
  if (!(water && ethane && b)) return 1;
  putProp(root + "/projA/ethane", state, "Ready");
  { ofstream(root + "/projA/notes.txt") << "notes"; }
  { ofstream(root + "/projA/other.txt") << "other"; }
  putProp(root + "/projA/notes.txt", ns + ":tag", "n");

  // isLocked: nothing locks a resource for long, as with DavEDSI.
  {
    EDSI *e = EDSIFactory::getEDSI(water->getURL());
    string locker;
    bool locked = e->isLocked(locker);
    check(!locked && e->m_msgStack.size() == 0, "isLocked false, no error",
          e->m_msgStack.getMessage());
    delete e;
  }

  // Find (WxFind: descendantSearch on the URL text below the start).
  {
    vector<EcceURL> hits = top->descendantSearch("DAV:creationdate", "wat");
    bool found = false;
    for (size_t i = 0; i < hits.size(); i++)
      if (hits[i].getFilePathTail() == "water") found = true;
    check(found, "find 'wat' finds the calculation",
          top->messages());
    bool meta = false;
    hits = top->descendantSearch("DAV:creationdate", "meta");
    for (size_t i = 0; i < hits.size(); i++)
      if (hits[i].toString().find(".ecce-meta") != string::npos) meta = true;
    check(!meta, "find never returns the property sidecar");
    hits = top->descendantSearch("DAV:creationdate", "zzzz");
    check(hits.empty() && top->messages().empty(), "find with no hit",
          top->messages());
  }

  // Rename a calculation in place.
  EcceURL oldWater = water->getURL();
  EcceURL target = url(root + "/projA/water2");
  bool ok = water->move(target);
  check(ok, "rename calculation", water->messages());
  check(!onDisk(root + "/projA/water") && onDisk(root + "/projA/water2"),
        "renamed on disk");
  check(water->getURL().getFilePathTail() == "water2",
        "Resource follows its rename", water->getURL().toString());
  Resource *again = EDSIFactory::getResource(url(root + "/projA/water2"));
  check(again && again->getProp(state) == "Created" &&
        again->getDescriptor() && again->getDescriptor()->getName() == "nwchem_es",
        "renamed calculation re-opens with its state and type");
  check(EDSIFactory::getResource(oldWater) == 0, "old name not served from the pool");
  check(hasChild(a, "water2") && !hasChild(a, "water"), "project lists new name");

  // Rename onto an existing name must refuse, losing nothing.
  target = url(root + "/projA/ethane");
  ok = water->move(target);
  check(!ok, "rename onto an existing calculation refused");
  check(diskProp(root + "/projA/ethane", state) == "Ready" &&
        diskProp(root + "/projA/water2", state) == "Created",
        "both calculations keep their state");
  {
    Resource *notes = EDSIFactory::getResource(url(root + "/projA/notes.txt"));
    target = url(root + "/projA/other.txt");
    ok = notes && notes->move(target);
    check(!ok, "rename onto an existing file refused");
    check(slurp(root + "/projA/other.txt") == "other" &&
          slurp(root + "/projA/notes.txt") == "notes",
          "both files keep their contents");
  }

  // Cut and paste / drag: move into another project.
  target = url(root + "/projB/water2");
  ok = water->move(target);
  check(ok, "move calculation to another project", water->messages());
  check(diskProp(root + "/projB/water2", state) == "Created" &&
        diskProp(root + "/projB/water2", ns + ":application") == "NWChem",
        "moved calculation keeps its properties");
  check(hasChild(b, "water2") && !hasChild(a, "water2"), "both projects list it right");

  // Paste (copy): onto a free name, then onto a taken one.
  target = url(root + "/projA/water2");
  ok = water->copy(target);
  check(ok && onDisk(root + "/projA/water2") &&
        diskProp(root + "/projA/water2", state) == "Created",
        "copy calculation", water->messages());
  target = url(root + "/projA/water2");
  ok = water->copy(target);
  check(ok && target.getFilePathTail() == "water2-1" &&
        onDisk(root + "/projA/water2-1"),
        "copy onto a taken name gets a new one",
        target.toString() + " " + water->messages());
  {
    Resource *notes = EDSIFactory::getResource(url(root + "/projA/notes.txt"));
    target = url(root + "/projA/other.txt");
    ok = notes && notes->copy(target);
    check(ok && target.getFilePathTail() == "other-1.txt" &&
          slurp(root + "/projA/other.txt") == "other" &&
          slurp(root + "/projA/other-1.txt") == "notes" &&
          diskProp(root + "/projA/other-1.txt", ns + ":tag") == "n",
          "copy of a file onto a taken name keeps both",
          target.toString());
  }

  // EDSI overwrite modes (Resource always passes SORTOF or NO).
  {
    { ofstream(root + "/projA/x.txt") << "x"; }
    putProp(root + "/projA/x.txt", ns + ":tag", "x");
    putProp(root + "/projA/other.txt", ns + ":old", "stale");
    EDSI *e = EDSIFactory::getEDSI(url(root + "/projA/x.txt"));
    target = url(root + "/projA/other.txt");
    check(!e->copyResource(target, EDSI::NO) &&
          slurp(root + "/projA/other.txt") == "other",
          "copy with overwrite NO leaves the target");
    target = url(root + "/projA/other.txt");
    check(e->copyResource(target, EDSI::YES) &&
          slurp(root + "/projA/other.txt") == "x" &&
          diskProp(root + "/projA/other.txt", ns + ":tag") == "x" &&
          diskProp(root + "/projA/other.txt", ns + ":old") == "",
          "copy with overwrite YES replaces content and properties");
    delete e;
  }

  // Delete a calculation, then a whole project.
  Resource *copy1 = EDSIFactory::getResource(url(root + "/projA/water2-1"));
  ok = copy1 && copy1->remove();
  check(ok && !onDisk(root + "/projA/water2-1") && !hasChild(a, "water2-1"),
        "delete calculation");
  ok = b->remove();
  check(ok && !onDisk(root + "/projB") && !hasChild(top, "projB"),
        "delete project");
  check(EDSIFactory::getResource(url(root + "/projB/water2")) == 0,
        "deleted calculation not served from the pool");

  concurrent(root);

  cout << (failures ? "FAILED " : "ALL OK ") << failures << " failure(s)" << endl;
  return failures ? 1 : 0;
}
