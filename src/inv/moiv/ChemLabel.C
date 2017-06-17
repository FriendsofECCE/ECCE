/*
 * ChemLabel.c++
 *
 *     Classes:  ChemLabel
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

#include <assert.h>
#include <ctype.h>

/*
	Porting to Windows:
	Daniel Soto	2002/02/13
*/
#ifdef WIN32
#include <windows.h>
#endif

#include <math.h>

#include "inv/SbBox.H"
#include "inv/SoPickedPoint.H"
#include "inv/SoPrimitiveVertex.H"

#include "inv/actions/SoCallbackAction.H"
#include "inv/actions/SoGLRenderAction.H"
#include "inv/actions/SoRayPickAction.H"
#include "inv/caches/SoGLRenderCache.H"

#include "inv/details/SoTextDetail.H"

#include "inv/elements/SoCacheElement.H"
#include "inv/elements/SoClipPlaneElement.H"
#include "inv/elements/SoFontNameElement.H"
#include "inv/elements/SoFontSizeElement.H"
#include "inv/elements/SoLightModelElement.H"
#include "inv/elements/SoGLCacheContextElement.H"
#include "inv/elements/SoGLDrawStyleElement.H"
#include "inv/elements/SoGLTextureCoordinateElement.H"
#include "inv/elements/SoGLTextureEnabledElement.H"
#include "inv/elements/SoGLTextureImageElement.H"
#include "inv/elements/SoModelMatrixElement.H"
#include "inv/elements/SoPickStyleElement.H"
#include "inv/elements/SoProjectionMatrixElement.H"
#include "inv/elements/SoViewingMatrixElement.H"
#include "inv/elements/SoViewportRegionElement.H"
#include "inv/elements/SoViewVolumeElement.H"

// Daniel Soto	2002-02-15
// FIX lighting-off with Coin3D
#ifdef __coin
// --> coin 1,2 compatibility
//#include "inv/elements/SoGLLightModelElement.H"
// <-- coin 1,2 compatibility
#endif

#ifdef IV2_0
#include "inv/bundles/SoMaterialBundle.H"
#else
#include "inv/elements/SoGLLazyElement.H"
#include "inv/elements/SoLazyElement.H"
#endif

#include "inv/flclient.h"
#include "inv/ChemKit/ChemBitmapFontCache.H"
#include "inv/ChemKit/ChemBBoxAction.H"
#include "inv/ChemKit/ChemIntList.H"
#include "inv/ChemKit/ChemLabel.H"
#include "inv/ChemKit/ChemLabelBBox.H"
#include "inv/ChemKit/ChemLabelPath.H"
#include "inv/ChemKit/ChemLabelPathList.H"
#include "inv/ChemKit/ChemLabelSelectionElement.H"
#include "inv/ChemKit/ChemSelectionElement.H"

/*
	Porting to Windows:
	Jamie Doornbos	2001/07/15
	Daniel Soto	2002/02/05
*/
#include "inv/ChemKit/FixFonts.H"

static int numLabelLoops = 0;
static int labelLoop = 0;
static int32_t labelStart = 0;
static int32_t labelEnd = 0;
static int32_t theLabel = 0;
   
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

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Static helper routine; checks to see if a point is inside
//    all of the clipping planes
//
// Use: internal, static

static SbBool
isPointBetweenPlanes(const SbVec3f &p0, const SbViewVolume &viewVol,
    const SoClipPlaneElement *clipPlaneElement)
