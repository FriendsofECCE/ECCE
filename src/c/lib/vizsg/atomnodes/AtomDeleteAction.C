#include "inv/elements/SoComplexityElement.H"
#include "inv/elements/SoComplexityTypeElement.H"
#include "inv/elements/SoCoordinateElement.H"
#include "inv/elements/SoElements.H"
#include "inv/elements/SoFontNameElement.H"
#include "inv/elements/SoFontSizeElement.H"
#include "inv/elements/SoModelMatrixElement.H"
#include "inv/elements/SoProfileCoordinateElement.H"
#include "inv/elements/SoProfileElement.H"
#include "inv/elements/SoSwitchElement.H"
#include "inv/elements/SoUnitsElement.H"
#include "inv/elements/SoViewVolumeElement.H"
#include "inv/elements/SoViewingMatrixElement.H"
#include "inv/elements/SoViewportRegionElement.H"
#include "inv/nodes/SoCamera.H"
#include "inv/nodes/SoComplexity.H"
#include "inv/nodes/SoCoordinate3.H"
#include "inv/nodes/SoCoordinate4.H"
#include "inv/nodes/SoFont.H"
#include "inv/nodes/SoGroup.H"
#include "inv/nodes/SoProfile.H"
#include "inv/nodes/SoProfileCoordinate2.H"
#include "inv/nodes/SoProfileCoordinate3.H"
#include "inv/nodes/SoSphere.H"
#include "inv/nodes/SoTransformation.H"
#include "inv/nodes/SoSeparator.H"

#include "viz/SGFragment.H"

#include "viz/AtomMeasureDist.H"
#include "viz/AtomMeasureAngle.H"
#include "viz/AtomMeasureTorsion.H"
#include "viz/AtomLine.H"

#include "viz/AtomRotDragger.H"
#include "viz/AtomRTDragger.H"

#include "viz/AtomDeleteAction.H"

SO_ACTION_SOURCE(AtomDeleteAction);

//
// Initializes the GetVolumeAction class. This is a one-time
// thing that is done after database initialization and before
// any instance of this class is constructed.
//

void AtomDeleteAction::initClass()
{
  // Initialize the run-time type variables
  SO_ACTION_INIT_CLASS(AtomDeleteAction, SoAction);

  // Enable elements that are involved in volume computation.
  // Most of these deal with geometrix properties
  // (coordinates, profiles) or transformations (model matrix,
  // units). Some are needed for certain groups (switches,
  // level-of-detail) to function correctly.
  SO_ENABLE(AtomDeleteAction, SoModelMatrixElement);
  SO_ENABLE(AtomDeleteAction, SoComplexityElement);
  SO_ENABLE(AtomDeleteAction, SoComplexityTypeElement);
  SO_ENABLE(AtomDeleteAction, SoCoordinateElement);
  SO_ENABLE(AtomDeleteAction, SoFontNameElement);
  SO_ENABLE(AtomDeleteAction, SoFontSizeElement);
  SO_ENABLE(AtomDeleteAction, SoProfileCoordinateElement);
  SO_ENABLE(AtomDeleteAction, SoProfileElement);
  SO_ENABLE(AtomDeleteAction, SoSwitchElement);
  SO_ENABLE(AtomDeleteAction, SoUnitsElement);
  SO_ENABLE(AtomDeleteAction, SoViewVolumeElement);
  SO_ENABLE(AtomDeleteAction, SoViewingMatrixElement);
  SO_ENABLE(AtomDeleteAction, SoViewportRegionElement);

  // Now we need to register methods to implement this action
  // for various node classes. We have created implementations
  // for two specific shape nodes, SoCube and SoSphere, so we
  // can register specific methods for those two classes. We
  // also want to make sure that group classes traverse their
  // children correctly for this action, so we will use a
  // method that calls doAction() to handle groups. Finally,
  // we need to make sure that relevant property nodes set up
  // the state correctly; we can use the same method that
  // calls doAction() for these classes, as well. We will use
  // the SO_ACTION_ADD_METHOD() macro to make this easier.

  // This registers a method to call for SoNode, so it will be
  // used for any node class that does not have a more
  // specific method registered for it. This makes sure that
  // there is always a method to call for any node. The
  // "nullAction" method is defined on SoAction for use in
  // cases like this.
  SO_ACTION_ADD_METHOD(SoNode,               nullAction);

  // These register methods for the two shapes that can
  // really handle the action
  SO_ACTION_ADD_METHOD(AtomRotDragger,  verify_rot    );
  SO_ACTION_ADD_METHOD(AtomRTDragger, verify_manip  );
  SO_ACTION_ADD_METHOD(AtomMeasureDist, verify_dist  );
  SO_ACTION_ADD_METHOD(AtomMeasureAngle, verify_theta  );
  SO_ACTION_ADD_METHOD(AtomMeasureTorsion, verify_phy  );
  SO_ACTION_ADD_METHOD(AtomLine, verify_line  );

  // Register the method that calls doAction() for all group
  // classes and for relevant properties (transformations,
  // coordinates, profiles, and so on).
  SO_ACTION_ADD_METHOD(SoCamera,             callDoAction);
  SO_ACTION_ADD_METHOD(SoComplexity,         callDoAction);
  SO_ACTION_ADD_METHOD(SoCoordinate3,        callDoAction);
  SO_ACTION_ADD_METHOD(SoCoordinate4,        callDoAction);
  SO_ACTION_ADD_METHOD(SoFont,               callDoAction);
  SO_ACTION_ADD_METHOD(SoGroup,              callDoAction);
  SO_ACTION_ADD_METHOD(SoProfile,            callDoAction);
  SO_ACTION_ADD_METHOD(SoProfileCoordinate2, callDoAction);
  SO_ACTION_ADD_METHOD(SoProfileCoordinate3, callDoAction);
  SO_ACTION_ADD_METHOD(SoTransformation,     callDoAction);


}

