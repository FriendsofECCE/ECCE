// Prints the PDB text GromacsStructure makes of a .gro file given as argv[1],
// or the error.  Driven by groconv_test.py, which compares it with gmx.
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include "dsm/GromacsStructure.H"

int main(int argc, char **argv)
{
  if (argc < 2) {
    std::cerr << "usage: groconv file.gro" << std::endl;
    return 2;
  }
  std::ifstream in(argv[1], std::ios::binary);
  std::ostringstream ss;
  ss << in.rdbuf();
  if (argc > 2 && std::string(argv[2]) == "--residues") {
    std::vector<std::string> r = GromacsStructure::residueNames(ss.str());
    for (size_t i = 0; i < r.size(); i++) std::cout << r[i] << std::endl;
    return 0;
  }
  std::string pdb, err;
  int n = GromacsStructure::groToPdb(ss.str(), pdb, err);
  if (n < 0) {
    std::cout << "ERROR " << err << std::endl;
    return 1;
  }
  std::cout << pdb;
  return 0;
}
