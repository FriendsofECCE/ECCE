/*
 * ChemDisplayResidues.cpp
 *
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

#ident "$Revision: 22566 $"

#include <assert.h>
#include <ctype.h>
#include <math.h>

#include <memory.h>//memcpy
#include <float.h> //FLT_MIN

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

//
#include "inv/ChemKit/ChemLOD.H"
#include "inv/ChemKit/ChemOctreeNode.H"
//

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

#ifndef min
#define min(A,B) (A<B) ? A : B
#endif
#ifndef max
#define max(A,B) (A>B) ? A : B
#endif

#include "inv/ChemKit/ChemData.H"

#include "inv/nodes/SoSeparator.H"
#include "inv/nodes/SoNurbsCurve.H"
#include "inv/nodes/SoNurbsSurface.H"
#include "inv/nodes/SoCoordinate3.H"
#include "inv/nodes/SoComplexity.H"
#include "inv/nodes/SoDrawStyle.H"
#include "inv/nodes/SoMaterial.H"

#include "inv/errors/SoDebugError.H"

#include "inv/nodes/SoShapeHints.H" 
#include "inv/elements/SoGLShapeHintsElement.H"

#include "inv/ChemKit/ChemResidueStuff.H"
// --> schematic
#include "inv/ChemKit/ChemSchematicChain.H"
// <-- schematic

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

// --> schematic
#define SCHEMATICLOOP_START(INDEX)												  \
numSchematicLoops = (INDEX).getNum();      \
for (schematicLoop = 0; schematicLoop < numSchematicLoops; schematicLoop++) {	  \
	(INDEX)[schematicLoop].getValue(schematicStart, schematicEnd);		  \
	if (schematicEnd == CHEM_DISPLAY_USE_REST_OF_SCHEMATICS) {	  \
	   schematicEnd = globalNumberOfSchematics;			  \
	}							  \
	else {			  \
	   schematicEnd += schematicStart;	  \
	}				  \
	for (theSchematic = schematicStart; theSchematic< schematicEnd; theSchematic++) {

#define SCHEMATICLOOP_END	  \
	}		  \
}

static int numSchematicLoops;
static int schematicLoop;
static int32_t schematicStart;
static int32_t schematicEnd;
static int32_t theSchematic;
// <-- schematic

// --> octree duplicates
#define RESDUP(ID) \
assert(ID>=0 && ID <globalNumberOfResidues && renderedResidues != 0); \
if (renderedResidues[ID]) \
continue; \
else renderedResidues[ID] = true;
// <-- octree duplicates

// --> stick lod
#define RENDER_STICK_OV(LOD) \
if (lodSelector->getResidueIndexCnt(LOD) > 0) { \
	ChemResidueStuff* res=0;\
	int myIndex;\
	SbVec2i fromTo;fromTo.setValue(-1,-1); \
	beginCylinders(displayStyle,lodSelector->getResidueCylinder(LOD)); \
	for (theResidue=0; theResidue < lodSelector->getResidueIndexCnt(LOD); theResidue ++) \
	{ \
		myIndex = lodSelector->getResidueIndex(LOD,theResidue);\
  		assert(myIndex<globalNumberOfResidues);\
		res = &residueAttributes[myIndex];\
		const SbVec3f &coords1 = chemData->getAtomCoordinates(res->from).getValue(); \
		const SbVec3f &coords2 = chemData->getAtomCoordinates(res->to).getValue(); \
		lodSelector->getResidueCylinder(LOD)->render(coords1, coords2, rad, lastResidueCylinderDoNormals, lastResidueCylinderDoTextures,displayStyle,fromTo,this,action); \
	} \
	endCylinders(displayStyle); \
}

#define RENDER_STICK_RPN(LOD) \
if (lodSelector->getResidueIndexCnt(LOD) > 0) { \
	ChemResidueStuff* res=0;\
	SbVec2i fromTo;fromTo.setValue(-1,-1); \
	int colorIndex, myIndex;\
	beginCylinders(displayStyle,lodSelector->getResidueCylinder(LOD)); \
	for (theResidue=0; theResidue < lodSelector->getResidueIndexCnt(LOD); theResidue ++) \
	{ \
		myIndex = lodSelector->getResidueIndex(LOD,theResidue);\
  		assert(myIndex<globalNumberOfResidues);\
		res = &residueAttributes[myIndex];\
		const SbVec3f &coords1 = chemData->getAtomCoordinates(res->from).getValue(); \
		const SbVec3f &coords2 = chemData->getAtomCoordinates(res->to).getValue(); \
		if (chemData->residues[myIndex].getIndex() < 0) colorIndex = 0;\
		else colorIndex = chemData->residueColorIndex[chemData->residues[myIndex].getIndex()];\
		glColor3fv(chemColor->residueColor[colorIndex].getValue());\
		lodSelector->getResidueCylinder(LOD)->render(coords1, coords2, rad, lastResidueCylinderDoNormals, lastResidueCylinderDoTextures,displayStyle,fromTo,this,action); \
	} \
	endCylinders(displayStyle); \
}

#define RENDER_STICK_RPC(LOD) \
if (lodSelector->getResidueIndexCnt(LOD) > 0) { \
	ChemResidueStuff* res=0;\
	SbVec2i fromTo;fromTo.setValue(-1,-1); \
	int colorIndex, myIndex;\
	beginCylinders(displayStyle,lodSelector->getResidueCylinder(LOD)); \
	for (theResidue=0; theResidue < lodSelector->getResidueIndexCnt(LOD); theResidue ++) \
	{ \
		myIndex = lodSelector->getResidueIndex(LOD,theResidue);\
  		assert(myIndex<globalNumberOfResidues);\
		res = &residueAttributes[myIndex];\
		const SbVec3f &coords1 = chemData->getAtomCoordinates(res->from).getValue(); \
		const SbVec3f &coords2 = chemData->getAtomCoordinates(res->to).getValue(); \
		assert(chemData->residues[theResidue].getChain()<chemData->residueChainIndex.getNum());\
		colorIndex = chemData->residueChainIndex[chemData->residues[theResidue].getChain()];\
		glColor3fv(chemColor->residueColor[colorIndex].getValue());\
		lodSelector->getResidueCylinder(LOD)->render(coords1, coords2, rad, lastResidueCylinderDoNormals, lastResidueCylinderDoTextures,displayStyle,fromTo,this,action); \
	} \
	endCylinders(displayStyle); \
}

// <-- stick lod

#define POINT_PLANE_DIST(PLANE,POINT) \
PLANE.getNormal().dot(POINT) - PLANE.getDistanceFromOrigin();

static void
checkResidueForPick(SoRayPickAction *action, SbLine &pickLine, 
	ChemData *chemData, ChemColor *chemColor, ChemDisplay *chemDisplay,
	int32_t theResidue,
	SbVec3f &coords1, SbVec3f &coords2);


void ChemDisplay::computeRibbonVectors(ChemData *chemData, ChemDisplayParam *cdp)
{
	//SbVec3f a,b,c,ca,cap,d,dp,e,h,f,g,o,p,l,r;
	SbVec3f a,b,c,ca,cap,d,dp,o,p,l,r;
	int i,j,caIndex,oIndex, nextResidueWithCAIndex;
	float ribbonWidth,helixOffsetWidth;
	bool bHelix, bFirstPoint, bNextCaFound;
	double temp;
	
	caIndex = SbResidue::RESIDUE_CONTROL_POINT_CA;
	oIndex = SbResidue::RESIDUE_CONTROL_POINT_O;

	bFirstPoint = true;

	if (chemData->residues.getNum() < 2)
		return;

	pl.setNum(chemData->residues.getNum()-1);
	pr.setNum(chemData->residues.getNum()-1);
	pIndex.setNum(chemData->residues.getNum()-1);

	for (i=0; i<chemData->residues.getNum()-1; i++)
	{
		pIndex.set1Value(i,-1);

		if (!chemData->residues[i].checkControlPoints())
			continue;

		ca = chemData->getAtomCoordinates(
				chemData->residues[i][caIndex]);
		o = chemData->getAtomCoordinates(
				chemData->residues[i][oIndex]);

		bNextCaFound = true;
		nextResidueWithCAIndex = i+1;

		if (chemData->residues[nextResidueWithCAIndex][caIndex] < 0)
		{
			bNextCaFound = false;
			// find the next valid point
			for (j=i+2; j<chemData->residues.getNum(); j++)
			{
				if (chemData->residues[j][caIndex] >= 0)
				{
					bNextCaFound = true;
					nextResidueWithCAIndex = j;
					break;
				}
			}
		}

		if (!bNextCaFound)
			continue;

		cap = chemData->getAtomCoordinates(chemData->residues[nextResidueWithCAIndex][caIndex]);
		
		a = cap - ca;
		b = o - ca;					//B is vector CAi to Oi
		c = a.cross(b);
		d = c.cross(a);
		c.normalize();
		d.normalize();

		p = ca + a*0.5f; // midpoint

		bHelix = (chemData->residues[i].residueType.getValue() == SbResidue::RESIDUE_TYPE_HELIX);
		
		switch(chemData->residues[i].residueType.getValue())
		{
			case SbResidue::RESIDUE_TYPE_HELIX:
			{
				ribbonWidth = cdp->residueHelixWidth.getValue();
				helixOffsetWidth = cdp->residueHelixOffsetWidth.getValue();
				break;
			}
			case SbResidue::RESIDUE_TYPE_SHEET:
			{
				ribbonWidth = cdp->residueSheetWidth.getValue();
				break;
			}
			
			case SbResidue::RESIDUE_TYPE_TURN:
			{
				ribbonWidth = cdp->residueTurnWidth.getValue();
				break;
			}
			case SbResidue::RESIDUE_TYPE_COIL:
			{
				ribbonWidth = cdp->residueCoilWidth.getValue();
				break;
			}
			default:
			{
				SoDebugError::post("ChemDisplay::computeRibbonVectors",
        "Unknow residue type. Ribbon width set to 1.0");
				ribbonWidth = 1.0f;
				break;
			}
		}
		
		if (bHelix)
			p += helixOffsetWidth*c;

		d *= (0.5f * ribbonWidth);

		l = p - d;
		r = p + d;
		
		if (bFirstPoint)
		{
			pl.set1Value(i,l);
			pr.set1Value(i,r);

			dp = d;
			bFirstPoint = false;
		}
		else // the other points
		{
			temp = dp.dot(d);

			// if the angle is > 90 or < -90 the cos is minor than 0
			if (temp < 0.0) // flip the line segment defined in the previous step
			{
				pl.set1Value(i,r);
				pr.set1Value(i,l);

				dp = -d;

			}
			else
			{
					pl.set1Value(i,l);
					pr.set1Value(i,r);

					dp = d;
			}
		}
		pIndex.set1Value(i,i);//valid ribbon point
	}
}

void ChemDisplay::computeResidueAttributes(SoAction *action)
{
	ChemData *chemData = (ChemData*)ChemBaseDataElement::get(action->getState());
	if (chemData == NULL)
		return;

	ChemDisplayParam *cdp = (ChemDisplayParam*)ChemDisplayParamElement::get(action->getState());
	if (cdp == NULL)
		return;

	computeRibbonVectors(chemData,cdp); // compute pl i pr;
	computeBoundsForSolidRibbon(action);

	if (residueAttributes != NULL)
		delete [] residueAttributes;

	residueAttributes = NULL;

	if (chemData->getNumberOfResidues()>0)
		residueAttributes = new ChemResidueStuff[chemData->getNumberOfResidues()];
	else
		return;

	assert(residueAttributes != NULL);

	SbVec3f ca, cap;
	ChemResidueStuff *res = NULL;
	
	float distance,maxDistance = cdp->residueMaxDistance.getValue();
	int i,j,k,l;
	
	// prepare stick structure
	
	// special case: first residue 
	int firstResidue = 0;
	for (i=0; i<chemData->residues.getNum(); i++)
	{
		res = &residueAttributes[i];
		assert(res != NULL);
		res->index = i;
		res->isStickRenderable = false;
		if (chemData->residues[i][SbResidue::RESIDUE_CONTROL_POINT_CA] >= 0)
		{
			res->from = res->to = chemData->residues[i][SbResidue::RESIDUE_CONTROL_POINT_CA];
			break;
		}
	}

	firstResidue = i;

	// remaining
	for (i=firstResidue+1; i<chemData->residues.getNum(); i++)
	{
		res = &residueAttributes[i];
		assert(res != NULL);
		// assign index, bools
		res->index = i;
		res->isStickRenderable = false;

		res->from = chemData->residues[i-1][SbResidue::RESIDUE_CONTROL_POINT_CA];
		res->to = chemData->residues[i][SbResidue::RESIDUE_CONTROL_POINT_CA];

		if (res->from < 0)
		{
			continue;
		}

		if (res->to < 0) // find next ca
		{
			for (j=i+1; j<chemData->residues.getNum(); j++)
			{
				if (chemData->residues[j][SbResidue::RESIDUE_CONTROL_POINT_CA] >= 0)
				{
					res->to = chemData->residues[j][SbResidue::RESIDUE_CONTROL_POINT_CA];
					break;// we got 2 ca's
				}
			}
		}

		if (res->to < 0) // if not ca found
		{
			continue;
		}

		// check ca distance
		ca = chemData->getAtomCoordinates(res->from);
		cap = chemData->getAtomCoordinates(res->to);
		distance = (cap - ca).length();

		if (distance > maxDistance)
		{
#ifdef _DEBUG
			  fprintf(stderr,"Residue %d discarded by residueMaxDistance\n", i);
#endif
				pIndex.set1Value(i-1,-2);//mark as discarded
				res->caDistance = false;
			continue;
		}
		res->caDistance = true;
		res->isStickRenderable = true;
	}

	// prepare line, flat and solid ribbon
	int state = 0;
// --> fix single amino acid bug
//	At least, 2 residues are required to display ribbon and ca structures
	if (chemData->residues.getNum()>1)
// <-- fix single amino acid bug
	for (i=firstResidue; i<chemData->residues.getNum(); i++) //all residues
	{
		res = &residueAttributes[i];

		res->isRibbonRenderable = false;

		if (!chemData->residues[i].checkControlPoints())
			continue; 

		// assign control point indices
		switch (state)
		{
			case 0: // first
			{
				res->indices[0] = i;
				res->indices[1] = i;
				res->indices[2] = i;
				res->indices[3] = i+1;
				state = 1;
				break;
			}
			case 1: // second
			{
				res->indices[0] = i-1;
				res->indices[1] = i-1;
				res->indices[2] = i;
				res->indices[3] = i+1;
				state = 2;
				break;
			}
			case 2: // interior part
			{
				res->indices[0] = i-2;
				res->indices[1] = i-1;
				res->indices[2] = i;
				res->indices[3] = i+1;
				break;
			}
			default: break;
		}

		for (j=0; j<4; j++)
		{
			if (res->indices[j] >= pl.getNum())
				res->indices[j] = pl.getNum()-1;
		}
		
		if (pIndex[res->indices[3]] == -2) // penultimate
		{
			res->indices[3] = res->indices[2];
		}
		else if (pIndex[res->indices[2]] == -2) // last
		{
			res->indices[2] = res->indices[3] = res->indices[1];
			state = 0;
		}
		
		// check and fix indices
		for (j=0; j<4; j++)
		{
			if (pIndex[res->indices[j]] == -1 ) // if ribbon point is not valid
			{
				if (j==0) // previous
				{
					l = res->indices[j];
					res->indices[j] = res->indices[j+1]; //default value if any point is found
					if (pIndex[l] != -2) // if point is not start of chain
					{
						for (k=l-1; k>=0; k--) 
						{
							if (pIndex[k] >= 0 ) // if ribbon point is valid
							{
								res->indices[j] = pIndex[k];
								break;//continue testing next ribbon points
							}
						}
					}
				}
				else // next
				{
					l = res->indices[j];
					res->indices[j] = res->indices[j-1];//default value if any point is found

					for (k=l+1; k<pl.getNum(); k++) 
					{
						if (pIndex[k] >= 0 ) // if ribbon point is valid
						{
							res->indices[j] = pIndex[k];
							// readjust the points // 
							for (l=j+1; l<4;l++) res->indices[l] = res->indices[l-1]+1;

							break;//continue testing next ribbon points
						}
					}
				}
			} // if ribbon point is not valid
		} // check and fix indices

		// last check: check corrected indices
		if (pIndex[res->indices[0]] == -1  ||
				pIndex[res->indices[1]] == -1  ||
				pIndex[res->indices[2]] == -1 ||
				pIndex[res->indices[3]] == -1)
				continue;

		res->isRibbonRenderable = true;
	} // for (int i...) // for interior residues

	
	// precompute basis coefficients
	preComputeBasisCoefficients();

	// prepare AABB information
	for (i=0;i<chemData->getNumberOfResidues();i++)
	{
		res = &residueAttributes[i];
		if (!res->isRibbonRenderable) continue;
		computeSolidRibbonGeometry(i);
		//
		SbVec3f minv(FLT_MAX,FLT_MAX,FLT_MAX);
		SbVec3f maxv(-FLT_MAX,-FLT_MAX,-FLT_MAX);

		minv[0] = min(minv[0],rtl[0][0]);
		minv[1] = min(minv[1],rtl[0][1]);
		minv[2] = min(minv[2],rtl[0][2]);
		minv[0] = min(minv[0],rtr[0][0]);
		minv[1] = min(minv[1],rtr[0][1]);
		minv[2] = min(minv[2],rtr[0][2]);
		minv[0] = min(minv[0],rbl[ribbonSegmentation-1][0]);
		minv[1] = min(minv[1],rbl[ribbonSegmentation-1][1]);
		minv[2] = min(minv[2],rbl[ribbonSegmentation-1][2]);
		minv[0] = min(minv[0],rbr[ribbonSegmentation-1][0]);
		minv[1] = min(minv[1],rbr[ribbonSegmentation-1][1]);
		minv[2] = min(minv[2],rbr[ribbonSegmentation-1][2]);

		maxv[0] = max(maxv[0],rtl[0][0]);
		maxv[1] = max(maxv[1],rtl[0][1]);
		maxv[2] = max(maxv[2],rtl[0][2]);
		maxv[0] = max(maxv[0],rtr[0][0]);
		maxv[1] = max(maxv[1],rtr[0][1]);
		maxv[2] = max(maxv[2],rtr[0][2]);
		maxv[0] = max(maxv[0],rbl[ribbonSegmentation-1][0]);
		maxv[1] = max(maxv[1],rbl[ribbonSegmentation-1][1]);
		maxv[2] = max(maxv[2],rbl[ribbonSegmentation-1][2]);
		maxv[0] = max(maxv[0],rbr[ribbonSegmentation-1][0]);
		maxv[1] = max(maxv[1],rbr[ribbonSegmentation-1][1]);
		maxv[2] = max(maxv[2],rbr[ribbonSegmentation-1][2]);
		
		res->center = (maxv + minv)*0.5f;
		res->halfsize = (maxv - minv)*0.5f;		
		//
	}
}

void ChemDisplay::setupResidues(SoGLRenderAction *action, SoState *&state,
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
	displayStyle = cdp->residueCylinderDisplayStyle.getValue();

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
	float bcc = cdp->residueCylinderComplexity.getValue();

	if ((lastResidueCylinderComplexity != bcc)	|| 
		(lastResidueCylinderDisplayStyle != displayStyle)) {

		lastResidueCylinderComplexity = bcc;
		lastResidueCylinderDisplayStyle = displayStyle;

		if (residueUnitCylinder != NULL) delete residueUnitCylinder;
		residueUnitCylinder = new ChemUnitCylinder(displayStyle, bcc);
	}
	lastResidueCylinderDoNormals = doNormals;
	lastResidueCylinderDoTextures = doTextures;

// --> solid ribbon
	residueInterpolateColor = cdp->residueInterpolateColor.getValue();
	solidRibbonSmoothNormals = cdp->solidRibbonSmoothNormals.getValue();
// <-- solid ribbon
// --> line ribbon
	int32_t numberOfThreads = cdp->residueNumberOfThreads.getValue();

	if (residueNumberOfThreads.getValue() != numberOfThreads)
	{
		if (lr!= NULL)
		{
			delete[] lr;
			lr = NULL;
		}

		if (numberOfThreads > 0)
			lr = new SoMFVec3f[numberOfThreads];

		residueNumberOfThreads = numberOfThreads;
	}
// <-- line ribbon
}

void ChemDisplay::renderResiduesAsWireframe(SoGLRenderAction *action)
{
#ifdef DEBUG
printf("In ChemDisplay::renderResiduesAsWireframe\n");
#endif
	SoState *state     = action->getState();
	ChemColor* chemColor = ChemColorElement::get(state);
	ChemDisplayParam *cdp       = ChemDisplayParamElement::get(state);
	ChemData *chemData  = (ChemData*)ChemBaseDataElement::get(state);

	// Check to see if there's anything to render.
	if (residueIndex.getNum() == 0 ||  globalNumberOfResidues == 0) return;

	if (chemData->residues.getNum() > 0)
	if ((normalResidueIndex.getNum() > 0) || (highlightResidueIndex.getNum() > 0)) 
	{

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
			(cdp->residueWireframeAntiAlias.getValue() !=
				ChemDisplayParam::WIREFRAME_ANTIALIAS_NONE))
		{
			glPushAttrib(GL_COLOR_BUFFER_BIT |  // for GL_BLEND
				     GL_DEPTH_BUFFER_BIT |  // for GL_DEPTH_TEST
				     GL_LINE_BIT         |  // for GL_LINE_SMOOTH
				     GL_HINT_BIT);          // for GL_LINE_SMOOTH_HINT
			glEnable( GL_LINE_SMOOTH );
			glHint  ( GL_LINE_SMOOTH_HINT, GL_NICEST );
			glEnable( GL_BLEND );
			if (cdp->residueWireframeAntiAlias.getValue() ==
				ChemDisplayParam::WIREFRAME_ANTIALIAS_WITH_DEPTH_COMPARISON)
			{
				glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
				glEnable( GL_DEPTH_TEST );
			}
			else
			{
				glBlendFunc( GL_SRC_ALPHA, GL_ONE );
				glDisable( GL_DEPTH_TEST );
			}
		}

		if (normalResidueIndex.getNum() > 0)
		{
			normalResiduesAsWireframe(vnormalResidueIndex,
				state, chemColor, cdp, chemData);
		}

		if (highlightResidueIndex.getNum() > 0)
		{
			highlightResiduesAsWireframe(vhighlightResidueIndex,state, chemColor, cdp, chemData);
		}
	
		if (!renderAsPoints &&
			(cdp->bondWireframeAntiAlias.getValue() !=
		ChemDisplayParam::WIREFRAME_ANTIALIAS_NONE))
		{
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

void ChemDisplay::normalResiduesAsWireframe(const MFVec2i &theResiduesIndex,
	SoState *state, const ChemColor *chemColor,
	const ChemDisplayParam *cdp, const ChemData *chemData)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf("In normalResiduesAsWireframe\n");
#endif

	// If there is nothing to display, then return.
	if (theResiduesIndex.getNum() == 0) return;

	SbBool renderAsPoints;

	// Since GL cannot render lines as points, we need to handle this
	// ourselves.
	renderAsPoints = (SoDrawStyleElement::get(state) == 
					  SoDrawStyleElement::POINTS);
	
	int32_t	colorIndex;

	// Render the wires
	switch (chemColor->residueColorBinding.getValue()) 
	{

		case ChemColor::RESIDUE_OVERALL:
		{
			// All residues are the same color which is the first color in the
			// residues color table.
			const SbColor *residueColors = chemColor->residueColor.getValues(0);
			glColor3fv(residueColors[0].getValue());
			
			if (renderAsPoints)
			{
				glBegin(GL_POINTS);
			}
			else
			{
				glLineWidth(cdp->residueWireframeLineWidth.getValue());
				glBegin(GL_LINES);
			}

			ChemResidueStuff *res = NULL;
			RESIDUELOOP_START(theResiduesIndex)
				
				res = &residueAttributes[theResidue];
				if (!res->isStickRenderable) continue;
				
				glVertex3fv(chemData->getAtomCoordinates(res->from).getValue());
				glVertex3fv(chemData->getAtomCoordinates(res->to).getValue());

			RESIDUELOOP_END

			glEnd();

		}
		break;

		case ChemColor::RESIDUE_PER_INDEX:
		{	
			if (renderAsPoints)
			{
				glBegin(GL_POINTS);
			}
			else
			{
				glLineWidth(cdp->residueWireframeLineWidth.getValue());
				glBegin(GL_LINES);
			}

			ChemResidueStuff *res = NULL;
			RESIDUELOOP_START(theResiduesIndex)
				
				res = &residueAttributes[theResidue];
				if (!res->isStickRenderable) continue;

				if (chemData->residues[theResidue].getIndex() < 0) colorIndex = 0;
				else colorIndex = chemData->residueColorIndex[chemData->residues[theResidue].getIndex()];
				glColor3fv(chemColor->residueColor[colorIndex].getValue());

				glVertex3fv(chemData->getAtomCoordinates(res->from).getValue());
				glVertex3fv(chemData->getAtomCoordinates(res->to).getValue());

			RESIDUELOOP_END

			glEnd();
		}
		break;

		case ChemColor::RESIDUE_PER_CHAIN:
		{
			if (renderAsPoints)
			{
				glBegin(GL_POINTS);
			}
			else
			{
				glLineWidth(cdp->residueWireframeLineWidth.getValue());
				glBegin(GL_LINES);
			}

			ChemResidueStuff *res = NULL;
			RESIDUELOOP_START(theResiduesIndex)
				
				res = &residueAttributes[theResidue];
				if (!res->isStickRenderable) continue;

				colorIndex = chemData->residueChainIndex[chemData->residues[theResidue].getChain()];
				glColor3fv(chemColor->residueColor[colorIndex].getValue());

				glVertex3fv(chemData->getAtomCoordinates(res->from).getValue());
				glVertex3fv(chemData->getAtomCoordinates(res->to).getValue());

			RESIDUELOOP_END

			glEnd();
		}
		break;

		default: break;
	}
}

void ChemDisplay::highlightResiduesAsWireframe(const MFVec2i &theResidueIndex,
																	SoState *state,
																	const ChemColor *chemColor,
																	const ChemDisplayParam *cdp,
																	const ChemData *chemData)
{
#ifdef DEBUG
printf("In highlightResiduesAsWireframe\n");
#endif

	// First check to see if there is anything to highlight
	if (theResidueIndex.getNum() == 0) return;

	SbBool	renderAsPoints;
	SbColor highlightColor = cdp->highlightColor.getValue();
	int32_t highlightStyle = cdp->highlightStyle.getValue();
	SbColor tmpColor;
	int32_t colorIndex;

	// Since GL cannot render lines as points, we need to handle this
	// ourselves.
	renderAsPoints = (SoDrawStyleElement::get(state) == SoDrawStyleElement::POINTS);

	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE)
	{
		glColor3fv(highlightColor.getValue());

		if (renderAsPoints)
		{
			glBegin(GL_POINTS);
		}
		else
		{
			glLineWidth(cdp->residueWireframeLineWidth.getValue());
			glBegin(GL_LINES);
		}

		ChemResidueStuff *res = NULL;
		
		RESIDUELOOP_START(theResidueIndex)
			
			res = &residueAttributes[theResidue];
			if (!res->isStickRenderable) continue;
			
			glVertex3fv(chemData->getAtomCoordinates(res->from).getValue());
			glVertex3fv(chemData->getAtomCoordinates(res->to).getValue());

		RESIDUELOOP_END

		glEnd();
	}
	else //if (highlightStyle != ChemDisplayParam::HIGHLIGHT_DIFFUSE)
	{
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
		{
			glEnable(GL_LINE_STIPPLE);
			glLineStipple(cdp->highlightStippleFactor.getValue(),
				cdp->highlightStipplePattern.getValue());
		}
		glLineWidth(cdp->residueWireframeLineWidth.getValue());

		switch (chemColor->residueColorBinding.getValue()) 
		{
			case ChemColor::RESIDUE_OVERALL:
			{
				// All residues are the same color which is the first color in the
				// residues color table.
				const SbColor *residueColors = chemColor->residueColor.getValues(0);
				glColor3fv(residueColors[0].getValue());

				if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
				{
					glColor3fv(residueColors[0].getValue());
				}
				else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
				{
					tmpColor = residueColors[0] + highlightColor;
					tmpColor.normalize();
					glColor3fv(tmpColor.getValue());
				}
						
				if (renderAsPoints)
				{
					glBegin(GL_POINTS);
				}
				else
				{
					glLineWidth(cdp->residueWireframeLineWidth.getValue());
					glBegin(GL_LINES);

				}

				ChemResidueStuff *res = NULL;
				
				RESIDUELOOP_START(theResidueIndex)
					
					res = &residueAttributes[theResidue];
					if (!res->isStickRenderable) continue;
					
					glVertex3fv(chemData->getAtomCoordinates(res->from).getValue());
					glVertex3fv(chemData->getAtomCoordinates(res->to).getValue());

				RESIDUELOOP_END

				glEnd();
			}
			break;

			case ChemColor::RESIDUE_PER_INDEX:
			{
				ChemResidueStuff *res = NULL;
				RESIDUELOOP_START(theResidueIndex)
					
					res = &residueAttributes[theResidue];
					if (!res->isStickRenderable) continue;

					if (chemData->residues[theResidue].getIndex() < 0) colorIndex = 0;
					else colorIndex = chemData->residueColorIndex[chemData->residues[theResidue].getIndex()];
				
					if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
					{
						glColor3fv(chemColor->residueColor[colorIndex].getValue());
					}
					else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
					{
						tmpColor = chemColor->residueColor[colorIndex] + highlightColor;
						tmpColor.normalize();
						glColor3fv(tmpColor.getValue());
					}
							
					if (renderAsPoints)
					{
						glBegin(GL_POINTS);
					}
					else
					{
						glLineWidth(cdp->residueWireframeLineWidth.getValue());
						glBegin(GL_LINES);
					}

					glVertex3fv(chemData->getAtomCoordinates(res->from).getValue());
					glVertex3fv(chemData->getAtomCoordinates(res->to).getValue());
					
					glEnd();

				RESIDUELOOP_END
			}
			break;

			case ChemColor::RESIDUE_PER_CHAIN:
			{
				ChemResidueStuff *res = NULL;
				RESIDUELOOP_START(theResidueIndex)
					
					res = &residueAttributes[theResidue];
					if (!res->isStickRenderable) continue;

					colorIndex = chemData->residueChainIndex[chemData->residues[theResidue].getChain()];
				
					if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
					{
						glColor3fv(chemColor->residueColor[colorIndex].getValue());
					}
					else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
					{
						tmpColor = chemColor->residueColor[colorIndex] + highlightColor;
						tmpColor.normalize();
						glColor3fv(tmpColor.getValue());
					}
							
					if (renderAsPoints)
					{
						glBegin(GL_POINTS);
					}
					else
					{
						glLineWidth(cdp->residueWireframeLineWidth.getValue());
						glBegin(GL_LINES);
					}
					glVertex3fv(chemData->getAtomCoordinates(res->from).getValue());
					glVertex3fv(chemData->getAtomCoordinates(res->to).getValue());
					
					glEnd();

				RESIDUELOOP_END
			}
			break;

			default: break;
		}

		if ((highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE) && !renderAsPoints)
		{
			glDisable(GL_LINE_STIPPLE);
		}
	}
}

void ChemDisplay::renderResiduesAsCylinders(SoGLRenderAction *action)
{
#ifdef DEBUG
printf ("In ChemDisplay::renderResiduesAsCylinders\n");
#endif

// Check to see if there's anything to render.
	if (residueIndex.getNum() == 0 ||  globalNumberOfResidues == 0) return;

	SoState *state;
	ChemColor *chemColor;
	ChemDisplayParam *cdp;
	ChemBaseData *chemBaseData;
	ChemData *chemData;

	setupResidues(action, state, chemColor, cdp, chemBaseData);

	chemData = (ChemData*)chemBaseData;

	// Make sure things are set up correctly for a solid object
	beginSolidShape(action);

// --> multiple scene graphs fix
	glEnable(GL_LIGHTING);
// <-- multiple scene graphs fix

#ifndef IV2_0
	int32_t myMask;
	if (cdp->highlightStyle.getValue() == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
	{
		myMask = SoLazyElement::DIFFUSE_MASK | SoLazyElement::EMISSIVE_MASK;
	}
	else
	{
		myMask = SoLazyElement::DIFFUSE_MASK;
	}

	SoGLLazyElement *lazyElt = (SoGLLazyElement *)
			SoLazyElement::getInstance(state);
	lazyElt->send(state, SoLazyElement::ALL_MASK &( ~myMask));
#endif

	beginCylinders(cdp->residueCylinderDisplayStyle.getValue(),residueUnitCylinder);

	//float rad = cdp->residueCylinderRadius.getValue();
	//int displayStyle = cdp->residueCylinderDisplayStyle.getValue();

	computeStickCulling(vnormalResidueIndex,cdp,chemData);

	// Render the cylinders
	switch (chemColor->residueColorBinding.getValue()) 
	{
		case ChemColor::RESIDUE_OVERALL:
		{
			normalStickOV(vnormalResidueIndex, state, chemColor->residueColor[0],cdp, chemData);
		}
		break;

		case ChemColor::RESIDUE_PER_INDEX:
		{
			normalStickRPN(vnormalResidueIndex, state, chemColor,cdp, chemData);
		}
		break;

		case ChemColor::RESIDUE_PER_CHAIN:
		{
			normalStickRPC(vnormalResidueIndex, state, chemColor,cdp, chemData);
		}
		break;

		default: break;
	}

	computeStickCulling(vhighlightResidueIndex,cdp,chemData);

	switch (chemColor->residueColorBinding.getValue()) 
	{
		case ChemColor::RESIDUE_OVERALL:
		{
			highlightStickOV(vhighlightResidueIndex, state, chemColor, cdp, chemData);
		}
		break;

		case ChemColor::RESIDUE_PER_INDEX:
		{
			highlightStickRPN(vhighlightResidueIndex, state, chemColor, cdp, chemData);
		}
		break;

		case ChemColor::RESIDUE_PER_CHAIN:
		{
			highlightStickRPC(vhighlightResidueIndex, state, chemColor, cdp, chemData);
		}
		break;

		default: break;
	}


#ifndef IV2_0
	lazyElt->reset(state, myMask);
#endif

	endCylinders(cdp->residueDisplayStyle.getValue());

	endSolidShape(action);
}

void ChemDisplay::normalStickOV(const MFVec2i &theResidueIndex,
																	SoState *state,
																	const SbColor &chemColor,
																	const ChemDisplayParam *cdp,
																	const ChemData *chemData)
{
	// Check to see if there's anything to render.
	if (theResidueIndex.getNum() == 0) return;

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

	glColor3fv(chemColor.getValue());

	int displayStyle = cdp->residueCylinderDisplayStyle.getValue();
	float rad = cdp->residueCylinderRadius.getValue();
	SoGLRenderAction* action = (SoGLRenderAction*)state->getAction();

	RENDER_STICK_OV(1);
	RENDER_STICK_OV(2);
	RENDER_STICK_OV(3);
	RENDER_STICK_OV(4);

	glDisable(GL_COLOR_MATERIAL);
}

void ChemDisplay::normalStickRPN(const MFVec2i &theResidueIndex,
																	SoState *state,
																	const ChemColor *chemColor,
																	const ChemDisplayParam *cdp,
																	const ChemData *chemData)
{
	// Check to see if there's anything to render.
	if (theResidueIndex.getNum() == 0) return;

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);
	
	int displayStyle = cdp->residueCylinderDisplayStyle.getValue();
	float rad = cdp->residueCylinderRadius.getValue();
	SoGLRenderAction* action = (SoGLRenderAction*)state->getAction();

	RENDER_STICK_RPN(1);
	RENDER_STICK_RPN(2);
	RENDER_STICK_RPN(3);
	RENDER_STICK_RPN(4);

	glDisable(GL_COLOR_MATERIAL);
}

void ChemDisplay::normalStickRPC(const MFVec2i &theResidueIndex,
																	SoState *state,
																	const ChemColor *chemColor,
																	const ChemDisplayParam *cdp,
																	const ChemData *chemData)
{
		// Check to see if there's anything to render.
	if (theResidueIndex.getNum() == 0) return;

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);
	
	int displayStyle = cdp->residueCylinderDisplayStyle.getValue();
	float rad = cdp->residueCylinderRadius.getValue();
	SoGLRenderAction* action = (SoGLRenderAction*)state->getAction();

	RENDER_STICK_RPC(1);
	RENDER_STICK_RPC(2);
	RENDER_STICK_RPC(3);
	RENDER_STICK_RPC(4);

	glDisable(GL_COLOR_MATERIAL);
}

void ChemDisplay::highlightStickOV(const MFVec2i &theResidueIndex,
																		 SoState *state,
																		 const ChemColor *chemColor,
																		 const ChemDisplayParam *cdp,
																		 const ChemData *chemData)
{
	// Check to see if there's anything to render.
	if (theResidueIndex.getNum() == 0) return;

	float		rad;
	SbVec3f		theCapScale;

	int displayStyle = cdp->residueCylinderDisplayStyle.getValue();
	rad = cdp->residueCylinderRadius.getValue();
	theCapScale.setValue(rad, rad, rad);

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

	int32_t highlightStyle = cdp->highlightStyle.getValue();
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
	{
		const SbColor &theColor = cdp->highlightColor.getValue();
		glMaterialfv(GL_FRONT, GL_EMISSION,
			SbVec4f(theColor[0], theColor[1], theColor[2], 1.0).getValue());
		glColor3fv(chemColor->residueColor[0].getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE)
	{
		const SbColor &theColor = cdp->highlightColor.getValue();
		glColor3fv(theColor.getValue());
	}

	SbVec2i fromTo;fromTo.setValue(-1,-1);
	ChemResidueStuff *res = NULL;
	SoGLRenderAction *action = (SoGLRenderAction *)state->getAction();

	RESIDUELOOP_START(theResidueIndex)

		res = &residueAttributes[theResidue];
		if (!res->isStickRenderable) continue;
		
		residueUnitCylinder->render(
			chemData->getAtomCoordinates(res->from).getValue(), 
			chemData->getAtomCoordinates(res->to).getValue(), rad,
			lastResidueCylinderDoNormals, lastResidueCylinderDoTextures, displayStyle,fromTo,this,action);

	RESIDUELOOP_END

	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
	{
		glMaterialfv(GL_FRONT, GL_EMISSION,
			SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	glDisable(GL_COLOR_MATERIAL);
}

void ChemDisplay::highlightStickRPN(const MFVec2i &theResidueIndex,
																		 SoState *state,
																		 const ChemColor *chemColor,
																		 const ChemDisplayParam *cdp,
																		 const ChemData *chemData)
{
	// Check to see if there's anything to render.
	if (theResidueIndex.getNum() == 0) return;

	int32_t colorIndex;
	float		rad;
	SbVec3f		theCapScale;

	int displayStyle = cdp->residueCylinderDisplayStyle.getValue();
	rad = cdp->residueCylinderRadius.getValue();
	theCapScale.setValue(rad, rad, rad);

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

	int32_t highlightStyle = cdp->highlightStyle.getValue();
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
	{
		const SbColor &theColor = cdp->highlightColor.getValue();
		glMaterialfv(GL_FRONT, GL_EMISSION,
			SbVec4f(theColor[0], theColor[1], theColor[2], 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	
	SbVec2i fromTo;fromTo.setValue(-1,-1);
	ChemResidueStuff *res = NULL;
	SoGLRenderAction *action = (SoGLRenderAction *)state->getAction();
	RESIDUELOOP_START(theResidueIndex)

		res = &residueAttributes[theResidue];
		if (!res->isStickRenderable) continue;
		
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE)
		{
			const SbColor &theColor = cdp->highlightColor.getValue();
			glColor3fv(theColor.getValue());
		}
		else
		{
			if (chemData->residues[theResidue].getIndex() < 0) colorIndex = 0;
			else colorIndex = chemData->residueColorIndex[chemData->residues[theResidue].getIndex()];
			glColor3fv(chemColor->residueColor[colorIndex].getValue());
		}

		residueUnitCylinder->render(
			chemData->getAtomCoordinates(res->from).getValue(), 
			chemData->getAtomCoordinates(res->to).getValue(), rad,
			lastResidueCylinderDoNormals, lastResidueCylinderDoTextures, displayStyle,fromTo,this,action);

	RESIDUELOOP_END

	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
	{
		glMaterialfv(GL_FRONT, GL_EMISSION,
			SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	glDisable(GL_COLOR_MATERIAL);
}

void ChemDisplay::highlightStickRPC(const MFVec2i &theResidueIndex,
																		 SoState *state,
																		 const ChemColor *chemColor,
																		 const ChemDisplayParam *cdp,
																		 const ChemData *chemData)
{
	// Check to see if there's anything to render.
	if (theResidueIndex.getNum() == 0) return;

	int32_t colorIndex;
	float		rad;
	SbVec3f		theCapScale;

	int displayStyle = cdp->residueCylinderDisplayStyle.getValue();
	rad = cdp->residueCylinderRadius.getValue();
	theCapScale.setValue(rad, rad, rad);

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

	int32_t highlightStyle = cdp->highlightStyle.getValue();
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
	{
		const SbColor &theColor = cdp->highlightColor.getValue();
		glMaterialfv(GL_FRONT, GL_EMISSION,
			SbVec4f(theColor[0], theColor[1], theColor[2], 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	
	SbVec2i fromTo;fromTo.setValue(-1,-1);
	ChemResidueStuff *res = NULL;
	SoGLRenderAction *action = (SoGLRenderAction *)state->getAction();
	RESIDUELOOP_START(theResidueIndex)

		res = &residueAttributes[theResidue];
		if (!res->isStickRenderable) continue;
		
		if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE)
		{
			const SbColor &theColor = cdp->highlightColor.getValue();
			glColor3fv(theColor.getValue());
		}
		else
		{
			colorIndex = chemData->residueChainIndex[chemData->residues[theResidue].getChain()];
			glColor3fv(chemColor->residueColor[colorIndex].getValue());
		}

		residueUnitCylinder->render(
			chemData->getAtomCoordinates(res->from).getValue(), 
			chemData->getAtomCoordinates(res->to).getValue(), rad,
			lastResidueCylinderDoNormals, lastResidueCylinderDoTextures, displayStyle,fromTo,this,action);

	RESIDUELOOP_END

	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
	{
		glMaterialfv(GL_FRONT, GL_EMISSION,
			SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	glDisable(GL_COLOR_MATERIAL);
}

#if 000
// GDB 2/1/12 Unused global function
static bool aabbInsideFrustum(const SbVec3f& center, const float halfsize,const SbPlane* planes)
{
	int i,q;
	SbVec3f vmin,vmax;
	float distance;

	for (i=0;i<6;i++)
	{
		for(q=0;q<=2;q++)
		{
			if(planes[i].getNormal()[q]>=0.0f)
			{
				vmin[q]=-halfsize;
				vmax[q]=halfsize;
			}
			else
			{
				vmin[q]=halfsize;
				vmax[q]=-halfsize;
			}
		}
		distance = POINT_PLANE_DIST(planes[i],center+vmin);
		if(distance>=0.0f) //full inside
			continue;

		distance = POINT_PLANE_DIST(planes[i],center+vmax);
		if(distance<0.0f) // full outside
			return false;
	}
	
	return true;
}
#endif

void ChemDisplay::computeRibbonLOD(const int index, const SbPlane* planes)
{
	ChemResidueStuff* res = &residueAttributes[index];

	if (res->isRibbonRenderable && res->overlapAABBFrustum(planes))
	{
		const SbVec3f &d = currentCameraPosition - res->center;
		float len = d.dot(d);

		if (len < 1000) res->segments = 10;
		else if (len < 3000) res->segments = 7;
		else if (len < 15000) res->segments = 4;
		else if (len < 80000) res->segments = 3;
		else res->segments = 2;

	//	printf("len %f segments %d\n",len,res->segments);
	}
	else
	{
		res->segments = 0;
		//printf("segments %d\n",0,res->segments);
	}

}

void ChemDisplay::computeRibbonCulling()
{
	SbPlane* planes = new SbPlane[6]();
	ChemOctreeNode::computeFrustumPlanes(planes);

	RESIDUELOOP_START(vnormalResidueIndex);
		computeRibbonLOD(theResidue,planes);
	RESIDUELOOP_END

	RESIDUELOOP_START(vhighlightResidueIndex);
		computeRibbonLOD(theResidue,planes);
	RESIDUELOOP_END

	delete [] planes;
}

void ChemDisplay::computeStickCulling(const MFVec2i &mf,ChemDisplayParam* cdp, ChemData* chemData)
{
	int lod;
	float rad = cdp->residueCylinderRadius.getValue();
	lodSelector->resetResidues(globalNumberOfResidues);
	ChemResidueStuff* res = 0;

	RESIDUELOOP_START(mf);

		RESDUP(theResidue);

		res = &residueAttributes[theResidue];
		assert(res != 0);
		if (!res->isStickRenderable) continue;

		lod = computeCylinderLOD(
			chemData->getAtomCoordinates(res->from),
			chemData->getAtomCoordinates(res->to),
			rad);

		// assign lod
		lodSelector->addResidueIndex(lod,theResidue);

	RESIDUELOOP_END
}

void ChemDisplay::renderResiduesAsFlatRibbons(SoGLRenderAction *action)
{
#ifdef DEBUG
printf ("In ChemDisplay::renderResiduesAsFlatRibbons\n");
#endif

	// Check to see if there's anything to render.
	if (residueIndex.getNum() == 0 ||  globalNumberOfResidues == 0) return;

	SoState *state;
	ChemColor *chemColor;
	ChemDisplayParam *cdp;
	ChemBaseData *chemBaseData;
	ChemData *chemData;

	setupResidues(action, state, chemColor, cdp, chemBaseData);

	chemData = (ChemData*)chemBaseData;

#ifndef IV2_0
	int32_t myMask;
	if (cdp->highlightStyle.getValue() == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
	{
		myMask = SoLazyElement::DIFFUSE_MASK | SoLazyElement::EMISSIVE_MASK;
	}
	else
	{
		myMask = SoLazyElement::DIFFUSE_MASK;
	}

	SoGLLazyElement *lazyElt = (SoGLLazyElement *)
			SoLazyElement::getInstance(state);
	lazyElt->send(state, SoLazyElement::ALL_MASK &( ~myMask));
#endif

	GLboolean bLighting = false;
	glGetBooleanv(GL_LIGHTING,&bLighting);
	if (!bLighting) glEnable(GL_LIGHTING);
	
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_TRUE);

	computeRibbonCulling();//compute individual culling

/*
	//>> debuggin residues
	glDisable(GL_LIGHTING);
	RESIDUELOOP_START(vhighlightResidueIndex)

	ChemResidueStuff* res = &residueAttributes[theResidue];
	assert(res != NULL);
	if (!res->isRibbonRenderable) continue;

	res->renderAABB();

	RESIDUELOOP_END
	glEnable(GL_LIGHTING);
	//<< debuggin residues
*/
	switch (chemColor->residueColorBinding.getValue()) 
	{
		case ChemColor::RESIDUE_OVERALL:
		{
			normalFlatRibbonOV(vnormalResidueIndex, state, chemColor->residueColor[0],
				cdp, chemData);
			highlightFlatRibbonOV(vhighlightResidueIndex, state, chemColor->residueColor[0],
				cdp, chemData);
		}
		break;

		case ChemColor::RESIDUE_PER_INDEX:
		{
			normalFlatRibbonRPN(vnormalResidueIndex, state, chemColor,
				cdp, chemData);
			highlightFlatRibbonRPN(vhighlightResidueIndex, state, chemColor,
				cdp, chemData);
		}
		break;

		case ChemColor::RESIDUE_PER_CHAIN:
		{
			normalFlatRibbonRPC(vnormalResidueIndex, state, chemColor,
				cdp, chemData);
			highlightFlatRibbonRPC(vhighlightResidueIndex, state, chemColor,
				cdp, chemData);
		}
		break;

		default: break;
	}
	
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_FALSE);

	if (!bLighting) glDisable(GL_LIGHTING);

