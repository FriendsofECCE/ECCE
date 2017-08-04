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
 * Homepage: http://www.tecn.upf.es/openMOIV
 * Contact:  openmoiv@upf.es
 *
\**************************************************************************/

#include <float.h>

#include "inv/SbPList.H"
#include "inv/SbLinear.H"

#include "inv/ChemKit/ChemOctreeNode.H"
#include "inv/ChemKit/ChemResidueStuff.H"

#include "inv/ChemKit/ChemData.H"
#include "inv/ChemKit/ChemDisplay.H"

#ifdef WIN32
#include <windows.h>
#endif
#include <GL/gl.h>

#define MAX_RECURSION_LEVEL 3
#define MAX_ENTITIES 100
#define X 0
#define Y 1
#define Z 2

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

static int numResidueLoops;
static int residueLoop;
static int32_t residueStart;
static int32_t residueEnd;
static int32_t theResidue;

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

#define POINT_PLANE_DIST(PLANE,POINT) \
PLANE.getNormal().dot(POINT) - PLANE.getDistanceFromOrigin();
	
ChemOctreeNode::ChemOctreeNode()
{
	level = 0;
	cullingType = INSIDE;
	childs = 0;
}

ChemOctreeNode::~ChemOctreeNode()
{
	if (childs)
		delete [] childs;
	childs = 0;
}

void ChemOctreeNode::build(const ChemDisplay* chemDisplay, const ChemData* chemData)
{
	//init first node: center, width, facelist=full

	level = 0;
	center = SbVec3f(0,0,0);
	cullingType=INSIDE;

	if (childs)
	{
		delete [] childs;
		childs = 0;
	}
		
	SbVec3f minatom(FLT_MAX,FLT_MAX,FLT_MAX);
	SbVec3f maxatom(-FLT_MAX,-FLT_MAX,-FLT_MAX);

	int globalNumberOfAtoms=chemData->numberOfAtoms.getValue();
	//int globalNumberOfBonds=chemData->numberOfBonds.getValue();

	ATOMLOOP_START(chemDisplay->atomIndex)
		const SbVec3f& atom = chemData->getAtomCoordinates(theAtom);
		if (atom[X] > maxatom[X])
			maxatom[X] = atom[X];
		if (atom[Y] > maxatom[Y])
			maxatom[Y] = atom[Y];
		if (atom[Z] > maxatom[Z])
			maxatom[Z] = atom[Z];
		if (atom[X] < minatom[X])
			minatom[X] = atom[X];
		if (atom[Y] < minatom[Y])
			minatom[Y] = atom[Y];
		if (atom[Z] < minatom[Z])
			minatom[Z] = atom[Z];
	ATOMLOOP_END

	center = minatom + 0.5f * (maxatom - minatom);
	maxatom -= center;
	minatom -= center;
	maxatom[X] = fabs(maxatom[X]);
	maxatom[Y] = fabs(maxatom[Y]);
	maxatom[Z] = fabs(maxatom[Z]);
	minatom[X] = fabs(minatom[X]);
	minatom[Y] = fabs(minatom[Y]);
	minatom[Z] = fabs(minatom[Z]);

#ifndef max
#define max(A,B) (A>B) ? A:B
#endif

	halfsize = max(maxatom[X],maxatom[Y]);
	halfsize = max(halfsize,maxatom[Z]);
	halfsize = max(halfsize,minatom[X]);
	halfsize = max(halfsize,minatom[Y]);
	halfsize = max(halfsize,minatom[Z]);

	// TO DO: increment halfsize by float = errors

	normalBondedAtomIndex.setNum(0);
	normalNonBondedAtomIndex.setNum(0);
	normalBondIndex.setNum(0);
	normalResidueIndex.setNum(0);

	normalAtomLabelIndex.setNum(0);
	normalBondLabelIndex.setNum(0);
	normalResidueLabelIndex.setNum(0);

	highlightBondedAtomIndex.setNum(0);
	highlightNonBondedAtomIndex.setNum(0);
	highlightBondIndex.setNum(0);
	highlightResidueIndex.setNum(0);

	highlightAtomLabelIndex.setNum(0);
	highlightBondLabelIndex.setNum(0);
	highlightResidueLabelIndex.setNum(0);
	
	if (chemDisplay->normalAtomIndex.getNum()>0)
		normalBondedAtomIndex.setValues(0,chemDisplay->normalAtomIndex.getNum(),chemDisplay->normalAtomIndex.getValues(0));
	if (chemDisplay->normalNonBondedAtomIndex.getNum()>0)
		normalNonBondedAtomIndex.setValues(0,chemDisplay->normalNonBondedAtomIndex.getNum(),chemDisplay->normalNonBondedAtomIndex.getValues(0));
	if (chemDisplay->normalBondIndex.getNum()>0)
		normalBondIndex.setValues(0,chemDisplay->normalBondIndex.getNum(),chemDisplay->normalBondIndex.getValues(0));
	if (chemDisplay->normalResidueIndex.getNum()>0)
		normalResidueIndex.setValues(0,chemDisplay->normalResidueIndex.getNum(),chemDisplay->normalResidueIndex.getValues(0));

	if (chemDisplay->normalAtomLabelIndex.getNum()>0)
		normalAtomLabelIndex.setValues(0,chemDisplay->normalAtomLabelIndex.getNum(),chemDisplay->normalAtomLabelIndex.getValues(0));
	if (chemDisplay->normalBondLabelIndex.getNum()>0)
		normalBondLabelIndex.setValues(0,chemDisplay->normalBondLabelIndex.getNum(),chemDisplay->normalBondLabelIndex.getValues(0));
	if (chemDisplay->normalResidueLabelIndex.getNum()>0)
		normalResidueLabelIndex.setValues(0,chemDisplay->normalResidueLabelIndex.getNum(),chemDisplay->normalResidueLabelIndex.getValues(0));

	if (chemDisplay->highlightAtomIndex.getNum()>0)
		highlightBondedAtomIndex.setValues(0,chemDisplay->highlightAtomIndex.getNum(),chemDisplay->highlightAtomIndex.getValues(0));
	if (chemDisplay->highlightNonBondedAtomIndex.getNum()>0)
		highlightNonBondedAtomIndex.setValues(0,chemDisplay->highlightNonBondedAtomIndex.getNum(),chemDisplay->highlightNonBondedAtomIndex.getValues(0));
	if (chemDisplay->highlightBondIndex.getNum()>0)
		highlightBondIndex.setValues(0,chemDisplay->highlightBondIndex.getNum(),chemDisplay->highlightBondIndex.getValues(0));
	if (chemDisplay->highlightResidueIndex.getNum()>0)
		highlightResidueIndex.setValues(0,chemDisplay->highlightResidueIndex.getNum(),chemDisplay->highlightResidueIndex.getValues(0));

	if (chemDisplay->highlightAtomLabelIndex.getNum()>0)
		highlightAtomLabelIndex.setValues(0,chemDisplay->highlightAtomLabelIndex.getNum(),chemDisplay->highlightAtomLabelIndex.getValues(0));
	if (chemDisplay->highlightBondLabelIndex.getNum()>0)
		highlightBondLabelIndex.setValues(0,chemDisplay->highlightBondLabelIndex.getNum(),chemDisplay->highlightBondLabelIndex.getValues(0));
	if (chemDisplay->highlightResidueLabelIndex.getNum()>0)
		highlightResidueLabelIndex.setValues(0,chemDisplay->highlightResidueLabelIndex.getNum(),chemDisplay->highlightResidueLabelIndex.getValues(0));

//	SbPlane planes[6];
//	computeFrustumPlanes(planes);

	buildNode(chemDisplay,chemData,level);
}

