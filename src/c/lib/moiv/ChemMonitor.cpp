/*
 * ChemMonitor.c++
 *
 *     Classes:  ChemMonitor
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

#ident "$Revision: 22177 $"

#include <math.h>

/*
	Porting to Windows:
	Jamie Doornbos	2001/07/15
	Daniel Soto	2002/02/05
*/
#ifdef WIN32
#include <windows.h>
#endif
#ifndef __gl_h_
#include <GL/gl.h>
#endif /* __gl_h_ */

#include "inv/SoPickedPoint.H"

#include "inv/actions/SoCallbackAction.H"
#include "inv/actions/SoGetBoundingBoxAction.H"
#include "inv/actions/SoGetMatrixAction.H"
#include "inv/actions/SoGLRenderAction.H"
#include "inv/actions/SoHandleEventAction.H"
#include "inv/actions/SoRayPickAction.H"
#include "inv/actions/SoWriteAction.H"

#include "inv/details/SoTextDetail.H"

#include "inv/elements/SoGLLazyElement.H"
#include "inv/elements/SoLightModelElement.H"
#include "inv/elements/SoLazyElement.H"
#include "inv/elements/SoModelMatrixElement.H"
#include "inv/elements/SoProjectionMatrixElement.H"
#include "inv/elements/SoViewingMatrixElement.H"
#include "inv/elements/SoViewportRegionElement.H"
#include "inv/elements/SoViewVolumeElement.H"

#include "inv/misc/SoChildList.H"

/*
	Porting to Windows:
	Jamie Doornbos	2001/07/15
	Daniel Soto	2002/02/05
*/
#include "inv/ChemKit/FixFonts.H"

#include "inv/ChemKit/ChemBaseDataElement.H"
#include "inv/ChemKit/ChemBBoxAction.H"
#include "inv/ChemKit/ChemDisplay.H"
#include "inv/ChemKit/ChemLabel.H"
#include "inv/ChemKit/ChemLabelBBox.H"
#include "inv/ChemKit/ChemMonitor.H"
#include "inv/ChemKit/ChemMonitorBBox.H"
#include "inv/ChemKit/ChemMonitorDetail.H"
#include "inv/ChemKit/ChemMonitorPath.H"
#include "inv/ChemKit/ChemMonitorPathList.H"
#include "inv/ChemKit/ChemMonitorSelectionElement.H"
#include "inv/ChemKit/ChemSelectionElement.H"

// Daniel Soto	2002-02-15
// FIX lighting-off with Coin3D
#ifdef __coin
// --> coin 1,2 compatibility
//#include "inv/elements/SoGLLightModelElement.H"
// <-- coin 1,2 compatibility
#endif

#define RAD_TO_DEG 57.29577951308232087684
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define ABS(a) ((a) < 0.0 ? -(a) : (a))

static int numLabelLoops;
static int labelLoop;
static int32_t labelStart;
static int32_t labelEnd;
static int32_t theLabel;
   
#define LABELLOOP_START(INDEX, NUM)                                           \
numLabelLoops = (INDEX).getNum();                                             \
for (labelLoop = 0; labelLoop < numLabelLoops; labelLoop++) {                 \
    (INDEX)[labelLoop].getValue(labelStart, labelEnd);                        \
    if (labelEnd == -1) {                                                     \
       labelEnd = NUM;                                                        \
    }                                                                         \
    else {                                                                    \
       labelEnd += labelStart;                                                \
    }                                                                         \
    for (theLabel = labelStart; theLabel < labelEnd; theLabel++) {
   
#define LABELLOOP_END                                                         \
    }                                                                         \
}  


SO_NODE_SOURCE(ChemMonitor);

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Constructor
//
// Use: public

