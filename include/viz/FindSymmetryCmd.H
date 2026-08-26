/**
 * @file
 *
 *
 */
#ifndef  FINDSYMMETRYCMD_H
#define  FINDSYMMETRYCMD_H

#include "viz/FragCloneCmd.H"

class Fragment;

/**
 * Executes the fortran code that does find symmetry.
 *
 */
class FindSymmetryCmd : public FragCloneCmd
{
   public:
      FindSymmetryCmd(const string& name, Receiver * receiver);
      virtual ~FindSymmetryCmd();

      virtual bool execute();

   protected:
      virtual void init();

};

#endif