void ChemOctreeNode::buildNode(const ChemDisplay* chemDisplay, const ChemData* chemData, const unsigned short lev)
{
	level = lev;

	int globalNumberOfAtoms=chemData->numberOfAtoms.getValue();
	int globalNumberOfBonds=chemData->numberOfBonds.getValue();
	int globalNumberOfResidues=chemData->numberOfResidues.getValue();

	int numentities =
		normalBondedAtomIndex.count(globalNumberOfAtoms)+
		normalNonBondedAtomIndex.count(globalNumberOfAtoms)+
		highlightBondedAtomIndex.count(globalNumberOfAtoms)+
		highlightNonBondedAtomIndex.count(globalNumberOfAtoms)+
		normalBondIndex.count(globalNumberOfBonds)+
		highlightBondIndex.count(globalNumberOfBonds)+
		normalResidueIndex.count(globalNumberOfResidues)+
		highlightResidueIndex.count(globalNumberOfResidues);

	if (level >= MAX_RECURSION_LEVEL || numentities  <= MAX_ENTITIES)
	{
		sortIndices(globalNumberOfAtoms,globalNumberOfBonds,globalNumberOfResidues);
		return;
	}

	childs = new ChemOctreeNode[8]();

	int i;
	for (i=0; i<8;i++)
		childs[i].setupChild(i,center,halfsize*2);


	ATOMLOOP_START(normalBondedAtomIndex)
	for (i=0;i<8;i++)
	{
		if (childs[i].nodeContains(chemData->getAtomCoordinates(theAtom)))
		{
			childs[i].normalBondedAtomIndex.set1Value(childs[i].normalBondedAtomIndex.getNum(),SbVec2i(theAtom,1));
			i=8;
		}
	}
	ATOMLOOP_END

	ATOMLOOP_START(highlightBondedAtomIndex)
	for (i=0;i<8;i++)
	{
		if (childs[i].nodeContains(chemData->getAtomCoordinates(theAtom)))
		{
			childs[i].highlightBondedAtomIndex.set1Value(childs[i].highlightBondedAtomIndex.getNum(),SbVec2i(theAtom,1));
			i=8;
		}
	}
	ATOMLOOP_END

	ATOMLOOP_START(normalNonBondedAtomIndex)
	for (i=0;i<8;i++)
	{
		if (childs[i].nodeContains(chemData->getAtomCoordinates(theAtom)))
		{
			childs[i].normalNonBondedAtomIndex.set1Value(childs[i].normalNonBondedAtomIndex.getNum(),SbVec2i(theAtom,1));
			i=8;
		}
	}
	ATOMLOOP_END

	ATOMLOOP_START(highlightNonBondedAtomIndex)
	for (i=0;i<8;i++)
	{
		if (childs[i].nodeContains(chemData->getAtomCoordinates(theAtom)))
		{
			childs[i].highlightNonBondedAtomIndex.set1Value(childs[i].highlightNonBondedAtomIndex.getNum(),SbVec2i(theAtom,1));
			i=8;
		}
	}
	ATOMLOOP_END

	BONDLOOP_START(normalBondIndex)
	for (i=0;i<8;i++)
	{
		if (
			childs[i].nodeContains(chemData->getAtomCoordinates(chemData->getBondFrom(theBond))) ||
			childs[i].nodeContains(chemData->getAtomCoordinates(chemData->getBondTo(theBond)))
			)
		{
				childs[i].normalBondIndex.set1Value(childs[i].normalBondIndex.getNum(),SbVec2i(theBond,1));
				//i=8;
		}
	}
	BONDLOOP_END

	BONDLOOP_START(highlightBondIndex)
	for (i=0;i<8;i++)
	{
		if (
			childs[i].nodeContains(chemData->getAtomCoordinates(chemData->getBondFrom(theBond))) ||
			childs[i].nodeContains(chemData->getAtomCoordinates(chemData->getBondTo(theBond)))
			)
		{
				childs[i].highlightBondIndex.set1Value(childs[i].highlightBondIndex.getNum(),SbVec2i(theBond,1));
				//i=8;
		}
	}
	BONDLOOP_END


	RESIDUELOOP_START(normalResidueIndex)
			
	if (!chemData->residues[theResidue].checkControlPoints())
		continue;
	
	const ChemResidueStuff& res = chemDisplay->getResidueAttributes()[theResidue];

	for (i=0;i<8;i++)
	{
		if (res.isStickRenderable &&
			(childs[i].nodeContains(chemData->getAtomCoordinates(res.from)) ||
			childs[i].nodeContains(chemData->getAtomCoordinates(res.to)))
			)
		{
			childs[i].normalResidueIndex.set1Value(childs[i].normalResidueIndex.getNum(),SbVec2i(theResidue,1));
		}

		else if (res.isRibbonRenderable && 
			childs[i].nodeContainsAABB(res.center,res.halfsize))
		{
			childs[i].normalResidueIndex.set1Value(childs[i].normalResidueIndex.getNum(),SbVec2i(theResidue,1));
		}
		
	}	
	RESIDUELOOP_END

	RESIDUELOOP_START(highlightResidueIndex)
	
	if (!chemData->residues[theResidue].checkControlPoints())
		continue;
	
	const ChemResidueStuff& res = chemDisplay->getResidueAttributes()[theResidue];

	for (i=0;i<8;i++)
	{
		if (res.isStickRenderable &&
			(childs[i].nodeContains(chemData->getAtomCoordinates(res.from)) ||
			childs[i].nodeContains(chemData->getAtomCoordinates(res.to)))
			)
		{
			childs[i].highlightResidueIndex.set1Value(childs[i].highlightResidueIndex.getNum(),SbVec2i(theResidue,1));
		}

		else if (res.isRibbonRenderable && 
			childs[i].nodeContainsAABB(res.center,res.halfsize))
		{
			childs[i].highlightResidueIndex.set1Value(childs[i].highlightResidueIndex.getNum(),SbVec2i(theResidue,1));
		}
		
	}	
	RESIDUELOOP_END

	// labels

	ATOMLOOP_START(normalAtomLabelIndex)
	for (i=0;i<8;i++)
	{
		if (childs[i].nodeContains(chemData->getAtomCoordinates(theAtom)))
		{
			childs[i].normalAtomLabelIndex.set1Value(childs[i].normalAtomLabelIndex.getNum(),SbVec2i(theAtom,1));
			i=8;
		}
	}
	ATOMLOOP_END

	ATOMLOOP_START(highlightAtomLabelIndex)
	for (i=0;i<8;i++)
	{
		if (childs[i].nodeContains(chemData->getAtomCoordinates(theAtom)))
		{
			childs[i].highlightAtomLabelIndex.set1Value(childs[i].highlightAtomLabelIndex.getNum(),SbVec2i(theAtom,1));
			i=8;
		}
	}
	ATOMLOOP_END

	BONDLOOP_START(normalBondLabelIndex)
	for (i=0;i<8;i++)
	{
		if (childs[i].nodeContains(
			chemData->getAtomCoordinates(chemData->getBondFrom(theBond)) +
			(chemData->getAtomCoordinates(chemData->getBondTo(theBond))-chemData->getAtomCoordinates(chemData->getBondFrom(theBond)))*0.5f)
			)
		{
				childs[i].normalBondLabelIndex.set1Value(childs[i].normalBondLabelIndex.getNum(),SbVec2i(theBond,1));
				i=8;
		}
	}
	BONDLOOP_END

	BONDLOOP_START(highlightBondLabelIndex)
	for (i=0;i<8;i++)
	{
		if (childs[i].nodeContains(
			chemData->getAtomCoordinates(chemData->getBondFrom(theBond)) +
			(chemData->getAtomCoordinates(chemData->getBondTo(theBond))-chemData->getAtomCoordinates(chemData->getBondFrom(theBond)))*0.5f)
			)
		{
				childs[i].highlightBondLabelIndex.set1Value(childs[i].highlightBondLabelIndex.getNum(),SbVec2i(theBond,1));
				i=8;
		}
	}
	BONDLOOP_END

	SbVec3f coord;
	RESIDUELOOP_START(normalResidueLabelIndex)

	if (chemData->residues[theResidue].checkControlPoints())
		coord = chemData->getAtomCoordinates(chemData->residues[theResidue][SbResidue::RESIDUE_CONTROL_POINT_CA]);
	else
	{
		if (!chemData->residues[theResidue].computeCenterOfMass(chemData,coord))
			continue;
	}

	for (i=0;i<8;i++)
	{
		if (childs[i].nodeContains(coord))
		{
				childs[i].normalResidueLabelIndex.set1Value(childs[i].normalResidueLabelIndex.getNum(),SbVec2i(theResidue,1));
				i=8;
		}
	}
	
	RESIDUELOOP_END

	RESIDUELOOP_START(highlightResidueLabelIndex)

	if (chemData->residues[theResidue].checkControlPoints())
		coord = chemData->getAtomCoordinates(chemData->residues[theResidue][SbResidue::RESIDUE_CONTROL_POINT_CA]);
	else
	{
		if (!chemData->residues[theResidue].computeCenterOfMass(chemData,coord))
			continue;
	}
	
	for (i=0;i<8;i++)
	{
		if (childs[i].nodeContains(coord))
		{
			childs[i].highlightResidueLabelIndex.set1Value(childs[i].highlightResidueLabelIndex.getNum(),SbVec2i(theResidue,1));
			i=8;
		}
	}
	
	RESIDUELOOP_END


	for (i=0; i<8;i++)
		childs[i].buildNode(chemDisplay,chemData,level+1);
}

