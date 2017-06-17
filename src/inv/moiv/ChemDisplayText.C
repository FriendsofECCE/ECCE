/*
 * ChemDisplayText.pp
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

#ident "$Revision: 22364 $"
#include <iostream>
using namespace std;

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
#include "inv/details/SoLineDetail.H"
#include "inv/details/SoPointDetail.H"
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
#include "inv/elements/SoProjectionMatrixElement.H"
#include "inv/elements/SoViewingMatrixElement.H"
#include "inv/elements/SoViewportRegionElement.H"
#include "inv/elements/SoViewVolumeElement.H"
#include "inv/elements/SoGLLazyElement.H"
#include "inv/elements/SoLazyElement.H"


#include "inv/flclient.h"

#include "inv/ChemKit/ChemBitmapFontCache.H"
// --> EGB && SGB && DSA
#include "inv/ChemKit/ChemResidueStuff.H"
// <-- EGB && SGB && DSA
// --> lod
#include "inv/ChemKit/ChemLOD.H"
#include "inv/ChemKit/ChemUnitSphere.H"
#include "inv/ChemKit/ChemUnitCylinder.H"
// <-- lod

#include "inv/ChemKit/ChemData.H"
#include "inv/ChemKit/ChemDetail.H"
#include "inv/ChemKit/ChemDisplay.H"
#include "inv/ChemKit/ChemLabelBBox.H"
#include "inv/ChemKit/ChemBaseDataElement.H"
#include "inv/ChemKit/ChemColorElement.H"
#include "inv/ChemKit/ChemDisplayParamElement.H"
#include "inv/ChemKit/ChemRadiiElement.H"

//
// Start of TEXT section
//

static int numAtomLoops;
static int atomLoop;
static int32_t atomStart;
static int32_t atomEnd;
static int32_t theAtom;

static int numBondLoops;
static int bondLoop;
static int32_t bondStart;
static int32_t bondEnd;
static int32_t theBond;

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
	for (theAtom = atomStart; theAtom < atomEnd; theAtom++) { \

#define ATOMLOOP_END														  \
	}																		  \
}

#define BONDLOOP_START(INDEX)												  \
numBondLoops = (INDEX).getNum();											  \
for (bondLoop = 0; bondLoop < numBondLoops; bondLoop++) {					  \
	(INDEX)[bondLoop].getValue(bondStart, bondEnd);							  \
	if (bondEnd == CHEM_DISPLAY_USE_REST_OF_BONDS) {						  \
	   bondEnd = globalNumberOfBonds;										  \
	}																		  \
	else {																	  \
	   bondEnd += bondStart;												  \
	}																		  \
	for (theBond = bondStart; theBond < bondEnd; theBond++) { \

#define BONDLOOP_END														  \
	}																		  \
}

// --> residue labels
#define RESIDUELOOP_START(INDEX)												  \
numResidueLoops = (INDEX).getNum();      \
for (residueLoop = 0; residueLoop < numResidueLoops; residueLoop++) {	  \
	(INDEX)[residueLoop].getValue(residueStart, residueEnd);		  \
	if (residueEnd == CHEM_DISPLAY_USE_REST_OF_RESIDUES) {	  \
	   residueEnd = globalNumberOfResidues;			  \
	}							  \
	else {			  \
	   residueEnd += residueStart;	  \
	}				  \
	for (theResidue = residueStart; theResidue < residueEnd; theResidue++) {

#define RESIDUELOOP_END	  \
	}		  \
}

static int numResidueLoops;
static int residueLoop;
static int32_t residueStart;
static int32_t residueEnd;
static int32_t theResidue;

// <-- residue labels

// --> labels clipping
bool ChemDisplay::pointInScreen(const SbVec3f &coord) const
{
	SbVec3f tmpCoord, scrCoord;
	currentMVP.multVecMatrix(coord,tmpCoord);
	
	scrCoord[0]= currentView[0] + currentView[2] * (tmpCoord[0] + 1) / 2;
	scrCoord[1] = currentView[1] + currentView[3] * (tmpCoord[1] + 1) / 2;
	//scrCoord[2] = (tmpCoord[2] + 1) / 2;
	
	// clamp values
	if (scrCoord[0] < currentView[0] ||
		scrCoord[0] > currentView[2] ||
		scrCoord[1] < currentView[1] ||
		scrCoord[1] > currentView[3])
		return false;
		//scrCoord[2] <  ||
		//scrCoord[2] > )

	return true;
}

void ChemDisplay::computeAtomLabelsCulling(SoGLRenderAction *action)
{
	int32_t numberOfAtomLabels = getNumberOfAtomLabels(action);
	if (numberOfAtomLabels == 0)
	{
		return;
	}

	SoState *state = action->getState();
	ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);
	ChemBaseData *chemData = ChemBaseDataElement::get(state);

	SbBool doHydrogens = cdp->showHydrogens.getValue();

	SbVec3f		sphereCoords;
	
	SbVec2i vec;
	MFVec2i tIndex;
	bool open=false;

	numAtomLoops = vnormalAtomLabelIndex.getNum();
	for (atomLoop = 0; atomLoop < numAtomLoops; atomLoop++)
	{
		vnormalAtomLabelIndex[atomLoop].getValue(atomStart, atomEnd);
		if (atomEnd == CHEM_DISPLAY_USE_REST_OF_ATOMS) {atomEnd = globalNumberOfAtoms;}
		else {atomEnd += atomStart;}
		for (theAtom = atomStart; theAtom < atomEnd; theAtom++)
		{
			if (!doHydrogens && chemData->getAtomicNumber(theAtom) == 1) continue;
			sphereCoords = chemData->getAtomCoordinates(theAtom);
			if (pointInScreen(sphereCoords))
			{
				if (open)
				{
					vec[1]++;
				}
				else
				{
					vec[0]=theAtom;
					vec[1]=1;
					open=true;
				}
			}
			else
			{
				if (open)
				{
					tIndex.set1Value(tIndex.getNum(),vec);
					open=false;
				}
			}
		}

		if (open)
		{
			tIndex.set1Value(tIndex.getNum(),vec);
			open=false;
		}
	}

	//printf("n %d %d\n",vnormalAtomIndex.count(globalNumberOfAtoms),tIndex.count(globalNumberOfAtoms));

	vnormalAtomIndex.setNum(0);
	if (tIndex.getNum()>0)
		vnormalAtomIndex.setValues(0,tIndex.getNum(),tIndex.getValues(0));
	
	tIndex.setNum(0);
	open=false;

	numAtomLoops = vhighlightAtomLabelIndex.getNum();
	for (atomLoop = 0; atomLoop < numAtomLoops; atomLoop++)
	{
		vhighlightAtomLabelIndex[atomLoop].getValue(atomStart, atomEnd);
		if (atomEnd == CHEM_DISPLAY_USE_REST_OF_ATOMS) {atomEnd = globalNumberOfAtoms;}
		else {atomEnd += atomStart;}
		for (theAtom = atomStart; theAtom < atomEnd; theAtom++)
		{
			if (!doHydrogens && chemData->getAtomicNumber(theAtom) == 1) continue;
			sphereCoords = chemData->getAtomCoordinates(theAtom);
			if (pointInScreen(sphereCoords))
			{
				if (open)
				{
					vec[1]++;
				}
				else
				{
					vec[0]=theAtom;
					vec[1]=1;
					open=true;
				}
			}
			else
			{
				if (open)
				{
					tIndex.set1Value(tIndex.getNum(),vec);
					open=false;
				}
			}
		}

		if (open)
		{
			tIndex.set1Value(tIndex.getNum(),vec);
			open=false;
		}
	}

	//printf("h %d %d\n",vhighlightAtomLabelIndex.count(globalNumberOfAtoms),tIndex.count(globalNumberOfAtoms));

	vhighlightAtomLabelIndex.setNum(0);
	if (tIndex.getNum()>0)
		vhighlightAtomLabelIndex.setValues(0,tIndex.getNum(),tIndex.getValues(0));
}

void ChemDisplay::computeBondLabelsCulling(SoGLRenderAction *action)
{
	int32_t numberOfBondLabels = getNumberOfBondLabels(action);
	if (numberOfBondLabels == 0)
	{
		return;
	}

	SoState *state = action->getState();
	//ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);
	ChemData *chemData = (ChemData*)ChemBaseDataElement::get(state);

	SbVec3f coords;

	SbVec2i vec;
	MFVec2i tIndex;
	bool open=false;

	numBondLoops = vnormalBondLabelIndex.getNum();
	for (bondLoop = 0; bondLoop  < numBondLoops; bondLoop ++)
	{
		vnormalBondLabelIndex[bondLoop].getValue(bondStart, bondEnd);
		if (bondEnd == CHEM_DISPLAY_USE_REST_OF_BONDS) {bondEnd = globalNumberOfBonds;}
		else {bondEnd += bondStart;}
		for (theBond = bondStart; theBond  < bondEnd; theBond ++)
		{
			coords = chemData->getAtomCoordinates(chemData->getBondFrom(theBond))+
				(chemData->getAtomCoordinates(chemData->getBondTo(theBond))-
				chemData->getAtomCoordinates(chemData->getBondFrom(theBond)))*0.5f;

			if (pointInScreen(coords))
			{
				if (open)
				{
					vec[1]++;
				}
				else
				{
					vec[0]=theBond;
					vec[1]=1;
					open=true;
				}
			}
			else
			{
				if (open)
				{
					tIndex.set1Value(tIndex.getNum(),vec);
					open=false;
				}
			}
		}

		if (open)
		{
			tIndex.set1Value(tIndex.getNum(),vec);
			open=false;
		}
	}
	//printf("n %d %d\n",vnormalBondLabelIndex.count(globalNumberOfBonds),tIndex.count(globalNumberOfBonds));

	vnormalBondLabelIndex.setNum(0);
	if (tIndex.getNum()>0)
		vnormalBondLabelIndex.setValues(0,tIndex.getNum(),tIndex.getValues(0));

	tIndex.setNum(0);
	open=false;

	numBondLoops = vhighlightBondLabelIndex.getNum();
	for (bondLoop = 0; bondLoop  < numBondLoops; bondLoop ++)
	{
		vhighlightBondLabelIndex[bondLoop].getValue(bondStart, bondEnd);
		if (bondEnd == CHEM_DISPLAY_USE_REST_OF_BONDS) {bondEnd = globalNumberOfBonds;}
		else {bondEnd += bondStart;}
		for (theBond = bondStart; theBond  < bondEnd; theBond ++)
		{
			coords = chemData->getAtomCoordinates(chemData->getBondFrom(theBond))+
				(chemData->getAtomCoordinates(chemData->getBondTo(theBond))-
				chemData->getAtomCoordinates(chemData->getBondFrom(theBond)))*0.5f;

			if (pointInScreen(coords))
			{
				if (open)
				{
					vec[1]++;
				}
				else
				{
					vec[0]=theBond;
					vec[1]=1;
					open=true;
				}
			}
			else
			{
				if (open)
				{
					tIndex.set1Value(tIndex.getNum(),vec);
					open=false;
				}
			}
		}

		if (open)
		{
			tIndex.set1Value(tIndex.getNum(),vec);
			open=false;
		}
	}

	//printf("h %d %d\n",vhighlightBondLabelIndex.count(globalNumberOfBonds),tIndex.count(globalNumberOfBonds));

	vhighlightBondLabelIndex.setNum(0);
	if (tIndex.getNum()>0)
		vhighlightBondLabelIndex.setValues(0,tIndex.getNum(),tIndex.getValues(0));
}

void ChemDisplay::computeResidueLabelsCulling(SoGLRenderAction *action)
{
	int32_t numberOfResidueLabels = getNumberOfResidueLabels(action);
	if (numberOfResidueLabels == 0)
	{
		return;
	}

	SoState *state = action->getState();
	//ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);
	//ChemRadii *chemRadii = ChemRadiiElement::get(state);
	ChemData *chemData = (ChemData*)ChemBaseDataElement::get(state);

	SbVec3f		coord;

	SbVec2i vec;
	MFVec2i tIndex;
	bool open=false;

	numResidueLoops = vnormalResidueLabelIndex.getNum();
	for (residueLoop = 0; residueLoop < numResidueLoops; residueLoop++)
	{
		vnormalResidueLabelIndex[residueLoop].getValue(residueStart, residueEnd);
		if (residueEnd == CHEM_DISPLAY_USE_REST_OF_RESIDUES) {residueEnd = globalNumberOfResidues;}
		else {residueEnd += residueStart;}
		for (theResidue = residueStart; theResidue  < residueEnd; theResidue ++)
		{
			if (chemData->residues[theResidue].checkControlPoints())
				coord = chemData->getAtomCoordinates(chemData->residues[theResidue][SbResidue::RESIDUE_CONTROL_POINT_CA]);
			else
			{
				if (!chemData->residues[theResidue].computeCenterOfMass(chemData,coord))
					continue;
			}

			if (pointInScreen(coord))
			{
				if (open)
				{
					vec[1]++;
				}
				else
				{
					vec[0]=theResidue;
					vec[1]=1;
					open=true;
				}
			}
			else
			{
				if (open)
				{
					tIndex.set1Value(tIndex.getNum(),vec);
					open=false;
				}
			}
		}

		if (open)
		{
			tIndex.set1Value(tIndex.getNum(),vec);
			open=false;
		}
	}

	//printf("n %d %d\n",vnormalResidueLabelIndex.count(globalNumberOfResidues),tIndex.count(globalNumberOfResidues));

	vnormalResidueLabelIndex.setNum(0);
	if (tIndex.getNum()>0)
		vnormalResidueLabelIndex.setValues(0,tIndex.getNum(),tIndex.getValues(0));

	open=false;
	tIndex.setNum(0);

	numResidueLoops = vhighlightResidueLabelIndex.getNum();
	for (residueLoop = 0; residueLoop < numResidueLoops; residueLoop++)
	{
		vhighlightResidueLabelIndex[residueLoop].getValue(residueStart, residueEnd);
		if (residueEnd == CHEM_DISPLAY_USE_REST_OF_RESIDUES) {residueEnd = globalNumberOfResidues;}
		else {residueEnd += residueStart;}
		for (theResidue = residueStart; theResidue  < residueEnd; theResidue ++)
		{
			if (chemData->residues[theResidue].checkControlPoints())
				coord = chemData->getAtomCoordinates(chemData->residues[theResidue][SbResidue::RESIDUE_CONTROL_POINT_CA]);
			else
			{
				if (!chemData->residues[theResidue].computeCenterOfMass(chemData,coord))
					continue;
			}

			if (pointInScreen(coord))
			{
				if (open)
				{
					vec[1]++;
				}
				else
				{
					vec[0]=theResidue;
					vec[1]=1;
					open=true;
				}
			}
			else
			{
				if (open)
				{
					tIndex.set1Value(tIndex.getNum(),vec);
					open=false;
				}
			}
		}

		if (open)
		{
			tIndex.set1Value(tIndex.getNum(),vec);
			open=false;
		}
	}

	//printf("h %d %d\n",vhighlightResidueLabelIndex.count(globalNumberOfResidues),tIndex.count(globalNumberOfResidues));

	vhighlightResidueLabelIndex.setNum(0);
	if (tIndex.getNum()>0)
		vhighlightResidueLabelIndex.setValues(0,tIndex.getNum(),tIndex.getValues(0));
}
// <-- labels clipping

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
//	  Static helper routine; given a vector in object space, a
//	  transformation matrix to screen (normalized-device coordinate)
//	  space, and a viewportRegion, returns the 2D pixel coordinates of
//	  the vector (relative to the origin of the viewport).	This
//	  actually returns a 3D vector; the z value is just the NDC z value.
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
	result[1] = (ndc[1]+1.0f)*vpSize[1]/2.0f;
	// Leave the z coordinate alone
	result[2] = ndc[2];

	return result;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Static helper routine; given 2D pixel coordinates, a
//	  transformation matrix to object space, and a viewportRegion,
//	  return the object-space point (assuming the 2D pixel is at z
//	  coordinate 0).
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
	ndc[0] = pixel[0]*2.0f/vpSize[0] - 1.0f;
	ndc[1] = pixel[1]*2.0f/vpSize[1] - 1.0f;
	ndc[2] = pixel[2];

	SbVec3f result;
	matrix.multVecMatrix(ndc, result);

	return result;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Returns the number of pixels the given line is offset from the
//	  text origin.
//
// Use: static

static SbVec3f
getPixelStringOffset(ChemBitmapFontCache *theFont,
	short lrJust, short tbJust, SbString label)
//
////////////////////////////////////////////////////////////////////////
{
	float offset;
	SbVec3f result(0,0,0);

	if (lrJust == ChemDisplayParam::LABEL_LR_RIGHT) {
		offset = theFont->getWidth(label);
		result[0] = -offset;
	}
	else if (lrJust == ChemDisplayParam::LABEL_LR_CENTER) {
		offset = theFont->getWidth(label);
		result[0] = -offset * 0.5;
	}

	if (tbJust == ChemDisplayParam::LABEL_TB_TOP) {
		offset = theFont->getHeight();
		result[1] = -offset;
	}
	else if (tbJust == ChemDisplayParam::LABEL_TB_MIDDLE) {
		offset = theFont->getHeight();
		result[1] = -offset * 0.5;
	}

	return result;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Set up the font to use for rendering
//
// Use: static

static SbBool
getFontCache(SoGLRenderAction *action, SoState *state,
    SbName name, float size,
	ChemBitmapFontCache *&theFont, SbBool forRender)
//
////////////////////////////////////////////////////////////////////////
{
	SbBool validFont;
	if (theFont != NULL) {
		validFont =
			(forRender ? theFont->isRenderValid(action, state, name, size) :
						 theFont->isValid(state, name, size));
		if (!validFont) {
			theFont->unref();
			theFont = NULL;
		}
	}
	if (theFont == NULL) {
		theFont =
			ChemBitmapFontCache::getFont(action, state, name, size, forRender);
		if (theFont == NULL) {
			return FALSE;
		}
	}
	return TRUE;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Given the label format, get the information from a
//    ChemBaseData-derived node and generate a label.   
//
// Use: static

static void
parseAtomLabel(const char *labelFormat, const int32_t theAtom,
	const ChemBaseData *chemData, SbString &label)	 
//
////////////////////////////////////////////////////////////////////////
{
	SbString name;
	char ch;
	const char *lf2;
	char tmpStr[2];
	tmpStr[1] = '\0';

	label.makeEmpty();
	lf2 = labelFormat;
	int id;
	char buf[32];
	while ((ch = *lf2++)) {
		if (ch == '%') {
			ch = *lf2++;
			if (isupper(ch)) {	 
				ch = tolower(ch);
			}
			switch(ch) {
			  case 'a':
				name = chemData->getAtomName(theAtom);
				label += name.getString();
				break;
			  case 'i':
				id = chemData->getAtomId(theAtom);
				sprintf(buf, "%d", id);
				label += buf;
				break;
			  case 'e':
				id = chemData->getAtomicNumber(theAtom);
				sprintf(buf, "%d", id);
				label += buf;
				break;
			  case '%':
				label += "%";
				break;
			  default:
				break;
			}
		}
		else if ((ch >= ' ') && (ch <= '~')) {
			tmpStr[0] = ch;
			label += tmpStr;
		}
	}
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Given the label format, get the information from a
//    ChemBaseData-derived node and generate a label.
//
// Use: static

static void
parseBondLabel(const char *labelFormat, const int32_t theBond,
	const int32_t from, const int32_t to,
	const SbVec3f &coords1, const SbVec3f &coords2,
	const ChemBaseData *chemData, SbString &label)
//
////////////////////////////////////////////////////////////////////////
{
	SbString name;
	char ch;
	const char *lf2;
	char tmpStr[2];   
	tmpStr[1] = '\0';  

	label.makeEmpty();
	lf2 = labelFormat;
	int id;
	char buf[32];
	while ((ch = *lf2++)) {
		if (ch == '%') {
			ch = *lf2++;
			if (isupper(ch)) {
				ch = tolower(ch);
			}
			switch(ch) {
			  case 'd':
				sprintf(buf, "%-6.3f", (coords2 - coords1).length());
				label += buf;
				break;
			  case 'i':
				sprintf(buf, "%d", theBond+1);
				label += buf;
				break;
			  case 'f':
				ch = *lf2++;
				if (isupper(ch)) {
					ch = tolower(ch);
				}
				switch(ch) {
				  case 'a':
					name = chemData->getAtomName(from);
					label += name.getString();
					break;
				  case 'i':
					id = chemData->getAtomId(from);
					sprintf(buf, "%d", id);
					label += buf;
					break;
				  case 'e':
					id = chemData->getAtomicNumber(from);
					sprintf(buf, "%d", id);
					label += buf;
					break;
				  default:
					break;
				}	  
				break; 
			  case 't':
				ch = *lf2++;
				if (isupper(ch)) {
					ch = tolower(ch);
				}
				switch(ch) {
				  case 'a':
					name = chemData->getAtomName(to);
					label += name.getString();
					break;
				  case 'i':
					id = chemData->getAtomId(to);
					sprintf(buf, "%d", id);
					label += buf;
					break;	 
				  case 'e':
					id = chemData->getAtomicNumber(to);
					sprintf(buf, "%d", id);   
					label += buf;
					break;
				  default:
					break;	
				}
				break;
			  case '%':
				label += "%";
				break;
			  default:
				break;
			}
		}
		else if ((ch >= ' ') && (ch <= '~')) {
			tmpStr[0] = ch;
			label += tmpStr;
		}
	}
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Setup the state rendering
//
// Use: static

static void
beginRenderState(SoState *state, GLint *curAlignment)
//
////////////////////////////////////////////////////////////////////////
{
	state->push();

	// Turn off lighting  
	SoGLLazyElement::setLightModel(state, SoLazyElement::BASE_COLOR);
	SoGLLazyElement::setColorMaterial(state, TRUE);
	SoGLLazyElement *lazyElt = (SoGLLazyElement *)
			SoLazyElement::getInstance(state);
	lazyElt->send(state, SoLazyElement::ALL_MASK);
	// --> label error lighting
	glPushAttrib(GL_LIGHTING_BIT);
	glDisable(GL_LIGHTING);
	// <-- label error lighting

	// Turn off texturing
	SoGLTextureEnabledElement::set(state, FALSE);

	// Get the current pixel alignment
	glGetIntegerv(GL_UNPACK_ALIGNMENT, curAlignment);

	// Set the pixel alignment
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Finished with rendering so restore state.
//
// Use: static

static void
endRenderState(SoState *state, GLint curAlignment)
//
////////////////////////////////////////////////////////////////////////
{
	// Reset the pixel alignment
	glPixelStorei(GL_UNPACK_ALIGNMENT, curAlignment);

	SoGLLazyElement::setColorMaterial(state, FALSE);
	((SoGLLazyElement *)SoLazyElement::getInstance(state))->
		reset(state, SoLazyElement::DIFFUSE_MASK);
// --> label error lighting
	glPopAttrib(); // pop GL_LIGHTING_BIT
// <-- label error lighting

	state->pop();
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Compute bounding box of atom labels when rendering atoms as spheres.
//
// Use: protected

void
ChemDisplay::bBoxAtomLabelsWithSpheres(SoState *state, ChemDisplayParam *cdp,
	int32_t &count, SbBox3f &box, SbVec3f &center)
//
////////////////////////////////////////////////////////////////////////
{
	ChemRadii *chemRadii = ChemRadiiElement::get(state);

	switch (chemRadii->atomRadiiBinding.getValue()) {
	  case ChemRadii::RADII_OVERALL:
		bBoxAtomLabelsWithSpheresRO(state, cdp, count, box, center);
		break;
	  case ChemRadii::RADII_PER_ATOM:
		bBoxAtomLabelsWithSpheresRA(state, cdp, count, box, center);
		break;
	  case ChemRadii::RADII_PER_ATOM_INDEXED:
		bBoxAtomLabelsWithSpheresRI(state, cdp, count, box, center);
		break;
	  default:
		break;
	}
}




////////////////////////////////////////////////////////////////////////
//
// Description:
//    Computes bounding box of text. Since 2D text is screen-aligned,
//    we need to look at the current view volume and viewport region.
//
// Use: protected

void
ChemDisplay::
bBoxAtomLabelsWithWireframe
       (SoState *state, ChemDisplayParam *cdp,
	       int32_t &count, SbBox3f &box, SbVec3f &center)
//
////////////////////////////////////////////////////////////////////////
{
       ChemBaseData *chemData = ChemBaseDataElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(NULL, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, FALSE)) {
	       return;
       }

       SbMatrix objToScreen;
       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));

       SbViewportRegion vpr = SoViewportRegionElement::get(state);

       SbMatrix screenToObj = objToScreen.inverse();

       // Figure out the screen-space bounding box of the characters:
       SbBox3f screenBbox, charBbox;

       //SbMatrix   theMatrix;
       //SbVec3f    theScale;
       //SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();
       SbString label;
       SbString name;
       ATOMLOOP_START(atomLabelIndex)
	       if (!doHydrogens && chemData->getAtomicNumber(theAtom) == 1) {
		       continue;
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       // The origin of the text on the screen is the object-space point
	       // coord
	       SbVec3f screenOrigin =
		       fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);

	       // Starting position of string, based on justification:
	       SbVec3f charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       const char *chars = label.getString();
	       screenBbox.makeEmpty();

	       for (int chr = 0; chr < label.getLength(); chr++) {
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
		       count++;
	       }
       ATOMLOOP_END
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Computes bounding box of text. Since 2D text is screen-aligned,
//    we need to look at the current view volume and viewport region.
//
// Use: protected

void
ChemDisplay::
bBoxAtomLabelsWithCylinders
       (SoState *state, ChemDisplayParam *cdp,
	       int32_t &count, SbBox3f &box, SbVec3f &center)
//
////////////////////////////////////////////////////////////////////////
{
       ChemBaseData *chemData = ChemBaseDataElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(NULL, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, FALSE)) {
	       return;
       }

       SbMatrix objToScreen;
       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));

       SbViewportRegion vpr = SoViewportRegionElement::get(state);

       SbMatrix screenToObj = objToScreen.inverse();

       // Figure out the screen-space bounding box of the characters:
       SbBox3f screenBbox, charBbox;

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();
       float rad = cdp->bondCylinderRadius.getValue();
       SbString label;
       SbString name;
       ATOMLOOP_START(atomLabelIndex)
	       if (!doHydrogens && chemData->getAtomicNumber(theAtom) == 1) {
		       continue;
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       theScale.setValue(rad, rad, rad);
	       theMatrix.setTransform(coord, theRotation, theScale);
	       theMatrix.multVecMatrix(theZAxis, coord);
	       // The origin of the text on the screen is the object-space point
	       // coord
	       SbVec3f screenOrigin =
		       fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);

	       // Starting position of string, based on justification:
	       SbVec3f charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       const char *chars = label.getString();
	       screenBbox.makeEmpty();

	       for (int chr = 0; chr < label.getLength(); chr++) {
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
		       count++;
	       }
       ATOMLOOP_END
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Computes bounding box of text. Since 2D text is screen-aligned,
//    we need to look at the current view volume and viewport region.
//
// Use: protected

void
ChemDisplay::
bBoxAtomLabelsWithSpheresRO
       (SoState *state, ChemDisplayParam *cdp,
	       int32_t &count, SbBox3f &box, SbVec3f &center)
//
////////////////////////////////////////////////////////////////////////
{
       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(NULL, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, FALSE)) {
	       return;
       }

       SbMatrix objToScreen;
       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));

       SbViewportRegion vpr = SoViewportRegionElement::get(state);

       SbMatrix screenToObj = objToScreen.inverse();

       // Figure out the screen-space bounding box of the characters:
       SbBox3f screenBbox, charBbox;

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();
       float rad;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }
       SbString label;
       SbString name;
       rad = chemRadii->atomRadii[0] * radScale;
       ATOMLOOP_START(atomLabelIndex)
	       if (!doHydrogens && chemData->getAtomicNumber(theAtom) == 1) {
		       continue;
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       theScale.setValue(rad, rad, rad);
	       theMatrix.setTransform(coord, theRotation, theScale);
	       theMatrix.multVecMatrix(theZAxis, coord);
	       // The origin of the text on the screen is the object-space point
	       // coord
	       SbVec3f screenOrigin =
		       fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);

	       // Starting position of string, based on justification:
	       SbVec3f charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       const char *chars = label.getString();
	       screenBbox.makeEmpty();

	       for (int chr = 0; chr < label.getLength(); chr++) {
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
		       count++;
	       }
       ATOMLOOP_END
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Computes bounding box of text. Since 2D text is screen-aligned,
//    we need to look at the current view volume and viewport region.
//
// Use: protected

void
ChemDisplay::
bBoxAtomLabelsWithSpheresRA
       (SoState *state, ChemDisplayParam *cdp,
	       int32_t &count, SbBox3f &box, SbVec3f &center)
//
////////////////////////////////////////////////////////////////////////
{
       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(NULL, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, FALSE)) {
	       return;
       }

       SbMatrix objToScreen;
       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));

       SbViewportRegion vpr = SoViewportRegionElement::get(state);

       SbMatrix screenToObj = objToScreen.inverse();

       // Figure out the screen-space bounding box of the characters:
       SbBox3f screenBbox, charBbox;

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();
       float rad;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }
       SbString label;
       SbString name;
       ATOMLOOP_START(atomLabelIndex)
	       if (!doHydrogens && chemData->getAtomicNumber(theAtom) == 1) {
		       continue;
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       rad = chemRadii->atomRadii[theAtom];
	       rad *= radScale;
	       theScale.setValue(rad, rad, rad);
	       theMatrix.setTransform(coord, theRotation, theScale);
	       theMatrix.multVecMatrix(theZAxis, coord);
	       // The origin of the text on the screen is the object-space point
	       // coord
	       SbVec3f screenOrigin =
		       fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);

	       // Starting position of string, based on justification:
	       SbVec3f charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       const char *chars = label.getString();
	       screenBbox.makeEmpty();

	       for (int chr = 0; chr < label.getLength(); chr++) {
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
		       count++;
	       }
       ATOMLOOP_END
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Computes bounding box of text. Since 2D text is screen-aligned,
//    we need to look at the current view volume and viewport region.
//
// Use: protected

void
ChemDisplay::
bBoxAtomLabelsWithSpheresRI
       (SoState *state, ChemDisplayParam *cdp,
	       int32_t &count, SbBox3f &box, SbVec3f &center)
//
////////////////////////////////////////////////////////////////////////
{
       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(NULL, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, FALSE)) {
	       return;
       }

       SbMatrix objToScreen;
       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));

       SbViewportRegion vpr = SoViewportRegionElement::get(state);

       SbMatrix screenToObj = objToScreen.inverse();

       // Figure out the screen-space bounding box of the characters:
       SbBox3f screenBbox, charBbox;

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();
       float rad;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }
       SbString label;
       SbString name;
       int32_t localAtomIndex;
       ATOMLOOP_START(atomLabelIndex)
	       if (!doHydrogens && chemData->getAtomicNumber(theAtom) == 1) {
		       continue;
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       localAtomIndex = chemData->getAtomIndex(theAtom);
	       rad = chemRadii->atomRadii[localAtomIndex];
	       rad *= radScale;
	       theScale.setValue(rad, rad, rad);
	       theMatrix.setTransform(coord, theRotation, theScale);
	       theMatrix.multVecMatrix(theZAxis, coord);
	       // The origin of the text on the screen is the object-space point
	       // coord
	       SbVec3f screenOrigin =
		       fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);

	       // Starting position of string, based on justification:
	       SbVec3f charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       const char *chars = label.getString();
	       screenBbox.makeEmpty();

	       for (int chr = 0; chr < label.getLength(); chr++) {
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
		       count++;
	       }
       ATOMLOOP_END
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Computes bounding box of text. Since 2D text is screen-aligned,
//    we need to look at the current view volume and viewport region.
//
// Use: protected

void
ChemDisplay::
bBoxBondLabelsWithWireframe
       (SoState *state, ChemDisplayParam *cdp,
	       int32_t &count, SbBox3f &box, SbVec3f &center)
//
////////////////////////////////////////////////////////////////////////
{
       ChemBaseData *chemData = ChemBaseDataElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(NULL, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, FALSE)) {
	       return;
       }

       SbMatrix objToScreen;
       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));

       SbViewportRegion vpr = SoViewportRegionElement::get(state);

       SbMatrix screenToObj = objToScreen.inverse();

       // Figure out the screen-space bounding box of the characters:
       SbBox3f screenBbox, charBbox;

       //SbMatrix   theMatrix;
       //SbVec3f    theScale;
       //SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->bondLabelString.getValue().getString();
       short lrJust = cdp->bondLabelLeftRightJustification.getValue();
       short tbJust = cdp->bondLabelTopBottomJustification.getValue();
       SbString label;
       SbString name;
       BONDLOOP_START(bondLabelIndex)
	       int32_t from = chemData->getBondFrom(theBond);
	       int32_t to = chemData->getBondTo(theBond);
	       if (!doHydrogens) {
		       if ((chemData->getAtomicNumber(from) == 1) ||
			       (chemData->getAtomicNumber(to) == 1)) continue;
	       }
	       SbVec3f coords1 = chemData->getAtomCoordinates(from).getValue();
	       SbVec3f coords2 = chemData->getAtomCoordinates(to).getValue();
	       coord = (coords2 - coords1) * 0.5f + coords1;
	       // The origin of the text on the screen is the object-space point
	       // coord
	       SbVec3f screenOrigin =
		       fromObjectSpace(coord, objToScreen, vpr);

	       parseBondLabel(labelFormat, theBond, from, to, coords1, coords2,
		       chemData, label);

	       // Starting position of string, based on justification:
	       SbVec3f charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       const char *chars = label.getString();
	       screenBbox.makeEmpty();

	       for (int chr = 0; chr < label.getLength(); chr++) {
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
		       count++;
	       }
       BONDLOOP_END
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Computes bounding box of text. Since 2D text is screen-aligned,
//    we need to look at the current view volume and viewport region.
//
// Use: protected

void
ChemDisplay::
bBoxBondLabelsWithCylinders
       (SoState *state, ChemDisplayParam *cdp,
	       int32_t &count, SbBox3f &box, SbVec3f &center)
//
////////////////////////////////////////////////////////////////////////
{
       ChemBaseData *chemData = ChemBaseDataElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(NULL, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, FALSE)) {
	       return;
       }

       SbMatrix objToScreen;
       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));

       SbViewportRegion vpr = SoViewportRegionElement::get(state);

       SbMatrix screenToObj = objToScreen.inverse();

       // Figure out the screen-space bounding box of the characters:
       SbBox3f screenBbox, charBbox;

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->bondLabelString.getValue().getString();
       short lrJust = cdp->bondLabelLeftRightJustification.getValue();
       short tbJust = cdp->bondLabelTopBottomJustification.getValue();
       float rad = cdp->bondCylinderRadius.getValue();
       SbString label;
       SbString name;
       BONDLOOP_START(bondLabelIndex)
	       int32_t from = chemData->getBondFrom(theBond);
	       int32_t to = chemData->getBondTo(theBond);
	       if (!doHydrogens) {
		       if ((chemData->getAtomicNumber(from) == 1) ||
			       (chemData->getAtomicNumber(to) == 1)) continue;
	       }
	       SbVec3f coords1 = chemData->getAtomCoordinates(from).getValue();
	       SbVec3f coords2 = chemData->getAtomCoordinates(to).getValue();
	       coord = (coords2 - coords1) * 0.5f + coords1;
	       theScale.setValue(rad, rad, rad);
	       theMatrix.setTransform(coord, theRotation, theScale);
	       theMatrix.multVecMatrix(theZAxis, coord);
	       // The origin of the text on the screen is the object-space point
	       // coord
	       SbVec3f screenOrigin =
		       fromObjectSpace(coord, objToScreen, vpr);

	       parseBondLabel(labelFormat, theBond, from, to, coords1, coords2,
		       chemData, label);

	       // Starting position of string, based on justification:
	       SbVec3f charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       const char *chars = label.getString();
	       screenBbox.makeEmpty();

	       for (int chr = 0; chr < label.getLength(); chr++) {
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
		       count++;
	       }
       BONDLOOP_END
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Computes bounding box of text. Since 2D text is screen-aligned,
//    we need to look at the current view volume and viewport region.
//
// Use: protected

void
ChemDisplay::
eachBBoxAtomLabelsWithWireframe
       (SoState *state, ChemDisplayParam *cdp,
	       SbBool clipOnCenter, int32_t &bCount, ChemLabelBBox *labelBBoxes)
//
////////////////////////////////////////////////////////////////////////
{
       ChemBaseData *chemData = ChemBaseDataElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(NULL, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, FALSE)) {
	       return;
       }

       SbMatrix objToScreen;
       objToScreen = currentMVP;
       SbViewportRegion vpr = currentVPR;
       const SoClipPlaneElement *clipPlaneElt =
	       SoClipPlaneElement::getInstance(state);

       SbMatrix screenToObj = objToScreen.inverse();

       // Figure out the screen-space bounding box of the characters:
       SbBox3f screenBbox, charBbox;

       //SbMatrix   theMatrix;
       //SbVec3f    theScale;
       //SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();
       SbString label;
       SbString name;
       bCount = 0;
       ATOMLOOP_START(atomLabelIndex)
	       if (!doHydrogens && chemData->getAtomicNumber(theAtom) == 1) {
		       continue;
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       // The origin of the text on the screen is the object-space point
	       // coord
	       SbVec3f screenOrigin =
		       fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);

	       // Starting position of string, based on justification:
	       SbVec3f charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       const char *chars = label.getString();
	       screenBbox.makeEmpty();

	       for (int chr = 0; chr < label.getLength(); chr++) {
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
		       SbVec3f tempMin, tempMax;
		       SbVec3f p0, p1;

		       tempMin.setValue(min[0], min[1], screenOrigin[2]);
		       tempMax.setValue(max[0], max[1], screenOrigin[2]);
		       tempMin = toObjectSpace(tempMin, screenToObj, vpr);
		       tempMax = toObjectSpace(tempMax, screenToObj, vpr);

		       if (clipOnCenter) {
			       // Check to see if the center is between clipping planes.
			       // If not, the don't increment bCount.
			       // The check to see if the center is between the clipping planes
			       // needs to be performed in world space
			       // (see SoRayPickAction::isBetweenPlanes) so multiply by the
			       // currentModelMatrix
			       p0 = (tempMin + tempMax) * 0.5f;
			       currentModelMatrix.multVecMatrix(p0, p1);
			       if (isPointBetweenPlanes(p1, currentViewVolume, clipPlaneElt)) {
				       labelBBoxes->bbox[bCount].setBounds(tempMin, tempMax);
				       labelBBoxes->center[bCount] = p0;
				       labelBBoxes->index[bCount] = theAtom;
				       bCount++;
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
				       labelBBoxes->bbox[bCount].setBounds(tempMin, tempMax);
				       labelBBoxes->center[bCount] = (tempMin + tempMax) * 0.5f;
				       labelBBoxes->index[bCount] = theAtom;
				       bCount++;
			       }
		       }
	       }
       ATOMLOOP_END
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Computes bounding box of text. Since 2D text is screen-aligned,
//    we need to look at the current view volume and viewport region.
//
// Use: protected

void
ChemDisplay::
eachBBoxAtomLabelsWithCylinders
       (SoState *state, ChemDisplayParam *cdp,
	       SbBool clipOnCenter, int32_t &bCount, ChemLabelBBox *labelBBoxes)
//
////////////////////////////////////////////////////////////////////////
{
       ChemBaseData *chemData = ChemBaseDataElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(NULL, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, FALSE)) {
	       return;
       }

       SbMatrix objToScreen;
       objToScreen = currentMVP;
       SbViewportRegion vpr = currentVPR;
       const SoClipPlaneElement *clipPlaneElt =
	       SoClipPlaneElement::getInstance(state);

       SbMatrix screenToObj = objToScreen.inverse();

       // Figure out the screen-space bounding box of the characters:
       SbBox3f screenBbox, charBbox;

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();
       float rad = cdp->bondCylinderRadius.getValue();
       SbString label;
       SbString name;
       bCount = 0;
       ATOMLOOP_START(atomLabelIndex)
	       if (!doHydrogens && chemData->getAtomicNumber(theAtom) == 1) {
		       continue;
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       theScale.setValue(rad, rad, rad);
	       theMatrix.setTransform(coord, theRotation, theScale);
	       theMatrix.multVecMatrix(theZAxis, coord);
	       // The origin of the text on the screen is the object-space point
	       // coord
	       SbVec3f screenOrigin =
		       fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);

	       // Starting position of string, based on justification:
	       SbVec3f charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       const char *chars = label.getString();
	       screenBbox.makeEmpty();

	       for (int chr = 0; chr < label.getLength(); chr++) {
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
		       SbVec3f tempMin, tempMax;
		       SbVec3f p0, p1;

		       tempMin.setValue(min[0], min[1], screenOrigin[2]);
		       tempMax.setValue(max[0], max[1], screenOrigin[2]);
		       tempMin = toObjectSpace(tempMin, screenToObj, vpr);
		       tempMax = toObjectSpace(tempMax, screenToObj, vpr);

		       if (clipOnCenter) {
			       // Check to see if the center is between clipping planes.
			       // If not, the don't increment bCount.
			       // The check to see if the center is between the clipping planes
			       // needs to be performed in world space
			       // (see SoRayPickAction::isBetweenPlanes) so multiply by the
			       // currentModelMatrix
			       p0 = (tempMin + tempMax) * 0.5f;
			       currentModelMatrix.multVecMatrix(p0, p1);
			       if (isPointBetweenPlanes(p1, currentViewVolume, clipPlaneElt)) {
				       labelBBoxes->bbox[bCount].setBounds(tempMin, tempMax);
				       labelBBoxes->center[bCount] = p0;
				       labelBBoxes->index[bCount] = theAtom;
				       bCount++;
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
				       labelBBoxes->bbox[bCount].setBounds(tempMin, tempMax);
				       labelBBoxes->center[bCount] = (tempMin + tempMax) * 0.5f;
				       labelBBoxes->index[bCount] = theAtom;
				       bCount++;
			       }
		       }
	       }
       ATOMLOOP_END
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Computes bounding box of text. Since 2D text is screen-aligned,
//    we need to look at the current view volume and viewport region.
//
// Use: protected

void
ChemDisplay::
eachBBoxAtomLabelsWithSpheresRO
       (SoState *state, ChemDisplayParam *cdp,
	       SbBool clipOnCenter, int32_t &bCount, ChemLabelBBox *labelBBoxes)
//
////////////////////////////////////////////////////////////////////////
{
       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(NULL, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, FALSE)) {
	       return;
       }

       SbMatrix objToScreen;
       objToScreen = currentMVP;
       SbViewportRegion vpr = currentVPR;
       const SoClipPlaneElement *clipPlaneElt =
	       SoClipPlaneElement::getInstance(state);

       SbMatrix screenToObj = objToScreen.inverse();

       // Figure out the screen-space bounding box of the characters:
       SbBox3f screenBbox, charBbox;

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();
       float rad;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }
       SbString label;
       SbString name;
       bCount = 0;
       rad = chemRadii->atomRadii[0] * radScale;
       ATOMLOOP_START(atomLabelIndex)
	       if (!doHydrogens && chemData->getAtomicNumber(theAtom) == 1) {
		       continue;
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       theScale.setValue(rad, rad, rad);
	       theMatrix.setTransform(coord, theRotation, theScale);
	       theMatrix.multVecMatrix(theZAxis, coord);
	       // The origin of the text on the screen is the object-space point
	       // coord
	       SbVec3f screenOrigin =
		       fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);

	       // Starting position of string, based on justification:
	       SbVec3f charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       const char *chars = label.getString();
	       screenBbox.makeEmpty();

	       for (int chr = 0; chr < label.getLength(); chr++) {
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
		       SbVec3f tempMin, tempMax;
		       SbVec3f p0, p1;

		       tempMin.setValue(min[0], min[1], screenOrigin[2]);
		       tempMax.setValue(max[0], max[1], screenOrigin[2]);
		       tempMin = toObjectSpace(tempMin, screenToObj, vpr);
		       tempMax = toObjectSpace(tempMax, screenToObj, vpr);

		       if (clipOnCenter) {
			       // Check to see if the center is between clipping planes.
			       // If not, the don't increment bCount.
			       // The check to see if the center is between the clipping planes
			       // needs to be performed in world space
			       // (see SoRayPickAction::isBetweenPlanes) so multiply by the
			       // currentModelMatrix
			       p0 = (tempMin + tempMax) * 0.5f;
			       currentModelMatrix.multVecMatrix(p0, p1);
			       if (isPointBetweenPlanes(p1, currentViewVolume, clipPlaneElt)) {
				       labelBBoxes->bbox[bCount].setBounds(tempMin, tempMax);
				       labelBBoxes->center[bCount] = p0;
				       labelBBoxes->index[bCount] = theAtom;
				       bCount++;
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
				       labelBBoxes->bbox[bCount].setBounds(tempMin, tempMax);
				       labelBBoxes->center[bCount] = (tempMin + tempMax) * 0.5f;
				       labelBBoxes->index[bCount] = theAtom;
				       bCount++;
			       }
		       }
	       }
       ATOMLOOP_END
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Computes bounding box of text. Since 2D text is screen-aligned,
//    we need to look at the current view volume and viewport region.
//
// Use: protected

void
ChemDisplay::
eachBBoxAtomLabelsWithSpheresRA
       (SoState *state, ChemDisplayParam *cdp,
	       SbBool clipOnCenter, int32_t &bCount, ChemLabelBBox *labelBBoxes)
//
////////////////////////////////////////////////////////////////////////
{
       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(NULL, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, FALSE)) {
	       return;
       }

       SbMatrix objToScreen;
       objToScreen = currentMVP;
       SbViewportRegion vpr = currentVPR;
       const SoClipPlaneElement *clipPlaneElt =
	       SoClipPlaneElement::getInstance(state);

       SbMatrix screenToObj = objToScreen.inverse();

       // Figure out the screen-space bounding box of the characters:
       SbBox3f screenBbox, charBbox;

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();
       float rad;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }
       SbString label;
       SbString name;
       bCount = 0;
       ATOMLOOP_START(atomLabelIndex)
	       if (!doHydrogens && chemData->getAtomicNumber(theAtom) == 1) {
		       continue;
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       rad = chemRadii->atomRadii[theAtom];
	       rad *= radScale;
	       theScale.setValue(rad, rad, rad);
	       theMatrix.setTransform(coord, theRotation, theScale);
	       theMatrix.multVecMatrix(theZAxis, coord);
	       // The origin of the text on the screen is the object-space point
	       // coord
	       SbVec3f screenOrigin =
		       fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);

	       // Starting position of string, based on justification:
	       SbVec3f charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       const char *chars = label.getString();
	       screenBbox.makeEmpty();

	       for (int chr = 0; chr < label.getLength(); chr++) {
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
		       SbVec3f tempMin, tempMax;
		       SbVec3f p0, p1;

		       tempMin.setValue(min[0], min[1], screenOrigin[2]);
		       tempMax.setValue(max[0], max[1], screenOrigin[2]);
		       tempMin = toObjectSpace(tempMin, screenToObj, vpr);
		       tempMax = toObjectSpace(tempMax, screenToObj, vpr);

		       if (clipOnCenter) {
			       // Check to see if the center is between clipping planes.
			       // If not, the don't increment bCount.
			       // The check to see if the center is between the clipping planes
			       // needs to be performed in world space
			       // (see SoRayPickAction::isBetweenPlanes) so multiply by the
			       // currentModelMatrix
			       p0 = (tempMin + tempMax) * 0.5f;
			       currentModelMatrix.multVecMatrix(p0, p1);
			       if (isPointBetweenPlanes(p1, currentViewVolume, clipPlaneElt)) {
				       labelBBoxes->bbox[bCount].setBounds(tempMin, tempMax);
				       labelBBoxes->center[bCount] = p0;
				       labelBBoxes->index[bCount] = theAtom;
				       bCount++;
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
				       labelBBoxes->bbox[bCount].setBounds(tempMin, tempMax);
				       labelBBoxes->center[bCount] = (tempMin + tempMax) * 0.5f;
				       labelBBoxes->index[bCount] = theAtom;
				       bCount++;
			       }
		       }
	       }
       ATOMLOOP_END
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Computes bounding box of text. Since 2D text is screen-aligned,
//    we need to look at the current view volume and viewport region.
//
// Use: protected

void
ChemDisplay::
eachBBoxAtomLabelsWithSpheresRI
       (SoState *state, ChemDisplayParam *cdp,
	       SbBool clipOnCenter, int32_t &bCount, ChemLabelBBox *labelBBoxes)
//
////////////////////////////////////////////////////////////////////////
{
       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(NULL, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, FALSE)) {
	       return;
       }

       SbMatrix objToScreen;
       objToScreen = currentMVP;
       SbViewportRegion vpr = currentVPR;
       const SoClipPlaneElement *clipPlaneElt =
	       SoClipPlaneElement::getInstance(state);

       SbMatrix screenToObj = objToScreen.inverse();

       // Figure out the screen-space bounding box of the characters:
       SbBox3f screenBbox, charBbox;

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();
       float rad;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }
       SbString label;
       SbString name;
       bCount = 0;
       int32_t localAtomIndex;
       ATOMLOOP_START(atomLabelIndex)
	       if (!doHydrogens && chemData->getAtomicNumber(theAtom) == 1) {
		       continue;
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       localAtomIndex = chemData->getAtomIndex(theAtom);
	       rad = chemRadii->atomRadii[localAtomIndex];
	       rad *= radScale;
	       theScale.setValue(rad, rad, rad);
	       theMatrix.setTransform(coord, theRotation, theScale);
	       theMatrix.multVecMatrix(theZAxis, coord);
	       // The origin of the text on the screen is the object-space point
	       // coord
	       SbVec3f screenOrigin =
		       fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);

	       // Starting position of string, based on justification:
	       SbVec3f charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       const char *chars = label.getString();
	       screenBbox.makeEmpty();

	       for (int chr = 0; chr < label.getLength(); chr++) {
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
		       SbVec3f tempMin, tempMax;
		       SbVec3f p0, p1;

		       tempMin.setValue(min[0], min[1], screenOrigin[2]);
		       tempMax.setValue(max[0], max[1], screenOrigin[2]);
		       tempMin = toObjectSpace(tempMin, screenToObj, vpr);
		       tempMax = toObjectSpace(tempMax, screenToObj, vpr);

		       if (clipOnCenter) {
			       // Check to see if the center is between clipping planes.
			       // If not, the don't increment bCount.
			       // The check to see if the center is between the clipping planes
			       // needs to be performed in world space
			       // (see SoRayPickAction::isBetweenPlanes) so multiply by the
			       // currentModelMatrix
			       p0 = (tempMin + tempMax) * 0.5f;
			       currentModelMatrix.multVecMatrix(p0, p1);
			       if (isPointBetweenPlanes(p1, currentViewVolume, clipPlaneElt)) {
				       labelBBoxes->bbox[bCount].setBounds(tempMin, tempMax);
				       labelBBoxes->center[bCount] = p0;
				       labelBBoxes->index[bCount] = theAtom;
				       bCount++;
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
				       labelBBoxes->bbox[bCount].setBounds(tempMin, tempMax);
				       labelBBoxes->center[bCount] = (tempMin + tempMax) * 0.5f;
				       labelBBoxes->index[bCount] = theAtom;
				       bCount++;
			       }
		       }
	       }
       ATOMLOOP_END
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Computes bounding box of text. Since 2D text is screen-aligned,
//    we need to look at the current view volume and viewport region.
//
// Use: protected

void ChemDisplay:: eachBBoxBondLabelsWithWireframe
       (SoState *state, ChemDisplayParam *cdp,
	       SbBool clipOnCenter, int32_t &bCount, ChemLabelBBox *labelBBoxes)
//
////////////////////////////////////////////////////////////////////////
{
       ChemBaseData *chemData = ChemBaseDataElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(NULL, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, FALSE)) {
	       return;
       }

       SbMatrix objToScreen;
       objToScreen = currentMVP;
       SbViewportRegion vpr = currentVPR;
       const SoClipPlaneElement *clipPlaneElt =
	       SoClipPlaneElement::getInstance(state);

       SbMatrix screenToObj = objToScreen.inverse();

       // Figure out the screen-space bounding box of the characters:
       SbBox3f screenBbox, charBbox;

       //SbMatrix   theMatrix;
       //SbRotation theRotation = currentRotation;
       //SbVec3f    theScale;
       //SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->bondLabelString.getValue().getString();
       short lrJust = cdp->bondLabelLeftRightJustification.getValue();
       short tbJust = cdp->bondLabelTopBottomJustification.getValue();
       SbString label;
       SbString name;
       bCount = 0;
       BONDLOOP_START(bondLabelIndex)
	       int32_t from = chemData->getBondFrom(theBond);
	       int32_t to = chemData->getBondTo(theBond);
	       if (!doHydrogens) {
		       if ((chemData->getAtomicNumber(from) == 1) ||
			       (chemData->getAtomicNumber(to) == 1)) continue;
	       }
	       SbVec3f coords1 = chemData->getAtomCoordinates(from).getValue();
	       SbVec3f coords2 = chemData->getAtomCoordinates(to).getValue();
	       coord = (coords2 - coords1) * 0.5f + coords1;
	       // The origin of the text on the screen is the object-space point
	       // coord
	       SbVec3f screenOrigin =
		       fromObjectSpace(coord, objToScreen, vpr);

	       parseBondLabel(labelFormat, theBond, from, to, coords1, coords2,
		       chemData, label);

	       // Starting position of string, based on justification:
	       SbVec3f charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       const char *chars = label.getString();
	       screenBbox.makeEmpty();

	       for (int chr = 0; chr < label.getLength(); chr++) {
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
		       SbVec3f tempMin, tempMax;
		       SbVec3f p0, p1;

		       tempMin.setValue(min[0], min[1], screenOrigin[2]);
		       tempMax.setValue(max[0], max[1], screenOrigin[2]);
		       tempMin = toObjectSpace(tempMin, screenToObj, vpr);
		       tempMax = toObjectSpace(tempMax, screenToObj, vpr);

		       if (clipOnCenter) {
			       // Check to see if the center is between clipping planes.
			       // If not, the don't increment bCount.
			       // The check to see if the center is between the clipping planes
			       // needs to be performed in world space
			       // (see SoRayPickAction::isBetweenPlanes) so multiply by the
			       // currentModelMatrix
			       p0 = (tempMin + tempMax) * 0.5f;
			       currentModelMatrix.multVecMatrix(p0, p1);
			       if (isPointBetweenPlanes(p1, currentViewVolume, clipPlaneElt)) {
				       labelBBoxes->bbox[bCount].setBounds(tempMin, tempMax);
				       labelBBoxes->center[bCount] = p0;
				       labelBBoxes->index[bCount] = theBond;
				       bCount++;
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
				       labelBBoxes->bbox[bCount].setBounds(tempMin, tempMax);
				       labelBBoxes->center[bCount] = (tempMin + tempMax) * 0.5f;
				       labelBBoxes->index[bCount] = theBond;
				       bCount++;
			       }
		       }
	       }
       BONDLOOP_END
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Computes bounding box of text. Since 2D text is screen-aligned,
//    we need to look at the current view volume and viewport region.
//
// Use: protected

void
ChemDisplay::
eachBBoxBondLabelsWithCylinders
       (SoState *state, ChemDisplayParam *cdp,
	       SbBool clipOnCenter, int32_t &bCount, ChemLabelBBox *labelBBoxes)
//
////////////////////////////////////////////////////////////////////////
{
       ChemBaseData *chemData = ChemBaseDataElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(NULL, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, FALSE)) {
	       return;
       }

       SbMatrix objToScreen;
       objToScreen = currentMVP;
       SbViewportRegion vpr = currentVPR;
       const SoClipPlaneElement *clipPlaneElt =
	       SoClipPlaneElement::getInstance(state);

       SbMatrix screenToObj = objToScreen.inverse();

       // Figure out the screen-space bounding box of the characters:
       SbBox3f screenBbox, charBbox;

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->bondLabelString.getValue().getString();
       short lrJust = cdp->bondLabelLeftRightJustification.getValue();
       short tbJust = cdp->bondLabelTopBottomJustification.getValue();
       float rad = cdp->bondCylinderRadius.getValue();
       SbString label;
       SbString name;
       bCount = 0;
       BONDLOOP_START(bondLabelIndex)
	       int32_t from = chemData->getBondFrom(theBond);
	       int32_t to = chemData->getBondTo(theBond);
	       if (!doHydrogens) {
		       if ((chemData->getAtomicNumber(from) == 1) ||
			       (chemData->getAtomicNumber(to) == 1)) continue;
	       }
	       SbVec3f coords1 = chemData->getAtomCoordinates(from).getValue();
	       SbVec3f coords2 = chemData->getAtomCoordinates(to).getValue();
	       coord = (coords2 - coords1) * 0.5f + coords1;
	       theScale.setValue(rad, rad, rad);
	       theMatrix.setTransform(coord, theRotation, theScale);
	       theMatrix.multVecMatrix(theZAxis, coord);
	       // The origin of the text on the screen is the object-space point
	       // coord
	       SbVec3f screenOrigin =
		       fromObjectSpace(coord, objToScreen, vpr);

	       parseBondLabel(labelFormat, theBond, from, to, coords1, coords2,
		       chemData, label);

	       // Starting position of string, based on justification:
	       SbVec3f charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       const char *chars = label.getString();
	       screenBbox.makeEmpty();

	       for (int chr = 0; chr < label.getLength(); chr++) {
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
		       SbVec3f tempMin, tempMax;
		       SbVec3f p0, p1;

		       tempMin.setValue(min[0], min[1], screenOrigin[2]);
		       tempMax.setValue(max[0], max[1], screenOrigin[2]);
		       tempMin = toObjectSpace(tempMin, screenToObj, vpr);
		       tempMax = toObjectSpace(tempMax, screenToObj, vpr);

		       if (clipOnCenter) {
			       // Check to see if the center is between clipping planes.
			       // If not, the don't increment bCount.
			       // The check to see if the center is between the clipping planes
			       // needs to be performed in world space
			       // (see SoRayPickAction::isBetweenPlanes) so multiply by the
			       // currentModelMatrix
			       p0 = (tempMin + tempMax) * 0.5f;
			       currentModelMatrix.multVecMatrix(p0, p1);
			       if (isPointBetweenPlanes(p1, currentViewVolume, clipPlaneElt)) {
				       labelBBoxes->bbox[bCount].setBounds(tempMin, tempMax);
				       labelBBoxes->center[bCount] = p0;
				       labelBBoxes->index[bCount] = theBond;
				       bCount++;
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
				       labelBBoxes->bbox[bCount].setBounds(tempMin, tempMax);
				       labelBBoxes->center[bCount] = (tempMin + tempMax) * 0.5f;
				       labelBBoxes->index[bCount] = theBond;
				       bCount++;
			       }
		       }
	       }
       BONDLOOP_END
}

// --> atom, bond, residue clipping
/*
#undef ATOMLOOP_START
#undef BONDLOOP_START
#undef RESIDUELOOP_START
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
	for (theAtom = atomStart; theAtom < atomEnd; theAtom++) { \
	if (clipAtoms && !lodSelector->atomLabels->testBit(theAtom)) continue; \

#define BONDLOOP_START(INDEX)												  \
numBondLoops = (INDEX).getNum();											  \
for (bondLoop = 0; bondLoop < numBondLoops; bondLoop++) {					  \
	(INDEX)[bondLoop].getValue(bondStart, bondEnd);							  \
	if (bondEnd == CHEM_DISPLAY_USE_REST_OF_BONDS) {						  \
	   bondEnd = globalNumberOfBonds;										  \
	}																		  \
	else {																	  \
	   bondEnd += bondStart;												  \
	}																		  \
	for (theBond = bondStart; theBond < bondEnd; theBond++) { \
	if (clipBonds && !lodSelector->bondLabels->testBit(theBond)) continue; \

#define RESIDUELOOP_START(INDEX)												  \
numResidueLoops = (INDEX).getNum();      \
for (residueLoop = 0; residueLoop < numResidueLoops; residueLoop++) {	  \
	(INDEX)[residueLoop].getValue(residueStart, residueEnd);		  \
	if (residueEnd == CHEM_DISPLAY_USE_REST_OF_RESIDUES) {	  \
	   residueEnd = globalNumberOfResidues;			  \
	}							  \
	else {			  \
	   residueEnd += residueStart;	  \
	}				  \
for (theResidue = residueStart; theResidue < residueEnd; theResidue++) { \
if (!lodSelector->residueLabels->testBit(theResidue)) continue; \
*/
// <-- atom, bond, residue clipping

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Renders atom labels when in wireframe mode
//
// Use: protected