//
// Constructor
//

AtomDeleteAction::AtomDeleteAction()
{
  SO_ACTION_CONSTRUCTOR(AtomDeleteAction);
}

//
// Destructor. Does nothing.
//

AtomDeleteAction::~AtomDeleteAction()
{
}


///////////////////////////////////////////////////////////////////////////////
// Description
//   Initiates action on a graph. This is called when the action
//   is applied to a node, a path, or a path list. It gives us a
//   chance to initialize things before beginning traversal.
///////////////////////////////////////////////////////////////////////////////
void AtomDeleteAction::beginTraversal(SoNode *node)
{
  traverse(node);        // Begin traversal at the given root node.
}

///////////////////////////////////////////////////////////////////////////////
// Description
//   This action deletes the RotTransDragger object (node) if it is
//   over any of the atoms in the highlight list.
//
// Implementation
//   Note that the parent SoSeparator was added to RotTransDragger just
//   so we could access it here and remove all the children.  Trying to
//   unref the ball itself led to the following run time death:
//      Inventor error in (internal) SoBase::destroy(): Got an auditor of type 1
//      ****Pure virtual function called
//   Also, since I added the parent separator and remove its children,
//   the separator is left lying around.  Attempts to unref it don't 
//   do anything.
//   I don't know if it is a problem to have them lying around.
///////////////////////////////////////////////////////////////////////////////
void AtomDeleteAction::verify_manip(SoAction *action, SoNode *node)
{
  // The action is really an instance of AtomDeleteAction

  // And the node pointer is really a RotTransDragger:
  AtomRTDragger * ball =   ( AtomRTDragger * ) node;

  SGFragment *frag = ball->getFragment();

  int at,ati;
  int cnt = frag->m_atomHighLight.size();
  for (int i=0 ; i < cnt  ; i++) {
    ati = frag->m_atomHighLight[i];
    for (int j=0 ; j < ball->m_selected.size()  ; j++) {

      at = ((Fragment*)frag)->atomIndex(*(ball->m_selected[j]));
      if (at == ati) {
        ball->getParent()->removeAllChildren();
        //ball->parent->unref();   // This doesn't seem to do anything
        return;  // done with this instance of RotTransDragger
      }
    }
  }
}


///////////////////////////////////////////////////////////////////////////////
// Description
//   This action deletes the rotator if either of the atoms has
//   been deleted from the fragment.  It is called after a delete.  An
//   alternative would be to call this before a delete and have it look
//   at the selection vector.  This would probably be faster but it
//   presumes that the command will suceed and has to be embedded in the
//   command itself.  I can't decide which is better but it wouldn't be
//   much work to change either way.
//  
///////////////////////////////////////////////////////////////////////////////
void AtomDeleteAction::verify_rot(SoAction *action, SoNode *node)
{
  // The action is really an instance of AtomDeleteAction

  AtomRotDragger * disc = ( AtomRotDragger *) node;

  SGFragment *frag = disc->getFragment();

  int at1, at2, ati;
  int cnt = frag->m_atomHighLight.size();
  for (int i=0 ; i < cnt  ; i++) {
    ati = frag->m_atomHighLight[i];
    at1 = ((Fragment*)frag)->atomIndex(*(disc->atom1()));
    at2 = ((Fragment*)frag)->atomIndex(*(disc->atom2()));
    if (at1 == ati || at2 == ati) {
      disc->getParent()->removeAllChildren();
      return;  // done with this instance of RotTransDragger
    }
  }

}


