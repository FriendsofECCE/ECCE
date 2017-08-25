/*
 * ChemSelection.h
 *
 *	This file defines the ChemSelection node class.
 *
 * Copyright 1996, 1997, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED
 *
 * UNPUBLISHED -- Rights reserved under the copyright laws of the United
 * States.   Use of a copyright notice is precautionary only and does not
 * imply publication or disclosure.
 *
 * U.S. GOVERNMENT RESTRICTED RIGHTS LEGEND:
 * Use, duplication or disclosure by the Government is subject to restrictions
 * as set forth in FAR 52.227.19(c)(2) or subparagraph (c)(1)(ii) of the Rights
 * in Technical Data and Computer Software clause at DFARS 252.227-7013 and/or
 * in similar or successor clauses in the FAR, or the DOD or NASA FAR
 * Supplement.  Contractor/manufacturer is Silicon Graphics, Inc.,
 * 2011 N. Shoreline Blvd. Mountain View, CA 94039-7311.
 *
 * THE CONTENT OF THIS WORK CONTAINS CONFIDENTIAL AND PROPRIETARY
 * INFORMATION OF SILICON GRAPHICS, INC. ANY DUPLICATION, MODIFICATION,
 * DISTRIBUTION, OR DISCLOSURE IN ANY FORM, IN WHOLE, OR IN PART, IS STRICTLY
 * PROHIBITED WITHOUT THE PRIOR EXPRESS WRITTEN PERMISSION OF SILICON
 * GRAPHICS, INC.
 */
/**************************************************************************\
 *
 * OpenMOIV - C++ library for molecular visualization using Inventor.
 * Copyright (C) 2001-2003 Universitat Pompeu Fabra - Barcelona (Spain)
 * 
 * Developers: Interactive Technology Group (GTI)
 * Team: Josep Blat, Eduard Gonzalez, Sergi Gonzalez,
 *       Daniel Soto, Alejandro Ramirez, Oscar Civit.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details (see the file 
 * LICENSE.LGPL at the root directory).
 *
 * REMARK: This library is a derived product.
 *         You need also to accept all other applicable licenses.
 * 
 * Homepage: http://www.tecn.upf.es/openMOIV/
 * Contact:  openmoiv@upf.es
 *
\**************************************************************************/

#ident "$Revision: 22148 $"

#ifndef  __CHEM_SELECTION_H__
#define  __CHEM_SELECTION_H__


#include "inv/nodes/SoIndexedLineSet.H"
#include "inv/nodes/SoLineSet.H"

#include "inv/fields/SoSFBitMask.H"
#include "inv/fields/SoSFBool.H"
#include "inv/fields/SoSFColor.H"
#include "inv/fields/SoSFName.H"
#include "inv/fields/SoSFNode.H"
#include "inv/fields/SoSFUShort.H"

#include "inv/misc/SoCallbackList.H"

#include "inv/nodes/SoOrthographicCamera.H"
#include "inv/nodes/SoSelection.H"
#include "inv/nodes/SoSeparator.H"
#include "inv/nodes/SoSubNode.H"

#include "inv/ChemKit/ChemBBoxAction.H"
#include "inv/ChemKit/ChemDisplayPathList.H"
#include "inv/ChemKit/ChemLabelPathList.H"
#include "inv/ChemKit/ChemMonitorPathList.H"
#include "inv/ChemKit/ChemkitBasic.H"


class SoColorIndex;
class SoComplexity;
class SoCoordinate3;
class SoDrawStyle;
class SoEvent;
class SoFieldSensor;
class SoFont;
class SoGetMatrixAction;
class SoHandleEventAction;
class SoMaterial;
class SoPickAction;
class SoPickedPoint;
class SoPickedPointList;
class SoSearchAction;
class SoSensor;
class SoSphere;
class SoSwitch;
class SoText2;
class SoTransform;
class SoTypeList;
#ifndef __nogl
class SoXtGLWidget;
#endif

class ChemBitmapFontCache;
class ChemAtomBBox;
class ChemBondBBox;
class ChemLabelBBox;
class ChemDisplayPath;
class ChemDisplayPathList;
class ChemLabelPath;
class ChemLabelPathList;
class ChemMonitorPath;
class ChemMonitorPathList;
class ChemPath;
class ChemSelection;

