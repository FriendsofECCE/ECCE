#ifndef DOCKFRAGMENTIO_HH
#define DOCKFRAGMENTIO_HH

#include <istream>
#include <string>

class Fragment;

/**
 * Structure files read by the docking library (SDF/MOL, MOL2, PDBQT) and, when
 * obabel is installed, every format Open Babel reads.  This is the one place
 * a DockMol becomes a Fragment, so the Builder's Open, Add Structure and the
 * tests cannot disagree.  Bond orders come from the file (aromatic as 1.5);
 * partial charges are kept on the atoms.  Only the first record is read.
 */
class DockFragmentIO
{
public:
  /** Types handled without an external program, any case. */
  static bool isNative(const std::string& type);
  /** True for a native type, or an Open Babel format when obabel is found. */
  static bool canRead(const std::string& type);
  static bool read(Fragment& frag, const std::string& path,
                   const std::string& type);
  /** As read(), from a stream (copied to a temporary file first). */
  static bool read(Fragment& frag, std::istream& in, const std::string& type);
  static const std::string& lastError();
};

#endif
