/**
 * @file
 *
 *
 */
#ifndef  GENERATEGHOSTSCMD_H
#define  GENERATEGHOSTSCMD_H

#include "viz/FragCloneCmd.H"

class Fragment;

/**
 * Executes the fortran code that generates a full fragment from 
 * the reduced frag and symmetry group.
 *
 */
class GenerateGhostsCmd : public FragCloneCmd
{
   public:
      GenerateGhostsCmd(const string& name, Receiver * receiver);
      virtual ~GenerateGhostsCmd();

      virtual bool execute() throw(EcceException);

   protected:
      virtual void init();

};

#endif




