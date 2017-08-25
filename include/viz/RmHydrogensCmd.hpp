/**
 * @file
 *
 *
 */
#ifndef  RMHYDROGENSCMD_H
#define  RMHYDROGENSCMD_H

#include "tdat/TAtm.H"

#include "viz/SGContainer.H"
#include "viz/FragCloneCmd.H"

class RmHydrogensCmd : public FragCloneCmd
{
   public:
      RmHydrogensCmd(const string& name, Receiver * receiver);
      virtual ~RmHydrogensCmd();

      virtual bool execute() throw(EcceException);

   protected:
      virtual void init();

};

#endif




