/**
 * @file
 *
 *
 */
#ifndef  SELECTRADIUSCMD_H
#define  SELECTRADIUSCMD_H

#include "util/Command.H"

class Fragment;
class wxWindow;

class SelectRadiusCmd : public Command
{
   public:
      SelectRadiusCmd(const string& name, Receiver * receiver, const string& sender="");
      virtual ~SelectRadiusCmd();

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
