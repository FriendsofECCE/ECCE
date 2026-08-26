/**
 * @file
 *
 *
 */
#ifndef  BONDEDITCMD_H
#define  BONDEDITCMD_H

#include "viz/FragCloneCmd.H"

class Fragment;

class BondEditCmd : public FragCloneCmd
{
   public:
      BondEditCmd(const string& name, Receiver * receiver);
      virtual ~BondEditCmd();

      virtual bool execute();
      virtual string getUndoLabel();

   protected:
      virtual void init();
      string p_undoLabel;


};

#endif




