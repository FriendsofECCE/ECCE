/**
 * @file
 *
 *
 */
#ifndef  CLEARRESIDUEINFOCMD_H
#define  CLEARRESIDUEINFOCMD_H

#include "viz/FragCloneCmd.H"

class Fragment;

class ClearResidueInfoCmd : public FragCloneCmd
{
   public:
      ClearResidueInfoCmd(const string& name, Receiver * receiver);
      virtual ~ClearResidueInfoCmd();

      virtual bool execute() throw(EcceException);

   protected:
      virtual void init();



};

#endif




