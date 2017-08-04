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
#include "inv/nodes/SoCube.H"
#include "inv/nodes/SoFont.H"
#include "inv/nodes/SoGroup.H"
#include "inv/nodes/SoProfile.H"
#include "inv/nodes/SoProfileCoordinate2.H"
#include "inv/nodes/SoProfileCoordinate3.H"
#include "inv/nodes/SoSphere.H"
#include "inv/nodes/SoTransformation.H"

#include "viz/AtomRotDragger.H"
#include "viz/AtomRTDragger.H"
#include "viz/SGFragment.H"
#include "viz/AtomMeasureDist.H"
#include "viz/AtomMeasureAngle.H"
#include "viz/AtomMeasureTorsion.H"
#include "viz/AtomLine.H"

#include "viz/MoveAction.H"

SO_ACTION_SOURCE(MoveAction);

//
// Initializes the GetVolumeAction class. This is a one-time
// thing that is done after database initialization and before
// any instance of this class is constructed.
//

void MoveAction::initClass()
{
   // Initialize the run-time type variables
   SO_ACTION_INIT_CLASS(MoveAction, SoAction);

   // Enable elements that are involved in volume computation.
   // Most of these deal with geometrix properties
   // (coordinates, profiles) or transformations (model matrix,
   // units). Some are needed for certain groups (switches,
   // level-of-detail) to function correctly.
   SO_ENABLE(MoveAction, SoModelMatrixElement);
   SO_ENABLE(MoveAction, SoComplexityElement);
   SO_ENABLE(MoveAction, SoComplexityTypeElement);
   SO_ENABLE(MoveAction, SoCoordinateElement);
   SO_ENABLE(MoveAction, SoFontNameElement);
   SO_ENABLE(MoveAction, SoFontSizeElement);
   SO_ENABLE(MoveAction, SoProfileCoordinateElement);
   SO_ENABLE(MoveAction, SoProfileElement);
   SO_ENABLE(MoveAction, SoSwitchElement);
   SO_ENABLE(MoveAction, SoUnitsElement);
   SO_ENABLE(MoveAction, SoViewVolumeElement);
   SO_ENABLE(MoveAction, SoViewingMatrixElement);
   SO_ENABLE(MoveAction, SoViewportRegionElement);

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
   SO_ACTION_ADD_METHOD(AtomRotDragger,  jump_rot    );
   SO_ACTION_ADD_METHOD(AtomRTDragger, jump_manip  );
   SO_ACTION_ADD_METHOD(AtomMeasureDist, jump_dist  );
   SO_ACTION_ADD_METHOD(AtomMeasureAngle, jump_theta  );
   SO_ACTION_ADD_METHOD(AtomMeasureTorsion, jump_phy  );
   SO_ACTION_ADD_METHOD(AtomLine, jump_line  );

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

MoveAction::MoveAction()
{
   SO_ACTION_CONSTRUCTOR(MoveAction);
}

//
// Destructor. Does nothing.
//

MoveAction::~MoveAction()
{
}

//
// Initiates action on a graph. This is called when the action
// is applied to a node, a path, or a path list. It gives us a
// chance to initialize things before beginning traversal.
//

void MoveAction::beginTraversal(SoNode *node)
{
  
   // Begin traversal at the given root node.
   traverse(node);
}

//
// This method implements the action for an SoCube node.
//

void MoveAction::jump_rot(SoAction *action, SoNode *node)
{
   // The action is really an instance of MoveAction

  AtomRotDragger * disc = ( AtomRotDragger *) node;

  SbVec3f pt1,pt2;

  const double * xyz;

  xyz = disc->atom1()->coordinates();
  pt1.setValue( (float)xyz[0],  (float)xyz[1] , (float)xyz[2]);

  xyz = disc->atom2()->coordinates();
  pt2.setValue( (float)xyz[0],  (float)xyz[1] , (float)xyz[2]);


  disc->getTransform()->translation.setValue(0.5 * ( pt1 + pt2));
  disc->getTransform()->rotation.setValue(SbRotation (SbVec3f(0.,0.,1.) ,(pt2 - pt1)) );
}

void MoveAction::jump_dist(SoAction *action, SoNode *node)
{
   // The action is really an instance of MoveAction

  AtomMeasureDist * dist = (AtomMeasureDist  *) node;

  dist->jump();
}

void MoveAction::jump_line(SoAction *action, SoNode *node)
{
   // The action is really an instance of MoveAction

  AtomLine * line = (AtomLine  *) node;

  line->jump();
}

void MoveAction::jump_theta(SoAction *action, SoNode *node)
{
   // The action is really an instance of MoveAction

  AtomMeasureAngle * the = (AtomMeasureAngle  *) node;

  the->jump();
}

void MoveAction::jump_phy(SoAction *action, SoNode *node)
{
   // The action is really an instance of MoveAction

  AtomMeasureTorsion * phy = (AtomMeasureTorsion  *) node;

  phy->jump();
}
//
// This method implements the action for an SoSphere node.
//

void MoveAction::jump_manip(SoAction *action, SoNode *node)
{
   // The action is really an instance of MoveAction

   // And the node pointer is really a sphere:
   AtomRTDragger * ball =   ( AtomRTDragger * ) node;
   SGFragment * frag = (SGFragment*)ball->getFragment(); 

   if ( ball->m_alert) return;     

   // get the center 
   int counter = 0;
   SbVec3f new_center;
   new_center.setValue(0.,0.,0.);
   int at;
   int j;
   for (j=0 ; j < ball->m_selected.size()  ; j++) {
     at = ((Fragment*)frag)->atomIndex(*(ball->m_selected[j]));
     new_center = new_center + frag->getAtomCoordinates(at);
     counter ++;
   }
   new_center = ( 1./counter ) * new_center;


   // get the radii
   SbVec3f  pt;
   float  radii = 1.0;
   for ( j=0 ; j < ball->m_selected.size()  ; j++)
   { 
     at = ((Fragment*)frag)->atomIndex(*( ball->m_selected[j]));
     pt = frag->getAtomCoordinates(at) - new_center;
     if (pt.length() > radii) radii = pt.length();
   }
   radii = 1.1 * radii;

   ball->m_save_scale = 0.5*radii;
   new_center = new_center - ball->translation.getValue() * ball->m_save_scale;

   ball->enableValueChangedCallbacks(FALSE );
   ball->m_sca->scaleFactor.setValue(0.5*radii,0.5*radii,0.5*radii);
   ball->m_tran->translation.setValue(new_center); 
   ball->m_center.setValue(new_center.getValue());
   ball->enableValueChangedCallbacks( TRUE );
}


//
// This method implements the action for all of the relevant
// non-shape node classes.
//
void MoveAction::callDoAction(SoAction *action, SoNode *node)
{
   node->doAction(action);
}

//
