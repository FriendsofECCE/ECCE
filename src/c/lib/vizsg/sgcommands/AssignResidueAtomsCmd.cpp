/**
 * @file
 *
 *
 */
#include <iostream>
  using namespace std;

#include "util/EventDispatcher.hpp"
#include "util/Event.hpp"
using namespace ecce;
#include "util/FFConfigData.hpp"
#include "util/StringConverter.hpp"
#include "util/StringTokenizer.hpp"

#include "dsm/SegFactory.hpp"

#include "tdat/Segment.hpp"
#include "tdat/FragUtil.hpp"
#include "tdat/TPerTab.hpp"
#include "tdat/ShapeData.hpp"

#include "viz/AssignResidueAtomsCmd.hpp"
#include "viz/SGFragment.hpp"
#include "viz/SGContainer.hpp"


AssignResidueAtomsCmd::AssignResidueAtomsCmd(const string& name, Receiver * receiver)
    : FragCloneCmd(name, receiver)
{
   init();
}

/**
 * Destructor.
 * Deletes undo related data.
 */
AssignResidueAtomsCmd::~AssignResidueAtomsCmd()
{
}

void AssignResidueAtomsCmd::init()
{
   string dir = StringConverter::toString(FFConfigData::getSegDirs(), "\n");
   addParameter(new CommandParameter("directories", dir));
   addParameter(new CommandParameter("fftype", "Amber"));
   addParameter(new CommandParameter("residue", -1));
   setHelpMessage(" ");
}

bool AssignResidueAtomsCmd::execute() throw(EcceException)
{
   bool ret = false;

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();
   if (frag->numAtoms() > 0 && frag->numResidues() > 0) {
      ret = true;

      // undo
      cloneFragment(frag);

      int resindex = getParameter("residue")->getInteger();
      Residue *res = frag->residue(resindex);

      string segdirs = getParameter("directories")->getString();

      SegFactory *segfactory = new SegFactory;
      const Residue::Coupling coupling=res->couplingInfo();

      StringTokenizer tokenizer(segdirs);
      string segptr = tokenizer.next(":\n");
      for (; !segptr.empty(); segptr = tokenizer.next(":\n"))
         segfactory->addEDSI(segptr.c_str());

      Segment *seg = segfactory->getSegment(res,coupling,frag);
      vector<TAtm*> atomlist = frag->findResidueAtoms(res);
      if (seg != (Segment*)0) {
         seg->assignResidueAtoms(res,atomlist);
         Residue::ResidueStatus status;
         status = seg->compare(res, frag);
         res->residueStatus(status);
      }


      frag->touchNumbers();
      sg->touchChemDisplay();

      EventDispatcher::getDispatcher().publish(Event("GeomChange"));
   }
   return ret;
}
