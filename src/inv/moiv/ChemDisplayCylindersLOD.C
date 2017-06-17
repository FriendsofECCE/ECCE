// ChemDisplayCylindersLOD.cpp
// Implementation of the ChemDisplay LOD...cylinderXXX() methods
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

#include <assert.h>
#include <ctype.h>
#include <math.h>

#ifdef WIN32
#include <windows.h>
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

#include "inv/ChemKit/ChemLOD.H"

static int numBondLoops;
static int bondLoop;
static int32_t bondStart;
static int32_t bondEnd;
static int32_t theBond;

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

// --> octree duplicates
#define BNDDUP(ID) \
assert(ID>=0 && ID <globalNumberOfBonds && renderedBonds != NULL); \
if (renderedBonds[ID]) \
continue; \
else renderedBonds[ID] = true;
// <-- octree duplicates

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

#define PRE_ADJUST_CYLINDER \
ChemRadii* chemRadii = ChemRadiiElement::get(state); \
const float *atomRadii = chemRadii->atomRadii.getValues(0); \
float cylRad= cdp->bondCylinderRadius.getValue(); \
float radScale = cdp->atomRadiiScaleFactor.getValue(); \
if ((cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK)|| (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE)) \
	   radScale *= cdp->ballStickSphereScaleFactor.getValue();

