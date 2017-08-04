#include <iostream>
using namespace std;
/*
 * ChemDisplay.cpp
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

/*
	Porting to Windows:
	Daniel Soto	2002/02/13
*/
/*
        Some bug fixing
        Manuel Pastor and Cristina Herraiz 2002/03/06
*/
#ifdef WIN32
#include <windows.h>
#endif

#include <math.h>

// --> lmw - added this for memset (needed on Mac OS X)
#include <string.h>
// <--

#include "inv/flclient.h"

#include "inv/ChemKit/SparseBits.H"

#include "inv/ChemKit/ChemBitmapFontCache.H"
#include "inv/ChemKit/ChemBondStuff.H"
// --> EGB && SGB && DSA
#include "inv/ChemKit/ChemLOD.H"
#include "inv/ChemKit/ChemResidueStuff.H"
// <-- EGB && SGB && DSA
// --> schematic
#include "inv/ChemKit/ChemSchematicChain.H"
// <-- schematic
#include "inv/ChemKit/ChemAromaticRing.H"
#include "inv/ChemKit/ChemUnitCylinder.H"
#include "inv/ChemKit/ChemUnitSphere.H"

#include "inv/ChemKit/ChemAtomBBox.H"
#include "inv/ChemKit/ChemBondBBox.H"
#include "inv/ChemKit/ChemLabelBBox.H"
#include "inv/ChemKit/ChemStickBondBBox.H"
#include "inv/ChemKit/ChemWireframeBondBBox.H"

#include "inv/ChemKit/ChemDetail.H"
#include "inv/ChemKit/ChemDisplay.H"
#include "inv/ChemKit/ChemDisplayPath.H"
#include "inv/ChemKit/ChemDisplayPathList.H"
#include "inv/ChemKit/ChemDisplaySelectionElement.H"
#include "inv/ChemKit/ChemBaseDataElement.H"
#include "inv/ChemKit/ChemColorElement.H"
#include "inv/ChemKit/ChemDisplayParamElement.H"
#include "inv/ChemKit/ChemRadiiElement.H"
#include "inv/ChemKit/ChemSelectionElement.H"
#include "inv/ChemKit/ChemSSSR.H"

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
// --> CAstick
#include "inv/ChemKit/ChemData.H"
#include "inv/nodes/SoSeparator.H"
// <-- CAstick

// --> octree culling
#include "inv/ChemKit/ChemOctreeNode.H"
// <-- octree culling

#define ATOMLOOP_START(INDEX)												  \
numAtomLoops = (INDEX).getNum();      \
for (atomLoop = 0; atomLoop < numAtomLoops; atomLoop++) {	  \
	(INDEX)[atomLoop].getValue(atomStart, atomEnd);		  \
	if (atomEnd == CHEM_DISPLAY_USE_REST_OF_ATOMS) {	  \
	   atomEnd = globalNumberOfAtoms;			  \
	}							  \
	else {			  \
	   atomEnd += atomStart;	  \
	}				  \
	for (theAtom = atomStart; theAtom < atomEnd; theAtom++) {

#define ATOMLOOP_END	  \
	}		  \
}

#define BONDLOOP_START(INDEX)	  \
numBondLoops = (INDEX).getNum();			  \
for (bondLoop = 0; bondLoop < numBondLoops; bondLoop++) {	  \
	(INDEX)[bondLoop].getValue(bondStart, bondEnd);		  \
	if (bondEnd == CHEM_DISPLAY_USE_REST_OF_BONDS) {	  \
	   bondEnd = globalNumberOfBonds;		  \
	}	  \
	else {	  \
	   bondEnd += bondStart;  \
	}			  \
	for (theBond = bondStart; theBond < bondEnd; theBond++) {

#define BONDLOOP_END	  \
	}		  \
}

// --> residue selection
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
// <-- residue selection

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
static SbVec3f junkVec(9999, 9999, 9999);

static SbBool isPointBetweenPlanes(const SbVec3f &point,
	const SbViewVolume &viewVol, const SoClipPlaneElement *clipPlaneElement);

static SbBool isBoxBetweenPlanes(const SbBox3f &box,
	const SbViewVolume &viewVol, const SoClipPlaneElement *clipPlaneElement);

static SbBool isSilhouetteBetweenPlanes(const Silhouette &s,
	const SbViewVolume &viewVol, const SoClipPlaneElement *clipPlaneElement);

static void checkAromaticity(ChemBaseData *chemData, 
	int32_t numberOfRings, int32_t maxRingSize, float aromaticRingScale,
	SbIntList *ringList, ChemBondStuff *bondAttributes, 
	SbPList &aromaticRingList);

static void getLinearPaths(SbIntList &theList, ChemBondStuff *bondAttributes,
	int32_t &numPaths, SbIntList *&thePaths);

static void processLinearResonanceBonds(float multiSep, int32_t numPaths,
	SbIntList *rPaths, ChemIntList *atch, ChemBondStuff *bondAttributes,
	ChemBaseData *chemData);

static void processCumulenes(int32_t numPaths, SbIntList *dPaths, 
	ChemIntList *atch, ChemBondStuff *bondAttributes, ChemBaseData *chemData);

static void getPlanes(int32_t firstBond, int32_t lastBond,
	ChemIntList *atch, ChemBondStuff *bondAttributes,
	ChemBaseData *chemData,
	SbVec3f &coord1, SbVec3f &coord2,
	SbBool &firstIsLinear, SbBool &lastIsLinear,
	SbVec3f &firstVec21, SbVec3f &lastVec21,
	SbVec3f &firstPlaneNormal, SbVec3f &firstPerpNormal,
	SbVec3f &lastPlaneNormal, SbVec3f &lastPerpNormal);

SO_NODE_SOURCE(ChemDisplay);



////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Constructor
//
// Use: public

ChemDisplay::ChemDisplay()
//
////////////////////////////////////////////////////////////////////////
{
	SO_NODE_CONSTRUCTOR(ChemDisplay);

	SO_NODE_ADD_FIELD(atomIndex, (0,-1));
	SO_NODE_ADD_FIELD(bondIndex, (0,-1));
	SO_NODE_ADD_FIELD(atomLabelIndex, (0,-1));
	SO_NODE_ADD_FIELD(bondLabelIndex, (0,-1));

	normalFont = NULL;
	highlightFont = NULL;

	bitmapsEqual = TRUE;

	lastAromaticRingNumSides = 0;
	aromaticRingCoords = NULL;

	lastSphereDoNormals = FALSE;
	lastSphereDoTextures = FALSE;
	lastSphereDisplayStyle = ChemDisplayParam::ATOMSPHERE_HEMISPHERES;
	lastSphereComplexity = 0.0;
	unitSphere = NULL;

#ifdef DISPLAYLIST
	unitSphereList = 0;
#endif

	lastCylinderDoNormals = FALSE;
	lastCylinderDoTextures = FALSE;
	lastCylinderDisplayStyle = ChemDisplayParam::DISPLAY_STICK;
	lastCylinderComplexity = 0.0;
	unitCylinder = NULL;

	lastChemBaseDataElement = NULL;
	lastChemDisplayParamElement = NULL;
	lastChemRadiiElement = NULL;
	lastChemSelectionElement = NULL;

	lastComputeBBoxBox.makeEmpty();
	lastComputeBBoxCenter.setValue(0.0, 0.0, 0.0);

	currentModelMatrix.makeIdentity();
	currentMVP.makeIdentity();

	globalNumberOfAtoms = 0;
	globalNumberOfBonds = 0;
	localNumberOfAtoms = 0;
	localNumberOfBonds = 0;
	bondAttributes = NULL;
// --> moiv doublebond error
	bondAttributesIndex = NULL;
// <-- moiv doublebond error
// --> CAstick
	globalNumberOfResidues = 0;
	localNumberOfResidues = 0;

	SO_NODE_ADD_FIELD(residueIndex, (0,-1));
	SO_NODE_ADD_FIELD(residueLabelIndex, (0,-1));

// --> schematic
  SO_NODE_ADD_FIELD(schematicIndex, (0,-1));
// <-- schematic

	lastResidueCylinderDoNormals = FALSE;
	lastResidueCylinderDoTextures = FALSE;
	lastResidueCylinderDisplayStyle = ChemDisplayParam::BONDCYLINDER_NOCAP;
	lastResidueCylinderComplexity = 0.0;
	residueUnitCylinder = NULL;
	residueAttributes = NULL;
// <-- CAstick
// --> line ribbon
	lr = NULL;
	residueNumberOfThreads = 0;
// <-- line ribbon
// --> schematic
	schematicUnitCylinder = NULL;
	scvtx = NULL;
	scnrm = NULL;
	schematicTubeShapeSegments = 0;
  schematicChains = NULL;
  globalNumberOfSchematics = 0;
	localNumberOfSchematics = 0;
// <-- schematic
// --> schematic
	schematicCylinderIndex.set1Value(0,8);
	schematicArrowIndex.set1Value(0,5);
	schematicTubeIndex.set1Value(0,1);
// <-- schematic
	isBuiltIn = TRUE;

	// Daniel Soto	2002-02-14
	// Fix default render in Coin3D (no culling and white color)
//    glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
//    glEnable(GL_COLOR_MATERIAL);

// --> EGB && SGB
	lodSelector = new ChemLOD();
	clipAtoms = true;
	clipBonds = true;
// <-- EGB && SGB
// --> octree culling
	octreenode = new ChemOctreeNode();
  // duplicates
  renderedBonds = NULL;
  renderedResidues = NULL;
// <-- octree culling
// --> improving ribbon speed
  basisCoefficientsLOD = new SoMFFloat[9]; // segment range from 2 to 10
// <-- improving ribbon speed
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Destructor
//
// Use: protected

ChemDisplay::~ChemDisplay()
//
////////////////////////////////////////////////////////////////////////
{
	if (aromaticRingCoords != NULL)
		delete [] aromaticRingCoords;

#ifdef DISPLAYLIST
	if (glIsList(unitSphereList))
		glDeleteLists(unitSphereList, 1);
#endif

	if (normalFont != NULL) normalFont->unref();
	if (highlightFont != NULL) highlightFont->unref();

	if (unitSphere != NULL) delete unitSphere;
	if (unitCylinder != NULL) delete unitCylinder;

	nonBondedAtoms.truncate(0);
	singleNormal.truncate(0);
	singleHighlight.truncate(0);
	doubleNormal.truncate(0);
	doubleHighlight.truncate(0);
	tripleNormal.truncate(0);
	tripleHighlight.truncate(0);
	quadrupleNormal.truncate(0);
	quadrupleHighlight.truncate(0);
	resonanceNormal.truncate(0);
	resonanceHighlight.truncate(0);
	hbondNormal.truncate(0);
	hbondHighlight.truncate(0);

	if (lastChemBaseDataElement != NULL) {
		delete (SoElement *)lastChemBaseDataElement;
	}
	if (lastChemDisplayParamElement != NULL) {
		delete (SoElement *)lastChemDisplayParamElement;
	}
	if (lastChemRadiiElement != NULL) {
		delete (SoElement *)lastChemRadiiElement;
	}
	if (lastChemSelectionElement != NULL) {
		delete lastChemSelectionElement;
	}

	if (bondAttributes != NULL) {
		delete [] bondAttributes;
	}
// --> moiv doublebond error
	if (bondAttributesIndex != NULL)
	{
		delete [] bondAttributesIndex;
		bondAttributesIndex = NULL;
	}
// <-- moiv doublebond error

	if (aromaticRingList.getLength() != 0) {
		for (int i = aromaticRingList.getLength()-1; i >= 0; i--) {
                        // GDB 2/1/12 Compiler warning on delete
			//delete aromaticRingList[i];
			free(aromaticRingList[i]);
		}
		aromaticRingList.truncate(0);
// --> coin, memory pointers aren't freed
#ifdef __coin
		aromaticRingList.truncate(0,TRUE);
#endif
// <-- coin, memory pointers aren't freed
	}
// --> lod private
	if (lodSelector!=NULL) delete lodSelector;
// <-- lod private
// --> CAstick
	if (residueUnitCylinder != NULL)	delete residueUnitCylinder ;
// <-- CAstick
// --> line ribbon
	if (lr != NULL)	delete[] lr;
// <-- line ribbon
// --> residues
 	if (residueAttributes != NULL)
		delete [] residueAttributes;
// <-- residues
// --> schematic
	if (schematicUnitCylinder != NULL)	delete schematicUnitCylinder ;
	if (scvtx != NULL)	delete[] scvtx;
	if (scnrm != NULL)	delete[] scnrm;
	cleanSchematic();
// <-- schematic

// --> octree culling
	if (octreenode) delete octreenode;
  // duplicates
  delete[] renderedBonds;
  delete[] renderedResidues;
// <-- octree culling
// --> improving ribbon speed
  delete [] basisCoefficientsLOD;
// <-- improving ribbon speed
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    If a change was made to one of the public "Index" nodes, then we
//	  need to reset some internal things.
//
// Use: internal

void
ChemDisplay::notify(SoNotList *list)
//
////////////////////////////////////////////////////////////////////////
{
	if ((list->getLastRec()->getType() == SoNotRec::CONTAINER)) {
		SoField *field = list->getLastField();
		if ((field == &atomIndex) || (field == &bondIndex) ||
			(field == &atomLabelIndex) || (field == &bondLabelIndex)) {
			normalAtomIndex.deleteValues(0);
			normalBondIndex.deleteValues(0);
			normalBondLabelIndex.deleteValues(0);
			normalAtomLabelIndex.deleteValues(0);

			normalAtomIndex.setValues(0, atomIndex.getNum(),
				atomIndex.getValues(0));
			normalBondIndex.setValues(0, bondIndex.getNum(),
				bondIndex.getValues(0));
			normalAtomLabelIndex.setValues(0, atomLabelIndex.getNum(),
				atomLabelIndex.getValues(0));
			normalBondLabelIndex.setValues(0, bondLabelIndex.getNum(),
				bondLabelIndex.getValues(0));

			highlightAtomIndex.deleteValues(0);
			highlightBondIndex.deleteValues(0);
			highlightBondLabelIndex.deleteValues(0);
			highlightAtomLabelIndex.deleteValues(0);

			nonBondedAtoms.truncate(0);
			normalNonBondedAtomIndex.deleteValues(0);

			singleNormal.truncate(0);
			singleHighlight.truncate(0);
			doubleNormal.truncate(0);
			doubleHighlight.truncate(0);
			tripleNormal.truncate(0);
			tripleHighlight.truncate(0);
			quadrupleNormal.truncate(0);
			quadrupleHighlight.truncate(0);
			resonanceNormal.truncate(0);
			resonanceHighlight.truncate(0);
			hbondNormal.truncate(0);
			hbondHighlight.truncate(0);

			localNumberOfAtoms = 0;
			localNumberOfBonds = 0;
			if (bondAttributes != NULL) {
				delete [] bondAttributes;
				bondAttributes = NULL;
			}
// --> moiv doublebond error
			if (bondAttributesIndex != NULL)
			{
				delete [] bondAttributesIndex;
				bondAttributesIndex = NULL;
			}
// <-- moiv doublebond error

			if (aromaticRingList.getLength() != 0) {
				for (int i = aromaticRingList.getLength()-1; i >= 0; i--) {
                                        // GDB 2/1/12 Compiler warning on delete
					//delete aromaticRingList[i];
					free(aromaticRingList[i]);
				}
				aromaticRingList.truncate(0);
			}

			if (lastChemBaseDataElement != NULL) {
				delete (SoElement *)lastChemBaseDataElement;
				lastChemBaseDataElement = NULL;
			}
			if (lastChemDisplayParamElement != NULL) {
				delete (SoElement *)lastChemDisplayParamElement;
				lastChemDisplayParamElement = NULL;
			}
			if (lastChemRadiiElement != NULL) {
				delete (SoElement *)lastChemRadiiElement;
				lastChemRadiiElement = NULL;
			}
			if (lastChemSelectionElement != NULL) {
				delete lastChemSelectionElement;
				lastChemSelectionElement = NULL;
			}
		}
// --> residues changes
		else if ((field == &residueIndex) || (field == &residueLabelIndex) ||
      (field == &schematicIndex))
		{
			normalResidueIndex.deleteValues(0);
			normalResidueLabelIndex.deleteValues(0);
			
			normalResidueIndex.setValues(0, residueIndex.getNum(),
				residueIndex.getValues(0));
			normalResidueLabelIndex.setValues(0, residueLabelIndex.getNum(),
				residueLabelIndex.getValues(0));

			localNumberOfResidues = 0;

			highlightResidueIndex.deleteValues(0);
			highlightResidueLabelIndex.deleteValues(0);

      normalSchematicIndex.deleteValues(0);
			normalSchematicIndex.setValues(0, schematicIndex.getNum(),
				schematicIndex.getValues(0));
			
			globalNumberOfSchematics = localNumberOfSchematics = 0;
			highlightSchematicIndex.deleteValues(0);

			if (lastChemBaseDataElement != NULL) {
				delete (SoElement *)lastChemBaseDataElement;
				lastChemBaseDataElement = NULL;
			}
			if (lastChemDisplayParamElement != NULL) {
				delete (SoElement *)lastChemDisplayParamElement;
				lastChemDisplayParamElement = NULL;
			}
			if (lastChemRadiiElement != NULL) {
				delete (SoElement *)lastChemRadiiElement;
				lastChemRadiiElement = NULL;
			}
			if (lastChemSelectionElement != NULL) {
				delete lastChemSelectionElement;
				lastChemSelectionElement = NULL;
			}
		}
// <-- residues changes
	}
	SoShape::notify(list);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Performs GL rendering of the atoms and bonds
//
// Use: public, extender

void
ChemDisplay::GLRender(SoGLRenderAction *action)
//
////////////////////////////////////////////////////////////////////////
{
	// First see if the object is visible and should be rendered now
	if (!shouldGLRender(action))
		return;

#ifdef IV2_0
	SoMaterialBundle mb(action);
	mb.sendFirst();
#endif
	SoState *state = action->getState();

// save rendering state 
	glPushAttrib(GL_CURRENT_BIT|GL_LIGHTING_BIT|GL_ENABLE_BIT);

// change rendering state
// -->
	glEnable(GL_LIGHTING);
	glEnable(GL_COLOR_MATERIAL);

	glColor3f(1,1,1);
	float ambient[] = {0.2, 0.2, 0.2, 1.0};
	glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,ambient);

	float diffuse[] = {0.8, 0.8, 0.8, 1.0};
	glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,diffuse);
	
	float specular[] = {0.0, 0.0, 0.0, 1.0};
	glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,specular);

	float emission[] = {0.0, 0.0, 0.0, 1.0};
	glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,emission);

	glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,0.0f);
// <--


	ChemBaseData *chemData = ChemBaseDataElement::get(state);
	globalNumberOfAtoms = chemData->getNumberOfAtoms();
	globalNumberOfBonds = chemData->getNumberOfBonds();
	localNumberOfAtoms = getNumberOfAtoms(action);
	localNumberOfBonds = getNumberOfBonds(action);
        //cout << "G atoms/bonds " << globalNumberOfAtoms << "/" << globalNumberOfBonds << endl;
        //cout << "l atoms/bonds " << localNumberOfAtoms << "/" << localNumberOfBonds << endl;
// --> CAstick
	ChemData* chemDataResidues = (ChemData*)chemData;
	if (chemDataResidues != NULL)
	{
		globalNumberOfResidues = chemDataResidues->residues.getNum();
		localNumberOfResidues = getNumberOfResidues(action);
	}
// <-- CAstick

	ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);
	int displayStyle = cdp->displayStyle.getValue();
// --> CAstick
	int residueDisplayStyle = cdp->residueDisplayStyle.getValue();
// <-- CAstick
	SbBool showAtomLabels = cdp->showAtomLabels.getValue();
	SbBool showBondLabels = cdp->showBondLabels.getValue();

	// Keep track of the MVP matrix.  Other actions need this
	// stored matrix (like ChemBBoxAction).
	currentViewMatrix = SoViewingMatrixElement::get(state);
	currentMVP = SoProjectionMatrixElement::get(state);
	currentMVP = currentMVP.multLeft(currentViewMatrix);
	currentModelMatrix = SoModelMatrixElement::get(state);
	currentMVP = currentMVP.multLeft(currentModelMatrix);
	currentVPR = SoViewportRegionElement::get(state);
	currentViewVolume = SoViewVolumeElement::get(state);

// --> compute billboard vertices, camera position
	SbVec3f x; x.setValue(currentViewMatrix[0][0],currentViewMatrix[1][0],currentViewMatrix[2][0]);
	SbVec3f y; y.setValue(currentViewMatrix[0][1],currentViewMatrix[1][1],currentViewMatrix[2][1]);

	currentABCD[0] = (-x-y) ;
	currentABCD[1] =  (x-y) ;
	currentABCD[2] =  (x+y) ;
	currentABCD[3] = (-x+y) ;

	SbMatrix inverseModelview = currentViewMatrix.inverse();
	
	SbVec3f position(0,0,0);
	inverseModelview.multVecMatrix(position,currentCameraPosition);

	currentView[0]=(int)currentVPR.getViewportOrigin()[0];
	currentView[1]=(int)currentVPR.getViewportOrigin()[1];
	currentView[2]=currentVPR.getWindowSize()[0];
	currentView[3]=currentVPR.getWindowSize()[1];

// <-- compute billboard vertices

	// Keep track of the direction of the camera.  Other actions need
	// this stored rotation (like ChemBBoxAction).
	calculateHemisphereRotation(state);

	SbBool showNonBondedAtoms = 
		(cdp->nonBondedAtomStyle.getValue() !=
			ChemDisplayParam::NONBONDED_ATOM_NONE);

	// Generate the indices for the items to be drawn normally and
	// highlighted.
	if (!action->isRenderingDelayedPaths()) {
		generateIndices(action);
	}

// --> EGB && SGB
	lodSelector->resetAtoms(chemData->getNumberOfAtoms());
	lodSelector->resetBonds(chemData->getNumberOfBonds());
  // Temp variables for lod statistics
	unsigned int sphereClipCnt = 0;
	unsigned int pointCnt = 0;
	unsigned int billboardCnt = 0;
	unsigned int sphere1Cnt = 0;
	unsigned int sphere2Cnt = 0;
	unsigned int sphere3Cnt = 0;
	unsigned int sphere4Cnt = 0;
// <-- EGB && SGB

// --> update clipping options
	clipAtoms = cdp->clipAtoms.getValue();
	clipBonds = cdp->clipBonds.getValue();
// <-- update clipping options

// --> billboard fix
  ChemUnitSphere::generateBillboard();
// <-- billboard fix

//--> octree culling
  if (globalNumberOfAtoms>0 || globalNumberOfBonds>0 || globalNumberOfResidues>0)
  {
	octreenode->getIndexes((const ChemData*)chemData,
		vnormalAtomIndex,vnormalNonBondedAtomIndex,vnormalBondIndex,vnormalResidueIndex,
		vhighlightAtomIndex,vhighlightNonBondedAtomIndex,vhighlightBondIndex,vhighlightResidueIndex,
		vnormalAtomLabelIndex,vnormalBondLabelIndex,vnormalResidueLabelIndex,
		vhighlightAtomLabelIndex,vhighlightBondLabelIndex,vhighlightResidueLabelIndex);
  } else {
     vnormalAtomIndex.setNum(0);
     vnormalNonBondedAtomIndex.setNum(0);
     vnormalBondIndex.setNum(0);
     vnormalResidueIndex.setNum(0);
     vhighlightAtomIndex.setNum(0);
     vhighlightNonBondedAtomIndex.setNum(0);
     vhighlightBondIndex.setNum(0);
     vhighlightResidueIndex.setNum(0);
     vnormalAtomLabelIndex.setNum(0);
     vnormalBondLabelIndex.setNum(0);
     vnormalResidueLabelIndex.setNum(0);
     vhighlightAtomLabelIndex.setNum(0);
     vhighlightBondLabelIndex.setNum(0);
     vhighlightResidueLabelIndex.setNum(0);
  }
  // duplicates
  if (globalNumberOfBonds>0)
	memset(renderedBonds,0,globalNumberOfBonds*sizeof(bool));
  if (globalNumberOfResidues>0)
	memset(renderedResidues,0,globalNumberOfResidues*sizeof(bool));
