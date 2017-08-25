/**
 * @file
 *
 *
 */
#ifndef  LENGTHEDITCMD_H
#define  LENGTHEDITCMD_H

#include "viz/FragCloneCmd.H"

class Fragment;

/**
 * Command wrapper for changing a distance between atoms.
 */
class LengthEditCmd : public FragCloneCmd
{
   public:
      LengthEditCmd(const string& name, Receiver * receiver);
      virtual ~LengthEditCmd();

      virtual bool execute() throw(EcceException);

   protected:
      virtual void init();

};

#endif




