//////////////////////////////////////////////////////////////////////
// ChemLOD.h: interface for the ChemLOD class.
//
//////////////////////////////////////////////////////////////////////
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

#ifndef _CHEMLOD_H_
#define _CHEMLOD_H_

#include "inv/SbLinear.H"

class ChemDisplay;
class ChemUnitSphere;
class ChemUnitCylinder;

class ChemLOD  
{
public:
	ChemLOD();
	~ChemLOD();

	unsigned int nAtoms;

	// lod sphere members

	unsigned int sphereClipCnt;
	unsigned int pointCnt;
	unsigned int billboardCnt;
	unsigned int sphere1Cnt;
	unsigned int sphere2Cnt;
	unsigned int sphere3Cnt;
	unsigned int sphere4Cnt;

	const int32_t& getBondIndexCnt(const int lod) const;
	const int32_t& getBondIndex(const int lod, const int index) const;
	const ChemUnitCylinder* getBondCylinder(const int lod) const;
	ChemUnitCylinder* getBondCylinder(const int lod);
	void addBondIndex(const int lod, const int index);

	const int32_t& getResidueIndexCnt(const int lod) const;
	const int32_t& getResidueIndex(const int lod, const int index) const;
	const ChemUnitCylinder* getResidueCylinder(const int lod) const;
	ChemUnitCylinder* getResidueCylinder(const int lod);
	void addResidueIndex(const int lod, const int index);

protected:
	int* pointIndices;
	int32_t* billboardIndices;
	int32_t* sphere1Indices;
	int32_t* sphere2Indices;
	int32_t* sphere3Indices;
	int32_t* sphere4Indices;
	ChemUnitSphere* sphere1;
	ChemUnitSphere* sphere2;
	ChemUnitSphere* sphere3;
	ChemUnitSphere* sphere4;
	SbMatrix *atomMatrices;

	// bond members

	int32_t nBonds;
	int32_t* bondIndicesCnt;
	int32_t** bondIndices;
	ChemUnitCylinder** bondCylinder;

	// residues members

	int32_t nResidues;
	int32_t* residueIndicesCnt;
	int32_t** residueIndices;
	ChemUnitCylinder** residueCylinder;

	// methods

	void resetAtoms(int32_t numberOfAtoms);
	void resetBonds(int32_t numberOfBonds);
	void resetResidues(int32_t numberOfResidues);

	void changeSphereComplexity(float complexity);
	void changeCylinderComplexity(float complexity,int displayStyle=0);

	bool bAtoms;
	void allocAtoms(int32_t numberOfAtoms);
	void allocBonds(int32_t numberOfBonds);
	void allocResidues(int32_t numberOfResidues);

	void deleteAtoms();
	void deleteBonds();
	void deleteResidues();

	friend class ChemDisplay;
};

#endif
