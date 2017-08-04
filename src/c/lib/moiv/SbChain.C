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
// chain.cpp: implementation of the SbChain, SFChain and MFChain classes.
//
//////////////////////////////////////////////////////////////////////

#include "inv/SoInput.H"
#include "inv/SoOutput.H"
#include "inv/errors/SoReadError.H"

#include "inv/ChemKit/SbChain.H"

//////////////////////////////////////////////////////////////////////////////
//
// SbChain class
//
//////////////////////////////////////////////////////////////////////////////

SbChain::SbChain()
{
}

// Constructor by copy
SbChain::SbChain(const SbChain &source)
{
}

int SbChain::getNumberOfResidues() const
{
	return residues.getNum();
}

const MFResidue& SbChain::getResidues() const
{
	return residues;
}

void SbChain::addResidue(const SbResidue& residue)
{
	residues.set1Value(residues.getNum(), residue);

}

// Set the value using operator =
SbChain & SbChain::operator =(const SbChain &chain)
{
	residues.startEditing();

	residues.deleteValues(0,-1);

	for (int i=0; i<chain.getNumberOfResidues(); i++)
		residues.set1Value(i, chain.getResidues()[i]);

	residues.finishEditing();

	return *this;

}

// Equality comparison operator
int operator ==(const SbChain &c1, const SbChain &c2)
{
	return (c1.getResidues() == c2.getResidues());
}

int operator !=(const SbChain &c1, const SbChain &c2)
{
	return !(c1==c2);
}


//////////////////////////////////////////////////////////////////////////////
//
// SFChain class
//
//////////////////////////////////////////////////////////////////////////////

extern SbBool sfresidue_read_value(SoInput * in, SbResidue & val);
extern void sfresidue_write_value(SoOutput * out, const SbResidue &val);

SO_SFIELD_SOURCE(SFChain, SbChain, const SbChain&);

SbBool sfchain_read_value(SoInput * in, SbChain & val)
{
	int numResidues = 0;

  if (!in->read(numResidues))
	{
    SoReadError::post(in, "Premature end of file");
    return FALSE;
  }

	for (int i=0; i<numResidues; i++)
	{
		SbResidue sbresidue;
		if (sfresidue_read_value(in,sbresidue))
		{
			SoReadError::post(in, "Premature end of file");
			return FALSE;
		}
		val.addResidue(sbresidue);
	}
  return TRUE;
}

SbBool SFChain::readValue(SoInput * in)
{
  SbChain val;
  if (!sfchain_read_value(in, val)) return FALSE;
  this->setValue(val);
  return TRUE;
}

void sfchain_write_value(SoOutput * out, const SbChain &val)
{
	// first, write number of residues
	const int num = val.getNumberOfResidues();
  out->write(num);
	// later, the residue indexes
	for (int i=0;i<num; i++)
		sfresidue_write_value(out, val.getResidues()[i]);
}

void SFChain::writeValue(SoOutput * out) const
{
  sfchain_write_value(out, this->getValue());
}


//////////////////////////////////////////////////////////////////////////////
//
// MFChain class
//
//////////////////////////////////////////////////////////////////////////////

// Use standard definitions of all basic methods
SO_MFIELD_SOURCE(MFChain, SbChain, const SbChain &);

SbBool MFChain::read1Value(SoInput *in, int index)
{
	SbChain val;
  if (!sfchain_read_value(in, val)) return FALSE;
  this->set1Value(index, val);
  return TRUE;
}

void MFChain::write1Value(SoOutput *out, int index) const
{
	sfchain_write_value(out, (*this)[index]);
}