///////////////////////////////////////////////////////////////////////////////
// Description
//   This action deletes the AtomMeasureDist object (node) if it is
//   connected two any of the atoms in the highlight list.
//  
// Implementation
//   Same principle as verify_rot.
///////////////////////////////////////////////////////////////////////////////
void AtomDeleteAction::verify_dist(SoAction *action, SoNode *node)
{
  // The action is really an instance of AtomDeleteAction

  AtomMeasureDist * dist = (AtomMeasureDist  *) node;

  SGFragment *frag = dist->getFragment();

  int at1, at2, ati;
  int cnt = frag->m_atomHighLight.size();
  for (int i=0 ; i < cnt  ; i++) {
    ati = frag->m_atomHighLight[i];
    at1 = ((Fragment*)frag)->atomIndex(*(dist->atom1()));
    at2 = ((Fragment*)frag)->atomIndex(*(dist->atom2()));
    if (at1 == ati || at2 == ati) {
      dist->getParent()->removeAllChildren();
      return;  // done with this instance 
    }
  }
}

void AtomDeleteAction::verify_line(SoAction *action, SoNode *node)
{
  // The action is really an instance of AtomDeleteAction

  AtomLine * line = (AtomLine  *) node;

  SGFragment *frag = line->getFragment();

  int at1, at2, ati;
  int cnt = frag->m_atomHighLight.size();
  for (int i=0 ; i < cnt  ; i++) {
    ati = frag->m_atomHighLight[i];
    at1 = ((Fragment*)frag)->atomIndex(*(line->atom1()));
    at2 = ((Fragment*)frag)->atomIndex(*(line->atom2()));
    if (at1 == ati || at2 == ati) {
      line->getParent()->removeAllChildren();
      return;  // done with this instance 
    }
  }
}

void AtomDeleteAction::verify_theta(SoAction *action, SoNode *node)
{
  // The action is really an instance of AtomDeleteAction

  AtomMeasureAngle * the = (AtomMeasureAngle  *) node;

  SGFragment *frag = the->getFragment();

  int at1, at2, at3, ati;
  for (int i=0 ; i < frag->m_atomHighLight.size()  ; i++) {
    ati = frag->m_atomHighLight[i];
    at1 = ((Fragment*)frag)->atomIndex(*(the->atom1()));
    at2 = ((Fragment*)frag)->atomIndex(*(the->atom2()));
    at3 = ((Fragment*)frag)->atomIndex(*(the->atom3()));
    if (at1 == ati || at2 == ati || at3 == ati) {
      the->getParent()->removeAllChildren();
      return;  // done with this instance 
    }
  }
}

void AtomDeleteAction::verify_phy(SoAction *action, SoNode *node)
{
  // The action is really an instance of AtomDeleteAction

  AtomMeasureTorsion * phy = (AtomMeasureTorsion  *) node;

  SGFragment *frag = phy->getFragment();

  int at1, at2, at3, at4, ati;
  for (int i=0 ; i < frag->m_atomHighLight.size()  ; i++) {
    ati = frag->m_atomHighLight[i];
    at1 = ((Fragment*)frag)->atomIndex(*(phy->atom1()));
    at2 = ((Fragment*)frag)->atomIndex(*(phy->atom2()));
    at3 = ((Fragment*)frag)->atomIndex(*(phy->atom3()));
    at4 = ((Fragment*)frag)->atomIndex(*(phy->atom4()));
    if (at1 == ati || at2 == ati || at3 == ati || at4 == ati) {
      phy->getParent()->removeAllChildren();
      return;  // done with this instance 
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// Description
//   This method implements the action for all of the relevant
//   non-shape node classes.  Copied from example somewhere I guess.
///////////////////////////////////////////////////////////////////////////////
void AtomDeleteAction::callDoAction(SoAction *action, SoNode *node)
{
  node->doAction(action);
}