void
ChemDisplay::renderAtomLabelsWithWireframe(SoGLRenderAction *action)
//
////////////////////////////////////////////////////////////////////////
{
	SoState *state = action->getState();
	ChemColor *chemColor = ChemColorElement::get(state);

// --> atom labels clipping
	if (clipAtoms) computeAtomLabelsCulling(action);
// <-- atom labels clipping

	switch (chemColor->atomLabelColorBinding.getValue()) {
	  case ChemColor::ATOM_LABEL_OVERALL:
	    normalAtomLabelsWLO(action, vnormalAtomLabelIndex);
	    highlightAtomLabelsWLO(action, vhighlightAtomLabelIndex);
	    break;

	  case ChemColor::ATOM_LABEL_PER_ATOM_LABEL:
	    normalAtomLabelsWLL(action, vnormalAtomLabelIndex);
	    highlightAtomLabelsWLL(action, vhighlightAtomLabelIndex);
	    break;

	  case ChemColor::ATOM_LABEL_PER_ATOM_LABEL_INDEXED:
	    normalAtomLabelsWLI(action, vnormalAtomLabelIndex);
	    highlightAtomLabelsWLI(action, vhighlightAtomLabelIndex);
	    break;

	  case ChemColor::ATOM_LABEL_PER_ATOM:
	    switch (chemColor->atomColorBinding.getValue()) {
	      case ChemColor::ATOM_OVERALL:
		normalAtomLabelsWAO(action, vnormalAtomLabelIndex);
		highlightAtomLabelsWAO(action, vhighlightAtomLabelIndex);
		    break;

		  case ChemColor::ATOM_PER_ATOM:
		normalAtomLabelsWAA(action, vnormalAtomLabelIndex);
		highlightAtomLabelsWAA(action, vhighlightAtomLabelIndex);
		    break;

		  case ChemColor::ATOM_PER_ATOM_INDEXED:
		normalAtomLabelsWAI(action, vnormalAtomLabelIndex);
		highlightAtomLabelsWAI(action, vhighlightAtomLabelIndex);
		    break;
		}
		break;

	  default:
		break;
	}
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Renders atom labels when sticks are used
//
// Use: protected

void
ChemDisplay::renderAtomLabelsWithCylinders(SoGLRenderAction *action)
//
////////////////////////////////////////////////////////////////////////
{
	SoState *state = action->getState();
	ChemColor *chemColor = ChemColorElement::get(state);

// --> atom labels clipping
	if (clipAtoms) computeAtomLabelsCulling(action);
// <-- atom labels clipping

	switch (chemColor->atomLabelColorBinding.getValue()) {
	  case ChemColor::ATOM_LABEL_OVERALL:
	    normalAtomLabelsCLO(action, vnormalAtomLabelIndex);
	    highlightAtomLabelsCLO(action, vhighlightAtomLabelIndex);
	    break;

	  case ChemColor::ATOM_LABEL_PER_ATOM_LABEL:
	    normalAtomLabelsCLL(action, vnormalAtomLabelIndex);
	    highlightAtomLabelsCLL(action, vhighlightAtomLabelIndex);
	    break;

	  case ChemColor::ATOM_LABEL_PER_ATOM_LABEL_INDEXED:
	    normalAtomLabelsCLI(action, vnormalAtomLabelIndex);
	    highlightAtomLabelsCLI(action, vhighlightAtomLabelIndex);
	    break;

	  case ChemColor::ATOM_LABEL_PER_ATOM:
	    switch (chemColor->atomColorBinding.getValue()) {
	      case ChemColor::ATOM_OVERALL:
		normalAtomLabelsCAO(action, vnormalAtomLabelIndex);
		highlightAtomLabelsCAO(action, vhighlightAtomLabelIndex);
		    break;

		  case ChemColor::ATOM_PER_ATOM:
		normalAtomLabelsCAA(action, vnormalAtomLabelIndex);
		highlightAtomLabelsCAA(action, vhighlightAtomLabelIndex);
		    break;

		  case ChemColor::ATOM_PER_ATOM_INDEXED:
		normalAtomLabelsCAI(action, vnormalAtomLabelIndex);
		highlightAtomLabelsCAI(action, vhighlightAtomLabelIndex);
		    break;
		}
		break;

	  default:
		break;
	}
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Renders atom labels when spheres are used
//
// Use: protected

void
ChemDisplay::renderAtomLabelsWithSpheres(SoGLRenderAction *action)
//
////////////////////////////////////////////////////////////////////////
{
	SoState *state = action->getState();
	//ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);
	ChemRadii *chemRadii = ChemRadiiElement::get(state);
	ChemColor *chemColor = ChemColorElement::get(state);

// --> atom labels clipping
	if (clipAtoms) computeAtomLabelsCulling(action);
// <-- atom labels clipping

	switch (chemRadii->atomRadiiBinding.getValue()) {
	  case ChemRadii::RADII_OVERALL:
	    switch (chemColor->atomLabelColorBinding.getValue()) {
	      case ChemColor::ATOM_LABEL_OVERALL:
		normalAtomLabelsSROLO(action, vnormalAtomLabelIndex);
		highlightAtomLabelsSROLO(action, vhighlightAtomLabelIndex);
		break;

	      case ChemColor::ATOM_LABEL_PER_ATOM_LABEL:
		normalAtomLabelsSROLL(action, vnormalAtomLabelIndex);
		highlightAtomLabelsSROLL(action, vhighlightAtomLabelIndex);
		break;

	      case ChemColor::ATOM_LABEL_PER_ATOM_LABEL_INDEXED:
		normalAtomLabelsSROLI(action, vnormalAtomLabelIndex);
		highlightAtomLabelsSROLI(action, vhighlightAtomLabelIndex);
		break;

	      case ChemColor::ATOM_LABEL_PER_ATOM:
		switch (chemColor->atomColorBinding.getValue()) {
		  case ChemColor::ATOM_OVERALL:
		    normalAtomLabelsSROAO(action, vnormalAtomLabelIndex);
		    highlightAtomLabelsSROAO(action, vhighlightAtomLabelIndex);
		    break;

		  case ChemColor::ATOM_PER_ATOM:
		    normalAtomLabelsSROAA(action, vnormalAtomLabelIndex);
		    highlightAtomLabelsSROAA(action, vhighlightAtomLabelIndex);
		    break;

		  case ChemColor::ATOM_PER_ATOM_INDEXED:
		    normalAtomLabelsSROAI(action, vnormalAtomLabelIndex);
		    highlightAtomLabelsSROAI(action, vhighlightAtomLabelIndex);
		    break;

		  default:
		    break;
		}

	      default:
		break;
	    }

	  case ChemRadii::RADII_PER_ATOM:
	    switch (chemColor->atomLabelColorBinding.getValue()) {
	      case ChemColor::ATOM_LABEL_OVERALL:
		normalAtomLabelsSRALO(action, vnormalAtomLabelIndex);
		highlightAtomLabelsSRALO(action, vhighlightAtomLabelIndex);
		break;

	      case ChemColor::ATOM_LABEL_PER_ATOM_LABEL:
		normalAtomLabelsSRALL(action, vnormalAtomLabelIndex);
		highlightAtomLabelsSRALL(action, vhighlightAtomLabelIndex);
		break;

	      case ChemColor::ATOM_LABEL_PER_ATOM_LABEL_INDEXED:
		normalAtomLabelsSRALI(action, vnormalAtomLabelIndex);
		highlightAtomLabelsSRALI(action, vhighlightAtomLabelIndex);
		break;

	      case ChemColor::ATOM_LABEL_PER_ATOM:
		switch (chemColor->atomColorBinding.getValue()) {
		  case ChemColor::ATOM_OVERALL:
		    normalAtomLabelsSRAAO(action, vnormalAtomLabelIndex);
		    highlightAtomLabelsSRAAO(action, vhighlightAtomLabelIndex);
		    break;

		  case ChemColor::ATOM_PER_ATOM:
		    normalAtomLabelsSRAAA(action, vnormalAtomLabelIndex);
		    highlightAtomLabelsSRAAA(action, vhighlightAtomLabelIndex);
		    break;

		  case ChemColor::ATOM_PER_ATOM_INDEXED:
		    normalAtomLabelsSRAAI(action, vnormalAtomLabelIndex);
		    highlightAtomLabelsSRAAI(action, vhighlightAtomLabelIndex);
		    break;

		  default:
		    break;
		}

	      default:
		break;
	    }

	  case ChemRadii::RADII_PER_ATOM_INDEXED:
	    switch (chemColor->atomLabelColorBinding.getValue()) {
	      case ChemColor::ATOM_LABEL_OVERALL:
		normalAtomLabelsSRILO(action, vnormalAtomLabelIndex);
		highlightAtomLabelsSRILO(action, vhighlightAtomLabelIndex);
		break;

	      case ChemColor::ATOM_LABEL_PER_ATOM_LABEL:
		normalAtomLabelsSRILL(action, vnormalAtomLabelIndex);
		highlightAtomLabelsSRILL(action, vhighlightAtomLabelIndex);
		break;

	      case ChemColor::ATOM_LABEL_PER_ATOM_LABEL_INDEXED:
		normalAtomLabelsSRILI(action, vnormalAtomLabelIndex);
		highlightAtomLabelsSRILI(action, vhighlightAtomLabelIndex);
		break;

	      case ChemColor::ATOM_LABEL_PER_ATOM:
		switch (chemColor->atomColorBinding.getValue()) {
		  case ChemColor::ATOM_OVERALL:
		    normalAtomLabelsSRIAO(action, vnormalAtomLabelIndex);
		    highlightAtomLabelsSRIAO(action, vhighlightAtomLabelIndex);
		    break;

		  case ChemColor::ATOM_PER_ATOM:
		    normalAtomLabelsSRIAA(action, vnormalAtomLabelIndex);
		    highlightAtomLabelsSRIAA(action, vhighlightAtomLabelIndex);
		    break;

		  case ChemColor::ATOM_PER_ATOM_INDEXED:
		    normalAtomLabelsSRIAI(action, vnormalAtomLabelIndex);
		    highlightAtomLabelsSRIAI(action, vhighlightAtomLabelIndex);
		    break;

		  default:
		    break;
		}

	      default:
		break;
	    }

	  default:
	    break;
	}
}



////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
normalAtomLabelsWLO
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::normalAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();

       vpr = SoViewportRegionElement::get(state);

       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       //SbMatrix   theMatrix;
       //SbVec3f    theScale;
       //SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomLabelColor.getValues(0);
       glColor3fv(labelColors[0].getValue());
       ATOMLOOP_START(theAtomLabelIndex)
	       if (!doHydrogens &&
		       chemData->getAtomicNumber(theAtom) == 1) { 
		       continue; 
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);
	       charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       // Transform the screen-space starting position into object
	       // space, and feed that back to the glRasterPos command (which
	       // will turn around and transform it back into screen-space,
	       // but oh well).

	       coord = toObjectSpace(charPosition, screenToObj, vpr);
	       glRasterPos3fv(coord.getValue());
	       normalFont->drawString(label);
       ATOMLOOP_END

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
normalAtomLabelsWLL
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::normalAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();

       vpr = SoViewportRegionElement::get(state);

       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       //SbMatrix   theMatrix;
       //SbVec3f    theScale;
       //SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomLabelColor.getValues(0);
       ATOMLOOP_START(theAtomLabelIndex)
	       if (!doHydrogens &&
		       chemData->getAtomicNumber(theAtom) == 1) { 
		       continue; 
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       glColor3fv(labelColors[theAtom].getValue());
	       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);
	       charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       // Transform the screen-space starting position into object
	       // space, and feed that back to the glRasterPos command (which
	       // will turn around and transform it back into screen-space,
	       // but oh well).

	       coord = toObjectSpace(charPosition, screenToObj, vpr);
	       glRasterPos3fv(coord.getValue());
	       normalFont->drawString(label);
       ATOMLOOP_END

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
normalAtomLabelsWLI
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::normalAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();

       vpr = SoViewportRegionElement::get(state);

       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       //SbMatrix   theMatrix;
       //SbVec3f    theScale;
       //SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();
       int32_t localAtomIndex;

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomLabelColor.getValues(0);
       ATOMLOOP_START(theAtomLabelIndex)
	       if (!doHydrogens &&
		       chemData->getAtomicNumber(theAtom) == 1) { 
		       continue; 
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       localAtomIndex = chemData->getAtomIndex(theAtom);
	       glColor3fv(labelColors[localAtomIndex].getValue());
	       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);
	       charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       // Transform the screen-space starting position into object
	       // space, and feed that back to the glRasterPos command (which
	       // will turn around and transform it back into screen-space,
	       // but oh well).

	       coord = toObjectSpace(charPosition, screenToObj, vpr);
	       glRasterPos3fv(coord.getValue());
	       normalFont->drawString(label);
       ATOMLOOP_END

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
normalAtomLabelsWAO
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::normalAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();

       vpr = SoViewportRegionElement::get(state);

       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       //SbMatrix   theMatrix;
       //SbVec3f    theScale;
       //SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomColor.getValues(0);
       glColor3fv(labelColors[0].getValue());
       ATOMLOOP_START(theAtomLabelIndex)
	       if (!doHydrogens &&
		       chemData->getAtomicNumber(theAtom) == 1) { 
		       continue; 
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);
	       charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       // Transform the screen-space starting position into object
	       // space, and feed that back to the glRasterPos command (which
	       // will turn around and transform it back into screen-space,
	       // but oh well).

	       coord = toObjectSpace(charPosition, screenToObj, vpr);
	       glRasterPos3fv(coord.getValue());
	       normalFont->drawString(label);
       ATOMLOOP_END

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
normalAtomLabelsWAA
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::normalAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();

       vpr = SoViewportRegionElement::get(state);

       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       //SbMatrix   theMatrix;
       //SbVec3f    theScale;
       //SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomColor.getValues(0);
       ATOMLOOP_START(theAtomLabelIndex)
	       if (!doHydrogens &&
		       chemData->getAtomicNumber(theAtom) == 1) { 
		       continue; 
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       glColor3fv(labelColors[theAtom].getValue());
	       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);
	       charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       // Transform the screen-space starting position into object
	       // space, and feed that back to the glRasterPos command (which
	       // will turn around and transform it back into screen-space,
	       // but oh well).

	       coord = toObjectSpace(charPosition, screenToObj, vpr);
	       glRasterPos3fv(coord.getValue());
	       normalFont->drawString(label);
       ATOMLOOP_END

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
normalAtomLabelsWAI
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::normalAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();

       vpr = SoViewportRegionElement::get(state);

       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       //SbMatrix   theMatrix;
       //SbVec3f    theScale;
       //SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();
       int32_t localAtomIndex;

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomColor.getValues(0);
       ATOMLOOP_START(theAtomLabelIndex)
	       if (!doHydrogens &&
		       chemData->getAtomicNumber(theAtom) == 1) { 
		       continue; 
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       localAtomIndex = chemData->getAtomIndex(theAtom);
	       glColor3fv(labelColors[localAtomIndex].getValue());
	       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);
	       charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       // Transform the screen-space starting position into object
	       // space, and feed that back to the glRasterPos command (which
	       // will turn around and transform it back into screen-space,
	       // but oh well).

	       coord = toObjectSpace(charPosition, screenToObj, vpr);
	       glRasterPos3fv(coord.getValue());
	       normalFont->drawString(label);
       ATOMLOOP_END

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
normalAtomLabelsCLO
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::normalAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();

       vpr = SoViewportRegionElement::get(state);

       float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
			 SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomLabelColor.getValues(0);
       glColor3fv(labelColors[0].getValue());
       ATOMLOOP_START(theAtomLabelIndex)
	       if (!doHydrogens &&
		       chemData->getAtomicNumber(theAtom) == 1) { 
		       continue; 
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       rad = cdp->bondCylinderRadius.getValue();
	       rad *= radScale;
	       theScale.setValue(rad, rad, rad);
	       theMatrix.setTransform(coord, theRotation, theScale);
	       theMatrix.multVecMatrix(theZAxis, coord);
	       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);
	       charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       // Transform the screen-space starting position into object
	       // space, and feed that back to the glRasterPos command (which
	       // will turn around and transform it back into screen-space,
	       // but oh well).

	       coord = toObjectSpace(charPosition, screenToObj, vpr);
	       glRasterPos3fv(coord.getValue());
	       normalFont->drawString(label);
       ATOMLOOP_END

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
normalAtomLabelsCLL
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::normalAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();

       vpr = SoViewportRegionElement::get(state);

       float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomLabelColor.getValues(0);
       ATOMLOOP_START(theAtomLabelIndex)
	       if (!doHydrogens &&
		       chemData->getAtomicNumber(theAtom) == 1) { 
		       continue; 
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       glColor3fv(labelColors[theAtom].getValue());
	       rad = cdp->bondCylinderRadius.getValue();
	       rad *= radScale;
	       theScale.setValue(rad, rad, rad);
	       theMatrix.setTransform(coord, theRotation, theScale);
	       theMatrix.multVecMatrix(theZAxis, coord);
	       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);
	       charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       // Transform the screen-space starting position into object
	       // space, and feed that back to the glRasterPos command (which
	       // will turn around and transform it back into screen-space,
	       // but oh well).

	       coord = toObjectSpace(charPosition, screenToObj, vpr);
	       glRasterPos3fv(coord.getValue());
	       normalFont->drawString(label);
       ATOMLOOP_END

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
normalAtomLabelsCLI
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::normalAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();

       vpr = SoViewportRegionElement::get(state);

       float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();
       int32_t localAtomIndex;

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomLabelColor.getValues(0);
       ATOMLOOP_START(theAtomLabelIndex)
	       if (!doHydrogens &&
		       chemData->getAtomicNumber(theAtom) == 1) { 
		       continue; 
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       localAtomIndex = chemData->getAtomIndex(theAtom);
	       glColor3fv(labelColors[localAtomIndex].getValue());
	       rad = cdp->bondCylinderRadius.getValue();
	       rad *= radScale;
	       theScale.setValue(rad, rad, rad);
	       theMatrix.setTransform(coord, theRotation, theScale);
	       theMatrix.multVecMatrix(theZAxis, coord);
	       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);
	       charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       // Transform the screen-space starting position into object
	       // space, and feed that back to the glRasterPos command (which
	       // will turn around and transform it back into screen-space,
	       // but oh well).

	       coord = toObjectSpace(charPosition, screenToObj, vpr);
	       glRasterPos3fv(coord.getValue());
	       normalFont->drawString(label);
       ATOMLOOP_END

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
normalAtomLabelsCAO
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::normalAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();

       vpr = SoViewportRegionElement::get(state);

       float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomColor.getValues(0);
       glColor3fv(labelColors[0].getValue());
       ATOMLOOP_START(theAtomLabelIndex)
	       if (!doHydrogens &&
		       chemData->getAtomicNumber(theAtom) == 1) { 
		       continue; 
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       rad = cdp->bondCylinderRadius.getValue();
	       rad *= radScale;
	       theScale.setValue(rad, rad, rad);
	       theMatrix.setTransform(coord, theRotation, theScale);
	       theMatrix.multVecMatrix(theZAxis, coord);
	       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);
	       charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       // Transform the screen-space starting position into object
	       // space, and feed that back to the glRasterPos command (which
	       // will turn around and transform it back into screen-space,
	       // but oh well).

	       coord = toObjectSpace(charPosition, screenToObj, vpr);
	       glRasterPos3fv(coord.getValue());
	       normalFont->drawString(label);
       ATOMLOOP_END

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
normalAtomLabelsCAA
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::normalAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();

       vpr = SoViewportRegionElement::get(state);

       float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomColor.getValues(0);
       ATOMLOOP_START(theAtomLabelIndex)
	       if (!doHydrogens &&
		       chemData->getAtomicNumber(theAtom) == 1) { 
		       continue; 
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       glColor3fv(labelColors[theAtom].getValue());
	       rad = cdp->bondCylinderRadius.getValue();
	       rad *= radScale;
	       theScale.setValue(rad, rad, rad);
	       theMatrix.setTransform(coord, theRotation, theScale);
	       theMatrix.multVecMatrix(theZAxis, coord);
	       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);
	       charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       // Transform the screen-space starting position into object
	       // space, and feed that back to the glRasterPos command (which
	       // will turn around and transform it back into screen-space,
	       // but oh well).

	       coord = toObjectSpace(charPosition, screenToObj, vpr);
	       glRasterPos3fv(coord.getValue());
	       normalFont->drawString(label);
       ATOMLOOP_END

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
normalAtomLabelsCAI
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::normalAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();

       vpr = SoViewportRegionElement::get(state);

       float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();
       int32_t localAtomIndex;

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomColor.getValues(0);
       ATOMLOOP_START(theAtomLabelIndex)
	       if (!doHydrogens &&
		       chemData->getAtomicNumber(theAtom) == 1) { 
		       continue; 
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       localAtomIndex = chemData->getAtomIndex(theAtom);
	       glColor3fv(labelColors[localAtomIndex].getValue());
	       rad = cdp->bondCylinderRadius.getValue();
	       rad *= radScale;
	       theScale.setValue(rad, rad, rad);
	       theMatrix.setTransform(coord, theRotation, theScale);
	       theMatrix.multVecMatrix(theZAxis, coord);
	       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);
	       charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       // Transform the screen-space starting position into object
	       // space, and feed that back to the glRasterPos command (which
	       // will turn around and transform it back into screen-space,
	       // but oh well).

	       coord = toObjectSpace(charPosition, screenToObj, vpr);
	       glRasterPos3fv(coord.getValue());
	       normalFont->drawString(label);
       ATOMLOOP_END

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
normalAtomLabelsSROLO
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::normalAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();

       vpr = SoViewportRegionElement::get(state);

       float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomLabelColor.getValues(0);
       glColor3fv(labelColors[0].getValue());
       ATOMLOOP_START(theAtomLabelIndex)
	       if (!doHydrogens &&
		       chemData->getAtomicNumber(theAtom) == 1) { 
		       continue; 
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       rad = chemRadii->atomRadii[0];
	       rad *= radScale;
	       theScale.setValue(rad, rad, rad);
	       theMatrix.setTransform(coord, theRotation, theScale);
	       theMatrix.multVecMatrix(theZAxis, coord);
	       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);
	       charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       // Transform the screen-space starting position into object
	       // space, and feed that back to the glRasterPos command (which
	       // will turn around and transform it back into screen-space,
	       // but oh well).

	       coord = toObjectSpace(charPosition, screenToObj, vpr);
	       glRasterPos3fv(coord.getValue());
	       normalFont->drawString(label);
       ATOMLOOP_END

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
normalAtomLabelsSROLL
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::normalAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();

       vpr = SoViewportRegionElement::get(state);

       float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomLabelColor.getValues(0);
       ATOMLOOP_START(theAtomLabelIndex)
	       if (!doHydrogens &&
		       chemData->getAtomicNumber(theAtom) == 1) { 
		       continue; 
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       glColor3fv(labelColors[theAtom].getValue());
	       rad = chemRadii->atomRadii[0];
	       rad *= radScale;
	       theScale.setValue(rad, rad, rad);
	       theMatrix.setTransform(coord, theRotation, theScale);
	       theMatrix.multVecMatrix(theZAxis, coord);
	       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);
	       charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       // Transform the screen-space starting position into object
	       // space, and feed that back to the glRasterPos command (which
	       // will turn around and transform it back into screen-space,
	       // but oh well).

	       coord = toObjectSpace(charPosition, screenToObj, vpr);
	       glRasterPos3fv(coord.getValue());
	       normalFont->drawString(label);
       ATOMLOOP_END

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
normalAtomLabelsSROLI
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::normalAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();

       vpr = SoViewportRegionElement::get(state);

       float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();
       int32_t localAtomIndex;

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomLabelColor.getValues(0);
       ATOMLOOP_START(theAtomLabelIndex)
	       if (!doHydrogens &&
		       chemData->getAtomicNumber(theAtom) == 1) { 
		       continue; 
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       localAtomIndex = chemData->getAtomIndex(theAtom);
	       glColor3fv(labelColors[localAtomIndex].getValue());
	       rad = chemRadii->atomRadii[0];
	       rad *= radScale;
	       theScale.setValue(rad, rad, rad);
	       theMatrix.setTransform(coord, theRotation, theScale);
	       theMatrix.multVecMatrix(theZAxis, coord);
	       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);
	       charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       // Transform the screen-space starting position into object
	       // space, and feed that back to the glRasterPos command (which
	       // will turn around and transform it back into screen-space,
	       // but oh well).

	       coord = toObjectSpace(charPosition, screenToObj, vpr);
	       glRasterPos3fv(coord.getValue());
	       normalFont->drawString(label);
       ATOMLOOP_END

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
normalAtomLabelsSROAO
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::normalAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();

       vpr = SoViewportRegionElement::get(state);

       float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomColor.getValues(0);
       glColor3fv(labelColors[0].getValue());
       ATOMLOOP_START(theAtomLabelIndex)
	       if (!doHydrogens &&
		       chemData->getAtomicNumber(theAtom) == 1) { 
		       continue; 
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       rad = chemRadii->atomRadii[0];
	       rad *= radScale;
	       theScale.setValue(rad, rad, rad);
	       theMatrix.setTransform(coord, theRotation, theScale);
	       theMatrix.multVecMatrix(theZAxis, coord);
	       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);
	       charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       // Transform the screen-space starting position into object
	       // space, and feed that back to the glRasterPos command (which
	       // will turn around and transform it back into screen-space,
	       // but oh well).

	       coord = toObjectSpace(charPosition, screenToObj, vpr);
	       glRasterPos3fv(coord.getValue());
	       normalFont->drawString(label);
       ATOMLOOP_END

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
normalAtomLabelsSROAA
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::normalAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();

       vpr = SoViewportRegionElement::get(state);

       float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomColor.getValues(0);
       ATOMLOOP_START(theAtomLabelIndex)
	       if (!doHydrogens &&
		       chemData->getAtomicNumber(theAtom) == 1) { 
		       continue; 
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       glColor3fv(labelColors[theAtom].getValue());
	       rad = chemRadii->atomRadii[0];
	       rad *= radScale;
	       theScale.setValue(rad, rad, rad);
	       theMatrix.setTransform(coord, theRotation, theScale);
	       theMatrix.multVecMatrix(theZAxis, coord);
	       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);
	       charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       // Transform the screen-space starting position into object
	       // space, and feed that back to the glRasterPos command (which
	       // will turn around and transform it back into screen-space,
	       // but oh well).

	       coord = toObjectSpace(charPosition, screenToObj, vpr);
	       glRasterPos3fv(coord.getValue());
	       normalFont->drawString(label);
       ATOMLOOP_END

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
normalAtomLabelsSROAI
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::normalAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();

       vpr = SoViewportRegionElement::get(state);

       float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();
       int32_t localAtomIndex;

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomColor.getValues(0);
       ATOMLOOP_START(theAtomLabelIndex)
	       if (!doHydrogens &&
		       chemData->getAtomicNumber(theAtom) == 1) { 
		       continue; 
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       localAtomIndex = chemData->getAtomIndex(theAtom);
	       glColor3fv(labelColors[localAtomIndex].getValue());
	       rad = chemRadii->atomRadii[0];
	       rad *= radScale;
	       theScale.setValue(rad, rad, rad);
	       theMatrix.setTransform(coord, theRotation, theScale);
	       theMatrix.multVecMatrix(theZAxis, coord);
	       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);
	       charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       // Transform the screen-space starting position into object
	       // space, and feed that back to the glRasterPos command (which
	       // will turn around and transform it back into screen-space,
	       // but oh well).

	       coord = toObjectSpace(charPosition, screenToObj, vpr);
	       glRasterPos3fv(coord.getValue());
	       normalFont->drawString(label);
       ATOMLOOP_END

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
normalAtomLabelsSRALO
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::normalAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();

       vpr = SoViewportRegionElement::get(state);

       float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomLabelColor.getValues(0);
       glColor3fv(labelColors[0].getValue());
       ATOMLOOP_START(theAtomLabelIndex)
	       if (!doHydrogens &&
		       chemData->getAtomicNumber(theAtom) == 1) { 
		       continue; 
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       rad = chemRadii->atomRadii[theAtom];
	       rad *= radScale;
	       theScale.setValue(rad, rad, rad);
	       theMatrix.setTransform(coord, theRotation, theScale);
	       theMatrix.multVecMatrix(theZAxis, coord);
	       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);
	       charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       // Transform the screen-space starting position into object
	       // space, and feed that back to the glRasterPos command (which
	       // will turn around and transform it back into screen-space,
	       // but oh well).

	       coord = toObjectSpace(charPosition, screenToObj, vpr);
	       glRasterPos3fv(coord.getValue());
	       normalFont->drawString(label);
       ATOMLOOP_END

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
normalAtomLabelsSRALL
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::normalAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();

       vpr = SoViewportRegionElement::get(state);

       float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomLabelColor.getValues(0);
       ATOMLOOP_START(theAtomLabelIndex)
	       if (!doHydrogens &&
		       chemData->getAtomicNumber(theAtom) == 1) { 
		       continue; 
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       glColor3fv(labelColors[theAtom].getValue());
	       rad = chemRadii->atomRadii[theAtom];
	       rad *= radScale;
	       theScale.setValue(rad, rad, rad);
	       theMatrix.setTransform(coord, theRotation, theScale);
	       theMatrix.multVecMatrix(theZAxis, coord);
	       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);
	       charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       // Transform the screen-space starting position into object
	       // space, and feed that back to the glRasterPos command (which
	       // will turn around and transform it back into screen-space,
	       // but oh well).

	       coord = toObjectSpace(charPosition, screenToObj, vpr);
	       glRasterPos3fv(coord.getValue());
	       normalFont->drawString(label);
       ATOMLOOP_END

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
normalAtomLabelsSRALI
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::normalAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();

       vpr = SoViewportRegionElement::get(state);

       float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();
       int32_t localAtomIndex;

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomLabelColor.getValues(0);
       ATOMLOOP_START(theAtomLabelIndex)
	       if (!doHydrogens &&
		       chemData->getAtomicNumber(theAtom) == 1) { 
		       continue; 
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       localAtomIndex = chemData->getAtomIndex(theAtom);
	       glColor3fv(labelColors[localAtomIndex].getValue());
	       rad = chemRadii->atomRadii[theAtom];
	       rad *= radScale;
	       theScale.setValue(rad, rad, rad);
	       theMatrix.setTransform(coord, theRotation, theScale);
	       theMatrix.multVecMatrix(theZAxis, coord);
	       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);
	       charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       // Transform the screen-space starting position into object
	       // space, and feed that back to the glRasterPos command (which
	       // will turn around and transform it back into screen-space,
	       // but oh well).

	       coord = toObjectSpace(charPosition, screenToObj, vpr);
	       glRasterPos3fv(coord.getValue());
	       normalFont->drawString(label);
       ATOMLOOP_END

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
normalAtomLabelsSRAAO
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::normalAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();

       vpr = SoViewportRegionElement::get(state);

       float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomColor.getValues(0);
       glColor3fv(labelColors[0].getValue());
       ATOMLOOP_START(theAtomLabelIndex)
	       if (!doHydrogens &&
		       chemData->getAtomicNumber(theAtom) == 1) { 
		       continue; 
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       rad = chemRadii->atomRadii[theAtom];
	       rad *= radScale;
	       theScale.setValue(rad, rad, rad);
	       theMatrix.setTransform(coord, theRotation, theScale);
	       theMatrix.multVecMatrix(theZAxis, coord);
	       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);
	       charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       // Transform the screen-space starting position into object
	       // space, and feed that back to the glRasterPos command (which
	       // will turn around and transform it back into screen-space,
	       // but oh well).

	       coord = toObjectSpace(charPosition, screenToObj, vpr);
	       glRasterPos3fv(coord.getValue());
	       normalFont->drawString(label);
       ATOMLOOP_END

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
normalAtomLabelsSRAAA
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::normalAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();

       vpr = SoViewportRegionElement::get(state);

       float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomColor.getValues(0);
       ATOMLOOP_START(theAtomLabelIndex)
	       if (!doHydrogens &&
		       chemData->getAtomicNumber(theAtom) == 1) { 
		       continue; 
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       glColor3fv(labelColors[theAtom].getValue());
	       rad = chemRadii->atomRadii[theAtom];
	       rad *= radScale;
	       theScale.setValue(rad, rad, rad);
	       theMatrix.setTransform(coord, theRotation, theScale);
	       theMatrix.multVecMatrix(theZAxis, coord);
	       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);
	       charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       // Transform the screen-space starting position into object
	       // space, and feed that back to the glRasterPos command (which
	       // will turn around and transform it back into screen-space,
	       // but oh well).

	       coord = toObjectSpace(charPosition, screenToObj, vpr);
	       glRasterPos3fv(coord.getValue());
	       normalFont->drawString(label);
       ATOMLOOP_END

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
normalAtomLabelsSRAAI
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::normalAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();

       vpr = SoViewportRegionElement::get(state);

       float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();
       int32_t localAtomIndex;

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomColor.getValues(0);
       ATOMLOOP_START(theAtomLabelIndex)
	       if (!doHydrogens &&
		       chemData->getAtomicNumber(theAtom) == 1) { 
		       continue; 
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       localAtomIndex = chemData->getAtomIndex(theAtom);
	       glColor3fv(labelColors[localAtomIndex].getValue());
	       rad = chemRadii->atomRadii[theAtom];
	       rad *= radScale;
	       theScale.setValue(rad, rad, rad);
	       theMatrix.setTransform(coord, theRotation, theScale);
	       theMatrix.multVecMatrix(theZAxis, coord);
	       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);
	       charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       // Transform the screen-space starting position into object
	       // space, and feed that back to the glRasterPos command (which
	       // will turn around and transform it back into screen-space,
	       // but oh well).

	       coord = toObjectSpace(charPosition, screenToObj, vpr);
	       glRasterPos3fv(coord.getValue());
	       normalFont->drawString(label);
       ATOMLOOP_END

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
normalAtomLabelsSRILO
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::normalAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();

       vpr = SoViewportRegionElement::get(state);

       float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();
       int32_t localAtomIndex;

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomLabelColor.getValues(0);
       glColor3fv(labelColors[0].getValue());
       ATOMLOOP_START(theAtomLabelIndex)
	       if (!doHydrogens &&
		       chemData->getAtomicNumber(theAtom) == 1) { 
		       continue; 
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       localAtomIndex = chemData->getAtomIndex(theAtom);
	       rad = chemRadii->atomRadii[localAtomIndex];
	       rad *= radScale;
	       theScale.setValue(rad, rad, rad);
	       theMatrix.setTransform(coord, theRotation, theScale);
	       theMatrix.multVecMatrix(theZAxis, coord);
	       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);
	       charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       // Transform the screen-space starting position into object
	       // space, and feed that back to the glRasterPos command (which
	       // will turn around and transform it back into screen-space,
	       // but oh well).

	       coord = toObjectSpace(charPosition, screenToObj, vpr);
	       glRasterPos3fv(coord.getValue());
	       normalFont->drawString(label);
       ATOMLOOP_END

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
normalAtomLabelsSRILL
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::normalAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();

       vpr = SoViewportRegionElement::get(state);

       float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();
       int32_t localAtomIndex;

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomLabelColor.getValues(0);
       ATOMLOOP_START(theAtomLabelIndex)
	       if (!doHydrogens &&
		       chemData->getAtomicNumber(theAtom) == 1) { 
		       continue; 
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       glColor3fv(labelColors[theAtom].getValue());
	       localAtomIndex = chemData->getAtomIndex(theAtom);
	       rad = chemRadii->atomRadii[localAtomIndex];
	       rad *= radScale;
	       theScale.setValue(rad, rad, rad);
	       theMatrix.setTransform(coord, theRotation, theScale);
	       theMatrix.multVecMatrix(theZAxis, coord);
	       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);
	       charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       // Transform the screen-space starting position into object
	       // space, and feed that back to the glRasterPos command (which
	       // will turn around and transform it back into screen-space,
	       // but oh well).

	       coord = toObjectSpace(charPosition, screenToObj, vpr);
	       glRasterPos3fv(coord.getValue());
	       normalFont->drawString(label);
       ATOMLOOP_END

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
normalAtomLabelsSRILI
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::normalAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();

       vpr = SoViewportRegionElement::get(state);

       float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();
       int32_t localAtomIndex;

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomLabelColor.getValues(0);
       ATOMLOOP_START(theAtomLabelIndex)
	       if (!doHydrogens &&
		       chemData->getAtomicNumber(theAtom) == 1) { 
		       continue; 
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       localAtomIndex = chemData->getAtomIndex(theAtom);
	       glColor3fv(labelColors[localAtomIndex].getValue());
	       rad = chemRadii->atomRadii[localAtomIndex];
	       rad *= radScale;
	       theScale.setValue(rad, rad, rad);
	       theMatrix.setTransform(coord, theRotation, theScale);
	       theMatrix.multVecMatrix(theZAxis, coord);
	       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);
	       charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       // Transform the screen-space starting position into object
	       // space, and feed that back to the glRasterPos command (which
	       // will turn around and transform it back into screen-space,
	       // but oh well).

	       coord = toObjectSpace(charPosition, screenToObj, vpr);
	       glRasterPos3fv(coord.getValue());
	       normalFont->drawString(label);
       ATOMLOOP_END

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
normalAtomLabelsSRIAO
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::normalAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();

       vpr = SoViewportRegionElement::get(state);

       float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();
       int32_t localAtomIndex;

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomColor.getValues(0);
       glColor3fv(labelColors[0].getValue());
       ATOMLOOP_START(theAtomLabelIndex)
	       if (!doHydrogens &&
		       chemData->getAtomicNumber(theAtom) == 1) { 
		       continue; 
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       localAtomIndex = chemData->getAtomIndex(theAtom);
	       rad = chemRadii->atomRadii[localAtomIndex];
	       rad *= radScale;
	       theScale.setValue(rad, rad, rad);
	       theMatrix.setTransform(coord, theRotation, theScale);
	       theMatrix.multVecMatrix(theZAxis, coord);
	       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);
	       charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       // Transform the screen-space starting position into object
	       // space, and feed that back to the glRasterPos command (which
	       // will turn around and transform it back into screen-space,
	       // but oh well).

	       coord = toObjectSpace(charPosition, screenToObj, vpr);
	       glRasterPos3fv(coord.getValue());
	       normalFont->drawString(label);
       ATOMLOOP_END

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
normalAtomLabelsSRIAA
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::normalAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();

       vpr = SoViewportRegionElement::get(state);

       float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();
       int32_t localAtomIndex;

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomColor.getValues(0);
       ATOMLOOP_START(theAtomLabelIndex)
	       if (!doHydrogens &&
		       chemData->getAtomicNumber(theAtom) == 1) { 
		       continue; 
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       glColor3fv(labelColors[theAtom].getValue());
	       localAtomIndex = chemData->getAtomIndex(theAtom);
	       rad = chemRadii->atomRadii[localAtomIndex];
	       rad *= radScale;
	       theScale.setValue(rad, rad, rad);
	       theMatrix.setTransform(coord, theRotation, theScale);
	       theMatrix.multVecMatrix(theZAxis, coord);
	       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);
	       charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       // Transform the screen-space starting position into object
	       // space, and feed that back to the glRasterPos command (which
	       // will turn around and transform it back into screen-space,
	       // but oh well).

	       coord = toObjectSpace(charPosition, screenToObj, vpr);
	       glRasterPos3fv(coord.getValue());
	       normalFont->drawString(label);
       ATOMLOOP_END

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
normalAtomLabelsSRIAI
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::normalAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();

       vpr = SoViewportRegionElement::get(state);

       float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();
       int32_t localAtomIndex;

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomColor.getValues(0);
       ATOMLOOP_START(theAtomLabelIndex)
	       if (!doHydrogens &&
		       chemData->getAtomicNumber(theAtom) == 1) { 
		       continue; 
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       localAtomIndex = chemData->getAtomIndex(theAtom);
	       glColor3fv(labelColors[localAtomIndex].getValue());
	       rad = chemRadii->atomRadii[localAtomIndex];
	       rad *= radScale;
	       theScale.setValue(rad, rad, rad);
	       theMatrix.setTransform(coord, theRotation, theScale);
	       theMatrix.multVecMatrix(theZAxis, coord);
	       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);
	       charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       // Transform the screen-space starting position into object
	       // space, and feed that back to the glRasterPos command (which
	       // will turn around and transform it back into screen-space,
	       // but oh well).

	       coord = toObjectSpace(charPosition, screenToObj, vpr);
	       glRasterPos3fv(coord.getValue());
	       normalFont->drawString(label);
       ATOMLOOP_END

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}



////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
highlightAtomLabelsWLO
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::highlightAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();
       vpr = SoViewportRegionElement::get(state);

       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       //SbMatrix   theMatrix;
       //SbVec3f    theScale;
       //SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomLabelColor.getValues(0);
       glColor3fv(labelColors[0].getValue());
       SbColor highlightColor = cdp->highlightColor.getValue();
       int32_t highlightStyle = cdp->highlightStyle.getValue();

       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
	       glColor3fv(highlightColor.getValue());
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       normalFont->drawString(label);
	       ATOMLOOP_END
       }
       else {
	       ChemBitmapFontCache *theFont = normalFont;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		       if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			       cdp->highlightFontSize.getValue(), highlightFont, TRUE)) {
			       endRenderState(state, curPixelAlignment);
			       return;
		       }
		       highlightFont->setupToRender(state);
		       theFont = highlightFont;
	       }
	       SbColor theColor;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
		       theColor = labelColors[0] + highlightColor;
		       theColor.normalize();
		       glColor3fv(theColor.getValue());
	       }
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       // Transform the screen-space starting position into object
		       // space, and feed that back to the glRasterPos command (which
		       // will turn around and transform it back into screen-space,
		       // but oh well).

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       theFont->drawString(label);
	       ATOMLOOP_END
       }

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
highlightAtomLabelsWLL
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::highlightAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();
       vpr = SoViewportRegionElement::get(state);

       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       //SbMatrix   theMatrix;
       //SbRotation theRotation = currentRotation;
       //SbVec3f    theScale;
       //SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomLabelColor.getValues(0);
       SbColor highlightColor = cdp->highlightColor.getValue();
       int32_t highlightStyle = cdp->highlightStyle.getValue();

       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
	       glColor3fv(highlightColor.getValue());
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       normalFont->drawString(label);
	       ATOMLOOP_END
       }
       else {
	       ChemBitmapFontCache *theFont = normalFont;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		       if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			       cdp->highlightFontSize.getValue(), highlightFont, TRUE)) {
			       endRenderState(state, curPixelAlignment);
			       return;
		       }
		       highlightFont->setupToRender(state);
		       theFont = highlightFont;
	       }
	       SbColor theColor;
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			       theColor = labelColors[theAtom] + highlightColor;
			       theColor.normalize();
			       glColor3fv(theColor.getValue());
		       }
		       else {
			       glColor3fv(labelColors[theAtom].getValue());
		       }
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       // Transform the screen-space starting position into object
		       // space, and feed that back to the glRasterPos command (which
		       // will turn around and transform it back into screen-space,
		       // but oh well).

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       theFont->drawString(label);
	       ATOMLOOP_END
       }

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
highlightAtomLabelsWLI
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::highlightAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();
       vpr = SoViewportRegionElement::get(state);

       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       //SbMatrix   theMatrix;
       //SbVec3f    theScale;
       //SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();
       int32_t localAtomIndex;

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomLabelColor.getValues(0);
       SbColor highlightColor = cdp->highlightColor.getValue();
       int32_t highlightStyle = cdp->highlightStyle.getValue();

       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
	       glColor3fv(highlightColor.getValue());
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       normalFont->drawString(label);
	       ATOMLOOP_END
       }
       else {
	       ChemBitmapFontCache *theFont = normalFont;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		       if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			       cdp->highlightFontSize.getValue(), highlightFont, TRUE)) {
			       endRenderState(state, curPixelAlignment);
			       return;
		       }
		       highlightFont->setupToRender(state);
		       theFont = highlightFont;
	       }
	       SbColor theColor;
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       localAtomIndex = chemData->getAtomIndex(theAtom);
		       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			       theColor = labelColors[localAtomIndex] + highlightColor;
			       theColor.normalize();
			       glColor3fv(theColor.getValue());
		       }
		       else {
			       glColor3fv(labelColors[localAtomIndex].getValue());
		       }
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       // Transform the screen-space starting position into object
		       // space, and feed that back to the glRasterPos command (which
		       // will turn around and transform it back into screen-space,
		       // but oh well).

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       theFont->drawString(label);
	       ATOMLOOP_END
       }

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
highlightAtomLabelsWAO
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::highlightAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();
       vpr = SoViewportRegionElement::get(state);

       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       //SbMatrix   theMatrix;
       //SbVec3f    theScale;
       //SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomColor.getValues(0);
       glColor3fv(labelColors[0].getValue());
       SbColor highlightColor = cdp->highlightColor.getValue();
       int32_t highlightStyle = cdp->highlightStyle.getValue();

       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
	       glColor3fv(highlightColor.getValue());
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       normalFont->drawString(label);
	       ATOMLOOP_END
       }
       else {
	       ChemBitmapFontCache *theFont = normalFont;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		       if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			       cdp->highlightFontSize.getValue(), highlightFont, TRUE)) {
			       endRenderState(state, curPixelAlignment);
			       return;
		       }
		       highlightFont->setupToRender(state);
		       theFont = highlightFont;
	       }
	       SbColor theColor;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
		       theColor = labelColors[0] + highlightColor;
		       theColor.normalize();
		       glColor3fv(theColor.getValue());
	       }
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       // Transform the screen-space starting position into object
		       // space, and feed that back to the glRasterPos command (which
		       // will turn around and transform it back into screen-space,
		       // but oh well).

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       theFont->drawString(label);
	       ATOMLOOP_END
       }

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
highlightAtomLabelsWAA
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::highlightAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();
       vpr = SoViewportRegionElement::get(state);

       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       //SbMatrix   theMatrix;
       //SbVec3f    theScale;
       //SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomColor.getValues(0);
       SbColor highlightColor = cdp->highlightColor.getValue();
       int32_t highlightStyle = cdp->highlightStyle.getValue();

       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
	       glColor3fv(highlightColor.getValue());
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       normalFont->drawString(label);
	       ATOMLOOP_END
       }
       else {
	       ChemBitmapFontCache *theFont = normalFont;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		       if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			       cdp->highlightFontSize.getValue(), highlightFont, TRUE)) {
			       endRenderState(state, curPixelAlignment);
			       return;
		       }
		       highlightFont->setupToRender(state);
		       theFont = highlightFont;
	       }
	       SbColor theColor;
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			       theColor = labelColors[theAtom] + highlightColor;
			       theColor.normalize();
			       glColor3fv(theColor.getValue());
		       }
		       else {
			       glColor3fv(labelColors[theAtom].getValue());
		       }
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       // Transform the screen-space starting position into object
		       // space, and feed that back to the glRasterPos command (which
		       // will turn around and transform it back into screen-space,
		       // but oh well).

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       theFont->drawString(label);
	       ATOMLOOP_END
       }

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
highlightAtomLabelsWAI
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::highlightAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();
       vpr = SoViewportRegionElement::get(state);

       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       //SbMatrix   theMatrix;
       //SbVec3f    theScale;
       //SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();
       int32_t localAtomIndex;

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomColor.getValues(0);
       SbColor highlightColor = cdp->highlightColor.getValue();
       int32_t highlightStyle = cdp->highlightStyle.getValue();

       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
	       glColor3fv(highlightColor.getValue());
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       normalFont->drawString(label);
	       ATOMLOOP_END
       }
       else {
	       ChemBitmapFontCache *theFont = normalFont;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		       if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			       cdp->highlightFontSize.getValue(), highlightFont, TRUE)) {
			       endRenderState(state, curPixelAlignment);
			       return;
		       }
		       highlightFont->setupToRender(state);
		       theFont = highlightFont;
	       }
	       SbColor theColor;
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       localAtomIndex = chemData->getAtomIndex(theAtom);
		       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			       theColor = labelColors[localAtomIndex] + highlightColor;
			       theColor.normalize();
			       glColor3fv(theColor.getValue());
		       }
		       else {
			       glColor3fv(labelColors[localAtomIndex].getValue());
		       }
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       // Transform the screen-space starting position into object
		       // space, and feed that back to the glRasterPos command (which
		       // will turn around and transform it back into screen-space,
		       // but oh well).

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       theFont->drawString(label);
	       ATOMLOOP_END
       }

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
highlightAtomLabelsCLO
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::highlightAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();
       vpr = SoViewportRegionElement::get(state);

			 float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomLabelColor.getValues(0);
       glColor3fv(labelColors[0].getValue());
       SbColor highlightColor = cdp->highlightColor.getValue();
       int32_t highlightStyle = cdp->highlightStyle.getValue();

       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
	       glColor3fv(highlightColor.getValue());
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       rad = cdp->bondCylinderRadius.getValue();
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       normalFont->drawString(label);
	       ATOMLOOP_END
       }
       else {
	       ChemBitmapFontCache *theFont = normalFont;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		       if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			       cdp->highlightFontSize.getValue(), highlightFont, TRUE)) {
			       endRenderState(state, curPixelAlignment);
			       return;
		       }
		       highlightFont->setupToRender(state);
		       theFont = highlightFont;
	       }
	       SbColor theColor;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
		       theColor = labelColors[0] + highlightColor;
		       theColor.normalize();
		       glColor3fv(theColor.getValue());
	       }
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       rad = cdp->bondCylinderRadius.getValue();
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       // Transform the screen-space starting position into object
		       // space, and feed that back to the glRasterPos command (which
		       // will turn around and transform it back into screen-space,
		       // but oh well).

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       theFont->drawString(label);
	       ATOMLOOP_END
       }

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
highlightAtomLabelsCLL
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::highlightAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();
       vpr = SoViewportRegionElement::get(state);

			 float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomLabelColor.getValues(0);
       SbColor highlightColor = cdp->highlightColor.getValue();
       int32_t highlightStyle = cdp->highlightStyle.getValue();

       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
	       glColor3fv(highlightColor.getValue());
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       rad = cdp->bondCylinderRadius.getValue();
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       normalFont->drawString(label);
	       ATOMLOOP_END
       }
       else {
	       ChemBitmapFontCache *theFont = normalFont;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		       if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			       cdp->highlightFontSize.getValue(), highlightFont, TRUE)) {
			       endRenderState(state, curPixelAlignment);
			       return;
		       }
		       highlightFont->setupToRender(state);
		       theFont = highlightFont;
	       }
	       SbColor theColor;
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			       theColor = labelColors[theAtom] + highlightColor;
			       theColor.normalize();
			       glColor3fv(theColor.getValue());
		       }
		       else {
			       glColor3fv(labelColors[theAtom].getValue());
		       }
		       rad = cdp->bondCylinderRadius.getValue();
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       // Transform the screen-space starting position into object
		       // space, and feed that back to the glRasterPos command (which
		       // will turn around and transform it back into screen-space,
		       // but oh well).

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       theFont->drawString(label);
	       ATOMLOOP_END
       }

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
highlightAtomLabelsCLI
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::highlightAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();
       vpr = SoViewportRegionElement::get(state);

			 float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();
       int32_t localAtomIndex;

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomLabelColor.getValues(0);
       SbColor highlightColor = cdp->highlightColor.getValue();
       int32_t highlightStyle = cdp->highlightStyle.getValue();

       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
	       glColor3fv(highlightColor.getValue());
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       rad = cdp->bondCylinderRadius.getValue();
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       normalFont->drawString(label);
	       ATOMLOOP_END
       }
       else {
	       ChemBitmapFontCache *theFont = normalFont;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		       if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			       cdp->highlightFontSize.getValue(), highlightFont, TRUE)) {
			       endRenderState(state, curPixelAlignment);
			       return;
		       }
		       highlightFont->setupToRender(state);
		       theFont = highlightFont;
	       }
	       SbColor theColor;
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       localAtomIndex = chemData->getAtomIndex(theAtom);
		       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			       theColor = labelColors[localAtomIndex] + highlightColor;
			       theColor.normalize();
			       glColor3fv(theColor.getValue());
		       }
		       else {
			       glColor3fv(labelColors[localAtomIndex].getValue());
		       }
		       rad = cdp->bondCylinderRadius.getValue();
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       // Transform the screen-space starting position into object
		       // space, and feed that back to the glRasterPos command (which
		       // will turn around and transform it back into screen-space,
		       // but oh well).

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       theFont->drawString(label);
	       ATOMLOOP_END
       }

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
highlightAtomLabelsCAO
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::highlightAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();
       vpr = SoViewportRegionElement::get(state);

			 float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomColor.getValues(0);
       glColor3fv(labelColors[0].getValue());
       SbColor highlightColor = cdp->highlightColor.getValue();
       int32_t highlightStyle = cdp->highlightStyle.getValue();

       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
	       glColor3fv(highlightColor.getValue());
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       rad = cdp->bondCylinderRadius.getValue();
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       normalFont->drawString(label);
	       ATOMLOOP_END
       }
       else {
	       ChemBitmapFontCache *theFont = normalFont;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		       if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			       cdp->highlightFontSize.getValue(), highlightFont, TRUE)) {
			       endRenderState(state, curPixelAlignment);
			       return;
		       }
		       highlightFont->setupToRender(state);
		       theFont = highlightFont;
	       }
	       SbColor theColor;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
		       theColor = labelColors[0] + highlightColor;
		       theColor.normalize();
		       glColor3fv(theColor.getValue());
	       }
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       rad = cdp->bondCylinderRadius.getValue();
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       // Transform the screen-space starting position into object
		       // space, and feed that back to the glRasterPos command (which
		       // will turn around and transform it back into screen-space,
		       // but oh well).

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       theFont->drawString(label);
	       ATOMLOOP_END
       }

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
highlightAtomLabelsCAA
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::highlightAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();
       vpr = SoViewportRegionElement::get(state);

			 float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomColor.getValues(0);
       SbColor highlightColor = cdp->highlightColor.getValue();
       int32_t highlightStyle = cdp->highlightStyle.getValue();

       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
	       glColor3fv(highlightColor.getValue());
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       rad = cdp->bondCylinderRadius.getValue();
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       normalFont->drawString(label);
	       ATOMLOOP_END
       }
       else {
	       ChemBitmapFontCache *theFont = normalFont;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		       if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			       cdp->highlightFontSize.getValue(), highlightFont, TRUE)) {
			       endRenderState(state, curPixelAlignment);
			       return;
		       }
		       highlightFont->setupToRender(state);
		       theFont = highlightFont;
	       }
	       SbColor theColor;
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			       theColor = labelColors[theAtom] + highlightColor;
			       theColor.normalize();
			       glColor3fv(theColor.getValue());
		       }
		       else {
			       glColor3fv(labelColors[theAtom].getValue());
		       }
		       rad = cdp->bondCylinderRadius.getValue();
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       // Transform the screen-space starting position into object
		       // space, and feed that back to the glRasterPos command (which
		       // will turn around and transform it back into screen-space,
		       // but oh well).

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       theFont->drawString(label);
	       ATOMLOOP_END
       }

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
highlightAtomLabelsCAI
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::highlightAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();
       vpr = SoViewportRegionElement::get(state);

			 float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();
       int32_t localAtomIndex;

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomColor.getValues(0);
       SbColor highlightColor = cdp->highlightColor.getValue();
       int32_t highlightStyle = cdp->highlightStyle.getValue();

       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
	       glColor3fv(highlightColor.getValue());
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       rad = cdp->bondCylinderRadius.getValue();
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       normalFont->drawString(label);
	       ATOMLOOP_END
       }
       else {
	       ChemBitmapFontCache *theFont = normalFont;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		       if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			       cdp->highlightFontSize.getValue(), highlightFont, TRUE)) {
			       endRenderState(state, curPixelAlignment);
			       return;
		       }
		       highlightFont->setupToRender(state);
		       theFont = highlightFont;
	       }
	       SbColor theColor;
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       localAtomIndex = chemData->getAtomIndex(theAtom);
		       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			       theColor = labelColors[localAtomIndex] + highlightColor;
			       theColor.normalize();
			       glColor3fv(theColor.getValue());
		       }
		       else {
			       glColor3fv(labelColors[localAtomIndex].getValue());
		       }
		       rad = cdp->bondCylinderRadius.getValue();
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       // Transform the screen-space starting position into object
		       // space, and feed that back to the glRasterPos command (which
		       // will turn around and transform it back into screen-space,
		       // but oh well).

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       theFont->drawString(label);
	       ATOMLOOP_END
       }

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
highlightAtomLabelsSROLO
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::highlightAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();
       vpr = SoViewportRegionElement::get(state);

			 float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomLabelColor.getValues(0);
       glColor3fv(labelColors[0].getValue());
       SbColor highlightColor = cdp->highlightColor.getValue();
       int32_t highlightStyle = cdp->highlightStyle.getValue();

       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
	       glColor3fv(highlightColor.getValue());
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       rad = chemRadii->atomRadii[0];
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       normalFont->drawString(label);
	       ATOMLOOP_END
       }
       else {
	       ChemBitmapFontCache *theFont = normalFont;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		       if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			       cdp->highlightFontSize.getValue(), highlightFont, TRUE)) {
			       endRenderState(state, curPixelAlignment);
			       return;
		       }
		       highlightFont->setupToRender(state);
		       theFont = highlightFont;
	       }
	       SbColor theColor;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
		       theColor = labelColors[0] + highlightColor;
		       theColor.normalize();
		       glColor3fv(theColor.getValue());
	       }
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       rad = chemRadii->atomRadii[0];
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       // Transform the screen-space starting position into object
		       // space, and feed that back to the glRasterPos command (which
		       // will turn around and transform it back into screen-space,
		       // but oh well).

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       theFont->drawString(label);
	       ATOMLOOP_END
       }

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
highlightAtomLabelsSROLL
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::highlightAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();
       vpr = SoViewportRegionElement::get(state);

			 float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomLabelColor.getValues(0);
       SbColor highlightColor = cdp->highlightColor.getValue();
       int32_t highlightStyle = cdp->highlightStyle.getValue();

       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
	       glColor3fv(highlightColor.getValue());
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       rad = chemRadii->atomRadii[0];
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       normalFont->drawString(label);
	       ATOMLOOP_END
       }
       else {
	       ChemBitmapFontCache *theFont = normalFont;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		       if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			       cdp->highlightFontSize.getValue(), highlightFont, TRUE)) {
			       endRenderState(state, curPixelAlignment);
			       return;
		       }
		       highlightFont->setupToRender(state);
		       theFont = highlightFont;
	       }
	       SbColor theColor;
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			       theColor = labelColors[theAtom] + highlightColor;
			       theColor.normalize();
			       glColor3fv(theColor.getValue());
		       }
		       else {
			       glColor3fv(labelColors[theAtom].getValue());
		       }
		       rad = chemRadii->atomRadii[0];
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       // Transform the screen-space starting position into object
		       // space, and feed that back to the glRasterPos command (which
		       // will turn around and transform it back into screen-space,
		       // but oh well).

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       theFont->drawString(label);
	       ATOMLOOP_END
       }

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
highlightAtomLabelsSROLI
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::highlightAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();
       vpr = SoViewportRegionElement::get(state);

			 float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();
       int32_t localAtomIndex;

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomLabelColor.getValues(0);
       SbColor highlightColor = cdp->highlightColor.getValue();
       int32_t highlightStyle = cdp->highlightStyle.getValue();

       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
	       glColor3fv(highlightColor.getValue());
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       rad = chemRadii->atomRadii[0];
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       normalFont->drawString(label);
	       ATOMLOOP_END
       }
       else {
	       ChemBitmapFontCache *theFont = normalFont;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		       if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			       cdp->highlightFontSize.getValue(), highlightFont, TRUE)) {
			       endRenderState(state, curPixelAlignment);
			       return;
		       }
		       highlightFont->setupToRender(state);
		       theFont = highlightFont;
	       }
	       SbColor theColor;
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       localAtomIndex = chemData->getAtomIndex(theAtom);
		       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			       theColor = labelColors[localAtomIndex] + highlightColor;
			       theColor.normalize();
			       glColor3fv(theColor.getValue());
		       }
		       else {
			       glColor3fv(labelColors[localAtomIndex].getValue());
		       }
		       rad = chemRadii->atomRadii[0];
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       // Transform the screen-space starting position into object
		       // space, and feed that back to the glRasterPos command (which
		       // will turn around and transform it back into screen-space,
		       // but oh well).

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       theFont->drawString(label);
	       ATOMLOOP_END
       }

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
highlightAtomLabelsSROAO
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::highlightAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();
       vpr = SoViewportRegionElement::get(state);

			 float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomColor.getValues(0);
       glColor3fv(labelColors[0].getValue());
       SbColor highlightColor = cdp->highlightColor.getValue();
       int32_t highlightStyle = cdp->highlightStyle.getValue();

       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
	       glColor3fv(highlightColor.getValue());
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       rad = chemRadii->atomRadii[0];
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       normalFont->drawString(label);
	       ATOMLOOP_END
       }
       else {
	       ChemBitmapFontCache *theFont = normalFont;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		       if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			       cdp->highlightFontSize.getValue(), highlightFont, TRUE)) {
			       endRenderState(state, curPixelAlignment);
			       return;
		       }
		       highlightFont->setupToRender(state);
		       theFont = highlightFont;
	       }
	       SbColor theColor;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
		       theColor = labelColors[0] + highlightColor;
		       theColor.normalize();
		       glColor3fv(theColor.getValue());
	       }
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       rad = chemRadii->atomRadii[0];
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       // Transform the screen-space starting position into object
		       // space, and feed that back to the glRasterPos command (which
		       // will turn around and transform it back into screen-space,
		       // but oh well).

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       theFont->drawString(label);
	       ATOMLOOP_END
       }

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
highlightAtomLabelsSROAA
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::highlightAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();
       vpr = SoViewportRegionElement::get(state);

			 float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomColor.getValues(0);
       SbColor highlightColor = cdp->highlightColor.getValue();
       int32_t highlightStyle = cdp->highlightStyle.getValue();

       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
	       glColor3fv(highlightColor.getValue());
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       rad = chemRadii->atomRadii[0];
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       normalFont->drawString(label);
	       ATOMLOOP_END
       }
       else {
	       ChemBitmapFontCache *theFont = normalFont;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		       if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			       cdp->highlightFontSize.getValue(), highlightFont, TRUE)) {
			       endRenderState(state, curPixelAlignment);
			       return;
		       }
		       highlightFont->setupToRender(state);
		       theFont = highlightFont;
	       }
	       SbColor theColor;
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			       theColor = labelColors[theAtom] + highlightColor;
			       theColor.normalize();
			       glColor3fv(theColor.getValue());
		       }
		       else {
			       glColor3fv(labelColors[theAtom].getValue());
		       }
		       rad = chemRadii->atomRadii[0];
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       // Transform the screen-space starting position into object
		       // space, and feed that back to the glRasterPos command (which
		       // will turn around and transform it back into screen-space,
		       // but oh well).

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       theFont->drawString(label);
	       ATOMLOOP_END
       }

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
highlightAtomLabelsSROAI
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::highlightAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();
       vpr = SoViewportRegionElement::get(state);

			 float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();
       int32_t localAtomIndex;

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomColor.getValues(0);
       SbColor highlightColor = cdp->highlightColor.getValue();
       int32_t highlightStyle = cdp->highlightStyle.getValue();

       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
	       glColor3fv(highlightColor.getValue());
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       rad = chemRadii->atomRadii[0];
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       normalFont->drawString(label);
	       ATOMLOOP_END
       }
       else {
	       ChemBitmapFontCache *theFont = normalFont;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		       if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			       cdp->highlightFontSize.getValue(), highlightFont, TRUE)) {
			       endRenderState(state, curPixelAlignment);
			       return;
		       }
		       highlightFont->setupToRender(state);
		       theFont = highlightFont;
	       }
	       SbColor theColor;
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       localAtomIndex = chemData->getAtomIndex(theAtom);
		       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			       theColor = labelColors[localAtomIndex] + highlightColor;
			       theColor.normalize();
			       glColor3fv(theColor.getValue());
		       }
		       else {
			       glColor3fv(labelColors[localAtomIndex].getValue());
		       }
		       rad = chemRadii->atomRadii[0];
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       // Transform the screen-space starting position into object
		       // space, and feed that back to the glRasterPos command (which
		       // will turn around and transform it back into screen-space,
		       // but oh well).

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       theFont->drawString(label);
	       ATOMLOOP_END
       }

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
highlightAtomLabelsSRALO
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::highlightAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();
       vpr = SoViewportRegionElement::get(state);

			 float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomLabelColor.getValues(0);
       glColor3fv(labelColors[0].getValue());
       SbColor highlightColor = cdp->highlightColor.getValue();
       int32_t highlightStyle = cdp->highlightStyle.getValue();

       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
	       glColor3fv(highlightColor.getValue());
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       rad = chemRadii->atomRadii[theAtom];
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       normalFont->drawString(label);
	       ATOMLOOP_END
       }
       else {
	       ChemBitmapFontCache *theFont = normalFont;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		       if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			       cdp->highlightFontSize.getValue(), highlightFont, TRUE)) {
			       endRenderState(state, curPixelAlignment);
			       return;
		       }
		       highlightFont->setupToRender(state);
		       theFont = highlightFont;
	       }
	       SbColor theColor;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
		       theColor = labelColors[0] + highlightColor;
		       theColor.normalize();
		       glColor3fv(theColor.getValue());
	       }
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       rad = chemRadii->atomRadii[theAtom];
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       // Transform the screen-space starting position into object
		       // space, and feed that back to the glRasterPos command (which
		       // will turn around and transform it back into screen-space,
		       // but oh well).

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       theFont->drawString(label);
	       ATOMLOOP_END
       }

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
highlightAtomLabelsSRALL
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::highlightAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();
       vpr = SoViewportRegionElement::get(state);

			 float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomLabelColor.getValues(0);
       SbColor highlightColor = cdp->highlightColor.getValue();
       int32_t highlightStyle = cdp->highlightStyle.getValue();

       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
	       glColor3fv(highlightColor.getValue());
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       rad = chemRadii->atomRadii[theAtom];
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       normalFont->drawString(label);
	       ATOMLOOP_END
       }
       else {
	       ChemBitmapFontCache *theFont = normalFont;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		       if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			       cdp->highlightFontSize.getValue(), highlightFont, TRUE)) {
			       endRenderState(state, curPixelAlignment);
			       return;
		       }
		       highlightFont->setupToRender(state);
		       theFont = highlightFont;
	       }
	       SbColor theColor;
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			       theColor = labelColors[theAtom] + highlightColor;
			       theColor.normalize();
			       glColor3fv(theColor.getValue());
		       }
		       else {
			       glColor3fv(labelColors[theAtom].getValue());
		       }
		       rad = chemRadii->atomRadii[theAtom];
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       // Transform the screen-space starting position into object
		       // space, and feed that back to the glRasterPos command (which
		       // will turn around and transform it back into screen-space,
		       // but oh well).

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       theFont->drawString(label);
	       ATOMLOOP_END
       }

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
highlightAtomLabelsSRALI
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::highlightAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();
       vpr = SoViewportRegionElement::get(state);

			 float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();
       int32_t localAtomIndex;

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomLabelColor.getValues(0);
       SbColor highlightColor = cdp->highlightColor.getValue();
       int32_t highlightStyle = cdp->highlightStyle.getValue();

       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
	       glColor3fv(highlightColor.getValue());
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       rad = chemRadii->atomRadii[theAtom];
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       normalFont->drawString(label);
	       ATOMLOOP_END
       }
       else {
	       ChemBitmapFontCache *theFont = normalFont;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		       if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			       cdp->highlightFontSize.getValue(), highlightFont, TRUE)) {
			       endRenderState(state, curPixelAlignment);
			       return;
		       }
		       highlightFont->setupToRender(state);
		       theFont = highlightFont;
	       }
	       SbColor theColor;
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       localAtomIndex = chemData->getAtomIndex(theAtom);
		       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			       theColor = labelColors[localAtomIndex] + highlightColor;
			       theColor.normalize();
			       glColor3fv(theColor.getValue());
		       }
		       else {
			       glColor3fv(labelColors[localAtomIndex].getValue());
		       }
		       rad = chemRadii->atomRadii[theAtom];
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       // Transform the screen-space starting position into object
		       // space, and feed that back to the glRasterPos command (which
		       // will turn around and transform it back into screen-space,
		       // but oh well).

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       theFont->drawString(label);
	       ATOMLOOP_END
       }

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
highlightAtomLabelsSRAAO
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::highlightAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();
       vpr = SoViewportRegionElement::get(state);

			 float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomColor.getValues(0);
       glColor3fv(labelColors[0].getValue());
       SbColor highlightColor = cdp->highlightColor.getValue();
       int32_t highlightStyle = cdp->highlightStyle.getValue();

       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
	       glColor3fv(highlightColor.getValue());
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       rad = chemRadii->atomRadii[theAtom];
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       normalFont->drawString(label);
	       ATOMLOOP_END
       }
       else {
	       ChemBitmapFontCache *theFont = normalFont;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		       if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			       cdp->highlightFontSize.getValue(), highlightFont, TRUE)) {
			       endRenderState(state, curPixelAlignment);
			       return;
		       }
		       highlightFont->setupToRender(state);
		       theFont = highlightFont;
	       }
	       SbColor theColor;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
		       theColor = labelColors[0] + highlightColor;
		       theColor.normalize();
		       glColor3fv(theColor.getValue());
	       }
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       rad = chemRadii->atomRadii[theAtom];
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       // Transform the screen-space starting position into object
		       // space, and feed that back to the glRasterPos command (which
		       // will turn around and transform it back into screen-space,
		       // but oh well).

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       theFont->drawString(label);
	       ATOMLOOP_END
       }

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
highlightAtomLabelsSRAAA
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::highlightAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();
       vpr = SoViewportRegionElement::get(state);

			 float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomColor.getValues(0);
       SbColor highlightColor = cdp->highlightColor.getValue();
       int32_t highlightStyle = cdp->highlightStyle.getValue();

       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
	       glColor3fv(highlightColor.getValue());
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       rad = chemRadii->atomRadii[theAtom];
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       normalFont->drawString(label);
	       ATOMLOOP_END
       }
       else {
	       ChemBitmapFontCache *theFont = normalFont;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		       if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			       cdp->highlightFontSize.getValue(), highlightFont, TRUE)) {
			       endRenderState(state, curPixelAlignment);
			       return;
		       }
		       highlightFont->setupToRender(state);
		       theFont = highlightFont;
	       }
	       SbColor theColor;
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			       theColor = labelColors[theAtom] + highlightColor;
			       theColor.normalize();
			       glColor3fv(theColor.getValue());
		       }
		       else {
			       glColor3fv(labelColors[theAtom].getValue());
		       }
		       rad = chemRadii->atomRadii[theAtom];
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       // Transform the screen-space starting position into object
		       // space, and feed that back to the glRasterPos command (which
		       // will turn around and transform it back into screen-space,
		       // but oh well).

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       theFont->drawString(label);
	       ATOMLOOP_END
       }

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
highlightAtomLabelsSRAAI
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::highlightAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();
       vpr = SoViewportRegionElement::get(state);

			 float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();
       int32_t localAtomIndex;

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomColor.getValues(0);
       SbColor highlightColor = cdp->highlightColor.getValue();
       int32_t highlightStyle = cdp->highlightStyle.getValue();

       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
	       glColor3fv(highlightColor.getValue());
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       rad = chemRadii->atomRadii[theAtom];
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       normalFont->drawString(label);
	       ATOMLOOP_END
       }
       else {
	       ChemBitmapFontCache *theFont = normalFont;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		       if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			       cdp->highlightFontSize.getValue(), highlightFont, TRUE)) {
			       endRenderState(state, curPixelAlignment);
			       return;
		       }
		       highlightFont->setupToRender(state);
		       theFont = highlightFont;
	       }
	       SbColor theColor;
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       localAtomIndex = chemData->getAtomIndex(theAtom);
		       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			       theColor = labelColors[localAtomIndex] + highlightColor;
			       theColor.normalize();
			       glColor3fv(theColor.getValue());
		       }
		       else {
			       glColor3fv(labelColors[localAtomIndex].getValue());
		       }
		       rad = chemRadii->atomRadii[theAtom];
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       // Transform the screen-space starting position into object
		       // space, and feed that back to the glRasterPos command (which
		       // will turn around and transform it back into screen-space,
		       // but oh well).

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       theFont->drawString(label);
	       ATOMLOOP_END
       }

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
highlightAtomLabelsSRILO
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::highlightAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();
       vpr = SoViewportRegionElement::get(state);

			 float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();
       int32_t localAtomIndex;

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomLabelColor.getValues(0);
       glColor3fv(labelColors[0].getValue());
       SbColor highlightColor = cdp->highlightColor.getValue();
       int32_t highlightStyle = cdp->highlightStyle.getValue();

       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
	       glColor3fv(highlightColor.getValue());
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       localAtomIndex = chemData->getAtomIndex(theAtom);
		       rad = chemRadii->atomRadii[localAtomIndex];
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       normalFont->drawString(label);
	       ATOMLOOP_END
       }
       else {
	       ChemBitmapFontCache *theFont = normalFont;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		       if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			       cdp->highlightFontSize.getValue(), highlightFont, TRUE)) {
			       endRenderState(state, curPixelAlignment);
			       return;
		       }
		       highlightFont->setupToRender(state);
		       theFont = highlightFont;
	       }
	       SbColor theColor;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
		       theColor = labelColors[0] + highlightColor;
		       theColor.normalize();
		       glColor3fv(theColor.getValue());
	       }
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       localAtomIndex = chemData->getAtomIndex(theAtom);
		       rad = chemRadii->atomRadii[localAtomIndex];
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       // Transform the screen-space starting position into object
		       // space, and feed that back to the glRasterPos command (which
		       // will turn around and transform it back into screen-space,
		       // but oh well).

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       theFont->drawString(label);
	       ATOMLOOP_END
       }

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
highlightAtomLabelsSRILL
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::highlightAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();
       vpr = SoViewportRegionElement::get(state);

			 float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();
       int32_t localAtomIndex;

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomLabelColor.getValues(0);
       SbColor highlightColor = cdp->highlightColor.getValue();
       int32_t highlightStyle = cdp->highlightStyle.getValue();

       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
	       glColor3fv(highlightColor.getValue());
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       localAtomIndex = chemData->getAtomIndex(theAtom);
		       rad = chemRadii->atomRadii[localAtomIndex];
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       normalFont->drawString(label);
	       ATOMLOOP_END
       }
       else {
	       ChemBitmapFontCache *theFont = normalFont;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		       if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			       cdp->highlightFontSize.getValue(), highlightFont, TRUE)) {
			       endRenderState(state, curPixelAlignment);
			       return;
		       }
		       highlightFont->setupToRender(state);
		       theFont = highlightFont;
	       }
	       SbColor theColor;
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			       theColor = labelColors[theAtom] + highlightColor;
			       theColor.normalize();
			       glColor3fv(theColor.getValue());
		       }
		       else {
			       glColor3fv(labelColors[theAtom].getValue());
		       }
		       localAtomIndex = chemData->getAtomIndex(theAtom);
		       rad = chemRadii->atomRadii[localAtomIndex];
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       // Transform the screen-space starting position into object
		       // space, and feed that back to the glRasterPos command (which
		       // will turn around and transform it back into screen-space,
		       // but oh well).

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       theFont->drawString(label);
	       ATOMLOOP_END
       }

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
highlightAtomLabelsSRILI
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::highlightAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();
       vpr = SoViewportRegionElement::get(state);

			 float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();
       int32_t localAtomIndex;

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomLabelColor.getValues(0);
       SbColor highlightColor = cdp->highlightColor.getValue();
       int32_t highlightStyle = cdp->highlightStyle.getValue();

       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
	       glColor3fv(highlightColor.getValue());
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       localAtomIndex = chemData->getAtomIndex(theAtom);
		       rad = chemRadii->atomRadii[localAtomIndex];
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       normalFont->drawString(label);
	       ATOMLOOP_END
       }
       else {
	       ChemBitmapFontCache *theFont = normalFont;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		       if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			       cdp->highlightFontSize.getValue(), highlightFont, TRUE)) {
			       endRenderState(state, curPixelAlignment);
			       return;
		       }
		       highlightFont->setupToRender(state);
		       theFont = highlightFont;
	       }
	       SbColor theColor;
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       localAtomIndex = chemData->getAtomIndex(theAtom);
		       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			       theColor = labelColors[localAtomIndex] + highlightColor;
			       theColor.normalize();
			       glColor3fv(theColor.getValue());
		       }
		       else {
			       glColor3fv(labelColors[localAtomIndex].getValue());
		       }
		       rad = chemRadii->atomRadii[localAtomIndex];
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       // Transform the screen-space starting position into object
		       // space, and feed that back to the glRasterPos command (which
		       // will turn around and transform it back into screen-space,
		       // but oh well).

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       theFont->drawString(label);
	       ATOMLOOP_END
       }

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
highlightAtomLabelsSRIAO
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::highlightAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();
       vpr = SoViewportRegionElement::get(state);

			 float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();
       int32_t localAtomIndex;

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomColor.getValues(0);
       glColor3fv(labelColors[0].getValue());
       SbColor highlightColor = cdp->highlightColor.getValue();
       int32_t highlightStyle = cdp->highlightStyle.getValue();

       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
	       glColor3fv(highlightColor.getValue());
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       localAtomIndex = chemData->getAtomIndex(theAtom);
		       rad = chemRadii->atomRadii[localAtomIndex];
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       normalFont->drawString(label);
	       ATOMLOOP_END
       }
       else {
	       ChemBitmapFontCache *theFont = normalFont;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		       if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			       cdp->highlightFontSize.getValue(), highlightFont, TRUE)) {
			       endRenderState(state, curPixelAlignment);
			       return;
		       }
		       highlightFont->setupToRender(state);
		       theFont = highlightFont;
	       }
	       SbColor theColor;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
		       theColor = labelColors[0] + highlightColor;
		       theColor.normalize();
		       glColor3fv(theColor.getValue());
	       }
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       localAtomIndex = chemData->getAtomIndex(theAtom);
		       rad = chemRadii->atomRadii[localAtomIndex];
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       // Transform the screen-space starting position into object
		       // space, and feed that back to the glRasterPos command (which
		       // will turn around and transform it back into screen-space,
		       // but oh well).

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       theFont->drawString(label);
	       ATOMLOOP_END
       }

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
highlightAtomLabelsSRIAA
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::highlightAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();
       vpr = SoViewportRegionElement::get(state);

			 float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();
       int32_t localAtomIndex;

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomColor.getValues(0);
       SbColor highlightColor = cdp->highlightColor.getValue();
       int32_t highlightStyle = cdp->highlightStyle.getValue();

       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
	       glColor3fv(highlightColor.getValue());
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       localAtomIndex = chemData->getAtomIndex(theAtom);
		       rad = chemRadii->atomRadii[localAtomIndex];
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       normalFont->drawString(label);
	       ATOMLOOP_END
       }
       else {
	       ChemBitmapFontCache *theFont = normalFont;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		       if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			       cdp->highlightFontSize.getValue(), highlightFont, TRUE)) {
			       endRenderState(state, curPixelAlignment);
			       return;
		       }
		       highlightFont->setupToRender(state);
		       theFont = highlightFont;
	       }
	       SbColor theColor;
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			       theColor = labelColors[theAtom] + highlightColor;
			       theColor.normalize();
			       glColor3fv(theColor.getValue());
		       }
		       else {
			       glColor3fv(labelColors[theAtom].getValue());
		       }
		       localAtomIndex = chemData->getAtomIndex(theAtom);
		       rad = chemRadii->atomRadii[localAtomIndex];
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       // Transform the screen-space starting position into object
		       // space, and feed that back to the glRasterPos command (which
		       // will turn around and transform it back into screen-space,
		       // but oh well).

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       theFont->drawString(label);
	       ATOMLOOP_END
       }

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights atom labels with appropriate offsets depending on the
//    ChemDisplayParam atomLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
highlightAtomLabelsSRIAI
       (SoGLRenderAction *action, const MFVec2i &theAtomLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::highlightAtomLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theAtomLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();
       vpr = SoViewportRegionElement::get(state);

			 float rad = 0.0f;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool     doHydrogens = cdp->showHydrogens.getValue();
       int32_t localAtomIndex;

       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       const SbColor *labelColors = chemColor->atomColor.getValues(0);
       SbColor highlightColor = cdp->highlightColor.getValue();
       int32_t highlightStyle = cdp->highlightStyle.getValue();

       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
	       glColor3fv(highlightColor.getValue());
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       localAtomIndex = chemData->getAtomIndex(theAtom);
		       rad = chemRadii->atomRadii[localAtomIndex];
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       normalFont->drawString(label);
	       ATOMLOOP_END
       }
       else {
	       ChemBitmapFontCache *theFont = normalFont;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		       if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			       cdp->highlightFontSize.getValue(), highlightFont, TRUE)) {
			       endRenderState(state, curPixelAlignment);
			       return;
		       }
		       highlightFont->setupToRender(state);
		       theFont = highlightFont;
	       }
	       SbColor theColor;
	       ATOMLOOP_START(theAtomLabelIndex)
		       if (!doHydrogens &&
			       chemData->getAtomicNumber(theAtom) == 1) { 
			       continue; 
		       }
		       coord = chemData->getAtomCoordinates(theAtom);
		       localAtomIndex = chemData->getAtomIndex(theAtom);
		       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			       theColor = labelColors[localAtomIndex] + highlightColor;
			       theColor.normalize();
			       glColor3fv(theColor.getValue());
		       }
		       else {
			       glColor3fv(labelColors[localAtomIndex].getValue());
		       }
		       rad = chemRadii->atomRadii[localAtomIndex];
		       rad *= radScale;
		       theScale.setValue(rad, rad, rad);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		       parseAtomLabel(labelFormat, theAtom, chemData, label);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       // Transform the screen-space starting position into object
		       // space, and feed that back to the glRasterPos command (which
		       // will turn around and transform it back into screen-space,
		       // but oh well).

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       theFont->drawString(label);
	       ATOMLOOP_END
       }

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Renders bond labels when wireframe is used
//
// Use: protected

