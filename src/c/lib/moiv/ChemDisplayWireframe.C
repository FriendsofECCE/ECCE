/*
 * ChemDisplayWireframe.pp
 *
 *     Classes:  ChemDisplay
 *     Note: This file was preprocessed from another file. Do not edit it.
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

#ident "$Revision: 22177 $"

#include <assert.h>
#include <ctype.h>
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

#include "inv/SbBox.H"
#include "inv/SoPickedPoint.H"
#include "inv/SoPrimitiveVertex.H"
#include "inv/actions/SoCallbackAction.H"
#include "inv/actions/SoGLRenderAction.H"
#include "inv/actions/SoRayPickAction.H"
#include "inv/caches/SoGLRenderCache.H"
#include "inv/details/SoLineDetail.H"
#include "inv/details/SoPointDetail.H"
#include "inv/elements/SoCacheElement.H"
#include "inv/elements/SoFontNameElement.H"
#include "inv/elements/SoFontSizeElement.H"
#include "inv/elements/SoLightModelElement.H"
#include "inv/elements/SoGLCacheContextElement.H"
#include "inv/elements/SoGLDrawStyleElement.H"
#include "inv/elements/SoGLTextureCoordinateElement.H"
#include "inv/elements/SoGLTextureEnabledElement.H"
#include "inv/elements/SoGLTextureImageElement.H"
#include "inv/elements/SoModelMatrixElement.H"
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

#include "inv/ChemKit/ChemDetail.H"
#include "inv/ChemKit/ChemDisplay.H"
#include "inv/ChemKit/ChemBaseDataElement.H"
#include "inv/ChemKit/ChemColorElement.H"
#include "inv/ChemKit/ChemDisplayParamElement.H"
#include "inv/ChemKit/ChemWireframeDetail.H"

#include "inv/ChemKit/ChemBondStuff.H"
#include "inv/ChemKit/ChemAromaticRing.H"

// --> compute cylinder lod
#include "inv/ChemKit/ChemUnitCylinder.H"
// <-- compute cylinder lod

static void singleBondsNormal(const SbIntList list, const SbBool renderAsPoints,
	const SbBool doHydrogens, const ChemBaseData *chemData,
	const ChemDisplayParam *cdp, const ChemColor *chemColor,
	const ChemBondStuff *bondAttributes);

static void doubleBondsNormal(const SbIntList list, const SbBool renderAsPoints,
	const SbBool doHydrogens, const ChemBaseData *chemData,
	const ChemDisplayParam *cdp, const ChemColor *chemColor,
	const ChemBondStuff *bondAttributes);

static void tripleBondsNormal(const SbIntList list, const SbBool renderAsPoints,
	const SbBool doHydrogens, const ChemBaseData *chemData,
	const ChemDisplayParam *cdp, const ChemColor *chemColor,
	const ChemBondStuff *bondAttributes);

static void quadrupleBondsNormal(const SbIntList list,
	const SbBool renderAsPoints,
	const SbBool doHydrogens, const ChemBaseData *chemData,
	const ChemDisplayParam *cdp, const ChemColor *chemColor,
	const ChemBondStuff *bondAttributes);

static void resonanceBondsNormal(const SbIntList list,
	const SbBool renderAsPoints,
	const SbBool doHydrogens, const ChemBaseData *chemData,
	const ChemDisplayParam *cdp, const ChemColor *chemColor,
	const ChemBondStuff *bondAttributes);

static void hydrogenBondsNormal(const SbIntList list,
	const SbBool renderAsPoints,
	const SbBool doHydrogens, const ChemBaseData *chemData,
	const ChemDisplayParam *cdp, const ChemColor *chemColor,
	const ChemBondStuff *bondAttributes);

static void singleBondsHighlighted(const SbIntList list,
	const SbBool renderAsPoints,
	const SbBool doHydrogens, const SbColor highlightColor,
	const int32_t highlightStyle,
	const ChemBaseData *chemData,
	const ChemDisplayParam *cdp, const ChemColor *chemColor,
	const ChemBondStuff *bondAttributes);

static void doubleBondsHighlighted(const SbIntList list,
	const SbBool renderAsPoints,
	const SbBool doHydrogens, const SbColor highlightColor,
	const int32_t highlightStyle,
	const ChemBaseData *chemData,
	const ChemDisplayParam *cdp, const ChemColor *chemColor,
	const ChemBondStuff *bondAttributes);

static void tripleBondsHighlighted(const SbIntList list,
	const SbBool renderAsPoints,
	const SbBool doHydrogens, const SbColor highlightColor,
	const int32_t highlightStyle,
	const ChemBaseData *chemData,
	const ChemDisplayParam *cdp, const ChemColor *chemColor,
	const ChemBondStuff *bondAttributes);

static void quadrupleBondsHighlighted(const SbIntList list,
	const SbBool renderAsPoints,
	const SbBool doHydrogens, const SbColor highlightColor,
	const int32_t highlightStyle,
	const ChemBaseData *chemData,
	const ChemDisplayParam *cdp, const ChemColor *chemColor,
	const ChemBondStuff *bondAttributes);

static void resonanceBondsHighlighted(const SbIntList list,
	const SbBool renderAsPoints,
	const SbBool doHydrogens, const SbColor highlightColor,
	const int32_t highlightStyle,
	const ChemBaseData *chemData,
	const ChemDisplayParam *cdp, const ChemColor *chemColor,
	const ChemBondStuff *bondAttributes);

static void hydrogenBondsHighlighted(const SbIntList list,
	const SbBool renderAsPoints,
	const SbBool doHydrogens, const SbColor highlightColor,
	const int32_t highlightStyle,
	const ChemBaseData *chemData,
	const ChemDisplayParam *cdp, const ChemColor *chemColor,
	const ChemBondStuff *bondAttributes);

static void checkForPick(SoRayPickAction *action, SbLine &pickLine, 
	ChemBaseData *chemData, ChemColor *chemColor, ChemDisplay *chemDisplay,
	int32_t from, int32_t to, int32_t theBond,
	SbVec3f &coords1, SbVec3f &coords2);

//
// Start of WIREFRAME section
//

////////////////////////////////////////////////////////////////////////
//
// Description:
//	   Performs GL rendering of the molecule using lines.
//	   Invoked by the GLRender method.
//
// Use: protected

void
ChemDisplay::renderBondsAsWireframe(SoGLRenderAction *action)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf("In ChemDisplay::renderBondsAsWireframe\n");
#endif
	SoState *state;
	ChemColor *chemColor;
	ChemDisplayParam *cdp;
	ChemBaseData *chemData;

	setupWireframe(action, state, chemColor, cdp, chemData);

	if ((normalBondIndex.getNum() > 0) || (highlightBondIndex.getNum() > 0)) {

		// Set the Inventor and GL states
		state->push();

		// Get the current GL_LINE_WIDTH as we should restore it after we
		// are done, in case we modified it
		float curGlLineWidth;
		glGetFloatv(GL_LINE_WIDTH, &curGlLineWidth);

		SbBool renderAsPoints;
		renderAsPoints = (SoDrawStyleElement::get(state) ==
						  SoDrawStyleElement::POINTS);

	    // Turn off lighting
#ifdef IV2_0
		SoLightModelElement::set(state, this, SoLightModelElement::BASE_COLOR);
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

		if (!renderAsPoints &&
			(cdp->bondWireframeAntiAlias.getValue() !=
				ChemDisplayParam::WIREFRAME_ANTIALIAS_NONE)) {
			glPushAttrib(GL_COLOR_BUFFER_BIT |  // for GL_BLEND
				     GL_DEPTH_BUFFER_BIT |  // for GL_DEPTH_TEST
				     GL_LINE_BIT         |  // for GL_LINE_SMOOTH
				     GL_HINT_BIT);          // for GL_LINE_SMOOTH_HINT
			glEnable( GL_LINE_SMOOTH );
			glHint  ( GL_LINE_SMOOTH_HINT, GL_NICEST );
			glEnable( GL_BLEND );
			if (cdp->bondWireframeAntiAlias.getValue() ==
				ChemDisplayParam::WIREFRAME_ANTIALIAS_WITH_DEPTH_COMPARISON) {
				glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
				glEnable( GL_DEPTH_TEST );
			}
			else {
				glBlendFunc( GL_SRC_ALPHA, GL_ONE );
				glDisable( GL_DEPTH_TEST );
			}
		}

		if (normalBondIndex.getNum() > 0) {
			normalBondsAsWireframe(normalBondIndex,
				state, chemColor, cdp, chemData);
		}
		if (highlightBondIndex.getNum() > 0) {
			highlightBondsAsWireframe(highlightBondIndex,
				state, chemColor, cdp, chemData);
		}

		if ((aromaticRingList.getLength() > 0) &&
			(cdp->showMultipleBonds.getValue() == TRUE)) {
			normalSixRing(state, chemColor, cdp, chemData);
		}

		if (!renderAsPoints &&
			(cdp->bondWireframeAntiAlias.getValue() !=
		ChemDisplayParam::WIREFRAME_ANTIALIAS_NONE)) {
			glPopAttrib();
		}

#ifndef IV2_0
		SoGLLazyElement::setColorMaterial(state, FALSE);
		((SoGLLazyElement *)SoLazyElement::getInstance(state))->
			reset(state, myMask);
#endif
		// Restore the GL_LINE_WIDTH
		glLineWidth(curGlLineWidth);

		state->pop();
	}
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//
// Use: private

void
ChemDisplay::setupWireframe(SoAction *action,
	SoState *&state, ChemColor *&chemColor, 
	ChemDisplayParam *&cdp, ChemBaseData *&chemData)
//
////////////////////////////////////////////////////////////////////////
{
	state     = action->getState();
	chemColor = ChemColorElement::get(state);
	cdp       = ChemDisplayParamElement::get(state);
	chemData  = ChemBaseDataElement::get(state);

	int numSides = cdp->aromaticRingNumSides.getValue();
    if (numSides < 3) numSides = 3;
	if (lastAromaticRingNumSides == numSides) {
		return;
	}
	lastAromaticRingNumSides = numSides;

	float theta, dTheta;
	if (aromaticRingCoords != NULL) delete [] aromaticRingCoords;
	aromaticRingCoords = new SbVec3f[lastAromaticRingNumSides];

	theta = 0.0;
	dTheta = 2.0 * M_PI / (lastAromaticRingNumSides);
	for (int i = 0; i < lastAromaticRingNumSides; i++) {
		aromaticRingCoords[i].setValue(sin(theta), -cos(theta), 0.0);
		theta += dTheta;
	}

// --> compute cylinder lod
	if (unitCylinder==0)
	unitCylinder = new ChemUnitCylinder(
		cdp->bondCylinderDisplayStyle.getValue()%3,cdp->bondCylinderComplexity.getValue());

	SbMatrix modelMatrix;
	glGetFloatv(GL_MODELVIEW_MATRIX, modelMatrix);		
	unitCylinder->lookAtVector = SbVec3f(modelMatrix[0][2],modelMatrix[1][2],modelMatrix[2][2]);

	SbMatrix inverseModelview = modelMatrix.inverse();
	SbVec3f position(0,0,0);
	SbVec3f camPos;
	inverseModelview.multVecMatrix(position,camPos);
	unitCylinder->camPos= camPos;
	
	SbMatrix projectionMatrix;
	glGetFloatv(GL_PROJECTION_MATRIX, projectionMatrix);
	unitCylinder->perspective = (projectionMatrix[2][3] == -1.0f); 
// <-- compute cylinder lod

}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	   Performs normal rendering of the molecule using lines.
//	   Invoked by the GLRender method.
//
// Use: private

void
ChemDisplay::normalBondsAsWireframe(const MFVec2i &theBondIndex,
	SoState *state, const ChemColor *chemColor,
	const ChemDisplayParam *cdp, const ChemBaseData *chemData)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf("In normalBondsAsWireframe\n");
#endif

	// If there is nothing to display, then return.
	if (theBondIndex.getNum() == 0) return;

	SbBool renderAsPoints;
	SbBool doHydrogens = cdp->showHydrogens.getValue();

	// Since GL cannot render lines as points, we need to handle this
	// ourselves.
	renderAsPoints = (SoDrawStyleElement::get(state) == 
					  SoDrawStyleElement::POINTS);

	if (singleNormal.getLength() > 0) {
		singleBondsNormal(singleNormal, renderAsPoints,
			doHydrogens, chemData, cdp, chemColor, bondAttributes);
	}

	if (doubleNormal.getLength() > 0) {
		doubleBondsNormal(doubleNormal, renderAsPoints,
			doHydrogens, chemData, cdp, chemColor, bondAttributes);
	}

	if (tripleNormal.getLength() > 0) {
		tripleBondsNormal(tripleNormal, renderAsPoints,
			doHydrogens, chemData, cdp, chemColor, bondAttributes);
	}

	if (quadrupleNormal.getLength() > 0) {
		quadrupleBondsNormal(quadrupleNormal, renderAsPoints,
			doHydrogens, chemData, cdp, chemColor, bondAttributes);
	}

	if (resonanceNormal.getLength() > 0) {
		resonanceBondsNormal(resonanceNormal, renderAsPoints,
			doHydrogens, chemData, cdp, chemColor, bondAttributes);
	}

	if (hbondNormal.getLength() > 0) {
		hydrogenBondsNormal(hbondNormal, renderAsPoints,
			doHydrogens, chemData, cdp, chemColor, bondAttributes);
	}
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	   Performs highlighting of the molecule using lines.
//	   Invoked by the GLRender method.
//
// Use: private

void
ChemDisplay::highlightBondsAsWireframe(const MFVec2i &theBondIndex,
	SoState *state, const ChemColor *chemColor,
	const ChemDisplayParam *cdp, const ChemBaseData *chemData)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf("In highlightBondsAsWireframe\n");
#endif

	// First check to see if there is anything to highlight
	if (theBondIndex.getNum() == 0) return;

	SbBool	renderAsPoints;
	SbColor highlightColor = cdp->highlightColor.getValue();
	int32_t highlightStyle = cdp->highlightStyle.getValue();

	SbBool  doHydrogens = cdp->showHydrogens.getValue();

	// Since GL cannot render lines as points, we need to handle this
	// ourselves.
	renderAsPoints = (SoDrawStyleElement::get(state) == 
					  SoDrawStyleElement::POINTS);

	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
		highlightWireframeOverall(chemData, cdp,
			theBondIndex, doHydrogens, renderAsPoints);
	}
	else
// --> EGB && SGB double bonds highlight error
	{
// <-- EGB && SGB
		if (singleHighlight.getLength() > 0) {
		singleBondsHighlighted(singleHighlight, renderAsPoints, doHydrogens,
			highlightColor, highlightStyle,
			chemData, cdp, chemColor, bondAttributes);
	}
// --> EGB && SGB double bonds highlight error
//	else
// <-- EGB && SGB
	if (doubleHighlight.getLength() > 0) {
		doubleBondsHighlighted(doubleHighlight, renderAsPoints, doHydrogens,
			highlightColor, highlightStyle,
			chemData, cdp, chemColor, bondAttributes);
	}
// --> EGB && SGB double bonds highlight error
//	else
// <-- EGB && SGB
	if (tripleHighlight.getLength() > 0) {
		tripleBondsHighlighted(tripleHighlight, renderAsPoints, doHydrogens,
			highlightColor, highlightStyle,
			chemData, cdp, chemColor, bondAttributes);
	}
// --> EGB && SGB double bonds highlight error
//	else
// <-- EGB && SGB
	if (quadrupleHighlight.getLength() > 0) {
		quadrupleBondsHighlighted(quadrupleHighlight, renderAsPoints,
			doHydrogens, highlightColor, highlightStyle,
			chemData, cdp, chemColor, bondAttributes);
	}
// --> EGB && SGB double bonds highlight error
//	else
// <-- EGB && SGB
	if (resonanceHighlight.getLength() > 0) {
		resonanceBondsHighlighted(resonanceHighlight, renderAsPoints,
			doHydrogens, highlightColor, highlightStyle,
			chemData, cdp, chemColor, bondAttributes);
	}
// --> EGB && SGB double bonds highlight error
//	else
// <-- EGB && SGB
	if (hbondHighlight.getLength() > 0) {
		hydrogenBondsHighlighted(hbondHighlight, renderAsPoints, doHydrogens,
			highlightColor, highlightStyle,
			chemData, cdp, chemColor, bondAttributes);
	}
// --> EGB && SGB double bonds highlight error
	}
// <-- EGB && SGB

}
















////////////////////////////////////////////////////////////////////////
//
// Description:
//	   Performs highlighting of the molecule using lines.
//	   All lines are being highlighted by a single diffuse color.
//
// Use: private

void
ChemDisplay::highlightWireframeOverall(const ChemBaseData *chemData,
	const ChemDisplayParam *cdp,
	const MFVec2i &theBondIndex, SbBool doHydrogens, SbBool renderAsPoints)
//
////////////////////////////////////////////////////////////////////////
{
	int32_t i;
	int32_t listIndex, listLen;
	int32_t	from, to;
	SbVec3f coords1, coords2;

	if (theBondIndex.getNum() == 0) return;

	SbBool showMulti = cdp->showMultipleBonds.getValue();
	float  multiSep  = cdp->multipleBondSeparation.getValue();
	float  multiSep2 = multiSep * 0.5;

	SbVec3f tsep, tsep2;
	//SbVec3f tcoord, tc0, tc1, tc2;
	SbVec3f tc1, tc2;
	SbBool  hasEnded = FALSE;

	// There is a single color so what we have to worry about is:
	// 1) whether rendering as points or not
	// 2) whether rendering multiple bonds or not

	const SbColor &theColor = cdp->highlightColor.getValue();
	glColor3fv(theColor.getValue());

	if (renderAsPoints) {
		glBegin(GL_POINTS);
	}
	else {
		glLineWidth(cdp->bondWireframeLineWidth.getValue());
		glBegin(GL_LINES);
	}

	// Do single bonds and aromatic bonds
	listLen = singleHighlight.getLength();
	for (i = 0; i < listLen; i++) {
		listIndex = singleHighlight[i];

		from = bondAttributes[listIndex].from;
		to = bondAttributes[listIndex].to;

		if (!doHydrogens) {
			if ((chemData->getAtomicNumber(from) == 1) || 
				(chemData->getAtomicNumber(to) == 1)) continue;
		}
		coords1 = chemData->getAtomCoordinates(from).getValue();
		coords2 = chemData->getAtomCoordinates(to).getValue();
		glVertex3fv(coords1.getValue());
		glVertex3fv(coords2.getValue());
	}

	// If showing multiple bonds, then...
	if (showMulti) {
		// Do double bonds
		listLen = doubleHighlight.getLength();
		for (i = 0; i < listLen; i++) {
			listIndex = doubleHighlight[i];

			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			tsep = bondAttributes[listIndex].perpNormal * multiSep;
			tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

			if (bondAttributes[listIndex].isSymmetric == FALSE) {
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
				coords1 = bondAttributes[listIndex].start;
				coords2 = bondAttributes[listIndex].end;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
			}
			else {
				coords1 += tsep2;
				coords2 += tsep2;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());

				coords1 -= tsep;
				coords2 -= tsep;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
			}
		}

		// Do triple bonds
		listLen = tripleHighlight.getLength();
		for (i = 0; i < listLen; i++) {
			listIndex = tripleHighlight[i];

			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			tsep = bondAttributes[listIndex].perpNormal * multiSep;
			tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

			tc1 = coords1 + tsep;
			tc2 = coords2 + tsep;
			glVertex3fv(tc1.getValue());
			glVertex3fv(tc2.getValue());

			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());

			tc1 = coords1 - tsep;
			tc2 = coords2 - tsep;
			glVertex3fv(tc1.getValue());
			glVertex3fv(tc2.getValue());
		}

		// Do quadruple bonds
		listLen = quadrupleHighlight.getLength();
		for (i = 0; i < listLen; i++) {
			listIndex = quadrupleHighlight[i];

			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			tsep = bondAttributes[listIndex].perpNormal * multiSep;
			tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

			coords1 += tsep2;
			coords2 += tsep2;
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());

			coords1 += tsep;
			coords2 += tsep;
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());

			coords1 -= 2.0f*tsep;
			coords2 -= 2.0f*tsep;
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());

			coords1 -= tsep;
			coords2 -= tsep;
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());
		}

		// Do resonance bonds if necessary
		listLen = resonanceHighlight.getLength();
		if (listLen > 0) {
			// If rendering as points, we don't need to switch to the
			// stipple pattern for the second line
			if (renderAsPoints) {
				for (i = 0; i < listLen; i++) {
					listIndex = resonanceHighlight[i];

					from = bondAttributes[listIndex].from;
					to = bondAttributes[listIndex].to;

					if (!doHydrogens) {
						if ((chemData->getAtomicNumber(from) == 1) || 
							(chemData->getAtomicNumber(to) == 1)) continue;
					}
					coords1 = chemData->getAtomCoordinates(from).getValue();
					coords2 = chemData->getAtomCoordinates(to).getValue();

					tsep = bondAttributes[listIndex].perpNormal * multiSep;
					tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

					if (bondAttributes[listIndex].isSymmetric == FALSE) {
						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
						coords1 = bondAttributes[listIndex].start;
						coords2 = bondAttributes[listIndex].end;
						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
					}
					else {
						coords1 += tsep2;
						coords2 += tsep2;
						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
						coords1 -= tsep;
						coords2 -= tsep;
						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
					}
				}
			}
			else {
				// Here we are not rendering as points, so first draw the
				// solid lines and then draw the stippled lines
				for (i = 0; i < listLen; i++) {
					listIndex = resonanceHighlight[i];

					from = bondAttributes[listIndex].from;
					to = bondAttributes[listIndex].to;

					if (!doHydrogens) {
						if ((chemData->getAtomicNumber(from) == 1) || 
							(chemData->getAtomicNumber(to) == 1)) continue;
					}
					coords1 = chemData->getAtomCoordinates(from).getValue();
					coords2 = chemData->getAtomCoordinates(to).getValue();

					tsep = bondAttributes[listIndex].perpNormal * multiSep;
					tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

					if (bondAttributes[listIndex].isSymmetric == FALSE) {
						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
					}
					else {
						coords1 += tsep2;
						coords2 += tsep2;
						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
					}
				}
				// End the solid lines
				glEnd();
				hasEnded = TRUE;

				// Now draw the stippled lines
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->resonanceBondStippleFactor.getValue(),
					cdp->resonanceBondStipplePattern.getValue());
				glLineWidth(cdp->bondWireframeLineWidth.getValue());
				glBegin(GL_LINES);
				for (i = 0; i < listLen; i++) {
					listIndex = resonanceHighlight[i];

					from = bondAttributes[listIndex].from;
					to = bondAttributes[listIndex].to;

					if (!doHydrogens) {
						if ((chemData->getAtomicNumber(from) == 1) || 
							(chemData->getAtomicNumber(to) == 1)) continue;
					}
					coords1 = chemData->getAtomCoordinates(from).getValue();
					coords2 = chemData->getAtomCoordinates(to).getValue();

					tsep = bondAttributes[listIndex].perpNormal * multiSep;
					tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

					if (bondAttributes[listIndex].isSymmetric == FALSE) {
						coords1 = bondAttributes[listIndex].start;
						coords2 = bondAttributes[listIndex].end;
						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
					}
					else {
						coords1 -= tsep2;
						coords2 -= tsep2;
						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
					}
				}
				glEnd();
				glDisable(GL_LINE_STIPPLE);
				hasEnded = TRUE;
			}
		}

		// Do hydrogen bonds if necessary
		listLen = hbondHighlight.getLength();
		if (listLen > 0) {
			if (!renderAsPoints) {
				if (!hasEnded) glEnd();

				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->hydrogenBondStippleFactor.getValue(),
					cdp->hydrogenBondStipplePattern.getValue());
				glBegin(GL_LINES);
			}
			for (i = 0; i < listLen; i++) {
				listIndex = hbondHighlight[i];

				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
			}
			glEnd();
			if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);
			hasEnded = TRUE;
		}
	}

	// Not showing multiple bonds.  Since not showing multiple bonds, there
	// hasn't been a glEnd yet since all we did was draw single bonds up
	// to this point.
	else {
		listLen = doubleHighlight.getLength();
		for (i = 0; i < listLen; i++) {
			listIndex = doubleHighlight[i];

			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());
		}

		listLen = tripleHighlight.getLength();
		for (i = 0; i < listLen; i++) {
			listIndex = tripleHighlight[i];

			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());
		}

		listLen = quadrupleHighlight.getLength();
		for (i = 0; i < listLen; i++) {
			listIndex = quadrupleHighlight[i];

			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());
		}

		listLen = resonanceHighlight.getLength();
		for (i = 0; i < listLen; i++) {
			listIndex = resonanceHighlight[i];

			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());
		}

		if (renderAsPoints) {
			// If rendering the hbonds as points, we don't need to glEnd()
			// since we are not going to stipple the hbonds
			listLen = hbondHighlight.getLength();
			for (i = 0; i < listLen; i++) {
				listIndex = hbondHighlight[i];

				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
			}
		}
		else {
			// This check isn't necessary, but just to be sure...
			// We need to end the solid lines and start the stipple lines
			// for hbonds
			if (!hasEnded) glEnd();

			glEnable(GL_LINE_STIPPLE);
			glLineStipple(cdp->hydrogenBondStippleFactor.getValue(),
				cdp->hydrogenBondStipplePattern.getValue());
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);

			listLen = hbondHighlight.getLength();
			for (i = 0; i < listLen; i++) {
				listIndex = hbondHighlight[i];

				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
			}
			glEnd();
			glDisable(GL_LINE_STIPPLE);
			hasEnded = TRUE;
		}
	}
	if (!hasEnded) glEnd();
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Draws the circle in aromatic rings
//
// Use: private

void
ChemDisplay::normalSixRing(SoState *state, const ChemColor *chemColor,
	const ChemDisplayParam *cdp, const ChemBaseData *chemData)
//
////////////////////////////////////////////////////////////////////////
{
	int i, j;
	SbBool renderAsPoints;

	// Since GL cannot render lines as points, we need to handle this
	// ourselves.
	renderAsPoints = (SoDrawStyleElement::get(state) == 
					  SoDrawStyleElement::POINTS);

	SbVec3f vec;
	SbColor aromaticRingColor = cdp->aromaticRingColor.getValue();
	glColor3fv(aromaticRingColor.getValue());
	for (i = 0; i < aromaticRingList.getLength(); i++) {
		ChemAromaticRing *aromaticRing =
			(ChemAromaticRing *)aromaticRingList[i];

		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINE_STRIP);
		}
		for (j = 0; j < lastAromaticRingNumSides; j++) {
			aromaticRing->transform.multVecMatrix(aromaticRingCoords[j], vec);
			glVertex3fv(vec.getValue());
		}
		aromaticRing->transform.multVecMatrix(aromaticRingCoords[0], vec);
		glVertex3fv(vec.getValue());
		glEnd();
	}
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Implements rayPicking.
//
// Use: protected

void
ChemDisplay::pickBondsAsWireframe(SoRayPickAction *action)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::pickBondsAsWireframe\n" );
#endif
	int32_t	i;
	int32_t	from, to, theBond;
	SbVec3f	coords1, coords2;
	SbLine	pickLine;

	SoState *state = action->getState();

	ChemBaseData *chemData = ChemBaseDataElement::get(state);
	ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);
	ChemColor *chemColor = ChemColorElement::get(state);

	SbBool doHydrogens = cdp->showHydrogens.getValue();
	SbBool showMulti   = cdp->showMultipleBonds.getValue();
	float  multiSep    = cdp->multipleBondSeparation.getValue();
	float  multiSep2   = multiSep * 0.5;

	computeObjectSpaceRay(action);
	pickLine = action->getLine();

	for (i = 0; i < localNumberOfBonds; i++) {
		theBond = bondAttributes[i].index;
		from = bondAttributes[i].from;
		to = bondAttributes[i].to;

		if (!doHydrogens) {
			if ((chemData->getAtomicNumber(from) == 1) || 
				(chemData->getAtomicNumber(to) == 1)) continue;
		}
		coords1 = chemData->getAtomCoordinates(from).getValue();
		coords2 = chemData->getAtomCoordinates(to).getValue();

		if (showMulti) {
			SbVec3f tsep = bondAttributes[i].perpNormal * multiSep;
			SbVec3f tsep2 = bondAttributes[i].perpNormal * multiSep2;

			if (bondAttributes[i].type == ChemBaseData::QUADRUPLE_BOND) {
				coords1 += tsep2;
				coords2 += tsep2;
				checkForPick(action, pickLine, chemData, chemColor,
					this, from, to, theBond, coords1, coords2);

				coords1 += tsep;
				coords2 += tsep;
				checkForPick(action, pickLine, chemData, chemColor,
					this, from, to, theBond, coords1, coords2);

				coords1 -= 2.0f*tsep;
				coords2 -= 2.0f*tsep;
				checkForPick(action, pickLine, chemData, chemColor,
					this, from, to, theBond, coords1, coords2);

				coords1 -= tsep;
				coords2 -= tsep;
				checkForPick(action, pickLine, chemData, chemColor,
					this, from, to, theBond, coords1, coords2);
			}
			else if (bondAttributes[i].type == ChemBaseData::TRIPLE_BOND) {

				checkForPick(action, pickLine, chemData, chemColor,
					this, from, to, theBond, coords1, coords2);

				coords1 += tsep;
				coords2 += tsep;
				checkForPick(action, pickLine, chemData, chemColor,
					this, from, to, theBond, coords1, coords2);

				coords1 -= 2.0f*tsep;
				coords2 -= 2.0f*tsep;
				checkForPick(action, pickLine, chemData, chemColor,
					this, from, to, theBond, coords1, coords2);
			}
			else if ((bondAttributes[i].type == ChemBaseData::DOUBLE_BOND) ||
					 (bondAttributes[i].type == ChemBaseData::RESONANCE_BOND)) {
				if (bondAttributes[i].isSymmetric == FALSE) {
					checkForPick(action, pickLine, chemData, chemColor,
						this, from, to, theBond, coords1, coords2);

					coords1 = bondAttributes[i].start;
					coords2 = bondAttributes[i].end;
					checkForPick(action, pickLine, chemData, chemColor,
						this, from, to, theBond, coords1, coords2);
				}
				else {
					coords1 += tsep2;
					coords2 += tsep2;
					checkForPick(action, pickLine, chemData, chemColor,
						this, from, to, theBond, coords1, coords2);

					coords1 -= tsep;
					coords2 -= tsep;
					checkForPick(action, pickLine, chemData, chemColor,
						this, from, to, theBond, coords1, coords2);
				}
			}
			else {
				checkForPick(action, pickLine, chemData, chemColor,
					this, from, to, theBond, coords1, coords2);
			}
		}
		else {
			checkForPick(action, pickLine, chemData, chemColor,
				this, from, to, theBond, coords1, coords2);
		}
	}
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Check for a pick with a line from a wireframe model.  If
//	  there is a pick, fill out the pickedPoint information.
//
// Use: static

static void
checkForPick(SoRayPickAction *action, SbLine &pickLine, 
	ChemBaseData *chemData, ChemColor *chemColor, ChemDisplay *chemDisplay,
	int32_t from, int32_t to, int32_t theBond,
	SbVec3f &coords1, SbVec3f &coords2)
//
////////////////////////////////////////////////////////////////////////
{
	float			ratioFromV1;
	SbVec3f			point;
	SbVec3f			norm;
	SoPickedPoint	*pp;
	ChemDetail		*detail;

	if (action->intersect(coords1, coords2, point) &&
		(pp = action->addIntersection(point)) != NULL) {

		ratioFromV1 = ((point - coords1).length() /
			(coords2 - coords1).length());

		// The "normal" will be in the direction of the ray pick
		norm = pickLine.getDirection();
		norm.negate();
		norm.normalize();
		pp->setObjectNormal(norm);

		// The texture coords will be zero
		pp->setObjectTextureCoords(SbVec4f(0.0, 0.0, 0.0, 0.0));

		// Material index
		switch (chemColor->bondColorBinding.getValue()) {
		  case ChemColor::BOND_OVERALL:
			pp->setMaterialIndex(0);
			break;
		  case ChemColor::BOND_PER_BOND:
			pp->setMaterialIndex(theBond);
			break;
		  case ChemColor::BOND_PER_BOND_INDEXED:
			pp->setMaterialIndex(chemData->getBondIndex(theBond));
			break;
		  case ChemColor::BOND_PER_ATOM:
		  case ChemColor::BOND_PER_ATOM_HALF_BONDED:
			switch (chemColor->atomColorBinding.getValue()) {
			  case ChemColor::ATOM_OVERALL:
				pp->setMaterialIndex(0);
				break;
			  case ChemColor::ATOM_PER_ATOM:
				if (ratioFromV1 < 0.5)
					pp->setMaterialIndex(from);
				else
					pp->setMaterialIndex(to);
				break;
			  case ChemColor::ATOM_PER_ATOM_INDEXED:
				if (ratioFromV1 < 0.5)
					pp->setMaterialIndex(chemData->getAtomIndex(from));
				else
					pp->setMaterialIndex(chemData->getAtomIndex(to));
				break;
			  default:
				break;
			}
		  default:
			break;
		}

		// Create a detail for the specific shape
		detail = new ChemDetail;
		detail->setAtomBondIndex(-1, theBond);
		pp->setDetail(detail, chemDisplay);
	}
}

////////////////////////////////////////////////////////////////////////
// 
// Description:
//    Renders the bonds as wireframe.
//    Invoked by the GLRender method.
//
// Use: static   

void
hydrogenBondsNormal
	(const SbIntList list, const SbBool renderAsPoints,
	const SbBool doHydrogens, const ChemBaseData *chemData,
	const ChemDisplayParam *cdp, const ChemColor *chemColor,
	const ChemBondStuff *bondAttributes)
// 
////////////////////////////////////////////////////////////////////////
{
	int32_t listLen;
	if ((listLen = list.getLength()) <= 0) return;

	int32_t i;
	int32_t listIndex;
	int32_t	from, to, theBond;
	SbVec3f coords1, coords2;
	SbColor color1, color2;
	//SbVec3f tcoord, tc0, tc1, tc2;
	SbVec3f tcoord;

	if (chemColor->hydrogenBondColorBinding.getValue() == 
		ChemColor::HBOND_OVERALL) {

		// All bonds are the same color which is the first color in the
		// bond color table.
		const SbColor *bondColors = chemColor->hydrogenBondColor.getValues(0);
		glColor3fv(bondColors[0].getValue());
		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			glEnable(GL_LINE_STIPPLE);
			glLineStipple(cdp->hydrogenBondStippleFactor.getValue(),
				cdp->hydrogenBondStipplePattern.getValue());
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		for (i = 0; i < listLen; i++) {
			listIndex = list[i];
			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());
		}
		glEnd();
		if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);
	}
	else if (chemColor->hydrogenBondColorBinding.getValue() ==
		ChemColor::HBOND_PER_HBOND) {

		// Each bond has a color in the bond color table.  In the table,
		// there is a separate color for each bond.
		const SbColor *bondColors = chemColor->hydrogenBondColor.getValues(0);
		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			glEnable(GL_LINE_STIPPLE);
			glLineStipple(cdp->hydrogenBondStippleFactor.getValue(),
				cdp->hydrogenBondStipplePattern.getValue());
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		for (i = 0; i < listLen; i++) {
			listIndex = list[i];
			theBond = bondAttributes[listIndex].index;
			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			glColor3fv(bondColors[theBond].getValue());
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());
		}
		glEnd();
		if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);
	}
	else if (chemColor->hydrogenBondColorBinding.getValue() ==
		ChemColor::HBOND_PER_HBOND_INDEXED) {

		// Each bond has an index into the bond color table.
		const SbColor *bondColors = chemColor->hydrogenBondColor.getValues(0);
		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			glEnable(GL_LINE_STIPPLE);
			glLineStipple(cdp->hydrogenBondStippleFactor.getValue(),
				cdp->hydrogenBondStipplePattern.getValue());
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		for (i = 0; i < listLen; i++) {
			listIndex = list[i];
			theBond = bondAttributes[listIndex].index;
			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			glColor3fv(bondColors[chemData->getBondIndex(theBond)].getValue());
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());
		}
		glEnd();
		if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);
	}
	else if (chemColor->hydrogenBondColorBinding.getValue() ==
		ChemColor::HBOND_PER_BOND) {
		if (chemColor->bondColorBinding.getValue() == 
			ChemColor::BOND_OVERALL) {

			// All bonds are the same color which is the first color in the
			// bond color table.
			const SbColor *bondColors = chemColor->bondColor.getValues(0);
			glColor3fv(bondColors[0].getValue());
			if (renderAsPoints) {
				glBegin(GL_POINTS);
			}
			else {
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->hydrogenBondStippleFactor.getValue(),
					cdp->hydrogenBondStipplePattern.getValue());
				glLineWidth(cdp->bondWireframeLineWidth.getValue());
				glBegin(GL_LINES);
			}

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
			}
			glEnd();
			if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);


		}
		else if (chemColor->bondColorBinding.getValue() == 
			ChemColor::BOND_PER_ATOM) {

			// The 2 vertices of the line comprising the bond are colored by
			// the atom color (indexed or not)
			const SbColor *atomColors = chemColor->atomColor.getValues(0);
			if (renderAsPoints) {
				glBegin(GL_POINTS);
			}
			else {
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->hydrogenBondStippleFactor.getValue(),
					cdp->hydrogenBondStipplePattern.getValue());
				glLineWidth(cdp->bondWireframeLineWidth.getValue());
				glBegin(GL_LINES);
			}

			if (chemColor->atomColorBinding.getValue() ==
				ChemColor::ATOM_OVERALL) {
				glColor3fv(atomColors[0].getValue());

				for (i = 0; i < listLen; i++) {
					listIndex = list[i];
					from = bondAttributes[listIndex].from;
					to = bondAttributes[listIndex].to;

					if (!doHydrogens) {
						if ((chemData->getAtomicNumber(from) == 1) || 
							(chemData->getAtomicNumber(to) == 1)) continue;
					}
					coords1 = chemData->getAtomCoordinates(from).getValue();
					coords2 = chemData->getAtomCoordinates(to).getValue();
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
				glEnd();
				if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);

			}
			else if (chemColor->atomColorBinding.getValue() ==
				ChemColor::ATOM_PER_ATOM) {

				for (i = 0; i < listLen; i++) {
					listIndex = list[i];
					from = bondAttributes[listIndex].from;
					to = bondAttributes[listIndex].to;

					if (!doHydrogens) {
						if ((chemData->getAtomicNumber(from) == 1) || 
							(chemData->getAtomicNumber(to) == 1)) continue;
					}
					coords1 = chemData->getAtomCoordinates(from).getValue();
					coords2 = chemData->getAtomCoordinates(to).getValue();

					color1 = atomColors[from];
					color2 = atomColors[to];
					glColor3fv(color1.getValue());
					glVertex3fv(coords1.getValue());

					glColor3fv(color2.getValue());
					glVertex3fv(coords2.getValue());
				}
				glEnd();
				if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);

			}
			else if (chemColor->atomColorBinding.getValue() == 
				ChemColor::ATOM_PER_ATOM_INDEXED) {

				for (i = 0; i < listLen; i++) {
					listIndex = list[i];
					from = bondAttributes[listIndex].from;
					to = bondAttributes[listIndex].to;

					if (!doHydrogens) {
						if ((chemData->getAtomicNumber(from) == 1) || 
							(chemData->getAtomicNumber(to) == 1)) continue;
					}
					coords1 = chemData->getAtomCoordinates(from).getValue();
					coords2 = chemData->getAtomCoordinates(to).getValue();

					color1 = atomColors[chemData->getAtomIndex(from)];
					color2 = atomColors[chemData->getAtomIndex(to)];

					glColor3fv(color1.getValue());
					glVertex3fv(coords1.getValue());

					glColor3fv(color2.getValue());
					glVertex3fv(coords2.getValue());
				}
				glEnd();
				if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);

			}
		}
		else if (chemColor->bondColorBinding.getValue() == 
			ChemColor::BOND_PER_ATOM_HALF_BONDED) {

			// Each bond is drawn as 2 lines with each line being the color of
			// one of the atoms (indexed or not????)
			const SbColor *atomColors = chemColor->atomColor.getValues(0);

			if (renderAsPoints) {
				glBegin(GL_POINTS);
			}
			else {
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->hydrogenBondStippleFactor.getValue(),
					cdp->hydrogenBondStipplePattern.getValue());
				glLineWidth(cdp->bondWireframeLineWidth.getValue());
				glBegin(GL_LINES);
			}

			if (chemColor->atomColorBinding.getValue() ==
				ChemColor::ATOM_OVERALL) {
				glColor3fv(atomColors[0].getValue());

				for (i = 0; i < listLen; i++) {
					listIndex = list[i];
					from = bondAttributes[listIndex].from;
					to = bondAttributes[listIndex].to;

					if (!doHydrogens) {
						if ((chemData->getAtomicNumber(from) == 1) || 
							(chemData->getAtomicNumber(to) == 1)) continue;
					}
					coords1 = chemData->getAtomCoordinates(from).getValue();
					coords2 = chemData->getAtomCoordinates(to).getValue();
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
				glEnd();
				if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);

			}
			else if (chemColor->atomColorBinding.getValue() ==
				ChemColor::ATOM_PER_ATOM) {
				for (i = 0; i < listLen; i++) {
					listIndex = list[i];
					from = bondAttributes[listIndex].from;
					to = bondAttributes[listIndex].to;

					if (!doHydrogens) {
						if ((chemData->getAtomicNumber(from) == 1) || 
							(chemData->getAtomicNumber(to) == 1)) continue;
					}
					coords1 = chemData->getAtomCoordinates(from).getValue();
					coords2 = chemData->getAtomCoordinates(to).getValue();

					color1 = atomColors[from];
					color2 = atomColors[to];

					if (color1.getValue() == color2.getValue()) {

						glColor3fv(color1.getValue());
						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
					}
					else {
						tcoord = (coords2 - coords1) * 0.5f + coords1;

						glColor3fv(color1.getValue());
						glVertex3fv(coords1.getValue());
						glVertex3fv(tcoord.getValue());

						glColor3fv(color2.getValue());
						glVertex3fv(tcoord.getValue());
						glVertex3fv(coords2.getValue());
					}
				}
				glEnd();
				if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);

			}
			else if (chemColor->atomColorBinding.getValue() ==
				ChemColor::ATOM_PER_ATOM_INDEXED) {

				for (i = 0; i < listLen; i++) {
					listIndex = list[i];
					from = bondAttributes[listIndex].from;
					to = bondAttributes[listIndex].to;

					if (!doHydrogens) {
						if ((chemData->getAtomicNumber(from) == 1) || 
							(chemData->getAtomicNumber(to) == 1)) continue;
					}
					coords1 = chemData->getAtomCoordinates(from).getValue();
					coords2 = chemData->getAtomCoordinates(to).getValue();

					color1 = atomColors[chemData->getAtomIndex(from)];
					color2 = atomColors[chemData->getAtomIndex(to)];

					if (color1.getValue() == color2.getValue()) {
						glColor3fv(color1.getValue());
						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
					}
					else {
						tcoord = (coords2 - coords1) * 0.5f + coords1;
						glColor3fv(color1.getValue());
						glVertex3fv(coords1.getValue());
						glVertex3fv(tcoord.getValue());

						glColor3fv(color2.getValue());
						glVertex3fv(tcoord.getValue());
						glVertex3fv(coords2.getValue());
					}
				}
				glEnd();
				if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);

			}
		}
		else if (chemColor->bondColorBinding.getValue() ==
			ChemColor::BOND_PER_BOND) {

			// Each bond has a color in the bond color table.  In the table,
			// there is a separate color for each bond.
			const SbColor *bondColors = chemColor->bondColor.getValues(0);
			if (renderAsPoints) {
				glBegin(GL_POINTS);
			}
			else {
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->hydrogenBondStippleFactor.getValue(),
					cdp->hydrogenBondStipplePattern.getValue());
				glLineWidth(cdp->bondWireframeLineWidth.getValue());
				glBegin(GL_LINES);
			}

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				theBond = bondAttributes[listIndex].index;
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				glColor3fv(bondColors[theBond].getValue());
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
			}
			glEnd();
			if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);

		}
		else if (chemColor->bondColorBinding.getValue() ==
			ChemColor::BOND_PER_BOND_INDEXED) {

			// Each bond has an index into the bond color table.
			const SbColor *bondColors = chemColor->bondColor.getValues(0);
			if (renderAsPoints) {
				glBegin(GL_POINTS);
			}
			else {
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->hydrogenBondStippleFactor.getValue(),
					cdp->hydrogenBondStipplePattern.getValue());
				glLineWidth(cdp->bondWireframeLineWidth.getValue());
				glBegin(GL_LINES);
			}

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				theBond = bondAttributes[listIndex].index;
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				glColor3fv(bondColors[chemData->getBondIndex(theBond)].getValue());
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
			}
			glEnd();
			if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);

		}
	}
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	   Performs highlighting of the molecule using lines.
//	   Invoked by the GLRender method.
//
// Use: static

void
hydrogenBondsHighlighted
	(const SbIntList list, const SbBool renderAsPoints,
	const SbBool doHydrogens, const SbColor highlightColor,
	const int32_t highlightStyle,
	const ChemBaseData *chemData,
	const ChemDisplayParam *cdp, const ChemColor *chemColor,
	const ChemBondStuff *bondAttributes)
//
////////////////////////////////////////////////////////////////////////
{
	int32_t listLen;
	if ((listLen = list.getLength()) <= 0) return;

	int32_t i;
	int32_t listIndex;
	int32_t	from, to, theBond;
	SbVec3f coords1, coords2;
	//SbVec3f tcoord, tc0, tc1, tc2;
	SbVec3f tcoord;
	SbColor tmpColor, color1, color2;

	if (chemColor->hydrogenBondColorBinding.getValue() == 
		ChemColor::HBOND_OVERALL) {

		// All bonds are the same color which is the first color in the
		// bond color table.
		const SbColor *bondColors = chemColor->hydrogenBondColor.getValues(0);

		// We should only be here if highlightStyle is either
		// HIGHLIGHT_DRAWSTYLE or HIGHLIGHT_EMISSIVE.  If
		// HIGHLIGHT_DIFFUSE, then highlightWireframeOverall should be called.

		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
			glColor3fv(bondColors[0].getValue());
		}
		else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			tmpColor = bondColors[0] + highlightColor;
			tmpColor.normalize();
			glColor3fv(tmpColor.getValue());
		}

		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			glEnable(GL_LINE_STIPPLE);
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glLineStipple(cdp->highlightStippleFactor.getValue(),
					cdp->highlightStippleFactor.getValue());
			}
			else {
				glLineStipple(cdp->hydrogenBondStippleFactor.getValue(),
					cdp->hydrogenBondStipplePattern.getValue());
			}
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		for (i = 0; i < listLen; i++) {
			listIndex = list[i];
			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());
		}
		glEnd();
		if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);
	}
	else if (chemColor->hydrogenBondColorBinding.getValue() ==
		ChemColor::HBOND_PER_HBOND) {

		// Each bond has a color in the bond color table.  In the table,
		// there is a separate color for each bond.
		const SbColor *bondColors = chemColor->hydrogenBondColor.getValues(0);

		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			glEnable(GL_LINE_STIPPLE);
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glLineStipple(cdp->highlightStippleFactor.getValue(),
					cdp->highlightStippleFactor.getValue());
			}
			else {
				glLineStipple(cdp->hydrogenBondStippleFactor.getValue(),
					cdp->hydrogenBondStipplePattern.getValue());
			}
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		for (i = 0; i < listLen; i++) {
			listIndex = list[i];
			theBond = bondAttributes[listIndex].index;
			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				tmpColor = bondColors[theBond] + highlightColor;
				tmpColor.normalize();
				glColor3fv(tmpColor.getValue());
			}
			else {
				glColor3fv(bondColors[theBond].getValue());
			}
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());
		}
		glEnd();
		if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);
	}
	else if (chemColor->hydrogenBondColorBinding.getValue() ==
		ChemColor::HBOND_PER_HBOND_INDEXED) {

		// Each bond has an index into the bond color table.
		const SbColor *bondColors = chemColor->hydrogenBondColor.getValues(0);

		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			glEnable(GL_LINE_STIPPLE);
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glLineStipple(cdp->highlightStippleFactor.getValue(),
					cdp->highlightStippleFactor.getValue());
			}
			else {
				glLineStipple(cdp->hydrogenBondStippleFactor.getValue(),
					cdp->hydrogenBondStipplePattern.getValue());
			}
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		for (i = 0; i < listLen; i++) {
			listIndex = list[i];
			theBond = bondAttributes[listIndex].index;
			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				tmpColor = bondColors[chemData->getBondIndex(theBond)] +
					highlightColor;
				tmpColor.normalize();
				glColor3fv(tmpColor.getValue());
			}
			else {
				glColor3fv(bondColors[chemData->getBondIndex(theBond)].getValue());
			}
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());
		}
		glEnd();
		if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);
	}
	else if (chemColor->hydrogenBondColorBinding.getValue() ==
		ChemColor::HBOND_PER_BOND) {
		if (chemColor->bondColorBinding.getValue() == 
			ChemColor::BOND_OVERALL) {

			// All bonds are the same color which is the first color in the
			// bond color table.
			const SbColor *bondColors = chemColor->bondColor.getValues(0);

			// We should only be here if highlightStyle is either
			// HIGHLIGHT_DRAWSTYLE or HIGHLIGHT_EMISSIVE.  If
			// HIGHLIGHT_DIFFUSE, then highlightWireframeOverall should be 
			// called.

			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glColor3fv(bondColors[0].getValue());
			}
			else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				tmpColor = bondColors[0] + highlightColor;
				tmpColor.normalize();
				glColor3fv(tmpColor.getValue());
			}

			if (renderAsPoints) {
				glBegin(GL_POINTS);
			}
			else {
				glEnable(GL_LINE_STIPPLE);
				if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
					glLineStipple(cdp->highlightStippleFactor.getValue(),
						cdp->highlightStipplePattern.getValue());
				}
				else {
					glLineStipple(cdp->hydrogenBondStippleFactor.getValue(),
						cdp->hydrogenBondStipplePattern.getValue());
				}
				glLineWidth(cdp->bondWireframeLineWidth.getValue());
				glBegin(GL_LINES);
			}

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				 glVertex3fv(coords1.getValue());
				 glVertex3fv(coords2.getValue());
			}
			glEnd();
			if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);
		}
		else if (chemColor->bondColorBinding.getValue() == 
			ChemColor::BOND_PER_ATOM) {

			// The 2 vertices of the line comprising the bond are colored by
			// the atom color (indexed or not)
			const SbColor *atomColors = chemColor->atomColor.getValues(0);

			if (renderAsPoints) {
				glBegin(GL_POINTS);
			}
			else {
				glEnable(GL_LINE_STIPPLE);
				if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
					glLineStipple(cdp->highlightStippleFactor.getValue(),
						cdp->highlightStipplePattern.getValue());
				}
				else {
					glLineStipple(cdp->hydrogenBondStippleFactor.getValue(),
						cdp->hydrogenBondStipplePattern.getValue());
				}
				glLineWidth(cdp->bondWireframeLineWidth.getValue());
				glBegin(GL_LINES);
			}

			if (chemColor->atomColorBinding.getValue() ==
				ChemColor::ATOM_OVERALL) {
				if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
					tmpColor = atomColors[0] + highlightColor;
					tmpColor.normalize();
					glColor3fv(tmpColor.getValue());
				}
				else {
					glColor3fv(atomColors[0].getValue());
				}

				for (i = 0; i < listLen; i++) {
					listIndex = list[i];
					from = bondAttributes[listIndex].from;
					to = bondAttributes[listIndex].to;

					if (!doHydrogens) {
						if ((chemData->getAtomicNumber(from) == 1) || 
							(chemData->getAtomicNumber(to) == 1)) continue;
					}
					coords1 = chemData->getAtomCoordinates(from).getValue();
					coords2 = chemData->getAtomCoordinates(to).getValue();

					 glVertex3fv(coords1.getValue());
					 glVertex3fv(coords2.getValue());
				}
				glEnd();
				if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);
			}
			else if (chemColor->atomColorBinding.getValue() ==
				ChemColor::ATOM_PER_ATOM) {

				for (i = 0; i < listLen; i++) {
					listIndex = list[i];
					from = bondAttributes[listIndex].from;
					to = bondAttributes[listIndex].to;

					if (!doHydrogens) {
						if ((chemData->getAtomicNumber(from) == 1) || 
							(chemData->getAtomicNumber(to) == 1)) continue;
					}
					coords1 = chemData->getAtomCoordinates(from).getValue();
					coords2 = chemData->getAtomCoordinates(to).getValue();

					if (highlightStyle ==
						ChemDisplayParam::HIGHLIGHT_EMISSIVE) {

						color1 = atomColors[from] + highlightColor;
						color1.normalize();

						color2 = atomColors[to] + highlightColor;
						color2.normalize();
					}
					else {
						color1 = atomColors[from];
						color2 = atomColors[to];
					}
					glColor3fv(color1.getValue());
					glVertex3fv(coords1.getValue());

					glColor3fv(color2.getValue());
					glVertex3fv(coords2.getValue());
				}
				glEnd();
				if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);
			}
			else if (chemColor->atomColorBinding.getValue() == 
				ChemColor::ATOM_PER_ATOM_INDEXED) {

				for (i = 0; i < listLen; i++) {
					listIndex = list[i];
					from = bondAttributes[listIndex].from;
					to = bondAttributes[listIndex].to;

					if (!doHydrogens) {
						if ((chemData->getAtomicNumber(from) == 1) || 
							(chemData->getAtomicNumber(to) == 1)) continue;
					}
					coords1 = chemData->getAtomCoordinates(from).getValue();
					coords2 = chemData->getAtomCoordinates(to).getValue();

					if (highlightStyle ==
						ChemDisplayParam::HIGHLIGHT_EMISSIVE) {

						color1 = atomColors[chemData->getAtomIndex(from)] +
							highlightColor;
						color1.normalize();

						color2 = atomColors[chemData->getAtomIndex(to)] +
							highlightColor;
						color2.normalize();
					}
					else {
						color1 = atomColors[chemData->getAtomIndex(from)];
						color2 = atomColors[chemData->getAtomIndex(to)];
					}

					glColor3fv(color1.getValue());
					glVertex3fv(coords1.getValue());

					glColor3fv(color2.getValue());
					glVertex3fv(coords2.getValue());
				}
				glEnd();
				if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);
			}
		}
		else if (chemColor->bondColorBinding.getValue() == 
			ChemColor::BOND_PER_ATOM_HALF_BONDED) {

			// Each bond is drawn as 2 lines with each line being the color of
			// one of the atoms (indexed or not????)
			const SbColor *atomColors = chemColor->atomColor.getValues(0);

			if (renderAsPoints) {
				glBegin(GL_POINTS);
			}
			else {
				glEnable(GL_LINE_STIPPLE);
				if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
					glLineStipple(cdp->highlightStippleFactor.getValue(),
						cdp->highlightStipplePattern.getValue());
				}
				else {
					glLineStipple(cdp->hydrogenBondStippleFactor.getValue(),
						cdp->hydrogenBondStipplePattern.getValue());
				}
				glLineWidth(cdp->bondWireframeLineWidth.getValue());
				glBegin(GL_LINES);
			}

			if (chemColor->atomColorBinding.getValue() ==
				ChemColor::ATOM_OVERALL) {
				if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
					tmpColor = atomColors[0] + highlightColor;
					tmpColor.normalize();
					glColor3fv(tmpColor.getValue());
				}
				else {
					glColor3fv(atomColors[0].getValue());
				}

				for (i = 0; i < listLen; i++) {
					listIndex = list[i];
					from = bondAttributes[listIndex].from;
					to = bondAttributes[listIndex].to;

					if (!doHydrogens) {
						if ((chemData->getAtomicNumber(from) == 1) || 
							(chemData->getAtomicNumber(to) == 1)) continue;
					}
					coords1 = chemData->getAtomCoordinates(from).getValue();
					coords2 = chemData->getAtomCoordinates(to).getValue();

					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
				glEnd();
				if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);
			}
			else if (chemColor->atomColorBinding.getValue() ==
				ChemColor::ATOM_PER_ATOM) {

				for (i = 0; i < listLen; i++) {
					listIndex = list[i];
					from = bondAttributes[listIndex].from;
					to = bondAttributes[listIndex].to;

					if (!doHydrogens) {
						if ((chemData->getAtomicNumber(from) == 1) || 
							(chemData->getAtomicNumber(to) == 1)) continue;
					}

					coords1 = chemData->getAtomCoordinates(from).getValue();
					coords2 = chemData->getAtomCoordinates(to).getValue();

					if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
						color1 = atomColors[from] + highlightColor;
						color1.normalize();

						color2 = atomColors[to] + highlightColor;
						color2.normalize();
					}
					else {
						color1 = atomColors[from];
						color2 = atomColors[to];
					}

					if (color1 == color2) {
						glColor3fv(color1.getValue());
						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
					}
					else {
						tcoord = (coords2 - coords1) * 0.5f + coords1;

						glColor3fv(color1.getValue());
						glVertex3fv(coords1.getValue());
						glVertex3fv(tcoord.getValue());

						glColor3fv(color2.getValue());
						glVertex3fv(tcoord.getValue());
						glVertex3fv(coords2.getValue());
					}
				}
				glEnd();
				if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);
			}
			else if (chemColor->atomColorBinding.getValue() ==
				ChemColor::ATOM_PER_ATOM_INDEXED) {

				for (i = 0; i < listLen; i++) {
					listIndex = list[i];
					from = bondAttributes[listIndex].from;
					to = bondAttributes[listIndex].to;

					if (!doHydrogens) {
						if ((chemData->getAtomicNumber(from) == 1) || 
							(chemData->getAtomicNumber(to) == 1)) continue;
					}
					coords1 = chemData->getAtomCoordinates(from).getValue();
					coords2 = chemData->getAtomCoordinates(to).getValue();

					if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
						color1 = atomColors[chemData->getAtomIndex(from)];
						color1 += highlightColor;
						color1.normalize();

						color2 = atomColors[chemData->getAtomIndex(to)];
						color2 += highlightColor;
						color2.normalize();
					}
					else {
						color1 = atomColors[chemData->getAtomIndex(from)];
						color2 = atomColors[chemData->getAtomIndex(to)];
					}

					if (color1 == color2) {
						glColor3fv(color1.getValue());
						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
					}
					else {
						tcoord = (coords2 - coords1) * 0.5f + coords1;

						glColor3fv(color1.getValue());
						glVertex3fv(coords1.getValue());
						glVertex3fv(tcoord.getValue());

						glColor3fv(color2.getValue());
						glVertex3fv(tcoord.getValue());
						glVertex3fv(coords2.getValue());
					}
				}
				glEnd();
				if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);
			}
		}
		else if (chemColor->bondColorBinding.getValue() ==
			ChemColor::BOND_PER_BOND) {

			// Each bond has a color in the bond color table.  In the table,
			// there is a separate color for each bond.
			const SbColor *bondColors = chemColor->bondColor.getValues(0);

			if (renderAsPoints) {
				glBegin(GL_POINTS);
			}
			else {
				glEnable(GL_LINE_STIPPLE);
				if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
					glLineStipple(cdp->highlightStippleFactor.getValue(),
						cdp->highlightStipplePattern.getValue());
				}
				else {
					glLineStipple(cdp->hydrogenBondStippleFactor.getValue(),
						cdp->hydrogenBondStipplePattern.getValue());
				}
				glLineWidth(cdp->bondWireframeLineWidth.getValue());
				glBegin(GL_LINES);
			}

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				theBond = bondAttributes[listIndex].index;
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}

				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
					tmpColor = bondColors[theBond] + highlightColor;
					tmpColor.normalize();
					glColor3fv(tmpColor.getValue());
				}
				else {
					glColor3fv(bondColors[theBond].getValue());
				}

				 glVertex3fv(coords1.getValue());
				 glVertex3fv(coords2.getValue());
			}
			glEnd();
			if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);
		}
		else if (chemColor->bondColorBinding.getValue() ==
			ChemColor::BOND_PER_BOND_INDEXED) {

			// Each bond has an index into the bond color table.
			const SbColor *bondColors = chemColor->bondColor.getValues(0);

			if (renderAsPoints) {
				glBegin(GL_POINTS);
			}
			else {
				glEnable(GL_LINE_STIPPLE);
				if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
					glLineStipple(cdp->highlightStippleFactor.getValue(),
						cdp->highlightStipplePattern.getValue());
				}
				else {
					glLineStipple(cdp->hydrogenBondStippleFactor.getValue(),
						cdp->hydrogenBondStipplePattern.getValue());
				}
				glLineWidth(cdp->bondWireframeLineWidth.getValue());
				glBegin(GL_LINES);
			}

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				theBond = bondAttributes[listIndex].index;
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
					tmpColor = bondColors[chemData->getBondIndex(theBond)] +
						highlightColor;
					tmpColor.normalize();
					glColor3fv(tmpColor.getValue());
				}
				else {
					glColor3fv(bondColors[chemData->getBondIndex(theBond)].getValue());
				}

				 glVertex3fv(coords1.getValue());
				 glVertex3fv(coords2.getValue());
			}
			glEnd();
			if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);
		}
	}
}


////////////////////////////////////////////////////////////////////////
// 
// Description:
//    Renders the bonds as wireframe.
//    Invoked by the GLRender method.
//
// Use: static   

void
singleBondsNormal
	(const SbIntList list, const SbBool renderAsPoints,
	const SbBool doHydrogens, const ChemBaseData *chemData,
	const ChemDisplayParam *cdp, const ChemColor *chemColor,
	const ChemBondStuff *bondAttributes)
// 
////////////////////////////////////////////////////////////////////////
{
	int32_t listLen;
	if ((listLen = list.getLength()) <= 0) return;

	int32_t i;
	int32_t listIndex;
	int32_t	from, to, theBond;
	SbVec3f coords1, coords2;
	SbColor color1, color2;
	//SbVec3f tcoord, tc0, tc1, tc2;
	SbVec3f tcoord;


	if (chemColor->bondColorBinding.getValue() == 
		ChemColor::BOND_OVERALL) {

		// All bonds are the same color which is the first color in the
		// bond color table.
		const SbColor *bondColors = chemColor->bondColor.getValues(0);
		glColor3fv(bondColors[0].getValue());
		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		for (i = 0; i < listLen; i++) {
			listIndex = list[i];
			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());
		}
		glEnd();


	}
	else if (chemColor->bondColorBinding.getValue() == 
		ChemColor::BOND_PER_ATOM) {

		// The 2 vertices of the line comprising the bond are colored by
		// the atom color (indexed or not)
		const SbColor *atomColors = chemColor->atomColor.getValues(0);
		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_OVERALL) {
			glColor3fv(atomColors[0].getValue());

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
			}
			glEnd();

		}
		else if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_PER_ATOM) {

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				color1 = atomColors[from];
				color2 = atomColors[to];
				glColor3fv(color1.getValue());
				glVertex3fv(coords1.getValue());

				glColor3fv(color2.getValue());
				glVertex3fv(coords2.getValue());
			}
			glEnd();

		}
		else if (chemColor->atomColorBinding.getValue() == 
			ChemColor::ATOM_PER_ATOM_INDEXED) {

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				color1 = atomColors[chemData->getAtomIndex(from)];
				color2 = atomColors[chemData->getAtomIndex(to)];

				glColor3fv(color1.getValue());
				glVertex3fv(coords1.getValue());

				glColor3fv(color2.getValue());
				glVertex3fv(coords2.getValue());
			}
			glEnd();

		}
	}
	else if (chemColor->bondColorBinding.getValue() == 
		ChemColor::BOND_PER_ATOM_HALF_BONDED) {

		// Each bond is drawn as 2 lines with each line being the color of
		// one of the atoms (indexed or not????)
		const SbColor *atomColors = chemColor->atomColor.getValues(0);

		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_OVERALL) {
			glColor3fv(atomColors[0].getValue());

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
			}
			glEnd();

		}
		else if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_PER_ATOM) {
			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				color1 = atomColors[from];
				color2 = atomColors[to];

				if (color1.getValue() == color2.getValue()) {

					glColor3fv(color1.getValue());
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
				else {
					tcoord = (coords2 - coords1) * 0.5f + coords1;

					glColor3fv(color1.getValue());
					glVertex3fv(coords1.getValue());
					glVertex3fv(tcoord.getValue());

					glColor3fv(color2.getValue());
					glVertex3fv(tcoord.getValue());
					glVertex3fv(coords2.getValue());
				}
			}
			glEnd();

		}
		else if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_PER_ATOM_INDEXED) {

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				color1 = atomColors[chemData->getAtomIndex(from)];
				color2 = atomColors[chemData->getAtomIndex(to)];

				if (color1.getValue() == color2.getValue()) {
					glColor3fv(color1.getValue());
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
				else {
					tcoord = (coords2 - coords1) * 0.5f + coords1;
					glColor3fv(color1.getValue());
					glVertex3fv(coords1.getValue());
					glVertex3fv(tcoord.getValue());

					glColor3fv(color2.getValue());
					glVertex3fv(tcoord.getValue());
					glVertex3fv(coords2.getValue());
				}
			}
			glEnd();

		}
	}
	else if (chemColor->bondColorBinding.getValue() ==
		ChemColor::BOND_PER_BOND) {

		// Each bond has a color in the bond color table.  In the table,
		// there is a separate color for each bond.
		const SbColor *bondColors = chemColor->bondColor.getValues(0);
		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		for (i = 0; i < listLen; i++) {
			listIndex = list[i];
			theBond = bondAttributes[listIndex].index;
			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			glColor3fv(bondColors[theBond].getValue());
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());
		}
		glEnd();

	}
	else if (chemColor->bondColorBinding.getValue() ==
		ChemColor::BOND_PER_BOND_INDEXED) {

		// Each bond has an index into the bond color table.
		const SbColor *bondColors = chemColor->bondColor.getValues(0);
		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		for (i = 0; i < listLen; i++) {
			listIndex = list[i];
			theBond = bondAttributes[listIndex].index;
			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			glColor3fv(bondColors[chemData->getBondIndex(theBond)].getValue());
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());
		}
		glEnd();

	}
}

////////////////////////////////////////////////////////////////////////
// 
// Description:
//    Renders the bonds as wireframe.
//    Invoked by the GLRender method.
//
// Use: static   

void
doubleBondsNormal
	(const SbIntList list, const SbBool renderAsPoints,
	const SbBool doHydrogens, const ChemBaseData *chemData,
	const ChemDisplayParam *cdp, const ChemColor *chemColor,
	const ChemBondStuff *bondAttributes)
// 
////////////////////////////////////////////////////////////////////////
{
	int32_t listLen;
	if ((listLen = list.getLength()) <= 0) return;

	int32_t i;
	int32_t listIndex;
	int32_t	from, to, theBond;
	SbVec3f coords1, coords2;
	SbColor color1, color2;
	SbVec3f tcoord, tc0, tc1, tc2;

	float   multiSep = cdp->multipleBondSeparation.getValue();
	float   multiSep2 = multiSep * 0.5;
	SbVec3f tsep, tsep2;

	if (chemColor->bondColorBinding.getValue() == 
		ChemColor::BOND_OVERALL) {

		// All bonds are the same color which is the first color in the
		// bond color table.
		const SbColor *bondColors = chemColor->bondColor.getValues(0);
		glColor3fv(bondColors[0].getValue());
		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		for (i = 0; i < listLen; i++) {
			listIndex = list[i];
			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();
			tsep = bondAttributes[listIndex].perpNormal * multiSep;
			tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

			if (bondAttributes[listIndex].isSymmetric == FALSE) {
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
				coords1 = bondAttributes[listIndex].start;
				coords2 = bondAttributes[listIndex].end;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
			}
			else {
				coords1 += tsep2;
				coords2 += tsep2;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());

				coords1 -= tsep;
				coords2 -= tsep;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
			}
		}
		glEnd();


	}
	else if (chemColor->bondColorBinding.getValue() == 
		ChemColor::BOND_PER_ATOM) {

		// The 2 vertices of the line comprising the bond are colored by
		// the atom color (indexed or not)
		const SbColor *atomColors = chemColor->atomColor.getValues(0);
		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_OVERALL) {
			glColor3fv(atomColors[0].getValue());

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();
				tsep = bondAttributes[listIndex].perpNormal * multiSep;
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

				if (bondAttributes[listIndex].isSymmetric == FALSE) {
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
					coords1 = bondAttributes[listIndex].start;
					coords2 = bondAttributes[listIndex].end;
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
				else {
					coords1 += tsep2;
					coords2 += tsep2;
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());

					coords1 -= tsep;
					coords2 -= tsep;
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
			}
			glEnd();

		}
		else if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_PER_ATOM) {

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				color1 = atomColors[from];
				color2 = atomColors[to];
				tsep = bondAttributes[listIndex].perpNormal * multiSep;
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

				if (bondAttributes[listIndex].isSymmetric == FALSE) {
					glColor3fv(color1.getValue());
					glVertex3fv(coords1.getValue());
					glColor3fv(color2.getValue());
					glVertex3fv(coords2.getValue());

					coords1 = bondAttributes[listIndex].start;
					coords2 = bondAttributes[listIndex].end;

					glColor3fv(color1.getValue());
					glVertex3fv(coords1.getValue());
					glColor3fv(color2.getValue());
					glVertex3fv(coords2.getValue());
				}
				else {
					coords1 += tsep2;
					coords2 += tsep2;

					glColor3fv(color1.getValue());
					glVertex3fv(coords1.getValue());
					glColor3fv(color2.getValue());
					glVertex3fv(coords2.getValue());

					coords1 -= tsep;
					coords2 -= tsep;

					glColor3fv(color1.getValue());
					glVertex3fv(coords1.getValue());
					glColor3fv(color2.getValue());
					glVertex3fv(coords2.getValue());
				}
			}
			glEnd();

		}
		else if (chemColor->atomColorBinding.getValue() == 
			ChemColor::ATOM_PER_ATOM_INDEXED) {

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				color1 = atomColors[chemData->getAtomIndex(from)];
				color2 = atomColors[chemData->getAtomIndex(to)];

				tsep = bondAttributes[listIndex].perpNormal * multiSep;
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

				if (bondAttributes[listIndex].isSymmetric == FALSE) {
					glColor3fv(color1.getValue());
					glVertex3fv(coords1.getValue());
					glColor3fv(color2.getValue());
					glVertex3fv(coords2.getValue());

					coords1 = bondAttributes[listIndex].start;
					coords2 = bondAttributes[listIndex].end;

					glColor3fv(color1.getValue());
					glVertex3fv(coords1.getValue());
					glColor3fv(color2.getValue());
					glVertex3fv(coords2.getValue());
				}
				else {
					coords1 += tsep2;
					coords2 += tsep2;

					glColor3fv(color1.getValue());
					glVertex3fv(coords1.getValue());
					glColor3fv(color2.getValue());
					glVertex3fv(coords2.getValue());

					coords1 -= tsep;
					coords2 -= tsep;

					glColor3fv(color1.getValue());
					glVertex3fv(coords1.getValue());
					glColor3fv(color2.getValue());
					glVertex3fv(coords2.getValue());
				}
			}
			glEnd();

		}
	}
	else if (chemColor->bondColorBinding.getValue() == 
		ChemColor::BOND_PER_ATOM_HALF_BONDED) {

		// Each bond is drawn as 2 lines with each line being the color of
		// one of the atoms (indexed or not????)
		const SbColor *atomColors = chemColor->atomColor.getValues(0);

		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_OVERALL) {
			glColor3fv(atomColors[0].getValue());

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();
				tsep = bondAttributes[listIndex].perpNormal * multiSep;
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

				if (bondAttributes[listIndex].isSymmetric == FALSE) {
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
					coords1 = bondAttributes[listIndex].start;
					coords2 = bondAttributes[listIndex].end;
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
				else {
					coords1 += tsep2;
					coords2 += tsep2;
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());

					coords1 -= tsep;
					coords2 -= tsep;
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
			}
			glEnd();

		}
		else if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_PER_ATOM) {
			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				color1 = atomColors[from];
				color2 = atomColors[to];

				tsep = bondAttributes[listIndex].perpNormal * multiSep;
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

				if (color1 == color2) {
					glColor3fv(color1.getValue());
					if (bondAttributes[listIndex].isSymmetric == FALSE) {
						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());

						coords1 = bondAttributes[listIndex].start;
						coords2 = bondAttributes[listIndex].end;

						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
					}
					else {
						coords1 += tsep2;
						coords2 += tsep2;

						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());

						coords1 -= tsep;
						coords2 -= tsep;

						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
					}
				}
				// If the 2 ends are not the same color, find the midpoint
				// and draw half-bonds
				else {
					tcoord = (coords2 - coords1) * 0.5f + coords1;
					if (bondAttributes[listIndex].isSymmetric == FALSE) {
						tc0 = tcoord + tsep;
						tc1 = bondAttributes[listIndex].start;
						tc2 = bondAttributes[listIndex].end;

						glColor3fv(color1.getValue());
						glVertex3fv(coords1.getValue());
						glVertex3fv(tcoord.getValue());
						glVertex3fv(tc1.getValue());
						glVertex3fv(tc0.getValue());

						glColor3fv(color2.getValue());
						glVertex3fv(tcoord.getValue());
						glVertex3fv(coords2.getValue());
						glVertex3fv(tc0.getValue());
						glVertex3fv(tc2.getValue());
					}
					else {
						tcoord += tsep2;
						coords1 += tsep2;
						coords2 += tsep2;
						tc0 = tcoord - tsep;
						tc1 = coords1 - tsep;
						tc2 = coords2 - tsep;

						glColor3fv(color1.getValue());
						glVertex3fv(coords1.getValue());
						glVertex3fv(tcoord.getValue());
						glVertex3fv(tc1.getValue());
						glVertex3fv(tc0.getValue());

						glColor3fv(color2.getValue());
						glVertex3fv(tcoord.getValue());
						glVertex3fv(coords2.getValue());
						glVertex3fv(tc0.getValue());
						glVertex3fv(tc2.getValue());
					}
				}
			}
			glEnd();

		}
		else if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_PER_ATOM_INDEXED) {

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				color1 = atomColors[chemData->getAtomIndex(from)];
				color2 = atomColors[chemData->getAtomIndex(to)];

				tsep = bondAttributes[listIndex].perpNormal * multiSep;
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

				if (color1 == color2) {
					glColor3fv(color1.getValue());
					if (bondAttributes[listIndex].isSymmetric == FALSE) {
						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());

						coords1 = bondAttributes[listIndex].start;
						coords2 = bondAttributes[listIndex].end;

						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
					}
					else {
						coords1 += tsep2;
						coords2 += tsep2;

						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());

						coords1 -= tsep;
						coords2 -= tsep;

						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
					}
				}
				// If the 2 ends are not the same color, find the midpoint
				// and draw half-bonds
				else {
					tcoord = (coords2 - coords1) * 0.5f + coords1;
					if (bondAttributes[listIndex].isSymmetric == FALSE) {
						tc0 = tcoord + tsep;
						tc1 = bondAttributes[listIndex].start;
						tc2 = bondAttributes[listIndex].end;

						glColor3fv(color1.getValue());
						glVertex3fv(coords1.getValue());
						glVertex3fv(tcoord.getValue());
						glVertex3fv(tc1.getValue());
						glVertex3fv(tc0.getValue());

						glColor3fv(color2.getValue());
						glVertex3fv(tcoord.getValue());
						glVertex3fv(coords2.getValue());
						glVertex3fv(tc0.getValue());
						glVertex3fv(tc2.getValue());
					}
					else {
						tcoord += tsep2;
						coords1 += tsep2;
						coords2 += tsep2;
						tc0 = tcoord - tsep;
						tc1 = coords1 - tsep;
						tc2 = coords2 - tsep;

						glColor3fv(color1.getValue());
						glVertex3fv(coords1.getValue());
						glVertex3fv(tcoord.getValue());
						glVertex3fv(tc1.getValue());
						glVertex3fv(tc0.getValue());

						glColor3fv(color2.getValue());
						glVertex3fv(tcoord.getValue());
						glVertex3fv(coords2.getValue());
						glVertex3fv(tc0.getValue());
						glVertex3fv(tc2.getValue());
					}
				}
			}
			glEnd();

		}
	}
	else if (chemColor->bondColorBinding.getValue() ==
		ChemColor::BOND_PER_BOND) {

		// Each bond has a color in the bond color table.  In the table,
		// there is a separate color for each bond.
		const SbColor *bondColors = chemColor->bondColor.getValues(0);
		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		for (i = 0; i < listLen; i++) {
			listIndex = list[i];
			theBond = bondAttributes[listIndex].index;
			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			glColor3fv(bondColors[theBond].getValue());
			tsep = bondAttributes[listIndex].perpNormal * multiSep;
			tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

			if (bondAttributes[listIndex].isSymmetric == FALSE) {
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
				coords1 = bondAttributes[listIndex].start;
				coords2 = bondAttributes[listIndex].end;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
			}
			else {
				coords1 += tsep2;
				coords2 += tsep2;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());

				coords1 -= tsep;
				coords2 -= tsep;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
			}
		}
		glEnd();

	}
	else if (chemColor->bondColorBinding.getValue() ==
		ChemColor::BOND_PER_BOND_INDEXED) {

		// Each bond has an index into the bond color table.
		const SbColor *bondColors = chemColor->bondColor.getValues(0);
		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		for (i = 0; i < listLen; i++) {
			listIndex = list[i];
			theBond = bondAttributes[listIndex].index;
			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			glColor3fv(bondColors[chemData->getBondIndex(theBond)].getValue());
			tsep = bondAttributes[listIndex].perpNormal * multiSep;
			tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

			if (bondAttributes[listIndex].isSymmetric == FALSE) {
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
				coords1 = bondAttributes[listIndex].start;
				coords2 = bondAttributes[listIndex].end;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
			}
			else {
				coords1 += tsep2;
				coords2 += tsep2;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());

				coords1 -= tsep;
				coords2 -= tsep;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
			}
		}
		glEnd();

	}
}

////////////////////////////////////////////////////////////////////////
// 
// Description:
//    Renders the bonds as wireframe.
//    Invoked by the GLRender method.
//
// Use: static   

void
tripleBondsNormal
	(const SbIntList list, const SbBool renderAsPoints,
	const SbBool doHydrogens, const ChemBaseData *chemData,
	const ChemDisplayParam *cdp, const ChemColor *chemColor,
	const ChemBondStuff *bondAttributes)
// 
////////////////////////////////////////////////////////////////////////
{
	int32_t listLen;
	if ((listLen = list.getLength()) <= 0) return;

	int32_t i;
	int32_t listIndex;
	int32_t	from, to, theBond;
	SbVec3f coords1, coords2;
	SbColor color1, color2;
	SbVec3f tcoord, tc0, tc1, tc2;

	float   multiSep = cdp->multipleBondSeparation.getValue();
	float   multiSep2 = multiSep * 0.5;
	SbVec3f tsep, tsep2;

	if (chemColor->bondColorBinding.getValue() == 
		ChemColor::BOND_OVERALL) {

		// All bonds are the same color which is the first color in the
		// bond color table.
		const SbColor *bondColors = chemColor->bondColor.getValues(0);
		glColor3fv(bondColors[0].getValue());
		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		for (i = 0; i < listLen; i++) {
			listIndex = list[i];
			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();
			tsep = bondAttributes[listIndex].perpNormal * multiSep;
			tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

			tc1 = coords1 + tsep;
			tc2 = coords2 + tsep;
			glVertex3fv(tc1.getValue());
			glVertex3fv(tc2.getValue());

			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());

			tc1 = coords1 - tsep;
			tc2 = coords2 - tsep;
			glVertex3fv(tc1.getValue());
			glVertex3fv(tc2.getValue());
		}
		glEnd();


	}
	else if (chemColor->bondColorBinding.getValue() == 
		ChemColor::BOND_PER_ATOM) {

		// The 2 vertices of the line comprising the bond are colored by
		// the atom color (indexed or not)
		const SbColor *atomColors = chemColor->atomColor.getValues(0);
		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_OVERALL) {
			glColor3fv(atomColors[0].getValue());

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();
				tsep = bondAttributes[listIndex].perpNormal * multiSep;
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

				tc1 = coords1 + tsep;
				tc2 = coords2 + tsep;
				glVertex3fv(tc1.getValue());
				glVertex3fv(tc2.getValue());

				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());

				tc1 = coords1 - tsep;
				tc2 = coords2 - tsep;
				glVertex3fv(tc1.getValue());
				glVertex3fv(tc2.getValue());
			}
			glEnd();

		}
		else if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_PER_ATOM) {

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				color1 = atomColors[from];
				color2 = atomColors[to];
				tsep = bondAttributes[listIndex].perpNormal * multiSep;
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

				tc1 = coords1 + tsep;
				tc2 = coords2 + tsep;
				glColor3fv(color1.getValue());
				glVertex3fv(tc1.getValue());
				glColor3fv(color2.getValue());
				glVertex3fv(tc2.getValue());

				glColor3fv(color1.getValue());
				glVertex3fv(coords1.getValue());
				glColor3fv(color2.getValue());
				glVertex3fv(coords2.getValue());

				tc1 = coords1 - tsep;
				tc2 = coords2 - tsep;
				glColor3fv(color1.getValue());
				glVertex3fv(tc1.getValue());
				glColor3fv(color2.getValue());
				glVertex3fv(tc2.getValue());
			}
			glEnd();

		}
		else if (chemColor->atomColorBinding.getValue() == 
			ChemColor::ATOM_PER_ATOM_INDEXED) {

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				color1 = atomColors[chemData->getAtomIndex(from)];
				color2 = atomColors[chemData->getAtomIndex(to)];

				tsep = bondAttributes[listIndex].perpNormal * multiSep;
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

				tc1 = coords1 + tsep;
				tc2 = coords2 + tsep;
				glColor3fv(color1.getValue());
				glVertex3fv(tc1.getValue());
				glColor3fv(color2.getValue());
				glVertex3fv(tc2.getValue());

				glColor3fv(color1.getValue());
				glVertex3fv(coords1.getValue());
				glColor3fv(color2.getValue());
				glVertex3fv(coords2.getValue());

				tc1 = coords1 - tsep;
				tc2 = coords2 - tsep;
				glColor3fv(color1.getValue());
				glVertex3fv(tc1.getValue());
				glColor3fv(color2.getValue());
				glVertex3fv(tc2.getValue());
			}
			glEnd();

		}
	}
	else if (chemColor->bondColorBinding.getValue() == 
		ChemColor::BOND_PER_ATOM_HALF_BONDED) {

		// Each bond is drawn as 2 lines with each line being the color of
		// one of the atoms (indexed or not????)
		const SbColor *atomColors = chemColor->atomColor.getValues(0);

		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_OVERALL) {
			glColor3fv(atomColors[0].getValue());

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();
				tsep = bondAttributes[listIndex].perpNormal * multiSep;
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

				tc1 = coords1 + tsep;
				tc2 = coords2 + tsep;
				glVertex3fv(tc1.getValue());
				glVertex3fv(tc2.getValue());

				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());

				tc1 = coords1 - tsep;
				tc2 = coords2 - tsep;
				glVertex3fv(tc1.getValue());
				glVertex3fv(tc2.getValue());
			}
			glEnd();

		}
		else if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_PER_ATOM) {
			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				color1 = atomColors[from];
				color2 = atomColors[to];

				tsep = bondAttributes[listIndex].perpNormal * multiSep;
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

				if (color1 == color2) {
					glColor3fv(color1.getValue());
					SbVec3f tcoord1, tcoord2;

					tcoord1 = coords1 + tsep;
					tcoord2 = coords2 + tsep;
					glVertex3fv(tcoord1.getValue());
					glVertex3fv(tcoord2.getValue());

					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());

					tcoord1 = coords1 - tsep;
					tcoord2 = coords2 - tsep;
					glVertex3fv(tcoord1.getValue());
					glVertex3fv(tcoord2.getValue());
				}
				// If the 2 ends are not the same color, find the midpoint
				// and draw half-bonds
				else {
					tcoord = (coords2 - coords1) * 0.5f + coords1;
					tc0 = tcoord + tsep;
					tc1 = coords1 + tsep;

					// Set the color and draw the first 3 half bonds
					glColor3fv(color1.getValue());
					glVertex3fv(tc1.getValue());
					glVertex3fv(tc0.getValue());
					glVertex3fv(coords1.getValue());
					glVertex3fv(tcoord.getValue());
					tc0 = tcoord - tsep;
					tc1 = coords1 - tsep;
					glVertex3fv(tc1.getValue());
					glVertex3fv(tc0.getValue());

					tc2 = coords2 - tsep;
					// Set the color and draw the second 3 half bonds
					glColor3fv(color2.getValue());
					glVertex3fv(tc0.getValue());
					glVertex3fv(tc2.getValue());
					glVertex3fv(tcoord.getValue());
					glVertex3fv(coords2.getValue());
					tc0 = tcoord + tsep;
					tc2 = coords2 + tsep;
					glVertex3fv(tc0.getValue());
					glVertex3fv(tc2.getValue());
				}
			}
			glEnd();

		}
		else if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_PER_ATOM_INDEXED) {

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				color1 = atomColors[chemData->getAtomIndex(from)];
				color2 = atomColors[chemData->getAtomIndex(to)];

				tsep = bondAttributes[listIndex].perpNormal * multiSep;
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

				if (color1 == color2) {
					glColor3fv(color1.getValue());
					SbVec3f tcoord1, tcoord2;

					tcoord1 = coords1 + tsep;
					tcoord2 = coords2 + tsep;
					glVertex3fv(tcoord1.getValue());
					glVertex3fv(tcoord2.getValue());

					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());

					tcoord1 = coords1 - tsep;
					tcoord2 = coords2 - tsep;
					glVertex3fv(tcoord1.getValue());
					glVertex3fv(tcoord2.getValue());
				}
				// If the 2 ends are not the same color, find the midpoint
				// and draw half-bonds
				else {
					tcoord = (coords2 - coords1) * 0.5f + coords1;
					tc0 = tcoord + tsep;
					tc1 = coords1 + tsep;

					// Set the color and draw the first 3 half bonds
					glColor3fv(color1.getValue());
					glVertex3fv(tc1.getValue());
					glVertex3fv(tc0.getValue());
					glVertex3fv(coords1.getValue());
					glVertex3fv(tcoord.getValue());
					tc0 = tcoord - tsep;
					tc1 = coords1 - tsep;
					glVertex3fv(tc1.getValue());
					glVertex3fv(tc0.getValue());

					tc2 = coords2 - tsep;
					// Set the color and draw the second 3 half bonds
					glColor3fv(color2.getValue());
					glVertex3fv(tc0.getValue());
					glVertex3fv(tc2.getValue());
					glVertex3fv(tcoord.getValue());
					glVertex3fv(coords2.getValue());
					tc0 = tcoord + tsep;
					tc2 = coords2 + tsep;
					glVertex3fv(tc0.getValue());
					glVertex3fv(tc2.getValue());
				}
			}
			glEnd();

		}
	}
	else if (chemColor->bondColorBinding.getValue() ==
		ChemColor::BOND_PER_BOND) {

		// Each bond has a color in the bond color table.  In the table,
		// there is a separate color for each bond.
		const SbColor *bondColors = chemColor->bondColor.getValues(0);
		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		for (i = 0; i < listLen; i++) {
			listIndex = list[i];
			theBond = bondAttributes[listIndex].index;
			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			glColor3fv(bondColors[theBond].getValue());
			tsep = bondAttributes[listIndex].perpNormal * multiSep;
			tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

			tc1 = coords1 + tsep;
			tc2 = coords2 + tsep;
			glVertex3fv(tc1.getValue());
			glVertex3fv(tc2.getValue());

			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());

			tc1 = coords1 - tsep;
			tc2 = coords2 - tsep;
			glVertex3fv(tc1.getValue());
			glVertex3fv(tc2.getValue());
		}
		glEnd();

	}
	else if (chemColor->bondColorBinding.getValue() ==
		ChemColor::BOND_PER_BOND_INDEXED) {

		// Each bond has an index into the bond color table.
		const SbColor *bondColors = chemColor->bondColor.getValues(0);
		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		for (i = 0; i < listLen; i++) {
			listIndex = list[i];
			theBond = bondAttributes[listIndex].index;
			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			glColor3fv(bondColors[chemData->getBondIndex(theBond)].getValue());
			tsep = bondAttributes[listIndex].perpNormal * multiSep;
			tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

			tc1 = coords1 + tsep;
			tc2 = coords2 + tsep;
			glVertex3fv(tc1.getValue());
			glVertex3fv(tc2.getValue());

			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());

			tc1 = coords1 - tsep;
			tc2 = coords2 - tsep;
			glVertex3fv(tc1.getValue());
			glVertex3fv(tc2.getValue());
		}
		glEnd();

	}
}

////////////////////////////////////////////////////////////////////////
// 
// Description:
//    Renders the bonds as wireframe.
//    Invoked by the GLRender method.
//
// Use: static   

void
quadrupleBondsNormal
	(const SbIntList list, const SbBool renderAsPoints,
	const SbBool doHydrogens, const ChemBaseData *chemData,
	const ChemDisplayParam *cdp, const ChemColor *chemColor,
	const ChemBondStuff *bondAttributes)
// 
////////////////////////////////////////////////////////////////////////
{
	int32_t listLen;
	if ((listLen = list.getLength()) <= 0) return;

	int32_t i;
	int32_t listIndex;
	int32_t	from, to, theBond;
	SbVec3f coords1, coords2;
	SbColor color1, color2;
	SbVec3f tcoord, tc0, tc1, tc2;

	float   multiSep = cdp->multipleBondSeparation.getValue();
	float   multiSep2 = multiSep * 0.5;
	SbVec3f tsep, tsep2;

	if (chemColor->bondColorBinding.getValue() == 
		ChemColor::BOND_OVERALL) {

		// All bonds are the same color which is the first color in the
		// bond color table.
		const SbColor *bondColors = chemColor->bondColor.getValues(0);
		glColor3fv(bondColors[0].getValue());
		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		for (i = 0; i < listLen; i++) {
			listIndex = list[i];
			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();
			tsep = bondAttributes[listIndex].perpNormal * multiSep;
			tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

			coords1 += tsep2;
			coords2 += tsep2;
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());

			coords1 += tsep;
			coords2 += tsep;
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());

			coords1 -= 2.0f*tsep;
			coords2 -= 2.0f*tsep;
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());

			coords1 -= tsep;
			coords2 -= tsep;
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());
		}
		glEnd();


	}
	else if (chemColor->bondColorBinding.getValue() == 
		ChemColor::BOND_PER_ATOM) {

		// The 2 vertices of the line comprising the bond are colored by
		// the atom color (indexed or not)
		const SbColor *atomColors = chemColor->atomColor.getValues(0);
		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_OVERALL) {
			glColor3fv(atomColors[0].getValue());

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();
				tsep = bondAttributes[listIndex].perpNormal * multiSep;
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

				coords1 += tsep2;
				coords2 += tsep2;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());

				coords1 += tsep;
				coords2 += tsep;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());

				coords1 -= 2.0f*tsep;
				coords2 -= 2.0f*tsep;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());

				coords1 -= tsep;
				coords2 -= tsep;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
			}
			glEnd();

		}
		else if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_PER_ATOM) {

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				color1 = atomColors[from];
				color2 = atomColors[to];
				tsep = bondAttributes[listIndex].perpNormal * multiSep;
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

				coords1 += tsep2;
				coords2 += tsep2;

				glColor3fv(color1.getValue());
				glVertex3fv(coords1.getValue());
				glColor3fv(color2.getValue());
				glVertex3fv(coords2.getValue());

				coords1 += tsep;
				coords2 += tsep;

				glColor3fv(color1.getValue());
				glVertex3fv(coords1.getValue());
				glColor3fv(color2.getValue());
				glVertex3fv(coords2.getValue());

				coords1 -= 2.0f*tsep;
				coords2 -= 2.0f*tsep;

				glColor3fv(color1.getValue());
				glVertex3fv(coords1.getValue());
				glColor3fv(color2.getValue());
				glVertex3fv(coords2.getValue());

				coords1 -= tsep;
				coords2 -= tsep;

				glColor3fv(color1.getValue());
				glVertex3fv(coords1.getValue());
				glColor3fv(color2.getValue());
				glVertex3fv(coords2.getValue());
			}
			glEnd();

		}
		else if (chemColor->atomColorBinding.getValue() == 
			ChemColor::ATOM_PER_ATOM_INDEXED) {

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				color1 = atomColors[chemData->getAtomIndex(from)];
				color2 = atomColors[chemData->getAtomIndex(to)];

				tsep = bondAttributes[listIndex].perpNormal * multiSep;
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

				coords1 += tsep2;
				coords2 += tsep2;

				glColor3fv(color1.getValue());
				glVertex3fv(coords1.getValue());
				glColor3fv(color2.getValue());
				glVertex3fv(coords2.getValue());

				coords1 += tsep;
				coords2 += tsep;

				glColor3fv(color1.getValue());
				glVertex3fv(coords1.getValue());
				glColor3fv(color2.getValue());
				glVertex3fv(coords2.getValue());

				coords1 -= 2.0f*tsep;
				coords2 -= 2.0f*tsep;

				glColor3fv(color1.getValue());
				glVertex3fv(coords1.getValue());
				glColor3fv(color2.getValue());
				glVertex3fv(coords2.getValue());

				coords1 -= tsep;
				coords2 -= tsep;

				glColor3fv(color1.getValue());
				glVertex3fv(coords1.getValue());
				glColor3fv(color2.getValue());
				glVertex3fv(coords2.getValue());
			}
			glEnd();

		}
	}
	else if (chemColor->bondColorBinding.getValue() == 
		ChemColor::BOND_PER_ATOM_HALF_BONDED) {

		// Each bond is drawn as 2 lines with each line being the color of
		// one of the atoms (indexed or not????)
		const SbColor *atomColors = chemColor->atomColor.getValues(0);

		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_OVERALL) {
			glColor3fv(atomColors[0].getValue());

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();
				tsep = bondAttributes[listIndex].perpNormal * multiSep;
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

				coords1 += tsep2;
				coords2 += tsep2;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());

				coords1 += tsep;
				coords2 += tsep;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());

				coords1 -= 2.0f*tsep;
				coords2 -= 2.0f*tsep;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());

				coords1 -= tsep;
				coords2 -= tsep;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
			}
			glEnd();

		}
		else if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_PER_ATOM) {
			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				color1 = atomColors[from];
				color2 = atomColors[to];

				tsep = bondAttributes[listIndex].perpNormal * multiSep;
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

				if (color1 == color2) {
					glColor3fv(color1.getValue());
					coords1 += tsep2;
					coords2 += tsep2;

					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());

					coords1 += tsep;
					coords2 += tsep;

					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());

					coords1 -= 2.0f*tsep;
					coords2 -= 2.0f*tsep;

					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());

					coords1 -= tsep;
					coords2 -= tsep;

					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
				// If the 2 ends are not the same color, find the midpoint
				// and draw half-bonds
				else {
					tcoord = (coords2 - coords1) * 0.5f + coords1;
					// Set the color and draw the first 4 half bonds
					glColor3fv(color1.getValue());

					tc0 = tcoord + tsep2;
					tc1 = coords1 + tsep2;
					glVertex3fv(tc1.getValue());
					glVertex3fv(tc0.getValue());

					tc0 += tsep;
					tc1 += tsep;
					glVertex3fv(tc1.getValue());
					glVertex3fv(tc0.getValue());

					tc0 -= 2.0f*tsep;
					tc1 -= 2.0f*tsep;
					glVertex3fv(tc1.getValue());
					glVertex3fv(tc0.getValue());

					tc0 -= tsep;
					tc1 -= tsep;
					glVertex3fv(tc1.getValue());
					glVertex3fv(tc0.getValue());

					// Set the color and draw the second 4 half bonds
					glColor3fv(color2.getValue());

					tc0 = tcoord + tsep2;
					tc2 = coords2 + tsep2;
					glVertex3fv(tc0.getValue());
					glVertex3fv(tc2.getValue());

					tc0 += tsep;
					tc2 += tsep;
					glVertex3fv(tc0.getValue());
					glVertex3fv(tc2.getValue());

					tc0 -= 2.0f*tsep;
					tc2 -= 2.0f*tsep;
					glVertex3fv(tc0.getValue());
					glVertex3fv(tc2.getValue());

					tc0 -= tsep;
					tc2 -= tsep;
					glVertex3fv(tc0.getValue());
					glVertex3fv(tc2.getValue());
				}
			}
			glEnd();

		}
		else if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_PER_ATOM_INDEXED) {

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				color1 = atomColors[chemData->getAtomIndex(from)];
				color2 = atomColors[chemData->getAtomIndex(to)];

				tsep = bondAttributes[listIndex].perpNormal * multiSep;
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

				if (color1 == color2) {
					glColor3fv(color1.getValue());
					coords1 += tsep2;
					coords2 += tsep2;

					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());

					coords1 += tsep;
					coords2 += tsep;

					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());

					coords1 -= 2.0f*tsep;
					coords2 -= 2.0f*tsep;

					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());

					coords1 -= tsep;
					coords2 -= tsep;

					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
				// If the 2 ends are not the same color, find the midpoint
				// and draw half-bonds
				else {
					tcoord = (coords2 - coords1) * 0.5f + coords1;
					// Set the color and draw the first 4 half bonds
					glColor3fv(color1.getValue());

					tc0 = tcoord + tsep2;
					tc1 = coords1 + tsep2;
					glVertex3fv(tc1.getValue());
					glVertex3fv(tc0.getValue());

					tc0 += tsep;
					tc1 += tsep;
					glVertex3fv(tc1.getValue());
					glVertex3fv(tc0.getValue());

					tc0 -= 2.0f*tsep;
					tc1 -= 2.0f*tsep;
					glVertex3fv(tc1.getValue());
					glVertex3fv(tc0.getValue());

					tc0 -= tsep;
					tc1 -= tsep;
					glVertex3fv(tc1.getValue());
					glVertex3fv(tc0.getValue());

					// Set the color and draw the second 4 half bonds
					glColor3fv(color2.getValue());

					tc0 = tcoord + tsep2;
					tc2 = coords2 + tsep2;
					glVertex3fv(tc0.getValue());
					glVertex3fv(tc2.getValue());

					tc0 += tsep;
					tc2 += tsep;
					glVertex3fv(tc0.getValue());
					glVertex3fv(tc2.getValue());

					tc0 -= 2.0f*tsep;
					tc2 -= 2.0f*tsep;
					glVertex3fv(tc0.getValue());
					glVertex3fv(tc2.getValue());

					tc0 -= tsep;
					tc2 -= tsep;
					glVertex3fv(tc0.getValue());
					glVertex3fv(tc2.getValue());
				}
			}
			glEnd();

		}
	}
	else if (chemColor->bondColorBinding.getValue() ==
		ChemColor::BOND_PER_BOND) {

		// Each bond has a color in the bond color table.  In the table,
		// there is a separate color for each bond.
		const SbColor *bondColors = chemColor->bondColor.getValues(0);
		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		for (i = 0; i < listLen; i++) {
			listIndex = list[i];
			theBond = bondAttributes[listIndex].index;
			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			glColor3fv(bondColors[theBond].getValue());
			tsep = bondAttributes[listIndex].perpNormal * multiSep;
			tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

			coords1 += tsep2;
			coords2 += tsep2;
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());

			coords1 += tsep;
			coords2 += tsep;
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());

			coords1 -= 2.0f*tsep;
			coords2 -= 2.0f*tsep;
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());

			coords1 -= tsep;
			coords2 -= tsep;
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());
		}
		glEnd();

	}
	else if (chemColor->bondColorBinding.getValue() ==
		ChemColor::BOND_PER_BOND_INDEXED) {

		// Each bond has an index into the bond color table.
		const SbColor *bondColors = chemColor->bondColor.getValues(0);
		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		for (i = 0; i < listLen; i++) {
			listIndex = list[i];
			theBond = bondAttributes[listIndex].index;
			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			glColor3fv(bondColors[chemData->getBondIndex(theBond)].getValue());
			tsep = bondAttributes[listIndex].perpNormal * multiSep;
			tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

			coords1 += tsep2;
			coords2 += tsep2;
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());

			coords1 += tsep;
			coords2 += tsep;
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());

			coords1 -= 2.0f*tsep;
			coords2 -= 2.0f*tsep;
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());

			coords1 -= tsep;
			coords2 -= tsep;
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());
		}
		glEnd();

	}
}

////////////////////////////////////////////////////////////////////////
// 
// Description:
//    Renders the bonds as wireframe.
//    Invoked by the GLRender method.
//
// Use: static   

void
resonanceBondsNormal
	(const SbIntList list, const SbBool renderAsPoints,
	const SbBool doHydrogens, const ChemBaseData *chemData,
	const ChemDisplayParam *cdp, const ChemColor *chemColor,
	const ChemBondStuff *bondAttributes)
// 
////////////////////////////////////////////////////////////////////////
{
	int32_t listLen;
	if ((listLen = list.getLength()) <= 0) return;

	int32_t i;
	int32_t listIndex;
	int32_t	from, to, theBond;
	SbVec3f coords1, coords2;
	SbColor color1, color2;
	SbVec3f tcoord, tc0, tc1, tc2;

	float   multiSep = cdp->multipleBondSeparation.getValue();
	float   multiSep2 = multiSep * 0.5;
	//SbVec3f tsep, tsep2;
	SbVec3f tsep2;

	if (chemColor->bondColorBinding.getValue() == 
		ChemColor::BOND_OVERALL) {

		// All bonds are the same color which is the first color in the
		// bond color table.
		const SbColor *bondColors = chemColor->bondColor.getValues(0);
		glColor3fv(bondColors[0].getValue());
		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		for (i = 0; i < listLen; i++) {
			listIndex = list[i];
			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();
			if (bondAttributes[listIndex].isSymmetric == FALSE) {
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
			}
			else {
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
				coords1 += tsep2;
				coords2 += tsep2;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
			}
		}
		glEnd();

		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			glEnable(GL_LINE_STIPPLE);
			glLineStipple(cdp->resonanceBondStippleFactor.getValue(),
				cdp->resonanceBondStipplePattern.getValue());
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}
		for (i = 0; i < listLen; i++) {
			listIndex = list[i];
			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			if (bondAttributes[listIndex].isSymmetric == FALSE) {
				coords1 = bondAttributes[listIndex].start;
				coords2 = bondAttributes[listIndex].end;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
			}
			else {
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
				coords1 -= tsep2;
				coords2 -= tsep2;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
			}
		}
		glEnd();
		if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);

	}
	else if (chemColor->bondColorBinding.getValue() == 
		ChemColor::BOND_PER_ATOM) {

		// The 2 vertices of the line comprising the bond are colored by
		// the atom color (indexed or not)
		const SbColor *atomColors = chemColor->atomColor.getValues(0);
		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_OVERALL) {
			glColor3fv(atomColors[0].getValue());

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();
				if (bondAttributes[listIndex].isSymmetric == FALSE) {
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
				else {
					tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
					coords1 += tsep2;
					coords2 += tsep2;
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
			}
			glEnd();

			if (renderAsPoints) {
				glBegin(GL_POINTS);
			}
			else {
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->resonanceBondStippleFactor.getValue(),
					cdp->resonanceBondStipplePattern.getValue());
				glLineWidth(cdp->bondWireframeLineWidth.getValue());
				glBegin(GL_LINES);
			}
			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				if (bondAttributes[listIndex].isSymmetric == FALSE) {
					coords1 = bondAttributes[listIndex].start;
					coords2 = bondAttributes[listIndex].end;
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
				else {
					tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
					coords1 -= tsep2;
					coords2 -= tsep2;
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
			}
			glEnd();
			if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);
		}
		else if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_PER_ATOM) {

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				color1 = atomColors[from];
				color2 = atomColors[to];
				if (bondAttributes[listIndex].isSymmetric == FALSE) {
					glColor3fv(color1.getValue());
					glVertex3fv(coords1.getValue());
					glColor3fv(color2.getValue());
					glVertex3fv(coords2.getValue());
				}
				else {
					tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
					coords1 += tsep2;
					coords2 += tsep2;

					glColor3fv(color1.getValue());
					glVertex3fv(coords1.getValue());
					glColor3fv(color2.getValue());
					glVertex3fv(coords2.getValue());
				}
			}
			glEnd();

			if (renderAsPoints) {
				glBegin(GL_POINTS);
			}
			else {
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->resonanceBondStippleFactor.getValue(),
					cdp->resonanceBondStipplePattern.getValue());
				glLineWidth(cdp->bondWireframeLineWidth.getValue());
				glBegin(GL_LINES);
			}
			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				color1 = atomColors[from];
				color2 = atomColors[to];

				if (bondAttributes[listIndex].isSymmetric == FALSE) {
					coords1 = bondAttributes[listIndex].start;
					coords2 = bondAttributes[listIndex].end;

					glColor3fv(color1.getValue());
					glVertex3fv(coords1.getValue());
					glColor3fv(color2.getValue());
					glVertex3fv(coords2.getValue());
				}
				else {
					tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
					coords1 -= tsep2;
					coords2 -= tsep2;

					glColor3fv(color1.getValue());
					glVertex3fv(coords1.getValue());
					glColor3fv(color2.getValue());
					glVertex3fv(coords2.getValue());
				}
			}
			glEnd();
			if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);
		}
		else if (chemColor->atomColorBinding.getValue() == 
			ChemColor::ATOM_PER_ATOM_INDEXED) {

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				color1 = atomColors[chemData->getAtomIndex(from)];
				color2 = atomColors[chemData->getAtomIndex(to)];

				if (bondAttributes[listIndex].isSymmetric == FALSE) {
					glColor3fv(color1.getValue());
					glVertex3fv(coords1.getValue());
					glColor3fv(color2.getValue());
					glVertex3fv(coords2.getValue());
				}
				else {
					tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
					coords1 += tsep2;
					coords2 += tsep2;

					glColor3fv(color1.getValue());
					glVertex3fv(coords1.getValue());
					glColor3fv(color2.getValue());
					glVertex3fv(coords2.getValue());
				}
			}
			glEnd();

			if (renderAsPoints) {
				glBegin(GL_POINTS);
			}
			else {
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->resonanceBondStippleFactor.getValue(),
					cdp->resonanceBondStipplePattern.getValue());
				glLineWidth(cdp->bondWireframeLineWidth.getValue());
				glBegin(GL_LINES);
			}
			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				color1 = atomColors[chemData->getAtomIndex(from)];
				color2 = atomColors[chemData->getAtomIndex(to)];

				if (bondAttributes[listIndex].isSymmetric == FALSE) {
					coords1 = bondAttributes[listIndex].start;
					coords2 = bondAttributes[listIndex].end;

					glColor3fv(color1.getValue());
					glVertex3fv(coords1.getValue());
					glColor3fv(color2.getValue());
					glVertex3fv(coords2.getValue());
				}
				else {
					tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
					coords1 -= tsep2;
					coords2 -= tsep2;

					glColor3fv(color1.getValue());
					glVertex3fv(coords1.getValue());
					glColor3fv(color2.getValue());
					glVertex3fv(coords2.getValue());
				}
			}
			glEnd();
			if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);
		}
	}
	else if (chemColor->bondColorBinding.getValue() == 
		ChemColor::BOND_PER_ATOM_HALF_BONDED) {

		// Each bond is drawn as 2 lines with each line being the color of
		// one of the atoms (indexed or not????)
		const SbColor *atomColors = chemColor->atomColor.getValues(0);

		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_OVERALL) {
			glColor3fv(atomColors[0].getValue());

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();
				if (bondAttributes[listIndex].isSymmetric == FALSE) {
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
				else {
					tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
					coords1 += tsep2;
					coords2 += tsep2;
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
			}
			glEnd();

			if (renderAsPoints) {
				glBegin(GL_POINTS);
			}
			else {
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->resonanceBondStippleFactor.getValue(),
					cdp->resonanceBondStipplePattern.getValue());
				glLineWidth(cdp->bondWireframeLineWidth.getValue());
				glBegin(GL_LINES);
			}
			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				if (bondAttributes[listIndex].isSymmetric == FALSE) {
					coords1 = bondAttributes[listIndex].start;
					coords2 = bondAttributes[listIndex].end;
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
				else {
					tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
					coords1 -= tsep2;
					coords2 -= tsep2;
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
			}
			glEnd();
			if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);
		}
		else if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_PER_ATOM) {
			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				color1 = atomColors[from];
				color2 = atomColors[to];

				if (color1 == color2) {
					glColor3fv(color1.getValue());
					if (bondAttributes[listIndex].isSymmetric == FALSE) {
						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
					}
					else {
						tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
						coords1 += tsep2;
						coords2 += tsep2;

						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
					}
				}
				// If the 2 ends are not the same color, find the midpoint
				// and draw half-bonds
				else {
					tcoord = (coords2 - coords1) * 0.5f + coords1;
					if (bondAttributes[listIndex].isSymmetric == FALSE) {
						glColor3fv(color1.getValue());
						glVertex3fv(coords1.getValue());
						glVertex3fv(tcoord.getValue());

						glColor3fv(color2.getValue());
						glVertex3fv(tcoord.getValue());
						glVertex3fv(coords2.getValue());
					}
					else {
						tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
						tcoord += tsep2;
						coords1 += tsep2;
						coords2 += tsep2;

						glColor3fv(color1.getValue());
						glVertex3fv(coords1.getValue());
						glVertex3fv(tcoord.getValue());

						glColor3fv(color2.getValue());
						glVertex3fv(tcoord.getValue());
						glVertex3fv(coords2.getValue());
					}
				}
			}
			glEnd();

			if (renderAsPoints) {
				glBegin(GL_POINTS);
			}
			else {
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->resonanceBondStippleFactor.getValue(),
					cdp->resonanceBondStipplePattern.getValue());
				glLineWidth(cdp->bondWireframeLineWidth.getValue());
				glBegin(GL_LINES);
			}
			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				color1 = atomColors[from];
				color2 = atomColors[to];

				if (color1 == color2) {
					glColor3fv(color1.getValue());
					if (bondAttributes[listIndex].isSymmetric == FALSE) {
						coords1 = bondAttributes[listIndex].start;
						coords2 = bondAttributes[listIndex].end;

						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
					}
					else {
						tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
						coords1 -= tsep2;
						coords2 -= tsep2;

						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
					}
				}
				// If the 2 ends are not the same color, find the midpoint
				// and draw half-bonds
				else {
					if (bondAttributes[listIndex].isSymmetric == FALSE) {
						tc1 = bondAttributes[listIndex].start;
						tc2 = bondAttributes[listIndex].end;
						tc0 = (tc2 - tc1) * 0.5f + tc1;

						glColor3fv(color1.getValue());
						glVertex3fv(tc1.getValue());
						glVertex3fv(tc0.getValue());

						glColor3fv(color2.getValue());
						glVertex3fv(tc0.getValue());
						glVertex3fv(tc2.getValue());
					}
					else {
						tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
						tcoord = (coords2 - coords1) * 0.5f + coords1;
						tcoord -= tsep2;
						coords1 -= tsep2;
						coords2 -= tsep2;

						glColor3fv(color2.getValue());
						glVertex3fv(coords1.getValue());
						glVertex3fv(tcoord.getValue());

						glColor3fv(color2.getValue());
						glVertex3fv(tcoord.getValue());
						glVertex3fv(coords2.getValue());
					}
				}
			}
			glEnd();
			if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);
		}
		else if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_PER_ATOM_INDEXED) {

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				color1 = atomColors[chemData->getAtomIndex(from)];
				color2 = atomColors[chemData->getAtomIndex(to)];

				if (color1 == color2) {
					glColor3fv(color1.getValue());
					if (bondAttributes[listIndex].isSymmetric == FALSE) {
						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
					}
					else {
						tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
						coords1 += tsep2;
						coords2 += tsep2;

						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
					}
				}
				// If the 2 ends are not the same color, find the midpoint
				// and draw half-bonds
				else {
					tcoord = (coords2 - coords1) * 0.5f + coords1;
					if (bondAttributes[listIndex].isSymmetric == FALSE) {
						glColor3fv(color1.getValue());
						glVertex3fv(coords1.getValue());
						glVertex3fv(tcoord.getValue());

						glColor3fv(color2.getValue());
						glVertex3fv(tcoord.getValue());
						glVertex3fv(coords2.getValue());
					}
					else {
						tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
						tcoord += tsep2;
						coords1 += tsep2;
						coords2 += tsep2;

						glColor3fv(color1.getValue());
						glVertex3fv(coords1.getValue());
						glVertex3fv(tcoord.getValue());

						glColor3fv(color2.getValue());
						glVertex3fv(tcoord.getValue());
						glVertex3fv(coords2.getValue());
					}
				}
			}
			glEnd();

			if (renderAsPoints) {
				glBegin(GL_POINTS);
			}
			else {
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->resonanceBondStippleFactor.getValue(),
					cdp->resonanceBondStipplePattern.getValue());
				glLineWidth(cdp->bondWireframeLineWidth.getValue());
				glBegin(GL_LINES);
			}
			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				color1 = atomColors[chemData->getAtomIndex(from)];
				color2 = atomColors[chemData->getAtomIndex(to)];

				if (color1 == color2) {
					glColor3fv(color1.getValue());
					if (bondAttributes[listIndex].isSymmetric == FALSE) {
						coords1 = bondAttributes[listIndex].start;
						coords2 = bondAttributes[listIndex].end;

						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
					}
					else {
						tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
						coords1 -= tsep2;
						coords2 -= tsep2;

						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
					}
				}
				// If the 2 ends are not the same color, find the midpoint
				// and draw half-bonds
				else {
					if (bondAttributes[listIndex].isSymmetric == FALSE) {
						tc1 = bondAttributes[listIndex].start;
						tc2 = bondAttributes[listIndex].end;
						tc0 = (tc2 - tc1) * 0.5f + tc1;

						glColor3fv(color1.getValue());
						glVertex3fv(tc1.getValue());
						glVertex3fv(tc0.getValue());

						glColor3fv(color2.getValue());
						glVertex3fv(tc0.getValue());
						glVertex3fv(tc2.getValue());
					}
					else {
						tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
						tcoord = (coords2 - coords1) * 0.5f + coords1;
						tcoord -= tsep2;
						coords1 -= tsep2;
						coords2 -= tsep2;

						glColor3fv(color2.getValue());
						glVertex3fv(coords1.getValue());
						glVertex3fv(tcoord.getValue());

						glColor3fv(color2.getValue());
						glVertex3fv(tcoord.getValue());
						glVertex3fv(coords2.getValue());
					}
				}
			}
			glEnd();
			if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);
		}
	}
	else if (chemColor->bondColorBinding.getValue() ==
		ChemColor::BOND_PER_BOND) {

		// Each bond has a color in the bond color table.  In the table,
		// there is a separate color for each bond.
		const SbColor *bondColors = chemColor->bondColor.getValues(0);
		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		for (i = 0; i < listLen; i++) {
			listIndex = list[i];
			theBond = bondAttributes[listIndex].index;
			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			glColor3fv(bondColors[theBond].getValue());
			if (bondAttributes[listIndex].isSymmetric == FALSE) {
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
			}
			else {
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
				coords1 += tsep2;
				coords2 += tsep2;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
			}
		}
		glEnd();

		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			glEnable(GL_LINE_STIPPLE);
			glLineStipple(cdp->resonanceBondStippleFactor.getValue(),
				cdp->resonanceBondStipplePattern.getValue());
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}
		for (i = 0; i < listLen; i++) {
			listIndex = list[i];
			theBond = bondAttributes[listIndex].index;
			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			glColor3fv(bondColors[theBond].getValue());

			if (bondAttributes[listIndex].isSymmetric == FALSE) {
				coords1 = bondAttributes[listIndex].start;
				coords2 = bondAttributes[listIndex].end;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
			}
			else {
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
				coords1 -= tsep2;
				coords2 -= tsep2;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
			}
		}
		glEnd();
		if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);
	}
	else if (chemColor->bondColorBinding.getValue() ==
		ChemColor::BOND_PER_BOND_INDEXED) {

		// Each bond has an index into the bond color table.
		const SbColor *bondColors = chemColor->bondColor.getValues(0);
		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		for (i = 0; i < listLen; i++) {
			listIndex = list[i];
			theBond = bondAttributes[listIndex].index;
			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			glColor3fv(bondColors[chemData->getBondIndex(theBond)].getValue());
			if (bondAttributes[listIndex].isSymmetric == FALSE) {
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
			}
			else {
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
				coords1 += tsep2;
				coords2 += tsep2;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
			}
		}
		glEnd();

		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			glEnable(GL_LINE_STIPPLE);
			glLineStipple(cdp->resonanceBondStippleFactor.getValue(),
				cdp->resonanceBondStipplePattern.getValue());
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}
		for (i = 0; i < listLen; i++) {
			listIndex = list[i];
			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			glColor3fv(bondColors[chemData->getBondIndex(theBond)].getValue());

			if (bondAttributes[listIndex].isSymmetric == FALSE) {
				coords1 = bondAttributes[listIndex].start;
				coords2 = bondAttributes[listIndex].end;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
			}
			else {
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
				coords1 -= tsep2;
				coords2 -= tsep2;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
			}
		}
		glEnd();
		if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);
	}
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//	   Performs highlighting of the molecule using lines.
//	   Invoked by the GLRender method.
//
// Use: static

void
singleBondsHighlighted
	(const SbIntList list, const SbBool renderAsPoints,
	const SbBool doHydrogens, const SbColor highlightColor,
	const int32_t highlightStyle,
	const ChemBaseData *chemData,
	const ChemDisplayParam *cdp, const ChemColor *chemColor,
	const ChemBondStuff *bondAttributes)
//
////////////////////////////////////////////////////////////////////////
{
	int32_t listLen;
	if ((listLen = list.getLength()) <= 0) return;

	int32_t i;
	int32_t listIndex;
	int32_t	from, to, theBond;
	SbVec3f coords1, coords2;
	//SbVec3f tcoord, tc0, tc1, tc2;
	SbVec3f tcoord;
	SbColor tmpColor, color1, color2;


	if (chemColor->bondColorBinding.getValue() == 
		ChemColor::BOND_OVERALL) {

		// All bonds are the same color which is the first color in the
		// bond color table.
		const SbColor *bondColors = chemColor->bondColor.getValues(0);

		// We should only be here if highlightStyle is either
		// HIGHLIGHT_DRAWSTYLE or HIGHLIGHT_EMISSIVE.  If
		// HIGHLIGHT_DIFFUSE, then highlightWireframeOverall should be called.

		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
			glColor3fv(bondColors[0].getValue());
		}
		else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			tmpColor = bondColors[0] + highlightColor;
			tmpColor.normalize();
			glColor3fv(tmpColor.getValue());
		}

		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->highlightStippleFactor.getValue(),
					cdp->highlightStipplePattern.getValue());
			}
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		for (i = 0; i < listLen; i++) {
			listIndex = list[i];
			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			 glVertex3fv(coords1.getValue());
			 glVertex3fv(coords2.getValue());
		}
		glEnd();

		if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
			!renderAsPoints) {
			glDisable(GL_LINE_STIPPLE);
		}

	}
	else if (chemColor->bondColorBinding.getValue() == 
		ChemColor::BOND_PER_ATOM) {

		// The 2 vertices of the line comprising the bond are colored by
		// the atom color (indexed or not)
		const SbColor *atomColors = chemColor->atomColor.getValues(0);

		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->highlightStippleFactor.getValue(),
					cdp->highlightStipplePattern.getValue());
			}
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_OVERALL) {
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				tmpColor = atomColors[0] + highlightColor;
				tmpColor.normalize();
				glColor3fv(tmpColor.getValue());
			}
			else {
				glColor3fv(atomColors[0].getValue());
			}

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				 glVertex3fv(coords1.getValue());
				 glVertex3fv(coords2.getValue());
			}
			glEnd();

			if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
				!renderAsPoints) {
				glDisable(GL_LINE_STIPPLE);
			}

		}
		else if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_PER_ATOM) {

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				if (highlightStyle ==
					ChemDisplayParam::HIGHLIGHT_EMISSIVE) {

					color1 = atomColors[from] + highlightColor;
					color1.normalize();

					color2 = atomColors[to] + highlightColor;
					color2.normalize();
				}
				else {
					color1 = atomColors[from];
					color2 = atomColors[to];
				}
				glColor3fv(color1.getValue());
				glVertex3fv(coords1.getValue());

				glColor3fv(color2.getValue());
				glVertex3fv(coords2.getValue());
			}
			glEnd();

			if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
				!renderAsPoints) {
				glDisable(GL_LINE_STIPPLE);
			}

		}
		else if (chemColor->atomColorBinding.getValue() == 
			ChemColor::ATOM_PER_ATOM_INDEXED) {

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				if (highlightStyle ==
					ChemDisplayParam::HIGHLIGHT_EMISSIVE) {

					color1 = atomColors[chemData->getAtomIndex(from)] +
						highlightColor;
					color1.normalize();

					color2 = atomColors[chemData->getAtomIndex(to)] +
						highlightColor;
					color2.normalize();
				}
				else {
					color1 = atomColors[chemData->getAtomIndex(from)];
					color2 = atomColors[chemData->getAtomIndex(to)];
				}

				glColor3fv(color1.getValue());
				glVertex3fv(coords1.getValue());

				glColor3fv(color2.getValue());
				glVertex3fv(coords2.getValue());
			}
			glEnd();

			if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
				!renderAsPoints) {
				glDisable(GL_LINE_STIPPLE);
			}

		}
	}
	else if (chemColor->bondColorBinding.getValue() == 
		ChemColor::BOND_PER_ATOM_HALF_BONDED) {

		// Each bond is drawn as 2 lines with each line being the color of
		// one of the atoms (indexed or not????)
		const SbColor *atomColors = chemColor->atomColor.getValues(0);

		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->highlightStippleFactor.getValue(),
					cdp->highlightStipplePattern.getValue());
			}
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_OVERALL) {
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				tmpColor = atomColors[0] + highlightColor;
				tmpColor.normalize();
				glColor3fv(tmpColor.getValue());
			}
			else {
				glColor3fv(atomColors[0].getValue());
			}

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				 glVertex3fv(coords1.getValue());
				 glVertex3fv(coords2.getValue());
			}
			glEnd();

			if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
				!renderAsPoints) {
				glDisable(GL_LINE_STIPPLE);
			}

		}
		else if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_PER_ATOM) {

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}

				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
					color1 = atomColors[from] + highlightColor;
					color1.normalize();

					color2 = atomColors[to] + highlightColor;
					color2.normalize();
				}
				else {
					color1 = atomColors[from];
					color2 = atomColors[to];
				}

				if (color1 == color2) {
					glColor3fv(color1.getValue());
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
				else {
					tcoord = (coords2 - coords1) * 0.5f + coords1;

					glColor3fv(color1.getValue());
					glVertex3fv(coords1.getValue());
					glVertex3fv(tcoord.getValue());

					glColor3fv(color2.getValue());
					glVertex3fv(tcoord.getValue());
					glVertex3fv(coords2.getValue());
				}
			}
			glEnd();

			if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
				!renderAsPoints) {
				glDisable(GL_LINE_STIPPLE);
			}

		}
		else if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_PER_ATOM_INDEXED) {

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
					color1 = atomColors[chemData->getAtomIndex(from)];
					color1 += highlightColor;
					color1.normalize();

					color2 = atomColors[chemData->getAtomIndex(to)];
					color2 += highlightColor;
					color2.normalize();
				}
				else {
					color1 = atomColors[chemData->getAtomIndex(from)];
					color2 = atomColors[chemData->getAtomIndex(to)];
				}

				if (color1 == color2) {
					glColor3fv(color1.getValue());
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
				else {
					tcoord = (coords2 - coords1) * 0.5f + coords1;

					glColor3fv(color1.getValue());
					glVertex3fv(coords1.getValue());
					glVertex3fv(tcoord.getValue());

					glColor3fv(color2.getValue());
					glVertex3fv(tcoord.getValue());
					glVertex3fv(coords2.getValue());
				}
			}
			glEnd();

			if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
				!renderAsPoints) {
				glDisable(GL_LINE_STIPPLE);
			}

		}
	}
	else if (chemColor->bondColorBinding.getValue() ==
		ChemColor::BOND_PER_BOND) {

		// Each bond has a color in the bond color table.  In the table,
		// there is a separate color for each bond.
		const SbColor *bondColors = chemColor->bondColor.getValues(0);

		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->highlightStippleFactor.getValue(),
					cdp->highlightStipplePattern.getValue());
			}
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		for (i = 0; i < listLen; i++) {
			listIndex = list[i];
			theBond = bondAttributes[listIndex].index;
			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}

			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				tmpColor = bondColors[theBond] + highlightColor;
				tmpColor.normalize();
				glColor3fv(tmpColor.getValue());
			}
			else {
				glColor3fv(bondColors[theBond].getValue());
			}

			 glVertex3fv(coords1.getValue());
			 glVertex3fv(coords2.getValue());
		}
		glEnd();

		if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
			!renderAsPoints) {
			glDisable(GL_LINE_STIPPLE);
		}

	}
	else if (chemColor->bondColorBinding.getValue() ==
		ChemColor::BOND_PER_BOND_INDEXED) {

		// Each bond has an index into the bond color table.
		const SbColor *bondColors = chemColor->bondColor.getValues(0);

		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->highlightStippleFactor.getValue(),
					cdp->highlightStipplePattern.getValue());
			}
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		for (i = 0; i < listLen; i++) {
			listIndex = list[i];
			theBond = bondAttributes[listIndex].index;
			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				tmpColor = bondColors[chemData->getBondIndex(theBond)] +
					highlightColor;
				tmpColor.normalize();
				glColor3fv(tmpColor.getValue());
			}
			else {
				glColor3fv(bondColors[chemData->getBondIndex(theBond)].getValue());
			}

			 glVertex3fv(coords1.getValue());
			 glVertex3fv(coords2.getValue());
		}
		glEnd();

		if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
			!renderAsPoints) {
			glDisable(GL_LINE_STIPPLE);
		}

	}
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	   Performs highlighting of the molecule using lines.
//	   Invoked by the GLRender method.
//
// Use: static

void
doubleBondsHighlighted
	(const SbIntList list, const SbBool renderAsPoints,
	const SbBool doHydrogens, const SbColor highlightColor,
	const int32_t highlightStyle,
	const ChemBaseData *chemData,
	const ChemDisplayParam *cdp, const ChemColor *chemColor,
	const ChemBondStuff *bondAttributes)
//
////////////////////////////////////////////////////////////////////////
{
	int32_t listLen;
	if ((listLen = list.getLength()) <= 0) return;

	int32_t i;
	int32_t listIndex;
	int32_t	from, to, theBond;
	SbVec3f coords1, coords2;
	SbVec3f tcoord, tc0, tc1, tc2;
	SbColor tmpColor, color1, color2;

	float  multiSep = cdp->multipleBondSeparation.getValue();
	float  multiSep2 = multiSep * 0.5;
	SbVec3f tsep, tsep2;

	if (chemColor->bondColorBinding.getValue() == 
		ChemColor::BOND_OVERALL) {

		// All bonds are the same color which is the first color in the
		// bond color table.
		const SbColor *bondColors = chemColor->bondColor.getValues(0);

		// We should only be here if highlightStyle is either
		// HIGHLIGHT_DRAWSTYLE or HIGHLIGHT_EMISSIVE.  If
		// HIGHLIGHT_DIFFUSE, then highlightWireframeOverall should be called.

		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
			glColor3fv(bondColors[0].getValue());
		}
		else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			tmpColor = bondColors[0] + highlightColor;
			tmpColor.normalize();
			glColor3fv(tmpColor.getValue());
		}

		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->highlightStippleFactor.getValue(),
					cdp->highlightStipplePattern.getValue());
			}
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		for (i = 0; i < listLen; i++) {
			listIndex = list[i];
			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			tsep = bondAttributes[listIndex].perpNormal * multiSep;
			tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

			if (bondAttributes[listIndex].isSymmetric == FALSE) {
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
				coords1 = bondAttributes[listIndex].start;
				coords2 = bondAttributes[listIndex].end;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
			}
			else {
				coords1 += tsep2;
				coords2 += tsep2;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());

				coords1 -= tsep;
				coords2 -= tsep;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
			}
		}
		glEnd();

		if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
			!renderAsPoints) {
			glDisable(GL_LINE_STIPPLE);
		}

	}
	else if (chemColor->bondColorBinding.getValue() == 
		ChemColor::BOND_PER_ATOM) {

		// The 2 vertices of the line comprising the bond are colored by
		// the atom color (indexed or not)
		const SbColor *atomColors = chemColor->atomColor.getValues(0);

		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->highlightStippleFactor.getValue(),
					cdp->highlightStipplePattern.getValue());
			}
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_OVERALL) {
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				tmpColor = atomColors[0] + highlightColor;
				tmpColor.normalize();
				glColor3fv(tmpColor.getValue());
			}
			else {
				glColor3fv(atomColors[0].getValue());
			}

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				tsep = bondAttributes[listIndex].perpNormal * multiSep;
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

				if (bondAttributes[listIndex].isSymmetric == FALSE) {
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
					coords1 = bondAttributes[listIndex].start;
					coords2 = bondAttributes[listIndex].end;
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
				else {
					coords1 += tsep2;
					coords2 += tsep2;
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());

					coords1 -= tsep;
					coords2 -= tsep;
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
			}
			glEnd();

			if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
				!renderAsPoints) {
				glDisable(GL_LINE_STIPPLE);
			}

		}
		else if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_PER_ATOM) {

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				if (highlightStyle ==
					ChemDisplayParam::HIGHLIGHT_EMISSIVE) {

					color1 = atomColors[from] + highlightColor;
					color1.normalize();

					color2 = atomColors[to] + highlightColor;
					color2.normalize();
				}
				else {
					color1 = atomColors[from];
					color2 = atomColors[to];
				}
				tsep = bondAttributes[listIndex].perpNormal * multiSep;
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

				if (bondAttributes[listIndex].isSymmetric == FALSE) {
					glColor3fv(color1.getValue());
					glVertex3fv(coords1.getValue());
					glColor3fv(color2.getValue());
					glVertex3fv(coords2.getValue());

					coords1 = bondAttributes[listIndex].start;
					coords2 = bondAttributes[listIndex].end;

					glColor3fv(color1.getValue());
					glVertex3fv(coords1.getValue());
					glColor3fv(color2.getValue());
					glVertex3fv(coords2.getValue());
				}
				else {
					coords1 += tsep2;
					coords2 += tsep2;

					glColor3fv(color1.getValue());
					glVertex3fv(coords1.getValue());
					glColor3fv(color2.getValue());
					glVertex3fv(coords2.getValue());

					coords1 -= tsep;
					coords2 -= tsep;

					glColor3fv(color1.getValue());
					glVertex3fv(coords1.getValue());
					glColor3fv(color2.getValue());
					glVertex3fv(coords2.getValue());
				}
			}
			glEnd();

			if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
				!renderAsPoints) {
				glDisable(GL_LINE_STIPPLE);
			}

		}
		else if (chemColor->atomColorBinding.getValue() == 
			ChemColor::ATOM_PER_ATOM_INDEXED) {

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				if (highlightStyle ==
					ChemDisplayParam::HIGHLIGHT_EMISSIVE) {

					color1 = atomColors[chemData->getAtomIndex(from)] +
						highlightColor;
					color1.normalize();

					color2 = atomColors[chemData->getAtomIndex(to)] +
						highlightColor;
					color2.normalize();
				}
				else {
					color1 = atomColors[chemData->getAtomIndex(from)];
					color2 = atomColors[chemData->getAtomIndex(to)];
				}

				tsep = bondAttributes[listIndex].perpNormal * multiSep;
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

				if (bondAttributes[listIndex].isSymmetric == FALSE) {
					glColor3fv(color1.getValue());
					glVertex3fv(coords1.getValue());
					glColor3fv(color2.getValue());
					glVertex3fv(coords2.getValue());

					coords1 = bondAttributes[listIndex].start;
					coords2 = bondAttributes[listIndex].end;

					glColor3fv(color1.getValue());
					glVertex3fv(coords1.getValue());
					glColor3fv(color2.getValue());
					glVertex3fv(coords2.getValue());
				}
				else {
					coords1 += tsep2;
					coords2 += tsep2;

					glColor3fv(color1.getValue());
					glVertex3fv(coords1.getValue());
					glColor3fv(color2.getValue());
					glVertex3fv(coords2.getValue());

					coords1 -= tsep;
					coords2 -= tsep;

					glColor3fv(color1.getValue());
					glVertex3fv(coords1.getValue());
					glColor3fv(color2.getValue());
					glVertex3fv(coords2.getValue());
				}
			}
			glEnd();

			if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
				!renderAsPoints) {
				glDisable(GL_LINE_STIPPLE);
			}

		}
	}
	else if (chemColor->bondColorBinding.getValue() == 
		ChemColor::BOND_PER_ATOM_HALF_BONDED) {

		// Each bond is drawn as 2 lines with each line being the color of
		// one of the atoms (indexed or not????)
		const SbColor *atomColors = chemColor->atomColor.getValues(0);

		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->highlightStippleFactor.getValue(),
					cdp->highlightStipplePattern.getValue());
			}
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_OVERALL) {
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				tmpColor = atomColors[0] + highlightColor;
				tmpColor.normalize();
				glColor3fv(tmpColor.getValue());
			}
			else {
				glColor3fv(atomColors[0].getValue());
			}

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				tsep = bondAttributes[listIndex].perpNormal * multiSep;
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

				if (bondAttributes[listIndex].isSymmetric == FALSE) {
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
					coords1 = bondAttributes[listIndex].start;
					coords2 = bondAttributes[listIndex].end;
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
				else {
					coords1 += tsep2;
					coords2 += tsep2;
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());

					coords1 -= tsep;
					coords2 -= tsep;
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
			}
			glEnd();

			if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
				!renderAsPoints) {
				glDisable(GL_LINE_STIPPLE);
			}

		}
		else if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_PER_ATOM) {

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}

				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
					color1 = atomColors[from] + highlightColor;
					color1.normalize();

					color2 = atomColors[to] + highlightColor;
					color2.normalize();
				}
				else {
					color1 = atomColors[from];
					color2 = atomColors[to];
				}

				tsep = bondAttributes[listIndex].perpNormal * multiSep;
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

				if (color1 == color2) {
					glColor3fv(color1.getValue());
					if (bondAttributes[listIndex].isSymmetric == FALSE) {
						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());

						coords1 = bondAttributes[listIndex].start;
						coords2 = bondAttributes[listIndex].end;

						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
					}
					else {
						coords1 += tsep2;
						coords2 += tsep2;

						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());

						coords1 -= tsep;
						coords2 -= tsep;

						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
					}
				}
				// If the 2 ends are not the same color, find the midpoint
				// and draw half-bonds
				else {
					tcoord = (coords2 - coords1) * 0.5f + coords1;
					if (bondAttributes[listIndex].isSymmetric == FALSE) {
						tc0 = tcoord + tsep;
						tc1 = bondAttributes[listIndex].start;
						tc2 = bondAttributes[listIndex].end;

						glColor3fv(color1.getValue());
						glVertex3fv(coords1.getValue());
						glVertex3fv(tcoord.getValue());
						glVertex3fv(tc1.getValue());
						glVertex3fv(tc0.getValue());

						glColor3fv(color2.getValue());
						glVertex3fv(tcoord.getValue());
						glVertex3fv(coords2.getValue());
						glVertex3fv(tc0.getValue());
						glVertex3fv(tc2.getValue());
					}
					else {
						tcoord += tsep2;
						coords1 += tsep2;
						coords2 += tsep2;
						tc0 = tcoord - tsep;
						tc1 = coords1 - tsep;
						tc2 = coords2 - tsep;

						glColor3fv(color1.getValue());
						glVertex3fv(coords1.getValue());
						glVertex3fv(tcoord.getValue());
						glVertex3fv(tc1.getValue());
						glVertex3fv(tc0.getValue());

						glColor3fv(color2.getValue());
						glVertex3fv(tcoord.getValue());
						glVertex3fv(coords2.getValue());
						glVertex3fv(tc0.getValue());
						glVertex3fv(tc2.getValue());
					}
				}
			}
			glEnd();

			if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
				!renderAsPoints) {
				glDisable(GL_LINE_STIPPLE);
			}

		}
		else if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_PER_ATOM_INDEXED) {

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
					color1 = atomColors[chemData->getAtomIndex(from)];
					color1 += highlightColor;
					color1.normalize();

					color2 = atomColors[chemData->getAtomIndex(to)];
					color2 += highlightColor;
					color2.normalize();
				}
				else {
					color1 = atomColors[chemData->getAtomIndex(from)];
					color2 = atomColors[chemData->getAtomIndex(to)];
				}

				tsep = bondAttributes[listIndex].perpNormal * multiSep;
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

				if (color1 == color2) {
					glColor3fv(color1.getValue());
					if (bondAttributes[listIndex].isSymmetric == FALSE) {
						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());

						coords1 = bondAttributes[listIndex].start;
						coords2 = bondAttributes[listIndex].end;

						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
					}
					else {
						coords1 += tsep2;
						coords2 += tsep2;

						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());

						coords1 -= tsep;
						coords2 -= tsep;

						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
					}
				}
				// If the 2 ends are not the same color, find the midpoint
				// and draw half-bonds
				else {
					tcoord = (coords2 - coords1) * 0.5f + coords1;
					if (bondAttributes[listIndex].isSymmetric == FALSE) {
						tc0 = tcoord + tsep;
						tc1 = bondAttributes[listIndex].start;
						tc2 = bondAttributes[listIndex].end;

						glColor3fv(color1.getValue());
						glVertex3fv(coords1.getValue());
						glVertex3fv(tcoord.getValue());
						glVertex3fv(tc1.getValue());
						glVertex3fv(tc0.getValue());

						glColor3fv(color2.getValue());
						glVertex3fv(tcoord.getValue());
						glVertex3fv(coords2.getValue());
						glVertex3fv(tc0.getValue());
						glVertex3fv(tc2.getValue());
					}
					else {
						tcoord += tsep2;
						coords1 += tsep2;
						coords2 += tsep2;
						tc0 = tcoord - tsep;
						tc1 = coords1 - tsep;
						tc2 = coords2 - tsep;

						glColor3fv(color1.getValue());
						glVertex3fv(coords1.getValue());
						glVertex3fv(tcoord.getValue());
						glVertex3fv(tc1.getValue());
						glVertex3fv(tc0.getValue());

						glColor3fv(color2.getValue());
						glVertex3fv(tcoord.getValue());
						glVertex3fv(coords2.getValue());
						glVertex3fv(tc0.getValue());
						glVertex3fv(tc2.getValue());
					}
				}
			}
			glEnd();

			if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
				!renderAsPoints) {
				glDisable(GL_LINE_STIPPLE);
			}

		}
	}
	else if (chemColor->bondColorBinding.getValue() ==
		ChemColor::BOND_PER_BOND) {

		// Each bond has a color in the bond color table.  In the table,
		// there is a separate color for each bond.
		const SbColor *bondColors = chemColor->bondColor.getValues(0);

		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->highlightStippleFactor.getValue(),
					cdp->highlightStipplePattern.getValue());
			}
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		for (i = 0; i < listLen; i++) {
			listIndex = list[i];
			theBond = bondAttributes[listIndex].index;
			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}

			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				tmpColor = bondColors[theBond] + highlightColor;
				tmpColor.normalize();
				glColor3fv(tmpColor.getValue());
			}
			else {
				glColor3fv(bondColors[theBond].getValue());
			}

			tsep = bondAttributes[listIndex].perpNormal * multiSep;
			tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

			if (bondAttributes[listIndex].isSymmetric == FALSE) {
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
				coords1 = bondAttributes[listIndex].start;
				coords2 = bondAttributes[listIndex].end;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
			}
			else {
				coords1 += tsep2;
				coords2 += tsep2;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());

				coords1 -= tsep;
				coords2 -= tsep;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
			}
		}
		glEnd();

		if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
			!renderAsPoints) {
			glDisable(GL_LINE_STIPPLE);
		}

	}
	else if (chemColor->bondColorBinding.getValue() ==
		ChemColor::BOND_PER_BOND_INDEXED) {

		// Each bond has an index into the bond color table.
		const SbColor *bondColors = chemColor->bondColor.getValues(0);

		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->highlightStippleFactor.getValue(),
					cdp->highlightStipplePattern.getValue());
			}
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		for (i = 0; i < listLen; i++) {
			listIndex = list[i];
			theBond = bondAttributes[listIndex].index;
			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				tmpColor = bondColors[chemData->getBondIndex(theBond)] +
					highlightColor;
				tmpColor.normalize();
				glColor3fv(tmpColor.getValue());
			}
			else {
				glColor3fv(bondColors[chemData->getBondIndex(theBond)].getValue());
			}

			tsep = bondAttributes[listIndex].perpNormal * multiSep;
			tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

			if (bondAttributes[listIndex].isSymmetric == FALSE) {
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
				coords1 = bondAttributes[listIndex].start;
				coords2 = bondAttributes[listIndex].end;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
			}
			else {
				coords1 += tsep2;
				coords2 += tsep2;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());

				coords1 -= tsep;
				coords2 -= tsep;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
			}
		}
		glEnd();

		if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
			!renderAsPoints) {
			glDisable(GL_LINE_STIPPLE);
		}

	}
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	   Performs highlighting of the molecule using lines.
//	   Invoked by the GLRender method.
//
// Use: static

void
tripleBondsHighlighted
	(const SbIntList list, const SbBool renderAsPoints,
	const SbBool doHydrogens, const SbColor highlightColor,
	const int32_t highlightStyle,
	const ChemBaseData *chemData,
	const ChemDisplayParam *cdp, const ChemColor *chemColor,
	const ChemBondStuff *bondAttributes)
//
////////////////////////////////////////////////////////////////////////
{
	int32_t listLen;
	if ((listLen = list.getLength()) <= 0) return;

	int32_t i;
	int32_t listIndex;
	int32_t	from, to, theBond;
	SbVec3f coords1, coords2;
	SbVec3f tcoord, tc0, tc1, tc2;
	SbColor tmpColor, color1, color2;

	float  multiSep = cdp->multipleBondSeparation.getValue();
	float  multiSep2 = multiSep * 0.5;
	SbVec3f tsep, tsep2;

	if (chemColor->bondColorBinding.getValue() == 
		ChemColor::BOND_OVERALL) {

		// All bonds are the same color which is the first color in the
		// bond color table.
		const SbColor *bondColors = chemColor->bondColor.getValues(0);

		// We should only be here if highlightStyle is either
		// HIGHLIGHT_DRAWSTYLE or HIGHLIGHT_EMISSIVE.  If
		// HIGHLIGHT_DIFFUSE, then highlightWireframeOverall should be called.

		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
			glColor3fv(bondColors[0].getValue());
		}
		else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			tmpColor = bondColors[0] + highlightColor;
			tmpColor.normalize();
			glColor3fv(tmpColor.getValue());
		}

		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->highlightStippleFactor.getValue(),
					cdp->highlightStipplePattern.getValue());
			}
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		for (i = 0; i < listLen; i++) {
			listIndex = list[i];
			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			tsep = bondAttributes[listIndex].perpNormal * multiSep;
			tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

			tc1 = coords1 + tsep;
			tc2 = coords2 + tsep;
			glVertex3fv(tc1.getValue());
			glVertex3fv(tc2.getValue());

			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());

			tc1 = coords1 - tsep;
			tc2 = coords2 - tsep;
			glVertex3fv(tc1.getValue());
			glVertex3fv(tc2.getValue());
		}
		glEnd();

		if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
			!renderAsPoints) {
			glDisable(GL_LINE_STIPPLE);
		}

	}
	else if (chemColor->bondColorBinding.getValue() == 
		ChemColor::BOND_PER_ATOM) {

		// The 2 vertices of the line comprising the bond are colored by
		// the atom color (indexed or not)
		const SbColor *atomColors = chemColor->atomColor.getValues(0);

		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->highlightStippleFactor.getValue(),
					cdp->highlightStipplePattern.getValue());
			}
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_OVERALL) {
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				tmpColor = atomColors[0] + highlightColor;
				tmpColor.normalize();
				glColor3fv(tmpColor.getValue());
			}
			else {
				glColor3fv(atomColors[0].getValue());
			}

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				tsep = bondAttributes[listIndex].perpNormal * multiSep;
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

				tc1 = coords1 + tsep;
				tc2 = coords2 + tsep;
				glVertex3fv(tc1.getValue());
				glVertex3fv(tc2.getValue());

				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());

				tc1 = coords1 - tsep;
				tc2 = coords2 - tsep;
				glVertex3fv(tc1.getValue());
				glVertex3fv(tc2.getValue());
			}
			glEnd();

			if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
				!renderAsPoints) {
				glDisable(GL_LINE_STIPPLE);
			}

		}
		else if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_PER_ATOM) {

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				if (highlightStyle ==
					ChemDisplayParam::HIGHLIGHT_EMISSIVE) {

					color1 = atomColors[from] + highlightColor;
					color1.normalize();

					color2 = atomColors[to] + highlightColor;
					color2.normalize();
				}
				else {
					color1 = atomColors[from];
					color2 = atomColors[to];
				}
				tsep = bondAttributes[listIndex].perpNormal * multiSep;
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

				tc1 = coords1 + tsep;
				tc2 = coords2 + tsep;
				glColor3fv(color1.getValue());
				glVertex3fv(tc1.getValue());
				glColor3fv(color2.getValue());
				glVertex3fv(tc2.getValue());

				glColor3fv(color1.getValue());
				glVertex3fv(coords1.getValue());
				glColor3fv(color2.getValue());
				glVertex3fv(coords2.getValue());

				tc1 = coords1 - tsep;
				tc2 = coords2 - tsep;
				glColor3fv(color1.getValue());
				glVertex3fv(tc1.getValue());
				glColor3fv(color2.getValue());
				glVertex3fv(tc2.getValue());
			}
			glEnd();

			if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
				!renderAsPoints) {
				glDisable(GL_LINE_STIPPLE);
			}

		}
		else if (chemColor->atomColorBinding.getValue() == 
			ChemColor::ATOM_PER_ATOM_INDEXED) {

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				if (highlightStyle ==
					ChemDisplayParam::HIGHLIGHT_EMISSIVE) {

					color1 = atomColors[chemData->getAtomIndex(from)] +
						highlightColor;
					color1.normalize();

					color2 = atomColors[chemData->getAtomIndex(to)] +
						highlightColor;
					color2.normalize();
				}
				else {
					color1 = atomColors[chemData->getAtomIndex(from)];
					color2 = atomColors[chemData->getAtomIndex(to)];
				}

				tsep = bondAttributes[listIndex].perpNormal * multiSep;
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

				tc1 = coords1 + tsep;
				tc2 = coords2 + tsep;
				glColor3fv(color1.getValue());
				glVertex3fv(tc1.getValue());
				glColor3fv(color2.getValue());
				glVertex3fv(tc2.getValue());

				glColor3fv(color1.getValue());
				glVertex3fv(coords1.getValue());
				glColor3fv(color2.getValue());
				glVertex3fv(coords2.getValue());

				tc1 = coords1 - tsep;
				tc2 = coords2 - tsep;
				glColor3fv(color1.getValue());
				glVertex3fv(tc1.getValue());
				glColor3fv(color2.getValue());
				glVertex3fv(tc2.getValue());
			}
			glEnd();

			if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
				!renderAsPoints) {
				glDisable(GL_LINE_STIPPLE);
			}

		}
	}
	else if (chemColor->bondColorBinding.getValue() == 
		ChemColor::BOND_PER_ATOM_HALF_BONDED) {

		// Each bond is drawn as 2 lines with each line being the color of
		// one of the atoms (indexed or not????)
		const SbColor *atomColors = chemColor->atomColor.getValues(0);

		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->highlightStippleFactor.getValue(),
					cdp->highlightStipplePattern.getValue());
			}
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_OVERALL) {
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				tmpColor = atomColors[0] + highlightColor;
				tmpColor.normalize();
				glColor3fv(tmpColor.getValue());
			}
			else {
				glColor3fv(atomColors[0].getValue());
			}

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				tsep = bondAttributes[listIndex].perpNormal * multiSep;
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

				tc1 = coords1 + tsep;
				tc2 = coords2 + tsep;
				glVertex3fv(tc1.getValue());
				glVertex3fv(tc2.getValue());

				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());

				tc1 = coords1 - tsep;
				tc2 = coords2 - tsep;
				glVertex3fv(tc1.getValue());
				glVertex3fv(tc2.getValue());
			}
			glEnd();

			if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
				!renderAsPoints) {
				glDisable(GL_LINE_STIPPLE);
			}

		}
		else if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_PER_ATOM) {

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}

				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
					color1 = atomColors[from] + highlightColor;
					color1.normalize();

					color2 = atomColors[to] + highlightColor;
					color2.normalize();
				}
				else {
					color1 = atomColors[from];
					color2 = atomColors[to];
				}

				tsep = bondAttributes[listIndex].perpNormal * multiSep;
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

				if (color1 == color2) {
					glColor3fv(color1.getValue());
					SbVec3f tcoord1, tcoord2;

					tcoord1 = coords1 + tsep;
					tcoord2 = coords2 + tsep;
					glVertex3fv(tcoord1.getValue());
					glVertex3fv(tcoord2.getValue());

					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());

					tcoord1 = coords1 - tsep;
					tcoord2 = coords2 - tsep;
					glVertex3fv(tcoord1.getValue());
					glVertex3fv(tcoord2.getValue());
				}
				// If the 2 ends are not the same color, find the midpoint
				// and draw half-bonds
				else {
					tcoord = (coords2 - coords1) * 0.5f + coords1;
					tc0 = tcoord + tsep;
					tc1 = coords1 + tsep;

					// Set the color and draw the first 3 half bonds
					glColor3fv(color1.getValue());
					glVertex3fv(tc1.getValue());
					glVertex3fv(tc0.getValue());
					glVertex3fv(coords1.getValue());
					glVertex3fv(tcoord.getValue());
					tc0 = tcoord - tsep;
					tc1 = coords1 - tsep;
					glVertex3fv(tc1.getValue());
					glVertex3fv(tc0.getValue());

					tc2 = coords2 - tsep;
					// Set the color and draw the second 3 half bonds
					glColor3fv(color2.getValue());
					glVertex3fv(tc0.getValue());
					glVertex3fv(tc2.getValue());
					glVertex3fv(tcoord.getValue());
					glVertex3fv(coords2.getValue());
					tc0 = tcoord + tsep;
					tc2 = coords2 + tsep;
					glVertex3fv(tc0.getValue());
					glVertex3fv(tc2.getValue());
				}
			}
			glEnd();

			if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
				!renderAsPoints) {
				glDisable(GL_LINE_STIPPLE);
			}

		}
		else if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_PER_ATOM_INDEXED) {

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
					color1 = atomColors[chemData->getAtomIndex(from)];
					color1 += highlightColor;
					color1.normalize();

					color2 = atomColors[chemData->getAtomIndex(to)];
					color2 += highlightColor;
					color2.normalize();
				}
				else {
					color1 = atomColors[chemData->getAtomIndex(from)];
					color2 = atomColors[chemData->getAtomIndex(to)];
				}

				tsep = bondAttributes[listIndex].perpNormal * multiSep;
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

				if (color1 == color2) {
					glColor3fv(color1.getValue());
					SbVec3f tcoord1, tcoord2;

					tcoord1 = coords1 + tsep;
					tcoord2 = coords2 + tsep;
					glVertex3fv(tcoord1.getValue());
					glVertex3fv(tcoord2.getValue());

					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());

					tcoord1 = coords1 - tsep;
					tcoord2 = coords2 - tsep;
					glVertex3fv(tcoord1.getValue());
					glVertex3fv(tcoord2.getValue());
				}
				// If the 2 ends are not the same color, find the midpoint
				// and draw half-bonds
				else {
					tcoord = (coords2 - coords1) * 0.5f + coords1;
					tc0 = tcoord + tsep;
					tc1 = coords1 + tsep;

					// Set the color and draw the first 3 half bonds
					glColor3fv(color1.getValue());
					glVertex3fv(tc1.getValue());
					glVertex3fv(tc0.getValue());
					glVertex3fv(coords1.getValue());
					glVertex3fv(tcoord.getValue());
					tc0 = tcoord - tsep;
					tc1 = coords1 - tsep;
					glVertex3fv(tc1.getValue());
					glVertex3fv(tc0.getValue());

					tc2 = coords2 - tsep;
					// Set the color and draw the second 3 half bonds
					glColor3fv(color2.getValue());
					glVertex3fv(tc0.getValue());
					glVertex3fv(tc2.getValue());
					glVertex3fv(tcoord.getValue());
					glVertex3fv(coords2.getValue());
					tc0 = tcoord + tsep;
					tc2 = coords2 + tsep;
					glVertex3fv(tc0.getValue());
					glVertex3fv(tc2.getValue());
				}
			}
			glEnd();

			if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
				!renderAsPoints) {
				glDisable(GL_LINE_STIPPLE);
			}

		}
	}
	else if (chemColor->bondColorBinding.getValue() ==
		ChemColor::BOND_PER_BOND) {

		// Each bond has a color in the bond color table.  In the table,
		// there is a separate color for each bond.
		const SbColor *bondColors = chemColor->bondColor.getValues(0);

		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->highlightStippleFactor.getValue(),
					cdp->highlightStipplePattern.getValue());
			}
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		for (i = 0; i < listLen; i++) {
			listIndex = list[i];
			theBond = bondAttributes[listIndex].index;
			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}

			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				tmpColor = bondColors[theBond] + highlightColor;
				tmpColor.normalize();
				glColor3fv(tmpColor.getValue());
			}
			else {
				glColor3fv(bondColors[theBond].getValue());
			}

			tsep = bondAttributes[listIndex].perpNormal * multiSep;
			tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

			tc1 = coords1 + tsep;
			tc2 = coords2 + tsep;
			glVertex3fv(tc1.getValue());
			glVertex3fv(tc2.getValue());

			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());

			tc1 = coords1 - tsep;
			tc2 = coords2 - tsep;
			glVertex3fv(tc1.getValue());
			glVertex3fv(tc2.getValue());
		}
		glEnd();

		if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
			!renderAsPoints) {
			glDisable(GL_LINE_STIPPLE);
		}

	}
	else if (chemColor->bondColorBinding.getValue() ==
		ChemColor::BOND_PER_BOND_INDEXED) {

		// Each bond has an index into the bond color table.
		const SbColor *bondColors = chemColor->bondColor.getValues(0);

		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->highlightStippleFactor.getValue(),
					cdp->highlightStipplePattern.getValue());
			}
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		for (i = 0; i < listLen; i++) {
			listIndex = list[i];
			theBond = bondAttributes[listIndex].index;
			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				tmpColor = bondColors[chemData->getBondIndex(theBond)] +
					highlightColor;
				tmpColor.normalize();
				glColor3fv(tmpColor.getValue());
			}
			else {
				glColor3fv(bondColors[chemData->getBondIndex(theBond)].getValue());
			}

			tsep = bondAttributes[listIndex].perpNormal * multiSep;
			tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

			tc1 = coords1 + tsep;
			tc2 = coords2 + tsep;
			glVertex3fv(tc1.getValue());
			glVertex3fv(tc2.getValue());

			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());

			tc1 = coords1 - tsep;
			tc2 = coords2 - tsep;
			glVertex3fv(tc1.getValue());
			glVertex3fv(tc2.getValue());
		}
		glEnd();

		if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
			!renderAsPoints) {
			glDisable(GL_LINE_STIPPLE);
		}

	}
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	   Performs highlighting of the molecule using lines.
//	   Invoked by the GLRender method.
//
// Use: static

void
quadrupleBondsHighlighted
	(const SbIntList list, const SbBool renderAsPoints,
	const SbBool doHydrogens, const SbColor highlightColor,
	const int32_t highlightStyle,
	const ChemBaseData *chemData,
	const ChemDisplayParam *cdp, const ChemColor *chemColor,
	const ChemBondStuff *bondAttributes)
//
////////////////////////////////////////////////////////////////////////
{
	int32_t listLen;
	if ((listLen = list.getLength()) <= 0) return;

	int32_t i;
	int32_t listIndex;
	int32_t	from, to, theBond;
	SbVec3f coords1, coords2;
	SbVec3f tcoord, tc0, tc1, tc2;
	SbColor tmpColor, color1, color2;

	float  multiSep = cdp->multipleBondSeparation.getValue();
	float  multiSep2 = multiSep * 0.5;
	SbVec3f tsep, tsep2;

	if (chemColor->bondColorBinding.getValue() == 
		ChemColor::BOND_OVERALL) {

		// All bonds are the same color which is the first color in the
		// bond color table.
		const SbColor *bondColors = chemColor->bondColor.getValues(0);

		// We should only be here if highlightStyle is either
		// HIGHLIGHT_DRAWSTYLE or HIGHLIGHT_EMISSIVE.  If
		// HIGHLIGHT_DIFFUSE, then highlightWireframeOverall should be called.

		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
			glColor3fv(bondColors[0].getValue());
		}
		else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			tmpColor = bondColors[0] + highlightColor;
			tmpColor.normalize();
			glColor3fv(tmpColor.getValue());
		}

		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->highlightStippleFactor.getValue(),
					cdp->highlightStipplePattern.getValue());
			}
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		for (i = 0; i < listLen; i++) {
			listIndex = list[i];
			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			tsep = bondAttributes[listIndex].perpNormal * multiSep;
			tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

			coords1 += tsep2;
			coords2 += tsep2;
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());

			coords1 += tsep;
			coords2 += tsep;
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());

			coords1 -= 2.0f*tsep;
			coords2 -= 2.0f*tsep;
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());

			coords1 -= tsep;
			coords2 -= tsep;
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());
		}
		glEnd();

		if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
			!renderAsPoints) {
			glDisable(GL_LINE_STIPPLE);
		}

	}
	else if (chemColor->bondColorBinding.getValue() == 
		ChemColor::BOND_PER_ATOM) {

		// The 2 vertices of the line comprising the bond are colored by
		// the atom color (indexed or not)
		const SbColor *atomColors = chemColor->atomColor.getValues(0);

		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->highlightStippleFactor.getValue(),
					cdp->highlightStipplePattern.getValue());
			}
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_OVERALL) {
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				tmpColor = atomColors[0] + highlightColor;
				tmpColor.normalize();
				glColor3fv(tmpColor.getValue());
			}
			else {
				glColor3fv(atomColors[0].getValue());
			}

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				tsep = bondAttributes[listIndex].perpNormal * multiSep;
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

				coords1 += tsep2;
				coords2 += tsep2;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());

				coords1 += tsep;
				coords2 += tsep;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());

				coords1 -= 2.0f*tsep;
				coords2 -= 2.0f*tsep;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());

				coords1 -= tsep;
				coords2 -= tsep;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
			}
			glEnd();

			if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
				!renderAsPoints) {
				glDisable(GL_LINE_STIPPLE);
			}

		}
		else if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_PER_ATOM) {

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				if (highlightStyle ==
					ChemDisplayParam::HIGHLIGHT_EMISSIVE) {

					color1 = atomColors[from] + highlightColor;
					color1.normalize();

					color2 = atomColors[to] + highlightColor;
					color2.normalize();
				}
				else {
					color1 = atomColors[from];
					color2 = atomColors[to];
				}
				tsep = bondAttributes[listIndex].perpNormal * multiSep;
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

				coords1 += tsep2;
				coords2 += tsep2;

				glColor3fv(color1.getValue());
				glVertex3fv(coords1.getValue());
				glColor3fv(color2.getValue());
				glVertex3fv(coords2.getValue());

				coords1 += tsep;
				coords2 += tsep;

				glColor3fv(color1.getValue());
				glVertex3fv(coords1.getValue());
				glColor3fv(color2.getValue());
				glVertex3fv(coords2.getValue());

				coords1 -= 2.0f*tsep;
				coords2 -= 2.0f*tsep;

				glColor3fv(color1.getValue());
				glVertex3fv(coords1.getValue());
				glColor3fv(color2.getValue());
				glVertex3fv(coords2.getValue());

				coords1 -= tsep;
				coords2 -= tsep;

				glColor3fv(color1.getValue());
				glVertex3fv(coords1.getValue());
				glColor3fv(color2.getValue());
				glVertex3fv(coords2.getValue());
			}
			glEnd();

			if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
				!renderAsPoints) {
				glDisable(GL_LINE_STIPPLE);
			}

		}
		else if (chemColor->atomColorBinding.getValue() == 
			ChemColor::ATOM_PER_ATOM_INDEXED) {

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				if (highlightStyle ==
					ChemDisplayParam::HIGHLIGHT_EMISSIVE) {

					color1 = atomColors[chemData->getAtomIndex(from)] +
						highlightColor;
					color1.normalize();

					color2 = atomColors[chemData->getAtomIndex(to)] +
						highlightColor;
					color2.normalize();
				}
				else {
					color1 = atomColors[chemData->getAtomIndex(from)];
					color2 = atomColors[chemData->getAtomIndex(to)];
				}

				tsep = bondAttributes[listIndex].perpNormal * multiSep;
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

				coords1 += tsep2;
				coords2 += tsep2;

				glColor3fv(color1.getValue());
				glVertex3fv(coords1.getValue());
				glColor3fv(color2.getValue());
				glVertex3fv(coords2.getValue());

				coords1 += tsep;
				coords2 += tsep;

				glColor3fv(color1.getValue());
				glVertex3fv(coords1.getValue());
				glColor3fv(color2.getValue());
				glVertex3fv(coords2.getValue());

				coords1 -= 2.0f*tsep;
				coords2 -= 2.0f*tsep;

				glColor3fv(color1.getValue());
				glVertex3fv(coords1.getValue());
				glColor3fv(color2.getValue());
				glVertex3fv(coords2.getValue());

				coords1 -= tsep;
				coords2 -= tsep;

				glColor3fv(color1.getValue());
				glVertex3fv(coords1.getValue());
				glColor3fv(color2.getValue());
				glVertex3fv(coords2.getValue());
			}
			glEnd();

			if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
				!renderAsPoints) {
				glDisable(GL_LINE_STIPPLE);
			}

		}
	}
	else if (chemColor->bondColorBinding.getValue() == 
		ChemColor::BOND_PER_ATOM_HALF_BONDED) {

		// Each bond is drawn as 2 lines with each line being the color of
		// one of the atoms (indexed or not????)
		const SbColor *atomColors = chemColor->atomColor.getValues(0);

		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->highlightStippleFactor.getValue(),
					cdp->highlightStipplePattern.getValue());
			}
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_OVERALL) {
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				tmpColor = atomColors[0] + highlightColor;
				tmpColor.normalize();
				glColor3fv(tmpColor.getValue());
			}
			else {
				glColor3fv(atomColors[0].getValue());
			}

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				tsep = bondAttributes[listIndex].perpNormal * multiSep;
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

				coords1 += tsep2;
				coords2 += tsep2;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());

				coords1 += tsep;
				coords2 += tsep;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());

				coords1 -= 2.0f*tsep;
				coords2 -= 2.0f*tsep;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());

				coords1 -= tsep;
				coords2 -= tsep;
				glVertex3fv(coords1.getValue());
				glVertex3fv(coords2.getValue());
			}
			glEnd();

			if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
				!renderAsPoints) {
				glDisable(GL_LINE_STIPPLE);
			}

		}
		else if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_PER_ATOM) {

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}

				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
					color1 = atomColors[from] + highlightColor;
					color1.normalize();

					color2 = atomColors[to] + highlightColor;
					color2.normalize();
				}
				else {
					color1 = atomColors[from];
					color2 = atomColors[to];
				}

				tsep = bondAttributes[listIndex].perpNormal * multiSep;
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

				if (color1 == color2) {
					glColor3fv(color1.getValue());
					coords1 += tsep2;
					coords2 += tsep2;

					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());

					coords1 += tsep;
					coords2 += tsep;

					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());

					coords1 -= 2.0f*tsep;
					coords2 -= 2.0f*tsep;

					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());

					coords1 -= tsep;
					coords2 -= tsep;

					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
				// If the 2 ends are not the same color, find the midpoint
				// and draw half-bonds
				else {
					tcoord = (coords2 - coords1) * 0.5f + coords1;
					// Set the color and draw the first 4 half bonds
					glColor3fv(color1.getValue());

					tc0 = tcoord + tsep2;
					tc1 = coords1 + tsep2;
					glVertex3fv(tc1.getValue());
					glVertex3fv(tc0.getValue());

					tc0 += tsep;
					tc1 += tsep;
					glVertex3fv(tc1.getValue());
					glVertex3fv(tc0.getValue());

					tc0 -= 2.0f*tsep;
					tc1 -= 2.0f*tsep;
					glVertex3fv(tc1.getValue());
					glVertex3fv(tc0.getValue());

					tc0 -= tsep;
					tc1 -= tsep;
					glVertex3fv(tc1.getValue());
					glVertex3fv(tc0.getValue());

					// Set the color and draw the second 4 half bonds
					glColor3fv(color2.getValue());

					tc0 = tcoord + tsep2;
					tc2 = coords2 + tsep2;
					glVertex3fv(tc0.getValue());
					glVertex3fv(tc2.getValue());

					tc0 += tsep;
					tc2 += tsep;
					glVertex3fv(tc0.getValue());
					glVertex3fv(tc2.getValue());

					tc0 -= 2.0f*tsep;
					tc2 -= 2.0f*tsep;
					glVertex3fv(tc0.getValue());
					glVertex3fv(tc2.getValue());

					tc0 -= tsep;
					tc2 -= tsep;
					glVertex3fv(tc0.getValue());
					glVertex3fv(tc2.getValue());
				}
			}
			glEnd();

			if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
				!renderAsPoints) {
				glDisable(GL_LINE_STIPPLE);
			}

		}
		else if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_PER_ATOM_INDEXED) {

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
					color1 = atomColors[chemData->getAtomIndex(from)];
					color1 += highlightColor;
					color1.normalize();

					color2 = atomColors[chemData->getAtomIndex(to)];
					color2 += highlightColor;
					color2.normalize();
				}
				else {
					color1 = atomColors[chemData->getAtomIndex(from)];
					color2 = atomColors[chemData->getAtomIndex(to)];
				}

				tsep = bondAttributes[listIndex].perpNormal * multiSep;
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

				if (color1 == color2) {
					glColor3fv(color1.getValue());
					coords1 += tsep2;
					coords2 += tsep2;

					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());

					coords1 += tsep;
					coords2 += tsep;

					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());

					coords1 -= 2.0f*tsep;
					coords2 -= 2.0f*tsep;

					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());

					coords1 -= tsep;
					coords2 -= tsep;

					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
				// If the 2 ends are not the same color, find the midpoint
				// and draw half-bonds
				else {
					tcoord = (coords2 - coords1) * 0.5f + coords1;
					// Set the color and draw the first 4 half bonds
					glColor3fv(color1.getValue());

					tc0 = tcoord + tsep2;
					tc1 = coords1 + tsep2;
					glVertex3fv(tc1.getValue());
					glVertex3fv(tc0.getValue());

					tc0 += tsep;
					tc1 += tsep;
					glVertex3fv(tc1.getValue());
					glVertex3fv(tc0.getValue());

					tc0 -= 2.0f*tsep;
					tc1 -= 2.0f*tsep;
					glVertex3fv(tc1.getValue());
					glVertex3fv(tc0.getValue());

					tc0 -= tsep;
					tc1 -= tsep;
					glVertex3fv(tc1.getValue());
					glVertex3fv(tc0.getValue());

					// Set the color and draw the second 4 half bonds
					glColor3fv(color2.getValue());

					tc0 = tcoord + tsep2;
					tc2 = coords2 + tsep2;
					glVertex3fv(tc0.getValue());
					glVertex3fv(tc2.getValue());

					tc0 += tsep;
					tc2 += tsep;
					glVertex3fv(tc0.getValue());
					glVertex3fv(tc2.getValue());

					tc0 -= 2.0f*tsep;
					tc2 -= 2.0f*tsep;
					glVertex3fv(tc0.getValue());
					glVertex3fv(tc2.getValue());

					tc0 -= tsep;
					tc2 -= tsep;
					glVertex3fv(tc0.getValue());
					glVertex3fv(tc2.getValue());
				}
			}
			glEnd();

			if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
				!renderAsPoints) {
				glDisable(GL_LINE_STIPPLE);
			}

		}
	}
	else if (chemColor->bondColorBinding.getValue() ==
		ChemColor::BOND_PER_BOND) {

		// Each bond has a color in the bond color table.  In the table,
		// there is a separate color for each bond.
		const SbColor *bondColors = chemColor->bondColor.getValues(0);

		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->highlightStippleFactor.getValue(),
					cdp->highlightStipplePattern.getValue());
			}
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		for (i = 0; i < listLen; i++) {
			listIndex = list[i];
			theBond = bondAttributes[listIndex].index;
			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}

			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				tmpColor = bondColors[theBond] + highlightColor;
				tmpColor.normalize();
				glColor3fv(tmpColor.getValue());
			}
			else {
				glColor3fv(bondColors[theBond].getValue());
			}

			tsep = bondAttributes[listIndex].perpNormal * multiSep;
			tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

			coords1 += tsep2;
			coords2 += tsep2;
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());

			coords1 += tsep;
			coords2 += tsep;
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());

			coords1 -= 2.0f*tsep;
			coords2 -= 2.0f*tsep;
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());

			coords1 -= tsep;
			coords2 -= tsep;
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());
		}
		glEnd();

		if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
			!renderAsPoints) {
			glDisable(GL_LINE_STIPPLE);
		}

	}
	else if (chemColor->bondColorBinding.getValue() ==
		ChemColor::BOND_PER_BOND_INDEXED) {

		// Each bond has an index into the bond color table.
		const SbColor *bondColors = chemColor->bondColor.getValues(0);

		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->highlightStippleFactor.getValue(),
					cdp->highlightStipplePattern.getValue());
			}
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		for (i = 0; i < listLen; i++) {
			listIndex = list[i];
			theBond = bondAttributes[listIndex].index;
			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				tmpColor = bondColors[chemData->getBondIndex(theBond)] +
					highlightColor;
				tmpColor.normalize();
				glColor3fv(tmpColor.getValue());
			}
			else {
				glColor3fv(bondColors[chemData->getBondIndex(theBond)].getValue());
			}

			tsep = bondAttributes[listIndex].perpNormal * multiSep;
			tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

			coords1 += tsep2;
			coords2 += tsep2;
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());

			coords1 += tsep;
			coords2 += tsep;
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());

			coords1 -= 2.0f*tsep;
			coords2 -= 2.0f*tsep;
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());

			coords1 -= tsep;
			coords2 -= tsep;
			glVertex3fv(coords1.getValue());
			glVertex3fv(coords2.getValue());
		}
		glEnd();

		if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
			!renderAsPoints) {
			glDisable(GL_LINE_STIPPLE);
		}

	}
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	   Performs highlighting of the molecule using lines.
//	   Invoked by the GLRender method.
//
// Use: static

void
resonanceBondsHighlighted
	(const SbIntList list, const SbBool renderAsPoints,
	const SbBool doHydrogens, const SbColor highlightColor,
	const int32_t highlightStyle,
	const ChemBaseData *chemData,
	const ChemDisplayParam *cdp, const ChemColor *chemColor,
	const ChemBondStuff *bondAttributes)
//
////////////////////////////////////////////////////////////////////////
{
	int32_t listLen;
	if ((listLen = list.getLength()) <= 0) return;

	int32_t i;
	int32_t listIndex;
	int32_t	from, to, theBond;
	SbVec3f coords1, coords2;
	SbVec3f tcoord, tc0, tc1, tc2;
	SbColor tmpColor, color1, color2;

	float  multiSep = cdp->multipleBondSeparation.getValue();
	float  multiSep2 = multiSep * 0.5;
	SbVec3f tsep, tsep2;

	if (chemColor->bondColorBinding.getValue() == 
		ChemColor::BOND_OVERALL) {

		// All bonds are the same color which is the first color in the
		// bond color table.
		const SbColor *bondColors = chemColor->bondColor.getValues(0);

		// We should only be here if highlightStyle is either
		// HIGHLIGHT_DRAWSTYLE or HIGHLIGHT_EMISSIVE.  If
		// HIGHLIGHT_DIFFUSE, then highlightWireframeOverall should be called.

		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
			glColor3fv(bondColors[0].getValue());
		}
		else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			tmpColor = bondColors[0] + highlightColor;
			tmpColor.normalize();
			glColor3fv(tmpColor.getValue());
		}

		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->highlightStippleFactor.getValue(),
					cdp->highlightStipplePattern.getValue());
			}
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		for (i = 0; i < listLen; i++) {
			listIndex = list[i];
			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				tsep = bondAttributes[listIndex].perpNormal * multiSep;
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

				if (bondAttributes[listIndex].isSymmetric == FALSE) {
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
					coords1 = bondAttributes[listIndex].start;
					coords2 = bondAttributes[listIndex].end;
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
				else {
					coords1 += tsep2;
					coords2 += tsep2;
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());

					coords1 -= tsep;
					coords2 -= tsep;
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
			}
			else {
				if (bondAttributes[listIndex].isSymmetric == FALSE) {
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
				else {
					tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
					coords1 += tsep2;
					coords2 += tsep2;
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
			}
		}
		glEnd();

		if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
			!renderAsPoints) {
			glDisable(GL_LINE_STIPPLE);
		}

		if (highlightStyle != ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
			if (renderAsPoints) {
				glBegin(GL_POINTS);
			}
			else {
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->resonanceBondStippleFactor.getValue(),
					cdp->resonanceBondStipplePattern.getValue());
				glLineWidth(cdp->bondWireframeLineWidth.getValue());
				glBegin(GL_LINES);
			}
			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();
				if (bondAttributes[listIndex].isSymmetric == FALSE) {
					coords1 = bondAttributes[listIndex].start;
					coords2 = bondAttributes[listIndex].end;
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
				else {
					tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
					coords1 -= tsep2;
					coords2 -= tsep2;
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
			}
			glEnd();
			if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);
		}
	}
	else if (chemColor->bondColorBinding.getValue() == 
		ChemColor::BOND_PER_ATOM) {

		// The 2 vertices of the line comprising the bond are colored by
		// the atom color (indexed or not)
		const SbColor *atomColors = chemColor->atomColor.getValues(0);

		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->highlightStippleFactor.getValue(),
					cdp->highlightStipplePattern.getValue());
			}
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_OVERALL) {
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				tmpColor = atomColors[0] + highlightColor;
				tmpColor.normalize();
				glColor3fv(tmpColor.getValue());
			}
			else {
				glColor3fv(atomColors[0].getValue());
			}

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
					tsep = bondAttributes[listIndex].perpNormal * multiSep;
					tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

					if (bondAttributes[listIndex].isSymmetric == FALSE) {
						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
						coords1 = bondAttributes[listIndex].start;
						coords2 = bondAttributes[listIndex].end;
						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
					}
					else {
						coords1 += tsep2;
						coords2 += tsep2;
						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());

						coords1 -= tsep;
						coords2 -= tsep;
						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
					}
				}
				else {
					if (bondAttributes[listIndex].isSymmetric == FALSE) {
						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
					}
					else {
						tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
						coords1 += tsep2;
						coords2 += tsep2;
						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
					}
				}
			}
			glEnd();

			if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
				!renderAsPoints) {
				glDisable(GL_LINE_STIPPLE);
			}

			if (highlightStyle != ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				if (renderAsPoints) {
					glBegin(GL_POINTS);
				}
				else {
					glEnable(GL_LINE_STIPPLE);
					glLineStipple(cdp->resonanceBondStippleFactor.getValue(),
						cdp->resonanceBondStipplePattern.getValue());
					glLineWidth(cdp->bondWireframeLineWidth.getValue());
					glBegin(GL_LINES);
				}
				for (i = 0; i < listLen; i++) {
					listIndex = list[i];
					from = bondAttributes[listIndex].from;
					to = bondAttributes[listIndex].to;

					if (!doHydrogens) {
						if ((chemData->getAtomicNumber(from) == 1) || 
							(chemData->getAtomicNumber(to) == 1)) continue;
					}
					coords1 = chemData->getAtomCoordinates(from).getValue();
					coords2 = chemData->getAtomCoordinates(to).getValue();
					if (bondAttributes[listIndex].isSymmetric == FALSE) {
						coords1 = bondAttributes[listIndex].start;
						coords2 = bondAttributes[listIndex].end;
						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
					}
					else {
						tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
						coords1 -= tsep2;
						coords2 -= tsep2;
						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
					}
				}
				glEnd();
				if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);
			}
		}
		else if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_PER_ATOM) {

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				if (highlightStyle ==
					ChemDisplayParam::HIGHLIGHT_EMISSIVE) {

					color1 = atomColors[from] + highlightColor;
					color1.normalize();

					color2 = atomColors[to] + highlightColor;
					color2.normalize();
				}
				else {
					color1 = atomColors[from];
					color2 = atomColors[to];
				}
				if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
					tsep = bondAttributes[listIndex].perpNormal * multiSep;
					tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

					if (bondAttributes[listIndex].isSymmetric == FALSE) {
						glColor3fv(color1.getValue());
						glVertex3fv(coords1.getValue());
						glColor3fv(color2.getValue());
						glVertex3fv(coords2.getValue());

						coords1 = bondAttributes[listIndex].start;
						coords2 = bondAttributes[listIndex].end;

						glColor3fv(color1.getValue());
						glVertex3fv(coords1.getValue());
						glColor3fv(color2.getValue());
						glVertex3fv(coords2.getValue());
					}
					else {
						coords1 += tsep2;
						coords2 += tsep2;

						glColor3fv(color1.getValue());
						glVertex3fv(coords1.getValue());
						glColor3fv(color2.getValue());
						glVertex3fv(coords2.getValue());

						coords1 -= tsep;
						coords2 -= tsep;

						glColor3fv(color1.getValue());
						glVertex3fv(coords1.getValue());
						glColor3fv(color2.getValue());
						glVertex3fv(coords2.getValue());
					}
				}
				else {
					if (bondAttributes[listIndex].isSymmetric == FALSE) {
						glColor3fv(color1.getValue());
						glVertex3fv(coords1.getValue());
						glColor3fv(color2.getValue());
						glVertex3fv(coords2.getValue());
					}
					else {
						tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
						coords1 += tsep2;
						coords2 += tsep2;

						glColor3fv(color1.getValue());
						glVertex3fv(coords1.getValue());
						glColor3fv(color2.getValue());
						glVertex3fv(coords2.getValue());
					}
				}
			}
			glEnd();

			if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
				!renderAsPoints) {
				glDisable(GL_LINE_STIPPLE);
			}

			if (highlightStyle != ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				if (renderAsPoints) {
					glBegin(GL_POINTS);
				}
				else {
					glEnable(GL_LINE_STIPPLE);
					glLineStipple(cdp->resonanceBondStippleFactor.getValue(),
						cdp->resonanceBondStipplePattern.getValue());
					glLineWidth(cdp->bondWireframeLineWidth.getValue());
					glBegin(GL_LINES);
				}
				for (i = 0; i < listLen; i++) {
					listIndex = list[i];
					from = bondAttributes[listIndex].from;
					to = bondAttributes[listIndex].to;

					if (!doHydrogens) {
						if ((chemData->getAtomicNumber(from) == 1) || 
							(chemData->getAtomicNumber(to) == 1)) continue;
					}
					coords1 = chemData->getAtomCoordinates(from).getValue();
					coords2 = chemData->getAtomCoordinates(to).getValue();

					if (highlightStyle ==
						ChemDisplayParam::HIGHLIGHT_EMISSIVE) {

						color1 = atomColors[from] + highlightColor;
						color1.normalize();

						color2 = atomColors[to] + highlightColor;
						color2.normalize();
					}
					else {
						color1 = atomColors[from];
						color2 = atomColors[to];
					}
					if (bondAttributes[listIndex].isSymmetric == FALSE) {
						coords1 = bondAttributes[listIndex].start;
						coords2 = bondAttributes[listIndex].end;

						glColor3fv(color1.getValue());
						glVertex3fv(coords1.getValue());
						glColor3fv(color2.getValue());
						glVertex3fv(coords2.getValue());
					}
					else {
						tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
						coords1 -= tsep2;
						coords2 -= tsep2;

						glColor3fv(color1.getValue());
						glVertex3fv(coords1.getValue());
						glColor3fv(color2.getValue());
						glVertex3fv(coords2.getValue());
					}
				}
				glEnd();
				if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);
			}
		}
		else if (chemColor->atomColorBinding.getValue() == 
			ChemColor::ATOM_PER_ATOM_INDEXED) {

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				if (highlightStyle ==
					ChemDisplayParam::HIGHLIGHT_EMISSIVE) {

					color1 = atomColors[chemData->getAtomIndex(from)] +
						highlightColor;
					color1.normalize();

					color2 = atomColors[chemData->getAtomIndex(to)] +
						highlightColor;
					color2.normalize();
				}
				else {
					color1 = atomColors[chemData->getAtomIndex(from)];
					color2 = atomColors[chemData->getAtomIndex(to)];
				}

				if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
					tsep = bondAttributes[listIndex].perpNormal * multiSep;
					tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

					if (bondAttributes[listIndex].isSymmetric == FALSE) {
						glColor3fv(color1.getValue());
						glVertex3fv(coords1.getValue());
						glColor3fv(color2.getValue());
						glVertex3fv(coords2.getValue());

						coords1 = bondAttributes[listIndex].start;
						coords2 = bondAttributes[listIndex].end;

						glColor3fv(color1.getValue());
						glVertex3fv(coords1.getValue());
						glColor3fv(color2.getValue());
						glVertex3fv(coords2.getValue());
					}
					else {
						coords1 += tsep2;
						coords2 += tsep2;

						glColor3fv(color1.getValue());
						glVertex3fv(coords1.getValue());
						glColor3fv(color2.getValue());
						glVertex3fv(coords2.getValue());

						coords1 -= tsep;
						coords2 -= tsep;

						glColor3fv(color1.getValue());
						glVertex3fv(coords1.getValue());
						glColor3fv(color2.getValue());
						glVertex3fv(coords2.getValue());
					}
				}
				else {
					if (bondAttributes[listIndex].isSymmetric == FALSE) {
						glColor3fv(color1.getValue());
						glVertex3fv(coords1.getValue());
						glColor3fv(color2.getValue());
						glVertex3fv(coords2.getValue());
					}
					else {
						tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
						coords1 += tsep2;
						coords2 += tsep2;

						glColor3fv(color1.getValue());
						glVertex3fv(coords1.getValue());
						glColor3fv(color2.getValue());
						glVertex3fv(coords2.getValue());
					}
				}
			}
			glEnd();

			if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
				!renderAsPoints) {
				glDisable(GL_LINE_STIPPLE);
			}

			if (highlightStyle != ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				if (renderAsPoints) {
					glBegin(GL_POINTS);
				}
				else {
					glEnable(GL_LINE_STIPPLE);
					glLineStipple(cdp->resonanceBondStippleFactor.getValue(),
						cdp->resonanceBondStipplePattern.getValue());
					glLineWidth(cdp->bondWireframeLineWidth.getValue());
					glBegin(GL_LINES);
				}
				for (i = 0; i < listLen; i++) {
					listIndex = list[i];
					from = bondAttributes[listIndex].from;
					to = bondAttributes[listIndex].to;

					if (!doHydrogens) {
						if ((chemData->getAtomicNumber(from) == 1) || 
							(chemData->getAtomicNumber(to) == 1)) continue;
					}
					coords1 = chemData->getAtomCoordinates(from).getValue();
					coords2 = chemData->getAtomCoordinates(to).getValue();

					if (highlightStyle ==
						ChemDisplayParam::HIGHLIGHT_EMISSIVE) {

						color1 = atomColors[chemData->getAtomIndex(from)] +
							highlightColor;
						color1.normalize();

						color2 = atomColors[chemData->getAtomIndex(to)] +
							highlightColor;
						color2.normalize();
					}
					else {
						color1 = atomColors[chemData->getAtomIndex(from)];
						color2 = atomColors[chemData->getAtomIndex(to)];
					}
					if (bondAttributes[listIndex].isSymmetric == FALSE) {
						coords1 = bondAttributes[listIndex].start;
						coords2 = bondAttributes[listIndex].end;

						glColor3fv(color1.getValue());
						glVertex3fv(coords1.getValue());
						glColor3fv(color2.getValue());
						glVertex3fv(coords2.getValue());
					}
					else {
						tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
						coords1 -= tsep2;
						coords2 -= tsep2;

						glColor3fv(color1.getValue());
						glVertex3fv(coords1.getValue());
						glColor3fv(color2.getValue());
						glVertex3fv(coords2.getValue());
					}
				}
				glEnd();
				if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);
			}
		}
	}
	else if (chemColor->bondColorBinding.getValue() == 
		ChemColor::BOND_PER_ATOM_HALF_BONDED) {

		// Each bond is drawn as 2 lines with each line being the color of
		// one of the atoms (indexed or not????)
		const SbColor *atomColors = chemColor->atomColor.getValues(0);

		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->highlightStippleFactor.getValue(),
					cdp->highlightStipplePattern.getValue());
			}
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_OVERALL) {
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				tmpColor = atomColors[0] + highlightColor;
				tmpColor.normalize();
				glColor3fv(tmpColor.getValue());
			}
			else {
				glColor3fv(atomColors[0].getValue());
			}

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
					tsep = bondAttributes[listIndex].perpNormal * multiSep;
					tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

					if (bondAttributes[listIndex].isSymmetric == FALSE) {
						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
						coords1 = bondAttributes[listIndex].start;
						coords2 = bondAttributes[listIndex].end;
						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
					}
					else {
						coords1 += tsep2;
						coords2 += tsep2;
						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());

						coords1 -= tsep;
						coords2 -= tsep;
						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
					}
				}
				else {
					if (bondAttributes[listIndex].isSymmetric == FALSE) {
						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
					}
					else {
						tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
						coords1 += tsep2;
						coords2 += tsep2;
						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
					}
				}
			}
			glEnd();

			if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
				!renderAsPoints) {
				glDisable(GL_LINE_STIPPLE);
			}

			if (highlightStyle != ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				if (renderAsPoints) {
					glBegin(GL_POINTS);
				}
				else {
					glEnable(GL_LINE_STIPPLE);
					glLineStipple(cdp->resonanceBondStippleFactor.getValue(),
						cdp->resonanceBondStipplePattern.getValue());
					glLineWidth(cdp->bondWireframeLineWidth.getValue());
					glBegin(GL_LINES);
				}
				for (i = 0; i < listLen; i++) {
					listIndex = list[i];
					from = bondAttributes[listIndex].from;
					to = bondAttributes[listIndex].to;

					if (!doHydrogens) {
						if ((chemData->getAtomicNumber(from) == 1) || 
							(chemData->getAtomicNumber(to) == 1)) continue;
					}
					coords1 = chemData->getAtomCoordinates(from).getValue();
					coords2 = chemData->getAtomCoordinates(to).getValue();
					if (bondAttributes[listIndex].isSymmetric == FALSE) {
						coords1 = bondAttributes[listIndex].start;
						coords2 = bondAttributes[listIndex].end;
						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
					}
					else {
						tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
						coords1 -= tsep2;
						coords2 -= tsep2;
						glVertex3fv(coords1.getValue());
						glVertex3fv(coords2.getValue());
					}
				}
				glEnd();
				if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);
			}
		}
		else if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_PER_ATOM) {

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}

				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
					color1 = atomColors[from] + highlightColor;
					color1.normalize();

					color2 = atomColors[to] + highlightColor;
					color2.normalize();
				}
				else {
					color1 = atomColors[from];
					color2 = atomColors[to];
				}

				if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
					tsep = bondAttributes[listIndex].perpNormal * multiSep;
					tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

					if (color1 == color2) {
						glColor3fv(color1.getValue());
						if (bondAttributes[listIndex].isSymmetric == FALSE) {
							glVertex3fv(coords1.getValue());
							glVertex3fv(coords2.getValue());

							coords1 = bondAttributes[listIndex].start;
							coords2 = bondAttributes[listIndex].end;

							glVertex3fv(coords1.getValue());
							glVertex3fv(coords2.getValue());
						}
						else {
							coords1 += tsep2;
							coords2 += tsep2;

							glVertex3fv(coords1.getValue());
							glVertex3fv(coords2.getValue());

							coords1 -= tsep;
							coords2 -= tsep;

							glVertex3fv(coords1.getValue());
							glVertex3fv(coords2.getValue());
						}
					}
					// If the 2 ends are not the same color, find the midpoint
					// and draw half-bonds
					else {
						tcoord = (coords2 - coords1) * 0.5f + coords1;
						if (bondAttributes[listIndex].isSymmetric == FALSE) {
							tc0 = tcoord + tsep;
							tc1 = bondAttributes[listIndex].start;
							tc2 = bondAttributes[listIndex].end;

							glColor3fv(color1.getValue());
							glVertex3fv(coords1.getValue());
							glVertex3fv(tcoord.getValue());
							glVertex3fv(tc1.getValue());
							glVertex3fv(tc0.getValue());

							glColor3fv(color2.getValue());
							glVertex3fv(tcoord.getValue());
							glVertex3fv(coords2.getValue());
							glVertex3fv(tc0.getValue());
							glVertex3fv(tc2.getValue());
						}
						else {
							tcoord += tsep2;
							coords1 += tsep2;
							coords2 += tsep2;
							tc0 = tcoord - tsep;
							tc1 = coords1 - tsep;
							tc2 = coords2 - tsep;

							glColor3fv(color1.getValue());
							glVertex3fv(coords1.getValue());
							glVertex3fv(tcoord.getValue());
							glVertex3fv(tc1.getValue());
							glVertex3fv(tc0.getValue());

							glColor3fv(color2.getValue());
							glVertex3fv(tcoord.getValue());
							glVertex3fv(coords2.getValue());
							glVertex3fv(tc0.getValue());
							glVertex3fv(tc2.getValue());
						}
					}
				}
				else {
					if (color1 == color2) {
						glColor3fv(color1.getValue());
						if (bondAttributes[listIndex].isSymmetric == FALSE) {
							glVertex3fv(coords1.getValue());
							glVertex3fv(coords2.getValue());
						}
						else {
							tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
							coords1 += tsep2;
							coords2 += tsep2;

							glVertex3fv(coords1.getValue());
							glVertex3fv(coords2.getValue());
						}
					}
					// If the 2 ends are not the same color, find the midpoint
					// and draw half-bonds
					else {
						tcoord = (coords2 - coords1) * 0.5f + coords1;
						if (bondAttributes[listIndex].isSymmetric == FALSE) {
							glColor3fv(color1.getValue());
							glVertex3fv(coords1.getValue());
							glVertex3fv(tcoord.getValue());

							glColor3fv(color2.getValue());
							glVertex3fv(tcoord.getValue());
							glVertex3fv(coords2.getValue());
						}
						else {
							tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
							tcoord += tsep2;
							coords1 += tsep2;
							coords2 += tsep2;

							glColor3fv(color1.getValue());
							glVertex3fv(coords1.getValue());
							glVertex3fv(tcoord.getValue());

							glColor3fv(color2.getValue());
							glVertex3fv(tcoord.getValue());
							glVertex3fv(coords2.getValue());
						}
					}
				}
			}
			glEnd();

			if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
				!renderAsPoints) {
				glDisable(GL_LINE_STIPPLE);
			}

			if (highlightStyle != ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				if (renderAsPoints) {
					glBegin(GL_POINTS);
				}
				else {
					glEnable(GL_LINE_STIPPLE);
					glLineStipple(cdp->resonanceBondStippleFactor.getValue(),
						cdp->resonanceBondStipplePattern.getValue());
					glLineWidth(cdp->bondWireframeLineWidth.getValue());
					glBegin(GL_LINES);
				}
				for (i = 0; i < listLen; i++) {
					listIndex = list[i];
					from = bondAttributes[listIndex].from;
					to = bondAttributes[listIndex].to;

					if (!doHydrogens) {
						if ((chemData->getAtomicNumber(from) == 1) || 
							(chemData->getAtomicNumber(to) == 1)) continue;
					}

					coords1 = chemData->getAtomCoordinates(from).getValue();
					coords2 = chemData->getAtomCoordinates(to).getValue();

					if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
						color1 = atomColors[from] + highlightColor;
						color1.normalize();

						color2 = atomColors[to] + highlightColor;
						color2.normalize();
					}
					else {
						color1 = atomColors[from];
						color2 = atomColors[to];
					}
					if (color1 == color2) {
						glColor3fv(color1.getValue());
						if (bondAttributes[listIndex].isSymmetric == FALSE) {
							coords1 = bondAttributes[listIndex].start;
							coords2 = bondAttributes[listIndex].end;

							glVertex3fv(coords1.getValue());
							glVertex3fv(coords2.getValue());
						}
						else {
							tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
							coords1 -= tsep2;
							coords2 -= tsep2;

							glVertex3fv(coords1.getValue());
							glVertex3fv(coords2.getValue());
						}
					}
					// If the 2 ends are not the same color, find the midpoint
					// and draw half-bonds
					else {
						if (bondAttributes[listIndex].isSymmetric == FALSE) {
							tc1 = bondAttributes[listIndex].start;
							tc2 = bondAttributes[listIndex].end;
							tc0 = (tc2 - tc1) * 0.5f + tc1;

							glColor3fv(color1.getValue());
							glVertex3fv(tc1.getValue());
							glVertex3fv(tc0.getValue());

							glColor3fv(color2.getValue());
							glVertex3fv(tc0.getValue());
							glVertex3fv(tc2.getValue());
						}
						else {
							tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
							tcoord = (coords2 - coords1) * 0.5f + coords1;
							tcoord -= tsep2;
							coords1 -= tsep2;
							coords2 -= tsep2;

							glColor3fv(color2.getValue());
							glVertex3fv(coords1.getValue());
							glVertex3fv(tcoord.getValue());

							glColor3fv(color2.getValue());
							glVertex3fv(tcoord.getValue());
							glVertex3fv(coords2.getValue());
						}
					}
				}
				glEnd();
				if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);
			}
		}
		else if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_PER_ATOM_INDEXED) {

			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
					color1 = atomColors[chemData->getAtomIndex(from)];
					color1 += highlightColor;
					color1.normalize();

					color2 = atomColors[chemData->getAtomIndex(to)];
					color2 += highlightColor;
					color2.normalize();
				}
				else {
					color1 = atomColors[chemData->getAtomIndex(from)];
					color2 = atomColors[chemData->getAtomIndex(to)];
				}

				if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
					tsep = bondAttributes[listIndex].perpNormal * multiSep;
					tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

					if (color1 == color2) {
						glColor3fv(color1.getValue());
						if (bondAttributes[listIndex].isSymmetric == FALSE) {
							glVertex3fv(coords1.getValue());
							glVertex3fv(coords2.getValue());

							coords1 = bondAttributes[listIndex].start;
							coords2 = bondAttributes[listIndex].end;

							glVertex3fv(coords1.getValue());
							glVertex3fv(coords2.getValue());
						}
						else {
							coords1 += tsep2;
							coords2 += tsep2;

							glVertex3fv(coords1.getValue());
							glVertex3fv(coords2.getValue());

							coords1 -= tsep;
							coords2 -= tsep;

							glVertex3fv(coords1.getValue());
							glVertex3fv(coords2.getValue());
						}
					}
					// If the 2 ends are not the same color, find the midpoint
					// and draw half-bonds
					else {
						tcoord = (coords2 - coords1) * 0.5f + coords1;
						if (bondAttributes[listIndex].isSymmetric == FALSE) {
							tc0 = tcoord + tsep;
							tc1 = bondAttributes[listIndex].start;
							tc2 = bondAttributes[listIndex].end;

							glColor3fv(color1.getValue());
							glVertex3fv(coords1.getValue());
							glVertex3fv(tcoord.getValue());
							glVertex3fv(tc1.getValue());
							glVertex3fv(tc0.getValue());

							glColor3fv(color2.getValue());
							glVertex3fv(tcoord.getValue());
							glVertex3fv(coords2.getValue());
							glVertex3fv(tc0.getValue());
							glVertex3fv(tc2.getValue());
						}
						else {
							tcoord += tsep2;
							coords1 += tsep2;
							coords2 += tsep2;
							tc0 = tcoord - tsep;
							tc1 = coords1 - tsep;
							tc2 = coords2 - tsep;

							glColor3fv(color1.getValue());
							glVertex3fv(coords1.getValue());
							glVertex3fv(tcoord.getValue());
							glVertex3fv(tc1.getValue());
							glVertex3fv(tc0.getValue());

							glColor3fv(color2.getValue());
							glVertex3fv(tcoord.getValue());
							glVertex3fv(coords2.getValue());
							glVertex3fv(tc0.getValue());
							glVertex3fv(tc2.getValue());
						}
					}
				}
				else {
					if (color1 == color2) {
						glColor3fv(color1.getValue());
						if (bondAttributes[listIndex].isSymmetric == FALSE) {
							glVertex3fv(coords1.getValue());
							glVertex3fv(coords2.getValue());
						}
						else {
							tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
							coords1 += tsep2;
							coords2 += tsep2;

							glVertex3fv(coords1.getValue());
							glVertex3fv(coords2.getValue());
						}
					}
					// If the 2 ends are not the same color, find the midpoint
					// and draw half-bonds
					else {
						tcoord = (coords2 - coords1) * 0.5f + coords1;
						if (bondAttributes[listIndex].isSymmetric == FALSE) {
							glColor3fv(color1.getValue());
							glVertex3fv(coords1.getValue());
							glVertex3fv(tcoord.getValue());

							glColor3fv(color2.getValue());
							glVertex3fv(tcoord.getValue());
							glVertex3fv(coords2.getValue());
						}
						else {
							tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
							tcoord += tsep2;
							coords1 += tsep2;
							coords2 += tsep2;

							glColor3fv(color1.getValue());
							glVertex3fv(coords1.getValue());
							glVertex3fv(tcoord.getValue());

							glColor3fv(color2.getValue());
							glVertex3fv(tcoord.getValue());
							glVertex3fv(coords2.getValue());
						}
					}
				}
			}
			glEnd();

			if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
				!renderAsPoints) {
				glDisable(GL_LINE_STIPPLE);
			}

			if (highlightStyle != ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				if (renderAsPoints) {
					glBegin(GL_POINTS);
				}
				else {
					glEnable(GL_LINE_STIPPLE);
					glLineStipple(cdp->resonanceBondStippleFactor.getValue(),
						cdp->resonanceBondStipplePattern.getValue());
					glLineWidth(cdp->bondWireframeLineWidth.getValue());
					glBegin(GL_LINES);
				}
				for (i = 0; i < listLen; i++) {
					listIndex = list[i];
					from = bondAttributes[listIndex].from;
					to = bondAttributes[listIndex].to;

					if (!doHydrogens) {
						if ((chemData->getAtomicNumber(from) == 1) || 
							(chemData->getAtomicNumber(to) == 1)) continue;
					}
					coords1 = chemData->getAtomCoordinates(from).getValue();
					coords2 = chemData->getAtomCoordinates(to).getValue();

					if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
						color1 = atomColors[chemData->getAtomIndex(from)];
						color1 += highlightColor;
						color1.normalize();

						color2 = atomColors[chemData->getAtomIndex(to)];
						color2 += highlightColor;
						color2.normalize();
					}
					else {
						color1 = atomColors[chemData->getAtomIndex(from)];
						color2 = atomColors[chemData->getAtomIndex(to)];
					}
					if (color1 == color2) {
						glColor3fv(color1.getValue());
						if (bondAttributes[listIndex].isSymmetric == FALSE) {
							coords1 = bondAttributes[listIndex].start;
							coords2 = bondAttributes[listIndex].end;

							glVertex3fv(coords1.getValue());
							glVertex3fv(coords2.getValue());
						}
						else {
							tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
							coords1 -= tsep2;
							coords2 -= tsep2;

							glVertex3fv(coords1.getValue());
							glVertex3fv(coords2.getValue());
						}
					}
					// If the 2 ends are not the same color, find the midpoint
					// and draw half-bonds
					else {
						if (bondAttributes[listIndex].isSymmetric == FALSE) {
							tc1 = bondAttributes[listIndex].start;
							tc2 = bondAttributes[listIndex].end;
							tc0 = (tc2 - tc1) * 0.5f + tc1;

							glColor3fv(color1.getValue());
							glVertex3fv(tc1.getValue());
							glVertex3fv(tc0.getValue());

							glColor3fv(color2.getValue());
							glVertex3fv(tc0.getValue());
							glVertex3fv(tc2.getValue());
						}
						else {
							tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
							tcoord = (coords2 - coords1) * 0.5f + coords1;
							tcoord -= tsep2;
							coords1 -= tsep2;
							coords2 -= tsep2;

							glColor3fv(color2.getValue());
							glVertex3fv(coords1.getValue());
							glVertex3fv(tcoord.getValue());

							glColor3fv(color2.getValue());
							glVertex3fv(tcoord.getValue());
							glVertex3fv(coords2.getValue());
						}
					}
				}
				glEnd();
				if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);
			}
		}
	}
	else if (chemColor->bondColorBinding.getValue() ==
		ChemColor::BOND_PER_BOND) {

		// Each bond has a color in the bond color table.  In the table,
		// there is a separate color for each bond.
		const SbColor *bondColors = chemColor->bondColor.getValues(0);

		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->highlightStippleFactor.getValue(),
					cdp->highlightStipplePattern.getValue());
			}
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		for (i = 0; i < listLen; i++) {
			listIndex = list[i];
			theBond = bondAttributes[listIndex].index;
			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}

			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				tmpColor = bondColors[theBond] + highlightColor;
				tmpColor.normalize();
				glColor3fv(tmpColor.getValue());
			}
			else {
				glColor3fv(bondColors[theBond].getValue());
			}

			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				tsep = bondAttributes[listIndex].perpNormal * multiSep;
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

				if (bondAttributes[listIndex].isSymmetric == FALSE) {
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
					coords1 = bondAttributes[listIndex].start;
					coords2 = bondAttributes[listIndex].end;
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
				else {
					coords1 += tsep2;
					coords2 += tsep2;
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());

					coords1 -= tsep;
					coords2 -= tsep;
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
			}
			else {
				if (bondAttributes[listIndex].isSymmetric == FALSE) {
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
				else {
					tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
					coords1 += tsep2;
					coords2 += tsep2;
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
			}
		}
		glEnd();

		if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
			!renderAsPoints) {
			glDisable(GL_LINE_STIPPLE);
		}

		if (highlightStyle != ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
			if (renderAsPoints) {
				glBegin(GL_POINTS);
			}
			else {
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->resonanceBondStippleFactor.getValue(),
					cdp->resonanceBondStipplePattern.getValue());
				glLineWidth(cdp->bondWireframeLineWidth.getValue());
				glBegin(GL_LINES);
			}
			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				theBond = bondAttributes[listIndex].index;
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}

				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
					tmpColor = bondColors[theBond] + highlightColor;
					tmpColor.normalize();
					glColor3fv(tmpColor.getValue());
				}
				else {
					glColor3fv(bondColors[theBond].getValue());
				}
				if (bondAttributes[listIndex].isSymmetric == FALSE) {
					coords1 = bondAttributes[listIndex].start;
					coords2 = bondAttributes[listIndex].end;
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
				else {
					tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
					coords1 -= tsep2;
					coords2 -= tsep2;
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
			}
			glEnd();
			if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);
		}
	}
	else if (chemColor->bondColorBinding.getValue() ==
		ChemColor::BOND_PER_BOND_INDEXED) {

		// Each bond has an index into the bond color table.
		const SbColor *bondColors = chemColor->bondColor.getValues(0);

		if (renderAsPoints) {
			glBegin(GL_POINTS);
		}
		else {
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->highlightStippleFactor.getValue(),
					cdp->highlightStipplePattern.getValue());
			}
			glLineWidth(cdp->bondWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		for (i = 0; i < listLen; i++) {
			listIndex = list[i];
			theBond = bondAttributes[listIndex].index;
			from = bondAttributes[listIndex].from;
			to = bondAttributes[listIndex].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				tmpColor = bondColors[chemData->getBondIndex(theBond)] +
					highlightColor;
				tmpColor.normalize();
				glColor3fv(tmpColor.getValue());
			}
			else {
				glColor3fv(bondColors[chemData->getBondIndex(theBond)].getValue());
			}

			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				tsep = bondAttributes[listIndex].perpNormal * multiSep;
				tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;

				if (bondAttributes[listIndex].isSymmetric == FALSE) {
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
					coords1 = bondAttributes[listIndex].start;
					coords2 = bondAttributes[listIndex].end;
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
				else {
					coords1 += tsep2;
					coords2 += tsep2;
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());

					coords1 -= tsep;
					coords2 -= tsep;
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
			}
			else {
				if (bondAttributes[listIndex].isSymmetric == FALSE) {
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
				else {
					tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
					coords1 += tsep2;
					coords2 += tsep2;
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
			}
		}
		glEnd();

		if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) &&
			!renderAsPoints) {
			glDisable(GL_LINE_STIPPLE);
		}

		if (highlightStyle != ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
			if (renderAsPoints) {
				glBegin(GL_POINTS);
			}
			else {
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(cdp->resonanceBondStippleFactor.getValue(),
					cdp->resonanceBondStipplePattern.getValue());
				glLineWidth(cdp->bondWireframeLineWidth.getValue());
				glBegin(GL_LINES);
			}
			for (i = 0; i < listLen; i++) {
				listIndex = list[i];
				theBond = bondAttributes[listIndex].index;
				from = bondAttributes[listIndex].from;
				to = bondAttributes[listIndex].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
					tmpColor = bondColors[chemData->getBondIndex(theBond)] +
						highlightColor;
					tmpColor.normalize();
					glColor3fv(tmpColor.getValue());
				}
				else {
					glColor3fv(bondColors[chemData->getBondIndex(theBond)].getValue());
				}
				if (bondAttributes[listIndex].isSymmetric == FALSE) {
					coords1 = bondAttributes[listIndex].start;
					coords2 = bondAttributes[listIndex].end;
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
				else {
					tsep2 = bondAttributes[listIndex].perpNormal * multiSep2;
					coords1 -= tsep2;
					coords2 -= tsep2;
					glVertex3fv(coords1.getValue());
					glVertex3fv(coords2.getValue());
				}
			}
			glEnd();
			if (!renderAsPoints) glDisable(GL_LINE_STIPPLE);
		}
	}
}

//
// End of WIREFRAME section
//
