#ifndef ATOMMOVECMD_H
#define ATOMMOVECMD_H

#include <vector>
using std::vector;

#include "util/Command.H"

class SoDragger;
class SoNode;
class TAtm;

class AtomMoveCmd : public Command
{
public:
  AtomMoveCmd(const string& name, Receiver * receiver);
  ~AtomMoveCmd();
  void init();
  virtual bool execute() throw(EcceException);
  virtual void undo();
  virtual bool isUndoable() const;

protected:
  bool manipExists(SoNode *top, vector<TAtm*> selected);
};

#endif
