/**
 * @file
 *
 *
 */
#ifndef  GENBONDS_H
#define  GENBONDS_H

#include "viz/FragCloneCmd.H"

class Fragment;

class GenBondsCmd : public FragCloneCmd
{
   public:
      GenBondsCmd(const string& name, Receiver * receiver);
      virtual ~GenBondsCmd();

      virtual bool execute() throw(EcceException);

   protected:
      virtual void init();

};

#endif
