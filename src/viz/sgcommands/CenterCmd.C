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

#include "viz/CenterCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

CenterCmd::CenterCmd(const string& name, Receiver * receiver)
    : Command(name, receiver)
{
   init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
CenterCmd::~CenterCmd()
{

}



void CenterCmd::init()
{
   addParameter(new CommandParameter("axis", ""));
   addParameter(new CommandParameter("apply", "all"));
   setHelpMessage("Clear the existing structure.");
}



bool CenterCmd::execute() throw(EcceException)
{
   bool ret = false;

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();

   if (frag->numAtoms() > 0 ) {
      ret = true;
      string apply = getParameter("apply")->getString();
      if (apply == "all") {
        p_all = true;
      } else {
        p_all = false;
      }
      frag->centerOfMass(p_x, p_y, p_z, p_all); 
      p_highLight = frag->m_atomHighLight;
       
      string axis = getParameter("axis")->getString();
      if (axis == "") {
      } else if (axis == "x" || axis == "X") {
         p_y = 0.0;
         p_z = 0.0;
         p_label = "Center X";
      } else if (axis == "y" || axis == "Y") {
         p_x = 0.0;
         p_z = 0.0;
         p_label = "Center Y";
      } else if (axis == "z" || axis == "Z") {
         p_x = 0.0;
         p_y = 0.0;
         p_label = "Center Z";
      }
      frag->translate(-p_x, -p_y, -p_z, p_all);

      frag->touchNumbers();
      sg->touchChemDisplay();

      Event event("GeomChange");
      EventDispatcher::getDispatcher().publish(event);
   }

   return ret;
}


string CenterCmd::getUndoLabel()
{
   return p_label;
}



void CenterCmd::undo()
{
   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();

   frag->m_atomHighLight = p_highLight;
   frag->translate(p_x, p_y, p_z, p_all);

   frag->touchNumbers();
   sg->touchChemDisplay();

   Event event("GeomChange");
   EventDispatcher::getDispatcher().publish(event);
}
