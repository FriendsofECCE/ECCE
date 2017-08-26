/**
 * @file
 *
 *
 */
#ifndef  RMHYDROGENSCMD_H
#define  RMHYDROGENSCMD_H

#include "tdat/TAtm.hpp"

#include "viz/SGContainer.hpp"
#include "viz/FragCloneCmd.hpp"

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




