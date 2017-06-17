/**
 * @file
 *
 *
 */
#ifndef  ASSIGNRESIDUEATOMSCMD_H
#define  ASSIGNRESIDUEATOMSCMD_H

#include "viz/FragCloneCmd.H"

class AssignResidueAtomsCmd : public FragCloneCmd
{
   public:
      AssignResidueAtomsCmd(const string& name, Receiver * receiver);
      virtual ~AssignResidueAtomsCmd();

      virtual bool execute() throw(EcceException);

   protected:
      virtual void init();

};

#endif




