/*
 * ChemContour.c++
 *
 *     Classes:  ChemContour
 *
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

#ident "$Revision: 22147 $"

#include "inv/actions/SoCallbackAction.H"
#include "inv/actions/SoGetBoundingBoxAction.H"
#include "inv/actions/SoGetMatrixAction.H"
#include "inv/actions/SoGLRenderAction.H"
#include "inv/actions/SoHandleEventAction.H"
#include "inv/actions/SoRayPickAction.H"
#include "inv/actions/SoWriteAction.H"

#include "inv/misc/SoChildList.H"

#include "inv/ChemKit/ChemContour.H"
#include "inv/ChemKit/ChemLattice3.H"
#include "inv/ChemKit/ContourLib.H"

//#define TIMEIT
#ifdef TIMEIT
SbTime now = 0.0;
#define RESET_TIMER()   now = SbTime::getTimeOfDay()
#define MARK_TIME(MSG) \
{                               \
    if (now == 0.0) RESET_TIMER();      \
    SbTime tmp = SbTime::getTimeOfDay() - now;   \
    fprintf(stderr, "%f : %s\n", tmp.getValue(), MSG); \
    RESET_TIMER(); \
}   
#define LAP_TIME(MSG) \
{                               \
    if (now == 0.0) RESET_TIMER();      \
    SbTime tmp = SbTime::getTimeOfDay() - now;   \
    fprintf(stderr, "%f : \t%s\n", tmp.getValue(), MSG); \
}
#else
#define RESET_TIMER()
#define MARK_TIME(X)
#define LAP_TIME(X)
#endif


SO_NODE_SOURCE(ChemContour);

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Constructor
//
// Use: public

ChemContour::ChemContour()
// 
////////////////////////////////////////////////////////////////////////
{ 
    SO_NODE_CONSTRUCTOR(ChemContour);

    SO_NODE_ADD_FIELD(antiAlias, (FALSE));
    SO_NODE_ADD_FIELD(iAxis, (TRUE));
    SO_NODE_ADD_FIELD(jAxis, (FALSE));
    SO_NODE_ADD_FIELD(kAxis, (FALSE));
    SO_NODE_ADD_FIELD(dataVar, (0));
    SO_NODE_ADD_FIELD(levels, (0.0));
    SO_NODE_ADD_FIELD(orderedRGBA, (0));
    SO_NODE_ADD_FIELD(lattice, (NULL));

    levels.deleteValues(0,-1);
    orderedRGBA.deleteValues(0,-1);

    // Make the child list.  SoChildList is derived from SoNodeList which
    // is derived from SoBaseList.  SoBaseList performs ref() and unref()
    // when append() and remove() are performed.

    children = new SoChildList(this, 1);
    lineSet = new SoIndexedLineSet;
    children->append(lineSet);

    SoVertexProperty *vp = new SoVertexProperty;
    vp->ref();
    lineSet->vertexProperty.setValue(vp);

    // Initialize private variables
    first = TRUE;
    lastIAxis = TRUE;
    lastJAxis = FALSE;
    lastKAxis = FALSE;
    lastLevels.setValue(0.0);
    lastColors.setValue(0);

    isBuiltIn = TRUE;
}
 
////////////////////////////////////////////////////////////////////////
//
// Description:
//    Destructor
//
// Use: protected
   
ChemContour::~ChemContour()
//
////////////////////////////////////////////////////////////////////////
{
    // Remove children from the list. "remove" does an unref.
    for (int i = children->getLength()-1; i >= 0; i--) {
        children->remove(i);
    }

    // Delete vertexProperty
    SoVertexProperty *vp =
        (SoVertexProperty *)lineSet->vertexProperty.getValue();
    if (vp != NULL) vp->unref();

    // Deleting lineSet is unnecessary since it was unrefed by
    // the remove.

    // Delete children
    if (children != NULL) delete children;
}
 
////////////////////////////////////////////////////////////////////////
//
// Description:
//    Implements traversal of the children for the given action
//
// Use: extender
    
void
ChemContour::doAction(SoAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    regenerate(FALSE);
    
    //this code is identical to that of SoGroup::doAction
    int          numIndices;
    const int    *indices;
    if (action->getPathCode(numIndices, indices) == SoAction::IN_PATH) {
        children->traverse(action, 0, indices[numIndices - 1]);
    }
    else {
        children->traverse(action);
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Implements GL rendering.
//
// Use: extender
   
void
ChemContour::GLRender(SoGLRenderAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    if (!shouldGLRender(action)) return;
    ChemContour::doAction(action);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Handles the pick action
//
// Use: extender

void
ChemContour::pick(SoPickAction *action)
//  
////////////////////////////////////////////////////////////////////////
{
    ChemContour::doAction(action);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Picks polys representing an indexedLineSet.
//
// Use: extender

void
ChemContour::rayPick(SoRayPickAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    if (!shouldRayPick(action)) return;
    ChemContour::doAction(action);
}

////////////////////////////////////////////////////////////////////////
// 
// Description:
//    Handles the callback action
// 
// Use: extender

void
ChemContour::callback(SoCallbackAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    ChemContour::doAction(action);
}
  
////////////////////////////////////////////////////////////////////////
//    
// Description:
//    Handles the write action.  Does not write out the children.  If
//    the user wants to write out the children, they can use getChildren
//    to get the list and write them out themselves.
//
// Use: extender

void
ChemContour::write(SoWriteAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    SoNode::write(action);
}

////////////////////////////////////////////////////////////////////////
// 
// Description:
//    Handle the getBoundingBox action
//
// Use: extender

void
ChemContour::getBoundingBox(SoGetBoundingBoxAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    // From Hansres:
    // note: we have to override the SoShape::getBoundingBox method, which
    // calls computeBBox.
    ChemContour::doAction(action);   
}

////////////////////////////////////////////////////////////////////////
//  
// Description:
//    Handles the search action.  It does not search the children.
//
// Use: extender
    
void
ChemContour::search(SoSearchAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    // Don't search hidden children
    SoNode::search(action);
}
  
////////////////////////////////////////////////////////////////////////
//
// Description:
//    Handles the event actions
//
// Use: extender
 
void
ChemContour::handleEvent(SoHandleEventAction *action)
//
////////////////////////////////////////////////////////////////////////
{  
    ChemContour::doAction(action);
}
  
////////////////////////////////////////////////////////////////////////
//
// Description:
//    Handles the getMatrix action
//
// Use: extender
 
void
ChemContour::getMatrix(SoGetMatrixAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    // most of this code is from SoGroup:
    int         numIndices;
    const int   *indices;

    // Only need to compute matrix if group is a node in middle of
    // current path chain or is off path chain (since the only way
    // this could be called if it is off the chain is if the group is
    // under a group that affects the chain).

    switch (action->getPathCode(numIndices, indices)) {
  
      case SoAction::NO_PATH:
        break;
    
      case SoAction::IN_PATH:
        regenerate(FALSE);
        children->traverse(action, 0, indices[numIndices - 1]);
        break;
  
      case SoAction::BELOW_PATH:
        break;
  
      case SoAction::OFF_PATH:
        regenerate(FALSE);
        children->traverse(action);
        break;
    }
}
   
////////////////////////////////////////////////////////////////////////
//
// Description:
//    Returns a pointer to the child list
//
// Use: public 

SoChildList *
ChemContour::getChildren() const
//
////////////////////////////////////////////////////////////////////////
{
    return children;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Generates polys representing an indexedLineSet.
//
// Use: protected

void
ChemContour::generatePrimitives(SoAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    regenerate(FALSE);
    ((ChemContour *)lineSet)->generatePrimitives(action);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Computes bounding box of polys representing an indexedLineSet.
//
// Use: protected

void
ChemContour::computeBBox(SoAction *action, SbBox3f &box, SbVec3f &center)
//
////////////////////////////////////////////////////////////////////////
{
fprintf(stderr, "In ChemContour::computeBBox.  THIS SHOULD NEVER HAPPEN\n\n");

    regenerate(FALSE);
    ((SoShape *)lineSet)->computeBBox(action, box, center);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Recalculate the contours if values have changed
//
// Use: private
    
void
ChemContour::regenerate(SbBool forceIt)
//  
////////////////////////////////////////////////////////////////////////
{ 
    ChemLattice3 *lat = (ChemLattice3 *)lattice.getValue();
    if (lat == NULL) return;

    if (first) {
        first = FALSE;
    }
    else {
        if (!forceIt) {
            if ((lastIAxis == iAxis.getValue()) &&
                (lastJAxis == jAxis.getValue()) &&
                (lastKAxis == kAxis.getValue()) &&
                (lastLevels == levels) &&
                (lastColors == orderedRGBA)) {
                return;
            }
        }
    }

    lastIAxis = iAxis.getValue();
    lastJAxis = jAxis.getValue();
    lastKAxis = kAxis.getValue();
    lastLevels.deleteValues(0,-1);
    lastLevels.setValues(0, levels.getNum(), levels.getValues(0));
    lastColors.deleteValues(0,-1);
    lastColors.setValues(0, orderedRGBA.getNum(), orderedRGBA.getValues(0));

    SoVertexProperty *vp =
        (SoVertexProperty *)lineSet->vertexProperty.getValue();

    lineSet->coordIndex.deleteValues(0,-1);

    vp->vertex.deleteValues(0,-1);   
    if ((vp->orderedRGBA.getNum() != 0) && (orderedRGBA.getNum() != 0)) {
        vp->orderedRGBA.deleteValues(0,-1);
    }

    const uint32_t *colors;
    if (orderedRGBA.getNum() < levels.getNum()) {
        colors = NULL;
#ifdef DEBUG
        SoDebug::post("ChemContour::regenerate",
            "The number of colors is less than the number of levels!");
#endif
    }
    else {
        colors = orderedRGBA.getValues(0);
    }

RESET_TIMER();

    contourLibContour(
        lastIAxis,
        lastJAxis,
        lastKAxis,
        dataVar.getValue(),
        lat,
        (short)levels.getNum(),
        levels.getValues(0),
        colors,
        lineSet, vp);

MARK_TIME("DONE with CONTOUR");
}
