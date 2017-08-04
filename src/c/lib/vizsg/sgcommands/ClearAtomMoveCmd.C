/**
 * Command to remove all sphere manipulator
 */

#include "viz/SGFragment.H"
#include "viz/SGContainer.H"
#include "viz/ClearAtomMoveCmd.H"

ClearAtomMoveCmd::ClearAtomMoveCmd(const string& name, Receiver * receiver)
  : Command(name, receiver)
{
  init();
}


ClearAtomMoveCmd::~ClearAtomMoveCmd()
{
}


void ClearAtomMoveCmd::init()
{
}


bool ClearAtomMoveCmd::execute() throw(EcceException)
{
  SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());

  if (sg) {
    sg->getTopManip()->removeAllChildren();
  }
  return true;
}


void ClearAtomMoveCmd::undo()
{
}


bool ClearAtomMoveCmd::isUndoable() const
{
  return false;
}