#ifndef IV2_0
	lazyElt->reset(state, myMask);
#endif
}

void ChemDisplay::normalFlatRibbonOV(const MFVec2i &theResidueIndex,
																	SoState *state,
																	const SbColor &chemColor,
																	const ChemDisplayParam *cdp,
																	const ChemData *chemData)
{
	// Check to see if there's anything to render.
	if (theResidueIndex.getNum() == 0) return;

	//SbVec3f 	coords1, coords2;

	glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);	
	
	glColor3fv(chemColor.getValue());

	ChemResidueStuff *res = NULL;
	RESIDUELOOP_START(theResidueIndex)

		RESDUP(theResidue);

		res = &residueAttributes[theResidue];
		assert(res != NULL);
		if (!res->isRibbonRenderable) continue;
		if (res->segments==0) continue;
		
		computeFlatRibbonGeometry(theResidue);
		computeResidueColor(theResidue,state);	
		renderFlatRibbon();

	RESIDUELOOP_END

	glDisable(GL_COLOR_MATERIAL);
}

void ChemDisplay::normalFlatRibbonRPN(const MFVec2i &theResidueIndex,
																	SoState *state,
																	const ChemColor *chemColor,
																	const ChemDisplayParam *cdp,
																	const ChemData *chemData)
{
	// Check to see if there's anything to render.
	if (theResidueIndex.getNum() == 0) return;

	int32_t colorIndex;
	//SbVec3f 	coords1, coords2;

	glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

	ChemResidueStuff *res = NULL;
	RESIDUELOOP_START(theResidueIndex)

		RESDUP(theResidue);

		res = &residueAttributes[theResidue];
		assert(res != NULL);
		if (!res->isRibbonRenderable) continue;
		if (res->segments==0) continue;
		
		if (chemData->residues[theResidue].getIndex() < 0) colorIndex = 0;
		else colorIndex = chemData->residueColorIndex[chemData->residues[theResidue].getIndex()];
		glColor3fv(chemColor->residueColor[colorIndex].getValue());

		computeFlatRibbonGeometry(theResidue);
		computeResidueColor(theResidue,state);	
		renderFlatRibbon();

	RESIDUELOOP_END

	glDisable(GL_COLOR_MATERIAL);
}

