/**
 * @file
 *
 *
 */
#ifndef  TORSIONEDITCMD_H
#define  TORSIONEDITCMD_H

#include "viz/FragCloneCmd.H"

class Fragment;

/**
 * Command wrapper for changing a distance between atoms.
 */
class TorsionEditCmd : public FragCloneCmd
{
   public:
      TorsionEditCmd(const string& name, Receiver * receiver);
      virtual ~TorsionEditCmd();

      virtual bool execute() throw(EcceException);

   protected:
      virtual void init();

};

#endif




