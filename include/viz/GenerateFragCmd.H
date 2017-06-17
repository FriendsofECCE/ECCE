/**
 * @file
 *
 *
 */
#ifndef  GENERATEFRAGCMD_H
#define  GENERATEFRAGCMD_H

#include "viz/FragCloneCmd.H"

class Fragment;

/**
 * Executes the fortran code that generates a full fragment from 
 * the reduced frag and symmetry group.
 *
 */
class GenerateFragCmd : public FragCloneCmd
{
   public:
      GenerateFragCmd(const string& name, Receiver * receiver);
      virtual ~GenerateFragCmd();

      virtual bool execute() throw(EcceException);

   protected:
      virtual void init();

};

#endif




