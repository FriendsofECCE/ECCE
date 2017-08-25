#ifndef FOREGROUNDCMD_HH
#define FOREGROUNDCMD_HH

#include "inv/SbColor.H"

#include "util/Command.H"

/**
 * Wrapper for command to set atom label color.
 */
class ForegroundCmd : public Command
{
public:
  ForegroundCmd(const string& name, Receiver * receiver);
  ~ForegroundCmd();

  virtual bool execute() throw(EcceException);
  virtual void undo();
  virtual bool isUndoable() const;

protected:
  void init();

  float p_red, p_green, p_blue;

  SbColor p_undoColor;
};

#endif
