/**
 * Command to add a sphere manipulator
 */

#include <iostream>
using std::cout;
using std::endl;

#include <algorithm>
using std::find;
using std::set_difference;

#include "inv/nodes/SoTransform.H"
#include "inv/nodes/SoScale.H"
#include "inv/nodes/SoTranslation.H"
#include "inv/actions/SoSearchAction.H"

#include "viz/SGFragment.H"
#include "viz/SGContainer.H"
#include "viz/AtomMoveCmd.H"
#include "viz/AtomRTDragger.H"

#include "util/EventDispatcher.H"
#include "util/Event.H"
using namespace ecce;


AtomMoveCmd::AtomMoveCmd(const string& name, Receiver * receiver)
  : Command(name, receiver)
{
  init();
}


AtomMoveCmd::~AtomMoveCmd()
{
}


void AtomMoveCmd::init()
{
  setHelpMessage("A new rotate/translate sphere manipulator will appear for the "
                 "atoms that are selected when you enter this mode.  Alternatively, "
                 "after entering this mode without a selection, select one or "
                 "more atoms to get a new manipulator.");
}


bool AtomMoveCmd::execute() throw(EcceException)
{
  bool ret = false;

  setErrorMessage("");

  SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
  SGFragment *frag = sg->getFragment();

  // Are there any atoms to move?
  if (frag->m_atomHighLight.size() > 0 ) {

    vector<TAtm*> selAtoms;
    int nSelAtoms = frag->m_atomHighLight.size();
    for ( int idx=0 ; idx < nSelAtoms ; idx++) {
      selAtoms.push_back(frag->atomRef(frag->m_atomHighLight[idx]));
    }

    if (!manipExists(sg->getTopManip(), selAtoms)) {
      ret = true;
      // create nodes
      SoSeparator     * sep   = new SoSeparator;
      sg->getTopManip()->addChild(sep);

      SoTranslation   * trans = new SoTranslation;
      sep->addChild(trans);
      SoScale         * scal  = new SoScale;
      sep->addChild(scal);
      AtomRTDragger * ball  = new AtomRTDragger;
      sep->addChild(ball);

      EventDispatcher::getDispatcher().publish(Event("AddDragger", "",
                                                     "", ball));

      ball->setFragment(frag);
      ball->setParent(sep);

      // Why does Paulo do this?
      sep->ref();

      //  insert atoms numbers in dragger
      int at,j;

      // Insert nub parents.  If a nub is selected, the atom is assumed to
      // be selected too.  Also insert the atoms actually selected by the
      // user.
      int at1;
      TAtm *atm, *parent, *ati;
      int cnt = frag->m_atomHighLight.size();
      for ( j=0 ; j < cnt ; j++) {
        at1 =  frag->m_atomHighLight[j];
        atm = frag->atomRef(at1);
        if (atm->atomicSymbol() == "Nub") {
          parent = frag->nubParent(atm);
          if (parent != (TAtm*)0) {
            at = ((Fragment*)frag)->atomIndex(*parent);
            ati = frag->atomRef(at);
            if(find(ball->m_selected.begin(), ball->m_selected.end(), ati) ==
               ball->m_selected.end()) {
              ball->m_selected.push_back(ati);
            }
          }
        } else {
          if(find(ball->m_selected.begin(), ball->m_selected.end(), atm) ==
             ball->m_selected.end()) {
            ball->m_selected.push_back(atm);
          }
        }

      }

      // Now insert all nubs of selected atoms.  If an atom is selected then
      // its nubs are implicitly selected.
      cnt = ball->m_selected.size();
      for ( j=0 ; j < cnt ; j++) {
        at1 =  ((Fragment*)frag)->atomIndex(*(ball->m_selected[j]));
        vector<TAtm*> *connected = frag->connectedAtoms(at1);
        int ccnt = connected->size();
        for (int idx=0; idx<ccnt; idx++) {
          if ((*connected)[idx]->atomicSymbol() == "Nub") {
            atm = (*connected)[idx];
            if(find(ball->m_selected.begin(), ball->m_selected.end(), atm) ==
               ball->m_selected.end()) {
              ball->m_selected.push_back(atm);
            }
          }
        }
        delete connected;
      }

      // get the center of all atoms
      int counter = 0;
      ball->m_center.setValue(0.,0.,0.);
      cnt = ball->m_selected.size();
      for (j=0 ; j < cnt  ; j++) {
        at = ((Fragment*)frag)->atomIndex(*(ball->m_selected[j]));
        ball->m_center = ball->m_center + frag->getAtomCoordinates(at);
        counter ++;
      }
      ball->m_center = ( 1./counter ) * ball->m_center;

      // get the radii
      SbVec3f  pt;
      float  radii = 1.0;
      for ( j=0 ; j < ball->m_selected.size()  ; j++) {
        at = ((Fragment*)frag)->atomIndex(*( ball->m_selected[j]));
        pt =   frag->getAtomCoordinates(at) - ball->m_center;
        if (pt.length() > radii) radii = pt.length();
      }
      radii = 1.1 * radii;

      ball->setFragment(frag);
      ball->m_tran = trans;
      ball->m_sca = scal;


      ball->m_save_scale = 0.5*radii;
      scal->scaleFactor.setValue(0.5*radii,0.5*radii,0.5*radii);
      trans->translation.setValue(ball->m_center);
      ball->m_old_trans.setValue(0.,0.,0.);
      ball->m_old_rot.setValue(ball->rotation.getValue().getValue());
    }
  }
  return ret;
}


/**
 * See if the set of atoms already has a manip on it.
 * Currently really dumb - atoms must be in same order.
 * Could check arbitrary order or any overlap for that matter.
 */
bool AtomMoveCmd::manipExists(SoNode *top, vector<TAtm*> selected)
{
  bool ret = false;
  SoSearchAction *sa = new SoSearchAction;

  sa->setType(AtomRTDragger::getClassTypeId(),true);
  sa->setInterest(SoSearchAction::ALL);
  sa->setSearchingAll(true);

  sa->apply(top);

  SoPathList& paths = sa->getPaths();
  vector<TAtm*> diffs;
  for (int idx=0; idx<paths.getLength(); idx++) {
    diffs.clear();
    SoPath *path = paths[idx];
    AtomRTDragger *dragger = (AtomRTDragger*)path->getTail();
    std::insert_iterator<vector<TAtm*> > oi(diffs,diffs.begin());
    set_difference(selected.begin(), selected.end(),
                   dragger->m_selected.begin(), dragger->m_selected.end(),
                   oi);

    if (diffs.size() == 0) {
      ret = true;
      break;
    }
  }
  delete sa;
  return ret;
}


void AtomMoveCmd::undo()
{
}


bool AtomMoveCmd::isUndoable() const
{
  return false;
}
