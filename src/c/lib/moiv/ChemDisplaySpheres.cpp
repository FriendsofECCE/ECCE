/*
 * ChemDisplaySpheres.pp
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
#include "inv/ChemKit/ChemUnitSphere.H"

// --> lod
#include "inv/ChemKit/ChemLOD.H"
// <-- lod

// --> octree culling
#include "inv/ChemKit/ChemOctreeNode.H"
// <-- octree culling

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

// --> EGB && SGB

#define DOLOD(RXCX) \
if (displayStyle == ChemDisplayParam::ATOMSPHERE_LOD) { \
		LODfNormalSphere##RXCX(doHydrogens,radScale,atomRadii,atomColors,chemData,theAtomIndex); \
		glDisable(GL_COLOR_MATERIAL); \
return; \
} else if (displayStyle == ChemDisplayParam::ATOMSPHERE_LOD_GEOMETRY) { \
		LODgNormalSphere##RXCX(doHydrogens,radScale,atomRadii,atomColors,chemData,theAtomIndex); \
		glDisable(GL_COLOR_MATERIAL); \
		return; \
}else if (clipAtoms) { \
		clipNormalSphere##RXCX(displayStyle,doHydrogens,doHemi,radScale,atomRadii,atomColors,chemData,theAtomIndex,action);\
		glDisable(GL_COLOR_MATERIAL); \
		return; \
		}


#define DOHLOD(RXCX) \
if (displayStyle == ChemDisplayParam::ATOMSPHERE_LOD) { \
		LODfHighlightSphere##RXCX(highlightStyle,highlightColor,doHydrogens,radScale,atomRadii,atomColors,chemData,theAtomIndex); \
		glDisable(GL_COLOR_MATERIAL); \
		return; \
}else if (displayStyle == ChemDisplayParam::ATOMSPHERE_LOD_GEOMETRY){ \
		LODgHighlightSphere##RXCX(highlightStyle,highlightColor,doHydrogens,radScale,atomRadii,atomColors,chemData,theAtomIndex); \
		glDisable(GL_COLOR_MATERIAL); \
		return; \
	}
// <-- EGB && SGB

//
// Start of SPHERE section
//

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Checks to see if the current display list is valid and if not
//    causes a new one to be created.  Also gets and returns values for
//    statem chemColor, chemRadii and cdp.
//
// Use: protected

void
ChemDisplay::setupSpheres(SoGLRenderAction *action, SoState *&state,
	ChemColor *&chemColor, ChemRadii *&chemRadii, ChemDisplayParam *&cdp,
	ChemBaseData *&chemData)
//
////////////////////////////////////////////////////////////////////////
{
	SbBool	doNormals, doTextures;
	int 	atomSphereDisplayStyle;
	float	complexity;

	state     = action->getState();
  chemColor = ChemColorElement::get(state);
	chemRadii = ChemRadiiElement::get(state);
	cdp       = ChemDisplayParamElement::get(state);
	chemData  = ChemBaseDataElement::get(state);

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

	atomSphereDisplayStyle = cdp->atomSphereDisplayStyle.getValue();
	complexity = cdp->atomSphereComplexity.getValue();

// --> EGB && SGB
	if (lastSphereComplexity != complexity ||
		lastSphereDisplayStyle != atomSphereDisplayStyle)
	{
		// generate lod again
		if (atomSphereDisplayStyle == ChemDisplayParam::ATOMSPHERE_LOD
			&& complexity < 0.9) // agressive LOD
			lodSelector->changeSphereComplexity(complexity*0.75);
		else
			lodSelector->changeSphereComplexity(complexity);
	}
// <-- EGB && SGB

	// Checks to perform:
	//	  Need to regenerate unit sphere vertices because of complexity change?
	//		  if yes, then also regenerate display list depending on the
	//			  values for doTextures and doNormals
	//		  if no, do we need to regenerate the display list? (dependent on
	//			  last values of doTextures and doNormals versus
	//			  the current values.  also depends on the "context" since
	//			  printing uses a different context than display on the 
	//			  screen and a display list is only valid for a given 
	//			  context.

#ifdef DISPLAYLIST
	if (lastSphereComplexity != complexity) {
		if (unitSphere != NULL) delete unitSphere;
		unitSphere = new ChemUnitSphere(complexity);
		if (!glIsList(unitSphereList)) {
			unitSphereList = glGenLists(1);
		}
		assert (unitSphereList != 0);
		glNewList(unitSphereList, GL_COMPILE);
// --> EGB && SGB
//		unitSphere->generateDisplay(doHemi, doNormals, doTextures);
		unitSphere->generateDisplay(doHemi, doNormals, doTextures, 
			atomSphereDisplayStyle, action);
// <-- EGB && SGB
		glEndList();
	}
	else {
		if ((lastSphereDoNormals != doNormals) ||
			(lastSphereDoTextures != doTextures) ||
			(lastSphereDisplayStyle != atomSphereDisplayStyle) ||
	    (lastCacheContext != action->getCacheContext())) {
			if (!glIsList(unitSphereList)) {
				unitSphereList = glGenLists(1);
			}
			assert (unitSphereList != 0);
			glNewList(unitSphereList, GL_COMPILE);
// --> EGB && SGB
//		unitSphere->generateDisplay(doHemi, doNormals, doTextures);
		unitSphere->generateDisplay(doHemi, doNormals, doTextures, 
			atomSphereDisplayStyle, action);
// <-- EGB && SGB
			glEndList();
		}
	}
#else
	if (lastSphereComplexity != complexity) {
		if (unitSphere != NULL) delete unitSphere;
		unitSphere = new ChemUnitSphere(complexity);
	}
#endif

	lastSphereComplexity = complexity;
	lastSphereDoNormals = doNormals;
	lastSphereDoTextures = doTextures;
	lastSphereDisplayStyle = atomSphereDisplayStyle;
	lastCacheContext = action->getCacheContext();
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Performs GL rendering of the molecule using spheres.
//	  Invoked by the GLRender method.
//
// Use: protected

void
ChemDisplay::renderAtomsAsSpheres(SoGLRenderAction *action,
	SbBool renderNormal, SbBool renderHighlight,
	const MFVec2i &normalIndex, const MFVec2i &highlightIndex)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::renderAtomsAsSpheres\n");
#endif
	SoState *state;
    ChemColor *chemColor;
	ChemRadii *chemRadii;
	ChemDisplayParam *cdp;
	ChemBaseData *chemData;

	// Check that display list is OK and get values from the state.
	setupSpheres(action, state, chemColor, chemRadii, cdp, chemData);

	// Get the rest of the values required
	int displayStyle = cdp->displayStyle.getValue();
	int highlightStyle = cdp->highlightStyle.getValue();
	const SbColor &highlightColor = cdp->highlightColor.getValue();
	SbBool doHydrogens = cdp->showHydrogens.getValue();
	SbBool doHemi = FALSE;
// --> EGB && SGB
//	if (cdp->atomSphereDisplayStyle.getValue() == ChemDisplayParam::ATOMSPHERE_HEMISPHERES) 
	if (cdp->atomSphereDisplayStyle.getValue() == ChemDisplayParam::ATOMSPHERE_HEMISPHERES ||
		  cdp->atomSphereDisplayStyle.getValue() == ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES) 
// <-- EGB && SGB
	{
		doHemi = TRUE;
	}
	float radScale = cdp->atomRadiiScaleFactor.getValue();
	if (displayStyle != ChemDisplayParam::DISPLAY_CPK) {
		radScale *= cdp->ballStickSphereScaleFactor.getValue();
	}
	const SbColor *atomColors = chemColor->atomColor.getValues(0);
	const float *atomRadii = chemRadii->atomRadii.getValues(0);

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
/*
	lazyElt->setColorMaterial(state, TRUE);
	lazyElt->sendNoMaterial(state);
*/
#endif

// --> EGB && SGB
	int atomSphereDisplayStyle = cdp->atomSphereDisplayStyle.getValue();
	if (displayStyle == ChemDisplayParam::DISPLAY_CPK &&
		atomSphereDisplayStyle == ChemDisplayParam::ATOMSPHERE_LOD)
		atomSphereDisplayStyle = ChemDisplayParam::ATOMSPHERE_LOD_GEOMETRY;

	displayStyle = atomSphereDisplayStyle;
	beginSpheres(displayStyle);
// <-- EGB && SGB

	// Render the spheres
	switch (chemRadii->atomRadiiBinding.getValue()) {
	  case ChemRadii::RADII_OVERALL:
		switch (chemColor->atomColorBinding.getValue()) {
		  case ChemColor::ATOM_OVERALL:
			if (renderNormal) {
				normalSphereROCO(action, 
					displayStyle, doHydrogens, doHemi, radScale,
					atomColors, atomRadii, chemData, normalIndex);
			}
			if (renderHighlight) {
				highlightSphereROCO(action, 
					displayStyle, highlightStyle, highlightColor,
					doHydrogens, doHemi, radScale,
					atomColors, atomRadii, chemData, highlightIndex);
			}
			break;
		  case ChemColor::ATOM_PER_ATOM:
			if (renderNormal) {
				normalSphereROCA(action, 
					displayStyle, doHydrogens, doHemi, radScale,
					atomColors, atomRadii, chemData, normalIndex);
			}
			if (renderHighlight) {
				highlightSphereROCA(action, 
					displayStyle, highlightStyle, highlightColor,
					doHydrogens, doHemi, radScale,
					atomColors, atomRadii, chemData, highlightIndex);
			}
			break;
		  case ChemColor::ATOM_PER_ATOM_INDEXED:
			if (renderNormal) {
				normalSphereROCI(action, 
					displayStyle, doHydrogens, doHemi, radScale,
					atomColors, atomRadii, chemData, normalIndex);
			}
			if (renderHighlight) {
				highlightSphereROCI(action, 
					displayStyle, highlightStyle, highlightColor,
					doHydrogens, doHemi, radScale,
					atomColors, atomRadii, chemData, highlightIndex);
			}
			break;
		  default:
			break;
		}
		break;

	  case ChemRadii::RADII_PER_ATOM:
		switch (chemColor->atomColorBinding.getValue()) {
		  case ChemColor::ATOM_OVERALL:
			if (renderNormal) {
				normalSphereRACO(action, 
					displayStyle, doHydrogens, doHemi, radScale,
					atomColors, atomRadii, chemData, normalIndex);
			}
			if (renderHighlight) {
				highlightSphereRACO(action, 
					displayStyle, highlightStyle, highlightColor,
					doHydrogens, doHemi, radScale,
					atomColors, atomRadii, chemData, highlightIndex);
			}
			break;
		  case ChemColor::ATOM_PER_ATOM:
			if (renderNormal) {
				normalSphereRACA(action, 
					displayStyle, doHydrogens, doHemi, radScale,
					atomColors, atomRadii, chemData, normalIndex);
			}
			if (renderHighlight) {
				highlightSphereRACA(action, 
					displayStyle, highlightStyle, highlightColor,
					doHydrogens, doHemi, radScale,
					atomColors, atomRadii, chemData, highlightIndex);
			}
			break;
		  case ChemColor::ATOM_PER_ATOM_INDEXED:
			if (renderNormal) {
				normalSphereRACI(action, 
					displayStyle, doHydrogens, doHemi, radScale,
					atomColors, atomRadii, chemData, normalIndex);
			}
			if (renderHighlight) {
				highlightSphereRACI(action, 
					displayStyle, highlightStyle, highlightColor,
					doHydrogens, doHemi, radScale,
					atomColors, atomRadii, chemData, highlightIndex);
			}
			break;
		  default:
			break;
		}
		break;

	  case ChemRadii::RADII_PER_ATOM_INDEXED:
		switch (chemColor->atomColorBinding.getValue()) {
		  case ChemColor::ATOM_OVERALL:
			if (renderNormal) {
				normalSphereRICO(action, 
					displayStyle, doHydrogens, doHemi, radScale,
					atomColors, atomRadii, chemData, normalIndex);
			}
			if (renderHighlight) {
				highlightSphereRICO(action, 
					displayStyle, highlightStyle, highlightColor,
					doHydrogens, doHemi, radScale,
					atomColors, atomRadii, chemData, highlightIndex);
			}
			break;
		  case ChemColor::ATOM_PER_ATOM:
			if (renderNormal) {
				normalSphereRICA(action, 
					displayStyle, doHydrogens, doHemi, radScale,
					atomColors, atomRadii, chemData, normalIndex);
			}
			if (renderHighlight) {
				highlightSphereRICA(action, 
					displayStyle, highlightStyle, highlightColor,
					doHydrogens, doHemi, radScale,
					atomColors, atomRadii, chemData, highlightIndex);
			}
			break;
		  case ChemColor::ATOM_PER_ATOM_INDEXED:
			if (renderNormal) {
				normalSphereRICI(action, 
					displayStyle, doHydrogens, doHemi, radScale,
					atomColors, atomRadii, chemData, normalIndex);
			}
			if (renderHighlight) {
				highlightSphereRICI(action, 
					displayStyle, highlightStyle, highlightColor,
					doHydrogens, doHemi, radScale,
					atomColors, atomRadii, chemData, highlightIndex);
			}
			break;
		  default:
			break;
		}
		break;

	  default:	
		break;
	}
