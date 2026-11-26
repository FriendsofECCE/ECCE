#ifndef MOLIO_HH
#define MOLIO_HH

#include <istream>
#include <string>

class Fragment;

/**
 * MDL SDF/MOL (V2000) and Tripos mol2 readers.
 *
 * Small-molecule databases hand out these two formats, so a molecule
 * fetched from one has to be openable in the Builder.  Both carry an
 * explicit bond block, so the bonds come from the file and are not
 * guessed from distances.  No wx dependency.
 */
class MolIO
{
public:
  // An .sdf holding several records: only the first is read.
  static bool readSdf(Fragment& frag, std::istream& in, bool genBonds);
  static bool readMol2(Fragment& frag, std::istream& in, bool genBonds);

  // Why the last read returned false.
  static const std::string& lastError();
};

#endif
