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

#ifndef __CHEM_OCTREE_NODE__
#define __CHEM_OCTREE_NODE__

#include "inv/SbLinear.H"

#include "inv/ChemKit/MFVec2i.H"

class ChemData;
class ChemDisplay;

class ChemOctreeNode
{
public:
	ChemOctreeNode();
	~ChemOctreeNode();

	enum OctreeNodeBinding
	{
		TOP_LEFT_FRONT = 0,			// 0
		TOP_LEFT_BACK,			// 1
		TOP_RIGHT_FRONT,
		TOP_RIGHT_BACK,			// etc.
		BOTTOM_LEFT_FRONT,
		BOTTOM_LEFT_BACK,
		BOTTOM_RIGHT_FRONT,
		BOTTOM_RIGHT_BACK
	};

	enum CullingBinding
	{
		INSIDE = 0,
		OUTSIDE,
		OVERLAP
	};

	void build(const ChemDisplay* chemDisplay, const ChemData* chemData);
	bool nodeContains(const SbVec3f &v) const;
	bool nodeContainsAABB(const SbVec3f &c,const SbVec3f& hs) const;
	void render() const;
	void getIndexes(const ChemData* chemData,
		MFVec2i &nbondedAtomIndex, MFVec2i &nnonBondedAtomIndex, MFVec2i &nbondIndex, MFVec2i &nresidueIndex,
		MFVec2i &hbondedAtomIndex, MFVec2i &hnonBondedAtomIndex, MFVec2i &hbondIndex, MFVec2i &hresidueIndex,
		MFVec2i &natomLabelIndex, MFVec2i &nbondLabelIndex, MFVec2i &nresidueLabelIndex,
		MFVec2i &hatomLabelIndex, MFVec2i &hbondLabelIndex, MFVec2i &hresidueLabelIndex);

	static void computeFrustumPlanes(SbPlane* planes);

protected:
	// culling methods
	void traverse(const SbPlane* planes,
		MFVec2i &nbondedAtomIndex, MFVec2i &nnonBondedAtomIndex, MFVec2i &nbondIndex, MFVec2i &nresidueIndex,
		MFVec2i &hbondedAtomIndex, MFVec2i &hnonBondedAtomIndex, MFVec2i &hbondIndex, MFVec2i &hresidueIndex,
		MFVec2i &natomLabelIndex, MFVec2i &nbondLabelIndex, MFVec2i &nresidueLabelIndex,
		MFVec2i &hatomLabelIndex, MFVec2i &hbondLabelIndex, MFVec2i &hresidueLabelIndex);
	
	// building methods
	void buildNode(const ChemDisplay* chemDisplay, const ChemData* chemData, const unsigned short lev);
	void setupChild(const int nodeID, const SbVec3f &c, const float parentWidth);
	void sortIndices(int globalNumberOfAtoms,int globalNumberOfBonds,
		int globalNumberOfResidues);

// aabb methods
	int overlap(const SbPlane &plane) const;

// render methods
	void renderBox() const;

	//members
	ChemOctreeNode* childs;
	SbVec3f center;
	float halfsize;
	short level;

	// visual entities
	MFVec2i normalBondedAtomIndex;
	MFVec2i normalNonBondedAtomIndex;
	MFVec2i normalBondIndex;
	MFVec2i normalResidueIndex;
	MFVec2i highlightBondedAtomIndex;
	MFVec2i highlightNonBondedAtomIndex;
	MFVec2i highlightBondIndex;
	MFVec2i highlightResidueIndex;
	
	// labels
	MFVec2i normalAtomLabelIndex;
	MFVec2i highlightAtomLabelIndex;
	MFVec2i normalBondLabelIndex;
	MFVec2i highlightBondLabelIndex;	
	MFVec2i normalResidueLabelIndex;
	MFVec2i highlightResidueLabelIndex;

	// visual variables
	int cullingType;
};

#endif