// --> EGB && SGB
	endSpheres(displayStyle);
// <-- EGB && SGB

#ifndef IV2_0
	lazyElt->reset(state, myMask);
#endif

	endSolidShape(action);
}

//
// The following preprocessor-generated functions handle the various
// ways to render the spheres.
// RO ... RadiusOverall
// RA ... RadiusPerAtom
// RI ... RadiusPerAtomIndexed
// CO ... ColorOverall
// CA ... ColorPerAtom
// CI ... ColorPerAtomIndexed
//
//

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders the molecule using the unit sphere display list.
//    Invoked by the GLRender method.
//  
// Use: private

void
ChemDisplay::
normalSphereROCO
	(SoGLRenderAction* action, const int displayStyle,
	const SbBool doHydrogens, const SbBool doHemi,
	const float radScale, const SbColor *atomColors, const float *atomRadii,
	const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
//  
////////////////////////////////////////////////////////////////////////
{
	float		rad;
	SbVec3f 	theScale;
	SbRotation	theRotation;
	SbMatrix	theMatrix;
	SbVec3f		sphereCoords;

	// Check to see if there is anything to render.
	if (theAtomIndex.getNum() == 0) return;

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

// --> compatible modes
	DOLOD(ROCO);
// <-- compatible modes

	if (doHemi) {
		theRotation = currentRotation;
		rad = atomRadii[0];
// --> EGB && SGB original MOIV bug , radscale *= ERROR
		rad *= radScale;
// <-- EGB && SGB original MOIV bug , radscale *= ERROR
		glColor3fv(atomColors[0].getValue());
		ATOMLOOP_START(theAtomIndex)
			if (!doHydrogens &&
				chemData->getAtomicNumber(theAtom) == 1) {
				continue;
			}
			sphereCoords = chemData->getAtomCoordinates(theAtom);
// --> EGB && SGB original MOIV bug , radscale *= ERROR
//			rad *= radScale;
// <-- EGB && SGB original MOIV bug , radscale *= ERROR
			theScale.setValue(rad, rad, rad);
			theMatrix.setTransform(sphereCoords, theRotation, theScale);
			glPushMatrix();
			glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
			glCallList(unitSphereList);
#else
// --> EGB && SGB
//		unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
		unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
			displayStyle, action);
// <-- EGB && SGB
			
#endif
			glPopMatrix();
		ATOMLOOP_END
	}
	else {
		// Not using hemispheres
		theRotation.setValue(0.0, 0.0, 0.0, 1.0);
		rad = atomRadii[0];
// --> EGB && SGB original MOIV bug , radscale *= ERROR
			rad *= radScale;
// <-- EGB && SGB original MOIV bug , radscale *= ERROR
		glColor3fv(atomColors[0].getValue());
		ATOMLOOP_START(theAtomIndex)
			if (!doHydrogens &&
				chemData->getAtomicNumber(theAtom) == 1) {
				continue;
			}
			sphereCoords = chemData->getAtomCoordinates(theAtom);
// --> EGB && SGB original MOIV bug , radscale *= ERROR
//			rad *= radScale;
// <-- EGB && SGB original MOIV bug , radscale *= ERROR
			theScale.setValue(rad, rad, rad);
			theMatrix.setTransform(sphereCoords, theRotation, theScale);
			glPushMatrix();
			glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
			glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
			glPopMatrix();
		ATOMLOOP_END
	}
	glDisable(GL_COLOR_MATERIAL);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders the molecule using the unit sphere display list.
//    Invoked by the GLRender method.
//  
// Use: private

void
ChemDisplay::
normalSphereROCA
	(SoGLRenderAction* action, const int displayStyle,
	const SbBool doHydrogens, const SbBool doHemi,
	const float radScale, const SbColor *atomColors, const float *atomRadii,
	const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
//  
////////////////////////////////////////////////////////////////////////
{
	float		rad;
	SbVec3f 	theScale;
	SbRotation	theRotation;
	SbMatrix	theMatrix;
	SbVec3f		sphereCoords;

	// Check to see if there is anything to render.
	if (theAtomIndex.getNum() == 0) return;

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

// --> compatible modes
	DOLOD(ROCA);
// <-- compatible modes

	if (doHemi) {
		theRotation = currentRotation;
		rad = atomRadii[0];
// --> EGB && SGB original MOIV bug , radscale *= ERROR
		rad *= radScale;
// <-- EGB && SGB original MOIV bug , radscale *= ERROR
		ATOMLOOP_START(theAtomIndex)
			if (!doHydrogens &&
				chemData->getAtomicNumber(theAtom) == 1) {
				continue;
			}
			sphereCoords = chemData->getAtomCoordinates(theAtom);
// --> EGB && SGB original MOIV bug , radscale *= ERROR
//			rad *= radScale;
// <-- EGB && SGB original MOIV bug , radscale *= ERROR
			theScale.setValue(rad, rad, rad);
			theMatrix.setTransform(sphereCoords, theRotation, theScale);
			glColor3fv(atomColors[theAtom].getValue());
			glPushMatrix();
			glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
			glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
			glPopMatrix();
		ATOMLOOP_END
	}
	else {
		// Not using hemispheres
		theRotation.setValue(0.0, 0.0, 0.0, 1.0);
		rad = atomRadii[0];
// --> EGB && SGB original MOIV bug , radscale *= ERROR
		rad *= radScale;
// <-- EGB && SGB original MOIV bug , radscale *= ERROR
		ATOMLOOP_START(theAtomIndex)
			if (!doHydrogens &&
				chemData->getAtomicNumber(theAtom) == 1) {
				continue;
			}
			sphereCoords = chemData->getAtomCoordinates(theAtom);
// --> EGB && SGB original MOIV bug , radscale *= ERROR
//			rad *= radScale;
// <-- EGB && SGB original MOIV bug , radscale *= ERROR
			theScale.setValue(rad, rad, rad);
			theMatrix.setTransform(sphereCoords, theRotation, theScale);
			glColor3fv(atomColors[theAtom].getValue());
			glPushMatrix();
			glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
			glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
			glPopMatrix();
		ATOMLOOP_END
	}
	glDisable(GL_COLOR_MATERIAL);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders the molecule using the unit sphere display list.
//    Invoked by the GLRender method.
//  
// Use: private

void
ChemDisplay::
normalSphereROCI
	(SoGLRenderAction* action, const int displayStyle,
	const SbBool doHydrogens, const SbBool doHemi,
	const float radScale, const SbColor *atomColors, const float *atomRadii,
	const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
//  
////////////////////////////////////////////////////////////////////////
{
	float		rad;
	SbVec3f 	theScale;
	SbRotation	theRotation;
	SbMatrix	theMatrix;
	SbVec3f		sphereCoords;
	int32_t		localAtomIndex;

	// Check to see if there is anything to render.
	if (theAtomIndex.getNum() == 0) return;

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

// --> compatible modes
	DOLOD(ROCI);
// <-- compatible modes

	if (doHemi) {
		theRotation = currentRotation;
		rad = atomRadii[0];
// --> EGB && SGB original MOIV bug , radscale *= ERROR
		rad *= radScale;
// <--EGB && SGB original MOIV bug , radscale *= ERROR

		ATOMLOOP_START(theAtomIndex)
			if (!doHydrogens &&
				chemData->getAtomicNumber(theAtom) == 1) {
				continue;
			}
			sphereCoords = chemData->getAtomCoordinates(theAtom);
			localAtomIndex = chemData->getAtomIndex(theAtom);
// --> EGB && SGB original MOIV bug , radscale *= ERROR
//			rad *= radScale;
// <--EGB && SGB original MOIV bug , radscale *= ERROR
			theScale.setValue(rad, rad, rad);
			theMatrix.setTransform(sphereCoords, theRotation, theScale);
			glColor3fv(atomColors[localAtomIndex].getValue());
			glPushMatrix();
			glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
			glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
			glPopMatrix();
		ATOMLOOP_END
	}
	else {
		// Not using hemispheres
		theRotation.setValue(0.0, 0.0, 0.0, 1.0);
		rad = atomRadii[0];
// --> EGB && SGB original MOIV bug , radscale *= ERROR
		rad *= radScale;
// <-- EGB && SGB original MOIV bug , radscale *= ERROR
		ATOMLOOP_START(theAtomIndex)
			if (!doHydrogens &&
				chemData->getAtomicNumber(theAtom) == 1) {
				continue;
			}
			sphereCoords = chemData->getAtomCoordinates(theAtom);
			localAtomIndex = chemData->getAtomIndex(theAtom);
// --> EGB && SGB original MOIV bug , radscale *= ERROR
//			rad *= radScale;
// <-- EGB && SGB original MOIV bug , radscale *= ERROR
			theScale.setValue(rad, rad, rad);
			theMatrix.setTransform(sphereCoords, theRotation, theScale);
			glColor3fv(atomColors[localAtomIndex].getValue());
			glPushMatrix();
			glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
			glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
			glPopMatrix();
		ATOMLOOP_END
	}
	glDisable(GL_COLOR_MATERIAL);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders the molecule using the unit sphere display list.
//    Invoked by the GLRender method.
//  
// Use: private

void
ChemDisplay::
normalSphereRACO
	(SoGLRenderAction* action, const int displayStyle,
	const SbBool doHydrogens, const SbBool doHemi,
	const float radScale, const SbColor *atomColors, const float *atomRadii,
	const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
//  
////////////////////////////////////////////////////////////////////////
{
	float		rad;
	SbVec3f 	theScale;
	SbRotation	theRotation;
	SbMatrix	theMatrix;
	SbVec3f		sphereCoords;

	// Check to see if there is anything to render.
	if (theAtomIndex.getNum() == 0) return;

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

// --> compatible modes
	DOLOD(RACO);
// <-- compatible modes

	if (doHemi) {
		theRotation = currentRotation;
		glColor3fv(atomColors[0].getValue());
		ATOMLOOP_START(theAtomIndex)
			if (!doHydrogens &&
				chemData->getAtomicNumber(theAtom) == 1) {
				continue;
			}
			sphereCoords = chemData->getAtomCoordinates(theAtom);
			rad = atomRadii[theAtom];
			rad *= radScale;
			theScale.setValue(rad, rad, rad);
			theMatrix.setTransform(sphereCoords, theRotation, theScale);
			glPushMatrix();
			glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
			glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
			glPopMatrix();
		ATOMLOOP_END
	}
	else {
		// Not using hemispheres
		theRotation.setValue(0.0, 0.0, 0.0, 1.0);
		glColor3fv(atomColors[0].getValue());
		ATOMLOOP_START(theAtomIndex)
			if (!doHydrogens &&
				chemData->getAtomicNumber(theAtom) == 1) {
				continue;
			}
			sphereCoords = chemData->getAtomCoordinates(theAtom);
			rad = atomRadii[theAtom];
			rad *= radScale;
			theScale.setValue(rad, rad, rad);
			theMatrix.setTransform(sphereCoords, theRotation, theScale);
			glPushMatrix();
			glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
			glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
			glPopMatrix();
		ATOMLOOP_END
	}
	glDisable(GL_COLOR_MATERIAL);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders the molecule using the unit sphere display list.
//    Invoked by the GLRender method.
//  
// Use: private

void
ChemDisplay::
normalSphereRACA
	(SoGLRenderAction* action, const int displayStyle,
	const SbBool doHydrogens, const SbBool doHemi,
	const float radScale, const SbColor *atomColors, const float *atomRadii,
	const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
//  
////////////////////////////////////////////////////////////////////////
{
	float		rad;
	SbVec3f 	theScale;
	SbRotation	theRotation;
	SbMatrix	theMatrix;
	SbVec3f		sphereCoords;

	// Check to see if there is anything to render.
	if (theAtomIndex.getNum() == 0) return;

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

// --> compatible modes
	DOLOD(RACA);
// <-- compatible modes

	if (doHemi) {
		theRotation = currentRotation;
		ATOMLOOP_START(theAtomIndex)
			if (!doHydrogens &&
				chemData->getAtomicNumber(theAtom) == 1) {
				continue;
			}
			sphereCoords = chemData->getAtomCoordinates(theAtom);
			rad = atomRadii[theAtom];
			rad *= radScale;
			theScale.setValue(rad, rad, rad);
			theMatrix.setTransform(sphereCoords, theRotation, theScale);
			glColor3fv(atomColors[theAtom].getValue());
			glPushMatrix();
			glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
			glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
			glPopMatrix();
		ATOMLOOP_END
	}
	else {
		// Not using hemispheres
		theRotation.setValue(0.0, 0.0, 0.0, 1.0);
		ATOMLOOP_START(theAtomIndex)
			if (!doHydrogens &&
				chemData->getAtomicNumber(theAtom) == 1) {
				continue;
			}
			sphereCoords = chemData->getAtomCoordinates(theAtom);
			rad = atomRadii[theAtom];
			rad *= radScale;
			theScale.setValue(rad, rad, rad);
			theMatrix.setTransform(sphereCoords, theRotation, theScale);
			glColor3fv(atomColors[theAtom].getValue());
			glPushMatrix();
			glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
			glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
			glPopMatrix();
		ATOMLOOP_END
	}
	glDisable(GL_COLOR_MATERIAL);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders the molecule using the unit sphere display list.
//    Invoked by the GLRender method.
//  
// Use: private

void
ChemDisplay::
normalSphereRACI
	(SoGLRenderAction* action, const int displayStyle,
	const SbBool doHydrogens, const SbBool doHemi,
	const float radScale, const SbColor *atomColors, const float *atomRadii,
	const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
//  
////////////////////////////////////////////////////////////////////////
{
	float		rad;
	SbVec3f 	theScale;
	SbRotation	theRotation;
	SbMatrix	theMatrix;
	SbVec3f		sphereCoords;
	int32_t		localAtomIndex;

	// Check to see if there is anything to render.
	if (theAtomIndex.getNum() == 0) return;

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

// --> compatible modes
	DOLOD(RACI);
// <-- compatible modes


	if (doHemi) {
		theRotation = currentRotation;
		ATOMLOOP_START(theAtomIndex)
			if (!doHydrogens &&
				chemData->getAtomicNumber(theAtom) == 1) {
				continue;
			}
			sphereCoords = chemData->getAtomCoordinates(theAtom);
			localAtomIndex = chemData->getAtomIndex(theAtom);
			rad = atomRadii[theAtom];
			rad *= radScale;
			theScale.setValue(rad, rad, rad);
			theMatrix.setTransform(sphereCoords, theRotation, theScale);
			glColor3fv(atomColors[localAtomIndex].getValue());
			glPushMatrix();
			glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
			glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
			glPopMatrix();
		ATOMLOOP_END
	}
	else {
		// Not using hemispheres
		theRotation.setValue(0.0, 0.0, 0.0, 1.0);
		ATOMLOOP_START(theAtomIndex)
			if (!doHydrogens &&
				chemData->getAtomicNumber(theAtom) == 1) {
				continue;
			}
			sphereCoords = chemData->getAtomCoordinates(theAtom);
			localAtomIndex = chemData->getAtomIndex(theAtom);
			rad = atomRadii[theAtom];
			rad *= radScale;
			theScale.setValue(rad, rad, rad);
			theMatrix.setTransform(sphereCoords, theRotation, theScale);
			glColor3fv(atomColors[localAtomIndex].getValue());
			glPushMatrix();
			glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
			glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
			glPopMatrix();
		ATOMLOOP_END
	}
	glDisable(GL_COLOR_MATERIAL);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders the molecule using the unit sphere display list.
//    Invoked by the GLRender method.
//  
// Use: private

void
ChemDisplay::
normalSphereRICO
	(SoGLRenderAction* action, const int displayStyle,
	const SbBool doHydrogens, const SbBool doHemi,
	const float radScale, const SbColor *atomColors, const float *atomRadii,
	const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
//  
////////////////////////////////////////////////////////////////////////
{
	float		rad;
	SbVec3f 	theScale;
	SbRotation	theRotation;
	SbMatrix	theMatrix;
	SbVec3f		sphereCoords;
	int32_t		localAtomIndex;

	// Check to see if there is anything to render.
	if (theAtomIndex.getNum() == 0) return;

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

// --> compatible modes
	DOLOD(RICO);
// <-- compatible modes

	if (doHemi) {
		theRotation = currentRotation;
		glColor3fv(atomColors[0].getValue());
		ATOMLOOP_START(theAtomIndex)
			if (!doHydrogens &&
				chemData->getAtomicNumber(theAtom) == 1) {
				continue;
			}
			sphereCoords = chemData->getAtomCoordinates(theAtom);
			localAtomIndex = chemData->getAtomIndex(theAtom);
			rad = atomRadii[localAtomIndex];
			rad *= radScale;
			theScale.setValue(rad, rad, rad);
			theMatrix.setTransform(sphereCoords, theRotation, theScale);
			glPushMatrix();
			glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
			glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
			glPopMatrix();
		ATOMLOOP_END
	}
	else {
		// Not using hemispheres
		theRotation.setValue(0.0, 0.0, 0.0, 1.0);
		glColor3fv(atomColors[0].getValue());
		ATOMLOOP_START(theAtomIndex)
			if (!doHydrogens &&
				chemData->getAtomicNumber(theAtom) == 1) {
				continue;
			}
			sphereCoords = chemData->getAtomCoordinates(theAtom);
			localAtomIndex = chemData->getAtomIndex(theAtom);
			rad = atomRadii[localAtomIndex];
			rad *= radScale;
			theScale.setValue(rad, rad, rad);
			theMatrix.setTransform(sphereCoords, theRotation, theScale);
			glPushMatrix();
			glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
			glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
			glPopMatrix();
		ATOMLOOP_END
	}
	glDisable(GL_COLOR_MATERIAL);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders the molecule using the unit sphere display list.
//    Invoked by the GLRender method.
//  
// Use: private

void
ChemDisplay::
normalSphereRICA
	(SoGLRenderAction* action, const int displayStyle,
	const SbBool doHydrogens, const SbBool doHemi,
	const float radScale, const SbColor *atomColors, const float *atomRadii,
	const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
//  
////////////////////////////////////////////////////////////////////////
{
	float		rad;
	SbVec3f 	theScale;
	SbRotation	theRotation;
	SbMatrix	theMatrix;
	SbVec3f		sphereCoords;
	int32_t		localAtomIndex;

	// Check to see if there is anything to render.
	if (theAtomIndex.getNum() == 0) return;

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

// --> EGB && SGB
	DOLOD(RICA);
// <-- EGB && SGB

	if (doHemi) {
		theRotation = currentRotation;
		ATOMLOOP_START(theAtomIndex)
			if (!doHydrogens &&
				chemData->getAtomicNumber(theAtom) == 1) {
				continue;
			}
			sphereCoords = chemData->getAtomCoordinates(theAtom);
		
			localAtomIndex = chemData->getAtomIndex(theAtom);
			rad = atomRadii[localAtomIndex];
			rad *= radScale;
			theScale.setValue(rad, rad, rad);
			theMatrix.setTransform(sphereCoords, theRotation, theScale);
			glColor3fv(atomColors[theAtom].getValue());
			glPushMatrix();
			glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
			glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
			glPopMatrix();
		ATOMLOOP_END
	}
	else {
		// Not using hemispheres
		theRotation.setValue(0.0, 0.0, 0.0, 1.0);
		ATOMLOOP_START(theAtomIndex)
			if (!doHydrogens &&
				chemData->getAtomicNumber(theAtom) == 1) {
				continue;
			}
			sphereCoords = chemData->getAtomCoordinates(theAtom);
			localAtomIndex = chemData->getAtomIndex(theAtom);
			rad = atomRadii[localAtomIndex];
			rad *= radScale;
			theScale.setValue(rad, rad, rad);
			theMatrix.setTransform(sphereCoords, theRotation, theScale);
			glColor3fv(atomColors[theAtom].getValue());
			glPushMatrix();
			glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
			glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
			glPopMatrix();
		ATOMLOOP_END
	}
	glDisable(GL_COLOR_MATERIAL);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Renders the molecule using the unit sphere display list.
//    Invoked by the GLRender method.
//  
// Use: private

void
ChemDisplay::
normalSphereRICI
	(SoGLRenderAction* action, const int displayStyle,
	const SbBool doHydrogens, const SbBool doHemi,
	const float radScale, const SbColor *atomColors, const float *atomRadii,
	const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
//  
////////////////////////////////////////////////////////////////////////
{
	float		rad;
	SbVec3f 	theScale;
	SbRotation	theRotation;
	SbMatrix	theMatrix;
	SbVec3f		sphereCoords;
	int32_t		localAtomIndex;

	// Check to see if there is anything to render.
	if (theAtomIndex.getNum() == 0) return;

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

// --> EGB && SGB
	DOLOD(RICI);
// <-- EGB && SGB

	if (doHemi) {
		theRotation = currentRotation;

		ATOMLOOP_START(theAtomIndex)
			if (!doHydrogens &&
				chemData->getAtomicNumber(theAtom) == 1) {
				continue;
			}
			sphereCoords = chemData->getAtomCoordinates(theAtom);
			localAtomIndex = chemData->getAtomIndex(theAtom);
			rad = atomRadii[localAtomIndex];
			rad *= radScale;
			theScale.setValue(rad, rad, rad);
			theMatrix.setTransform(sphereCoords, theRotation, theScale);
			glColor3fv(atomColors[localAtomIndex].getValue());
			glPushMatrix();
			glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
			glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
			glPopMatrix();
		ATOMLOOP_END
	}
	else {
		// Not using hemispheres
		theRotation.setValue(0.0, 0.0, 0.0, 1.0);

		ATOMLOOP_START(theAtomIndex)
			if (!doHydrogens &&
				chemData->getAtomicNumber(theAtom) == 1) {
				continue;
			}
			sphereCoords = chemData->getAtomCoordinates(theAtom);
			localAtomIndex = chemData->getAtomIndex(theAtom);
			rad = atomRadii[localAtomIndex];
			rad *= radScale;
			theScale.setValue(rad, rad, rad);
			
			theMatrix.setTransform(sphereCoords, theRotation, theScale);
			
			glColor3fv(atomColors[localAtomIndex].getValue());
			glPushMatrix();
			glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
			glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
			glPopMatrix();

		ATOMLOOP_END
	}

	glDisable(GL_COLOR_MATERIAL);
}

//
// The following preprocessor-generated functions handle the various
// ways to highlight the spheres.
// RO ... RadiusOverall
// RA ... RadiusPerAtom
// RI ... RadiusPerAtomIndexed
// CO ... ColorOverall
// CA ... ColorPerAtom
// CI ... ColorPerAtomIndexed
//
//

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights the molecule using the unit sphere display list.
//    Invoked by the GLRender method.
//  
// Use: private

void
ChemDisplay::
highlightSphereROCO
	(SoGLRenderAction* action, const int displayStyle,
	const int highlightStyle, const SbColor &highlightColor,
	const SbBool doHydrogens, const SbBool doHemi,
	const float radScale, const SbColor *atomColors, const float *atomRadii,
	const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
//  
////////////////////////////////////////////////////////////////////////
{
	float		rad;
	SbVec3f 	theScale;
	SbRotation	theRotation;
	SbMatrix	theMatrix;
	SbVec3f		sphereCoords;

	// Check to see if there is anything to render.
	if (theAtomIndex.getNum() == 0) return;

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

// --> compatible modes
	DOHLOD(ROCO)
// <-- compatible modes

	if (doHemi) {
		theRotation = currentRotation;
		rad = atomRadii[0];
// --> EGB && SGB original MOIV bug , radscale *= ERROR
		rad *= radScale;
// <-- EGB && SGB original MOIV bug , radscale *= ERROR
		glColor3fv(atomColors[0].getValue());
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			glMaterialfv(GL_FRONT, GL_EMISSION,
				SbVec4f(highlightColor[0], highlightColor[1],
						highlightColor[2], 1.0).getValue());
		}
		else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
			glColor3fv(highlightColor.getValue());
			ATOMLOOP_START(theAtomIndex)
				if (!doHydrogens &&
					chemData->getAtomicNumber(theAtom) == 1) {
					continue;
				}
				sphereCoords = chemData->getAtomCoordinates(theAtom);
// --> EGB && SGB original MOIV bug , radscale *= ERROR
//			rad *= radScale;
// <-- EGB && SGB original MOIV bug , radscale *= ERROR
				theScale.setValue(rad, rad, rad);
				theMatrix.setTransform(sphereCoords, theRotation, theScale);
				glPushMatrix();
				glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
				glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
				glPopMatrix();
			ATOMLOOP_END
		}
		else {
			ATOMLOOP_START(theAtomIndex)
				if (!doHydrogens &&
					chemData->getAtomicNumber(theAtom) == 1) {
					continue;
				}
				sphereCoords = chemData->getAtomCoordinates(theAtom);
// --> EGB && SGB original MOIV bug , radscale *= ERROR
//			rad *= radScale;
// <-- EGB && SGB original MOIV bug , radscale *= ERROR
				theScale.setValue(rad, rad, rad);
				theMatrix.setTransform(sphereCoords, theRotation, theScale);
				glPushMatrix();
				glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
				glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
				glPopMatrix();
			ATOMLOOP_END
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				glMaterialfv(GL_FRONT, GL_EMISSION,
					SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
			}
			else if (highlightStyle ==
					 ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}
		}
	}
	else {
		// Not using hemispheres
		theRotation.setValue(0.0, 0.0, 0.0, 1.0);
		rad = atomRadii[0];
// --> EGB && SGB original MOIV bug , radscale *= ERROR
		rad *= radScale;
// <-- EGB && SGB original MOIV bug , radscale *= ERROR
		glColor3fv(atomColors[0].getValue());
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			glMaterialfv(GL_FRONT, GL_EMISSION,
				SbVec4f(highlightColor[0], highlightColor[1],
						highlightColor[2], 1.0).getValue());
		}
		else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
			glColor3fv(highlightColor.getValue());
			ATOMLOOP_START(theAtomIndex)
				if (!doHydrogens &&
					chemData->getAtomicNumber(theAtom) == 1) {
					continue;
				}
				sphereCoords = chemData->getAtomCoordinates(theAtom);
// --> EGB && SGB original MOIV bug , radscale *= ERROR
//			rad *= radScale;
// <-- EGB && SGB original MOIV bug , radscale *= ERROR
				theScale.setValue(rad, rad, rad);
				theMatrix.setTransform(sphereCoords, theRotation, theScale);
				glPushMatrix();
				glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
				glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
				glPopMatrix();
			ATOMLOOP_END
		}
		else {
			ATOMLOOP_START(theAtomIndex)
				if (!doHydrogens &&
					chemData->getAtomicNumber(theAtom) == 1) {
					continue;
				}
				sphereCoords = chemData->getAtomCoordinates(theAtom);
// --> EGB && SGB original MOIV bug , radscale *= ERROR
//			rad *= radScale;
// <-- EGB && SGB original MOIV bug , radscale *= ERROR
				theScale.setValue(rad, rad, rad);
				theMatrix.setTransform(sphereCoords, theRotation, theScale);
				glPushMatrix();
				glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
				glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
				glPopMatrix();
			ATOMLOOP_END
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				glMaterialfv(GL_FRONT, GL_EMISSION,
					SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
			}
			else if (highlightStyle ==
					 ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}
		}
	}
	glDisable(GL_COLOR_MATERIAL);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights the molecule using the unit sphere display list.
//    Invoked by the GLRender method.
//  
// Use: private

void
ChemDisplay::
highlightSphereROCA
	(SoGLRenderAction* action, const int displayStyle,
	const int highlightStyle, const SbColor &highlightColor,
	const SbBool doHydrogens, const SbBool doHemi,
	const float radScale, const SbColor *atomColors, const float *atomRadii,
	const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
//  
////////////////////////////////////////////////////////////////////////
{
	float		rad;
	SbVec3f 	theScale;
	SbRotation	theRotation;
	SbMatrix	theMatrix;
	SbVec3f		sphereCoords;

	// Check to see if there is anything to render.
	if (theAtomIndex.getNum() == 0) return;

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

// --> compatible modes
	DOHLOD(ROCA)
// <-- compatible modes

	if (doHemi) {
		theRotation = currentRotation;
		rad = atomRadii[0];
// --> EGB && SGB original MOIV bug , radscale *= ERROR
		rad *= radScale;
// <-- EGB && SGB original MOIV bug , radscale *= ERROR
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			glMaterialfv(GL_FRONT, GL_EMISSION,
				SbVec4f(highlightColor[0], highlightColor[1],
						highlightColor[2], 1.0).getValue());
		}
		else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
			glColor3fv(highlightColor.getValue());
			ATOMLOOP_START(theAtomIndex)
				if (!doHydrogens &&
					chemData->getAtomicNumber(theAtom) == 1) {
					continue;
				}
				sphereCoords = chemData->getAtomCoordinates(theAtom);
// --> EGB && SGB original MOIV bug , radscale *= ERROR
//			rad *= radScale;
// <-- EGB && SGB original MOIV bug , radscale *= ERROR
				theScale.setValue(rad, rad, rad);
				theMatrix.setTransform(sphereCoords, theRotation, theScale);
				glPushMatrix();
				glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
				glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
				glPopMatrix();
			ATOMLOOP_END
		}
		else {
			ATOMLOOP_START(theAtomIndex)
				if (!doHydrogens &&
					chemData->getAtomicNumber(theAtom) == 1) {
					continue;
				}
				sphereCoords = chemData->getAtomCoordinates(theAtom);
// --> EGB && SGB original MOIV bug , radscale *= ERROR
//			rad *= radScale;
// <-- EGB && SGB original MOIV bug , radscale *= ERROR
				theScale.setValue(rad, rad, rad);
				theMatrix.setTransform(sphereCoords, theRotation, theScale);
				glColor3fv(atomColors[theAtom].getValue());
				glPushMatrix();
				glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
				glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
				glPopMatrix();
			ATOMLOOP_END
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				glMaterialfv(GL_FRONT, GL_EMISSION,
					SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
			}
			else if (highlightStyle ==
					 ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}
		}
	}
	else {
		// Not using hemispheres
		theRotation.setValue(0.0, 0.0, 0.0, 1.0);
		rad = atomRadii[0];
// --> EGB && SGB original MOIV bug , radscale *= ERROR
		rad *= radScale;
// <-- EGB && SGB original MOIV bug , radscale *= ERROR
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			glMaterialfv(GL_FRONT, GL_EMISSION,
				SbVec4f(highlightColor[0], highlightColor[1],
						highlightColor[2], 1.0).getValue());
		}
		else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
			glColor3fv(highlightColor.getValue());
			ATOMLOOP_START(theAtomIndex)
				if (!doHydrogens &&
					chemData->getAtomicNumber(theAtom) == 1) {
					continue;
				}
				sphereCoords = chemData->getAtomCoordinates(theAtom);
// --> EGB && SGB original MOIV bug , radscale *= ERROR
//			rad *= radScale;
// <-- EGB && SGB original MOIV bug , radscale *= ERROR
				theScale.setValue(rad, rad, rad);
				theMatrix.setTransform(sphereCoords, theRotation, theScale);
				glPushMatrix();
				glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
				glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
				glPopMatrix();
			ATOMLOOP_END
		}
		else {
			ATOMLOOP_START(theAtomIndex)
				if (!doHydrogens &&
					chemData->getAtomicNumber(theAtom) == 1) {
					continue;
				}
				sphereCoords = chemData->getAtomCoordinates(theAtom);
// --> EGB && SGB original MOIV bug , radscale *= ERROR
//			rad *= radScale;
// <-- EGB && SGB original MOIV bug , radscale *= ERROR
				theScale.setValue(rad, rad, rad);
				theMatrix.setTransform(sphereCoords, theRotation, theScale);
				glColor3fv(atomColors[theAtom].getValue());
				glPushMatrix();
				glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
				glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
				glPopMatrix();
			ATOMLOOP_END
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				glMaterialfv(GL_FRONT, GL_EMISSION,
					SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
			}
			else if (highlightStyle ==
					 ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}
		}
	}
	glDisable(GL_COLOR_MATERIAL);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights the molecule using the unit sphere display list.
//    Invoked by the GLRender method.
//  
// Use: private

void
ChemDisplay::
highlightSphereROCI
	(SoGLRenderAction* action, const int displayStyle,
	const int highlightStyle, const SbColor &highlightColor,
	const SbBool doHydrogens, const SbBool doHemi,
	const float radScale, const SbColor *atomColors, const float *atomRadii,
	const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
//  
////////////////////////////////////////////////////////////////////////
{
	float		rad;
	SbVec3f 	theScale;
	SbRotation	theRotation;
	SbMatrix	theMatrix;
	SbVec3f		sphereCoords;
	int32_t		localAtomIndex;

	// Check to see if there is anything to render.
	if (theAtomIndex.getNum() == 0) return;

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

// --> compatible modes
	DOHLOD(ROCI)
// <-- compatible modes

	if (doHemi) {
		theRotation = currentRotation;
		rad = atomRadii[0];
// --> EGB && SGB original MOIV bug , radscale *= ERROR
		rad *= radScale;
// <-- EGB && SGB original MOIV bug , radscale *= ERROR
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			glMaterialfv(GL_FRONT, GL_EMISSION,
				SbVec4f(highlightColor[0], highlightColor[1],
						highlightColor[2], 1.0).getValue());
		}
		else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
			glColor3fv(highlightColor.getValue());
			ATOMLOOP_START(theAtomIndex)
				if (!doHydrogens &&
					chemData->getAtomicNumber(theAtom) == 1) {
					continue;
				}
				sphereCoords = chemData->getAtomCoordinates(theAtom);
// --> EGB && SGB original MOIV bug , radscale *= ERROR
//			rad *= radScale;
// <-- EGB && SGB original MOIV bug , radscale *= ERROR
				theScale.setValue(rad, rad, rad);
				theMatrix.setTransform(sphereCoords, theRotation, theScale);
				glPushMatrix();
				glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
				glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
				glPopMatrix();
			ATOMLOOP_END
		}
		else {
			ATOMLOOP_START(theAtomIndex)
				if (!doHydrogens &&
					chemData->getAtomicNumber(theAtom) == 1) {
					continue;
				}
				sphereCoords = chemData->getAtomCoordinates(theAtom);
				localAtomIndex = chemData->getAtomIndex(theAtom);
// --> EGB && SGB original MOIV bug , radscale *= ERROR
//			rad *= radScale;
// <-- EGB && SGB original MOIV bug , radscale *= ERROR
				theScale.setValue(rad, rad, rad);
				theMatrix.setTransform(sphereCoords, theRotation, theScale);
				glColor3fv(atomColors[localAtomIndex].getValue());
				glPushMatrix();
				glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
				glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
				glPopMatrix();
			ATOMLOOP_END
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				glMaterialfv(GL_FRONT, GL_EMISSION,
					SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
			}
			else if (highlightStyle ==
					 ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}
		}
	}
	else {
		// Not using hemispheres
		theRotation.setValue(0.0, 0.0, 0.0, 1.0);
		rad = atomRadii[0];
// --> EGB && SGB original MOIV bug , radscale *= ERROR
		rad *= radScale;
// <-- EGB && SGB original MOIV bug , radscale *= ERROR
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			glMaterialfv(GL_FRONT, GL_EMISSION,
				SbVec4f(highlightColor[0], highlightColor[1],
						highlightColor[2], 1.0).getValue());
		}
		else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
			glColor3fv(highlightColor.getValue());
			ATOMLOOP_START(theAtomIndex)
				if (!doHydrogens &&
					chemData->getAtomicNumber(theAtom) == 1) {
					continue;
				}
				sphereCoords = chemData->getAtomCoordinates(theAtom);
// --> EGB && SGB original MOIV bug , radscale *= ERROR
//			rad *= radScale;
// <-- EGB && SGB original MOIV bug , radscale *= ERROR
				theScale.setValue(rad, rad, rad);
				theMatrix.setTransform(sphereCoords, theRotation, theScale);
				glPushMatrix();
				glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
				glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
				glPopMatrix();
			ATOMLOOP_END
		}
		else {
			ATOMLOOP_START(theAtomIndex)
				if (!doHydrogens &&
					chemData->getAtomicNumber(theAtom) == 1) {
					continue;
				}
				sphereCoords = chemData->getAtomCoordinates(theAtom);
				localAtomIndex = chemData->getAtomIndex(theAtom);
// --> EGB && SGB original MOIV bug , radscale *= ERROR
//			rad *= radScale;
// <-- EGB && SGB original MOIV bug , radscale *= ERROR
				theScale.setValue(rad, rad, rad);
				theMatrix.setTransform(sphereCoords, theRotation, theScale);
				glColor3fv(atomColors[localAtomIndex].getValue());
				glPushMatrix();
				glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
				glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
				glPopMatrix();
			ATOMLOOP_END
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				glMaterialfv(GL_FRONT, GL_EMISSION,
					SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
			}
			else if (highlightStyle ==
					 ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}
		}
	}
	glDisable(GL_COLOR_MATERIAL);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights the molecule using the unit sphere display list.
//    Invoked by the GLRender method.
//  
// Use: private

void
ChemDisplay::
highlightSphereRACO
	(SoGLRenderAction* action, const int displayStyle,
	const int highlightStyle, const SbColor &highlightColor,
	const SbBool doHydrogens, const SbBool doHemi,
	const float radScale, const SbColor *atomColors, const float *atomRadii,
	const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
//  
////////////////////////////////////////////////////////////////////////
{
	float		rad;
	SbVec3f 	theScale;
	SbRotation	theRotation;
	SbMatrix	theMatrix;
	SbVec3f		sphereCoords;

	// Check to see if there is anything to render.
	if (theAtomIndex.getNum() == 0) return;

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

// --> compatible modes
	DOHLOD(RACO)
// <-- compatible modes

	if (doHemi) {
		theRotation = currentRotation;
		glColor3fv(atomColors[0].getValue());
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			glMaterialfv(GL_FRONT, GL_EMISSION,
				SbVec4f(highlightColor[0], highlightColor[1],
						highlightColor[2], 1.0).getValue());
		}
		else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
			glColor3fv(highlightColor.getValue());
			ATOMLOOP_START(theAtomIndex)
				if (!doHydrogens &&
					chemData->getAtomicNumber(theAtom) == 1) {
					continue;
				}
				sphereCoords = chemData->getAtomCoordinates(theAtom);
				rad = atomRadii[theAtom];
				rad *= radScale;
				theScale.setValue(rad, rad, rad);
				theMatrix.setTransform(sphereCoords, theRotation, theScale);
				glPushMatrix();
				glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
				glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
				glPopMatrix();
			ATOMLOOP_END
		}
		else {
			ATOMLOOP_START(theAtomIndex)
				if (!doHydrogens &&
					chemData->getAtomicNumber(theAtom) == 1) {
					continue;
				}
				sphereCoords = chemData->getAtomCoordinates(theAtom);
				rad = atomRadii[theAtom];
				rad *= radScale;
				theScale.setValue(rad, rad, rad);
				theMatrix.setTransform(sphereCoords, theRotation, theScale);
				glPushMatrix();
				glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
				glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
				glPopMatrix();
			ATOMLOOP_END
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				glMaterialfv(GL_FRONT, GL_EMISSION,
					SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
			}
			else if (highlightStyle ==
					 ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}
		}
	}
	else {
		// Not using hemispheres
		theRotation.setValue(0.0, 0.0, 0.0, 1.0);
		glColor3fv(atomColors[0].getValue());
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			glMaterialfv(GL_FRONT, GL_EMISSION,
				SbVec4f(highlightColor[0], highlightColor[1],
						highlightColor[2], 1.0).getValue());
		}
		else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
			glColor3fv(highlightColor.getValue());
			ATOMLOOP_START(theAtomIndex)
				if (!doHydrogens &&
					chemData->getAtomicNumber(theAtom) == 1) {
					continue;
				}
				sphereCoords = chemData->getAtomCoordinates(theAtom);
				rad = atomRadii[theAtom];
				rad *= radScale;
				theScale.setValue(rad, rad, rad);
				theMatrix.setTransform(sphereCoords, theRotation, theScale);
				glPushMatrix();
				glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
				glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
				glPopMatrix();
			ATOMLOOP_END
		}
		else {
			ATOMLOOP_START(theAtomIndex)
				if (!doHydrogens &&
					chemData->getAtomicNumber(theAtom) == 1) {
					continue;
				}
				sphereCoords = chemData->getAtomCoordinates(theAtom);
				rad = atomRadii[theAtom];
				rad *= radScale;
				theScale.setValue(rad, rad, rad);
				theMatrix.setTransform(sphereCoords, theRotation, theScale);
				glPushMatrix();
				glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
				glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
				glPopMatrix();
			ATOMLOOP_END
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				glMaterialfv(GL_FRONT, GL_EMISSION,
					SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
			}
			else if (highlightStyle ==
					 ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}
		}
	}
	glDisable(GL_COLOR_MATERIAL);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights the molecule using the unit sphere display list.
//    Invoked by the GLRender method.
//  
// Use: private

void
ChemDisplay::
highlightSphereRACA
	(SoGLRenderAction* action, const int displayStyle,
	const int highlightStyle, const SbColor &highlightColor,
	const SbBool doHydrogens, const SbBool doHemi,
	const float radScale, const SbColor *atomColors, const float *atomRadii,
	const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
//  
////////////////////////////////////////////////////////////////////////
{
	float		rad;
	SbVec3f 	theScale;
	SbRotation	theRotation;
	SbMatrix	theMatrix;
	SbVec3f		sphereCoords;

	// Check to see if there is anything to render.
	if (theAtomIndex.getNum() == 0) return;

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

// --> compatible modes
	DOHLOD(RACA)
// <-- compatible modes

	if (doHemi) {
		theRotation = currentRotation;
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			glMaterialfv(GL_FRONT, GL_EMISSION,
				SbVec4f(highlightColor[0], highlightColor[1],
						highlightColor[2], 1.0).getValue());
		}
		else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
			glColor3fv(highlightColor.getValue());
			ATOMLOOP_START(theAtomIndex)
				if (!doHydrogens &&
					chemData->getAtomicNumber(theAtom) == 1) {
					continue;
				}
				sphereCoords = chemData->getAtomCoordinates(theAtom);
				rad = atomRadii[theAtom];
				rad *= radScale;
				theScale.setValue(rad, rad, rad);
				theMatrix.setTransform(sphereCoords, theRotation, theScale);
				glPushMatrix();
				glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
				glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
				glPopMatrix();
			ATOMLOOP_END
		}
		else {
			ATOMLOOP_START(theAtomIndex)
				if (!doHydrogens &&
					chemData->getAtomicNumber(theAtom) == 1) {
					continue;
				}
				sphereCoords = chemData->getAtomCoordinates(theAtom);
				rad = atomRadii[theAtom];
				rad *= radScale;
				theScale.setValue(rad, rad, rad);
				theMatrix.setTransform(sphereCoords, theRotation, theScale);
				glColor3fv(atomColors[theAtom].getValue());
				glPushMatrix();
				glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
				glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
				glPopMatrix();
			ATOMLOOP_END
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				glMaterialfv(GL_FRONT, GL_EMISSION,
					SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
			}
			else if (highlightStyle ==
					 ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}
		}
	}
	else {
		// Not using hemispheres
		theRotation.setValue(0.0, 0.0, 0.0, 1.0);
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			glMaterialfv(GL_FRONT, GL_EMISSION,
				SbVec4f(highlightColor[0], highlightColor[1],
						highlightColor[2], 1.0).getValue());
		}
		else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
			glColor3fv(highlightColor.getValue());
			ATOMLOOP_START(theAtomIndex)
				if (!doHydrogens &&
					chemData->getAtomicNumber(theAtom) == 1) {
					continue;
				}
				sphereCoords = chemData->getAtomCoordinates(theAtom);
				rad = atomRadii[theAtom];
				rad *= radScale;
				theScale.setValue(rad, rad, rad);
				theMatrix.setTransform(sphereCoords, theRotation, theScale);
				glPushMatrix();
				glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
				glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
				glPopMatrix();
			ATOMLOOP_END
		}
		else {
			ATOMLOOP_START(theAtomIndex)
				if (!doHydrogens &&
					chemData->getAtomicNumber(theAtom) == 1) {
					continue;
				}
				sphereCoords = chemData->getAtomCoordinates(theAtom);
				rad = atomRadii[theAtom];
				rad *= radScale;
				theScale.setValue(rad, rad, rad);
				theMatrix.setTransform(sphereCoords, theRotation, theScale);
				glColor3fv(atomColors[theAtom].getValue());
				glPushMatrix();
				glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
				glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
				glPopMatrix();
			ATOMLOOP_END
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				glMaterialfv(GL_FRONT, GL_EMISSION,
					SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
			}
			else if (highlightStyle ==
					 ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}
		}
	}
	glDisable(GL_COLOR_MATERIAL);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights the molecule using the unit sphere display list.
//    Invoked by the GLRender method.
//  
// Use: private

void
ChemDisplay::
highlightSphereRACI
	(SoGLRenderAction* action, const int displayStyle,
	const int highlightStyle, const SbColor &highlightColor,
	const SbBool doHydrogens, const SbBool doHemi,
	const float radScale, const SbColor *atomColors, const float *atomRadii,
	const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
//  
////////////////////////////////////////////////////////////////////////
{
	float		rad;
	SbVec3f 	theScale;
	SbRotation	theRotation;
	SbMatrix	theMatrix;
	SbVec3f		sphereCoords;
	int32_t		localAtomIndex;

	// Check to see if there is anything to render.
	if (theAtomIndex.getNum() == 0) return;

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

// --> compatible modes
	DOHLOD(RACI)
// <-- compatible modes

	if (doHemi) {
		theRotation = currentRotation;
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			glMaterialfv(GL_FRONT, GL_EMISSION,
				SbVec4f(highlightColor[0], highlightColor[1],
						highlightColor[2], 1.0).getValue());
		}
		else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
			glColor3fv(highlightColor.getValue());
			ATOMLOOP_START(theAtomIndex)
				if (!doHydrogens &&
					chemData->getAtomicNumber(theAtom) == 1) {
					continue;
				}
				sphereCoords = chemData->getAtomCoordinates(theAtom);
				rad = atomRadii[theAtom];
				rad *= radScale;
				theScale.setValue(rad, rad, rad);
				theMatrix.setTransform(sphereCoords, theRotation, theScale);
				glPushMatrix();
				glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
				glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
				glPopMatrix();
			ATOMLOOP_END
		}
		else {
			ATOMLOOP_START(theAtomIndex)
				if (!doHydrogens &&
					chemData->getAtomicNumber(theAtom) == 1) {
					continue;
				}
				sphereCoords = chemData->getAtomCoordinates(theAtom);
				localAtomIndex = chemData->getAtomIndex(theAtom);
				rad = atomRadii[theAtom];
				rad *= radScale;
				theScale.setValue(rad, rad, rad);
				theMatrix.setTransform(sphereCoords, theRotation, theScale);
				glColor3fv(atomColors[localAtomIndex].getValue());
				glPushMatrix();
				glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
				glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
				glPopMatrix();
			ATOMLOOP_END
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				glMaterialfv(GL_FRONT, GL_EMISSION,
					SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
			}
			else if (highlightStyle ==
					 ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}
		}
	}
	else {
		// Not using hemispheres
		theRotation.setValue(0.0, 0.0, 0.0, 1.0);
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			glMaterialfv(GL_FRONT, GL_EMISSION,
				SbVec4f(highlightColor[0], highlightColor[1],
						highlightColor[2], 1.0).getValue());
		}
		else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
			glColor3fv(highlightColor.getValue());
			ATOMLOOP_START(theAtomIndex)
				if (!doHydrogens &&
					chemData->getAtomicNumber(theAtom) == 1) {
					continue;
				}
				sphereCoords = chemData->getAtomCoordinates(theAtom);
				rad = atomRadii[theAtom];
				rad *= radScale;
				theScale.setValue(rad, rad, rad);
				theMatrix.setTransform(sphereCoords, theRotation, theScale);
				glPushMatrix();
				glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
				glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
				glPopMatrix();
			ATOMLOOP_END
		}
		else {
			ATOMLOOP_START(theAtomIndex)
				if (!doHydrogens &&
					chemData->getAtomicNumber(theAtom) == 1) {
					continue;
				}
				sphereCoords = chemData->getAtomCoordinates(theAtom);
				localAtomIndex = chemData->getAtomIndex(theAtom);
				rad = atomRadii[theAtom];
				rad *= radScale;
				theScale.setValue(rad, rad, rad);
				theMatrix.setTransform(sphereCoords, theRotation, theScale);
				glColor3fv(atomColors[localAtomIndex].getValue());
				glPushMatrix();
				glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
				glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
				glPopMatrix();
			ATOMLOOP_END
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				glMaterialfv(GL_FRONT, GL_EMISSION,
					SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
			}
			else if (highlightStyle ==
					 ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}
		}
	}
	glDisable(GL_COLOR_MATERIAL);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights the molecule using the unit sphere display list.
//    Invoked by the GLRender method.
//  
// Use: private

void
ChemDisplay::
highlightSphereRICO
	(SoGLRenderAction* action, const int displayStyle,
	const int highlightStyle, const SbColor &highlightColor,
	const SbBool doHydrogens, const SbBool doHemi,
	const float radScale, const SbColor *atomColors, const float *atomRadii,
	const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
//  
////////////////////////////////////////////////////////////////////////
{
	float		rad;
	SbVec3f 	theScale;
	SbRotation	theRotation;
	SbMatrix	theMatrix;
	SbVec3f		sphereCoords;
	int32_t		localAtomIndex;

	// Check to see if there is anything to render.
	if (theAtomIndex.getNum() == 0) return;

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

// --> compatible modes
	DOHLOD(RICO)
// <-- compatible modes

	if (doHemi) {
		theRotation = currentRotation;
		glColor3fv(atomColors[0].getValue());
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			glMaterialfv(GL_FRONT, GL_EMISSION,
				SbVec4f(highlightColor[0], highlightColor[1],
						highlightColor[2], 1.0).getValue());
		}
		else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
			glColor3fv(highlightColor.getValue());
			ATOMLOOP_START(theAtomIndex)
				if (!doHydrogens &&
					chemData->getAtomicNumber(theAtom) == 1) {
					continue;
				}
				sphereCoords = chemData->getAtomCoordinates(theAtom);
				localAtomIndex = chemData->getAtomIndex(theAtom);
				rad = atomRadii[localAtomIndex];
				rad *= radScale;
				theScale.setValue(rad, rad, rad);
				theMatrix.setTransform(sphereCoords, theRotation, theScale);
				glPushMatrix();
				glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
				glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
				glPopMatrix();
			ATOMLOOP_END
		}
		else {
			ATOMLOOP_START(theAtomIndex)
				if (!doHydrogens &&
					chemData->getAtomicNumber(theAtom) == 1) {
					continue;
				}
				sphereCoords = chemData->getAtomCoordinates(theAtom);
				localAtomIndex = chemData->getAtomIndex(theAtom);
				rad = atomRadii[localAtomIndex];
				rad *= radScale;
				theScale.setValue(rad, rad, rad);
				theMatrix.setTransform(sphereCoords, theRotation, theScale);
				glPushMatrix();
				glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
				glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
				glPopMatrix();
			ATOMLOOP_END
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				glMaterialfv(GL_FRONT, GL_EMISSION,
					SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
			}
			else if (highlightStyle ==
					 ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}
		}
	}
	else {
		// Not using hemispheres
		theRotation.setValue(0.0, 0.0, 0.0, 1.0);
		glColor3fv(atomColors[0].getValue());
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			glMaterialfv(GL_FRONT, GL_EMISSION,
				SbVec4f(highlightColor[0], highlightColor[1],
						highlightColor[2], 1.0).getValue());
		}
		else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
			glColor3fv(highlightColor.getValue());
			ATOMLOOP_START(theAtomIndex)
				if (!doHydrogens &&
					chemData->getAtomicNumber(theAtom) == 1) {
					continue;
				}
				sphereCoords = chemData->getAtomCoordinates(theAtom);
				localAtomIndex = chemData->getAtomIndex(theAtom);
				rad = atomRadii[localAtomIndex];
				rad *= radScale;
				theScale.setValue(rad, rad, rad);
				theMatrix.setTransform(sphereCoords, theRotation, theScale);
				glPushMatrix();
				glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
				glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
				glPopMatrix();
			ATOMLOOP_END
		}
		else {
			ATOMLOOP_START(theAtomIndex)
				if (!doHydrogens &&
					chemData->getAtomicNumber(theAtom) == 1) {
					continue;
				}
				sphereCoords = chemData->getAtomCoordinates(theAtom);
				localAtomIndex = chemData->getAtomIndex(theAtom);
				rad = atomRadii[localAtomIndex];
				rad *= radScale;
				theScale.setValue(rad, rad, rad);
				theMatrix.setTransform(sphereCoords, theRotation, theScale);
				glPushMatrix();
				glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
				glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
				glPopMatrix();
			ATOMLOOP_END
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				glMaterialfv(GL_FRONT, GL_EMISSION,
					SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
			}
			else if (highlightStyle ==
					 ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}
		}
	}
	glDisable(GL_COLOR_MATERIAL);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights the molecule using the unit sphere display list.
//    Invoked by the GLRender method.
//  
// Use: private

void
ChemDisplay::
highlightSphereRICA
	(SoGLRenderAction* action, const int displayStyle,
	const int highlightStyle, const SbColor &highlightColor,
	const SbBool doHydrogens, const SbBool doHemi,
	const float radScale, const SbColor *atomColors, const float *atomRadii,
	const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
//  
////////////////////////////////////////////////////////////////////////
{
	float		rad;
	SbVec3f 	theScale;
	SbRotation	theRotation;
	SbMatrix	theMatrix;
	SbVec3f		sphereCoords;
	int32_t		localAtomIndex;

	// Check to see if there is anything to render.
	if (theAtomIndex.getNum() == 0) return;

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

// --> Compatible modes
	DOHLOD(RICA);
// <-- Compatible modes

	if (doHemi) {
		theRotation = currentRotation;
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			glMaterialfv(GL_FRONT, GL_EMISSION,
				SbVec4f(highlightColor[0], highlightColor[1],
						highlightColor[2], 1.0).getValue());
		}
		else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
			glColor3fv(highlightColor.getValue());
			ATOMLOOP_START(theAtomIndex)
				if (!doHydrogens &&
					chemData->getAtomicNumber(theAtom) == 1) {
					continue;
				}
				sphereCoords = chemData->getAtomCoordinates(theAtom);
				localAtomIndex = chemData->getAtomIndex(theAtom);
				rad = atomRadii[localAtomIndex];
				rad *= radScale;
				theScale.setValue(rad, rad, rad);
				theMatrix.setTransform(sphereCoords, theRotation, theScale);
				glPushMatrix();
				glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
				glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
				glPopMatrix();
			ATOMLOOP_END
		}
		else {
			ATOMLOOP_START(theAtomIndex)
				if (!doHydrogens &&
					chemData->getAtomicNumber(theAtom) == 1) {
					continue;
				}
				sphereCoords = chemData->getAtomCoordinates(theAtom);
				localAtomIndex = chemData->getAtomIndex(theAtom);
				rad = atomRadii[localAtomIndex];
				rad *= radScale;
				theScale.setValue(rad, rad, rad);
				theMatrix.setTransform(sphereCoords, theRotation, theScale);
				glColor3fv(atomColors[theAtom].getValue());
				glPushMatrix();
				glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
				glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
				glPopMatrix();
			ATOMLOOP_END
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				glMaterialfv(GL_FRONT, GL_EMISSION,
					SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
			}
			else if (highlightStyle ==
					 ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}
		}
	}
	else {
		// Not using hemispheres
		theRotation.setValue(0.0, 0.0, 0.0, 1.0);
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			glMaterialfv(GL_FRONT, GL_EMISSION,
				SbVec4f(highlightColor[0], highlightColor[1],
						highlightColor[2], 1.0).getValue());
		}
		else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
			glColor3fv(highlightColor.getValue());
			ATOMLOOP_START(theAtomIndex)
				if (!doHydrogens &&
					chemData->getAtomicNumber(theAtom) == 1) {
					continue;
				}
				sphereCoords = chemData->getAtomCoordinates(theAtom);
				localAtomIndex = chemData->getAtomIndex(theAtom);
				rad = atomRadii[localAtomIndex];
				rad *= radScale;
				theScale.setValue(rad, rad, rad);
				theMatrix.setTransform(sphereCoords, theRotation, theScale);
				glPushMatrix();
				glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
				glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
				glPopMatrix();
			ATOMLOOP_END
		}
		else {
			ATOMLOOP_START(theAtomIndex)
				if (!doHydrogens &&
					chemData->getAtomicNumber(theAtom) == 1) {
					continue;
				}
				sphereCoords = chemData->getAtomCoordinates(theAtom);
				localAtomIndex = chemData->getAtomIndex(theAtom);
				rad = atomRadii[localAtomIndex];
				rad *= radScale;
				theScale.setValue(rad, rad, rad);
				theMatrix.setTransform(sphereCoords, theRotation, theScale);
				glColor3fv(atomColors[theAtom].getValue());
				glPushMatrix();
				glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
				glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
				glPopMatrix();
			ATOMLOOP_END
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				glMaterialfv(GL_FRONT, GL_EMISSION,
					SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
			}
			else if (highlightStyle ==
					 ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}
		}
	}
	glDisable(GL_COLOR_MATERIAL);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Highlights the molecule using the unit sphere display list.
