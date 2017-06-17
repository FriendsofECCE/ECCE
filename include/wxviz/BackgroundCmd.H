#ifndef BACKGROUNDCMD_HH
#define BACKGROUNDCMD_HH

#include "inv/SbColor.H"


#include "util/Command.H"

/**
 * Wrapper for command to set viewer background color.
 */
class BackgroundCmd : public Command
{
   public:
      BackgroundCmd(const string& name, Receiver * receiver);
      ~BackgroundCmd();

      virtual bool execute() throw(EcceException);
      virtual void undo();
      virtual bool isUndoable() const;

   protected:
      void init();

      SbColor p_undoColor;

};
#endif
