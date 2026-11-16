/*
 * ecce-batch-smiles [options] <folder> <table.csv|.tsv|->  (#961)
 *
 * One calculation per row of a table of names, SMILES and measured values,
 * in <folder>, as the Organizer's Tools > Batch Add from SMILES makes them:
 * the structure from the SMILES (formal charges give the charge), cleaned,
 * with the setup of a template calculation (default: xTB GFN2-xTB
 * optimisation, through a "QSAR template" made in the folder and removed
 * after the batch).  The measured values are stored on
 * each calculation (ecce:qsar.measured, JSON keyed by the column header).
 *
 *   --template CALC     the setup to use (a calculation path or URL)
 *   --name COL --smiles COL --response COL (repeatable) --no-responses
 *                       columns by header or 1-based number; default: guessed
 *   --header yes|no     whether the first line is a header (default: guessed)
 *   --replace           make again the calculations of the same name that were
 *                       never launched (the template edited and applied again)
 *   --dry-run           only read the table and report the rows
 *   --launch            ecce-launch the new calculations (needs a running ECCE)
 *   --json              one JSON object instead of the lines
 *
 * Lines: "name: made <url> (charge N)" or "name: not made: reason".
 * Exit 0: all made; 1: some were not; 2: no folder or table; 64: usage.
 */
#include <climits>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>
#include <sys/wait.h>

#include "dsm/SmilesBatch.H"
#include "util/Ecce.H"
#include "util/EcceURL.H"
#include "util/LocalData.H"

using std::string;
using std::vector;

namespace {

bool toUrl(const string& arg, string& url, string& why)
{
  if (arg.find("://") != string::npos) { url = arg; return true; }
  string root = LocalData::dir();
  if (root.empty()) { why = "this session uses a data server: give a URL"; return false; }
  char buf[PATH_MAX], rbuf[PATH_MAX];
  if (!realpath(arg.c_str(), buf)) { why = "no such calculation or folder"; return false; }
  string path = buf;
  string base = realpath(root.c_str(), rbuf) ? string(rbuf) : root;
  if (path != base && path.compare(0, base.size() + 1, base + "/") != 0) {
    why = "not inside the data folder " + base;
    return false;
  }
  url = "file://" + path;
  return true;
}

int usage()
{
  std::cerr << "usage: ecce-batch-smiles [--template CALC] [--name COL] [--smiles COL]\n"
               "         [--response COL]... [--no-responses] [--header yes|no]\n"
               "         [--replace] [--dry-run] [--launch] [--json] <folder> <table.csv|.tsv|->"
            << std::endl;
  return 64;
}

}  // namespace


