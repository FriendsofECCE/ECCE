/**
 * @file
 *
 *
 */

#ifndef  UNSELECTCMD_H
#define  UNSELECTCMD_H

#include "util/Command.H"

class Fragment;

class UnselectCmd : public Command
{
public:
  UnselectCmd(const string& name, Receiver * receiver);
  virtual ~UnselectCmd();

  virtual bool execute() throw(EcceException);
  virtual void undo();

protected:
  virtual void init();

  vector<int> p_selAtoms;
  vector<int> p_selBonds;

};

#endif
