/**
 * @file
 *
 *
 */
#include <iostream>
  using namespace std;

#include "util/SFile.H"

#include "util/EventDispatcher.H"
#include "util/Event.H"
  using namespace ecce;

#include "viz/AddFragmentCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

AddFragmentCmd::AddFragmentCmd(const string& name, Receiver * receiver)
    : FragCloneCmd(name, receiver)
{
   init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
AddFragmentCmd::~AddFragmentCmd()
{

}



void AddFragmentCmd::init()
{
   addParameter(new CommandParameter("streamType", ""));
   addParameter(new CommandParameter("mvmStream", ""));
   addParameter(new CommandParameter("units", "angstroms"));
   addParameter(new CommandParameter("fileName", ""));

   // If interactively connected fragment parts, connatom is the atom index 
   // in the new fragment to connect to in the existing system.
   addParameter(new CommandParameter("conatom", -1));

   // Location to add if adding in free space
   addParameter(new CommandParameter("x", 0.0));
   addParameter(new CommandParameter("y", 0.0));
   addParameter(new CommandParameter("z", 0.0));

   // Should bond orders be generated afterwords?
   addParameter(new CommandParameter("genBondOrders", true));

   // PDB specific parameters
   addParameter(new CommandParameter("selectModel", 1));
   addParameter(new CommandParameter("selectAltLoc", " "));
   addParameter(new CommandParameter("selectChainID", " "));

   setHelpMessage("Select a structure from the structure library.  Then click where you want it added.");
}



bool AddFragmentCmd::execute() throw(EcceException)
{
   bool ret = true;

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();


   int conatom = getParameter("conatom")->getInteger();

   string units = getParameter("units")->getString();

   string streamType = getParameter("streamType")->getString();
   string memStream = getParameter("mvmStream")->getString();
   string fileStream = getParameter("fileName")->getString();

   bool genBOs = getParameter("genBondOrders")->getBoolean();

   double x = getParameter("x")->getDouble();
   double y = getParameter("y")->getDouble();
   double z = getParameter("z")->getDouble();

   if (streamType == "" && fileStream != "") {
      // Need to figure it out
      SFile file(fileStream);
      streamType = file.extension();
      for (int idx=0; idx<streamType.size(); idx++) {
         streamType[idx] = toupper(streamType[idx]);
      }
   }
   if (streamType == "ENT") streamType = "PDB";


   if (memStream != "" || fileStream != "") {

      int selectMdl = getParameter("selectModel")->getInteger();
      string selectAlt = getParameter("selectAltLoc")->getString();
      string selectChain = getParameter("selectChainID")->getString();

      try {
         cloneFragment(frag);
         frag->addFragment(streamType, memStream, fileStream,
               units, conatom, x, y, z, genBOs, 
               selectMdl, selectAlt, selectChain);


         frag->touchNumbers();
         sg->touchChemDisplay();
         sg->touchLattice(); // Not sure if this should be here.  Maybe
                             // only if we went from 0 to > 0 atoms.
                             // What happens if you merge one PBC system to
                             // another???

         // event notification or vcommand properties
         string ename = "GeomChange";
         if (frag->numResidues() > 0) {
            ename = "ResidueChange";
         }
         Event event(ename);
         EventDispatcher::getDispatcher().publish(event);

      } catch (EcceException& ex) {
         // Clearing is helpful in the context of structlib
         frag->m_atomHighLight.clear();
         frag->m_bondHighLight.clear();
         EventDispatcher::getDispatcher().publish(Event("SelectionChange"));
         ret = false;
         clearUndoFragment();
         throw ex;
      }

   }

   return ret;
}
