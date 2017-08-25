/**
 * @file
 *
 *
 */
#ifndef  HYDROGENSCMD_H
#define  HYDROGENSCMD_H

#include "tdat/TAtm.H"

#include "viz/SGContainer.H"
#include "viz/FragCloneCmd.H"

class HydrogensCmd : public FragCloneCmd
{
   public:
      HydrogensCmd(const string& name, Receiver * receiver);
      virtual ~HydrogensCmd();

      virtual bool execute() throw(EcceException);

   protected:
      virtual void init();

};

#endif




