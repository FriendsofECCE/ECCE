#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

#include "inv/SoPickedPoint.H"
#include "inv/actions/SoHandleEventAction.H"

#include "inv/events/SoEvent.H"
#include "inv/events/SoLocation2Event.H"
#include "inv/events/SoMouseButtonEvent.H"
#include "inv/events/SoLocation2Event.H"
#include "inv/events/SoKeyboardEvent.H"
#include "inv/elements/SoViewVolumeElement.H"

#include "inv/misc/SoCallbackList.H"
#include "inv/nodes/SoCamera.H"
#include "inv/SbLinear.H"
#include "inv/nodes/SoCube.H"
#include "inv/nodes/SoCone.H"
#include "inv/nodes/SoTransform.H"
#include "inv/actions/SoSearchAction.H"

#include "inv/ChemKit/ChemBaseData.H"
#include "inv/ChemKit/ChemDisplay.H"
#include "inv/ChemKit/ChemInit.H"
#include "inv/ChemKit/ChemDisplayPath.H"

#include "viz/SGFragment.H"

#include "wxviz/MotionListener.H"
#include "wxviz/MotionData.H"
#include "wxviz/SGSelection.H"
#include "wxviz/SGViewer.H"

using std::find;

SO_NODE_SOURCE(SGSelection);


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Constructor initialization
//
// Use: private
//
////////////////////////////////////////////////////////////////////////
void SGSelection::constructorCommon()
{
  SO_NODE_CONSTRUCTOR(SGSelection);

  clearRightClick();

  setDoubleClick(false);
  p_lastClickUpTime = 0;
  p_doubleClickAtom = -1;
  setDoubleClickInterval( 300 );

  // Mouse event callback functions
  mouseDownCBList = NULL;
  mouseUpCBList = NULL;
  mouseMoveCBList = NULL;

  mouse3Active = false;
  p_shiftActive = false;

  isBuiltIn = true;
  freePickflag = false;
  distPickflag = false;
  thetaPickflag = false;
  phyPickflag = false;
  lassoPickflag = false;

  togglePolicy.setValue(ChemSelection::SHIFT);

  p_inMotion = false;
  p_isdown = false;
  p_down = 0;

  // This doesn't seem to do anything.  You can see I tested it with
  // a large number.
  //lassoSelectCutoff.setValue(500);

}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Constructors
//  
// Use: public
////////////////////////////////////////////////////////////////////////
SGSelection::SGSelection()
{
  constructorCommon();
}     
    