//    Invoked by the GLRender method.
//  
// Use: private

void
ChemDisplay::
highlightSphereRICI
	(SoGLRenderAction* action, const int displayStyle,
	const int highlightStyle, const SbColor &highlightColor,
	const SbBool doHydrogens, const SbBool doHemi,
	const float radScale, const SbColor *atomColors, const float *atomRadii,
	const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
//  
////////////////////////////////////////////////////////////////////////
{
	float		rad;
	SbVec3f 	theScale;
	SbRotation	theRotation;
	SbMatrix	theMatrix;
	SbVec3f		sphereCoords;
	int32_t		localAtomIndex;

	// Check to see if there is anything to render.
	if (theAtomIndex.getNum() == 0) return;

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

// --> Compatible modes
	DOHLOD(RICI);
// <-- Compatible modes

	if (doHemi) {
		theRotation = currentRotation;
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			glMaterialfv(GL_FRONT, GL_EMISSION,
				SbVec4f(highlightColor[0], highlightColor[1],
						highlightColor[2], 1.0).getValue());
		}
		else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
			glColor3fv(highlightColor.getValue());
			ATOMLOOP_START(theAtomIndex)
				if (!doHydrogens &&
					chemData->getAtomicNumber(theAtom) == 1) {
					continue;
				}
				sphereCoords = chemData->getAtomCoordinates(theAtom);
				localAtomIndex = chemData->getAtomIndex(theAtom);
				rad = atomRadii[localAtomIndex];
				rad *= radScale;
				theScale.setValue(rad, rad, rad);
				theMatrix.setTransform(sphereCoords, theRotation, theScale);
				glPushMatrix();
				glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
				glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
				glPopMatrix();
			ATOMLOOP_END
		}
		else {
			ATOMLOOP_START(theAtomIndex)
				if (!doHydrogens &&
					chemData->getAtomicNumber(theAtom) == 1) {
					continue;
				}
				sphereCoords = chemData->getAtomCoordinates(theAtom);
				localAtomIndex = chemData->getAtomIndex(theAtom);
				rad = atomRadii[localAtomIndex];
				rad *= radScale;
				theScale.setValue(rad, rad, rad);
				theMatrix.setTransform(sphereCoords, theRotation, theScale);
				glColor3fv(atomColors[localAtomIndex].getValue());
				glPushMatrix();
				glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
				glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
				glPopMatrix();
			ATOMLOOP_END
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				glMaterialfv(GL_FRONT, GL_EMISSION,
					SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
			}
			else if (highlightStyle ==
					 ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}
		}
	}
	else {
		// Not using hemispheres
		theRotation.setValue(0.0, 0.0, 0.0, 1.0);
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
			glMaterialfv(GL_FRONT, GL_EMISSION,
				SbVec4f(highlightColor[0], highlightColor[1],
						highlightColor[2], 1.0).getValue());
		}
		else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
			glColor3fv(highlightColor.getValue());
			ATOMLOOP_START(theAtomIndex)
				if (!doHydrogens &&
					chemData->getAtomicNumber(theAtom) == 1) {
					continue;
				}
				sphereCoords = chemData->getAtomCoordinates(theAtom);
				localAtomIndex = chemData->getAtomIndex(theAtom);
				rad = atomRadii[localAtomIndex];
				rad *= radScale;
				theScale.setValue(rad, rad, rad);
				theMatrix.setTransform(sphereCoords, theRotation, theScale);
				glPushMatrix();
				glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
				glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
				glPopMatrix();
			ATOMLOOP_END
		}
		else {
			ATOMLOOP_START(theAtomIndex)
				if (!doHydrogens &&
					chemData->getAtomicNumber(theAtom) == 1) {
					continue;
				}
				sphereCoords = chemData->getAtomCoordinates(theAtom);
				localAtomIndex = chemData->getAtomIndex(theAtom);
				rad = atomRadii[localAtomIndex];
				rad *= radScale;
				theScale.setValue(rad, rad, rad);
				theMatrix.setTransform(sphereCoords, theRotation, theScale);
				glColor3fv(atomColors[localAtomIndex].getValue());
				glPushMatrix();
				glMultMatrixf((float *)theMatrix.getValue());
#ifdef DISPLAYLIST
				glCallList(unitSphereList);
#else
// --> EGB && SGB
//			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures);
			unitSphere->generateDisplay(doHemi, lastSphereDoNormals,lastSphereDoTextures,
				displayStyle,action);
// <-- EGB && SGB
#endif
				glPopMatrix();
			ATOMLOOP_END
			if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
				glMaterialfv(GL_FRONT, GL_EMISSION,
					SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
			}
			else if (highlightStyle ==
					 ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}
		}
	}
	glDisable(GL_COLOR_MATERIAL);
}

