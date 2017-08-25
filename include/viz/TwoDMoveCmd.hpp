/**
 * @file
 *
 *
 */
#ifndef  TWODMOVECMD_H
#define  TWODMOVECMD_H

#include "viz/FragCloneCmd.H"

class Fragment;

class TwoDMoveCmd : public FragCloneCmd
{
   public:
      TwoDMoveCmd(const string& name, Receiver * receiver);
      virtual ~TwoDMoveCmd();

      virtual bool execute() throw(EcceException);

   protected:
      virtual void init();

};

#endif
