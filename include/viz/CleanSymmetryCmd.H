/**
 * @file
 *
 *
 */
#ifndef  CLEANSYMMETRYCMD_H
#define  CLEANSYMMETRYCMD_H

#include "viz/FragCloneCmd.H"

class Fragment;

/**
 * Executes the fortran code that does symmetry clean.
 *
 * This command writes a chem sys to a file for cleansym to read and
 * execute.  Then reads cleansym's output file and updates the
 * current coordinates.  It also checks for duplicate atoms that may
 * have resulted after the clean.  An error message is issued should
 * this happen.
 *
 * Ghost atoms:  Ghost atoms are ignored and not included in the input
 * file for cleansyme.  When the outfile is read, only the coordinates
 * which are not ghost atoms are updated.
 */
class CleanSymmetryCmd : public FragCloneCmd
{
   public:
      CleanSymmetryCmd(const string& name, Receiver * receiver);
      virtual ~CleanSymmetryCmd();

      virtual bool execute() throw(EcceException);

   protected:
      virtual void init();

};

#endif




