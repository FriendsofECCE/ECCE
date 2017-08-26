/**
 * @file
 *
 *
 */
#include <iostream>
  using namespace std;

#include "util/EventDispatcher.hpp"
#include "util/Event.hpp"
using namespace ecce;

#include "tdat/PointGroup.hpp"
#include "tdat/SymmetryOps.hpp"

#include "viz/AssignSymmetryCmd.hpp"
#include "viz/SGFragment.hpp"
#include "viz/SGContainer.hpp"

AssignSymmetryCmd::AssignSymmetryCmd(const string& name, Receiver * receiver)
    : Command(name, receiver)
{
   init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
AssignSymmetryCmd::~AssignSymmetryCmd()
{

}



void AssignSymmetryCmd::init()
{
   addParameter(new CommandParameter("group", "C1"));
   p_undoGroup = "C1";

}



bool AssignSymmetryCmd::execute() throw(EcceException)
{

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();


   p_undoGroup = frag->pointGroup();

   string group = getParameter("group")->getString();

   if (PointGroup::isGroup(group)) {


      // add group value to molecule
      frag->pointGroup(group);

      // set useSymmetry flag for benefit of calced
      frag->useSymmetry(true);

//   EventDispatcher::getDispatcher().publish(Event("GeomChange"));
   } else {
      throw EcceException("Invalid symmetry group name", WHERE);
   }

   return true;
}

void AssignSymmetryCmd::undo()
{
   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();
   frag->pointGroup(p_undoGroup);
}
