/**
 * @file
 *
 *
 */
#ifndef  NEWFRAGMENTCMD_H
#define  NEWFRAGMENTCMD_H

#include "viz/FragCloneCmd.H"

class Fragment;

class NewFragmentCmd : public FragCloneCmd
{
   public:
      NewFragmentCmd(const string& name, Receiver * receiver);
      virtual ~NewFragmentCmd();

      virtual bool execute() throw(EcceException);

   protected:
      virtual void init();

};

#endif




