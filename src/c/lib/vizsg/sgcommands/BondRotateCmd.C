/**
   Add a rotation manipulator between two atoms (bonded or not).
*/

#include <iostream>
using std::cout;
using std::endl;

#include "inv/nodes/SoTransform.H"
#include "inv/nodes/SoScale.H"
#include "inv/actions/SoSearchAction.H"

#include "viz/SGFragment.H"
#include "viz/SGContainer.H"
#include "viz/BondRotateCmd.H"
#include "viz/AtomRotDragger.H"
#include "viz/MoveAction.H"

#include "util/EventDispatcher.H"
#include "util/Event.H"
using namespace ecce;


BondRotateCmd::BondRotateCmd(const string& name, Receiver * receiver)
  : Command(name, receiver)
{
  init();
}


BondRotateCmd::~BondRotateCmd()
{
}


void BondRotateCmd::init()
{
  setHelpMessage("Select two atoms to get a new bond rotation manipulator "
                 "wheel. The first atom selected will remain fixed.  If you "
                 "select a bond instead of two atoms, the selection of the "
                 "fixed atom is arbitrary.");
}


bool BondRotateCmd::execute() throw(EcceException)
{
  bool ret = false;

  setErrorMessage("");

  SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
  SGFragment *frag = sg->getFragment();

  if (frag->getNumberOfAtoms() > 0) {
    int numSel = frag->m_atomHighLight.size();
    //if (numSel != 2 && numSel != 4) {
    if (numSel > 4 || numSel == 0) {
      setErrorMessage("Either select two atoms between which a bond "
                      "rotation manipulator should be created or select 4 atoms "
                      "comprising the torsion you want to adjust.");
    } else if (numSel == 1 || numSel == 3) {
      // I found it obnoxious to post an error so just let this go through
      // without doing anything or complaining
    } else {
      TAtm *at1 = (TAtm*)0;
      TAtm *at2 = (TAtm*)0;
      if (numSel == 2) {
        at1 = frag->atomRef(frag->m_atomHighLight[0]);
        at2 = frag->atomRef(frag->m_atomHighLight[1]);
      } else {
        at1 = frag->atomRef(frag->m_atomHighLight[1]);
        at2 = frag->atomRef(frag->m_atomHighLight[2]);
      }
      if (!rotatorExists(sg->getTopRotManip(), at1, at2)) {

        SoTransform  * transf =  new SoTransform;
        SoSeparator  * sep = new SoSeparator;
        SoScale  * scal = new SoScale;
        AtomRotDragger * disc  = new AtomRotDragger(frag,at1,at2,sep);;

        // Why did paulo do stuff like this?
        //disc->parent->ref();

        sep->addChild(transf);
        sep->addChild(scal);
        sep->addChild(disc);

        EventDispatcher::getDispatcher().publish(Event("AddWheel", "",
                                                       "", disc));

        disc->setTransform(transf);

        SbVec3f pt1,pt2;
        const double *xyz = at1->coordinates();
        pt1.setValue( (float)xyz[0],  (float)xyz[1] , (float)xyz[2]);
        xyz = at2->coordinates();
        pt2.setValue( (float)xyz[0],  (float)xyz[1] , (float)xyz[2]);

        sg->getTopRotManip()->addChild(sep);
        transf->translation.setValue(0.5 * (pt1 + pt2));
        transf->rotation.setValue(SbRotation (SbVec3f(0.,0.,1.) ,(pt2 - pt1)) );
        scal->scaleFactor.setValue(0.5,0.5,0.5);
        ret = true;
      }
    }
  }
  return ret;
}


/**
 * Find out if we already have a rotator between these atoms.
 */
bool BondRotateCmd::rotatorExists(SoNode *top, TAtm *at1, TAtm *at2)
{
  bool ret = false;

  SoSearchAction *sa = new SoSearchAction;

  sa->setType(AtomRotDragger::getClassTypeId(),true);
  sa->setInterest(SoSearchAction::ALL);
  sa->setSearchingAll(true);

  sa->apply(top);

  SoPathList& paths = sa->getPaths();
  for (int idx=0; idx<paths.getLength(); idx++) {
    SoPath *path = paths[idx];
    AtomRotDragger *dragger = (AtomRotDragger*)path->getTail();
    if ((dragger->atom1() == at1 && dragger->atom2() == at2) ||
        (dragger->atom2() == at1 && dragger->atom1() == at2)) {
      ret = true;
      break;
    }
  }
  delete sa;
  return ret;
}


void BondRotateCmd::undo()
{
}


bool BondRotateCmd::isUndoable() const
{
  return false;
}