#define POST_ADJUST_CYLINDER \
if (cdp->displayStyle.getValue() != ChemDisplayParam::DISPLAY_STICK) { \
bool renderFrom, renderTo; \
int localAtomIndex; \
ADJUST_CYLINDER(from,renderFrom);\
ADJUST_CYLINDER(to,renderTo); \
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

#define RENDER_LOD_OV(LOD) \
if (lodSelector->getBondIndexCnt(LOD) > 0) { \
	ChemRadii* chemRadii = ChemRadiiElement::get(state); \
	const float *atomRadii = chemRadii->atomRadii.getValues(0); \
	float cylRad= cdp->bondCylinderRadius.getValue(); \
	float radScale = cdp->atomRadiiScaleFactor.getValue(); \
	if (cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLSTICK || cdp->displayStyle.getValue() == ChemDisplayParam::DISPLAY_BALLWIRE) \
	   radScale *= cdp->ballStickSphereScaleFactor.getValue(); \
	float radFrom, radTo; \
	SbVec3f dir; \
	float t1,t2,dirlength; \
	SbVec2i fromTo;\
	beginCylinders(displayStyle,lodSelector->getBondCylinder(LOD)); \
	for (theBond=0; theBond< lodSelector->getBondIndexCnt(LOD); theBond ++) \
	{ \
		from = chemData->getBondFrom(lodSelector->getBondIndex(LOD,theBond)); \
		to = chemData->getBondTo(lodSelector->getBondIndex(LOD,theBond)); \
		coords1 = chemData->getAtomCoordinates(from).getValue(); \
		coords2 = chemData->getAtomCoordinates(to).getValue(); \
		POST_ADJUST_CYLINDER \
		fromTo.setValue(from,to); \
		lodSelector->getBondCylinder(LOD)->render(coords1, coords2, cylRad, lastCylinderDoNormals, lastCylinderDoTextures,displayStyle,fromTo,this,action); \
	} \
	endCylinders(displayStyle); \
}

#define RENDER_LOD_BB(LOD) \
if (lodSelector->getBondIndexCnt(LOD) > 0) { \
	PRE_ADJUST_CYLINDER \
	float radFrom, radTo; \
	SbVec3f dir; \
	float t1,t2,dirlength; \
	SbVec2i fromTo;\
	beginCylinders(displayStyle,lodSelector->getBondCylinder(LOD)); \
	const SbColor *bondColors = chemColor->bondColor.getValues(0); \
	for (theBond=0; theBond < lodSelector->getBondIndexCnt(LOD); theBond ++) \
	{ \
		from = chemData->getBondFrom(lodSelector->getBondIndex(LOD,theBond)); \
		to = chemData->getBondTo(lodSelector->getBondIndex(LOD,theBond)); \
		coords1 = chemData->getAtomCoordinates(from).getValue(); \
		coords2 = chemData->getAtomCoordinates(to).getValue(); \
		glColor3fv(bondColors[lodSelector->getBondIndex(LOD,theBond)].getValue()); \
		POST_ADJUST_CYLINDER \
		fromTo.setValue(from,to); \
		lodSelector->getBondCylinder(LOD)->render(coords1, coords2, cylRad, lastCylinderDoNormals, lastCylinderDoTextures,displayStyle,fromTo,this,action); \
	} \
	endCylinders(displayStyle); \
}

#define RENDER_LOD_BI(LOD) \
if (lodSelector->getBondIndexCnt(LOD) > 0) { \
	PRE_ADJUST_CYLINDER \
	float radFrom, radTo; \
	SbVec3f dir; \
	float t1,t2,dirlength; \
	SbVec2i fromTo;\
	beginCylinders(displayStyle,lodSelector->getBondCylinder(LOD)); \
	const SbColor *bondColors = chemColor->bondColor.getValues(0); \
	for (theBond=0; theBond < lodSelector->getBondIndexCnt(LOD); theBond ++) \
	{ \
		from = chemData->getBondFrom(lodSelector->getBondIndex(LOD,theBond)); \
		to = chemData->getBondTo(lodSelector->getBondIndex(LOD,theBond)); \
		coords1 = chemData->getAtomCoordinates(from).getValue(); \
		coords2 = chemData->getAtomCoordinates(to).getValue(); \
		glColor3fv(bondColors[chemData->getBondIndex(lodSelector->getBondIndex(LOD,theBond))].getValue()); \
		POST_ADJUST_CYLINDER \
		fromTo.setValue(from,to); \
		lodSelector->getBondCylinder(LOD)->render(coords1, coords2, cylRad, lastCylinderDoNormals, lastCylinderDoTextures,displayStyle,fromTo, this, action); \
	} \
	endCylinders(displayStyle); \
}

#define RENDER_LOD_BHAI(LOD) \
if (lodSelector->getBondIndexCnt(LOD) > 0) { \
	PRE_ADJUST_CYLINDER \
	SbColor colorFrom, colorTo; \
	float radFrom, radTo; \
	SbVec3f dir; \
	SbVec2i fromTo;\
	float t1,t2,dirlength; \
	beginCylinders(displayStyle,lodSelector->getBondCylinder(LOD)); \
	for (theBond=0; theBond < lodSelector->getBondIndexCnt(LOD); theBond ++) \
	{ \
		from = chemData->getBondFrom(lodSelector->getBondIndex(LOD,theBond)); \
		to = chemData->getBondTo(lodSelector->getBondIndex(LOD,theBond)); \
		colorFrom = chemColor->atomColor.getValues(0)[chemData->getAtomIndex(from)]; \
		colorTo = chemColor->atomColor.getValues(0)[chemData->getAtomIndex(to)]; \
		coords1 = chemData->getAtomCoordinates(from).getValue(); \
		coords2 = chemData->getAtomCoordinates(to).getValue(); \
		POST_ADJUST_CYLINDER \
		fromTo.setValue(from,to); \
		lodSelector->getBondCylinder(LOD)->renderHalfBonded(coords1, coords2, cylRad, colorFrom, colorTo,lastCylinderDoNormals, lastCylinderDoTextures,displayStyle,fromTo, this, action); \
	} \
	endCylinders(displayStyle); \
}

#define RENDER_LOD_BHAA(LOD) \
if (lodSelector->getBondIndexCnt(LOD) > 0) { \
	PRE_ADJUST_CYLINDER \
	SbColor colorFrom, colorTo; \
	float radFrom, radTo; \
	SbVec3f dir; \
	float t1,t2,dirlength; \
	SbVec2i fromTo;\
	beginCylinders(displayStyle,lodSelector->getBondCylinder(LOD)); \
	for (theBond=0; theBond < lodSelector->getBondIndexCnt(LOD); theBond ++) \
	{ \
		from = chemData->getBondFrom(lodSelector->getBondIndex(LOD,theBond)); \
		to = chemData->getBondTo(lodSelector->getBondIndex(LOD,theBond)); \
		colorFrom = chemColor->atomColor.getValues(0)[from]; \
		colorTo = chemColor->atomColor.getValues(0)[to]; \
		coords1 = chemData->getAtomCoordinates(from).getValue(); \
		coords2 = chemData->getAtomCoordinates(to).getValue(); \
		POST_ADJUST_CYLINDER \
		fromTo[0] = from; fromTo[1] = to; \
		lodSelector->getBondCylinder(LOD)->renderHalfBonded(coords1, coords2, cylRad, colorFrom, colorTo,lastCylinderDoNormals, lastCylinderDoTextures,displayStyle,fromTo,this, action); \
	} \
	endCylinders(displayStyle); \
}

#define RENDER_LOD_BAAI(LOD) \
if (lodSelector->getBondIndexCnt(LOD) > 0) { \
	PRE_ADJUST_CYLINDER \
	SbColor colorFrom, colorTo; \
	float radFrom, radTo; \
	SbVec3f dir; \
	SbVec2i fromTo; \
	float t1,t2,dirlength; \
	beginCylinders(displayStyle,lodSelector->getBondCylinder(LOD)); \
	for (theBond=0; theBond < lodSelector->getBondIndexCnt(LOD); theBond ++) \
	{ \
		from = chemData->getBondFrom(lodSelector->getBondIndex(LOD,theBond)); \
		to = chemData->getBondTo(lodSelector->getBondIndex(LOD,theBond)); \
		colorFrom = chemColor->atomColor.getValues(0)[chemData->getAtomIndex(from)]; \
		colorTo = chemColor->atomColor.getValues(0)[chemData->getAtomIndex(to)]; \
		coords1 = chemData->getAtomCoordinates(from).getValue(); \
		coords2 = chemData->getAtomCoordinates(to).getValue(); \
		POST_ADJUST_CYLINDER \
		fromTo.setValue(from,to); \
		lodSelector->getBondCylinder(LOD)->renderWithInterpolatedColor(coords1, coords2, cylRad, colorFrom, colorTo, lastCylinderDoNormals, lastCylinderDoTextures,displayStyle,fromTo, this, action); \
	} \
	endCylinders(displayStyle); \
}

#define RENDER_LOD_BAAA(LOD) \
if (lodSelector->getBondIndexCnt(LOD) > 0) { \
	PRE_ADJUST_CYLINDER \
	SbColor colorFrom, colorTo; \
	float radFrom, radTo; \
	SbVec3f dir; \
	float t1,t2,dirlength; \
	SbVec2i fromTo;\
	beginCylinders(displayStyle,lodSelector->getBondCylinder(LOD)); \
	for (theBond=0; theBond < lodSelector->getBondIndexCnt(LOD); theBond ++) \
	{ \
		from = chemData->getBondFrom(lodSelector->getBondIndex(LOD,theBond)); \
		to = chemData->getBondTo(lodSelector->getBondIndex(LOD,theBond)); \
		colorFrom = chemColor->atomColor.getValues(0)[from]; \
		colorTo = chemColor->atomColor.getValues(0)[to]; \
		coords1 = chemData->getAtomCoordinates(from).getValue(); \
		coords2 = chemData->getAtomCoordinates(to).getValue(); \
		POST_ADJUST_CYLINDER \
		fromTo.setValue(from,to); \
		lodSelector->getBondCylinder(LOD)->renderWithInterpolatedColor(coords1, coords2, cylRad, colorFrom, colorTo, lastCylinderDoNormals, lastCylinderDoTextures, displayStyle, fromTo, this, action); \
	} \
	endCylinders(displayStyle); \
}


#define COMPUTE_LOD \
int lod = computeCylinderLOD(coords1,coords2,rad); \
lodSelector->addBondIndex(lod,theBond);

#define CLIP_CYLINDER \
int lod = computeCylinderLOD(coords1,coords2,rad); \
if (lod > 0) lodSelector->addBondIndex(4,theBond); \
else lodSelector->addBondIndex(0,theBond);

#define LOD_NORMAL_CYLINDER \
int32_t		from, to; \
float rad = cdp->bondCylinderRadius.getValue();\
SbVec3f 	coords1, coords2; \
if (theBondIndex.getNum() == 0) return; \
SbBool doHydrogens = cdp->showHydrogens.getValue(); \
lodSelector->resetBonds(chemData->getNumberOfBonds()); \
BONDLOOP_START(theBondIndex) \
BNDDUP(theBond); \
from = chemData->getBondFrom(theBond); \
to = chemData->getBondTo(theBond); \
if (!doHydrogens) { \
if ((chemData->getAtomicNumber(from) == 1) || \
(chemData->getAtomicNumber(to) == 1)) continue; } \
const SbVec3f & coords1 = chemData->getAtomCoordinates(from).getValue(); \
const SbVec3f & coords2 = chemData->getAtomCoordinates(to).getValue(); \
COMPUTE_LOD \
BONDLOOP_END

#define CLIP_NORMAL_CYLINDER \
int32_t		from, to; \
float rad = cdp->bondCylinderRadius.getValue();\
SbVec3f 	coords1, coords2; \
if (theBondIndex.getNum() == 0) return; \
SbBool doHydrogens = cdp->showHydrogens.getValue(); \
lodSelector->resetBonds(chemData->getNumberOfBonds()); \
BONDLOOP_START(theBondIndex) \
BNDDUP(theBond); \
from = chemData->getBondFrom(theBond); \
to = chemData->getBondTo(theBond); \
if (!doHydrogens) { \
if ((chemData->getAtomicNumber(from) == 1) || \
(chemData->getAtomicNumber(to) == 1)) continue; } \
const SbVec3f & coords1 = chemData->getAtomCoordinates(from).getValue(); \
const SbVec3f & coords2 = chemData->getAtomCoordinates(to).getValue(); \
CLIP_CYLINDER \
BONDLOOP_END

/*
int ChemDisplay::computeCylinderLOD(const SbVec3f &coordFrom, const SbVec3f &coordTo, const float &rad,
									const SbMatrix &tempMatrix, const int *view) const
{
	// calculate projected area
	int lod;
	SbVec3f tempMin,tempMax;

	tempMatrix.multVecMatrix(coordFrom,tempMin);
	tempMatrix.multVecMatrix(coordTo,tempMax);
	
	SbVec3f scrMin, scrMax;
	scrMax[0]= view[0] + view[2] * (tempMax[0] + 1) * 0.5f;// / 2
	scrMax[1] = view[1] + view[3] * (tempMax[1] + 1) * 0.5f;
	scrMin[0]= view[0] + view[2] * (tempMin[0] + 1) * 0.5f;
	scrMin[1] = view[1] + view[3] * (tempMin[1] + 1) * 0.5f;
	
	// clamp values
	if (scrMin[0] < view[0]) scrMin[0] = (float)view[0];
	if (scrMax[0] < view[0]) scrMax[0] = (float)view[0];
	if (scrMin[1] < view[1]) scrMin[1] = (float)view[1];
	if (scrMax[1] < view[1]) scrMax[1] = (float)view[1];
	if (scrMin[0] > view[2]) scrMin[0] = (float)view[2];
	if (scrMax[0] > view[2]) scrMax[0] = (float)view[2];
	if (scrMin[1] > view[3]) scrMin[1] = (float)view[3];
	if (scrMax[1] > view[3]) scrMax[1] = (float)view[3];

	if ( (scrMin[0] + scrMax[0] < view[0] + 0.1f) || 
		(scrMin[1] + scrMax[1] < view[1] + 0.1f) ||
		(scrMin[0] == view[2] && scrMax[0] == view[2]) ||
		(scrMin[1] == view[3] && scrMax[1] == view[3]) )
	{
		return -1;
	}

	float len;
	
	if (tempMin[2]<tempMax[2])
	{
		len = (unitCylinder->camPos-coordFrom).sqrLength();
	}
	else
	{
		len = (unitCylinder->camPos-coordTo).sqrLength();
	}

	if (len < 25) lod = 4;
	else if (len < 100) lod = 3;
	else if (len < 300) lod = 2;
	else lod = 1;

	//printf("%f %f",(camPos-coordFrom).sqrLength(),(camPos-coordTo).sqrLength());
	//printf(" len %f lod %d\n",len,lod);

	return lod;
}
*/

int ChemDisplay::computeCylinderLOD(const SbVec3f &coordFrom,
									const SbVec3f &coordTo,
									const float &rad) const
{
	// SGB 
	// Fast cylinder culling test
	// Prerequisites: billboard vertexs a,b,c,d must be precomputed
	// Algorithm steps
	// 1. Take the original atom coordinates from the bond and transform them to
	// device coordinates
	// 2. Compute the min,max coordinates in world coordinates 
	// that encloses the cylinder in a AABB
	// using a,b,c,d and the radius of the cylinder.
	// Choose a,b,c,d in function of the device coordinates
	// 3. Transform the min,max coordinates to device coordinates and
	// later to window coordinates. Clamp them to maximum viexport extents.
	// 4. If manhattan distance between coordinates is less than EPSILON, 
	// Cull the cylinder
	// Note: Test is useful for rounded cap cylinders too

	// 
	// calculate projected area
	int lod;

	//  device coordinates
	SbVec3f tempFrom,tempTo;
	currentMVP.multVecMatrix(coordFrom,tempFrom);
	currentMVP.multVecMatrix(coordTo,tempTo);

	// window coordinates
	SbVec3f scrFrom, scrTo;
	SbVec3f coordFrom0,coordTo0;

	if (tempFrom[0]>tempTo[0] ) //coordfromx > coordtox
	{
		if (tempFrom[1]>tempTo[1] ) //coordfromx > coordtox
		{
			coordFrom0 = coordFrom + currentABCD[2]*rad;
			coordTo0 = coordTo + currentABCD[0]*rad;
		}
		else
		{
			coordFrom0 = coordFrom + currentABCD[1]*rad;
			coordTo0 = coordTo + currentABCD[3]*rad;
		}
	}
	else
	{
		if (tempTo[1]>tempFrom[1] ) //coordtox > coordfromx
		{
			coordTo0 = coordTo + currentABCD[2]*rad;
			coordFrom0 = coordFrom + currentABCD[0]*rad;
		}
		else
		{
			coordTo0 = coordTo + currentABCD[1]*rad;
			coordFrom0 = coordFrom + currentABCD[3]*rad;
		}
	}

	currentMVP.multVecMatrix(coordFrom0,tempFrom);
	currentMVP.multVecMatrix(coordTo0,tempTo);
	
	scrTo[0]= currentView[0] + currentView[2] * (tempTo[0] + 1) * 0.5f;// / 2
	scrTo[1] = currentView[1] + currentView[3] * (tempTo[1] + 1) * 0.5f;
	scrFrom[0]= currentView[0] + currentView[2] * (tempFrom[0] + 1) * 0.5f;
	scrFrom[1] = currentView[1] + currentView[3] * (tempFrom[1] + 1) * 0.5f;
	
	// clamp values
	if (scrFrom[0] < currentView[0]) scrFrom[0] = (float)currentView[0];
	if (scrTo[0] < currentView[0]) scrTo[0] = (float)currentView[0];
	if (scrFrom[1] < currentView[1]) scrFrom[1] = (float)currentView[1];
	if (scrTo[1] < currentView[1]) scrTo[1] = (float)currentView[1];
	if (scrFrom[0] > currentView[2]) scrFrom[0] = (float)currentView[2];
	if (scrTo[0] > currentView[2]) scrTo[0] = (float)currentView[2];
	if (scrFrom[1] > currentView[3]) scrFrom[1] = (float)currentView[3];
	if (scrTo[1] > currentView[3]) scrTo[1] = (float)currentView[3];

	if ( (scrFrom[0] + scrTo[0] < currentView[0] + 0.1f) || 
		(scrFrom[1] + scrTo[1] < currentView[1] + 0.1f) ||
		(scrFrom[0] == currentView[2] && scrTo[0] == currentView[2]) ||
		(scrFrom[1] == currentView[3] && scrTo[1] == currentView[3]) )
	{
		return 0;
	}

	// The cylinder is visible, compute level of detail 

	float len;
	
	// Simple aproximation based on nearest atom coordinate

	
	if (tempFrom[2]<tempTo[2])
	{
		const SbVec3f& d = currentCameraPosition-coordFrom;
		len = d.dot(d);
	}
	else
	{
		const SbVec3f& d = currentCameraPosition-coordTo;
		len = d.dot(d);
	}

	if (len < 25) lod = 4;
	else if (len < 100) lod = 3;
	else if (len < 300) lod = 2;
	else lod = 1;

	//printf(" len %f lod %d\n",len,lod);

	return lod;
}

/************************** OV ************************************/

void ChemDisplay::LODnormalCylinderOV(const MFVec2i &theBondIndex,
                               SoState *state,
                               const SbColor &chemColor,
                               const ChemDisplayParam *cdp,
                               const ChemBaseData *chemData)
{
	LOD_NORMAL_CYLINDER
	int displayStyle = ChemDisplayParam::BONDCYLINDER_SEMI_NOCAP + cdp->bondCylinderDisplayStyle.getValue() % 3;
	SoGLRenderAction *action = (SoGLRenderAction *)state->getAction();

	glColor3fv(chemColor.getValue());
	RENDER_LOD_OV(1)
	RENDER_LOD_OV(2)
	RENDER_LOD_OV(3)
	RENDER_LOD_OV(4)
}

void ChemDisplay::LODhighlightCylinderOV(const MFVec2i &theBondIndex, 
																					SoState *state,
																					const ChemColor *chemColor,
																					const ChemDisplayParam *cdp,
																					const ChemBaseData *chemData)
{
	LOD_NORMAL_CYLINDER
	int displayStyle = ChemDisplayParam::BONDCYLINDER_SEMI_NOCAP + cdp->bondCylinderDisplayStyle.getValue() % 3;
	SoGLRenderAction *action = (SoGLRenderAction*)state->getAction();	

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

	RENDER_LOD_OV(1)
	RENDER_LOD_OV(2)
	RENDER_LOD_OV(3)
	RENDER_LOD_OV(4)

	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
		glMaterialfv(GL_FRONT, GL_EMISSION,
			SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

}


void ChemDisplay::clipNormalCylinderOV(const MFVec2i &theBondIndex,
											 SoState *state,
											 const SbColor &chemColor,
											 const ChemDisplayParam *cdp,
											 const ChemBaseData *chemData)
{
	CLIP_NORMAL_CYLINDER
	int displayStyle = cdp->bondCylinderDisplayStyle.getValue();
	SoGLRenderAction *action = (SoGLRenderAction *)state->getAction();
	
	RENDER_LOD_OV(4)
}

/************************** BB ************************************/

void ChemDisplay::LODnormalCylinderBB(const MFVec2i &theBondIndex,
                               SoState *state,
                               const ChemColor *chemColor,
                               const ChemDisplayParam *cdp,
                               const ChemBaseData *chemData)
{
	LOD_NORMAL_CYLINDER
	int displayStyle = ChemDisplayParam::BONDCYLINDER_SEMI_NOCAP + cdp->bondCylinderDisplayStyle.getValue() % 3;
	SoGLRenderAction *action = (SoGLRenderAction *)state->getAction();
	
	RENDER_LOD_BB(1)
	RENDER_LOD_BB(2)
	RENDER_LOD_BB(3)
	RENDER_LOD_BB(4)
}

void ChemDisplay::LODhighlightCylinderBB(const MFVec2i &theBondIndex, 
																					SoState *state,
																					const ChemColor *chemColor,
																					const ChemDisplayParam *cdp,
																					const ChemBaseData *chemData)
{
	LOD_NORMAL_CYLINDER
	SbVec3f		theCapScale;
	theCapScale.setValue(rad, rad, rad);

	int displayStyle = ChemDisplayParam::BONDCYLINDER_SEMI_NOCAP + cdp->bondCylinderDisplayStyle.getValue() % 3;
	
	SbBool doCaps = (displayStyle % 3 > 0);
	
	SoGLRenderAction *action = (SoGLRenderAction*)state->getAction();	
	//const SbColor *atomColors = chemColor->atomColor.getValues(0);
	//SbColor colorFrom, colorTo;
	int32_t highlightStyle = cdp->highlightStyle.getValue();
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
		LODhighlightCylindersOverall(chemData, cdp, theBondIndex,
			displayStyle, doHydrogens, doCaps, rad, theCapScale, state);
		return;
	}
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
		const SbColor &theColor = cdp->highlightColor.getValue();
		glMaterialfv(GL_FRONT, GL_EMISSION,
			SbVec4f(theColor[0], theColor[1], theColor[2], 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

	RENDER_LOD_BB(1)
	RENDER_LOD_BB(2)
	RENDER_LOD_BB(3)
	RENDER_LOD_BB(4)

	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
		glMaterialfv(GL_FRONT, GL_EMISSION,
			SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

}


void ChemDisplay::clipNormalCylinderBB(const MFVec2i &theBondIndex,
											 SoState *state,
											 const ChemColor *chemColor,
											 const ChemDisplayParam *cdp,
											 const ChemBaseData *chemData)
{
	CLIP_NORMAL_CYLINDER
	int displayStyle = cdp->bondCylinderDisplayStyle.getValue();
	SoGLRenderAction *action = (SoGLRenderAction *)state->getAction();

	RENDER_LOD_BB(4)
}

/************************** BI ************************************/

void ChemDisplay::LODnormalCylinderBI(const MFVec2i &theBondIndex,
                               SoState *state,
                               const ChemColor *chemColor,
                               const ChemDisplayParam *cdp,
                               const ChemBaseData *chemData)
{
	LOD_NORMAL_CYLINDER
	int displayStyle = ChemDisplayParam::BONDCYLINDER_SEMI_NOCAP + cdp->bondCylinderDisplayStyle.getValue() % 3;
	SoGLRenderAction *action = (SoGLRenderAction *)state->getAction();
	
	RENDER_LOD_BI(1)
	RENDER_LOD_BI(2)
	RENDER_LOD_BI(3)
	RENDER_LOD_BI(4)
}

void ChemDisplay::LODhighlightCylinderBI(const MFVec2i &theBondIndex, 
																					SoState *state,
																					const ChemColor *chemColor,
																					const ChemDisplayParam *cdp,
																					const ChemBaseData *chemData)
{
	LOD_NORMAL_CYLINDER
	SbVec3f		theCapScale;
	theCapScale.setValue(rad, rad, rad);

	int displayStyle = ChemDisplayParam::BONDCYLINDER_SEMI_NOCAP + cdp->bondCylinderDisplayStyle.getValue() % 3;
	
	SbBool doCaps = (displayStyle % 3 > 0);
	
	SoGLRenderAction *action = (SoGLRenderAction*)state->getAction();	
	//const SbColor *atomColors = chemColor->atomColor.getValues(0);
	//SbColor colorFrom, colorTo;
	int32_t highlightStyle = cdp->highlightStyle.getValue();
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
		LODhighlightCylindersOverall(chemData, cdp, theBondIndex,
			displayStyle, doHydrogens, doCaps, rad, theCapScale, state);
		return;
	}
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
		const SbColor &theColor = cdp->highlightColor.getValue();
		glMaterialfv(GL_FRONT, GL_EMISSION,
			SbVec4f(theColor[0], theColor[1], theColor[2], 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

	RENDER_LOD_BI(1)
	RENDER_LOD_BI(2)
	RENDER_LOD_BI(3)
	RENDER_LOD_BI(4)

	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
		glMaterialfv(GL_FRONT, GL_EMISSION,
			SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

}

void ChemDisplay::clipNormalCylinderBI(const MFVec2i &theBondIndex,
											 SoState *state,
											 const ChemColor *chemColor,
											 const ChemDisplayParam *cdp,
											 const ChemBaseData *chemData)
{
	CLIP_NORMAL_CYLINDER
	int displayStyle = cdp->bondCylinderDisplayStyle.getValue();
	SoGLRenderAction *action = (SoGLRenderAction *)state->getAction();

	RENDER_LOD_BI(4)
}

/************************** BHAI ************************************/

void ChemDisplay::LODnormalCylinderBHAI(const MFVec2i &theBondIndex,
                               SoState *state,
                               const ChemColor *chemColor,
                               const ChemDisplayParam *cdp,
                               const ChemBaseData *chemData)
{

	LOD_NORMAL_CYLINDER
	int displayStyle = ChemDisplayParam::BONDCYLINDER_SEMI_NOCAP + cdp->bondCylinderDisplayStyle.getValue() % 3;
	SoGLRenderAction *action = (SoGLRenderAction *)state->getAction();
	
	RENDER_LOD_BHAI(1)
	RENDER_LOD_BHAI(2)
	RENDER_LOD_BHAI(3)
	RENDER_LOD_BHAI(4)
}

void ChemDisplay::LODhighlightCylinderBHAI(const MFVec2i &theBondIndex, 
																					SoState *state,
																					const ChemColor *chemColor,
																					const ChemDisplayParam *cdp,
																					const ChemBaseData *chemData)
{
	LOD_NORMAL_CYLINDER
	SbVec3f		theCapScale;
	theCapScale.setValue(rad, rad, rad);

	int displayStyle = ChemDisplayParam::BONDCYLINDER_SEMI_NOCAP + cdp->bondCylinderDisplayStyle.getValue() % 3;
	
	SbBool doCaps = (displayStyle % 3 > 0);
	
	SoGLRenderAction *action = (SoGLRenderAction*)state->getAction();	
	//const SbColor *atomColors = chemColor->atomColor.getValues(0);
	//SbColor colorFrom, colorTo;
	int32_t highlightStyle = cdp->highlightStyle.getValue();
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
		LODhighlightCylindersOverall(chemData, cdp, theBondIndex,
			displayStyle, doHydrogens, doCaps, rad, theCapScale, state);
		return;
	}
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
		const SbColor &theColor = cdp->highlightColor.getValue();
		glMaterialfv(GL_FRONT, GL_EMISSION,
			SbVec4f(theColor[0], theColor[1], theColor[2], 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

	RENDER_LOD_BHAI(1)
	RENDER_LOD_BHAI(2)
	RENDER_LOD_BHAI(3)
	RENDER_LOD_BHAI(4)


	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
		glMaterialfv(GL_FRONT, GL_EMISSION,
			SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

}

void ChemDisplay::clipNormalCylinderBHAI(const MFVec2i &theBondIndex,
											 SoState *state,
											 const ChemColor *chemColor,
											 const ChemDisplayParam *cdp,
											 const ChemBaseData *chemData)
{
	CLIP_NORMAL_CYLINDER
	int displayStyle = cdp->bondCylinderDisplayStyle.getValue();	
	SoGLRenderAction *action = (SoGLRenderAction *)state->getAction();
	
	RENDER_LOD_BHAI(4)

//	printf("cylinder cull %d/%d\n", lodSelector->cylinderClipCnt, vnormalBondIndex.count(globalNumberOfBonds));
}

/************************** BHAA ************************************/

void ChemDisplay::LODnormalCylinderBHAA(const MFVec2i &theBondIndex,
                               SoState *state,
                               const ChemColor *chemColor,
                               const ChemDisplayParam *cdp,
                               const ChemBaseData *chemData)
{
	LOD_NORMAL_CYLINDER
	int displayStyle = ChemDisplayParam::BONDCYLINDER_SEMI_NOCAP + cdp->bondCylinderDisplayStyle.getValue() % 3;
	SoGLRenderAction *action = (SoGLRenderAction *)state->getAction();
	
	RENDER_LOD_BHAA(1)
	RENDER_LOD_BHAA(2)
	RENDER_LOD_BHAA(3)
	RENDER_LOD_BHAA(4)
}

void ChemDisplay::LODhighlightCylinderBHAA(const MFVec2i &theBondIndex, 
																					SoState *state,
																					const ChemColor *chemColor,
																					const ChemDisplayParam *cdp,
																					const ChemBaseData *chemData)
{
	LOD_NORMAL_CYLINDER
	SbVec3f		theCapScale;
	theCapScale.setValue(rad, rad, rad);

	int displayStyle = ChemDisplayParam::BONDCYLINDER_SEMI_NOCAP + cdp->bondCylinderDisplayStyle.getValue() % 3;
	
	SbBool doCaps = (displayStyle % 3 > 0);
	
	SoGLRenderAction *action = (SoGLRenderAction*)state->getAction();	
	//const SbColor *atomColors = chemColor->atomColor.getValues(0);
	//SbColor colorFrom, colorTo;
	int32_t highlightStyle = cdp->highlightStyle.getValue();
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
		LODhighlightCylindersOverall(chemData, cdp, theBondIndex,
			displayStyle, doHydrogens, doCaps, rad, theCapScale, state);
		return;
	}
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
		const SbColor &theColor = cdp->highlightColor.getValue();
		glMaterialfv(GL_FRONT, GL_EMISSION,
			SbVec4f(theColor[0], theColor[1], theColor[2], 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

	RENDER_LOD_BHAA(1)
	RENDER_LOD_BHAA(2)
	RENDER_LOD_BHAA(3)
	RENDER_LOD_BHAA(4)

	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
		glMaterialfv(GL_FRONT, GL_EMISSION,
			SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

}

void ChemDisplay::clipNormalCylinderBHAA(const MFVec2i &theBondIndex,
											 SoState *state,
											 const ChemColor *chemColor,
											 const ChemDisplayParam *cdp,
											 const ChemBaseData *chemData)
{
	CLIP_NORMAL_CYLINDER
	int displayStyle = cdp->bondCylinderDisplayStyle.getValue();	
	SoGLRenderAction *action = (SoGLRenderAction *)state->getAction();
	
	RENDER_LOD_BHAA(4)
}

/************************** BAAI ************************************/

void ChemDisplay::LODnormalCylinderBAAI(const MFVec2i &theBondIndex,
                               SoState *state,
                               const ChemColor *chemColor,
                               const ChemDisplayParam *cdp,
                               const ChemBaseData *chemData)
{

	LOD_NORMAL_CYLINDER
	int displayStyle = ChemDisplayParam::BONDCYLINDER_SEMI_NOCAP + cdp->bondCylinderDisplayStyle.getValue() % 3;
	SoGLRenderAction *action = (SoGLRenderAction *)state->getAction();
	
	RENDER_LOD_BAAI(1)
	RENDER_LOD_BAAI(2)
	RENDER_LOD_BAAI(3)
	RENDER_LOD_BAAI(4)
}

void ChemDisplay::LODhighlightCylinderBAAI(const MFVec2i &theBondIndex, 
																					SoState *state,
																					const ChemColor *chemColor,
																					const ChemDisplayParam *cdp,
																					const ChemBaseData *chemData)
{
	LOD_NORMAL_CYLINDER
	SbVec3f		theCapScale;
	theCapScale.setValue(rad, rad, rad);

	int displayStyle = ChemDisplayParam::BONDCYLINDER_SEMI_NOCAP + cdp->bondCylinderDisplayStyle.getValue() % 3;
	
	SbBool doCaps = (displayStyle % 3 > 0);
	
	SoGLRenderAction *action = (SoGLRenderAction*)state->getAction();	
	//const SbColor *atomColors = chemColor->atomColor.getValues(0);
	//SbColor colorFrom, colorTo;
	int32_t highlightStyle = cdp->highlightStyle.getValue();
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
		LODhighlightCylindersOverall(chemData, cdp, theBondIndex,
			displayStyle, doHydrogens, doCaps, rad, theCapScale, state);
		return;
	}
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
		const SbColor &theColor = cdp->highlightColor.getValue();
		glMaterialfv(GL_FRONT, GL_EMISSION,
			SbVec4f(theColor[0], theColor[1], theColor[2], 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

	RENDER_LOD_BAAI(1)
	RENDER_LOD_BAAI(2)
	RENDER_LOD_BAAI(3)
	RENDER_LOD_BAAI(4)

	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
		glMaterialfv(GL_FRONT, GL_EMISSION,
			SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

}

void ChemDisplay::clipNormalCylinderBAAI(const MFVec2i &theBondIndex,
											 SoState *state,
											 const ChemColor *chemColor,
											 const ChemDisplayParam *cdp,
											 const ChemBaseData *chemData)
{
	CLIP_NORMAL_CYLINDER
	int displayStyle = cdp->bondCylinderDisplayStyle.getValue();	
	SoGLRenderAction *action = (SoGLRenderAction *)state->getAction();

	RENDER_LOD_BAAI(4)
}

/************************** BAAA ************************************/

void ChemDisplay::LODnormalCylinderBAAA(const MFVec2i &theBondIndex,
                               SoState *state,
                               const ChemColor *chemColor,
                               const ChemDisplayParam *cdp,
                               const ChemBaseData *chemData)
{
	LOD_NORMAL_CYLINDER
	int displayStyle = ChemDisplayParam::BONDCYLINDER_SEMI_NOCAP + cdp->bondCylinderDisplayStyle.getValue() % 3;
	SoGLRenderAction *action = (SoGLRenderAction *)state->getAction();
	
	RENDER_LOD_BAAA(1)
	RENDER_LOD_BAAA(2)
	RENDER_LOD_BAAA(3)
	RENDER_LOD_BAAA(4)
}

void ChemDisplay::LODhighlightCylinderBAAA(const MFVec2i &theBondIndex, 
																					SoState *state,
																					const ChemColor *chemColor,
																					const ChemDisplayParam *cdp,
																					const ChemBaseData *chemData)
{
	LOD_NORMAL_CYLINDER
	SbVec3f		theCapScale;
	theCapScale.setValue(rad, rad, rad);

	int displayStyle = ChemDisplayParam::BONDCYLINDER_SEMI_NOCAP + cdp->bondCylinderDisplayStyle.getValue() % 3;
	
	SbBool doCaps = (displayStyle % 3 > 0);
	
	SoGLRenderAction *action = (SoGLRenderAction*)state->getAction();	
	//const SbColor *atomColors = chemColor->atomColor.getValues(0);
	//SbColor colorFrom, colorTo;
	int32_t highlightStyle = cdp->highlightStyle.getValue();
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE) {
		LODhighlightCylindersOverall(chemData, cdp, theBondIndex,
			displayStyle, doHydrogens, doCaps, rad, theCapScale, state);
		return;
	}
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
		const SbColor &theColor = cdp->highlightColor.getValue();
		glMaterialfv(GL_FRONT, GL_EMISSION,
			SbVec4f(theColor[0], theColor[1], theColor[2], 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

	RENDER_LOD_BAAA(1)
	RENDER_LOD_BAAA(2)
	RENDER_LOD_BAAA(3)
	RENDER_LOD_BAAA(4)

	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE) {
		glMaterialfv(GL_FRONT, GL_EMISSION,
			SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

}

void ChemDisplay::clipNormalCylinderBAAA(const MFVec2i &theBondIndex,
											 SoState *state,
											 const ChemColor *chemColor,
											 const ChemDisplayParam *cdp,
											 const ChemBaseData *chemData)
{
	CLIP_NORMAL_CYLINDER
	int displayStyle = cdp->bondCylinderDisplayStyle.getValue();	
	SoGLRenderAction *action = (SoGLRenderAction *)state->getAction();

	RENDER_LOD_BAAA(4)
}

void ChemDisplay::LODhighlightCylindersOverall(const ChemBaseData *chemData,const ChemDisplayParam *cdp, 
																	 const MFVec2i &theBondIndex,int displayStyle, 
																	 SbBool doHydrogens, SbBool doCaps,float rad,
																	 SbVec3f theCapScale, SoState* state)
{
	int32_t		from, to;
	SbVec3f 	coords1, coords2;

	SoGLRenderAction *action = (SoGLRenderAction *)state->getAction();
	//const SbColor &chemColor = cdp->highlightColor.getValue();

	RENDER_LOD_OV(1)
	RENDER_LOD_OV(2)
	RENDER_LOD_OV(3)
	RENDER_LOD_OV(4)

}

#undef BONDLOOP_START
#undef BONDLOOP_END
#undef ADJUST_CYLINDER
#undef PRE_ADJUST_CYLINDER
#undef POST_ADJUST_CYLINDER
#undef RENDER_LOD_OV
#undef RENDER_LOD_BB
#undef RENDER_LOD_BI
#undef RENDER_LOD_BHAI
#undef RENDER_LOD_BHAA
#undef RENDER_LOD_BAAI
#undef RENDER_LOD_BAAA
#undef COMPUTE_LOD
#undef CLIP_CYLINDER
#undef LOD_NORMAL_CYLINDER
#undef CLIP_NORMAL_CYLINDER
#undef BNDDUP