void
ChemDisplay::renderBondLabelsWithWireframe(SoGLRenderAction *action)
//
////////////////////////////////////////////////////////////////////////
{
// --> bond labels clipping
	if (clipBonds) computeBondLabelsCulling(action);
// <-- bond labels clipping

	if (normalBondLabelIndex.getNum() > 0) {
		normalBondLabelsWithWireframe(action, vnormalBondLabelIndex);
	}
	if (highlightBondLabelIndex.getNum() > 0) {
		highlightBondLabelsWithWireframe(action, vhighlightBondLabelIndex);
	}
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Renders bond labels when cylinders are used
//
// Use: protected

void
ChemDisplay::renderBondLabelsWithCylinders(SoGLRenderAction *action)
//
////////////////////////////////////////////////////////////////////////
{
// --> bond labels clipping
	if (clipBonds) computeBondLabelsCulling(action);
// <-- bond labels clipping

	if (normalBondLabelIndex.getNum() > 0) {
		normalBondLabelsWithCylinders(action, vnormalBondLabelIndex);
	}
	if (highlightBondLabelIndex.getNum() > 0) {
		highlightBondLabelsWithCylinders(action, vhighlightBondLabelIndex);
	}
}



////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders bond labels with appropriate offsets depending on the
//    ChemDisplayParam bondLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
normalBondLabelsWithWireframe
       (SoGLRenderAction *action, const MFVec2i &theBondLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::renderBondLabels\n");
#endif

       // Check to see if there is anything to display.
       if (theBondLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);

       short lrJust = cdp->bondLabelLeftRightJustification.getValue();
       short tbJust = cdp->bondLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();
       vpr = SoViewportRegionElement::get(state);

       SbBool doHydrogens = cdp->showHydrogens.getValue();

       //SbMatrix   theMatrix;
       //SbVec3f    theScale;
       //SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       const char *labelFormat = cdp->bondLabelString.getValue().getString();
       if (chemColor->bondLabelColorBinding.getValue() == 
	       ChemColor::BOND_LABEL_OVERALL) {
	       const SbColor *labelColors = chemColor->bondLabelColor.getValues(0);

	       glColor3fv(labelColors[0].getValue());
	       BONDLOOP_START(theBondLabelIndex)
		       int32_t from = chemData->getBondFrom(theBond);
		       int32_t to = chemData->getBondTo(theBond);
		       if (!doHydrogens) {
			       if ((chemData->getAtomicNumber(from) == 1) ||
				       (chemData->getAtomicNumber(to) == 1)) continue;
		       }
		       SbVec3f coords1 = chemData->getAtomCoordinates(from).getValue();
		       SbVec3f coords2 = chemData->getAtomCoordinates(to).getValue();
		       coord = (coords2 - coords1) * 0.5f + coords1;
		       parseBondLabel(labelFormat, theBond, from, to, coords1, coords2,
			       chemData, label);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       // Transform the screen-space starting position into object
		       // space, and feed that back to the glRasterPos command (which
		       // will turn around and transform it back into screen-space,
		       // but oh well).

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       normalFont->drawString(label);
	       BONDLOOP_END
       }
       else if (chemColor->bondLabelColorBinding.getValue() == 
	       ChemColor::BOND_LABEL_PER_BOND) {
	       const SbColor *bondColors = chemColor->bondColor.getValues(0);
	       BONDLOOP_START(theBondLabelIndex)
		       int32_t from = chemData->getBondFrom(theBond);
		       int32_t to = chemData->getBondTo(theBond);
		       if (!doHydrogens) {
			       if ((chemData->getAtomicNumber(from) == 1) ||
				       (chemData->getAtomicNumber(to) == 1)) continue;
		       }
		       SbVec3f coords1 = chemData->getAtomCoordinates(from).getValue();
		       SbVec3f coords2 = chemData->getAtomCoordinates(to).getValue();
		       coord = (coords2 - coords1) * 0.5f + coords1;
		       parseBondLabel(labelFormat, theBond, from, to, coords1, coords2,
			       chemData, label);
		       glColor3fv(bondColors[theBond].getValue());
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       // Transform the screen-space starting position into object
		       // space, and feed that back to the glRasterPos command (which
		       // will turn around and transform it back into screen-space,
		       // but oh well).

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       normalFont->drawString(label);
	       BONDLOOP_END
       }
       else if (chemColor->bondLabelColorBinding.getValue() == 
	       ChemColor::BOND_LABEL_PER_BOND_INDEXED) {
	       const SbColor *bondColors = chemColor->bondColor.getValues(0);
	       BONDLOOP_START(theBondLabelIndex)
		       int32_t from = chemData->getBondFrom(theBond);
		       int32_t to = chemData->getBondTo(theBond);
		       if (!doHydrogens) {
			       if ((chemData->getAtomicNumber(from) == 1) ||
				       (chemData->getAtomicNumber(to) == 1)) continue;
		       }
		       SbVec3f coords1 = chemData->getAtomCoordinates(from).getValue();
		       SbVec3f coords2 = chemData->getAtomCoordinates(to).getValue();
		       coord = (coords2 - coords1) * 0.5f + coords1;
		       parseBondLabel(labelFormat, theBond, from, to, coords1, coords2,
			       chemData, label);
		       glColor3fv(
			       bondColors[chemData->getBondIndex(theBond)].getValue());
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       // Transform the screen-space starting position into object
		       // space, and feed that back to the glRasterPos command (which
		       // will turn around and transform it back into screen-space,
		       // but oh well).

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       normalFont->drawString(label);
	       BONDLOOP_END
       }
       else if (chemColor->bondLabelColorBinding.getValue() == 
	       ChemColor::BOND_LABEL_PER_BOND_LABEL) {
	       const SbColor *bondColors = chemColor->bondLabelColor.getValues(0);
	       BONDLOOP_START(theBondLabelIndex)
		       int32_t from = chemData->getBondFrom(theBond);
		       int32_t to = chemData->getBondTo(theBond);
		       if (!doHydrogens) {
			       if ((chemData->getAtomicNumber(from) == 1) ||
				       (chemData->getAtomicNumber(to) == 1)) continue;
		       }
		       SbVec3f coords1 = chemData->getAtomCoordinates(from).getValue();
		       SbVec3f coords2 = chemData->getAtomCoordinates(to).getValue();
		       coord = (coords2 - coords1) * 0.5f + coords1;
		       parseBondLabel(labelFormat, theBond, from, to, coords1, coords2,
			       chemData, label);
		       glColor3fv(bondColors[theBond].getValue());
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       // Transform the screen-space starting position into object
		       // space, and feed that back to the glRasterPos command (which
		       // will turn around and transform it back into screen-space,
		       // but oh well).

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       normalFont->drawString(label);
	       BONDLOOP_END
       }
       else if (chemColor->bondLabelColorBinding.getValue() == 
	       ChemColor::BOND_LABEL_PER_BOND_INDEXED) {
	       const SbColor *bondColors = chemColor->bondLabelColor.getValues(0);
	       BONDLOOP_START(theBondLabelIndex)
		       int32_t from = chemData->getBondFrom(theBond);
		       int32_t to = chemData->getBondTo(theBond);
		       if (!doHydrogens) {
			       if ((chemData->getAtomicNumber(from) == 1) ||
				       (chemData->getAtomicNumber(to) == 1)) continue;
		       }
		       SbVec3f coords1 = chemData->getAtomCoordinates(from).getValue();
		       SbVec3f coords2 = chemData->getAtomCoordinates(to).getValue();
		       coord = (coords2 - coords1) * 0.5f + coords1;
		       parseBondLabel(labelFormat, theBond, from, to, coords1, coords2,
			       chemData, label);
		       glColor3fv(
			       bondColors[chemData->getBondIndex(theBond)].getValue());
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       // Transform the screen-space starting position into object
		       // space, and feed that back to the glRasterPos command (which
		       // will turn around and transform it back into screen-space,
		       // but oh well).

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       normalFont->drawString(label);
	       BONDLOOP_END
       }

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders bond labels with appropriate offsets depending on the
//    ChemDisplayParam bondLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
normalBondLabelsWithCylinders
       (SoGLRenderAction *action, const MFVec2i &theBondLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::renderBondLabels\n");
#endif

       // Check to see if there is anything to display.
       if (theBondLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);

       short lrJust = cdp->bondLabelLeftRightJustification.getValue();
       short tbJust = cdp->bondLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();
       vpr = SoViewportRegionElement::get(state);

       float offset = cdp->bondCylinderRadius.getValue();
       SbBool doHydrogens = cdp->showHydrogens.getValue();

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       const char *labelFormat = cdp->bondLabelString.getValue().getString();
       if (chemColor->bondLabelColorBinding.getValue() == 
	       ChemColor::BOND_LABEL_OVERALL) {
	       const SbColor *labelColors = chemColor->bondLabelColor.getValues(0);

	       glColor3fv(labelColors[0].getValue());
	       BONDLOOP_START(theBondLabelIndex)
		       int32_t from = chemData->getBondFrom(theBond);
		       int32_t to = chemData->getBondTo(theBond);
		       if (!doHydrogens) {
			       if ((chemData->getAtomicNumber(from) == 1) ||
				       (chemData->getAtomicNumber(to) == 1)) continue;
		       }
		       SbVec3f coords1 = chemData->getAtomCoordinates(from).getValue();
		       SbVec3f coords2 = chemData->getAtomCoordinates(to).getValue();
		       coord = (coords2 - coords1) * 0.5f + coords1;
		       parseBondLabel(labelFormat, theBond, from, to, coords1, coords2,
			       chemData, label);
		       theScale.setValue(offset, offset, offset);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       // Transform the screen-space starting position into object
		       // space, and feed that back to the glRasterPos command (which
		       // will turn around and transform it back into screen-space,
		       // but oh well).

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       normalFont->drawString(label);
	       BONDLOOP_END
       }
       else if (chemColor->bondLabelColorBinding.getValue() == 
	       ChemColor::BOND_LABEL_PER_BOND) {
	       const SbColor *bondColors = chemColor->bondColor.getValues(0);
	       BONDLOOP_START(theBondLabelIndex)
		       int32_t from = chemData->getBondFrom(theBond);
		       int32_t to = chemData->getBondTo(theBond);
		       if (!doHydrogens) {
			       if ((chemData->getAtomicNumber(from) == 1) ||
				       (chemData->getAtomicNumber(to) == 1)) continue;
		       }
		       SbVec3f coords1 = chemData->getAtomCoordinates(from).getValue();
		       SbVec3f coords2 = chemData->getAtomCoordinates(to).getValue();
		       coord = (coords2 - coords1) * 0.5f + coords1;
		       parseBondLabel(labelFormat, theBond, from, to, coords1, coords2,
			       chemData, label);
		       glColor3fv(bondColors[theBond].getValue());
		       theScale.setValue(offset, offset, offset);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       // Transform the screen-space starting position into object
		       // space, and feed that back to the glRasterPos command (which
		       // will turn around and transform it back into screen-space,
		       // but oh well).

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       normalFont->drawString(label);
	       BONDLOOP_END
       }
       else if (chemColor->bondLabelColorBinding.getValue() == 
	       ChemColor::BOND_LABEL_PER_BOND_INDEXED) {
	       const SbColor *bondColors = chemColor->bondColor.getValues(0);
	       BONDLOOP_START(theBondLabelIndex)
		       int32_t from = chemData->getBondFrom(theBond);
		       int32_t to = chemData->getBondTo(theBond);
		       if (!doHydrogens) {
			       if ((chemData->getAtomicNumber(from) == 1) ||
				       (chemData->getAtomicNumber(to) == 1)) continue;
		       }
		       SbVec3f coords1 = chemData->getAtomCoordinates(from).getValue();
		       SbVec3f coords2 = chemData->getAtomCoordinates(to).getValue();
		       coord = (coords2 - coords1) * 0.5f + coords1;
		       parseBondLabel(labelFormat, theBond, from, to, coords1, coords2,
			       chemData, label);
		       glColor3fv(
			       bondColors[chemData->getBondIndex(theBond)].getValue());
		       theScale.setValue(offset, offset, offset);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       // Transform the screen-space starting position into object
		       // space, and feed that back to the glRasterPos command (which
		       // will turn around and transform it back into screen-space,
		       // but oh well).

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       normalFont->drawString(label);
	       BONDLOOP_END
       }
       else if (chemColor->bondLabelColorBinding.getValue() == 
	       ChemColor::BOND_LABEL_PER_BOND_LABEL) {
	       const SbColor *bondColors = chemColor->bondLabelColor.getValues(0);
	       BONDLOOP_START(theBondLabelIndex)
		       int32_t from = chemData->getBondFrom(theBond);
		       int32_t to = chemData->getBondTo(theBond);
		       if (!doHydrogens) {
			       if ((chemData->getAtomicNumber(from) == 1) ||
				       (chemData->getAtomicNumber(to) == 1)) continue;
		       }
		       SbVec3f coords1 = chemData->getAtomCoordinates(from).getValue();
		       SbVec3f coords2 = chemData->getAtomCoordinates(to).getValue();
		       coord = (coords2 - coords1) * 0.5f + coords1;
		       parseBondLabel(labelFormat, theBond, from, to, coords1, coords2,
			       chemData, label);
		       glColor3fv(bondColors[theBond].getValue());
		       theScale.setValue(offset, offset, offset);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       // Transform the screen-space starting position into object
		       // space, and feed that back to the glRasterPos command (which
		       // will turn around and transform it back into screen-space,
		       // but oh well).

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       normalFont->drawString(label);
	       BONDLOOP_END
       }
       else if (chemColor->bondLabelColorBinding.getValue() == 
	       ChemColor::BOND_LABEL_PER_BOND_INDEXED) {
	       const SbColor *bondColors = chemColor->bondLabelColor.getValues(0);
	       BONDLOOP_START(theBondLabelIndex)
		       int32_t from = chemData->getBondFrom(theBond);
		       int32_t to = chemData->getBondTo(theBond);
		       if (!doHydrogens) {
			       if ((chemData->getAtomicNumber(from) == 1) ||
				       (chemData->getAtomicNumber(to) == 1)) continue;
		       }
		       SbVec3f coords1 = chemData->getAtomCoordinates(from).getValue();
		       SbVec3f coords2 = chemData->getAtomCoordinates(to).getValue();
		       coord = (coords2 - coords1) * 0.5f + coords1;
		       parseBondLabel(labelFormat, theBond, from, to, coords1, coords2,
			       chemData, label);
		       glColor3fv(
			       bondColors[chemData->getBondIndex(theBond)].getValue());
		       theScale.setValue(offset, offset, offset);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       // Transform the screen-space starting position into object
		       // space, and feed that back to the glRasterPos command (which
		       // will turn around and transform it back into screen-space,
		       // but oh well).

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       normalFont->drawString(label);
	       BONDLOOP_END
       }

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}



////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights bond labels with appropriate offsets depending on the
//    ChemDisplayParam bondLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
highlightBondLabelsWithWireframe
       (SoGLRenderAction *action, const MFVec2i &theBondLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::renderBondLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theBondLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);

       short lrJust = cdp->bondLabelLeftRightJustification.getValue();
       short tbJust = cdp->bondLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();
       vpr = SoViewportRegionElement::get(state);

       SbBool doHydrogens = cdp->showHydrogens.getValue();
       SbColor highlightColor = cdp->highlightColor.getValue();
       int32_t highlightStyle = cdp->highlightStyle.getValue();

       //SbMatrix   theMatrix;
       //SbVec3f    theScale;
       //SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       const char *labelFormat = cdp->bondLabelString.getValue().getString();
       if (chemColor->bondLabelColorBinding.getValue() == 
	       ChemColor::BOND_LABEL_OVERALL) {
	       const SbColor *labelColors = chemColor->bondLabelColor.getValues(0);

	       glColor3fv(labelColors[0].getValue());

	       ChemBitmapFontCache *theFont = normalFont;
	       SbColor theColor;

	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
		       theColor = labelColors[0] + highlightColor;
		       theColor.normalize();
		       glColor3fv(theColor.getValue());
	       }
	       else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
		       glColor3fv(highlightColor.getValue());
	       }
	       else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		       if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			       cdp->highlightFontSize.getValue(), highlightFont, TRUE)) {
			       endRenderState(state, curPixelAlignment);
			       return;
		       }
		       highlightFont->setupToRender(state);
		       theFont = highlightFont;
	       }
	       BONDLOOP_START(theBondLabelIndex)
		       int32_t from = chemData->getBondFrom(theBond);
		       int32_t to = chemData->getBondTo(theBond);
		       if (!doHydrogens) {
			       if ((chemData->getAtomicNumber(from) == 1) ||
				       (chemData->getAtomicNumber(to) == 1)) continue;
		       }
		       SbVec3f coords1 = chemData->getAtomCoordinates(from).getValue();
		       SbVec3f coords2 = chemData->getAtomCoordinates(to).getValue();
		       coord = (coords2 - coords1) * 0.5f + coords1;
		       parseBondLabel(labelFormat, theBond, from, to, coords1, coords2,
			       chemData, label);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       theFont->drawString(label);
	       BONDLOOP_END
       }
       else if (chemColor->bondLabelColorBinding.getValue() == 
	       ChemColor::BOND_LABEL_PER_BOND) {
	       const SbColor *bondColors = chemColor->bondColor.getValues(0);

	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
		       glColor3fv(highlightColor.getValue());
		       BONDLOOP_START(theBondLabelIndex)
			       int32_t from = chemData->getBondFrom(theBond);
			       int32_t to = chemData->getBondTo(theBond);
			       if (!doHydrogens) {
				       if ((chemData->getAtomicNumber(from) == 1) ||
					       (chemData->getAtomicNumber(to) == 1)) continue;
			       }
			       SbVec3f coords1 = chemData->getAtomCoordinates(from).getValue();
			       SbVec3f coords2 = chemData->getAtomCoordinates(to).getValue();
			       coord = (coords2 - coords1) * 0.5f + coords1;
			       parseBondLabel(labelFormat, theBond, from, to, coords1, coords2,
				       chemData, label);
			       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);
			       charPosition = 
				       getPixelStringOffset(normalFont, lrJust, tbJust, label)
					       + screenOrigin;

			       coord = toObjectSpace(charPosition, screenToObj, vpr);
			       glRasterPos3fv(coord.getValue());
			       normalFont->drawString(label);
		       BONDLOOP_END
		       goto BONDLABEL_HIGHLIGHT_DONE;
	       }

	       SbColor theColor;
	       ChemBitmapFontCache *theFont = normalFont;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		       if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			       cdp->highlightFontSize.getValue(), highlightFont, TRUE)) {
			       endRenderState(state, curPixelAlignment);
			       return;
		       }
		       highlightFont->setupToRender(state);
		       theFont = highlightFont;
	       }
	       BONDLOOP_START(theBondLabelIndex)
		       int32_t from = chemData->getBondFrom(theBond);
		       int32_t to = chemData->getBondTo(theBond);
		       if (!doHydrogens) {
			       if ((chemData->getAtomicNumber(from) == 1) ||
				       (chemData->getAtomicNumber(to) == 1)) continue;
		       }
		       SbVec3f coords1 = chemData->getAtomCoordinates(from).getValue();
		       SbVec3f coords2 = chemData->getAtomCoordinates(to).getValue();
		       coord = (coords2 - coords1) * 0.5f + coords1;
		       parseBondLabel(labelFormat, theBond, from, to, coords1, coords2,
			       chemData, label);
		       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			       theColor = bondColors[theBond] + highlightColor;
			       theColor.normalize();
			       glColor3fv(theColor.getValue());
		       }
		       else {
			       glColor3fv(bondColors[theBond].getValue());
		       }
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       theFont->drawString(label);
	       BONDLOOP_END
       }
       else if (chemColor->bondLabelColorBinding.getValue() == 
	       ChemColor::BOND_LABEL_PER_BOND_INDEXED) {
	       const SbColor *bondColors = chemColor->bondColor.getValues(0);

	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
		       glColor3fv(highlightColor.getValue());
		       BONDLOOP_START(theBondLabelIndex)
			       int32_t from = chemData->getBondFrom(theBond);
			       int32_t to = chemData->getBondTo(theBond);
			       if (!doHydrogens) {
				       if ((chemData->getAtomicNumber(from) == 1) ||
					       (chemData->getAtomicNumber(to) == 1)) continue;
			       }
			       SbVec3f coords1 = chemData->getAtomCoordinates(from).getValue();
			       SbVec3f coords2 = chemData->getAtomCoordinates(to).getValue();
			       coord = (coords2 - coords1) * 0.5f + coords1;
			       parseBondLabel(labelFormat, theBond, from, to, coords1, coords2,
				       chemData, label);
			       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);
			       charPosition = 
				       getPixelStringOffset(normalFont, lrJust, tbJust, label)
					       + screenOrigin;

			       coord = toObjectSpace(charPosition, screenToObj, vpr);
			       glRasterPos3fv(coord.getValue());
			       normalFont->drawString(label);
		       BONDLOOP_END
		       goto BONDLABEL_HIGHLIGHT_DONE;
	       }

	       SbColor theColor;
	       ChemBitmapFontCache *theFont = normalFont;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		       if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			       cdp->highlightFontSize.getValue(), highlightFont, TRUE)) {
			       endRenderState(state, curPixelAlignment);
			       return;
		       }
		       highlightFont->setupToRender(state);
		       theFont = highlightFont;
	       }
	       BONDLOOP_START(theBondLabelIndex)
		       int32_t from = chemData->getBondFrom(theBond);
		       int32_t to = chemData->getBondTo(theBond);
		       if (!doHydrogens) {
			       if ((chemData->getAtomicNumber(from) == 1) ||
				       (chemData->getAtomicNumber(to) == 1)) continue;
		       }
		       SbVec3f coords1 = chemData->getAtomCoordinates(from).getValue();
		       SbVec3f coords2 = chemData->getAtomCoordinates(to).getValue();
		       coord = (coords2 - coords1) * 0.5f + coords1;
		       parseBondLabel(labelFormat, theBond, from, to, coords1, coords2,
			       chemData, label);
		       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			       theColor = bondColors[chemData->getBondIndex(theBond)];
			       theColor += highlightColor;
			       theColor.normalize();
			       glColor3fv(theColor.getValue());
		       }
		       else {
			       glColor3fv(
			       bondColors[chemData->getBondIndex(theBond)].getValue());
		       }
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       theFont->drawString(label);
	       BONDLOOP_END
       }
       else if (chemColor->bondLabelColorBinding.getValue() == 
	       ChemColor::BOND_LABEL_PER_BOND_LABEL) {
	       const SbColor *bondColors = chemColor->bondLabelColor.getValues(0);

	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
		       glColor3fv(highlightColor.getValue());
		       BONDLOOP_START(theBondLabelIndex)
			       int32_t from = chemData->getBondFrom(theBond);
			       int32_t to = chemData->getBondTo(theBond);
			       if (!doHydrogens) {
				       if ((chemData->getAtomicNumber(from) == 1) ||
					       (chemData->getAtomicNumber(to) == 1)) continue;
			       }
			       SbVec3f coords1 = chemData->getAtomCoordinates(from).getValue();
			       SbVec3f coords2 = chemData->getAtomCoordinates(to).getValue();
			       coord = (coords2 - coords1) * 0.5f + coords1;
			       parseBondLabel(labelFormat, theBond, from, to, coords1, coords2,
				       chemData, label);
			       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);
			       charPosition = 
				       getPixelStringOffset(normalFont, lrJust, tbJust, label)
					       + screenOrigin;

			       coord = toObjectSpace(charPosition, screenToObj, vpr);
			       glRasterPos3fv(coord.getValue());
			       normalFont->drawString(label);
		       BONDLOOP_END
		       goto BONDLABEL_HIGHLIGHT_DONE;
	       }

	       SbColor theColor;
	       ChemBitmapFontCache *theFont = normalFont;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		       if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			       cdp->highlightFontSize.getValue(), highlightFont, TRUE)) {
			       endRenderState(state, curPixelAlignment);
			       return;
		       }
		       highlightFont->setupToRender(state);
		       theFont = highlightFont;
	       }
	       BONDLOOP_START(theBondLabelIndex)
		       int32_t from = chemData->getBondFrom(theBond);
		       int32_t to = chemData->getBondTo(theBond);
		       if (!doHydrogens) {
			       if ((chemData->getAtomicNumber(from) == 1) ||
				       (chemData->getAtomicNumber(to) == 1)) continue;
		       }
		       SbVec3f coords1 = chemData->getAtomCoordinates(from).getValue();
		       SbVec3f coords2 = chemData->getAtomCoordinates(to).getValue();
		       coord = (coords2 - coords1) * 0.5f + coords1;
		       parseBondLabel(labelFormat, theBond, from, to, coords1, coords2,
			       chemData, label);
		       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			       theColor = bondColors[theBond] + highlightColor;
			       theColor.normalize();
			       glColor3fv(theColor.getValue());
		       }
		       else {
			       glColor3fv(bondColors[theBond].getValue());
		       }
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       theFont->drawString(label);
	       BONDLOOP_END
       }
       else if (chemColor->bondLabelColorBinding.getValue() == 
	       ChemColor::BOND_LABEL_PER_BOND_INDEXED) {
	       const SbColor *bondColors = chemColor->bondLabelColor.getValues(0);

	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
		       glColor3fv(highlightColor.getValue());
		       BONDLOOP_START(theBondLabelIndex)
			       int32_t from = chemData->getBondFrom(theBond);
			       int32_t to = chemData->getBondTo(theBond);
			       if (!doHydrogens) {
				       if ((chemData->getAtomicNumber(from) == 1) ||
					       (chemData->getAtomicNumber(to) == 1)) continue;
			       }
			       SbVec3f coords1 = chemData->getAtomCoordinates(from).getValue();
			       SbVec3f coords2 = chemData->getAtomCoordinates(to).getValue();
			       coord = (coords2 - coords1) * 0.5f + coords1;
			       parseBondLabel(labelFormat, theBond, from, to, coords1, coords2,
				       chemData, label);
			       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);
			       charPosition = 
				       getPixelStringOffset(normalFont, lrJust, tbJust, label)
					       + screenOrigin;

			       coord = toObjectSpace(charPosition, screenToObj, vpr);
			       glRasterPos3fv(coord.getValue());
			       normalFont->drawString(label);
		       BONDLOOP_END
		       goto BONDLABEL_HIGHLIGHT_DONE;
	       }

	       SbColor theColor;
	       ChemBitmapFontCache *theFont = normalFont;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		       if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			       cdp->highlightFontSize.getValue(), highlightFont, TRUE)) {
			       endRenderState(state, curPixelAlignment);
			       return;
		       }
		       highlightFont->setupToRender(state);
		       theFont = highlightFont;
	       }
	       BONDLOOP_START(theBondLabelIndex)
		       int32_t from = chemData->getBondFrom(theBond);
		       int32_t to = chemData->getBondTo(theBond);
		       if (!doHydrogens) {
			       if ((chemData->getAtomicNumber(from) == 1) ||
				       (chemData->getAtomicNumber(to) == 1)) continue;
		       }
		       SbVec3f coords1 = chemData->getAtomCoordinates(from).getValue();
		       SbVec3f coords2 = chemData->getAtomCoordinates(to).getValue();
		       coord = (coords2 - coords1) * 0.5f + coords1;
		       parseBondLabel(labelFormat, theBond, from, to, coords1, coords2,
			       chemData, label);
		       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			       theColor = bondColors[chemData->getBondIndex(theBond)];
			       theColor += highlightColor;
			       theColor.normalize();
			       glColor3fv(theColor.getValue());
		       }
		       else {
			       glColor3fv(
			       bondColors[chemData->getBondIndex(theBond)].getValue());
		       }
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       theFont->drawString(label);
	       BONDLOOP_END
       }

