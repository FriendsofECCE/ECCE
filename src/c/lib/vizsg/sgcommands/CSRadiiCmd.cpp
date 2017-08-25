/**
 * @file
 *
 *
 */
#include <iostream>
  using namespace std;

#include "inv/ChemKit/ChemRadii.H"

#include "viz/CSRadiiCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

CSRadiiCmd::CSRadiiCmd(const string& name, Receiver * receiver)
    : Command(name, receiver)
{
   init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
CSRadiiCmd::~CSRadiiCmd()
{
}



void CSRadiiCmd::init()
{
   addParameter(new CommandParameter("atomIdx", 1));
   addParameter(new CommandParameter("radius", 1.0));
   addParameter(new CommandParameter("cpkradius", 1.0));

   setHelpMessage("Load system and user element colors.");
}



bool CSRadiiCmd::execute() throw(EcceException)
{

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());

   int index = getParameter("atomIdx")->getInteger();

   double radius = getParameter("radius")->getDouble();
   double cpkradius = getParameter("cpkradius")->getDouble();


   ChemRadii *radii = sg->getRadiiNode();
   radii->atomRadii.set1Value(index, radius);

   ChemRadii *cpkradii = sg->getCPKRadiiNode();
   cpkradii->atomRadii.set1Value(index, cpkradius);

   return true;
}

bool CSRadiiCmd::isUndoable() const
{
   return false;
}

void CSRadiiCmd::undo()
{
}