// 
////////////////////////////////////////////////////////////////////////
{
    int32_t i;
    float t;   

    t = viewVol.getProjectionDirection().dot(
        p0 - viewVol.getProjectionPoint());
    if ((t < viewVol.getNearDist()) ||
        (t > viewVol.getNearDist() + viewVol.getDepth())) {
        return FALSE;
    }
        
    int32_t eltNum = clipPlaneElement->getNum();
    for (i = 0; i < eltNum; i++) {
        const SbPlane &thePlane = clipPlaneElement->get(i, TRUE);
        if (!thePlane.isInHalfSpace(p0)) {
            return FALSE;
        }
    }
    return TRUE;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Static helper routine; checks to see if a bounding box is inside
//    all of the clipping planes
//
// Use: internal, static

static SbBool
isBoxBetweenPlanes(const SbVec3f &p0, const SbVec3f &p1,
    const SbViewVolume &viewVol,
    const SoClipPlaneElement *clipPlaneElement)
// 
////////////////////////////////////////////////////////////////////////
{
    int32_t i;
    float t;   
    SbBool pointIn[2];
    
    pointIn[0] = pointIn[1] = TRUE;

    t = viewVol.getProjectionDirection().dot(
        p0 - viewVol.getProjectionPoint());
    if ((t < viewVol.getNearDist()) ||
        (t > viewVol.getNearDist() + viewVol.getDepth())) {
        pointIn[0] = FALSE;
    }
        
    t = viewVol.getProjectionDirection().dot(
        p1 - viewVol.getProjectionPoint());
    if ((t < viewVol.getNearDist()) ||
        (t > viewVol.getNearDist() + viewVol.getDepth())) {
        pointIn[1] = FALSE;
    }
        
    if ((!pointIn[0]) && (!pointIn[1])) return FALSE;

    int32_t eltNum = clipPlaneElement->getNum();
    for (i = 0; i < eltNum; i++) {
        const SbPlane &thePlane = clipPlaneElement->get(i, TRUE);
        if (!thePlane.isInHalfSpace(p0)) {
            pointIn[0] = FALSE;
        }
        if (!thePlane.isInHalfSpace(p1)) {
            pointIn[1] = FALSE;
        }
        if ((!pointIn[0]) && (!pointIn[1])) return FALSE;
    }
    return TRUE;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Static helper routine; given a vector in object space, a
//    transformation matrix to screen (normalized-device coordinate)
//    space, and a viewportRegion, returns the 2D pixel coordinates of
//    the vector (relative to the origin of the viewport).  This
//    actually returns a 3D vector; the z value is just the NDC z value.
//
// Use: internal, static

static SbVec3f
fromObjectSpace(const SbVec3f &vector, const SbMatrix &matrix,
                const SbViewportRegion &vpr)
//
////////////////////////////////////////////////////////////////////////
{
    // First, transform to NDC (-1 to 1 in viewport)
    SbVec3f ndc;
    matrix.multVecMatrix(vector, ndc);

    // And do the viewport transformation:
    SbVec2s vpSize = vpr.getViewportSizePixels();
    SbVec3f result;
    result[0] = (ndc[0]+1.0)*vpSize[0]/2.0;
    result[1] = (ndc[1]+1.0)*vpSize[1]/2.0;
    // Leave the z coordinate alone
    result[2] = ndc[2];

    return result;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Static helper routine; given 2D pixel coordinates, a
//    transformation matrix to object space, and a viewportRegion,
//    return the object-space point (assuming the 2D pixel is at z
//    coordinate 0).
//
// Use: internal, static

static SbVec3f
toObjectSpace(const SbVec3f &pixel, const SbMatrix &matrix,
              const SbViewportRegion &vpr)
//
////////////////////////////////////////////////////////////////////////
{
    // Viewport transformation, to normalized device coordinates:
    SbVec2s vpSize = vpr.getViewportSizePixels();
    SbVec3f ndc;
    ndc[0] = pixel[0]*2.0/vpSize[0] - 1.0;
    ndc[1] = pixel[1]*2.0/vpSize[1] - 1.0;
    ndc[2] = pixel[2];

    SbVec3f result;
    matrix.multVecMatrix(ndc, result);

    return result;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Returns the number of pixels the given line is offset from the
//    text origin.
//
// Use: internal, static

static SbVec3f
getPixelStringOffset(short lrJust, short tbJust,
    SbString label, ChemBitmapFontCache *normalFont)
//
////////////////////////////////////////////////////////////////////////
{
    float offset;
    SbVec3f result(0,0,0);

    if (lrJust == ChemLabel::LR_RIGHT) {
        offset = normalFont->getWidth(label);
        result[0] = -offset;
    }
    else if (lrJust == ChemLabel::LR_CENTER) {
        offset = normalFont->getWidth(label);
        result[0] = -offset * 0.5;
    }

    offset = normalFont->getHeight();
    if (tbJust == ChemLabel::TB_TOP) {
        result[1] = -offset;
    }
    else if (tbJust == ChemLabel::TB_MIDDLE) {
        result[1] = -offset * 0.5;
    }

    return result;
}

SO_NODE_SOURCE(ChemLabel);

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Constructor
//
// Use: public

ChemLabel::ChemLabel()
//
////////////////////////////////////////////////////////////////////////
{
    SO_NODE_CONSTRUCTOR(ChemLabel);

    SO_NODE_ADD_FIELD(fontName, (DEFAULT_FONT));
    SO_NODE_ADD_FIELD(fontSize, (10.0));
    SO_NODE_ADD_FIELD(color, (1.0, 1.0, 1.0));
    SO_NODE_ADD_FIELD(colorIndex, (0));
    SO_NODE_ADD_FIELD(colorBinding, (ChemLabel::OVERALL));
    SO_NODE_ADD_FIELD(doHighlighting, (TRUE));
    SO_NODE_ADD_FIELD(highlightColor, (0.3f, 0.3f, 0.0f));
    SO_NODE_ADD_FIELD(leftRightJustification, (ChemLabel::LR_DEFAULT));
    SO_NODE_ADD_FIELD(leftRightJustificationIndex, (0));
    SO_NODE_ADD_FIELD(leftRightJustificationBinding, (ChemLabel::OVERALL));
    SO_NODE_ADD_FIELD(topBottomJustification, (ChemLabel::TB_DEFAULT));
    SO_NODE_ADD_FIELD(topBottomJustificationIndex, (0));
    SO_NODE_ADD_FIELD(topBottomJustificationBinding, (ChemLabel::OVERALL));

    SO_NODE_ADD_FIELD(position, (0.0, 0.0, 0.0));
    SO_NODE_ADD_FIELD(text, (" "));

    SO_NODE_DEFINE_ENUM_VALUE(Binding, OVERALL);
    SO_NODE_DEFINE_ENUM_VALUE(Binding, PER_LABEL);
    SO_NODE_DEFINE_ENUM_VALUE(Binding, PER_LABEL_INDEXED);

    SO_NODE_SET_SF_ENUM_TYPE(colorBinding, Binding);
    SO_NODE_SET_SF_ENUM_TYPE(leftRightJustificationBinding, Binding);
    SO_NODE_SET_SF_ENUM_TYPE(topBottomJustificationBinding, Binding);

    SO_NODE_DEFINE_ENUM_VALUE(LeftRightJustification, LR_LEFT);
    SO_NODE_DEFINE_ENUM_VALUE(LeftRightJustification, LR_RIGHT);
    SO_NODE_DEFINE_ENUM_VALUE(LeftRightJustification, LR_CENTER);
    SO_NODE_DEFINE_ENUM_VALUE(LeftRightJustification, LR_DEFAULT);

    SO_NODE_SET_SF_ENUM_TYPE(leftRightJustification,
        LeftRightJustification);

    SO_NODE_DEFINE_ENUM_VALUE(TopBottomJustification, TB_TOP);
    SO_NODE_DEFINE_ENUM_VALUE(TopBottomJustification, TB_BOTTOM);
    SO_NODE_DEFINE_ENUM_VALUE(TopBottomJustification, TB_MIDDLE);
    SO_NODE_DEFINE_ENUM_VALUE(TopBottomJustification, TB_DEFAULT);

    SO_NODE_SET_SF_ENUM_TYPE(topBottomJustification,
        TopBottomJustification);

    normalFont = NULL;

    lastSelectionElement = NULL;
    resetIndices();

    currentMVP.makeIdentity();
    currentModelMatrix.makeIdentity();

    isBuiltIn = TRUE;
}

      
////////////////////////////////////////////////////////////////////////
//
// Description:
//    Destructor
//
// Use: private

ChemLabel::~ChemLabel()
//
////////////////////////////////////////////////////////////////////////
{
    if (normalFont != NULL) normalFont->unref();

    if (lastSelectionElement != NULL) {
        delete lastSelectionElement;
    }
}

////////////////////////////////////////////////////////////////////////
//  
// Description:
//    If a change was made to one of the text, position or
//    doHighlighting fields, then we need to reset some internal things.
//
// Use: internal
    
void
ChemLabel::notify(SoNotList *list)
//
////////////////////////////////////////////////////////////////////////
{
    if ((list->getLastRec()->getType() == SoNotRec::CONTAINER)) {
        SoField *field = list->getLastField();
        if ((field == &position) || (field == &text) ||
            (field == &doHighlighting)) {
            resetIndices();
        }
    }
    SoShape::notify(list);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Performs GL rendering of the text
//
// Use: public, extender
 
void
ChemLabel::GLRender(SoGLRenderAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    // Check to see if there is anything to render.
    if (text.getNum() == 0) return;

    SoState *state = action->getState();

    // Keep track of the MVP matrix.  Other actions need this
    // stored matrix (like ChemBBoxAction).
    currentMVP = SoProjectionMatrixElement::get(state);
    currentMVP = currentMVP.multLeft(SoViewingMatrixElement::get(state));
    currentModelMatrix = SoModelMatrixElement::get(state);
    currentMVP = currentMVP.multLeft(currentModelMatrix);
    currentVPR = SoViewportRegionElement::get(state);
    currentViewVolume = SoViewVolumeElement::get(state);

    // Setup the indices for normal and highlighted labels
    generateIndices(action);

    state->push();

    // Get a font cache we can pull stuff out of
    if (normalFont != NULL) {
        if (!normalFont->isRenderValid(action, state, fontName.getValue(),
            fontSize.getValue())) {
            normalFont->unref();
            normalFont = NULL;
        }
    }
    if (normalFont == NULL) {
        normalFont = ChemBitmapFontCache::getFont(action, state,
            fontName.getValue(), fontSize.getValue(), TRUE);
        if (normalFont == NULL) {
            state->pop();
            return;
        }
    }

    // Turn off lighting
#ifdef IV2_0
    SoLightModelElement::set(state, this, SoLightModelElement::BASE_COLOR);
#else
    SoGLLazyElement::setLightModel(state, SoLazyElement::BASE_COLOR);
    SoGLLazyElement::setColorMaterial(state, TRUE);
    SoGLLazyElement *lazyElt = (SoGLLazyElement *)
            SoLazyElement::getInstance(state);
    lazyElt->send(state, SoLazyElement::ALL_MASK);
#endif
// Daniel Soto	2002-02-15
// FIX lighting-off with Coin3D
#ifdef __coin
// --> coin 1,2 compatibility
//		SoGLLightModelElement::getInstance(state)->forceSend(SoLightModelElement::BASE_COLOR);
		glDisable(GL_LIGHTING);
// <-- coin 1,2 compatibility
#endif

    // Turn off texturing
    SoGLTextureEnabledElement::set(state, FALSE);
    
    // Get the current pixel alignment
    GLint curPixelAlignment;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &curPixelAlignment);

    // Set the pixel alignment
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    normalFont->setupToRender(state);

    SbMatrix objToScreen = currentMVP;
    SbMatrix screenToObj = objToScreen.inverse();

    SbViewportRegion vpr = currentVPR;

    SbVec3f coord;
    const int *theLRJust = leftRightJustification.getValues(0);
    const int *theTBJust = topBottomJustification.getValues(0);
    const SbColor *theColor = color.getValues(0);
    const SbVec3f *thePosition = position.getValues(0);
    const SbString *theText = text.getValues(0);

    int cBinding = colorBinding.getValue();
    int lrBinding = leftRightJustificationBinding.getValue();
    int tbBinding = topBottomJustificationBinding.getValue();

    const int *cIndex = colorIndex.getValues(0);
    const int *lrIndex = leftRightJustificationIndex.getValues(0);
    const int *tbIndex = topBottomJustificationIndex.getValues(0);

    if (cBinding == ChemLabel::OVERALL) {
        glColor3fv(theColor[0].getValue());
    }

    LABELLOOP_START(normalLabelIndex, text.getNum())
        if (cBinding == ChemLabel::PER_LABEL) {
            glColor3fv(theColor[theLabel].getValue());
        }
        else if (cBinding == ChemLabel::PER_LABEL_INDEXED) {
            glColor3fv(theColor[cIndex[theLabel]].getValue());
        }
        SbVec3f screenOrigin =
            fromObjectSpace(thePosition[theLabel], objToScreen, vpr);

        short lrjust, tbjust;
        if (lrBinding == ChemLabel::PER_LABEL) {
            lrjust = theLRJust[theLabel];
        }
        else if (lrBinding == ChemLabel::PER_LABEL_INDEXED) {
            lrjust = theLRJust[lrIndex[theLabel]];
        }
        else {
            lrjust = theLRJust[0];
        }

        if (tbBinding == ChemLabel::PER_LABEL) {
            tbjust = theTBJust[theLabel];
        }
        else if (tbBinding == ChemLabel::PER_LABEL_INDEXED) {
            tbjust = theTBJust[tbIndex[theLabel]];
        }
        else {
            tbjust = theTBJust[0];
        }

        SbVec3f charPosition = 
            getPixelStringOffset(lrjust, tbjust, theText[theLabel],
                normalFont) + screenOrigin;

        // Transform the screen-space starting position into object
        // space, and feed that back to the glRasterPos command (which
        // will turn around and transform it back into screen-space,
        // but oh well).

        coord = toObjectSpace(charPosition, screenToObj, vpr);
        glRasterPos3fv(coord.getValue());
        normalFont->drawString(theText[theLabel]);
    LABELLOOP_END

    glColor3fv(highlightColor.getValue().getValue());
    LABELLOOP_START(highlightLabelIndex, text.getNum())
        SbVec3f screenOrigin =
            fromObjectSpace(thePosition[theLabel], objToScreen, vpr);

        short lrjust, tbjust;
        if (lrBinding == ChemLabel::PER_LABEL) {
            lrjust = theLRJust[theLabel];
        }
        else if (lrBinding == ChemLabel::PER_LABEL_INDEXED) {
            lrjust = theLRJust[lrIndex[theLabel]];
        }
        else {
            lrjust = theLRJust[0];
        }

        if (tbBinding == ChemLabel::PER_LABEL) {
            tbjust = theTBJust[theLabel];
        }
        else if (tbBinding == ChemLabel::PER_LABEL_INDEXED) {
            tbjust = theTBJust[tbIndex[theLabel]];
        }
        else {
            tbjust = theTBJust[0];
        }

        SbVec3f charPosition = 
            getPixelStringOffset(lrjust, tbjust, theText[theLabel],
                normalFont) + screenOrigin;

        // Transform the screen-space starting position into object
        // space, and feed that back to the glRasterPos command (which
        // will turn around and transform it back into screen-space,
        // but oh well).

        coord = toObjectSpace(charPosition, screenToObj, vpr);
        glRasterPos3fv(coord.getValue());
        normalFont->drawString(theText[theLabel]);
    LABELLOOP_END

#ifndef IV2_0
    SoGLLazyElement::setColorMaterial(state, FALSE);
    ((SoGLLazyElement *)SoLazyElement::getInstance(state))->
        reset(state, SoLazyElement::DIFFUSE_MASK);

    // Don't auto-cache since dependent on camera
    SoGLCacheContextElement::shouldAutoCache(state,
        SoGLCacheContextElement::DONT_AUTO_CACHE);
#else
    SoCacheElement::invalidate(state);
#endif

    // Reset the pixel alignment
    glPixelStorei(GL_UNPACK_ALIGNMENT, curPixelAlignment);
    state->pop();
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Implements ray picking
//
// Use: public, extender
                    
void
ChemLabel::rayPick(SoRayPickAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    if ((ChemSelectionElement::get(action->getState()) & 
         ChemSelectionElement::CHEMLABELS) != 0) {
        commonPick(action, FALSE, NULL, NULL);
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    When this node is part of another node (such as ChemMonitor),
//    this allows the encapsulating node to determine which labels
//    have been picked by the ray.
//
// Use: public, internal

void
ChemLabel::getPickedPoints(SoRayPickAction *action, SoNode *parentNode,
    SoPickedPointList *ptList)
//
////////////////////////////////////////////////////////////////////////
{
    commonPick(action, TRUE, parentNode, ptList);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Determines which labels intersect the ray
//
// Use: private

void
ChemLabel::commonPick(SoRayPickAction *action, SbBool fillPickList,
    SoNode *parentNode, SoPickedPointList *ptList)
//
////////////////////////////////////////////////////////////////////////
{
    SoState *state = action->getState();
    state->push();

    // Get a font cache we can pull stuff out of
    if (normalFont != NULL) {
        if (!normalFont->isValid(state,
                                 fontName.getValue(),
                                 fontSize.getValue())) {
            normalFont->unref();
            normalFont = NULL;
        }
    }
    if (normalFont == NULL) {
        normalFont = ChemBitmapFontCache::getFont(NULL, state,
            fontName.getValue(), fontSize.getValue(), FALSE);
        if (normalFont == NULL) {
            state->pop();
            return;
        }
    }

    action->setObjectSpace();

    SbMatrix objToScreen = SoProjectionMatrixElement::get(state);
    objToScreen =
        objToScreen.multLeft(SoViewingMatrixElement::get(state));
    objToScreen =
        objToScreen.multLeft(SoModelMatrixElement::get(state));

    SbMatrix screenToObj = objToScreen.inverse();

    SbViewportRegion vpr = SoViewportRegionElement::get(state);

    // Figure out the screen-space bounding box of the characters:
    SbBox3f lineBbox, charBbox;

    //SbVec3f coord;
    const int *theLRJust = leftRightJustification.getValues(0);
    const int *theTBJust = topBottomJustification.getValues(0);
    //const SbColor *theColor = color.getValues(0);
    const SbVec3f *thePosition = position.getValues(0);
    const SbString *theText = text.getValues(0);

    int cBinding = colorBinding.getValue();
    int lrBinding = leftRightJustificationBinding.getValue();
    int tbBinding = topBottomJustificationBinding.getValue();

    const int *cIndex = colorIndex.getValues(0);
    const int *lrIndex = leftRightJustificationIndex.getValues(0);
    const int *tbIndex = topBottomJustificationIndex.getValues(0);

    int len = text.getNum();
    for (int i = 0; i < len; i++) {
        SbVec3f screenOrigin =
            fromObjectSpace(thePosition[i], objToScreen, vpr);

        short lrjust, tbjust;
        if (lrBinding == ChemLabel::PER_LABEL) {
            lrjust = theLRJust[i];
        }
        else if (lrBinding == ChemLabel::PER_LABEL_INDEXED) {
            lrjust = theLRJust[lrIndex[i]];
        }
        else {
            lrjust = theLRJust[0];
        }

        if (tbBinding == ChemLabel::PER_LABEL) {
            tbjust = theTBJust[i];
        }
        else if (tbBinding == ChemLabel::PER_LABEL_INDEXED) {
            tbjust = theTBJust[tbIndex[i]];
        }
        else {
            tbjust = theTBJust[0];
        }

        // Starting position of string, based on justification:
        SbVec3f charPosition = 
            getPixelStringOffset(lrjust, tbjust, theText[i],
                normalFont) + screenOrigin;

        const char *chars = theText[i].getString();
        SbVec3f p0, p1, p2, p3;

        lineBbox.makeEmpty();
	// int chr = 0 put out of the for loop by fabien fontaine the 
	// 15/12/2000 to avoid compiler warning
	int chr = 0 ;
        for ( ; chr < theText[i].getLength(); chr++) {
            normalFont->getCharBbox(chars[chr], charBbox);

            if (!charBbox.isEmpty()) {

                SbVec3f charMin = charBbox.getMin() + charPosition;
                SbVec3f charMax = charBbox.getMax() + charPosition;

                // Extend the line's bounding box by this character's
                // bounding box (both boxes are in screen-space):
                lineBbox.extendBy(charMin);
                lineBbox.extendBy(charMax);

                // Advance to next character...
                charPosition += normalFont->getCharOffset(chars[chr]);
            }
        }
        // And transform line's box into object space:
        SbVec3f min = lineBbox.getMin();
        SbVec3f max = lineBbox.getMax();

        SbVec3f t;
        t.setValue(min[0], min[1], screenOrigin[2]);
        p0 = toObjectSpace(t, screenToObj, vpr);
        t.setValue(max[0], min[1], screenOrigin[2]);
        p1 = toObjectSpace(t, screenToObj, vpr);
        t.setValue(min[0], max[1], screenOrigin[2]);
        p2 = toObjectSpace(t, screenToObj, vpr);
        t.setValue(max[0], max[1], screenOrigin[2]);
        p3 = toObjectSpace(t, screenToObj, vpr);

        // intersect the two triangles:
        SbVec3f point;
        // Info we get back from the pick that we don't need:
        SbVec3f junk1; SbBool junk2;
        if (action->intersect(p0, p1, p2, point, junk1, junk2) ||
            action->intersect(p2, p1, p3, point, junk1, junk2)) {

            SoPickedPoint *pp = NULL;
            if (fillPickList) {
                pp = new SoPickedPoint(action->getCurPath(),
                                       action->getState(), point);
                ptList->append(pp);
            }
            else {
                pp = action->addIntersection(point);
            }
            if (pp) {
                SoTextDetail *detail = new SoTextDetail();
                detail->setStringIndex(i);

                // Figure out which character was hit:
                // Transform picked point into screen space:
                SbVec3f screenPoint =
                    fromObjectSpace(pp->getObjectPoint(), objToScreen, vpr);

                // Figure out which character that corresponds to, by
                // adding on the offset of each character until we
                // go past the picked point:
                charPosition = getPixelStringOffset(lrjust, tbjust,
                    theText[i], normalFont) + screenOrigin;

                for (chr = 0; chr < theText[i].getLength(); chr++) {
                    charPosition += normalFont->getCharOffset(chars[chr]);
                    // Assuming left-to-right drawing of characters:
                    if (charPosition[0] >= screenPoint[0]) break;
                }
                detail->setCharacterIndex(chr);

                if (fillPickList) {
                    pp->setDetail(detail, parentNode);
                }
                else {
                    pp->setDetail(detail, this);
                }
                if (cBinding == ChemLabel::OVERALL) {
                    pp->setMaterialIndex(0);
                }
                else if (cBinding == ChemLabel::PER_LABEL) {
                    pp->setMaterialIndex(i);
                }
                else if (cBinding == ChemLabel::PER_LABEL_INDEXED) {
                    pp->setMaterialIndex(cIndex[i]);
                }

                // We'll define normal to be object-space 0,0,1:
                pp->setObjectNormal(SbVec3f(0,0,1));
                // And texture coordinates to be zero:
                pp->setObjectTextureCoords(SbVec4f(0,0,0,0));
            }
        }
    }
    state->pop();
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Compute bounding box 
//
// Use: protected

void
ChemLabel::computeBBox(SoAction *action, SbBox3f &box,  SbVec3f &center)
//
////////////////////////////////////////////////////////////////////////
{
    SoState *state = action->getState();
    state->push();

    box.makeEmpty();
    center.setValue(0.0, 0.0, 0.0);

    // Get a font cache we can pull stuff out of
    if (normalFont != NULL) {
        if (!normalFont->isValid(state,
                                 fontName.getValue(),
                                 fontSize.getValue())) {
            normalFont->unref();
            normalFont = NULL;
        }
    }
    if (normalFont == NULL) {
        normalFont = ChemBitmapFontCache::getFont(NULL, state,
            fontName.getValue(), fontSize.getValue(), FALSE);
        if (normalFont == NULL) {
            state->pop();
            return;
        }
    }

    SbMatrix objToScreen = SoProjectionMatrixElement::get(state);
    objToScreen =
        objToScreen.multLeft(SoViewingMatrixElement::get(state));
    objToScreen =
        objToScreen.multLeft(SoModelMatrixElement::get(state));

    SbMatrix screenToObj = objToScreen.inverse();

    SbViewportRegion vpr = SoViewportRegionElement::get(state);

    // Figure out the screen-space bounding box of the characters:
    SbBox3f screenBbox, charBbox;

    const int *theLRJust = leftRightJustification.getValues(0);
    const int *theTBJust = topBottomJustification.getValues(0);
    const SbString *theText = text.getValues(0);
    const SbVec3f *thePosition = position.getValues(0);

    int lrBinding = leftRightJustificationBinding.getValue();
    int tbBinding = topBottomJustificationBinding.getValue();

    const int *lrIndex = leftRightJustificationIndex.getValues(0);
    const int *tbIndex = topBottomJustificationIndex.getValues(0);

    int32_t numUsed = 0;
    int32_t len = text.getNum();
    for (int32_t i = 0; i < len; i++) {
        // The origin of the text on the screen is the object-space point
        // coord
        SbVec3f screenOrigin =
            fromObjectSpace(thePosition[i], objToScreen, vpr);
        short lrjust, tbjust;
        if (lrBinding == ChemLabel::PER_LABEL) {
            lrjust = theLRJust[i];
        }
        else if (lrBinding == ChemLabel::PER_LABEL_INDEXED) {
            lrjust = theLRJust[lrIndex[i]];
        }
        else {
            lrjust = theLRJust[0];
        }

        if (tbBinding == ChemLabel::PER_LABEL) {
            tbjust = theTBJust[i];
        }
        else if (tbBinding == ChemLabel::PER_LABEL_INDEXED) {
            tbjust = theTBJust[tbIndex[i]];
        }
        else {
            tbjust = theTBJust[0];
        }

        // Starting position of string, based on justification:
        SbVec3f charPosition = 
            getPixelStringOffset(lrjust, tbjust, theText[i],
                normalFont) + screenOrigin;

        const char *chars = theText[i].getString();
        screenBbox.makeEmpty();

        for (int chr = 0; chr < theText[i].getLength(); chr++) {
            normalFont->getCharBbox(chars[chr], charBbox);
            if (!charBbox.isEmpty()) {
                SbVec3f min = charBbox.getMin() + charPosition;
                SbVec3f max = charBbox.getMax() + charPosition;
                screenBbox.extendBy(min);
                screenBbox.extendBy(max);
            }

            // And advance...
            charPosition += normalFont->getCharOffset(chars[chr]);
        }
        // Ok, screenBbox now contains the pixel-space extent of the
        // characters.  We'll transform the bounds of that box back into
        // object space and extend the object-space bounding box:
    
        if (!screenBbox.isEmpty()) {
            // Do each of the 4 corners of the screen-space box:
            const SbVec3f &min = screenBbox.getMin();
            const SbVec3f &max = screenBbox.getMax();
            SbVec3f objectPoint, temp;
            SbVec3f centerPoint(0, 0, 0);

            temp.setValue(min[0], min[1], screenOrigin[2]);
            objectPoint = toObjectSpace(temp, screenToObj, vpr);
            centerPoint = objectPoint;
            box.extendBy(objectPoint);

            temp.setValue(max[0], max[1], screenOrigin[2]);
            objectPoint = toObjectSpace(temp, screenToObj, vpr);
            centerPoint += objectPoint;
            box.extendBy(objectPoint);

            temp.setValue(min[0], max[1], screenOrigin[2]);
            objectPoint = toObjectSpace(temp, screenToObj, vpr);
            centerPoint += objectPoint;
            box.extendBy(objectPoint);

            temp.setValue(max[0], min[1], screenOrigin[2]);
            objectPoint = toObjectSpace(temp, screenToObj, vpr);
            centerPoint += objectPoint;
            box.extendBy(objectPoint);

            // Set the center to be the center of the 4 points.
            center += centerPoint * 0.25f;
            numUsed++;
        }
    }
    if (numUsed > 0) {
        center /= (float)numUsed;
    }
    state->pop();
}

////////////////////////////////////////////////////////////////////////
// 
// Description:
//    Generates primitives - no-op.
//
// Use: protected

void
ChemLabel::generatePrimitives(SoAction *)
//
////////////////////////////////////////////////////////////////////////
{
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

#undef END_OF_LIST

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Checks to see if the selectionList has changed and if so, regenerates
//    the internal sets of indices.
//
// Use: private
    
void
ChemLabel::generateIndices(SoAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    SoState *state = action->getState();
    SbBool selectionChanged = FALSE;

    if (doHighlighting.getValue() == FALSE) {
        if (highlightLabelIndex.getNum() > 0) {
            resetIndices();
        }
        if (lastSelectionElement != NULL) {
            delete lastSelectionElement;
            lastSelectionElement = NULL;
        }
        return;
    }

    const ChemLabelSelectionElement *clse =
        ChemLabelSelectionElement::getInstance(state);
        
    // If there is no ChemLabelSelectionElement in the state, check to see if
    // there used to be a ChemLabelSelectionElement in the state.  If there
    // was, then set selectionChanged to TRUE.
    if ((clse == NULL) && (lastSelectionElement != NULL)) {
        selectionChanged = TRUE;
    }
    
    if (!selectionChanged) {
        if (clse != NULL) {
    
            // Get the selectionList from the ChemLabelSelectionElement.  If the
            // data changed, then the ChemSelection node would have reset the
            // ChemLabelSelectionElement to contain no selections.  If there
            // previously had been selections, then the
            // lastSelectionElement would not match and things would be
            // OK.  If there had not been any previous selections, then
            // things would match and we would still be OK.

            //const ChemLabelPathList &selectionList = clse->getList();
            if (lastSelectionElement == NULL) {
                selectionChanged = TRUE;
                lastSelectionElement =
                    (ChemLabelSelectionElement *)clse->copyMatchInfo();
            }
            else {
                if (lastSelectionElement->matches(clse)) {
                    selectionChanged = FALSE;
                }
                else {
                    selectionChanged = TRUE;
                    delete lastSelectionElement;
                    lastSelectionElement =
                        (ChemLabelSelectionElement *)clse->copyMatchInfo();
                }
            }
        }
    }

    if (selectionChanged) {
        // If the selection changed, the first thing to do is to reset all
        // the "normal" indices.
        resetIndices();

        // If there is a ChemLabelSeletionElement, get the lists of selected items
        // and fill out the highlighted indices.
        if (clse != NULL) {
            const ChemLabelPathList &selectionList = clse->getList();
            int32_t length = selectionList.getLength();
            int32_t i;
            for (i = 0; i < length; i++) {
                if (((ChemPath *)selectionList[i])->isOfType(ChemLabelPath::getClassTypeId())) {
                    const ChemLabelPath *si =
                        (ChemLabelPath *)selectionList[i];
                    SoFullPath *path = (SoFullPath *)si->getSoPath();
                    if (path->getTail() == this) {
                        const MFVec2i &slLabelIndex = si->getLabelIndex();
                        highlightLabelIndex.setValues(0,
                            slLabelIndex.getNum(),
                            slLabelIndex.getValues(0));

                        break;
                    }
                }
            }

            // Remove the values in the "highlight" indices from the values
            // in the "normal" indices.

            if (highlightLabelIndex.getNum() > 0) {
                removeIndices(normalLabelIndex, highlightLabelIndex,
                     text.getNum());
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
ChemLabel::resetIndices()
//
////////////////////////////////////////////////////////////////////////
{
    highlightLabelIndex.deleteValues(0);
    normalLabelIndex.setValue(0, -1);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Compute each bounding box.  This is invoked by ChemBBoxAction.
//
// Use: protected

void
ChemLabel::getChemLabelBBoxes(SoAction *action, SbBool clipOnCenter,
    ChemLabelBBox *&chemLabelBBoxes)
//
////////////////////////////////////////////////////////////////////////
{
    chemLabelBBoxes = NULL;
    SoState *state = action->getState();

    if ((int)SoPickStyleElement::get(state) == (int)SoPickStyle::UNPICKABLE) {
        return;
    }

    state->push();

    // Get a font cache we can pull stuff out of
    if (normalFont != NULL) {
        if (!normalFont->isValid(state,
                                 fontName.getValue(),
                                 fontSize.getValue())) {
            normalFont->unref();
            normalFont = NULL;
        }
    }
    if (normalFont == NULL) {
        normalFont = ChemBitmapFontCache::getFont(NULL, state,
            fontName.getValue(), fontSize.getValue(), FALSE);
        if (normalFont == NULL) {
            state->pop();
            return;
        }
    }

    SbMatrix objToScreen = currentMVP;
    SbMatrix screenToObj = objToScreen.inverse();
    SbViewportRegion vpr = currentVPR;

    const SoClipPlaneElement *clipPlaneElt =
        SoClipPlaneElement::getInstance(state);

    // Figure out the screen-space bounding box of the characters:
    SbBox3f screenBbox, charBbox;

    const int *theLRJust = leftRightJustification.getValues(0);
    const int *theTBJust = topBottomJustification.getValues(0);
    const SbString *theText = text.getValues(0);
    const SbVec3f *thePosition = position.getValues(0);

    int lrBinding = leftRightJustificationBinding.getValue();
    int tbBinding = topBottomJustificationBinding.getValue();

    const int *lrIndex = leftRightJustificationIndex.getValues(0);
    const int *tbIndex = topBottomJustificationIndex.getValues(0);

    // Allocate space for the individual bboxes
    int32_t len = text.getNum();
    ChemLabelBBox *tmpBBox = new ChemLabelBBox(len);

    int32_t bCount = 0;
    for (int32_t i = 0; i < len; i++) {
        // The origin of the text on the screen is the object-space point
        // coord
        SbVec3f screenOrigin =
            fromObjectSpace(thePosition[i], objToScreen, vpr);
        short lrjust, tbjust;
        if (lrBinding == ChemLabel::PER_LABEL) {
            lrjust = theLRJust[i];
        }
        else if (lrBinding == ChemLabel::PER_LABEL_INDEXED) {
            lrjust = theLRJust[lrIndex[i]];
        }
        else {
            lrjust = theLRJust[0];
        }

        if (tbBinding == ChemLabel::PER_LABEL) {
            tbjust = theTBJust[i];
        }
        else if (tbBinding == ChemLabel::PER_LABEL_INDEXED) {
            tbjust = theTBJust[tbIndex[i]];
        }
        else {
            tbjust = theTBJust[0];
        }

        // Starting position of string, based on justification:
        SbVec3f charPosition = 
            getPixelStringOffset(lrjust, tbjust, theText[i],
                normalFont) + screenOrigin;

        const char *chars = theText[i].getString();
        screenBbox.makeEmpty();

        for (int chr = 0; chr < theText[i].getLength(); chr++) {
            normalFont->getCharBbox(chars[chr], charBbox);
            if (!charBbox.isEmpty()) {
                SbVec3f min = charBbox.getMin() + charPosition;
                SbVec3f max = charBbox.getMax() + charPosition;
                screenBbox.extendBy(min);
                screenBbox.extendBy(max);
            }

            // And advance...
            charPosition += normalFont->getCharOffset(chars[chr]);
        }
        // Ok, screenBbox now contains the pixel-space extent of the
        // characters.  We'll transform the bounds of that box back into
        // object space and extend the object-space bounding box:
    
        if (!screenBbox.isEmpty()) {
            // Do each of the 4 corners of the screen-space box:
            const SbVec3f &min = screenBbox.getMin();
            const SbVec3f &max = screenBbox.getMax();
            SbVec3f tempMin, tempMax, center;
            SbVec3f p0, p1;

            tempMin.setValue(min[0], min[1], screenOrigin[2]);
            tempMax.setValue(max[0], max[1], screenOrigin[2]);
            tempMin = toObjectSpace(tempMin, screenToObj, vpr);
            tempMax = toObjectSpace(tempMax, screenToObj, vpr);

            SbBool addBBox = FALSE;
            center = (tempMin + tempMax) * 0.5f;
            if (clipOnCenter) {
                // Check to see if the center is between clipping planes.
                // If not, the don't increment bCount.
                // The check to see if the center is between the clipping planes
                // needs to be performed in world space
                // (see SoRayPickAction::isBetweenPlanes) so multiply by the
                // currentModelMatrix
                currentModelMatrix.multVecMatrix(center, p0);
                if (isPointBetweenPlanes(p0, currentViewVolume, clipPlaneElt)) {
                    addBBox = TRUE;
                }
            }
            else {
                // Check to see if the bounding box is between clipping planes.
                // If not, the don't increment bCount.
                // The check to see if the points are between the clipping planes
                // needs to be performed in world space
                // (see SoRayPickAction::isBetweenPlanes) so multiply by the
                // currentModelMatrix
                currentModelMatrix.multVecMatrix(tempMin, p0);
                currentModelMatrix.multVecMatrix(tempMax, p1);
                if (isBoxBetweenPlanes(p0, p1, currentViewVolume, clipPlaneElt)) {
                    addBBox = TRUE;
                }
            }
            if (addBBox) {
                tmpBBox->bbox[bCount].setBounds(tempMin, tempMax);
                tmpBBox->center[bCount] = center;
                tmpBBox->index[bCount] = i;
                bCount++;
            }
        }
    }
    if (bCount == len) {
        chemLabelBBoxes = tmpBBox;
    }
    else {
        chemLabelBBoxes = new ChemLabelBBox(*tmpBBox, bCount);
        delete tmpBBox;
    }
    state->pop();
}