// Computes S and T texture coordinates from point on surface
#define COMPUTE_S_T(point, s, t)											  \
	s = atan2f(point[0], point[2]) * .159 + .5; 							  \
	t = atan2f(point[1],													  \
		   sqrtf(point[0]*point[0] + point[2]*point[2])) * .318 + .5

;
// Adjusts S texture coordinate in unstable areas
#define ADJUST_S(s, vec)													  \
	if (s < .001 && (vec[0] < 0.0 && vec[2] > 0.0)) 						  \
		s = 1.0;															  \
	else if (s > .999 && (vec[0] < 0.0 && vec[2] < 0.0))					  \
		s = 0.0 	

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Implements rayPicking.
//
// Use: protected

void
ChemDisplay::pickAtomsAsSpheres(SoRayPickAction *action)
//
////////////////////////////////////////////////////////////////////////
{
	ChemRadii *chemRadii = ChemRadiiElement::get(action->getState());

	switch (chemRadii->atomRadiiBinding.getValue()) {
	  case ChemRadii::RADII_OVERALL:
		pickSphereRO(action);
		break;
	  case ChemRadii::RADII_PER_ATOM:
		pickSphereRA(action);
		break;
	  case ChemRadii::RADII_PER_ATOM_INDEXED:
		pickSphereRI(action);
		break;
	  default:
		break;
	}
}

