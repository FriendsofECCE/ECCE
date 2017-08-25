/**
 * @file
 *
 *
 */
#ifndef  FINDIRREDUCFRAGCMD_H
#define  FINDIRREDUCFRAGCMD_H

#include "viz/FragCloneCmd.H"

class Fragment;

/**
 * Executes the fortran code that does find irreducible fragment.
 *
 */
class FindIrreduceFragCmd : public FragCloneCmd
{
   public:
      FindIrreduceFragCmd(const string& name, Receiver * receiver);
      virtual ~FindIrreduceFragCmd();

      virtual bool execute() throw(EcceException);

   protected:
      virtual void init();

};

#endif




