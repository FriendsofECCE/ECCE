// The receptor/ligand split against the golden corpus captured from the
// retired docking.pl (#1335).  fixtures/split_golden.txt sections read
// "## name<TAB>pdb file<TAB>ligand atoms spec", then the expected report.
#include "dock/DockSplit.H"

#include <fstream>
#include <iostream>
#include <sstream>

int main(int argc, char** argv)
{
  std::string root = argc > 1 ? argv[1] : ".";
  std::string dir = root + "/tests/dock/fixtures/";
  std::ifstream in((dir + "split_golden.txt").c_str());
  if (!in) { std::cout << "SKIP: no golden file\n"; return 77; }
  std::vector<std::string> head, body;
  std::string line;
  while (std::getline(in, line)) {
    if (line.compare(0, 3, "## ") == 0) { head.push_back(line.substr(3)); body.push_back(""); }
    else if (!body.empty()) body.back() += line + "\n";
  }
  int bad = 0;
  for (size_t c = 0; c < head.size(); c++) {
    std::istringstream h(head[c]);
    std::string name, file, spec;
    std::getline(h, name, '\t'); std::getline(h, file, '\t'); std::getline(h, spec);
    std::vector<DockSplitAtom> atoms;
    std::string err;
    if (!DockSplit::readPdb(dir + file, atoms, err)) { std::cout << "FAIL " << name << ": " << err << "\n"; bad++; continue; }
    std::string got = DockSplit::report(atoms, DockSplit::split(atoms, spec));
    if (got != body[c]) {
      std::cout << "FAIL " << name << "\n--- expected\n" << body[c] << "--- got\n" << got;
      bad++;
    } else std::cout << "ok   " << name << "\n";
  }
  if (head.size() < 12) { std::cout << "FAIL: only " << head.size() << " cases\n"; bad++; }
  return bad ? 1 : 0;
}
