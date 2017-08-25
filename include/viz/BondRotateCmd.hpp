#ifndef BONDROTATECMD_H
#define BONDROTATECMD_H

#include "util/Command.H"

class TAtm;
class SoDragger;

class BondRotateCmd : public Command
{
public:
  BondRotateCmd(const string& name, Receiver * receiver);
  ~BondRotateCmd();
  void init();
  virtual bool execute() throw(EcceException);
  virtual void undo();
  virtual bool isUndoable() const;

protected:
  bool rotatorExists(SoNode *top, TAtm *at1, TAtm *at2);
};

#endif
