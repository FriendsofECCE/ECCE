#include <cstdlib>
#include <iostream>
#include "dsm/FileEDSI.H"
#include "../../src/dsm/edsiimpl/SftpRead.H"

int main(int argc, char **argv)
{
  if (argc < 3) return 2;
  const std::string mode = argv[1], path = argv[2];
  FileEDSI source(EcceURL("file://" + path));
  if (mode == "size") { std::cout << source.getDataSetSize(); return 0; }
  if (mode == "size-script") { std::cout << SftpRead::sizeScript(path); return 0; }
  if (argc != 5) return 2;
  const int at = std::atoi(argv[3]), n = std::atoi(argv[4]);
  if (mode == "script") { std::cout << SftpRead::rangeScript(path, at, n); return 0; }
  return source.getDataSubSet(std::cout, at, n) ? 0 : 1;
}
