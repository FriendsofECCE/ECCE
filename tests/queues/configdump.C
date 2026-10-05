// Prints RefMachine::config(<refname>) as "key: value" lines, sorted by key
// (newlines in a value written as \n), for tests/queues/config_test.py to
// compare with gensub's effective values.  With -accessors it prints what
// the RefMachine accessors return for a registered machine instead, to show
// that they read the merged view.
#include <cstring>
#include <iostream>

#include "tdat/RefMachine.H"

static std::string oneLine(const std::string& v)
{
  std::string out;
  for (size_t i = 0; i < v.size(); i++) {
    if (v[i] == '\n')
      out += "\\n";
    else
      out += v[i];
  }
  return out;
}

int main(int argc, char** argv)
{
  bool accessors = (argc == 3 && strcmp(argv[1], "-accessors") == 0);
  if (argc != 2 && !accessors) {
    std::cerr << "usage: configdump [-accessors] <refname>" << std::endl;
    return 2;
  }
  std::string name = argv[argc - 1];

  if (accessors) {
    RefMachine* m = RefMachine::refLookup(name);
    if (m == 0) {
      std::cerr << "no machine " << name << std::endl;
      return 1;
    }
    std::cout << "shell: " << m->shell() << std::endl;
    std::cout << "sourceFile: " << m->sourceFile() << std::endl;
    std::cout << "libPath: " << m->libPath() << std::endl;
    std::cout << "shellPath: " << m->shellPath() << std::endl;
    std::cout << "frontendMachine: " << m->frontendMachine() << std::endl;
    std::cout << "frontendBypass: " << m->frontendBypass() << std::endl;
    std::cout << "singleConnect: " << m->singleConnect() << std::endl;
    std::cout << "checkScratch: " << m->checkScratch() << std::endl;
    std::cout << "noRemoteAccess: " << m->noRemoteAccess() << std::endl;
    std::cout << "userSubmit: " << m->userSubmit() << std::endl;
    std::cout << "exePath NWChem: "
              << RefMachine::exePath("NWChem", name) << std::endl;
    return 0;
  }

  map<string,string> cfg = RefMachine::config(name);
  for (map<string,string>::const_iterator it = cfg.begin(); it != cfg.end();
       ++it)
    std::cout << it->first << ": " << oneLine(it->second) << std::endl;
  return 0;
}
