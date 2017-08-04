#include <iostream>
using namespace std;
// ChemDisplaySpheresLOD.cpp
// Implementation of the ChemDisplay LOD...sphereXXX() methods
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

#include "inv/flclient.h"

#include "inv/ChemKit/SparseBits.H"

#include "inv/ChemKit/ChemUnitSphere.H"
#include "inv/ChemKit/ChemLOD.H"

#include "inv/ChemKit/ChemAtomBBox.H"
#include "inv/ChemKit/ChemLabelBBox.H"
#include "inv/ChemKit/ChemStickBondBBox.H"
#include "inv/ChemKit/ChemWireframeBondBBox.H"

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

#ifdef __coin
// --> coin 1,2 compatibility
//#include "inv/elements/SoGLLightModelElement.H"
// <-- coin 1,2 compatibility
#endif

// --> EGB && SGB
#ifdef _DEBUG   
#include "inv/errors/SoDebugError.H"
#endif
// <-- EGB && SGB

#ifdef IV2_0
#include "inv/bundles/SoMaterialBundle.H"
#else
#include "inv/elements/SoGLLazyElement.H"
#include "inv/elements/SoLazyElement.H"
#endif

#include "inv/ChemKit/ChemData.H"
#include "inv/nodes/SoSeparator.H"

static int numAtomLoops;
static int atomLoop;
static int32_t atomStart;
static int32_t atomEnd;
static int32_t theAtom;

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

static int lodAtomIndex;