// Callback function prototypes
typedef void ChemSelectionPathCB(void *userData, ChemPath *path);
typedef void ChemSelectionClassCB(void *userData, ChemSelection *sel);
typedef ChemPath * ChemSelectionPickCB(void *userData,
                                       const SoPickedPoint *pick);

//////////////////////////////////////////////////////////////////////////////
//
//  Class: ChemSelection
//
//  Selection group node: Manages selection lists from picks of its children.
//
//////////////////////////////////////////////////////////////////////////////

class CHEMKIT_DLL_API ChemSelection : public SoSeparator {

    SO_NODE_HEADER(ChemSelection);

  public:
    // Constructor. nChildren is the approximate number of children.
    ChemSelection();
    ChemSelection(int nChildren);

    //
    // Enums
    // -------------------------

    // Default selection toggle policy is ChemSelection::SHIFT.
    enum TogglePolicy {
        // left mouse pick on object clears selection, then selects object.
        // left mouse pick on nothing clears selection.
        // only one object may be selected at a time. 
        SINGLE,
        
        // left mouse pick on object toggles its selection status.
        // left mouse pick on nothing does nothing.
        // multiple objects may be selected.
        TOGGLE,
        
        // when shift key is down, selection policy is TOGGLE.
        // when shift key is up, selection policy is SINGLE.
        // multiple objects may be selected.
        SHIFT
    };

    // Whether lassoing is used or not, and what type of lassoing.
    enum LassoType {
        NOLASSO,

        // Left mouse down starts picking operation.  As mouse is
        // dragged, the lasso outline is drawn.  Left mouse up initiates
        // the lasso pick.
        LASSO,

        // Left mouse down starts a drag selection.  As mouse is moved,
        // a rectangle is drawn.  Left mouse up initiates the pick.
        DRAGGER
    };

    // These are the items which can be selected.
    enum ChemPart {
        ATOMS = 0x01,
        BONDS = 0x02,
        ATOMLABELS = 0x04,
        BONDLABELS = 0x08,
        CHEMLABELS = 0x10,
        CHEMMONITORS = 0x20,
// --> residue selection
				RESIDUES = 0x40,
				RESIDUELABELS = 0x80,
				ALL = 0xFF
				//ALL = 0x3F,
// <-- residue selection
    };

    // When lassoing, this determines if the item has been selected.
    enum LassoPolicy {
        CENTER,
        THRU_BBOX,
        ENTIRE_BBOX
    };

    enum RadiusSelectSphereDrawStyle {
        FILLED,
        LINES,
        POINTS
    };

    //
    // Fields
    // -------------------------

    SoSFBitMask   parts;
    SoSFEnum      togglePolicy;
    SoSFEnum      atomLassoPolicy;
    SoSFEnum      bondLassoPolicy;
    SoSFEnum      atomLabelLassoPolicy;
    SoSFEnum      bondLabelLassoPolicy;
    SoSFEnum      chemLabelLassoPolicy;
    SoSFEnum      chemMonitorLassoPolicy;
// --> residue selection
		SoSFEnum      residueLassoPolicy;
		SoSFEnum      residueLabelLassoPolicy;
// <-- residue selection
    SoSFBool      selectInvisibleHydrogens;

    // The following fields have to do with the drawing of the lasso
    // and the cutoff beneath which the lasso will not be in effect.
    SoSFFloat     lassoSelectCutoff;
    SoSFEnum      lassoType;
    SoSFFloat     lassoLineWidth;
    SoSFUShort    lassoLinePattern;
    SoSFUShort    lassoLineColorIndex;
    SoSFNode      lassoPoints;

    // Values for the sphere portion of the radiusSelect
    SoSFBool      radiusSelect;
    SoSFFloat     radiusSelectCutoff;
    SoSFColor     radiusSelectSphereColor;
    SoSFFloat     radiusSelectSphereTransparency;
    SoSFEnum      radiusSelectSphereDrawStyle;
    SoSFFloat     radiusSelectSphereComplexity;

    // The font color, name and size to use for the radius selection display.
    SoSFColor     radiusSelectLabelColor;
    SoSFName      radiusSelectLabelFontName;
    SoSFFloat     radiusSelectLabelFontSize;