int main(int argc, char **argv)
{
  if (!getenv("ECCE_REALUSERHOME")) {
    const char *h = getenv("HOME");
    if (h) setenv("ECCE_REALUSERHOME", h, 0);
  }
  string templ, nameCol, smilesCol;
  vector<string> responses;
  bool noResponses = false, dry = false, replace = false, launch = false, json = false;
  int header = -1;
  vector<string> args;
  for (int i = 1; i < argc; i++) {
    const string a = argv[i];
    const bool more = i + 1 < argc;
    if (a == "--template" && more) templ = argv[++i];
    else if (a == "--name" && more) nameCol = argv[++i];
    else if (a == "--smiles" && more) smilesCol = argv[++i];
    else if (a == "--response" && more) responses.push_back(argv[++i]);
    else if (a == "--no-responses") noResponses = true;
    else if (a == "--header" && more) header = string(argv[++i]) == "no" ? 0 : 1;
    else if (a == "--dry-run") dry = true;
    else if (a == "--replace") replace = true;
    else if (a == "--launch") launch = true;
    else if (a == "--json") json = true;
    else if (a.size() > 1 && a[0] == '-' && a[1] == '-') return usage();
    else args.push_back(a);
  }
  if (args.size() != 2) return usage();
  Ecce::initialize();

  string text;
  if (args[1] == "-") text.assign(std::istreambuf_iterator<char>(std::cin), {});
  else {
    std::ifstream in(args[1].c_str(), std::ios::binary);
    if (!in) { std::cout << args[1] << ": cannot read" << std::endl; return 2; }
    text.assign(std::istreambuf_iterator<char>(in), {});
    if (text.compare(0, 2, "PK") == 0) {
      std::cout << args[1] << ": a spreadsheet file; save it as CSV first" << std::endl;
      return 2;
    }
  }
  SmilesBatch::Table table;
  string why;
  if (!SmilesBatch::parseTable(text, table, why, header)) {
    std::cout << args[1] << ": " << why << std::endl;
    return 2;
  }
  SmilesBatch::Columns cols = SmilesBatch::guessColumns(table);
  auto column = [&](const string& key, int& into) {
    if (key.empty()) return true;
    into = SmilesBatch::findColumn(table, key);
    if (into < 0) std::cout << "no column " << key << std::endl;
    return into >= 0;
  };
  if (!column(nameCol, cols.name) || !column(smilesCol, cols.smiles)) return 2;
  if (noResponses || !responses.empty()) cols.responses.clear();
  for (const string& r : responses) {
    int k = -1;
    if (!column(r, k)) return 2;
    cols.responses.push_back(k);
  }
  if (cols.smiles < 0) { std::cout << "no SMILES column found: use --smiles" << std::endl; return 2; }
  table.source = args[1] == "-" ? "standard input" : EcceURL(args[1]).getFilePathTail();
  vector<SmilesBatch::Row> rows = SmilesBatch::preview(table, cols);

  if (dry) {
    if (!json) {
      std::cout << "columns: name=" << (cols.name >= 0 ? table.header[cols.name] : "-")
                << " smiles=" << table.header[cols.smiles] << " responses=";
      for (size_t i = 0; i < cols.responses.size(); i++)
        std::cout << (i ? "," : "") << table.header[cols.responses[i]];
      std::cout << std::endl;
    }
    vector<SmilesBatch::Outcome> preview;
    for (const auto& r : rows) {
      SmilesBatch::Outcome o;
      o.name = r.name; o.smiles = r.smiles; o.line = r.line; o.charge = r.charge;
      o.ok = r.problem.empty(); o.note = r.problem;
      preview.push_back(o);
      if (!json)
        std::cout << r.name << ": " << (o.ok ? "ok" : "not made: " + r.problem)
                  << " (charge " << r.charge << ", " << r.atoms << " atoms)" << std::endl;
    }
    if (json) std::cout << SmilesBatch::json("", "", preview) << std::endl;
    return 0;
  }

  string fUrl, tUrl;
  if (!toUrl(args[0], fUrl, why)) { std::cout << args[0] << ": " << why << std::endl; return 2; }
  if (!templ.empty() && !toUrl(templ, tUrl, why)) {
    std::cout << templ << ": " << why << std::endl;
    return 2;
  }
  string used;
  vector<SmilesBatch::Outcome> out =
      SmilesBatch::make(EcceURL(fUrl), tUrl, rows, used, why, replace);
  if (used.empty()) { std::cout << "template: " << why << std::endl; return 2; }
  bool allOk = true;
  vector<string> made;
  for (const auto& o : out) {
    allOk = allOk && o.ok;
    if (o.ok) made.push_back(o.url);
    if (!json) {
      if (o.ok) std::cout << o.name << ": made " << o.url << " (charge " << o.charge << ")"
                          << (o.note.empty() ? "" : " (" + o.note + ")") << std::endl;
      else std::cout << o.name << ": not made: " << o.note << std::endl;
    }
  }
  if (json) std::cout << SmilesBatch::json(fUrl, used, out) << std::endl;
  else std::cout << "template: " << used << std::endl;
  if (launch && !made.empty()) {
    // One Launch Selected in the running session for the whole batch.
    std::cout.flush();
    vector<string> cmd = {"ecce-launch"};
    for (const string& u : made) cmd.push_back(u);
    vector<char*> av;
    for (string& c : cmd) av.push_back(&c[0]);
    av.push_back(0);
    pid_t pid = fork();
    if (pid == 0) {
      if (json) dup2(2, 1);   // keep stdout one JSON object
      execvp("ecce-launch", av.data());
      _exit(127);
    }
    int st = 0;
    waitpid(pid, &st, 0);
    if (!WIFEXITED(st) || WEXITSTATUS(st) != 0) allOk = false;
  }
  return allOk ? 0 : 1;
}