BONDLABEL_HIGHLIGHT_DONE:

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights bond labels with appropriate offsets depending on the
//    ChemDisplayParam bondLabelJustification and displayStyle settings.
//
// Use: protected

void
ChemDisplay::
highlightBondLabelsWithCylinders
       (SoGLRenderAction *action, const MFVec2i &theBondLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::renderBondLabels\n");
#endif

       // Check to see if there is anything to render.
       if (theBondLabelIndex.getNum() == 0) return;

       SbString label;
       SbString name;
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(action, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, TRUE)) {
	       return;
       }

       // Push the state and set the appropriate lighting and color model
       GLint curPixelAlignment;
       beginRenderState(state, &curPixelAlignment);

       normalFont->setupToRender(state);

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);

       short lrJust = cdp->bondLabelLeftRightJustification.getValue();
       short tbJust = cdp->bondLabelTopBottomJustification.getValue();

       SbMatrix objToScreen;
       SbMatrix screenToObj;
       SbViewportRegion vpr;
       SbVec3f screenOrigin;
       SbVec3f charPosition;

       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));
       screenToObj = objToScreen.inverse();
       vpr = SoViewportRegionElement::get(state);

       float offset = cdp->bondCylinderRadius.getValue();
       SbBool doHydrogens = cdp->showHydrogens.getValue();
       SbColor highlightColor = cdp->highlightColor.getValue();
       int32_t highlightStyle = cdp->highlightStyle.getValue();

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       const char *labelFormat = cdp->bondLabelString.getValue().getString();
       if (chemColor->bondLabelColorBinding.getValue() == 
	       ChemColor::BOND_LABEL_OVERALL) {
	       const SbColor *labelColors = chemColor->bondLabelColor.getValues(0);

	       glColor3fv(labelColors[0].getValue());

	       ChemBitmapFontCache *theFont = normalFont;
	       SbColor theColor;

	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
		       theColor = labelColors[0] + highlightColor;
		       theColor.normalize();
		       glColor3fv(theColor.getValue());
	       }
	       else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
		       glColor3fv(highlightColor.getValue());
	       }
	       else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		       if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			       cdp->highlightFontSize.getValue(), highlightFont, TRUE)) {
			       endRenderState(state, curPixelAlignment);
			       return;
		       }
		       highlightFont->setupToRender(state);
		       theFont = highlightFont;
	       }
	       BONDLOOP_START(theBondLabelIndex)
		       int32_t from = chemData->getBondFrom(theBond);
		       int32_t to = chemData->getBondTo(theBond);
		       if (!doHydrogens) {
			       if ((chemData->getAtomicNumber(from) == 1) ||
				       (chemData->getAtomicNumber(to) == 1)) continue;
		       }
		       SbVec3f coords1 = chemData->getAtomCoordinates(from).getValue();
		       SbVec3f coords2 = chemData->getAtomCoordinates(to).getValue();
		       coord = (coords2 - coords1) * 0.5f + coords1;
		       parseBondLabel(labelFormat, theBond, from, to, coords1, coords2,
			       chemData, label);
		       theScale.setValue(offset, offset, offset);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       theFont->drawString(label);
	       BONDLOOP_END
       }
       else if (chemColor->bondLabelColorBinding.getValue() == 
	       ChemColor::BOND_LABEL_PER_BOND) {
	       const SbColor *bondColors = chemColor->bondColor.getValues(0);

	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
		       glColor3fv(highlightColor.getValue());
		       BONDLOOP_START(theBondLabelIndex)
			       int32_t from = chemData->getBondFrom(theBond);
			       int32_t to = chemData->getBondTo(theBond);
			       if (!doHydrogens) {
				       if ((chemData->getAtomicNumber(from) == 1) ||
					       (chemData->getAtomicNumber(to) == 1)) continue;
			       }
			       SbVec3f coords1 = chemData->getAtomCoordinates(from).getValue();
			       SbVec3f coords2 = chemData->getAtomCoordinates(to).getValue();
			       coord = (coords2 - coords1) * 0.5f + coords1;
			       parseBondLabel(labelFormat, theBond, from, to, coords1, coords2,
				       chemData, label);
			       theScale.setValue(offset, offset, offset);
			       theMatrix.setTransform(coord, theRotation, theScale);
			       theMatrix.multVecMatrix(theZAxis, coord);
			       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);
			       charPosition = 
				       getPixelStringOffset(normalFont, lrJust, tbJust, label)
					       + screenOrigin;

			       coord = toObjectSpace(charPosition, screenToObj, vpr);
			       glRasterPos3fv(coord.getValue());
			       normalFont->drawString(label);
		       BONDLOOP_END
		       goto BONDLABEL_HIGHLIGHT_DONE;
	       }

	       SbColor theColor;
	       ChemBitmapFontCache *theFont = normalFont;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		       if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			       cdp->highlightFontSize.getValue(), highlightFont, TRUE)) {
			       endRenderState(state, curPixelAlignment);
			       return;
		       }
		       highlightFont->setupToRender(state);
		       theFont = highlightFont;
	       }
	       BONDLOOP_START(theBondLabelIndex)
		       int32_t from = chemData->getBondFrom(theBond);
		       int32_t to = chemData->getBondTo(theBond);
		       if (!doHydrogens) {
			       if ((chemData->getAtomicNumber(from) == 1) ||
				       (chemData->getAtomicNumber(to) == 1)) continue;
		       }
		       SbVec3f coords1 = chemData->getAtomCoordinates(from).getValue();
		       SbVec3f coords2 = chemData->getAtomCoordinates(to).getValue();
		       coord = (coords2 - coords1) * 0.5f + coords1;
		       parseBondLabel(labelFormat, theBond, from, to, coords1, coords2,
			       chemData, label);
		       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			       theColor = bondColors[theBond] + highlightColor;
			       theColor.normalize();
			       glColor3fv(theColor.getValue());
		       }
		       else {
			       glColor3fv(bondColors[theBond].getValue());
		       }
		       theScale.setValue(offset, offset, offset);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       theFont->drawString(label);
	       BONDLOOP_END
       }
       else if (chemColor->bondLabelColorBinding.getValue() == 
	       ChemColor::BOND_LABEL_PER_BOND_INDEXED) {
	       const SbColor *bondColors = chemColor->bondColor.getValues(0);

	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
		       glColor3fv(highlightColor.getValue());
		       BONDLOOP_START(theBondLabelIndex)
			       int32_t from = chemData->getBondFrom(theBond);
			       int32_t to = chemData->getBondTo(theBond);
			       if (!doHydrogens) {
				       if ((chemData->getAtomicNumber(from) == 1) ||
					       (chemData->getAtomicNumber(to) == 1)) continue;
			       }
			       SbVec3f coords1 = chemData->getAtomCoordinates(from).getValue();
			       SbVec3f coords2 = chemData->getAtomCoordinates(to).getValue();
			       coord = (coords2 - coords1) * 0.5f + coords1;
			       parseBondLabel(labelFormat, theBond, from, to, coords1, coords2,
				       chemData, label);
			       theScale.setValue(offset, offset, offset);
			       theMatrix.setTransform(coord, theRotation, theScale);
			       theMatrix.multVecMatrix(theZAxis, coord);
			       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);
			       charPosition = 
				       getPixelStringOffset(normalFont, lrJust, tbJust, label)
					       + screenOrigin;

			       coord = toObjectSpace(charPosition, screenToObj, vpr);
			       glRasterPos3fv(coord.getValue());
			       normalFont->drawString(label);
		       BONDLOOP_END
		       goto BONDLABEL_HIGHLIGHT_DONE;
	       }

	       SbColor theColor;
	       ChemBitmapFontCache *theFont = normalFont;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		       if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			       cdp->highlightFontSize.getValue(), highlightFont, TRUE)) {
			       endRenderState(state, curPixelAlignment);
			       return;
		       }
		       highlightFont->setupToRender(state);
		       theFont = highlightFont;
	       }
	       BONDLOOP_START(theBondLabelIndex)
		       int32_t from = chemData->getBondFrom(theBond);
		       int32_t to = chemData->getBondTo(theBond);
		       if (!doHydrogens) {
			       if ((chemData->getAtomicNumber(from) == 1) ||
				       (chemData->getAtomicNumber(to) == 1)) continue;
		       }
		       SbVec3f coords1 = chemData->getAtomCoordinates(from).getValue();
		       SbVec3f coords2 = chemData->getAtomCoordinates(to).getValue();
		       coord = (coords2 - coords1) * 0.5f + coords1;
		       parseBondLabel(labelFormat, theBond, from, to, coords1, coords2,
			       chemData, label);
		       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			       theColor = bondColors[chemData->getBondIndex(theBond)];
			       theColor += highlightColor;
			       theColor.normalize();
			       glColor3fv(theColor.getValue());
		       }
		       else {
			       glColor3fv(
			       bondColors[chemData->getBondIndex(theBond)].getValue());
		       }
		       theScale.setValue(offset, offset, offset);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       theFont->drawString(label);
	       BONDLOOP_END
       }
       else if (chemColor->bondLabelColorBinding.getValue() == 
	       ChemColor::BOND_LABEL_PER_BOND_LABEL) {
	       const SbColor *bondColors = chemColor->bondLabelColor.getValues(0);

	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
		       glColor3fv(highlightColor.getValue());
		       BONDLOOP_START(theBondLabelIndex)
			       int32_t from = chemData->getBondFrom(theBond);
			       int32_t to = chemData->getBondTo(theBond);
			       if (!doHydrogens) {
				       if ((chemData->getAtomicNumber(from) == 1) ||
					       (chemData->getAtomicNumber(to) == 1)) continue;
			       }
			       SbVec3f coords1 = chemData->getAtomCoordinates(from).getValue();
			       SbVec3f coords2 = chemData->getAtomCoordinates(to).getValue();
			       coord = (coords2 - coords1) * 0.5f + coords1;
			       parseBondLabel(labelFormat, theBond, from, to, coords1, coords2,
				       chemData, label);
			       theScale.setValue(offset, offset, offset);
			       theMatrix.setTransform(coord, theRotation, theScale);
			       theMatrix.multVecMatrix(theZAxis, coord);
			       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);
			       charPosition = 
				       getPixelStringOffset(normalFont, lrJust, tbJust, label)
					       + screenOrigin;

			       coord = toObjectSpace(charPosition, screenToObj, vpr);
			       glRasterPos3fv(coord.getValue());
			       normalFont->drawString(label);
		       BONDLOOP_END
		       goto BONDLABEL_HIGHLIGHT_DONE;
	       }

	       SbColor theColor;
	       ChemBitmapFontCache *theFont = normalFont;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		       if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			       cdp->highlightFontSize.getValue(), highlightFont, TRUE)) {
			       endRenderState(state, curPixelAlignment);
			       return;
		       }
		       highlightFont->setupToRender(state);
		       theFont = highlightFont;
	       }
	       BONDLOOP_START(theBondLabelIndex)
		       int32_t from = chemData->getBondFrom(theBond);
		       int32_t to = chemData->getBondTo(theBond);
		       if (!doHydrogens) {
			       if ((chemData->getAtomicNumber(from) == 1) ||
				       (chemData->getAtomicNumber(to) == 1)) continue;
		       }
		       SbVec3f coords1 = chemData->getAtomCoordinates(from).getValue();
		       SbVec3f coords2 = chemData->getAtomCoordinates(to).getValue();
		       coord = (coords2 - coords1) * 0.5f + coords1;
		       parseBondLabel(labelFormat, theBond, from, to, coords1, coords2,
			       chemData, label);
		       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			       theColor = bondColors[theBond] + highlightColor;
			       theColor.normalize();
			       glColor3fv(theColor.getValue());
		       }
		       else {
			       glColor3fv(bondColors[theBond].getValue());
		       }
		       theScale.setValue(offset, offset, offset);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       theFont->drawString(label);
	       BONDLOOP_END
       }
       else if (chemColor->bondLabelColorBinding.getValue() == 
	       ChemColor::BOND_LABEL_PER_BOND_INDEXED) {
	       const SbColor *bondColors = chemColor->bondLabelColor.getValues(0);

	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
		       glColor3fv(highlightColor.getValue());
		       BONDLOOP_START(theBondLabelIndex)
			       int32_t from = chemData->getBondFrom(theBond);
			       int32_t to = chemData->getBondTo(theBond);
			       if (!doHydrogens) {
				       if ((chemData->getAtomicNumber(from) == 1) ||
					       (chemData->getAtomicNumber(to) == 1)) continue;
			       }
			       SbVec3f coords1 = chemData->getAtomCoordinates(from).getValue();
			       SbVec3f coords2 = chemData->getAtomCoordinates(to).getValue();
			       coord = (coords2 - coords1) * 0.5f + coords1;
			       parseBondLabel(labelFormat, theBond, from, to, coords1, coords2,
				       chemData, label);
			       theScale.setValue(offset, offset, offset);
			       theMatrix.setTransform(coord, theRotation, theScale);
			       theMatrix.multVecMatrix(theZAxis, coord);
			       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);
			       charPosition = 
				       getPixelStringOffset(normalFont, lrJust, tbJust, label)
					       + screenOrigin;

			       coord = toObjectSpace(charPosition, screenToObj, vpr);
			       glRasterPos3fv(coord.getValue());
			       normalFont->drawString(label);
		       BONDLOOP_END
		       goto BONDLABEL_HIGHLIGHT_DONE;
	       }

	       SbColor theColor;
	       ChemBitmapFontCache *theFont = normalFont;
	       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		       if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			       cdp->highlightFontSize.getValue(), highlightFont, TRUE)) {
			       endRenderState(state, curPixelAlignment);
			       return;
		       }
		       highlightFont->setupToRender(state);
		       theFont = highlightFont;
	       }
	       BONDLOOP_START(theBondLabelIndex)
		       int32_t from = chemData->getBondFrom(theBond);
		       int32_t to = chemData->getBondTo(theBond);
		       if (!doHydrogens) {
			       if ((chemData->getAtomicNumber(from) == 1) ||
				       (chemData->getAtomicNumber(to) == 1)) continue;
		       }
		       SbVec3f coords1 = chemData->getAtomCoordinates(from).getValue();
		       SbVec3f coords2 = chemData->getAtomCoordinates(to).getValue();
		       coord = (coords2 - coords1) * 0.5f + coords1;
		       parseBondLabel(labelFormat, theBond, from, to, coords1, coords2,
			       chemData, label);
		       if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			       theColor = bondColors[chemData->getBondIndex(theBond)];
			       theColor += highlightColor;
			       theColor.normalize();
			       glColor3fv(theColor.getValue());
		       }
		       else {
			       glColor3fv(
			       bondColors[chemData->getBondIndex(theBond)].getValue());
		       }
		       theScale.setValue(offset, offset, offset);
		       theMatrix.setTransform(coord, theRotation, theScale);
		       theMatrix.multVecMatrix(theZAxis, coord);
		       screenOrigin = fromObjectSpace(coord, objToScreen, vpr);
		       charPosition = 
			       getPixelStringOffset(normalFont, lrJust, tbJust, label)
				       + screenOrigin;

		       coord = toObjectSpace(charPosition, screenToObj, vpr);
		       glRasterPos3fv(coord.getValue());
		       theFont->drawString(label);
	       BONDLOOP_END
       }

