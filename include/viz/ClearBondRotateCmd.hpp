#ifndef CLEARBONDROTATECMD_H
#define CLEARBONDROTATECMD_H

#include "util/Command.H"

class ClearBondRotateCmd : public Command
{
public:
  ClearBondRotateCmd(const string& name, Receiver * receiver);
  ~ClearBondRotateCmd();
  void init();
  virtual bool execute() throw(EcceException);
  virtual void undo();
  virtual bool isUndoable() const;
};

#endif
