/*
 * homeTest <data-folder>   (run with ECCE_LOCAL_DATA=<data-folder>,
 *                           ECCE_SERVER_LOGIN=tester)
 *
 * Local mode writes only inside the user's own home, <dir>/users/<user>,
 * as a data server refuses writes to users/ and the root: a refused write
 * returns failure with the NOT_PRIVLEDGES message the Organizer shows for
 * a server's 403.  Reads, and paths outside the data folder, are unaffected.
 * Prints "PASS name" / "FAIL name: why"; exit 1 on any FAIL.
 */
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
using namespace std;

#include <stdio.h>
#include <sys/stat.h>

#include "dsm/EDSI.H"
#include "dsm/EDSIFactory.H"
#include "dsm/EDSIServerCentral.H"
#include "util/EcceURL.H"
#include "util/LocalData.H"

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

static EDSI *edsi(const string& path)
{
  return EDSIFactory::getEDSI(EcceURL("file://" + path));
}

static bool refused(EDSI *e)
{
  string m = e->m_msgStack.getMessage();
  return m.find("privileges") != string::npos;
}

int main(int argc, char **argv)
{
  if (argc < 2) return 2;
  string root = argv[1];
  string home = root + "/users/tester";

  // First start: the home appears with the server list, not via FileEDSI.
  {
    EDSIServerCentral central;
    check(onDisk(home), "first start creates users/<user>");
  }
  check(LocalData::userHome() == home, "userHome is users/tester",
        LocalData::userHome());

  // Something of another user's and of the root's own, made outside EDSI.
  mkdir((root + "/users/other").c_str(), 0755);
  { ofstream(root + "/users/other/f.txt") << "x"; }
  { ofstream(root + "/rootfile.txt") << "x"; }

  const char *dirs[] = { "/users", "", "/users/other" };
  for (int i = 0; i < 3; i++) {
    string d = root + dirs[i];
    string what = string("in ") + (dirs[i][0] ? dirs[i] + 1 : "root");
    EDSI *e = edsi(d);
    EcceURL *c = e->makeCollection("newdir");
    check(c == 0 && refused(e), "makeCollection refused " + what,
          e->m_msgStack.getMessage());
    delete c;
    c = e->makeDataSet("newfile");
    check(c == 0 && refused(e), "makeDataSet refused " + what,
          e->m_msgStack.getMessage());
    delete c;
    delete e;
    check(!onDisk(d + "/newdir") && !onDisk(d + "/newfile"),
          "nothing created " + what);
  }

  {
    EDSI *e = edsi(root + "/rootfile.txt");
    check(!e->putDataSet("y") && refused(e), "putDataSet refused in root",
          e->m_msgStack.getMessage());
    check(!e->appendDataSet("y") && refused(e), "appendDataSet refused in root");
    vector<MetaDataResult> md(1);
    md[0].name = "ecce:tag";
    md[0].value = "v";
    check(!e->putMetaData(md) && refused(e), "putMetaData refused in root");
    check(!e->removeResource() && refused(e), "remove refused in root");
    delete e;
    check(onDisk(root + "/rootfile.txt") && !onDisk(root + "/.ecce-meta"),
          "root file untouched, no sidecar");
  }
  {
    EDSI *e = edsi(root + "/users");
    check(!e->removeResource() && refused(e), "remove of users/ refused");
    delete e;
    e = edsi(root);
    check(!e->removeResource() && refused(e), "remove of the root refused");
    delete e;
    check(onDisk(root + "/users/other/f.txt"), "other user's file kept");
  }

  // Copy and move: both ends must be in the home.
  {
    EDSI *e = edsi(home + "/mine.txt");
    check(e->putDataSet("mine"), "putDataSet in home works",
          e->m_msgStack.getMessage());
    EcceURL to("file://" + root + "/users/copy.txt");
    check(!e->copyResource(to) && refused(e), "copy into users/ refused");
    EcceURL to2("file://" + root + "/moved.txt");
    check(!e->moveResource(to2) && refused(e), "move into root refused");
    check(!onDisk(root + "/users/copy.txt") && !onDisk(root + "/moved.txt")
          && onDisk(home + "/mine.txt"), "copy/move left things as they were");
    delete e;
    e = edsi(root + "/users/other/f.txt");
    EcceURL in("file://" + home + "/f.txt");
    check(!e->moveResource(in) && refused(e), "move out of another home refused");
    check(onDisk(root + "/users/other/f.txt"), "moved-from file still there");
    delete e;
  }

  // Inside the home: everything works, including "..", which cannot escape.
  {
    EDSI *e = edsi(home);
    EcceURL *c = e->makeCollection("proj");
    check(c != 0, "makeCollection in home works", e->m_msgStack.getMessage());
    delete c;
    c = e->makeDataSet("data");
    check(c != 0, "makeDataSet in home works", e->m_msgStack.getMessage());
    delete c;
    delete e;
    e = edsi(home + "/proj/../../other");
    EcceURL *x = e->makeCollection("sneak");
    check(x == 0 && !onDisk(root + "/users/other/sneak"),
          "'..' does not escape the home");
    delete x;
    delete e;

    e = edsi(home + "/mine.txt");
    EcceURL to("file://" + home + "/proj/mine2.txt");
    check(e->copyResource(to), "copy within home works",
          e->m_msgStack.getMessage());
    vector<MetaDataResult> md(1);
    md[0].name = "ecce:tag";
    md[0].value = "v";
    check(e->putMetaData(md), "putMetaData in home works",
          e->m_msgStack.getMessage());
    EcceURL to2("file://" + home + "/proj/mine3.txt");
    check(e->moveResource(to2), "move within home works",
          e->m_msgStack.getMessage());
    delete e;
    e = edsi(home + "/proj/mine3.txt");
    check(e->removeResource(), "remove in home works",
          e->m_msgStack.getMessage());
    delete e;
  }

  // Reads are unaffected.
  {
    EDSI *e = edsi(root + "/users");
    vector<ResourceResult> list;
    check(e->listCollection(list) && list.size() >= 2, "listing users/ works",
          e->m_msgStack.getMessage());
    delete e;
    e = edsi(root + "/rootfile.txt");
    check(e->getDataSetSize() == 1, "reading the root file works");
    delete e;
  }

  // Outside the data folder (libraries, job directories): not this rule's.
  {
    string outside = root + "-elsewhere";
    mkdir(outside.c_str(), 0755);
    EDSI *e = edsi(outside);
    EcceURL *c = e->makeCollection("ok");
    check(c != 0, "outside the data folder is not restricted",
          e->m_msgStack.getMessage());
    delete c;
    delete e;
  }

  return failures ? 1 : 0;
}
