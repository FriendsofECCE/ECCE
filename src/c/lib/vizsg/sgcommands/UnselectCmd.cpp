/**
 * @file
 *
 *
 */
#include <iostream>
    using std::cout;
    using std::endl;
    
#include "util/EventDispatcher.H"
#include "util/Event.H"
using namespace ecce;

#include "viz/UnselectCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

UnselectCmd::UnselectCmd(const string& name, Receiver * receiver)
  : Command(name, receiver)
{
  init();
}


UnselectCmd::~UnselectCmd()
{
}


void UnselectCmd::init()
{
}


bool UnselectCmd::execute() throw(EcceException)
{

  SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
  SGFragment *frag = sg->getFragment();

  // For undo
  if (p_selAtoms.size() == 0 && p_selBonds.size() == 0) {
    p_selAtoms = frag->m_atomHighLight;
    p_selBonds = frag->m_bondHighLight;
  }
  
  // Clear last selection
  frag->m_atomHighLight.clear();
  frag->m_bondHighLight.clear();

  EventDispatcher::getDispatcher().publish(Event("SelectionChanged"));

  return true;
}


void UnselectCmd::undo()
{
  SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
  SGFragment *frag = sg->getFragment();
  
  frag->m_atomHighLight = p_selAtoms;
  frag->m_bondHighLight = p_selBonds; 
}

