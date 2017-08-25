/**
 * @file
 *
 *
 */
#ifndef  REMOVEGHOSTSCMD_H
#define  REMOVEGHOSTSCMD_H

#include "viz/FragCloneCmd.H"

class Fragment;

/**
 *
 */
class RemoveGhostsCmd : public FragCloneCmd
{
   public:
      RemoveGhostsCmd(const string& name, Receiver * receiver);
      virtual ~RemoveGhostsCmd();

      virtual bool execute() throw(EcceException);

   protected:
      virtual void init();

};

#endif




