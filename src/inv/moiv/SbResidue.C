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

//////////////////////////////////////////////////////////////////////
// residue.cpp: implementation of the SbResidue and MFResidue classes.
//
//////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <string.h>

#include "inv/SoInput.H"
#include "inv/SoOutput.H"
#include "inv/errors/SoReadError.H"

#include "inv/ChemKit/SbResidue.H"
#include "inv/ChemKit/ChemData.H"

//////////////////////////////////////////////////////////////////////////////
//
// SbResidue class
//
//////////////////////////////////////////////////////////////////////////////

SbResidue::SbResidue()
{
	index = -1;
	number = -1;
	chain = 0;
	name = "";
	residueType.setValue(SbResidue::RESIDUE_TYPE_DEFAULT);
	atomIndices.setNum(2);
	atomIndices.set1Value(RESIDUE_CONTROL_POINT_CA,-1);
	atomIndices.set1Value(RESIDUE_CONTROL_POINT_O,-1);
}

SbResidue::~SbResidue()
{
	atomIndices.deleteValues(0,-1);
}

SbResidue::SbResidue(const SbResidue &source)
{
	setValue(source);
}

void SbResidue::setValue(const SbResidue &source)
{
	number = source.number;
	name = source.name;
	index = source.index;
	chain = source.chain;
	residueType.setValue(source.residueType.getValue());
	atomIndices.copyFrom(source.getAtomIndices());
}

void SbResidue::addAtomIndex(const int32_t index, int cp/* = RESIDUE_CONTROL_POINT_NONE*/)
{
	if (cp == RESIDUE_CONTROL_POINT_NONE)
		atomIndices.set1Value(atomIndices.getNum(),index);
	else
		atomIndices.set1Value(cp,index);
}

bool SbResidue::removeAtomIndex(const int32_t index)
{
	SoMFInt32 newAtomIndices;
	int i;
	bool bFound = false;
	for (i=0; i<atomIndices.getNum(); i++)
	{
		if (atomIndices[i] == index)
		{
			bFound = true;
			if (i == RESIDUE_CONTROL_POINT_CA || i == RESIDUE_CONTROL_POINT_O)
				newAtomIndices.set1Value(i,-1);
		}
		else
			newAtomIndices.set1Value(newAtomIndices.getNum(),atomIndices[i]);
	}

	if (bFound)
	{
		atomIndices.copyFrom(newAtomIndices);
	}

	return bFound;
}

SbString SbResidue::getName() const
{
	return name;
}

void SbResidue::setName(const SbString &newName)
{
	name = newName;
}

short SbResidue::getNumber() const
{
	return number;
}

void SbResidue::setChain(const unsigned char newChain)
{
	chain = newChain;
}

unsigned char SbResidue::getChain() const
{
	return chain;
}

void SbResidue::setNumber(const short newNumber)
{
	number = newNumber;
}

short SbResidue::getIndex() const
{
	return index;
}

void SbResidue::setIndex(const short newIndex)
{
	index = newIndex;
}

const SoMFInt32& SbResidue::getAtomIndices() const
{
	return atomIndices;
}

bool SbResidue::checkControlPoints() const
{
	return (atomIndices[0] != -1 && atomIndices[1] != -1);
}

int SbResidue::search(const char *atomName, const ChemData *chemData) const
{
	for (int i=0; i<atomIndices.getNum(); i++)
	{
		const char *string = chemData->getAtomName(atomIndices[i]).getString();
		
		if (strcmp(atomName,string) == 0)
		{
			return atomIndices[i];
		}
	}

	return -1;
}

SbResidue & SbResidue::operator =(const SbResidue &source)
{
	setValue(source);
	return *this;
}

  // Equality comparison operator
int operator ==(const SbResidue &a, const SbResidue &b)
{
	if (a.number != b.number ||  (a.index != b.index) || (a.name!= b.name) || (a.chain!= b.chain))
		return 0;
	const int anum = a.getAtomIndices().getNum();
	const int bnum = b.getAtomIndices().getNum();
	
	if ( anum != bnum) return 0;

	for (int i=0; i<anum; i++)
	{
		if (a[i] != b[i])
			return 0;
	}

	return 1;
}

int operator !=(const SbResidue &a, const SbResidue &b)
{
	return !(a==b);
}

bool SbResidue::computeCenterOfMass(const ChemData* chemData, SbVec3f &cm) const
{
	bool anyPoint = false;
	int i,nPoints;
	nPoints = 0;
	cm.setValue(0.0f,0.0f,0.0f);
	for (i=0;i<atomIndices.getNum(); i++)
	{
		if (atomIndices[i]  == -1) continue;
		anyPoint = true;
		nPoints++;
		cm += chemData->getAtomCoordinates(atomIndices[i]);
	}

	if (nPoints > 1) cm /= (float)nPoints;
	
	return anyPoint;
}

//////////////////////////////////////////////////////////////////////////////
//
// SFResidue class
//
//////////////////////////////////////////////////////////////////////////////

SO_SFIELD_SOURCE(SFResidue, SbResidue, const SbResidue &);

SbBool sfresidue_read_value(SoInput * in, SbResidue & val)
{
  return false;
}

SbBool SFResidue::readValue(SoInput * in)
{
	return false;
}

void sfresidue_write_value(SoOutput * out, const SbResidue &val)
{
}

void SFResidue::writeValue(SoOutput * out) const
{
  sfresidue_write_value(out, this->getValue());
}

//////////////////////////////////////////////////////////////////////////////
//
// MFResidue class
//
//////////////////////////////////////////////////////////////////////////////

SO_MFIELD_SOURCE(MFResidue, SbResidue, const SbResidue &);

SbBool MFResidue::read1Value(SoInput *in, int index)
{
	SbResidue val;
  if (!sfresidue_read_value(in, val)) return FALSE;
  this->set1Value(index, val);
  return TRUE;
}

void MFResidue::write1Value(SoOutput *out, int index) const
{
	sfresidue_write_value(out, (*this)[index]);
}