// Computes the center of a child node given the center, width of parent node

void ChemOctreeNode::setupChild(const int nodeID, const SbVec3f &c, const float parentWidth)
{
	halfsize = parentWidth/4.0f;
	
	// Switch on the ID to see which subdivided node we are finding the center
	switch(nodeID)							
	{
		case TOP_LEFT_FRONT:
			// Calculate the center of this new node
			center = SbVec3f(c[X] - parentWidth/4, c[Y] + parentWidth/4, c[Z] + parentWidth/4);
			break;

		case TOP_LEFT_BACK:
			// Calculate the center of this new node
			center = SbVec3f(c[X] - parentWidth/4, c[Y] + parentWidth/4, c[Z] - parentWidth/4);
			break;

		case TOP_RIGHT_BACK:
			// Calculate the center of this new node
			center = SbVec3f(c[X] + parentWidth/4, c[Y] + parentWidth/4, c[Z] - parentWidth/4);
			break;

		case TOP_RIGHT_FRONT:
			// Calculate the center of this new node
			center = SbVec3f(c[X] + parentWidth/4, c[Y] + parentWidth/4, c[Z] + parentWidth/4);
			break;

		case BOTTOM_LEFT_FRONT:
			// Calculate the center of this new node
			center = SbVec3f(c[X] - parentWidth/4, c[Y] - parentWidth/4, c[Z] + parentWidth/4);
			break;

		case BOTTOM_LEFT_BACK:
			// Calculate the center of this new node
			center = SbVec3f(c[X] - parentWidth/4, c[Y] - parentWidth/4, c[Z] - parentWidth/4);
			break;

		case BOTTOM_RIGHT_BACK:
			// Calculate the center of this new node
			center = SbVec3f(c[X] + parentWidth/4, c[Y] - parentWidth/4, c[Z] - parentWidth/4);
			break;

		case BOTTOM_RIGHT_FRONT:
			// Calculate the center of this new node
			center = SbVec3f(c[X] + parentWidth/4, c[Y] - parentWidth/4, c[Z] + parentWidth/4);
			break;
	}
}