//
// The following preprocessor-generated functions handle the various
// ways to pick the spheres.
// WithinRadius ... within a user-specified radius
// RO ... RadiusOverall
// RA ... RadiusPerAtom
// RI ... RadiusPerAtomIndexed
//
//

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Picks a sphere.
//    Invoked by the rayPick method.
//  
// Use: private

void
ChemDisplay::
pickSphereRO
	(SoRayPickAction* action)
//  
////////////////////////////////////////////////////////////////////////
{
	float			rad;
	float			radScale;
	SbSphere		sph;
	SbVec3f			enterPoint, exitPoint, normal;
	SbVec4f			texCoord(0.0, 0.0, 0.0, 1.0);
	SoPickedPoint	*pp;
	ChemDetail		*detail;
	SbVec3f			sphereCoords;

	SoState *state = action->getState();
	ChemBaseData *chemData = ChemBaseDataElement::get(state);
	ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);
	ChemRadii *chemRadii = ChemRadiiElement::get(state);
	ChemColor *chemColor = ChemColorElement::get(state);

	int displayStyle = cdp->displayStyle.getValue();
	SbBool doHydrogens = cdp->showHydrogens.getValue();

	// Scale the radii of the spheres by the atomRadiiScaleFactor and if we
	// are doing a BALLSTICK or BALLWIRE model, also scale by the
	// ballStickSphereScaleFactor.

	radScale = cdp->atomRadiiScaleFactor.getValue();
	if ((displayStyle == ChemDisplayParam::DISPLAY_BALLSTICK) ||
	(displayStyle == ChemDisplayParam::DISPLAY_BALLWIRE)) {
		radScale *= cdp->ballStickSphereScaleFactor.getValue();
	}

	// Compute the picking ray in our current object space
	computeObjectSpaceRay(action);

	rad = chemRadii->atomRadii[0];
	ATOMLOOP_START(atomIndex)
		if (!doHydrogens &&
			chemData->getAtomicNumber(theAtom) == 1) {
			continue;
		}
		sphereCoords = chemData->getAtomCoordinates(theAtom);
		rad *= radScale;
		sph.setValue(sphereCoords, rad);

		// Intersect with pick ray. If found, set up picked point(s)
		if (sph.intersect(action->getLine(), enterPoint, exitPoint)) {
			if (action->isBetweenPlanes(enterPoint) &&
				(pp = action->addIntersection(enterPoint)) != NULL) {

				normal = enterPoint - sphereCoords;
				normal.normalize();
				pp->setObjectNormal(normal);

				COMPUTE_S_T(enterPoint, texCoord[0], texCoord[1]);
				pp->setObjectTextureCoords(texCoord);

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
				  default:
					break;
				}

				detail = new ChemDetail();
				detail->setAtomBondIndex(theAtom, -1);
				pp->setDetail(detail, this);
			}

			if (action->isBetweenPlanes(exitPoint) &&
				(pp = action->addIntersection(exitPoint)) != NULL) {

				normal = exitPoint - sphereCoords;
				normal.normalize();
				pp->setObjectNormal(normal);

				COMPUTE_S_T(exitPoint, texCoord[0], texCoord[1]);
				texCoord[2] = texCoord[3] = 0.0;
				pp->setObjectTextureCoords(texCoord);

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
				  default:
					break;
				}

				detail = new ChemDetail();
				detail->setAtomBondIndex(theAtom, -1);
				pp->setDetail(detail, this);
			}
		}
	ATOMLOOP_END
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Picks a sphere.
//    Invoked by the rayPick method.
//  
// Use: private

