/*
 * ChemDisplayCylinders.pp
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
//#endif
#include <gl/gl.h>
#endif

#include "inv/ChemKit/ChemDetail.H"
#include "inv/ChemKit/ChemDisplay.H"
#include "inv/ChemKit/ChemBaseDataElement.H"
#include "inv/ChemKit/ChemColorElement.H"
#include "inv/ChemKit/ChemDisplayParamElement.H"
#include "inv/ChemKit/ChemRadiiElement.H"
#include "inv/ChemKit/ChemUnitCylinder.H"

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

// --> SGB
#include "inv/ChemKit/ChemLOD.H"
// <-- SGB

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

static int numBondLoops;
static int bondLoop;
static int32_t bondStart;
static int32_t bondEnd;
static int32_t theBond;

// --> octree duplicates
#define BNDDUP(ID) \
assert(ID>=0 && ID<globalNumberOfBonds && renderedBonds != NULL); \
if (renderedBonds[ID]) \
continue; \
else renderedBonds[ID] = true;
// <-- octree duplicates

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
	for (theBond = bondStart; theBond < bondEnd; theBond++) {

#define BONDLOOP_END														  \
	}																		  \
}

// --> roundcap optimization
#define ADJUST_CYLINDER(ATOM,BOOLEAN) \
BOOLEAN = true;  \
if (cdp->displayStyle.getValue() != ChemDisplayParam::DISPLAY_STICK) \
{ \
int __i; \
for (__i=0; __i<atomIndex.getNum();__i++) \
{ \
const SbVec2i range = *atomIndex.getValues(__i); \
if (range[1] == -1) BOOLEAN  = (ATOM >= range[0]); \
else BOOLEAN = (range[0]>=ATOM && ATOM<=range[1]); \
if (!BOOLEAN) break; \
} \
}
// <-- roundcap optimization

// --> EGB && SGB
#define PRE_ADJUST_CYLINDER \
float radScale; \
ChemRadii* chemRadii; \
const float *atomRadii; \
chemRadii = ChemRadiiElement::get(state); \
atomRadii = chemRadii->atomRadii.getValues(0); \
radScale = cdp->atomRadiiScaleFactor.getValue(); \
if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)|| (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) \
	   radScale *= cdp->ballStickSphereScaleFactor.getValue();

#define POST_ADJUST_CYLINDER \
if (cdp->displayStyle.getValue() != ChemDisplayParam::DISPLAY_STICK) { \
bool renderFrom, renderTo; \
ADJUST_CYLINDER(from,renderFrom);\
ADJUST_CYLINDER(to,renderTo); \
float radFrom, radTo; \
float dirlength,t1,t2; \
SbVec3f dir; \
int localAtomIndex; \
switch (chemRadii->atomRadiiBinding.getValue() ) \
{ \
	case ChemRadii::RADII_OVERALL: \
	radFrom = radTo = atomRadii[0] * radScale; \
	break; \
	case ChemRadii::RADII_PER_ATOM: \
		radFrom = atomRadii[from] * radScale; \
		radTo = atomRadii[to] * radScale; \
		break; \
	case ChemRadii::RADII_PER_ATOM_INDEXED: \
		localAtomIndex = chemData->getAtomIndex(from); \
		radFrom = atomRadii[localAtomIndex] * radScale; \
		localAtomIndex = chemData->getAtomIndex(to); \
		radTo = atomRadii[localAtomIndex] * radScale; \
		break; \
	default: break; \
} \
dir = coords2 - coords1; \
dirlength = dir.length(); \
t1 = (float)radFrom/dirlength; \
t1-=0.05f; \
if (renderFrom) coords1 = (t1*dir) + coords1; \
t2 = (float)radTo/dirlength; \
t2-=0.05f; \
if (renderTo) coords2 =  coords2 - (t2*dir); \
}

#define DOLOD(BXAX) \
if (displayStyle == ChemDisplayParam::BONDCYLINDER_LOD_NOCAP || displayStyle == ChemDisplayParam::BONDCYLINDER_LOD_FLATCAP || displayStyle == ChemDisplayParam::BONDCYLINDER_LOD_ROUNDCAP) { \
	LODnormalCylinder##BXAX(theBondIndex,state,chemColor,cdp,chemData); \
	glDisable(GL_COLOR_MATERIAL); \
	return; \
} else if (clipBonds){ \
	clipNormalCylinder##BXAX(theBondIndex,state,chemColor,cdp,chemData); \
	glDisable(GL_COLOR_MATERIAL); \
	return; \
}

#define DOHLOD(BXAX) \
if (displayStyle == ChemDisplayParam::BONDCYLINDER_LOD_NOCAP || displayStyle == ChemDisplayParam::BONDCYLINDER_LOD_FLATCAP || displayStyle == ChemDisplayParam::BONDCYLINDER_LOD_ROUNDCAP) { \
	LODhighlightCylinder##BXAX(theBondIndex,state,chemColor,cdp,chemData); \
	glDisable(GL_COLOR_MATERIAL); \
	return; \
}
// <-- EGB && SGB
//
// Start of CYLINDER section
//

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Checks to see if the unit cylinder needs to be recreated.
//	  Also retrieves values for state, chemColor and cdp.
//
// Use: protected

void ChemDisplay::setupCylinders(SoGLRenderAction *action, SoState *&state,
	ChemColor *&chemColor, ChemDisplayParam *&cdp,
	ChemBaseData *&chemData)
//
////////////////////////////////////////////////////////////////////////
{
	int 	displayStyle;
	SbBool	doNormals, doTextures;

	state     = action->getState();
	chemColor = ChemColorElement::get(state);
	cdp       = ChemDisplayParamElement::get(state);
	chemData  = ChemBaseDataElement::get(state);
	displayStyle = cdp->bondCylinderDisplayStyle.getValue();

	// See if texturing is enabled
	doTextures = SoGLTextureEnabledElement::get(state);

	// See if we need to generate normals
#ifdef IV2_0
	doNormals = (SoLightModelElement::get(state) !=
		SoLightModelElement::BASE_COLOR);
#else
	doNormals = (SoLazyElement::getLightModel(state) !=
		SoLazyElement::BASE_COLOR);
#endif

	// Compute depth based on complexity
	float bcc = cdp->bondCylinderComplexity.getValue();

	// Checks to perform:
	//	  Need to regenerate unit cylinder vertices because of complexity 
	//	  change?
	//		  if yes, then also regenerate display list depending on the
	//			  values for doTextures and doNormals
	//		  if no, do we need to regenerate the display list? (dependent on
	//			  last values of doTextures and doNormals versus
	//			  the current values.

	if ((lastCylinderComplexity != bcc)	|| 
		(lastCylinderDisplayStyle != displayStyle)) {

		lastCylinderComplexity = bcc;
		lastCylinderDisplayStyle = displayStyle;

		if (unitCylinder != NULL) delete unitCylinder;
		unitCylinder = new ChemUnitCylinder(displayStyle, bcc);

// --> EGB && SGB regenerate lod
		lodSelector->changeCylinderComplexity(bcc,displayStyle);
// <-- EGB && SGB regenerate lod
	}
	lastCylinderDoNormals = doNormals;
	lastCylinderDoTextures = doTextures;

// --> compute cylinder lod
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
//	  Performs GL rendering of the cylinders representing bonds in the
//	  molecule.  Invoked by the GLRender method.
//
// Use: protected

void
ChemDisplay::renderBondsAsCylinders(SoGLRenderAction *action)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::renderBondsAsCylinders\n");
#endif

	SoState *state;
	ChemColor *chemColor;
	ChemDisplayParam *cdp;
	ChemBaseData *chemData;

	// Check to see if the unit cylinder needs to be remade and
	// get values for state, chemColor and cdp.
	setupCylinders(action, state, chemColor, cdp, chemData);

	// Make sure things are set up correctly for a solid object
	beginSolidShape(action);

// --> multiple scene graphs fix
	glEnable(GL_LIGHTING);
// <-- multiple scene graphs fix

#ifndef IV2_0
	int32_t myMask;
	if (cdp->highlightStyle.getValue() ==
		ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
		myMask = SoLazyElement::DIFFUSE_MASK | SoLazyElement::EMISSIVE_MASK;
	}
	else {
		myMask = SoLazyElement::DIFFUSE_MASK;
	}

	SoGLLazyElement *lazyElt = (SoGLLazyElement *)
			SoLazyElement::getInstance(state);
	lazyElt->send(state, SoLazyElement::ALL_MASK &( ~myMask));
#endif

// --> EGB && SGB
	beginCylinders(cdp->bondCylinderDisplayStyle.getValue());
// <-- EGB && SGB

	// Render the cylinders
	switch (chemColor->bondColorBinding.getValue()) {
	  case ChemColor::BOND_OVERALL:
		normalCylinderOV(vnormalBondIndex, state, chemColor->bondColor[0],
			cdp, chemData);
		highlightCylinderOV(vhighlightBondIndex, state, chemColor, cdp, 
			chemData);
		break;
	  case ChemColor::BOND_PER_BOND:
		normalCylinderBB(vnormalBondIndex, state, chemColor, cdp, chemData);
		highlightCylinderBB(vhighlightBondIndex, state, chemColor, cdp, 
			chemData);
		break;
	  case ChemColor::BOND_PER_BOND_INDEXED:
		normalCylinderBI(vnormalBondIndex, state, chemColor, cdp, chemData);
		highlightCylinderBI(vhighlightBondIndex, state, chemColor, cdp, 
			chemData);
		break;
	  case ChemColor::BOND_PER_ATOM:
		switch (chemColor->atomColorBinding.getValue()) {
		  case ChemColor::ATOM_OVERALL:
			normalCylinderOV(vnormalBondIndex, state, chemColor->atomColor[0],
				cdp, chemData);
			highlightCylinderOV(vhighlightBondIndex, state, chemColor, cdp, 
				chemData);
			break;
		  case ChemColor::ATOM_PER_ATOM:
// --> roundcap optimization
//			normalCylinderBAAA(normalBondIndex, state, chemColor, cdp, 
//				chemData);
			normalCylinderBAAA(vnormalBondIndex, state, chemColor, cdp, 
				chemData,action);
// <-- roundcap optimization
			highlightCylinderBAAA(vhighlightBondIndex, state, chemColor, 
				cdp, chemData);
			break;
		  case ChemColor::ATOM_PER_ATOM_INDEXED:
// --> roundcap optimization
//			normalCylinderBAAI(normalBondIndex, state, chemColor, cdp, 
//				chemData);
			normalCylinderBAAI(vnormalBondIndex, state, chemColor, cdp, 
				chemData,action);
// <-- roundcap optimization
			highlightCylinderBAAI(vhighlightBondIndex, state, chemColor, 
				cdp, chemData);
			break;
		  default:
			break;
		}
	  case ChemColor::BOND_PER_ATOM_HALF_BONDED:
		switch (chemColor->atomColorBinding.getValue()) {
		  case ChemColor::ATOM_OVERALL:
			normalCylinderOV(vnormalBondIndex, state, chemColor->atomColor[0],
				cdp, chemData);
			highlightCylinderOV(vhighlightBondIndex, state, chemColor, cdp, 
				chemData);
			break;
		  case ChemColor::ATOM_PER_ATOM:
			normalCylinderBHAA(vnormalBondIndex, state, chemColor, cdp, 
				chemData);
			highlightCylinderBHAA(vhighlightBondIndex, state, chemColor, 
				cdp, chemData);
			break;
		  case ChemColor::ATOM_PER_ATOM_INDEXED:
			normalCylinderBHAI(vnormalBondIndex, state, chemColor, cdp, 
				chemData);
			highlightCylinderBHAI(vhighlightBondIndex, state, chemColor, 
				cdp, chemData);
			break;
		  default:
			break;
		}
	  default:
		break;
	}

#ifndef IV2_0
	lazyElt->reset(state, myMask);
#endif

// --> EGB && SGB
	endCylinders(cdp->bondCylinderDisplayStyle.getValue());
// <-- EGB && SGB

	endSolidShape(action);
}

////////////////////////////////////////////////////////////////////////
// 
// Description:
//    Renders the cylinder "display list" for each bond in the molecule.
//    Invoked by the GLRender method.
// 
// Use: private

void
ChemDisplay::normalCylinderOV
	(const MFVec2i &theBondIndex, SoState *state, const SbColor &chemColor,
	const ChemDisplayParam *cdp, const ChemBaseData *chemData)
//
////////////////////////////////////////////////////////////////////////
{
	int32_t 	from, to;
	float		rad;
	SbVec3f 	coords1, coords2;

	// Check to see if there's anything to render.
	if (theBondIndex.getNum() == 0) return;

	int displayStyle = cdp->bondCylinderDisplayStyle.getValue();
	SbBool doHydrogens = cdp->showHydrogens.getValue();

	rad = cdp->bondCylinderRadius.getValue();

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

// --> EGB && SGB
	DOLOD(OV)
// <-- EGB && SGB

	glColor3fv(chemColor.getValue());

// --> EGB && SGB
	PRE_ADJUST_CYLINDER
// <--EGB && SGB

// --> EGB && SGB
	SoGLRenderAction *action = (SoGLRenderAction *)state->getAction();
// <-- EGB && SGB
	BONDLOOP_START(theBondIndex)
    // --> octree duplicates
    BNDDUP(theBond);
    // <-- octree duplicates
		from = chemData->getBondFrom(theBond);
		to = chemData->getBondTo(theBond);
		if (!doHydrogens) {
			if ((chemData->getAtomicNumber(from) == 1) ||
				(chemData->getAtomicNumber(to) == 1)) continue;
		}
		coords1 = chemData->getAtomCoordinates(from).getValue();
		coords2 = chemData->getAtomCoordinates(to).getValue();
// --> EGB && SGB
	POST_ADJUST_CYLINDER
// <--EGB && SGB
// --> EGB && SGB
//		unitCylinder->render(coords1, coords2, rad,lastCylinderDoNormals, lastCylinderDoTextures);
// --> roundcap optimization
		//unitCylinder->render(coords1, coords2, rad,lastCylinderDoNormals, lastCylinderDoTextures,
		//displayStyle,,action);
// <-- EGB && SGB
		SbVec2i fromTo; fromTo.setValue(from,to);
		unitCylinder->render(coords1, coords2, rad,lastCylinderDoNormals, lastCylinderDoTextures,
		displayStyle,fromTo,this,action);
// <-- roundcap optimization

	BONDLOOP_END
	glDisable(GL_COLOR_MATERIAL);
}

//
// The following preprocessor-generated functions handle the various
// ways to render the cylinders.  These all have to do with how the
// bonds are colored.
// BB ... BondPerBond
// BI ... BondPerBondIndexed
// BA ... BondPerAtom
// BH ... BondPerAtomHalfBonded
// AA ... AtomPerAtom
// AI ... AtomPerAtomIndexed
//
//

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Renders the cylinder "display list" for each bond in the molecule.
//	  Invoked by the GLRender method.
//
// Use: private
void
ChemDisplay::
normalCylinderBB
    (const MFVec2i &theBondIndex, SoState *state, const ChemColor *chemColor,
	const ChemDisplayParam *cdp, const ChemBaseData *chemData)
//
////////////////////////////////////////////////////////////////////////
{
	int32_t		from, to;
	float		rad;
	SbVec3f 	coords1, coords2;

	// Check to see if there's anything to render.
	if (theBondIndex.getNum() == 0) return;

	int displayStyle = cdp->bondCylinderDisplayStyle.getValue();
	SbBool doHydrogens = cdp->showHydrogens.getValue();

	rad = cdp->bondCylinderRadius.getValue();

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

// --> EGB && SGB
	DOLOD(BB)
// <--EGB && SGB

// --> EGB && SGB
	PRE_ADJUST_CYLINDER
// <--EGB && SGB
// --> EGB && SGB
	SoGLRenderAction *action = (SoGLRenderAction *)state->getAction();
// <-- EGB && SGB
	const SbColor *bondColors = chemColor->bondColor.getValues(0);
	BONDLOOP_START(theBondIndex)
    // --> octree duplicates
    BNDDUP(theBond);
    // <-- octree duplicates
		from = chemData->getBondFrom(theBond);
		to = chemData->getBondTo(theBond);
		if (!doHydrogens) {
			if ((chemData->getAtomicNumber(from) == 1) ||
				(chemData->getAtomicNumber(to) == 1)) continue;
		}
		coords1 = chemData->getAtomCoordinates(from).getValue();
		coords2 = chemData->getAtomCoordinates(to).getValue();
// --> EGB && SGB
		POST_ADJUST_CYLINDER
// <--EGB && SGB
		glColor3fv(bondColors[theBond].getValue());
// --> roundcap optimization
// --> EGB && SGB
// //		unitCylinder->render(coords1, coords2, rad,lastCylinderDoNormals, lastCylinderDoTextures);
//		unitCylinder->render(coords1, coords2, rad,lastCylinderDoNormals, lastCylinderDoTextures,
//		displayStyle,action);
// // <-- EGB && SGB
		SbVec2i fromTo; fromTo.setValue(from,to);
		unitCylinder->render(coords1, coords2, rad,lastCylinderDoNormals, lastCylinderDoTextures,
		displayStyle,fromTo,this,action);
// <-- roundcap optimization
	BONDLOOP_END
	glDisable(GL_COLOR_MATERIAL);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Renders the cylinder "display list" for each bond in the molecule.
//	  Invoked by the GLRender method.
//
// Use: private
void
ChemDisplay::
normalCylinderBI
    (const MFVec2i &theBondIndex, SoState *state, const ChemColor *chemColor,
	const ChemDisplayParam *cdp, const ChemBaseData *chemData)
//
////////////////////////////////////////////////////////////////////////
{
	int32_t		from, to;
	float		rad;
	SbVec3f 	coords1, coords2;

	// Check to see if there's anything to render.
	if (theBondIndex.getNum() == 0) return;

	int displayStyle = cdp->bondCylinderDisplayStyle.getValue();
	SbBool doHydrogens = cdp->showHydrogens.getValue();

	rad = cdp->bondCylinderRadius.getValue();

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

// --> EGB && SGB
	DOLOD(BI)
// <--EGB && SGB

// --> EGB && SGB
	PRE_ADJUST_CYLINDER
// <--EGB && SGB
// --> EGB && SGB
	SoGLRenderAction *action = (SoGLRenderAction *)state->getAction();
// <-- EGB && SGB
	const SbColor *bondColors = chemColor->bondColor.getValues(0);
	BONDLOOP_START(theBondIndex)
    // --> octree duplicates
    BNDDUP(theBond);
    // <-- octree duplicates
		from = chemData->getBondFrom(theBond);
		to = chemData->getBondTo(theBond);
		if (!doHydrogens) {
			if ((chemData->getAtomicNumber(from) == 1) ||
				(chemData->getAtomicNumber(to) == 1)) continue;
		}
		coords1 = chemData->getAtomCoordinates(from).getValue();
		coords2 = chemData->getAtomCoordinates(to).getValue();
// --> EGB && SGB
	POST_ADJUST_CYLINDER
// <--EGB && SGB
		glColor3fv(bondColors[chemData->getBondIndex(theBond)].getValue());
// --> roundcap optimization
// // --> EGB && SGB
// //		unitCylinder->render(coords1, coords2, rad,lastCylinderDoNormals, lastCylinderDoTextures);
//		unitCylinder->render(coords1, coords2, rad,lastCylinderDoNormals, lastCylinderDoTextures,
//		displayStyle,action);
		SbVec2i fromTo; fromTo.setValue(from,to);
		unitCylinder->render(coords1, coords2, rad,lastCylinderDoNormals, lastCylinderDoTextures,
		displayStyle,fromTo,this,action);
// <-- EGB && SGB
// <-- roundcap optimization
	BONDLOOP_END
	glDisable(GL_COLOR_MATERIAL);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Renders the cylinder "display list" for each bond in the molecule.
//	  Invoked by the GLRender method.
//
// Use: private
// --> roundcap optimization
//void
//ChemDisplay::
//normalCylinderBAAA
//    (const MFVec2i &theBondIndex, SoState *state, const ChemColor *chemColor,
//	const ChemDisplayParam *cdp, const ChemBaseData *chemData)
void
ChemDisplay::
normalCylinderBAAA
    (const MFVec2i &theBondIndex, SoState *state, const ChemColor *chemColor,
	const ChemDisplayParam *cdp, const ChemBaseData *chemData, SoGLRenderAction* action)
// <-- roundcap optimization
//
////////////////////////////////////////////////////////////////////////
{
	int32_t		from, to;
	float		rad;
	SbVec3f 	coords1, coords2;

	// Check to see if there's anything to render.
	if (theBondIndex.getNum() == 0) return;

	int displayStyle = cdp->bondCylinderDisplayStyle.getValue();
	SbBool doHydrogens = cdp->showHydrogens.getValue();

	rad = cdp->bondCylinderRadius.getValue();

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

// --> EGB && SGB
	DOLOD(BAAA)
// <--EGB && SGB


// --> EGB && SGB
	PRE_ADJUST_CYLINDER
// <--EGB && SGB

	const SbColor *atomColors = chemColor->atomColor.getValues(0);
	SbColor colorFrom, colorTo;
	BONDLOOP_START(theBondIndex)
    // --> octree duplicates
    BNDDUP(theBond);
    // <-- octree duplicates
		from = chemData->getBondFrom(theBond);
		to = chemData->getBondTo(theBond);
		if (!doHydrogens) {
			if ((chemData->getAtomicNumber(from) == 1) ||
				(chemData->getAtomicNumber(to) == 1)) continue;
		}
		colorFrom = atomColors[from];
		colorTo = atomColors[to];
		coords1 = chemData->getAtomCoordinates(from);
		coords2 = chemData->getAtomCoordinates(to);

// --> EGB && SGB
	POST_ADJUST_CYLINDER
// <--EGB && SGB
// --> roundcap optimization
//		unitCylinder->renderWithInterpolatedColor(coords1, coords2, rad,
//			colorFrom, colorTo, 
//			lastCylinderDoNormals, lastCylinderDoTextures);
			SbVec2i fromTo; fromTo.setValue(from,to);
			unitCylinder->renderWithInterpolatedColor(coords1, coords2, rad,
			colorFrom, colorTo, 
			lastCylinderDoNormals, lastCylinderDoTextures,displayStyle,fromTo,this,action);
// <-- roundcap optimization
	BONDLOOP_END
	glDisable(GL_COLOR_MATERIAL);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Renders the cylinder "display list" for each bond in the molecule.
//	  Invoked by the GLRender method.
//
// Use: private
// --> roundcap optimization
//void
//ChemDisplay::
//normalCylinderBAAI
//    (const MFVec2i &theBondIndex, SoState *state, const ChemColor *chemColor,
//	const ChemDisplayParam *cdp, const ChemBaseData *chemData)
void
ChemDisplay::
normalCylinderBAAI
    (const MFVec2i &theBondIndex, SoState *state, const ChemColor *chemColor,
	const ChemDisplayParam *cdp, const ChemBaseData *chemData, SoGLRenderAction* action)
// <-- roundcap optimization
//
////////////////////////////////////////////////////////////////////////
{
	int32_t		from, to;
	float		rad;
	SbVec3f 	coords1, coords2;

	// Check to see if there's anything to render.
	if (theBondIndex.getNum() == 0) return;

	int displayStyle = cdp->bondCylinderDisplayStyle.getValue();
	SbBool doHydrogens = cdp->showHydrogens.getValue();

	rad = cdp->bondCylinderRadius.getValue();

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

// --> EGB && SGB
	DOLOD(BAAI)
// <--EGB && SGB

// --> EGB && SGB
	PRE_ADJUST_CYLINDER
// <--EGB && SGB

	const SbColor *atomColors = chemColor->atomColor.getValues(0);
	SbColor colorFrom, colorTo;
	BONDLOOP_START(theBondIndex)
    // --> octree duplicates
    BNDDUP(theBond);
    // <-- octree duplicates
		from = chemData->getBondFrom(theBond);
		to = chemData->getBondTo(theBond);
		if (!doHydrogens) {
			if ((chemData->getAtomicNumber(from) == 1) ||
				(chemData->getAtomicNumber(to) == 1)) continue;
		}
		colorFrom = atomColors[chemData->getAtomIndex(from)];
		colorTo = atomColors[chemData->getAtomIndex(to)];
		coords1 = chemData->getAtomCoordinates(from);
		coords2 = chemData->getAtomCoordinates(to);

// --> EGB && SGB
	POST_ADJUST_CYLINDER
// <--EGB && SGB
// --> roundcap optimization
//		unitCylinder->renderWithInterpolatedColor(coords1, coords2, rad,
//			colorFrom, colorTo, 
//			lastCylinderDoNormals, lastCylinderDoTextures);
		SbVec2i fromTo; fromTo.setValue(from,to);
		unitCylinder->renderWithInterpolatedColor(coords1, coords2, rad,
			colorFrom, colorTo, 
			lastCylinderDoNormals, lastCylinderDoTextures,displayStyle,fromTo,this,action);
// <-- roundcap optimization
	BONDLOOP_END
	glDisable(GL_COLOR_MATERIAL);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Renders the cylinder "display list" for each bond in the molecule.
//	  Invoked by the GLRender method.
//
// Use: private
void
ChemDisplay::
normalCylinderBHAA
    (const MFVec2i &theBondIndex, SoState *state, const ChemColor *chemColor,
	const ChemDisplayParam *cdp, const ChemBaseData *chemData)
//
////////////////////////////////////////////////////////////////////////
{
	int32_t		from, to;
	float		rad;
	SbVec3f 	coords1, coords2;

	// Check to see if there's anything to render.
	if (theBondIndex.getNum() == 0) return;

	int displayStyle = cdp->bondCylinderDisplayStyle.getValue();
	SbBool doHydrogens = cdp->showHydrogens.getValue();

	rad = cdp->bondCylinderRadius.getValue();

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

// --> EGB && SGB
	DOLOD(BHAA)
// <-- EGB && SGB

// --> EGB && SGB
	PRE_ADJUST_CYLINDER
// <-- EGB && SGB

	const SbColor *atomColors = chemColor->atomColor.getValues(0);
	SbColor colorFrom, colorTo;

// --> EGB && SGB
	SoGLRenderAction *action = (SoGLRenderAction *)state->getAction();
// <-- EGB && SGB
	BONDLOOP_START(theBondIndex)
    // --> octree duplicates
    BNDDUP(theBond);
    // <-- octree duplicates
		from = chemData->getBondFrom(theBond);
		to = chemData->getBondTo(theBond);
		if (!doHydrogens) {
			if ((chemData->getAtomicNumber(from) == 1) ||
				(chemData->getAtomicNumber(to) == 1)) continue;
		}
		colorFrom = atomColors[from];
		colorTo = atomColors[to];
		coords1 = chemData->getAtomCoordinates(from).getValue();
		coords2 = chemData->getAtomCoordinates(to).getValue();

// --> EGB && SGB
		POST_ADJUST_CYLINDER
// <-- EGB && SGB
// --> roundcap optimization
    SbVec2i fromTo; fromTo[0] = from; fromTo[1] = to;
// <-- roundcap optimization
		unitCylinder->renderHalfBonded(coords1, coords2, rad,
			colorFrom, colorTo, 
			lastCylinderDoNormals, lastCylinderDoTextures,displayStyle, fromTo, this, action);

	BONDLOOP_END
	glDisable(GL_COLOR_MATERIAL);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Renders the cylinder "display list" for each bond in the molecule.
//	  Invoked by the GLRender method.
//
// Use: private
void
ChemDisplay::
normalCylinderBHAI
    (const MFVec2i &theBondIndex, SoState *state, const ChemColor *chemColor,
	const ChemDisplayParam *cdp, const ChemBaseData *chemData)
//
////////////////////////////////////////////////////////////////////////
{
	int32_t		from, to;
	float		rad;
	SbVec3f 	coords1, coords2;

	// Check to see if there's anything to render.
	if (theBondIndex.getNum() == 0) return;

	int displayStyle = cdp->bondCylinderDisplayStyle.getValue();
	SbBool doHydrogens = cdp->showHydrogens.getValue();

	rad = cdp->bondCylinderRadius.getValue();

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

// --> EGB && SGB
	DOLOD(BHAI)
// <-- EGB && SGB

// --> EGB && SGB
	PRE_ADJUST_CYLINDER
// <-- EGB && SGB
	const SbColor *atomColors = chemColor->atomColor.getValues(0);
	SbColor colorFrom, colorTo;
	SoGLRenderAction *action = (SoGLRenderAction*)state->getAction();
	BONDLOOP_START(theBondIndex)
    // --> octree duplicates
    BNDDUP(theBond);
    // <-- octree duplicates
		from = chemData->getBondFrom(theBond);
		to = chemData->getBondTo(theBond);
		if (!doHydrogens) {
			if ((chemData->getAtomicNumber(from) == 1) ||
				(chemData->getAtomicNumber(to) == 1)) continue;
		}
		colorFrom = atomColors[chemData->getAtomIndex(from)];
		colorTo = atomColors[chemData->getAtomIndex(to)];
		coords1 = chemData->getAtomCoordinates(from).getValue();
		coords2 = chemData->getAtomCoordinates(to).getValue();
// --> EGB && SGB
		POST_ADJUST_CYLINDER
// <-- EGB && SGB
// --> EGB && SGB
//		unitCylinder->renderHalfBonded(coords1, coords2, rad,
//			colorFrom, colorTo, 
//			lastCylinderDoNormals, lastCylinderDoTextures);
// --> roundcap optimization
    SbVec2i fromTo; fromTo[0] = from; fromTo[1] = to;
// <-- roundcap optimization
		unitCylinder->renderHalfBonded(coords1, coords2, rad,
			colorFrom, colorTo, 
			lastCylinderDoNormals, lastCylinderDoTextures, displayStyle, fromTo, this, action);
// <-- EGB && SGB
	BONDLOOP_END
	glDisable(GL_COLOR_MATERIAL);
}


//
// The following preprocessor-generated functions handle the various
// ways to render the cylinders.  These all have to do with how the
// bonds are colored.
// OV ... BondOverall or AtomOverall
// BB ... BondPerBond
// BI ... BondPerBondIndexed
// BA ... BondPerAtom
// BH ... BondPerAtomHalfBonded
// AA ... AtomPerAtom
// AI ... AtomPerAtomIndexed
//
//

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Renders the cylinder "display list" for each bond in the molecule.
//	  Invoked by the GLRender method.
//
// Use: private
void
ChemDisplay::
highlightCylinderOV
    (const MFVec2i &theBondIndex, SoState *state, const ChemColor *chemColor,
	const ChemDisplayParam *cdp, const ChemBaseData *chemData)
//
////////////////////////////////////////////////////////////////////////
{
	int32_t		from, to;
	float		rad;
	SbVec3f 	coords1, coords2;
	SbVec3f		theCapScale;

	// Check to see if there's anything to render.
	if (theBondIndex.getNum() == 0) return;

	int displayStyle = cdp->bondCylinderDisplayStyle.getValue();
	SbBool doHydrogens = cdp->showHydrogens.getValue();

	rad = cdp->bondCylinderRadius.getValue();
	theCapScale.setValue(rad, rad, rad);

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

// --> EGB && SGB
	DOHLOD(OV)
	SoGLRenderAction *action = (SoGLRenderAction*)state->getAction();
// <-- EGB && SGB

	int32_t highlightStyle = cdp->highlightStyle.getValue();
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
		const SbColor &theColor = cdp->highlightColor.getValue();
		glMaterialfv(GL_FRONT, GL_EMISSION,
			SbVec4f(theColor[0], theColor[1], theColor[2], 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
		const SbColor &theColor = cdp->highlightColor.getValue();
		glColor3fv(theColor.getValue());
	}
// --> EGB && SGB
	PRE_ADJUST_CYLINDER
// <--EGB && SGB

	BONDLOOP_START(theBondIndex)
    // --> octree duplicates
    BNDDUP(theBond);
    // <-- octree duplicates
		from = chemData->getBondFrom(theBond);
		to = chemData->getBondTo(theBond);
		if (!doHydrogens) {
			if ((chemData->getAtomicNumber(from) == 1) ||
				(chemData->getAtomicNumber(to) == 1)) continue;
		}
		coords1 = chemData->getAtomCoordinates(from).getValue();
		coords2 = chemData->getAtomCoordinates(to).getValue();

// --> EGB && SGB
		POST_ADJUST_CYLINDER
// <--EGB && SGB

// --> roundcap optimization
// // --> EGB && SGB
// //		unitCylinder->render(coords1, coords2, rad,lastCylinderDoNormals, lastCylinderDoTextures);
//		unitCylinder->render(coords1, coords2, rad,lastCylinderDoNormals, lastCylinderDoTextures,
//		displayStyle,action);
// // <-- EGB && SGB
		SbVec2i fromTo; fromTo.setValue(from,to);
		unitCylinder->render(coords1, coords2, rad,lastCylinderDoNormals, lastCylinderDoTextures,
		displayStyle,fromTo,this,action);
// <-- roundcap optimization
	BONDLOOP_END
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
		glMaterialfv(GL_FRONT, GL_EMISSION,
			SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
//CYL_HIGHLIGHT_DONE:
	glDisable(GL_COLOR_MATERIAL);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Renders the cylinder "display list" for each bond in the molecule.
//	  Invoked by the GLRender method.
//
// Use: private
void
ChemDisplay::
highlightCylinderBB
    (const MFVec2i &theBondIndex, SoState *state, const ChemColor *chemColor,
	const ChemDisplayParam *cdp, const ChemBaseData *chemData)
//
////////////////////////////////////////////////////////////////////////
{
	int32_t		from, to;
	float		rad;
	SbVec3f 	coords1, coords2;
	SbVec3f		theCapScale;

	// Check to see if there's anything to render.
	if (theBondIndex.getNum() == 0) return;

	int displayStyle = cdp->bondCylinderDisplayStyle.getValue();
	SbBool doHydrogens = cdp->showHydrogens.getValue();

	rad = cdp->bondCylinderRadius.getValue();
	theCapScale.setValue(rad, rad, rad);

	SbBool doCaps = FALSE;
// --> EGB && SGB
	//if (displayStyle == ChemDisplayParam::BONDCYLINDER_FLATCAP) 
	if (displayStyle % 3 > 0)
// <-- EGB && SGB
	{
		doCaps = TRUE;
	}

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

// --> EGB && SGB
	DOHLOD(BB)
	SoGLRenderAction *action = (SoGLRenderAction*)state->getAction();
// <-- EGB && SGB

	const SbColor *bondColors = chemColor->bondColor.getValues(0);
	int32_t highlightStyle = cdp->highlightStyle.getValue();

	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
// --> EGB && SGB
//		highlightCylindersOverall(chemData, cdp, theBondIndex,
//			displayStyle, doHydrogens, doCaps, rad, theCapScale);
		highlightCylindersOverall(chemData, cdp, theBondIndex,
			displayStyle, doHydrogens, doCaps, rad, theCapScale, state);
// <-- EGB && SGB
		goto CYL_HIGHLIGHT_DONE;
	}
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
		const SbColor &theColor = cdp->highlightColor.getValue();
		glMaterialfv(GL_FRONT, GL_EMISSION,
			SbVec4f(theColor[0], theColor[1], theColor[2], 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

// --> EGB && SGB
	PRE_ADJUST_CYLINDER
// <--EGB && SGB

	BONDLOOP_START(theBondIndex)
    // --> octree duplicates
    BNDDUP(theBond);
    // <-- octree duplicates
		from = chemData->getBondFrom(theBond);
		to = chemData->getBondTo(theBond);
		if (!doHydrogens) {
			if ((chemData->getAtomicNumber(from) == 1) ||
				(chemData->getAtomicNumber(to) == 1)) continue;
		}
		coords1 = chemData->getAtomCoordinates(from).getValue();
		coords2 = chemData->getAtomCoordinates(to).getValue();
		glColor3fv(bondColors[theBond].getValue());

// --> EGB && SGB
		POST_ADJUST_CYLINDER
// <--EGB && SGB

// --> roundcap optimization
// // --> EGB && SGB
// //		unitCylinder->render(coords1, coords2, rad,lastCylinderDoNormals, lastCylinderDoTextures);
//		unitCylinder->render(coords1, coords2, rad,lastCylinderDoNormals, lastCylinderDoTextures,
//		displayStyle,action);
// // <-- EGB && SGB
		SbVec2i fromTo; fromTo.setValue(from,to);
		unitCylinder->render(coords1, coords2, rad,lastCylinderDoNormals, lastCylinderDoTextures,
		displayStyle,fromTo,this,action);
// <-- roundcap optimization
	BONDLOOP_END
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
		glMaterialfv(GL_FRONT, GL_EMISSION,
			SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
CYL_HIGHLIGHT_DONE:
	glDisable(GL_COLOR_MATERIAL);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Renders the cylinder "display list" for each bond in the molecule.
//	  Invoked by the GLRender method.
//
// Use: private
void
ChemDisplay::
highlightCylinderBI
    (const MFVec2i &theBondIndex, SoState *state, const ChemColor *chemColor,
	const ChemDisplayParam *cdp, const ChemBaseData *chemData)
//
////////////////////////////////////////////////////////////////////////
{
	int32_t		from, to;
	float		rad;
	SbVec3f 	coords1, coords2;
	SbVec3f		theCapScale;

	// Check to see if there's anything to render.
	if (theBondIndex.getNum() == 0) return;

	int displayStyle = cdp->bondCylinderDisplayStyle.getValue();
	SbBool doHydrogens = cdp->showHydrogens.getValue();

	rad = cdp->bondCylinderRadius.getValue();
	theCapScale.setValue(rad, rad, rad);

	SbBool doCaps = FALSE;
// --> EGB && SGB
	//if (displayStyle == ChemDisplayParam::BONDCYLINDER_FLATCAP) 
	if (displayStyle % 3 > 0)
// <-- EGB && SGB
	{
		doCaps = TRUE;
	}

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

// --> EGB && SGB
	DOHLOD(BI)
	SoGLRenderAction *action = (SoGLRenderAction*)state->getAction();
// <-- EGB && SGB

	const SbColor *bondColors = chemColor->bondColor.getValues(0);
	int32_t highlightStyle = cdp->highlightStyle.getValue();

	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
// --> EGB && SGB
//		highlightCylindersOverall(chemData, cdp, theBondIndex,
//			displayStyle, doHydrogens, doCaps, rad, theCapScale);
		highlightCylindersOverall(chemData, cdp, theBondIndex,
			displayStyle, doHydrogens, doCaps, rad, theCapScale, state);
// <-- EGB && SGB
		goto CYL_HIGHLIGHT_DONE;
	}
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
		const SbColor &theColor = cdp->highlightColor.getValue();
		glMaterialfv(GL_FRONT, GL_EMISSION,
			SbVec4f(theColor[0], theColor[1], theColor[2], 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

// --> EGB && SGB
	PRE_ADJUST_CYLINDER
// <--EGB && SGB

	BONDLOOP_START(theBondIndex)
    // --> octree duplicates
    BNDDUP(theBond);
    // <-- octree duplicates
		from = chemData->getBondFrom(theBond);
		to = chemData->getBondTo(theBond);
		if (!doHydrogens) {
			if ((chemData->getAtomicNumber(from) == 1) ||
				(chemData->getAtomicNumber(to) == 1)) continue;
		}
		coords1 = chemData->getAtomCoordinates(from).getValue();
		coords2 = chemData->getAtomCoordinates(to).getValue();
		glColor3fv(bondColors[chemData->getBondIndex(theBond)].getValue());
// --> EGB && SGB
		POST_ADJUST_CYLINDER
// <--EGB && SGB
// --> roundcap optimization
//// --> EGB && SGB
////		unitCylinder->render(coords1, coords2, rad,lastCylinderDoNormals, lastCylinderDoTextures);
//		unitCylinder->render(coords1, coords2, rad,lastCylinderDoNormals, lastCylinderDoTextures,
//		displayStyle,action);
// // <-- EGB && SGB
			SbVec2i fromTo; fromTo.setValue(from,to);
			unitCylinder->render(coords1, coords2, rad,lastCylinderDoNormals, lastCylinderDoTextures,
			displayStyle,fromTo,this,action);
// <-- roundcap optimization
	BONDLOOP_END
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
		glMaterialfv(GL_FRONT, GL_EMISSION,
			SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
CYL_HIGHLIGHT_DONE:
	glDisable(GL_COLOR_MATERIAL);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Renders the cylinder "display list" for each bond in the molecule.
//	  Invoked by the GLRender method.
//
// Use: private
void
ChemDisplay::
highlightCylinderBAAA
    (const MFVec2i &theBondIndex, SoState *state, const ChemColor *chemColor,
	const ChemDisplayParam *cdp, const ChemBaseData *chemData)
//
////////////////////////////////////////////////////////////////////////
{
	int32_t		from, to;
	float		rad;
	SbVec3f 	coords1, coords2;
	SbVec3f		theCapScale;

	// Check to see if there's anything to render.
	if (theBondIndex.getNum() == 0) return;

	int displayStyle = cdp->bondCylinderDisplayStyle.getValue();
	SbBool doHydrogens = cdp->showHydrogens.getValue();

	rad = cdp->bondCylinderRadius.getValue();
	theCapScale.setValue(rad, rad, rad);

	SbBool doCaps = FALSE;
// --> EGB && SGB
	//if (displayStyle == ChemDisplayParam::BONDCYLINDER_FLATCAP) 
	if (displayStyle % 3 > 0)
// <-- EGB && SGB
	{
		doCaps = TRUE;
	}

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

// --> EGB && SGB
	DOHLOD(BAAA)
	SoGLRenderAction *action = (SoGLRenderAction*)state->getAction();
// <-- EGB && SGB

	const SbColor *atomColors = chemColor->atomColor.getValues(0);
	SbColor colorFrom, colorTo;
	int32_t highlightStyle = cdp->highlightStyle.getValue();
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
// --> EGB && SGB
//		highlightCylindersOverall(chemData, cdp, theBondIndex,
//			displayStyle, doHydrogens, doCaps, rad, theCapScale);
		highlightCylindersOverall(chemData, cdp, theBondIndex,
			displayStyle, doHydrogens, doCaps, rad, theCapScale, state);
// <-- EGB && SGB
		goto CYL_HIGHLIGHT_DONE;
	}
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
		const SbColor &theColor = cdp->highlightColor.getValue();
		glMaterialfv(GL_FRONT, GL_EMISSION,
			SbVec4f(theColor[0], theColor[1], theColor[2], 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
// --> EGB && SGB
	PRE_ADJUST_CYLINDER
// <--EGB && SGB
	BONDLOOP_START(theBondIndex)
    // --> octree duplicates
    BNDDUP(theBond);
    // <-- octree duplicates
		from = chemData->getBondFrom(theBond);
		to = chemData->getBondTo(theBond);
		if (!doHydrogens) {
			if ((chemData->getAtomicNumber(from) == 1) ||
				(chemData->getAtomicNumber(to) == 1)) continue;
		}
		colorFrom = atomColors[from];
		colorTo = atomColors[to];
		coords1 = chemData->getAtomCoordinates(from);
		coords2 = chemData->getAtomCoordinates(to);
// --> EGB && SGB
		POST_ADJUST_CYLINDER
// <--EGB && SGB
// --> roundcap optimization
//		unitCylinder->renderWithInterpolatedColor(coords1, coords2, rad,
//			colorFrom, colorTo,
//			lastCylinderDoNormals, lastCylinderDoTextures);
		SbVec2i fromTo; fromTo.setValue(from,to);
		unitCylinder->renderWithInterpolatedColor(coords1, coords2, rad,
			colorFrom, colorTo,
			lastCylinderDoNormals, lastCylinderDoTextures,displayStyle,
			fromTo,this,action);
// <-- roundcap optimization
	BONDLOOP_END
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
		glMaterialfv(GL_FRONT, GL_EMISSION,
			SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
CYL_HIGHLIGHT_DONE:
	glDisable(GL_COLOR_MATERIAL);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Renders the cylinder "display list" for each bond in the molecule.
//	  Invoked by the GLRender method.
//
// Use: private
void
ChemDisplay::
highlightCylinderBAAI
    (const MFVec2i &theBondIndex, SoState *state, const ChemColor *chemColor,
	const ChemDisplayParam *cdp, const ChemBaseData *chemData)
//
////////////////////////////////////////////////////////////////////////
{
	int32_t		from, to;
	float		rad;
	SbVec3f 	coords1, coords2;
	SbVec3f		theCapScale;

	// Check to see if there's anything to render.
	if (theBondIndex.getNum() == 0) return;

	int displayStyle = cdp->bondCylinderDisplayStyle.getValue();
	SbBool doHydrogens = cdp->showHydrogens.getValue();

	rad = cdp->bondCylinderRadius.getValue();
	theCapScale.setValue(rad, rad, rad);

	SbBool doCaps = FALSE;
// --> EGB && SGB
	//if (displayStyle == ChemDisplayParam::BONDCYLINDER_FLATCAP) 
	if (displayStyle % 3 > 0)
// <-- EGB && SGB
	{
		doCaps = TRUE;
	}

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

// --> EGB && SGB
	DOHLOD(BAAI)
	SoGLRenderAction *action = (SoGLRenderAction*)state->getAction();
// <-- EGB && SGB

	const SbColor *atomColors = chemColor->atomColor.getValues(0);
	SbColor colorFrom, colorTo;
	int32_t highlightStyle = cdp->highlightStyle.getValue();
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
// --> EGB && SGB
//		highlightCylindersOverall(chemData, cdp, theBondIndex,
//			displayStyle, doHydrogens, doCaps, rad, theCapScale);
		highlightCylindersOverall(chemData, cdp, theBondIndex,
			displayStyle, doHydrogens, doCaps, rad, theCapScale, state);
// <-- EGB && SGB
		goto CYL_HIGHLIGHT_DONE;
	}
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
		const SbColor &theColor = cdp->highlightColor.getValue();
		glMaterialfv(GL_FRONT, GL_EMISSION,
			SbVec4f(theColor[0], theColor[1], theColor[2], 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
// --> EGB && SGB
	PRE_ADJUST_CYLINDER
// <--EGB && SGB
	BONDLOOP_START(theBondIndex)
    // --> octree duplicates
    BNDDUP(theBond);
    // <-- octree duplicates
		from = chemData->getBondFrom(theBond);
		to = chemData->getBondTo(theBond);
		if (!doHydrogens) {
			if ((chemData->getAtomicNumber(from) == 1) ||
				(chemData->getAtomicNumber(to) == 1)) continue;
		}
		colorFrom = atomColors[chemData->getAtomIndex(from)];
		colorTo = atomColors[chemData->getAtomIndex(to)];
		coords1 = chemData->getAtomCoordinates(from);
		coords2 = chemData->getAtomCoordinates(to);
// --> EGB && SGB
		POST_ADJUST_CYLINDER
// <--EGB && SGB
// --> roundcap optimization
//		unitCylinder->renderWithInterpolatedColor(coords1, coords2, rad,
//			colorFrom, colorTo,
//			lastCylinderDoNormals, lastCylinderDoTextures);
		SbVec2i fromTo; fromTo.setValue(from,to);
		unitCylinder->renderWithInterpolatedColor(coords1, coords2, rad,
			colorFrom, colorTo,
			lastCylinderDoNormals, lastCylinderDoTextures, displayStyle,
			fromTo,this,action);
// <-- roundcap optimization
	BONDLOOP_END
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
		glMaterialfv(GL_FRONT, GL_EMISSION,
			SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
CYL_HIGHLIGHT_DONE:
	glDisable(GL_COLOR_MATERIAL);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Renders the cylinder "display list" for each bond in the molecule.
//	  Invoked by the GLRender method.
//
// Use: private
void
ChemDisplay::
highlightCylinderBHAA
    (const MFVec2i &theBondIndex, SoState *state, const ChemColor *chemColor,
	const ChemDisplayParam *cdp, const ChemBaseData *chemData)
//
////////////////////////////////////////////////////////////////////////
{
	int32_t		from, to;
	float		rad;
	SbVec3f 	coords1, coords2;
	SbVec3f		theCapScale;

	// Check to see if there's anything to render.
	if (theBondIndex.getNum() == 0) return;

	int displayStyle = cdp->bondCylinderDisplayStyle.getValue();
	SbBool doHydrogens = cdp->showHydrogens.getValue();

	rad = cdp->bondCylinderRadius.getValue();
	theCapScale.setValue(rad, rad, rad);

	SbBool doCaps = FALSE;
// --> EGB && SGB
	//if (displayStyle == ChemDisplayParam::BONDCYLINDER_FLATCAP) 
	if (displayStyle % 3 > 0)
// <-- EGB && SGB
	{
		doCaps = TRUE;
	}

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

// --> EGB && SGB
	DOHLOD(BHAA)
	SoGLRenderAction *action = (SoGLRenderAction*)state->getAction();
// <-- EGB && SGB

	const SbColor *atomColors = chemColor->atomColor.getValues(0);
	SbColor colorFrom, colorTo;
	int32_t highlightStyle = cdp->highlightStyle.getValue();
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
// --> EGB && SGB
//		highlightCylindersOverall(chemData, cdp, theBondIndex,
//			displayStyle, doHydrogens, doCaps, rad, theCapScale);
		highlightCylindersOverall(chemData, cdp, theBondIndex,
			displayStyle, doHydrogens, doCaps, rad, theCapScale, state);
// <-- EGB && SGB		
		goto CYL_HIGHLIGHT_DONE;
	}
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
		const SbColor &theColor = cdp->highlightColor.getValue();
		glMaterialfv(GL_FRONT, GL_EMISSION,
			SbVec4f(theColor[0], theColor[1], theColor[2], 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
// --> EGB && SGB
	PRE_ADJUST_CYLINDER
// <--EGB && SGB

	BONDLOOP_START(theBondIndex)
    // --> octree duplicates
    BNDDUP(theBond);
    // <-- octree duplicates
		from = chemData->getBondFrom(theBond);
		to = chemData->getBondTo(theBond);
		if (!doHydrogens) {
			if ((chemData->getAtomicNumber(from) == 1) ||
				(chemData->getAtomicNumber(to) == 1)) continue;
		}
		colorFrom = atomColors[from];
		colorTo = atomColors[to];
		coords1 = chemData->getAtomCoordinates(from).getValue();
		coords2 = chemData->getAtomCoordinates(to).getValue();
// --> EGB && SGB
		POST_ADJUST_CYLINDER
// <--EGB && SGB
// --> roundcap optimization
    SbVec2i fromTo; fromTo[0] = from; fromTo[1] = to;
// <-- roundcap optimization
		unitCylinder->renderHalfBonded(coords1, coords2, rad,
			colorFrom, colorTo,
			lastCylinderDoNormals, lastCylinderDoTextures,displayStyle, fromTo, this, action);
	BONDLOOP_END
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
		glMaterialfv(GL_FRONT, GL_EMISSION,
			SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
CYL_HIGHLIGHT_DONE:
	glDisable(GL_COLOR_MATERIAL);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Renders the cylinder "display list" for each bond in the molecule.
//	  Invoked by the GLRender method.
//
// Use: private
void
ChemDisplay::
highlightCylinderBHAI
    (const MFVec2i &theBondIndex, SoState *state, const ChemColor *chemColor,
	const ChemDisplayParam *cdp, const ChemBaseData *chemData)
//
////////////////////////////////////////////////////////////////////////
{
	int32_t		from, to;
	float		rad;
	SbVec3f 	coords1, coords2;
	SbVec3f		theCapScale;

	// Check to see if there's anything to render.
	if (theBondIndex.getNum() == 0) return;

	int displayStyle = cdp->bondCylinderDisplayStyle.getValue();
	SbBool doHydrogens = cdp->showHydrogens.getValue();

	rad = cdp->bondCylinderRadius.getValue();
	theCapScale.setValue(rad, rad, rad);

	SbBool doCaps = FALSE;
// --> EGB && SGB
	//if (displayStyle == ChemDisplayParam::BONDCYLINDER_FLATCAP) 
	if (displayStyle % 3 > 0)
// <-- EGB && SGB
	{
		doCaps = TRUE;
	}

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

// --> EGB && SGB
	DOHLOD(BHAI)
	SoGLRenderAction *action = (SoGLRenderAction*)state->getAction();
// <-- EGB && SGB

	const SbColor *atomColors = chemColor->atomColor.getValues(0);
	SbColor colorFrom, colorTo;
	int32_t highlightStyle = cdp->highlightStyle.getValue();
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
// --> EGB && SGB
//		highlightCylindersOverall(chemData, cdp, theBondIndex,
//			displayStyle, doHydrogens, doCaps, rad, theCapScale);
		highlightCylindersOverall(chemData, cdp, theBondIndex,
			displayStyle, doHydrogens, doCaps, rad, theCapScale, state);
// <-- EGB && SGB		
		goto CYL_HIGHLIGHT_DONE;
	}
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
		const SbColor &theColor = cdp->highlightColor.getValue();
		glMaterialfv(GL_FRONT, GL_EMISSION,
			SbVec4f(theColor[0], theColor[1], theColor[2], 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
// --> EGB && SGB
	PRE_ADJUST_CYLINDER
// <--EGB && SGB

	BONDLOOP_START(theBondIndex)
    // --> octree duplicates
    BNDDUP(theBond);
    // <-- octree duplicates
		from = chemData->getBondFrom(theBond);
		to = chemData->getBondTo(theBond);
		if (!doHydrogens) {
			if ((chemData->getAtomicNumber(from) == 1) ||
				(chemData->getAtomicNumber(to) == 1)) continue;
		}
		colorFrom = atomColors[chemData->getAtomIndex(from)];
		colorTo = atomColors[chemData->getAtomIndex(to)];
		coords1 = chemData->getAtomCoordinates(from).getValue();
		coords2 = chemData->getAtomCoordinates(to).getValue();
// --> EGB && SGB
		POST_ADJUST_CYLINDER
// <--EGB && SGB
// --> roundcap optimization
    SbVec2i fromTo; fromTo[0] = from; fromTo[1] = to;
// <-- roundcap optimization
		unitCylinder->renderHalfBonded(coords1, coords2, rad,
			colorFrom, colorTo,
			lastCylinderDoNormals, lastCylinderDoTextures,displayStyle, fromTo, this, action);
	BONDLOOP_END
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
		glMaterialfv(GL_FRONT, GL_EMISSION,
			SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
CYL_HIGHLIGHT_DONE:
	glDisable(GL_COLOR_MATERIAL);
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Performs highlighting of the cylinders representing bonds in the
//	  molecule.  This is called if the highlighting is setting the diffuse
//    color of all of the bonds to one color.
//
// Use: private

// --> EGB && SGB
// void
//ChemDisplay::highlightCylindersOverall(const ChemBaseData *chemData,
//	const ChemDisplayParam *cdp, const MFVec2i &theBondIndex,
//	int displayStyle, SbBool doHydrogens, SbBool doCaps,
//	float rad, SbVec3f theCapScale)
void
ChemDisplay::highlightCylindersOverall(const ChemBaseData *chemData,
	const ChemDisplayParam *cdp, const MFVec2i &theBondIndex,
	int displayStyle, SbBool doHydrogens, SbBool doCaps,
	float rad, SbVec3f theCapScale, SoState* state)
// <-- EGB && SGB
//
////////////////////////////////////////////////////////////////////////
{
	int32_t		from, to;
	SbVec3f 	coords1, coords2;

	const SbColor &theColor = cdp->highlightColor.getValue();
	glColor3fv(theColor.getValue());
// --> EGB && SGB
	PRE_ADJUST_CYLINDER
// <--EGB && SGB
// --> EGB && SGB
	SoGLRenderAction *action = (SoGLRenderAction *)state->getAction();
// <-- EGB && SGB
	BONDLOOP_START(theBondIndex)
    // --> octree duplicates
    BNDDUP(theBond);
    // <-- octree duplicates
		from = chemData->getBondFrom(theBond);
		to = chemData->getBondTo(theBond);
		if (!doHydrogens) {
			if ((chemData->getAtomicNumber(from) == 1) ||
				(chemData->getAtomicNumber(to) == 1)) continue;
		}
		coords1 = chemData->getAtomCoordinates(from).getValue();
		coords2 = chemData->getAtomCoordinates(to).getValue();

// --> EGB && SGB
		POST_ADJUST_CYLINDER
// <--EGB && SGB

// --> roundcap optimization
// // --> EGB && SGB
////		unitCylinder->render(coords1, coords2, rad,lastCylinderDoNormals, lastCylinderDoTextures);
//		unitCylinder->render(coords1, coords2, rad,lastCylinderDoNormals, lastCylinderDoTextures,
//		displayStyle,action);
// // <-- EGB && SGB
		SbVec2i fromTo; fromTo.setValue(from,to);
		unitCylinder->render(coords1, coords2, rad,lastCylinderDoNormals, lastCylinderDoTextures,
		displayStyle,fromTo,this,action);
// <-- roundcap optimization
	BONDLOOP_END
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Implements rayPicking.
//
// Use: protected

void
ChemDisplay::pickBondsAsCylinders(SoRayPickAction *action)
//
////////////////////////////////////////////////////////////////////////
{
	SbLine			pickLine;
	float			rad;
	float			halfHeight;
	float			ratioFromV1;
	SbVec3f			enterPoint, exitPoint, normal;
	SbVec4f			texCoord;
	SoPickedPoint	*pp;
	ChemDetail		*detail;
	int				numHits;
	SbRotation		theRotation;
	SbVec3f			theTranslation;
	SbLine			cylinderAxis;
	SbVec3f			yAxis;
	SbVec3f			src;
	SbVec3f			tEnterPoint;
	SbVec3f			tExitPoint;
	SbVec3f			coords1;
	SbVec3f			coords2;
	SbVec3f			tcoord;
	int32_t			from, to;

	SoState *state = action->getState();
	ChemBaseData *chemData = ChemBaseDataElement::get(state);
	ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);
	ChemColor *chemColor = ChemColorElement::get(state);

	int displayStyle = cdp->bondCylinderDisplayStyle.getValue();
	SbBool doHydrogens = cdp->showHydrogens.getValue();

	rad = cdp->bondCylinderRadius.getValue();

	SbBool doCaps = FALSE;
// --> EGB && SGB
	//if (displayStyle == ChemDisplayParam::BONDCYLINDER_ROUNDCAP) 
	if (displayStyle % 3 > 0)
// <-- EGB && SGB
	{
		doCaps = TRUE;
	}

	yAxis.setValue(0.0, 1.0, 0.0);

	// Compute the picking ray in our current object space
	computeObjectSpaceRay(action);
	pickLine = action->getLine();

	BONDLOOP_START(bondIndex)
		numHits = 0;
		// Get size of this cylinder
		from = chemData->getBondFrom(theBond);
		to = chemData->getBondTo(theBond);
		if (!doHydrogens) {
			if ((chemData->getAtomicNumber(from) == 1) ||
				(chemData->getAtomicNumber(to) == 1)) continue;
		} 
		coords1 = chemData->getAtomCoordinates(from).getValue();
		coords2 = chemData->getAtomCoordinates(to).getValue();
		tcoord = coords2 - coords1;
		halfHeight = tcoord.length() * 0.5f;
		cylinderAxis.setValue(coords1, coords2);


		// Construct an infinite cylinder to test sides for intersection
		SbCylinder		infiniteCyl;
		infiniteCyl.setValue(cylinderAxis, rad);

		// See if the line intersects the cylinder
		if (infiniteCyl.intersect(pickLine, enterPoint, exitPoint)) {

			// See if the enter point is within the real cylinder and is
			// between the near and far clipping planes.

			// Find the translation and rotation necessary to align
			// the cylinder along the yAxis with its midpoint at 0,0,0 and
			// apply these to the enterPoint.

			theRotation.setValue(tcoord, yAxis);
			theTranslation = tcoord * 0.5f + coords1;
			src = enterPoint - theTranslation;
			theRotation.multVec(src, tEnterPoint);

			if (tEnterPoint[1] <= halfHeight && tEnterPoint[1] >= -halfHeight) {
				numHits++;
				if (action->isBetweenPlanes(enterPoint) &&
					(pp = action->addIntersection(enterPoint)) != NULL) {

					normal = cylinderAxis.getClosestPoint(enterPoint);
					normal = enterPoint - normal;
					normal.normalize();
					pp->setObjectNormal(normal);

					texCoord.setValue(atan2f(tEnterPoint[0], tEnterPoint[2])
						* (1.0f / (float)(2.0 * M_PI)) + 0.5f,
						(tEnterPoint[1] + halfHeight) /
						(2.0f * halfHeight),
						0.0f, 1.0f);
					pp->setObjectTextureCoords(texCoord);

					ratioFromV1 = ((enterPoint - coords1).length() /
						(coords2 - coords1).length());
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
								pp->setMaterialIndex(
									chemData->getAtomIndex(from));
							else
								pp->setMaterialIndex(
									chemData->getAtomIndex(to));
							break;
						  default:
							break;
						}
					  default:
						break;
					}

					detail = new ChemDetail();
					detail->setAtomBondIndex(-1, theBond);
					pp->setDetail(detail, this);
				}
			}

			// Do same for exit point
			src = exitPoint - theTranslation;
			theRotation.multVec(src, tExitPoint);
			if (tExitPoint[1] <= halfHeight && tExitPoint[1] >= -halfHeight) {
				numHits++;
				if (action->isBetweenPlanes(exitPoint) &&
					(pp = action->addIntersection(exitPoint)) != NULL) {

					normal = cylinderAxis.getClosestPoint(exitPoint);
					normal = exitPoint - normal;
					normal.normalize();
					pp->setObjectNormal(normal);

					ratioFromV1 = ((exitPoint - coords1).length() /
						(coords2 - coords1).length());
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
								pp->setMaterialIndex(
									chemData->getAtomIndex(from));
							else
								pp->setMaterialIndex(
									chemData->getAtomIndex(to));
							break;
						  default:
							break;
						}
					  default:
						break;
					}

					texCoord.setValue(atan2f(tExitPoint[0], tExitPoint[2])
						* (1.0f / (float)(2.0 * M_PI)) + 0.5f,
						(tExitPoint[1] + halfHeight) /
						(2.0f * halfHeight),
						0.0f, 1.0f);
					pp->setObjectTextureCoords(texCoord);

					detail = new ChemDetail();
					detail->setAtomBondIndex(-1, theBond);
					pp->setDetail(detail, this);
				}
			}
		}

		// If we haven't hit the cylinder twice already, check for an
		// intersection with the top face
		if (numHits < 2) {
			normal = coords2 - coords1;
			normal.normalize();

			// Construct a plane containing the top face
			SbPlane	topFacePlane(normal, coords2);

			// See if the ray hits this plane
			if (topFacePlane.intersect(pickLine, enterPoint)) {

				// See if the intersection is within the correct radius
				// and is within the clipping planes

				src = cylinderAxis.getClosestPoint(enterPoint);
				src -= enterPoint;
				if (src.length() <= rad) {
					numHits++;
					if (action->isBetweenPlanes(enterPoint) &&
						(pp = action->addIntersection(enterPoint)) != NULL) {

						pp->setObjectNormal(normal);

						src = enterPoint - theTranslation;
						theRotation.multVec(src, tEnterPoint);
						texCoord.setValue(0.5f + tEnterPoint[0] / (float)(2.0 * rad),
							0.5f - tEnterPoint[2] / (float)(2.0 * rad),
							0.0f, 1.0f);
						pp->setObjectTextureCoords(texCoord);

						switch (chemColor->bondColorBinding.getValue()) {
						  case ChemColor::BOND_OVERALL:
							pp->setMaterialIndex(0);
							break;
						  case ChemColor::BOND_PER_BOND:
							pp->setMaterialIndex(theBond);
							break;
						  case ChemColor::BOND_PER_BOND_INDEXED:
							pp->setMaterialIndex(
								chemData->getBondIndex(theBond));
							break;
						  case ChemColor::BOND_PER_ATOM:
						  case ChemColor::BOND_PER_ATOM_HALF_BONDED:
							switch (chemColor->atomColorBinding.getValue()) {
							  case ChemColor::ATOM_OVERALL:
								pp->setMaterialIndex(0);
								break;
							  case ChemColor::ATOM_PER_ATOM:
								pp->setMaterialIndex(to);
								break;
							  case ChemColor::ATOM_PER_ATOM_INDEXED:
								pp->setMaterialIndex(
									chemData->getAtomIndex(to));
								break;
							  default:
								break;
							}
						  default:
							break;
						}

						detail = new ChemDetail();
						detail->setAtomBondIndex(-1, theBond);
						pp->setDetail(detail, this);
					}
				}
			}
		}

		// If we haven't hit the cylinder twice already, check for an
		// intersection with the bottom face
		if (numHits < 2) {
			normal = coords1 - coords2;
			normal.normalize();

			// Construct a plane containing the bottom face
			SbPlane		bottomFacePlane(normal, coords1);

			// See if the ray hits this plane
			if (bottomFacePlane.intersect(pickLine, enterPoint)) {

				// See if the intersection is within the correct radius
				// and is within the clipping planes

				src = cylinderAxis.getClosestPoint(enterPoint);
				src -= enterPoint;
				if (src.length() <= rad) {
					numHits++;
					if (action->isBetweenPlanes(enterPoint) &&
						(pp = action->addIntersection(enterPoint)) != NULL) {

						pp->setObjectNormal(normal);

						src = enterPoint - theTranslation;
						theRotation.multVec(src, tEnterPoint);
						texCoord.setValue(0.5f + tEnterPoint[0] / (float)(2.0 * rad),
							0.5f + tEnterPoint[2] / (float)(2.0 * rad),
							0.0f, 1.0f);
						pp->setObjectTextureCoords(texCoord);

						switch (chemColor->bondColorBinding.getValue()) {
						  case ChemColor::BOND_OVERALL:
							pp->setMaterialIndex(0);
							break;
						  case ChemColor::BOND_PER_BOND:
							pp->setMaterialIndex(theBond);
							break;
						  case ChemColor::BOND_PER_BOND_INDEXED:
							pp->setMaterialIndex(
								chemData->getBondIndex(theBond));
							break;
						  case ChemColor::BOND_PER_ATOM:
						  case ChemColor::BOND_PER_ATOM_HALF_BONDED:
							switch (chemColor->atomColorBinding.getValue()) {
							  case ChemColor::ATOM_OVERALL:
								pp->setMaterialIndex(0);
								break;
							  case ChemColor::ATOM_PER_ATOM:
								pp->setMaterialIndex(from);
								break;
							  case ChemColor::ATOM_PER_ATOM_INDEXED:
								pp->setMaterialIndex(
									chemData->getAtomIndex(from));
								break;
							  default:
								break;
							}
						  default:
							break;
						}

						detail = new ChemDetail();
						detail->setAtomBondIndex(-1, theBond);
						pp->setDetail(detail, this);
					}
				}
			}
		}

		if ((numHits == 0) && doCaps) {
			SbSphere	sph;
			sph.setValue(coords2, rad);
			if (sph.intersect(pickLine, enterPoint, exitPoint)) {
				if (action->isBetweenPlanes(enterPoint) &&
					(pp = action->addIntersection(enterPoint)) != NULL) {

					normal = enterPoint - coords2;

					texCoord.setValue(0.5 + normal[0] / (2.0 * rad),
						0.5 - normal[2] / (2.0 * rad),
						0.0, 1.0);
					pp->setObjectTextureCoords(texCoord);

					normal.normalize();
					pp->setObjectNormal(normal);

					switch (chemColor->bondColorBinding.getValue()) {
					  case ChemColor::BOND_OVERALL:
						pp->setMaterialIndex(0);
						break;
					  case ChemColor::BOND_PER_BOND:
						pp->setMaterialIndex(theBond);
						break;
					  case ChemColor::BOND_PER_BOND_INDEXED:
						pp->setMaterialIndex(
							chemData->getBondIndex(theBond));
						break;
					  case ChemColor::BOND_PER_ATOM:
					  case ChemColor::BOND_PER_ATOM_HALF_BONDED:
						switch (chemColor->atomColorBinding.getValue()) {
						  case ChemColor::ATOM_OVERALL:
							pp->setMaterialIndex(0);
							break;
						  case ChemColor::ATOM_PER_ATOM:
							pp->setMaterialIndex(to);
							break;
						  case ChemColor::ATOM_PER_ATOM_INDEXED:
							pp->setMaterialIndex(
								chemData->getAtomIndex(to));
							break;
						  default:
							break;
						}
					  default:
						break;
					}

					detail = new ChemDetail;
					detail->setAtomBondIndex(-1, theBond);
					pp->setDetail(detail, this);
				}
				if (action->isBetweenPlanes(exitPoint) &&
					(pp = action->addIntersection(exitPoint)) != NULL) {

					normal = exitPoint - coords2;

					texCoord.setValue(0.5f + normal[0] / (float)(2.0 * rad),
						0.5f - normal[2] / (float)(2.0 * rad),
						0.0f, 1.0f);
					pp->setObjectTextureCoords(texCoord);

					normal.normalize();
					pp->setObjectNormal(normal);

					switch (chemColor->bondColorBinding.getValue()) {
					  case ChemColor::BOND_OVERALL:
						pp->setMaterialIndex(0);
						break;
					  case ChemColor::BOND_PER_BOND:
						pp->setMaterialIndex(theBond);
						break;
					  case ChemColor::BOND_PER_BOND_INDEXED:
						pp->setMaterialIndex(
							chemData->getBondIndex(theBond));
						break;
					  case ChemColor::BOND_PER_ATOM:
					  case ChemColor::BOND_PER_ATOM_HALF_BONDED:
						switch (chemColor->atomColorBinding.getValue()) {
						  case ChemColor::ATOM_OVERALL:
							pp->setMaterialIndex(0);
							break;
						  case ChemColor::ATOM_PER_ATOM:
							pp->setMaterialIndex(to);
							break;
						  case ChemColor::ATOM_PER_ATOM_INDEXED:
							pp->setMaterialIndex(
								chemData->getAtomIndex(to));
							break;
						  default:
							break;
						}
					  default:
						break;
					}

					detail = new ChemDetail;
					detail->setAtomBondIndex(-1, theBond);
					pp->setDetail(detail, this);
				}
			}
			else {
				sph.setValue(coords1, rad);
				if (sph.intersect(pickLine, enterPoint, exitPoint)) {
					if (action->isBetweenPlanes(enterPoint) &&
						(pp = action->addIntersection(enterPoint)) != NULL) {

						normal = enterPoint - coords1;

						texCoord.setValue(0.5f + normal[0] / (float)(2.0 * rad),
							0.5f + normal[2] / (float)(2.0 * rad),
							0.0f, 1.0f);
						pp->setObjectTextureCoords(texCoord);

						normal.normalize();
						pp->setObjectNormal(normal);

						switch (chemColor->bondColorBinding.getValue()) {
						  case ChemColor::BOND_OVERALL:
							pp->setMaterialIndex(0);
							break;
						  case ChemColor::BOND_PER_BOND:
							pp->setMaterialIndex(theBond);
							break;
						  case ChemColor::BOND_PER_BOND_INDEXED:
							pp->setMaterialIndex(
								chemData->getBondIndex(theBond));
							break;
						  case ChemColor::BOND_PER_ATOM:
						  case ChemColor::BOND_PER_ATOM_HALF_BONDED:
							switch (chemColor->atomColorBinding.getValue()) {
							  case ChemColor::ATOM_OVERALL:
								pp->setMaterialIndex(0);
								break;
							  case ChemColor::ATOM_PER_ATOM:
								pp->setMaterialIndex(from);
								break;
							  case ChemColor::ATOM_PER_ATOM_INDEXED:
								pp->setMaterialIndex(
									chemData->getAtomIndex(from));
								break;
							  default:
								break;
							}
						  default:
							break;
						}

						detail = new ChemDetail;
						detail->setAtomBondIndex(-1, theBond);
						pp->setDetail(detail, this);
					}
					if (action->isBetweenPlanes(exitPoint) &&
						(pp = action->addIntersection(exitPoint)) != NULL) {

						normal = exitPoint - coords1;

						texCoord.setValue(0.5f + normal[0] / (float)(2.0 * rad),
							0.5f + normal[2] / (float)(2.0 * rad),
							0.0f, 1.0f);
						pp->setObjectTextureCoords(texCoord);

						normal.normalize();
						pp->setObjectNormal(normal);

						switch (chemColor->bondColorBinding.getValue()) {
						  case ChemColor::BOND_OVERALL:
							pp->setMaterialIndex(0);
							break;
						  case ChemColor::BOND_PER_BOND:
							pp->setMaterialIndex(theBond);
							break;
						  case ChemColor::BOND_PER_BOND_INDEXED:
							pp->setMaterialIndex(
								chemData->getBondIndex(theBond));
							break;
						  case ChemColor::BOND_PER_ATOM:
						  case ChemColor::BOND_PER_ATOM_HALF_BONDED:
							switch (chemColor->atomColorBinding.getValue()) {
							  case ChemColor::ATOM_OVERALL:
								pp->setMaterialIndex(0);
								break;
							  case ChemColor::ATOM_PER_ATOM:
								pp->setMaterialIndex(from);
								break;
							  case ChemColor::ATOM_PER_ATOM_INDEXED:
								pp->setMaterialIndex(
									chemData->getAtomIndex(from));
								break;
							  default:
								break;
							}
						  default:
							break;
						}

						detail = new ChemDetail;
						detail->setAtomBondIndex(-1, theBond);
						pp->setDetail(detail, this);
					}
				}
			}
		}
	BONDLOOP_END
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Generates triangles representing a cylinder.
//    Invoked by the generatePrimitives method.
//
// Use: protected

void
ChemDisplay::generateBondsAsCylinders(SoAction *action)
//
////////////////////////////////////////////////////////////////////////
{
	SoState *state = action->getState();
	ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);
	ChemColor *chemColor = ChemColorElement::get(state);

	int displayStyle = cdp->bondCylinderDisplayStyle.getValue();

	// See if we need to generate a new unit cylinder
	float bcc = cdp->bondCylinderComplexity.getValue();
	if ((lastCylinderComplexity != bcc)	|| 
		(lastCylinderDisplayStyle != displayStyle)) {

		lastCylinderComplexity = bcc;
		lastCylinderDisplayStyle = displayStyle;

		if (unitCylinder != NULL) delete unitCylinder;
		unitCylinder = new ChemUnitCylinder(displayStyle, bcc);
	}

	// Generate the cylinders
	switch (chemColor->bondColorBinding.getValue()) {
	  case ChemColor::BOND_OVERALL:
		generateCylinderOV(action);
		break;
	  case ChemColor::BOND_PER_BOND:
		generateCylinderBB(action);
		break;
	  case ChemColor::BOND_PER_BOND_INDEXED:
		generateCylinderBI(action);
		break;
	  case ChemColor::BOND_PER_ATOM:
		switch (chemColor->atomColorBinding.getValue()) {
		  case ChemColor::ATOM_OVERALL:
			generateCylinderOV(action);
			break;
		  case ChemColor::ATOM_PER_ATOM:
			generateCylinderBAAA(action);
			break;
		  case ChemColor::ATOM_PER_ATOM_INDEXED:
			generateCylinderBAAI(action);
			break;
		  default:
			break;
		}
	  case ChemColor::BOND_PER_ATOM_HALF_BONDED:
		switch (chemColor->atomColorBinding.getValue()) {
		  case ChemColor::ATOM_OVERALL:
			generateCylinderOV(action);
			break;
		  case ChemColor::ATOM_PER_ATOM:
			generateCylinderBHAA(action);
			break;
		  case ChemColor::ATOM_PER_ATOM_INDEXED:
			generateCylinderBHAI(action);
			break;
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
//    Generates triangles representing a cylinder.
//    Invoked by the generatePrimitives method.
//  
// Use: private

void
ChemDisplay::
generateCylinderOV
    (SoAction* action)
//
////////////////////////////////////////////////////////////////////////
{
	int32_t		from, to;
	float		rad;
	SbVec3f 	coords1, coords2;
	SbBool		genTexCoords;
	SoPrimitiveVertex					pv;
	const SoTextureCoordinateElement	*tce;

	SoState *state = action->getState();
	ChemBaseData *chemData = ChemBaseDataElement::get(state);
	ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);
	//ChemColor *chemColor = ChemColorElement::get(state);
	ChemDetail *detail;

	//int displayStyle = cdp->bondCylinderDisplayStyle.getValue();
	SbBool doHydrogens = cdp->showHydrogens.getValue();

	// Determine whether we should generate our own texture coordinates
	genTexCoords = TRUE;
	switch (SoTextureCoordinateElement::getType(state)) {
		case SoTextureCoordinateElement::EXPLICIT:
			genTexCoords = TRUE;
			break;
		case SoTextureCoordinateElement::FUNCTION:
			genTexCoords = FALSE;
			break;
	}

	// If we're not generating our own coordinates, we'll need the
	// texture coordinate element to get coords based on points/normals.
	if (! genTexCoords) {
		tce = SoTextureCoordinateElement::getInstance(state);
	}

	rad = cdp->bondCylinderRadius.getValue();
	pv.setMaterialIndex(0);
	BONDLOOP_START(bondIndex)
		from = chemData->getBondFrom(theBond);
		to = chemData->getBondTo(theBond);

		if (!doHydrogens) {
			if ((chemData->getAtomicNumber(from) == 1) || 
				(chemData->getAtomicNumber(to) == 1)) continue;
		}
		detail = new ChemDetail;
		detail->setAtomBondIndex(-1, theBond);
		pv.setDetail(detail);

		coords1 = chemData->getAtomCoordinates(from).getValue();
		coords2 = chemData->getAtomCoordinates(to).getValue();

		spewCylinder(pv, action, tce, coords1, coords2, rad, genTexCoords,
			unitCylinder);
	BONDLOOP_END
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Generates triangles representing a cylinder.
//    Invoked by the generatePrimitives method.
//  
// Use: private

void
ChemDisplay::
generateCylinderBB
    (SoAction* action)
//
////////////////////////////////////////////////////////////////////////
{
	int32_t		from, to;
	float		rad;
	SbVec3f 	coords1, coords2;
	SbBool		genTexCoords;
	SoPrimitiveVertex					pv;
	const SoTextureCoordinateElement	*tce;

	SoState *state = action->getState();
	ChemBaseData *chemData = ChemBaseDataElement::get(state);
	ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);
	//ChemColor *chemColor = ChemColorElement::get(state);
	ChemDetail *detail;

	//int displayStyle = cdp->bondCylinderDisplayStyle.getValue();
	SbBool doHydrogens = cdp->showHydrogens.getValue();

	// Determine whether we should generate our own texture coordinates
	genTexCoords = TRUE;
	switch (SoTextureCoordinateElement::getType(state)) {
		case SoTextureCoordinateElement::EXPLICIT:
			genTexCoords = TRUE;
			break;
		case SoTextureCoordinateElement::FUNCTION:
			genTexCoords = FALSE;
			break;
	}

	// If we're not generating our own coordinates, we'll need the
	// texture coordinate element to get coords based on points/normals.
	if (! genTexCoords) {
		tce = SoTextureCoordinateElement::getInstance(state);
	}

	rad = cdp->bondCylinderRadius.getValue();
	BONDLOOP_START(bondIndex)
		pv.setMaterialIndex(theBond);
		from = chemData->getBondFrom(theBond);
		to = chemData->getBondTo(theBond);

		if (!doHydrogens) {
			if ((chemData->getAtomicNumber(from) == 1) || 
				(chemData->getAtomicNumber(to) == 1)) continue;
		}
		detail = new ChemDetail;
		detail->setAtomBondIndex(-1, theBond);
		pv.setDetail(detail);

		coords1 = chemData->getAtomCoordinates(from).getValue();
		coords2 = chemData->getAtomCoordinates(to).getValue();

		spewCylinder(pv, action, tce, coords1, coords2, rad, genTexCoords,
			unitCylinder);
	BONDLOOP_END
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Generates triangles representing a cylinder.
//    Invoked by the generatePrimitives method.
//  
// Use: private

void
ChemDisplay::
generateCylinderBI
    (SoAction* action)
//
////////////////////////////////////////////////////////////////////////
{
	int32_t		from, to;
	float		rad;
	SbVec3f 	coords1, coords2;
	SbBool		genTexCoords;
	SoPrimitiveVertex					pv;
	const SoTextureCoordinateElement	*tce;

	SoState *state = action->getState();
	ChemBaseData *chemData = ChemBaseDataElement::get(state);
	ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);
	//ChemColor *chemColor = ChemColorElement::get(state);
	ChemDetail *detail;

	//int displayStyle = cdp->bondCylinderDisplayStyle.getValue();
	SbBool doHydrogens = cdp->showHydrogens.getValue();

	// Determine whether we should generate our own texture coordinates
	genTexCoords = TRUE;
	switch (SoTextureCoordinateElement::getType(state)) {
		case SoTextureCoordinateElement::EXPLICIT:
			genTexCoords = TRUE;
			break;
		case SoTextureCoordinateElement::FUNCTION:
			genTexCoords = FALSE;
			break;
	}

	// If we're not generating our own coordinates, we'll need the
	// texture coordinate element to get coords based on points/normals.
	if (! genTexCoords) {
		tce = SoTextureCoordinateElement::getInstance(state);
	}

	rad = cdp->bondCylinderRadius.getValue();
	BONDLOOP_START(bondIndex)
		pv.setMaterialIndex(chemData->getBondIndex(theBond));
		from = chemData->getBondFrom(theBond);
		to = chemData->getBondTo(theBond);

		if (!doHydrogens) {
			if ((chemData->getAtomicNumber(from) == 1) || 
				(chemData->getAtomicNumber(to) == 1)) continue;
		}
		detail = new ChemDetail;
		detail->setAtomBondIndex(-1, theBond);
		pv.setDetail(detail);

		coords1 = chemData->getAtomCoordinates(from).getValue();
		coords2 = chemData->getAtomCoordinates(to).getValue();

		spewCylinder(pv, action, tce, coords1, coords2, rad, genTexCoords,
			unitCylinder);
	BONDLOOP_END
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Generates triangles representing a cylinder.
//    Invoked by the generatePrimitives method.
//  
// Use: private

void
ChemDisplay::
generateCylinderBAAA
    (SoAction* action)
//
////////////////////////////////////////////////////////////////////////
{
	int32_t		from, to;
	float		rad;
	SbVec3f 	coords1, coords2;
	SbBool		genTexCoords;
	SoPrimitiveVertex					pv;
	const SoTextureCoordinateElement	*tce;

	SoState *state = action->getState();
	ChemBaseData *chemData = ChemBaseDataElement::get(state);
	ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);
	ChemColor *chemColor = ChemColorElement::get(state);

	//int displayStyle = cdp->bondCylinderDisplayStyle.getValue();
	SbBool doHydrogens = cdp->showHydrogens.getValue();

	// Determine whether we should generate our own texture coordinates
	genTexCoords = TRUE;
	switch (SoTextureCoordinateElement::getType(state)) {
		case SoTextureCoordinateElement::EXPLICIT:
			genTexCoords = TRUE;
			break;
		case SoTextureCoordinateElement::FUNCTION:
			genTexCoords = FALSE;
			break;
	}

	// If we're not generating our own coordinates, we'll need the
	// texture coordinate element to get coords based on points/normals.
	if (! genTexCoords) {
		tce = SoTextureCoordinateElement::getInstance(state);
	}

	rad = cdp->bondCylinderRadius.getValue();
	SbColor colorFrom, colorTo;
	int32_t indexFrom, indexTo;
	const SbColor *atomColors = chemColor->atomColor.getValues(0);
	BONDLOOP_START(bondIndex)
		from = chemData->getBondFrom(theBond);
		to = chemData->getBondTo(theBond);
		if (!doHydrogens) {
			if ((chemData->getAtomicNumber(from) == 1) ||
				(chemData->getAtomicNumber(to) == 1)) continue;
		}
		colorFrom = atomColors[from];
		colorTo = atomColors[to];
		indexFrom = from;
		indexTo = to;
		if (colorFrom == colorTo) indexTo = indexFrom;
		coords1 = chemData->getAtomCoordinates(from);
		coords2 = chemData->getAtomCoordinates(to);

		spewCylinderWithInterpolatedColor(pv, action, tce,
			coords1, coords2, rad, indexFrom, indexTo, genTexCoords,
			unitCylinder);
	BONDLOOP_END
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Generates triangles representing a cylinder.
//    Invoked by the generatePrimitives method.
//  
// Use: private

void
ChemDisplay::
generateCylinderBAAI
    (SoAction* action)
//
////////////////////////////////////////////////////////////////////////
{
	int32_t		from, to;
	float		rad;
	SbVec3f 	coords1, coords2;
	SbBool		genTexCoords;
	SoPrimitiveVertex					pv;
	const SoTextureCoordinateElement	*tce;

	SoState *state = action->getState();
	ChemBaseData *chemData = ChemBaseDataElement::get(state);
	ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);
	ChemColor *chemColor = ChemColorElement::get(state);

	//int displayStyle = cdp->bondCylinderDisplayStyle.getValue();
	SbBool doHydrogens = cdp->showHydrogens.getValue();

	// Determine whether we should generate our own texture coordinates
	genTexCoords = TRUE;
	switch (SoTextureCoordinateElement::getType(state)) {
		case SoTextureCoordinateElement::EXPLICIT:
			genTexCoords = TRUE;
			break;
		case SoTextureCoordinateElement::FUNCTION:
			genTexCoords = FALSE;
			break;
	}

	// If we're not generating our own coordinates, we'll need the
	// texture coordinate element to get coords based on points/normals.
	if (! genTexCoords) {
		tce = SoTextureCoordinateElement::getInstance(state);
	}

	rad = cdp->bondCylinderRadius.getValue();
	SbColor colorFrom, colorTo;
	int32_t indexFrom, indexTo;
	const SbColor *atomColors = chemColor->atomColor.getValues(0);
	BONDLOOP_START(bondIndex)
		from = chemData->getBondFrom(theBond);
		to = chemData->getBondTo(theBond);
		if (!doHydrogens) {
			if ((chemData->getAtomicNumber(from) == 1) ||
				(chemData->getAtomicNumber(to) == 1)) continue;
		}
		colorFrom = atomColors[chemData->getAtomIndex(from)];
		colorTo = atomColors[chemData->getAtomIndex(to)];
		indexFrom = chemData->getAtomIndex(from);
		indexTo = chemData->getAtomIndex(to);
		if (colorFrom == colorTo) indexTo = indexFrom;
		coords1 = chemData->getAtomCoordinates(from);
		coords2 = chemData->getAtomCoordinates(to);

		spewCylinderWithInterpolatedColor(pv, action, tce,
			coords1, coords2, rad, indexFrom, indexTo, genTexCoords,
			unitCylinder);
	BONDLOOP_END
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Generates triangles representing a cylinder.
//    Invoked by the generatePrimitives method.
//  
// Use: private

void
ChemDisplay::
generateCylinderBHAA
    (SoAction* action)
//
////////////////////////////////////////////////////////////////////////
{
	int32_t		from, to;
	float		rad;
	SbVec3f 	coords1, coords2;
	SbBool		genTexCoords;
	SoPrimitiveVertex					pv;
	const SoTextureCoordinateElement	*tce;

	SoState *state = action->getState();
	ChemBaseData *chemData = ChemBaseDataElement::get(state);
	ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);
	ChemColor *chemColor = ChemColorElement::get(state);

	//int displayStyle = cdp->bondCylinderDisplayStyle.getValue();
	SbBool doHydrogens = cdp->showHydrogens.getValue();

	// Determine whether we should generate our own texture coordinates
	genTexCoords = TRUE;
	switch (SoTextureCoordinateElement::getType(state)) {
		case SoTextureCoordinateElement::EXPLICIT:
			genTexCoords = TRUE;
			break;
		case SoTextureCoordinateElement::FUNCTION:
			genTexCoords = FALSE;
			break;
	}

	// If we're not generating our own coordinates, we'll need the
	// texture coordinate element to get coords based on points/normals.
	if (! genTexCoords) {
		tce = SoTextureCoordinateElement::getInstance(state);
	}

	rad = cdp->bondCylinderRadius.getValue();
	SbColor colorFrom, colorTo;
	int32_t indexFrom, indexTo;
	const SbColor *atomColors = chemColor->atomColor.getValues(0);
	BONDLOOP_START(bondIndex)
		from = chemData->getBondFrom(theBond);
		to = chemData->getBondTo(theBond);
		if (!doHydrogens) {
			if ((chemData->getAtomicNumber(from) == 1) ||
				(chemData->getAtomicNumber(to) == 1)) continue;
		}
		colorFrom = atomColors[from];
		colorTo = atomColors[to];
		indexFrom = from;
		indexTo = to;
		if (colorFrom == colorTo) indexTo = indexFrom;
		coords1 = chemData->getAtomCoordinates(from).getValue();
		coords2 = chemData->getAtomCoordinates(to).getValue();

		spewCylinderHalfBonded(pv, action, tce,
			coords1, coords2, rad, indexFrom, indexTo, genTexCoords,
			unitCylinder);
	BONDLOOP_END
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Generates triangles representing a cylinder.
//    Invoked by the generatePrimitives method.
//  
// Use: private

void
ChemDisplay::
generateCylinderBHAI
    (SoAction* action)
//
////////////////////////////////////////////////////////////////////////
{
	int32_t		from, to;
	float		rad;
	SbVec3f 	coords1, coords2;
	SbBool		genTexCoords;
	SoPrimitiveVertex					pv;
	const SoTextureCoordinateElement	*tce;

	SoState *state = action->getState();
	ChemBaseData *chemData = ChemBaseDataElement::get(state);
	ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);
	ChemColor *chemColor = ChemColorElement::get(state);

	//int displayStyle = cdp->bondCylinderDisplayStyle.getValue();
	SbBool doHydrogens = cdp->showHydrogens.getValue();

	// Determine whether we should generate our own texture coordinates
	genTexCoords = TRUE;
	switch (SoTextureCoordinateElement::getType(state)) {
		case SoTextureCoordinateElement::EXPLICIT:
			genTexCoords = TRUE;
			break;
		case SoTextureCoordinateElement::FUNCTION:
			genTexCoords = FALSE;
			break;
	}

	// If we're not generating our own coordinates, we'll need the
	// texture coordinate element to get coords based on points/normals.
	if (! genTexCoords) {
		tce = SoTextureCoordinateElement::getInstance(state);
	}

	rad = cdp->bondCylinderRadius.getValue();
	SbColor colorFrom, colorTo;
	int32_t indexFrom, indexTo;
	const SbColor *atomColors = chemColor->atomColor.getValues(0);
	BONDLOOP_START(bondIndex)
		from = chemData->getBondFrom(theBond);
		to = chemData->getBondTo(theBond);
		if (!doHydrogens) {
			if ((chemData->getAtomicNumber(from) == 1) ||
				(chemData->getAtomicNumber(to) == 1)) continue;
		}
		colorFrom = atomColors[chemData->getAtomIndex(from)];
		colorTo = atomColors[chemData->getAtomIndex(to)];
		indexFrom = chemData->getAtomIndex(from);
		indexTo = chemData->getAtomIndex(to);
		if (colorFrom == colorTo) indexTo = indexFrom;
		coords1 = chemData->getAtomCoordinates(from).getValue();
		coords2 = chemData->getAtomCoordinates(to).getValue();

		spewCylinderHalfBonded(pv, action, tce,
			coords1, coords2, rad, indexFrom, indexTo, genTexCoords,
			unitCylinder);
	BONDLOOP_END
}

// --> EGB && SGB
void		
ChemDisplay::beginCylinders(const int displayStyle, ChemUnitCylinder *unitCylinder)
{
	if (unitCylinder == NULL)
		unitCylinder = this->unitCylinder;
		
	if 	(displayStyle == ChemDisplayParam::BONDCYLINDER_ARRAY_NOCAP||
		 displayStyle == ChemDisplayParam::BONDCYLINDER_ARRAY_FLATCAP || 
		 displayStyle == ChemDisplayParam::BONDCYLINDER_ARRAY_ROUNDCAP)
	{
			glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

			glEnableClientState(GL_NORMAL_ARRAY);
			glNormalPointer(GL_FLOAT,sizeof(SbVec3f),unitCylinder->getArrayCylinderNormals());
		
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3,GL_FLOAT,sizeof(SbVec3f),unitCylinder->getCylinderVertices());

	}
	else if 
		(displayStyle == ChemDisplayParam::BONDCYLINDER_SEMI_NOCAP ||
		displayStyle == ChemDisplayParam::BONDCYLINDER_SEMI_FLATCAP ||
		displayStyle == ChemDisplayParam::BONDCYLINDER_SEMI_ROUNDCAP)
	{
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


		glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT,sizeof(SbVec3f),unitCylinder->getSemiCylinderNormals());

		glEnableClientState(GL_VERTEX_ARRAY);	
		glVertexPointer(3,GL_FLOAT,sizeof(SbVec3f),unitCylinder->getSemiCylinderVertices());
	}
	else if 
		(displayStyle == ChemDisplayParam::BONDCYLINDER_GLUCYLINDER_NOCAP ||
		displayStyle == ChemDisplayParam::BONDCYLINDER_GLUCYLINDER_FLATCAP ||
		displayStyle == ChemDisplayParam::BONDCYLINDER_GLUCYLINDER_ROUNDCAP)
	{
		
			gluQuadricNormals(unitCylinder->getCylinderQuadric(),GLU_SMOOTH);
			gluQuadricNormals(unitCylinder->getTopCapQuadric(),GLU_SMOOTH);
			gluQuadricNormals(unitCylinder->getBottomCapQuadric(),GLU_SMOOTH);

	}
}

void		
ChemDisplay::endCylinders(const int displayStyle)
{
	if 
		(displayStyle == ChemDisplayParam::BONDCYLINDER_ARRAY_NOCAP ||
		 displayStyle == ChemDisplayParam::BONDCYLINDER_ARRAY_FLATCAP || 
		 displayStyle == ChemDisplayParam::BONDCYLINDER_ARRAY_ROUNDCAP ||
		 displayStyle == ChemDisplayParam::BONDCYLINDER_SEMI_NOCAP ||
		displayStyle == ChemDisplayParam::BONDCYLINDER_SEMI_FLATCAP ||
		displayStyle == ChemDisplayParam::BONDCYLINDER_SEMI_ROUNDCAP)
	{
		glPopClientAttrib();
	}
}
// <-- EGB && SGB

//
// End of CYLINDER section
//

#undef BONDLOOP_START
#undef BONDLOOP_END
// --> EGB && SGB
#undef ADJUST_CYLINDER
#undef PRE_ADJUST_CYLINDER
#undef POST_ADJUST_CYLINDER
#undef DOLOD
#undef DOHLOD
// <-- EGB && SGB
// --> octree duplicates
#undef BNDDUP
// <-- octree duplicates

