/**
 * @file
 */
#ifndef GROMACSSTRUCTURE_HH
#define GROMACSSTRUCTURE_HH

#include <string>
using std::string;

/**
 * Reading what ECCE needs from a GROMACS structure (.gro) file.  The GROMACS
 * tools are not needed, and are often not on the machine ECCE's windows run
 * on, so this is done here.
 */
class GromacsStructure
{
   public:
      /**
       * Converts the text of a .gro file to PDB text, in Angstrom, for
       * the Builder.  Atom names are laid out the way ECCE's PDB reader
       * takes the element from them.  Velocities and the box are not
       * carried over beyond a CRYST1 record.
       *
       * @return the number of atoms, or -1 with a reason in err
       */
      static int groToPdb(const string& gro, string& pdb, string& err);

      /** The atom count a .gro file declares on its second line, or -1. */
      static int atomCount(const string& gro);
};
#endif
