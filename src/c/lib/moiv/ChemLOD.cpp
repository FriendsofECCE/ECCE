//////////////////////////////////////////////////////////////////////
// ChemLOD.cpp: implementation of the ChemLOD class.
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

// defines
#define MI_SPHERE_LOD1 0.0f//0.15f
#define MI_SPHERE_LOD2 0.25f
#define MI_SPHERE_LOD3 0.50f
#define MI_SPHERE_LOD4 0.70f
#define MI_SPHERE_INCREMENT 0.25f

#include <string.h>
#include <assert.h>

#include "inv/ChemKit/ChemLOD.H"
#include "inv/ChemKit/ChemUnitSphere.H"
#include "inv/ChemKit/ChemUnitCylinder.H"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ChemLOD::ChemLOD()
{
	nAtoms = 0;

	sphereClipCnt = 0;
	pointCnt = 0;
	billboardCnt = 0;
	sphere1Cnt = 0;
	sphere2Cnt = 0;
	sphere3Cnt = 0;
	sphere4Cnt = 0;
	pointIndices = NULL;
	billboardIndices = NULL;
	sphere1Indices = NULL;
	sphere2Indices = NULL;
	sphere3Indices = NULL;
	sphere4Indices = NULL;
	atomMatrices = NULL;
	sphere1 = new ChemUnitSphere(MI_SPHERE_LOD1);
	sphere2 = new ChemUnitSphere(MI_SPHERE_LOD2);
	sphere3 = new ChemUnitSphere(MI_SPHERE_LOD3);
	sphere4 = new ChemUnitSphere(MI_SPHERE_LOD4);

	nBonds = 0;
	bondIndices = 0;
	bondIndicesCnt = 0;
	bondCylinder = 0;

	nResidues = 0;
	residueIndices = 0;
	residueIndicesCnt = 0;
	residueCylinder = 0;
}

ChemLOD::~ChemLOD()
{
	deleteAtoms();

	if ( sphere1 != NULL ) delete sphere1;
	if ( sphere2 != NULL ) delete sphere2;
	if ( sphere3 != NULL ) delete sphere3;
	if ( sphere4 != NULL ) delete sphere4;

	deleteBonds();
	deleteResidues();
}

void ChemLOD::deleteAtoms()
{
        // 2/9/06 KLS replaced simple deletes with delete & pointer resets 
	if ( pointIndices != NULL ) {
           delete [] pointIndices;
           pointIndices = 0;
        }
	if ( billboardIndices != NULL ) {
           delete [] billboardIndices;
           billboardIndices = 0;
        }
	if ( sphere1Indices != NULL ) {
           delete [] sphere1Indices;
           sphere1Indices = 0;
        }
	if ( sphere2Indices != NULL ) {
           delete [] sphere2Indices;
           sphere2Indices = 0;
        }
	if ( sphere3Indices != NULL ) {
           delete [] sphere3Indices;
           sphere3Indices = 0;
        }
	if ( sphere4Indices != NULL ) {
           delete [] sphere4Indices;
           sphere4Indices = 0;
        }
	if ( atomMatrices != NULL ) {
           delete [] atomMatrices;
           atomMatrices = 0;
        }

}

void ChemLOD::deleteBonds()
{	
	int i;
	if (bondIndices != 0)
	{
		for (i=0;i<5;i++)
		{
			delete [] bondIndices[i];
		}
		delete [] bondIndices;
		bondIndices = 0;
	}
	if (bondIndicesCnt != 0)
	{
		delete [] bondIndicesCnt;
		bondIndicesCnt = 0;
	}

	if (bondCylinder!=0)
	{
		for (i=1;i<5;i++)
			delete bondCylinder[i];
		delete [] bondCylinder;
		bondCylinder = 0;
	}
}

void ChemLOD::deleteResidues()
{	
	int i;
	if (residueIndices != 0)
	{
		for (i=0;i<5;i++)
		{
			delete [] residueIndices[i];
		}
		delete [] residueIndices;
	}
	if (residueIndicesCnt != 0)
	{
		delete [] residueIndicesCnt;
		residueIndicesCnt = 0;
	}

	if (residueCylinder!=0)
	{
		for (i=1;i<5;i++)
			delete residueCylinder[i];
		delete [] residueCylinder;

		residueCylinder = 0;
	}

}

void ChemLOD::allocAtoms(int32_t numberOfAtoms)
{
        // 2/9/06 KLS always reset nAtoms before deciding if we need
        // to return or re-initializing a system to 0 atoms will crash
	nAtoms = numberOfAtoms;

	if (numberOfAtoms == 0) return;

	pointIndices = new int32_t[nAtoms];
	billboardIndices = new int32_t[nAtoms];
	sphere1Indices = new int32_t[nAtoms];
	sphere2Indices = new int32_t[nAtoms];
	sphere3Indices = new int32_t[nAtoms];
	sphere4Indices = new int32_t[nAtoms];
	atomMatrices = new SbMatrix[nAtoms];
}

void ChemLOD::resetAtoms(int32_t numberOfAtoms)
{
	if (nAtoms != numberOfAtoms)
	{
		deleteAtoms();
		allocAtoms(numberOfAtoms);
	}

	sphereClipCnt = 0;
	pointCnt = 0;
	billboardCnt = 0;
	sphere1Cnt = 0;
	sphere2Cnt = 0;
	sphere3Cnt = 0;
	sphere4Cnt = 0;
}

