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

#include "viz/TranslateCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

TranslateCmd::TranslateCmd(const string& name, Receiver * receiver)
    : Command(name, receiver)
{
   init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
TranslateCmd::~TranslateCmd()
{

}



void TranslateCmd::init()
{
   addParameter(new CommandParameter("x", 0.0));
   addParameter(new CommandParameter("y", 0.0));
   addParameter(new CommandParameter("z", 0.0));
   addParameter(new CommandParameter("apply","all"));
   setHelpMessage("Translate the existing structure.");
}



bool TranslateCmd::execute() throw(EcceException)
{
   bool ret = false;

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();

   if (frag->numAtoms() > 0 ) {
      ret = true;

       
      double x = getParameter("x")->getDouble();
      double y = getParameter("y")->getDouble();
      double z = getParameter("z")->getDouble();
      string apply = getParameter("apply")->getString();
      if (apply == "all") {
        p_all = true;
      } else {
        p_all = false;
      }
      p_x = -x;
      p_y = -y;
      p_z = -z;
      p_highlight.clear();
      p_highlight = frag->m_atomHighLight;

      frag->translate(x,y,z,p_all);

      frag->touchNumbers();
      sg->touchChemDisplay();

      Event event("GeomChange");
      EventDispatcher::getDispatcher().publish(event);
   }

   return ret;
}



void TranslateCmd::undo()
{
   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();
   if (frag->numAtoms() > 0) {
     frag->m_atomHighLight.clear();
     int i, nsize=p_highlight.size();
     for (i=0; i<nsize; i++) {
       frag->m_atomHighLight.push_back(p_highlight[i]);
     }
     frag->translate(p_x, p_y, p_z, p_all);
     frag->touchNumbers();
     sg->touchChemDisplay();

     Event event("GeomChange");
     EventDispatcher::getDispatcher().publish(event);
   }
}
