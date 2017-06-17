/**
 * @file
 *
 *
 */
#ifndef  SELECTALLCMD_H
#define  SELECTALLCMD_H

#include "util/Command.H"

class Fragment;

class SelectAllCmd : public Command
{
   public:
      SelectAllCmd(const string& name, Receiver * receiver);
      virtual ~SelectAllCmd();

      virtual bool execute() throw(EcceException);
      virtual void undo();

   protected:
      virtual void init();

      vector<int> p_selAtoms;
      vector<int> p_selBonds;

};

#endif




