#ifndef CLEARATOMMOVECMD_H
#define CLEARATOMMOVECMD_H

#include "util/Command.H"

class ClearAtomMoveCmd : public Command
{
public:
  ClearAtomMoveCmd(const string& name, Receiver * receiver);
  ~ClearAtomMoveCmd();
  void init();
  virtual bool execute() throw(EcceException);
  virtual void undo();
  virtual bool isUndoable() const;
};

#endif