void
ChemDisplay::
pickSphereRA
	(SoRayPickAction* action)
//  
////////////////////////////////////////////////////////////////////////
{
	float			rad;
	float			radScale;
	SbSphere		sph;
	SbVec3f			enterPoint, exitPoint, normal;
	SbVec4f			texCoord(0.0, 0.0, 0.0, 1.0);
	SoPickedPoint	*pp;
	ChemDetail		*detail;
	SbVec3f			sphereCoords;

	SoState *state = action->getState();
	ChemBaseData *chemData = ChemBaseDataElement::get(state);
	ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);
	ChemRadii *chemRadii = ChemRadiiElement::get(state);
	ChemColor *chemColor = ChemColorElement::get(state);

	int displayStyle = cdp->displayStyle.getValue();
	SbBool doHydrogens = cdp->showHydrogens.getValue();

	// Scale the radii of the spheres by the atomRadiiScaleFactor and if we
	// are doing a BALLSTICK or BALLWIRE model, also scale by the
	// ballStickSphereScaleFactor.

	radScale = cdp->atomRadiiScaleFactor.getValue();
	if ((displayStyle == ChemDisplayParam::DISPLAY_BALLSTICK) ||
	(displayStyle == ChemDisplayParam::DISPLAY_BALLWIRE)) {
		radScale *= cdp->ballStickSphereScaleFactor.getValue();
	}

	// Compute the picking ray in our current object space
	computeObjectSpaceRay(action);

	ATOMLOOP_START(atomIndex)
		if (!doHydrogens &&
			chemData->getAtomicNumber(theAtom) == 1) {
			continue;
		}
		sphereCoords = chemData->getAtomCoordinates(theAtom);
		rad = chemRadii->atomRadii[theAtom];
		rad *= radScale;
		sph.setValue(sphereCoords, rad);

		// Intersect with pick ray. If found, set up picked point(s)
		if (sph.intersect(action->getLine(), enterPoint, exitPoint)) {
			if (action->isBetweenPlanes(enterPoint) &&
				(pp = action->addIntersection(enterPoint)) != NULL) {

				normal = enterPoint - sphereCoords;
				normal.normalize();
				pp->setObjectNormal(normal);

				COMPUTE_S_T(enterPoint, texCoord[0], texCoord[1]);
				pp->setObjectTextureCoords(texCoord);

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
				  default:
					break;
				}

				detail = new ChemDetail();
				detail->setAtomBondIndex(theAtom, -1);
				pp->setDetail(detail, this);
			}

			if (action->isBetweenPlanes(exitPoint) &&
				(pp = action->addIntersection(exitPoint)) != NULL) {

				normal = exitPoint - sphereCoords;
				normal.normalize();
				pp->setObjectNormal(normal);

				COMPUTE_S_T(exitPoint, texCoord[0], texCoord[1]);
				texCoord[2] = texCoord[3] = 0.0;
				pp->setObjectTextureCoords(texCoord);

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
				  default:
					break;
				}

				detail = new ChemDetail();
				detail->setAtomBondIndex(theAtom, -1);
				pp->setDetail(detail, this);
			}
		}
	ATOMLOOP_END
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Picks a sphere.
//    Invoked by the rayPick method.
//  
// Use: private