void ChemLOD::allocBonds(int32_t numberOfBonds)
{
        // 2/9/06 KLS always reset nAtoms before deciding if we need
        // to return or re-initializing a system to 0 atoms will crash
	nBonds = numberOfBonds;

	if (numberOfBonds == 0) return;

	bondIndices = new int32_t*[5];
	bondIndicesCnt = new int32_t[5];
	bondCylinder = new ChemUnitCylinder*[5];

	int i;
	for (i=0;i<5;i++)
	{
		bondIndices[i] = new int32_t[nBonds];
	}
	
	bondCylinder[0] = 0;
	for (i=1;i<5;i++)
		bondCylinder[i] = new ChemUnitCylinder(ChemDisplayParam::BONDCYLINDER_SEMI_NOCAP,0.0f+0.15f*i);
}

void ChemLOD::allocResidues(int32_t numberOfResidues)
{
	if (numberOfResidues == 0) return;

	nResidues = numberOfResidues;
	residueIndices = new int32_t*[5];
	residueIndicesCnt = new int32_t[5];
	residueCylinder = new ChemUnitCylinder*[5];

	int i;
	for (i=0;i<5;i++)
	{
		residueIndices[i] = new int32_t[nResidues];
	}
	
	residueCylinder[0] = 0;
	for (i=1;i<5;i++)
		residueCylinder[i] = new ChemUnitCylinder(ChemDisplayParam::BONDCYLINDER_SEMI_ROUNDCAP,0.2f+0.1f*i);
}

void ChemLOD::resetBonds(int32_t numberOfBonds)
{
	if (numberOfBonds != nBonds)
	{
		deleteBonds();
		allocBonds(numberOfBonds);
	}

	if (nBonds!=0)
	{
		memset(bondIndicesCnt,0,sizeof(int32_t)*5);
	}
}

void ChemLOD::resetResidues(int32_t numberOfResidues)
{
	if (numberOfResidues != nResidues)
	{
		deleteResidues();
		allocResidues(numberOfResidues);
	}

	if (nResidues!=0)
	{
		memset(residueIndicesCnt,0,sizeof(int32_t)*5);
	}
}


void ChemLOD::changeSphereComplexity(float complexity)
{
	if ( sphere1 != NULL ) delete sphere1;
	if ( sphere2 != NULL ) delete sphere2;
	if ( sphere3 != NULL ) delete sphere3;
	if ( sphere4 != NULL ) delete sphere4;

	if (complexity <= 0.0f )
	{
		complexity = 0.1f;
	}

	if (complexity < 0.5f)
	{
		sphere1 = new ChemUnitSphere(MI_SPHERE_LOD1 - (MI_SPHERE_LOD1 * (1.0f-complexity)/2) );
		sphere2 = new ChemUnitSphere(MI_SPHERE_LOD2 - (MI_SPHERE_LOD2 * (1.0f-complexity)/2) );
		sphere3 = new ChemUnitSphere(MI_SPHERE_LOD3 - (MI_SPHERE_LOD3 * (1.0f-complexity)/2) );
		sphere4 = new ChemUnitSphere(MI_SPHERE_LOD4 - (MI_SPHERE_LOD4 * (1.0f-complexity)/2) );
	}
	else
	{
		float increment = MI_SPHERE_INCREMENT * (complexity);
		sphere1 = new ChemUnitSphere(MI_SPHERE_LOD1 + increment);
		sphere2 = new ChemUnitSphere(MI_SPHERE_LOD2 + increment);
		sphere3 = new ChemUnitSphere(MI_SPHERE_LOD3 + increment);
		sphere4 = new ChemUnitSphere(MI_SPHERE_LOD4 + increment);
	}
}

void ChemLOD::changeCylinderComplexity(float complexity, int displayStyle /*=0*/)
{
	int i;

	if (bondCylinder==0) return;

	float factor = 1.0f+complexity-0.5f;
	for (i=1;i<5;i++)
	{
		delete bondCylinder[i];
		bondCylinder[i] = new ChemUnitCylinder(displayStyle,0.0f+0.15f*factor*i);
	}
}

const int32_t& ChemLOD::getBondIndexCnt(const int lod) const
{
	assert (lod>=0 && lod < 5);
	return bondIndicesCnt[lod];
}

const int32_t& ChemLOD::getBondIndex(const int lod, const int index) const
{
	assert (lod>=0 && lod < 5);
	assert (index<bondIndicesCnt[lod]);
	return bondIndices[lod][index];
}

const ChemUnitCylinder* ChemLOD::getBondCylinder(const int lod) const
{
	assert (lod>=0 && lod < 5);
	return bondCylinder[lod];
}

ChemUnitCylinder* ChemLOD::getBondCylinder(const int lod)
{
	assert (lod>=0 && lod < 5);
	return bondCylinder[lod];
}

void ChemLOD::addBondIndex(const int lod, const int index)
{
	assert (lod>=0 && lod < 5);
	assert (bondIndicesCnt[lod]<nBonds);
	bondIndices[lod][bondIndicesCnt[lod]++]=index;
}


const int32_t& ChemLOD::getResidueIndexCnt(const int lod) const
{
	assert (lod>=0 && lod < 5);
	return residueIndicesCnt[lod];
}

const int32_t& ChemLOD::getResidueIndex(const int lod, const int index) const
{
	assert (lod>=0 && lod < 5);
	assert (index<residueIndicesCnt[lod]);
	return residueIndices[lod][index];
}

const ChemUnitCylinder* ChemLOD::getResidueCylinder(const int lod) const
{
	assert (lod>=0 && lod < 5);
	return residueCylinder[lod];
}

ChemUnitCylinder* ChemLOD::getResidueCylinder(const int lod)
{
	assert (lod>=0 && lod < 5);
	return residueCylinder[lod];
}

void ChemLOD::addResidueIndex(const int lod, const int index)
{
	assert (lod>=0 && lod < 5);
	assert (residueIndicesCnt[lod]<nResidues);
	residueIndices[lod][residueIndicesCnt[lod]++]=index;
}
