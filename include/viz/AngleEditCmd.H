/**
 * @file
 *
 *
 */
#ifndef  ANGLEEDITCMD_H
#define  ANGLEEDITCMD_H

#include "viz/FragCloneCmd.H"

class Fragment;

/**
 * Command wrapper for changing a distance between atoms.
 */
class AngleEditCmd : public FragCloneCmd
{
   public:
      AngleEditCmd(const string& name, Receiver * receiver);
      virtual ~AngleEditCmd();

      virtual bool execute() throw(EcceException);

   protected:
      virtual void init();

};

#endif




