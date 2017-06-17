/**
 * @file
 *
 *
 */
#include <iostream>
  using namespace std;

#include "util/EventDispatcher.H"
#include "util/Event.H"
using namespace ecce;

#include "viz/SelectAllCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

SelectAllCmd::SelectAllCmd(const string& name, Receiver * receiver)
    : Command(name, receiver)
{
   init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
SelectAllCmd::~SelectAllCmd()
{
}



void SelectAllCmd::init()
{
   setHelpMessage("Selects all atoms and bonds.");
}



bool SelectAllCmd::execute() throw(EcceException)
{

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());

   // For undo
   if (p_selAtoms.size() == 0 && p_selBonds.size() == 0) {
      SGFragment *frag = sg->getFragment();
      p_selAtoms = frag->m_atomHighLight;
      p_selBonds = frag->m_bondHighLight;
   }


   sg->selectAll();

   EventDispatcher::getDispatcher().publish(Event("SelectionChanged"));

   return true;
}



void SelectAllCmd::undo()
{
   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();

   frag->m_atomHighLight = p_selAtoms;
   frag->m_bondHighLight = p_selBonds;

   EventDispatcher::getDispatcher().publish(Event("SelectionChanged"));

}
