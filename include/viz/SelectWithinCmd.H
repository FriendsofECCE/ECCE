#ifndef SELECTWITHINCMD_H
#define SELECTWITHINCMD_H

#include "util/Command.H"

class SelectWithinCmd : public Command
{
public:
  SelectWithinCmd(const string& name, Receiver *receiver);
  bool execute() override;
  void undo() override;
private:
  void init() override;
  bool p_saved = false;
  vector<int> p_atoms, p_bonds;
};

#endif
