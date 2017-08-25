/**
 * @file
 *
 *
 */
#ifndef  INSERTRESIDUECMD_H
#define  INSERTRESIDUECMD_H

#include "viz/FragCloneCmd.H"

class Fragment;

class InsertResidueCmd : public FragCloneCmd
{
   public:
      InsertResidueCmd(const string& name, Receiver * receiver);
      virtual ~InsertResidueCmd();

      virtual bool execute() throw(EcceException);

   protected:
      virtual void init();



};

#endif