bool ChemOctreeNode::nodeContains(const SbVec3f &p) const
{
	return (
		p[X] >= (center[X]-halfsize) && p[X] <= (center[X]+halfsize) && 
		p[Y] >= (center[Y]-halfsize) && p[Y] <= (center[Y]+halfsize) &&
		p[Z] >= (center[Z]-halfsize) && p[Z] <= (center[Z]+halfsize));
}

bool ChemOctreeNode::nodeContainsAABB(const SbVec3f &c, const SbVec3f& hs) const
{
	const SbVec3f& T = c - this->center;
	
	return fabsf(T[X]) <= (hs[X] + this->halfsize)
	&&
	fabsf(T[Y]) <= (hs[Y] + this->halfsize)
	&&
	fabsf(T[Z]) <= (hs[Z] + this->halfsize);
}

void ChemOctreeNode::render() const
{
	if (cullingType==INSIDE)
	{
		glColor3f(0,1,0);
		renderBox();

	}
	else if (cullingType==OVERLAP && childs==0)
	{
		glColor3f(1,1,0);
		renderBox();
	}

	if (childs!=0 && cullingType==OVERLAP)
		for (int i=0;i<8;i++)
			childs[i].render();
}

void ChemOctreeNode::renderBox() const
{
	glPushMatrix();

	glTranslatef(center[0],center[1],center[2]);
	
	// sides
	glBegin (GL_LINE_STRIP);

	glVertex3f (-halfsize,halfsize,-halfsize);
	glVertex3f (-halfsize,halfsize,halfsize);
	glVertex3f (halfsize,halfsize,halfsize);
	glVertex3f (halfsize,halfsize,-halfsize);

	glVertex3f (-halfsize,halfsize,-halfsize);

	glVertex3f (-halfsize,-halfsize,-halfsize);
	glVertex3f (-halfsize,-halfsize,halfsize);
	glVertex3f (halfsize,-halfsize,halfsize);
	glVertex3f (halfsize,-halfsize,-halfsize);

	glVertex3f (-halfsize,-halfsize,-halfsize);

	glEnd();

	glBegin(GL_LINES);

	glVertex3f (halfsize,halfsize,-halfsize);
	glVertex3f (halfsize,-halfsize,-halfsize);

	glVertex3f (halfsize,halfsize,halfsize);
	glVertex3f (halfsize,-halfsize,halfsize);

	glVertex3f (-halfsize,halfsize,halfsize);
	glVertex3f (-halfsize,-halfsize,halfsize);

	glEnd();

	glPopMatrix();
}

