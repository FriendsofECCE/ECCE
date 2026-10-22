#ifndef SGSELECTION_H
#define SGSELECTION_H

#include <iostream>
using std::ostream;

#include <vector>
using std::vector;

#include "inv/events/SoMouseButtonEvent.H"
#include "inv/elements/SoViewVolumeElement.H"

#include "inv/ChemKit/ChemSelection.H"

#include "wxviz/MotionData.H"

class SGViewer;
class SGFragment;
class MotionListener;

// Callback function prototypes
typedef void SGSelectionPathCB(void *userData, ChemPath *path);


/**
 * This class does handling of scene graph selection and maps it to
 * internal scene graph selection structures.  
 *
 * SGSelection reports several kinds of selection:
 *   normal left button selection
 *   left button double click selection
 *   right click selection (single atom)
 *   free space clicks
 *   selection of the various measures.
 *
 * See ChemSelection for rules on how modifier keys work with normal 
 * left button selection.
 *
 * TODO It would be nice to move the selection arrays in SGFragment into 
 * this class and set them in handleEvent but that would require lots
 * of code cleanup so the less ambitious goal is to share more code.
 *
 * Clients get callbacks by adding code like:
 * esel->addFinishCallback(&MyTool::selectionChangeCB, this);
 * to their app.
 */
class SGSelection : public ChemSelection {

  SO_NODE_HEADER(SGSelection);

public:
  // Constructor.  nChildren is the approximate number of children.
  SGSelection();
  SGSelection(int nChildren);

  // Sets the toggle as to whether to track the cursor location and
  // report what is under the mouse
  void setTrackLocation(SbBool track);

  void setDoubleClickInterval(int millsec);

  // Retrieve the number of milliseconds used for double click
  int  getDoubleClickInterval() const;

  bool isDoubleClick() const;

  int getDoubleClickAtom() const;

  bool isRightClick() const;

  int getRightClickAtom() const;

  // This adds to the list of callbacks invoked on a mouse down event
  void addMouseDownCallback(SGSelectionPathCB *f, void *userData = NULL);
  void removeMouseDownCallback(SGSelectionPathCB *f,
                               void *userData = NULL);

  // This adds to the list of callbacks invoked on a mouse up event
  void addMouseUpCallback(SGSelectionPathCB *f, void *userData = NULL);
  void removeMouseUpCallback(SGSelectionPathCB *f, void *userData = NULL);


  // This adds to the list of callbacks invoked on a mouse move event
  void addMouseMoveCallback(SGSelectionPathCB *f, void *userData = NULL);
  void removeMouseMoveCallback(SGSelectionPathCB *f, void *userData = NULL);

  void addMotionListener(MotionListener *l);

  bool readSelection(SGFragment *frag);

  /* Not used but saved in case of value. */
  bool readLassoSelection(SGFragment *frag);

  /* true if a select was done on empty space. */
  bool isFreePick() const;

  /* true if selection is lasso selection. */
  bool isLassoPick() const;

  /* true if a pick was done on a distance measure object. */
  bool isDistancePick() const;

  /* true if a pick was done on a angle measure object. */
  bool isAnglePick() const;

  /* true if a pick was done on a torsion measure object. */
  bool isTorsionPick() const;

  /* pick position in 3d space */
  void getPickCoords(float& x, float& y, float& z) const;

  /* pick position in 2d (window space) */
  void getPickPosition(int& x, int& y) const;

  /* Returns Measure node for measure selections. */
  SoNode *getSelectedNode() const;

  void stopProcessing2DMotion();

  /* On (Builder Select mode): a press on empty space drags a rubber-band
     box that selects on release, a press on a selected atom moves the
     selection after the drag delay, a press on any other atom only
     selects it.  Off: lassoType and the viewer's select-mode-drag flag
     decide as before. */
  void setBoxSelect(bool on);


  void setViewer(SGViewer * );
  SGViewer * viewer();

  SbBool              getLassoActive() ;
  SbBool 		shiftActive();

  friend ostream& operator<< (ostream& os, const SGSelection&); 


  SoINTERNAL public:
  static void         initClass();

protected:
  // Traversal routine for SoHandleEventAction - this will call
  // action->setHandled() if the event is handled
  virtual void        handleEvent(SoHandleEventAction *action);
  void  readAtomSelection(const ChemDisplayPath& cdp, 
                          vector<int>& pickedAtoms);

  ~SGSelection();

  void setDoubleClick(bool value);
  void setRightClickAtom(int atom);
  void clearRightClick();

  // Callbacks for mouseDown, mouseUp and mouseMove
  SoCallbackList *mouseDownCBList;
  SoCallbackList *mouseUpCBList;
  SoCallbackList *mouseMoveCBList;

  float xyz_pick[3];
  short pos_pick[2];
  SoNode * lll_node;
  SbBool freePickflag;
  SbBool distPickflag;
  SbBool thetaPickflag;
  SbBool phyPickflag;
  SbBool alertLasso;
  SbBool lassoPickflag;




  void getRealCoords(const SbViewVolume& vv,
                     const SoMouseButtonEvent *event,
                     float& x, float& y, float& z);
  void processMotion(const SbViewVolume& vv,
                     const SoMouseButtonEvent *be, 
                     MotionData::MOTION_STATE state);

private:
  void constructorCommon();
  ChemPath * mouseDownPickPathII;
  SbBool mouse3Active;
  SbBool p_shiftActive;
  SGViewer * p_viewer;

  /* These used to implement drag to move effect.  A timer is used
     to decide between move and regular selection processing */
  bool p_isdown;
  unsigned long p_down;

  bool p_boxSelect;
  /** The press that is down began on a selected atom (may move it). */
  bool p_dragStarts;
  /** The press that is down is drawing a box. */
  bool p_banding;
  short p_bandX0, p_bandY0;

  void chooseGesture(SoHandleEventAction *action);
  bool atomSelected(int atom) const;
  void showBand(bool on, short x1 = 0, short y1 = 0);

  /** Holds starting x point for current motion event in real coords. */
  float               p_motionx;
  /** Holds starting y point for current motion event in real coords. */
  float               p_motiony;
  /** Holds starting z point for current motion event in real coords. */
  float               p_motionz;
  /** Keeps track of whether we are in motion or not. */
  bool                p_inMotion;
  /** Which button is motion on?  1,2, or 3 */
  int                 p_motionButton;

  /** Registered MotionListeners. */
  vector<MotionListener*> p_motionListeners;

  int   p_doubleClickInterval;
  bool  p_doubleClick;
  int   p_doubleClickAtom;
  unsigned long p_lastClickUpTime;
  int   p_rightClickAtom;
};

#endif
