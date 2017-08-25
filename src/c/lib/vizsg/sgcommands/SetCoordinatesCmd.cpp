/**
 * @file
 *
 *
 */

#include <iostream>
  using std::cout;
  using std::endl;
  using std::ends;

#include "util/EventDispatcher.H"
#include "util/Event.H"
  using namespace ecce;

#include "tdat/TAtm.H"

#include "wxgui/WindowEvent.H"
  using namespace ecce;

#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

#include "viz/SetCoordinatesCmd.H"



SetCoordinatesCmd::SetCoordinatesCmd(const string& name, Receiver * receiver, const string& sender)
    : Command(name, receiver)
{
  p_sender = sender;
  init();
}


/**
 * Destructor.
 * Deletes undo related data.
 */
SetCoordinatesCmd::~SetCoordinatesCmd()
{
}


void SetCoordinatesCmd::init()
{
  addParameter(new CommandParameter("atomnum", 0));
  addParameter(new CommandParameter("x", 0.0));
  addParameter(new CommandParameter("y", 0.0));
  addParameter(new CommandParameter("z", 0.0));
  setHelpMessage("Set coordinates");
}



bool SetCoordinatesCmd::execute() throw(EcceException)
{
  bool ret = false;


  SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
  SGFragment *frag = sg->getFragment();

  if (frag != 0) {

    // Get parameter values
    int atomnum = getParameter("atomnum")->getInteger();
    double x = getParameter("x")->getDouble();
    double y = getParameter("y")->getDouble();
    double z = getParameter("z")->getDouble();


    if ((atomnum < frag->numAtoms()) && (atomnum >= 0)) {

      TAtm* atom = frag->atomRef(atomnum);

      // Save current values and Make change
      p_atomnum = atomnum;
      const double *atomCoords = atom->coordinates() ;
      p_x = atomCoords[0];
      p_y = atomCoords[1];
      p_z = atomCoords[2];

      // Current selection
      MPoint selPoint (atomCoords[0], atomCoords[1], atomCoords[2]) ;

      // Our translate to point
      MPoint targetVect (x,y,z) ;

      // The translation vector
      MPoint transVect = targetVect.subtract1(selPoint) ;

      MPoint newAtom, nubPoint ;

      // Translate point
      newAtom = selPoint.add1(transVect) ;
      atom->coordinates(newAtom.x(), newAtom.y(), newAtom.z()) ;

      // Translate the nubs
      TAtm *N ;
      vector<TAtm*>* neighbors = frag->connectedAtoms(atomnum);
      for (int t = 0; t < neighbors->size(); t++) {
        N = (*neighbors)[t] ;
        if ( N->atomicSymbol() == "Nub" ) {
          const double *coords = N->coordinates();
          nubPoint = MPoint (coords[0], coords[1], coords[2]);
          newAtom = nubPoint.add1 (transVect) ;
          N->coordinates (newAtom.x(), newAtom.y(), newAtom.z()) ;
        }
      }
      delete neighbors;
      frag->touchNumbers() ;
      sg->touchChemDisplay() ;

      ret = true;
      
      // Publish change message
      Event event("GeomChange", "", p_sender);
      EventDispatcher::getDispatcher().publish(event);
    }
  }
  return ret;
}


void SetCoordinatesCmd::undo()
{
   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();

   //hack - should this be copied over in fragment?
   DisplayStyle ds = sg->getFragment()->getMainDisplayStyle();

   if (p_atomnum >= 0) {
      TAtm *atm = frag->atomRef(p_atomnum);
      const double *atomCoords = atm->coordinates();

      // Current position
      MPoint selPoint (atomCoords[0], atomCoords[1], atomCoords[2]) ;

      // Our translate to point
      MPoint targetVect (p_x,p_y,p_z) ;

      // The translation vector
      MPoint transVect = targetVect.subtract1(selPoint) ;

      MPoint newAtom, nubPoint ;

      // Translate point
      newAtom = selPoint.add1(transVect) ;
      atm->coordinates(newAtom.x(), newAtom.y(), newAtom.z()) ;

      // Translate the nubs
      TAtm *N ;
      vector<TAtm*>* neighbors = frag->connectedAtoms(p_atomnum);
      for (int t = 0; t < neighbors->size(); t++) {
        N = (*neighbors)[t] ;
        if ( N->atomicSymbol() == "Nub" ) {
          const double *coords = N->coordinates();
          nubPoint = MPoint (coords[0], coords[1], coords[2]);
          newAtom = nubPoint.add1 (transVect) ;
          N->coordinates (newAtom.x(), newAtom.y(), newAtom.z()) ;
        }
      }
      delete neighbors;
      sg->touchChemDisplay();

      EventDispatcher::getDispatcher().publish(Event("GeomChange"));
   }

}
