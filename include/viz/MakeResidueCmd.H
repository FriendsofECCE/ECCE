/**
 * @file
 *
 *
 */
#ifndef  MAKERESIDUECMD_H
#define  MAKERESIDUECMD_H

#include "viz/FragCloneCmd.H"

class Fragment;

class MakeResidueCmd : public FragCloneCmd
{
   public:
      MakeResidueCmd(const string& name, Receiver * receiver);
      virtual ~MakeResidueCmd();

      virtual bool execute() throw(EcceException);

   protected:
      virtual void init();

};

#endif




