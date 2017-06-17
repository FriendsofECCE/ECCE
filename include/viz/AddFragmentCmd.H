/**
 * @file
 *
 *
 */
#ifndef  ADDFRAGMENTCMD_H
#define  ADDFRAGMENTCMD_H

#include "viz/FragCloneCmd.H"

class Fragment;

class AddFragmentCmd : public FragCloneCmd
{
   public:
      AddFragmentCmd(const string& name, Receiver * receiver);
      virtual ~AddFragmentCmd();

      virtual bool execute() throw(EcceException);

   protected:
      virtual void init();


};

#endif




