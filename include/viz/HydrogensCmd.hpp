/**
 * @file
 *
 *
 */
#ifndef  HYDROGENSCMD_H
#define  HYDROGENSCMD_H

#include "tdat/TAtm.hpp"

#include "viz/SGContainer.hpp"
#include "viz/FragCloneCmd.hpp"

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