    //
    // Methods
    // -------------------------

#ifndef __nogl
    // The following method sets the SoXtViewer being used.  It is necessary
    // to have access to the viewer in order to determine if the viewer is
    // in stereo mode or not.  Stereo may affect the lasso rendering.
    void          setViewer(SoXtGLWidget *_viewer) { viewer = _viewer; }
#endif

    // The following method returns the sceneGraph used to draw the lasso.
    // This is used in conjunction with the viewer's setOverlaySceneGraph
    // method.
    SoNode        *getSceneGraph() const { return (SoNode *)lassoRoot; }

    //
    // Child node management
    // -------------------------
    
    // Adds a child as last one in group
    void          addChild(SoNode *child);
                                       
    // Adds a child so that it becomes the one with the given index
    void          insertChild(SoNode *child, int newChildIndex);
  
    // Returns pointer to nth child node
    SoNode *      getChild(int index) const;

    // Finds index of given child within group
    int           findChild(const SoNode *child) const;

    // Returns number of children
    int           getNumChildren() const;
    
    // Removes child with given index from group
    void          removeChild(int index);
    
    // Removes first instance of given child from group
    void          removeChild(SoNode *child);

    // Removes all children from group
    void          removeAllChildren();

    // Replaces child with given index with new child
    void          replaceChild(int index, SoNode *newChild);

    // Replaces first instance of given child with new child
    void          replaceChild(SoNode *oldChild, SoNode *newChild);

    //
    // Selection list management
    // -------------------------

    // select() adds a path to the selection lists.
    // 'this' must lie in the path, and when added to the list,
    // the path will be truncated such that 'this' is the head.
    // select() makes a copy of the passed path.
    void          select(const ChemPath *path);    

    // deselect removes a path from the selection lists by path.
    void          deselect(ChemPath *path);
    
    // deselect all the paths in the selection lists (i.e. clear)
    void          deselectAll();

    // deselect all paths in the displaySelectionList
    void          deselectAllDisplays();

    // deselect all paths in the labelSelectionList
    void          deselectAllLabels();

    // deselect all paths in the monitorSelectionList
    void          deselectAllMonitors();

    // toggles a path in the selection lists - if the path is not there,
    // it is selected; if a path is there, it is deselected.
    void          toggle(const ChemPath *path);

    // merges a path into the selection lists - if the path is not there,
    // it is selected; if a path is there, it remains selected.
    void          merge(const ChemPath *path);

    // returns TRUE if the path is selected
    SbBool        isSelected(const ChemPath *path) const;
    // This convenience version of isSelected will search for the first instance
    // of the passed node under this selection node, and return whether
    // that path is selected.
    SbBool        isSelected(SoNode *node) const;

    // Get information about the displaySelectionList
    int getNumDisplaysSelected() const
        { return displaySelectionList.getLength(); }
    const ChemDisplayPathList *getDisplaySelectionList() const
        { return &displaySelectionList; }
    ChemDisplayPath *getDisplayPath(int index) const;

    // Get information about the labelSelectionList
    int getNumLabelsSelected() const
        { return labelSelectionList.getLength(); }
    const ChemLabelPathList *getLabelSelectionList() const
        { return &labelSelectionList; }
    ChemLabelPath *getLabelPath(int index) const;

    // Get information about the monitorSelectionList
    int getNumMonitorsSelected() const
        { return monitorSelectionList.getLength(); }
    const ChemMonitorPathList *getMonitorSelectionList() const
        { return &monitorSelectionList; }
    ChemMonitorPath *getMonitorPath(int index) const;

    //
    // Selection change callbacks
    // --------------------------
    //
    // Callbacks are invoked:
    // 1) when a lasso is started and stopped.
    // 2) when an object is selected or deselected.
    // 3) when the delete key is pressed and objects have been selected.
    //

    // These callbacks are invoked whenever the selection changes.
    void addChangeCallback(ChemSelectionClassCB *f, void *userData = NULL);
    void removeChangeCallback(ChemSelectionClassCB *f, void *userData = NULL);
    
    // These callbacks are invoked every time an object is selected, whether
    // it be from user interaction or from program call to select() or
    // toggle().
    // This is invoked after the object has been added to the selection lists.
    void addSelectionCallback(ChemSelectionPathCB *f, void *userData = NULL);
    void removeSelectionCallback(ChemSelectionPathCB *f, void *userData = NULL);
    
