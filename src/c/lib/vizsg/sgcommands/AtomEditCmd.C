#include <iostream>
  using std::cout;
  using std::endl;

#include <algorithm>
  using std::find;

#include "util/EventDispatcher.H"
#include "util/Event.H"
using namespace ecce;

#include "util/ErrMsg.H"
#include "util/STLUtil.H"

#include "tdat/TPerTab.H"
#include "tdat/TResItem.H"
#include "tdat/TResTab.H"
#include "tdat/ShapeData.H"
#include "tdat/TAtm.H"
#include "tdat/TBond.H"
#include "tdat/Measures.H"

#include "inv/ChemKit/ChemDisplayParam.H"

#include "viz/SGContainer.H"
#include "viz/SGFragment.H"
#include "viz/AtomEditCmd.H"


AtomEditCmd::AtomEditCmd(const string& name, Receiver *receiver) 
      : FragCloneCmd(name, receiver)
{
  init();
}

AtomEditCmd::~AtomEditCmd()
{
}

void AtomEditCmd::init()
{
   addParameter(new CommandParameter("elem", "C"));
   addParameter(new CommandParameter("geom", "Tetrahedral"));
   addParameter(new CommandParameter("x", 0.0));
   addParameter(new CommandParameter("y", 0.0));
   addParameter(new CommandParameter("z", 0.0));
   addParameter(new CommandParameter("bondOrder", TBond::Single));
   addParameter(new CommandParameter("resIndex", -1));

   setHelpMessage("Click in free space to add atoms with nubs.  "
         "Click on an "
         "existing nub to bond a new atom to an existing one.  Click "
         "on an existing atom to change the atom type or shape.");
}


bool AtomEditCmd::execute() throw(EcceException)
{

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());

   bool ret = true;

   string elem = getParameter("elem")->getString();
   string geom = getParameter("geom")->getString();

   double bondOrder = getParameter("bondOrder")->getDouble();
   double xval = getParameter("x")->getDouble();
   double yval = getParameter("y")->getDouble();
   double zval = getParameter("z")->getDouble();
   int resIndex = getParameter("resIndex")->getInteger();

   // First get our fragment object.  
   SGFragment *frag = sg->getFragment();

   if (resIndex >= 0) {
      if (resIndex > frag->numResidues()) 
         throw EcceException("Invalide Residue Index",WHERE);
      frag->editResidueMode(1,resIndex);
   }


   // In this case we decided to always add it at 0,0,0
   if (frag->numAtoms() == 0) {
      xval = yval = zval = 0;
   }

   try {
      // Set up undo data
      cloneFragment(frag);

      bool addedNotChanged = 
         frag->addAtom(elem, geom, xval, yval, zval, bondOrder);

      frag->clearSelection();
      EventDispatcher::getDispatcher().publish(Event("SelectionChanged"));

      p_undoname = addedNotChanged?"Add Atom":"Change Atom";

      frag->touchNumbers();
      sg->touchChemDisplay();

      // event notification or vcommand properties
      EventDispatcher::getDispatcher().publish(Event("GeomChange"));
   } catch (EcceException& ex) {
      clearUndoFragment();
      throw ex;
   }


   return ret;
}


string AtomEditCmd::getUndoLabel() 
{
   return p_undoname;
}


