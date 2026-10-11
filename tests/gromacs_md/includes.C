// Prints what GromacsIncludes finds for the topology given as argv[1]:
// "file <name>", "system <name>" and "missing <name>" lines.  Driven by
// run_tests.py.
#include <iostream>

#include "dsm/GromacsIncludes.H"

int main(int argc, char **argv)
{
  if (argc < 2) {
    std::cerr << "usage: gromacs_includes topol.top" << std::endl;
    return 2;
  }
  vector<GromacsIncludes::File> found;
  vector<string> system, missing;
  GromacsIncludes::collect(argv[1], found, system, missing);
  for (size_t i = 0; i < found.size(); i++)
    std::cout << "file " << GromacsIncludes::encodeName(found[i].name) << "\n";
  for (size_t i = 0; i < system.size(); i++)
    std::cout << "system " << system[i] << "\n";
  for (size_t i = 0; i < missing.size(); i++)
    std::cout << "missing " << missing[i] << "\n";
  return 0;
}
