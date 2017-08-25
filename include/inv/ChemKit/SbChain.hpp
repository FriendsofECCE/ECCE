/*
 * chain.h
 *
 * This file defines the SbChain, SFChain and MFChain classes.
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

#ifndef __CHAIN_H__
#define __CHAIN_H__

#include "inv/SbLinear.H"
#include "inv/fields/SoSubField.H"
#include "inv/SbLinear.H"

#include "inv/ChemKit/SbResidue.H"
#include "inv/ChemKit/ChemkitBasic.H"

// -----------------------------------
//
// Types/classes defined in this file:
// 
// -----------------------------------

class SbChain;
class SFChain;
class MFChain;

//////////////////////////////////////////////////////////////////////////////
//
//  Class: SbChain
//
//  The class represents a chain of residues.
//
//////////////////////////////////////////////////////////////////////////////

class CHEMKIT_DLL_API SbChain
{
public:

	// Default constructor
	SbChain();

	// Constructor by copy
	SbChain(const SbChain &source);

	int getNumberOfResidues() const;

	const MFResidue& getResidues() const;

	void addResidue(const SbResidue& residue);

	// Set the value using operator =
	SbChain & operator =(const SbChain &chain);

	// Equality comparison operator
	friend int          operator ==(const SbChain &c1, const SbChain &c2);
	friend int          operator !=(const SbChain &c1, const SbChain &c2);

protected:
  MFResidue residues; // Storage for residues
};

class CHEMKIT_DLL_API SFChain: public SoSField 
{
  typedef SoSField inherited;
  
  SO_SFIELD_HEADER(SFChain, SbChain, const SbChain &);

public:
  static void initClass(void);
};


class CHEMKIT_DLL_API MFChain : public SoMField {
  typedef SoMField inherited;

  SO_MFIELD_HEADER(MFChain, SbChain, const SbChain &);

public:
  static void initClass(void);

};

#endif //__CHAIN_H__