#define RENDER_LOD_CI(ARRAY_NAME, DISPLAY_STYLE) \
if (lodSelector->ARRAY_NAME##Cnt > 0) { \
		unsigned int localAtomIndex = 0; \
		beginSpheres(DISPLAY_STYLE,lodSelector->ARRAY_NAME); \
		for (lodAtomIndex=0;lodAtomIndex<lodSelector->ARRAY_NAME##Cnt;lodAtomIndex++) { \
			localAtomIndex = chemData->getAtomIndex(lodSelector->ARRAY_NAME##Indices[lodAtomIndex]); \
			glColor3fv(atomColors[localAtomIndex].getValue()); \
			glPushMatrix(); \
			glMultMatrixf((float *)lodSelector->atomMatrices[lodSelector->ARRAY_NAME##Indices[lodAtomIndex]].getValue()); \
			lodSelector->ARRAY_NAME->generateDisplay(true,lastSphereDoNormals,lastSphereDoTextures,DISPLAY_STYLE); \
			glPopMatrix(); \
} \
		endSpheres(DISPLAY_STYLE); \
	}

#define RENDER_POINT_CI \
if (lodSelector->pointCnt > 0) { \
	glDisable(GL_TEXTURE_2D); \
	glDisable(GL_LIGHTING); \
	glPointSize(2); \
	for (unsigned int i=0;i<lodSelector->pointCnt;i++) { \
		unsigned int localAtomIndex = chemData->getAtomIndex(lodSelector->pointIndices[i]); \
		glColor3fv(atomColors[localAtomIndex].getValue()); \
		glPushMatrix(); \
		glMultMatrixf((float *)lodSelector->atomMatrices[lodSelector->pointIndices[i]].getValue()); \
		glBegin(GL_POINTS); \
			glVertex3f(0,0,0); \
		glEnd(); \
		glPopMatrix(); \
	} \
	glEnable(GL_LIGHTING); \
}

#define	RENDER_BILLBOARD_CI \
if (lodSelector->billboardCnt > 0) { \
	displayStyle = ChemDisplayParam::ATOMSPHERE_BILLBOARD; \
	beginSpheres(displayStyle); \
	for (unsigned int i=0;i<lodSelector->billboardCnt;i++) { \
		int32_t localAtomIndex = chemData->getAtomIndex(lodSelector->billboardIndices[i]); \
		glColor3fv(atomColors[localAtomIndex].getValue()); \
		glPushMatrix(); \
		glMultMatrixf((float *)lodSelector->atomMatrices[lodSelector->billboardIndices[i]].getValue()); \
		unitSphere->generateDisplay(false,lastSphereDoNormals,lastSphereDoTextures,displayStyle); \
		glPopMatrix(); \
	} \
	endSpheres(displayStyle); \
}

#define RENDER_LOD_CA(ARRAY_NAME, DISPLAY_STYLE) \
if (lodSelector->ARRAY_NAME##Cnt > 0) { \
		beginSpheres(DISPLAY_STYLE,lodSelector->ARRAY_NAME); \
		for (lodAtomIndex=0;lodAtomIndex<lodSelector->ARRAY_NAME##Cnt;lodAtomIndex++) { \
			glColor3fv(atomColors[lodSelector->ARRAY_NAME##Indices[lodAtomIndex]].getValue()); \
			glPushMatrix(); \
			glMultMatrixf((float *)lodSelector->atomMatrices[lodSelector->ARRAY_NAME##Indices[lodAtomIndex]].getValue()); \
			lodSelector->ARRAY_NAME->generateDisplay(true,lastSphereDoNormals,lastSphereDoTextures,DISPLAY_STYLE); \
			glPopMatrix(); \
} \
		endSpheres(DISPLAY_STYLE); \
	}

#define RENDER_POINT_CA \
if (lodSelector->pointCnt > 0) { \
	glDisable(GL_TEXTURE_2D); \
	glDisable(GL_LIGHTING); \
	glPointSize(2); \
	for (unsigned int i=0;i<lodSelector->pointCnt;i++) { \
		glColor3fv(atomColors[lodSelector->pointIndices[i]].getValue()); \
		glPushMatrix(); \
		glMultMatrixf((float *)lodSelector->atomMatrices[lodSelector->pointIndices[i]].getValue()); \
		glBegin(GL_POINTS); \
			glVertex3f(0,0,0); \
		glEnd(); \
		glPopMatrix(); \
	} \
	glEnable(GL_LIGHTING); \
}

#define	RENDER_BILLBOARD_CA \
if (lodSelector->billboardCnt > 0) { \
	displayStyle = ChemDisplayParam::ATOMSPHERE_BILLBOARD; \
	beginSpheres(displayStyle); \
	for (unsigned int i=0;i<lodSelector->billboardCnt;i++) { \
		glColor3fv(atomColors[lodSelector->billboardIndices[i]].getValue()); \
		glPushMatrix(); \
		glMultMatrixf((float *)lodSelector->atomMatrices[lodSelector->billboardIndices[i]].getValue()); \
		unitSphere->generateDisplay(false,lastSphereDoNormals,lastSphereDoTextures,displayStyle); \
		glPopMatrix(); \
	} \
	endSpheres(displayStyle); \
}

#define RENDER_LOD_CO(ARRAY_NAME, DISPLAY_STYLE) \
if (lodSelector->ARRAY_NAME##Cnt > 0) { \
		glColor3fv(atomColors[0].getValue()); \
		beginSpheres(DISPLAY_STYLE,lodSelector->ARRAY_NAME); \
		for (lodAtomIndex=0;lodAtomIndex<lodSelector->ARRAY_NAME##Cnt;lodAtomIndex++) { \
			glPushMatrix(); \
			glMultMatrixf((float *)lodSelector->atomMatrices[lodSelector->ARRAY_NAME##Indices[lodAtomIndex]].getValue()); \
			lodSelector->ARRAY_NAME->generateDisplay(true,lastSphereDoNormals,lastSphereDoTextures,DISPLAY_STYLE); \
			glPopMatrix(); \
} \
		endSpheres(DISPLAY_STYLE); \
	}

#define RENDER_POINT_CO \
if (lodSelector->pointCnt > 0) { \
	glDisable(GL_TEXTURE_2D); \
		glDisable(GL_LIGHTING); \
		glPointSize(2); \
		glColor3fv(atomColors[0].getValue()); \
		for (unsigned int i=0;i<lodSelector->pointCnt;i++) { \
			glPushMatrix(); \
			glMultMatrixf((float *)lodSelector->atomMatrices[lodSelector->pointIndices[i]].getValue()); \
			glBegin(GL_POINTS); \
				glVertex3f(0,0,0); \
			glEnd(); \
			glPopMatrix(); \
		} \
		glEnable(GL_LIGHTING); \
	}

#define	RENDER_BILLBOARD_CO \
	if (lodSelector->billboardCnt > 0) { \
		displayStyle = ChemDisplayParam::ATOMSPHERE_BILLBOARD; \
		beginSpheres(displayStyle); \
		glColor3fv(atomColors[0].getValue()); \
		for (unsigned int i=0;i<lodSelector->billboardCnt;i++) { \
			glPushMatrix(); \
			glMultMatrixf((float *)lodSelector->atomMatrices[lodSelector->billboardIndices[i]].getValue()); \
			unitSphere->generateDisplay(false,lastSphereDoNormals,lastSphereDoTextures,displayStyle); \
			glPopMatrix(); \
		} \
		endSpheres(displayStyle); \
	}

#define RENDER_LOD_HIGHLIGHT(ARRAY_NAME, DISPLAY_STYLE) \
	if (lodSelector->ARRAY_NAME##Cnt > 0) { \
			glColor3fv(highlightColor.getValue()); \
			beginSpheres(DISPLAY_STYLE,lodSelector->ARRAY_NAME); \
			for (lodAtomIndex=0;lodAtomIndex<lodSelector->ARRAY_NAME##Cnt;lodAtomIndex++) { \
				glPushMatrix(); \
				glMultMatrixf((float *)lodSelector->atomMatrices[lodSelector->ARRAY_NAME##Indices[lodAtomIndex]].getValue()); \
				lodSelector->ARRAY_NAME->generateDisplay(true,lastSphereDoNormals,lastSphereDoTextures,DISPLAY_STYLE); \
				glPopMatrix(); \
	} \
			endSpheres(DISPLAY_STYLE); \
		}

#define RENDER_POINT_HIGHLIGHT \
	if (lodSelector->pointCnt > 0) { \
		glDisable(GL_TEXTURE_2D); \
	glDisable(GL_LIGHTING); \
	glPointSize(2); \
	glColor3fv(highlightColor.getValue()); \
	for (unsigned int i=0;i<lodSelector->pointCnt;i++) { \
		glPushMatrix(); \
		glMultMatrixf((float *)lodSelector->atomMatrices[lodSelector->pointIndices[i]].getValue()); \
		glBegin(GL_POINTS); \
			glVertex3f(0,0,0); \
		glEnd(); \
		glPopMatrix(); \
	} \
	glEnable(GL_LIGHTING); \
}

#define	RENDER_BILLBOARD_HIGHLIGHT \
if (lodSelector->billboardCnt > 0) { \
	displayStyle = ChemDisplayParam::ATOMSPHERE_BILLBOARD; \
	beginSpheres(displayStyle); \
	glColor3fv(highlightColor.getValue()); \
	for (unsigned int i=0;i<lodSelector->billboardCnt;i++) { \
		glPushMatrix(); \
		glMultMatrixf((float *)lodSelector->atomMatrices[lodSelector->billboardIndices[i]].getValue()); \
		unitSphere->generateDisplay(false,lastSphereDoNormals,lastSphereDoTextures,displayStyle); \
		glPopMatrix(); \
	} \
	endSpheres(displayStyle); \
}
// TODO: use static member variables in ChemLOD instead constants

#define COMPUTE_LODg \
float n = computeSphereProjectedArea(sphereCoords, rad); \
if (n < 1.0f || n == viewsize ) { \
	lodSelector->sphereClipCnt++;	\
}	else if (n < 500.0f) { \
	lodSelector->sphere1Indices[lodSelector->sphere1Cnt++] = theAtom; \
	lodSelector->atomMatrices[theAtom].setTransform(sphereCoords, currentRotation, theScale); \
} else if (n < 10000.0f) { \
	lodSelector->sphere2Indices[lodSelector->sphere2Cnt++] = theAtom; \
	lodSelector->atomMatrices[theAtom].setTransform(sphereCoords, currentRotation, theScale); \
} else if (n < 25000.0f) { \
	lodSelector->sphere3Indices[lodSelector->sphere3Cnt++] = theAtom; \
	lodSelector->atomMatrices[theAtom].setTransform(sphereCoords, currentRotation, theScale); \
} else { \
	lodSelector->sphere4Indices[lodSelector->sphere4Cnt++] = theAtom; \
	lodSelector->atomMatrices[theAtom].setTransform(sphereCoords, currentRotation, theScale); \
}

#define COMPUTE_LODf \
float n = computeSphereProjectedArea(sphereCoords, rad); \
if (n < 1.0f || n == viewsize ) { \
	lodSelector->sphereClipCnt++; \
}else if (n < 10.0f) {  \
	lodSelector->pointIndices[lodSelector->pointCnt] = theAtom; \
	lodSelector->pointCnt++; \
	lodSelector->atomMatrices[theAtom].setTransform(sphereCoords, currentRotation, theScale); \
} else if (n < 1000.0f) { \
	lodSelector->billboardIndices[lodSelector->billboardCnt] = theAtom; \
	lodSelector->billboardCnt++; \
	lodSelector->atomMatrices[theAtom].setTransform(sphereCoords, zeroRotation, theScale); \
} else if (n < 5000.0f) { \
	lodSelector->sphere2Indices[lodSelector->sphere2Cnt++] = theAtom; \
	lodSelector->atomMatrices[theAtom].setTransform(sphereCoords, currentRotation, theScale); \
} else if (n < 50000.0f) { \
	lodSelector->sphere3Indices[lodSelector->sphere3Cnt++] = theAtom; \
	lodSelector->atomMatrices[theAtom].setTransform(sphereCoords, currentRotation, theScale); \
}else{ \
	lodSelector->sphere4Indices[lodSelector->sphere4Cnt++] = theAtom; \
	lodSelector->atomMatrices[theAtom].setTransform(sphereCoords, currentRotation, theScale); \
}

#define CLIP_RI \
	float		rad; \
	SbVec3f 	theScale; \
	SbRotation	theRotation; \
	SbVec3f		sphereCoords; \
	int32_t		localAtomIndex; \
	theRotation = currentRotation; \
	float viewsize = currentVPR.getWindowSize()[0]*currentVPR.getWindowSize()[1];\
	SbRotation zeroRotation; \
	zeroRotation.setValue(0.0,0.0,0.0,1.0); \
	lodSelector->resetAtoms(chemData->getNumberOfAtoms()); \
	SbRotation *whichRotation = &theRotation; \
	if (displayStyle == ChemDisplayParam::ATOMSPHERE_BILLBOARD) whichRotation = &zeroRotation; \
	ATOMLOOP_START(theAtomIndex) \
		if (!doHydrogens && chemData->getAtomicNumber(theAtom) == 1) continue; \
		sphereCoords = chemData->getAtomCoordinates(theAtom); \
		localAtomIndex = chemData->getAtomIndex(theAtom); \
		rad = atomRadii[localAtomIndex]; \
		rad *= radScale; \
		theScale.setValue(rad, rad, rad); \
		float n = computeSphereProjectedArea(sphereCoords, rad); \
		if (n < 1.0f || n == viewsize ) { \
			lodSelector->sphereClipCnt++; \
		} else {  \
			lodSelector->sphere4Indices[lodSelector->sphere4Cnt++] = theAtom; \
			lodSelector->atomMatrices[theAtom].setTransform(sphereCoords, *whichRotation, theScale); \
		} \
	ATOMLOOP_END

#define CLIP_RA \
	float		rad; \
	SbVec3f 	theScale; \
	SbRotation	theRotation; \
	SbVec3f		sphereCoords; \
	theRotation = currentRotation; \
	SbRotation zeroRotation; \
	float viewsize = currentVPR.getWindowSize()[0]*currentVPR.getWindowSize()[1];\
	zeroRotation.setValue(0.0,0.0,0.0,1.0); \
	lodSelector->resetAtoms(chemData->getNumberOfAtoms()); \
	SbRotation *whichRotation = &theRotation; \
	if (displayStyle == ChemDisplayParam::ATOMSPHERE_BILLBOARD) whichRotation = &zeroRotation; \
	ATOMLOOP_START(theAtomIndex) \
		if (!doHydrogens && chemData->getAtomicNumber(theAtom) == 1) continue; \
		sphereCoords = chemData->getAtomCoordinates(theAtom); \
		rad = atomRadii[theAtom]; \
		rad *= radScale; \
		theScale.setValue(rad, rad, rad); \
		float n = computeSphereProjectedArea(sphereCoords, rad); \
		if (n < 1.0f || n == viewsize ) { \
			lodSelector->sphereClipCnt++;\
		} else {  \
			lodSelector->sphere4Indices[lodSelector->sphere4Cnt++] = theAtom; \
			lodSelector->atomMatrices[theAtom].setTransform(sphereCoords, *whichRotation, theScale); \
		} \
	ATOMLOOP_END

#define CLIP_RO \
	float		rad; \
	SbVec3f 	theScale; \
	SbRotation	theRotation; \
	SbVec3f		sphereCoords; \
	theRotation = currentRotation; \
	SbRotation zeroRotation; \
	float viewsize = currentVPR.getWindowSize()[0]*currentVPR.getWindowSize()[1];\
	zeroRotation.setValue(0.0,0.0,0.0,1.0); \
	lodSelector->resetAtoms(chemData->getNumberOfAtoms()); \
	SbRotation *whichRotation = &theRotation; \
	if (displayStyle == ChemDisplayParam::ATOMSPHERE_BILLBOARD) whichRotation = &zeroRotation; \
	rad = atomRadii[0]; \
	rad *= radScale; \
	ATOMLOOP_START(theAtomIndex) \
		if (!doHydrogens && chemData->getAtomicNumber(theAtom) == 1) continue; \
		sphereCoords = chemData->getAtomCoordinates(theAtom); \
		theScale.setValue(rad, rad, rad); \
		float n = computeSphereProjectedArea(sphereCoords, rad); \
		if (n < 1.0f || n == viewsize ) { \
			lodSelector->sphereClipCnt++;\
		} else {  \
			lodSelector->sphere4Indices[lodSelector->sphere4Cnt++] = theAtom; \
			lodSelector->atomMatrices[theAtom].setTransform(sphereCoords, *whichRotation, theScale); \
		} \
	ATOMLOOP_END

float ChemDisplay::computeSphereProjectedArea(const SbVec3f &sphereCoords,
											  const float &rad) const
{
		SbVec3f objMax, objMin;
		SbVec3f tempMax, tempMin;

		objMin = sphereCoords + (unitSphere->a * rad);
		objMax = sphereCoords + (unitSphere->c * rad);

		currentMVP.multVecMatrix(objMin,tempMin);
		currentMVP.multVecMatrix(objMax,tempMax);
		
		SbVec3f scrMin, scrMax;

		scrMax[0]= currentView[0] + currentView[2] * (tempMax[0] + 1) / 2;
		scrMax[1] = currentView[1] + currentView[3] * (tempMax[1] + 1) / 2;
//		scrMax[2] = (tempMax[2] + 1) / 2;

		scrMin[0]= currentView[0] + currentView[2] * (tempMin[0] + 1) / 2;
		scrMin[1] = currentView[1] + currentView[3] * (tempMin[1] + 1) / 2;
//		scrMin[2] = (tempMin[2] + 1) / 2;
		
		//printf("\n%d min %0.2f max %0.2f",theAtom,scrMin[2],scrMax[2]);
		
		// clamp values
		if (scrMin[0] < (float)currentView[0]) scrMin[0] = (float)currentView[0];
		if (scrMax[0] < (float)currentView[0]) scrMax[0] = (float)currentView[0];
		if (scrMin[1] < (float)currentView[1]) scrMin[1] = (float)currentView[1];
		if (scrMax[1] < (float)currentView[1]) scrMax[1] = (float)currentView[1];
		
		if (scrMin[0] > (float)currentView[2]) scrMin[0] = (float)currentView[2];
		if (scrMax[0] > (float)currentView[2]) scrMax[0] = (float)currentView[2];
		if (scrMin[1] > (float)currentView[3]) scrMin[1] = (float)currentView[3];
		if (scrMax[1] > (float)currentView[3]) scrMax[1] = (float)currentView[3];

		return (scrMax[0] - scrMin[0])  * (scrMax[1] - scrMin[1]);
}

//****************************** RI *****************************************//

void ChemDisplay::computeLODgSphereRI(const SbBool doHydrogens,
					const float radScale, const float *atomRadii,
					const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
{
	float		rad;
	SbVec3f 	theScale;
	SbRotation	theRotation;
	SbVec3f		sphereCoords;
	int32_t		localAtomIndex;
	
	theRotation = currentRotation;
	SbRotation zeroRotation;
	zeroRotation.setValue(0.0,0.0,0.0,1.0);

	// prepare lod indices
	lodSelector->resetAtoms(chemData->getNumberOfAtoms());

	float viewsize = currentVPR.getWindowSize()[0]*currentVPR.getWindowSize()[1];
	
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

		COMPUTE_LODg

	ATOMLOOP_END
}

void ChemDisplay::computeLODfSphereRI(const SbBool doHydrogens,
					const float radScale, const float *atomRadii,
					const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
{
	float		rad;
	SbVec3f 	theScale;
	SbRotation	theRotation;
	SbVec3f		sphereCoords;
	int32_t		localAtomIndex;
	
	theRotation = currentRotation;
	SbRotation zeroRotation;
	zeroRotation.setValue(0.0,0.0,0.0,1.0);

	// prepare lod indices
	lodSelector->resetAtoms(chemData->getNumberOfAtoms());

	float viewsize = currentVPR.getWindowSize()[0]*currentVPR.getWindowSize()[1];
	
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

		COMPUTE_LODf
		
	ATOMLOOP_END
}


//****************************** RO *****************************************//
void ChemDisplay::computeLODgSphereRO(const SbBool doHydrogens,
			const float radScale, const float *atomRadii,
			const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
{
	float		rad;
	SbVec3f 	theScale;
	SbRotation	theRotation;
	SbVec3f		sphereCoords;
	
	theRotation = currentRotation;
	SbRotation zeroRotation;
	zeroRotation.setValue(0.0,0.0,0.0,1.0);

	// prepare lod indices
	lodSelector->resetAtoms(chemData->getNumberOfAtoms());
	
	float viewsize = currentVPR.getWindowSize()[0]*currentVPR.getWindowSize()[1];\

	rad = atomRadii[0];
	rad *= radScale;

	ATOMLOOP_START(theAtomIndex)
	
		if (!doHydrogens &&
			chemData->getAtomicNumber(theAtom) == 1) {
			continue;
		}
		sphereCoords = chemData->getAtomCoordinates(theAtom);

		theScale.setValue(rad, rad, rad);

		COMPUTE_LODg

	ATOMLOOP_END

}

void ChemDisplay::computeLODfSphereRO(const SbBool doHydrogens,
			const float radScale, const float *atomRadii,
			const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
{
	float		rad;
	SbVec3f 	theScale;
	SbRotation	theRotation;
	SbVec3f		sphereCoords;
	
	theRotation = currentRotation;
	SbRotation zeroRotation;
	zeroRotation.setValue(0.0,0.0,0.0,1.0);

	// prepare lod indices
	lodSelector->resetAtoms(chemData->getNumberOfAtoms());

	float viewsize = currentVPR.getWindowSize()[0]*currentVPR.getWindowSize()[1];

	rad = atomRadii[0];
	rad *= radScale;
	
	ATOMLOOP_START(theAtomIndex)
	
		if (!doHydrogens &&
			chemData->getAtomicNumber(theAtom) == 1) {
			continue;
		}
		sphereCoords = chemData->getAtomCoordinates(theAtom);
		theScale.setValue(rad, rad, rad);

		COMPUTE_LODf

	ATOMLOOP_END
}

//****************************** RA *****************************************//
void ChemDisplay::computeLODgSphereRA(const SbBool doHydrogens,
			const float radScale, const float *atomRadii,
			const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
{
	float		rad;
	SbVec3f 	theScale;
	SbRotation	theRotation;
	SbVec3f		sphereCoords;
	
	theRotation = currentRotation;
	SbRotation zeroRotation;
	zeroRotation.setValue(0.0,0.0,0.0,1.0);

	// prepare lod indices
	lodSelector->resetAtoms(chemData->getNumberOfAtoms());

	float viewsize = currentVPR.getWindowSize()[0]*currentVPR.getWindowSize()[1];
	
	ATOMLOOP_START(theAtomIndex)
	
		if (!doHydrogens &&
			chemData->getAtomicNumber(theAtom) == 1) {
			continue;
		}
		sphereCoords = chemData->getAtomCoordinates(theAtom);
		rad = atomRadii[theAtom];
		rad *= radScale;
		theScale.setValue(rad, rad, rad);

		COMPUTE_LODg

	ATOMLOOP_END
}

void ChemDisplay::computeLODfSphereRA(const SbBool doHydrogens,
			const float radScale, const float *atomRadii,
			const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
{
		float		rad;
	SbVec3f 	theScale;
	SbRotation	theRotation;
	SbVec3f		sphereCoords;
	
	theRotation = currentRotation;
	SbRotation zeroRotation;
	zeroRotation.setValue(0.0,0.0,0.0,1.0);

	// prepare lod indices
	lodSelector->resetAtoms(chemData->getNumberOfAtoms());

	float viewsize = currentVPR.getWindowSize()[0]*currentVPR.getWindowSize()[1];
	
	ATOMLOOP_START(theAtomIndex)
	
		if (!doHydrogens &&
			chemData->getAtomicNumber(theAtom) == 1) {
			continue;
		}
		sphereCoords = chemData->getAtomCoordinates(theAtom);
		//rad = atomRadii[localAtomIndex];
		//rad = atomRadii[0];
		rad = atomRadii[theAtom];
		rad *= radScale;
		theScale.setValue(rad, rad, rad);

		COMPUTE_LODf

	ATOMLOOP_END
}

//****************************** CI *****************************************//

void ChemDisplay::renderLODgNormalSphereCI(const SbColor *atomColors,
			const ChemBaseData *chemData)
{
	RENDER_LOD_CI(sphere1,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
	RENDER_LOD_CI(sphere2,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
	RENDER_LOD_CI(sphere3,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
	RENDER_LOD_CI(sphere4,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
}

void ChemDisplay::renderLODgHighlightSphereCI(const int highlightStyle,
																							const SbColor &highlightColor,
																							const SbColor *atomColors,
																							const ChemBaseData *chemData)
{
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
	{
			glMaterialfv(GL_FRONT, GL_EMISSION,
				SbVec4f(highlightColor[0], highlightColor[1],
						highlightColor[2], 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
	{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE)
	{
		RENDER_LOD_HIGHLIGHT(sphere1,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
		RENDER_LOD_HIGHLIGHT(sphere2,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
		RENDER_LOD_HIGHLIGHT(sphere3,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
		RENDER_LOD_HIGHLIGHT(sphere4,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
	}
	else // if (highlightStyle != ChemDisplayParam::HIGHLIGHT_DIFFUSE)
	{
		RENDER_LOD_CI(sphere1,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
		RENDER_LOD_CI(sphere2,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
		RENDER_LOD_CI(sphere3,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
		RENDER_LOD_CI(sphere4,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)

	} // if (highlightStyle != ChemDisplayParam::HIGHLIGHT_DIFFUSE)

	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
	{
		glMaterialfv(GL_FRONT, GL_EMISSION,
			SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
}

void ChemDisplay::renderLODfNormalSphereCI(const SbColor *atomColors,
			const ChemBaseData *chemData)
{
	int displayStyle = 0;
	RENDER_POINT_CI
	RENDER_BILLBOARD_CI
	RENDER_LOD_CI(sphere2,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
	RENDER_LOD_CI(sphere3,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
	RENDER_LOD_CI(sphere4,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
}

void ChemDisplay::renderLODfHighlightSphereCI(const int highlightStyle,
																							const SbColor &highlightColor,
																							const SbColor *atomColors,
																							const ChemBaseData *chemData)
{
	int displayStyle = ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES;

	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
	{
			glMaterialfv(GL_FRONT, GL_EMISSION,
				SbVec4f(highlightColor[0], highlightColor[1],
						highlightColor[2], 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
	{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE)
	{
		RENDER_POINT_HIGHLIGHT
		RENDER_BILLBOARD_HIGHLIGHT
		RENDER_LOD_HIGHLIGHT(sphere2,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
		RENDER_LOD_HIGHLIGHT(sphere3,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
		RENDER_LOD_HIGHLIGHT(sphere4,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
	}
	else // if (highlightStyle != ChemDisplayParam::HIGHLIGHT_DIFFUSE)
	{
		RENDER_POINT_CI
		RENDER_BILLBOARD_CI
		RENDER_LOD_CI(sphere2,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
		RENDER_LOD_CI(sphere3,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
		RENDER_LOD_CI(sphere4,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)

	} // if (highlightStyle != ChemDisplayParam::HIGHLIGHT_DIFFUSE)

	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
	{
		glMaterialfv(GL_FRONT, GL_EMISSION,
			SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
}

//****************************** CA *****************************************//

void ChemDisplay::renderLODgNormalSphereCA(const SbColor *atomColors,
			const ChemBaseData *chemData)
{
	RENDER_LOD_CA(sphere1,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
	RENDER_LOD_CA(sphere2,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
	RENDER_LOD_CA(sphere3,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
	RENDER_LOD_CA(sphere4,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
}

void ChemDisplay::renderLODgHighlightSphereCA(const int highlightStyle,
																							const SbColor &highlightColor,
																							const SbColor *atomColors,
																							const ChemBaseData *chemData)
{
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
	{
			glMaterialfv(GL_FRONT, GL_EMISSION,
				SbVec4f(highlightColor[0], highlightColor[1],
						highlightColor[2], 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
	{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE)
	{
		RENDER_LOD_HIGHLIGHT(sphere1,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
		RENDER_LOD_HIGHLIGHT(sphere2,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
		RENDER_LOD_HIGHLIGHT(sphere3,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
		RENDER_LOD_HIGHLIGHT(sphere4,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
	}
	else // if (highlightStyle != ChemDisplayParam::HIGHLIGHT_DIFFUSE)
	{
		RENDER_LOD_CA(sphere1,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
		RENDER_LOD_CA(sphere2,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
		RENDER_LOD_CA(sphere3,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
		RENDER_LOD_CA(sphere4,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
		
	} // if (highlightStyle != ChemDisplayParam::HIGHLIGHT_DIFFUSE)

	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
	{
		glMaterialfv(GL_FRONT, GL_EMISSION,
			SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
}

void ChemDisplay::renderLODfNormalSphereCA(const SbColor *atomColors,
			const ChemBaseData *chemData)
{
	int displayStyle = 0;
	RENDER_POINT_CA
	RENDER_BILLBOARD_CA
	RENDER_LOD_CA(sphere2,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
	RENDER_LOD_CA(sphere3,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
	RENDER_LOD_CA(sphere4,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
}

void ChemDisplay::renderLODfHighlightSphereCA(const int highlightStyle,
																							const SbColor &highlightColor,
																							const SbColor *atomColors,
																							const ChemBaseData *chemData)
{
	int displayStyle = ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES;

	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
	{
			glMaterialfv(GL_FRONT, GL_EMISSION,
				SbVec4f(highlightColor[0], highlightColor[1],
						highlightColor[2], 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
	{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE)
	{
		RENDER_POINT_HIGHLIGHT
		RENDER_BILLBOARD_HIGHLIGHT
		RENDER_LOD_HIGHLIGHT(sphere2,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
		RENDER_LOD_HIGHLIGHT(sphere3,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
		RENDER_LOD_HIGHLIGHT(sphere4,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
	}
	else // if (highlightStyle != ChemDisplayParam::HIGHLIGHT_DIFFUSE)
	{
		RENDER_POINT_CA
		RENDER_BILLBOARD_CA
		RENDER_LOD_CA(sphere2,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
		RENDER_LOD_CA(sphere3,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
		RENDER_LOD_CA(sphere4,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
		
	} // if (highlightStyle != ChemDisplayParam::HIGHLIGHT_DIFFUSE)

	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
	{
		glMaterialfv(GL_FRONT, GL_EMISSION,
			SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
}

//****************************** CO *****************************************//

void ChemDisplay::renderLODgNormalSphereCO(const SbColor *atomColors,
			const ChemBaseData *chemData)
{
	RENDER_LOD_CO(sphere1,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
	RENDER_LOD_CO(sphere2,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
	RENDER_LOD_CO(sphere3,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
	RENDER_LOD_CO(sphere4,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
}

void ChemDisplay::renderLODgHighlightSphereCO(const int highlightStyle,
																							const SbColor &highlightColor,
																							const SbColor *atomColors,
																							const ChemBaseData *chemData)
{
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
	{
			glMaterialfv(GL_FRONT, GL_EMISSION,
				SbVec4f(highlightColor[0], highlightColor[1],
						highlightColor[2], 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
	{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE)
	{
		RENDER_LOD_HIGHLIGHT(sphere1,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
		RENDER_LOD_HIGHLIGHT(sphere2,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
		RENDER_LOD_HIGHLIGHT(sphere3,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
		RENDER_LOD_HIGHLIGHT(sphere4,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
	}
	else // if (highlightStyle != ChemDisplayParam::HIGHLIGHT_DIFFUSE)
	{
		RENDER_LOD_CO(sphere1,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
		RENDER_LOD_CO(sphere2,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
		RENDER_LOD_CO(sphere3,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
		RENDER_LOD_CO(sphere4,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
		
	} // if (highlightStyle != ChemDisplayParam::HIGHLIGHT_DIFFUSE)

	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
	{
		glMaterialfv(GL_FRONT, GL_EMISSION,
			SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
}

void ChemDisplay::renderLODfNormalSphereCO(const SbColor *atomColors,
			const ChemBaseData *chemData)
{
	int displayStyle = 0;
	RENDER_POINT_CO
	RENDER_BILLBOARD_CO
	RENDER_LOD_CO(sphere2,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
	RENDER_LOD_CO(sphere3,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
	RENDER_LOD_CO(sphere4,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
}

void ChemDisplay::renderLODfHighlightSphereCO(const int highlightStyle,
																							const SbColor &highlightColor,
																							const SbColor *atomColors,
																							const ChemBaseData *chemData)
{
	int displayStyle = ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES;

	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
	{
			glMaterialfv(GL_FRONT, GL_EMISSION,
				SbVec4f(highlightColor[0], highlightColor[1],
						highlightColor[2], 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
	{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE)
	{
		RENDER_POINT_HIGHLIGHT
		RENDER_BILLBOARD_HIGHLIGHT
		RENDER_LOD_HIGHLIGHT(sphere2,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
		RENDER_LOD_HIGHLIGHT(sphere3,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
		RENDER_LOD_HIGHLIGHT(sphere4,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
	}
	else // if (highlightStyle != ChemDisplayParam::HIGHLIGHT_DIFFUSE)
	{
		RENDER_POINT_CO
		RENDER_BILLBOARD_CO
		RENDER_LOD_CO(sphere2,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
		RENDER_LOD_CO(sphere3,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
		RENDER_LOD_CO(sphere4,ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES)
		
	} // if (highlightStyle != ChemDisplayParam::HIGHLIGHT_DIFFUSE)

	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
	{
		glMaterialfv(GL_FRONT, GL_EMISSION,
			SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
}


//****************************** RICI *****************************************//

void ChemDisplay::LODgNormalSphereRICI(const SbBool doHydrogens,
	const float radScale, const float *atomRadii,
	const SbColor *atomColors,
	const ChemBaseData *chemData, const MFVec2i &theAtomIndex)

{
	computeLODgSphereRI(doHydrogens,radScale,atomRadii,chemData,theAtomIndex);
	renderLODgNormalSphereCI(atomColors,chemData);

//	printf("sphere cull %d/%d\n", lodSelector->sphereClipCnt, vnormalAtomIndex.count(globalNumberOfAtoms));
}

void ChemDisplay::LODgHighlightSphereRICI(const int highlightStyle,
																					const SbColor &highlightColor,
																					const SbBool doHydrogens,
																					const float radScale,
																					const float *atomRadii,
																					const SbColor *atomColors,
																					const ChemBaseData *chemData,
																					const MFVec2i &theAtomIndex)
{
	computeLODgSphereRI(doHydrogens,radScale,atomRadii,chemData,theAtomIndex);
	renderLODgHighlightSphereCI(highlightStyle,highlightColor,atomColors,chemData);
}


void ChemDisplay:: LODfNormalSphereRICI(const SbBool doHydrogens,
	const float radScale, const float *atomRadii,
	const SbColor *atomColors,
	const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
{
	computeLODfSphereRI(doHydrogens,radScale,atomRadii,chemData,theAtomIndex);
	renderLODfNormalSphereCI(atomColors,chemData);
}

void ChemDisplay::LODfHighlightSphereRICI(const int highlightStyle,
			const SbColor &highlightColor,const SbBool doHydrogens,
			const float radScale, const float *atomRadii,
			const SbColor *atomColors,
			const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
{
	computeLODfSphereRI(doHydrogens,radScale,atomRadii,chemData,theAtomIndex);
	renderLODfHighlightSphereCI(highlightStyle,highlightColor,atomColors,chemData);
}


void ChemDisplay::clipNormalSphereRICI(const int displayStyle, const SbBool doHydrogens,
											const SbBool doHemi, const float radScale, 
											const float *atomRadii, const SbColor *atomColors,
											const ChemBaseData *chemData, const MFVec2i &theAtomIndex, SoGLRenderAction *action)
{
	CLIP_RI

	if (lodSelector->sphere4Cnt> 0)
	{
		//beginSpheres(displayStyle); // dont reexecute 
		for (unsigned int i=0;i<lodSelector->sphere4Cnt;i++)
		{
			int32_t localAtomIndex = chemData->getAtomIndex(lodSelector->sphere4Indices[i]);
			glColor3fv(atomColors[localAtomIndex].getValue());
			
			glPushMatrix();
			glMultMatrixf((float *)lodSelector->atomMatrices[lodSelector->sphere4Indices[i]].getValue());
			unitSphere->generateDisplay(doHemi,lastSphereDoNormals,lastSphereDoTextures,displayStyle,action);
			glPopMatrix();
		}
		//endSpheres(displayStyle);
	}
}

//****************************** RICA *****************************************//

void ChemDisplay::LODgNormalSphereRICA(const SbBool doHydrogens,
	const float radScale, const float *atomRadii,
	const SbColor *atomColors,
	const ChemBaseData *chemData, const MFVec2i &theAtomIndex)

{
	computeLODgSphereRI(doHydrogens,radScale,atomRadii,chemData,theAtomIndex);
	renderLODgNormalSphereCA(atomColors,chemData);
}

void ChemDisplay::LODgHighlightSphereRICA(const int highlightStyle,
																					const SbColor &highlightColor,
																					const SbBool doHydrogens,
																					const float radScale,
																					const float *atomRadii,
																					const SbColor *atomColors,
																					const ChemBaseData *chemData,
																					const MFVec2i &theAtomIndex)
{
	computeLODgSphereRI(doHydrogens,radScale,atomRadii,chemData,theAtomIndex);
	renderLODgHighlightSphereCA(highlightStyle,highlightColor,atomColors,chemData);
}

void ChemDisplay:: LODfNormalSphereRICA(const SbBool doHydrogens,
	const float radScale, const float *atomRadii,
	const SbColor *atomColors,
	const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
{
	computeLODfSphereRI(doHydrogens,radScale,atomRadii,chemData,theAtomIndex);
	renderLODfNormalSphereCA(atomColors,chemData);
}

void ChemDisplay::LODfHighlightSphereRICA(const int highlightStyle,
			const SbColor &highlightColor,const SbBool doHydrogens,
			const float radScale, const float *atomRadii,
			const SbColor *atomColors,
			const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
{
	computeLODfSphereRI(doHydrogens,radScale,atomRadii,chemData,theAtomIndex);
	renderLODfHighlightSphereCA(highlightStyle,highlightColor,atomColors,chemData);
}

void ChemDisplay::clipNormalSphereRICA(const int displayStyle, const SbBool doHydrogens,
											const SbBool doHemi, const float radScale, 
											const float *atomRadii, const SbColor *atomColors,
											const ChemBaseData *chemData, const MFVec2i &theAtomIndex, SoGLRenderAction *action)
{
	CLIP_RI

	if (lodSelector->sphere4Cnt> 0)
	{
		//beginSpheres(displayStyle); // dont reexecute 
		for (unsigned int i=0;i<lodSelector->sphere4Cnt;i++)
		{
			//int32_t localAtomIndex = chemData->getAtomIndex(lodSelector->sphere4Indices[i]);
			//glColor3fv(atomColors[localAtomIndex].getValue());
			glColor3fv(atomColors[lodSelector->sphere4Indices[i]].getValue());
			
			glPushMatrix();
			glMultMatrixf((float *)lodSelector->atomMatrices[lodSelector->sphere4Indices[i]].getValue());
			unitSphere->generateDisplay(doHemi,lastSphereDoNormals,lastSphereDoTextures,displayStyle,action);
			glPopMatrix();
		}
		//endSpheres(displayStyle);
	}
}

//****************************** RICO *****************************************//

void ChemDisplay::LODgNormalSphereRICO(const SbBool doHydrogens,
	const float radScale, const float *atomRadii,
	const SbColor *atomColors,
	const ChemBaseData *chemData, const MFVec2i &theAtomIndex)

{
	computeLODgSphereRI(doHydrogens,radScale,atomRadii,chemData,theAtomIndex);
	renderLODgNormalSphereCO(atomColors,chemData);
}

void ChemDisplay::LODgHighlightSphereRICO(const int highlightStyle,
																					const SbColor &highlightColor,
																					const SbBool doHydrogens,
																					const float radScale,
																					const float *atomRadii,
																					const SbColor *atomColors,
																					const ChemBaseData *chemData,
																					const MFVec2i &theAtomIndex)
{
	computeLODgSphereRI(doHydrogens,radScale,atomRadii,chemData,theAtomIndex);
	renderLODgHighlightSphereCO(highlightStyle,highlightColor,atomColors,chemData);
}

void ChemDisplay:: LODfNormalSphereRICO(const SbBool doHydrogens,
	const float radScale, const float *atomRadii,
	const SbColor *atomColors,
	const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
{
	computeLODfSphereRI(doHydrogens,radScale,atomRadii,chemData,theAtomIndex);
	renderLODfNormalSphereCO(atomColors,chemData);
}

void ChemDisplay::LODfHighlightSphereRICO(const int highlightStyle,
			const SbColor &highlightColor,const SbBool doHydrogens,
			const float radScale, const float *atomRadii,
			const SbColor *atomColors,
			const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
{
	computeLODfSphereRI(doHydrogens,radScale,atomRadii,chemData,theAtomIndex);
	renderLODfHighlightSphereCO(highlightStyle,highlightColor,atomColors,chemData);
}

void ChemDisplay::clipNormalSphereRICO(const int displayStyle, const SbBool doHydrogens,
											const SbBool doHemi, const float radScale, 
											const float *atomRadii, const SbColor *atomColors,
											const ChemBaseData *chemData, const MFVec2i &theAtomIndex, SoGLRenderAction *action)
{
	CLIP_RI

	if (lodSelector->sphere4Cnt> 0)
	{
		//beginSpheres(displayStyle); // dont reexecute 
		glColor3fv(atomColors[0].getValue());
		for (unsigned int i=0;i<lodSelector->sphere4Cnt;i++)
		{
			glPushMatrix();
			glMultMatrixf((float *)lodSelector->atomMatrices[lodSelector->sphere4Indices[i]].getValue());
			unitSphere->generateDisplay(doHemi,lastSphereDoNormals,lastSphereDoTextures,displayStyle,action);
			glPopMatrix();
		}
		//endSpheres(displayStyle); //dont reexecute
	}
}

//****************************** RACI *****************************************//

void ChemDisplay::LODgNormalSphereRACI(const SbBool doHydrogens,
	const float radScale, const float *atomRadii,
	const SbColor *atomColors,
	const ChemBaseData *chemData, const MFVec2i &theAtomIndex)

{
	computeLODgSphereRA(doHydrogens,radScale,atomRadii,chemData,theAtomIndex);
	renderLODgNormalSphereCI(atomColors,chemData);
}

void ChemDisplay::LODgHighlightSphereRACI(const int highlightStyle,
																					const SbColor &highlightColor,
																					const SbBool doHydrogens,
																					const float radScale,
																					const float *atomRadii,
																					const SbColor *atomColors,
																					const ChemBaseData *chemData,
																					const MFVec2i &theAtomIndex)
{
	computeLODgSphereRA(doHydrogens,radScale,atomRadii,chemData,theAtomIndex);
	renderLODgHighlightSphereCI(highlightStyle,highlightColor,atomColors,chemData);
}

void ChemDisplay:: LODfNormalSphereRACI(const SbBool doHydrogens,
	const float radScale, const float *atomRadii,
	const SbColor *atomColors,
	const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
{
	computeLODfSphereRA(doHydrogens,radScale,atomRadii,chemData,theAtomIndex);
	renderLODfNormalSphereCI(atomColors,chemData);
}

void ChemDisplay::LODfHighlightSphereRACI(const int highlightStyle,
			const SbColor &highlightColor,const SbBool doHydrogens,
			const float radScale, const float *atomRadii,
			const SbColor *atomColors,
			const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
{
	computeLODfSphereRA(doHydrogens,radScale,atomRadii,chemData,theAtomIndex);
	renderLODfHighlightSphereCI(highlightStyle,highlightColor,atomColors,chemData);
}

void ChemDisplay::clipNormalSphereRACI(const int displayStyle, const SbBool doHydrogens,
											const SbBool doHemi, const float radScale, 
											const float *atomRadii, const SbColor *atomColors,
											const ChemBaseData *chemData, const MFVec2i &theAtomIndex, SoGLRenderAction *action)
{

	CLIP_RA

	if (lodSelector->sphere4Cnt> 0)
	{
		//beginSpheres(displayStyle); // dont reexecute 

		for (unsigned int i=0;i<lodSelector->sphere4Cnt;i++)
		{
			int32_t localAtomIndex = chemData->getAtomIndex(lodSelector->sphere4Indices[i]);
			glColor3fv(atomColors[localAtomIndex].getValue());
			
			glPushMatrix();
			glMultMatrixf((float *)lodSelector->atomMatrices[lodSelector->sphere4Indices[i]].getValue());
			unitSphere->generateDisplay(doHemi,lastSphereDoNormals,lastSphereDoTextures,displayStyle,action);
			glPopMatrix();
		}
		//endSpheres(displayStyle); //dont reexecute
	}
}

//****************************** RACA *****************************************//

void ChemDisplay::LODgNormalSphereRACA(const SbBool doHydrogens,
	const float radScale, const float *atomRadii,
	const SbColor *atomColors,
	const ChemBaseData *chemData, const MFVec2i &theAtomIndex)

{
	computeLODgSphereRA(doHydrogens,radScale,atomRadii,chemData,theAtomIndex);
	renderLODgNormalSphereCA(atomColors,chemData);
}

void ChemDisplay::LODgHighlightSphereRACA(const int highlightStyle,
																					const SbColor &highlightColor,
																					const SbBool doHydrogens,
																					const float radScale,
																					const float *atomRadii,
																					const SbColor *atomColors,
																					const ChemBaseData *chemData,
																					const MFVec2i &theAtomIndex)
{
	computeLODgSphereRA(doHydrogens,radScale,atomRadii,chemData,theAtomIndex);
	renderLODgHighlightSphereCA(highlightStyle,highlightColor,atomColors,chemData);
}

void ChemDisplay:: LODfNormalSphereRACA(const SbBool doHydrogens,
	const float radScale, const float *atomRadii,
	const SbColor *atomColors,
	const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
{
	computeLODfSphereRA(doHydrogens,radScale,atomRadii,chemData,theAtomIndex);
	renderLODfNormalSphereCA(atomColors,chemData);
}

void ChemDisplay::LODfHighlightSphereRACA(const int highlightStyle,
			const SbColor &highlightColor,const SbBool doHydrogens,
			const float radScale, const float *atomRadii,
			const SbColor *atomColors,
			const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
{
	computeLODfSphereRA(doHydrogens,radScale,atomRadii,chemData,theAtomIndex);
	renderLODfHighlightSphereCA(highlightStyle,highlightColor,atomColors,chemData);
}

void ChemDisplay::clipNormalSphereRACA(const int displayStyle, const SbBool doHydrogens,
											const SbBool doHemi, const float radScale, 
											const float *atomRadii, const SbColor *atomColors,
											const ChemBaseData *chemData, const MFVec2i &theAtomIndex, SoGLRenderAction *action)
{
	CLIP_RA

	if (lodSelector->sphere4Cnt> 0)
	{
		//beginSpheres(displayStyle); // dont reexecute 
		for (unsigned int i=0;i<lodSelector->sphere4Cnt;i++)
		{
			//int32_t localAtomIndex = chemData->getAtomIndex(lodSelector->sphere4Indices[i]);
			//glColor3fv(atomColors[localAtomIndex].getValue());
			glColor3fv(atomColors[lodSelector->sphere4Indices[i]].getValue());
			
			glPushMatrix();
			glMultMatrixf((float *)lodSelector->atomMatrices[lodSelector->sphere4Indices[i]].getValue());
			unitSphere->generateDisplay(doHemi,lastSphereDoNormals,lastSphereDoTextures,displayStyle,action);
			glPopMatrix();
		}
		//endSpheres(displayStyle);
	}
}

//****************************** RACO *****************************************//

void ChemDisplay::LODgNormalSphereRACO(const SbBool doHydrogens,
	const float radScale, const float *atomRadii,
	const SbColor *atomColors,
	const ChemBaseData *chemData, const MFVec2i &theAtomIndex)

{
	computeLODgSphereRA(doHydrogens,radScale,atomRadii,chemData,theAtomIndex);
	renderLODgNormalSphereCO(atomColors,chemData);
}

void ChemDisplay::LODgHighlightSphereRACO(const int highlightStyle,
																					const SbColor &highlightColor,
																					const SbBool doHydrogens,
																					const float radScale,
																					const float *atomRadii,
																					const SbColor *atomColors,
																					const ChemBaseData *chemData,
																					const MFVec2i &theAtomIndex)
{
	computeLODgSphereRA(doHydrogens,radScale,atomRadii,chemData,theAtomIndex);
	renderLODgHighlightSphereCO(highlightStyle,highlightColor,atomColors,chemData);
}


void ChemDisplay:: LODfNormalSphereRACO(const SbBool doHydrogens,
	const float radScale, const float *atomRadii,
	const SbColor *atomColors,
	const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
{
	computeLODfSphereRA(doHydrogens,radScale,atomRadii,chemData,theAtomIndex);
	renderLODfNormalSphereCO(atomColors,chemData);
}

void ChemDisplay::LODfHighlightSphereRACO(const int highlightStyle,
			const SbColor &highlightColor,const SbBool doHydrogens,
			const float radScale, const float *atomRadii,
			const SbColor *atomColors,
			const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
{
	computeLODfSphereRA(doHydrogens,radScale,atomRadii,chemData,theAtomIndex);
	renderLODfHighlightSphereCO(highlightStyle,highlightColor,atomColors,chemData);
}

void ChemDisplay::clipNormalSphereRACO(const int displayStyle, const SbBool doHydrogens,
											const SbBool doHemi, const float radScale, 
											const float *atomRadii, const SbColor *atomColors,
											const ChemBaseData *chemData, const MFVec2i &theAtomIndex, SoGLRenderAction *action)
{
	CLIP_RA

	if (lodSelector->sphere4Cnt> 0)
	{
		//beginSpheres(displayStyle); // dont reexecute 
		glColor3fv(atomColors[0].getValue());
		for (unsigned int i=0;i<lodSelector->sphere4Cnt;i++)
		{
			glPushMatrix();
			glMultMatrixf((float *)lodSelector->atomMatrices[lodSelector->sphere4Indices[i]].getValue());
			unitSphere->generateDisplay(doHemi,lastSphereDoNormals,lastSphereDoTextures,displayStyle,action);
			glPopMatrix();
		}
		//endSpheres(displayStyle); //dont reexecute
	}
}

//****************************** ROCI *****************************************//

void ChemDisplay::LODgNormalSphereROCI(const SbBool doHydrogens,
	const float radScale, const float *atomRadii,
	const SbColor *atomColors,
	const ChemBaseData *chemData, const MFVec2i &theAtomIndex)

{
	computeLODgSphereRO(doHydrogens,radScale,atomRadii,chemData,theAtomIndex);
	renderLODgNormalSphereCI(atomColors,chemData);
}

void ChemDisplay::LODgHighlightSphereROCI(const int highlightStyle,
																					const SbColor &highlightColor,
																					const SbBool doHydrogens,
																					const float radScale,
																					const float *atomRadii,
																					const SbColor *atomColors,
																					const ChemBaseData *chemData,
																					const MFVec2i &theAtomIndex)
{
	computeLODgSphereRO(doHydrogens,radScale,atomRadii,chemData,theAtomIndex);
	renderLODgHighlightSphereCI(highlightStyle,highlightColor,atomColors,chemData);
}

void ChemDisplay:: LODfNormalSphereROCI(const SbBool doHydrogens,
	const float radScale, const float *atomRadii,
	const SbColor *atomColors,
	const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
{
	computeLODfSphereRO(doHydrogens,radScale,atomRadii,chemData,theAtomIndex);
	renderLODfNormalSphereCI(atomColors,chemData);
}

void ChemDisplay::LODfHighlightSphereROCI(const int highlightStyle,
			const SbColor &highlightColor,const SbBool doHydrogens,
			const float radScale, const float *atomRadii,
			const SbColor *atomColors,
			const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
{
	computeLODfSphereRO(doHydrogens,radScale,atomRadii,chemData,theAtomIndex);
	renderLODfHighlightSphereCI(highlightStyle,highlightColor,atomColors,chemData);
}

void ChemDisplay::clipNormalSphereROCI(const int displayStyle, const SbBool doHydrogens,
											const SbBool doHemi, const float radScale, 
											const float *atomRadii, const SbColor *atomColors,
											const ChemBaseData *chemData, const MFVec2i &theAtomIndex, SoGLRenderAction *action)
{

	CLIP_RO

	if (lodSelector->sphere4Cnt> 0)
	{
		//beginSpheres(displayStyle); // dont reexecute 

		for (unsigned int i=0;i<lodSelector->sphere4Cnt;i++)
		{
			int32_t localAtomIndex = chemData->getAtomIndex(lodSelector->sphere4Indices[i]);
			glColor3fv(atomColors[localAtomIndex].getValue());
			
			glPushMatrix();
			glMultMatrixf((float *)lodSelector->atomMatrices[lodSelector->sphere4Indices[i]].getValue());
			unitSphere->generateDisplay(doHemi,lastSphereDoNormals,lastSphereDoTextures,displayStyle,action);
			glPopMatrix();
		}
		//endSpheres(displayStyle); //dont reexecute
	}
}

//****************************** ROCA *****************************************//

void ChemDisplay::LODgNormalSphereROCA(const SbBool doHydrogens,
	const float radScale, const float *atomRadii,
	const SbColor *atomColors,
	const ChemBaseData *chemData, const MFVec2i &theAtomIndex)

{
	computeLODgSphereRO(doHydrogens,radScale,atomRadii,chemData,theAtomIndex);
	renderLODgNormalSphereCA(atomColors,chemData);
}

void ChemDisplay::LODgHighlightSphereROCA(const int highlightStyle,
																					const SbColor &highlightColor,
																					const SbBool doHydrogens,
																					const float radScale,
																					const float *atomRadii,
																					const SbColor *atomColors,
																					const ChemBaseData *chemData,
																					const MFVec2i &theAtomIndex)
{
	computeLODgSphereRO(doHydrogens,radScale,atomRadii,chemData,theAtomIndex);
	renderLODgHighlightSphereCA(highlightStyle,highlightColor,atomColors,chemData);
}

void ChemDisplay:: LODfNormalSphereROCA(const SbBool doHydrogens,
	const float radScale, const float *atomRadii,
	const SbColor *atomColors,
	const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
{
	computeLODfSphereRO(doHydrogens,radScale,atomRadii,chemData,theAtomIndex);
	renderLODfNormalSphereCA(atomColors,chemData);
}

void ChemDisplay::LODfHighlightSphereROCA(const int highlightStyle,
			const SbColor &highlightColor,const SbBool doHydrogens,
			const float radScale, const float *atomRadii,
			const SbColor *atomColors,
			const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
{
	computeLODfSphereRO(doHydrogens,radScale,atomRadii,chemData,theAtomIndex);
	renderLODfHighlightSphereCA(highlightStyle,highlightColor,atomColors,chemData);
}

void ChemDisplay::clipNormalSphereROCA(const int displayStyle, const SbBool doHydrogens,
											const SbBool doHemi, const float radScale, 
											const float *atomRadii, const SbColor *atomColors,
											const ChemBaseData *chemData, const MFVec2i &theAtomIndex, SoGLRenderAction *action)
{
	CLIP_RO

	if (lodSelector->sphere4Cnt> 0)
	{
		//beginSpheres(displayStyle); // dont reexecute 
		for (unsigned int i=0;i<lodSelector->sphere4Cnt;i++)
		{
			//int32_t localAtomIndex = chemData->getAtomIndex(lodSelector->sphere4Indices[i]);
			glColor3fv(atomColors[lodSelector->sphere4Indices[i]].getValue());
			
			glPushMatrix();
			glMultMatrixf((float *)lodSelector->atomMatrices[lodSelector->sphere4Indices[i]].getValue());
			unitSphere->generateDisplay(doHemi,lastSphereDoNormals,lastSphereDoTextures,displayStyle,action);
			glPopMatrix();
		}
		//endSpheres(displayStyle);
	}
}

//****************************** ROCO *****************************************//

void ChemDisplay::LODgNormalSphereROCO(const SbBool doHydrogens,
	const float radScale, const float *atomRadii,
	const SbColor *atomColors,
	const ChemBaseData *chemData, const MFVec2i &theAtomIndex)

{
	computeLODgSphereRO(doHydrogens,radScale,atomRadii,chemData,theAtomIndex);
	renderLODgNormalSphereCO(atomColors,chemData);
}

void ChemDisplay::LODgHighlightSphereROCO(const int highlightStyle,
																					const SbColor &highlightColor,
																					const SbBool doHydrogens,
																					const float radScale,
																					const float *atomRadii,
																					const SbColor *atomColors,
																					const ChemBaseData *chemData,
																					const MFVec2i &theAtomIndex)
{
	computeLODgSphereRO(doHydrogens,radScale,atomRadii,chemData,theAtomIndex);
	renderLODgHighlightSphereCO(highlightStyle,highlightColor,atomColors,chemData);
}

void ChemDisplay:: LODfNormalSphereROCO(const SbBool doHydrogens,
	const float radScale, const float *atomRadii,
	const SbColor *atomColors,
	const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
{
	computeLODfSphereRO(doHydrogens,radScale,atomRadii,chemData,theAtomIndex);
	renderLODfNormalSphereCO(atomColors,chemData);
}

void ChemDisplay::LODfHighlightSphereROCO(const int highlightStyle,
			const SbColor &highlightColor,const SbBool doHydrogens,
			const float radScale, const float *atomRadii,
			const SbColor *atomColors,
			const ChemBaseData *chemData, const MFVec2i &theAtomIndex)
{
	computeLODfSphereRO(doHydrogens,radScale,atomRadii,chemData,theAtomIndex);
	renderLODfHighlightSphereCO(highlightStyle,highlightColor,atomColors,chemData);
}

void ChemDisplay::clipNormalSphereROCO(const int displayStyle, const SbBool doHydrogens,
											const SbBool doHemi, const float radScale, 
											const float *atomRadii, const SbColor *atomColors,
											const ChemBaseData *chemData, const MFVec2i &theAtomIndex, SoGLRenderAction *action)
{
	CLIP_RO

	if (lodSelector->sphere4Cnt> 0)
	{
		//beginSpheres(displayStyle); // dont reexecute 
		glColor3fv(atomColors[0].getValue());
		for (unsigned int i=0;i<lodSelector->sphere4Cnt;i++)
		{
			glPushMatrix();
			glMultMatrixf((float *)lodSelector->atomMatrices[lodSelector->sphere4Indices[i]].getValue());
			unitSphere->generateDisplay(doHemi,lastSphereDoNormals,lastSphereDoTextures,displayStyle,action);
			glPopMatrix();
		}
		//endSpheres(displayStyle); //dont reexecute
	}
}
//****************************** X *****************************************//
#undef ATOMLOOP_START
#undef ATOMLOOP_END
#undef RENDER_LOD_CI
#undef RENDER_LOD_CA
#undef RENDER_LOD_CO
#undef RENDER_LOD_HIGHLIGHT
#undef COMPUTE_LODg
#undef COMPUTE_LODf
#undef CLIP_RI
#undef CLIP_RA
#undef CLIP_RO