void
ChemDisplay::
pickSphereRI
	(SoRayPickAction* action)
//  
////////////////////////////////////////////////////////////////////////
{
	float			rad;
	float			radScale;
	SbSphere		sph;
	SbVec3f			enterPoint, exitPoint, normal;
	SbVec4f			texCoord(0.0, 0.0, 0.0, 1.0);
	SoPickedPoint	*pp;
	ChemDetail		*detail;
	SbVec3f			sphereCoords;

	SoState *state = action->getState();
	ChemBaseData *chemData = ChemBaseDataElement::get(state);
	ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);
	ChemRadii *chemRadii = ChemRadiiElement::get(state);
	ChemColor *chemColor = ChemColorElement::get(state);

	int displayStyle = cdp->displayStyle.getValue();
	SbBool doHydrogens = cdp->showHydrogens.getValue();

	// Scale the radii of the spheres by the atomRadiiScaleFactor and if we
	// are doing a BALLSTICK or BALLWIRE model, also scale by the
	// ballStickSphereScaleFactor.

	radScale = cdp->atomRadiiScaleFactor.getValue();
	if ((displayStyle == ChemDisplayParam::DISPLAY_BALLSTICK) ||
	(displayStyle == ChemDisplayParam::DISPLAY_BALLWIRE)) {
		radScale *= cdp->ballStickSphereScaleFactor.getValue();
	}

	// Compute the picking ray in our current object space
	computeObjectSpaceRay(action);

	ATOMLOOP_START(atomIndex)
		if (!doHydrogens &&
			chemData->getAtomicNumber(theAtom) == 1) {
			continue;
		}
		sphereCoords = chemData->getAtomCoordinates(theAtom);
		rad = chemRadii->atomRadii[chemData->getAtomIndex(theAtom)];
		rad *= radScale;
		sph.setValue(sphereCoords, rad);

		// Intersect with pick ray. If found, set up picked point(s)
		if (sph.intersect(action->getLine(), enterPoint, exitPoint)) {
			if (action->isBetweenPlanes(enterPoint) &&
				(pp = action->addIntersection(enterPoint)) != NULL) {

				normal = enterPoint - sphereCoords;
				normal.normalize();
				pp->setObjectNormal(normal);

				COMPUTE_S_T(enterPoint, texCoord[0], texCoord[1]);
				pp->setObjectTextureCoords(texCoord);

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
				  default:
					break;
				}

				detail = new ChemDetail();
				detail->setAtomBondIndex(theAtom, -1);
				pp->setDetail(detail, this);
			}

			if (action->isBetweenPlanes(exitPoint) &&
				(pp = action->addIntersection(exitPoint)) != NULL) {

				normal = exitPoint - sphereCoords;
				normal.normalize();
				pp->setObjectNormal(normal);

				COMPUTE_S_T(exitPoint, texCoord[0], texCoord[1]);
				texCoord[2] = texCoord[3] = 0.0;
				pp->setObjectTextureCoords(texCoord);

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
				  default:
					break;
				}

				detail = new ChemDetail();
				detail->setAtomBondIndex(theAtom, -1);
				pp->setDetail(detail, this);
			}
		}
	ATOMLOOP_END
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Picks a sphere.
//    Invoked by the rayPick method.
//  
// Use: private

void
ChemDisplay::
pickAtomsWithinRadius
	(SoRayPickAction* action)
