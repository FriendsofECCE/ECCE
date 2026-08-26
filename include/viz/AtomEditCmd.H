/**
 * @file
 *
 *
 */
#ifndef  ATOMEDITCMD_H
#define  ATOMEDITCMD_H

#include "viz/FragCloneCmd.H"

class Fragment;

class AtomEditCmd : public FragCloneCmd
{
   public:
      AtomEditCmd(const string& name, Receiver * receiver);
      virtual ~AtomEditCmd();

      virtual bool execute();
      virtual string getUndoLabel();

   protected:
      virtual void init();

      string p_undoname;


};

#endif




