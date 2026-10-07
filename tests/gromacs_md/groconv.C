// Prints the PDB text GromacsStructure makes of a .gro file given as argv[1],
// or the error.  Driven by groconv_test.py, which compares it with gmx.
#include <fstream>
#include <iostream>
#include <sstream>

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
  std::string pdb, err;
  int n = GromacsStructure::groToPdb(ss.str(), pdb, err);
  if (n < 0) {
    std::cout << "ERROR " << err << std::endl;
    return 1;
  }
  std::cout << pdb;
  return 0;
}
