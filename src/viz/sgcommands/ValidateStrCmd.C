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
#include "util/StringConverter.H"
#include "util/FFConfigData.H"

#include "dsm/SegFactory.H"

#include "viz/ValidateStrCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

ValidateStrCmd::ValidateStrCmd(const string& name, Receiver * receiver)
    : FragCloneCmd(name, receiver)
{
   init();
}

/**
 * Destructor.
 * Deletes undo related data.
 */
ValidateStrCmd::~ValidateStrCmd()
{
}

void ValidateStrCmd::init()
{
   string dir = StringConverter::toString(FFConfigData::getSegDirs(), "\n");
   addParameter(new CommandParameter("directories", dir));
   addParameter(new CommandParameter("fftype", "Amber"));
   setHelpMessage("Assign properties to atoms from fragment and segment files");
}

bool ValidateStrCmd::execute() throw(EcceException)
{
  bool ret = false;

  SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
  SGFragment *frag = sg->getFragment();

  if (frag->numAtoms() > 0 && frag->numResidues() > 0) {
    ret = true;

    cloneFragment(frag);

    //NEW STUFF

    // OK, You've got the scene graph fragment, now compare against segment
    // and fragment files. Start by compiling a list of directories containing
    // segment and fragment files.
    string segdirs = getParameter("directories")->getString();
    string fftype = getParameter("fftype")->getString();

    SegFactory segfactory;
    segfactory.clearEDSI();
    char* sdirect = strdup((char*)segdirs.c_str());
    char* segptr = strtok(sdirect,":\n");
    string msg;
    for (;segptr != NULL; segptr = strtok(NULL, ":\n")) {
      segfactory.addEDSI(segptr);
    }
    delete sdirect;

    // Scan through all residues

    int nresidues = frag->numResidues();
    for (int i=0; i<nresidues; i++) {

      // Check residue against available segments (or fragments) and assign
      // it a status.

      Residue* residue = frag->residue(i);
      Residue::Coupling coupling = residue->couplingInfo();
      Segment* segment = segfactory.getSegment(residue,coupling,frag,fftype);
      Residue::ResidueStatus status;
      if (segment != (Segment*)0) {
        status = segment->compare(residue,frag);
      } else {
        status = Residue::UNKNOWN;
      }
      residue->residueStatus(status);
    }

    //END NEW STUFF
    frag->touchNumbers();
    sg->touchChemDisplay();

    EventDispatcher::getDispatcher().publish(Event("ResidueChange"));
  }
  return ret;
}