    // These callbacks are invoked every time an object is deselected, whether
    // it be from user interaction or from program call to deselect() or
    // toggle().
    // This is invoked after the object has been removed from the selection
    // list.
    void addDeselectionCallback(ChemSelectionPathCB *f, void *userData = NULL);
    void removeDeselectionCallback(ChemSelectionPathCB *f,
                                   void *userData = NULL);

    // Invoked when the user has initiated a change to the selection lists
    // by picking objects. This will be followed by invocations of the select
    // and/or deselect callbacks, finally followed by the finish callback.
    void addStartCallback(ChemSelectionClassCB *f, void *userData = NULL);
    void removeStartCallback(ChemSelectionClassCB *f, void *userData = NULL);
    
    // Invoked when the user has finished changing the selection lists
    // by picking objects. This was preceeded by an invocation of the start
    // callback, and invocations of the select and/or deselect callbacks.
    void addFinishCallback(ChemSelectionClassCB *f, void *userData = NULL);
    void removeFinishCallback(ChemSelectionClassCB *f, void *userData = NULL);

    // Invoked when the user has initiated a change to the selection lists
    // by lassoing objects. This will be followed by invocations of the select
    // and/or deselect callbacks, finally followed by the finish callback.
    void addLassoStartCallback(ChemSelectionClassCB *f, void *userData = NULL);
    void removeLassoStartCallback(ChemSelectionClassCB *f,
                                  void *userData = NULL);
    
    // Invoked when the user has finished changing the selection lists
    // by lassoing objects. This was preceeded by an invocation of the 
    // lassoStart // callback, and invocations of the select and/or 
    // deselect callbacks.
    void addLassoFinishCallback(ChemSelectionClassCB *f, void *userData = NULL);
    void removeLassoFinishCallback(ChemSelectionClassCB *f,
                                   void *userData = NULL);
    //
    // Selection interaction - selection will pick once on mouse down
    // and once on mouse up, and make sure the picks match before changing
    // the selection lists. This allows the user to pick down on an object,
    // change their mind and drag off the object, release the mouse button
    // and not affect the selection. Pass TRUE to enable this behavior.
    // Pass FALSE to disable this, meaning whatever is picked on a mouse
    // release is added to/removed from the selection lists.
    // Default is pick-matching on.
    //
    void   setPickMatching(SbBool pickTwice) { pickMatching = pickTwice; }
    SbBool isPickMatching() const { return pickMatching; }
    
    // Provided for backwards compatibility
    SbBool getPickMatching() const { return pickMatching; }
    
  SoEXTENDER public:
    virtual void GLRender(SoGLRenderAction *action);
    virtual void getBoundingBox(SoGetBoundingBoxAction *action);  
    virtual void write(SoWriteAction *action);
    virtual void GLRenderBelowPath(SoGLRenderAction *action);
    virtual void GLRenderInPath(SoGLRenderAction *action);
    virtual void GLRenderOffPath(SoGLRenderAction *action);
    virtual void pick(SoPickAction *action);

  SoINTERNAL public:

  // Internal:
    virtual void        notify(SoNotList *list);
    static void         initClass();
    
    // Returns the points which comprise the lasso.
    SoVertexProperty *getLassoPoints()
        { return ((SoVertexProperty *)lassoPoints.getValue()); }

    // Invoked when a pick has occurred and the selection node is about
    // to change the selection lists. The callback function returns the
    // path that the selection node should use when selecting and deselecting.
    // If no pick callback is registered (the default), the selection node
    // will use the path returned by SoPickedPoin::getPath().
    // The returned path need not be ref'd - selection will ref() and unref()
    // it.
    //
    // Note that a picked object may or may not be a child of the selection
    // node. A selection node will only select paths that pass through it.
    //
    //      Nothing picked - selection policy applied, traversal halts
    //      Object picked under selection - policy applied, traversal halts
    //      Object picked not under selection - event ignored, traversal
    //                                          continues
    //
    // In the case of nothing picked, the pick callback is not invoked.
    // In the other cases, the pick callback is invoked, giving it the chance
    // to decide what path gets selected and deselected.
    // 
    // Return values from the callback:
    //
    //      NULL - selection behaves as if nothing was picked (i.e. for
    //          SINGLE and SHIFT policies, this clears the selection lists)
    //
    //      path - this path will be selected/deselected according to
    //          the selection policy (it must lie under the selection node)
    //
    //      path not passing through the selection node - selection ignores
    //          this pick event and no change is made to the selection lists
    //
    //      path containing ONLY the selection node - apply the selection
    //          policy as if nothing was picked, but continue traversal
    //
    // A simple way to tell selection to ignore the pick is to return
    // an ChemPath with no nodes in it. (i.e. return new ChemPath;)
    //
    // Selection will always ref the path returned by the callback, make a
    // copy of the path, then unref the path.
    //
    void setPickFilterCallback(ChemSelectionPickCB *f,
                               void *userData = NULL, 
                               SbBool callOnlyIfSelectable = TRUE); 

