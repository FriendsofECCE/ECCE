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

#include "viz/CSStyleCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

CSStyleCmd::CSStyleCmd(const string& name, Receiver * receiver)
    : Command(name, receiver)
{
   init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
CSStyleCmd::~CSStyleCmd()
{

}



void CSStyleCmd::init()
{
   addParameter(new CommandParameter("descriptor", 
            "_default_ _Ball_And_Wireframe_ _Element_"));
   addParameter(new CommandParameter("all", true));
   setHelpMessage("Set the display style.");
}



bool CSStyleCmd::execute() throw(EcceException)
{
   bool ret = false;
   setErrorMessage("");

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();

   string ddstring = getParameter("descriptor")->getString();
   DisplayDescriptor dd(ddstring);

   bool all = getParameter("all")->getBoolean();

   if (dd.getStyle() == "Invisible")
   {
      setErrorMessage("Invisible display style set.  Use Display All Atoms to restore visibility of hidden atoms.");
   }


   bool changed = sg->setStyle(dd, all, frag);


   if ( changed == true ) {
     frag->touchNumbers();
     sg->touchChemDisplay() ;
     ret = true;

     EventDispatcher::getDispatcher().publish(Event("StyleChange"));
   }


   return ret;
}



/**
 * @todo support undo of styles.
 */
void CSStyleCmd::undo()
{
}



bool CSStyleCmd::isUndoable() const
{
   return false;
}