//<-- octree culling

	// Invoke the appropriate rendering as determined from the display style.
	// For the STICK and WIREFRAME styles, also display any atoms that are
	// not bonded to anything.
	switch (displayStyle) {
		case ChemDisplayParam::DISPLAY_NONE:
			if (showAtomLabels) renderAtomLabelsWithWireframe(action);
			if (showBondLabels) renderBondLabelsWithWireframe(action);
			break;

		case ChemDisplayParam::DISPLAY_CPK:
			if (cdp->atomSphereDisplayStyle.getValue() == 
				ChemDisplayParam::ATOMSPHERE_HEMISPHERES) {
#ifdef IV2_0
				SoCacheElement::invalidate(state);
#else
				SoGLCacheContextElement::shouldAutoCache(state,
					SoGLCacheContextElement::DONT_AUTO_CACHE);
#endif
			}
			else {
				if (showAtomLabels || showBondLabels) {
#ifdef IV2_0
					SoCacheElement::invalidate(state);
#else
					SoGLCacheContextElement::shouldAutoCache(state,
						SoGLCacheContextElement::DONT_AUTO_CACHE);
#endif
				}
#ifndef IV2_0
				else {
					SoGLCacheContextElement::shouldAutoCache(state,
						SoGLCacheContextElement::DO_AUTO_CACHE);
				}
#endif
			}
			// Render the bonded atoms as spheres
// --> octree culling
			//renderAtomsAsSpheres(action, TRUE, TRUE,normalAtomIndex, highlightAtomIndex);
			renderAtomsAsSpheres(action, TRUE, TRUE,vnormalAtomIndex, vhighlightAtomIndex);
// <-- octree culling

// --> EGB && SGB
					sphereClipCnt = lodSelector->sphereClipCnt;
					pointCnt = lodSelector->pointCnt;
					billboardCnt = lodSelector->billboardCnt;
					sphere1Cnt = lodSelector->sphere1Cnt;
					sphere2Cnt = lodSelector->sphere2Cnt;
					sphere3Cnt = lodSelector->sphere3Cnt;
					sphere4Cnt = lodSelector->sphere4Cnt;

					lodSelector->resetAtoms(chemData->getNumberOfAtoms());
// <-- EGB && SGB

			// Render the nonBonded atoms as spheres
// --> octree culling
			//renderAtomsAsSpheres(action, TRUE, TRUE,normalNonBondedAtomIndex, highlightNonBondedAtomIndex);
			renderAtomsAsSpheres(action, TRUE, TRUE,vnormalNonBondedAtomIndex, vhighlightNonBondedAtomIndex);
// <-- octree culling

// --> EGB && SGB
					lodSelector->sphereClipCnt += sphereClipCnt;
					lodSelector->pointCnt += pointCnt;
					lodSelector->billboardCnt += billboardCnt;
					lodSelector->sphere1Cnt += sphere1Cnt;
					lodSelector->sphere2Cnt += sphere2Cnt;
					lodSelector->sphere3Cnt += sphere3Cnt;
					lodSelector->sphere4Cnt += sphere4Cnt;
// <-- EGB && SGB

			if (showAtomLabels) renderAtomLabelsWithSpheres(action);
			if (showBondLabels) renderBondLabelsWithWireframe(action);
			break;

		case ChemDisplayParam::DISPLAY_STICK:
			if (showAtomLabels || showBondLabels ||
				(highlightAtomIndex.getNum() > 0 && 
				 cdp->highlightAtomStyle.getValue() ==
					ChemDisplayParam::HIGHLIGHT_ATOM_BITMAP) ||
				(nonBondedAtoms.getLength() > 0 && showNonBondedAtoms)) {
#ifdef IV2_0
				SoCacheElement::invalidate(state);
#else
				SoGLCacheContextElement::shouldAutoCache(state,
					SoGLCacheContextElement::DONT_AUTO_CACHE);
#endif
			}
#ifndef IV2_0
			else {
				SoGLCacheContextElement::shouldAutoCache(state,
					SoGLCacheContextElement::DO_AUTO_CACHE);
			}
#endif
			renderBondsAsCylinders(action);
			if (showAtomLabels) renderAtomLabelsWithCylinders(action);
			if (showBondLabels) renderBondLabelsWithCylinders(action);
			if (showNonBondedAtoms) renderSingleAtoms(action);
			if (highlightAtomIndex.getNum() > 0) {
				highlightAtomsWithCylinders(action);
			}
			break;

		case ChemDisplayParam::DISPLAY_BALLSTICK:
			if (cdp->atomSphereDisplayStyle.getValue() == 
				ChemDisplayParam::ATOMSPHERE_HEMISPHERES) {
#ifdef IV2_0
				SoCacheElement::invalidate(state);
#else
				SoGLCacheContextElement::shouldAutoCache(state,
					SoGLCacheContextElement::DONT_AUTO_CACHE);
#endif
			}
			else {
				if (showAtomLabels || showBondLabels ||
					(highlightAtomIndex.getNum() > 0 && 
					 cdp->highlightAtomStyle.getValue() ==
						ChemDisplayParam::HIGHLIGHT_ATOM_BITMAP) ||
					(nonBondedAtoms.getLength() > 0 && showNonBondedAtoms)) {
#ifdef IV2_0
					SoCacheElement::invalidate(state);
#else
					SoGLCacheContextElement::shouldAutoCache(state,
						SoGLCacheContextElement::DONT_AUTO_CACHE);
#endif
				}
#ifndef IV2_0
				else {
					SoGLCacheContextElement::shouldAutoCache(state,
						SoGLCacheContextElement::DO_AUTO_CACHE);
				}
#endif
			}
			renderBondsAsCylinders(action);

			// Render the bonded atoms as spheres
// --> octree culling
			//renderAtomsAsSpheres(action, TRUE, TRUE,normalAtomIndex, highlightAtomIndex);
			renderAtomsAsSpheres(action, TRUE, TRUE,vnormalAtomIndex, vhighlightAtomIndex);
// <-- octree culling

// --> EGB && SGB
					sphereClipCnt = lodSelector->sphereClipCnt;
					pointCnt = lodSelector->pointCnt;
					billboardCnt = lodSelector->billboardCnt;
					sphere1Cnt = lodSelector->sphere1Cnt;
					sphere2Cnt = lodSelector->sphere2Cnt;
					sphere3Cnt = lodSelector->sphere3Cnt;
					sphere4Cnt = lodSelector->sphere4Cnt;

					lodSelector->resetAtoms(chemData->getNumberOfAtoms());
// <-- EGB && SGB

			// Render the nonBonded atoms as spheres
// --> octree culling
			//renderAtomsAsSpheres(action, TRUE, TRUE,normalNonBondedAtomIndex, highlightNonBondedAtomIndex);
			renderAtomsAsSpheres(action, TRUE, TRUE,vnormalNonBondedAtomIndex, vhighlightNonBondedAtomIndex);
// <-- octree culling

// --> EGB && SGB
					lodSelector->sphereClipCnt += sphereClipCnt;
					lodSelector->pointCnt += pointCnt;
					lodSelector->billboardCnt += billboardCnt;
					lodSelector->sphere1Cnt += sphere1Cnt;
					lodSelector->sphere2Cnt += sphere2Cnt;
					lodSelector->sphere3Cnt += sphere3Cnt;
					lodSelector->sphere4Cnt += sphere4Cnt;
// <-- EGB && SGB

			if (showAtomLabels) renderAtomLabelsWithSpheres(action);
			if (showBondLabels) renderBondLabelsWithCylinders(action);
			break;

		case ChemDisplayParam::DISPLAY_BALLWIRE:
			// Check if antiAliasing and either addDelayedPath or render
			if (cdp->bondWireframeAntiAlias.getValue() !=
				ChemDisplayParam::WIREFRAME_ANTIALIAS_NONE) {
#ifdef IV2_0
				SoCacheElement::invalidate(state);
#else
				SoGLCacheContextElement::shouldAutoCache(state,
					SoGLCacheContextElement::DONT_AUTO_CACHE);
#endif
				if (action->isRenderingDelayedPaths()) {
					renderBondsAsWireframe(action);
				}
				else {
// --> octree culling
			//renderAtomsAsSpheres(action, TRUE, TRUE,normalAtomIndex, highlightAtomIndex);
			renderAtomsAsSpheres(action, TRUE, TRUE,vnormalAtomIndex, vhighlightAtomIndex);
// <-- octree culling

// --> EGB && SGB
					sphereClipCnt = lodSelector->sphereClipCnt;
					pointCnt = lodSelector->pointCnt;
					billboardCnt = lodSelector->billboardCnt;
					sphere1Cnt = lodSelector->sphere1Cnt;
					sphere2Cnt = lodSelector->sphere2Cnt;
					sphere3Cnt = lodSelector->sphere3Cnt;
					sphere4Cnt = lodSelector->sphere4Cnt;

					lodSelector->resetAtoms(chemData->getNumberOfAtoms());
// <-- EGB && SGB
					// Render the nonBonded atoms as spheres
// --> octree culling
			//renderAtomsAsSpheres(action, TRUE, TRUE,normalNonBondedAtomIndex, highlightNonBondedAtomIndex);
			renderAtomsAsSpheres(action, TRUE, TRUE,vnormalNonBondedAtomIndex, vhighlightNonBondedAtomIndex);
// <-- octree culling

// --> EGB && SGB
					lodSelector->sphereClipCnt += sphereClipCnt;
					lodSelector->pointCnt += pointCnt;
					lodSelector->billboardCnt += billboardCnt;
					lodSelector->sphere1Cnt += sphere1Cnt;
					lodSelector->sphere2Cnt += sphere2Cnt;
					lodSelector->sphere3Cnt += sphere3Cnt;
					lodSelector->sphere4Cnt += sphere4Cnt;
// <-- EGB && SGB

					if (showAtomLabels) renderAtomLabelsWithSpheres(action);
					if (showBondLabels) renderBondLabelsWithWireframe(action);

					action->addDelayedPath(action->getCurPath()->copy());
				}
			}
			else {
				if (cdp->atomSphereDisplayStyle.getValue() == 
					ChemDisplayParam::ATOMSPHERE_HEMISPHERES) {
#ifdef IV2_0
					SoCacheElement::invalidate(state);
#else
					SoGLCacheContextElement::shouldAutoCache(state,
						SoGLCacheContextElement::DONT_AUTO_CACHE);
#endif
				}
				else {
					if (showAtomLabels || showBondLabels ||
						(highlightAtomIndex.getNum() > 0 && 
						 cdp->highlightAtomStyle.getValue() ==
							ChemDisplayParam::HIGHLIGHT_ATOM_BITMAP) ||
						(nonBondedAtoms.getLength() > 0 &&
							showNonBondedAtoms)) {
#ifdef IV2_0
						SoCacheElement::invalidate(state);
#else
						SoGLCacheContextElement::shouldAutoCache(state,
							SoGLCacheContextElement::DONT_AUTO_CACHE);
#endif
					}
#ifndef IV2_0
					else {
						SoGLCacheContextElement::shouldAutoCache(state,
							SoGLCacheContextElement::DO_AUTO_CACHE);
					}
#endif
				}
				// Render the bonded atoms as spheres
// --> octree culling
			//renderAtomsAsSpheres(action, TRUE, TRUE,normalAtomIndex, highlightAtomIndex);
			renderAtomsAsSpheres(action, TRUE, TRUE,vnormalAtomIndex, vhighlightAtomIndex);
// <-- octree culling

// --> EGB && SGB
			sphereClipCnt = lodSelector->sphereClipCnt;
			pointCnt = lodSelector->pointCnt;
			billboardCnt = lodSelector->billboardCnt;
			sphere1Cnt = lodSelector->sphere1Cnt;
			sphere2Cnt = lodSelector->sphere2Cnt;
			sphere3Cnt = lodSelector->sphere3Cnt;
			sphere4Cnt = lodSelector->sphere4Cnt;

			lodSelector->resetAtoms(chemData->getNumberOfAtoms());
// <-- EGB && SGB

				// Render the nonBonded atoms as spheres
// --> octree culling
			//renderAtomsAsSpheres(action, TRUE, TRUE,normalNonBondedAtomIndex, highlightNonBondedAtomIndex);
			renderAtomsAsSpheres(action, TRUE, TRUE,vnormalNonBondedAtomIndex, vhighlightNonBondedAtomIndex);
// <-- octree culling

// --> EGB && SGB
			lodSelector->sphereClipCnt += sphereClipCnt;
			lodSelector->pointCnt += pointCnt;
			lodSelector->billboardCnt += billboardCnt;
			lodSelector->sphere1Cnt += sphere1Cnt;
			lodSelector->sphere2Cnt += sphere2Cnt;
			lodSelector->sphere3Cnt += sphere3Cnt;
			lodSelector->sphere4Cnt += sphere4Cnt;
// <-- EGB && SGB

				renderBondsAsWireframe(action);
				if (showAtomLabels) renderAtomLabelsWithSpheres(action);
				if (showBondLabels) renderBondLabelsWithWireframe(action);

			}
			break;

		case ChemDisplayParam::DISPLAY_WIREFRAME:
			// Check if antiAliasing and either addDelayedPath or render
			if (cdp->bondWireframeAntiAlias.getValue() !=
				ChemDisplayParam::WIREFRAME_ANTIALIAS_NONE) {
#ifdef IV2_0
				SoCacheElement::invalidate(state);
#else
				SoGLCacheContextElement::shouldAutoCache(state,
					SoGLCacheContextElement::DONT_AUTO_CACHE);
#endif
				if (action->isRenderingDelayedPaths()) {
					renderBondsAsWireframe(action);
				}
				else {
					if (highlightAtomIndex.getNum() > 0) {
						highlightAtomsWithWireframe(action);
					}
					if (showAtomLabels) renderAtomLabelsWithWireframe(action);
					if (showBondLabels) renderBondLabelsWithWireframe(action);
					if (showNonBondedAtoms) renderSingleAtoms(action);
					action->addDelayedPath(action->getCurPath()->copy());
				}
			}
			else {
				if (showAtomLabels || showBondLabels ||
					(highlightAtomIndex.getNum() > 0 && 
					 cdp->highlightAtomStyle.getValue() ==
						ChemDisplayParam::HIGHLIGHT_ATOM_BITMAP) ||
					(nonBondedAtoms.getLength() > 0 && showNonBondedAtoms)) {
#ifdef IV2_0
					SoCacheElement::invalidate(state);
#else
					SoGLCacheContextElement::shouldAutoCache(state,
						SoGLCacheContextElement::DONT_AUTO_CACHE);
#endif
				}
#ifndef IV2_0
				else {
					SoGLCacheContextElement::shouldAutoCache(state,
						SoGLCacheContextElement::DO_AUTO_CACHE);
				}
#endif
				renderBondsAsWireframe(action);
				if (showAtomLabels) renderAtomLabelsWithWireframe(action);
				if (showBondLabels) renderBondLabelsWithWireframe(action);
				if (showNonBondedAtoms) renderSingleAtoms(action);
				if (highlightAtomIndex.getNum() > 0) {
					highlightAtomsWithWireframe(action);
				}
			}

			break;

		default:
			break;
	}

// --> CAstick
	switch (residueDisplayStyle)
	{
		case ChemDisplayParam::DISPLAY_RESIDUES_NONE:
		{
		}
		break;

		case ChemDisplayParam::DISPLAY_RESIDUES_CAWIRE:
		{
			renderResiduesAsWireframe(action);
		}
		break;

		case ChemDisplayParam::DISPLAY_RESIDUES_CASTICK:
		{
			renderResiduesAsCylinders(action);
		}
		break;

		case ChemDisplayParam::DISPLAY_RESIDUES_LINERIBBON:
		{
			renderResiduesAsLineRibbons(action);
		}
		
		break;

		case ChemDisplayParam::DISPLAY_RESIDUES_FLATRIBBON:
		{
			renderResiduesAsFlatRibbons(action);
		}
		break;

		case ChemDisplayParam::DISPLAY_RESIDUES_SOLIDRIBBON:
		{
			renderResiduesAsSolidRibbons(action);
		}
		break;

		case ChemDisplayParam::DISPLAY_RESIDUES_SCHEMATIC:
		{
			renderResiduesAsSchematic(action);
		}

		break;
		
	default: break;
	}

	bool showResidueLabels = cdp->showResidueLabels.getValue();
	if (showResidueLabels)
	{
		renderResidueLabels(action);
	}
// <-- CAstick
// --> octree culling
	if (cdp->showOctree.getValue())
	{
		glLineWidth(2);
		glDisable(GL_LIGHTING);
		octreenode->render();
		glEnable(GL_LIGHTING);	
	}
// <-- octree culling

// restore rendering state
	glPopAttrib();

}

#define SELECT(PARTS, PART)   ((PARTS & (PART)) != 0)

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Implements ray picking
//
// Use: public, extender

void
ChemDisplay::rayPick(SoRayPickAction *action)
//
////////////////////////////////////////////////////////////////////////
{
	// First see if the object is pickable
	if (!shouldRayPick(action))
		return;

	SoState *state = action->getState();

	ChemBaseData *chemData = ChemBaseDataElement::get(state);
	globalNumberOfAtoms = chemData->getNumberOfAtoms();
	globalNumberOfBonds = chemData->getNumberOfBonds();
	localNumberOfAtoms = getNumberOfAtoms(action);
	localNumberOfBonds = getNumberOfBonds(action);
// --> CAstick
	ChemData* chemDataResidues = (ChemData*)chemData;
	if (chemDataResidues != NULL)
	{
		globalNumberOfResidues = chemDataResidues->residues.getNum();
		localNumberOfResidues = getNumberOfResidues(action);
	}
// <-- CAstick

	ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);
	int displayStyle = cdp->displayStyle.getValue();
	SbBool showAtomLabels = cdp->showAtomLabels.getValue();
	SbBool showBondLabels = cdp->showBondLabels.getValue();
// --> CAstick
	int residueDisplayStyle = cdp->residueDisplayStyle.getValue();
	//SbBool showResidueLabels = cdp->showResidueLabels.getValue();
// <-- CAstick

	// If the data has changed since the last time we were in ChemDisplay,
	// then we might need to generate the multiple bond stuff.  We check
	// here whether or not to invoke generateIndices because generateIndices
	// also checks for changes in the selection list (which we don't want to
	// have any influence at this point).

	SbBool dataChanged = FALSE;
	if (lastChemBaseDataElement == NULL) {
		dataChanged = TRUE;
	}
	else {
		const ChemBaseDataElement *cbde =
			ChemBaseDataElement::getInstance(state);
		if (!lastChemBaseDataElement->matches(cbde)) {
			dataChanged = TRUE;
		}
	}
	if (dataChanged) generateIndices(action);

	int32_t parts = (int32_t)ChemSelectionElement::get(state);

	// Invoke the appropriate picking as determined from the display style.
	switch (displayStyle) {
		case ChemDisplayParam::DISPLAY_NONE:
			if (SELECT(parts, ChemSelectionElement::ATOMLABELS) &&
				showAtomLabels) {
				pickAtomLabelsWithWireframe(action);
			}
			if (SELECT(parts, ChemSelectionElement::BONDLABELS) &&
				showBondLabels) {
				pickBondLabelsWithWireframe(action);
			}
			break;

		case ChemDisplayParam::DISPLAY_CPK:
			if (SELECT(parts, ChemSelectionElement::ATOMS)) {
				pickAtomsAsSpheres(action);
			}
			if (showAtomLabels || showBondLabels) {
				calculateHemisphereRotation(state);
			}
			if (SELECT(parts, ChemSelectionElement::ATOMLABELS) &&
				showAtomLabels) {
				pickAtomLabelsWithSpheres(action);
			}
			if (SELECT(parts, ChemSelectionElement::BONDLABELS) &&
				showBondLabels) {
				pickBondLabelsWithWireframe(action);
			}
			break;

		case ChemDisplayParam::DISPLAY_STICK:
			if (SELECT(parts, ChemSelectionElement::BONDS)) {
				pickBondsAsCylinders(action);
			}
			if (SELECT(parts, ChemSelectionElement::ATOMS)) {
				pickAtomsWithinRadius(action);
			}
			if (showAtomLabels || showBondLabels) {
				calculateHemisphereRotation(state);
			}
			if (SELECT(parts, ChemSelectionElement::ATOMLABELS) &&
				showAtomLabels) {
				pickAtomLabelsWithCylinders(action);
			}
			if (SELECT(parts, ChemSelectionElement::BONDLABELS) &&
				showBondLabels) {
				pickBondLabelsWithCylinders(action);
			}
			break;

		case ChemDisplayParam::DISPLAY_BALLSTICK:
			if (SELECT(parts, ChemSelectionElement::BONDS)) {
				pickBondsAsCylinders(action);
			}
			if (SELECT(parts, ChemSelectionElement::ATOMS)) {
				pickAtomsAsSpheres(action);
			}
			if (showAtomLabels || showBondLabels) {
				calculateHemisphereRotation(state);
			}
			if (SELECT(parts, ChemSelectionElement::ATOMLABELS) &&
				showAtomLabels) {
				pickAtomLabelsWithSpheres(action);
			}
			if (SELECT(parts, ChemSelectionElement::BONDLABELS) &&
				showBondLabels) {
				pickBondLabelsWithCylinders(action);
			}
			break;

		case ChemDisplayParam::DISPLAY_BALLWIRE:
			if (SELECT(parts, ChemSelectionElement::BONDS)) {
				pickBondsAsWireframe(action);
			}
			if (SELECT(parts, ChemSelectionElement::ATOMS)) {
				pickAtomsAsSpheres(action);
			}
			if (showAtomLabels || showBondLabels) {
				calculateHemisphereRotation(state);
			}
			if (SELECT(parts, ChemSelectionElement::ATOMLABELS) &&
				showAtomLabels) {
				pickAtomLabelsWithSpheres(action);
			}
			if (SELECT(parts, ChemSelectionElement::BONDLABELS) &&
				showBondLabels) {
				pickBondLabelsWithWireframe(action);
			}
			break;

		case ChemDisplayParam::DISPLAY_WIREFRAME:
			if (SELECT(parts, ChemSelectionElement::BONDS)) {
				pickBondsAsWireframe(action);
			}
			if (SELECT(parts, ChemSelectionElement::ATOMS)) {
				pickAtomsWithinRadius(action);
			}
			if (SELECT(parts, ChemSelectionElement::ATOMLABELS) &&
				showAtomLabels) {
				pickAtomLabelsWithWireframe(action);
			}
			if (SELECT(parts, ChemSelectionElement::BONDLABELS) &&
				showBondLabels) {
				pickBondLabelsWithWireframe(action);
			}
			break;

		default:
			break;
	}

// --> CAstick
	if (SELECT(parts, ChemSelectionElement::RESIDUES))
	{
		switch (residueDisplayStyle)
		{
			case ChemDisplayParam::DISPLAY_RESIDUES_NONE:
			{
			}
			break;

			case ChemDisplayParam::DISPLAY_RESIDUES_CAWIRE:
			{
				pickResiduesAsWireframe(action);
			}
			break;

			case ChemDisplayParam::DISPLAY_RESIDUES_CASTICK:
			{
				pickResiduesAsCylinders(action);
			}
			break;

			case ChemDisplayParam::DISPLAY_RESIDUES_LINERIBBON:
			case ChemDisplayParam::DISPLAY_RESIDUES_FLATRIBBON:
			{
				pickResiduesAsFlatRibbons(action);
			}
			break;

			case ChemDisplayParam::DISPLAY_RESIDUES_SOLIDRIBBON:
			{
				pickResiduesAsSolidRibbons(action);
			}
			break;

      case ChemDisplayParam::DISPLAY_RESIDUES_SCHEMATIC:
			{
				pickSchematics(action);
			}
			break;

			default: break;

		}
	} //if (SELECT(parts, ChemSelectionElement::RESIDUES))

	if (SELECT(parts, ChemSelectionElement::RESIDUELABELS))
	{
		pickResiduesLabels(action);
	} //if (SELECT(parts, ChemSelectionElement::RESIDUELABELS))

// <-- CAstick
}
#undef SELECT

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Generate the primitives used in the current display style
//	  of the molecule
//
// Use: protected

void
ChemDisplay::generatePrimitives(SoAction *action)
//
////////////////////////////////////////////////////////////////////////
{
	SoState *state = action->getState();

	ChemBaseData *chemData = ChemBaseDataElement::get(state);
	globalNumberOfAtoms = chemData->getNumberOfAtoms();
	globalNumberOfBonds = chemData->getNumberOfBonds();
	localNumberOfAtoms = getNumberOfAtoms(action);
	localNumberOfBonds = getNumberOfBonds(action);
// --> CAstick
	ChemData* chemDataResidues = (ChemData*)chemData;
	if (chemDataResidues != NULL)
	{
		globalNumberOfResidues = chemDataResidues->residues.getNum();
		localNumberOfResidues = getNumberOfResidues(action);
	}
// <-- CAstick

	ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);
	int displayStyle = cdp->displayStyle.getValue();

	// If the data has changed since the last time we were in ChemDisplay,
	// then we might need to generate the multiple bond stuff.  We check
	// here whether or not to invoke generateIndices because generateIndices
	// also checks for changes in the selection list (which we don't want to
	// have any influence at this point).

	SbBool dataChanged = FALSE;
	if (lastChemBaseDataElement == NULL) {
		dataChanged = TRUE;
	}
	else {
		const ChemBaseDataElement *cbde =
			ChemBaseDataElement::getInstance(state);
		if (!lastChemBaseDataElement->matches(cbde)) {
			dataChanged = TRUE;
		}
	}
	if (dataChanged) generateIndices(action);

	// Generate the appropriate primitives as determined from the display 
	// style.
	switch (displayStyle) {
		case ChemDisplayParam::DISPLAY_NONE:
			break;

		case ChemDisplayParam::DISPLAY_CPK:
			generateAtomsAsSpheres(action);
			break;

		case ChemDisplayParam::DISPLAY_STICK:
			generateBondsAsCylinders(action);
			break;

		case ChemDisplayParam::DISPLAY_BALLSTICK:
			generateBondsAsCylinders(action);
			generateAtomsAsSpheres(action);
			break;

		case ChemDisplayParam::DISPLAY_BALLWIRE:
			generateBondsAsWireframe(action);
			generateAtomsAsSpheres(action);
			break;

		case ChemDisplayParam::DISPLAY_WIREFRAME:
			generateBondsAsWireframe(action);
			break;

		default:
			break;
	}
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Computes the bounding box of the molecule based on the display
//	  style
//
// Use: protected

void
ChemDisplay::computeBBox(SoAction *action, SbBox3f &box,  SbVec3f &center)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::computeBBox\n");
#endif
	int32_t count = 0;
	SoState *state = action->getState();

	ChemBaseData *chemData = ChemBaseDataElement::get(state);
	globalNumberOfAtoms = chemData->getNumberOfAtoms();
	globalNumberOfBonds = chemData->getNumberOfBonds();
	localNumberOfAtoms = getNumberOfAtoms(action);
	localNumberOfBonds = getNumberOfBonds(action);
// --> CAstick
	ChemData* chemDataResidues = (ChemData*)chemData;
	if (chemDataResidues != NULL)
	{
		globalNumberOfResidues = chemDataResidues->residues.getNum();
		localNumberOfResidues = getNumberOfResidues(action);
	}
// <-- CAstick

	ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);
	int displayStyle = cdp->displayStyle.getValue();
	SbBool showAtomLabels = cdp->showAtomLabels.getValue();
	SbBool showBondLabels = cdp->showBondLabels.getValue();

	// If the data has changed since the last time we were in ChemDisplay,
	// then we might need to generate the multiple bond stuff.  We check
	// here whether or not to invoke generateIndices because generateIndices
	// also checks for changes in the selection list (which we don't want to
	// have any influence at this point).

	SbBool dataChanged = FALSE;
	if (lastChemBaseDataElement == NULL) {
		dataChanged = TRUE;
	}
	else {
		const ChemBaseDataElement *cbde =
			ChemBaseDataElement::getInstance(state);
		if (!lastChemBaseDataElement->matches(cbde)) {
			dataChanged = TRUE;
		}
	}
	if (dataChanged) generateIndices(action);

	SbBool cdpChanged = FALSE;
	const ChemDisplayParamElement *cdpe =
		ChemDisplayParamElement::getInstance(state);
	if (lastChemDisplayParamElement == NULL) {
		cdpChanged = TRUE;
		lastChemDisplayParamElement =
			(ChemDisplayParamElement *)cdpe->copyMatchInfo();
	}
	else {
		if (lastChemDisplayParamElement->matches(cdpe)) {
			cdpChanged = FALSE;
		}
		else {
			cdpChanged = TRUE;
			delete (SoElement *)lastChemDisplayParamElement;
			lastChemDisplayParamElement =
				(ChemDisplayParamElement *)cdpe->copyMatchInfo();
		}
	}

	// Check if the highlightAtomBitmap and nonBondedAtomBitmap are the
	// same
	if (cdpChanged) {
		bitmapsEqual = TRUE;
		if ((cdp->highlightAtomBitmapWidth != cdp->nonBondedAtomBitmapWidth) ||
			(cdp->highlightAtomBitmapHeight != cdp->nonBondedAtomBitmapHeight) ||
			(cdp->highlightAtomBitmap != cdp->nonBondedAtomBitmap)) {
			bitmapsEqual = FALSE;
		}
	}