  protected:
    SbBool              lassoActive;
    SbBox3f             lassoBBox;
    SbBox3f             sphereBBox;
    const SoEvent       *currentEvent;

    // Local sceneGraph for rendering the lasso into the overlay plane
    SoSeparator         *lassoRoot;

    // Camera for rendering the lasso
    SoOrthographicCamera *orthoCam;

    // Nodes we need access to for the display of the lasso
    SoColorIndex        *lassoColorIndex;
    SoDrawStyle         *lassoDrawStyle;

    // Nodes we need access to for the display of the radiusSelection
    SoSwitch            *sceneSwitch;
    SoSeparator         *sphereRoot;
    SoMaterial          *sphereMaterial;
    SoDrawStyle         *sphereDrawStyle;
    SoComplexity        *sphereComplexity;
    SoTransform         *sphereTransform;
    SoSphere            *sphere;
    SoMaterial          *labelMaterial;
    SoFont              *labelFont;
    SoTransform         *labelTransform;
    SoText2             *labelText;

    // List of paths of selected objects
    ChemDisplayPathList displaySelectionList;
    ChemLabelPathList   labelSelectionList;
    ChemMonitorPathList monitorSelectionList;

    // Selection callback lists
    SoCallbackList      *selCBList;
    SoCallbackList      *deselCBList;
    SoCallbackList      *startCBList;
    SoCallbackList      *finishCBList;
    SoCallbackList      *lassoStartCBList;
    SoCallbackList      *lassoFinishCBList;
    SoCallbackList      *changeCBList;
    
    // Pick filter is a single callback function, not a list
    ChemSelectionPickCB *pickCBFunc;
    void                *pickCBData;
    SbBool              callPickCBOnlyIfSelectable;
    
    // Mouse down picked path
    ChemPath            *mouseDownPickPath;
    SbBool              pickMatching;

    // Selection policies (shiftSelection is a combination of these)
    // invokeSelectionPolicy() is called from handleEvent() - for a customized
    // selection policy, either derive a class and write a new handleEvent()
    // method, or use an SoEventCallback node placed such that it receives
    // events before the selection node.
    void                invokeSelectionPolicy(ChemPath *path,
                                              SbBool shiftDown,
                                              SbBool ctrlDown,
                                              SbBool notify);
    void                performSingleSelection(ChemPath *path, SbBool notify);
    void                performToggleSelection(ChemPath *path, SbBool ctrlDown,
                                               SbBool notify);
    
    // This copies the path so the copy is rooted by 'this'.
    // Returns NULL if 'this' not found in the passed path.
    ChemPath            *copyFromThis(const ChemPath *path) const;
    
    // addPath() assumes the path is rooted by 'this'
    void                addPath(ChemPath *path, SbBool toggle);
    void                removeDisplayPath(int which);
    void                removeLabelPath(int which);
    void                removeMonitorPath(int which);
    
    // This locates path in the selection lists. The head of this path
    // does not necessarily have to be 'this'.
    int                 findPath(const ChemPath *path) const;

    // Traversal routine for SoHandleEventAction - this will call
    // action->setHandled() if the event is handled
    virtual void        handleEvent(SoHandleEventAction *action);

    // Returns the event being handled by the lasso selection.
    const SoEvent *     getCurrentEvent() const
        { return (lassoActive) ? NULL : currentEvent; }

    // Converts the mouse coordinates to the coordinates for the lasso.
    SbVec2f            getModelCoordinates(SoHandleEventAction *action);

    // Returns the paths to all nodes of type classTypeId.
    SoPathList *       getPathList(SoType classTypeId);

    ChemPath           *processPickedPoint(const SoPickedPoint *pp);
    virtual SbBool     readInstance(SoInput *in, unsigned short flags);