void ChemOctreeNode::traverse(const SbPlane* planes,
		MFVec2i &nbondedAtomIndex, MFVec2i &nnonBondedAtomIndex, MFVec2i &nbondIndex, MFVec2i &nresidueIndex,
		MFVec2i &hbondedAtomIndex, MFVec2i &hnonBondedAtomIndex, MFVec2i &hbondIndex, MFVec2i &hresidueIndex,
		MFVec2i &natomLabelIndex, MFVec2i &nbondLabelIndex, MFVec2i &nresidueLabelIndex,
		MFVec2i &hatomLabelIndex, MFVec2i &hbondLabelIndex, MFVec2i &hresidueLabelIndex)
{
	int i,overlapValue;
	cullingType=INSIDE;
	for (i=0;i<6;i++)
	{
		overlapValue=overlap(planes[i]);
		if (overlapValue==OUTSIDE)
		{
			cullingType=OUTSIDE;
			return;//i=6;
		}
		else if (overlapValue==OVERLAP)
		{
			cullingType=OVERLAP;
		}
	}

	if (cullingType==INSIDE ||
		(cullingType==OVERLAP && childs==0)) // if leaf nodes
	{
		if (normalBondedAtomIndex.getNum()>0)
			nbondedAtomIndex.setValues(nbondedAtomIndex.getNum(),normalBondedAtomIndex.getNum(),normalBondedAtomIndex.getValues(0));

		if (normalNonBondedAtomIndex.getNum()>0)
			nnonBondedAtomIndex.setValues(nnonBondedAtomIndex.getNum(),normalNonBondedAtomIndex.getNum(),normalNonBondedAtomIndex.getValues(0));

		if (normalBondIndex.getNum()>0)
			nbondIndex.setValues(nbondIndex.getNum(),normalBondIndex.getNum(),normalBondIndex.getValues(0));

		if (normalResidueIndex.getNum()>0)
			nresidueIndex.setValues(nresidueIndex.getNum(),normalResidueIndex.getNum(),normalResidueIndex.getValues(0));

		if (highlightBondedAtomIndex.getNum()>0)
			hbondedAtomIndex.setValues(hbondedAtomIndex.getNum(),highlightBondedAtomIndex.getNum(),highlightBondedAtomIndex.getValues(0));

		if (highlightNonBondedAtomIndex.getNum()>0)
			hnonBondedAtomIndex.setValues(hnonBondedAtomIndex.getNum(),highlightNonBondedAtomIndex.getNum(),highlightNonBondedAtomIndex.getValues(0));

		if (highlightBondIndex.getNum()>0)
			hbondIndex.setValues(hbondIndex.getNum(),highlightBondIndex.getNum(),highlightBondIndex.getValues(0));

		if (highlightResidueIndex.getNum()>0)
			hresidueIndex.setValues(hresidueIndex.getNum(),highlightResidueIndex.getNum(),highlightResidueIndex.getValues(0));

		//labels

		if (normalAtomLabelIndex.getNum()>0)
			natomLabelIndex.setValues(natomLabelIndex.getNum(),normalAtomLabelIndex.getNum(),normalAtomLabelIndex.getValues(0));

		if (normalBondLabelIndex.getNum()>0)
			nbondLabelIndex.setValues(nbondLabelIndex.getNum(),normalBondLabelIndex.getNum(),normalBondLabelIndex.getValues(0));

		if (normalResidueLabelIndex.getNum()>0)
			nresidueLabelIndex.setValues(nresidueLabelIndex.getNum(),normalResidueLabelIndex.getNum(),normalResidueLabelIndex.getValues(0));

		if (highlightAtomLabelIndex.getNum()>0)
			hatomLabelIndex.setValues(hatomLabelIndex.getNum(),highlightAtomLabelIndex.getNum(),highlightAtomLabelIndex.getValues(0));

		if (highlightBondLabelIndex.getNum()>0)
			hbondLabelIndex.setValues(hbondLabelIndex.getNum(),highlightBondLabelIndex.getNum(),highlightBondLabelIndex.getValues(0));

		if (highlightResidueLabelIndex.getNum()>0)
			hresidueLabelIndex.setValues(hresidueLabelIndex.getNum(),highlightResidueLabelIndex.getNum(),highlightResidueLabelIndex.getValues(0));
		return;
	}

	if (cullingType==OVERLAP)
	{
		for (i=0;i<8;i++)
			childs[i].traverse(planes,
			nbondedAtomIndex,nnonBondedAtomIndex,nbondIndex,nresidueIndex,
			hbondedAtomIndex,hnonBondedAtomIndex,hbondIndex,hresidueIndex,
			natomLabelIndex,nbondLabelIndex,nresidueLabelIndex,
			hatomLabelIndex,hbondLabelIndex,hresidueLabelIndex);
	}
}