#ifdef NOTYET
	SbBool radiiChanged = FALSE;
#endif
	const ChemRadiiElement *cre =
		ChemRadiiElement::getInstance(state);
	if (lastChemRadiiElement == NULL) {
#ifdef NOTYET
		radiiChanged = TRUE;
#endif
		lastChemRadiiElement =
			(ChemRadiiElement *)cre->copyMatchInfo();
	}
	else {
		if (lastChemRadiiElement->matches(cre)) {
#ifdef NOTYET
			radiiChanged = FALSE;
#endif
		}
		else {
#ifdef NOTYET
			radiiChanged = TRUE;
#endif
			delete (SoElement *)lastChemRadiiElement;
			lastChemRadiiElement =
				(ChemRadiiElement *)cre->copyMatchInfo();
		}
	}

#ifdef NOTYET
	if ((!dataChanged) && (!cdpChanged) && (!radiiChanged) &&
		(!lastComputeBBoxBox.isEmpty())) {
		center = lastComputeBBoxCenter;
		box = lastComputeBBoxBox;
		return;
	}
#endif

	// If we made it here, then something has changed which requires the
	// bounding box to be recomputed.
	center.setValue(0.0, 0.0, 0.0);
	box.makeEmpty();

	// Invoke the appropriate bBox as determined from the display style.
	switch (displayStyle) {
	  case ChemDisplayParam::DISPLAY_CPK:
		bBoxAtomsAsSpheres(state, cdp, count, box, center);
		if (showAtomLabels || showBondLabels) {
			calculateHemisphereRotation(state);
		}
		if (showAtomLabels)
			bBoxAtomLabelsWithSpheres(state, cdp, count, box, center);
		if (showBondLabels) 
			bBoxBondLabelsWithWireframe(state, cdp, count, box, center);
		break;

	  case ChemDisplayParam::DISPLAY_STICK:
		bBoxBondsAsCylinders(state, cdp, count, box, center);
		bBoxNone(state, cdp, count, box, center);
		if (showAtomLabels || showBondLabels) {
			calculateHemisphereRotation(state);
		}
		if (showAtomLabels)
			bBoxAtomLabelsWithCylinders(state, cdp, count, box, center);
		if (showBondLabels)
			bBoxBondLabelsWithCylinders(state, cdp, count, box, center);
		break;

	  case ChemDisplayParam::DISPLAY_BALLSTICK:
		bBoxBondsAsCylinders(state, cdp, count, box, center);
		bBoxAtomsAsSpheres(state, cdp, count, box, center);
		if (showAtomLabels || showBondLabels) {
			calculateHemisphereRotation(state);
		}
		if (showAtomLabels)
			bBoxAtomLabelsWithSpheres(state, cdp, count, box, center);
		if (showBondLabels)
			bBoxBondLabelsWithCylinders(state, cdp, count, box, center);
		break;

	  case ChemDisplayParam::DISPLAY_BALLWIRE:
		bBoxBondsAsWireframe(state, cdp, count, box, center);
		bBoxAtomsAsSpheres(state, cdp, count, box, center);
		if (showAtomLabels || showBondLabels) {
			calculateHemisphereRotation(state);
		}
		if (showAtomLabels)
			bBoxAtomLabelsWithSpheres(state, cdp, count, box, center);
		if (showBondLabels)
			bBoxBondLabelsWithWireframe(state, cdp, count, box, center);
		break;

	  case ChemDisplayParam::DISPLAY_WIREFRAME:
		bBoxBondsAsWireframe(state, cdp, count, box, center);
		bBoxNone(state, cdp, count, box, center);
		if (showAtomLabels)
			bBoxAtomLabelsWithWireframe(state, cdp, count, box, center);
		if (showBondLabels)
			bBoxBondLabelsWithWireframe(state, cdp, count, box, center);
		break;

	  case ChemDisplayParam::DISPLAY_NONE:
	  default:
		bBoxNone(state, cdp, count, box, center);
		if (showAtomLabels)
			bBoxAtomLabelsWithWireframe(state, cdp, count, box, center);
		if (showBondLabels)
			bBoxBondLabelsWithWireframe(state, cdp, count, box, center);
		break;
	}

// --> residue box
	// Invoke the appropriate bBox as determined from the display style.
	int residueDisplayStyle = cdp->residueDisplayStyle.getValue();
	switch (residueDisplayStyle)
	{
		case ChemDisplayParam::DISPLAY_RESIDUES_CAWIRE:
		case ChemDisplayParam::DISPLAY_RESIDUES_CASTICK:
		{
			bBoxResiduesAsCylinders(state, cdp, count, box, center);
			break;
		}

		case ChemDisplayParam::DISPLAY_RESIDUES_LINERIBBON:
		case ChemDisplayParam::DISPLAY_RESIDUES_FLATRIBBON:
		case ChemDisplayParam::DISPLAY_RESIDUES_SOLIDRIBBON:
		{
			bBoxResiduesAsRibbons(state, cdp, count ,box, center);
			break;
		}

		case ChemDisplayParam::DISPLAY_RESIDUES_SCHEMATIC:
		{
			bBoxResiduesAsSchematic(state, cdp, count, box, center);
			break;
		}

		case ChemDisplayParam::DISPLAY_RESIDUES_NONE:
		default:
		{
			bBoxResiduesNone(state, cdp, count, box, center);
			break;
		}

	}
	// TO DO: residue labels bounding box
// <-- residue box

	if (count > 0) {
		center /= (float)count;
	}

// --> octree culling
// (favoring first level culling)
	float factor = 0.25f;
	SbVec3f vdir=box.getMin()-center;	
	box.getMin()+=vdir*factor;
	box.getMax()-=vdir*factor;
// <-- octree culling

	lastComputeBBoxBox = box;
	lastComputeBBoxCenter = center;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Computes the bounding box of the atoms rendered as spheres.
//
// Use: private

void
ChemDisplay::bBoxAtomsAsSpheres(SoState *state, ChemDisplayParam *cdp,
	int32_t &count, SbBox3f &box, SbVec3f &center)