    // Destructor removes everything from the list.
    virtual ~ChemSelection();

  private:

#ifndef __nogl
    // The viewer being used.  This is required in order to determine
    // if the application is using stereo.
    SoXtGLWidget *viewer;
#endif

    // Whether to use radiusSelection or not
    SbBool useRadiusSelection;

    // Current orthocam width and height
    float orthoCamXres;
    float orthoCamYres;

    static SbVec2f ringCoords[16];

    SoFieldSensor *lassoLineWidthSensor;
    SoFieldSensor *lassoLinePatternSensor;
    SoFieldSensor *lassoLineColorIndexSensor;
    SoFieldSensor *radiusSelectSphereColorSensor;
    SoFieldSensor *radiusSelectSphereTransparencySensor;
    SoFieldSensor *radiusSelectSphereDrawStyleSensor;
    SoFieldSensor *radiusSelectSphereComplexitySensor;
    SoFieldSensor *radiusSelectLabelColorSensor;
    SoFieldSensor *radiusSelectLabelFontNameSensor;
    SoFieldSensor *radiusSelectLabelFontSizeSensor;
    static void   selectSensorCB(void *, SoSensor *);

    // Selected atom on which the radiusSelect was started.
    // This is the coordinate as returned from ChemData.
    SbVec3f  selectedAtomCoord;

    // This is the value of selectedAtomCoord after all transforms in the
    // scene graph are applied to it.
    SbVec3f  selectedAtomCoordX;

    // This is the center of the radius selection sphere.  It is
    // selectedAtomCoordX with all transforms before the ChemSelection
    // removed (that is, selectedAtomCoordX is multiplied by 
    // currentModelMatrixInverse).
    SbVec3f  sphereCoord;

    // If some sort of scaling occurs before the ChemSelection node,
    // we have to take that into account when determining the radius
    // of the selection sphere
    float    sphereRadiusScaleFactor;

    // The inverse of the ChemSelection modelMatrix.  It is used to position
    // the radius selection sphere correctly.  Applying this to
    // selectedAtomCoordX gives sphereCoord.
    SbMatrix   currentModelMatrix;
    SbMatrix   currentModelMatrixInverse;
    SbRotation currentRotation;
    SbViewVolume currentViewVolume;
    SbMatrix   currentViewMatrix;

    // The radius of the "radiusSelect".
    float    selectedRadius;
    float    selectedRadiusSquared;

    // Some things need to be initialized when the radiusSelected is
    // started and this flag is used to signal that fact.
    SbBool   radiusSelectionFirstPoint;

    void startRadiusSelection(ChemDisplayPath *chemPath);
    void stopRadiusSelection();
    void getSelectedAtomCoord(ChemDisplayPath *chemPath);

    // This action is used to search for Chem type nodes.
    static SoSearchAction *searchAction;
    // Get the various bounding boxes from the ChemDisplay node.
    static ChemBBoxAction *chemBBoxAction;

    // this is called from the two constructors.
    void constructorCommon();  

    void reallyHandleEvent(SoHandleEventAction *action);

    // Adds a point to the lasso coordinate list.
    void          addPointToLasso(SoHandleEventAction *action);

    // Left mouse button UP invokes the selection of objects within the lasso
    // which is handled here.
    void          doLassoPick(SbBool shiftDown, SbBool ctrlDown);
    void          doRadiusPick(SbBool shiftDown, SbBool ctrlDown);
    void          lassoChemDisplay(int32_t numLassoVertices,
                                   short theLassoType,
                                   const SbVec3f &lassoMin,
                                   const SbVec3f &lassoMax,
                                   SbPList *selectedItemList);
    void          lassoChemLabel(int32_t numLassoVertices,
                                 short theLassoType,
                                 const SbVec3f &lassoMin,
                                 const SbVec3f &lassoMax,
                                 SbPList *selectedItemList);
    void          lassoChemMonitor(int32_t numLassoVertices,
                                   short theLassoType,
                                   const SbVec3f &lassoMin,
                                   const SbVec3f &lassoMax,
                                   SbPList *selectedItemList);
    void          radiusPickChemDisplay(SbPList *selectedItemList);
    void          radiusPickChemLabel(SbPList *selectedItemList);
    void          radiusPickChemMonitor(SbPList *selectedItemList);
};

#endif /* !__CHEM_SELECTION_H__ */
