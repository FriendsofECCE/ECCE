/*
 * ChemDisplaySingleAtoms.pp
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

#ident "$Revision: 22147 $"

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

#include "inv/ChemKit/ChemDetail.H"
#include "inv/ChemKit/ChemDisplay.H"
#include "inv/ChemKit/ChemBaseDataElement.H"
#include "inv/ChemKit/ChemColorElement.H"
#include "inv/ChemKit/ChemDisplayParamElement.H"
#include "inv/ChemKit/ChemRadiiElement.H"

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

static int numAtomLoops;
static int atomLoop;   
static int32_t atomStart;
static int32_t atomEnd;
static int32_t theAtom;  

#define ATOMLOOP_START(INDEX)												  \
numAtomLoops = (INDEX).getNum();										      \
for (atomLoop = 0; atomLoop < numAtomLoops; atomLoop++) {					  \
	(INDEX)[atomLoop].getValue(atomStart, atomEnd);							  \
	if (atomEnd == CHEM_DISPLAY_USE_REST_OF_ATOMS) {						  \
	   atomEnd = globalNumberOfAtoms;										  \
	}																		  \
	else {																	  \
	   atomEnd += atomStart;												  \
	}																		  \
	for (theAtom = atomStart; theAtom < atomEnd; theAtom++) {

#define ATOMLOOP_END														  \
	}																		  \
}

//
// Start of Single Atom section
//

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Renders atoms with no bonds when in WIREFRAME and STICK display modes.
//    This is for normal rendering (not highlighted)
//
// Use: static, internal

static void
renderNormalSingleAtoms(int32_t globalNumberOfAtoms,
	const MFVec2i &singleIndex, const ChemBaseData *chemData,
	const ChemDisplayParam *cdp, const ChemColor *chemColor)
//
////////////////////////////////////////////////////////////////////////
{
	if (singleIndex.getNum() == 0) return;

	SbBool doHydrogens = cdp->showHydrogens.getValue();
	const SbColor *atomColors = chemColor->atomColor.getValues(0);
	SbVec3f coord;

	const uint8_t *xbmap;
	GLsizei xbmapWidth;
	GLsizei xbmapHeight;
	GLfloat xbmapWidth2;
	GLfloat xbmapHeight2;

	xbmap = cdp->nonBondedAtomBitmap.getValues(0);
	xbmapWidth = cdp->nonBondedAtomBitmapWidth.getValue();
	xbmapHeight = cdp->nonBondedAtomBitmapHeight.getValue();
	xbmapWidth2 = xbmapWidth * 0.5;
	xbmapHeight2 = xbmapHeight * 0.5;
	switch (chemColor->atomColorBinding.getValue()) {
	  case ChemColor::ATOM_OVERALL:
		glColor3fv(atomColors[0].getValue());
		ATOMLOOP_START(singleIndex)
			if (!doHydrogens && chemData->getAtomicNumber(theAtom) == 1) {
				continue;
			}
			coord = chemData->getAtomCoordinates(theAtom);

			glRasterPos3fv(coord.getValue());
			glBitmap(xbmapWidth, xbmapHeight, xbmapWidth2, xbmapHeight2,
					 (GLfloat)xbmapWidth, 0.0, xbmap);
		ATOMLOOP_END
		break;
	  case ChemColor::ATOM_PER_ATOM:
		ATOMLOOP_START(singleIndex)
			if (!doHydrogens && chemData->getAtomicNumber(theAtom) == 1) {
				continue;
			}
			coord = chemData->getAtomCoordinates(theAtom);

			glColor3fv(atomColors[theAtom].getValue());
			glRasterPos3fv(coord.getValue());
			glBitmap(xbmapWidth, xbmapHeight, xbmapWidth2, xbmapHeight2,
					 (GLfloat)xbmapWidth, 0.0, xbmap);
		ATOMLOOP_END
		break;
	  case ChemColor::ATOM_PER_ATOM_INDEXED:
		int32_t localAtomIndex;
		ATOMLOOP_START(singleIndex)
			if (!doHydrogens && chemData->getAtomicNumber(theAtom) == 1) {
				continue;
			}
			coord = chemData->getAtomCoordinates(theAtom);

			localAtomIndex = chemData->getAtomIndex(theAtom);
			glColor3fv(atomColors[localAtomIndex].getValue());
			glRasterPos3fv(coord.getValue());
			glBitmap(xbmapWidth, xbmapHeight, xbmapWidth2, xbmapHeight2,
					 (GLfloat)xbmapWidth, 0.0, xbmap);
		ATOMLOOP_END
		break;
	  default:
		break;
	}
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Highlight selected nonBonded atoms when in WIREFRAME and STICK
//    display modes.
//
// Use: static, internal

static void
renderHighlightedSingleAtoms(int32_t globalNumberOfAtoms,
	const MFVec2i &singleIndex, const ChemBaseData *chemData,
	const ChemDisplayParam *cdp, const ChemColor *chemColor)
//
////////////////////////////////////////////////////////////////////////
{
	if (singleIndex.getNum() == 0) return;

	SbBool doHydrogens = cdp->showHydrogens.getValue();

	const uint8_t *xbmap;
	GLsizei xbmapWidth;
	GLsizei xbmapHeight;
	GLfloat xbmapWidth2;
	GLfloat xbmapHeight2;

	int32_t highlightStyle = cdp->highlightStyle.getValue();
	int32_t highlightAtomStyle = cdp->highlightAtomStyle.getValue();

	if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) ||
		(highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)) {
		glColor3fv(cdp->highlightColor.getValue().getValue());
	}
	else {
		glColor3f(1.0, 1.0, 1.0);
	}

	if (highlightAtomStyle == ChemDisplayParam::HIGHLIGHT_ATOM_BITMAP) {
		xbmap = cdp->highlightAtomBitmap.getValues(0);
		xbmapWidth = cdp->highlightAtomBitmapWidth.getValue();
		xbmapHeight = cdp->highlightAtomBitmapHeight.getValue();
		xbmapWidth2 = xbmapWidth * 0.5;
		xbmapHeight2 = xbmapHeight * 0.5;

		ATOMLOOP_START(singleIndex)
			if (!doHydrogens && chemData->getAtomicNumber(theAtom) == 1) {
				continue;
			}
			SbVec3f coord = chemData->getAtomCoordinates(theAtom);

			glRasterPos3fv(coord.getValue());
			glBitmap(xbmapWidth, xbmapHeight, xbmapWidth2, xbmapHeight2,
					 (GLfloat)xbmapWidth, 0.0, xbmap);
		ATOMLOOP_END
	}
	else if (highlightAtomStyle == ChemDisplayParam::HIGHLIGHT_ATOM_BBOX) {
		float rad = cdp->bondCylinderRadius.getValue();
		SbVec3f coord;
		SbVec3f verts[6][4];
		verts[1][2] = verts[2][3] = verts[4][3] = SbVec3f(-rad,  rad, -rad);
		verts[1][3] = verts[3][2] = verts[4][2] = SbVec3f( rad,  rad, -rad);
		verts[1][1] = verts[2][0] = verts[5][0] = SbVec3f(-rad, -rad, -rad);
		verts[1][0] = verts[3][1] = verts[5][1] = SbVec3f( rad, -rad, -rad);
		verts[0][3] = verts[2][2] = verts[4][0] = SbVec3f(-rad,  rad,  rad);
		verts[0][2] = verts[3][3] = verts[4][1] = SbVec3f( rad,  rad,  rad);
		verts[0][0] = verts[2][1] = verts[5][3] = SbVec3f(-rad, -rad,  rad);
		verts[0][1] = verts[3][0] = verts[5][2] = SbVec3f( rad, -rad,  rad);

		ATOMLOOP_START(singleIndex)
			if (!doHydrogens && chemData->getAtomicNumber(theAtom) == 1) {
				continue;
			}
			coord = chemData->getAtomCoordinates(theAtom);
			glBegin(GL_LINE_STRIP);
			for (int face = 0; face < 6; face++) {
				for (int vert = 0; vert < 4; vert++) {
					glVertex3fv((coord + verts[face][vert]).getValue());
				}
			}
			glEnd();
		ATOMLOOP_END
	}
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Renders atoms with no bonds when in WIREFRAME and STICK display modes.
//
// Use: protected

void
ChemDisplay::renderSingleAtoms(SoGLRenderAction *action)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf("In ChemDisplay::renderSingleAtoms\n");
#endif

	if (nonBondedAtoms.getLength() <= 0) return;

	SoState *state = action->getState();
	ChemBaseData *chemData = ChemBaseDataElement::get(state);
	ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);
	ChemColor *chemColor = ChemColorElement::get(state);

	int32_t nonBondedAtomStyle = cdp->nonBondedAtomStyle.getValue();

	if (nonBondedAtomStyle == ChemDisplayParam::NONBONDED_ATOM_BITMAP) {
		state->push();

	    // Turn off lighting
#ifdef IV2_0
		SoLightModelElement::set(state, this, SoLightModelElement::BASE_COLOR);
#else
		SoGLLazyElement *lazyElt = (SoGLLazyElement *)
				SoLazyElement::getInstance(state);
		lazyElt->setLightModel(state, SoLazyElement::BASE_COLOR);
		lazyElt->sendNoMaterial(state);
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

		// Get the current bitmap alignment
		GLint curAlignment;
		glGetIntegerv(GL_UNPACK_ALIGNMENT, &curAlignment);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		if (normalNonBondedAtomIndex.getNum() > 0) {
			renderNormalSingleAtoms(globalNumberOfAtoms,
				normalNonBondedAtomIndex, chemData, cdp, chemColor);
		}
		if ((cdp->highlightStyle.getValue() !=
				ChemDisplayParam::HIGHLIGHT_NONE) &&
			(cdp->highlightAtomStyle.getValue() !=
				ChemDisplayParam::HIGHLIGHT_ATOM_NONE)) {

			// We are supposed to highlight, so check to see if there is
			// is anything to highlight.
			if (highlightNonBondedAtomIndex.getNum() > 0) {

				// There is something to highlight.  If the highlighting
				// is done by displaying a bitmap and the normal
				// bitmap is NOT the same as the highlight bitmap, then
				// render the highlighted atoms using the normal bitmap.
				// Also, if the highlight style is BBOX, render
				// the highlighted atoms with the normal bitmap.  Then
				// render the highlighted atoms using the highlight bitmap
				// or BBOX.

				if (cdp->highlightAtomStyle.getValue() ==
					ChemDisplayParam::HIGHLIGHT_ATOM_BBOX) {
					renderNormalSingleAtoms(globalNumberOfAtoms,
						highlightNonBondedAtomIndex, chemData, cdp, chemColor);
					renderHighlightedSingleAtoms(globalNumberOfAtoms,
						highlightNonBondedAtomIndex, chemData, cdp, chemColor);
				}
				else {
					if (bitmapsEqual) {
						renderHighlightedSingleAtoms(globalNumberOfAtoms,
							highlightNonBondedAtomIndex, chemData, cdp,
							chemColor);
					}
					else {
						renderNormalSingleAtoms(globalNumberOfAtoms,
							highlightNonBondedAtomIndex, chemData, cdp,
							chemColor);
						renderHighlightedSingleAtoms(globalNumberOfAtoms,
							highlightNonBondedAtomIndex, chemData, cdp,
							chemColor);
					}
				}
			}
		}
		// Restore bitmap alignment
		glPixelStorei(GL_UNPACK_ALIGNMENT, curAlignment);

#ifndef IV2_0
		lazyElt->reset(state, SoLazyElement::DIFFUSE_MASK |
					   SoLazyElement::LIGHT_MODEL_MASK);
#endif
		state->pop();
	}
}

#undef ATOMLOOP_START
#undef ATOMLOOP_END