ChemMonitor::ChemMonitor()
//
////////////////////////////////////////////////////////////////////////
{
    SO_NODE_CONSTRUCTOR(ChemMonitor);

    SO_NODE_ADD_FIELD(distanceLabelLeftRightJustification,
        (ChemMonitor::LR_DEFAULT));
    SO_NODE_ADD_FIELD(distanceLabelTopBottomJustification,
        (ChemMonitor::TB_DEFAULT));
    SO_NODE_ADD_FIELD(angleLabelLeftRightJustification,
        (ChemMonitor::LR_DEFAULT));
    SO_NODE_ADD_FIELD(angleLabelTopBottomJustification,
        (ChemMonitor::TB_DEFAULT));
    SO_NODE_ADD_FIELD(torsionalLabelLeftRightJustification,
        (ChemMonitor::LR_DEFAULT));
    SO_NODE_ADD_FIELD(torsionalLabelTopBottomJustification,
        (ChemMonitor::TB_DEFAULT));

    SO_NODE_ADD_FIELD(distanceLabelFormat, ("%-6.3f"));
    SO_NODE_ADD_FIELD(distanceLabelFontName, (DEFAULT_FONT));
    SO_NODE_ADD_FIELD(distanceLabelFontSize, (10));
    SO_NODE_ADD_FIELD(distanceLabelColor, (1.0, 1.0, 1.0));

    SO_NODE_ADD_FIELD(angleLabelFormat, ("%-6.3f"));
    SO_NODE_ADD_FIELD(angleLabelFontName, (DEFAULT_FONT));
    SO_NODE_ADD_FIELD(angleLabelFontSize, (10));
    SO_NODE_ADD_FIELD(angleLabelColor, (1.0, 1.0, 1.0));

    SO_NODE_ADD_FIELD(torsionalLabelFormat, ("%-6.3f"));
    SO_NODE_ADD_FIELD(torsionalLabelFontName, (DEFAULT_FONT));
    SO_NODE_ADD_FIELD(torsionalLabelFontSize, (10));
    SO_NODE_ADD_FIELD(torsionalLabelColor, (1.0, 1.0, 1.0));

    SO_NODE_ADD_FIELD(distanceGeomColor, (1.0, 1.0, 1.0));
    SO_NODE_ADD_FIELD(angleGeomColor, (1.0, 1.0, 1.0));
    SO_NODE_ADD_FIELD(torsionalGeomColor, (1.0, 1.0, 1.0));

    SO_NODE_ADD_FIELD(doHighlighting, (TRUE));
    SO_NODE_ADD_FIELD(highlightColor, (0.3f, 0.3f, 0.0f));

    SO_NODE_ADD_FIELD(distanceStippleFactor, (1));
    SO_NODE_ADD_FIELD(distanceStipplePattern, (0x0F0F));
    SO_NODE_ADD_FIELD(angleStippleFactor, (1));
    SO_NODE_ADD_FIELD(angleStipplePattern, (0xFFFF));
    SO_NODE_ADD_FIELD(torsionalStippleFactor, (1));
    SO_NODE_ADD_FIELD(torsionalStipplePattern, (0xFFFF));

    SO_NODE_ADD_FIELD(drawDistanceLine, (TRUE));
    SO_NODE_ADD_FIELD(drawAngleLines, (TRUE));
    SO_NODE_ADD_FIELD(drawTorsionalLines, (TRUE));

    SO_NODE_ADD_FIELD(angleArcScale, (0.5));
    SO_NODE_ADD_FIELD(torsionalArcScale, (0.5));

    SbVec2AtomSpec atomSpec2;
    SbVec3AtomSpec atomSpec3;
    SbVec4AtomSpec atomSpec4;
    SO_NODE_ADD_FIELD(distance, (atomSpec2));
    SO_NODE_ADD_FIELD(angle, (atomSpec3));
    SO_NODE_ADD_FIELD(torsional, (atomSpec4));

    distance.deleteValues(0);
    angle.deleteValues(0);
    torsional.deleteValues(0);

    distance.setDefault(TRUE);
    angle.setDefault(TRUE);
    torsional.setDefault(TRUE);

    SO_NODE_DEFINE_ENUM_VALUE(LeftRightJustification, LR_LEFT);
    SO_NODE_DEFINE_ENUM_VALUE(LeftRightJustification, LR_RIGHT);
    SO_NODE_DEFINE_ENUM_VALUE(LeftRightJustification, LR_CENTER);
    SO_NODE_DEFINE_ENUM_VALUE(LeftRightJustification, LR_DEFAULT);

    SO_NODE_DEFINE_ENUM_VALUE(TopBottomJustification, TB_TOP);
    SO_NODE_DEFINE_ENUM_VALUE(TopBottomJustification, TB_BOTTOM);
    SO_NODE_DEFINE_ENUM_VALUE(TopBottomJustification, TB_MIDDLE);
    SO_NODE_DEFINE_ENUM_VALUE(TopBottomJustification, TB_DEFAULT);

    SO_NODE_SET_SF_ENUM_TYPE(distanceLabelLeftRightJustification,
        LeftRightJustification);
    SO_NODE_SET_SF_ENUM_TYPE(angleLabelLeftRightJustification,
        LeftRightJustification);
    SO_NODE_SET_SF_ENUM_TYPE(torsionalLabelLeftRightJustification,
        LeftRightJustification);

    SO_NODE_SET_SF_ENUM_TYPE(distanceLabelTopBottomJustification,
        TopBottomJustification);
    SO_NODE_SET_SF_ENUM_TYPE(angleLabelTopBottomJustification,
        TopBottomJustification);
    SO_NODE_SET_SF_ENUM_TYPE(torsionalLabelTopBottomJustification,
        TopBottomJustification);

    children = new SoChildList(this, 3);

    distanceLabels = new ChemLabel;
    angleLabels = new ChemLabel;
    torsionalLabels = new ChemLabel;

    children->append(distanceLabels);
    children->append(angleLabels);
    children->append(torsionalLabels);

    currentMVP.makeIdentity();
    currentModelMatrix.makeIdentity();
    currentModelMatrixInverse.makeIdentity();

    lastSelectionElement = NULL;

    resetIndices();

    isBuiltIn = TRUE;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Destructor
//
// Use: protected

ChemMonitor::~ChemMonitor()
//
////////////////////////////////////////////////////////////////////////
{
    // Remove children from the list. "remove" does an unref.
    if (children != NULL) {
        for (int i = children->getLength()-1; i >= 0; i--) {
            children->remove(i);
        }
        delete children;
    }

    if (lastSelectionElement != NULL) {
        delete lastSelectionElement;
    }
}
            
////////////////////////////////////////////////////////////////////////
//
// Description:
//    If a change was made to one of the distance, angle or torsional
//    fields, or the doHighlighting field, then we need to reset some
//    internal things.  
//
// Use: internal

void
ChemMonitor::notify(SoNotList *list)
//
////////////////////////////////////////////////////////////////////////
{
    if ((list->getLastRec()->getType() == SoNotRec::CONTAINER)) {
        SoField *field = list->getLastField();
        if ((field == &distance) || (field == &angle) ||
            (field == &torsional) || (field == &doHighlighting)) {
            resetIndices();
        }
    }
    SoShape::notify(list);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Performs GL rendering of the monitors
//
// Use: public, extender

void
ChemMonitor::GLRender(SoGLRenderAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    // First see if the object is visible and should be rendered now
    if (!shouldGLRender(action))
        return;

    if ((distance.getNum() <= 0) && (angle.getNum() <= 0) &&
        (torsional.getNum() <= 0)) return;

    SoState *state = action->getState();

    // Get the currentModelMatrix and its inverse
    currentModelMatrix = SoModelMatrixElement::get(state);
    currentModelMatrixInverse = currentModelMatrix.inverse();

    currentMVP = SoProjectionMatrixElement::get(state);
    currentMVP = currentMVP.multLeft(SoViewingMatrixElement::get(state));
    currentMVP = currentMVP.multLeft(currentModelMatrix);

    generateIndices(action);

    // Setup for rendering of lines.  The ChemLabels will do their own
    // setting up of state.
    state->push();

    // Turn off lighting
#ifdef IV2_0
    SoLightModelElement::set(state, this, SoLightModelElement::BASE_COLOR);
// --> original oiv bug?
//SoLightModelElement::set(state, this, SoLightModelElement::BASE_COLOR); #else
// <-- original oiv bug?
#else
    int32_t myMask = SoLazyElement::DIFFUSE_MASK;

    SoGLLazyElement::setLightModel(state, SoLazyElement::BASE_COLOR);
    SoGLLazyElement::setColorMaterial(state, TRUE);
    SoGLLazyElement *lazyElt = (SoGLLazyElement *)
            SoLazyElement::getInstance(state);
    lazyElt->send(state, SoLazyElement::ALL_MASK &( ~myMask)); 
#endif
// Daniel Soto	2002-02-15
// FIX lighting-off with Coin3D
#ifdef __coin
// --> coin 1,2 compatibility
//		SoGLLightModelElement::getInstance(state)->forceSend(SoLightModelElement::BASE_COLOR);
		glDisable(GL_LIGHTING);
// <-- coin 1,2 compatibility
#endif

    // Empty the labels since we will be creating new ones
    resetLabels();

    // Render the distance monitor geom
    if (distance.getNum() > 0) renderDistance(action);

    // Render the angle monitor geom
    if (angle.getNum() > 0) renderAngle(action);

    // Render the torsional monitor geom
    if (torsional.getNum() > 0) renderTorsional(action);

#ifndef IV2_0
    SoGLLazyElement::setColorMaterial(state, FALSE);
    ((SoGLLazyElement *)SoLazyElement::getInstance(state))->
        reset(state, myMask);
#endif
    state->pop();

    // Render the labels
    if (distance.getNum() > 0) {
        ((SoShape *)distanceLabels)->GLRender(action);
    }
    if (angle.getNum() > 0) {
        ((SoShape *)angleLabels)->GLRender(action);
    }
    if (torsional.getNum() > 0) {
        ((SoShape *)torsionalLabels)->GLRender(action);
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Implements ray picking
//
// Use: public, extender

void
ChemMonitor::rayPick(SoRayPickAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    if (!shouldRayPick(action)) return;

    //SoState *state = action->getState();

    // See if the ChemSelection node has specified to pick monitors
    if ((ChemSelectionElement::get(action->getState()) &
         ChemSelectionElement::CHEMMONITORS) == 0) return;
    
    // Do the labels first since we will be performing a
    // computeObjectSpaceRay before picking the geoms and I 
    // don't know if that will mess up the picking of labels.
    SoPickedPointList *ptList = new SoPickedPointList;
    SoPickedPoint *newPP, *pp;

    // See if any distanceLabels where picked
    if (distance.getNum() > 0) {
        distanceLabels->getPickedPoints(action, (SoNode *)this, ptList);
        for (int32_t i = 0; i < ptList->getLength(); i++) {
            pp = (*ptList)[i];
            const SbVec3f &point = pp->getPoint();
            const SoTextDetail *textDetail =
                (const SoTextDetail *)pp->getDetail();
            if (textDetail != NULL) {
                int32_t whichLabel = textDetail->getStringIndex();
                if ((newPP = action->addIntersection(point)) != NULL) {
                    newPP->setMaterialIndex(pp->getMaterialIndex());
                    newPP->setObjectNormal(SbVec3f(0,0,1));
                    newPP->setObjectTextureCoords(SbVec4f(0,0,0,0));
                    ChemMonitorDetail *newDetail = new ChemMonitorDetail;
                    newDetail->setDistanceIndex(whichLabel);
                    newPP->setDetail(newDetail, this);
                }
            }
        }
        // Truncate deletes all instances therefore no need to
        // explicitly delete pp
        ptList->truncate(0);
// --> coin, memory pointers aren't freed
#ifdef __coin
				ptList->truncate(0,TRUE);
#endif
// <-- coin, memory pointers aren't freed
    }

    // See if any angleLabels where picked
    if (angle.getNum() > 0) {
        angleLabels->getPickedPoints(action, (SoNode *)this, ptList);
        for (int32_t i = 0; i < ptList->getLength(); i++) {
            pp = (*ptList)[i];
            const SbVec3f &point = pp->getPoint();
            const SoTextDetail *textDetail =
                (const SoTextDetail *)pp->getDetail();
            if (textDetail != NULL) {
                int32_t whichLabel = textDetail->getStringIndex();
                if ((newPP = action->addIntersection(point)) != NULL) {
                    newPP->setMaterialIndex(pp->getMaterialIndex());
                    newPP->setObjectNormal(SbVec3f(0,0,1));
                    newPP->setObjectTextureCoords(SbVec4f(0,0,0,0));
                    ChemMonitorDetail *newDetail = new ChemMonitorDetail;
                    newDetail->setAngleIndex(whichLabel);
                    newPP->setDetail(newDetail, this);
                }
            }
        }
        // Truncate deletes all instances therefore no need to
        // explicitly delete pp
        ptList->truncate(0);
// --> coin, memory pointers aren't freed
#ifdef __coin
				ptList->truncate(0,TRUE);
#endif
// <-- coin, memory pointers aren't freed
    }

    // See if any torsionalLabels where picked
    if (torsional.getNum() > 0) {
        torsionalLabels->getPickedPoints(action, (SoNode *)this, ptList);
        for (int32_t i = 0; i < ptList->getLength(); i++) {
            pp = (*ptList)[i];
            const SbVec3f &point = pp->getPoint();
            const SoTextDetail *textDetail =
                (const SoTextDetail *)pp->getDetail();
            if (textDetail != NULL) {
                int32_t whichLabel = textDetail->getStringIndex();
                if ((newPP = action->addIntersection(point)) != NULL) {
                    newPP->setMaterialIndex(pp->getMaterialIndex());
                    newPP->setObjectNormal(SbVec3f(0,0,1));
                    newPP->setObjectTextureCoords(SbVec4f(0,0,0,0));
                    ChemMonitorDetail *newDetail = new ChemMonitorDetail;
                    newDetail->setTorsionalIndex(whichLabel);
                    newPP->setDetail(newDetail, this);
                }
            }
        }
        // Truncate deletes all instances therefore no need to
        // explicitly delete pp
        ptList->truncate(0);
// --> coin, memory pointers aren't freed
#ifdef __coin
				ptList->truncate(0,TRUE);
#endif
// <-- coin, memory pointers aren't freed
    }
    delete ptList;

    // Now do the geoms
    computeObjectSpaceRay(action);

    SbVec3f norm = action->getLine().getDirection();
    norm.negate();
    norm.normalize();

    // See if any distance geometries were picked
    pickDistanceGeoms(action, norm);

    // See if any angle geometries were picked
    pickAngleGeoms(action, norm);

    // See of any torsional geometries were picked
    pickTorsionalGeoms(action, norm);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Computes the bounding box of the monitors
//
// Use: protected

void
ChemMonitor::computeBBox(SoAction *action, SbBox3f &box,
    SbVec3f &center)
//
////////////////////////////////////////////////////////////////////////
{
    box.makeEmpty();
    center.setValue(0.0, 0.0, 0.0);

    if ((distance.getNum() <= 0) && (angle.getNum() <= 0) &&
        (torsional.getNum() <= 0)) return;

    // Get the bounding box of the distance monitor geoms
    if (distance.getNum() > 0) computeDistanceBBox(action, box, center);

    // Get the bounding box of the angle monitor geoms
    if (angle.getNum() > 0) computeAngleBBox(action, box, center);

    // Get the bounding box of the torsional monitor geoms
    if (torsional.getNum() > 0) computeTorsionalBBox(action, box, center);

    // Get the bounding box of the labels
    int32_t labelCount;
    SbBox3f lBox;
    SbVec3f lCenter, tmpCenter;
    tmpCenter.setValue(0.0, 0.0, 0.0);

    labelCount = 0;
    if (distance.getNum() > 0) {
        ((SoShape *)distanceLabels)->computeBBox(action, lBox, lCenter);
        box.extendBy(lBox);
        tmpCenter += lCenter;
        labelCount++;
    }

    if (angle.getNum() > 0) {
        ((SoShape *)angleLabels)->computeBBox(action, lBox, lCenter);
        box.extendBy(lBox);
        tmpCenter += lCenter;
        labelCount++;
    }

    if (torsional.getNum() > 0) {
        ((SoShape *)torsionalLabels)->computeBBox(action, lBox, lCenter);
        box.extendBy(lBox);
        tmpCenter += lCenter;
        labelCount++;
    }

    if (labelCount > 0) {
        tmpCenter /= (float)labelCount;
        center += tmpCenter;
        center *= 0.5;
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Generate the primitives.  This is a no-op since the labels
//    cannot be "generated".
//
// Use: protected

void
ChemMonitor::generatePrimitives(SoAction *action)
//
////////////////////////////////////////////////////////////////////////
{
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Setup rendering of the distance monitors
// 
// Use: private

void
ChemMonitor::renderDistance(SoGLRenderAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    // Initialize the distanceLabel values
    distanceLabels->fontName = distanceLabelFontName.getValue();
    distanceLabels->fontSize = distanceLabelFontSize.getValue();
    distanceLabels->leftRightJustificationBinding = ChemLabel::OVERALL;
    distanceLabels->leftRightJustification =
        distanceLabelLeftRightJustification.getValue();
    distanceLabels->topBottomJustificationBinding = ChemLabel::OVERALL;
    distanceLabels->topBottomJustification =
        distanceLabelTopBottomJustification.getValue();
    distanceLabels->colorBinding = ChemLabel::PER_LABEL;
    distanceLabels->color.setValue(distanceLabelColor.getValue());

    // Set the color and turn on stippling
    glColor3fv(distanceGeomColor.getValue().getValue());
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(distanceStippleFactor.getValue(),
        distanceStipplePattern.getValue());
    glBegin(GL_LINES);

    doDistanceRender(normalDistanceIndex, distanceLabelColor.getValue());

    glColor3fv(highlightColor.getValue().getValue());
    doDistanceRender(highlightDistanceIndex, highlightColor.getValue());

    glEnd();
    glDisable(GL_LINE_STIPPLE);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Render the distance monitors
// 
// Use: private

void
ChemMonitor::doDistanceRender(const MFVec2i &index,
    const SbColor &labelColor)
//
////////////////////////////////////////////////////////////////////////
{
    int32_t from, to;
    float length;
    char buf[32];
    SbVec3f coords1, coords2, midCoord;

    SbAtomSpec atomSpecFrom, atomSpecTo;
    ChemBaseData *chemDataFrom, *chemDataTo;
    ChemDisplay  *chemDisplayFrom, *chemDisplayTo;
    SbMatrix matrixFrom, matrixTo;

    LABELLOOP_START(index, distance.getNum())
        distance[theLabel].getValue(atomSpecFrom, atomSpecTo);
        atomSpecFrom.getValue(chemDataFrom, chemDisplayFrom, from);
        atomSpecTo.getValue(chemDataTo, chemDisplayTo, to);

        coords1 = chemDataFrom->getAtomCoordinates(from).getValue();
        coords2 = chemDataTo->getAtomCoordinates(to).getValue();

        matrixFrom = chemDisplayFrom->getCurrentModelMatrix();
        matrixTo = chemDisplayTo->getCurrentModelMatrix();
        matrixFrom.multVecMatrix(coords1, coords1);
        matrixTo.multVecMatrix(coords2, coords2);

        // Remove the affect of any model matrices applied to
        // ChemMonitor
        currentModelMatrixInverse.multVecMatrix(coords1, coords1);
        currentModelMatrixInverse.multVecMatrix(coords2, coords2);

        if (drawDistanceLine[theLabel] == TRUE) {
            glVertex3fv(coords1.getValue());
            glVertex3fv(coords2.getValue());
        }

        midCoord = coords2 - coords1;
        length = midCoord.length();
        sprintf(buf, distanceLabelFormat.getValue().getString(), length);
        distanceLabels->text.set1Value(theLabel, buf);
        midCoord = midCoord * 0.5f + coords1;
        distanceLabels->position.set1Value(theLabel, midCoord);
        distanceLabels->color.set1Value(theLabel, labelColor);
    LABELLOOP_END
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Generate the vertices for the arcs used in renderAngle and
//    renderTorsional.  The arc is in the XY plane, starts at
//    (0,1,0) and goes clockwise.
// 
// Use: static

static void
generateUnitArc(float theta, int32_t &numVertices, SbVec3f *&vertices)
//
////////////////////////////////////////////////////////////////////////
{
    int32_t i;

    if (vertices != NULL) delete [] vertices;
    numVertices = 10;
    vertices = new SbVec3f[numVertices];

    float tmpTheta = 0.0;
    float dTheta = theta / (numVertices - 1);
    for (i = 0; i < numVertices; i++) {
        vertices[i].setValue(sin(tmpTheta), cos(tmpTheta), 0.0);
        tmpTheta += dTheta;
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Generate the vertices for the arcs used in renderAngle and
//    computeAngleBBox.  It calculates the value of the angle (theta)
//    as well as the transformation matrix (theMatrix) needed to put 
//    the unit arc coordinates into the correct location.  The angle,
//    the number of vertices, the vertices themselves and the 
//    transformation matrix are returned.
// 
// Use: static

static void
generateAngleVertices(const SbVec3f &coords1, const SbVec3f &coords2,
    const SbVec3f &coords3, float radiusScale,
    float &theta, int32_t &numVertices, SbVec3f *&vertices,
    SbMatrix &theMatrix)
//
////////////////////////////////////////////////////////////////////////
{
    float radius, cosTheta;
    SbVec3f vec12, vec32, crossVec;
    SbVec3f theTranslation;
    SbRotation theRotation, tmpRotation;
    SbVec3f theScale;
    SbVec3f yAxis(0.0, 1.0, 0.0);
    SbVec3f zAxis(0.0, 0.0, 1.0);

    // Get the radius and angle, and generate the unit arc
    vec12 = coords1 - coords2;
    vec32 = coords3 - coords2;
    radius = MIN(vec12.length(), vec32.length()) * radiusScale;
    vec12.normalize();
    vec32.normalize();
    cosTheta = vec12.dot(vec32);
    if (cosTheta < -1.0) cosTheta = -1.0;
    if (cosTheta >  1.0) cosTheta =  1.0;
    theta = acosf(cosTheta);
    generateUnitArc(theta, numVertices, vertices);

    theScale.setValue(radius, radius, radius);
    theTranslation = coords2;

    // Get the rotation that takes the normal to the plane
    // of the 3 coordinates (crossVec) onto the Z axis.
    crossVec = vec32.cross(vec12);
    tmpRotation.setValue(crossVec, zAxis);

    // Apply this rotation to vec12
    tmpRotation.multVec(vec12, crossVec);

    // Get the rotation required to put the transformed vec12
    // onto the Y axis (actually the other way around - that is
    // the rotation to put the Y axis onto the transformed vec12).
    theRotation.setValue(yAxis, crossVec);
    theRotation *= tmpRotation.inverse();
    theMatrix.setTransform(theTranslation, theRotation,
        theScale);

    // Apply theMatrix to all of the vertices
    for (int32_t i = 0; i < numVertices; i++) {
        theMatrix.multVecMatrix(vertices[i], vertices[i]);
    }
}

// The following TOLERANCE is used to see if 2 vectors are parallel.  If
// the angle between them is less than 1 degree, then they are parallel.
// cos(0) - cos(1 degree) = 0.00015230844
#define TOLERANCE 0.00015230844

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Generate the vertices for the arcs used in renderTorsional and
//    computeTorsionalBBox.  It calculates the value of the torsional
//    (theta) as well as the transformation matrix (theMatrix) needed to put 
//    the unit arc coordinates into the correct location.  The torsional,
//    the number of vertices, the vertices themselves and the 
//    transformation matrix are returned.  Also returned are the points
//    on the end vectors at which rendering should begin.
// 
// Use: static

static void
generateTorsionalVertices(const SbVec3f &coords1, const SbVec3f &coords2,
    const SbVec3f &coords3, const SbVec3f &coords4, float radiusScale,
    SbBool &hasLinearVectors,
    float &theta, int32_t &numVertices, SbVec3f *&vertices,
    SbMatrix &theMatrix, SbVec3f &midVec12, SbVec3f &midVec34)
//
////////////////////////////////////////////////////////////////////////
{
    int32_t i;
    float   cosTheta;
    float   radius, coords1NormalLength, coords4NormalLength;
    SbVec3f vec12, vec23, vec43, tmpVec;
    SbLine line23;
    SbVec3f perpNormal;
    SbVec3f theTranslation;
    SbRotation theRotation, tmpRotation;
    SbVec3f theScale;
    SbVec3f yAxis(0.0, 1.0, 0.0);
    SbVec3f zAxis(0.0, 0.0, 1.0);

    // Get the vectors we need for later
    vec12 = coords1 - coords2;
    vec23 = coords2 - coords3;
    vec43 = coords4 - coords3;

    // Need to check if vec12 and vec23 are linear.
    hasLinearVectors = FALSE;
    float vec23Len = vec23.length();
    float dotP = vec12.dot(vec23);
    cosTheta = ABS(dotP / (vec12.length() * vec23Len));
    if (ABS(1.0 - cosTheta) < TOLERANCE) hasLinearVectors = TRUE;

    // If vec12 and vec23 aren't linear, then check vec23 and vec43.
    if (!hasLinearVectors) {
        dotP = vec43.dot(vec23);
        cosTheta = ABS(dotP / (vec43.length() * vec23Len));
        if (ABS(1.0 - cosTheta) < TOLERANCE) hasLinearVectors = TRUE;
    }

    // If one of the angles is linear, then we can't really determine
    // the torsional value.
    if (hasLinearVectors) {
        // Set theta to 0.0.
        // Make sure that vertices is NULL'd out and numVertices is 0;
        theta = 0.0;
        numVertices = 0;
        if (vertices != NULL) delete [] vertices;
        vertices = NULL;

        // NULL out theMatrix
        theMatrix.makeIdentity();

        // Set the value of midVec12 and midVec34
        midVec12 = (coords1 + coords2) * 0.5f;
        midVec34 = (coords3 + coords4) * 0.5f;

        return;
    }

    SbPlane plane1(coords1, coords2, coords3);
    SbPlane plane2(coords2, coords3, coords4);
    SbVec3f plane2Normal = plane2.getNormal();

    cosTheta = plane1.getNormal().dot(plane2Normal);
    if (cosTheta < -1.0) cosTheta = -1.0;
    if (cosTheta >  1.0) cosTheta =  1.0;

    // The next line causes trans to be 0.0 and cis to be 180.0
    // cosTheta = -cosTheta;

    theta = acosf(cosTheta);
    if ((coords1 - coords2).dot(plane2Normal) > 0.0) {
        theta = -theta; }
    generateUnitArc(theta, numVertices, vertices);

    // Generate the rotation that takes the zAxis onto the
    // coords2-coords3 vector.  This is because generateUnitArc
    // creates the arc in the XY plane and we want the arc to
    // go around the coords2-coords3 vector.
    tmpRotation.setValue(vec23, zAxis);

    // Get the translation which is the midpoint of vec23
    theTranslation = vec23 * 0.5f + coords3;

    // We need to find the 4th coordinate such that the torsional
    // value for 1-2-3-new4 is 0.0.  This 4th point will be the
    // starting point for the arc.  We get this by dropping a 
    // perpendicular from coords1 to the line defined by coords2 
    // and coords3.
    line23.setValue(coords2, coords3);
    perpNormal = line23.getClosestPoint(coords1);
    perpNormal = coords1 - perpNormal;

    coords1NormalLength = perpNormal.length();

    // Apply tmpRotation to perpNormal
    tmpRotation.multVec(perpNormal, tmpVec);

    // Get the rotation required to put the transformed perpNormal
    // onto the Y axis (actually the other way around - that is
    // the rotation required to put the Y axis onto the
    // transformed perpNormal).
    theRotation.setValue(yAxis, tmpVec);
    theRotation *= tmpRotation.inverse();

    // Now we need to set theScale.  This is done by chosing the
    // midpoint of either (coords1 - coords2) or (coords4 - coords3).
    // The midpoint used will be the one whose distance to line23
    // is the shortest.  We already have the distance from
    // coords1 to line23 (this is in coords1NormalLength), so get the
    // distance from coords4 to line23.
    coords4NormalLength =
        (coords4 - line23.getClosestPoint(coords4)).length();
    if (coords4NormalLength < coords1NormalLength) {
        radius = coords4NormalLength * radiusScale;
        midVec34 = (coords4 - coords3) * radiusScale + coords3;

        // Find a point on (coords1 - coords2) such that it is a
        // distance coords4NormalLength/2 from line23.  This point
        // will be midVec12.
        // We need (coords3 - coords2) so negate vec23.
        vec23.negate();
        vec12.normalize();
        vec23.normalize();
        cosTheta = vec12.dot(vec23);
        if (cosTheta < -1.0) cosTheta = -1.0;
        if (cosTheta >  1.0) cosTheta =  1.0;
        float phi = M_PI - acosf(cosTheta);
        float radius2 = radius / sinf(phi);    // Fix name for msvc compile.
        midVec12 = vec12 * radius2 + coords2;  // Daniel Soto  2002-02-05
    }
    else {
        radius = coords1NormalLength * radiusScale;
        midVec12 = (coords1 - coords2) * radiusScale + coords2;

        // Find a point on (coords4 - coords3) such that it is a
        // distance coords1NormalLength/2 from line23.  This point
        // will be midVec34.
        vec23.normalize();
        vec43.normalize();
        cosTheta = vec23.dot(vec43);
        if (cosTheta < -1.0) cosTheta = -1.0;
        if (cosTheta >  1.0) cosTheta =  1.0;
        float phi = M_PI - acosf(cosTheta);
        float radius2 = radius / sinf(phi);    // Fix name for msvc compile.
        midVec34 = vec43 * radius2 + coords3;  // Daniel Soto  2002-02-05
    }
    theScale.setValue(radius, radius, radius);

    theMatrix.setTransform(theTranslation, theRotation,
        theScale);

    // Transform all of the vertices
    for (i = 0; i < numVertices; i++) {
        theMatrix.multVecMatrix(vertices[i], vertices[i]);
    }
}

#undef TOLERANCE

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Setup rendering of the angle monitors
// 
// Use: private

void
ChemMonitor::renderAngle(SoGLRenderAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    // Initialize the angleLabel values
    angleLabels->fontName = angleLabelFontName.getValue();
    angleLabels->fontSize = angleLabelFontSize.getValue();
    angleLabels->leftRightJustificationBinding = ChemLabel::OVERALL;
    angleLabels->leftRightJustification =
        angleLabelLeftRightJustification.getValue();
    angleLabels->topBottomJustificationBinding = ChemLabel::OVERALL;
    angleLabels->topBottomJustification =
        angleLabelTopBottomJustification.getValue();
    angleLabels->colorBinding = ChemLabel::PER_LABEL;

    // Set the color and turn on stippling
    glColor3fv(angleGeomColor.getValue().getValue());
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(angleStippleFactor.getValue(),
        angleStipplePattern.getValue());

    doAngleRender(normalAngleIndex, angleLabelColor.getValue());

    glColor3fv(highlightColor.getValue().getValue());
    doAngleRender(highlightAngleIndex, highlightColor.getValue());

    glDisable(GL_LINE_STIPPLE);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Render the angle monitors
// 
// Use: private

void
ChemMonitor::doAngleRender(const MFVec2i &index,
    const SbColor &labelColor)
//
////////////////////////////////////////////////////////////////////////
{
    int32_t j;
    int32_t atom1, atom2, atom3;
    float theta;
    SbVec3f coords1, coords2, coords3;
    SbVec3f tmpVec;
    char buf[32];

    int32_t numVertices;
    SbVec3f *vertices = NULL;
    SbMatrix theVertexMatrix;

    // What we need to do is find the angle and the radius of the arc.
    // We will figure out the coordinates for renderin the arc about the
    // origin and in the XY plane.  Then we will transform the rendering
    // to the correct location.  The cross product of vec32 and vec12 will
    // give a perpendicular vector which we can align with the Z axis in
    // order to get part of the rotation.  We also want vec12 to be on the
    // Y axis.
    // The radius will give us the scale.
    // We get the translation from the central coordinate (coords2).

    SbAtomSpec atomSpec1, atomSpec2, atomSpec3;
    ChemBaseData *chemData1, *chemData2, *chemData3;
    ChemDisplay  *chemDisplay1, *chemDisplay2, *chemDisplay3;
    SbMatrix matrix1, matrix2, matrix3;

    LABELLOOP_START(index, angle.getNum())
        angle[theLabel].getValue(atomSpec1, atomSpec2, atomSpec3);
        atomSpec1.getValue(chemData1, chemDisplay1, atom1);
        atomSpec2.getValue(chemData2, chemDisplay2, atom2);
        atomSpec3.getValue(chemData3, chemDisplay3, atom3);

        coords1 = chemData1->getAtomCoordinates(atom1).getValue();
        coords2 = chemData2->getAtomCoordinates(atom2).getValue();
        coords3 = chemData3->getAtomCoordinates(atom3).getValue();

        matrix1 = chemDisplay1->getCurrentModelMatrix();
        matrix2 = chemDisplay2->getCurrentModelMatrix();
        matrix3 = chemDisplay3->getCurrentModelMatrix();

        matrix1.multVecMatrix(coords1, coords1);
        matrix2.multVecMatrix(coords2, coords2);
        matrix3.multVecMatrix(coords3, coords3);

        // Remove the affect of any model matrices applied to
        // ChemMonitor
        currentModelMatrixInverse.multVecMatrix(coords1, coords1);
        currentModelMatrixInverse.multVecMatrix(coords2, coords2);
        currentModelMatrixInverse.multVecMatrix(coords3, coords3);

        // Generate the vertices making up the arc.  Also get theta
        // and theVertexMatrix
        generateAngleVertices(coords1, coords2, coords3,
            angleArcScale.getValue(),
            theta, numVertices, vertices, theVertexMatrix);

        // Render the sides of the angle
        if (drawAngleLines[theLabel] == TRUE) {
            glBegin(GL_LINE_STRIP);
            glVertex3fv(coords1.getValue());
            glVertex3fv(coords2.getValue());
            glVertex3fv(coords3.getValue());
            glEnd();
        }

        // Render the arc
        glBegin(GL_LINE_STRIP);
        for (j = 0; j < numVertices; j++) {
            glVertex3fv(vertices[j].getValue());
        }
        glEnd();

        // coords1 (and tmpVec) is on the bisector of theta
        float theta2 = theta * 0.5;
        coords1.setValue(sin(theta2), cos(theta2), 0.0);
        theVertexMatrix.multVecMatrix(coords1, tmpVec);

        // Generate the label
        theta *= (float)RAD_TO_DEG;
        sprintf(buf, angleLabelFormat.getValue().getString(), theta);
        angleLabels->text.set1Value(theLabel, buf);
        angleLabels->position.set1Value(theLabel, tmpVec);
        angleLabels->color.set1Value(theLabel, labelColor);
    LABELLOOP_END

    if (vertices != NULL) delete [] vertices;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Setup rendering of the torsional monitors
// 
// Use: private

void
ChemMonitor::renderTorsional(SoGLRenderAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    // Initialize the torsionalLabel values
    torsionalLabels->fontName = torsionalLabelFontName.getValue();
    torsionalLabels->fontSize = torsionalLabelFontSize.getValue();
    torsionalLabels->leftRightJustificationBinding = ChemLabel::OVERALL;
    torsionalLabels->leftRightJustification =
        torsionalLabelLeftRightJustification.getValue();
    torsionalLabels->topBottomJustificationBinding = ChemLabel::OVERALL;
    torsionalLabels->topBottomJustification =
        torsionalLabelTopBottomJustification.getValue();
    torsionalLabels->colorBinding = ChemLabel::PER_LABEL;

    // Set the color and turn on stippling
    glColor3fv(torsionalGeomColor.getValue().getValue());
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(torsionalStippleFactor.getValue(),
        torsionalStipplePattern.getValue());

    doTorsionalRender(normalTorsionalIndex, torsionalLabelColor.getValue());

    glColor3fv(highlightColor.getValue().getValue());
    doTorsionalRender(highlightTorsionalIndex, highlightColor.getValue());

    glDisable(GL_LINE_STIPPLE);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Render the torsional monitors
// 
// Use: private

void
ChemMonitor::doTorsionalRender(const MFVec2i &index,
    const SbColor &labelColor)
//
////////////////////////////////////////////////////////////////////////
{
    int32_t j;
    float   theta;
    int32_t atom1, atom2, atom3, atom4;
    SbVec3f coords1, coords2, coords3, coords4;
    SbVec3f midVec12, midVec34, tmpVec;
    char    buf[32];

    int32_t numVertices;
    SbVec3f *vertices = NULL;
    SbMatrix theVertexMatrix;
    SbAtomSpec atomSpec1, atomSpec2, atomSpec3, atomSpec4;
    ChemBaseData *chemData1, *chemData2, *chemData3, *chemData4;
    ChemDisplay  *chemDisplay1, *chemDisplay2, *chemDisplay3, *chemDisplay4;
    SbMatrix matrix1, matrix2, matrix3, matrix4;
    SbBool hasLinearVectors;

    LABELLOOP_START(index, torsional.getNum())
        torsional[theLabel].getValue(atomSpec1, atomSpec2, atomSpec3, atomSpec4);
        atomSpec1.getValue(chemData1, chemDisplay1, atom1);
        atomSpec2.getValue(chemData2, chemDisplay2, atom2);
        atomSpec3.getValue(chemData3, chemDisplay3, atom3);
        atomSpec4.getValue(chemData4, chemDisplay4, atom4);

        coords1 = chemData1->getAtomCoordinates(atom1).getValue();
        coords2 = chemData2->getAtomCoordinates(atom2).getValue();
        coords3 = chemData3->getAtomCoordinates(atom3).getValue();
        coords4 = chemData4->getAtomCoordinates(atom4).getValue();

        matrix1 = chemDisplay1->getCurrentModelMatrix();
        matrix2 = chemDisplay2->getCurrentModelMatrix();
        matrix3 = chemDisplay3->getCurrentModelMatrix();
        matrix4 = chemDisplay4->getCurrentModelMatrix();

        matrix1.multVecMatrix(coords1, coords1);
        matrix2.multVecMatrix(coords2, coords2);
        matrix3.multVecMatrix(coords3, coords3);
        matrix4.multVecMatrix(coords4, coords4);

        // Remove the affect of any model matrices applied to
        // ChemMonitor
        currentModelMatrixInverse.multVecMatrix(coords1, coords1);
        currentModelMatrixInverse.multVecMatrix(coords2, coords2);
        currentModelMatrixInverse.multVecMatrix(coords3, coords3);
        currentModelMatrixInverse.multVecMatrix(coords4, coords4);

        // Generate the vertices making up the arc.  Also get theta
        // and theVertexMatrix
        generateTorsionalVertices(coords1, coords2, coords3, coords4,
            torsionalArcScale.getValue(), hasLinearVectors,
            theta, numVertices, vertices, theVertexMatrix,
            midVec12, midVec34);

        // Render the lines connecting the atoms involved in the
        // description of the torsional
        if (drawTorsionalLines[theLabel] == TRUE) {
            glBegin(GL_LINE_STRIP);
            glVertex3fv(coords1.getValue());
            glVertex3fv(coords2.getValue());
            glVertex3fv(coords3.getValue());
            glVertex3fv(coords4.getValue());
            glEnd();
        }

        if (hasLinearVectors) {
            // Theta will be 0.0 and there is no arc to draw.
            // Instead draw a line from the midpoint of coords1 and 2
            // to the midpoint of coords3 and 4.  The position of the 
            // label will be the midpoint of this line.
            glBegin(GL_LINES);
            glVertex3fv(midVec12.getValue());
            glVertex3fv(midVec34.getValue());
            glEnd();

            // Render the label
            tmpVec = (midVec12 + midVec34) * 0.5f;
            sprintf(buf, torsionalLabelFormat.getValue().getString(), theta);
            torsionalLabels->text.set1Value(theLabel, buf);
            torsionalLabels->position.set1Value(theLabel, tmpVec);
            torsionalLabels->color.set1Value(theLabel, labelColor);
        }
        else {
            // Render the lines from the "midpoints" of the 2 end vectors
            // to the arc
            glBegin(GL_LINES);
            glVertex3fv(midVec12.getValue());
            glVertex3fv(vertices[0].getValue());
            glVertex3fv(midVec34.getValue());
            glVertex3fv(vertices[numVertices-1].getValue());
            glEnd();

            // Render the arc
            tmpVec = (coords3 - coords2) * 0.5f + coords2;
            glBegin(GL_LINE_LOOP);
            glVertex3fv(tmpVec.getValue());
            for (j = 0; j < numVertices; j++) {
                glVertex3fv(vertices[j].getValue());
            }
            glEnd();

            // coords1 (and tmpVec) is on the bisector of theta
            float theta2 = theta * 0.5;
            coords1.setValue(sin(theta2), cos(theta2), 0.0);
            theVertexMatrix.multVecMatrix(coords1, tmpVec);

            // Generate the label
            if (theta < 0.0) theta = -theta;
            theta *= (float)RAD_TO_DEG;
            sprintf(buf, torsionalLabelFormat.getValue().getString(), theta);
            torsionalLabels->text.set1Value(theLabel, buf);
            torsionalLabels->position.set1Value(theLabel, tmpVec);
            torsionalLabels->color.set1Value(theLabel, labelColor);
        }
    LABELLOOP_END

    if (vertices != NULL) delete [] vertices;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Compute bounding box for the distance monitor geoms
//
// Use: protected

void
ChemMonitor::computeDistanceBBox(SoAction *action, SbBox3f &box,
    SbVec3f &center)
//
////////////////////////////////////////////////////////////////////////
{
    int32_t i;
    int32_t from, to;
    SbVec3f coords1, coords2;
    SbVec3f lCenter;
    lCenter.setValue(0.0, 0.0, 0.0);

    int32_t count = 0;
    SbAtomSpec atomSpecFrom, atomSpecTo;
    ChemBaseData *chemDataFrom, *chemDataTo;
    ChemDisplay  *chemDisplayFrom, *chemDisplayTo;
    SbMatrix matrixFrom, matrixTo;

    for (i = 0; i < distance.getNum(); i++) {
        distance[i].getValue(atomSpecFrom, atomSpecTo);
        atomSpecFrom.getValue(chemDataFrom, chemDisplayFrom, from);
        atomSpecTo.getValue(chemDataTo, chemDisplayTo, to);

        coords1 = chemDataFrom->getAtomCoordinates(from).getValue();
        coords2 = chemDataTo->getAtomCoordinates(to).getValue();
    
        matrixFrom = chemDisplayFrom->getCurrentModelMatrix();
        matrixTo = chemDisplayTo->getCurrentModelMatrix();
        matrixFrom.multVecMatrix(coords1, coords1);
        matrixTo.multVecMatrix(coords2, coords2);

        // Remove the affect of any model matrices applied to
        // ChemMonitor
        currentModelMatrixInverse.multVecMatrix(coords1, coords1);
        currentModelMatrixInverse.multVecMatrix(coords2, coords2);

        box.extendBy(coords1);
        box.extendBy(coords2);
        lCenter += coords1;
        lCenter += coords2;
        count += 2;
    }
    if (count > 0) {
        center += lCenter / (float)count;
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Compute bounding box for the angle monitor geoms
//
// Use: protected

void
ChemMonitor::computeAngleBBox(SoAction *action, SbBox3f &box,
    SbVec3f &center)
//
////////////////////////////////////////////////////////////////////////
{
    int32_t i, j;
    int32_t atom1, atom2, atom3;
    SbVec3f coords1, coords2, coords3;
    SbVec3f lCenter;
    lCenter.setValue(0.0, 0.0, 0.0);
    
    float theta;
    int32_t numVertices;
    SbVec3f *vertices = NULL;
    SbMatrix theVertexMatrix;

    int32_t count = 0;
    SbAtomSpec atomSpec1, atomSpec2, atomSpec3; 
    ChemBaseData *chemData1, *chemData2, *chemData3;
    ChemDisplay  *chemDisplay1, *chemDisplay2, *chemDisplay3;
    SbMatrix matrix1, matrix2, matrix3;

    for (i = 0; i < angle.getNum(); i++) {
        angle[i].getValue(atomSpec1, atomSpec2, atomSpec3);
        atomSpec1.getValue(chemData1, chemDisplay1, atom1);
        atomSpec2.getValue(chemData2, chemDisplay2, atom2);
        atomSpec3.getValue(chemData3, chemDisplay3, atom3);

        coords1 = chemData1->getAtomCoordinates(atom1).getValue();
        coords2 = chemData2->getAtomCoordinates(atom2).getValue();
        coords3 = chemData3->getAtomCoordinates(atom3).getValue();
    
        matrix1 = chemDisplay1->getCurrentModelMatrix();
        matrix2 = chemDisplay2->getCurrentModelMatrix();
        matrix3 = chemDisplay3->getCurrentModelMatrix();

        matrix1.multVecMatrix(coords1, coords1);
        matrix2.multVecMatrix(coords2, coords2);
        matrix3.multVecMatrix(coords3, coords3);

        // Remove the affect of any model matrices applied to
        // ChemMonitor
        currentModelMatrixInverse.multVecMatrix(coords1, coords1);
        currentModelMatrixInverse.multVecMatrix(coords2, coords2);
        currentModelMatrixInverse.multVecMatrix(coords3, coords3);

        box.extendBy(coords1);
        box.extendBy(coords2);
        box.extendBy(coords3);
        lCenter += coords1;
        lCenter += coords2;
        lCenter += coords3;
        count += 3;

        // Get the vertices used in the arc
        generateAngleVertices(coords1, coords2, coords3, 
            angleArcScale.getValue(),
            theta, numVertices, vertices, theVertexMatrix);

        for (j = 0; j < numVertices; j++) {
            box.extendBy(vertices[j]);
            lCenter += vertices[j];
        }
        count += numVertices;
    }
    if (count > 0) {
        center += lCenter / (float)count;
    }
    if (vertices != NULL) delete [] vertices;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Compute bounding box for the torsional monitor geoms
//
// Use: protected

void
ChemMonitor::computeTorsionalBBox(SoAction *action, SbBox3f &box,
    SbVec3f &center)
//
////////////////////////////////////////////////////////////////////////
{
    int32_t i, j;
    int32_t atom1, atom2, atom3, atom4;
    SbVec3f coords1, coords2, coords3, coords4;
    SbVec3f lCenter;
    lCenter.setValue(0.0, 0.0, 0.0);

    float theta;
    int32_t numVertices;
    SbVec3f *vertices = NULL;
    SbVec3f midVec12, midVec34;
    SbMatrix theVertexMatrix;

    int32_t count = 0;
    SbAtomSpec atomSpec1, atomSpec2, atomSpec3, atomSpec4;
    ChemBaseData *chemData1, *chemData2, *chemData3, *chemData4;
    ChemDisplay  *chemDisplay1, *chemDisplay2, *chemDisplay3, *chemDisplay4;
    SbMatrix matrix1, matrix2, matrix3, matrix4;
    SbBool hasLinearVectors;

    for (i = 0; i < torsional.getNum(); i++) {
        torsional[i].getValue(atomSpec1, atomSpec2, atomSpec3, atomSpec4);
        atomSpec1.getValue(chemData1, chemDisplay1, atom1);
        atomSpec2.getValue(chemData2, chemDisplay2, atom2);
        atomSpec3.getValue(chemData3, chemDisplay3, atom3);
        atomSpec4.getValue(chemData4, chemDisplay4, atom4);

        coords1 = chemData1->getAtomCoordinates(atom1).getValue();
        coords2 = chemData2->getAtomCoordinates(atom2).getValue();
        coords3 = chemData3->getAtomCoordinates(atom3).getValue();
        coords4 = chemData4->getAtomCoordinates(atom4).getValue();
    
        matrix1 = chemDisplay1->getCurrentModelMatrix();
        matrix2 = chemDisplay2->getCurrentModelMatrix();
        matrix3 = chemDisplay3->getCurrentModelMatrix();
        matrix4 = chemDisplay4->getCurrentModelMatrix();

        matrix1.multVecMatrix(coords1, coords1);
        matrix2.multVecMatrix(coords2, coords2);
        matrix3.multVecMatrix(coords3, coords3);
        matrix4.multVecMatrix(coords4, coords4);

        // Remove the affect of any model matrices applied to
        // ChemMonitor
        currentModelMatrixInverse.multVecMatrix(coords1, coords1);
        currentModelMatrixInverse.multVecMatrix(coords2, coords2);
        currentModelMatrixInverse.multVecMatrix(coords3, coords3);
        currentModelMatrixInverse.multVecMatrix(coords4, coords4);

        box.extendBy(coords1);
        box.extendBy(coords2);
        box.extendBy(coords3);
        box.extendBy(coords4);
        lCenter += coords1;
        lCenter += coords2;
        lCenter += coords3;
        lCenter += coords4;
        count += 4;

        // Get the vertices used in the arc
        generateTorsionalVertices(coords1, coords2, coords3, coords4,
            torsionalArcScale.getValue(), hasLinearVectors,
            theta, numVertices, vertices, theVertexMatrix,
            midVec12, midVec34);

        // If hasLinearVectors is TRUE, then there will be a line segment
        // from midVec12 to midVec34.  This line segment should already
        // be in a bounding box.  So just do the arc if necessary.
        if (!hasLinearVectors) {
            for (j = 0; j < numVertices; j++) {
                box.extendBy(vertices[j]);
                lCenter += vertices[j];
            }
            count += numVertices;
        }
    }
    if (count > 0) {
        center += lCenter / (float)count;
    }
    if (vertices != NULL) delete [] vertices;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Implements ray picking of the distance geoms
//
// Use: private

void
ChemMonitor::pickDistanceGeoms(SoRayPickAction *action, SbVec3f &norm)
//
////////////////////////////////////////////////////////////////////////
{
    int32_t i;
    int32_t from, to;
    SbVec3f coords1, coords2;

    SbVec3f point;
    SoPickedPoint *pp;
    ChemMonitorDetail *detail;

    SbAtomSpec atomSpecFrom, atomSpecTo;
    ChemBaseData *chemDataFrom, *chemDataTo;
    ChemDisplay  *chemDisplayFrom, *chemDisplayTo;
    SbMatrix matrixFrom, matrixTo;

    for (i = 0; i < distance.getNum(); i++) {
        if (drawDistanceLine[i] == TRUE) {
            distance[i].getValue(atomSpecFrom, atomSpecTo);
            atomSpecFrom.getValue(chemDataFrom, chemDisplayFrom, from);
            atomSpecTo.getValue(chemDataTo, chemDisplayTo, to);
    
            coords1 = chemDataFrom->getAtomCoordinates(from).getValue();
            coords2 = chemDataTo->getAtomCoordinates(to).getValue();
    
            matrixFrom = chemDisplayFrom->getCurrentModelMatrix();
            matrixTo = chemDisplayTo->getCurrentModelMatrix();
            matrixFrom.multVecMatrix(coords1, coords1);
            matrixTo.multVecMatrix(coords2, coords2);

            // Remove the affect of any model matrices applied to
            // ChemMonitor
            currentModelMatrixInverse.multVecMatrix(coords1, coords1);
            currentModelMatrixInverse.multVecMatrix(coords2, coords2);

            if (action->intersect(coords1, coords2, point) &&
                (pp = action->addIntersection(point)) != NULL) {
                pp->setObjectNormal(norm);
                pp->setObjectTextureCoords(SbVec4f(0.0, 0.0, 0.0, 0.0));
                pp->setMaterialIndex(0);
                detail = new ChemMonitorDetail;
                detail->setDistanceIndex(i);
                pp->setDetail(detail, this);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Implements ray picking of the angle geoms
//
// Use: private

void
ChemMonitor::pickAngleGeoms(SoRayPickAction *action, SbVec3f &norm)
//
////////////////////////////////////////////////////////////////////////
{
    int32_t i, j;
    int32_t atom1, atom2, atom3;
    float theta;
    SbVec3f coords1, coords2, coords3;

    int32_t numVertices;
    SbVec3f *vertices = NULL;
    SbMatrix theVertexMatrix;

    SbVec3f point;
    SoPickedPoint *pp;
    ChemMonitorDetail *detail;

    SbAtomSpec atomSpec1, atomSpec2, atomSpec3;
    ChemBaseData *chemData1, *chemData2, *chemData3;
    ChemDisplay  *chemDisplay1, *chemDisplay2, *chemDisplay3;
    SbMatrix matrix1, matrix2, matrix3;

    for (i = 0; i < angle.getNum(); i++) {
        angle[i].getValue(atomSpec1, atomSpec2, atomSpec3);
        atomSpec1.getValue(chemData1, chemDisplay1, atom1);
        atomSpec2.getValue(chemData2, chemDisplay2, atom2);
        atomSpec3.getValue(chemData3, chemDisplay3, atom3);

        coords1 = chemData1->getAtomCoordinates(atom1).getValue();
        coords2 = chemData2->getAtomCoordinates(atom2).getValue();
        coords3 = chemData3->getAtomCoordinates(atom3).getValue();

        matrix1 = chemDisplay1->getCurrentModelMatrix();
        matrix2 = chemDisplay2->getCurrentModelMatrix();
        matrix3 = chemDisplay3->getCurrentModelMatrix();

        matrix1.multVecMatrix(coords1, coords1);
        matrix2.multVecMatrix(coords2, coords2);
        matrix3.multVecMatrix(coords3, coords3);

        // Remove the affect of any model matrices applied to
        // ChemMonitor
        currentModelMatrixInverse.multVecMatrix(coords1, coords1);
        currentModelMatrixInverse.multVecMatrix(coords2, coords2);
        currentModelMatrixInverse.multVecMatrix(coords3, coords3);

        // Generate the vertices making up the arc.  Also get theta
        // and theVertexMatrix
        generateAngleVertices(coords1, coords2, coords3,
            angleArcScale.getValue(),
            theta, numVertices, vertices, theVertexMatrix);

        // Check the sides of the angle
        if (drawAngleLines[i] == TRUE) {
            if (action->intersect(coords1, coords2, point) &&
                (pp = action->addIntersection(point)) != NULL) {
                pp->setObjectNormal(norm);
                pp->setObjectTextureCoords(SbVec4f(0.0, 0.0, 0.0, 0.0));
                pp->setMaterialIndex(0);
                detail = new ChemMonitorDetail;
                detail->setAngleIndex(i);
                pp->setDetail(detail, this);
            }
            if (action->intersect(coords2, coords3, point) &&
                (pp = action->addIntersection(point)) != NULL) {
                pp->setObjectNormal(norm);
                pp->setObjectTextureCoords(SbVec4f(0.0, 0.0, 0.0, 0.0));
                pp->setMaterialIndex(0);
                detail = new ChemMonitorDetail;
                detail->setAngleIndex(i);
                pp->setDetail(detail, this);
            }
        }

        // Check the arc
        for (j = 0; j < numVertices-1; j++) {
            if (action->intersect(vertices[j], vertices[j+1], point) &&
                (pp = action->addIntersection(point)) != NULL) {
                pp->setObjectNormal(norm);
                pp->setObjectTextureCoords(SbVec4f(0.0, 0.0, 0.0, 0.0));
                pp->setMaterialIndex(0);
                detail = new ChemMonitorDetail;
                detail->setAngleIndex(i);
                pp->setDetail(detail, this);
            }
        }
    }
    if (vertices != NULL) delete [] vertices;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Implements ray picking of the torsional geoms
//
// Use: private

void
ChemMonitor::pickTorsionalGeoms(SoRayPickAction *action, SbVec3f &norm)
//
////////////////////////////////////////////////////////////////////////
{
    int32_t i, j;
    float   theta;
    int32_t atom1, atom2, atom3, atom4;
    SbVec3f coords1, coords2, coords3, coords4;
    SbVec3f midVec12, midVec34, tmpVec;

    int32_t numVertices;
    SbVec3f *vertices = NULL;
    SbMatrix theVertexMatrix;

    SbVec3f point;
    SoPickedPoint *pp;
    ChemMonitorDetail *detail;

    SbAtomSpec atomSpec1, atomSpec2, atomSpec3, atomSpec4;
    ChemBaseData *chemData1, *chemData2, *chemData3, *chemData4;
    ChemDisplay  *chemDisplay1, *chemDisplay2, *chemDisplay3, *chemDisplay4;
    SbMatrix matrix1, matrix2, matrix3, matrix4;
    SbBool hasLinearVectors;

    for (i = 0; i < torsional.getNum(); i++) {
        torsional[i].getValue(atomSpec1, atomSpec2, atomSpec3, atomSpec4);
        atomSpec1.getValue(chemData1, chemDisplay1, atom1);
        atomSpec2.getValue(chemData2, chemDisplay2, atom2);
        atomSpec3.getValue(chemData3, chemDisplay3, atom3);
        atomSpec4.getValue(chemData4, chemDisplay4, atom4);

        coords1 = chemData1->getAtomCoordinates(atom1).getValue();
        coords2 = chemData2->getAtomCoordinates(atom2).getValue();
        coords3 = chemData3->getAtomCoordinates(atom3).getValue();
        coords4 = chemData4->getAtomCoordinates(atom4).getValue();

        matrix1 = chemDisplay1->getCurrentModelMatrix();
        matrix2 = chemDisplay2->getCurrentModelMatrix();
        matrix3 = chemDisplay3->getCurrentModelMatrix();
        matrix4 = chemDisplay4->getCurrentModelMatrix();

        matrix1.multVecMatrix(coords1, coords1);
        matrix2.multVecMatrix(coords2, coords2);
        matrix3.multVecMatrix(coords3, coords3);
        matrix4.multVecMatrix(coords4, coords4);

        // Generate the vertices making up the arc.  Also get theta
        // and theVertexMatrix
        currentModelMatrixInverse.multVecMatrix(coords1, coords1);
        currentModelMatrixInverse.multVecMatrix(coords2, coords2);
        currentModelMatrixInverse.multVecMatrix(coords3, coords3);
        currentModelMatrixInverse.multVecMatrix(coords4, coords4);

        // Generate the vertices making up the arc.  Also get theta
        // and theVertexMatrix
        generateTorsionalVertices(coords1, coords2, coords3, coords4,
            torsionalArcScale.getValue(), hasLinearVectors,
            theta, numVertices, vertices, theVertexMatrix,
            midVec12, midVec34);

        // Check the lines connecting the atoms involved in the
        // description of the torsional
        if (drawTorsionalLines[i] == TRUE) {
            if (action->intersect(coords1, coords2, point) &&
                (pp = action->addIntersection(point)) != NULL) {
                pp->setObjectNormal(norm);
                pp->setObjectTextureCoords(SbVec4f(0.0, 0.0, 0.0, 0.0));
                pp->setMaterialIndex(0);
                detail = new ChemMonitorDetail;
                detail->setTorsionalIndex(i);
                pp->setDetail(detail, this);
            }
            if (action->intersect(coords2, coords3, point) &&
                (pp = action->addIntersection(point)) != NULL) {
                pp->setObjectNormal(norm);
                pp->setObjectTextureCoords(SbVec4f(0.0, 0.0, 0.0, 0.0));
                pp->setMaterialIndex(0);
                detail = new ChemMonitorDetail;
                detail->setTorsionalIndex(i);
                pp->setDetail(detail, this);
            }
            if (action->intersect(coords3, coords4, point) &&
                (pp = action->addIntersection(point)) != NULL) {
                pp->setObjectNormal(norm);
                pp->setObjectTextureCoords(SbVec4f(0.0, 0.0, 0.0, 0.0));
                pp->setMaterialIndex(0);
                detail = new ChemMonitorDetail;
                detail->setTorsionalIndex(i);
                pp->setDetail(detail, this);
            }
        }

        // If hasLinearVectors is TRUE, then we need to check the
        // line segment from midVec12 to midVec34.
        // If hasLinearVectors is FALSE, then we need to check the arc
        // and the lines to the arc.
        if (hasLinearVectors) {
            if (action->intersect(midVec12, midVec34, point) &&
                (pp = action->addIntersection(point)) != NULL) {
                pp->setObjectNormal(norm);
                pp->setObjectTextureCoords(SbVec4f(0.0, 0.0, 0.0, 0.0));
                pp->setMaterialIndex(0);
                detail = new ChemMonitorDetail;
                detail->setTorsionalIndex(i);
                pp->setDetail(detail, this);
            }
        }
        else {
            // Check the lines from the "midpoints" of the 2 end vectors
            // to the arc
            if (action->intersect(midVec12, vertices[0], point) &&
                (pp = action->addIntersection(point)) != NULL) {
                pp->setObjectNormal(norm);
                pp->setObjectTextureCoords(SbVec4f(0.0, 0.0, 0.0, 0.0));
                pp->setMaterialIndex(0);
                detail = new ChemMonitorDetail;
                detail->setTorsionalIndex(i);
                pp->setDetail(detail, this);
            }
            if (action->intersect(midVec34, vertices[numVertices-1], point) &&
                (pp = action->addIntersection(point)) != NULL) {
                pp->setObjectNormal(norm);
                pp->setObjectTextureCoords(SbVec4f(0.0, 0.0, 0.0, 0.0));
                pp->setMaterialIndex(0);
                detail = new ChemMonitorDetail;
                detail->setTorsionalIndex(i);
                pp->setDetail(detail, this);
            }

            // Check the arc
            tmpVec = (coords3 - coords2) * 0.5f + coords2;
            if (action->intersect(tmpVec, vertices[0], point) &&
                (pp = action->addIntersection(point)) != NULL) {
                pp->setObjectNormal(norm);
                pp->setObjectTextureCoords(SbVec4f(0.0, 0.0, 0.0, 0.0));
                pp->setMaterialIndex(0);
                detail = new ChemMonitorDetail;
                detail->setTorsionalIndex(i);
                pp->setDetail(detail, this);
            }
            if (action->intersect(tmpVec, vertices[numVertices-1], point) &&
                (pp = action->addIntersection(point)) != NULL) {
                pp->setObjectNormal(norm);
                pp->setObjectTextureCoords(SbVec4f(0.0, 0.0, 0.0, 0.0));
                pp->setMaterialIndex(0);
                detail = new ChemMonitorDetail;
                detail->setTorsionalIndex(i);
                pp->setDetail(detail, this);
            }
            for (j = 0; j < numVertices-1; j++) {
                if (action->intersect(vertices[j], vertices[j+1], point) &&
                    (pp = action->addIntersection(point)) != NULL) {
                    pp->setObjectNormal(norm);
                    pp->setObjectTextureCoords(SbVec4f(0.0, 0.0, 0.0, 0.0));
                    pp->setMaterialIndex(0);
                    detail = new ChemMonitorDetail;
                    detail->setTorsionalIndex(i);
                    pp->setDetail(detail, this);
                }
            }
        }
    }
    if (vertices != NULL) delete [] vertices;
}


#define END_OF_LIST 9999999

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Removes the values in "newIndex" from "listIndex".
//
// Use: static, internal

static void
removeIndices(MFVec2i &listIndex, MFVec2i &newIndex, int32_t numberOfItems)
//
////////////////////////////////////////////////////////////////////////
{
    int32_t end_indicator = -1;

    int32_t lastStart, lastIncr, count;
    int32_t index;

    int32_t numLoops, loop;
    int32_t start, end, item;

    // Make a list of all the unique entries in listIndex and newIndex.
    if ((numLoops = listIndex.getNum()) > 0) {
        ChemIntList theList(numberOfItems);

        for (loop = 0; loop < numLoops; loop++) {
            listIndex[loop].getValue(start, end);
            if (end == end_indicator) {
                end = numberOfItems;
            }
            else {
                end += start;
            }
            for (item = start; item < end; item++) {
                theList.append(item);
            }
        }

        // Sort theList in preparation for the searches.
        theList.sort();
        int32_t listStart = 0;
        int32_t listEnd = theList.getLength() - 1;

        if ((numLoops = newIndex.getNum()) > 0) {
            for (loop = 0; loop < numLoops; loop++) {
                newIndex[loop].getValue(start, end);
                if (end == end_indicator) {
                    end = numberOfItems;
                }
                else {
                    end += start;
                }
                for (item = start; item < end; item++) {
                    if ((index = theList.search(&item, listStart, listEnd))
                            != -1) {
                        theList[index] = END_OF_LIST;
                        listStart = index + 1;   
                    }
                }
            }
        }
    
        // Sort theList.  All END_OF_LIST's should be at the end.
        // Find the first of these and truncate the list.
        theList.sort();
        if ((index = theList.find(END_OF_LIST)) != -1) theList.truncate(index);
        
        // Remake listIndex using contents of theList.
        lastStart = -1;
        lastIncr = 0;
        count = 0;
        end = theList.getLength();  
        listIndex.deleteValues(0);
        for (loop = 0; loop < end; loop++) {
            item = theList[loop];
            if (lastStart == -1) {
                lastStart = item;
                lastIncr = 1;
            }
            else if (item == lastStart + lastIncr) {
                lastIncr++;
            }
            else {
                listIndex.set1Value(count++, SbVec2i(lastStart, lastIncr));
                lastStart = item;
                lastIncr = 1;
            }
        }
        if (lastStart != -1) {
            listIndex.set1Value(count, SbVec2i(lastStart, lastIncr));
        }
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Checks to see if the selectionList has changed and if so, regenerates
//    the internal sets of indices.
//                      
// Use: private

void
ChemMonitor::generateIndices(SoAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    SoState *state = action->getState();
    SbBool selectionChanged = FALSE;

    if (doHighlighting.getValue() == FALSE) {
        if ((highlightDistanceIndex.getNum() > 0) ||
            (highlightAngleIndex.getNum() > 0) ||
            (highlightTorsionalIndex.getNum() > 0)) {
            resetIndices();
        }
        if (lastSelectionElement != NULL) {
            delete lastSelectionElement;
            lastSelectionElement = NULL;
        }
        return;
    }

    const ChemMonitorSelectionElement *cmse =
        ChemMonitorSelectionElement::getInstance(state);

    // If there is no ChemMonitorSelectionElement in the state, check to see if
    // there used to be a ChemMonitorSelectionElement in the state.  If there
    // was, then set selectionChanged to TRUE.
    if ((cmse == NULL) && (lastSelectionElement != NULL)) {
        selectionChanged = TRUE;
    }

    if (!selectionChanged) {
        if (cmse != NULL) {
    
            // Get the selectionList from the ChemMonitorSelectionElement.  If the
            // data changed, then the ChemSelection node would have reset the
            // ChemMonitorSelectionElement to contain no selections.  If there
            // previously had been selections, then the
            // lastSelectionElement would not match and things would be
            // OK.  If there had not been any previous selections, then
            // things would match and we would still be OK.
    
            //const ChemMonitorPathList &selectionList = cmse->getList();
            if (lastSelectionElement == NULL) {
                selectionChanged = TRUE;
                lastSelectionElement =
                    (ChemMonitorSelectionElement *)cmse->copyMatchInfo();
            }
            else {
                if (lastSelectionElement->matches(cmse)) {
                    selectionChanged = FALSE;
                }
                else {
                    selectionChanged = TRUE;
                    delete lastSelectionElement;
                    lastSelectionElement =
                        (ChemMonitorSelectionElement *)cmse->copyMatchInfo();
                }
            }
        }
    }

    if (selectionChanged) {
        // If the selection changed, the first thing to do is to reset all
        // the "normal" indices.
        resetIndices();

        // If there is a ChemMonitorSeletionElement, get the lists of selected items
        // and fill out the highlighted indices.
        if (cmse != NULL) {
            const ChemMonitorPathList &selectionList = cmse->getList();
            int32_t length = selectionList.getLength();
            int32_t i;
            for (i = 0; i < length; i++) {
                if (((ChemPath *)selectionList[i])->isOfType(ChemMonitorPath::getClassTypeId())) {
                    const ChemMonitorPath *si =
                        (ChemMonitorPath *)selectionList[i];
                    SoFullPath *path = (SoFullPath *)si->getSoPath();
                    if (path->getTail() == this) {
                        const MFVec2i &slDistanceIndex =
                            si->getDistanceIndex();
                        highlightDistanceIndex.setValues(0,
                            slDistanceIndex.getNum(),
                            slDistanceIndex.getValues(0));

                        const MFVec2i &slAngleIndex =
                            si->getAngleIndex();
                        highlightAngleIndex.setValues(0,
                            slAngleIndex.getNum(),
                            slAngleIndex.getValues(0));

                        const MFVec2i &slTorsionalIndex =
                            si->getTorsionalIndex();
                        highlightTorsionalIndex.setValues(0,
                            slTorsionalIndex.getNum(),
                            slTorsionalIndex.getValues(0));

                        break;
                    }
                }
            }

            // Remove the values in the "highlight" indices from the values
            // in the "normal" indices.

            int32_t numIndices;
            if (highlightDistanceIndex.getNum() > 0) {
                numIndices = distance.getNum();
                removeIndices(normalDistanceIndex, highlightDistanceIndex,
                    numIndices);
            }
            if (highlightAngleIndex.getNum() > 0) {
                numIndices = angle.getNum();
                removeIndices(normalAngleIndex, highlightAngleIndex,
                    numIndices);
            }
            if (highlightTorsionalIndex.getNum() > 0) {
                numIndices = torsional.getNum();
                removeIndices(normalTorsionalIndex, highlightTorsionalIndex,
                    numIndices);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Reset the values of the various indices back to their default state.
//
// Use: private

void
ChemMonitor::resetIndices()
//
////////////////////////////////////////////////////////////////////////
{
    highlightDistanceIndex.deleteValues(0);
    highlightAngleIndex.deleteValues(0);
    highlightTorsionalIndex.deleteValues(0);

    normalDistanceIndex.setValue(0, -1);
    normalAngleIndex.setValue(0, -1);
    normalTorsionalIndex.setValue(0, -1);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Delete the entries in the label text and position fields
//
// Use: private

void
ChemMonitor::resetLabels()
//
////////////////////////////////////////////////////////////////////////
{
    if (distanceLabels != NULL) {
        distanceLabels->position.deleteValues(0);
        distanceLabels->text.deleteValues(0);
    }
    if (angleLabels != NULL) {
        angleLabels->position.deleteValues(0);
        angleLabels->text.deleteValues(0);
    }
    if (torsionalLabels != NULL) {
        torsionalLabels->position.deleteValues(0);
        torsionalLabels->text.deleteValues(0);
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Get the individual bounding boxes.  This is the method invoked
//    by ChemBBoxaction.  It just returns the individual bounding
//    boxes of the monitor labels.
//
// Use: public

void
ChemMonitor::getChemMonitorBBoxes(SoAction *action, SbBool clipOnCenter,
    ChemMonitorBBox *&chemMonitorBBoxes)
//
////////////////////////////////////////////////////////////////////////
{
    if ((int)SoPickStyleElement::get(action->getState()) ==
        (int)SoPickStyle::UNPICKABLE) {
        return;
    }

    int32_t numBoxes = 0;
    ChemLabelBBox *tmpDistanceBBoxes = NULL;
    ChemLabelBBox *tmpAngleBBoxes = NULL;
    ChemLabelBBox *tmpTorsionalBBoxes = NULL;

    // Since some of the labels may be clipped, numBoxes is incremented
    // based on the number of labels returned by the getChemLabelBBoxes
    // invocations.
    if (distance.getNum() > 0) {
        distanceLabels->getChemLabelBBoxes(action, clipOnCenter, tmpDistanceBBoxes);
        numBoxes += tmpDistanceBBoxes->numberOfLabels;
    }

    if (angle.getNum() > 0) {
        angleLabels->getChemLabelBBoxes(action, clipOnCenter, tmpAngleBBoxes);
        numBoxes += tmpAngleBBoxes->numberOfLabels;
    }

    if (torsional.getNum() > 0) {
        torsionalLabels->getChemLabelBBoxes(action, clipOnCenter, tmpTorsionalBBoxes);
        numBoxes += tmpTorsionalBBoxes->numberOfLabels;
    }

    // Copy the results of the ChemLabel getChemLabelBBoxes invocations
    // to the chemMonitorBBoxes removing any model matrix.
    chemMonitorBBoxes = NULL;
    if (numBoxes > 0) {
        int32_t numDistanceLabels = 0;
        int32_t numAngleLabels = 0;
        int32_t numTorsionalLabels = 0;
        if (tmpDistanceBBoxes != NULL) {
            numDistanceLabels = tmpDistanceBBoxes->numberOfLabels;
        }
        if (tmpAngleBBoxes != NULL) {
            numAngleLabels = tmpAngleBBoxes->numberOfLabels;
        }
        if (tmpTorsionalBBoxes != NULL) {
            numTorsionalLabels = tmpTorsionalBBoxes->numberOfLabels;
        }
        chemMonitorBBoxes =
            new ChemMonitorBBox(numDistanceLabels, numAngleLabels,
                                numTorsionalLabels);
        int32_t i;
        //SbVec3f boxMin, boxMax;
        if (tmpDistanceBBoxes != NULL) {
            for (i = 0; i < numDistanceLabels; i++) {
                chemMonitorBBoxes->distanceIndex[i] = tmpDistanceBBoxes->index[i];
                chemMonitorBBoxes->distanceBBox[i] = tmpDistanceBBoxes->bbox[i];
                chemMonitorBBoxes->distanceCenter[i] = tmpDistanceBBoxes->center[i];
            }
            delete tmpDistanceBBoxes;
        }
        if (tmpAngleBBoxes != NULL) {
            for (i = 0; i < tmpAngleBBoxes->numberOfLabels; i++) {
                chemMonitorBBoxes->angleIndex[i] = tmpAngleBBoxes->index[i];
                chemMonitorBBoxes->angleBBox[i] = tmpAngleBBoxes->bbox[i];
                chemMonitorBBoxes->angleCenter[i] = tmpAngleBBoxes->center[i];
            }
            delete tmpAngleBBoxes;
        }
        if (tmpTorsionalBBoxes != NULL) {
            for (i = 0; i < tmpTorsionalBBoxes->numberOfLabels; i++) {
                chemMonitorBBoxes->torsionalIndex[i] = tmpTorsionalBBoxes->index[i];
                chemMonitorBBoxes->torsionalBBox[i] = tmpTorsionalBBoxes->bbox[i];
                chemMonitorBBoxes->torsionalCenter[i] = tmpTorsionalBBoxes->center[i];
            }
            delete tmpTorsionalBBoxes;
        }
    }
}

#undef END_OF_LIST
#undef ABS
#undef MIN
#undef RAD_TO_DEG
