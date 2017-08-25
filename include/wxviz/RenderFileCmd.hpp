#ifndef RENDERFILECMD_H
#define RENDERFILECMD_H


#include "util/Command.H"

/**
 * A wrapper for the off screen rendering command.
 */
class RenderFileCmd : public Command
{
   public:
      RenderFileCmd(const string& name, Receiver * receiver);
      ~RenderFileCmd();

      virtual bool execute() throw(EcceException);
      virtual bool isUndoable() const;
      virtual void undo();

   protected:
      void init();

};
#endif