BONDLABEL_HIGHLIGHT_DONE:

       // Pop the state and restore the lighting and color model
       endRenderState(state, curPixelAlignment);
}

// --> residue labels
void ChemDisplay::renderResidueLabels(SoGLRenderAction *action)
{
	if (residueLabelIndex.getNum() == 0 ||  globalNumberOfResidues == 0) return;

	SoState *state = action->getState();
	ChemColor *chemColor = ChemColorElement::get(state);

	computeResidueLabelsCulling(action);

	switch (chemColor->residueLabelColorBinding.getValue())
	{
		case ChemColor::RESIDUE_LABEL_OVERALL:
			normalResidueLabelsOV(action,vnormalResidueLabelIndex);
			highlightResidueLabelsOV(action,vhighlightResidueLabelIndex);
			break;
		case ChemColor::RESIDUE_LABEL_PER_INDEX:
			normalResidueLabelsRPN(action,vnormalResidueLabelIndex);
			highlightResidueLabelsRPN(action,vhighlightResidueLabelIndex);
			break;
		case ChemColor::RESIDUE_LABEL_PER_CHAIN:
			normalResidueLabelsRPC(action,vnormalResidueLabelIndex);
			highlightResidueLabelsRPC(action,vhighlightResidueLabelIndex);
			break;
		default:
			break;
	}
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Given the label format, get the information from a
//    ChemData-derived node and generate a label.   
//
// Use: static

static void parseResidueLabel(const char *labelFormat,
															const int32_t theResidue,
															const ChemData *chemData, SbString &label)
{
	SbString name;
	char ch;
	const char *lf2;
	char tmpStr[2];
	tmpStr[1] = '\0';

	label.makeEmpty();
	lf2 = labelFormat;
	int id;
	char buf[32];
	while ((ch = *lf2++))
	{
		if (ch == '%')
		{
			ch = *lf2++;
			if (isupper(ch))
			{
				ch = tolower(ch);
			}
			switch(ch)
			{
			  case 'i':
				if (chemData->getResidueIndex(theResidue) == -1) name = chemData->getCustomResidueName(theResidue);
				else name = chemData->getResidueName(theResidue);
				label += name.getString();
				break;
			  case 'c':
				sprintf(buf, "%c", chemData->getResidueChain(theResidue));
				label += buf;
				break;
			  case 's':
				id = chemData->getResidueNumber(theResidue);
				sprintf(buf, "%d", id);
				label += buf;
				break;
				case 'n':
				name = chemData->getCustomResidueName(theResidue);
				label += name.getString();
				break;
			  case '%':
				label += "%";
				break;
			  default:
				break;
			}
		}
		else if ((ch >= ' ') && (ch <= '~'))
		{
			tmpStr[0] = ch;
			label += tmpStr;
		}
	}
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders residue labels with appropriate offsets depending on the
//    ChemDisplayParam residueLabelJustification and displayStyle settings.
//
// Use: protected

void ChemDisplay::normalResidueLabelsOV(SoGLRenderAction *action, 
																				const MFVec2i &theResidueLabelIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::normalResidueLabelsOV\n");
#endif

	// Check to see if there is anything to render.
	if (theResidueLabelIndex.getNum() == 0) return;

	SbString label;
	SoState *state = action->getState();

	ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

	// Get a font cache we can pull stuff out of
	if (!getFontCache(action, state, cdp->fontName.getValue(),
	 cdp->fontSize.getValue(), normalFont, TRUE)) {
	 return;
	}

	// Push the state and set the appropriate lighting and color model
	GLint curPixelAlignment;
	beginRenderState(state, &curPixelAlignment);

	normalFont->setupToRender(state);

	ChemData *chemData = (ChemData*)ChemBaseDataElement::get(state);
	ChemColor *chemColor = ChemColorElement::get(state);

	short lrJust = cdp->residueLabelLeftRightJustification.getValue();
	short tbJust = cdp->residueLabelTopBottomJustification.getValue();

	SbMatrix objToScreen;
	SbMatrix screenToObj;
	SbViewportRegion vpr;
	SbVec3f screenOrigin;
	SbVec3f charPosition;

	objToScreen = SoProjectionMatrixElement::get(state);
	objToScreen =
	 objToScreen.multLeft(SoViewingMatrixElement::get(state));
	objToScreen =
	 objToScreen.multLeft(SoModelMatrixElement::get(state));
	screenToObj = objToScreen.inverse();

	vpr = SoViewportRegionElement::get(state);

	SbVec3f coord;

	const char *labelFormat = cdp->residueLabelString.getValue().getString();
	const SbColor *labelColors = chemColor->residueLabelColor.getValues(0);
	
	glColor3fv(labelColors[0].getValue());

	RESIDUELOOP_START(theResidueLabelIndex)
 
		parseResidueLabel(labelFormat, theResidue, chemData, label);
		
		if (chemData->residues[theResidue].checkControlPoints())
		{
			coord = chemData->getAtomCoordinates(chemData->residues[theResidue][SbResidue::RESIDUE_CONTROL_POINT_CA]);
		}
		else
		{
			if (!chemData->residues[theResidue].computeCenterOfMass(chemData,coord))
				continue;
		}

		screenOrigin = fromObjectSpace(coord, objToScreen, vpr);		
		charPosition = 
		 getPixelStringOffset(normalFont, lrJust, tbJust, label)
				 + screenOrigin;
		coord = toObjectSpace(charPosition, screenToObj, vpr);

		glRasterPos3fv(coord.getValue());
		normalFont->drawString(label);

	RESIDUELOOP_END

	// Pop the state and restore the lighting and color model
	endRenderState(state, curPixelAlignment);
}

void ChemDisplay::highlightResidueLabelsOV(SoGLRenderAction *action,
																					 const MFVec2i &theResidueLabelIndex)
{
#ifdef DEBUG
printf ("In ChemDisplay::highlightResidueLabelsOV\n");
#endif

	// Check to see if there is anything to render.
	if (theResidueLabelIndex.getNum() == 0) return;

	SbString label;
	SoState *state = action->getState();

	ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

	// Get a font cache we can pull stuff out of
	if (!getFontCache(action, state, cdp->fontName.getValue(),
	 cdp->fontSize.getValue(), normalFont, TRUE))
	{
	 return;
	}

	// Push the state and set the appropriate lighting and color model
	GLint curPixelAlignment;
	beginRenderState(state, &curPixelAlignment);

	normalFont->setupToRender(state);

	ChemData *chemData = (ChemData*)ChemBaseDataElement::get(state);
	ChemColor *chemColor = ChemColorElement::get(state);

	short lrJust = cdp->residueLabelLeftRightJustification.getValue();
	short tbJust = cdp->residueLabelTopBottomJustification.getValue();

	SbMatrix objToScreen;
	SbMatrix screenToObj;
	SbViewportRegion vpr;
	SbVec3f screenOrigin;
	SbVec3f charPosition;

	objToScreen = SoProjectionMatrixElement::get(state);
	objToScreen =
	 objToScreen.multLeft(SoViewingMatrixElement::get(state));
	objToScreen =
	 objToScreen.multLeft(SoModelMatrixElement::get(state));
	screenToObj = objToScreen.inverse();

	vpr = SoViewportRegionElement::get(state);

	SbVec3f coord;

	const char *labelFormat = cdp->residueLabelString.getValue().getString();
	const SbColor *labelColors = chemColor->residueLabelColor.getValues(0);
	
	SbColor highlightColor = cdp->highlightColor.getValue();
  int32_t highlightStyle = cdp->highlightStyle.getValue();
	ChemBitmapFontCache *theFont = normalFont;

	glColor3fv(labelColors[0].getValue());
  if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE)
	{
		glColor3fv(highlightColor.getValue());
	}
	else
	{  
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
		{
		 if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			 cdp->highlightFontSize.getValue(), highlightFont, TRUE))
		 {
			 endRenderState(state, curPixelAlignment);
			 return;
		 }
		 highlightFont->setupToRender(state);
		 theFont = highlightFont;
		}
		SbColor theColor;
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
		{
		 theColor = labelColors[0] + highlightColor;
		 theColor.normalize();
		 glColor3fv(theColor.getValue());
		}
	}

	RESIDUELOOP_START(theResidueLabelIndex)
 
		parseResidueLabel(labelFormat, theResidue, chemData, label);

		if (chemData->residues[theResidue].checkControlPoints())
		{
			coord = chemData->getAtomCoordinates(chemData->residues[theResidue][SbResidue::RESIDUE_CONTROL_POINT_CA]);
		}
		else
		{
			if (!chemData->residues[theResidue].computeCenterOfMass(chemData,coord))
				continue;
		}

		screenOrigin = fromObjectSpace(coord, objToScreen, vpr);
		charPosition = 
		getPixelStringOffset(normalFont, lrJust, tbJust, label)
			 + screenOrigin;
		coord = toObjectSpace(charPosition, screenToObj, vpr);

		glRasterPos3fv(coord.getValue());
		theFont->drawString(label);

	RESIDUELOOP_END

	// Pop the state and restore the lighting and color model
	endRenderState(state, curPixelAlignment);
}