//
////////////////////////////////////////////////////////////////////////
{
	ChemRadii *chemRadii = ChemRadiiElement::get(state);

	switch (chemRadii->atomRadiiBinding.getValue()) {
	  case ChemRadii::RADII_OVERALL:
		bBoxAtomsAsSpheresRO(state, cdp, count, box, center);
		break;
	  case ChemRadii::RADII_PER_ATOM:
		bBoxAtomsAsSpheresRA(state, cdp, count, box, center);
		break;
	  case ChemRadii::RADII_PER_ATOM_INDEXED:
		bBoxAtomsAsSpheresRI(state, cdp, count, box, center);
		break;
	  default:
		break;
	}
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Computes the bounding box of the atoms.
//    Invoked by the computeBBox method.
//
// Use: private

void
ChemDisplay::
bBoxNone
	(SoState *state, ChemDisplayParam *cdp,
		int32_t &count, SbBox3f &box, SbVec3f &center)
//
////////////////////////////////////////////////////////////////////////
{
	float rad;
	SbVec3f tmpVec;
	ChemBaseData *chemData = ChemBaseDataElement::get(state);
	SbBool doHydrogens = cdp->showHydrogens.getValue();


	rad = 0.1f;
	tmpVec.setValue(rad, rad, rad);
	ATOMLOOP_START(atomIndex)
		if (!doHydrogens && chemData->getAtomicNumber(theAtom) == 1) {
			continue;
		}
		SbVec3f coord = chemData->getAtomCoordinates(theAtom);
		box.extendBy(coord + tmpVec);
		box.extendBy(coord - tmpVec);
		center += coord;
		count++;
	ATOMLOOP_END
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Computes the bounding box of the atoms.
//    Invoked by the computeBBox method.
//
// Use: private

void
ChemDisplay::
bBoxAtomsAsSpheresRO
	(SoState *state, ChemDisplayParam *cdp,
		int32_t &count, SbBox3f &box, SbVec3f &center)
//
////////////////////////////////////////////////////////////////////////
{
	float rad;
	SbVec3f tmpVec;
	ChemBaseData *chemData = ChemBaseDataElement::get(state);
	SbBool doHydrogens = cdp->showHydrogens.getValue();

	ChemRadii *chemRadii = ChemRadiiElement::get(state);
	float radScaleFactor;
	radScaleFactor = cdp->atomRadiiScaleFactor.getValue();
	if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
		(cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
		radScaleFactor *= cdp->ballStickSphereScaleFactor.getValue();
	}

	rad = chemRadii->atomRadii[0] * radScaleFactor;
	tmpVec.setValue(rad, rad, rad);
	ATOMLOOP_START(atomIndex)
		if (!doHydrogens && chemData->getAtomicNumber(theAtom) == 1) {
			continue;
		}
		SbVec3f coord = chemData->getAtomCoordinates(theAtom);
		box.extendBy(coord + tmpVec);
		box.extendBy(coord - tmpVec);
		center += coord;
		count++;
	ATOMLOOP_END
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Computes the bounding box of the atoms.
//    Invoked by the computeBBox method.
//
// Use: private

void
ChemDisplay::
bBoxAtomsAsSpheresRA
	(SoState *state, ChemDisplayParam *cdp,
		int32_t &count, SbBox3f &box, SbVec3f &center)
//
////////////////////////////////////////////////////////////////////////
{
	float rad;
	SbVec3f tmpVec;
	ChemBaseData *chemData = ChemBaseDataElement::get(state);
	SbBool doHydrogens = cdp->showHydrogens.getValue();

	ChemRadii *chemRadii = ChemRadiiElement::get(state);
	float radScaleFactor;
	radScaleFactor = cdp->atomRadiiScaleFactor.getValue();
	if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
		(cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
		radScaleFactor *= cdp->ballStickSphereScaleFactor.getValue();
	}

	ATOMLOOP_START(atomIndex)
		if (!doHydrogens && chemData->getAtomicNumber(theAtom) == 1) {
			continue;
		}
		rad = chemRadii->atomRadii[theAtom];
		rad *= radScaleFactor;
		tmpVec.setValue(rad, rad, rad);
		SbVec3f coord = chemData->getAtomCoordinates(theAtom);
		box.extendBy(coord + tmpVec);
		box.extendBy(coord - tmpVec);
		center += coord;
		count++;
	ATOMLOOP_END
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Computes the bounding box of the atoms.
//    Invoked by the computeBBox method.
//
// Use: private

void
ChemDisplay::
bBoxAtomsAsSpheresRI
	(SoState *state, ChemDisplayParam *cdp,
		int32_t &count, SbBox3f &box, SbVec3f &center)
//
////////////////////////////////////////////////////////////////////////
{
	float rad;
	SbVec3f tmpVec;
	ChemBaseData *chemData = ChemBaseDataElement::get(state);
	SbBool doHydrogens = cdp->showHydrogens.getValue();

	ChemRadii *chemRadii = ChemRadiiElement::get(state);
	float radScaleFactor;
	radScaleFactor = cdp->atomRadiiScaleFactor.getValue();
	if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
		(cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
		radScaleFactor *= cdp->ballStickSphereScaleFactor.getValue();
	}

	ATOMLOOP_START(atomIndex)
		if (!doHydrogens && chemData->getAtomicNumber(theAtom) == 1) {
			continue;
		}
		rad = chemRadii->atomRadii[chemData->getAtomIndex(theAtom)];
		rad *= radScaleFactor;
		tmpVec.setValue(rad, rad, rad);
		SbVec3f coord = chemData->getAtomCoordinates(theAtom);
		box.extendBy(coord + tmpVec);
		box.extendBy(coord - tmpVec);
		center += coord;
		count++;
	ATOMLOOP_END
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Computes the bounding box of the atoms.
//    Invoked by the computeBBox method.
//
// Use: private

void
ChemDisplay::
eachBBoxAtomsPickRadius
	(SoState *state, ChemDisplayParam *cdp,
		SbBool clipOnCenter, int32_t &count, ChemAtomBBox *atomBBoxes)
//
////////////////////////////////////////////////////////////////////////
{
	float rad;
	SbVec3f tmpVec;
	ChemBaseData *chemData = ChemBaseDataElement::get(state);
	//SbBool doHydrogens = cdp->showHydrogens.getValue();


	count = 0;
	SbBox3f tmpBox;
	const SoClipPlaneElement *clipPlaneElt =
		SoClipPlaneElement::getInstance(state);
	rad = cdp->atomPickRadius.getValue();
	tmpVec.setValue(rad, rad, rad);
	ATOMLOOP_START(atomIndex)
		SbVec3f coord = chemData->getAtomCoordinates(theAtom);
		if (clipOnCenter) {
			// Need to convert center to world coordinates for the clip plane
			// check (multiply by currentModelMatrix)
			SbVec3f center;
			currentModelMatrix.multVecMatrix(coord, center);
			if (isPointBetweenPlanes(center, currentViewVolume, clipPlaneElt)) {
				atomBBoxes->radius[count] = rad;
				atomBBoxes->center[count] = coord;
				atomBBoxes->index[count] = theAtom;
				count++;
			}
		}
		else {
			tmpBox.makeEmpty();
			tmpBox.extendBy(coord + tmpVec);
			tmpBox.extendBy(coord - tmpVec);
			tmpBox.transform(currentModelMatrix);
			// See if the tmpBox is clipped in any way
			if (isBoxBetweenPlanes(tmpBox, currentViewVolume, clipPlaneElt)) {
				atomBBoxes->radius[count] = rad;
				atomBBoxes->center[count] = coord;
				atomBBoxes->index[count] = theAtom;
				count++;
			}
		}
	ATOMLOOP_END
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Computes the bounding box of the atoms.
//    Invoked by the computeBBox method.
//
// Use: private

void
ChemDisplay::
eachBBoxAtomsAsSpheresRO
	(SoState *state, ChemDisplayParam *cdp,
		SbBool clipOnCenter, int32_t &count, ChemAtomBBox *atomBBoxes)
//
////////////////////////////////////////////////////////////////////////
{
	float rad;
	SbVec3f tmpVec;
	ChemBaseData *chemData = ChemBaseDataElement::get(state);
	//SbBool doHydrogens = cdp->showHydrogens.getValue();

	ChemRadii *chemRadii = ChemRadiiElement::get(state);
	float radScaleFactor;
	radScaleFactor = cdp->atomRadiiScaleFactor.getValue();
	if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
		(cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
		radScaleFactor *= cdp->ballStickSphereScaleFactor.getValue();
	}

	count = 0;
	SbBox3f tmpBox;
	const SoClipPlaneElement *clipPlaneElt =
		SoClipPlaneElement::getInstance(state);
	rad = chemRadii->atomRadii[0] * radScaleFactor;
	tmpVec.setValue(rad, rad, rad);
	ATOMLOOP_START(atomIndex)
		SbVec3f coord = chemData->getAtomCoordinates(theAtom);
		if (clipOnCenter) {
			// Need to convert center to world coordinates for the clip plane
			// check (multiply by currentModelMatrix)
			SbVec3f center;
			currentModelMatrix.multVecMatrix(coord, center);
			if (isPointBetweenPlanes(center, currentViewVolume, clipPlaneElt)) {
				atomBBoxes->radius[count] = rad;
				atomBBoxes->center[count] = coord;
				atomBBoxes->index[count] = theAtom;
				count++;
			}
		}
		else {
			tmpBox.makeEmpty();
			tmpBox.extendBy(coord + tmpVec);
			tmpBox.extendBy(coord - tmpVec);
			tmpBox.transform(currentModelMatrix);
			// See if the tmpBox is clipped in any way
			if (isBoxBetweenPlanes(tmpBox, currentViewVolume, clipPlaneElt)) {
				atomBBoxes->radius[count] = rad;
				atomBBoxes->center[count] = coord;
				atomBBoxes->index[count] = theAtom;
				count++;
			}
		}
	ATOMLOOP_END
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Computes the bounding box of the atoms.
//    Invoked by the computeBBox method.
//
// Use: private

void
ChemDisplay::
eachBBoxAtomsAsSpheresRA
	(SoState *state, ChemDisplayParam *cdp,
		SbBool clipOnCenter, int32_t &count, ChemAtomBBox *atomBBoxes)
//
////////////////////////////////////////////////////////////////////////
{
	float rad;
	SbVec3f tmpVec;
	ChemBaseData *chemData = ChemBaseDataElement::get(state);
	//SbBool doHydrogens = cdp->showHydrogens.getValue();

	ChemRadii *chemRadii = ChemRadiiElement::get(state);
	float radScaleFactor;
	radScaleFactor = cdp->atomRadiiScaleFactor.getValue();
	if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
		(cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
		radScaleFactor *= cdp->ballStickSphereScaleFactor.getValue();
	}

	count = 0;
	SbBox3f tmpBox;
	const SoClipPlaneElement *clipPlaneElt =
		SoClipPlaneElement::getInstance(state);
	ATOMLOOP_START(atomIndex)
		rad = chemRadii->atomRadii[theAtom];
		rad *= radScaleFactor;
		tmpVec.setValue(rad, rad, rad);
		SbVec3f coord = chemData->getAtomCoordinates(theAtom);
		if (clipOnCenter) {
			// Need to convert center to world coordinates for the clip plane
			// check (multiply by currentModelMatrix)
			SbVec3f center;
			currentModelMatrix.multVecMatrix(coord, center);
			if (isPointBetweenPlanes(center, currentViewVolume, clipPlaneElt)) {
				atomBBoxes->radius[count] = rad;
				atomBBoxes->center[count] = coord;
				atomBBoxes->index[count] = theAtom;
				count++;
			}
		}
		else {
			tmpBox.makeEmpty();
			tmpBox.extendBy(coord + tmpVec);
			tmpBox.extendBy(coord - tmpVec);
			tmpBox.transform(currentModelMatrix);
			// See if the tmpBox is clipped in any way
			if (isBoxBetweenPlanes(tmpBox, currentViewVolume, clipPlaneElt)) {
				atomBBoxes->radius[count] = rad;
				atomBBoxes->center[count] = coord;
				atomBBoxes->index[count] = theAtom;
				count++;
			}
		}
	ATOMLOOP_END
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Computes the bounding box of the atoms.
//    Invoked by the computeBBox method.
//
// Use: private

void
ChemDisplay::
eachBBoxAtomsAsSpheresRI
	(SoState *state, ChemDisplayParam *cdp,
		SbBool clipOnCenter, int32_t &count, ChemAtomBBox *atomBBoxes)
//
////////////////////////////////////////////////////////////////////////
{
	float rad;
	SbVec3f tmpVec;
	ChemBaseData *chemData = ChemBaseDataElement::get(state);
	//SbBool doHydrogens = cdp->showHydrogens.getValue();

	ChemRadii *chemRadii = ChemRadiiElement::get(state);
	float radScaleFactor;
	radScaleFactor = cdp->atomRadiiScaleFactor.getValue();
	if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)||
		(cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) {
		radScaleFactor *= cdp->ballStickSphereScaleFactor.getValue();
	}

	count = 0;
	SbBox3f tmpBox;
	const SoClipPlaneElement *clipPlaneElt =
		SoClipPlaneElement::getInstance(state);
	ATOMLOOP_START(atomIndex)
		rad = chemRadii->atomRadii[chemData->getAtomIndex(theAtom)];
		rad *= radScaleFactor;
		tmpVec.setValue(rad, rad, rad);
		SbVec3f coord = chemData->getAtomCoordinates(theAtom);
		if (clipOnCenter) {
			// Need to convert center to world coordinates for the clip plane
			// check (multiply by currentModelMatrix)
			SbVec3f center;
			currentModelMatrix.multVecMatrix(coord, center);
			if (isPointBetweenPlanes(center, currentViewVolume, clipPlaneElt)) {
				atomBBoxes->radius[count] = rad;
				atomBBoxes->center[count] = coord;
				atomBBoxes->index[count] = theAtom;
				count++;
			}
		}
		else {
			tmpBox.makeEmpty();
			tmpBox.extendBy(coord + tmpVec);
			tmpBox.extendBy(coord - tmpVec);
			tmpBox.transform(currentModelMatrix);
			// See if the tmpBox is clipped in any way
			if (isBoxBetweenPlanes(tmpBox, currentViewVolume, clipPlaneElt)) {
				atomBBoxes->radius[count] = rad;
				atomBBoxes->center[count] = coord;
				atomBBoxes->index[count] = theAtom;
				count++;
			}
		}
	ATOMLOOP_END
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Computes the bounding box of the bonds.
//    Invoked by the computeBBox method.
//
// Use: private

void
ChemDisplay::bBoxBondsAsCylinders(SoState *state, ChemDisplayParam *cdp,
	int32_t &count, SbBox3f &box, SbVec3f &center)
//
////////////////////////////////////////////////////////////////////////
{
	int32_t from, to;
	float rad;
	ChemBaseData *chemData = ChemBaseDataElement::get(state);
	SbBool doHydrogens = cdp->showHydrogens.getValue();

	rad = cdp->bondCylinderRadius.getValue();

	SbVec3f tmpVec;
	tmpVec.setValue(rad, rad, rad);
	BONDLOOP_START(bondIndex)
		from = chemData->getBondFrom(theBond);
		to = chemData->getBondTo(theBond);
		if (!doHydrogens) {
			if ((chemData->getAtomicNumber(from) == 1) ||
				(chemData->getAtomicNumber(to) == 1)) continue;
		}
		SbVec3f coords1 = chemData->getAtomCoordinates(from);
		SbVec3f coords2 = chemData->getAtomCoordinates(to);
		box.extendBy(coords1 + tmpVec);
		box.extendBy(coords1 - tmpVec);
		center += coords1;
		count++;
		box.extendBy(coords2 + tmpVec);
		box.extendBy(coords2 - tmpVec);
		center += coords2;
		count++;
	BONDLOOP_END
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Computes the bounding box of the bonds.
//    Invoked by the computeBBox method.
//
// Use: private

void
ChemDisplay::eachBBoxBondsAsCylinders(SoState *state, ChemDisplayParam *cdp,
	SbBool clipOnCenter, int32_t &bCount, ChemStickBondBBox *bondBBoxes)
//
////////////////////////////////////////////////////////////////////////
{
	int32_t from, to;
	float rad;
	ChemBaseData *chemData = ChemBaseDataElement::get(state);
	//SbBool doHydrogens = cdp->showHydrogens.getValue();
	const SoClipPlaneElement *clipPlaneElt =
		SoClipPlaneElement::getInstance(state);

	rad = cdp->bondCylinderRadius.getValue();

	// The individual bondBBoxes are for use in ChemSelection.  The following
	// code constructs a unit cylinder from 0 to 1 along the Z axis.
	// The matrix which takes this cylinder to the proper position for the
	// bond is found.  The current camera position is transformed into the
	// coordinate space of the unit cylinder by using the inverse of the
	// cylinder matrix and the current view matrix.  We can then determine
	// the "silhouette points".  These points are then transformed using
	// the cylinder matrix.  This idea is taken from Jim Blinn's "Optimal
	// Tubes" article from Jim Blinn's Corner in IEEE CG&A, 1989.

	bCount = 0;
	SbVec3f     theTranslation;
	SbRotation  theRotation;
	SbVec3f     theScale;
	SbMatrix    theMatrix;
	SbMatrix    eyeMatrix;
	SbMatrix    eyeMatrixInverse;
	SbVec3f     eyePoint;
	SbVec3f     yAxis(0.0, 1.0, 0.0);
	SbVec3f     zAxis(0.0, 0.0, 1.0);
	SbVec3f		coords1, coords2, tcoord;
	float		height;
	float		eyePointDistance, eyePointDistanceRecip;
	float		eyePointSqr;
	Silhouette	tmpS;

	BONDLOOP_START(bondIndex)
		from = chemData->getBondFrom(theBond);
		to = chemData->getBondTo(theBond);
		coords1 = chemData->getAtomCoordinates(from);
		coords2 = chemData->getAtomCoordinates(to);
		tcoord = coords2 - coords1;
		height = tcoord.length();
		bondBBoxes->center[bCount] = tcoord * 0.5f + coords1;

		theTranslation = coords1;
		theRotation.setValue(zAxis, tcoord);
		theScale.setValue(rad, rad, height);
		theMatrix.setTransform(theTranslation, theRotation, theScale);

		// Set the eyeMatrix.  We don't include the currentModelMatrix.
		// This is because we want the silhouette points to be in the
		// same space as the coordinates in ChemData.
		eyeMatrix = theMatrix;
		eyeMatrix.multLeft(currentViewMatrix);
		eyeMatrixInverse = eyeMatrix.inverse();

		// Transform the eyePoint
		eyePoint = currentViewVolume.getProjectionPoint();
		eyeMatrixInverse.multVecMatrix(eyePoint, eyePoint);
		eyePoint[2] = 0.0;

		// Determine the silhouette points
		tmpS[0][2] = tmpS[1][2] = 0.0;
		eyePointDistance =
			eyePoint[0]*eyePoint[0] + eyePoint[1]*eyePoint[1];

		// If the distance from the eyePoint to the origin (eyePointDistance)
		// is less than 1, then we are in the unit cylinder.  Set the
		// silhouette points to 0, 1, 0 and 0, -1, 0.
	if (eyePointDistance < 1.0) {
			tmpS[0][0] = tmpS[1][0] = 0.0;
			tmpS[0][1] =  1.0;
			tmpS[1][1] = -1.0;
	}
	else {
			eyePointDistanceRecip = 1.0 / eyePointDistance;
			if (eyePointDistance == 1.0) {
				// Here the eyePoint is right on the edge of the cylinder
				tmpS[0][0] = tmpS[1][0] = eyePoint[0] * eyePointDistanceRecip;
				tmpS[0][1] = tmpS[1][1] = eyePoint[1] * eyePointDistanceRecip;;
			}
			else {
				eyePointSqr = sqrtf(eyePointDistance - 1.0);
				tmpS[0][0] = (eyePoint[0] - eyePoint[1]*eyePointSqr)
						* eyePointDistanceRecip;
				tmpS[1][0] = (eyePoint[0] + eyePoint[1]*eyePointSqr)
						* eyePointDistanceRecip;
				tmpS[0][1] = (eyePoint[1] + eyePoint[0]*eyePointSqr)
						* eyePointDistanceRecip;
				tmpS[1][1] = (eyePoint[1] - eyePoint[0]*eyePointSqr)
						* eyePointDistanceRecip;
			}
		}
		tmpS[2] = tmpS[0];
		tmpS[3] = tmpS[1];
		tmpS[2][2] = tmpS[3][2] = 1.0;

		theMatrix.multVecMatrix(tmpS[0], bondBBoxes->silhouette[bCount][0]);
		theMatrix.multVecMatrix(tmpS[2], bondBBoxes->silhouette[bCount][1]);
		theMatrix.multVecMatrix(tmpS[3], bondBBoxes->silhouette[bCount][2]);
		theMatrix.multVecMatrix(tmpS[1], bondBBoxes->silhouette[bCount][3]);

		// Check if silhouette is between clipping planes.  If not, then
		// don't increment bCount.  First transform silhouette into world
		// coordinates.
		currentModelMatrix.multVecMatrix(bondBBoxes->silhouette[bCount][0],
			tmpS[0]);
		currentModelMatrix.multVecMatrix(bondBBoxes->silhouette[bCount][1],
			tmpS[1]);
		currentModelMatrix.multVecMatrix(bondBBoxes->silhouette[bCount][2],
			tmpS[2]);
		currentModelMatrix.multVecMatrix(bondBBoxes->silhouette[bCount][3],
			tmpS[3]);

		SbBool addBBox = FALSE;
		if (clipOnCenter) {
			// Need to convert center to world coordinates for the clip plane
			// check (multiply by currentModelMatrix)
			SbVec3f center;
			currentModelMatrix.multVecMatrix(bondBBoxes->center[bCount],
				center);
			if (isPointBetweenPlanes(center, currentViewVolume,
				clipPlaneElt)) {
				addBBox = TRUE;
			}
		}
		else {
			if (isSilhouetteBetweenPlanes(tmpS, currentViewVolume, clipPlaneElt)) {
				addBBox = TRUE;
			}
		}
		if (addBBox) {
			height *= 0.5;
			theTranslation = tcoord * 0.5f + coords1;
			theRotation.setValue(yAxis, tcoord);
			theScale.setValue(rad, height, rad);
			theMatrix.setTransform(theTranslation, theRotation, theScale);

			bondBBoxes->transform[bCount] = theMatrix;
			bondBBoxes->index[bCount] = theBond;
			bCount++;
		}
	BONDLOOP_END
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Calculates the contribution to the total bounding box made by
//    the bonds displayed as wireframe.
//
// Use: private

void
ChemDisplay::bBoxBondsAsWireframe(SoState *state, ChemDisplayParam *cdp,
	int32_t &count, SbBox3f &box, SbVec3f &center)
//
////////////////////////////////////////////////////////////////////////
{
	int32_t i, from, to;
	ChemBaseData *chemData = ChemBaseDataElement::get(state);
	SbBool doHydrogens = cdp->showHydrogens.getValue();
	SbBool showMulti   = cdp->showMultipleBonds.getValue();
	float  multiSep    = cdp->multipleBondSeparation.getValue(); 
	float  multiSep2   = multiSep * 0.5;
	//SbVec3f coords1, coords2, tmpVec;
	SbVec3f coords1, coords2;

	for (i = 0; i < localNumberOfBonds; i++) {
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
				coords1 += (tsep + tsep2);
				coords2 -= (tsep + tsep2);
				box.extendBy(coords1);
				box.extendBy(coords2);
				center += coords1;
				center += coords2;
				count += 2;
			}
			else if (bondAttributes[i].type == ChemBaseData::TRIPLE_BOND) {
				coords1 += tsep;
				coords2 -= tsep;
				box.extendBy(coords1);
				box.extendBy(coords2);
				center += coords1;
				center += coords2;
				count += 2;
			}
			else if ((bondAttributes[i].type == ChemBaseData::DOUBLE_BOND) ||
					 (bondAttributes[i].type == ChemBaseData::RESONANCE_BOND)) {
				if (bondAttributes[i].isSymmetric == FALSE) {
					coords2 -= tsep;
					box.extendBy(coords1);
					box.extendBy(coords2);
					center += coords1;
					center += coords2;
					count += 2;
				}
				else {
					coords1 += tsep2;
					coords2 -= tsep2;
					box.extendBy(coords1);
					box.extendBy(coords2);
					center += coords1;
					center += coords2;
					count += 2;
				}
			}
			else {
				box.extendBy(coords1);
				box.extendBy(coords2);
				center += coords1;
				center += coords2;
				count += 2;
			}
		}
		else {
			box.extendBy(coords1);
			box.extendBy(coords2);
			center += coords1;
			center += coords2;
			count += 2;
		}
	}
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Calculates the bounding box of each box when displayed as wireframe.
//
// Use: private

void
ChemDisplay::eachBBoxBondsAsWireframe(SoState *state, ChemDisplayParam *cdp,
	SbBool clipOnCenter, int32_t &bCount, ChemWireframeBondBBox *bondBBoxes)
//
////////////////////////////////////////////////////////////////////////
{
	int32_t i;
	int32_t from, to;
	ChemBaseData *chemData = ChemBaseDataElement::get(state);
	//SbBool doHydrogens = cdp->showHydrogens.getValue();
	SbBool showMulti   = cdp->showMultipleBonds.getValue();
	float  multiSep    = cdp->multipleBondSeparation.getValue(); 
	float  multiSep2   = multiSep * 0.5;
	Silhouette tmpS;

	bCount = 0;
	const SoClipPlaneElement *clipPlaneElt =
		SoClipPlaneElement::getInstance(state);

	for (i = 0; i < localNumberOfBonds; i++) {
		from = bondAttributes[i].from;
		to = bondAttributes[i].to;

		SbVec3f coords1, coords2, tcoord;
		coords1 = chemData->getAtomCoordinates(from);
		coords2 = chemData->getAtomCoordinates(to);
		tcoord = coords2 - coords1;
		bondBBoxes->center[bCount] = tcoord * 0.5f + coords1;

		if (showMulti) {
			SbVec3f tsep = bondAttributes[i].perpNormal * multiSep;
			SbVec3f tsep2 = bondAttributes[i].perpNormal * multiSep2;

			if (bondAttributes[i].type == ChemBaseData::QUADRUPLE_BOND) {
				SbVec3f totalSep = tsep + tsep2;
				bondBBoxes->silhouette[bCount][0] = coords1 - totalSep;
				bondBBoxes->silhouette[bCount][1] = coords2 - totalSep;
				bondBBoxes->silhouette[bCount][2] = coords2 + totalSep;
				bondBBoxes->silhouette[bCount][3] = coords1 + totalSep;
				bondBBoxes->bondType[bCount] = ChemBaseData::QUADRUPLE_BOND;
			}
			else if (bondAttributes[i].type == ChemBaseData::TRIPLE_BOND) {
				bondBBoxes->silhouette[bCount][0] = coords1 - tsep;
				bondBBoxes->silhouette[bCount][1] = coords2 - tsep;
				bondBBoxes->silhouette[bCount][2] = coords2 + tsep;
				bondBBoxes->silhouette[bCount][3] = coords1 + tsep;
				bondBBoxes->bondType[bCount] = ChemBaseData::TRIPLE_BOND;
			}
			else if ((bondAttributes[i].type == ChemBaseData::DOUBLE_BOND) ||
					 (bondAttributes[i].type == ChemBaseData::RESONANCE_BOND)) {
				if (bondAttributes[i].isSymmetric == FALSE) {
					bondBBoxes->silhouette[bCount][0] = coords1;
					bondBBoxes->silhouette[bCount][1] = coords2;
					bondBBoxes->silhouette[bCount][2] = coords2 + tsep;
					bondBBoxes->silhouette[bCount][3] = coords1 + tsep;
				}
				else {
					bondBBoxes->silhouette[bCount][0] = coords1 - tsep2;
					bondBBoxes->silhouette[bCount][1] = coords2 - tsep2;
					bondBBoxes->silhouette[bCount][2] = coords2 + tsep2;
					bondBBoxes->silhouette[bCount][3] = coords1 + tsep2;
				}
				bondBBoxes->bondType[bCount] = ChemBaseData::DOUBLE_BOND;
			}
			else {
				bondBBoxes->silhouette[bCount][0] = coords1;
				bondBBoxes->silhouette[bCount][1] = coords2;
				bondBBoxes->silhouette[bCount][2] = coords2;
				bondBBoxes->silhouette[bCount][3] = coords1;
				bondBBoxes->bondType[bCount] = ChemBaseData::SINGLE_BOND;
			}
		}
		else {
			bondBBoxes->silhouette[bCount][0] = coords1;
			bondBBoxes->silhouette[bCount][1] = coords2;
			bondBBoxes->silhouette[bCount][2] = coords2;
			bondBBoxes->silhouette[bCount][3] = coords1;
			bondBBoxes->bondType[bCount] = ChemBaseData::SINGLE_BOND;
		}

		if (clipOnCenter) {
			// Need to convert center to world coordinates for the clip plane
			// check (multiply by currentModelMatrix)
			SbVec3f center;
			currentModelMatrix.multVecMatrix(bondBBoxes->center[bCount],
				center);
			if (isPointBetweenPlanes(center, currentViewVolume,
				clipPlaneElt)) {
				bondBBoxes->index[bCount] = bondAttributes[i].index;
				bCount++;
			}
		}
		else {
			// Check if silhouette is between clipping planes.  If not, then
			// don't increment bCount.  First transform silhouette into world
			// coordinates.
			currentModelMatrix.multVecMatrix(bondBBoxes->silhouette[bCount][0],
				tmpS[0]);
			currentModelMatrix.multVecMatrix(bondBBoxes->silhouette[bCount][1],
				tmpS[1]);
			currentModelMatrix.multVecMatrix(bondBBoxes->silhouette[bCount][2],
				tmpS[2]);
			currentModelMatrix.multVecMatrix(bondBBoxes->silhouette[bCount][3],
				tmpS[3]);
			if (isSilhouetteBetweenPlanes(tmpS, currentViewVolume, clipPlaneElt)) {
				bondBBoxes->index[bCount] = bondAttributes[i].index;
				bCount++;
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Return the number of atoms this node will display.
//
// Use: public

int32_t
ChemDisplay::getNumberOfAtoms(SoAction *action) const
//
////////////////////////////////////////////////////////////////////////
{
	ChemBaseData *chemData = ChemBaseDataElement::get(action->getState());
	if (chemData == NULL) {
		return 0;
	}

	int32_t sum = 0;
	numAtomLoops = atomIndex.getNum();
	for (atomLoop = 0; atomLoop < numAtomLoops; atomLoop++) {
		atomIndex[atomLoop].getValue(atomStart, atomEnd);
		if (atomEnd == CHEM_DISPLAY_USE_REST_OF_ATOMS) {
			sum += globalNumberOfAtoms - atomStart;
		}
		else {
			sum += atomEnd;
		}
	}
	return sum;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Return the number of bonds this node will display.
//
// Use: public

int32_t
ChemDisplay::getNumberOfBonds(SoAction *action) const
//
////////////////////////////////////////////////////////////////////////
{
	ChemBaseData *chemData = ChemBaseDataElement::get(action->getState());
	if (chemData == NULL) {
		return 0;
	}

	int32_t sum = 0;
	numBondLoops = bondIndex.getNum();
	for (bondLoop = 0; bondLoop < numBondLoops; bondLoop++) {
		bondIndex[bondLoop].getValue(bondStart, bondEnd);
		if (bondEnd == CHEM_DISPLAY_USE_REST_OF_ATOMS) {
		   sum += globalNumberOfBonds - bondStart;
		}
		else {
		   sum += bondEnd;
		}
	}
	return sum;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Return the number of atom labels this node will display.
//
// Use: public

int32_t
ChemDisplay::getNumberOfAtomLabels(SoAction *action) const
//
////////////////////////////////////////////////////////////////////////
{
	ChemBaseData *chemData = ChemBaseDataElement::get(action->getState());
	if (chemData == NULL) {
		return 0;
	}

	int32_t sum = 0;
	numAtomLoops = atomLabelIndex.getNum();
	for (atomLoop = 0; atomLoop < numAtomLoops; atomLoop++) {
		atomLabelIndex[atomLoop].getValue(atomStart, atomEnd);
		if (atomEnd == CHEM_DISPLAY_USE_REST_OF_ATOMS) {
		   sum += globalNumberOfAtoms - atomStart;
		}
		else {
		   sum += atomEnd;
		}
	}
	return sum;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Return the number of bond labels this node will display.
//
// Use: public

int32_t
ChemDisplay::getNumberOfBondLabels(SoAction *action) const
//
////////////////////////////////////////////////////////////////////////
{
	ChemBaseData *chemData = ChemBaseDataElement::get(action->getState());
	if (chemData == NULL) {
		return 0;
	}

	int32_t sum = 0;
	numBondLoops = bondLabelIndex.getNum();
	for (bondLoop = 0; bondLoop < numBondLoops; bondLoop++) {
		bondLabelIndex[bondLoop].getValue(bondStart, bondEnd);
		if (bondEnd == CHEM_DISPLAY_USE_REST_OF_ATOMS) {
		   sum += globalNumberOfBonds - bondStart;
		}
		else {
		   sum += bondEnd;
		}
	}
	return sum;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Computes the individual bounding boxes of the atoms based on the
//	  display style.  Actually just returns the radii and the centers
//    since the bounding box can be determined from these.  This is
//    invoked by ChemBBoxAction.
//
// Use: public

void
ChemDisplay::getAtomBBoxes(SoAction *action, SbBool clipOnCenter,
	ChemAtomBBox *&atomBBoxes)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::getAtomBBoxes\n");
#endif
	int32_t numAtoms;
	atomBBoxes = NULL;

	if ((numAtoms = getNumberOfAtoms(action)) == 0) {
		return;
	}

	SoState *state = action->getState();
	if ((int)SoPickStyleElement::get(state) ==
            (int)SoPickStyle::UNPICKABLE) {
		return;
	}

	ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);
	ChemRadii *chemRadii = ChemRadiiElement::get(state);

	int displayStyle = cdp->displayStyle.getValue();
	if (displayStyle == ChemDisplayParam::DISPLAY_NONE) return;

	// Create the unitSphere if it has not been done already.
	if (unitSphere == NULL) {
		lastSphereComplexity = cdp->atomSphereComplexity.getValue();
		unitSphere = new ChemUnitSphere(lastSphereComplexity);
	}

	int32_t count = 0;
	ChemAtomBBox *tmpBBoxes = new ChemAtomBBox(numAtoms);

	switch (displayStyle) {
	  case ChemDisplayParam::DISPLAY_CPK:
	  case ChemDisplayParam::DISPLAY_BALLSTICK:
	  case ChemDisplayParam::DISPLAY_BALLWIRE:

		// Invoke the appropriate sphere bBox as determined from the 
		// display style.
		switch (chemRadii->atomRadiiBinding.getValue()) {
		  case ChemRadii::RADII_OVERALL:
			eachBBoxAtomsAsSpheresRO(state, cdp, clipOnCenter, count, tmpBBoxes);
			break;
		  case ChemRadii::RADII_PER_ATOM:
			eachBBoxAtomsAsSpheresRA(state, cdp, clipOnCenter, count, tmpBBoxes);
			break;
		  case ChemRadii::RADII_PER_ATOM_INDEXED:
			eachBBoxAtomsAsSpheresRI(state, cdp, clipOnCenter, count, tmpBBoxes);
			break;
		  default:
			break;
		}
		break;

	  case ChemDisplayParam::DISPLAY_WIREFRAME:
	  case ChemDisplayParam::DISPLAY_STICK:
		eachBBoxAtomsPickRadius(state, cdp, clipOnCenter, count, tmpBBoxes);
		break;
	}

	if (count == numAtoms) {
		atomBBoxes = tmpBBoxes;
	}
	else {
		atomBBoxes = new ChemAtomBBox(*tmpBBoxes, count);
		delete tmpBBoxes;
	}
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Computes the individual bounding boxes of the bonds based on the
//	  display style
//
// Use: public

void
ChemDisplay::getBondBBoxes(SoAction *action, SbBool clipOnCenter,
	ChemBondBBox *&bondBBoxes)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::getBondBBoxes\n");
#endif
	int32_t i;
	int32_t numBonds, count;
	bondBBoxes = NULL;

	if ((numBonds = getNumberOfBonds(action)) == 0) {
		return;
	}

	SoState *state = action->getState();
	if ((int)SoPickStyleElement::get(state) == 
            (int)SoPickStyle::UNPICKABLE) {
		return;
	}

	ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

	int displayStyle = cdp->displayStyle.getValue();

	if ((displayStyle == ChemDisplayParam::DISPLAY_CPK) ||
		(displayStyle == ChemDisplayParam::DISPLAY_NONE)) {
		return;
	}

	count = 0;
        //gcc don't like initializations inside the switch loop (MPM 5/12/2000)
        //the above patch introduced some bugs, fixed by MPM and CHA 6/03/2002
	ChemWireframeBondBBox *wireBBox;
	ChemWireframeBondBBox *tmpBBox;
	int32_t numVertices;
	const SbVec2f *vert2;
	ChemStickBondBBox *stickBBox;
	ChemStickBondBBox *tmpSBBox;

        // Invoke the appropriate bBox as determined from the display style.
	switch (displayStyle) {
	  case ChemDisplayParam::DISPLAY_WIREFRAME:
	  case ChemDisplayParam::DISPLAY_BALLWIRE:
	        wireBBox = new ChemWireframeBondBBox(numBonds);

		eachBBoxBondsAsWireframe(state, cdp, clipOnCenter, count, wireBBox);
		if (count == numBonds) {
			bondBBoxes = wireBBox;
		}
		else {
			tmpBBox = new ChemWireframeBondBBox(*wireBBox, count);
			bondBBoxes = tmpBBox;
			delete wireBBox;
		}
		break;

	  case ChemDisplayParam::DISPLAY_STICK:
	  case ChemDisplayParam::DISPLAY_BALLSTICK:
		if (unitCylinder == NULL) {
			float bcc = cdp->bondCylinderComplexity.getValue();
			int cylDisplayStyle = cdp->bondCylinderDisplayStyle.getValue();
			lastCylinderComplexity = bcc;
			lastCylinderDisplayStyle = cylDisplayStyle;
			unitCylinder = new ChemUnitCylinder(cylDisplayStyle, bcc);
		}
	        numVertices = unitCylinder->getNumberOfRingCoords();
	        vert2 = unitCylinder->getRingCoords();
	        stickBBox = new ChemStickBondBBox(numBonds);
		stickBBox->numberOfVertices = numVertices;
		stickBBox->vertices = new SbVec3f[numVertices];
		// FIX for MSVC++	Daniel Soto	2002-02-07
		//for (int32_t i = 0; i < numVertices; i++) {
		for (i = 0; i < numVertices; i++) {
			stickBBox->vertices[i].setValue(vert2[i][0], 0.0, vert2[i][1]);
		}

		eachBBoxBondsAsCylinders(state, cdp, clipOnCenter, count, stickBBox);
		if (count == numBonds) {
			bondBBoxes = stickBBox;
		}
		else {
			tmpSBBox = new ChemStickBondBBox(*stickBBox, count);
			bondBBoxes = tmpSBBox;
			delete stickBBox;
		}
		break;

	  default:
		break;
	}
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Computes the individual bounding boxes of the atom labels based on
//	  the display style
//
// Use: public

void
ChemDisplay::getAtomLabelBBoxes(SoAction *action, SbBool clipOnCenter,
	ChemLabelBBox *&atomLabelBBoxes)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::getAtomLabelBBoxes\n");
#endif
	int32_t count;
	int32_t numberOfAtomLabels;
	atomLabelBBoxes = NULL;

	if ((numberOfAtomLabels = getNumberOfAtomLabels(action)) == 0) {
		return;
	}

	SoState *state = action->getState();
	if ((int)SoPickStyleElement::get(state) ==
            (int)SoPickStyle::UNPICKABLE) {
		return;
	}

	ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);
	ChemRadii *chemRadii = ChemRadiiElement::get(state);

	int displayStyle = cdp->displayStyle.getValue();

	if (displayStyle == ChemDisplayParam::DISPLAY_NONE) return;
	if (cdp->showAtomLabels.getValue() == FALSE) return;

	ChemLabelBBox *tmpBBox = new ChemLabelBBox(numberOfAtomLabels);

	count = 0;
	// Invoke the appropriate bBox as determined from the display style.
	switch (displayStyle) {
	  case ChemDisplayParam::DISPLAY_CPK:
	  case ChemDisplayParam::DISPLAY_BALLSTICK:
	  case ChemDisplayParam::DISPLAY_BALLWIRE:
		switch (chemRadii->atomRadiiBinding.getValue()) {
		  case ChemRadii::RADII_OVERALL:
			eachBBoxAtomLabelsWithSpheresRO(state, cdp, clipOnCenter, count, tmpBBox);
			break;
		  case ChemRadii::RADII_PER_ATOM:
			eachBBoxAtomLabelsWithSpheresRA(state, cdp, clipOnCenter, count, tmpBBox);
			break;
		  case ChemRadii::RADII_PER_ATOM_INDEXED:
			eachBBoxAtomLabelsWithSpheresRI(state, cdp, clipOnCenter, count, tmpBBox);
			break;
		  default:
			break;
		}
		break;

	  case ChemDisplayParam::DISPLAY_WIREFRAME:
		eachBBoxAtomLabelsWithWireframe(state, cdp, clipOnCenter, count, tmpBBox);
		break;

	  case ChemDisplayParam::DISPLAY_STICK:
		eachBBoxAtomLabelsWithCylinders(state, cdp, clipOnCenter, count, tmpBBox);
		break;

	  default:
		break;
	}

	if (count == numberOfAtomLabels) {
		atomLabelBBoxes = tmpBBox;
	}
	else {
		atomLabelBBoxes = new ChemLabelBBox(*tmpBBox, count);
		delete tmpBBox;
	}
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Computes the individual bounding boxes of the bond labels based on
//	  the display style
//
// Use: public

void
ChemDisplay::getBondLabelBBoxes(SoAction *action, SbBool clipOnCenter,
	ChemLabelBBox *&bondLabelBBoxes)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::getBondLabelBBoxes\n");
#endif
	int32_t count;
	int32_t numberOfBondLabels;
	bondLabelBBoxes = NULL;

	if ((numberOfBondLabels = getNumberOfBondLabels(action)) == 0) {
		return;
	}

	SoState *state = action->getState();
	if ((int)SoPickStyleElement::get(state) ==
            (int)SoPickStyle::UNPICKABLE) {
		return;
	}

	ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

	int displayStyle = cdp->displayStyle.getValue();

	if (displayStyle == ChemDisplayParam::DISPLAY_NONE) return;
	if (cdp->showBondLabels.getValue() == FALSE) return;

	ChemLabelBBox *tmpBBox = new ChemLabelBBox(numberOfBondLabels);

	count = 0;
	// Invoke the appropriate bBox as determined from the display style.
	switch (displayStyle) {
	  case ChemDisplayParam::DISPLAY_WIREFRAME:
	  case ChemDisplayParam::DISPLAY_BALLWIRE:
		eachBBoxBondLabelsWithWireframe(state, cdp, clipOnCenter, count, tmpBBox);

	  case ChemDisplayParam::DISPLAY_STICK:
	  case ChemDisplayParam::DISPLAY_BALLSTICK:
		eachBBoxBondLabelsWithCylinders(state, cdp, clipOnCenter, count, tmpBBox);

	  default:
		break;
	}
	if (count == numberOfBondLabels) {
		bondLabelBBoxes = tmpBBox;
	}
	else {
		bondLabelBBoxes = new ChemLabelBBox(*tmpBBox, count);
		delete tmpBBox;
	}
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Highlight selected atoms when in wireframe mode.
//
// Use: private

void
ChemDisplay::highlightAtomsWithWireframe(SoGLRenderAction *action)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf("In ChemDisplay::highlightAtomsWithWireframe\n");
#endif
	if (highlightAtomIndex.getNum() == 0) return;

	SoState *state = action->getState();
	ChemBaseData *chemData = ChemBaseDataElement::get(state);
	ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

	if (cdp->highlightStyle.getValue() ==
		ChemDisplayParam::HIGHLIGHT_NONE) return;

	if (cdp->highlightAtomStyle.getValue() ==
		ChemDisplayParam::HIGHLIGHT_ATOM_NONE) return;

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

		GLint curAlignment;
		glGetIntegerv(GL_UNPACK_ALIGNMENT, &curAlignment);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		ATOMLOOP_START(highlightAtomIndex)
			if (!doHydrogens && chemData->getAtomicNumber(theAtom) == 1) {
				continue;
			}
			SbVec3f coord = chemData->getAtomCoordinates(theAtom);

			glRasterPos3fv(coord.getValue());
			glBitmap(xbmapWidth, xbmapHeight, xbmapWidth2, xbmapHeight2,
					 (GLfloat)xbmapWidth, 0.0, xbmap);
		ATOMLOOP_END
		glPixelStorei(GL_UNPACK_ALIGNMENT, curAlignment);
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

		ATOMLOOP_START(highlightAtomIndex)
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
#ifndef IV2_0
	lazyElt->reset(state, SoLazyElement::DIFFUSE_MASK |
				   SoLazyElement::LIGHT_MODEL_MASK);
#endif
	state->pop();
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Highlight selected atoms when in stick mode.
//
// Use: private

void
ChemDisplay::highlightAtomsWithCylinders(SoGLRenderAction *action)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf("In ChemDisplay::highlightAtomsWithCylinders\n");
#endif
	if (highlightAtomIndex.getNum() == 0) return;

	SoState *state = action->getState();
	ChemBaseData *chemData = ChemBaseDataElement::get(state);
	ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

	if (cdp->highlightStyle.getValue() ==
		ChemDisplayParam::HIGHLIGHT_NONE) return;

	if (cdp->highlightAtomStyle.getValue() ==
		ChemDisplayParam::HIGHLIGHT_ATOM_NONE) return;

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

	// Turn off texturing
	SoGLTextureEnabledElement::set(state, FALSE);

	SbVec3f coord;
	SbBool doHydrogens = cdp->showHydrogens.getValue();

	float rad = cdp->bondCylinderRadius.getValue();

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

		SbMatrix   theMatrix;
		SbRotation theRotation = currentRotation;
		SbVec3f    theScale(rad, rad, rad);
		SbVec3f    theZAxis(0, 0, 1);

		GLint curAlignment;
		glGetIntegerv(GL_UNPACK_ALIGNMENT, &curAlignment);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		ATOMLOOP_START(highlightAtomIndex)
			if (!doHydrogens && chemData->getAtomicNumber(theAtom) == 1) {
				continue;
			}
			coord = chemData->getAtomCoordinates(theAtom);
			theMatrix.setTransform(coord, theRotation, theScale);
			theMatrix.multVecMatrix(theZAxis, coord);

			glRasterPos3fv(coord.getValue());
			glBitmap(xbmapWidth, xbmapHeight, xbmapWidth2, xbmapHeight2,
					 (GLfloat)xbmapWidth, 0.0, xbmap);
		ATOMLOOP_END
		glPixelStorei(GL_UNPACK_ALIGNMENT, curAlignment);
	}
	else if (highlightAtomStyle == ChemDisplayParam::HIGHLIGHT_ATOM_BBOX) {
		SbVec3f verts[6][4];
		verts[1][2] = verts[2][3] = verts[4][3] = SbVec3f(-rad,  rad, -rad);
		verts[1][3] = verts[3][2] = verts[4][2] = SbVec3f( rad,  rad, -rad);
		verts[1][1] = verts[2][0] = verts[5][0] = SbVec3f(-rad, -rad, -rad);
		verts[1][0] = verts[3][1] = verts[5][1] = SbVec3f( rad, -rad, -rad);
		verts[0][3] = verts[2][2] = verts[4][0] = SbVec3f(-rad,  rad,  rad);
		verts[0][2] = verts[3][3] = verts[4][1] = SbVec3f( rad,  rad,  rad);
		verts[0][0] = verts[2][1] = verts[5][3] = SbVec3f(-rad, -rad,  rad);
		verts[0][1] = verts[3][0] = verts[5][2] = SbVec3f( rad, -rad,  rad);

		ATOMLOOP_START(highlightAtomIndex)
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
#ifndef IV2_0
	lazyElt->reset(state, SoLazyElement::DIFFUSE_MASK |
				   SoLazyElement::LIGHT_MODEL_MASK);
#endif
	state->pop();
}

#define END_OF_LIST 9999999

#define TOPO_UNKNOWN  0
#define TOPO_RING     1
#define TOPO_AROMATIC 2

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Checks to see if the selectionList has changed and if so, regenerates
//    the internal sets of indices.
//
// Use: private

void
ChemDisplay::generateIndices(SoAction *action)
//
////////////////////////////////////////////////////////////////////////
{
	SoState *state = action->getState();

	// Check to see if any of the data has changed.
	int32_t i;
	int32_t numDoubleBonds;
	int32_t numHigherBonds;
	SbBool dataChanged;
	SbBool selectionChanged;

	ChemBaseData *chemData = ChemBaseDataElement::get(state);
	ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);
	SbBool showHydrogens = cdp->showHydrogens.getValue();
	SbBool showMultipleBonds = cdp->showMultipleBonds.getValue();

	dataChanged = FALSE;
	const ChemBaseDataElement *cbde = ChemBaseDataElement::getInstance(state);
	if (lastChemBaseDataElement == NULL) {
		dataChanged = TRUE;
		lastChemBaseDataElement = 
			(ChemBaseDataElement *)cbde->copyMatchInfo();
	}
	else {
		if (!lastChemBaseDataElement->matches(cbde)) {
			dataChanged = TRUE;
			delete (SoElement *)lastChemBaseDataElement;
			lastChemBaseDataElement =
				(ChemBaseDataElement *)cbde->copyMatchInfo();
		}
	}

	selectionChanged = FALSE;
	const ChemDisplaySelectionElement *cse =
		ChemDisplaySelectionElement::getInstance(state);
	// If there is no ChemDisplaySelectionElement in the state, check to see if
	// there used to be a ChemDisplaySelectionElement in the state.  If there was
	// then set selectionChanged to TRUE.
	if ((cse == NULL) && (lastChemSelectionElement != NULL)) {
		selectionChanged = TRUE;
	}

	// Do further checks if necessary.
	if (!selectionChanged) {
		if (cse != NULL) {

			// Get the selectionList from the ChemDisplaySelectionElement.  If the 
			// data changed, then the ChemSelection node would have reset the
			// ChemDisplaySelectionElement to contain no selections.  If there 
			// previously had been selections, then the 
			// lastChemSelectionElement would not match and things would be 
			// OK.  If there had not been any previous selections, then 
			// things would match and we would still be OK.

                        // GDB 2/1/12 unused variable
			//const ChemDisplayPathList &selectionList = cse->getList();

			if (lastChemSelectionElement == NULL) {
				selectionChanged = TRUE;
				lastChemSelectionElement =
					(ChemDisplaySelectionElement *)cse->copyMatchInfo();
			}
			else {
				if (lastChemSelectionElement->matches(cse)) {
					selectionChanged = FALSE;
				}
				else {
					selectionChanged = TRUE;
					delete lastChemSelectionElement;
					lastChemSelectionElement =
						(ChemDisplaySelectionElement *)cse->copyMatchInfo();
				}
			}
		}
	}

	// If the data in the ChemBaseData node or in this node has changed,
	// we need to remake the nonBondedAtoms list and the bondAttributes
	// data
	if (dataChanged) {

		nonBondedAtoms.truncate(0);

		if (aromaticRingList.getLength() != 0) {
			for (i = aromaticRingList.getLength()-1; i >= 0; i--) {
                                // GDB 2/1/12 Compiler warning on delete
				//delete aromaticRingList[i];
				free(aromaticRingList[i]);
			}
			aromaticRingList.truncate(0);
		}

		// Make a list of all atoms in bonds
		RWBitVec *bList = new RWBitVec(globalNumberOfAtoms, FALSE);

		if (bondAttributes != NULL) {
			delete [] bondAttributes;
		}
// --> moiv doublebond error
	if (bondAttributesIndex != NULL)
	{
		delete [] bondAttributesIndex;
		bondAttributesIndex = NULL;
	}
// <-- moiv doublebond error

		bondAttributes = new ChemBondStuff[localNumberOfBonds];
// --> moiv doublebond error
		bondAttributesIndex = new int32_t[globalNumberOfBonds];
// <-- moiv doublebond error

		// Keep track of "showHydrogens"
		lastShowHydrogens = showHydrogens;

		numDoubleBonds = numHigherBonds = 0;
		int32_t bondCount = 0;

		BONDLOOP_START(bondIndex)
		
			int32_t from = chemData->getBondFrom(theBond);
			int32_t to = chemData->getBondTo(theBond);
			if (!showHydrogens) {
				if ((chemData->getAtomicNumber(from) != 1) &&
					(chemData->getAtomicNumber(to) != 1)) {
					bList->setBit(chemData->getBondFrom(theBond));
					bList->setBit(chemData->getBondTo(theBond));
				}
			}
			else {
				bList->setBit(chemData->getBondFrom(theBond));
				bList->setBit(chemData->getBondTo(theBond));
			}
			if (chemData->getBondType(theBond) == ChemBaseData::DOUBLE_BOND) {
				numDoubleBonds++;
			}
			else if (chemData->getBondType(theBond) == 
				ChemBaseData::AROMATIC_BOND) {
				numDoubleBonds++;
			}
			else if (chemData->getBondType(theBond) == 
				ChemBaseData::RESONANCE_BOND) {
				numDoubleBonds++;
			}
			else if (chemData->getBondType(theBond) == 
				ChemBaseData::TRIPLE_BOND) {
				numHigherBonds++;
			}
			else if (chemData->getBondType(theBond) == 
				ChemBaseData::QUADRUPLE_BOND) {
				numHigherBonds++;
			}

			// Fill out bondAttributes
			// --> moiv doublebond error
			// global to local index
			bondAttributesIndex[theBond] = bondCount;
			// <-- moiv doublebond error
			bondAttributes[bondCount].index = theBond;
			bondAttributes[bondCount].from = chemData->getBondFrom(theBond);
			bondAttributes[bondCount].to = chemData->getBondTo(theBond);
			bondAttributes[bondCount].type = chemData->getBondType(theBond);
			bondAttributes[bondCount].topo = TOPO_UNKNOWN;
			bondAttributes[bondCount].planeNormal = junkVec;
			bondAttributes[bondCount].perpNormal = junkVec;
			bondAttributes[bondCount].isSymmetric = FALSE;
			bondAttributes[bondCount].isHighlighted = FALSE;
			bondCount++;
		BONDLOOP_END

		ATOMLOOP_START(atomIndex)
			if (bList->testBit(theAtom) == FALSE) {
				nonBondedAtoms.append(theAtom);
			}
		ATOMLOOP_END

		// Done with bList
		delete bList;

		// Keep track of whether we have generated multiple bonds
		// with firstShowMultiple.
		lastShowMultipleBonds = showMultipleBonds;
		firstShowMultiple = TRUE;
		if (showMultipleBonds) firstShowMultiple = FALSE;

		// Generate the double and triple bond info if necessary
		if (showMultipleBonds && ((numDoubleBonds > 0) || (numHigherBonds > 0))) {
			// For each atom, make a ChemIntList to hold attachments
			ChemIntList *atch = new ChemIntList[globalNumberOfAtoms];
			int32_t atom1, atom2;

			BONDLOOP_START(bondIndex)
				atom1 = chemData->getBondFrom(theBond);
				atom2 = chemData->getBondTo(theBond);
				atch[atom1].append(atom2);
				atch[atom2].append(atom1);

				// Keep the bond number in the atch list
				atch[atom1].append(theBond);
				atch[atom2].append(theBond);
			BONDLOOP_END

			// If have double bonds, then we need to do a lot of work to figure
			// out where to draw the 2 parts of the bond.  We also need to 
			// generate the list of double bonds.
			if (numDoubleBonds > 0) {
				generateDoubleBondList(chemData, cdp, localNumberOfBonds,
					atch, bondAttributes);
			}

			// If have triple or quadruple bonds, then there is also some 
			// work involved  determining the placement of the outer lines.
			// We also need to generate the list of higher bonds.
			if (numHigherBonds > 0) {
				generateHigherBondList(chemData, localNumberOfBonds,
					atch, bondAttributes);
			}
			delete [] atch;
		}

// --> CAstick
		computeResidueAttributes(action);
		globalNumberOfResidues = ((ChemData*)chemData)->residues.getNum();
		localNumberOfResidues = getNumberOfResidues(action);
// <-- CAstick
// --> schematic
		computeSchematic(action);
		computeBoundsForSchematic(action);
// <-- schematic

		// If the data changed, then the selection list is no longer valid.
		// Need to reset the indices and lists

// --> schematics
//// --> CAstick
//		//resetIndices(globalNumberOfAtoms, globalNumberOfBonds);
//		resetIndices(globalNumberOfAtoms, globalNumberOfBonds, globalNumberOfResidues);
//// <-- CAstick
    resetIndices(globalNumberOfAtoms, globalNumberOfBonds, globalNumberOfResidues,globalNumberOfSchematics);
// <-- schematics

		// Update the wireframe rendering lists
		singleNormal.truncate(0);
		singleHighlight.truncate(0);
		doubleNormal.truncate(0);
		doubleHighlight.truncate(0);
		tripleNormal.truncate(0);
		tripleHighlight.truncate(0);
		quadrupleNormal.truncate(0);
		quadrupleHighlight.truncate(0);
		resonanceNormal.truncate(0);
		resonanceHighlight.truncate(0);
		hbondNormal.truncate(0);
		hbondHighlight.truncate(0);

		if (showMultipleBonds) {
			for (i = 0; i < localNumberOfBonds; i++) {
				switch (bondAttributes[i].type) {
				  case ChemBaseData::SINGLE_BOND:
					singleNormal.append(i);
					break;

				  case ChemBaseData::DOUBLE_BOND:
					doubleNormal.append(i);
					break;

				  case ChemBaseData::TRIPLE_BOND:
					tripleNormal.append(i);
					break;

				  case ChemBaseData::QUADRUPLE_BOND:
					quadrupleNormal.append(i);
					break;

				  case ChemBaseData::RESONANCE_BOND:
					resonanceNormal.append(i);
					break;

				  case ChemBaseData::HYDROGEN_BOND:
					hbondNormal.append(i);
					break;

				  default:
					singleNormal.append(i);
					break;
				}
			}
		}
		else {
			for (i = 0; i < localNumberOfBonds; i++) {
				if (bondAttributes[i].type == ChemBaseData::HYDROGEN_BOND) {
					hbondNormal.append(i);
				}
				else {
					singleNormal.append(i);
				}
			}
		}

// --> octree culling
	octreenode->build(this,(ChemData*)chemData);
  // duplicates
  if (globalNumberOfBonds)
  renderedBonds = new bool[globalNumberOfBonds];
  if (globalNumberOfResidues)
  renderedResidues = new bool[globalNumberOfResidues];
// <-- octree culling

	} //if (dataChanged)

	// If there was a change in "showHydrogens", then we need to remake
	// normalAtomIndex, highlightAtomIndex, nonBondedAtoms, 
	// normalNonBondedAtomIndex and highlightNonBondedAtomIndex
	if (lastShowHydrogens != showHydrogens) {
		lastShowHydrogens = showHydrogens;

		RWBitVec *bList = new RWBitVec(globalNumberOfAtoms, FALSE);
		nonBondedAtoms.truncate(0);

		BONDLOOP_START(bondIndex)
			int32_t from = chemData->getBondFrom(theBond);
			int32_t to = chemData->getBondTo(theBond);
			if (!showHydrogens) {
				if ((chemData->getAtomicNumber(from) != 1) &&
					(chemData->getAtomicNumber(to) != 1)) {
					bList->setBit(chemData->getBondFrom(theBond));
					bList->setBit(chemData->getBondTo(theBond));
				}
			}
			else {
				bList->setBit(chemData->getBondFrom(theBond));
				bList->setBit(chemData->getBondTo(theBond));
			}
		BONDLOOP_END

		ATOMLOOP_START(atomIndex)
			if (bList->testBit(theAtom) == FALSE) {
				nonBondedAtoms.append(theAtom);
			}
		ATOMLOOP_END

		delete bList;

		// Set the values for normalNonBondedAtoms
		normalNonBondedAtomIndex.deleteValues(0);

		if (nonBondedAtoms.getLength() > 0) {
			int32_t loop, end, item;
			int32_t lastStart, lastIncr, count;

			lastStart = -1;
			lastIncr = 0;
			count = 0;
			end = nonBondedAtoms.getLength();
			for (loop = 0; loop < end; loop++) {
				item = nonBondedAtoms[loop];
				if (lastStart == -1) {
					lastStart = item;
					lastIncr = 1;
				}
				else if (item == lastStart + lastIncr) {
					lastIncr++;
				}
				else {
					normalNonBondedAtomIndex.set1Value(count++,
						SbVec2i(lastStart, lastIncr));
					lastStart = item;
					lastIncr = 1;
				}
			}
			if (lastStart != -1) {
				normalNonBondedAtomIndex.set1Value(count,
					SbVec2i(lastStart, lastIncr));
			}
		}

		// Make a new normalAtomIndex with all atoms
		sortIndices(atomIndex, normalAtomIndex,
			CHEM_DISPLAY_USE_REST_OF_ATOMS, globalNumberOfAtoms);

		// Remove the nonBonded atoms from the normalAtoms
		removeIndices(normalAtomIndex, normalNonBondedAtomIndex,
			CHEM_DISPLAY_USE_REST_OF_ATOMS, globalNumberOfAtoms);

		// Remove the highlighted atoms from the nonBonded atoms.
		// Only do this if the selection has not changed since the code
		// in the selectionChanged section will deal with this.
		if (!selectionChanged) {
			// Append the highlightAtomIndex to the
			// highlightNonBondedAtomIndex
			if (highlightNonBondedAtomIndex.getNum() > 0) {
				highlightNonBondedAtomIndex.setValues(
					highlightNonBondedAtomIndex.getNum(),
					highlightAtomIndex.getNum(),
					highlightAtomIndex.getValues(0));

				// Make a sorted list and store the result in
				// highlightAtomIndex.  The list needs to be sorted
				// in order for removeIndices to work correctly.
				sortIndices(highlightNonBondedAtomIndex, highlightAtomIndex,
					CHEM_DISPLAY_USE_REST_OF_ATOMS, globalNumberOfAtoms);
			}

			// Set highlightNonBondedAtomIndex equal to
			// normalNonBondedAtomIndex
			highlightNonBondedAtomIndex.deleteValues(0);
			highlightNonBondedAtomIndex.setValues(0,
				normalNonBondedAtomIndex.getNum(),
				normalNonBondedAtomIndex.getValues(0));

			// Remove highlightAtomIndex from normalNonBondedAtomIndex
			removeIndices(normalNonBondedAtomIndex, highlightAtomIndex,
				CHEM_DISPLAY_USE_REST_OF_ATOMS, globalNumberOfAtoms);

			// Remove the normal nonBonded atoms from the highlighted 
			// nonBonded atoms
			removeIndices(highlightNonBondedAtomIndex,
				normalNonBondedAtomIndex,
				CHEM_DISPLAY_USE_REST_OF_ATOMS, globalNumberOfAtoms);

			// Remove the highlightNonBondedAtomIndex from
			// highlightAtomIndex
			removeIndices(highlightAtomIndex, highlightNonBondedAtomIndex,
				CHEM_DISPLAY_USE_REST_OF_ATOMS, globalNumberOfAtoms);

			// Remove the highlighted atoms from the normal atoms
			removeIndices(normalAtomIndex, highlightAtomIndex,
				CHEM_DISPLAY_USE_REST_OF_ATOMS, globalNumberOfAtoms);
		}
// --> octree culling
	octreenode->build(this,(ChemData*)chemData);
// <-- octree culling
	}

	// Deal with change in showing multiple bonds
	if (lastShowMultipleBonds != showMultipleBonds) {
		lastShowMultipleBonds = showMultipleBonds;
		if (showMultipleBonds) {
			if (firstShowMultiple) {
				firstShowMultiple = FALSE;

				if (aromaticRingList.getLength() != 0) {
					for (i = aromaticRingList.getLength()-1; i >= 0; i--) {
                                                // GDB 2/1/12 Compiler warning
                                                // on delete
						//delete aromaticRingList[i];
						free(aromaticRingList[i]);
					}
					aromaticRingList.truncate(0);
				}

				// Count the number of multiple bonds
				numDoubleBonds = numHigherBonds = 0;
				BONDLOOP_START(bondIndex)
					if (chemData->getBondType(theBond) ==
						ChemBaseData::DOUBLE_BOND) {
						numDoubleBonds++;
					}
					else if (chemData->getBondType(theBond) == 
						ChemBaseData::AROMATIC_BOND) {
						numDoubleBonds++;
					}
					else if (chemData->getBondType(theBond) == 
						ChemBaseData::RESONANCE_BOND) {
						numDoubleBonds++;
					}
					else if (chemData->getBondType(theBond) == 
						ChemBaseData::TRIPLE_BOND) {
						numHigherBonds++;
					}
					else if (chemData->getBondType(theBond) == 
						ChemBaseData::QUADRUPLE_BOND) {
						numHigherBonds++;
					}
				BONDLOOP_END
				if ((numDoubleBonds > 0) || (numHigherBonds > 0)) {
					// For each atom, make a ChemIntList to hold attachments
					ChemIntList *atch = new ChemIntList[globalNumberOfAtoms];
					int32_t atom1, atom2;

					BONDLOOP_START(bondIndex)
						atom1 = chemData->getBondFrom(theBond);
						atom2 = chemData->getBondTo(theBond);
						atch[atom1].append(atom2);
						atch[atom2].append(atom1);

						// Keep the bond number in the atch list
						atch[atom1].append(theBond);
						atch[atom2].append(theBond);
					BONDLOOP_END

					// If have double bonds, then we need to do a lot of work 
					// to figure out where to draw the 2 parts of the bond.  
					// We also need to generate the list of double bonds.
					if (numDoubleBonds > 0) {
						generateDoubleBondList(chemData, cdp, localNumberOfBonds,
							atch, bondAttributes);
					}

					// If have triple or quadruple bonds, then there is also 
					// some work involved  determining the placement of the 
					// outer lines.  We also need to generate the list of 
					// higher bonds.
					if (numHigherBonds > 0) {
						generateHigherBondList(chemData, localNumberOfBonds,
							atch, bondAttributes);
					}
					delete [] atch;
				}
			}
		}

		// If the selection hasn't changed, then we need to regenerate
		// the "normal" and "highlighted" indices.  Otherwise, this
		// will be taken care of in the "selectionChanged" section of
		// code.
		if (!selectionChanged) {
			// Update the wireframe rendering lists
			singleNormal.truncate(0);
			singleHighlight.truncate(0);
			doubleNormal.truncate(0);
			doubleHighlight.truncate(0);
			tripleNormal.truncate(0);
			tripleHighlight.truncate(0);
			quadrupleNormal.truncate(0);
			quadrupleHighlight.truncate(0);
			resonanceNormal.truncate(0);
			resonanceHighlight.truncate(0);
			hbondNormal.truncate(0);
			hbondHighlight.truncate(0);

			BONDLOOP_START(highlightBondIndex)
				for (i = 0; i < localNumberOfBonds; i++) {
					if (bondAttributes[i].index == theBond) {
						bondAttributes[i].isHighlighted = TRUE;
						break;
					}
				}
			BONDLOOP_END

			if (showMultipleBonds) {
				for (i = 0; i < localNumberOfBonds; i++) {
					switch (bondAttributes[i].type) {
					  case ChemBaseData::SINGLE_BOND:
						if (bondAttributes[i].isHighlighted) {
							singleHighlight.append(i);
						}
						else {
							singleNormal.append(i);
						}
						break;

					  case ChemBaseData::DOUBLE_BOND:
						if (bondAttributes[i].isHighlighted) {
							doubleHighlight.append(i);
						}
						else {
							doubleNormal.append(i);
						}
						break;

					  case ChemBaseData::TRIPLE_BOND:
						if (bondAttributes[i].isHighlighted) {
							tripleHighlight.append(i);
						}
						else {
							tripleNormal.append(i);
						}
						break;

					  case ChemBaseData::QUADRUPLE_BOND:
						if (bondAttributes[i].isHighlighted) {
							quadrupleHighlight.append(i);
						}
						else {
							quadrupleNormal.append(i);
						}
						break;

					  case ChemBaseData::RESONANCE_BOND:
						if (bondAttributes[i].isHighlighted) {
							resonanceHighlight.append(i);
						}
						else {
							resonanceNormal.append(i);
						}
						break;

					  case ChemBaseData::HYDROGEN_BOND:
						if (bondAttributes[i].isHighlighted) {
							hbondHighlight.append(i);
						}
						else {
							hbondNormal.append(i);
						}
						break;

					  default:
						if (bondAttributes[i].isHighlighted) {
							singleHighlight.append(i);
						}
						else {
							singleNormal.append(i);
						}
						break;
					}
				}
			}
			else {
				for (i = 0; i < localNumberOfBonds; i++) {
					if (bondAttributes[i].type == ChemBaseData::HYDROGEN_BOND) {
						if (bondAttributes[i].isHighlighted) {
							hbondHighlight.append(i);
						}
						else {
							hbondNormal.append(i);
						}
					}
					else {
						if (bondAttributes[i].isHighlighted) {
							singleHighlight.append(i);
						}
						else {
							singleNormal.append(i);
						}
					}
				}
			}
		}
// --> residue selection
		// If the selection hasn't changed, then we need to regenerate
		// the "normal" and "highlighted" indices.  Otherwise, this
		// will be taken care of in the "selectionChanged" section of
		// code.
		if (!selectionChanged)
		{
			// Remove the highlighted residues from the normal residues
			removeIndices(normalResidueIndex, highlightResidueIndex,
				CHEM_DISPLAY_USE_REST_OF_RESIDUES, globalNumberOfResidues);

			// Remove the highlighted residue labels from the normal residues
			removeIndices(normalResidueLabelIndex, highlightResidueLabelIndex,
				CHEM_DISPLAY_USE_REST_OF_RESIDUES, globalNumberOfResidues);

// --> schematics
      // Remove the highlighted schematics from the normal schematics
			removeIndices(normalSchematicIndex, highlightSchematicIndex,
				CHEM_DISPLAY_USE_REST_OF_SCHEMATICS, globalNumberOfSchematics);
// <-- schematics
		}
// <-- residue selection
	}

	// Deal with selection changes
	if (selectionChanged) {
		// If the selection changed, the first thing to do is to reset all
		// the "normal" indices and set the isHighlighted flag to FALSE.
// --> schematics
//// --> CAstick
//		//resetIndices(globalNumberOfAtoms, globalNumberOfBonds);
//		resetIndices(globalNumberOfAtoms, globalNumberOfBonds, globalNumberOfResidues);
//// <-- CAstick
    resetIndices(globalNumberOfAtoms, globalNumberOfBonds, globalNumberOfResidues, globalNumberOfSchematics);
// <-- schematics
		for (i = 0; i < localNumberOfBonds; i++) {
			bondAttributes[i].isHighlighted = FALSE;
		}

		// If there is a ChemDisplaySelectionElement, get the lists of selected items
		// and fill out the highlighted indices.
		if (cse != NULL) {
			const ChemDisplayPathList &selectionList = cse->getList();
			int32_t length = selectionList.getLength();
			for (i = 0; i < length; i++) {
				if (((ChemPath *)selectionList[i])->isOfType(ChemDisplayPath::getClassTypeId())) {
					const ChemDisplayPath *si = (ChemDisplayPath *)selectionList[i];
					SoFullPath *path = (SoFullPath *)si->getSoPath();
					if (path->getTail() == this) {
						const MFVec2i &slAtomIndex = si->getAtomIndex();
						highlightAtomIndex.setValues(0, slAtomIndex.getNum(),
							slAtomIndex.getValues(0));

						const MFVec2i &slBondIndex = si->getBondIndex();
						highlightBondIndex.setValues(0, slBondIndex.getNum(),
							slBondIndex.getValues(0));

						const MFVec2i &slAtomLabelIndex = si->getAtomLabelIndex();
						highlightAtomLabelIndex.setValues(0, slAtomLabelIndex.getNum(),
							slAtomLabelIndex.getValues(0));

						const MFVec2i &slBondLabelIndex = si->getBondLabelIndex();
						highlightBondLabelIndex.setValues(0, slBondLabelIndex.getNum(),
							slBondLabelIndex.getValues(0));

// --> residue selection
						const MFVec2i &slResidueIndex = si->getResidueIndex();
						highlightResidueIndex.setValues(0, slResidueIndex.getNum(),
							slResidueIndex.getValues(0));

						const MFVec2i &slResidueLabelIndex = si->getResidueLabelIndex();
						highlightResidueLabelIndex.setValues(0, slResidueLabelIndex.getNum(),
							slResidueLabelIndex.getValues(0));
// <-- residue selection
// --> schematics
            const MFVec2i &slSchematicIndex = si->getSchematicIndex();
						highlightSchematicIndex.setValues(0, slSchematicIndex.getNum(),
							slSchematicIndex.getValues(0));
// <-- schematics
						break;
					}
				}
			}

			// Remove the values in the "highlight" indices from the values 
			// in the "normal" indices.

			if (highlightAtomIndex.getNum() > 0) {
				// Make the highlightNonBondedAtomIndex from the
				// normalNonBondedAtomIndex.  The normalNonBondedAtomIndex
				// at this point actually contains all nonBonded atoms.
				highlightNonBondedAtomIndex.deleteValues(0);
				highlightNonBondedAtomIndex.setValues(0,
					normalNonBondedAtomIndex.getNum(),
					normalNonBondedAtomIndex.getValues(0));

				// Remove highlighted atoms from the normalNonBondedAtomIndex
				removeIndices(normalNonBondedAtomIndex, highlightAtomIndex,
					CHEM_DISPLAY_USE_REST_OF_ATOMS, globalNumberOfAtoms);

				// Remove the normal nonBonded atoms from the highlighted 
				// nonBonded atoms
				removeIndices(highlightNonBondedAtomIndex,
					normalNonBondedAtomIndex,
					CHEM_DISPLAY_USE_REST_OF_ATOMS, globalNumberOfAtoms);

				// Remove the highlightNonBondedAtomIndex from
				// highlightAtomIndex
				removeIndices(highlightAtomIndex, highlightNonBondedAtomIndex,
					CHEM_DISPLAY_USE_REST_OF_ATOMS, globalNumberOfAtoms);

				// Remove the highlighted atoms from the normal atoms
				removeIndices(normalAtomIndex, highlightAtomIndex,
					CHEM_DISPLAY_USE_REST_OF_ATOMS, globalNumberOfAtoms);
			}

			if (highlightBondIndex.getNum() > 0) {
				removeIndices(normalBondIndex, highlightBondIndex,
					CHEM_DISPLAY_USE_REST_OF_BONDS, globalNumberOfBonds);
			}

			if (highlightAtomLabelIndex.getNum() > 0) {
				removeIndices(normalAtomLabelIndex, highlightAtomLabelIndex,
					CHEM_DISPLAY_USE_REST_OF_ATOMS, globalNumberOfAtoms);
			}

			if (highlightBondLabelIndex.getNum() > 0) {
				removeIndices(normalBondLabelIndex, highlightBondLabelIndex,
					CHEM_DISPLAY_USE_REST_OF_BONDS, globalNumberOfBonds);
			}
		}

		// Update the wireframe rendering lists
		singleNormal.truncate(0);
		singleHighlight.truncate(0);
		doubleNormal.truncate(0);
		doubleHighlight.truncate(0);
		tripleNormal.truncate(0);
		tripleHighlight.truncate(0);
		quadrupleNormal.truncate(0);
		quadrupleHighlight.truncate(0);
		resonanceNormal.truncate(0);
		resonanceHighlight.truncate(0);
		hbondNormal.truncate(0);
		hbondHighlight.truncate(0);

                //cout << "Start bond loop" << theBond << endl;
		BONDLOOP_START(highlightBondIndex)
			for (i = 0; i < localNumberOfBonds; i++) {
				if (bondAttributes[i].index == theBond) {
					bondAttributes[i].isHighlighted = TRUE;
					break;
				}
			}
		BONDLOOP_END
                //cout << "End bond loop" << theBond << endl;

		if (showMultipleBonds) {
			for (i = 0; i < localNumberOfBonds; i++) {
				switch (bondAttributes[i].type) {
				  case ChemBaseData::SINGLE_BOND:
					if (bondAttributes[i].isHighlighted) {
						singleHighlight.append(i);
					}
					else {
						singleNormal.append(i);
					}
					break;

				  case ChemBaseData::DOUBLE_BOND:
					if (bondAttributes[i].isHighlighted) {
						doubleHighlight.append(i);
					}
					else {
						doubleNormal.append(i);
					}
					break;

				  case ChemBaseData::TRIPLE_BOND:
					if (bondAttributes[i].isHighlighted) {
						tripleHighlight.append(i);
					}
					else {
						tripleNormal.append(i);
					}
					break;

				  case ChemBaseData::QUADRUPLE_BOND:
					if (bondAttributes[i].isHighlighted) {
						quadrupleHighlight.append(i);
					}
					else {
						quadrupleNormal.append(i);
					}
					break;

				  case ChemBaseData::RESONANCE_BOND:
					if (bondAttributes[i].isHighlighted) {
						resonanceHighlight.append(i);
					}
					else {
						resonanceNormal.append(i);
					}
					break;

				  case ChemBaseData::HYDROGEN_BOND:
					if (bondAttributes[i].isHighlighted) {
						hbondHighlight.append(i);
					}
					else {
						hbondNormal.append(i);
					}
					break;

				  default:
					if (bondAttributes[i].isHighlighted) {
						singleHighlight.append(i);
					}
					else {
						singleNormal.append(i);
					}
					break;
				}
			}
		}
		else {
			for (i = 0; i < localNumberOfBonds; i++) {
				if (bondAttributes[i].type == ChemBaseData::HYDROGEN_BOND) {
					if (bondAttributes[i].isHighlighted) {
						hbondHighlight.append(i);
					}
					else {
						hbondNormal.append(i);
					}
				}
				else {
					if (bondAttributes[i].isHighlighted) {
						singleHighlight.append(i);
					}
					else {
						singleNormal.append(i);
					}
				}
			}
		}
// --> residue selection
		if (highlightResidueIndex.getNum() > 0)
		{
				// Remove the highlighted residues from the normal residues
				removeIndices(normalResidueIndex, highlightResidueIndex,
					CHEM_DISPLAY_USE_REST_OF_RESIDUES, globalNumberOfResidues);
		}

		if (highlightResidueLabelIndex.getNum() > 0)
		{
				// Remove the highlighted residues from the normal residues
				removeIndices(normalResidueLabelIndex, highlightResidueLabelIndex,
					CHEM_DISPLAY_USE_REST_OF_RESIDUES, globalNumberOfResidues);
		}
// <-- residue selection
// --> schematic
    if (highlightSchematicIndex.getNum() > 0)
		{
				// Remove the highlighted schematics from the normal schematics
				removeIndices(normalSchematicIndex, highlightSchematicIndex,
					CHEM_DISPLAY_USE_REST_OF_SCHEMATICS, globalNumberOfSchematics);
		}
// <-- schematic
// --> octree culling
	octreenode->build(this,(ChemData*)chemData);
// <-- octree culling
	}
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Reset the values of the various indices back to their default state.
//
// Use: private

// --> schematics
// // --> CAstick
// //void ChemDisplay::resetIndices(int32_t numAtoms, int32_t numBonds)
// void ChemDisplay::resetIndices(int32_t numAtoms, int32_t numBonds, int32_t numResidues)
// // --> CAstick
void ChemDisplay::resetIndices(int32_t numAtoms, int32_t numBonds, int32_t numResidues, int32_t numSchematics)
// <-- schematics
//
////////////////////////////////////////////////////////////////////////
{
	// Sort and make unique the "public" indices to make the "normal"
	// internal indices.
	sortIndices(atomIndex, normalAtomIndex, 
		CHEM_DISPLAY_USE_REST_OF_ATOMS, numAtoms);
	sortIndices(bondIndex, normalBondIndex, 
		CHEM_DISPLAY_USE_REST_OF_BONDS, numBonds);
	sortIndices(atomLabelIndex, normalAtomLabelIndex, 
		CHEM_DISPLAY_USE_REST_OF_ATOMS, numAtoms);
	sortIndices(bondLabelIndex, normalBondLabelIndex, 
		CHEM_DISPLAY_USE_REST_OF_BONDS, numBonds);

	// Delete the values in the highlight indices.
	highlightAtomIndex.deleteValues(0);
	highlightBondIndex.deleteValues(0);
	highlightBondLabelIndex.deleteValues(0);
	highlightAtomLabelIndex.deleteValues(0);

	// Set the values for normalNonBondedAtoms.
	normalNonBondedAtomIndex.deleteValues(0);
	highlightNonBondedAtomIndex.deleteValues(0);

	if (nonBondedAtoms.getLength() > 0) {
		int32_t loop, end, item;
		int32_t lastStart, lastIncr, count;

		lastStart = -1;
		lastIncr = 0;
		count = 0;
		end = nonBondedAtoms.getLength();
		for (loop = 0; loop < end; loop++) {
			item = nonBondedAtoms[loop];
			if (lastStart == -1) {
				lastStart = item;
				lastIncr = 1;
			}
			else if (item == lastStart + lastIncr) {
				lastIncr++;
			}
			else {
				normalNonBondedAtomIndex.set1Value(count++,
					SbVec2i(lastStart, lastIncr));
				lastStart = item;
				lastIncr = 1;
			}
		}
		if (lastStart != -1) {
			normalNonBondedAtomIndex.set1Value(count,
				SbVec2i(lastStart, lastIncr));
		}

		// Remove the nonBonded atoms from the bonded atoms
		removeIndices(normalAtomIndex, normalNonBondedAtomIndex,
			CHEM_DISPLAY_USE_REST_OF_ATOMS, numAtoms);
	}
// --> residue selection
	sortIndices(residueIndex, normalResidueIndex, 
		CHEM_DISPLAY_USE_REST_OF_RESIDUES, numResidues);

	sortIndices(residueLabelIndex, normalResidueLabelIndex, 
		CHEM_DISPLAY_USE_REST_OF_RESIDUES, numResidues);

	highlightResidueIndex.deleteValues(0);
	highlightResidueLabelIndex.deleteValues(0);
// <-- residue selection
// --> schematics
	sortIndices(schematicIndex, normalSchematicIndex, 
		CHEM_DISPLAY_USE_REST_OF_SCHEMATICS, numSchematics);
  highlightSchematicIndex.deleteValues(0);
// <-- schematics
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Sorts and makes unique the values in listIndex to make newIndex.
//
// Use: private
void
ChemDisplay::sortIndices(MFVec2i &listIndex, MFVec2i &newIndex,
	int32_t end_indicator, int32_t numberOfItems)
//
////////////////////////////////////////////////////////////////////////
{
// --> octree culling
//	newIndex.deleteValues(0);
	newIndex.setNum(0);
// <-- octree culling
	if (listIndex.getNum() == 0) return;

	ChemIntList theList(numberOfItems);

	int32_t loop, numLoops, start, end, item;
	int32_t lastStart, lastIncr, count;

	// Make a list of all the entries in listIndex.
	numLoops = listIndex.getNum();
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

	// Sort theList and make it unique.
	theList.sort(TRUE);

	if (theList.getLength() == 0) return;

	// Make newIndex using contents of theList.
	lastStart = -1;
	lastIncr = 0;
	count = 0;
	end = theList.getLength();
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
			newIndex.set1Value(count++, SbVec2i(lastStart, lastIncr));
			lastStart = item;
			lastIncr = 1;
		}
	}
	if (lastStart != -1) {
		newIndex.set1Value(count, SbVec2i(lastStart, lastIncr));
	}
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Removes the values in "newIndex" from "listIndex".
//
// Use: private

void
ChemDisplay::removeIndices(MFVec2i &listIndex, MFVec2i &newIndex,
	int32_t end_indicator, int32_t numberOfItems)
//
////////////////////////////////////////////////////////////////////////
{
	int32_t loop, numLoops, start, end, item;
	int32_t lastStart, lastIncr, count;
	int32_t index;

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

#define DEG_TO_RAD 0.01745329251994329576

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Generates a list of double bonds and aromatic rings.  For each
//    double bond, the direction vector and the "side" of the second
//    line is determined.
//
// Use: private

void
ChemDisplay::generateDoubleBondList(ChemBaseData *chemData,
	ChemDisplayParam *cdp,
	int32_t localNumberOfBonds, ChemIntList *atch, 
	ChemBondStuff *bondAttributes)
//
////////////////////////////////////////////////////////////////////////
{
	int32_t i, j;

	int32_t atom1;
	int32_t atom2;
	int32_t curBond;

	float multiSep = cdp->multipleBondSeparation.getValue();

#define rings
#ifdef rings
        if (cdp->getShowRings()) {
           int style = cdp->displayStyle.getValue();
           if (style == ChemDisplayParam::DISPLAY_WIREFRAME ||
                 style == ChemDisplayParam::DISPLAY_BALLWIRE) {
              // Search for rings
              ChemSSSR *sssr = new ChemSSSR(globalNumberOfAtoms, localNumberOfBonds, 
                    atch,this);
              int32_t numberOfRings, minRingSize, maxRingSize;
              sssr->findRings(cdp->maxRingSize.getValue(),
                    numberOfRings, minRingSize, maxRingSize, false/*, chemData*/);

              // If there are rings, see which are aromatic.  If there are bonds marked
              // aromatic but they are not in rings, change them to resonance bonds.
              if (numberOfRings > 0) {
                 // Check to see if any of the rings are aromatic (which means that
                 // all bonds in the ring are marked ChemBaseData::AROMATIC_BOND).
                 const RWBitVec *ringBitVecTable = sssr->getRings();
                 SbIntList *ringList = new SbIntList[numberOfRings];
                 for (i = 0; i < numberOfRings; i++) {
                    for (j = 0; j < localNumberOfBonds; j++) {
                       if (ringBitVecTable[i].testBit(j)) {
                          ringList[i].append(j);
                          bondAttributes[j].topo = TOPO_RING;
                       }
                    }
                 }
                 checkAromaticity(chemData, numberOfRings, maxRingSize,
                       cdp->aromaticRingScale.getValue(),
                       ringList, bondAttributes, aromaticRingList);

                 // May want to keep the list of rings for some reason
                 delete [] ringList;
              }
#ifdef AROMATIC_TO_RESONANCE
              else {
                 for (i = 0; i < localNumberOfBonds; i++) {
                    if (bondAttributes[i].type == ChemBaseData::AROMATIC_BOND) {
                       bondAttributes[i].type = ChemBaseData::RESONANCE_BOND;
                    }
                 }
              }
#endif

              // Cleanup after ring search
              delete sssr;
              //cout << "done generating rings" << numberOfRings << endl;
           }
        }
#endif

	// Start determining the sidedness of the bonds
	int32_t atom3;
	int natch1, natch2;
	SbVec3f planeNormal;
	SbVec3f perpNormal;
	SbVec3f coord1, coord2, coord3;
	SbVec3f vec21;


        // 2/9/06 KLS 
        // Get the lists of all linearly connected resonance and 
        // double bonds that are not in rings.
        // Most codes skip double bond processing for proteins.  Rather
        // than add API control, set a cutoff.
        // THe dList processing for large systems with lots of double
        // bonds scales ~n**2 where n is the number of double bonds.
        const int usefulCutOffLevel = 1000;
        if (localNumberOfBonds < usefulCutOffLevel) {

	SbIntList rList;
	SbIntList dList;
	int32_t listLen;
	for (i = 0; i < localNumberOfBonds; i++) {
		if ((bondAttributes[i].topo != TOPO_RING) ||
			(bondAttributes[i].topo != TOPO_AROMATIC)) {
			if (bondAttributes[i].type == ChemBaseData::RESONANCE_BOND) {
				rList.append(i);
			}
			else if (bondAttributes[i].type == ChemBaseData::DOUBLE_BOND) {
				dList.append(i);
			}
		}
	}

	// Process the resonance bonds
	listLen = rList.getLength();
	if (listLen > 1) {
		SbIntList *rPaths;
		int32_t numPaths;
		getLinearPaths(rList, bondAttributes, numPaths, rPaths);

		// Now work through the paths and determine sidedness
		rList.truncate(0);
		if (numPaths > 0) {
			processLinearResonanceBonds(multiSep, numPaths, rPaths,
				atch, bondAttributes, chemData);
		}
		delete [] rPaths;
	}
	else {
		rList.truncate(0);
	}

	// Process the double bonds
	listLen = dList.getLength();
	if (listLen > 1) {
		SbIntList *dPaths;
		int32_t numPaths;
		getLinearPaths(dList, bondAttributes, numPaths, dPaths);

		// Now work through the paths and determine sidedness
		dList.truncate(0);
		if (numPaths > 0) {
			processCumulenes(numPaths, dPaths, atch, bondAttributes, chemData);
		}
		delete [] dPaths;
	}
	else {
		dList.truncate(0);
	}
        }

	// Determine on which side to draw the double bond
	int sameSideWeight;
	int otherSideWeight;
	int incr;
	float cosTheta, theta;
	SbBool linear, sameSide;
	SbVec3f tmpVec;
	SbIntList otherSideAtoms;

	// The following defines linearity as anything less than 5 degrees
	float linearTolerance = (float)(5.0 * DEG_TO_RAD);

	for (i = 0; i < localNumberOfBonds; i++) {
		if (((bondAttributes[i].type == ChemBaseData::DOUBLE_BOND) ||
			(bondAttributes[i].type == ChemBaseData::RESONANCE_BOND)) &&
			(bondAttributes[i].perpNormal == junkVec)) {
			atom1 = bondAttributes[i].from;
			atom2 = bondAttributes[i].to;
			coord1 = chemData->getAtomCoordinates(atom1);
			coord2 = chemData->getAtomCoordinates(atom2);
			natch1 = atch[atom1].getLength() / 2;
			natch2 = atch[atom2].getLength() / 2;
			SbLine line12(coord1, coord2);

			vec21 = coord2 - coord1;

			// Determine a plane.  If there is more than one attachment to
			// atom1, use the first non-linear, non-atom2 attachment as the 
			// 3rd point defining the plane.
			linear = TRUE;
			for (j = 0; j < natch1*2; j+=2) {
				atom3 = atch[atom1][j];
				if (atom3 == atom2) continue;

				coord3 = chemData->getAtomCoordinates(atom3);
				tmpVec = coord1 - coord3;
				tmpVec.normalize();
				cosTheta = vec21.dot(tmpVec);
				if (cosTheta < -1.0) cosTheta = -1.0;
				if (cosTheta >  1.0) cosTheta =  1.0;
				if (acosf(cosTheta) > linearTolerance) {
					linear = FALSE;
					break;
				}
			}

			// If everything attached to atom1 is linear, then try atom2.
			if (linear) {
				for (j = 0; j < natch2*2; j+=2) {
					atom3 = atch[atom2][j];
					if (atom3 == atom1) continue;

					coord3 = chemData->getAtomCoordinates(atom3);
					tmpVec = coord3 - coord2;
					tmpVec.normalize();
					cosTheta = vec21.dot(tmpVec);
					if (cosTheta < -1.0) cosTheta = -1.0;
					if (cosTheta >  1.0) cosTheta =  1.0;
					if (acosf(cosTheta) > linearTolerance) {
						linear = FALSE;
						break;
					}
				}
			}

			// At this point, we have checked for any non-linear atoms from
			// which we can make the first plane.  If one does not exist,
			// then do the following.  If one does exist, then it is atom3
			// with coord3.
			if (linear) {
				coord3 = coord1;

				// If the bond is parallel to Y axis, then increment the 
				// X coordinate of atom1 by 1 and make that coord3.
				if (((vec21[0] >= -0.01) && (vec21[0] <= 0.01)) &&
					((vec21[2] >= -0.01) && (vec21[2] <= 0.01))) {
					coord3[0] += 1.0;
				}

				// If the bond is not along the Y axis, increment the
				// Y coordinate of atom1 by 1 and make that coord3.
				else {
					coord3[1] += 1.0;
				}
			}

			// Create the first plane using coord1, coord2 and coord3,
			// then find its normal.  The planeNormal is used in the
			// determination of bounding boxes.
			SbPlane tmpPlane(coord1, coord2, coord3);
			planeNormal = tmpPlane.getNormal();

			// Drop a perpendicular from coord3 to the line defined by coord1 
			// and coord2.  This line is defined by coord3 - closestPoint on
			// the coord1-coord2 line.  When this is normalized, this
			// is the normal perpendicular to the bond along which the
			// the second line should be translated.
			perpNormal = line12.getClosestPoint(coord3);
			perpNormal = coord3 - perpNormal;
			perpNormal.normalize();

			// Define the plane which is used to determine if an atom
			// is on one side of the bond or the other.
			coord3 = (coord2 - coord1) * 0.5f + coord1 + planeNormal;
			SbPlane plane(perpNormal, coord3);

			// Now calculate the "weights" to determine on which side the
			// second line should be drawn or if the bond is symmetric
			sameSideWeight = otherSideWeight = 0;

			if (linear) {
				bondAttributes[i].isSymmetric = TRUE;
			}
			else if ((natch1 >= 3 && natch2 == 1) ||
					 (natch1 == 1 && natch2 >= 3)) {
				bondAttributes[i].isSymmetric = TRUE;
			}
			else {
				natch1 *= 2;
				natch2 *= 2;
				otherSideAtoms.truncate(0);
				for (j = 0; j < natch1; j+=2) {
					incr = 0;
					atom3 = atch[atom1][j];
					if (atom3 == atom2) continue;

					coord3 = chemData->getAtomCoordinates(atom3);
					tmpVec = coord1 - coord3;
					tmpVec.normalize();

					// If not linear, do weighting
					cosTheta = vec21.dot(tmpVec);
					if (cosTheta < -1.0) cosTheta = -1.0;
					if (cosTheta >  1.0) cosTheta =  1.0;
					if (acosf(cosTheta) > linearTolerance) {
						sameSide = plane.isInHalfSpace(coord3);
						curBond = atch[atom1][j+1];
// --> moiv doublebond error
						curBond = bondAttributesIndex[curBond];// global to local index
						assert(bondAttributes[curBond].index == atch[atom1][j+1]);
// <-- moiv doublebond error
						if (bondAttributes[curBond].topo == TOPO_AROMATIC) {
							incr = 20;
						}
						else if (bondAttributes[curBond].topo == TOPO_RING) {
							incr = 3;
						}
						else {
							incr = 1;
						}
						if (sameSide) {
							sameSideWeight += incr;
						}
						else {
							otherSideWeight += incr;
							otherSideAtoms.append(atom3);
						}
					}
				}

				// Now do the other atom
				for (j = 0; j < natch2; j+=2) {
					atom3 = atch[atom2][j];
					if (atom3 == atom1) continue;

					coord3 = chemData->getAtomCoordinates(atom3);
					tmpVec = coord3 - coord2;
					tmpVec.normalize();

					// If not linear, do weighting
					cosTheta = vec21.dot(tmpVec);
					if (cosTheta < -1.0) cosTheta = -1.0;
					if (cosTheta >  1.0) cosTheta =  1.0;
					if (acosf(cosTheta) > linearTolerance) {
						sameSide = plane.isInHalfSpace(coord3);
						curBond = atch[atom2][j+1];
// --> moiv doublebond error
						curBond = bondAttributesIndex[curBond];// global to local index
						assert(bondAttributes[curBond].index == atch[atom2][j+1]);
// <-- moiv doublebond error
						if (bondAttributes[curBond].topo == TOPO_AROMATIC) {
							incr = 20;
						}
						else if (bondAttributes[curBond].topo == TOPO_RING) {
							incr = 3;
						}
						else {
							incr = 1;
						}
						if (sameSide) {
							sameSideWeight += incr;
						}
						else {
							otherSideWeight += incr;
							otherSideAtoms.append(atom3);
						}
					}
				}

			    if (otherSideWeight > sameSideWeight) {
					// If moving line to the other side, we need to make sure
					// the line is in the plane of the atoms on the other side
					atom3 = otherSideAtoms[0];
					coord3 = chemData->getAtomCoordinates(atom3);
					SbPlane tmpPlane(coord1, coord2, coord3);
					planeNormal = tmpPlane.getNormal();
					perpNormal = line12.getClosestPoint(coord3);
					perpNormal = coord3 - perpNormal;
					perpNormal.normalize();

					coord3 = (coord2 - coord1) * 0.5f + coord1 + planeNormal;
					//SbPlane plane(perpNormal, coord3);
			    }
				else if (otherSideWeight == sameSideWeight) {
					if ((bondAttributes[i].topo != TOPO_RING) &&
						(bondAttributes[i].topo != TOPO_AROMATIC)) {
						bondAttributes[i].isSymmetric = TRUE;
						bondAttributes[i].perpNormal = perpNormal;
						bondAttributes[i].planeNormal = planeNormal;
						continue;
					}
				}

				// For these non-symmetric double bonds, figure out the
				// start and end points are for the rendering.  If
				// there are 2 double-type bonds next to each other, then
				// we need to find the point on the angle bisector that
				// the second line will start or end at.
				float   dist;

				bondAttributes[i].start = coord1 + multiSep*perpNormal;
				bondAttributes[i].end = coord2 + multiSep*perpNormal;
				for (j = 0; j < natch1; j+=2) {
					atom3 = atch[atom1][j];
					if (atom3 == atom2) continue;

					curBond = atch[atom1][j+1];
// --> moiv doublebond error
						curBond = bondAttributesIndex[curBond];// global to local index
						assert(bondAttributes[curBond].index == atch[atom1][j+1]);
// <-- moiv doublebond error
					if ((bondAttributes[curBond].type ==
							ChemBaseData::DOUBLE_BOND) ||
						(bondAttributes[curBond].type ==
							ChemBaseData::RESONANCE_BOND)) {

						coord3 = chemData->getAtomCoordinates(atom3);
						if (otherSideWeight > sameSideWeight) {
							sameSide = !plane.isInHalfSpace(coord3);
						}
						else {
							sameSide = plane.isInHalfSpace(coord3);
						}

						if (sameSide) {
							tmpVec = coord3 - coord1;
							tmpVec.normalize();
							cosTheta = tmpVec.dot(vec21);
							if (cosTheta < -1.0) cosTheta = -1.0;
							if (cosTheta >  1.0) cosTheta =  1.0;
							theta = acosf(cosTheta);
							if (theta != 0.0) {
								dist = multiSep * tanf((M_PI - theta) * 0.5);
								bondAttributes[i].start =
									coord1 + dist*vec21 + multiSep*perpNormal;
							}
						}
					}
				}

				vec21.negate();
				for (j = 0; j < natch2; j+=2) {
					atom3 = atch[atom2][j];
					if (atom3 == atom1) continue;

					curBond = atch[atom2][j+1];
// --> moiv doublebond error
						curBond = bondAttributesIndex[curBond];// global to local index
						assert(bondAttributes[curBond].index == atch[atom2][j+1]);
// <-- moiv doublebond error
					if ((bondAttributes[curBond].type ==
							ChemBaseData::DOUBLE_BOND) ||
						(bondAttributes[curBond].type ==
							ChemBaseData::RESONANCE_BOND)) {

						coord3 = chemData->getAtomCoordinates(atom3);
						if (otherSideWeight > sameSideWeight) {
							sameSide = !plane.isInHalfSpace(coord3);
						}
						else {
							sameSide = plane.isInHalfSpace(coord3);
						}

						if (sameSide) {
							tmpVec = coord3 - coord2;
							tmpVec.normalize();
							cosTheta = tmpVec.dot(vec21);
							if (cosTheta < -1.0) cosTheta = -1.0;
							if (cosTheta >  1.0) cosTheta =  1.0;
							theta = acosf(cosTheta);
							if (theta != 0.0) {
								dist = multiSep * tanf((M_PI - theta) * 0.5);
								bondAttributes[i].end =
									coord2 + dist*vec21 + multiSep*perpNormal;
							}
						}
					}
				}
			}

			bondAttributes[i].perpNormal = perpNormal;
			bondAttributes[i].planeNormal = planeNormal;
		}
	}
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Generates a list of bonds with more lines than 2.
//
// Use: private

void
ChemDisplay::generateHigherBondList(ChemBaseData *chemData,
	int32_t localNumberOfBonds, ChemIntList *atch, 
	ChemBondStuff *bondAttributes)
//
////////////////////////////////////////////////////////////////////////
{
	int32_t i;
	int32_t atom1;
	int32_t atom2;
	SbVec3f planeNormal;
	SbVec3f perpNormal;
	SbVec3f coord1, coord2, coord3;

	for (i = 0; i < localNumberOfBonds; i++) {
		if ((bondAttributes[i].type == ChemBaseData::TRIPLE_BOND) ||
			(bondAttributes[i].type == ChemBaseData::QUADRUPLE_BOND)) {
			atom1 = bondAttributes[i].from;
			atom2 = bondAttributes[i].to;
			coord1 = chemData->getAtomCoordinates(atom1);
			coord2 = chemData->getAtomCoordinates(atom2);

			// Find a 3rd point to use to define the plane.
			coord3 = coord1;
			SbVec3f tmpVec = coord2 - coord1;

			// If the bond is parallel to Y axis, then increment the 
			// X coordinate of atom1 by 1 and make that coord3.
			if (((tmpVec[0] >= -0.01) && (tmpVec[0] <= 0.01)) &&
				((tmpVec[2] >= -0.01) && (tmpVec[2] <= 0.01))) {
				coord3[0] += 1.0;
			}

			// If the bond is not along the Y axis, increment the
			// Y coordinate of atom1 by 1 and make that coord3.
			else {
				coord3[1] += 1.0;
			}

			// Create the first plane using coord1, coord2 and coord3,
			// then find its normal
			SbPlane tmpPlane(coord1, coord2, coord3);
			planeNormal = tmpPlane.getNormal();

			// Get the midpoint of the bond and create a new coord3
			// that goes in the direction of the first normal beginning
			// at the bond midpoint.  Create a new plane.  This plane will
			// intersect the bond and its normal will point in the direction
			// (or the inverse of the direction) that the double bond should
			// be drawn.
			coord3 = (coord2 - coord1) * 0.5f + coord1 + planeNormal;
			SbPlane plane(coord1, coord2, coord3);
			perpNormal = plane.getNormal();

			bondAttributes[i].isSymmetric = TRUE;
			bondAttributes[i].planeNormal = planeNormal;
			bondAttributes[i].perpNormal = perpNormal;
		}
	}
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Check to see if a point is between clipping planes.  Used
//    with the ChemBBoxAction.
//
// Use: static

static SbBool
isPointBetweenPlanes(const SbVec3f &point, const SbViewVolume &viewVol,
	const SoClipPlaneElement *clipPlaneElement)
//
////////////////////////////////////////////////////////////////////////
{
	int32_t i;
	float t;

	t = viewVol.getProjectionDirection().dot(
		point - viewVol.getProjectionPoint());
	if ((t < viewVol.getNearDist()) ||
		(t > viewVol.getNearDist() + viewVol.getDepth())) {
		return FALSE;
	}

	int32_t eltNum = clipPlaneElement->getNum();
	for (i = 0; i < eltNum; i++) {
		const SbPlane &thePlane = clipPlaneElement->get(i, TRUE);
		if (!thePlane.isInHalfSpace(point)) {
			return FALSE;
		}
	}
	return TRUE;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Check to see if a bounding box is between clipping planes.  Used
//    with the ChemBBoxAction.
//
// Use: static

static SbBool
isBoxBetweenPlanes(const SbBox3f &box, const SbViewVolume &viewVol,
	const SoClipPlaneElement *clipPlaneElement)
//
////////////////////////////////////////////////////////////////////////
{
	int32_t i;
	float t;
	SbVec3f boxMin, boxMax;
	SbBool boxMinIn = TRUE;
	SbBool boxMaxIn = TRUE;

	boxMin = box.getMin();
	boxMax = box.getMax();

	t = viewVol.getProjectionDirection().dot(
		boxMin - viewVol.getProjectionPoint());
	if ((t < viewVol.getNearDist()) ||
		(t > viewVol.getNearDist() + viewVol.getDepth())) {
		boxMinIn = FALSE;
	}

	t = viewVol.getProjectionDirection().dot(
		boxMax - viewVol.getProjectionPoint());
	if ((t < viewVol.getNearDist()) ||
		(t > viewVol.getNearDist() + viewVol.getDepth())) {
		boxMaxIn = FALSE;
	}

	if ((!boxMinIn) && (!boxMaxIn)) return FALSE;

	int32_t eltNum = clipPlaneElement->getNum();
	for (i = 0; i < eltNum; i++) {
		const SbPlane &thePlane = clipPlaneElement->get(i, TRUE);
		if (!thePlane.isInHalfSpace(boxMin)) {
			boxMinIn = FALSE;
		}
		if (!thePlane.isInHalfSpace(boxMax)) {
			boxMaxIn = FALSE;
		}
		if ((!boxMinIn) && (!boxMaxIn)) return FALSE;
	}
	return TRUE;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Check to see if a Silhouette is between clipping planes.  Used
//    with the ChemBBoxAction.
//
// Use: static

static SbBool
isSilhouetteBetweenPlanes(const Silhouette &s,
	const SbViewVolume &viewVol, const SoClipPlaneElement *clipPlaneElement)
//
////////////////////////////////////////////////////////////////////////
{
	int32_t i;
	float t;
	SbBool isIn[4];

	for (i = 0; i < 4; i++) {
		isIn[i] = TRUE;
		t = viewVol.getProjectionDirection().dot(
			s[i] - viewVol.getProjectionPoint());
		if ((t < viewVol.getNearDist()) ||
			(t > viewVol.getNearDist() + viewVol.getDepth())) {
			isIn[i] = FALSE;
		}
	}
	if ((!isIn[0]) && (!isIn[1]) && (!isIn[2]) && (!isIn[3])) {
		return FALSE;
	}

	int32_t eltNum = clipPlaneElement->getNum();
	for (i = 0; i < eltNum; i++) {
		const SbPlane &thePlane = clipPlaneElement->get(i, TRUE);
		if (!thePlane.isInHalfSpace(s[0])) {
			isIn[0] = FALSE;
		}
		if (!thePlane.isInHalfSpace(s[1])) {
			isIn[1] = FALSE;
		}
		if (!thePlane.isInHalfSpace(s[2])) {
			isIn[2] = FALSE;
		}
		if (!thePlane.isInHalfSpace(s[3])) {
			isIn[3] = FALSE;
		}
		if ((!isIn[0]) && (!isIn[1]) && (!isIn[2]) && (!isIn[3])) {
			return FALSE;
		}
	}
	return TRUE;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Checks for aromatic rings in chemData.
//
// Use: static

static void
checkAromaticity(ChemBaseData *chemData, 
	int32_t numberOfRings, int32_t maxRingSize, float aromaticRingScale,
	SbIntList *ringList, ChemBondStuff *bondAttributes, 
	SbPList &aromaticRingList)
//
////////////////////////////////////////////////////////////////////////
{
	int32_t i, j;
	int32_t aromaticSum;
	int32_t resonanceSum;
	int32_t ringSize;
	ChemAromaticRing *aromaticRing;
	RWBitVec bondTouched(maxRingSize, FALSE);

	int32_t atomCounter, bondCounter;
	int32_t curAtom, curBond;
	int32_t from, to;

	// For each ring, check to see if it is aromatic or not.  For now, this
	// means that all bonds in the ring are of type ChemBaseData::AROMATIC_BOND.
	// If one bond in the ring is marked aromatic, then all must be marked 
	// aromatic.  
	// For ifdef AROMATIC_TO_RESONANCE:
	//     for those rings that are partially marked aromatic, change 
	//     all bonds to ChemBaseData::RESONANCE_BOND.

	for (i = 0; i < numberOfRings; i++) {
		aromaticSum = 0;
		resonanceSum = 0;
		ringSize = ringList[i].getLength();
		for (j = 0; j < ringSize; j++) {
			if (bondAttributes[ringList[i][j]].type ==
				ChemBaseData::AROMATIC_BOND) {
				aromaticSum++;
			}
			else if (bondAttributes[ringList[i][j]].type ==
				ChemBaseData::RESONANCE_BOND) {
				resonanceSum++;
			}
		}
		if (aromaticSum == ringSize) {
			// All bonds in the ring are of type ChemBaseData::AROMATIC_BOND.
			// Mark bonds as TOPO_AROMATIC.

			aromaticRing = new ChemAromaticRing(ringSize);
			for (j = 0; j < ringSize; j++) {
				bondAttributes[ringList[i][j]].topo = TOPO_AROMATIC;
			}

			for (j = 0; j < maxRingSize; j++) {
				bondTouched.clearBit(j);
			}

			// Need to add vertices in a connected order
			from = bondAttributes[ringList[i][0]].from;
			to =   bondAttributes[ringList[i][0]].to;
			aromaticRing->vertices[0] = chemData->getAtomCoordinates(from);
			aromaticRing->vertices[1] = chemData->getAtomCoordinates(to);
			aromaticRing->bonds[0] = ringList[i][0];
			bondTouched.setBit(0);

			atomCounter = 2;
			bondCounter = 1;
			curAtom = to;
			while (atomCounter < ringSize) {
				// Start a j = 1 since we have already done the first bond
				for (j = 1; j < ringSize; j++) {
					//if (!bondTouched(j)) {
					if (!bondTouched[j]) {
						curBond = ringList[i][j];
						from = bondAttributes[curBond].from;
						to   = bondAttributes[curBond].to;
						if (from == curAtom) {
							aromaticRing->vertices[atomCounter++] =
								chemData->getAtomCoordinates(to);
							aromaticRing->bonds[bondCounter++] = curBond;
							bondTouched.setBit(j);
							curAtom = to;
							if (atomCounter >= ringSize) break;
						}
						else if (to == curAtom) {
							aromaticRing->vertices[atomCounter++] =
								chemData->getAtomCoordinates(from);
							aromaticRing->bonds[bondCounter++] = curBond;
							bondTouched.setBit(j);
							curAtom = from;
							if (atomCounter >= ringSize) break;
						}
					}
				}
			}

			// At this point, there is one more bond that needs to be added
			// to aromaticRing->bonds.
			curBond = bondTouched.firstFalse();
			assert(curBond < ringSize);
			aromaticRing->bonds[bondCounter] = ringList[i][curBond];

			// Generate the transformation matrices for the aromatic rings
			aromaticRing->generateMatrix(aromaticRingScale);

			// Add to aromaticRingList
			aromaticRingList.append(aromaticRing);
		}
#ifdef AROMATIC_TO_RESONANCE
		else {
			// Here, there are not as many bonds marked aromatic as there
			// bonds in the ring.  Make sure any that are marked aromatic
			// are changed to resonance.  Need a check here since with
			// 2 fused rings, a bond may be aromatic in one ring but not
			// aromatic in another ring.
			for (j = 0; j < ringSize; j++) {
				if (bondAttributes[ringList[i][j]].type ==
					ChemBaseData::AROMATIC_BOND) {
					bondAttributes[ringList[i][j]].type =
						ChemBaseData::RESONANCE_BOND;
				}
			}
		}
#endif
		else if (resonanceSum == ringSize) {
			// If all bonds in the ring are marked as resonance, then
			// set the topo to TOPO_AROMATIC.  Used for sidedness in
			// double bond placement.
			for (j = 0; j < ringSize; j++) {
				bondAttributes[ringList[i][j]].topo = TOPO_AROMATIC;
			}
		}
		else if (ringSize == 6) {
			// Look for alternating single/double bonds in 6-membered rings.
			// Used for sidedness in double bond placement.
			if (bondAttributes[ringList[i][0]].type ==
				ChemBaseData::DOUBLE_BOND) {
				if ((bondAttributes[ringList[i][1]].type ==
					 ChemBaseData::SINGLE_BOND) &&
					(bondAttributes[ringList[i][2]].type ==
					 ChemBaseData::DOUBLE_BOND) &&
					(bondAttributes[ringList[i][3]].type ==
					 ChemBaseData::SINGLE_BOND) &&
					(bondAttributes[ringList[i][4]].type ==
					 ChemBaseData::DOUBLE_BOND) &&
					(bondAttributes[ringList[i][5]].type ==
					 ChemBaseData::SINGLE_BOND)) {
					for (j = 0; j < 6; j++) {
						bondAttributes[ringList[i][j]].topo = TOPO_AROMATIC;
					}
				}
			}
			else if (bondAttributes[ringList[i][0]].type ==
				ChemBaseData::SINGLE_BOND) {
				if ((bondAttributes[ringList[i][1]].type ==
					 ChemBaseData::DOUBLE_BOND) &&
					(bondAttributes[ringList[i][2]].type ==
					 ChemBaseData::SINGLE_BOND) &&
					(bondAttributes[ringList[i][3]].type ==
					 ChemBaseData::DOUBLE_BOND) &&
					(bondAttributes[ringList[i][4]].type ==
					 ChemBaseData::SINGLE_BOND) &&
					(bondAttributes[ringList[i][5]].type ==
					 ChemBaseData::DOUBLE_BOND)) {
					for (j = 0; j < 6; j++) {
						bondAttributes[ringList[i][j]].topo = TOPO_AROMATIC;
					}
				}
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Checks for a linear sequence of double or resonance bonds
//
// Use: static

static void
getLinearPaths(SbIntList &theList, ChemBondStuff *bondAttributes,
	int32_t &numPaths, SbIntList *&thePaths)
//
////////////////////////////////////////////////////////////////////////
{
	// Depending on where this is called from, theList contains a list of
	// all double bonds or all resonance bonds.  We are trying to determine
	// which of these are connected.

	int32_t listLen = theList.getLength();

	numPaths = -1;
	if (listLen > 1) {
		int32_t j;
		int32_t atom1, atom2;
		int32_t newAtom1, newAtom2;
		int32_t index;
		SbBool done;

		thePaths = new SbIntList[listLen];
		done = FALSE;

		while (!done) {
			listLen = theList.getLength();
			if (listLen > 1) {
				numPaths++;
				thePaths[numPaths].append(theList[0]);
				newAtom1 = atom1 = bondAttributes[theList[0]].to;
				for (j = 1; j < listLen; j++) {
					if (bondAttributes[theList[j]].from == newAtom1) {
						thePaths[numPaths].append(theList[j]);
						newAtom1 = bondAttributes[theList[j]].to;
					}
					else if (bondAttributes[theList[j]].to == newAtom1) {
						// Switch the from and to atoms so that we
						// have a directed list of atoms from one end to the
						// other
						bondAttributes[theList[j]].to =
							bondAttributes[theList[j]].from;
						bondAttributes[theList[j]].from = newAtom1;
						thePaths[numPaths].append(theList[j]);
						newAtom1 = bondAttributes[theList[j]].to;
					}
				}
				newAtom2 = atom2 = bondAttributes[theList[0]].from;
				for (j = 1; j < listLen; j++) {
					if (bondAttributes[theList[j]].from == newAtom2) {
						// Switch the from and to atoms so that we
						// have a directed list of atoms from one end to the
						// other
						bondAttributes[theList[j]].from =
							bondAttributes[theList[j]].to;
						bondAttributes[theList[j]].to = newAtom2;
						thePaths[numPaths].insert(theList[j], 0);
						newAtom2 = bondAttributes[theList[j]].from;
					}
					else if (bondAttributes[theList[j]].to == newAtom2) {
						thePaths[numPaths].insert(theList[j], 0);
						newAtom2 = bondAttributes[theList[j]].from;
					}
				}

				// Check to see if newAtom1 == atom1 and newAtom2 == atom2.
				// If so, then the length of this list is 1, and we don't want
				// to keep track of lists with length 1.

				if ((newAtom1 == atom1) && (newAtom2 == atom2)) {
					thePaths[numPaths].truncate(0);
					theList.remove(0);
					numPaths--;
				}
				else {
					// The following removes the bonds found in the current
					// pass through the while loop from theList so that they
					// won't be processed in a subsequent pass.
					for (j = 0; j < thePaths[numPaths].getLength(); j++) {
						index = theList.find(thePaths[numPaths][j]);
						if (index != -1) theList.remove(index);
					}
				}
			}
			else {
				done = TRUE;
			}
		}
	}
	numPaths++;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Sets the information used to render resonance bonds
//
// Use: static

static void
processLinearResonanceBonds(float multiSep, int32_t numPaths,
	SbIntList *rPaths, ChemIntList *atch, ChemBondStuff *bondAttributes,
	ChemBaseData *chemData)
//
////////////////////////////////////////////////////////////////////////
{
	int32_t i, j;
	int32_t pathLen;
	int32_t firstBond, lastBond;
	SbBool  firstIsLinear, lastIsLinear;
	SbVec3f coord1, coord2, coord3;
	SbVec3f firstPlaneNormal, lastPlaneNormal;
	SbVec3f firstPerpNormal, lastPerpNormal;
	SbVec3f firstVec21, lastVec21;

	for (i = 0; i < numPaths; i++) {
		// Figure out the first plane and use this for all bonds.
		// For these resonance bonds, we  also need to place all of the 
		// stippled lines on the same side.
		pathLen = rPaths[i].getLength();

		// Get the first 2 bonds and determine the first atom in
		// the list.
		firstBond = rPaths[i][0];
		lastBond = rPaths[i][pathLen-1];

		getPlanes(firstBond, lastBond,
			atch, bondAttributes, chemData,
			coord1, coord2,
			firstIsLinear, lastIsLinear, firstVec21, lastVec21,
			firstPlaneNormal, firstPerpNormal,
			lastPlaneNormal, lastPerpNormal);

		// If everything is still linear, make up something
		if (firstIsLinear && lastIsLinear) {
			// See if "last" is parallel to Y.  We use "last" since it has
			// its values stored in coord1, coord2, vec21, etc.
			coord3 = coord1;

			// If the bond is parallel to Y axis, then increment the 
			// X coord of atom1 by 1 and make that coord3.
			if (((lastVec21[0] >= -0.01) && (lastVec21[0] <= 0.01)) &&
				((lastVec21[2] >= -0.01) && (lastVec21[2] <= 0.01))) {
				coord3[0] += 1.0;
			}

			// If the bond is not along the Y axis, increment the
			// Y coord of atom1 by 1 and make that coord3.
			else {
				coord3[1] += 1.0;
			}

			// Create the first plane using coord1, coord2 and coord3,
			// then find its normal.  The planeNormal is used in the
			// determination of bounding boxes.
			SbPlane tmpPlane(coord1, coord2, coord3);
			lastPlaneNormal = tmpPlane.getNormal();

			// Drop a perpendicular from coord3 to the line defined by coord1 
			// and coord2.  This line is defined by coord3 - closestPoint on
			// the coord1-coord2 line.  When this is normalized, this
			// is the normal perpendicular to the bond along which the
			// the second line should be translated.
			SbLine lastLine12(coord1, coord2);
			lastPerpNormal = lastLine12.getClosestPoint(coord3);
			lastPerpNormal = coord3 - lastPerpNormal;
			lastPerpNormal.normalize();

			// Now make the firstPerpNormal which is a vector perpendicular
			// to vec21 and lastPerpNormal.  This is found using the cross
			// product.
			firstPlaneNormal = firstVec21.cross(lastPlaneNormal);
			firstPlaneNormal.normalize();
			firstPerpNormal = firstVec21.cross(lastPerpNormal);
			firstPerpNormal.normalize();
		}
		else if (firstIsLinear) {
			// Make the plane of the first bond equal to the plane of the
			// last bond.
			firstPlaneNormal = firstVec21.cross(lastPlaneNormal);
			firstPlaneNormal.normalize();
			firstPerpNormal = firstVec21.cross(lastPerpNormal);
			firstPerpNormal.normalize();
		}

		// Set the bondAttributes for the first bond
		bondAttributes[rPaths[i][0]].planeNormal = firstPlaneNormal;
		bondAttributes[rPaths[i][0]].perpNormal = firstPerpNormal;
		bondAttributes[rPaths[i][0]].isSymmetric = FALSE;
		bondAttributes[rPaths[i][0]].start = 
			chemData->getAtomCoordinates(bondAttributes[rPaths[i][0]].from);
		bondAttributes[rPaths[i][0]].end =
			chemData->getAtomCoordinates(bondAttributes[rPaths[i][0]].to);

		SbVec3f vec12, vec21;
		SbVec3f vec23, vec32;
		SbVec3f d, g;
		SbVec3f closestPoint;
		float cosTheta, theta, dist, dihedral, sign;

		// Make sure the perpNormal of the first bond is going the way we
		// want.  That is, we want the "to" coord of the second bond to be
		// in the half-space of the perpNormal plane.  This way, the first
		// 2 resonance bonds will be in the "inside" of the first angle instead
		// of the "outside".
		SbPlane tmpPlane(bondAttributes[rPaths[i][0]].perpNormal,
			bondAttributes[rPaths[i][0]].start);
		if (!tmpPlane.isInHalfSpace(
			chemData->getAtomCoordinates(bondAttributes[rPaths[i][1]].to))) {
			bondAttributes[rPaths[i][0]].perpNormal.negate();
		}
		bondAttributes[rPaths[i][0]].start +=
			multiSep * bondAttributes[rPaths[i][0]].perpNormal;

		// Now find the normals for the rest of the bonds in the list
		for (j = 1; j < pathLen; j++) {
			bondAttributes[rPaths[i][j]].start =
				chemData->getAtomCoordinates(bondAttributes[rPaths[i][j]].from);
			bondAttributes[rPaths[i][j]].end =
				chemData->getAtomCoordinates(bondAttributes[rPaths[i][j]].to);

			// firstAtom2 should be the same as lastAtom1
			assert(bondAttributes[rPaths[i][j-1]].to ==
				bondAttributes[rPaths[i][j]].from);
			coord1 =
				chemData->getAtomCoordinates(bondAttributes[rPaths[i][j-1]].from);
			coord2 =
				chemData->getAtomCoordinates(bondAttributes[rPaths[i][j]].from);
			coord3 =
				chemData->getAtomCoordinates(bondAttributes[rPaths[i][j]].to);

			// Get the angle of coords 1, 2, 3
			vec21 = coord2 - coord1;
			vec21.normalize();
			vec12 = vec21;
			vec12.negate();
			vec32 = coord3 - coord2;
			vec32.normalize();
			vec23 = vec32;
			vec23.negate();
			cosTheta = vec12.dot(vec32);
			if (cosTheta < -1.0) cosTheta = -1.0;
			if (cosTheta >  1.0) cosTheta =  1.0;
			theta = acosf(cosTheta);

			// Get the dihedral
			d = bondAttributes[rPaths[i][j-1]].perpNormal.cross(vec21);
			g = vec21.cross(vec23);
			d.normalize(); g.normalize();
			dihedral = d.dot(g);
			if (dihedral > 1.0) dihedral = 1.0;
			if (dihedral < -1.0) dihedral = -1.0;
			dihedral = acosf(dihedral);
			sign = bondAttributes[rPaths[i][j-1]].perpNormal.dot(g);
			if (sign > 0.0) dihedral = -dihedral;

			// Need to find the angle bisector.  Get the line from
			// coord1 to coord3 and find the closest point on this line
			// to coord2.  The line from coord2 to this closest point will
			// be a line which bisects the angle.  Then we need to determine
			// if this angle-bisector line is going in the correct direction
			// or if it needs to be negated.  We use a dihedral angle
			// calculator to figure out this sidedness.
			SbLine line13(coord1, coord3);
			closestPoint = line13.getClosestPoint(coord2);
			closestPoint = closestPoint - coord2;
			closestPoint.normalize();

			// If dihedral is between -M_PI_2 and M_PI_2 then the line from
			// coord2 to the closestPoint will work.  If not, then we need 
			// to negate the coord2->closestPoint line.
			if ((dihedral < -M_PI_2) || (dihedral > M_PI_2)) {
				closestPoint.negate();
				theta = (2*M_PI - theta) * 0.5 - M_PI_2;;
				dist = multiSep / cosf(theta);
			}
			else {
				theta *= 0.5;
				dist = multiSep / sinf(theta);
			}

			bondAttributes[rPaths[i][j]].planeNormal =
				closestPoint.cross(vec32);
			bondAttributes[rPaths[i][j]].start = coord2 + dist*closestPoint;
			bondAttributes[rPaths[i][j-1]].end =
				bondAttributes[rPaths[i][j]].start;

			// Get the perpNormal for the current bond.  Create a line that
			// goes from coord3 to coord2 and find the closest point on that
			// line to the current bond's "start".  The vector between "start"
			// and "closestPoint" will be perpendicular to the current bond.
			SbLine tmpLine(coord3, coord2);
			closestPoint =
				tmpLine.getClosestPoint(bondAttributes[rPaths[i][j]].start);
			bondAttributes[rPaths[i][j]].perpNormal =
				bondAttributes[rPaths[i][j]].start - closestPoint;
			bondAttributes[rPaths[i][j]].perpNormal.normalize();

			bondAttributes[rPaths[i][j]].isSymmetric = FALSE;
		}
		// Now we have to set the "end" point of the last bond in the list.
		bondAttributes[rPaths[i][pathLen-1]].end =
			coord3 + multiSep*bondAttributes[rPaths[i][pathLen-1]].perpNormal;
	}
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Sets the information used to render cumulenes (double bonds alternate
//    planes)
//
// Use: static

static void
processCumulenes(int32_t numPaths, SbIntList *dPaths, ChemIntList *atch,
	ChemBondStuff *bondAttributes, ChemBaseData *chemData)
//
////////////////////////////////////////////////////////////////////////
{
	if (numPaths <= 0) return;

	int32_t i, j;
	int32_t pathLen;
	int32_t firstBond, lastBond;
	int32_t firstAtom1, firstAtom2;
	int32_t lastAtom1, lastAtom2;
	SbBool  firstIsLinear, lastIsLinear, even;
	SbVec3f coord1, coord2, coord3;
	SbVec3f firstPlaneNormal, lastPlaneNormal;
	SbVec3f firstPerpNormal, lastPerpNormal;
	SbVec3f firstVec21, lastVec21;

	// Now work through the paths and determine sidedness
	for (i = 0; i < numPaths; i++) {
		// Get the first and last atoms in the path and determine the
		// planes of their bonds.  We need to do different things depending
		// on whether there are an even or odd number of bonds in the path.
		// If there are an even number of bonds, then in the ideal case,
		// the planes of the 2 end atoms will be perpendicular to each
		// other.  If there are an odd number of bonds, then the planes of
		// the 2 end atoms will be the same.
		pathLen = dPaths[i].getLength();
		even = ((pathLen % 2) == 0);

		// Get the first 2 bonds and determine the first atom in
		// the list.
		firstBond = dPaths[i][0];
		lastBond = dPaths[i][pathLen-1];

		getPlanes(firstBond, lastBond, 
			atch, bondAttributes, chemData,
			coord1, coord2,
			firstIsLinear, lastIsLinear, firstVec21, lastVec21,
			firstPlaneNormal, firstPerpNormal,
			lastPlaneNormal, lastPerpNormal);

		// If everything is still linear, make up something
		if (firstIsLinear && lastIsLinear) {
			// See if "last" is parallel to Y.  We use "last" since it has
			// its values stored in coord1, coord2, vec21, etc.
			coord3 = coord1;

			// If the bond is parallel to Y axis, then increment the 
			// X coord of atom1 by 1 and make that coord3.
			if (((lastVec21[0] >= -0.01) && (lastVec21[0] <= 0.01)) &&
				((lastVec21[2] >= -0.01) && (lastVec21[2] <= 0.01))) {
				coord3[0] += 1.0;
			}

			// If the bond is not along the Y axis, increment the
			// Y coord of atom1 by 1 and make that coord3.
			else {
				coord3[1] += 1.0;
			}

			// Create the first plane using coord1, coord2 and coord3,
			// then find its normal.  The planeNormal is used in the
			// determination of bounding boxes.
			SbPlane tmpPlane(coord1, coord2, coord3);
			lastPlaneNormal = tmpPlane.getNormal();

			// Drop a perpendicular from coord3 to the line defined by coord1 
			// and coord2.  This line is defined by coord3 - closestPoint on
			// the coord1-coord2 line.  When this is normalized, this
			// is the normal perpendicular to the bond along which the
			// the second line should be translated.
			SbLine lastLine12(coord1, coord2);
			lastPerpNormal = lastLine12.getClosestPoint(coord3);
			lastPerpNormal = coord3 - lastPerpNormal;
			lastPerpNormal.normalize();

			// Now make the firstPerpNormal which is a vector perpendicular
			// to vec21 and lastPerpNormal.  This is found using the cross
			// product.
			firstPlaneNormal = firstVec21.cross(lastPlaneNormal);
			firstPlaneNormal.normalize();
			firstPerpNormal = firstVec21.cross(lastPerpNormal);
			firstPerpNormal.normalize();
		}
		else if (firstIsLinear) {
			// If even, make the plane of the first bond perpendicular to the
			// plane of the last bond.  Use the cross product of the coords
			// of the atoms in lastBond and its perpNormal.  If odd, then
			// the first and last planes the same.
			if (even) {
				firstPlaneNormal = firstVec21.cross(lastPlaneNormal);
				firstPlaneNormal.normalize();
				firstPerpNormal = firstVec21.cross(lastPerpNormal);
				firstPerpNormal.normalize();
			}
			else {
				// NOT CORRECT
				firstPlaneNormal = lastPlaneNormal;
				firstPerpNormal = lastPerpNormal;
			}
		}
		else if (lastIsLinear) {
			// If even, make the plane of the last bond perpendicular to the
			// plane of the first bond.  Use the cross product of the coords
			// of the atoms in the firstBond and its perpNormal.  If odd, then
			// make the first and last planes the same.
			if (even) {
				lastPlaneNormal = lastVec21.cross(firstPlaneNormal);
				lastPlaneNormal.normalize();
				lastPerpNormal = lastVec21.cross(firstPerpNormal);
				lastPerpNormal.normalize();
			}
			else {
				// NOT CORRECT
				lastPlaneNormal = firstPlaneNormal;
				lastPerpNormal = firstPerpNormal;
			}
		}

		// Set the values for the normals of the first and last bonds in
		// the list
		bondAttributes[dPaths[i][0]].planeNormal = firstPlaneNormal;
		bondAttributes[dPaths[i][0]].perpNormal = firstPerpNormal;
		bondAttributes[dPaths[i][0]].isSymmetric = TRUE;

		bondAttributes[dPaths[i][pathLen-1]].planeNormal = lastPlaneNormal;
		bondAttributes[dPaths[i][pathLen-1]].perpNormal = lastPerpNormal;
		bondAttributes[dPaths[i][pathLen-1]].isSymmetric = TRUE;

		// Now find the normals for the rest of the bonds in the list
		for (j = 1; j < pathLen-1; j++) {
			// Need to make sure that vec21 goes the right direction, that is,
			// bondAttributes[j].from == bondAttributes[j-1].to.
			firstAtom1 = bondAttributes[dPaths[i][j-1]].from;
			firstAtom2 = bondAttributes[dPaths[i][j-1]].to;
			lastAtom1  = bondAttributes[dPaths[i][j]].from;
			lastAtom2  = bondAttributes[dPaths[i][j]].to;
			if ((lastAtom1 == firstAtom1) || (lastAtom1 == firstAtom2)) {
				coord1 = chemData->getAtomCoordinates(lastAtom1);
				coord2 = chemData->getAtomCoordinates(lastAtom2);
			}
			else {
				coord1 = chemData->getAtomCoordinates(lastAtom2);
				coord2 = chemData->getAtomCoordinates(lastAtom1);
			}
			firstVec21 = (coord2 - coord1);
			firstVec21.normalize();

			// Need to make sure we take the correct cross products.
			// If j is even, then...
			if ((j % 2) == 0) {
				bondAttributes[dPaths[i][j]].perpNormal =
					bondAttributes[dPaths[i][j-1]].perpNormal.cross(firstVec21);
				bondAttributes[dPaths[i][j]].perpNormal.normalize();

				bondAttributes[dPaths[i][j]].planeNormal =
					firstVec21.cross(bondAttributes[dPaths[i][j]].perpNormal);
				bondAttributes[dPaths[i][j]].planeNormal.normalize();
			}
			else {
				bondAttributes[dPaths[i][j]].perpNormal =
					firstVec21.cross(bondAttributes[dPaths[i][j-1]].perpNormal);
				bondAttributes[dPaths[i][j]].perpNormal.normalize();

				bondAttributes[dPaths[i][j]].planeNormal =
					bondAttributes[dPaths[i][j]].perpNormal.cross(firstVec21);
				bondAttributes[dPaths[i][j]].planeNormal.normalize();
			}
			bondAttributes[dPaths[i][j]].isSymmetric = TRUE;
		}
	}
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    When rendering double or resonance bonds, this returns the planes
//    in which to render the lines
//
// Use: static

static void
getPlanes(int32_t firstBond, int32_t lastBond,
	ChemIntList *atch, ChemBondStuff *bondAttributes,
	ChemBaseData *chemData,
	SbVec3f &coord1, SbVec3f &coord2,
	SbBool &firstIsLinear, SbBool &lastIsLinear,
	SbVec3f &firstVec21, SbVec3f &lastVec21,
	SbVec3f &firstPlaneNormal, SbVec3f &firstPerpNormal,
	SbVec3f &lastPlaneNormal, SbVec3f &lastPerpNormal)
//
////////////////////////////////////////////////////////////////////////
{
	int32_t j;
	int32_t firstAtom1, firstAtom2;
	int32_t lastAtom1, lastAtom2;
	int32_t atom3;
	int32_t natch1;
	SbVec3f coord3, tmpVec;
	float   cosTheta;

	float linearTolerance = (float)(5.0 * DEG_TO_RAD);

	// Determine the plane of the first bond in the list
	firstAtom1 = bondAttributes[firstBond].from;
	firstAtom2 = bondAttributes[firstBond].to;

#ifdef DONT_HAVE_TO_DO
	if ((firstAtom1 == bondAttributes[secondBond].from) ||
		(firstAtom1 == bondAttributes[secondBond].to)) {
		firstAtom1 = bondAttributes[firstBond].to;
		firstAtom2 = bondAttributes[firstBond].from;
	}
#endif

	coord1 = chemData->getAtomCoordinates(firstAtom1);
	coord2 = chemData->getAtomCoordinates(firstAtom2);
	natch1 = atch[firstAtom1].getLength();
	SbLine firstLine12(coord1, coord2);

	firstVec21 = coord2 - coord1;
	firstVec21.normalize();

	// Use the first non-linear, non-atom2 attachment to atom1
	// as the 3rd point defining the plane.
	firstIsLinear = TRUE;
	for (j = 0; j < natch1; j+=2) {
		atom3 = atch[firstAtom1][j];
		if (atom3 == firstAtom2) continue;

		coord3 = chemData->getAtomCoordinates(atom3);
		tmpVec = coord1 - coord3;
		tmpVec.normalize();
		cosTheta = firstVec21.dot(tmpVec);
		if (cosTheta < -1.0) cosTheta = -1.0;
		if (cosTheta >  1.0) cosTheta =  1.0;
		if (acosf(cosTheta) > linearTolerance) {
			firstIsLinear = FALSE;
			break;
		}
	}

	if (!firstIsLinear) {
		// Create the plane using coord1, coord2 and coord3,
		// then find its normal.  The planeNormal is used in the
		// determination of bounding boxes.
		SbPlane firstTmpPlane(coord1, coord2, coord3);
		firstPlaneNormal = firstTmpPlane.getNormal();

		// Drop a perpendicular from coord3 to the line defined by 
		// coord1 and coord2.  This line is defined by 
		// coord3 - closestPoint on the coord1-coord2 line.  When this
		// is normalized, this is the normal perpendicular to the bond
		// along which the the second line should be translated.
		firstPerpNormal = firstLine12.getClosestPoint(coord3);
		firstPerpNormal = coord3 - firstPerpNormal;
		firstPerpNormal.normalize();
	}

	// Determine the plane of the last bond in the list
	lastAtom1 = bondAttributes[lastBond].from;
	lastAtom2 = bondAttributes[lastBond].to;

#ifdef DONT_HAVE_TO_DO
	if ((lastAtom1 == bondAttributes[nextToLastBond].from) ||
		(lastAtom1 == bondAttributes[nextToLastBond].to)) {
		lastAtom1 = bondAttributes[lastBond].to;
		lastAtom2 = bondAttributes[lastBond].from;
	}
#endif

	coord1 = chemData->getAtomCoordinates(lastAtom1);
	coord2 = chemData->getAtomCoordinates(lastAtom2);
	natch1 = atch[lastAtom1].getLength();
	SbLine lastLine12(coord1, coord2);

	lastVec21 = coord2 - coord1;
	lastVec21.normalize();

	// Use the first non-linear, non-atom2 attachment to atom1
	// as the 3rd point defining the plane.
	lastIsLinear = TRUE;
	for (j = 0; j < natch1; j+=2) {
		atom3 = atch[lastAtom1][j];
		if (atom3 == lastAtom2) continue;

		coord3 = chemData->getAtomCoordinates(atom3);
		tmpVec = coord1 - coord3;
		tmpVec.normalize();
		cosTheta = lastVec21.dot(tmpVec);
		if (cosTheta < -1.0) cosTheta = -1.0;
		if (cosTheta >  1.0) cosTheta =  1.0;
		if (acosf(cosTheta) > linearTolerance) {
			lastIsLinear = FALSE;
			break;
		}
	}

	if (!lastIsLinear) {
		// Create the plane using coord1, coord2 and coord3,
		// then find its normal.  The planeNormal is used in the
		// determination of bounding boxes.
		SbPlane lastTmpPlane(coord1, coord2, coord3);
		lastPlaneNormal = lastTmpPlane.getNormal();

		// Drop a perpendicular from coord3 to the line defined by 
		// coord1 and coord2.  This line is defined by 
		// coord3 - closestPoint on the coord1-coord2 line.  When this
		// is normalized, this is the normal perpendicular to the bond
		// along which the the second line should be translated.
		lastPerpNormal = lastLine12.getClosestPoint(coord3);
		lastPerpNormal = coord3 - lastPerpNormal;
		lastPerpNormal.normalize();
	}
}

// --> residue selection
void ChemDisplay::getResidueBBoxes(SoAction *action, SbBool clipOnCenter,
                        ChemBondBBox *&bondBBoxes)
{

#ifdef DEBUG
printf ("In ChemDisplay::getResidueBBoxes\n");
#endif
	int32_t i;
	int32_t numResidues, count;
	bondBBoxes = NULL;

	if ((numResidues = getNumberOfResidues(action)) == 0)
	{
		return;
	}

	SoState *state = action->getState();
	if ((int)SoPickStyleElement::get(state) == (int)SoPickStyle::UNPICKABLE)
	{
		return;
	}

	ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);

	int displayStyle = cdp->residueDisplayStyle.getValue();

	if (displayStyle == ChemDisplayParam::DISPLAY_RESIDUES_NONE ||
		displayStyle == ChemDisplayParam::DISPLAY_RESIDUES_SCHEMATIC)
	{
		return;
	}

	count = 0;
	ChemWireframeBondBBox *wireBBox;
	ChemWireframeBondBBox *tmpBBox;
	int32_t numVertices;
	const SbVec2f *vert2;
	ChemStickBondBBox *stickBBox;
	ChemStickBondBBox *tmpSBBox;

  // Invoke the appropriate bBox as determined from the display style.
	switch (displayStyle)
	{
	  case ChemDisplayParam::DISPLAY_RESIDUES_CAWIRE:
		{
	    wireBBox = new ChemWireframeBondBBox(numResidues);

			eachBBoxResiduesAsWireframe(state, cdp, clipOnCenter, count, wireBBox);
			if (count == numResidues)
			{
				bondBBoxes = wireBBox;
			}
			else
			{
				tmpBBox = new ChemWireframeBondBBox(*wireBBox, count);
				bondBBoxes = tmpBBox;
				delete wireBBox;
			}
		}
		break;

	  case ChemDisplayParam::DISPLAY_RESIDUES_CASTICK:
		{
			if (residueUnitCylinder == NULL)
			{
				float rcc = cdp->residueCylinderComplexity.getValue();
				int cylDisplayStyle = cdp->residueCylinderDisplayStyle.getValue();
				lastResidueCylinderComplexity = rcc;
				lastResidueCylinderDisplayStyle = cylDisplayStyle;
				residueUnitCylinder = new ChemUnitCylinder(cylDisplayStyle, rcc);
			}
			numVertices = residueUnitCylinder->getNumberOfRingCoords();
			vert2 = residueUnitCylinder->getRingCoords();
			stickBBox = new ChemStickBondBBox(numResidues);
			stickBBox->numberOfVertices = numVertices;
			stickBBox->vertices = new SbVec3f[numVertices];
	
			for (i = 0; i < numVertices; i++)
			{
				stickBBox->vertices[i].setValue(vert2[i][0], 0.0, vert2[i][1]);
			}

			eachBBoxResiduesAsCylinders(state, cdp, clipOnCenter, count, stickBBox);
			if (count == numResidues)
			{
				bondBBoxes = stickBBox;
			}
			else
			{
				tmpSBBox = new ChemStickBondBBox(*stickBBox, count);
				bondBBoxes = tmpSBBox;
				delete stickBBox;
			}
		}
		break;

		case ChemDisplayParam::DISPLAY_RESIDUES_LINERIBBON:
		case ChemDisplayParam::DISPLAY_RESIDUES_FLATRIBBON:
		case ChemDisplayParam::DISPLAY_RESIDUES_SOLIDRIBBON:
		{
			wireBBox = new ChemWireframeBondBBox(numResidues);
			
			ChemResidueStuff *res;
			i=0;
			RESIDUELOOP_START(residueIndex)
			
				res = &residueAttributes[theResidue];
				assert(res != NULL);
				if (!res->isRibbonRenderable) continue;

				wireBBox->index[i] = res->index;
				wireBBox->center[i] = (pl[res->indices[1]] + pr[res->indices[2]])*0.5f;
				i++;
			RESIDUELOOP_END

			bondBBoxes = wireBBox;

		}
		break;

	  default:
		break;
	}
}

void ChemDisplay::getResidueLabelBBoxes(SoAction *action, SbBool clipOnCenter,
                        ChemLabelBBox *&residueLabelBBoxes)
{
#ifdef DEBUG
printf ("In ChemDisplay::getResidueLabelBBoxes\n");
#endif
	int32_t count;
	int32_t numberOfResidueLabels;
	residueLabelBBoxes = NULL;

	if ((numberOfResidueLabels = getNumberOfResidueLabels(action)) == 0)
	{
		return;
	}

	SoState *state = action->getState();
	if ((int)SoPickStyleElement::get(state) == (int)SoPickStyle::UNPICKABLE)
	{
		return;
	}

	ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);
	//ChemRadii *chemRadii = ChemRadiiElement::get(state);

	//int displayStyle = cdp->residueDisplayStyle.getValue();

	//if (displayStyle == ChemDisplayParam::DISPLAY_RESIDUES_NONE) return;
	if (cdp->showResidueLabels.getValue() == FALSE) return;

	ChemLabelBBox *tmpBBox = new ChemLabelBBox(numberOfResidueLabels);

	count = 0;
	eachBBoxResidueLabels(state,cdp,clipOnCenter,count,tmpBBox);
	
	if (count == numberOfResidueLabels)
	{
		residueLabelBBoxes = tmpBBox;
	}
	else
	{
		residueLabelBBoxes = new ChemLabelBBox(*tmpBBox, count);
		delete tmpBBox;
	}
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Calculates the bounding box of each residue when displayed as wireframe (cawire).
//
// Use: private

void ChemDisplay::eachBBoxResiduesAsWireframe(SoState *state, ChemDisplayParam *cdp,
																							SbBool clipOnCenter, int32_t &bCount, ChemWireframeBondBBox *bondBBoxes)
{
	ChemData *chemData = (ChemData*)ChemBaseDataElement::get(state);
	Silhouette tmpS;

	bCount = 0;
	const SoClipPlaneElement *clipPlaneElt = SoClipPlaneElement::getInstance(state);

	ChemResidueStuff* res = NULL;
	SbVec3f coords1, coords2, tcoord;

	RESIDUELOOP_START(residueIndex)
		res = &residueAttributes[theResidue];
		if (!res->isStickRenderable) continue;

		coords1 = chemData->getAtomCoordinates(res->from);
		coords2 = chemData->getAtomCoordinates(res->to);
		tcoord = coords2 - coords1;
		bondBBoxes->center[bCount] = tcoord * 0.5f + coords1;

		bondBBoxes->silhouette[bCount][0] = coords1;
		bondBBoxes->silhouette[bCount][1] = coords2;
		bondBBoxes->silhouette[bCount][2] = coords2;
		bondBBoxes->silhouette[bCount][3] = coords1;
		bondBBoxes->bondType[bCount] = ChemBaseData::SINGLE_BOND;


		if (clipOnCenter)
		{
			// Need to convert center to world coordinates for the clip plane
			// check (multiply by currentModelMatrix)
			SbVec3f center;
			currentModelMatrix.multVecMatrix(bondBBoxes->center[bCount],center);
			if (isPointBetweenPlanes(center, currentViewVolume,clipPlaneElt))
			{
				bondBBoxes->index[bCount] = residueAttributes[theResidue].index;
				bCount++;
			}
		}
		else
		{
			// Check if silhouette is between clipping planes.  If not, then
			// don't increment bCount.  First transform silhouette into world
			// coordinates.
			currentModelMatrix.multVecMatrix(bondBBoxes->silhouette[bCount][0],
				tmpS[0]);
			currentModelMatrix.multVecMatrix(bondBBoxes->silhouette[bCount][1],
				tmpS[1]);
			currentModelMatrix.multVecMatrix(bondBBoxes->silhouette[bCount][2],
				tmpS[2]);
			currentModelMatrix.multVecMatrix(bondBBoxes->silhouette[bCount][3],
				tmpS[3]);
			if (isSilhouetteBetweenPlanes(tmpS, currentViewVolume, clipPlaneElt))
			{
				bondBBoxes->index[bCount] = residueAttributes[theResidue].index;
				bCount++;
			}
		}
	RESIDUELOOP_END
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Computes the bounding box of the residues.
//    Invoked by the computeBBox method.
//
// Use: private

void ChemDisplay::eachBBoxResiduesAsCylinders(SoState *state, ChemDisplayParam *cdp,
																							SbBool clipOnCenter, int32_t &bCount, ChemStickBondBBox *bondBBoxes)
//
////////////////////////////////////////////////////////////////////////
{
	float rad;
	ChemData *chemData = (ChemData*) ChemBaseDataElement::get(state);

	const SoClipPlaneElement *clipPlaneElt = SoClipPlaneElement::getInstance(state);
	rad = cdp->residueCylinderRadius.getValue();

	// The individual bondBBoxes are for use in ChemSelection.  The following
	// code constructs a unit cylinder from 0 to 1 along the Z axis.
	// The matrix which takes this cylinder to the proper position for the
	// bond is found.  The current camera position is transformed into the
	// coordinate space of the unit cylinder by using the inverse of the
	// cylinder matrix and the current view matrix.  We can then determine
	// the "silhouette points".  These points are then transformed using
	// the cylinder matrix.  This idea is taken from Jim Blinn's "Optimal
	// Tubes" article from Jim Blinn's Corner in IEEE CG&A, 1989.

	bCount = 0;
	SbVec3f     theTranslation;
	SbRotation  theRotation;
	SbVec3f     theScale;
	SbMatrix    theMatrix;
	SbMatrix    eyeMatrix;
	SbMatrix    eyeMatrixInverse;
	SbVec3f     eyePoint;
	SbVec3f     yAxis(0.0, 1.0, 0.0);
	SbVec3f     zAxis(0.0, 0.0, 1.0);
	SbVec3f		coords1, coords2, tcoord;
	float		height;
	float		eyePointDistance, eyePointDistanceRecip;
	float		eyePointSqr;
	Silhouette	tmpS;

	ChemResidueStuff *res = NULL;
	RESIDUELOOP_START(residueIndex);

		res = &residueAttributes[theResidue];
		if (!res->isStickRenderable) continue;

		coords1 = chemData->getAtomCoordinates(res->from);
		coords2 = chemData->getAtomCoordinates(res->to);
		tcoord = coords2 - coords1;
		height = tcoord.length();
		bondBBoxes->center[bCount] = tcoord * 0.5f + coords1;

		theTranslation = coords1;
		theRotation.setValue(zAxis, tcoord);
		theScale.setValue(rad, rad, height);
		theMatrix.setTransform(theTranslation, theRotation, theScale);

		// Set the eyeMatrix.  We don't include the currentModelMatrix.
		// This is because we want the silhouette points to be in the
		// same space as the coordinates in ChemData.
		eyeMatrix = theMatrix;
		eyeMatrix.multLeft(currentViewMatrix);
		eyeMatrixInverse = eyeMatrix.inverse();

		// Transform the eyePoint
		eyePoint = currentViewVolume.getProjectionPoint();
		eyeMatrixInverse.multVecMatrix(eyePoint, eyePoint);
		eyePoint[2] = 0.0;

		// Determine the silhouette points
		tmpS[0][2] = tmpS[1][2] = 0.0;
		eyePointDistance =
			eyePoint[0]*eyePoint[0] + eyePoint[1]*eyePoint[1];

		// If the distance from the eyePoint to the origin (eyePointDistance)
		// is less than 1, then we are in the unit cylinder.  Set the
		// silhouette points to 0, 1, 0 and 0, -1, 0.
	if (eyePointDistance < 1.0)
	{
			tmpS[0][0] = tmpS[1][0] = 0.0f;
			tmpS[0][1] =  1.0f;
			tmpS[1][1] = -1.0f;
	}
	else
	{
			eyePointDistanceRecip = 1.0f / eyePointDistance;
			if (eyePointDistance == 1.0f)
			{
				// Here the eyePoint is right on the edge of the cylinder
				tmpS[0][0] = tmpS[1][0] = eyePoint[0] * eyePointDistanceRecip;
				tmpS[0][1] = tmpS[1][1] = eyePoint[1] * eyePointDistanceRecip;;
			}
			else
			{
				eyePointSqr = sqrtf(eyePointDistance - 1.0f);
				tmpS[0][0] = (eyePoint[0] - eyePoint[1]*eyePointSqr)
						* eyePointDistanceRecip;
				tmpS[1][0] = (eyePoint[0] + eyePoint[1]*eyePointSqr)
						* eyePointDistanceRecip;
				tmpS[0][1] = (eyePoint[1] + eyePoint[0]*eyePointSqr)
						* eyePointDistanceRecip;
				tmpS[1][1] = (eyePoint[1] - eyePoint[0]*eyePointSqr)
						* eyePointDistanceRecip;
			}
		}
		tmpS[2] = tmpS[0];
		tmpS[3] = tmpS[1];
		tmpS[2][2] = tmpS[3][2] = 1.0f;

		theMatrix.multVecMatrix(tmpS[0], bondBBoxes->silhouette[bCount][0]);
		theMatrix.multVecMatrix(tmpS[2], bondBBoxes->silhouette[bCount][1]);
		theMatrix.multVecMatrix(tmpS[3], bondBBoxes->silhouette[bCount][2]);
		theMatrix.multVecMatrix(tmpS[1], bondBBoxes->silhouette[bCount][3]);

		// Check if silhouette is between clipping planes.  If not, then
		// don't increment bCount.  First transform silhouette into world
		// coordinates.
		currentModelMatrix.multVecMatrix(bondBBoxes->silhouette[bCount][0],
			tmpS[0]);
		currentModelMatrix.multVecMatrix(bondBBoxes->silhouette[bCount][1],
			tmpS[1]);
		currentModelMatrix.multVecMatrix(bondBBoxes->silhouette[bCount][2],
			tmpS[2]);
		currentModelMatrix.multVecMatrix(bondBBoxes->silhouette[bCount][3],
			tmpS[3]);

		SbBool addBBox = FALSE;
		if (clipOnCenter)
		{
			// Need to convert center to world coordinates for the clip plane
			// check (multiply by currentModelMatrix)
			SbVec3f center;
			currentModelMatrix.multVecMatrix(bondBBoxes->center[bCount],
				center);
			if (isPointBetweenPlanes(center, currentViewVolume,clipPlaneElt))
			{
				addBBox = TRUE;
			}
		}
		else
		{
			if (isSilhouetteBetweenPlanes(tmpS, currentViewVolume, clipPlaneElt))
			{
				addBBox = TRUE;
			}
		}
		if (addBBox)
		{
			height *= 0.5f;
			theTranslation = tcoord * 0.5f + coords1;
			theRotation.setValue(yAxis, tcoord);
			theScale.setValue(rad, height, rad);
			theMatrix.setTransform(theTranslation, theRotation, theScale);

			bondBBoxes->transform[bCount] = theMatrix;
			bondBBoxes->index[bCount] = theResidue;
			bCount++;
		}

	RESIDUELOOP_END
}

#undef RESIDUELOOP_START
#undef RESIDUELOOP_END
// <-- residue selection

#undef DEG_TO_RAD
#undef END_OF_LIST

#undef TOPO_UNKNOWN
#undef TOPO_RING
#undef TOPO_AROMATIC

#undef ATOMLOOP_START
#undef ATOMLOOP_END
#undef BONDLOOP_START
#undef BONDLOOP_END
