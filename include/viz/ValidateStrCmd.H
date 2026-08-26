/**
 * @file
 *
 *
 */
#ifndef  VALIDATESTRCMD_H
#define  VALIDATESTRCMD_H

#include "viz/FragCloneCmd.H"

class ValidateStrCmd : public FragCloneCmd
{
   public:
      ValidateStrCmd(const string& name, Receiver * receiver);
      virtual ~ValidateStrCmd();

      virtual bool execute();

   protected:
      virtual void init();

};

#endif




