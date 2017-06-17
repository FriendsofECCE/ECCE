/**
 * Command to remove all sphere manipulator
 */

#include "viz/SGFragment.H"
#include "viz/SGContainer.H"
#include "viz/ClearBondRotateCmd.H"

ClearBondRotateCmd::ClearBondRotateCmd(const string& name, Receiver * receiver)
  : Command(name, receiver)
{
  init();
}


ClearBondRotateCmd::~ClearBondRotateCmd()
{
}


void ClearBondRotateCmd::init()
{
}


bool ClearBondRotateCmd::execute() throw(EcceException)
{
  SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());

  if (sg) {
    sg->getTopRotManip()->removeAllChildren();
  }
  return true;
}


void ClearBondRotateCmd::undo()
{
}


bool ClearBondRotateCmd::isUndoable() const
{
  return false;
}
