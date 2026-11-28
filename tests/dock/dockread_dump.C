// Prints a structure file as read by DockReader, for tests/dock/structure_formats.py.
//   dockread_dump <file> <type>     atoms and bonds, or "ERROR <why>"
//   dockread_dump --canread <ext>   prints 1 or 0 (obabel bridge)
#include "dock/DockPrep.H"

#include <cstdio>
#include <cstring>

int main(int argc, char** argv)
{
  if (argc == 3 && !strcmp(argv[1], "--canread")) {
    printf("%d\n", DockObabel::canRead(argv[2]) ? 1 : 0);
    return 0;
  }
  if (argc != 3) { fprintf(stderr, "usage: dockread_dump file type\n"); return 2; }
  DockMol m;
  std::string err;
  if (!DockReader::readStructure(argv[1], argv[2], m, err)) {
    printf("ERROR %s\n", err.c_str());
    return 1;
  }
  printf("N %d %d\n", m.numAtoms(), (int)m.bonds.size());
  for (int i = 0; i < m.numAtoms(); i++)
    printf("A %s %.6f %.6f %.6f %.4f\n", DockMol::symbolFromElement(m.atoms[i].z).c_str(),
           m.atoms[i].x, m.atoms[i].y, m.atoms[i].zc, m.atoms[i].charge);
  for (size_t b = 0; b < m.bonds.size(); b++)
    printf("B %d %d %d\n", m.bonds[b].i, m.bonds[b].j, m.bonds[b].order);
  return 0;
}
