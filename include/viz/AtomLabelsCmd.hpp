/**
 * @file
 *
 *
 */

#ifndef ATOMLABELSCMD_HH
#define ATOMLABELSCMD_HH

#include <string>
using std::string;

#include "util/Command.H"

class wxWindow;

class AtomLabelsCmd : public Command
{
public:
  AtomLabelsCmd(const string& name, Receiver * receiver, wxWindow *sender=0); 
  ~AtomLabelsCmd();

  enum AtomLabelType { NONE=0, ELEMENT, ATOMNAME, CHARGE2, CHARGE3, CHARGE4, TYPE };

  virtual bool execute() throw(EcceException);
  virtual void undo();
  virtual bool isUndoable() const;

protected:

  void init();

  vector<int> p_targets;
  vector<string> p_labels;

  wxWindow *p_sender;

};
#endif