void ChemDisplay::normalResidueLabelsRPN(SoGLRenderAction *action, 
														const MFVec2i &theResidueLabelIndex)
{
#ifdef DEBUG
printf ("In ChemDisplay::normalResidueLabelsRPN\n");
#endif

	// Check to see if there is anything to render.
	if (theResidueLabelIndex.getNum() == 0) return;

	SbString label;
	SoState *state = action->getState();

	ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

	// Get a font cache we can pull stuff out of
	if (!getFontCache(action, state, cdp->fontName.getValue(),
	 cdp->fontSize.getValue(), normalFont, TRUE)) {
	 return;
	}

	// Push the state and set the appropriate lighting and color model
	GLint curPixelAlignment;
	beginRenderState(state, &curPixelAlignment);

	normalFont->setupToRender(state);

	ChemData *chemData = (ChemData*)ChemBaseDataElement::get(state);
	ChemColor *chemColor = ChemColorElement::get(state);

	short lrJust = cdp->residueLabelLeftRightJustification.getValue();
	short tbJust = cdp->residueLabelTopBottomJustification.getValue();

	SbMatrix objToScreen;
	SbMatrix screenToObj;
	SbViewportRegion vpr;
	SbVec3f screenOrigin;
	SbVec3f charPosition;

	objToScreen = SoProjectionMatrixElement::get(state);
	objToScreen =
	 objToScreen.multLeft(SoViewingMatrixElement::get(state));
	objToScreen =
	 objToScreen.multLeft(SoModelMatrixElement::get(state));
	screenToObj = objToScreen.inverse();

	vpr = SoViewportRegionElement::get(state);

	SbVec3f coord;

	const char *labelFormat = cdp->residueLabelString.getValue().getString();
	const SbColor *labelColors = chemColor->residueLabelColor.getValues(0);
	int32_t colorIndex;

	RESIDUELOOP_START(theResidueLabelIndex)
 
		parseResidueLabel(labelFormat, theResidue, chemData, label);

		if (chemData->residues[theResidue].getIndex() < 0) colorIndex = 0;
		else colorIndex = chemData->residueColorIndex[chemData->residues[theResidue].getIndex()];
		glColor3fv(labelColors[colorIndex].getValue());
		
		glColor3fv(labelColors[colorIndex].getValue());

		if (chemData->residues[theResidue].checkControlPoints())
		{
			coord = chemData->getAtomCoordinates(chemData->residues[theResidue][SbResidue::RESIDUE_CONTROL_POINT_CA]);
		}
		else
		{
			if (!chemData->residues[theResidue].computeCenterOfMass(chemData,coord))
				continue;
		}

		screenOrigin = fromObjectSpace(coord, objToScreen, vpr);
		charPosition = 
		 getPixelStringOffset(normalFont, lrJust, tbJust, label)
			 + screenOrigin;
		coord = toObjectSpace(charPosition, screenToObj, vpr);

		glRasterPos3fv(coord.getValue());
		normalFont->drawString(label);

	RESIDUELOOP_END

	// Pop the state and restore the lighting and color model
	endRenderState(state, curPixelAlignment);
}

