/**
 * Command to remove all sphere manipulator
 */

#include "viz/SGFragment.hpp"
#include "viz/SGContainer.hpp"
#include "viz/ClearAtomMoveCmd.hpp"

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
