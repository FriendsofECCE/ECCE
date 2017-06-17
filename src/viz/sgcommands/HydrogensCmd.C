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

#include "dsm/SegFactory.H"

#include "tdat/Segment.H"
#include "tdat/FragUtil.H"
#include "tdat/TPerTab.H"
#include "tdat/ShapeData.H"

#include "viz/HydrogensCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

#define PI 3.14159265358979323846

HydrogensCmd::HydrogensCmd(const string& name, Receiver * receiver)
    : FragCloneCmd(name, receiver)
{
   init();
}

/**
 * Destructor.
 * Deletes undo related data.
 */
HydrogensCmd::~HydrogensCmd()
{
}

void HydrogensCmd::init()
{
   addParameter(new CommandParameter("all", false));
   addParameter(new CommandParameter("checkStatus", false));
   setHelpMessage("Convert all nubs to hydrogen atoms");
}

bool HydrogensCmd::execute() throw(EcceException)
{
  bool ret = false;

  SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
  SGFragment *frag = sg->getFragment();
  if (frag->numAtoms() > 0) {
    ret = true;

    cloneFragment(frag);

    bool all = getParameter("all")->getBoolean();
    bool checkStatus = getParameter("checkStatus")->getBoolean();

    ret = sg->addHydrogens(frag, all, checkStatus);

    if (ret) {
      frag->touchNumbers();
      sg->touchChemDisplay();
    }

    EventDispatcher::getDispatcher().publish(Event("GeomChange"));
  }
  return ret;
}
