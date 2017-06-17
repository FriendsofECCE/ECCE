/**
 * @file
 *
 *
 */
#ifndef  SELECTBONDDISCMD_H
#define  SELECTBONDDISCMD_H

#include "util/Command.H"

class Fragment;
class wxWindow;

class SelectBondDisCmd : public Command
{
   public:
      SelectBondDisCmd(const string& name, Receiver * receiver, const string& sender="");
      virtual ~SelectBondDisCmd();

      virtual bool execute() throw(EcceException);
      virtual void undo();

   protected:
      virtual void init();

      /* sender of event - default to "". Used to filter own messages */
      string p_sender;
      vector<int> p_selAtoms;
      vector<int> p_selBonds;

};

#endif
