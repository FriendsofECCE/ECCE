/*
 * lockTest <scratch-dir>
 *
 * N processes hammer one directory at once: each makes PROPS putMetaData
 * calls on distinct names for one file and APPENDS overwrite-and-append
 * calls (the DavPropCache shape: replace the closing tag, write a step and
 * the closing tag again) on one shared file.  Without the directory lock
 * the sidecar loses records and the appended file is torn.
 */
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
using namespace std;

#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include "dsm/EDSI.H"
#include "dsm/EDSIFactory.H"
#include "util/EcceURL.H"

static const int NPROC = 6, PROPS = 200, APPENDS = 60;

static EDSI *edsi(const string& path)
{
  return EDSIFactory::getEDSI(EcceURL("file://" + path));
}

static int worker(int id, const string& root)
{
  EDSI *meta = edsi(root + "/calc.nw");
  EDSI *log = edsi(root + "/steps.xml");
  int bad = 0;
  for (int i = 0; i < PROPS; i++) {
    MetaDataResult m;
    char n[64];
    sprintf(n, "ecce:p%d_%d", id, i);
    m.name = n;
    m.type = "string";
    m.value = "v";
    vector<MetaDataResult> v(1, m);
    if (!meta->putMetaData(v)) bad++;
    if (i % (PROPS / APPENDS) == 0) {
      char step[64];
      sprintf(step, "<s id=\"%d.%d\"/>\n</r>\n", id, i);
      if (!log->appendDataSet(step, 5)) bad++;
    }
  }
  return bad ? 1 : 0;
}

int main(int argc, char **argv)
{
  if (argc < 2) return 2;
  string root = argv[1];
  { ofstream(root + "/calc.nw") << "x"; }
  { ofstream(root + "/steps.xml") << "<r>\n</r>\n"; }

  vector<pid_t> kids;
  for (int id = 0; id < NPROC; id++) {
    pid_t pid = fork();
    if (pid == 0) _exit(worker(id, root));
    kids.push_back(pid);
  }
  int failedWorkers = 0;
  for (size_t i = 0; i < kids.size(); i++) {
    int st = 0;
    waitpid(kids[i], &st, 0);
    if (!WIFEXITED(st) || WEXITSTATUS(st) != 0) failedWorkers++;
  }

  int failures = 0;
  EDSI *meta = edsi(root + "/calc.nw");
  int found = 0;
  for (int id = 0; id < NPROC; id++) {
    for (int i = 0; i < PROPS; i++) {
      char n[64];
      sprintf(n, "ecce:p%d_%d", id, i);
      vector<MetaDataRequest> rq(1);
      rq[0].name = n;
      vector<MetaDataResult> res;
      meta->getMetaData(rq, res);
      if (!res.empty()) found++;
    }
  }
  cout << (found == NPROC * PROPS ? "PASS" : "FAIL") << " no record lost: "
       << found << " of " << NPROC * PROPS << endl;
  if (found != NPROC * PROPS) failures++;

  ifstream in((root + "/steps.xml").c_str());
  string line, all;
  int steps = 0, closes = 0;
  while (getline(in, line)) {
    all += line + "\n";
    if (line.compare(0, 3, "<s ") == 0) steps++;
    if (line == "</r>") closes++;
  }
  int want = NPROC * ((PROPS + PROPS / APPENDS - 1) / (PROPS / APPENDS));
  bool intact = steps == want && closes == 1 &&
                all.compare(0, 4, "<r>\n") == 0 &&
                all.size() >= 5 && all.substr(all.size() - 5) == "</r>\n";
  cout << (intact ? "PASS" : "FAIL") << " appended file intact: " << steps
       << " of " << want << " steps, " << closes << " closing tag(s)" << endl;
  if (!intact) failures++;
  if (failedWorkers) { cout << "FAIL " << failedWorkers << " worker(s) reported an error" << endl; failures++; }
  cout << (failures ? "FAILED " : "ALL OK ") << failures << " failure(s)" << endl;
  return failures ? 1 : 0;
}