SGSelection::SGSelection(int32_t numChildren) : ChemSelection(numChildren)
{
  constructorCommon();
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Destructor.
//
// Use: protected
//
////////////////////////////////////////////////////////////////////////
SGSelection::~SGSelection()
{
  delete mouseDownCBList;
  delete mouseUpCBList;
  delete mouseMoveCBList;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Class initialization
//
// Use: protected
//
////////////////////////////////////////////////////////////////////////
void SGSelection::initClass()
{
  // Initialize Molecular Inventor classes if necessary
  ChemInit::initClasses();

  SO_NODE_INIT_CLASS(SGSelection, ChemSelection, "ChemSelection");
}

void SGSelection::addMotionListener(MotionListener *l)
{
  p_motionListeners.push_back(l);
}

bool SGSelection::isFreePick() const
{
  return freePickflag;
}

bool SGSelection::isLassoPick() const
{
  return lassoPickflag;
}

bool SGSelection::isDistancePick() const
{
  return distPickflag;
}

bool SGSelection::isAnglePick() const
{
  return thetaPickflag;
}

bool SGSelection::isTorsionPick() const
{
  return phyPickflag;
}

void SGSelection::getPickCoords(float& x, float& y, float& z) const
{
  x = xyz_pick[0];
  y = xyz_pick[1];
  z = xyz_pick[2];
}

void SGSelection::getPickPosition(int& x, int& y) const
{
  x = pos_pick[0];
  y = pos_pick[1];
}

SoNode *SGSelection::getSelectedNode() const
{
  return lll_node;
}

/**
 * Move selection to SGFragment m_atom/m_bond HighLight vectors.
 *
 * Selection order is preserved for first 4 atoms (needed for torsions).
 * If more are selected, the selection list is in arbitrary order.
 * Bond selection order is NOT maintained.
 * If this becomes too limited, we could insert hash sets to maintain order.
 *
 * @return true if something is selected.
 */
bool SGSelection::readSelection(SGFragment *sgfrag)
{
  bool maintainOrder = true;
  static const int orderThreshHold = 4;   // above this, its unordered
  int numSelected = 0;
  int32_t atomStart, atomEnd, theAtom;
  int numAtomLoops, atomLoop, nDisplay;
  ChemDisplayPath* chemDisplayPath;

  // Stuff for maintaining selection order
  static vector<int> lastAtomSel;
  static vector<int> newAtomSel;

  lastAtomSel.clear();
  newAtomSel.clear();
  sgfrag->m_bondHighLight.clear() ;

  // the number of paths in the appropriate selection list
  int  length = getNumDisplaysSelected();

  // do we need to clear the atom highlight vector?
  // all display styles must be free of highlights
  bool clear = false ;
  for(int t=0; t<length; t++){
    if (getDisplayPath(t)->getAtomIndex().getNum() != 0){
      clear = false ;
      break ;
    } else {
      clear = true ;
    }
  }
  if (clear) {
     sgfrag->m_atomHighLight.clear() ;
  }


  // optimization purposes.  If we have less than 5 atoms we want to keep
  // them in order.  Otherewise, wipe the list out and replace it.
  for (nDisplay=0; nDisplay<length; nDisplay++) {

    chemDisplayPath = getDisplayPath(nDisplay);
    const MFVec2i &aI = chemDisplayPath->getAtomIndex();
    numAtomLoops = aI.getNum();

    for (atomLoop = 0; atomLoop < numAtomLoops; atomLoop++) {

      aI[atomLoop].getValue(atomStart, atomEnd);

      if (atomEnd == CHEM_DISPLAY_USE_REST_OF_ATOMS) {
        atomEnd = sgfrag->getNumberOfAtoms();
      } else {
        atomEnd += atomStart;
      }

      numSelected += atomEnd - atomStart;

      if (numSelected > orderThreshHold) {
        maintainOrder = FALSE;
        break;
      }
    }
  }


  // ==========================================
  //      Loop through each display style
  // ==========================================
  for (nDisplay=0; nDisplay<length; nDisplay++) {

    // ==========================================
    //            Atom HighLights
    // ==========================================
    chemDisplayPath = getDisplayPath(nDisplay);
    const MFVec2i &atomIndex = chemDisplayPath->getAtomIndex();

    // This loop just calculates the number of selections so we can
    // decide if we want to maintain order or not.  This is all for
    // performance.
    numAtomLoops = atomIndex.getNum();
    // cout << "numAtomLoops: " << numAtomLoops << endl ;

    if (numAtomLoops != 0 )  // skip atoms not in this ChemDisplay
        {
          if (maintainOrder) {
            lastAtomSel = sgfrag->m_atomHighLight;
          } else {
            // cout << "clearing atom list" << endl ;
            sgfrag->m_atomHighLight.clear();
          }
          // Here we actually build the array of highlighted atoms.  Its
          // complicated because we may or may not choose to maintain order.
          // If we do, we have to remove atoms to as they are removed
          // from the selection list.
          for (atomLoop = 0; atomLoop < numAtomLoops; atomLoop++) {
            atomIndex[atomLoop].getValue(atomStart, atomEnd);
            if (atomEnd == CHEM_DISPLAY_USE_REST_OF_ATOMS) {
              atomEnd = sgfrag->getNumberOfAtoms();
            } else {
              atomEnd += atomStart;
            }
            for (theAtom = atomStart; theAtom < atomEnd; theAtom++) {
              int at = (int)sgfrag->getAtomId(theAtom);
              if (maintainOrder) {
                vector<int>::iterator it;
                it = find(lastAtomSel.begin(), lastAtomSel.end(), at);
                //cout << "selection index " << at << endl;
                if (it == lastAtomSel.end())
                  sgfrag->m_atomHighLight.push_back(at);
                newAtomSel.push_back(at);
              } else {
                sgfrag->m_atomHighLight.push_back(at);
              }
            }
          }

          // Now remove any that were removed from the selection
          if (maintainOrder) {
            for (int idx=sgfrag->m_atomHighLight.size()-1; idx>=0; idx--)
                {
                  vector<int>::iterator it;
                  it = find(newAtomSel.begin(), newAtomSel.end(),
                            sgfrag->m_atomHighLight[idx]);
                  if (it == newAtomSel.end()) {
                    it = sgfrag->m_atomHighLight.begin();
                    it += idx;
                    sgfrag->m_atomHighLight.erase(it);
                  }
                }
          }
        } // end if for numAtomLoops

    // ==========================================
    //            Bond HighLights
    // ==========================================
    // Now look at selected bonds - we don't order these
    const MFVec2i &bondIndex =
      getDisplayPath(nDisplay)->getBondIndex();
    int32_t bondStart, bondEnd, theBond;
    int numBondLoops = (bondIndex).getNum();
    for (int bondLoop = 0; bondLoop < numBondLoops; bondLoop++)
        {
          (bondIndex)[bondLoop].getValue(bondStart, bondEnd);

          if (bondEnd == CHEM_DISPLAY_USE_REST_OF_BONDS) {
            bondEnd = sgfrag->getNumberOfBonds();
          }
          else {
            bondEnd += bondStart;
          }

          for (theBond = bondStart; theBond < bondEnd; theBond++) {
            sgfrag->m_bondHighLight.push_back(theBond);
          }
        }


  }

  return (sgfrag->m_bondHighLight.size() > 0 ||
          sgfrag->m_atomHighLight.size() > 0);

}



/**
 * A specialized method to read double click atom selections.
 * This method is stripped down from a full ChemDisplayPath reader
 * and makes some assumptions.
 * so don't use it for general purpose selection.
 */
void SGSelection::readAtomSelection(const ChemDisplayPath& cdp,
                                    vector<int>& atoms)
{
  const MFVec2i &atomIndex = cdp.getAtomIndex();

  int numAtomLoops = atomIndex.getNum();
  int32_t atomStart, atomEnd, theAtom;

  if (numAtomLoops != 0 )
      {
        // Here we actually build the array of highlighted atoms.  Its
        // complicated because we may or may not choose to maintain order.
        // If we do, we have to remove atoms to as they are removed
        // from the selection list.
        for (int atomLoop = 0; atomLoop < numAtomLoops; atomLoop++) {
          atomIndex[atomLoop].getValue(atomStart, atomEnd);
          if (atomEnd == CHEM_DISPLAY_USE_REST_OF_ATOMS) {
            cerr << "Warning - unexpected condition readAtomSelection" << endl;
          } else {
            atomEnd += atomStart;
            for (theAtom = atomStart; theAtom < atomEnd; theAtom++) {
              atoms.push_back(theAtom);
            }
          }
        }
      }
}



/**
 * Reads lasso selection.
 * Extracted and saved from builder code but not currently used.
 * @return true if anything is selected.
 */
bool SGSelection::readLassoSelection(SGFragment *sgfrag)
{
  if (!shiftActive()) {
    sgfrag->m_bondHighLight.clear() ;
    sgfrag->m_atomHighLight.clear() ;
  }

  int  length = getNumDisplaysSelected();

  //for each display....  why we have to go through each display, I don't
  //know, but we do.  This way, we don't worry about order and just dump
  //all selected atoms and bonds into the highLight vectors
  for ( int d = 0; d < length; d++)
      {
        //get atoms selected
        const MFVec2i &atoms = getDisplayPath(d)->getAtomIndex();
        int32_t atomStart, atomEnd, theAtom;
        int numAtomLoops = (atoms).getNum();
        for (int atomLoop = 0; atomLoop < numAtomLoops; atomLoop++)
            {
              (atoms)[atomLoop].getValue(atomStart, atomEnd);
              atomEnd += atomStart;

              for (theAtom = atomStart; theAtom < atomEnd; theAtom++)
                  {
                    sgfrag->m_atomHighLight.push_back(theAtom);

                  }
            }


        // get bonds selected
        const MFVec2i &bonds = getDisplayPath(d)->getBondIndex();
        int32_t bondStart, bondEnd, theBond;
        int numBondLoops = (bonds).getNum();
        for (int bondLoop = 0; bondLoop < numBondLoops; bondLoop++)
            {
              (bonds)[bondLoop].getValue(bondStart, bondEnd);
              bondEnd += bondStart;

              for (theBond = bondStart; theBond < bondEnd; theBond++)
                  {
                    sgfrag->m_bondHighLight.push_back(theBond);

                  }
            }

      }

  return (sgfrag->m_bondHighLight.size() > 0 ||
          sgfrag->m_atomHighLight.size() > 0);
}


/**
 * Cancels processing of motion.
 * Motion is a process that starts with mouse down and ends with
 * mouse up.  However there are cases where the up event is not
 * registered at this level so we need others to be able to stop
 * the motion processing.
 * Example: hitting escape key to toggle viewer mode.
 */
void SGSelection::stopProcessing2DMotion()
{
  p_inMotion = false;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Save the currentEvent in case its needed and invokes reallyHandleEvent
//    to actually handle the event.
//
//     2/10/04 Added support for double click.  The doubleclick atom
//      must be captured when the event happens.  Otherwise if you
//      have the shift key down while doing a double click, ChemSelection
//      will use its toggle policy to cancel out the select and there will
//      be no way to find the atom.  Added to support selecting of residues
//      via double click.
//
// Use: protected
//
////////////////////////////////////////////////////////////////////////
void SGSelection::handleEvent(SoHandleEventAction *action)
{
  clearRightClick();

  ChemPath *pickPath = NULL;
  ChemPath *mouseUpPickPath = NULL;
  SoPath * p = NULL;

  const SoEvent *event = action->getEvent();


  // Do what ChemSel has to do 
  // The new exception is that we don't pass the event on if we are doing 2d drag
  // movement of atoms because this causes selection cancel which is very annoying.
  if (!p_inMotion) {
     ChemSelection::handleEvent(action);
  }


  // lassoActive is defined in ChemSelection
  if (lassoActive)  alertLasso = true;

// ----------------------------------------------------------------------------
//
//NOTE: 1/19/09 Key events currently NOT being passed through to here.
//      Not sure why - sowxrender code seems to process them
//

  if (SoKeyboardEvent::isKeyPressEvent(event, SoKeyboardEvent::LEFT_SHIFT) ||
      SoKeyboardEvent::isKeyPressEvent(event, SoKeyboardEvent::RIGHT_SHIFT)) {
    p_shiftActive = true ;
  }

  if (SoKeyboardEvent::isKeyReleaseEvent(event,SoKeyboardEvent::RIGHT_SHIFT) ||
      SoKeyboardEvent::isKeyReleaseEvent(event,SoKeyboardEvent::LEFT_SHIFT)) {
    p_shiftActive = false ;
  }
// -----------------------------------------------------------------------------

  if (viewer()->getSelectModeDrag() && p_isdown) {
     // If we have recorded that down was pressed (but not up yet)
     // then if some time has elapsed and we aren't already in 2d move mode,
     // go into 2d move mode.
     unsigned long time = event->getTime().getMsecValue();
     if (!p_inMotion) {
        // The delta time used here is subject to fiddling
        if (p_down > 0 && time - p_down > 333) {
           //cout << "pdown" << p_down << "/" << time - p_down << endl;
           //start motion

           p_motionButton = 1;
           const SoMouseButtonEvent *be = (const SoMouseButtonEvent *) event;
           const SbViewVolume &vv =
              SoViewVolumeElement::get(action->getState());
           processMotion(vv, be, MotionData::START);
           p_inMotion = true;
        }
     }
  }

  // Process and notify of motion events
  if (p_inMotion ) {
    // NOTE: for some reason, I am able to cast to a MouseButtonEvent
    // by if I check the type, its only "Event" which is an abstract
    // class I think.  
    // So anyway, this must be outside the if statement below to work.
    const SoMouseButtonEvent *be = (const SoMouseButtonEvent *) event;
    const SbViewVolume &vv = SoViewVolumeElement::get(action->getState());
    processMotion(vv, be, MotionData::CONTINUE);
    //        cout << "continue " << endl;
  }



  if (event->isOfType(SoMouseButtonEvent::getClassTypeId())) {

    const SoMouseButtonEvent *be = (const SoMouseButtonEvent *) event;


    //////////////////////////////////////////
    // BUTTON1
    /////////////////////////////////////////
    if (be->getButton() == SoMouseButtonEvent::BUTTON1) {
      const SoPickedPoint *pickedPoint = action->getPickedPoint();
      pickPath = NULL;
      p = NULL;

      // Get the pick path - set the Path
      if (pickedPoint) {
        p = pickedPoint->getPath();
        pickPath = processPickedPoint(pickedPoint);
        if (pickPath != (ChemDisplayPath*)0)  pickPath->ref();
      }

      // button 1 down 
      if (be->getState() == SoButtonEvent::DOWN ) {

         p_isdown = true;
         setDoubleClick( false );
         p_doubleClickAtom = -1;

         // Start the atom drag timer so long as the user hasn't picked
         // some other draggable object.
         // capture start time for down event related to 2d motion
         if (p) {
            string tail = p->getTail()->getTypeId().getName().getString();
            if (tail != "AtomRTDragger" && tail != "AtomRotDragger") {
               p_down = event->getTime().getMsecValue();
            }
         }

         alertLasso = false;
         mouseDownPickPathII = copyFromThis(pickPath);
         if (mouseDownPickPathII) 
         {
            if (mouseDownPickPathII->isOfType(ChemDisplayPath::getClassTypeId())) {
               // keep track of double-clicks.
               //cout << time << " " << p_lastClickUpTime << endl;
               if (p_down - p_lastClickUpTime < p_doubleClickInterval) {
                  setDoubleClick( true );
                  ChemDisplayPath *cdp = (ChemDisplayPath*)mouseDownPickPathII;
                  vector<int> atoms;
                  readAtomSelection(*cdp, atoms);
                  if (atoms.size() > 0) {
                     p_doubleClickAtom = atoms[0];
                  }
               }
               p_lastClickUpTime = p_down;
            }
            mouseDownPickPathII->ref();
            lassoPickflag = false ;


         }
         else
            lassoPickflag = true ;
         //<cout << "lassoPickflag : " << lassoPickflag << endl ;
      }

      // button 1 up 
      if (be->getState() == SoButtonEvent::UP) {
         p_isdown = false;
         p_down = 0;

         mouseUpPickPath = copyFromThis(pickPath);
         if (p_inMotion) {
            //cout << "stop " << endl;
            const SbViewVolume &vv=SoViewVolumeElement::get(action->getState());
            processMotion(vv, be, MotionData::FINISH);
            p_inMotion = false;

            // Reset things that didn't get reset by skipping other selection
            if (mouseDownPickPathII) {
               mouseDownPickPathII->unref();
                  mouseDownPickPathII = 0;
            }
         } else {

            if (!mouseDownPickPathII && !mouseUpPickPath) {

               if (!alertLasso) {

                  // Free space pick
                  if (!p) {
                     float x,y,z;

                     // get position in real coordinates 

                     const SbViewVolume &vv =
                        SoViewVolumeElement::get(action->getState());
                     const SbVec2f & posit =    be->getNormalizedPosition(viewer()->getViewportRegion());                    


                     vv.getPlanePoint(vv.getNearDist()+vv.getDepth()/2 , posit).getValue(x,y,z);


                     // Set the window pixel location of the cursor when the 
                     // event occurred.  The position is relative to the 
                     // lower left corner of the window in which the 
                     // event occurred.
#if (!defined(INSTALL) && defined(DEBUG))
#endif

                     // store for finishCB
                     xyz_pick[0] = x;
                     xyz_pick[1] = y;
                     xyz_pick[2] = z;
                     freePickflag = true;
                     // Invoke commands (free pick)
                     //cout << "freePickflag: " << freePickflag << endl ;
                     if (finishCBList) finishCBList->invokeCallbacks(this);
                     freePickflag = false;
                     //cout << "after Pickflag: " << freePickflag << endl ;

                     // One of the measures
                  } else {
                     string name = p->getTail()->getName().getString();
                     lll_node = pickedPoint->getPath()->getTail();
                     be->getPosition().getValue(pos_pick[0],pos_pick[1]);
                     pickedPoint->getPoint().getValue(xyz_pick[0], xyz_pick[1],xyz_pick[2]);

                     if (name == "Angle") {

                        thetaPickflag = true;         
                        if (finishCBList) finishCBList->invokeCallbacks(this);
                        thetaPickflag = false;


                     } else if (name == "Distance") {

                        distPickflag = true;         
                        if (finishCBList) finishCBList->invokeCallbacks(this);
                        distPickflag = false;


                     } else if (name == "Torsion") {


                        phyPickflag = true;         
                        if (finishCBList) finishCBList->invokeCallbacks(this);
                        phyPickflag = false;

                     }

                  }
               } else {
                  alertLasso = false;
               } 
            }
         }

      }





    } else if (be->getButton() == SoMouseButtonEvent::BUTTON3) {
      // Add processing for Right Button up selection.
      // We don't actually select it though
      if (be->getState() == SoButtonEvent::UP ) {
        const SoPickedPoint *pickedPoint = action->getPickedPoint();
        pickPath = NULL;
        p = NULL;

        // Get the pick path - set the Path
        if (pickedPoint) {
          pickPath = processPickedPoint(pickedPoint);
          if (pickPath && 
              pickPath->isOfType(ChemDisplayPath::getClassTypeId())) {
            ChemDisplayPath *cdp = (ChemDisplayPath*)pickPath;
            vector<int> atoms;
            readAtomSelection(*cdp, atoms);
            if (atoms.size() > 0) {
              setRightClickAtom(atoms[atoms.size()-1]);
              if (finishCBList) finishCBList->invokeCallbacks(this);
            }
          }
        }
      }
    }

  } else {

    // Something else - like motion etc
    ChemSelection::handleEvent(action);
    return;
  }

}

/**
 * Process mouse motion events and report to listeners.
 * Currently processes movements in the plane paralle to the screen.
 * Probably should support in/out movement as well as rotation.
 */
void SGSelection::processMotion(const SbViewVolume& vv,
                                const SoMouseButtonEvent *be,
                                MotionData::MOTION_STATE state)
{
  // Make sure somebody cares before bothering to process it
  if (p_motionListeners.size() > 0) {

    float mx, my, mz;
    getRealCoords(vv, be, mx, my, mz);
    //cout << "real coods " << mx << " " << my << " " << mz <<endl;
    //cout << "start coods " << p_motionx << " " << p_motiony << " " << p_motionz <<endl;
    //cout << "delta coords" << mx - p_motionx << " " << 
    //   my - p_motiony << " " << 
    //   mz - p_motionz << " " <<  endl;

    // The start even should have no displacement
    float deltax = 0.;
    float deltay = 0.;
    float deltaz = 0.;
    if (state != MotionData::START) {
      deltax = mx - p_motionx;
      deltay = my - p_motiony;
      deltaz = mz - p_motionz;
    }
    MotionData data(p_motionButton, deltax, deltay, deltaz,
                    be->wasShiftDown(), be->wasCtrlDown(), state);
    p_motionx = mx;
    p_motiony = my;
    p_motionz = mz;
    for (int idx=0; idx<p_motionListeners.size(); idx++) {
      p_motionListeners[idx]->motionChanged(data);
    }
  }
}


void SGSelection::getRealCoords(const SbViewVolume& vv,
                                const SoMouseButtonEvent *event,
                                float& x, float& y, float& z)
{
  const SbVec2f & posit = event->getNormalizedPosition(viewer()->getViewportRegion());                    


  vv.getPlanePoint(vv.getNearDist()+vv.getDepth()/2 , posit).getValue(x,y,z);

}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Toggle on/off tracking the location of the cursor.
//
// Use: public
//
////////////////////////////////////////////////////////////////////////
void SGSelection::setTrackLocation(SbBool track)
{
  mouse3Active = track;
}

////////////////////////////////////////////////////////////////////////
//  
// Description:
//    Methods for setting callback functions.
//
// Use: public
//  
////////////////////////////////////////////////////////////////////////
void
SGSelection::addMouseDownCallback(SGSelectionPathCB *f, void *userData)
{
  if (mouseDownCBList == NULL) {
    mouseDownCBList = new SoCallbackList;
  }
  mouseDownCBList->addCallback((SoCallbackListCB *)f, userData);
}

void
SGSelection::removeMouseDownCallback(SGSelectionPathCB *f, void *userData)
{
  if (mouseDownCBList != NULL) {
    mouseDownCBList->removeCallback((SoCallbackListCB *)f, userData);
  }
}

void
SGSelection::addMouseUpCallback(SGSelectionPathCB *f, void *userData)
{
  if (mouseUpCBList == NULL) {
    mouseUpCBList = new SoCallbackList;
  }
  mouseUpCBList->addCallback((SoCallbackListCB *)f, userData);
}

void
SGSelection::removeMouseUpCallback(SGSelectionPathCB *f, void *userData)
{
  if (mouseUpCBList != NULL) {
    mouseUpCBList->removeCallback((SoCallbackListCB *)f, userData);
  }
}

void
SGSelection::addMouseMoveCallback(SGSelectionPathCB *f, void *userData)
{
  if (mouseMoveCBList == NULL) {
    mouseMoveCBList = new SoCallbackList;
  }
  mouseMoveCBList->addCallback((SoCallbackListCB *)f, userData);
}

void
SGSelection::removeMouseMoveCallback(SGSelectionPathCB *f, void *userData)
{
  if (mouseMoveCBList != NULL) {
    mouseMoveCBList->removeCallback((SoCallbackListCB *)f, userData);
  }
}




////////////////////////////////////////////////////////////////////////
//
// Description:
//    Needed to get viewport for measure popups.  Gotta be a better way!
//
// Use: public
//
////////////////////////////////////////////////////////////////////////
SGViewer * SGSelection::viewer()
{
  return p_viewer;
}

void SGSelection::setViewer(SGViewer * vi)
{
  p_viewer= vi;
}

SbBool SGSelection::getLassoActive()
{
  return lassoActive ;
}

SbBool SGSelection::shiftActive()
{
  return p_shiftActive ;
}



/**
 * Clears right click state.
 */
void SGSelection::clearRightClick()
{
  setRightClickAtom(-1);
}



/**
 * Set the right click atom.
 * @param atom  atom index or -1 to clear state
 */
void SGSelection::setRightClickAtom(int atom)
{
  p_rightClickAtom = atom;
}



/**
 * Query whether last button event was a right click event.
 *
 * A right click is defined to be mouse up over an atom regardless
 * of the location of mouse down.
 * The purpose of right click is to detect a SINGLE atom right click to
 * allow editing of that atom.  There is no concept of multiple selection
 * at this time.
 *
 * Use getRightClickAtom() to find out which atom was right clicked.
 *
 * @return true if the last event of interest to the application
 *              was a right click.
 */
bool SGSelection::isRightClick() const
{
  return p_rightClickAtom >= 0;
}



/**
 * Get the atom associated with a right click event.
 * @see isRightClick
 *
 * @return -1 if no right click event or the atom number.
 */
int SGSelection::getRightClickAtom() const
{
  return p_rightClickAtom;
}



/**
 * Query whether last button event was a double click event.
 * A double click event pertains only to mouse button 1 at this time.
 *
 * @return true if the last event of interest to the application
 *              was a double click.
 */
bool SGSelection::isDoubleClick() const
{
  return p_doubleClick;
}



/**
 * Set whether there is a current double click action. 
 * @param value  true if is double click, false to clear double click.
 */
void SGSelection::setDoubleClick( bool value)
{
  p_doubleClick = value;
}



/**
 * Retrieve the atom associated with a double click action.
 *
 * @return atom number or -1 if there was no double click.
 */
int SGSelection::getDoubleClickAtom() const
{
  return p_doubleClickAtom ;
}



/**
 * Retrieve the number of milliseconds used for double click action.
 *
 * @return interval in milliseconds
 */
int SGSelection::getDoubleClickInterval() const
{
  return p_doubleClickInterval ;
}

/**
 * Specify the number of milliseconds to use for double click action.
 *
 * @param value milliseconds
 */
void SGSelection::setDoubleClickInterval(int value)
{
  p_doubleClickInterval = value;
}
