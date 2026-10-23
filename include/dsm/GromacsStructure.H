/**
 * @file
 */
#ifndef GROMACSSTRUCTURE_HH
#define GROMACSSTRUCTURE_HH

#include <string>
#include <vector>
using std::string;
using std::vector;

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

      /**
       * For a triclinic box: the .gro text with every molecule made whole
       * and moved by lattice vectors to the image nearest the box centre
       * (a+b+c)/2, the compact cell, as gmx trjconv -pbc mol -ur compact
       * does with a topology.  Molecules are taken from the file alone:
       * the atoms of a residue, joined to the next residue when a heavy
       * atom of each lies within a bond length.  A rectangular box, or text
       * that is not a .gro file, comes back unchanged.
       */
      static string wrapCompact(const string& gro);

      /** The atom count a .gro file declares on its second line, or -1. */
      static int atomCount(const string& gro);

      /**
       * The residue names of a .gro file, each once, in order of first
       * appearance (the names the MD editor offers as groups).
       */
      static vector<string> residueNames(const string& gro);

      /**
       * The molecule names under [ molecules ] of a topology, each once, in
       * order (a protein chain from pdb2gmx is one: Protein_chain_A).
       */
      static vector<string> moleculeNames(const string& top);
};
#endif
