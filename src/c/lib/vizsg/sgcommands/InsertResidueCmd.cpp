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

#include "viz/InsertResidueCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"
#include "tdat/Residue.H"

InsertResidueCmd::InsertResidueCmd(const string& name, Receiver * receiver)
      : FragCloneCmd(name, receiver)
{
   init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
InsertResidueCmd::~InsertResidueCmd()
{

}



void InsertResidueCmd::init()
{
  addParameter(new CommandParameter("index", 0));
  addParameter(new CommandParameter("name", "UNK"));
}



bool InsertResidueCmd::execute() throw(EcceException)
{
   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();

   // Set up undo data
   cloneFragment(frag);

   int index = getParameter("index")->getInteger();
   string name = getParameter("name")->getString();

   int nres = frag->numResidues();

   if (nres == 0) {
      Residue *res = new Residue(name.c_str(),1);
      frag->addResidue(res);
   } else if (index >= nres) {
      Residue* res = frag->findResidue(nres-1);
      int new_number = res->number()+1;
      char new_chain = res->chain();
      char new_icode = ' ';
      string new_segID = res->segID();
      Residue *res1 = new Residue(name.c_str(),new_number);
      res1->insertCode(new_icode);
      res1->chain(new_chain);
      if (strlen(new_segID.c_str()) > 0) {
         res1->segID(new_segID.c_str());
      }
      frag->addResidue(res1);
   } else {
      Residue* res = frag->findResidue(index);
      int new_number = res->number()-1;
      char new_chain = res->chain();
      char new_icode = ' ';
      string new_segID = res->segID();
      if (new_number < 1) new_number = 1;

      // Check residue before selected residue to see if it has
      // the same residue number, insertion code, and chain ID.
      // Modify insertion code if there is a match.

      if (index > 0) {
         Residue* res1 = frag->findResidue(index-1);
         int r1_number = res1->number();
         char r1_chain = res1->chain();
         char r1_icode = res1->insertCode();
         if (r1_number == new_number && r1_chain == new_chain &&
               r1_icode == new_icode) {
            if (new_number == res->number()) {
               if (res->insertCode() == 'A') {
                  new_icode = 'B';
               } else {
                  new_icode = 'A';
               }
            } else {
               new_icode = 'A';
            }
         }
      } else {

         // There is no previous row. Check to see if new row
         // matches selected row.

         if (new_number == res->number() && new_chain == res->chain() &&
               new_icode == res->insertCode()) {
            new_icode = 'A';
         }
      }
      Residue *res1 = new Residue(name.c_str(),new_number);
      res1->insertCode(new_icode);
      res1->chain(new_chain);
      if (strlen(new_segID.c_str()) > 0) {
         res1->segID(new_segID.c_str());
      }
      frag->insertResidue(index,res1);
   }

   EventDispatcher::getDispatcher().publish(Event("ResidueChange"));

   return true;
}
