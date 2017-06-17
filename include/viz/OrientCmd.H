/**
 * @file
 *
 *
 */
#ifndef  ORIENTCMD_H
#define  ORIENTCMD_H

#include "viz/FragCloneCmd.H"

class Fragment;

class OrientCmd : public FragCloneCmd
{
   public:
      OrientCmd(const string& name, Receiver * receiver);
      virtual ~OrientCmd();

      virtual bool execute() throw(EcceException);

   protected:
      virtual void init();

};

#endif