void ChemDisplay::normalFlatRibbonRPC(const MFVec2i &theResidueIndex,
																	SoState *state,
																	const ChemColor *chemColor,
																	const ChemDisplayParam *cdp,
																	const ChemData *chemData)
{
	// Check to see if there's anything to render.
	if (theResidueIndex.getNum() == 0) return;

	int32_t colorIndex;
	//SbVec3f 	coords1, coords2;

	glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

	ChemResidueStuff *res = NULL;
	RESIDUELOOP_START(theResidueIndex)

		RESDUP(theResidue);

		res = &residueAttributes[theResidue];
		assert(res != NULL);
		if (!res->isRibbonRenderable) continue;
		if (res->segments==0) continue;
		
		colorIndex = chemData->residueChainIndex[chemData->residues[theResidue].getChain()];
		glColor3fv(chemColor->residueColor[colorIndex].getValue());

		computeFlatRibbonGeometry(theResidue);
		computeResidueColor(theResidue,state);
		renderFlatRibbon();

	RESIDUELOOP_END

	glDisable(GL_COLOR_MATERIAL);
}

void ChemDisplay::highlightFlatRibbonOV(const MFVec2i &theResidueIndex,
																	SoState *state,
																	const SbColor &chemColor,
																	const ChemDisplayParam *cdp,
																	const ChemData *chemData)
{
	// Check to see if there's anything to render.
	if (theResidueIndex.getNum() == 0) return;

	//SbVec3f 	coords1, coords2;

	glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

	int32_t highlightStyle = cdp->highlightStyle.getValue();
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
	{
		const SbColor &theColor = cdp->highlightColor.getValue();
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,
			SbVec4f(theColor[0], theColor[1], theColor[2], 1.0).getValue());

		glColor3fv(chemColor.getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE)
	{
		const SbColor &theColor = cdp->highlightColor.getValue();
		glColor3fv(theColor.getValue());
	}

	ChemResidueStuff *res = NULL;
	//SoGLRenderAction *action = (SoGLRenderAction *)state->getAction();

	RESIDUELOOP_START(theResidueIndex)

		RESDUP(theResidue);

		res = &residueAttributes[theResidue];
		assert(res != NULL);
		if (!res->isRibbonRenderable) continue;
		if (res->segments==0) continue;

		computeFlatRibbonGeometry(theResidue);
		computeResidueColor(theResidue,state);	
		renderFlatRibbon();

	RESIDUELOOP_END

	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
	{
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,
			SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	
	glDisable(GL_COLOR_MATERIAL);
}

void ChemDisplay::highlightFlatRibbonRPN(const MFVec2i &theResidueIndex,
																	SoState *state,
																	const ChemColor *chemColor,
																	const ChemDisplayParam *cdp,
																	const ChemData *chemData)
{
	// Check to see if there's anything to render.
	if (theResidueIndex.getNum() == 0) return;

	int highlightStyle = cdp->highlightStyle.getValue();
	const SbColor &highlightColor = cdp->highlightColor.getValue();
	int32_t colorIndex;

	glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
	{
			glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,
				SbVec4f(highlightColor[0], highlightColor[1],
						highlightColor[2], 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
	{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE)
	{
		glColor3fv(highlightColor.getValue());
		ChemResidueStuff *res = NULL;
		RESIDUELOOP_START(theResidueIndex)

			RESDUP(theResidue);

			res = &residueAttributes[theResidue];
			assert(res != NULL);
			if (!res->isRibbonRenderable) continue;
			if (res->segments==0) continue;

			computeFlatRibbonGeometry(theResidue);
			computeResidueColor(theResidue,state);	
			renderFlatRibbon();

		RESIDUELOOP_END
	}
	else
	{
		ChemResidueStuff *res = NULL;
		RESIDUELOOP_START(theResidueIndex)

			RESDUP(theResidue);

			res = &residueAttributes[theResidue];
			assert(res != NULL);
			if (!res->isRibbonRenderable) continue;
			if (res->segments==0) continue;
			
			if (chemData->residues[theResidue].getIndex() < 0) colorIndex = 0;
			else colorIndex = chemData->residueColorIndex[chemData->residues[theResidue].getIndex()];
			glColor3fv(chemColor->residueColor[colorIndex].getValue());

			computeFlatRibbonGeometry(theResidue);
			computeResidueColor(theResidue,state);	
			renderFlatRibbon();

		RESIDUELOOP_END
	}

	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
	{
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,
			SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	
	glDisable(GL_COLOR_MATERIAL);
}

void ChemDisplay::highlightFlatRibbonRPC(const MFVec2i &theResidueIndex,
																	SoState *state,
																	const ChemColor *chemColor,
																	const ChemDisplayParam *cdp,
																	const ChemData *chemData)
{
	// Check to see if there's anything to render.
	if (theResidueIndex.getNum() == 0) return;

	int highlightStyle = cdp->highlightStyle.getValue();
	const SbColor &highlightColor = cdp->highlightColor.getValue();
	int32_t colorIndex;

	glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
	{
			glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,
				SbVec4f(highlightColor[0], highlightColor[1],
						highlightColor[2], 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
	{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE)
	{
		glColor3fv(highlightColor.getValue());
		ChemResidueStuff *res = NULL;
		RESIDUELOOP_START(theResidueIndex)

			RESDUP(theResidue);

			res = &residueAttributes[theResidue];
			assert(res != NULL);
			if (!res->isRibbonRenderable) continue;
			if (res->segments==0) continue;

			computeFlatRibbonGeometry(theResidue);
			computeResidueColor(theResidue,state);	
			renderFlatRibbon();

		RESIDUELOOP_END
	}
	else
	{
		ChemResidueStuff *res = NULL;
		RESIDUELOOP_START(theResidueIndex)

			RESDUP(theResidue);

			res = &residueAttributes[theResidue];
			assert(res != NULL);
			if (!res->isRibbonRenderable) continue;
			if (res->segments==0) continue;
			
			colorIndex = chemData->residueChainIndex[chemData->residues[theResidue].getChain()];
			glColor3fv(chemColor->residueColor[colorIndex].getValue());

			computeFlatRibbonGeometry(theResidue);
			computeResidueColor(theResidue,state);	
			renderFlatRibbon();

		RESIDUELOOP_END
	}
	
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
	{
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,
			SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	
	glDisable(GL_COLOR_MATERIAL);
}

void ChemDisplay::renderResiduesAsLineRibbons(SoGLRenderAction *action)
{
#ifdef DEBUG
printf ("In ChemDisplay::renderResiduesAsLineRibbons\n");
#endif

	// Check to see if there's anything to render.
	if (residueIndex.getNum() == 0 ||  globalNumberOfResidues == 0) return;
	
	SoState *state     = action->getState();
	ChemColor* chemColor = ChemColorElement::get(state);
	ChemDisplayParam *cdp       = ChemDisplayParamElement::get(state);
	ChemData *chemData  = (ChemData*)ChemBaseDataElement::get(state);
	ChemBaseData* chemBaseData = NULL;


	setupResidues(action, state, chemColor, cdp, chemBaseData);
	chemData = (ChemData*)chemBaseData;
	

	if ((normalResidueIndex.getNum() > 0) || (highlightResidueIndex.getNum() > 0)) 
	{

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
			(cdp->residueWireframeAntiAlias.getValue() !=
				ChemDisplayParam::WIREFRAME_ANTIALIAS_NONE))
		{
			glPushAttrib(GL_COLOR_BUFFER_BIT |  // for GL_BLEND
				     GL_DEPTH_BUFFER_BIT |  // for GL_DEPTH_TEST
				     GL_LINE_BIT         |  // for GL_LINE_SMOOTH
				     GL_HINT_BIT);          // for GL_LINE_SMOOTH_HINT
			glEnable( GL_LINE_SMOOTH );
			glHint  ( GL_LINE_SMOOTH_HINT, GL_NICEST );
			glEnable( GL_BLEND );
			if (cdp->residueWireframeAntiAlias.getValue() ==
				ChemDisplayParam::WIREFRAME_ANTIALIAS_WITH_DEPTH_COMPARISON)
			{
				glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
				glEnable( GL_DEPTH_TEST );
			}
			else
			{
				glBlendFunc( GL_SRC_ALPHA, GL_ONE );
				glDisable( GL_DEPTH_TEST );
			}
		}

		computeRibbonCulling();//compute individual culling

		normalLineRibbon(vnormalResidueIndex, state, chemColor,
			cdp, chemData);
		highlightLineRibbon(vhighlightResidueIndex, state, chemColor,
			cdp, chemData);
		
		if (!renderAsPoints &&
			(cdp->bondWireframeAntiAlias.getValue() !=
		ChemDisplayParam::WIREFRAME_ANTIALIAS_NONE))
		{
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

void ChemDisplay::normalLineRibbon(const MFVec2i &theResidueIndex,
																	SoState *state,
																	const ChemColor *chemColor,
																	const ChemDisplayParam *cdp,
																	const ChemData *chemData)
{
	// Check to see if there's anything to render.
	if (theResidueIndex.getNum() == 0) return;

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

	ChemResidueStuff *res = NULL;
	int32_t colorIndex;
	
	glLineWidth(cdp->residueWireframeLineWidth.getValue());

	switch (chemColor->residueColorBinding.getValue())
	{
		case ChemColor::RESIDUE_OVERALL:
		{
			glColor3fv(chemColor->residueColor[0].getValue());
			RESIDUELOOP_START(theResidueIndex);
	
				RESDUP(theResidue);

				res = &residueAttributes[theResidue];
				assert(res != NULL);
				if (!res->isRibbonRenderable) continue;
				if (res->segments==0) continue;

				computeLineRibbonGeometry(theResidue);
				computeResidueColor(theResidue,state);
				renderLineRibbon();

			RESIDUELOOP_END
			break;
		}

		case ChemColor::RESIDUE_PER_INDEX:
		{
			RESIDUELOOP_START(theResidueIndex);
			
				RESDUP(theResidue);

				res = &residueAttributes[theResidue];
				assert(res != NULL);
				if (!res->isRibbonRenderable) continue;
				if (res->segments==0) continue;

				if (chemData->residues[theResidue].getIndex() < 0) colorIndex = 0;
				else colorIndex = chemData->residueColorIndex[chemData->residues[theResidue].getIndex()];
				glColor3fv(chemColor->residueColor[colorIndex].getValue());

				computeLineRibbonGeometry(theResidue);
				computeResidueColor(theResidue,state);	
				renderLineRibbon();

			RESIDUELOOP_END
			break;
		}

		case ChemColor::RESIDUE_PER_CHAIN:
		{
			RESIDUELOOP_START(theResidueIndex);
			
				RESDUP(theResidue);

				res = &residueAttributes[theResidue];
				assert(res != NULL);
				if (!res->isRibbonRenderable) continue;
				if (res->segments==0) continue;

				colorIndex = chemData->residueChainIndex[chemData->residues[theResidue].getChain()];
				glColor3fv(chemColor->residueColor[colorIndex].getValue());

				computeLineRibbonGeometry(theResidue);
				computeResidueColor(theResidue,state);	
				renderLineRibbon();

			RESIDUELOOP_END
			break;
		}
		default:
			break;
	}

	glDisable(GL_COLOR_MATERIAL);
}

void ChemDisplay::highlightLineRibbon(const MFVec2i &theResidueIndex,
																	SoState *state,
																	const ChemColor *chemColor,
																	const ChemDisplayParam *cdp,
																	const ChemData *chemData)
{
	// Check to see if there's anything to render.
	if (theResidueIndex.getNum() == 0) return;

	SbColor highlightColor = cdp->highlightColor.getValue();
	int32_t highlightStyle = cdp->highlightStyle.getValue();
	int32_t colorIndex;
	SbColor tmpColor;


	ChemResidueStuff *res = NULL;
	glLineWidth(cdp->residueWireframeLineWidth.getValue());

	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE)
	{
		glColor3fv(highlightColor.getValue());
		RESIDUELOOP_START(theResidueIndex)
			
			RESDUP(theResidue);

			res = &residueAttributes[theResidue];
			assert(res != NULL);
			if (!res->isRibbonRenderable) continue;
			if (res->segments==0) continue;

			computeLineRibbonGeometry(theResidue);
			computeResidueColor(theResidue,state);	
			renderLineRibbon(true);

		RESIDUELOOP_END
	}
	else //if (highlightStyle != ChemDisplayParam::HIGHLIGHT_DIFFUSE)
	{
		switch (chemColor->residueColorBinding.getValue()) 
		{
			case ChemColor::RESIDUE_OVERALL:
			{
				// All residues are the same color which is the first color in the
				// residues color table.
				const SbColor *residueColors = chemColor->residueColor.getValues(0);
				glColor3fv(residueColors[0].getValue());

				if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
				{
					glColor3fv(residueColors[0].getValue());
				}
				else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
				{
					tmpColor = residueColors[0] + highlightColor;
					tmpColor.normalize();
					glColor3fv(tmpColor.getValue());
				}
	
				ChemResidueStuff *res = NULL;
				RESIDUELOOP_START(theResidueIndex)
					
					RESDUP(theResidue);

					res = &residueAttributes[theResidue];
					assert(res != NULL);
					if (!res->isRibbonRenderable) continue;
					if (res->segments==0) continue;

					computeLineRibbonGeometry(theResidue);
					computeResidueColor(theResidue,state);	
					renderLineRibbon(true);
					
				RESIDUELOOP_END
			}
			break;

			case ChemColor::RESIDUE_PER_INDEX:
			{
				RESIDUELOOP_START(theResidueIndex)
					
					RESDUP(theResidue);

					res = &residueAttributes[theResidue];
					assert(res != NULL);
					if (!res->isRibbonRenderable) continue;
					if (res->segments==0) continue;

					if (chemData->residues[theResidue].getIndex() < 0) colorIndex = 0;
					else colorIndex = chemData->residueColorIndex[chemData->residues[theResidue].getIndex()];
				
					if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
					{
						glColor3fv(chemColor->residueColor[colorIndex].getValue());
					}
					else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
					{
						tmpColor = chemColor->residueColor[colorIndex] + highlightColor;
						tmpColor.normalize();
						glColor3fv(tmpColor.getValue());
					}	

					computeLineRibbonGeometry(theResidue);
					computeResidueColor(theResidue,state);	
					renderLineRibbon(true);
					
				RESIDUELOOP_END
			}
			break;

			case ChemColor::RESIDUE_PER_CHAIN:
			{
				RESIDUELOOP_START(theResidueIndex)
					
					RESDUP(theResidue);

					res = &residueAttributes[theResidue];
					assert(res != NULL);
					if (!res->isRibbonRenderable) continue;
					if (res->segments==0) continue;

					colorIndex = chemData->residueChainIndex[chemData->residues[theResidue].getChain()];
				
					if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
					{
						glColor3fv(chemColor->residueColor[colorIndex].getValue());
					}
					else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
					{
						tmpColor = chemColor->residueColor[colorIndex] + highlightColor;
						tmpColor.normalize();
						glColor3fv(tmpColor.getValue());
					}

					computeLineRibbonGeometry(theResidue);
					computeResidueColor(theResidue,state);	
					renderLineRibbon(true);
					
				RESIDUELOOP_END
			}
			break;

			default: break;
		}
	}
}

// --> solid ribbon
double ChemDisplay::B(const int i, const int k, const float u, const float* knots)
{
	if (k==1)
	{
		if (knots[i] <= u && u < knots[i+1])
			return 1;
		else
			return 0;
	}

	double a,b,a1,a2,b1,b2;
	a1 = u-knots[i];
	a2 = knots[i+k-1]-knots[i];

	b1 = knots[i+k]-u;
	b2 = knots[i+k]-knots[i+1];
	
	if (a2==0 ||a1 == 0) a =0;
	else
		a= (a1/a2) * B(i, k-1, u,knots);
	
	if (b2==0 || b1 == 0) b =0;
	else
		b = (b1/b2) * B(i+1, k-1, u,knots);

	return (a+b);
		
}

void ChemDisplay::computeBoundsForSolidRibbon(SoAction *action)
{
	ChemDisplayParam* cdp = ChemDisplayParamElement::get(action->getState());
	float scaleFactor = cdp->residueHeigthFactor.getValue();

	SbVec3f left,frontl,frontr,upl,upr;
	
	int i=0, nextIndex;
	for (i=0;i<pl.getNum();i++)
	{
		if (pIndex[i] < 0) // if index i doesn't contain a valid ribbon point
			continue;

		left = pr[i] - pl[i];
		left.normalize();

		if (i<pl.getNum()-1)
		{
			// find next valid index
			nextIndex = i+1;
			while (pIndex[nextIndex] < 0 && nextIndex < pl.getNum()) 
				nextIndex++;

			if (nextIndex == pl.getNum()) // not found
			{
				nextIndex = i-1;
				while (pIndex[nextIndex] < 0 && nextIndex >=0) 
					nextIndex--;
					
				if (nextIndex <0) // not found
				{
					nextIndex = i; //fix result
				}

				frontl = pl[i] - pl[nextIndex];
				frontr = pr[i] - pr[nextIndex];
			}
			else
			{
				frontl = pl[nextIndex] - pl[i];
				frontr = pr[nextIndex] - pr[i];
			}
			
			frontl.normalize();
			frontr.normalize();
			upl = frontl.cross(left);
			upr = frontr.cross(left);

			upl = scaleFactor * upl;
			upr = scaleFactor * upr;
		}
		else // last residue
		{
			upl = frontl.cross(left);
			upr = frontr.cross(left);

			upl = scaleFactor * upl;
			upr = scaleFactor * upr;
		}

		tl.set1Value(i,pl[i] + upl);
		tr.set1Value(i,pr[i] + upr);
		bl.set1Value(i,pl[i] - upl);
		br.set1Value(i,pr[i] - upr);
	}
}

void ChemDisplay::computeBoundsForSchematic(SoAction *action)
{
	ChemDisplayParam* cdp = ChemDisplayParamElement::get(action->getState());
	float scaleFactor = cdp->residueHeigthFactor.getValue();

	SbVec3f left,frontl,frontr,upl,upr;

	int i=0, nextIndex;
	for (i=0;i<pl.getNum();i++)
	{
		if (pIndex[i] < 0) // if index i doesn't contain a valid ribbon point
			continue;

		left = scpr[i] - scpl[i];

		if (i!=scpl.getNum()-1)
		{
			// find next valid index
			nextIndex = i+1;
			while (pIndex[nextIndex] < 0 && nextIndex < pl.getNum()) nextIndex++;
			if (nextIndex == pl.getNum()) nextIndex = i;//fix result

			frontl = scpl[nextIndex]  - scpl[i];
			frontr = scpr[nextIndex]  - scpr[i];
			left.normalize();
			frontl.normalize();
			frontr.normalize();
			upl = frontl.cross(left);
			upr = frontr.cross(left);
			upl.normalize();
			upr.normalize();

			upl = scaleFactor * upl;
			upr = scaleFactor * upr;
		}

		sctl.set1Value(i,scpl[i] + upl);
		sctr.set1Value(i,scpr[i] + upr);
		scbl.set1Value(i,scpl[i] - upl);
		scbr.set1Value(i,scpr[i] - upr);
	}
}
#define NCTLPTS 4
#define DEGREE 3
#define ORDER DEGREE+1
#define KNOTS DEGREE+NCTLPTS+1
#define TMIN ORDER-1
#define TMAX (NCTLPTS-1)+2-1
const float knots[] = {0,1,2,3,4,5,6,7};

void ChemDisplay::computeBasisCoefficients(const float t)
{
	for (int i=0;i<NCTLPTS;i++)
		basisCoefficients.set1Value(i, (float)B(i,ORDER,t,(const float*)knots));
}

void ChemDisplay::preComputeBasisCoefficients()
{
	int i,j,k;
	float t,s;
	float knots[]={0,1,2,3,4,5,6,7};

	for (i=2;i<=10;i++) // for each level of detail (from 2 segments to 10 segments)
		basisCoefficientsLOD[i-2].setNum(4*i);
	
	t = knots[TMIN]; // ORDER-1 = (DEGREE+1)-1 = (3+1)-1

	for (i=2;i<=10;i++) // for level of detail i (i==i segments for ribbon)
	{
		s = (float)((knots[TMAX]-knots[TMIN])/(i-1)-0.000001);

		for (j=0;j<i;j++) // each segment has i*NCTLPTS=i*4 points
		for (k=0;k<NCTLPTS;k++)
			basisCoefficientsLOD[i-2].set1Value(j*4+k, (float)B(k,ORDER,t+j*s,knots));
	}
}

void ChemDisplay::computeResidueColor(int residue,SoState* state)
{
	ChemColor* cc = ChemColorElement::get(state);
	ChemData* cd = (ChemData*)ChemBaseDataElement::get(state);

	int i,j,k;
	i = residue - 1;
	j = residue;
	k = residue + 1;

	if (i<0) i=0;
	if (k==globalNumberOfResidues) k--;

	switch (cc->residueColorBinding.getValue())
	{
		case ChemColor::RESIDUE_OVERALL: // DO NOTHING
		{
			residueInterpolateColor = false;
		}
		break;

		case ChemColor::RESIDUE_PER_INDEX:
		{
			if (cd->residues[i].getIndex() < 0) residueColor[0] = cc->residueColor[0];
			else residueColor[0] = cc->residueColor[cd->residueColorIndex[cd->residues[i].getIndex()]];
			
      if (cd->residues[j].getIndex() < 0) residueColor[1] = cc->residueColor[0];
			else residueColor[1] = cc->residueColor[cd->residueColorIndex[cd->residues[j].getIndex()]];
			
      if (cd->residues[k].getIndex() < 0) residueColor[2] = cc->residueColor[0];
			else residueColor[2] = cc->residueColor[cd->residueColorIndex[cd->residues[k].getIndex()]];

      residueColor[0] = residueColor[0] * 0.5f + residueColor[1] * 0.5f;
      residueColor[2] = residueColor[1] * 0.5f + residueColor[2] * 0.5f;
		}
		break;
		
		case ChemColor::RESIDUE_PER_CHAIN:
		{
			residueColor[0] = 
				cc->residueColor[cd->residueChainIndex[cd->residues[i].getChain()]] * 0.50f +
				cc->residueColor[cd->residueChainIndex[cd->residues[j].getChain()]] * 0.50f;
			
			residueColor[1] = 
				cc->residueColor[cd->residueChainIndex[cd->residues[j].getChain()]];
				
			
			residueColor[2] = 
				cc->residueColor[cd->residueChainIndex[cd->residues[j].getChain()]] * 0.50f +
				cc->residueColor[cd->residueChainIndex[cd->residues[k].getChain()]] * 0.50f;
		}
		break;

		default: break;

	}

}


void ChemDisplay::computeLineRibbonGeometry(int residue)
{
	//ribbonSegmentation = lineRibbonTesselationSegments.getValue();
	//if (currentRibbonHelix) ribbonSegmentation = (int)(ribbonSegmentation*ribbonHelixTesselationFactor.getValue());
	int numberOfThreads = residueNumberOfThreads.getValue();
	
	ChemResidueStuff *res = NULL;	
	res = &residueAttributes[residue];
	ribbonSegmentation = res->segments;
	assert(ribbonSegmentation>=2 && ribbonSegmentation<=10);

	const SoMFFloat &basisCoefficients = (basisCoefficientsLOD[ribbonSegmentation-2]);
	int offset;

	SbVec3f A,B,C,dir;
	int i,j,k;

	for (i=0;i<ribbonSegmentation;i++)
	{
		A.setValue(0.0f,0.0f,0.0f);
		B.setValue(0.0f,0.0f,0.0f);
		for (j=0;j<NCTLPTS;j++)
		{
			 const SbVec3f &a = pl[res->indices[j]];
			 const SbVec3f &b = pr[res->indices[j]];

			 offset = i*NCTLPTS+j;

			 A += (a * basisCoefficients[offset]);
			 B += (b * basisCoefficients[offset]);
		}
		
		dir = B-A;
		for (k=0;k<numberOfThreads;k++)
		{
			C = A + (k/((float)numberOfThreads-1))*dir;
			(lr+k)->set1Value(i,C);
		}
	}
}

void ChemDisplay::renderLineRibbon(bool highlight)
{
	int i,j;
	if (!highlight && residueInterpolateColor)
	{
		for (i=0;i<residueNumberOfThreads.getValue();i++)
		{
			glBegin(GL_LINE_STRIP);
			
				glColor3fv(residueColor[0].getValue());		
				glVertex3fv(lr[i][0].getValue());
			
				glColor3fv(residueColor[1].getValue());
				for (j=1;j<ribbonSegmentation-1;j++)
				{
					glVertex3fv(lr[i][j].getValue());
				}

				glColor3fv(residueColor[2].getValue());
				glVertex3fv(lr[i][j].getValue());

			glEnd();
		}			
	}
	else
	{
		for (i=0;i<residueNumberOfThreads.getValue();i++)
		{
			glBegin(GL_LINE_STRIP);
			for (j=0;j<ribbonSegmentation;j++)
			{
				glVertex3fv(lr[i][j].getValue());
			}
			glEnd();
		}
	}
}

void ChemDisplay::computeFlatRibbonGeometry(int residue, bool computeNormals /*= true*/)
{
	//ribbonSegmentation = flatRibbonTesselationSegments.getValue();
	//if (currentRibbonHelix) ribbonSegmentation = (int)(ribbonSegmentation * ribbonHelixTesselationFactor.getValue());

	ChemResidueStuff *res = NULL;	
	res = &residueAttributes[residue];
	ribbonSegmentation = res->segments;
	assert(ribbonSegmentation>=2 && ribbonSegmentation<=10);
	
	const SoMFFloat &basisCoefficients = (basisCoefficientsLOD[ribbonSegmentation-2]);
	int offset;

	SbVec3f normal, A,B,C,D;
	int i,j;
	for (i=0;i<ribbonSegmentation;i++)
	{
		A.setValue(0.0f,0.0f,0.0f);
		B.setValue(0.0f,0.0f,0.0f);
		C.setValue(0.0f,0.0f,0.0f);
		D.setValue(0.0f,0.0f,0.0f);

		for (j=0;j<NCTLPTS;j++)
		{
			 const SbVec3f &a = tl[res->indices[j]];
			 const SbVec3f &b = tr[res->indices[j]];
			 const SbVec3f &c = pl[res->indices[j]];
			 const SbVec3f &d = pr[res->indices[j]];

			 offset = i*NCTLPTS+j;
			 
			 A += (a * basisCoefficients[offset]);
			 B += (b * basisCoefficients[offset]);
			 C += (c * basisCoefficients[offset]);
			 D += (d * basisCoefficients[offset]);
		}
		fl.set1Value(i,C);
		fr.set1Value(i,D);

		if (computeNormals)
		{
			normal = A-C; normal.normalize();
			ful.set1Value(i,normal);
			fur.set1Value(i,normal);		
		}
	}
}

void ChemDisplay::renderFlatRibbon()
{
	int i = 0;
	if (residueInterpolateColor)
	{
		glBegin(GL_TRIANGLE_STRIP);
		
		glColor3fv(residueColor[0].getValue());

		glNormal3fv(fur[0].getValue());
		glVertex3fv(fr[0].getValue());

		glNormal3fv(ful[0].getValue()); 
		glVertex3fv(fl[0].getValue());
		
		glColor3fv(residueColor[1].getValue());
		for (i=1;i<ribbonSegmentation-1;i++)
		{
			glNormal3fv(fur[i].getValue());
			glVertex3fv(fr[i].getValue());

			glNormal3fv(ful[i].getValue());
			glVertex3fv(fl[i].getValue());
		}

		glColor3fv(residueColor[2].getValue());
		
		glNormal3fv(fur[i].getValue());
		glVertex3fv(fr[i].getValue());

		glNormal3fv(ful[i].getValue());
		glVertex3fv(fl[i].getValue());

		glEnd();
		
	}
	else
	{
		glBegin(GL_TRIANGLE_STRIP);
		for (i=0;i<ribbonSegmentation;i++)
		{
			glNormal3fv(fur[i].getValue());
			glVertex3fv(fr[i].getValue());

			glNormal3fv(ful[i].getValue());
			glVertex3fv(fl[i].getValue());
		}
		glEnd();
	}
}

void ChemDisplay::computeSolidRibbonGeometry(int residue, bool computeNormals /*=true*/)
{
	//ribbonSegmentation = solidRibbonTesselationSegments.getValue();
	//if (currentRibbonHelix) ribbonSegmentation = (int)(ribbonSegmentation * ribbonHelixTesselationFactor.getValue());

	ChemResidueStuff *res = NULL;	
	res = &residueAttributes[residue];
	ribbonSegmentation = res->segments;
	assert(ribbonSegmentation>=2 && ribbonSegmentation<=10);

	const SoMFFloat &basisCoefficients = (basisCoefficientsLOD[ribbonSegmentation-2]);
	int offset;

	SbVec3f normal,A,B,C,D;
	int i,j;
	for (i=0;i<ribbonSegmentation;i++)
	{
		A.setValue(0.0f,0.0f,0.0f);
		B.setValue(0.0f,0.0f,0.0f);
		C.setValue(0.0f,0.0f,0.0f);
		D.setValue(0.0f,0.0f,0.0f);
		for (j=0;j<NCTLPTS;j++)
		{
			 const SbVec3f& a = *tl.getValues(res->indices[j]);
			 const SbVec3f& b = *tr.getValues(res->indices[j]);
			 const SbVec3f& c = *bl.getValues(res->indices[j]);
			 const SbVec3f& d = *br.getValues(res->indices[j]);

			 offset = i*NCTLPTS+j;

			 A += (a * basisCoefficients[offset]);
			 B += (b * basisCoefficients[offset]);
			 C += (c * basisCoefficients[offset]);
			 D += (d * basisCoefficients[offset]);
		}
		rtl.set1Value(i,A);
		rtr.set1Value(i,B);
		rbl.set1Value(i,C);
		rbr.set1Value(i,D);

		if (computeNormals) // accel selection
		{
			SbVec3f up = A - C; up.normalize();
			SbVec3f left = A - B; left.normalize();				

			if (solidRibbonSmoothNormals.getValue())
			{
				normal = up + left; normal.normalize();
				runtl.set1Value(i,normal); 
				rlntl.set1Value(i,normal);

				normal = -normal;
				runbr.set1Value(i,normal);
				rlnbr.set1Value(i,normal);
				
				normal = -up + left; normal.normalize();
				runbl.set1Value(i,normal);
				rlnbl.set1Value(i,normal);

				normal = -normal;
				runtr.set1Value(i,normal);
				rlntr.set1Value(i,normal);
			}
			else
			{	
				runtl.set1Value(i,up); 
				runtr.set1Value(i,up);		

				up = -up;
				runbl.set1Value(i,up);
				runbr.set1Value(i,up);

				rlntl.set1Value(i,left);
				rlnbl.set1Value(i,left);

				left = -left;
				rlntr.set1Value(i,left);
				rlnbr.set1Value(i,left);
				
			}
		} // if (computeNormals)
	}
}

void ChemDisplay::renderSolidRibbon()
{
#ifdef DEBUG_NORMALS
	int i;
	glDisable(GL_LIGHTING);
	glColor3f(1,1,1);
	glBegin(GL_LINES);
	
	for (i=0;i<ribbonSegmentation;i++)
	{
		glVertex3fv(rtl[i].getValue()); 
		glVertex3fv((rtl[i] + runtl[i]).getValue());

		glVertex3fv(rtr[i].getValue()); 
		glVertex3fv((rtr[i] + runtr[i]).getValue());

		glVertex3fv(rbl[i].getValue()); 
		glVertex3fv((rbl[i] + runbl[i]).getValue());

		glVertex3fv(rbr[i].getValue()); 
		glVertex3fv((rbr[i] + runbr[i]).getValue());
	}
	glEnd();
	glEnable(GL_LIGHTING);
#endif

		renderSolidRibbonTop();
		renderSolidRibbonBottom();
		renderSolidRibbonLeft();
		renderSolidRibbonRight();
}

void ChemDisplay::renderSolidRibbonTop()
{
	int i = 0;

	if (residueInterpolateColor)
	{
		glBegin(GL_TRIANGLE_STRIP);
		
		glColor3fv(residueColor[0].getValue());
		glNormal3fv(runtl[0].getValue()); 
		glVertex3fv(rtl[0].getValue());
		glNormal3fv(runtr[0].getValue());
		glVertex3fv(rtr[0].getValue());
		
		glColor3fv(residueColor[1].getValue());
		for (i=1;i<ribbonSegmentation-1;i++)
		{
			glNormal3fv(runtl[i].getValue());
			glVertex3fv(rtl[i].getValue());

			glNormal3fv(runtr[i].getValue());
			glVertex3fv(rtr[i].getValue());
		}

		glColor3fv(residueColor[2].getValue());
		glNormal3fv(runtl[i].getValue());
		glVertex3fv(rtl[i].getValue());

		glNormal3fv(runtr[i].getValue());
		glVertex3fv(rtr[i].getValue());

		glEnd();
		
	}
	else
	{
		glBegin(GL_TRIANGLE_STRIP);
		for (i=0;i<ribbonSegmentation;i++)
		{
			glNormal3fv(runtl[i].getValue());
			glVertex3fv(rtl[i].getValue());

			glNormal3fv(runtr[i].getValue());
			glVertex3fv(rtr[i].getValue());
		}
		glEnd();
	}
}

void ChemDisplay::renderSolidRibbonBottom()
{
	int i = 0;
	if (residueInterpolateColor)
	{
		glBegin(GL_TRIANGLE_STRIP);
		
		glColor3fv(residueColor[0].getValue());
		glNormal3fv(runbl[0].getValue()); 
		glVertex3fv(rbl[0].getValue());
		glNormal3fv(runbr[0].getValue());
		glVertex3fv(rbr[0].getValue());
		
		glColor3fv(residueColor[1].getValue());
		for (i=1;i<ribbonSegmentation-1;i++)
		{
			glNormal3fv(runbl[i].getValue());
			glVertex3fv(rbl[i].getValue());

			glNormal3fv(runbr[i].getValue());
			glVertex3fv(rbr[i].getValue());
		}

		glColor3fv(residueColor[2].getValue());
		glNormal3fv(runbl[i].getValue());
		glVertex3fv(rbl[i].getValue());

		glNormal3fv(runbr[i].getValue());
		glVertex3fv(rbr[i].getValue());

		glEnd();
		
	}
	else
	{
		glBegin(GL_TRIANGLE_STRIP);
		for (i=0;i<ribbonSegmentation;i++)
		{
			glNormal3fv(runbl[i].getValue());
			glVertex3fv(rbl[i].getValue());

			glNormal3fv(runbr[i].getValue());
			glVertex3fv(rbr[i].getValue());
		}
		glEnd();
	}
}

void ChemDisplay::renderSolidRibbonLeft()
{
	int i = 0;
	if (residueInterpolateColor)
	{
		glBegin(GL_TRIANGLE_STRIP);
		
		glColor3fv(residueColor[0].getValue());
		glNormal3fv(rlntl[0].getValue()); 
		glVertex3fv(rtl[0].getValue());
		glNormal3fv(rlnbl[0].getValue());
		glVertex3fv(rbl[0].getValue());
		
		glColor3fv(residueColor[1].getValue());
		for (i=1;i<ribbonSegmentation-1;i++)
		{
			glNormal3fv(rlntl[i].getValue());
			glVertex3fv(rtl[i].getValue());

			glNormal3fv(rlnbl[i].getValue());
			glVertex3fv(rbl[i].getValue());
		}

		glColor3fv(residueColor[2].getValue());
		glNormal3fv(rlntl[i].getValue());
		glVertex3fv(rtl[i].getValue());

		glNormal3fv(rlnbl[i].getValue());
		glVertex3fv(rbl[i].getValue());

		glEnd();
		
	}
	else
	{
		glBegin(GL_TRIANGLE_STRIP);
		for (i=0;i<ribbonSegmentation;i++)
		{
			glNormal3fv(rlntl[i].getValue());
			glVertex3fv(rtl[i].getValue());

			glNormal3fv(rlnbl[i].getValue());
			glVertex3fv(rbl[i].getValue());
		}
		glEnd();
	}
}

void ChemDisplay::renderSolidRibbonRight()
{
	int i = 0;
	if (residueInterpolateColor)
	{
		glBegin(GL_TRIANGLE_STRIP);
		
		glColor3fv(residueColor[0].getValue());
		glNormal3fv(rlntr[0].getValue()); 
		glVertex3fv(rtr[0].getValue());
		glNormal3fv(rlnbr[0].getValue());
		glVertex3fv(rbr[0].getValue());
		
		glColor3fv(residueColor[1].getValue());
		for (i=1;i<ribbonSegmentation-1;i++)
		{
			glNormal3fv(rlntr[i].getValue());
			glVertex3fv(rtr[i].getValue());

			glNormal3fv(rlnbr[i].getValue());
			glVertex3fv(rbr[i].getValue());
		}

		glColor3fv(residueColor[2].getValue());
		glNormal3fv(rlntr[i].getValue());
		glVertex3fv(rtr[i].getValue());

		glNormal3fv(rlnbr[i].getValue());
		glVertex3fv(rbr[i].getValue());

		glEnd();
		
	}
	else
	{
		glBegin(GL_TRIANGLE_STRIP);
		for (i=0;i<ribbonSegmentation;i++)
		{
			glNormal3fv(rlntr[i].getValue());
			glVertex3fv(rtr[i].getValue());

			glNormal3fv(rlnbr[i].getValue());
			glVertex3fv(rbr[i].getValue());
		}
		glEnd();
	}
}
// <-- solid ribbon

void ChemDisplay::renderResiduesAsSolidRibbons(SoGLRenderAction *action)
{
#ifdef DEBUG
printf ("In ChemDisplay::renderResiduesAsSolidRibbons\n");
#endif

	// Check to see if there's anything to render.
	if (residueIndex.getNum() == 0 ||  globalNumberOfResidues == 0) return;

	SoState *state;
	ChemColor *chemColor;
	ChemDisplayParam *cdp;
	ChemBaseData *chemBaseData;
	ChemData *chemData;

	setupResidues(action, state, chemColor, cdp, chemBaseData);

	chemData = (ChemData*)chemBaseData;

#ifndef IV2_0
	int32_t myMask;
	if (cdp->highlightStyle.getValue() == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
	{
		myMask = SoLazyElement::DIFFUSE_MASK | SoLazyElement::EMISSIVE_MASK;
	}
	else
	{
		myMask = SoLazyElement::DIFFUSE_MASK;
	}

	SoGLLazyElement *lazyElt = (SoGLLazyElement *)
			SoLazyElement::getInstance(state);
	lazyElt->send(state, SoLazyElement::ALL_MASK &( ~myMask));
#endif

	GLboolean bLighting = false;
	glGetBooleanv(GL_LIGHTING,&bLighting);
	if (!bLighting) glEnable(GL_LIGHTING);

	computeRibbonCulling();//compute individual culling

	switch (chemColor->residueColorBinding.getValue()) 
	{
		case ChemColor::RESIDUE_OVERALL:
		{
			normalSolidRibbonOV(vnormalResidueIndex, state, chemColor->residueColor[0],
				cdp, chemData);
			highlightSolidRibbonOV(vhighlightResidueIndex, state, chemColor->residueColor[0],
				cdp, chemData);
		}
		break;

		case ChemColor::RESIDUE_PER_INDEX:
		{
			normalSolidRibbonRPN(vnormalResidueIndex, state, chemColor,
				cdp, chemData);
			highlightSolidRibbonRPN(vhighlightResidueIndex, state, chemColor,
				cdp, chemData);
		}
		break;

		case ChemColor::RESIDUE_PER_CHAIN:
		{
			normalSolidRibbonRPC(vnormalResidueIndex, state, chemColor,
				cdp, chemData);
			highlightSolidRibbonRPC(vhighlightResidueIndex, state, chemColor,
				cdp, chemData);
		}
		break;

		default: break;
	}

	if (!bLighting) glDisable(GL_LIGHTING);

#ifndef IV2_0
	lazyElt->reset(state, myMask);
#endif

}


void ChemDisplay::normalSolidRibbonOV(const MFVec2i &theResidueIndex,
																	SoState *state,
																	const SbColor &chemColor,
																	const ChemDisplayParam *cdp,
																	const ChemData *chemData)
{
	// Check to see if there's anything to render.
	if (theResidueIndex.getNum() == 0) return;

	//SbVec3f 	coords1, coords2;

	glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

	glColor3fv(chemColor.getValue());

	ChemResidueStuff *res = NULL;
	RESIDUELOOP_START(theResidueIndex)

		RESDUP(theResidue);

		res = &residueAttributes[theResidue];
		assert(res != NULL);
		if (!res->isRibbonRenderable) continue;
		if (res->segments == 0) continue;

		computeSolidRibbonGeometry(theResidue);
		computeResidueColor(theResidue,state);	
		renderSolidRibbon();

	RESIDUELOOP_END

	glDisable(GL_COLOR_MATERIAL);
}

void ChemDisplay::normalSolidRibbonRPN(const MFVec2i &theResidueIndex,
																	SoState *state,
																	const ChemColor *chemColor,
																	const ChemDisplayParam *cdp,
																	const ChemData *chemData)
{
	// Check to see if there's anything to render.
	if (theResidueIndex.getNum() == 0) return;

	int32_t colorIndex;
	//SbVec3f 	coords1, coords2;

	glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

	ChemResidueStuff *res = NULL;
	RESIDUELOOP_START(theResidueIndex)

		RESDUP(theResidue);

		res = &residueAttributes[theResidue];
		assert(res != NULL);
		if (!res->isRibbonRenderable) continue;
		if (res->segments==0) continue;
		
		if (chemData->residues[theResidue].getIndex() < 0) colorIndex = 0;
		else colorIndex = chemData->residueColorIndex[chemData->residues[theResidue].getIndex()];
		glColor3fv(chemColor->residueColor[colorIndex].getValue());

		computeSolidRibbonGeometry(theResidue);
		computeResidueColor(theResidue,state);	
		renderSolidRibbon();

	RESIDUELOOP_END

	glDisable(GL_COLOR_MATERIAL);
}

void ChemDisplay::normalSolidRibbonRPC(const MFVec2i &theResidueIndex,
																	SoState *state,
																	const ChemColor *chemColor,
																	const ChemDisplayParam *cdp,
																	const ChemData *chemData)
{
	// Check to see if there's anything to render.
	if (theResidueIndex.getNum() == 0) return;

	int32_t colorIndex;
	//SbVec3f 	coords1, coords2;

	glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

	ChemResidueStuff *res = NULL;
	RESIDUELOOP_START(theResidueIndex)

		RESDUP(theResidue);

		res = &residueAttributes[theResidue];
		assert(res != NULL);
		if (!res->isRibbonRenderable) continue;
		if (res->segments==0) continue;
		
		colorIndex = chemData->residueChainIndex[chemData->residues[theResidue].getChain()];
		glColor3fv(chemColor->residueColor[colorIndex].getValue());

		computeSolidRibbonGeometry(theResidue);
		computeResidueColor(theResidue,state);	
		renderSolidRibbon();

	RESIDUELOOP_END

	glDisable(GL_COLOR_MATERIAL);
}

void ChemDisplay::highlightSolidRibbonOV(const MFVec2i &theResidueIndex,
																	SoState *state,
																	const SbColor &chemColor,
																	const ChemDisplayParam *cdp,
																	const ChemData *chemData)
{
	// Check to see if there's anything to render.
	if (theResidueIndex.getNum() == 0) return;

	//SbVec3f 	coords1, coords2;

	glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

	int32_t highlightStyle = cdp->highlightStyle.getValue();
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
	{
		const SbColor &theColor = cdp->highlightColor.getValue();
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,
			SbVec4f(theColor[0], theColor[1], theColor[2], 1.0).getValue());

		glColor3fv(chemColor.getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE)
	{
		const SbColor &theColor = cdp->highlightColor.getValue();
		glColor3fv(theColor.getValue());
	}

	ChemResidueStuff *res = NULL;
	//SoGLRenderAction *action = (SoGLRenderAction *)state->getAction();
	RESIDUELOOP_START(theResidueIndex)

		RESDUP(theResidue);

		res = &residueAttributes[theResidue];
		assert(res != NULL);
		if (!res->isRibbonRenderable) continue;
		if (res->segments==0) continue;

		computeSolidRibbonGeometry(theResidue);
		computeResidueColor(theResidue,state);	
		renderSolidRibbon();

	RESIDUELOOP_END

	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
	{
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,
			SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	
	glDisable(GL_COLOR_MATERIAL);
}

void ChemDisplay::highlightSolidRibbonRPN(const MFVec2i &theResidueIndex,
																	SoState *state,
																	const ChemColor *chemColor,
																	const ChemDisplayParam *cdp,
																	const ChemData *chemData)
{
	// Check to see if there's anything to render.
	if (theResidueIndex.getNum() == 0) return;

	int highlightStyle = cdp->highlightStyle.getValue();
	const SbColor &highlightColor = cdp->highlightColor.getValue();
	int32_t colorIndex;

	glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
	{
			glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,
				SbVec4f(highlightColor[0], highlightColor[1],
						highlightColor[2], 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
	{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE)
	{
		glColor3fv(highlightColor.getValue());
		ChemResidueStuff *res = NULL;
		RESIDUELOOP_START(theResidueIndex)

			RESDUP(theResidue);

			res = &residueAttributes[theResidue];
			assert(res != NULL);
			if (!res->isRibbonRenderable) continue;
			if (res->segments==0) continue;

			computeSolidRibbonGeometry(theResidue);
			computeResidueColor(theResidue,state);	
		  renderSolidRibbon();

		RESIDUELOOP_END
	}
	else
	{
		ChemResidueStuff *res = NULL;
		RESIDUELOOP_START(theResidueIndex)

			RESDUP(theResidue);

			res = &residueAttributes[theResidue];
			assert(res != NULL);
			if (!res->isRibbonRenderable) continue;
			if (res->segments==0) continue;
			
			if (chemData->residues[theResidue].getIndex() < 0) colorIndex = 0;
			else colorIndex = chemData->residueColorIndex[chemData->residues[theResidue].getIndex()];
			glColor3fv(chemColor->residueColor[colorIndex].getValue());

			computeSolidRibbonGeometry(theResidue);
			computeResidueColor(theResidue,state);	
			renderSolidRibbon();

		RESIDUELOOP_END
	}

	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
	{
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,
			SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	
	glDisable(GL_COLOR_MATERIAL);
}

void ChemDisplay::highlightSolidRibbonRPC(const MFVec2i &theResidueIndex,
																	SoState *state,
																	const ChemColor *chemColor,
																	const ChemDisplayParam *cdp,
																	const ChemData *chemData)
{
	// Check to see if there's anything to render.
	if (theResidueIndex.getNum() == 0) return;

	int highlightStyle = cdp->highlightStyle.getValue();
	const SbColor &highlightColor = cdp->highlightColor.getValue();
	int32_t colorIndex;

	glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
	{
			glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,
				SbVec4f(highlightColor[0], highlightColor[1],
						highlightColor[2], 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
	{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE)
	{
		glColor3fv(highlightColor.getValue());
		ChemResidueStuff *res = NULL;
		RESIDUELOOP_START(theResidueIndex)

			RESDUP(theResidue);

			res = &residueAttributes[theResidue];
			assert(res != NULL);
			if (!res->isRibbonRenderable) continue;
			if (res->segments==0) continue;

			computeSolidRibbonGeometry(theResidue);
			computeResidueColor(theResidue,state);	
			renderSolidRibbon();

		RESIDUELOOP_END
	}
	else
	{
		ChemResidueStuff *res = NULL;
		RESIDUELOOP_START(theResidueIndex)

			RESDUP(theResidue);

			res = &residueAttributes[theResidue];
			assert(res != NULL);
			if (!res->isRibbonRenderable) continue;
			if (res->segments==0) continue;
			
			colorIndex = chemData->residueChainIndex[chemData->residues[theResidue].getChain()];
			glColor3fv(chemColor->residueColor[colorIndex].getValue());

			computeSolidRibbonGeometry(theResidue);
			computeResidueColor(theResidue,state);	
			renderSolidRibbon();

		RESIDUELOOP_END
	}
	
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
	{
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,
			SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	
	glDisable(GL_COLOR_MATERIAL);
}

// <-- solid ribbon


// --> schematic

void ChemDisplay::setupSchematic(SoGLRenderAction *action, SoState *&state,
	ChemColor *&chemColor, ChemDisplayParam *&cdp,
	ChemBaseData *&chemData)
//
////////////////////////////////////////////////////////////////////////
{
	//int 	displayStyle;
	
	state     = action->getState();
	chemColor = ChemColorElement::get(state);
	cdp       = ChemDisplayParamElement::get(state);
	chemData  = ChemBaseDataElement::get(state);
	//displayStyle = cdp->residueCylinderDisplayStyle.getValue();

	schematicCylinderRadius = cdp->schematicCylinderRadius;
	if (schematicCylinderComplexity != cdp->schematicCylinderComplexity)
	{
		schematicCylinderComplexity = cdp->schematicCylinderComplexity;
		if (schematicUnitCylinder != NULL) delete schematicUnitCylinder;
			schematicUnitCylinder = new ChemUnitCylinder(
				ChemDisplayParam::BONDCYLINDER_ARRAY_FLATCAP,
				schematicCylinderComplexity.getValue());
	}

	schematicArrowTesselationSegments = cdp->schematicArrowTesselationSegments.getValue();	

	schematicTubeTesselationSegments = cdp->schematicTubeTesselationSegments.getValue();	
	schematicTubeRadius = cdp->schematicTubeRadius.getValue();
	
	int32_t shapeSegments = cdp->schematicTubeShapeSegments.getValue();

	if (schematicTubeShapeSegments.getValue() != shapeSegments && shapeSegments >= 3)
	{
		if (scvtx!= NULL)
		{
			delete[] scvtx;
			delete[] scnrm;
			scvtx = NULL;
			scnrm = NULL;
		}

		if (shapeSegments > 0)
		{
			scvtx = new SoMFVec3f[shapeSegments];
			scnrm = new SoMFVec3f[shapeSegments];

			// precompute shape vertices
			int j;
			double angle,R;
			R = schematicTubeRadius.getValue();
			SbVec3f C;
			scTubeVertices.setNum(shapeSegments);
			for (j=0;j<shapeSegments;j++)
			{
				angle = (2*M_PI/shapeSegments)*j;	
				C.setValue(R*cos(angle), R*sin(angle),0);
				scTubeVertices.set1Value(j,C);
			}
		}

		schematicTubeShapeSegments = shapeSegments;
	}
}

void ChemDisplay::renderResiduesAsSchematic(SoGLRenderAction *action)
{
#ifdef DEBUG
printf ("In ChemDisplay::renderResiduesAsSchematic\n");
#endif

	// Check to see if there's anything to render.
	if (schematicIndex.getNum() == 0 ||  globalNumberOfSchematics == 0) return;

	SoState *state;
	ChemColor *chemColor;
	ChemDisplayParam *cdp;
	ChemBaseData *chemBaseData;
	ChemData *chemData;

	//setupResidues(action, state, chemColor, cdp, chemBaseData); // tmp
	setupSchematic(action, state, chemColor, cdp, chemBaseData);

	chemData = (ChemData*)chemBaseData;

#ifndef IV2_0
	int32_t myMask;
	if (cdp->highlightStyle.getValue() == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
	{
		myMask = SoLazyElement::DIFFUSE_MASK | SoLazyElement::EMISSIVE_MASK;
	}
	else
	{
		myMask = SoLazyElement::DIFFUSE_MASK;
	}

	SoGLLazyElement *lazyElt = (SoGLLazyElement *)
			SoLazyElement::getInstance(state);
	lazyElt->send(state, SoLazyElement::ALL_MASK &( ~myMask));
#endif

	GLboolean bLighting = false;
	glGetBooleanv(GL_LIGHTING,&bLighting);
	if (!bLighting) glEnable(GL_LIGHTING);

	switch (chemColor->schematicColorBinding.getValue()) 
	{
		case ChemColor::SCHEMATIC_OVERALL:
		{
			normalSchematicOV(normalSchematicIndex, state, chemColor->schematicColor[0],
				cdp, chemData);
			highlightSchematicOV(highlightSchematicIndex, state, chemColor->residueColor[0],
				cdp, chemData);
		}
		break;

		case ChemColor::SCHEMATIC_PER_SECONDARY_TYPE:
		{
			normalSchematicPST(normalSchematicIndex, state, chemColor,
				cdp, chemData);
			highlightSchematicPST(highlightSchematicIndex, state, chemColor,
				cdp, chemData);
		}
		break;

		case ChemColor::SCHEMATIC_PER_ELEMENT:
		{
			normalSchematicPE(normalSchematicIndex, state, chemColor,
				cdp, chemData);
			highlightSchematicPE(highlightSchematicIndex, state, chemColor,
				cdp, chemData);
		}
		break;

		default: break;
	}

	if (!bLighting) glDisable(GL_LIGHTING);

#ifndef IV2_0
	lazyElt->reset(state, myMask);
#endif
}

// --> schematic
void ChemDisplay::renderSchematicCylinders(const SbIntList& cylinders, SoState *state)
{
	int i;
  SbVec2i fromTo(-1,-1);
		
	beginCylinders(ChemDisplayParam::BONDCYLINDER_ARRAY_FLATCAP,schematicUnitCylinder);

  for (i=0;i<cylinders.getLength();i++)
	{
	  schematicUnitCylinder->render(scfrom[cylinders[i]], scto[cylinders[i]], 
		schematicCylinderRadius.getValue(), true,false,ChemDisplayParam::BONDCYLINDER_ARRAY_FLATCAP,fromTo,this,(SoGLRenderAction*)state->getAction());
	}

	endCylinders(ChemDisplayParam::BONDCYLINDER_ARRAY_FLATCAP);
}

void ChemDisplay::renderSchematicArrows(const SbIntList& arrows, SoState *state)
{
	int i,j;
	
	for (i=0;i<arrows.getLength();i++)
	{

		const SbIntList& list = *((SbIntList*)schematicArrows.get(arrows[i]));
		for(j=0;j<list.getLength()-1;j++)
		{
			   computeSchematicArrowGeometry(list[j]);
				 renderSchematicArrow();
		}
		//draw ace
		computeSchematicArrowGeometry(list[j],true);
		renderSchematicArrow();
	}
}

void ChemDisplay::renderSchematicTubes(const SbIntList& tubes, SoState *state)
{
	int i;
	//ChemData* chemData = (ChemData*) ChemBaseDataElement::get(state);
	//ChemDisplayParam* cdp = ChemDisplayParamElement::get(state);
	
	for (i=0;i<tubes.getLength();i++)
	{
		assert (tubes[i]<schematicTubes.getLength());
		const SbIntList& list = *((SbIntList*)schematicTubes.get(tubes[i]));
		assert (list.getLength()>=2);
		computeTubePath(list);
		computeSchematicTubeGeometry();
		renderSchematicTube();
	}
}


void ChemDisplay::normalSchematicOV(const MFVec2i &theSchematicIndex,
									SoState *state,
									const SbColor &chemColor,
									const ChemDisplayParam *cdp,
									const ChemData *chemData)
{
	// Check to see if there's anything to render.
	if (theSchematicIndex.getNum() == 0) return;

	ChemSchematicChain* schematic = NULL;
	
	glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);	
	
	glColor3fv(chemColor.getValue());
	

	SCHEMATICLOOP_START(theSchematicIndex)

		schematic = &schematicChains[theSchematic];

		renderSchematicCylinders(schematic->cylinders,state);
		renderSchematicArrows(schematic->arrows,state);
		renderSchematicTubes(schematic->tubes,state);

	SCHEMATICLOOP_END

	glDisable(GL_COLOR_MATERIAL);
}

void ChemDisplay::normalSchematicPST(const MFVec2i &theSchematicIndex,
									SoState *state,
									const ChemColor *chemColor,
									const ChemDisplayParam *cdp,
									const ChemData *chemData)
{
	// Check to see if there's anything to render.
	if (theSchematicIndex.getNum() == 0) return;

	ChemSchematicChain* schematic = NULL;

	glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

	SCHEMATICLOOP_START(theSchematicIndex)

		schematic = &schematicChains[theSchematic];

                // KLS 2/9/06
                // If the start/end range (theSchematic) gets to be greater
                // than 1 (not sure what causes it but you can see this on
                // insulin, when the system is reset, only the first pointer
                // gets reset to 0.  The end range is not correctly updated
                // and the 'end' value (see the macro), is not reset either.
                // Since the first pointer always seems to get reset to 0,
                // we can simply break out of this loop if we hit it
                // and avoid crashes.  
                // I could not trace the problem down to its actual source
                // unfortunately.
                if (schematic == NULL) break;

		glColor3fv(chemColor->schematicColor[schematicCylinderIndex[0]].getValue());
		renderSchematicCylinders(schematic->cylinders,state);
		glColor3fv(chemColor->schematicColor[schematicArrowIndex[0]].getValue());
		renderSchematicArrows(schematic->arrows,state);
		glColor3fv(chemColor->schematicColor[schematicTubeIndex[0]].getValue());
		renderSchematicTubes(schematic->tubes,state);
			
	SCHEMATICLOOP_END

	
	glDisable(GL_COLOR_MATERIAL);
}

void ChemDisplay::normalSchematicPE(const MFVec2i &theSchematicIndex,
									SoState *state,
									const ChemColor *chemColor,
									const ChemDisplayParam *cdp,
									const ChemData *chemData)
{
	// Check to see if there's anything to render.
	if (theSchematicIndex.getNum() == 0) return;

  int i,j,k;
	ChemSchematicChain* schematic = NULL;
	SbVec2i fromTo(-1,-1);

	glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

  SCHEMATICLOOP_START(theSchematicIndex)

    schematic = &schematicChains[theSchematic];

    // cylinders
    beginCylinders(ChemDisplayParam::BONDCYLINDER_ARRAY_FLATCAP,schematicUnitCylinder);

    for (i=0;i<schematic->cylinders.getLength();i++)
    {
      k = schematic->cylinders[i];
	    glColor3fv(chemColor->schematicColor[schematicCylinderIndex[k+1]].getValue());
	    schematicUnitCylinder->render(scfrom[k], scto[k], 
	    schematicCylinderRadius.getValue(), true,false,
	    ChemDisplayParam::BONDCYLINDER_ARRAY_FLATCAP,fromTo,this,
	    (SoGLRenderAction*)state->getAction());
    }

    endCylinders(ChemDisplayParam::BONDCYLINDER_ARRAY_FLATCAP);

    // arrows
    for (i=0;i<schematic->arrows.getLength();i++)
    {
      k = schematic->arrows[i];
	    glColor3fv(chemColor->schematicColor[schematicArrowIndex[k+1]].getValue());
	    const SbIntList& list = *((SbIntList*)schematicArrows.get(k));
	    for(j=0;j<list.getLength()-1;j++)
	    {
		    computeSchematicArrowGeometry(list[j]);
		    renderSchematicArrow();
	    }
	    //draw ace
	    computeSchematicArrowGeometry(list[j],true);
	    renderSchematicArrow();
    }

    // tubes
    for (i=0;i<schematic->tubes.getLength();i++)
    {
      k = schematic->tubes[i];
	    glColor3fv(chemColor->schematicColor[schematicTubeIndex[k+1]].getValue());
	    const SbIntList& list = *((SbIntList*)schematicTubes.get(k));
	    computeTubePath(list);
	    computeSchematicTubeGeometry();
	    renderSchematicTube();
    }

	SCHEMATICLOOP_END

	glDisable(GL_COLOR_MATERIAL);
}

void ChemDisplay::highlightSchematicOV(const MFVec2i &theSchematicIndex,
										SoState *state,
										const SbColor &chemColor,
										const ChemDisplayParam *cdp,
										const ChemData *chemData)
{
	// Check to see if there's anything to render.
	if (theSchematicIndex.getNum() == 0) return;

	ChemSchematicChain* schematic;

	glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

	int32_t highlightStyle = cdp->highlightStyle.getValue();
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
	{
		const SbColor &theColor = cdp->highlightColor.getValue();
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,
			SbVec4f(theColor[0], theColor[1], theColor[2], 1.0).getValue());
		glColor3fv(chemColor.getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE)
	{
		const SbColor &theColor = cdp->highlightColor.getValue();
		glColor3fv(theColor.getValue());
	}

	SCHEMATICLOOP_START(theSchematicIndex)

		schematic = &schematicChains[theSchematic];

		renderSchematicCylinders(schematic->cylinders,state);
		renderSchematicArrows(schematic->arrows,state);
		renderSchematicTubes(schematic->tubes,state);

	SCHEMATICLOOP_END

	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
	{
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,
			SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	
	glDisable(GL_COLOR_MATERIAL);
}

void ChemDisplay::highlightSchematicPST(const MFVec2i &theSchematicIndex,
										SoState *state,
										const ChemColor *chemColor,
										const ChemDisplayParam *cdp,
										const ChemData *chemData)
{
	// Check to see if there's anything to render.
	if (theSchematicIndex.getNum() == 0) return;

  ChemSchematicChain* schematic = NULL;
	int highlightStyle = cdp->highlightStyle.getValue();
	const SbColor &highlightColor = cdp->highlightColor.getValue();
	
	glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
	{
			glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,
				SbVec4f(highlightColor[0], highlightColor[1],
						highlightColor[2], 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
	{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE)
	{
		glColor3fv(highlightColor.getValue());

		SCHEMATICLOOP_START(theSchematicIndex)

			schematic = &schematicChains[theSchematic];

			renderSchematicCylinders(schematic->cylinders,state);
			renderSchematicArrows(schematic->arrows,state);
			renderSchematicTubes(schematic->tubes,state);
			
		SCHEMATICLOOP_END
		
	}
	else // HIGHLIGHT_EMISSIVE || HIGHLIGHT_DRAWSTYLE
	{
		SCHEMATICLOOP_START(theSchematicIndex)

			schematic = &schematicChains[theSchematic];

			glColor3fv(chemColor->schematicColor[schematicCylinderIndex[0]].getValue());
			renderSchematicCylinders(schematic->cylinders,state);
			glColor3fv(chemColor->schematicColor[schematicArrowIndex[0]].getValue());
			renderSchematicArrows(schematic->arrows,state);
			glColor3fv(chemColor->schematicColor[schematicTubeIndex[0]].getValue());
			renderSchematicTubes(schematic->tubes,state);
				
		SCHEMATICLOOP_END
	}
	
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
	{
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,
			SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	
	glDisable(GL_COLOR_MATERIAL);
}

void ChemDisplay::highlightSchematicPE (const MFVec2i &theSchematicIndex,
									    SoState *state,
									    const ChemColor *chemColor,
									    const ChemDisplayParam *cdp,
										const ChemData *chemData)
{
	// Check to see if there's anything to render.
	if (theSchematicIndex.getNum() == 0) return;

  int i,j,k;
  ChemSchematicChain* schematic = NULL;
  SbVec2i fromTo(-1,-1);
	int highlightStyle = cdp->highlightStyle.getValue();
	const SbColor &highlightColor = cdp->highlightColor.getValue();	

	glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
	{
			glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,
				SbVec4f(highlightColor[0], highlightColor[1],
						highlightColor[2], 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
	{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DIFFUSE)
	{
		  glColor3fv(highlightColor.getValue());

  		SCHEMATICLOOP_START(theSchematicIndex)

			  schematic = &schematicChains[theSchematic];

		    // cylinders
        beginCylinders(ChemDisplayParam::BONDCYLINDER_ARRAY_FLATCAP,schematicUnitCylinder);

        for (i=0;i<schematic->cylinders.getLength();i++)
        {
          k = schematic->cylinders[i];
	        schematicUnitCylinder->render(scfrom[k], scto[k], 
	        schematicCylinderRadius.getValue(), true,false,
	        ChemDisplayParam::BONDCYLINDER_ARRAY_FLATCAP,fromTo,this,
	        (SoGLRenderAction*)state->getAction());
        }

        endCylinders(ChemDisplayParam::BONDCYLINDER_ARRAY_FLATCAP);

        // arrows
        for (i=0;i<schematic->arrows.getLength();i++)
        {
          k = schematic->arrows[i];
	        const SbIntList& list = *((SbIntList*)schematicArrows.get(k));
	        for(j=0;j<list.getLength()-1;j++)
	        {
		        computeSchematicArrowGeometry(list[j]);
		        renderSchematicArrow();
	        }
	        //draw ace
	        computeSchematicArrowGeometry(list[j],true);
	        renderSchematicArrow();
        }

        // tubes
        for (i=0;i<schematic->tubes.getLength();i++)
        {
          k = schematic->tubes[i];
	        const SbIntList& list = *((SbIntList*)schematicTubes.get(k));
	        computeTubePath(list);
	        computeSchematicTubeGeometry();
	        renderSchematicTube();
        }

		SCHEMATICLOOP_END
	}
	else
	{
  		SCHEMATICLOOP_START(theSchematicIndex)

			  schematic = &schematicChains[theSchematic];

        // cylinders
        beginCylinders(ChemDisplayParam::BONDCYLINDER_ARRAY_FLATCAP,schematicUnitCylinder);

        for (i=0;i<schematic->cylinders.getLength();i++)
        {
          k = schematic->cylinders[i];
	        glColor3fv(chemColor->schematicColor[schematicCylinderIndex[k+1]].getValue());
	        schematicUnitCylinder->render(scfrom[k], scto[k], 
	        schematicCylinderRadius.getValue(), true,false,
	        ChemDisplayParam::BONDCYLINDER_ARRAY_FLATCAP,fromTo,this,
	        (SoGLRenderAction*)state->getAction());
        }

        endCylinders(ChemDisplayParam::BONDCYLINDER_ARRAY_FLATCAP);

        // arrows
        for (i=0;i<schematic->arrows.getLength();i++)
        {
          k = schematic->arrows[i];
	        glColor3fv(chemColor->schematicColor[schematicArrowIndex[k+1]].getValue());
	        const SbIntList& list = *((SbIntList*)schematicArrows.get(k));
	        for(j=0;j<list.getLength()-1;j++)
	        {
		        computeSchematicArrowGeometry(list[j]);
		        renderSchematicArrow();
	        }
	        //draw ace
	        computeSchematicArrowGeometry(list[j],true);
	        renderSchematicArrow();
        }

        // tubes
        for (i=0;i<schematic->tubes.getLength();i++)
        {
          k = schematic->tubes[i];
	        glColor3fv(chemColor->schematicColor[schematicTubeIndex[k+1]].getValue());
	        const SbIntList& list = *((SbIntList*)schematicTubes.get(k));
	        computeTubePath(list);
	        computeSchematicTubeGeometry();
	        renderSchematicTube();
        }

		SCHEMATICLOOP_END
	}
	
	if (highlightStyle == ChemDisplayParam::HIGHLIGHT_EMISSIVE)
	{
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,
			SbVec4f(0.0, 0.0, 0.0, 1.0).getValue());
	}
	else if (highlightStyle == ChemDisplayParam::HIGHLIGHT_DRAWSTYLE)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	
	glDisable(GL_COLOR_MATERIAL);
}

/*
void ChemDisplay::normalSchematic(SoState *state)
{
	glEnable(GL_LIGHTING);
	glEnable(GL_COLOR_MATERIAL);

	if (schematicCylinders.getLength() > 0)
		renderSchematicCylinders(state);
	if (schematicArrows.getLength() > 0)
		renderSchematicArrows(state);
	if (schematicTubes.getLength() > 0)
		renderSchematicTubes(state);


	glDisable(GL_COLOR_MATERIAL);
}
*/
// compute matrix inverse 
// remove when different implementations 
void MATINV(double **AA) 
{                            
   double DET;
   double PC[10],PL[10],CS[10];                                          
   double PV,PAV,TT;
   int I,IK,J,JK,K;

   DET= 1.0;
   for (I=0; I<3; I++)  {
        PC[I]= 0.0;
        PL[I]= 0.0;
        CS[I]= 0.0;
   }

   for (K=0; K<3; K++) {
        PV=AA[K][K];
        IK=K;
        JK=K;
        PAV= fabs(PV);
        for (I=K; I<3; I++) {
          for (J=K; J<3; J++) {
            if (fabs(AA[I][J]) > PAV) {
              PV=AA[I][J];
              PAV= fabs(PV);
              IK=I;
              JK=J;
            }
          }
        }

        PC[K]=(double)JK;
        PL[K]=(double)IK;

        if (IK!=K) DET=-DET;
        if (JK!=K) DET=-DET;
        DET=DET*PV;
        if ( fabs(DET) < FLT_MIN) {
          return;
        }

        if(IK!=K)
          for (I=0; I<3; I++) {
            TT=AA[IK][I];
            AA[IK][I]=AA[K][I];
            AA[K][I]=TT;
          }

        if(JK!=K)
          for (I=0; I<3; I++) {
            TT=AA[I][JK];
            AA[I][JK]=AA[I][K];
            AA[I][K]=TT;
          }

        for (I=0; I<3; I++) {
          CS[I]=AA[I][K];
          AA[I][K]= 0.0;
        }

        CS[K]= 0.0;
        AA[K][K]= 1.0;
        if(fabs(PV)< FLT_MIN) {
          return;
        }
        for (I=0; I<3; I++)
          AA[K][I]=AA[K][I]/PV;

        for (J=0; J<3; J++) {
          if (J==K) continue;
          for (I=0; I<3; I++)
            AA[J][I]=AA[J][I]-CS[J]*AA[K][I];
        }
   }

   for (I=3-1; I>=0; I--) {
        IK=(int) PC[I];
        if (IK==I) continue; 
        for (J=0; J<3; J++) {
          TT=AA[I][J];
          AA[I][J]=AA[IK][J];
          AA[IK][J]=TT;
        }
   } 

   for (J=3-1; J>=0; J--) {
        JK=(int) PL[J];
        if (JK==J) continue;
        for (I=0; I<3; I++) {
          TT=AA[I][J];
          AA[I][J]=AA[I][JK];
          AA[I][JK]=TT;
        }
   }
}

/***********************************************************
* calculate greatest eigenvalue and associated eigenvector *
* by the power method                                      *
***********************************************************/
void minEig(double **A, double *X1)  
{
  int it;
  double eps = FLT_MIN;
  int m = 1000;
  double dta = 0.0001f;
  double gamma;
  int i,j,l;
  double phi,s;
  double X0[3];

  MATINV(A);

  for (i=0; i<3; i++) 
     X0[i]=1.0/sqrtf(i+1.0f);

  it=-1; 
  l=1;

  while (it==-1 && l<=m) {
    gamma=0.0;
    for (i=0; i<3; i++) {
      X1[i]=0.0;
      for (j=0; j<3; j++) 
          X1[i] += A[i][j]*X0[j];
      if (fabs(X1[i])>fabs(gamma))
          gamma=X1[i];
    }
    if (fabs(gamma) < eps) it=0;
    else {
      for (i=0; i<3; i++) 
          X1[i] /= gamma;
      phi=0.0;
      for (i=0; i<3; i++)  {
        s=fabs(X1[i]-X0[i]);
        if (s>phi) phi=s;
      }
      if (phi<dta) it=1;
      else {
        for (i=0; i<3; i++) X0[i]=X1[i];
        l++;
      }
    }
  }
}                

// --> schematics
void ChemDisplay::getResiduesForSchematicChain(MFVec2i* residues, int32_t* from, int32_t chain)
{
  int i,j;
  ChemSchematicChain *schematicChain = NULL;
  SbIntList* list = NULL;
  if ( !(chain>=0 && chain < globalNumberOfSchematics) )
    return;

  schematicChain = &schematicChains[chain];
  for (i=0;i<schematicChain->cylinders.getLength();i++)
  {
    list = (SbIntList*) schematicCylinders[schematicChain->cylinders[i]];
    for (j=0;j<list->getLength();j++)
      residues->set1Value((*from)++,SbVec2i((*list)[j],1));
  }

  for (i=0;i<schematicChain->arrows.getLength();i++)
  {
    list = (SbIntList*) schematicArrows[schematicChain->arrows[i]];
    for (j=0;j<list->getLength();j++)
      residues->set1Value((*from)++,SbVec2i((*list)[j],1));
  }

  for (i=0;i<schematicChain->tubes.getLength();i++)
  {
    list = (SbIntList*) schematicTubes[schematicChain->tubes[i]];
    for (j=0;j<list->getLength();j++)
      residues->set1Value((*from)++,SbVec2i((*list)[j],1));
  }
}
// <-- schematics

void ChemDisplay::computeSchematicChains(ChemData* chemData, ChemDisplayParam* cdp)
{
	int i;
	SbIntList* indices = NULL;
	int32_t currentResidueType;
	int32_t chainType = -1;
	int32_t minimumResiduesForCylinder = cdp->schematicCylinderMininumResidues.getValue();
	int32_t minimunResiduesForArrow = cdp->schematicArrowMininumResidues.getValue();
	ChemResidueStuff* res = NULL;

// ca distance support
	MFVec2i newResidueIndex;
	int from = -1,to, num;
	RESIDUELOOP_START(residueIndex)
		res = &residueAttributes[theResidue];
		if (from == -1)
		{
			if (res->caDistance)
				from = theResidue;
			continue;
		}
		if (theResidue == residueEnd-1 || !res->caDistance )
		{
			if (!res->caDistance)
				to = theResidue-1;
			else
				to = theResidue;

			num = to-from;
			if (num>1)
				newResidueIndex.set1Value(newResidueIndex.getNum(),from,num);
			from = -1;
		}			
	RESIDUELOOP_END

// chains support
  int cnt;
  cnt = newResidueIndex.getNum();
  if (cnt>0)
    schematicChains = new ChemSchematicChain[cnt];

  localNumberOfSchematics = globalNumberOfSchematics = cnt;

	RESIDUELOOP_START(newResidueIndex)
  
		res = &residueAttributes[theResidue];
		assert(res != NULL);
		
		if (!res->isRibbonRenderable)
			continue;

		currentResidueType = chemData->residues[theResidue].residueType.getValue();

		if (indices == NULL)
			indices = new SbIntList();

		if (indices->getLength() == 0)
		{
			indices->append(theResidue);
			chainType  = currentResidueType;
			continue;
		}
		
		if (currentResidueType != chainType 
			|| theResidue == residueEnd-1 )
		{
				if (theResidue == residueEnd-1 && currentResidueType == chainType)
					indices->append(theResidue);

				//store new chain
				
				if (chainType  == SbResidue::RESIDUE_TYPE_HELIX && indices->getLength() >= minimumResiduesForCylinder)
				{
					schematicCylinders.append(indices);
					schematicChains[residueLoop].cylinders.append(schematicCylinders.getLength()-1);
					indices = NULL;
				}
				else if (chainType  == SbResidue::RESIDUE_TYPE_SHEET && indices->getLength() >= minimunResiduesForArrow)
				{
					schematicArrows.append(indices);
					schematicChains[residueLoop].arrows.append(schematicArrows.getLength()-1);
					indices = NULL;
				}
				else if (indices->getLength()>0)// tube
				{
					// enlarge previous tube with invalid cylinders or arrows
					if (schematicTubes.getLength()>0)
					{
						SbIntList* list = (SbIntList*)schematicTubes.get(schematicTubes.getLength()-1);
						if ((*list)[list->getLength()-1]+1 == (*indices)[0])
						{
							for (i=0; i<indices->getLength(); i++)
								list->append( (*indices)[i] );
							delete indices;
							indices = NULL;
						}
					}

					if (indices != NULL && indices->getLength()>=2) // it doesnt join
					{
						schematicTubes.append(indices);
						schematicChains[residueLoop].tubes.append(schematicTubes.getLength()-1);
						indices = NULL;
					}
					else if (indices)// invalid tube (insufficient residues, residues<2)
					{
						delete indices;
						indices = NULL;
					}					
				}
				else
				{
					delete indices;
					indices = NULL;
				}

				// first new element
				if (theResidue != residueEnd-1)
				{
					chainType  = chemData->residues[theResidue].residueType.getValue();
					indices = new SbIntList();
					indices->append(theResidue);
				}
				else
					chainType = -1;
		}
		else
			indices->append(theResidue);

	RESIDUELOOP_END

	if (indices != NULL)
		delete indices;
	indices = NULL;

	// enlarge tubes to grant connectivity
	for (i=0;i<schematicTubes.getLength();i++)
	{
		const SbIntList& list = *((SbIntList*)schematicTubes.get(i));

		// enlarge tube if previous residue is helix-type
		if (list[0]>0 && 
			chemData->residues[list[0]-1].residueType.getValue() == SbResidue::RESIDUE_TYPE_HELIX
			&& residueAttributes[list[0]-1].isRibbonRenderable)
				((SbIntList*)schematicTubes.get(i))->insert(list[0]-1,0);

		// enlarge tube if next residue is helix-type
		if (list[list.getLength()-1]<globalNumberOfResidues-1 && 
			chemData->residues[list[list.getLength()-1]+1].residueType.getValue() == SbResidue::RESIDUE_TYPE_HELIX
			&& residueAttributes[list[list.getLength()-1]+1].isRibbonRenderable)
				((SbIntList*)schematicTubes.get(i))->append(list[list.getLength()-1]+1);	
	}
	
	// alloc color indexes per element
	schematicCylinderIndex.setNum(schematicCylinders.getLength()+1);
	for (i=1;i<schematicCylinders.getLength();i++) schematicCylinderIndex.set1Value(i,0);
	schematicArrowIndex.setNum(schematicArrows.getLength()+1);
	for (i=1;i<schematicArrows.getLength();i++) schematicArrowIndex.set1Value(i,0);
	schematicTubeIndex.setNum(schematicTubes.getLength()+1);
	for (i=1;i<schematicTubes.getLength();i++) schematicTubeIndex.set1Value(i,0);

}

// this function only must remove those schematic variables 
// that change if residueIndex is modified
void ChemDisplay::cleanSchematic()
{
	if (schematicChains != NULL)
	{
		delete[] schematicChains;
		schematicChains = NULL;
	}

	SbIntList* indices = NULL;
	while (schematicCylinders.getLength() > 0)
	{
		indices = (SbIntList*)schematicCylinders[0];
		schematicCylinders.remove(0);
		delete indices;
	}
	while (schematicArrows.getLength() > 0)
	{
		indices = (SbIntList*)schematicArrows[0];
		schematicArrows.remove(0);
		delete indices;
	}
	while (schematicTubes.getLength() > 0)
	{
		indices = (SbIntList*)schematicTubes[0];
		schematicTubes.remove(0);
		delete indices;
	}
#ifdef __coin
		// this is done for avoiding memory leaks
		// memory pointers aren't freed
		schematicCylinders.truncate(0,TRUE);
		schematicArrows.truncate(0,TRUE);
		schematicTubes.truncate(0,TRUE);
#else // _coin
    schematicCylinders.truncate(0);
    schematicArrows.truncate(0);
    schematicTubes.truncate(0);
#endif
}

void ChemDisplay::computeSchematicCylinders(ChemData* chemData)
{
	// Compute cylinder coordinates for each helix type residue:
// - Compute Inertial Moment Matrix for each helix type residue.
// - Compute min eigenvector for that matrix.
// - Set a cylinder projecting first and last CA of a helix type residue
// in that eigenvector.

	int i,j;
	//SbVec2i fromTo;
	SbVec3f average;
	double x2,y2,z2,xy,yz,zx;
	SbVec3f centroid, atom;
	double** M;
	M=new double*[3];
	M[0]=new double[3];
	M[1]=new double[3];
	M[2]=new double[3];
	double X[3];
	
	int32_t caIndex = SbResidue::RESIDUE_CONTROL_POINT_CA;
	for (i=0;i<schematicCylinders.getLength();i++)
	{
		average[0]= average[1]=average[2]= 0.0f;
		const SbIntList& list = *((SbIntList*)schematicCylinders.get(i));
		for(j=0;j<list.getLength();j++)
		{
			average+= chemData->getAtomCoordinates(
				chemData->residues[list[j]][caIndex]);
		}
		centroid = average/(float)list.getLength();
		
		x2=y2=z2=0.0f;
		xy=yz=zx=0.0f;
		for(j=0;j<list.getLength();j++)
		{
			atom = chemData->getAtomCoordinates(chemData->residues[list[j]][caIndex]) - centroid;
			x2 += atom[0]*atom[0];
			y2 += atom[1]*atom[1];
			z2 += atom[2]*atom[2];
			xy += atom[0]*atom[1];
			yz += atom[1]*atom[2];
			zx += atom[2]*atom[0];
		}
		M[0][0]= y2 + z2;
		M[0][1]= -xy;
		M[0][2]= -zx;
		M[1][0]= -xy;
		M[1][1]= x2 + z2;
		M[1][2]= -yz;
		M[2][0]= -zx;
		M[2][1]= -yz;
		M[2][2]= x2 + y2;

		minEig(M,X); 
		SbVec3f axis(X[0],X[1],X[2]);
		axis.normalize();
		
		SbVec3f p1 = chemData->getAtomCoordinates(chemData->residues[list[0]][caIndex]);
		SbVec3f p2 = chemData->getAtomCoordinates(chemData->residues[list[list.getLength()-1]][caIndex]);
		SbLine line(centroid, centroid+axis);
		scfrom.set1Value(i,line.getClosestPoint(p1));
		scto.set1Value(i,line.getClosestPoint(p2));
	}
	delete[] M[0];
	delete[] M[1];
	delete[] M[2];
	delete[] M;
}

void ChemDisplay::computeSchematicTubeControlPoints(ChemData* chemData)
{
	// Adjust CA's of helix type residue projecting their position on the axis 
// of their cylinder
// Do the same for before and next CA's of non helix type residues

	//SbVec3f a,b,c,ca,cap,d,dp,e,h,f,g,o,p,l,r;
	SbVec3f a,b,c,ca,cap,d,dp,o,p,l,r;
	int i,j,k,caIndex,oIndex, nextResidueWithCAIndex;
	bool bFirstPoint, bNextCaFound;
	double temp;
	//SbVec2i fromTo;
	
	caIndex = SbResidue::RESIDUE_CONTROL_POINT_CA;
	oIndex = SbResidue::RESIDUE_CONTROL_POINT_O;

	bFirstPoint = true;

	for (i=0; i<chemData->residues.getNum()-1; i++)
	{
		if (!chemData->residues[i].checkControlPoints())
			continue;

		ca = chemData->getAtomCoordinates(
				chemData->residues[i][caIndex]);

		if (chemData->residues[i].residueType.getValue() == SbResidue::RESIDUE_TYPE_HELIX)
		{
			for (j=0;j<schematicCylinders.getLength();j++)
			{
				const SbIntList& list = *((SbIntList*)schematicCylinders.get(j));
				for (k=0;k<list.getLength();k++)
					if (i == list[k])
					{
						SbLine line(scfrom[j],scto[j]);
						ca = line.getClosestPoint(ca);
						j = schematicCylinders.getLength();
						k = list.getLength();
					}
			}
		}
		o = chemData->getAtomCoordinates(
				chemData->residues[i][oIndex]);

		bNextCaFound = true;
		nextResidueWithCAIndex = i+1;

		if (chemData->residues[nextResidueWithCAIndex][caIndex] < 0)
		{
			bNextCaFound = false;
			// find the next valid point
			for (j=i+2; j<chemData->residues.getNum(); j++)
			{
				if (chemData->residues[j][caIndex] >= 0)
				{
					bNextCaFound = true;
					nextResidueWithCAIndex = j;
					break;
				}
			}
		}

		if (!bNextCaFound)
			continue;

		cap = chemData->getAtomCoordinates(chemData->residues[nextResidueWithCAIndex][caIndex]);

		if (chemData->residues[nextResidueWithCAIndex].residueType.getValue() == SbResidue::RESIDUE_TYPE_HELIX)
		{
			for (j=0;j<schematicCylinders.getLength();j++)
			{
				const SbIntList& list = *((SbIntList*)schematicCylinders.get(j));
				for (k=0;k<list.getLength();k++)
					if (nextResidueWithCAIndex == list[k])
					{
						SbLine line(scfrom[j],scto[j]);
						cap = line.getClosestPoint(cap);
						j = schematicCylinders.getLength();
						k = list.getLength();
					}
			}

		}
		
		a = cap - ca;
		b = o - ca;					//B is vector CAi to Oi
		c = a.cross(b);
		d = c.cross(a);
		c.normalize();
		d.normalize();

		p = ca + a*0.5f; // midpoint

		d *= (0.5f * 3.0f);

		l = p - d;
		r = p + d;
		
		if (bFirstPoint)
		{
			scpl.set1Value(i,l);
			scpr.set1Value(i,r);

			dp = d;
			bFirstPoint = false;
		}
		else // the other points
		{
			temp = dp.dot(d);

			// if the angle is > 90 or < -90 the cos is minor than 0
			if (temp < 0.0) // flip the line segment defined in the previous step
			{
				scpl.set1Value(i,r);
				scpr.set1Value(i,l);

				dp = -d;

			}
			else
			{
					scpl.set1Value(i,l);
					scpr.set1Value(i,r);

					dp = d;
			}
		}
	}

// Compute a path formed by central points of each residue

	for (i=0;i<scpl.getNum();i++)
	{
		if (pIndex[i] >= 0)
			scc.set1Value(i,scpl[i] + (scpr[i]-scpl[i])*0.5f);		
	}

}

void ChemDisplay::computeSchematic(SoAction *action)
{
	cleanSchematic();

	ChemData *chemData = (ChemData*)ChemBaseDataElement::get(action->getState());
	if (chemData == NULL)
		return;

	ChemDisplayParam *cdp = (ChemDisplayParam*)ChemDisplayParamElement::get(action->getState());
	if (cdp == NULL)
		return;

	if (globalNumberOfResidues == 0)
		return;

	computeSchematicChains(chemData,cdp);
	computeSchematicCylinders(chemData);
	computeSchematicTubeControlPoints(chemData);
 
}

void ChemDisplay::computeSchematicTubeGeometry(bool doNormals /* = TRUE */)
{	
	int i,j,shape;

	shape = schematicTubeShapeSegments.getValue();

	SbVec3f lastC(0,0,0);
	SbVec3f lastDirection(0,0,1);
	SbRotation lastRotation;

	SbVec3f P,newP,C,direction, theTranslation, theScale;
	SbRotation theRotation(0,0,0,1);
	SbMatrix theVertexMatrix;

	theScale.setValue(1,1,1);
	theVertexMatrix.makeIdentity();
	
	for (i=0; i<tubePathCnt;i++)
	{
		C = tubePath[i];
	
		if (i>0)
		{
			direction = C - lastC;
			direction.normalize();
				
			theTranslation = lastC;
			theRotation = lastRotation;
			theRotation *= SbRotation(lastDirection, direction);

			theVertexMatrix.setTransform(theTranslation,theRotation,theScale);

			for (j=0;j<shape;j++)
			{
				P = scTubeVertices[j];
			
				theVertexMatrix.multVecMatrix(P,newP);
				scvtx[j].set1Value(i-1,newP);
				
        if (doNormals)
        {
				  P.normalize();
				  theVertexMatrix.multDirMatrix(P,newP);
				  scnrm[j].set1Value(i-1,newP);
        }
			}
			// special case, last shape
			if (i == tubePathCnt-1)
			{
				// same direction	
				theTranslation = C;
				// same rotation

				theVertexMatrix.setTransform(theTranslation,theRotation,theScale);

				for (j=0;j<shape;j++)
				{
					P = scTubeVertices[j];
				
					theVertexMatrix.multVecMatrix(P,newP);
					scvtx[j].set1Value(i,newP);
					
          if (doNormals)
          {
					  P.normalize();
					  theVertexMatrix.multDirMatrix(P,newP);
					  scnrm[j].set1Value(i,newP);
          }
				}
			}			

			lastDirection = direction;
		}
	
		lastRotation = theRotation;
		lastC = C;
	}
}


void ChemDisplay::renderSchematicTube()
{	
	int i,j;

	//glPointSize(2);
	/*
	glDisable(GL_LIGHTING);
	glBegin(GL_LINE_STRIP);
	for (i=0; i<tubePath.getNum(); i++)
		glVertex3fv(tubePath[i].getValue());
	glEnd();
	glEnable(GL_LIGHTING);
	//return;
	*/
	
//
	//glPointSize(2);
	/*
	glDisable(GL_LIGHTING);
	
	//glBegin(GL_POINTS);
	glBegin(GL_LINE_STRIP);
	for (i=0;i<schematicTubeShapeSegments.getValue();i++)
	{
			glColor3f(0,1*((double)i)/schematicTubeShapeSegments.getValue(),0);
			glVertex3fv(scvtx[i][0].getValue());
	}
			
	glVertex3fv(scvtx[0][0].getValue());
	glEnd();
	
	glEnable(GL_LIGHTING);
	//return;
	*/
	//
	
	for (i=0; i<schematicTubeShapeSegments.getValue()-1;i++)
	{
		glBegin(GL_TRIANGLE_STRIP);
		for (j=0;j<tubePathCnt;j++)
		{
				glNormal3fv(scnrm[i][j].getValue());
				glVertex3fv(scvtx[i][j].getValue());
				glNormal3fv(scnrm[i+1][j].getValue());
				glVertex3fv(scvtx[i+1][j].getValue());
		}	
		glEnd();
	}
	
	glBegin(GL_TRIANGLE_STRIP);
	for (j=0;j<tubePathCnt;j++)
	{
			glNormal3fv(scnrm[i][j].getValue());
			glVertex3fv(scvtx[i][j].getValue());
			glNormal3fv(scnrm[0][j].getValue());
			glVertex3fv(scvtx[0][j].getValue());
	}	
	glEnd();

}

void ChemDisplay::computeSchematicArrowGeometry(int residue,bool ace/*=false*/,bool doNormals/*=true*/)
{
	schematicElementSegmentation = schematicArrowTesselationSegments.getValue();

	ChemResidueStuff* res = NULL;	
	res = &residueAttributes[residue];
	assert(res != NULL);

	if (!res->isRibbonRenderable) return;

	float t = knots[TMIN];
	float s = (knots[TMAX]-knots[TMIN])/(schematicElementSegmentation-1)-0.000001;

	for (int j=0;j<schematicElementSegmentation;j++)
	{
		computeBasisCoefficients(t+j*s);
		SbVec3f A(0,0,0),B(0,0,0),C(0,0,0),D(0,0,0);
		for (int i=0;i<NCTLPTS;i++)
		{
			// for non-width uniform arrows use tl,tr,bl,br instead of sctl,sctr,scbl,scbr
			 const SbVec3f& a = sctl[res->indices[i]];
			 const SbVec3f& b = sctr[res->indices[i]];
			 const SbVec3f& c = scbl[res->indices[i]];
			 const SbVec3f& d = scbr[res->indices[i]];
			 
			 A += (a * basisCoefficients[i]);
			 B += (b * basisCoefficients[i]);
			 C += (c * basisCoefficients[i]);
			 D += (d * basisCoefficients[i]);
		}

		SbVec3f normal;
    if (ace || doNormals) // ace requires normals...
    {
		  normal = A-C; normal.normalize();
		  scruntl.set1Value(j,normal); 
		  scrunbl.set1Value(j,-normal);

		  normal = B-D; normal.normalize();
		  scruntr.set1Value(j,normal);		
		  scrunbr.set1Value(j,-normal);

		  normal = A-B; normal.normalize();
		  scrlntl.set1Value(j,normal);
		  scrlntr.set1Value(j,-normal);
		  
		  normal = C-D; normal.normalize();
		  scrlnbl.set1Value(j,normal);
		  scrlnbr.set1Value(j,-normal);
    }

		if (ace)
		{
			SbVec3f Tp = A+(B-A)*0.5f;
			SbVec3f Bp = C+(D-C)*0.5f;
			float f = 2.2f*(schematicElementSegmentation-1-j)/(float)(schematicElementSegmentation-1);
			scrtl.set1Value(j,Tp+f*scrlntl[j]);
			scrtr.set1Value(j,Tp+f*scrlntr[j]);
			scrbl.set1Value(j,Bp+f*scrlnbl[j]);
			scrbr.set1Value(j,Bp+f*scrlnbr[j]);
		}
		else
		{
			scrtl.set1Value(j,A);
			scrtr.set1Value(j,B);
			scrbl.set1Value(j,C);
			scrbr.set1Value(j,D);
		}

	}
}

void ChemDisplay::renderSchematicArrow()
{
	int i = 0;

	// top
	glBegin(GL_TRIANGLE_STRIP);
	for (i=0;i<schematicElementSegmentation;i++)
	{
		glNormal3fv(scruntl[i].getValue());
		glVertex3fv(scrtl[i].getValue());

		glNormal3fv(scruntr[i].getValue());
		glVertex3fv(scrtr[i].getValue());
	}
	glEnd();

	// bottom
	glBegin(GL_TRIANGLE_STRIP);
	for (i=0;i<schematicElementSegmentation;i++)
	{
		glNormal3fv(scrunbl[i].getValue());
		glVertex3fv(scrbl[i].getValue());

		glNormal3fv(scrunbr[i].getValue());
		glVertex3fv(scrbr[i].getValue());
	}
	glEnd();

	// left
	glBegin(GL_TRIANGLE_STRIP);
	for (i=0;i<schematicElementSegmentation;i++)
	{
		glNormal3fv(scrlntl[i].getValue());
		glVertex3fv(scrtl[i].getValue());

		glNormal3fv(scrlnbl[i].getValue());
		glVertex3fv(scrbl[i].getValue());
	}
	glEnd();
	
	// right
	glBegin(GL_TRIANGLE_STRIP);
	for (i=0;i<schematicElementSegmentation;i++)
	{
		glNormal3fv(scrlntr[i].getValue());
		glVertex3fv(scrtr[i].getValue());

		glNormal3fv(scrlnbr[i].getValue());
		glVertex3fv(scrbr[i].getValue());
	}
	glEnd();
}
/*
void ChemDisplay::renderSchematicCylinders(SoState *state)
{
	int i;
	
	SbVec2i fromTo;fromTo.setValue(-1,-1);
	ChemColor* cc = ChemColorElement::get(state);
	ChemDisplayParam* cdp = ChemDisplayParamElement::get(state);
	
	beginCylinders(ChemDisplayParam::BONDCYLINDER_ARRAY_FLATCAP,schematicUnitCylinder);

	
	for (i=0;i<schematicCylinders.getLength();i++)
	{
		switch(cc->schematicColorBinding.getValue())
		{
			case ChemColor::SCHEMATIC_OVERALL:
			{
				glColor3fv(cc->schematicColor[0].getValue());
				break;
			}
			case ChemColor::SCHEMATIC_PER_SECONDARY_TYPE:
			{
				glColor3fv(cc->schematicColor[schematicCylinderIndex[0]].getValue());
				break;
			}
			case ChemColor::SCHEMATIC_PER_ELEMENT:
			{
				glColor3fv(cc->schematicColor[schematicCylinderIndex[i+1]].getValue());
				break;
			}
			default: break;
		}

		schematicUnitCylinder->render(scfrom[i], scto[i], 
		schematicCylinderRadius.getValue(), true,false,ChemDisplayParam::BONDCYLINDER_ARRAY_FLATCAP,fromTo,this,(SoGLRenderAction*)state->getAction());
	}

	endCylinders(ChemDisplayParam::BONDCYLINDER_ARRAY_FLATCAP);
}

void ChemDisplay::renderSchematicArrows(SoState *state)
{
	int i,j;

	ChemColor* cc = ChemColorElement::get(state);
	ChemDisplayParam* cdp = ChemDisplayParamElement::get(state);

	for (i=0;i<schematicArrows.getLength();i++)
	{
		switch(cc->schematicColorBinding.getValue())
		{
			case ChemColor::SCHEMATIC_OVERALL:
			{
				glColor3fv(cc->schematicColor[0].getValue());
				break;
			}
			case ChemColor::SCHEMATIC_PER_SECONDARY_TYPE:
			{
				glColor3fv(cc->schematicColor[schematicArrowIndex[0]].getValue());
				break;
			}
			case ChemColor::SCHEMATIC_PER_ELEMENT:
			{
				glColor3fv(cc->schematicColor[schematicArrowIndex[i+1]].getValue());
				break;
			}
			default: break;
		}

		const SbIntList& list = *((SbIntList*)schematicArrows.get(i));
		for(j=0;j<list.getLength()-1;j++)
		{
			   computeSchematicArrowGeometry(list[j]);
				 renderSchematicArrow();
		}
		//draw ace
		computeSchematicArrowGeometry(list[j],true);
		renderSchematicArrow();
	}
}

void ChemDisplay::renderSchematicTubes(SoState *state)
{
	int i;

	ChemData* chemData = (ChemData*) ChemBaseDataElement::get(state);
	ChemColor* cc = ChemColorElement::get(state);
	ChemDisplayParam* cdp = ChemDisplayParamElement::get(state);
	
	for (i=0;i<schematicTubes.getLength();i++)
	{
		switch(cc->schematicColorBinding.getValue())
		{
			case ChemColor::SCHEMATIC_OVERALL:
			{
				glColor3fv(cc->schematicColor[0].getValue());
				break;
			}
			case ChemColor::SCHEMATIC_PER_SECONDARY_TYPE:
			{
				glColor3fv(cc->schematicColor[schematicTubeIndex[0]].getValue());
				break;
			}
			case ChemColor::SCHEMATIC_PER_ELEMENT:
			{
				glColor3fv(cc->schematicColor[schematicTubeIndex[i]].getValue());
				break;
			}
			default: break;
		}
				
		const SbIntList& list = *((SbIntList*)schematicTubes.get(i));
		computeTubePath(list);
		computeSchematicTubeGeometry();
		renderSchematicTube();
	}
}
*/
void ChemDisplay::computeTubePath(const SbIntList &chain)
{
	ChemResidueStuff* res = NULL;
	int j,k,l;
	float t,s;
	SbVec3f C;

	schematicElementSegmentation = schematicTubeTesselationSegments.getValue();
	tubePathCnt = 0;
	for(j=0;j<chain.getLength();j++)
	{
		k = chain[j];
		res = &residueAttributes[chain[j]];
		assert(res != NULL);
		if (!res->isRibbonRenderable) continue;
		
		t = knots[TMIN];
		s = (knots[TMAX]-knots[TMIN])/(schematicElementSegmentation-1)-0.000001;
		for (k=0;k<schematicElementSegmentation-1;k++) // -1, because the last segment is the next first
		{
			computeBasisCoefficients(t+k*s);
			C.setValue(0.0f,0.0f,0.0f);
			for (l=0;l<NCTLPTS;l++)
			{
				const SbVec3f &c = scc[res->indices[l]];
				C += (c * basisCoefficients[l]);
			}
			// add c
			tubePath.set1Value(tubePathCnt++,C);
		}
	}

	// compute the last C of the tube
	computeBasisCoefficients(t+k*s);
	C.setValue(0,0,0);
	for (l=0;l<NCTLPTS;l++)
	{
		const SbVec3f &c = scc[res->indices[l]];
		C += (c * basisCoefficients[l]);
	}
	// add c
	tubePath.set1Value(tubePathCnt++,C);
}

int32_t ChemDisplay::getNumberOfSchematics() const
{
  return globalNumberOfSchematics;
}
// <-- schematic

int32_t ChemDisplay::getNumberOfResidues(SoAction *action) const
{
	ChemData *chemData = (ChemData*)ChemBaseDataElement::get(action->getState());
	if (chemData == NULL)
	{
		return 0;
	}

	int32_t sum = 0;
	int32_t numResidueLoops = residueIndex.getNum();
	int32_t residueLoop, residueStart, residueEnd;

	for (residueLoop = 0; residueLoop < numResidueLoops; residueLoop++)
	{
		residueIndex[residueLoop].getValue(residueStart, residueEnd);
		if (residueEnd == CHEM_DISPLAY_USE_REST_OF_RESIDUES)
		{
			sum += globalNumberOfResidues - residueStart;
		}
		else
		{
			sum += residueEnd;
		}
	}
	return sum;
}

int32_t ChemDisplay::getNumberOfResidueLabels(SoAction *action) const
{
	ChemData *chemData = (ChemData*)ChemBaseDataElement::get(action->getState());
	if (chemData == NULL)
	{
		return 0;
	}

	int32_t sum = 0;
	int32_t numResidueLoops = residueIndex.getNum();
	int32_t residueLoop, residueStart, residueEnd;

	for (residueLoop = 0; residueLoop < numResidueLoops; residueLoop++)
	{
		residueIndex[residueLoop].getValue(residueStart, residueEnd);
		if (residueEnd == CHEM_DISPLAY_USE_REST_OF_RESIDUES)
		{
			sum += globalNumberOfResidues - residueStart;
		}
		else
		{
			sum += residueEnd;
		}
	}
	return sum;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Implements rayPicking.
//
// Use: protected

void ChemDisplay::pickResiduesAsWireframe(SoRayPickAction *action)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::pickResiduesAsWireframe\n" );
#endif

	SbVec3f	coords1, coords2;
	SbLine	pickLine;

	SoState *state = action->getState();

	ChemData *chemData = (ChemData*)ChemBaseDataElement::get(state);
	//ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);
	ChemColor *chemColor = ChemColorElement::get(state);
	
	computeObjectSpaceRay(action);
	pickLine = action->getLine();

	ChemResidueStuff *res = NULL;
	RESIDUELOOP_START(residueIndex)
	
		res = &residueAttributes[theResidue];
		if (!res->isStickRenderable) continue;
	
		coords1 = chemData->getAtomCoordinates(res->from).getValue();
		coords2 = chemData->getAtomCoordinates(res->to).getValue();

			checkResidueForPick(action, pickLine, chemData, chemColor,
				this, theResidue, coords1, coords2);
	RESIDUELOOP_END
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Check for a pick with a line (residue) from a wireframe model.  If
//	  there is a pick, fill out the pickedPoint information.
//
// Use: static

static void
checkResidueForPick(SoRayPickAction *action, SbLine &pickLine, 
	ChemData *chemData, ChemColor *chemColor, ChemDisplay *chemDisplay,
	int32_t theResidue,
	SbVec3f &coords1, SbVec3f &coords2)
//
////////////////////////////////////////////////////////////////////////
{
	SbVec3f			point;
	SbVec3f			norm;
	SoPickedPoint	*pp;
	ChemDetail		*detail;

	if (action->intersect(coords1, coords2, point) &&
		(pp = action->addIntersection(point)) != NULL) {

		// The "normal" will be in the direction of the ray pick
		norm = pickLine.getDirection();
		norm.negate();
		norm.normalize();
		pp->setObjectNormal(norm);

		// The texture coords will be zero
		pp->setObjectTextureCoords(SbVec4f(0.0, 0.0, 0.0, 0.0));

		// Material index
		switch (chemColor->residueColorBinding.getValue())
		{
		  case ChemColor::RESIDUE_OVERALL:
			{
				pp->setMaterialIndex(0);
				break;
			}
		  case ChemColor::RESIDUE_PER_INDEX:
			{
				int index = 0;
				if (chemData->residues[theResidue].getIndex() >= 0)
				index = chemData->residueColorIndex[chemData->residues[theResidue].getIndex()];
				pp->setMaterialIndex(index);
				break;
			}
		  case ChemColor::RESIDUE_PER_CHAIN:
			{
				pp->setMaterialIndex(chemData->residueChainIndex[chemData->residues[theResidue].getChain()]);
				break;
			}
			
		  default:
			break;
		}

		// Create a detail for the specific shape
		detail = new ChemDetail;
		detail->setResidueIndex(theResidue);
		pp->setDetail(detail, chemDisplay);
	}
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Implements rayPicking.
//
// Use: protected

void ChemDisplay::pickResiduesAsCylinders(SoRayPickAction *action)
//
////////////////////////////////////////////////////////////////////////
{
	SbLine			pickLine;
	float			rad;
	float			halfHeight;
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

	SoState *state = action->getState();
	ChemData *chemData = (ChemData*)ChemBaseDataElement::get(state);
	ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);
	ChemColor *chemColor = ChemColorElement::get(state);

	int displayStyle = cdp->residueCylinderDisplayStyle.getValue();
	rad = cdp->residueCylinderRadius.getValue();

	SbBool doCaps = (displayStyle % 3 > 0);

	yAxis.setValue(0.0, 1.0, 0.0);

	// Compute the picking ray in our current object space
	computeObjectSpaceRay(action);
	pickLine = action->getLine();

	ChemResidueStuff *res = NULL;
	RESIDUELOOP_START(residueIndex)
		numHits = 0;
		res = &residueAttributes[theResidue];
		if (!res->isStickRenderable) continue;
		// Get size of this cylinder
		coords1 = chemData->getAtomCoordinates(res->from).getValue();
		coords2 = chemData->getAtomCoordinates(res->to).getValue();
		tcoord = coords2 - coords1;
		halfHeight = tcoord.length() * 0.5;
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
						* (1.0 / (2.0 * M_PI)) + 0.5,
						(tEnterPoint[1] + halfHeight) /
						(2.0 * halfHeight),
						0.0, 1.0);
					pp->setObjectTextureCoords(texCoord);

					// Material index
					switch (chemColor->residueColorBinding.getValue())
					{
						case ChemColor::RESIDUE_OVERALL:
						{
							pp->setMaterialIndex(0);
							break;
						}
						case ChemColor::RESIDUE_PER_INDEX:
						{
							int index = 0;
							if (chemData->residues[theResidue].getIndex() >= 0)
							index = chemData->residueColorIndex[chemData->residues[theResidue].getIndex()];
							pp->setMaterialIndex(index);
							break;
						}
						case ChemColor::RESIDUE_PER_CHAIN:
						{
							pp->setMaterialIndex(chemData->residueChainIndex[chemData->residues[theResidue].getChain()]);
							break;
						}
						
						default:
						break;
					}

					detail = new ChemDetail();
					detail->setResidueIndex(theResidue);
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

					// Material index
					switch (chemColor->residueColorBinding.getValue())
					{
						case ChemColor::RESIDUE_OVERALL:
						{
							pp->setMaterialIndex(0);
							break;
						}
						case ChemColor::RESIDUE_PER_INDEX:
						{
							int index = 0;
							if (chemData->residues[theResidue].getIndex() >= 0)
							index = chemData->residueColorIndex[chemData->residues[theResidue].getIndex()];
							pp->setMaterialIndex(index);
							break;
						}
						case ChemColor::RESIDUE_PER_CHAIN:
						{
							pp->setMaterialIndex(chemData->residueChainIndex[chemData->residues[theResidue].getChain()]);
							break;
						}
						
						default:
						break;
					}

					texCoord.setValue(atan2f(tExitPoint[0], tExitPoint[2])
						* (1.0 / (2.0 * M_PI)) + 0.5,
						(tExitPoint[1] + halfHeight) /
						(2.0 * halfHeight),
						0.0, 1.0);
					pp->setObjectTextureCoords(texCoord);

					detail = new ChemDetail();
					detail->setResidueIndex(theResidue);
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
						texCoord.setValue(0.5 + tEnterPoint[0] / (2.0 * rad),
							0.5 - tEnterPoint[2] / (2.0 * rad),
							0.0, 1.0);
						pp->setObjectTextureCoords(texCoord);

						// Material index
						switch (chemColor->residueColorBinding.getValue())
						{
							case ChemColor::RESIDUE_OVERALL:
							{
								pp->setMaterialIndex(0);
								break;
							}
							case ChemColor::RESIDUE_PER_INDEX:
							{
								int index = 0;
								if (chemData->residues[theResidue].getIndex() >= 0)
								index = chemData->residueColorIndex[chemData->residues[theResidue].getIndex()];
								pp->setMaterialIndex(index);
								break;
							}
							case ChemColor::RESIDUE_PER_CHAIN:
							{
								pp->setMaterialIndex(chemData->residueChainIndex[chemData->residues[theResidue].getChain()]);
								break;
							}
							
							default:
							break;
						}

						detail = new ChemDetail();
						detail->setResidueIndex(theResidue);
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
						texCoord.setValue(0.5 + tEnterPoint[0] / (2.0 * rad),
							0.5 + tEnterPoint[2] / (2.0 * rad),
							0.0, 1.0);
						pp->setObjectTextureCoords(texCoord);

						// Material index
						switch (chemColor->residueColorBinding.getValue())
						{
							case ChemColor::RESIDUE_OVERALL:
							{
								pp->setMaterialIndex(0);
								break;
							}
							case ChemColor::RESIDUE_PER_INDEX:
							{
								int index = 0;
								if (chemData->residues[theResidue].getIndex() >= 0)
								index = chemData->residueColorIndex[chemData->residues[theResidue].getIndex()];
								pp->setMaterialIndex(index);
								break;
							}
							case ChemColor::RESIDUE_PER_CHAIN:
							{
								pp->setMaterialIndex(chemData->residueChainIndex[chemData->residues[theResidue].getChain()]);
								break;
							}
							
							default:
							break;
						}
						detail = new ChemDetail();
						detail->setResidueIndex(theResidue);
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

					// Material index
					switch (chemColor->residueColorBinding.getValue())
					{
						case ChemColor::RESIDUE_OVERALL:
						{
							pp->setMaterialIndex(0);
							break;
						}
						case ChemColor::RESIDUE_PER_INDEX:
						{
							int index = 0;
							if (chemData->residues[theResidue].getIndex() >= 0)
							index = chemData->residueColorIndex[chemData->residues[theResidue].getIndex()];
							pp->setMaterialIndex(index);
							break;
						}
						case ChemColor::RESIDUE_PER_CHAIN:
						{
							pp->setMaterialIndex(chemData->residueChainIndex[chemData->residues[theResidue].getChain()]);
							break;
						}
						
						default:
						break;
					}
					detail = new ChemDetail;
					detail->setResidueIndex(theResidue);
					pp->setDetail(detail, this);
				}
				if (action->isBetweenPlanes(exitPoint) &&
					(pp = action->addIntersection(exitPoint)) != NULL) {

					normal = exitPoint - coords2;

					texCoord.setValue(0.5 + normal[0] / (2.0 * rad),
						0.5 - normal[2] / (2.0 * rad),
						0.0, 1.0);
					pp->setObjectTextureCoords(texCoord);

					normal.normalize();
					pp->setObjectNormal(normal);

					// Material index
					switch (chemColor->residueColorBinding.getValue())
					{
						case ChemColor::RESIDUE_OVERALL:
						{
							pp->setMaterialIndex(0);
							break;
						}
						case ChemColor::RESIDUE_PER_INDEX:
						{
							int index = 0;
							if (chemData->residues[theResidue].getIndex() >= 0)
							index = chemData->residueColorIndex[chemData->residues[theResidue].getIndex()];
							pp->setMaterialIndex(index);
							break;
						}
						case ChemColor::RESIDUE_PER_CHAIN:
						{
							pp->setMaterialIndex(chemData->residueChainIndex[chemData->residues[theResidue].getChain()]);
							break;
						}
						
						default:
						break;
					}

					detail = new ChemDetail;
					detail->setResidueIndex(theResidue);
					pp->setDetail(detail, this);
				}
			}
			else {
				sph.setValue(coords1, rad);
				if (sph.intersect(pickLine, enterPoint, exitPoint)) {
					if (action->isBetweenPlanes(enterPoint) &&
						(pp = action->addIntersection(enterPoint)) != NULL) {

						normal = enterPoint - coords1;

						texCoord.setValue(0.5 + normal[0] / (2.0 * rad),
							0.5 + normal[2] / (2.0 * rad),
							0.0, 1.0);
						pp->setObjectTextureCoords(texCoord);

						normal.normalize();
						pp->setObjectNormal(normal);

						// Material index
						switch (chemColor->residueColorBinding.getValue())
						{
							case ChemColor::RESIDUE_OVERALL:
							{
								pp->setMaterialIndex(0);
								break;
							}
							case ChemColor::RESIDUE_PER_INDEX:
							{
								int index = 0;
								if (chemData->residues[theResidue].getIndex() >= 0)
								index = chemData->residueColorIndex[chemData->residues[theResidue].getIndex()];
								pp->setMaterialIndex(index);
								break;
							}
							case ChemColor::RESIDUE_PER_CHAIN:
							{
								pp->setMaterialIndex(chemData->residueChainIndex[chemData->residues[theResidue].getChain()]);
								break;
							}
							
							default:
							break;
						}
						detail = new ChemDetail;
						detail->setResidueIndex(theResidue);
						pp->setDetail(detail, this);
					}
					if (action->isBetweenPlanes(exitPoint) &&
						(pp = action->addIntersection(exitPoint)) != NULL) {

						normal = exitPoint - coords1;

						texCoord.setValue(0.5 + normal[0] / (2.0 * rad),
							0.5 + normal[2] / (2.0 * rad),
							0.0, 1.0);
						pp->setObjectTextureCoords(texCoord);

						normal.normalize();
						pp->setObjectNormal(normal);

						// Material index
						switch (chemColor->residueColorBinding.getValue())
						{
							case ChemColor::RESIDUE_OVERALL:
							{
								pp->setMaterialIndex(0);
								break;
							}
							case ChemColor::RESIDUE_PER_INDEX:
							{
								int index = 0;
								if (chemData->residues[theResidue].getIndex() > 0)
								index = chemData->residueColorIndex[chemData->residues[theResidue].getIndex()];
								pp->setMaterialIndex(index);
								break;
							}
							case ChemColor::RESIDUE_PER_CHAIN:
							{
								pp->setMaterialIndex(chemData->residueChainIndex[chemData->residues[theResidue].getChain()]);
								break;
							}
							
							default:
							break;
						}

						detail = new ChemDetail;
						detail->setResidueIndex(theResidue);
						pp->setDetail(detail, this);
					}
				}
			}
		}
	RESIDUELOOP_END
}

// Ray intersects triangle code adapted from ...
// http://www.lighthouse3d.com/opengl/maths/index.php3?raytriint

static SbBool
rayIntersectsTriangle(const SbLine &line, const SbVec3f &v0, const SbVec3f &v1, const SbVec3f &v2, SbVec3f &point)
{
	SbVec3f p = line.getPosition();
	SbVec3f d = line.getDirection();

	SbVec3f e1,e2,h,s,q;
	float a,f,u,v;
	
	e1 = v1 - v0;
	e2 = v2 - v0;
	h = d.cross(e2);
	a = e1.dot(h);
	
	if (a > -0.00001 && a < 0.00001)
		return(false);
	
	f = 1.0f/a;
	s = p - v0;
	u = f * s.dot(h);
	
	if (u < 0.0 || u > 1.0)
		return(false);
	
	q = s.cross(e1);
	v = f * d.dot(q);
	if (v < 0.0 || u + v > 1.0)
		return(false);

	point =  (1-u-v)*v0 + u*v1 + v*v2;
	
	return(true);
}

// Ray box code based on the following paper:
// http://www.cs.utah.edu/~rmorley/pubs/box.pdf
/*
static bool aabbRayIntersect(const SbVec3f& parameters[2], float halfsize,
							 int sign[3], const SbVec3f &inv_direction,
							 float &t0, float &t1)
{
	float tmin, tmax, tymin, tymax, tzmin, tzmax;
	tmin = (parameters[sign[0]][0] - r.origin[0]) * r.inv_direction[0];
	tmax = (parameters[1-sign[0]][0] - r.origin[0]) * r.inv_direction[0];
	tymin = (parameters[sign[1]][1] - r.origin[1]) * r.inv_direction[1];
	tymax = (parameters[1-sign[1]][1] - r.origin[1]) * r.inv_direction[1];
	if ( (tmin > tymax) || (tymin > tmax) )
	return false;
	if (tymin > tmin)
	tmin = tymin;
	if (tymax < tmax)
	tmax = tymax;
	tzmin = (parameters[r.sign[2]][2] - r.origin[2]) * r.inv_direction[2];
	tzmax = (parameters[1-r.sign[2]][2] - r.origin[2]) * r.inv_direction[2];
	if ( (tmin > tzmax) || (tzmin > tmax) )
	return false;
	if (tzmin > tmin)
	tmin = tzmin;
	if (tzmax < tmax)
	tmax = tzmax;
	return ( (tmin < t1) && (tmax > t0) );
}
*/
void ChemDisplay::pickResiduesAsFlatRibbons(SoRayPickAction *action)
{
#ifdef DEBUG
printf ("In ChemDisplay::pickResiduesAsFlatRibbons\n" );
#endif

	SbLine	pickLine;
	SbVec3f			point;
	SbVec3f			norm;
	SoPickedPoint	*pp = NULL;
	ChemDetail *detail = NULL;

	SoState *state = action->getState();

	ChemData *chemData = (ChemData*) ChemBaseDataElement::get(state);
	//ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);
	ChemColor *chemColor = ChemColorElement::get(state);
	
	computeObjectSpaceRay(action);
	pickLine = action->getLine();

	ChemResidueStuff* res = NULL;

	RESIDUELOOP_START(residueIndex)
	
		res = &residueAttributes[theResidue];
		assert(res != NULL);

		if (!res->isRibbonRenderable) continue;
		
		res->segments = 3;
		computeFlatRibbonGeometry(theResidue, false);

		if ((
			rayIntersectsTriangle(pickLine,fr[0],fl[0],fr[1],point) ||
			rayIntersectsTriangle(pickLine,fl[0],fr[1],fl[1],point) ||
			rayIntersectsTriangle(pickLine,fr[1],fl[1],fr[2],point) ||
			rayIntersectsTriangle(pickLine,fl[1],fr[2],fl[2],point)) 
			&& (pp = action->addIntersection(point)) != NULL)
		{
			pp->setObjectTextureCoords(SbVec4f(0,0,0,0));

			norm = pickLine.getDirection();
			norm.negate();
			norm.normalize();
			pp->setObjectNormal(norm);

			// Material index
			switch (chemColor->residueColorBinding.getValue())
			{
				case ChemColor::RESIDUE_OVERALL:
				{
					pp->setMaterialIndex(0);
					break;
				}
				case ChemColor::RESIDUE_PER_INDEX:
				{
					int index = 0;
					if (chemData->residues[theResidue].getIndex() > 0)
					index = chemData->residueColorIndex[chemData->residues[theResidue].getIndex()];
					pp->setMaterialIndex(index);
					break;
				}
				case ChemColor::RESIDUE_PER_CHAIN:
				{
					pp->setMaterialIndex(chemData->residueChainIndex[chemData->residues[theResidue].getChain()]);
					break;
				}
				
				default:
				break;
			}

			detail = new ChemDetail;
			detail->setResidueIndex(theResidue);
			pp->setDetail(detail, this);
		}

	RESIDUELOOP_END	
}

void ChemDisplay::pickResiduesAsSolidRibbons(SoRayPickAction *action)
{
#ifdef DEBUG
printf ("In ChemDisplay::pickResiduesAsSolidRibbons\n" );
#endif
	int32_t	theResidue;
	SbLine	pickLine;
	SbVec3f			point;
	SbVec3f			norm;
	SoPickedPoint	*pp = NULL;
	ChemDetail *detail = NULL;

	SoState *state = action->getState();

	ChemData *chemData = (ChemData*) ChemBaseDataElement::get(state);
	//ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);
	ChemColor *chemColor = ChemColorElement::get(state);
	
	computeObjectSpaceRay(action);
	pickLine = action->getLine();

	ChemResidueStuff* res = NULL;
  
	RESIDUELOOP_START(residueIndex)
	
		res = &residueAttributes[theResidue];
		assert(res != NULL);

		if (!res->isRibbonRenderable) continue;
		res->segments = 3;
		computeSolidRibbonGeometry(theResidue,false);// dont compute normals

		if ((
			// top
			rayIntersectsTriangle(pickLine,rtr[0],rtl[0],rtr[1],point) ||
			rayIntersectsTriangle(pickLine,rtl[0],rtr[1],rtl[1],point) ||
			rayIntersectsTriangle(pickLine,rtr[1],rtl[1],rtr[2],point) ||
			rayIntersectsTriangle(pickLine,rtl[1],rtr[2],rtl[2],point) ||
			// bottom
			rayIntersectsTriangle(pickLine,rbr[0],rbl[0],rbr[1],point) ||
			rayIntersectsTriangle(pickLine,rbl[0],rbr[1],rbl[1],point) ||
			rayIntersectsTriangle(pickLine,rbr[1],rbl[1],rbr[2],point) ||
			rayIntersectsTriangle(pickLine,rbl[1],rbr[2],rbl[2],point) ||
			// left
			rayIntersectsTriangle(pickLine,rtl[0],rbl[0],rtl[1],point) ||
			rayIntersectsTriangle(pickLine,rbl[0],rtl[1],rbl[1],point) ||
			rayIntersectsTriangle(pickLine,rtl[1],rbl[1],rtl[2],point) ||
			rayIntersectsTriangle(pickLine,rbl[1],rtl[2],rbl[2],point) ||
			// right
			rayIntersectsTriangle(pickLine,rtr[0],rbr[0],rtr[1],point) ||
			rayIntersectsTriangle(pickLine,rbr[0],rtr[1],rbr[1],point) ||
			rayIntersectsTriangle(pickLine,rtr[1],rbr[1],rtr[2],point) ||
			rayIntersectsTriangle(pickLine,rbr[1],rtr[2],rbr[2],point)
			) && ( (pp = action->addIntersection(point)) != NULL))
		{
			pp->setObjectTextureCoords(SbVec4f(0,0,0,0));

			norm = pickLine.getDirection();
			norm.negate();
			norm.normalize();
			pp->setObjectNormal(norm);

			// Material index
			switch (chemColor->residueColorBinding.getValue())
			{
				case ChemColor::RESIDUE_OVERALL:
				{
					pp->setMaterialIndex(0);
					break;
				}
				case ChemColor::RESIDUE_PER_INDEX:
				{
					int index = 0;
					if (chemData->residues[theResidue].getIndex() > 0)
					index = chemData->residueColorIndex[chemData->residues[theResidue].getIndex()];
					pp->setMaterialIndex(index);
					break;
				}
				case ChemColor::RESIDUE_PER_CHAIN:
				{
					pp->setMaterialIndex(chemData->residueChainIndex[chemData->residues[theResidue].getChain()]);
					break;
				}
				
				default:
				break;
			}

			detail = new ChemDetail;
			detail->setResidueIndex(theResidue);
			pp->setDetail(detail, this);
		}

	RESIDUELOOP_END
}

// --> schematic
////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Implements rayPicking.
//
// Use: protected

void ChemDisplay::pickSchematics(SoRayPickAction *action)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::pickSchematics\n" );
#endif

	SbLine			pickLine;
	float			rad;
	float			halfHeight;
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

  int i, j;
	//SoState *state = action->getState();
	//ChemData *chemData = (ChemData*)ChemBaseDataElement::get(state);
	//ChemDisplayParam *cdp = ChemDisplayParamElement::get(state);
	//ChemColor *chemColor = ChemColorElement::get(state);

  int displayStyle = ChemDisplayParam::BONDCYLINDER_ARRAY_FLATCAP; // schematicCylinderDisplayStyle
	rad = schematicCylinderRadius.getValue();

	SbBool doCaps = (displayStyle % 3 > 0);

	yAxis.setValue(0.0, 1.0, 0.0);

	// Compute the picking ray in our current object space
	computeObjectSpaceRay(action);
	pickLine = action->getLine();

	ChemSchematicChain* schematic = NULL;

	SCHEMATICLOOP_START(schematicIndex)
    schematic = &schematicChains[theSchematic];
		numHits = 0;

    // --> cylinder selection
    for (i=0;i<schematic->cylinders.getLength();i++)
    {
		  // Get size of this cylinder
		  coords1 = scfrom[schematic->cylinders[i]];
		  coords2 = scto[schematic->cylinders[i]];
		  tcoord = coords2 - coords1;
		  halfHeight = tcoord.length() * 0.5;
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
						  * (1.0 / (2.0 * M_PI)) + 0.5,
						  (tEnterPoint[1] + halfHeight) /
						  (2.0 * halfHeight),
						  0.0, 1.0);
					  pp->setObjectTextureCoords(texCoord);
/*
					  // Material index
					  switch (chemColor->residueColorBinding.getValue())
					  {
						  case ChemColor::RESIDUE_OVERALL:
						  {
							  pp->setMaterialIndex(0);
							  break;
						  }
						  case ChemColor::RESIDUE_PER_INDEX:
						  {
							  int index = 0;
							  if (chemData->residues[theResidue].getIndex() >= 0)
							  index = chemData->residueColorIndex[chemData->residues[theResidue].getIndex()];
							  pp->setMaterialIndex(index);
							  break;
						  }
						  case ChemColor::RESIDUE_PER_CHAIN:
						  {
							  pp->setMaterialIndex(chemData->residueChainIndex[chemData->residues[theResidue].getChain()]);
							  break;
						  }
						  
						  default:
						  break;
					  }
*/
					  detail = new ChemDetail();
					  detail->setSchematicIndex(theSchematic);
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
/*
					  // Material index
					  switch (chemColor->residueColorBinding.getValue())
					  {
						  case ChemColor::RESIDUE_OVERALL:
						  {
							  pp->setMaterialIndex(0);
							  break;
						  }
						  case ChemColor::RESIDUE_PER_INDEX:
						  {
							  int index = 0;
							  if (chemData->residues[theResidue].getIndex() >= 0)
							  index = chemData->residueColorIndex[chemData->residues[theResidue].getIndex()];
							  pp->setMaterialIndex(index);
							  break;
						  }
						  case ChemColor::RESIDUE_PER_CHAIN:
						  {
							  pp->setMaterialIndex(chemData->residueChainIndex[chemData->residues[theResidue].getChain()]);
							  break;
						  }
						  
						  default:
						  break;
					  }
*/
					  texCoord.setValue(atan2f(tExitPoint[0], tExitPoint[2])
						  * (1.0 / (2.0 * M_PI)) + 0.5,
						  (tExitPoint[1] + halfHeight) /
						  (2.0 * halfHeight),
						  0.0, 1.0);
					  pp->setObjectTextureCoords(texCoord);

					  detail = new ChemDetail();
					  detail->setSchematicIndex(theSchematic);
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
						  texCoord.setValue(0.5 + tEnterPoint[0] / (2.0 * rad),
							  0.5 - tEnterPoint[2] / (2.0 * rad),
							  0.0, 1.0);
						  pp->setObjectTextureCoords(texCoord);
/*
						  // Material index
						  switch (chemColor->residueColorBinding.getValue())
						  {
							  case ChemColor::RESIDUE_OVERALL:
							  {
								  pp->setMaterialIndex(0);
								  break;
							  }
							  case ChemColor::RESIDUE_PER_INDEX:
							  {
								  int index = 0;
								  if (chemData->residues[theResidue].getIndex() >= 0)
								  index = chemData->residueColorIndex[chemData->residues[theResidue].getIndex()];
								  pp->setMaterialIndex(index);
								  break;
							  }
							  case ChemColor::RESIDUE_PER_CHAIN:
							  {
								  pp->setMaterialIndex(chemData->residueChainIndex[chemData->residues[theResidue].getChain()]);
								  break;
							  }
							  
							  default:
							  break;
						  }
*/
						  detail = new ChemDetail();
						  detail->setSchematicIndex(theSchematic);
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
						  texCoord.setValue(0.5 + tEnterPoint[0] / (2.0 * rad),
							  0.5 + tEnterPoint[2] / (2.0 * rad),
							  0.0, 1.0);
						  pp->setObjectTextureCoords(texCoord);
/*
						  // Material index
						  switch (chemColor->residueColorBinding.getValue())
						  {
							  case ChemColor::RESIDUE_OVERALL:
							  {
								  pp->setMaterialIndex(0);
								  break;
							  }
							  case ChemColor::RESIDUE_PER_INDEX:
							  {
								  int index = 0;
								  if (chemData->residues[theResidue].getIndex() >= 0)
								  index = chemData->residueColorIndex[chemData->residues[theResidue].getIndex()];
								  pp->setMaterialIndex(index);
								  break;
							  }
							  case ChemColor::RESIDUE_PER_CHAIN:
							  {
								  pp->setMaterialIndex(chemData->residueChainIndex[chemData->residues[theResidue].getChain()]);
								  break;
							  }
							  
							  default:
							  break;
						  }
*/
						  detail = new ChemDetail();
						  detail->setSchematicIndex(theSchematic);
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
/*
					  // Material index
					  switch (chemColor->residueColorBinding.getValue())
					  {
						  case ChemColor::RESIDUE_OVERALL:
						  {
							  pp->setMaterialIndex(0);
							  break;
						  }
						  case ChemColor::RESIDUE_PER_INDEX:
						  {
							  int index = 0;
							  if (chemData->residues[theResidue].getIndex() >= 0)
							  index = chemData->residueColorIndex[chemData->residues[theResidue].getIndex()];
							  pp->setMaterialIndex(index);
							  break;
						  }
						  case ChemColor::RESIDUE_PER_CHAIN:
						  {
							  pp->setMaterialIndex(chemData->residueChainIndex[chemData->residues[theResidue].getChain()]);
							  break;
						  }
						  
						  default:
						  break;
					  }
*/
					  detail = new ChemDetail;
					  detail->setSchematicIndex(theSchematic);
					  pp->setDetail(detail, this);
				  }
				  if (action->isBetweenPlanes(exitPoint) &&
					  (pp = action->addIntersection(exitPoint)) != NULL) {

					  normal = exitPoint - coords2;

					  texCoord.setValue(0.5 + normal[0] / (2.0 * rad),
						  0.5 - normal[2] / (2.0 * rad),
						  0.0, 1.0);
					  pp->setObjectTextureCoords(texCoord);

					  normal.normalize();
					  pp->setObjectNormal(normal);
/*
					  // Material index
					  switch (chemColor->residueColorBinding.getValue())
					  {
						  case ChemColor::RESIDUE_OVERALL:
						  {
							  pp->setMaterialIndex(0);
							  break;
						  }
						  case ChemColor::RESIDUE_PER_INDEX:
						  {
							  int index = 0;
							  if (chemData->residues[theResidue].getIndex() >= 0)
							  index = chemData->residueColorIndex[chemData->residues[theResidue].getIndex()];
							  pp->setMaterialIndex(index);
							  break;
						  }
						  case ChemColor::RESIDUE_PER_CHAIN:
						  {
							  pp->setMaterialIndex(chemData->residueChainIndex[chemData->residues[theResidue].getChain()]);
							  break;
						  }
						  
						  default:
						  break;
					  }
*/
					  detail = new ChemDetail;
					  detail->setSchematicIndex(theSchematic);
					  pp->setDetail(detail, this);
				  }
			  }
			  else {
				  sph.setValue(coords1, rad);
				  if (sph.intersect(pickLine, enterPoint, exitPoint)) {
					  if (action->isBetweenPlanes(enterPoint) &&
						  (pp = action->addIntersection(enterPoint)) != NULL) {

						  normal = enterPoint - coords1;

						  texCoord.setValue(0.5 + normal[0] / (2.0 * rad),
							  0.5 + normal[2] / (2.0 * rad),
							  0.0, 1.0);
						  pp->setObjectTextureCoords(texCoord);

						  normal.normalize();
						  pp->setObjectNormal(normal);
/*
						  // Material index
						  switch (chemColor->residueColorBinding.getValue())
						  {
							  case ChemColor::RESIDUE_OVERALL:
							  {
								  pp->setMaterialIndex(0);
								  break;
							  }
							  case ChemColor::RESIDUE_PER_INDEX:
							  {
								  int index = 0;
								  if (chemData->residues[theResidue].getIndex() >= 0)
								  index = chemData->residueColorIndex[chemData->residues[theResidue].getIndex()];
								  pp->setMaterialIndex(index);
								  break;
							  }
							  case ChemColor::RESIDUE_PER_CHAIN:
							  {
								  pp->setMaterialIndex(chemData->residueChainIndex[chemData->residues[theResidue].getChain()]);
								  break;
							  }
							  
							  default:
							  break;
						  }
*/
						  detail = new ChemDetail;
						  detail->setSchematicIndex(theSchematic);
						  pp->setDetail(detail, this);
					  }
					  if (action->isBetweenPlanes(exitPoint) &&
						  (pp = action->addIntersection(exitPoint)) != NULL) {

						  normal = exitPoint - coords1;

						  texCoord.setValue(0.5 + normal[0] / (2.0 * rad),
							  0.5 + normal[2] / (2.0 * rad),
							  0.0, 1.0);
						  pp->setObjectTextureCoords(texCoord);

						  normal.normalize();
						  pp->setObjectNormal(normal);
/*
						  // Material index
						  switch (chemColor->residueColorBinding.getValue())
						  {
							  case ChemColor::RESIDUE_OVERALL:
							  {
								  pp->setMaterialIndex(0);
								  break;
							  }
							  case ChemColor::RESIDUE_PER_INDEX:
							  {
								  int index = 0;
								  if (chemData->residues[theResidue].getIndex() > 0)
								  index = chemData->residueColorIndex[chemData->residues[theResidue].getIndex()];
								  pp->setMaterialIndex(index);
								  break;
							  }
							  case ChemColor::RESIDUE_PER_CHAIN:
							  {
								  pp->setMaterialIndex(chemData->residueChainIndex[chemData->residues[theResidue].getChain()]);
								  break;
							  }
							  
							  default:
							  break;
						  }
*/
						  detail = new ChemDetail;
						  detail->setSchematicIndex(theSchematic);
						  pp->setDetail(detail, this);
					  }
				  }
			  }
		  }
    } // <-- cylinder selection 

    // --> arrow selection
    SbVec3f			point;
	  SbVec3f			norm;
    int oldTessel = schematicArrowTesselationSegments.getValue();
    schematicArrowTesselationSegments = 3;

    for (i=0;i<schematic->arrows.getLength();i++)
    {

	    const SbIntList& list = *((SbIntList*)schematicArrows.get(schematic->arrows[i]));
	    for(j=0;j<list.getLength()-1;j++)
      {
	      computeSchematicArrowGeometry(list[j],false,false);
        if ((
	      // top
	      rayIntersectsTriangle(pickLine,scrtr[0],scrtl[0],scrtr[1],point) ||
	      rayIntersectsTriangle(pickLine,scrtl[0],scrtr[1],scrtl[1],point) ||
	      rayIntersectsTriangle(pickLine,scrtr[1],scrtl[1],scrtr[2],point) ||
	      rayIntersectsTriangle(pickLine,scrtl[1],scrtr[2],scrtl[2],point) ||
	      // bottom
	      rayIntersectsTriangle(pickLine,scrbr[0],scrbl[0],scrbr[1],point) ||
	      rayIntersectsTriangle(pickLine,scrbl[0],scrbr[1],scrbl[1],point) ||
	      rayIntersectsTriangle(pickLine,scrbr[1],scrbl[1],scrbr[2],point) ||
	      rayIntersectsTriangle(pickLine,scrbl[1],scrbr[2],scrbl[2],point) ||
	      // left
	      rayIntersectsTriangle(pickLine,scrtl[0],scrbl[0],scrtl[1],point) ||
	      rayIntersectsTriangle(pickLine,scrbl[0],scrtl[1],scrbl[1],point) ||
	      rayIntersectsTriangle(pickLine,scrtl[1],scrbl[1],scrtl[2],point) ||
	      rayIntersectsTriangle(pickLine,scrbl[1],scrtl[2],scrbl[2],point) ||
	      // right
	      rayIntersectsTriangle(pickLine,scrtr[0],scrbr[0],scrtr[1],point) ||
	      rayIntersectsTriangle(pickLine,scrbr[0],scrtr[1],scrbr[1],point) ||
	      rayIntersectsTriangle(pickLine,scrtr[1],scrbr[1],scrtr[2],point) ||
	      rayIntersectsTriangle(pickLine,scrbr[1],scrtr[2],scrbr[2],point)
	      ) && ( (pp = action->addIntersection(point)) != NULL))
        {
	        pp->setObjectTextureCoords(SbVec4f(0,0,0,0));

	        norm = pickLine.getDirection();
	        norm.negate();
	        norm.normalize();
	        pp->setObjectNormal(norm);

          /*
	        // Material index
	        switch (chemColor->residueColorBinding.getValue())
	        {
		        case ChemColor::RESIDUE_OVERALL:
		        {
			        pp->setMaterialIndex(0);
			        break;
		        }
		        case ChemColor::RESIDUE_PER_INDEX:
		        {
			        int index = 0;
			        if (chemData->residues[theResidue].getIndex() > 0)
			        index = chemData->residueColorIndex[chemData->residues[theResidue].getIndex()];
			        pp->setMaterialIndex(index);
			        break;
		        }
		        case ChemColor::RESIDUE_PER_CHAIN:
		        {
			        pp->setMaterialIndex(chemData->residueChainIndex[chemData->residues[theResidue].getChain()]);
			        break;
		        }
		        
		        default:
		        break;
	        }
          */
	        detail = new ChemDetail;
	        detail->setSchematicIndex(theSchematic);
	        pp->setDetail(detail, this);
        }
      }
 
      // ace
	    computeSchematicArrowGeometry(list[j],true);
      if ((
	    // top
	    rayIntersectsTriangle(pickLine,scrtr[0],scrtl[0],scrtr[1],point) ||
	    rayIntersectsTriangle(pickLine,scrtl[0],scrtr[1],scrtl[1],point) ||
	    rayIntersectsTriangle(pickLine,scrtr[1],scrtl[1],scrtr[2],point) ||
	    rayIntersectsTriangle(pickLine,scrtl[1],scrtr[2],scrtl[2],point) ||
	    // bottom
	    rayIntersectsTriangle(pickLine,scrbr[0],scrbl[0],scrbr[1],point) ||
	    rayIntersectsTriangle(pickLine,scrbl[0],scrbr[1],scrbl[1],point) ||
	    rayIntersectsTriangle(pickLine,scrbr[1],scrbl[1],scrbr[2],point) ||
	    rayIntersectsTriangle(pickLine,scrbl[1],scrbr[2],scrbl[2],point) ||
	    // left
	    rayIntersectsTriangle(pickLine,scrtl[0],scrbl[0],scrtl[1],point) ||
	    rayIntersectsTriangle(pickLine,scrbl[0],scrtl[1],scrbl[1],point) ||
	    rayIntersectsTriangle(pickLine,scrtl[1],scrbl[1],scrtl[2],point) ||
	    rayIntersectsTriangle(pickLine,scrbl[1],scrtl[2],scrbl[2],point) ||
	    // right
	    rayIntersectsTriangle(pickLine,scrtr[0],scrbr[0],scrtr[1],point) ||
	    rayIntersectsTriangle(pickLine,scrbr[0],scrtr[1],scrbr[1],point) ||
	    rayIntersectsTriangle(pickLine,scrtr[1],scrbr[1],scrtr[2],point) ||
	    rayIntersectsTriangle(pickLine,scrbr[1],scrtr[2],scrbr[2],point)
	    ) && ( (pp = action->addIntersection(point)) != NULL))
      {
	      pp->setObjectTextureCoords(SbVec4f(0,0,0,0));

	      norm = pickLine.getDirection();
	      norm.negate();
	      norm.normalize();
	      pp->setObjectNormal(norm);

        /*
	      // Material index
	      switch (chemColor->residueColorBinding.getValue())
	      {
		      case ChemColor::RESIDUE_OVERALL:
		      {
			      pp->setMaterialIndex(0);
			      break;
		      }
		      case ChemColor::RESIDUE_PER_INDEX:
		      {
			      int index = 0;
			      if (chemData->residues[theResidue].getIndex() > 0)
			      index = chemData->residueColorIndex[chemData->residues[theResidue].getIndex()];
			      pp->setMaterialIndex(index);
			      break;
		      }
		      case ChemColor::RESIDUE_PER_CHAIN:
		      {
			      pp->setMaterialIndex(chemData->residueChainIndex[chemData->residues[theResidue].getChain()]);
			      break;
		      }
		      
		      default:
		      break;
	      }
        */
	      detail = new ChemDetail;
	      detail->setSchematicIndex(theSchematic);
	      pp->setDetail(detail, this);
      }
    }
    schematicArrowTesselationSegments = oldTessel;
    // <-- arrow selection

    // --> tube selection
    oldTessel = schematicTubeTesselationSegments.getValue();
    schematicTubeTesselationSegments = 2;
    int oldShape = schematicTubeShapeSegments.getValue();
    schematicTubeShapeSegments = 3;
    
		double angle,R;
		R = schematicTubeRadius.getValue();
		SbVec3f C;
		for (i=0;i<3;i++)
		{
			angle = (2*M_PI/3)*i;	
			C.setValue(R*cos(angle), R*sin(angle),0);
			scTubeVertices.set1Value(i,C);
		}
    
    for (i=0;i<schematic->tubes.getLength();i++)
    {     
      const SbIntList& list = *((SbIntList*)schematicTubes.get(schematic->tubes[i]));
	    computeTubePath(list);
		  computeSchematicTubeGeometry(false);

      for (j=0; j<tubePathCnt-1; j++)
      {
        if ((
        // A
        rayIntersectsTriangle(pickLine,scvtx[0][j],scvtx[1][j],scvtx[1][j+1],point) ||
        // B
        rayIntersectsTriangle(pickLine,scvtx[0][j],scvtx[1][j+1],scvtx[0][j+1],point) ||
        // C
        rayIntersectsTriangle(pickLine,scvtx[0][j],scvtx[2][j],scvtx[2][j+1],point) ||
        // D
        rayIntersectsTriangle(pickLine,scvtx[0][j],scvtx[2][j+1],scvtx[1][j+1],point) ||  
         // E
        rayIntersectsTriangle(pickLine,scvtx[1][j],scvtx[1][j+1],scvtx[2][j+1],point) ||
        // F
        rayIntersectsTriangle(pickLine,scvtx[1][j],scvtx[2][j],scvtx[2][j+1],point)
        ) && ( (pp = action->addIntersection(point)) != NULL))
        {
          pp->setObjectTextureCoords(SbVec4f(0,0,0,0));

	        norm = pickLine.getDirection();
	        norm.negate();
	        norm.normalize();
	        pp->setObjectNormal(norm);

	        detail = new ChemDetail;
	        detail->setSchematicIndex(theSchematic);
	        pp->setDetail(detail, this);
          break;
        }	      
      }
    }

    schematicTubeTesselationSegments = oldTessel;
    schematicTubeShapeSegments = oldShape;
    for (i=0;i<oldShape;i++)
		{
			angle = (2*M_PI/oldShape)*i;	
			C.setValue(R*cos(angle), R*sin(angle),0);
			scTubeVertices.set1Value(i,C);
		}
    // <-- tube selection

	SCHEMATICLOOP_END
}

// <-- schematic

// --> residue box
////////////////////////////////////////////////////////////////////////
//
// Description:
//    Computes the bounding box of the residues in ca stick mode.
//    Invoked by the computeBBox method.
//
// Use: private
////////////////////////////////////////////////////////////////////////
void ChemDisplay::bBoxResiduesAsCylinders(SoState *state, ChemDisplayParam *cdp,
																					int32_t &count, SbBox3f &box, SbVec3f &center)

{
	float rad;
	SbVec3f coords1, coords2,tmpVec;
	ChemBaseData* chemData = ChemBaseDataElement::get(state);
	ChemResidueStuff* res = NULL;
	
	rad = cdp->residueCylinderRadius.getValue();
	tmpVec.setValue(rad, rad, rad);

	RESIDUELOOP_START(residueIndex)

		res = &residueAttributes[theResidue];
		if (!res->isStickRenderable) continue;

		coords1 = chemData->getAtomCoordinates(res->from);
		coords2 = chemData->getAtomCoordinates(res->to);
		box.extendBy(coords1 + tmpVec);
		box.extendBy(coords1 - tmpVec);
		center += coords1;
		count++;
		box.extendBy(coords2 + tmpVec);
		box.extendBy(coords2 - tmpVec);
		center += coords2;
		count++;
	RESIDUELOOP_END
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Computes the bounding box of the residues in ribbon modes.
//    Invoked by the computeBBox method.
//
// Use: private
////////////////////////////////////////////////////////////////////////
void ChemDisplay::bBoxResiduesAsRibbons(SoState *state, ChemDisplayParam *cdp,
														 int32_t &count, SbBox3f &box, SbVec3f &center)
{
	ChemData* chemData = (ChemData*)ChemBaseDataElement::get(state);
	ChemResidueStuff* res = NULL;
	SbVec3f coord;
  SbVec3f tmpVec;
  tmpVec.setValue(0.1f,0.1f,0.1f);

	RESIDUELOOP_START(residueIndex)
		res = &residueAttributes[theResidue];
		if (!res->isStickRenderable) continue;
		coord = chemData->getAtomCoordinates(res->from);
		box.extendBy(coord+tmpVec);
    box.extendBy(coord-tmpVec);
		center += coord;
		count++;
	RESIDUELOOP_END
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Computes the bounding box of the residues in schematic mode.
//    Invoked by the computeBBox method.
//
// Use: private
////////////////////////////////////////////////////////////////////////
void ChemDisplay::bBoxResiduesAsSchematic(SoState *state, ChemDisplayParam *cdp,
														 int32_t &count, SbBox3f &box, SbVec3f &center)
{
	// TO DO: Improve box accuracy
	bBoxResiduesAsRibbons(state, cdp, count, box, center);
}

void ChemDisplay::bBoxResiduesNone(SoState *state, ChemDisplayParam *cdp,
																	 int32_t &count, SbBox3f &box,SbVec3f &center)

{
	// TO DO: Any other idea?
	bBoxResiduesAsRibbons(state, cdp, count, box, center);
}

// <-- residue box

#undef RESIDUELOOP_START
#undef RESIDUELOOP_END
#undef RESDUP
//  --> stick lod
#undef RENDER_STICK_OV
#undef RENDER_STICK_RPN
#undef RENDER_STICK_RPC
// <-- stick lod
#undef POINT_PLANE_DIST