int ChemOctreeNode::overlap(const SbPlane &plane) const
{
	int q;
	SbVec3f vmin,vmax;
	for(q=0;q<=2;q++)
	{
		if(plane.getNormal()[q]>=0.0f)
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

	float distance = POINT_PLANE_DIST(plane,center+vmin);
	if(distance>=0.0f) return INSIDE;//full inside

	distance = POINT_PLANE_DIST(plane,center+vmax);
	if(distance<0.0f) return OUTSIDE;//full outside
	else return OVERLAP;//overlaps
	
	return OUTSIDE;
}

void ChemOctreeNode::computeFrustumPlanes(SbPlane* planes)
{
	SbMatrix projection, modelview;

	glGetFloatv( GL_PROJECTION_MATRIX, (float*)projection);
	glGetFloatv( GL_MODELVIEW_MATRIX, (float*)modelview);

	SbVec3f normal;
	float norma;
	float d;

	// projection * modelview
	//SbMatrix clip = projection.multRight(modelview);
	SbMatrix clip = projection.multLeft(modelview);
	
	// This will extract the RIGHT side of the frustum
	normal[0] = clip[0][3] - clip[0][0]; //3-0
	normal[1] = clip[1][3] - clip[1][0];//7-4
	normal[2] = clip[2][3] - clip[2][0];//11-8
	d = clip[3][3] - clip[3][0];//15-12

	// Normalize the RIGHT side
	norma = 1.0f/normal.length();
	planes[0]=SbPlane(normal*norma, -d*norma);

	// This will extract the LEFT side of the frustum
	normal[0] = clip[0][3] + clip[0][0]; //3-0
	normal[1] = clip[1][3] + clip[1][0];//7-4
	normal[2] = clip[2][3] + clip[2][0];//11-8
	d = clip[3][3] + clip[3][0];//15-12

	// Normalize the LEFT side
	norma = 1.0f/normal.length();
	planes[1]=SbPlane(normal*norma, -d*norma);

	// This will extract the BOTTOM side of the frustum
	normal[0] = clip[0][3] + clip[0][1];//3-1
	normal[1] = clip[1][3] + clip[1][1];//7-5
	normal[2] = clip[2][3] + clip[2][1];//11-9
	d = clip[3][3] + clip[3][1];//15-13

	// Normalize the BOTTOM side
	norma = 1.0f/normal.length();
	planes[2]=SbPlane(normal*norma, -d*norma);

	// This will extract the TOP side of the frustum
	normal[0] = clip[0][3] - clip[0][1];//3-1
	normal[1] = clip[1][3] - clip[1][1];//7-5
	normal[2] = clip[2][3] - clip[2][1];//11-9
	d = clip[3][3] - clip[3][1];//15-13

	// Normalize the TOP side
	norma = 1.0f/normal.length();
	planes[3]=SbPlane(normal*norma, -d*norma);

	// This will extract the BACK side of the frustum
	normal[0] = clip[0][3] - clip[0][2];//3-2
	normal[1] = clip[1][3] - clip[1][2];//7-6
	normal[2] = clip[2][3] - clip[2][2];//11-10
	d = clip[3][3] - clip[3][2];//15-14

	// Normalize the BACK side
	norma = 1.0f/normal.length();
	planes[4]=SbPlane(normal*norma, -d*norma);

	// This will extract the FRONT side of the frustum
	normal[0] = clip[0][3] + clip[0][2];//3-2
	normal[1] = clip[1][3] + clip[1][2];//7-6
	normal[2] = clip[2][3] + clip[2][2];//11-10
	d = clip[3][3] + clip[3][2];//15-14

	// Normalize the FRONT side
	norma = 1.0f/normal.length();
	planes[5]=SbPlane(normal*norma, -d*norma);
}

void ChemOctreeNode::getIndexes(const ChemData* chemData,
		MFVec2i &nbondedAtomIndex, MFVec2i &nnonBondedAtomIndex, MFVec2i &nbondIndex, MFVec2i &nresidueIndex,
		MFVec2i &hbondedAtomIndex, MFVec2i &hnonBondedAtomIndex, MFVec2i &hbondIndex, MFVec2i &hresidueIndex,
		MFVec2i &natomLabelIndex, MFVec2i &nbondLabelIndex, MFVec2i &nresidueLabelIndex,
		MFVec2i &hatomLabelIndex, MFVec2i &hbondLabelIndex, MFVec2i &hresidueLabelIndex)
{
	SbPlane planes[6];
	computeFrustumPlanes(planes);

	nbondedAtomIndex.startEditing();
	nnonBondedAtomIndex.startEditing();
	nbondIndex.startEditing();
	nresidueIndex.startEditing();

	natomLabelIndex.startEditing();
	nbondLabelIndex.startEditing();
	nresidueLabelIndex.startEditing();

	hbondedAtomIndex.startEditing();
	hnonBondedAtomIndex.startEditing();
	hbondIndex.startEditing();
	hresidueIndex.startEditing();

	hatomLabelIndex.startEditing();
	hbondLabelIndex.startEditing();
	hresidueLabelIndex.startEditing();

	nbondedAtomIndex.setNum(0);
	nnonBondedAtomIndex.setNum(0);
	nbondIndex.setNum(0);
	nresidueIndex.setNum(0);

	natomLabelIndex.setNum(0);
	nbondLabelIndex.setNum(0);
	nresidueLabelIndex.setNum(0);

	hbondedAtomIndex.setNum(0);
	hnonBondedAtomIndex.setNum(0);
	hbondIndex.setNum(0);
	hresidueIndex.setNum(0);

	hatomLabelIndex.setNum(0);
	hbondLabelIndex.setNum(0);
	hresidueLabelIndex.setNum(0);
	
	traverse(planes,
		nbondedAtomIndex,nnonBondedAtomIndex,nbondIndex,nresidueIndex,
		hbondedAtomIndex,hnonBondedAtomIndex,hbondIndex,hresidueIndex,
		natomLabelIndex,nbondLabelIndex,nresidueLabelIndex,
		hatomLabelIndex,hbondLabelIndex,hresidueLabelIndex);
/*
	MFVec2i tIndex;
	int globalNumberOfResidues = chemData->numberOfResidues.getValue();
	if (nresidueIndex.getNum()>0)
	{
		MFVec2i::sortIndices(nresidueIndex,tIndex,-1,globalNumberOfResidues);
		nresidueIndex.setNum(0);
		nresidueIndex.setValues(0,tIndex.getNum(),tIndex.getValues(0));
	}

	if (hresidueIndex.getNum()>0)
	{
		MFVec2i::sortIndices(hresidueIndex,tIndex,-1,globalNumberOfResidues);
		hresidueIndex.setNum(0);
		hresidueIndex.setValues(0,tIndex.getNum(),tIndex.getValues(0));
	}
*/
	nbondedAtomIndex.finishEditing();
	nnonBondedAtomIndex.finishEditing();
	nbondIndex.finishEditing();
	nresidueIndex.finishEditing();

	natomLabelIndex.finishEditing();
	nbondLabelIndex.finishEditing();
	nresidueLabelIndex.finishEditing();

	hbondedAtomIndex.finishEditing();
	hnonBondedAtomIndex.finishEditing();
	hbondIndex.finishEditing();
	hresidueIndex.finishEditing();

	hatomLabelIndex.finishEditing();
	hbondLabelIndex.finishEditing();
	hresidueLabelIndex.finishEditing();
	
	//delete [] planes;
}

void ChemOctreeNode::sortIndices(int globalNumberOfAtoms,
							 int globalNumberOfBonds,
							 int globalNumberOfResidues)
{
	MFVec2i tIndex;
	if (normalBondedAtomIndex.getNum()>0)
	{
		MFVec2i::sortIndices(normalBondedAtomIndex,tIndex,-1,globalNumberOfAtoms);
		normalBondedAtomIndex.setNum(0);
		normalBondedAtomIndex.setValues(0,tIndex.getNum(),tIndex.getValues(0));
	}
	if (normalNonBondedAtomIndex.getNum()>0)
	{
		MFVec2i::sortIndices(normalNonBondedAtomIndex,tIndex,-1,globalNumberOfAtoms);
		normalNonBondedAtomIndex.setNum(0);
		normalNonBondedAtomIndex.setValues(0,tIndex.getNum(),tIndex.getValues(0));
	}
	if (normalBondIndex.getNum()>0)
	{
		MFVec2i::sortIndices(normalBondIndex,tIndex,-1,globalNumberOfBonds);
		normalBondIndex.setNum(0);
		normalBondIndex.setValues(0,tIndex.getNum(),tIndex.getValues(0));
	}
	if (normalResidueIndex.getNum()>0)
	{
		MFVec2i::sortIndices(normalResidueIndex,tIndex,-1,globalNumberOfResidues);
		normalResidueIndex.setNum(0);
		normalResidueIndex.setValues(0,tIndex.getNum(),tIndex.getValues(0));
	}
	if (normalAtomLabelIndex.getNum()>0)
	{
		MFVec2i::sortIndices(normalAtomLabelIndex,tIndex,-1,globalNumberOfAtoms);
		normalAtomLabelIndex.setNum(0);
		normalAtomLabelIndex.setValues(0,tIndex.getNum(),tIndex.getValues(0));
	}
	if (normalBondLabelIndex.getNum()>0)
	{
		MFVec2i::sortIndices(normalBondLabelIndex,tIndex,-1,globalNumberOfBonds);
		normalBondLabelIndex.setNum(0);
		normalBondLabelIndex.setValues(0,tIndex.getNum(),tIndex.getValues(0));
	}
	if (normalResidueLabelIndex.getNum()>0)
	{
		MFVec2i::sortIndices(normalResidueLabelIndex,tIndex,-1,globalNumberOfResidues);
		normalResidueLabelIndex.setNum(0);
		normalResidueLabelIndex.setValues(0,tIndex.getNum(),tIndex.getValues(0));
	}

	if (highlightBondedAtomIndex.getNum()>0)
	{
		MFVec2i::sortIndices(highlightBondedAtomIndex,tIndex,-1,globalNumberOfAtoms);
		highlightBondedAtomIndex.setNum(0);
		highlightBondedAtomIndex.setValues(0,tIndex.getNum(),tIndex.getValues(0));
	}
	if (highlightNonBondedAtomIndex.getNum()>0)
	{
		MFVec2i::sortIndices(highlightNonBondedAtomIndex,tIndex,-1,globalNumberOfAtoms);
		highlightNonBondedAtomIndex.setNum(0);
		highlightNonBondedAtomIndex.setValues(0,tIndex.getNum(),tIndex.getValues(0));
	}
	if (highlightBondIndex.getNum()>0)
	{
		MFVec2i::sortIndices(highlightBondIndex,tIndex,-1,globalNumberOfBonds);
		highlightBondIndex.setNum(0);
		highlightBondIndex.setValues(0,tIndex.getNum(),tIndex.getValues(0));
	}
	if (highlightResidueIndex.getNum()>0)
	{
		MFVec2i::sortIndices(highlightResidueIndex,tIndex,-1,globalNumberOfResidues);
		highlightResidueIndex.setNum(0);
		highlightResidueIndex.setValues(0,tIndex.getNum(),tIndex.getValues(0));
	}
	if (highlightAtomLabelIndex.getNum()>0)
	{
		MFVec2i::sortIndices(highlightAtomLabelIndex,tIndex,-1,globalNumberOfAtoms);
		highlightAtomLabelIndex.setNum(0);
		highlightAtomLabelIndex.setValues(0,tIndex.getNum(),tIndex.getValues(0));
	}
	if (highlightBondLabelIndex.getNum()>0)
	{
		MFVec2i::sortIndices(highlightBondLabelIndex,tIndex,-1,globalNumberOfBonds);
		highlightBondLabelIndex.setNum(0);
		highlightBondLabelIndex.setValues(0,tIndex.getNum(),tIndex.getValues(0));
	}
	if (highlightResidueLabelIndex.getNum()>0)
	{
		MFVec2i::sortIndices(highlightResidueLabelIndex,tIndex,-1,globalNumberOfResidues);
		highlightResidueLabelIndex.setNum(0);
		highlightResidueLabelIndex.setValues(0,tIndex.getNum(),tIndex.getValues(0));
	}
}

#undef MAX_RECURSION_LEVEL
#undef MAX_ENTITIES
#undef X
#undef Y
#undef Z
#undef ATOMLOOP_START
#undef ATOMLOOP_END
#undef BONDLOOP_START
#undef BONDLOOP_END
#undef RESIDUELOOP_START
#undef RESIDUELOOP_END
#undef POINT_PLANE_DIST
