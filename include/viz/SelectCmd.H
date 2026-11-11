/**
 * @file
 *
 *
 */
#ifndef  SELECTCMD_H
#define  SELECTCMD_H

#include "util/Command.H"

class Fragment;
class wxWindow;

class SelectCmd : public Command
{
   public:
      SelectCmd(const string& name, Receiver * receiver, const string& sender="");
      virtual ~SelectCmd();

      virtual bool execute();
      virtual void undo();

      /// What a protein/ligand selection chose, or empty.
      const string& message() const { return p_message; }

   protected:
      virtual void init();

      /* sender of event - default to "". Used to filter own messages */
      string p_message;
      string p_sender;
      vector<int> p_selAtoms;
      vector<int> p_selBonds;

};

#endif