void ChemDisplay::highlightResidueLabelsRPN(SoGLRenderAction *action, 
															 const MFVec2i &theResidueLabelIndex)
{
#ifdef DEBUG
printf ("In ChemDisplay::highlightResidueLabelsOV\n");
#endif

	// Check to see if there is anything to render.
	if (theResidueLabelIndex.getNum() == 0) return;

	SbString label;
	SoState *state = action->getState();

	ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

	// Get a font cache we can pull stuff out of
	if (!getFontCache(action, state, cdp->fontName.getValue(),
	 cdp->fontSize.getValue(), normalFont, TRUE))
	{
	 return;
	}

	// Push the state and set the appropriate lighting and color model
	GLint curPixelAlignment;
	beginRenderState(state, &curPixelAlignment);

	normalFont->setupToRender(state);

	ChemData *chemData = (ChemData*)ChemBaseDataElement::get(state);
	ChemColor *chemColor = ChemColorElement::get(state);

	short lrJust = cdp->residueLabelLeftRightJustification.getValue();
	short tbJust = cdp->residueLabelTopBottomJustification.getValue();

	SbMatrix objToScreen;
	SbMatrix screenToObj;
	SbViewportRegion vpr;
	SbVec3f screenOrigin;
	SbVec3f charPosition;

	objToScreen = SoProjectionMatrixElement::get(state);
	objToScreen =
	 objToScreen.multLeft(SoViewingMatrixElement::get(state));
	objToScreen =
	 objToScreen.multLeft(SoModelMatrixElement::get(state));
	screenToObj = objToScreen.inverse();

	vpr = SoViewportRegionElement::get(state);

	SbVec3f coord;

	const char *labelFormat = cdp->residueLabelString.getValue().getString();
	const SbColor *labelColors = chemColor->residueLabelColor.getValues(0);
	
	SbColor highlightColor = cdp->highlightColor.getValue();
  int32_t highlightStyle = cdp->highlightStyle.getValue();
	ChemBitmapFontCache *theFont = normalFont;

	glColor3fv(labelColors[0].getValue());
  if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE)
	{
		glColor3fv(highlightColor.getValue());

		RESIDUELOOP_START(theResidueLabelIndex)
 		
		parseResidueLabel(labelFormat, theResidue, chemData, label);

		if (chemData->residues[theResidue].checkControlPoints())
		{
			coord = chemData->getAtomCoordinates(chemData->residues[theResidue][SbResidue::RESIDUE_CONTROL_POINT_CA]);
		}
		else
		{
			if (!chemData->residues[theResidue].computeCenterOfMass(chemData,coord))
				continue;
		}

		screenOrigin = fromObjectSpace(coord, objToScreen, vpr);
		charPosition = 
		getPixelStringOffset(normalFont, lrJust, tbJust, label)
		 + screenOrigin;
		coord = toObjectSpace(charPosition, screenToObj, vpr);

		glRasterPos3fv(coord.getValue());
		theFont->drawString(label);

		RESIDUELOOP_END
	}
	else
	{
	  
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
		{
		 if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			 cdp->highlightFontSize.getValue(), highlightFont, TRUE))
		 {
			 endRenderState(state, curPixelAlignment);
			 return;
		 }
		 highlightFont->setupToRender(state);
		 theFont = highlightFont;
		}
		SbColor theColor;
		int32_t colorIndex;

		RESIDUELOOP_START(theResidueLabelIndex)
 
			parseResidueLabel(labelFormat, theResidue, chemData, label);
			
			if (chemData->residues[theResidue].checkControlPoints())
			{
				coord = chemData->getAtomCoordinates(chemData->residues[theResidue][SbResidue::RESIDUE_CONTROL_POINT_CA]);
			}
			else
			{
				if (!chemData->residues[theResidue].computeCenterOfMass(chemData,coord))
					continue;
			}

			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
			{
				if (chemData->residues[theResidue].getIndex() < 0) colorIndex = 0;
				else colorIndex = chemData->residueColorIndex[chemData->residues[theResidue].getIndex()];
				theColor = labelColors[colorIndex] + highlightColor;
				theColor.normalize();
				glColor3fv(theColor.getValue());
			}
			else
			{
				if (chemData->residues[theResidue].getIndex() < 0) colorIndex = 0;
				else colorIndex = chemData->residueColorIndex[chemData->residues[theResidue].getIndex()];
				glColor3fv(labelColors[colorIndex].getValue());
			}

			screenOrigin = fromObjectSpace(coord, objToScreen, vpr);
			charPosition = 
			getPixelStringOffset(normalFont, lrJust, tbJust, label)
				+ screenOrigin;
			coord = toObjectSpace(charPosition, screenToObj, vpr);

			glRasterPos3fv(coord.getValue());
			theFont->drawString(label);

			RESIDUELOOP_END
	}
	// Pop the state and restore the lighting and color model
	endRenderState(state, curPixelAlignment);
}

void ChemDisplay::normalResidueLabelsRPC(SoGLRenderAction *action,
																				 const MFVec2i &theResidueLabelIndex)
{
#ifdef DEBUG
printf ("In ChemDisplay::normalResidueLabelsRPN\n");
#endif

	// Check to see if there is anything to render.
	if (theResidueLabelIndex.getNum() == 0) return;

	SbString label;
	SoState *state = action->getState();

	ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

	// Get a font cache we can pull stuff out of
	if (!getFontCache(action, state, cdp->fontName.getValue(),
	 cdp->fontSize.getValue(), normalFont, TRUE)) {
	 return;
	}

	// Push the state and set the appropriate lighting and color model
	GLint curPixelAlignment;
	beginRenderState(state, &curPixelAlignment);

	normalFont->setupToRender(state);

	ChemData *chemData = (ChemData*)ChemBaseDataElement::get(state);
	ChemColor *chemColor = ChemColorElement::get(state);

	short lrJust = cdp->residueLabelLeftRightJustification.getValue();
	short tbJust = cdp->residueLabelTopBottomJustification.getValue();

	SbMatrix objToScreen;
	SbMatrix screenToObj;
	SbViewportRegion vpr;
	SbVec3f screenOrigin;
	SbVec3f charPosition;

	objToScreen = SoProjectionMatrixElement::get(state);
	objToScreen =
	 objToScreen.multLeft(SoViewingMatrixElement::get(state));
	objToScreen =
	 objToScreen.multLeft(SoModelMatrixElement::get(state));
	screenToObj = objToScreen.inverse();

	vpr = SoViewportRegionElement::get(state);

	SbVec3f coord;

	const char *labelFormat = cdp->residueLabelString.getValue().getString();
	const SbColor *labelColors = chemColor->residueLabelColor.getValues(0);
	
	RESIDUELOOP_START(theResidueLabelIndex)
 
	parseResidueLabel(labelFormat, theResidue, chemData, label);

	if (chemData->residues[theResidue].checkControlPoints())
	{
		coord = chemData->getAtomCoordinates(chemData->residues[theResidue][SbResidue::RESIDUE_CONTROL_POINT_CA]);
	}
	else
	{
		if (!chemData->residues[theResidue].computeCenterOfMass(chemData,coord))
			continue;
	}

	screenOrigin = fromObjectSpace(coord, objToScreen, vpr);
	charPosition = 
	 getPixelStringOffset(normalFont, lrJust, tbJust, label)
		 + screenOrigin;
	coord = toObjectSpace(charPosition, screenToObj, vpr);

	glColor3fv(labelColors[chemData->residueChainIndex[chemData->residues[theResidue].getChain()]].getValue());
	glRasterPos3fv(coord.getValue());
	normalFont->drawString(label);

	RESIDUELOOP_END

	// Pop the state and restore the lighting and color model
	endRenderState(state, curPixelAlignment);
}

void ChemDisplay::highlightResidueLabelsRPC(SoGLRenderAction *action,
																						const MFVec2i &theResidueLabelIndex)
{
#ifdef DEBUG
printf ("In ChemDisplay::highlightResidueLabelsOV\n");
#endif

	// Check to see if there is anything to render.
	if (theResidueLabelIndex.getNum() == 0) return;

	SbString label;
	SoState *state = action->getState();

	ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

	// Get a font cache we can pull stuff out of
	if (!getFontCache(action, state, cdp->fontName.getValue(),
	 cdp->fontSize.getValue(), normalFont, TRUE))
	{
	 return;
	}

	// Push the state and set the appropriate lighting and color model
	GLint curPixelAlignment;
	beginRenderState(state, &curPixelAlignment);

	normalFont->setupToRender(state);

	ChemData *chemData = (ChemData*)ChemBaseDataElement::get(state);
	ChemColor *chemColor = ChemColorElement::get(state);

	short lrJust = cdp->residueLabelLeftRightJustification.getValue();
	short tbJust = cdp->residueLabelTopBottomJustification.getValue();

	SbMatrix objToScreen;
	SbMatrix screenToObj;
	SbViewportRegion vpr;
	SbVec3f screenOrigin;
	SbVec3f charPosition;

	objToScreen = SoProjectionMatrixElement::get(state);
	objToScreen =
	 objToScreen.multLeft(SoViewingMatrixElement::get(state));
	objToScreen =
	 objToScreen.multLeft(SoModelMatrixElement::get(state));
	screenToObj = objToScreen.inverse();

	vpr = SoViewportRegionElement::get(state);

	SbVec3f coord;

	const char *labelFormat = cdp->residueLabelString.getValue().getString();
	const SbColor *labelColors = chemColor->residueLabelColor.getValues(0);
	
	SbColor highlightColor = cdp->highlightColor.getValue();
  int32_t highlightStyle = cdp->highlightStyle.getValue();
	ChemBitmapFontCache *theFont = normalFont;

	glColor3fv(labelColors[0].getValue());
  if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE)
	{
		glColor3fv(highlightColor.getValue());

		RESIDUELOOP_START(theResidueLabelIndex)
 
		parseResidueLabel(labelFormat, theResidue, chemData, label);

		if (chemData->residues[theResidue].checkControlPoints())
		{
			coord = chemData->getAtomCoordinates(chemData->residues[theResidue][SbResidue::RESIDUE_CONTROL_POINT_CA]);	
		}
		else
		{
			if (!chemData->residues[theResidue].computeCenterOfMass(chemData,coord))
				continue;
		}
		
		screenOrigin = fromObjectSpace(coord, objToScreen, vpr);
		charPosition = 
		getPixelStringOffset(normalFont, lrJust, tbJust, label)
		 + screenOrigin;
		coord = toObjectSpace(charPosition, screenToObj, vpr);
		
		glRasterPos3fv(coord.getValue());
		theFont->drawString(label);

	RESIDUELOOP_END
	}
	else
	{
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
		{
		 if (!getFontCache(action, state, cdp->highlightFontName.getValue(),
			 cdp->highlightFontSize.getValue(), highlightFont, TRUE))
		 {
			 endRenderState(state, curPixelAlignment);
			 return;
		 }
		 highlightFont->setupToRender(state);
		 theFont = highlightFont;
		}
		SbColor theColor;

		RESIDUELOOP_START(theResidueLabelIndex)

		parseResidueLabel(labelFormat, theResidue, chemData, label);

		if (chemData->residues[theResidue].checkControlPoints())
		{
			coord = chemData->getAtomCoordinates(chemData->residues[theResidue][SbResidue::RESIDUE_CONTROL_POINT_CA]);
		}
		else
		{
			if (!chemData->residues[theResidue].computeCenterOfMass(chemData,coord))
				continue;
		}

		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
		{
			theColor = labelColors[chemData->residueChainIndex[chemData->residues[theResidue].getChain()]] + highlightColor;
			theColor.normalize();
			glColor3fv(theColor.getValue());
		}
		else
		{
			glColor3fv(labelColors[chemData->residueChainIndex[chemData->residues[theResidue].getChain()]].getValue());
		}

		screenOrigin = fromObjectSpace(coord, objToScreen, vpr);
		charPosition = 
		 getPixelStringOffset(normalFont, lrJust, tbJust, label)
		 + screenOrigin;
		coord = toObjectSpace(charPosition, screenToObj, vpr);

		glRasterPos3fv(coord.getValue());
		theFont->drawString(label);

		RESIDUELOOP_END
	}
	// Pop the state and restore the lighting and color model
	endRenderState(state, curPixelAlignment);
}
// <-- residue labels

// --> atom, bond clipping
#undef ATOMLOOP_START
#undef BONDLOOP_START
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
	for (theAtom = atomStart; theAtom < atomEnd; theAtom++) { \

#define ATOMLOOP_END														  \
	}																		  \
}

#define BONDLOOP_START(INDEX)												  \
numBondLoops = (INDEX).getNum();											  \
for (bondLoop = 0; bondLoop < numBondLoops; bondLoop++) {					  \
	(INDEX)[bondLoop].getValue(bondStart, bondEnd);							  \
	if (bondEnd == CHEM_DISPLAY_USE_REST_OF_BONDS) {						  \
	   bondEnd = globalNumberOfBonds;										  \
	}																		  \
	else {																	  \
	   bondEnd += bondStart;												  \
	}																		  \
	for (theBond = bondStart; theBond < bondEnd; theBond++) { \

#define BONDLOOP_END														  \
	}																		  \
}
// <-- atom, bond clipping

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Implements rayPicking for atom labels when in CPK mode
//
// Use: protected

void
ChemDisplay::pickAtomLabelsWithSpheres(SoRayPickAction *action)
//
////////////////////////////////////////////////////////////////////////
{
	SoState *state = action->getState();
	//ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);
	ChemRadii *chemRadii = ChemRadiiElement::get(state);
	//ChemColor *chemColor = ChemColorElement::get(state);

	switch (chemRadii->atomRadiiBinding.getValue()) {
	  case ChemRadii::RADII_OVERALL:
	    pickAtomLabelsWithSpheresRO(action);
	break;

	  case ChemRadii::RADII_PER_ATOM:
	    pickAtomLabelsWithSpheresRA(action);
	break;

	  case ChemRadii::RADII_PER_ATOM_INDEXED:
	    pickAtomLabelsWithSpheresRI(action);
	break;

	  default:
	    break;
	}
}



////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Implements rayPicking for labels
//
// Use: protected

void
ChemDisplay::
pickAtomLabelsWithWireframe
       (SoRayPickAction *action)
//
////////////////////////////////////////////////////////////////////////
{
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(NULL, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, FALSE)) {
	       return;
       }

       // Tell the action about our current object space
       action->setObjectSpace();

       SbMatrix objToScreen;
       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));

       SbMatrix screenToObj = objToScreen.inverse();

       SbViewportRegion vpr = SoViewportRegionElement::get(state);

       // Figure out the screen-space bounding box of the characters:
       SbBox3f lineBbox, charBbox;

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);

       //SbMatrix   theMatrix;
       //SbVec3f    theScale;
       //SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool doHydrogens = cdp->showHydrogens.getValue();
       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();
       SbString label;
       SbString name;

       ATOMLOOP_START(atomLabelIndex)
	       if (!doHydrogens && chemData->getAtomicNumber(theAtom) == 1) {
		       continue;
	       }
	       coord = chemData->getAtomCoordinates(theAtom);

	       // The origin of the text on the screen is the object-space point
	       // coord
	       SbVec3f screenOrigin =
		       fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);

	       // Starting position of string, based on justification:
	       SbVec3f charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       const char *chars = label.getString();
	       SbVec3f p0, p1, p2, p3;

	       lineBbox.makeEmpty();
	       for (int chr = 0; chr < label.getLength(); chr++) {
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

		       SoPickedPoint *pp = action->addIntersection(point);
		       if (pp) {
			       ChemDetail *detail = new ChemDetail();
			       detail->setAtomBondLabelIndex(theAtom, -1);
			       pp->setDetail(detail, this);
			       switch (chemColor->atomLabelColorBinding.getValue()) {
				 case ChemColor::ATOM_LABEL_OVERALL:
				       pp->setMaterialIndex(0);
				       break;
				 case ChemColor::ATOM_LABEL_PER_ATOM_LABEL:
				       pp->setMaterialIndex(theAtom);
				       break;
				 case ChemColor::ATOM_LABEL_PER_ATOM_LABEL_INDEXED:
				       pp->setMaterialIndex(chemData->getAtomIndex(theAtom));
				       break;
				 case ChemColor::ATOM_LABEL_PER_ATOM:
				       switch (chemColor->atomColorBinding.getValue()) {
					 case ChemColor::ATOM_OVERALL:
					       pp->setMaterialIndex(0);
					       break;
					 case ChemColor::ATOM_PER_ATOM:
					       pp->setMaterialIndex(theAtom);
					       break;
					 case ChemColor::ATOM_PER_ATOM_INDEXED:
					       pp->setMaterialIndex(chemData->getAtomIndex(theAtom));
					       break;
				       }
				 default:
				       break;
			       }
			       // We'll define normal to be object-space 0,0,1:
			       pp->setObjectNormal(SbVec3f(0,0,1));
			       // And texture coordinates to be zero:
			       pp->setObjectTextureCoords(SbVec4f(0,0,0,0));
		       }
	       }
       ATOMLOOP_END
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Implements rayPicking for labels
//
// Use: protected

