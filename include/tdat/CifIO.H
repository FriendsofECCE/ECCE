#ifndef CIFIO_HH
#define CIFIO_HH

#include <istream>
#include <ostream>
#include <string>
#include <vector>

class Fragment;

/**
 * Crystallographic Information File reader and writer (#358).
 *
 * read() builds the full unit cell (1x1x1) from the asymmetric unit and the
 * symmetry operations the file lists, or, when it gives only a space-group
 * name, through the space-group tables SymmetryOps uses for .car files.
 * write() stores the cell and every atom as P1.  No wx dependency.
 */
class CifIO
{
public:
  static bool read(Fragment& frag, std::istream& in, bool genBonds);
  static bool write(Fragment& frag, std::ostream& out);

  // Why the last read() or write() returned false.
  static const std::string& lastError();
  // Warnings of the last read(); cleared by the call.
  static std::vector<std::string> takeWarnings();

  // "F m -3 m" -> "Fm-3m", "P 63/m m c" -> "P6_3/mmc": the spelling of
  // data/client/config/SpaceGroups.
  static std::string tableName(const std::string& hm);
};

#endif