//  
////////////////////////////////////////////////////////////////////////
{
	float			rad;
	float			radScale;
	SbSphere		sph;
	SbVec3f			enterPoint, exitPoint, normal;
	SbVec4f			texCoord(0.0, 0.0, 0.0, 1.0);
	SoPickedPoint	*pp;
	ChemDetail		*detail;
	SbVec3f			sphereCoords;

	SoState *state = action->getState();
	ChemBaseData *chemData = ChemBaseDataElement::get(state);
	ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);
	//ChemRadii *chemRadii = ChemRadiiElement::get(state);
	ChemColor *chemColor = ChemColorElement::get(state);

	int displayStyle = cdp->displayStyle.getValue();
	SbBool doHydrogens = cdp->showHydrogens.getValue();

	// Scale the radii of the spheres by the atomRadiiScaleFactor and if we
	// are doing a BALLSTICK or BALLWIRE model, also scale by the
	// ballStickSphereScaleFactor.

	radScale = cdp->atomRadiiScaleFactor.getValue();
	if ((displayStyle == ChemDisplayParam::DISPLAY_BALLSTICK) ||
	(displayStyle == ChemDisplayParam::DISPLAY_BALLWIRE)) {
		radScale *= cdp->ballStickSphereScaleFactor.getValue();
	}

	// Compute the picking ray in our current object space
	computeObjectSpaceRay(action);

	rad = cdp->atomPickRadius.getValue();
	ATOMLOOP_START(atomIndex)
		if (!doHydrogens &&
			chemData->getAtomicNumber(theAtom) == 1) {
			continue;
		}
		sphereCoords = chemData->getAtomCoordinates(theAtom);
		sph.setValue(sphereCoords, rad);

		// Intersect with pick ray. If found, set up picked point(s)
		if (sph.intersect(action->getLine(), enterPoint, exitPoint)) {
			if (action->isBetweenPlanes(enterPoint) &&
				(pp = action->addIntersection(enterPoint)) != NULL) {

				normal = enterPoint - sphereCoords;
				normal.normalize();
				pp->setObjectNormal(normal);

				COMPUTE_S_T(enterPoint, texCoord[0], texCoord[1]);
				pp->setObjectTextureCoords(texCoord);

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
				  default:
					break;
				}

				detail = new ChemDetail();
				detail->setAtomBondIndex(theAtom, -1);
				pp->setDetail(detail, this);
			}

			if (action->isBetweenPlanes(exitPoint) &&
				(pp = action->addIntersection(exitPoint)) != NULL) {

				normal = exitPoint - sphereCoords;
				normal.normalize();
				pp->setObjectNormal(normal);

				COMPUTE_S_T(exitPoint, texCoord[0], texCoord[1]);
				texCoord[2] = texCoord[3] = 0.0;
				pp->setObjectTextureCoords(texCoord);

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
				  default:
					break;
				}

				detail = new ChemDetail();
				detail->setAtomBondIndex(theAtom, -1);
				pp->setDetail(detail, this);
			}
		}
	ATOMLOOP_END
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Generates triangles representing the molecule using spheres.
//	  Whereas the renderAtomsAsSpheres method can display atoms as
//	  billboarded hemispheres, the generateAtomsAsSpheres method generates
//	  triangles for the entire sphere.	Invoked by the generatePrimitives 
//	  method.
//
// Use: protected

void
ChemDisplay::generateAtomsAsSpheres(SoAction *action)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::generateAtomsAsSpheres\n");
#endif
	int32_t	i;
	float	rad;
	float	radScaleFactor;
	SbVec3f vec;
	SbVec3f	sphereCoords;
	int 	atomColorBinding;
	int 	atomRadiiBinding;
	float 	complexity;
	SbVec4f tex;
	SbBool	genTexCoords;
	SoPrimitiveVertex	pv;
	const SoTextureCoordinateElement	*tce;

	SoState *state = action->getState();

	ChemBaseData *chemData = ChemBaseDataElement::get(state);
	ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);
	ChemRadii *chemRadii = ChemRadiiElement::get(state);
	ChemColor *chemColor = ChemColorElement::get(state);
	ChemDetail *detail;

	SbBool doHydrogens = cdp->showHydrogens.getValue();

	// Compute depth based on complexity
	complexity = cdp->atomSphereComplexity.getValue();
	if (lastSphereComplexity != complexity) {
		if (unitSphere != NULL) delete unitSphere;
		unitSphere = new ChemUnitSphere(complexity);
		lastSphereComplexity = complexity;
	}

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
	if (! genTexCoords)
		tce = SoTextureCoordinateElement::getInstance(state);
	else {
		tex[2] = 0.0;
		tex[3] = 1.0;
	}

	atomColorBinding = chemColor->atomColorBinding.getValue();
	atomRadiiBinding = chemRadii->atomRadiiBinding.getValue();

	radScaleFactor = cdp->atomRadiiScaleFactor.getValue();
	if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
		(cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
		radScaleFactor *= cdp->ballStickSphereScaleFactor.getValue();
	}

	int32_t sphereVertCount = unitSphere->getNumberOfVertices();
	int32_t vend = sphereVertCount / 2;
	const SbVec3f *sphereVertices = unitSphere->getVertices();

	ATOMLOOP_START(atomIndex)
		if (!doHydrogens && 
			chemData->getAtomicNumber(theAtom) == 1) {
			continue;
		}
		detail = new ChemDetail;
		detail->setAtomBondIndex(theAtom, -1);
		pv.setDetail(detail);

		sphereCoords = chemData->getAtomCoordinates(theAtom);

		if (atomRadiiBinding == ChemRadii::RADII_OVERALL) {
			rad = chemRadii->atomRadii[0] * radScaleFactor;
		}
		else if (atomRadiiBinding == ChemRadii::RADII_PER_ATOM) {
			rad = chemRadii->atomRadii[theAtom] * radScaleFactor;
		}
		else if (atomRadiiBinding == ChemRadii::RADII_PER_ATOM_INDEXED) {
			rad = chemRadii->atomRadii[chemData->getAtomIndex(theAtom)] 
				* radScaleFactor;
		}

		if (atomColorBinding == ChemColor::ATOM_OVERALL) {
			pv.setMaterialIndex(0);
		}
		else if (atomColorBinding == ChemColor::ATOM_PER_ATOM) {
			pv.setMaterialIndex(theAtom);
		}
		else if (atomColorBinding == ChemColor::ATOM_PER_ATOM_INDEXED) {
			pv.setMaterialIndex(chemData->getAtomIndex(theAtom));
		}

		beginShape(action, TRIANGLE_STRIP);
		for (i = 0; i < vend; i++) {
			vec = sphereVertices[i];

			if (genTexCoords) {
				COMPUTE_S_T(vec, tex[0], tex[1]);
				ADJUST_S(tex[0], vec);
			}
			else
				tex = tce->get(vec * rad + sphereCoords, vec);
			pv.setPoint(vec * rad + sphereCoords);
			pv.setNormal(vec);
			pv.setTextureCoords(tex);
			shapeVertex(&pv);
		}  // end for (i = 0 ...
		endShape();

		beginShape(action, TRIANGLE_STRIP);
		for (i = vend; i < sphereVertCount; i++) {
			vec = sphereVertices[i];

			if (genTexCoords) {
				COMPUTE_S_T(vec, tex[0], tex[1]);
				ADJUST_S(tex[0], vec);
			}
			else
				tex = tce->get(vec * rad + sphereCoords, vec);
			pv.setPoint(vec * rad + sphereCoords);
			pv.setNormal(vec);
			pv.setTextureCoords(tex);
			shapeVertex(&pv);
		}  // end for (i = vend ...
		endShape();
	ATOMLOOP_END
}
#undef COMPUTE_S_T
#undef ADJUST_S

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  If using hemispheres, this determines the rotation necessary to
//	  keep the hemispheres facing forward.  It also determines the
//    line of sight which is the direction to translate labels so that
//    they remain visible.
//
// Use: private

void
ChemDisplay::calculateHemisphereRotation(SoState *state)
//
////////////////////////////////////////////////////////////////////////
{
	float angle;
	SbVec3f axis, normal;
	axis.setValue(0.0, 1.0, 0.0);
	normal.setValue(0.0, 0.0, 1.0);

	SbMatrix modelMatrix = SoModelMatrixElement::get(state);
	SbViewVolume viewVol = SoViewVolumeElement::get(state);
	currentLineOfSight   = viewVol.getProjectionDirection();

	SbVec3f  translate, scale;
	SbRotation	rotation, scaleOrientation;
	modelMatrix.getTransform(translate, rotation, scale, scaleOrientation);
	SbRotation answer = rotation.inverse();


	// rotate the normal to point in the direction opposite to LOS	
	SbRotation		orientation;
	orientation.setValue(normal.getValue(), -currentLineOfSight);

	// calculate the new position of the up vector
	SbVec3f newUp;
	orientation.multVec(axis.getValue(), newUp);

	// This is what the up vector should be lined up with
	// It becomes +Y after the viewing xform
	//	  (0, 1, 0) = desiredUp * viewMatrix
	//	  desiredUp = (0, 1, 0) * (viewMatrix)^-1
	SbVec3f desiredUp;
	SbMatrix viewMatrix = SoViewingMatrixElement::get(state);
	// we know that viewMatrix is orthonormal
	desiredUp[0] = viewMatrix[0][1];
	desiredUp[1] = viewMatrix[1][1];
	desiredUp[2] = viewMatrix[2][1];

	// calculate the angle between newUp and desiredUp
	desiredUp.normalize();	 
	newUp.normalize();
// --> hemisphere rotation error, acos promps for a value between -1.0 and 1.0
//angle = acos(newUp.dot(desiredUp));
	double dNan = newUp.dot(desiredUp);
	//assert(dNan >= -1.0 && dNan <= 1.0);
	if (dNan <= -1.0 || dNan >= 1.0)
	{
		if (dNan < -1.0)
			dNan = -1.0;
		else dNan = 1.0;
	}
	angle = acos(dNan);
// <-- hemisphere rotation error
	if ((newUp.cross(desiredUp)).dot(currentLineOfSight) < 0.0)
		angle = -angle;

	// rotate about currentLineOfSight to line newUp with desiredUp
	SbRotation		twist;
	twist.setValue(currentLineOfSight, angle);

	currentRotation = orientation * twist * answer;
}

// --> EGB && SGB
void		
ChemDisplay::beginSpheres(const int displayStyle, ChemUnitSphere *unitSphere)
{
	if (unitSphere == NULL)
		unitSphere = this->unitSphere;

	if (displayStyle == ChemDisplayParam::ATOMSPHERE_BILLBOARD ||
			displayStyle == ChemDisplayParam::ATOMSPHERE_LOD ||
			displayStyle == ChemDisplayParam::ATOMSPHERE_LOD_GEOMETRY ||
			clipAtoms)
	{
		SbMatrix mm;
		glGetFloatv(GL_MODELVIEW_MATRIX,(float*)mm);
		SbVec3f x; x.setValue(mm[0][0],mm[1][0],mm[2][0]);
		SbVec3f y; y.setValue(mm[0][1],mm[1][1],mm[2][1]);

		SbVec3f a,b,c,d,normal,normala,normalb,normalc,normald;
		
		a = (-x-y) ;
		b =  (x-y) ;
		c =  (x+y) ;
		d = (-x+y) ;
		normal = (b-a).cross(d-a);
		normala = (a-b) + (a-d) + normal; normala.normalize();
		normalb = (b-a) + (b-c) + normal; normalb.normalize();
		normalc = (c-d) + (c-b) + normal; normalc.normalize();
		normald = (d-c) + (d-a) + normal; normald.normalize();

		unitSphere->a = a;
		unitSphere->b = b;
		unitSphere->c = c;
		unitSphere->d = d;
		unitSphere->normala = normala;
		unitSphere->normalb = normalb;
		unitSphere->normalc = normalc;
		unitSphere->normald = normald;
	}

	if (displayStyle == ChemDisplayParam::ATOMSPHERE_BILLBOARD)
	{
		glBindTexture(GL_TEXTURE_2D,unitSphere->getSphereTextureId());
		
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_NOTEQUAL,0);
	}
	else if 
		(displayStyle == ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES ||
		 displayStyle == ChemDisplayParam::ATOMSPHERE_ARRAY_FULLSPHERES)
	{
			glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
			if (lastSphereDoNormals) 
			{
				glEnableClientState(GL_NORMAL_ARRAY);
			  glNormalPointer(GL_FLOAT,sizeof(SbVec3f),unitSphere->getVertices());
			}
			if (lastSphereDoTextures) 
			{
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexCoordPointer(2,GL_FLOAT,sizeof(SbVec2f),unitSphere->getTexCoords());
				if (unitSphere->getTexCoords() == NULL) 
				{
					int32_t vertCount = unitSphere->getNumberOfVertices();

					if ( displayStyle == ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
					{
						int32_t vend = vertCount/2;
						unitSphere->generateSphereTexCoords((int32_t&)vend);
					}
					else if (displayStyle == ChemDisplayParam::ATOMSPHERE_ARRAY_FULLSPHERES)
						unitSphere->generateSphereTexCoords((int32_t&)vertCount);
				}
			}
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3,GL_FLOAT,sizeof(SbVec3f),unitSphere->getVertices());
	}
	else if 
		(displayStyle == ChemDisplayParam::ATOMSPHERE_GLUSPHERE)
	{
			if (lastSphereDoNormals) 	gluQuadricNormals(unitSphere->getQuadric(),GLU_SMOOTH);
			if (lastSphereDoTextures) gluQuadricTexture(unitSphere->getQuadric(),GL_TRUE);
	}
}

void		
ChemDisplay::endSpheres(const int displayStyle)
{
	if (displayStyle == ChemDisplayParam::ATOMSPHERE_BILLBOARD)
	{
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_ALPHA_TEST);
	}
	else if 
		(displayStyle == ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES ||
		 displayStyle == ChemDisplayParam::ATOMSPHERE_ARRAY_FULLSPHERES)
	{
		glPopClientAttrib();
	}
}
// <-- EGB && SGB

//
// End of SPHERE section
//

#undef ATOMLOOP_START
#undef ATOMLOOP_END
// --> EGB && SGB
#undef DOLOD
#undef DOHLOD
// <-- EGB && SGB