void
ChemDisplay::
pickAtomLabelsWithCylinders
       (SoRayPickAction *action)
//
////////////////////////////////////////////////////////////////////////
{
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(NULL, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, FALSE)) {
	       return;
       }

       // Tell the action about our current object space
       action->setObjectSpace();

       SbMatrix objToScreen;
       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));

       SbMatrix screenToObj = objToScreen.inverse();

       SbViewportRegion vpr = SoViewportRegionElement::get(state);

       // Figure out the screen-space bounding box of the characters:
       SbBox3f lineBbox, charBbox;

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool doHydrogens = cdp->showHydrogens.getValue();
       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();
       float rad = cdp->bondCylinderRadius.getValue();
       SbString label;
       SbString name;

       ATOMLOOP_START(atomLabelIndex)
	       if (!doHydrogens && chemData->getAtomicNumber(theAtom) == 1) {
		       continue;
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       theScale.setValue(rad, rad, rad);
	       theMatrix.setTransform(coord, theRotation, theScale);
	       theMatrix.multVecMatrix(theZAxis, coord);

	       // The origin of the text on the screen is the object-space point
	       // coord
	       SbVec3f screenOrigin =
		       fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);

	       // Starting position of string, based on justification:
	       SbVec3f charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       const char *chars = label.getString();
	       SbVec3f p0, p1, p2, p3;

	       lineBbox.makeEmpty();
	       for (int chr = 0; chr < label.getLength(); chr++) {
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

		       SoPickedPoint *pp = action->addIntersection(point);
		       if (pp) {
			       ChemDetail *detail = new ChemDetail();
			       detail->setAtomBondLabelIndex(theAtom, -1);
			       pp->setDetail(detail, this);
			       switch (chemColor->atomLabelColorBinding.getValue()) {
				 case ChemColor::ATOM_LABEL_OVERALL:
				       pp->setMaterialIndex(0);
				       break;
				 case ChemColor::ATOM_LABEL_PER_ATOM_LABEL:
				       pp->setMaterialIndex(theAtom);
				       break;
				 case ChemColor::ATOM_LABEL_PER_ATOM_LABEL_INDEXED:
				       pp->setMaterialIndex(chemData->getAtomIndex(theAtom));
				       break;
				 case ChemColor::ATOM_LABEL_PER_ATOM:
				       switch (chemColor->atomColorBinding.getValue()) {
					 case ChemColor::ATOM_OVERALL:
					       pp->setMaterialIndex(0);
					       break;
					 case ChemColor::ATOM_PER_ATOM:
					       pp->setMaterialIndex(theAtom);
					       break;
					 case ChemColor::ATOM_PER_ATOM_INDEXED:
					       pp->setMaterialIndex(chemData->getAtomIndex(theAtom));
					       break;
				       }
				 default:
				       break;
			       }
			       // We'll define normal to be object-space 0,0,1:
			       pp->setObjectNormal(SbVec3f(0,0,1));
			       // And texture coordinates to be zero:
			       pp->setObjectTextureCoords(SbVec4f(0,0,0,0));
		       }
	       }
       ATOMLOOP_END
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Implements rayPicking for labels
//
// Use: protected

void
ChemDisplay::
pickAtomLabelsWithSpheresRO
       (SoRayPickAction *action)
//
////////////////////////////////////////////////////////////////////////
{
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(NULL, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, FALSE)) {
	       return;
       }

       // Tell the action about our current object space
       action->setObjectSpace();

       SbMatrix objToScreen;
       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));

       SbMatrix screenToObj = objToScreen.inverse();

       SbViewportRegion vpr = SoViewportRegionElement::get(state);

       // Figure out the screen-space bounding box of the characters:
       SbBox3f lineBbox, charBbox;

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);
       //ChemRadii *chemRadii = ChemRadiiElement::get(state);

       //SbMatrix   theMatrix;
       //SbVec3f    theScale;
       //SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool doHydrogens = cdp->showHydrogens.getValue();
       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();
       //float rad;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }
       SbString label;
       SbString name;

	       //rad = chemRadii->atomRadii[0] * radScale;
       ATOMLOOP_START(atomLabelIndex)
	       if (!doHydrogens && chemData->getAtomicNumber(theAtom) == 1) {
		       continue;
	       }
	       coord = chemData->getAtomCoordinates(theAtom);

	       // The origin of the text on the screen is the object-space point
	       // coord
	       SbVec3f screenOrigin =
		       fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);

	       // Starting position of string, based on justification:
	       SbVec3f charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       const char *chars = label.getString();
	       SbVec3f p0, p1, p2, p3;

	       lineBbox.makeEmpty();
	       for (int chr = 0; chr < label.getLength(); chr++) {
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

		       SoPickedPoint *pp = action->addIntersection(point);
		       if (pp) {
			       ChemDetail *detail = new ChemDetail();
			       detail->setAtomBondLabelIndex(theAtom, -1);
			       pp->setDetail(detail, this);
			       switch (chemColor->atomLabelColorBinding.getValue()) {
				 case ChemColor::ATOM_LABEL_OVERALL:
				       pp->setMaterialIndex(0);
				       break;
				 case ChemColor::ATOM_LABEL_PER_ATOM_LABEL:
				       pp->setMaterialIndex(theAtom);
				       break;
				 case ChemColor::ATOM_LABEL_PER_ATOM_LABEL_INDEXED:
				       pp->setMaterialIndex(chemData->getAtomIndex(theAtom));
				       break;
				 case ChemColor::ATOM_LABEL_PER_ATOM:
				       switch (chemColor->atomColorBinding.getValue()) {
					 case ChemColor::ATOM_OVERALL:
					       pp->setMaterialIndex(0);
					       break;
					 case ChemColor::ATOM_PER_ATOM:
					       pp->setMaterialIndex(theAtom);
					       break;
					 case ChemColor::ATOM_PER_ATOM_INDEXED:
					       pp->setMaterialIndex(chemData->getAtomIndex(theAtom));
					       break;
				       }
				 default:
				       break;
			       }
			       // We'll define normal to be object-space 0,0,1:
			       pp->setObjectNormal(SbVec3f(0,0,1));
			       // And texture coordinates to be zero:
			       pp->setObjectTextureCoords(SbVec4f(0,0,0,0));
		       }
	       }
       ATOMLOOP_END
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Implements rayPicking for labels
//
// Use: protected

void
ChemDisplay::
pickAtomLabelsWithSpheresRA
       (SoRayPickAction *action)
//
////////////////////////////////////////////////////////////////////////
{
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(NULL, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, FALSE)) {
	       return;
       }

       // Tell the action about our current object space
       action->setObjectSpace();

       SbMatrix objToScreen;
       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));

       SbMatrix screenToObj = objToScreen.inverse();

       SbViewportRegion vpr = SoViewportRegionElement::get(state);

       // Figure out the screen-space bounding box of the characters:
       SbBox3f lineBbox, charBbox;

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool doHydrogens = cdp->showHydrogens.getValue();
       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();
       float rad;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }
       SbString label;
       SbString name;

       ATOMLOOP_START(atomLabelIndex)
	       if (!doHydrogens && chemData->getAtomicNumber(theAtom) == 1) {
		       continue;
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       rad = chemRadii->atomRadii[theAtom];
	       rad *= radScale;
	       theScale.setValue(rad, rad, rad);
	       theMatrix.setTransform(coord, theRotation, theScale);
	       theMatrix.multVecMatrix(theZAxis, coord);

	       // The origin of the text on the screen is the object-space point
	       // coord
	       SbVec3f screenOrigin =
		       fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);

	       // Starting position of string, based on justification:
	       SbVec3f charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       const char *chars = label.getString();
	       SbVec3f p0, p1, p2, p3;

	       lineBbox.makeEmpty();
	       for (int chr = 0; chr < label.getLength(); chr++) {
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

		       SoPickedPoint *pp = action->addIntersection(point);
		       if (pp) {
			       ChemDetail *detail = new ChemDetail();
			       detail->setAtomBondLabelIndex(theAtom, -1);
			       pp->setDetail(detail, this);
			       switch (chemColor->atomLabelColorBinding.getValue()) {
				 case ChemColor::ATOM_LABEL_OVERALL:
				       pp->setMaterialIndex(0);
				       break;
				 case ChemColor::ATOM_LABEL_PER_ATOM_LABEL:
				       pp->setMaterialIndex(theAtom);
				       break;
				 case ChemColor::ATOM_LABEL_PER_ATOM_LABEL_INDEXED:
				       pp->setMaterialIndex(chemData->getAtomIndex(theAtom));
				       break;
				 case ChemColor::ATOM_LABEL_PER_ATOM:
				       switch (chemColor->atomColorBinding.getValue()) {
					 case ChemColor::ATOM_OVERALL:
					       pp->setMaterialIndex(0);
					       break;
					 case ChemColor::ATOM_PER_ATOM:
					       pp->setMaterialIndex(theAtom);
					       break;
					 case ChemColor::ATOM_PER_ATOM_INDEXED:
					       pp->setMaterialIndex(chemData->getAtomIndex(theAtom));
					       break;
				       }
				 default:
				       break;
			       }
			       // We'll define normal to be object-space 0,0,1:
			       pp->setObjectNormal(SbVec3f(0,0,1));
			       // And texture coordinates to be zero:
			       pp->setObjectTextureCoords(SbVec4f(0,0,0,0));
		       }
	       }
       ATOMLOOP_END
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Implements rayPicking for labels
//
// Use: protected

void
ChemDisplay::
pickAtomLabelsWithSpheresRI
       (SoRayPickAction *action)
//
////////////////////////////////////////////////////////////////////////
{
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(NULL, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, FALSE)) {
	       return;
       }

       // Tell the action about our current object space
       action->setObjectSpace();

       SbMatrix objToScreen;
       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));

       SbMatrix screenToObj = objToScreen.inverse();

       SbViewportRegion vpr = SoViewportRegionElement::get(state);

       // Figure out the screen-space bounding box of the characters:
       SbBox3f lineBbox, charBbox;

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);
       ChemRadii *chemRadii = ChemRadiiElement::get(state);

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool doHydrogens = cdp->showHydrogens.getValue();
       const char *labelFormat = cdp->atomLabelString.getValue().getString();
       short lrJust = cdp->atomLabelLeftRightJustification.getValue();
       short tbJust = cdp->atomLabelTopBottomJustification.getValue();
       float rad;
       float radScale = cdp->atomRadiiScaleFactor.getValue();
       if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
	       (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
	       radScale *= cdp->ballStickSphereScaleFactor.getValue();
       }
       SbString label;
       SbString name;
       int32_t localAtomIndex;

       ATOMLOOP_START(atomLabelIndex)
	       if (!doHydrogens && chemData->getAtomicNumber(theAtom) == 1) {
		       continue;
	       }
	       coord = chemData->getAtomCoordinates(theAtom);
	       localAtomIndex = chemData->getAtomIndex(theAtom);
	       rad = chemRadii->atomRadii[localAtomIndex];
	       rad *= radScale;
	       theScale.setValue(rad, rad, rad);
	       theMatrix.setTransform(coord, theRotation, theScale);
	       theMatrix.multVecMatrix(theZAxis, coord);

	       // The origin of the text on the screen is the object-space point
	       // coord
	       SbVec3f screenOrigin =
		       fromObjectSpace(coord, objToScreen, vpr);

	       parseAtomLabel(labelFormat, theAtom, chemData, label);

	       // Starting position of string, based on justification:
	       SbVec3f charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       const char *chars = label.getString();
	       SbVec3f p0, p1, p2, p3;

	       lineBbox.makeEmpty();
	       for (int chr = 0; chr < label.getLength(); chr++) {
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

		       SoPickedPoint *pp = action->addIntersection(point);
		       if (pp) {
			       ChemDetail *detail = new ChemDetail();
			       detail->setAtomBondLabelIndex(theAtom, -1);
			       pp->setDetail(detail, this);
			       switch (chemColor->atomLabelColorBinding.getValue()) {
				 case ChemColor::ATOM_LABEL_OVERALL:
				       pp->setMaterialIndex(0);
				       break;
				 case ChemColor::ATOM_LABEL_PER_ATOM_LABEL:
				       pp->setMaterialIndex(theAtom);
				       break;
				 case ChemColor::ATOM_LABEL_PER_ATOM_LABEL_INDEXED:
				       pp->setMaterialIndex(chemData->getAtomIndex(theAtom));
				       break;
				 case ChemColor::ATOM_LABEL_PER_ATOM:
				       switch (chemColor->atomColorBinding.getValue()) {
					 case ChemColor::ATOM_OVERALL:
					       pp->setMaterialIndex(0);
					       break;
					 case ChemColor::ATOM_PER_ATOM:
					       pp->setMaterialIndex(theAtom);
					       break;
					 case ChemColor::ATOM_PER_ATOM_INDEXED:
					       pp->setMaterialIndex(chemData->getAtomIndex(theAtom));
					       break;
				       }
				 default:
				       break;
			       }
			       // We'll define normal to be object-space 0,0,1:
			       pp->setObjectNormal(SbVec3f(0,0,1));
			       // And texture coordinates to be zero:
			       pp->setObjectTextureCoords(SbVec4f(0,0,0,0));
		       }
	       }
       ATOMLOOP_END
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Implements rayPicking for labels
//
// Use: protected

void
ChemDisplay::
pickBondLabelsWithWireframe
       (SoRayPickAction *action)
//
////////////////////////////////////////////////////////////////////////
{
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(NULL, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, FALSE)) {
	       return;
       }

       // Tell the action about our current object space
       action->setObjectSpace();

       SbMatrix objToScreen;
       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));

       SbMatrix screenToObj = objToScreen.inverse();

       SbViewportRegion vpr = SoViewportRegionElement::get(state);

       // Figure out the screen-space bounding box of the characters:
       SbBox3f lineBbox, charBbox;

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);

       //SbMatrix   theMatrix;
       //SbVec3f    theScale;
       //SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool doHydrogens = cdp->showHydrogens.getValue();
       const char *labelFormat = cdp->bondLabelString.getValue().getString();
       short lrJust = cdp->bondLabelLeftRightJustification.getValue();
       short tbJust = cdp->bondLabelTopBottomJustification.getValue();
       SbString label;
       SbString name;

       BONDLOOP_START(bondLabelIndex)
	       int32_t from = chemData->getBondFrom(theBond);
	       int32_t to = chemData->getBondTo(theBond);
	       if (!doHydrogens) {
		       if ((chemData->getAtomicNumber(from) == 1) ||
			       (chemData->getAtomicNumber(to) == 1)) continue;
	       }
	       SbVec3f coords1 = chemData->getAtomCoordinates(from).getValue();
	       SbVec3f coords2 = chemData->getAtomCoordinates(to).getValue();
	       coord = (coords2 - coords1) * 0.5f + coords1;

	       // The origin of the text on the screen is the object-space point
	       // coord
	       SbVec3f screenOrigin =
		       fromObjectSpace(coord, objToScreen, vpr);

	       parseBondLabel(labelFormat, theBond, from, to, coords1, coords2,
		       chemData, label);

	       // Starting position of string, based on justification:
	       SbVec3f charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       const char *chars = label.getString();
	       SbVec3f p0, p1, p2, p3;

	       lineBbox.makeEmpty();
	       for (int chr = 0; chr < label.getLength(); chr++) {
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

		       SoPickedPoint *pp = action->addIntersection(point);
		       if (pp) {
			       ChemDetail *detail = new ChemDetail();
			       detail->setAtomBondLabelIndex(-1, theBond);
			       pp->setDetail(detail, this);
			       switch (chemColor->bondLabelColorBinding.getValue()) {
				 case ChemColor::BOND_LABEL_OVERALL:
				       pp->setMaterialIndex(0);
				       break;
				 case ChemColor::BOND_LABEL_PER_BOND:
				 case ChemColor::BOND_LABEL_PER_BOND_LABEL:
				       pp->setMaterialIndex(theBond);
				       break;
				 case ChemColor::BOND_LABEL_PER_BOND_INDEXED:
				 case ChemColor::BOND_LABEL_PER_BOND_LABEL_INDEXED:
				       pp->setMaterialIndex(chemData->getBondIndex(theBond));
				       break;
				 default:
				       break;
			       }
			       // We'll define normal to be object-space 0,0,1:
			       pp->setObjectNormal(SbVec3f(0,0,1));
			       // And texture coordinates to be zero:
			       pp->setObjectTextureCoords(SbVec4f(0,0,0,0));
		       }
	       }
       BONDLOOP_END
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Implements rayPicking for labels
//
// Use: protected

void
ChemDisplay::
pickBondLabelsWithCylinders
       (SoRayPickAction *action)
//
////////////////////////////////////////////////////////////////////////
{
       SoState *state = action->getState();

       ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

       // Get a font cache we can pull stuff out of
       if (!getFontCache(NULL, state, cdp->fontName.getValue(),
	       cdp->fontSize.getValue(), normalFont, FALSE)) {
	       return;
       }

       // Tell the action about our current object space
       action->setObjectSpace();

       SbMatrix objToScreen;
       objToScreen = SoProjectionMatrixElement::get(state);
       objToScreen =
	       objToScreen.multLeft(SoViewingMatrixElement::get(state));
       objToScreen =
	       objToScreen.multLeft(SoModelMatrixElement::get(state));

       SbMatrix screenToObj = objToScreen.inverse();

       SbViewportRegion vpr = SoViewportRegionElement::get(state);

       // Figure out the screen-space bounding box of the characters:
       SbBox3f lineBbox, charBbox;

       ChemBaseData *chemData = ChemBaseDataElement::get(state);
       ChemColor *chemColor = ChemColorElement::get(state);

       SbMatrix   theMatrix;
       SbRotation theRotation = currentRotation;
       SbVec3f    theScale;
       SbVec3f    theZAxis(0, 0, 1);
       SbVec3f    coord;
       SbBool doHydrogens = cdp->showHydrogens.getValue();
       const char *labelFormat = cdp->bondLabelString.getValue().getString();
       short lrJust = cdp->bondLabelLeftRightJustification.getValue();
       short tbJust = cdp->bondLabelTopBottomJustification.getValue();
       float rad = cdp->bondCylinderRadius.getValue();
       SbString label;
       SbString name;

       BONDLOOP_START(bondLabelIndex)
	       int32_t from = chemData->getBondFrom(theBond);
	       int32_t to = chemData->getBondTo(theBond);
	       if (!doHydrogens) {
		       if ((chemData->getAtomicNumber(from) == 1) ||
			       (chemData->getAtomicNumber(to) == 1)) continue;
	       }
	       SbVec3f coords1 = chemData->getAtomCoordinates(from).getValue();
	       SbVec3f coords2 = chemData->getAtomCoordinates(to).getValue();
	       coord = (coords2 - coords1) * 0.5f + coords1;
	       theScale.setValue(rad, rad, rad);
	       theMatrix.setTransform(coord, theRotation, theScale);
	       theMatrix.multVecMatrix(theZAxis, coord);

	       // The origin of the text on the screen is the object-space point
	       // coord
	       SbVec3f screenOrigin =
		       fromObjectSpace(coord, objToScreen, vpr);

	       parseBondLabel(labelFormat, theBond, from, to, coords1, coords2,
		       chemData, label);

	       // Starting position of string, based on justification:
	       SbVec3f charPosition = 
		       getPixelStringOffset(normalFont, lrJust, tbJust, label)
			       + screenOrigin;

	       const char *chars = label.getString();
	       SbVec3f p0, p1, p2, p3;

	       lineBbox.makeEmpty();
	       for (int chr = 0; chr < label.getLength(); chr++) {
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

		       SoPickedPoint *pp = action->addIntersection(point);
		       if (pp) {
			       ChemDetail *detail = new ChemDetail();
			       detail->setAtomBondLabelIndex(-1, theBond);
			       pp->setDetail(detail, this);
			       switch (chemColor->bondLabelColorBinding.getValue()) {
				 case ChemColor::BOND_LABEL_OVERALL:
				       pp->setMaterialIndex(0);
				       break;
				 case ChemColor::BOND_LABEL_PER_BOND:
				 case ChemColor::BOND_LABEL_PER_BOND_LABEL:
				       pp->setMaterialIndex(theBond);
				       break;
				 case ChemColor::BOND_LABEL_PER_BOND_INDEXED:
				 case ChemColor::BOND_LABEL_PER_BOND_LABEL_INDEXED:
				       pp->setMaterialIndex(chemData->getBondIndex(theBond));
				       break;
				 default:
				       break;
			       }
			       // We'll define normal to be object-space 0,0,1:
			       pp->setObjectNormal(SbVec3f(0,0,1));
			       // And texture coordinates to be zero:
			       pp->setObjectTextureCoords(SbVec4f(0,0,0,0));
		       }
	       }
       BONDLOOP_END
}

// --> residue labels
#undef RESIDUELOOP_START
#define RESIDUELOOP_START(INDEX)												  \
numResidueLoops = (INDEX).getNum();      \
for (residueLoop = 0; residueLoop < numResidueLoops; residueLoop++) {	  \
	(INDEX)[residueLoop].getValue(residueStart, residueEnd);		  \
	if (residueEnd == CHEM_DISPLAY_USE_REST_OF_RESIDUES) {	  \
	   residueEnd = globalNumberOfResidues;			  \
	}							  \
	else {			  \
	   residueEnd += residueStart;	  \
	}				  \
	for (theResidue = residueStart; theResidue < residueEnd; theResidue++) {

//////////////////////////////////////////////////////////////////////////
// compute residue labels bounding boxes
//////////////////////////////////////////////////////////////////////////

void ChemDisplay::eachBBoxResidueLabels(SoState *state, ChemDisplayParam *cdp,
			SbBool clipOnCenter, int32_t &bCount, ChemLabelBBox *labelBBoxes)
{
	ChemData *chemData = (ChemData*)ChemBaseDataElement::get(state);

	// Get a font cache we can pull stuff out of
	if (!getFontCache(NULL, state, cdp->fontName.getValue(),
	 cdp->fontSize.getValue(), normalFont, FALSE))
	{
	 return;
	}

	SbMatrix objToScreen;
	objToScreen = currentMVP;
	SbViewportRegion vpr = currentVPR;
	const SoClipPlaneElement *clipPlaneElt = SoClipPlaneElement::getInstance(state);

	SbMatrix screenToObj = objToScreen.inverse();
	SbVec3f screenOrigin;

	// Figure out the screen-space bounding box of the characters:
	SbBox3f screenBbox, charBbox;
	SbVec3f coord;
	SbVec3f charPosition;

	const char *labelFormat = cdp->residueLabelString.getValue().getString();
	short lrJust = cdp->residueLabelLeftRightJustification.getValue();
	short tbJust = cdp->residueLabelTopBottomJustification.getValue();

	SbString label;
	bCount = 0;

	ChemResidueStuff *stuff = NULL;
	const SbResidue *residue = NULL;
	int32_t atomIndex;
	int32_t caIndex = SbResidue::RESIDUE_CONTROL_POINT_CA;
	
	RESIDUELOOP_START(residueLabelIndex)
	
		stuff = &residueAttributes[theResidue];
		residue = chemData->residues.getValues(stuff->index);
		atomIndex = (*residue)[caIndex];
		coord = chemData->getAtomCoordinates(atomIndex);

		screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

		parseResidueLabel(labelFormat, theResidue, chemData, label);

		charPosition = 
		 getPixelStringOffset(normalFont, lrJust, tbJust, label)
			 + screenOrigin;

	 const char *chars = label.getString();
	 screenBbox.makeEmpty();

	 for (int chr = 0; chr < label.getLength(); chr++)
	 {
		 normalFont->getCharBbox(chars[chr], charBbox);
		 if (!charBbox.isEmpty())
		 {
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

	 if (!screenBbox.isEmpty())
	 {
		 // Do each of the 4 corners of the screen-space box:
		 const SbVec3f &min = screenBbox.getMin();
		 const SbVec3f &max = screenBbox.getMax();
		 SbVec3f tempMin, tempMax;
		 SbVec3f p0, p1;

		 tempMin.setValue(min[0], min[1], screenOrigin[2]);
		 tempMax.setValue(max[0], max[1], screenOrigin[2]);
		 tempMin = toObjectSpace(tempMin, screenToObj, vpr);
		 tempMax = toObjectSpace(tempMax, screenToObj, vpr);

		 if (clipOnCenter)
		 {
			 // Check to see if the center is between clipping planes.
			 // If not, the don't increment bCount.
			 // The check to see if the center is between the clipping planes
			 // needs to be performed in world space
			 // (see SoRayPickAction::isBetweenPlanes) so multiply by the
			 // currentModelMatrix
			 p0 = (tempMin + tempMax) * 0.5f;
			 currentModelMatrix.multVecMatrix(p0, p1);
			 if (isPointBetweenPlanes(p1, currentViewVolume, clipPlaneElt))
			 {
				 labelBBoxes->bbox[bCount].setBounds(tempMin, tempMax);
				 labelBBoxes->center[bCount] = p0;
				 labelBBoxes->index[bCount] = theResidue;
				 bCount++;
			 }
		 }
		 else
		 {
			 // Check to see if the bounding box is between clipping planes.
			 // If not, the don't increment bCount.
			 // The check to see if the points are between the clipping planes
			 // needs to be performed in world space
			 // (see SoRayPickAction::isBetweenPlanes) so multiply by the
			 // currentModelMatrix
			 currentModelMatrix.multVecMatrix(tempMin, p0);
			 currentModelMatrix.multVecMatrix(tempMax, p1);
			 if (isBoxBetweenPlanes(p0, p1, currentViewVolume, clipPlaneElt))
			 {
				 labelBBoxes->bbox[bCount].setBounds(tempMin, tempMax);
				 labelBBoxes->center[bCount] = (tempMin + tempMax) * 0.5f;
				 labelBBoxes->index[bCount] = theResidue;
				 bCount++;
			 }
		 }
	 }
	RESIDUELOOP_END
}

void ChemDisplay::pickResiduesLabels(SoRayPickAction *action)
{
	SoState *state = action->getState();
	ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

	if (!cdp->showResidueLabels.getValue())
		return;

	// Get a font cache we can pull stuff out of
	if (!getFontCache(NULL, state, cdp->fontName.getValue(),
	cdp->fontSize.getValue(), normalFont, FALSE)) {
	return;
	}

	// Tell the action about our current object space
	action->setObjectSpace();

	SbMatrix objToScreen;
	objToScreen = SoProjectionMatrixElement::get(state);
	objToScreen =
	objToScreen.multLeft(SoViewingMatrixElement::get(state));
	objToScreen =
	objToScreen.multLeft(SoModelMatrixElement::get(state));

	SbMatrix screenToObj = objToScreen.inverse();

	SbViewportRegion vpr = SoViewportRegionElement::get(state);
	SbVec3f screenOrigin, charPosition;

	// Figure out the screen-space bounding box of the characters:
	SbBox3f lineBbox, charBbox;

	ChemData *chemData = (ChemData*)ChemBaseDataElement::get(state);
	ChemColor *chemColor = ChemColorElement::get(state);
	//ChemRadii *chemRadii = ChemRadiiElement::get(state);

	const char *labelFormat = cdp->residueLabelString.getValue().getString();
	short lrJust = cdp->residueLabelLeftRightJustification.getValue();
	short tbJust = cdp->residueLabelTopBottomJustification.getValue();

	SbString label;
	SbVec3f coord;

	RESIDUELOOP_START(residueLabelIndex)

	if (chemData->residues[theResidue].checkControlPoints())
	{
		coord = chemData->getAtomCoordinates(chemData->residues[theResidue][SbResidue::RESIDUE_CONTROL_POINT_CA]);
	}
	else
	{
		if (!chemData->residues[theResidue].computeCenterOfMass(chemData,coord))
			continue;
	}

	screenOrigin = fromObjectSpace(coord, objToScreen, vpr);

	parseResidueLabel(labelFormat, theResidue, chemData, label);

	charPosition = 
	 getPixelStringOffset(normalFont, lrJust, tbJust, label)
		 + screenOrigin;

	 const char *chars = label.getString();
	 SbVec3f p0, p1, p2, p3;

	 lineBbox.makeEmpty();
	 for (int chr = 0; chr < label.getLength(); chr++)
	 {
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
		 action->intersect(p2, p1, p3, point, junk1, junk2))
	 {

		 SoPickedPoint *pp = action->addIntersection(point);
		 if (pp != NULL)
		 {
			 ChemDetail *detail = new ChemDetail();
			 detail->setResidueLabelIndex(theResidue);
			 pp->setDetail(detail, this);
			 
			 switch (chemColor->residueLabelColorBinding.getValue())
			 {
				case ChemColor::RESIDUE_LABEL_OVERALL:
				 pp->setMaterialIndex(0);
				 break;
				case ChemColor::RESIDUE_LABEL_PER_INDEX:
				{
					int index = 0;
					if (chemData->residues[theResidue].getIndex() > 0)
					index = chemData->residueColorIndex[chemData->residues[theResidue].getIndex()];
					pp->setMaterialIndex(index);
				}
				 break;
				case ChemColor::RESIDUE_LABEL_PER_CHAIN:
				 pp->setMaterialIndex(chemData->residueChainIndex[chemData->residues[theResidue].getChain()]);
				 break;
				default:
				 break;
			 }
			 // We'll define normal to be object-space 0,0,1:
			 pp->setObjectNormal(SbVec3f(0,0,1));
			 // And texture coordinates to be zero:
			 pp->setObjectTextureCoords(SbVec4f(0,0,0,0));
		 }
	 }
 RESIDUELOOP_END
}
// <-- residue labels

//
// End of TEXT section
//

#undef ATOMLOOP_START
#undef ATOMLOOP_END
#undef BONDLOOP_START
#undef BONDLOOP_END
// --> residue labels
#undef RESIDUELOOP_START
#undef RESIDUELOOP_END
// <-- residue labels
