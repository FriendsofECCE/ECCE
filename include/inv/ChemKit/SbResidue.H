//
// residue.h
//
// This file defines the SbResidue, SFResidue and MFResidue classes.
//
//
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

#ifndef __RESIDUE_H__
#define __RESIDUE_H__

#include "inv/SbLinear.H"
#include "inv/fields/SoSubField.H"
#include "inv/fields/SoMFInt32.H"
#include "inv/SbString.H" 
#include "inv/SbPList.H"
#include "inv/fields/SoSFEnum.H"

#include "inv/ChemKit/ChemkitBasic.H"

class ChemData;
class SbResidue;
class SFResidue;
class MFResidue;

//////////////////////////////////////////////////////////////////////////////
//
//  Class: SbResidue
//
//  The class represents a residue
//
//////////////////////////////////////////////////////////////////////////////

class CHEMKIT_DLL_API SbResidue
{
public:

  // Default constructor
	SbResidue();

	~SbResidue();

  // Copy constructor
  SbResidue(const SbResidue &source);

	// Set the value from another SbResidue
  void setValue(const SbResidue &source);

  // Add an atom index to the array
  void addAtomIndex(const int32_t index, int cp = RESIDUE_CONTROL_POINT_NONE);

	// Search and remove index from atom indices
	// Returns true if index is found
  bool removeAtomIndex(const int32_t index);

	// Get the value from atomIndices[index]
	int32_t operator [](int index) const
	{
		return atomIndices[index];
	}

	// Get/Set the index to a closed list of residues
  short getIndex() const;
  void setIndex(const short newIndex);

	// Get/Set the sequencial residue number
  short getNumber() const;
  void setNumber(const short newNumber);

	SbString getName() const;
  void setName(const SbString &newName);

	unsigned char getChain() const;
  void setChain(const unsigned char newChain);

	const SoMFInt32& getAtomIndices() const;

	bool checkControlPoints() const;

	// Searching method to find control points
	// return -1 if the control point is not found
	int search(const char *atomName,const ChemData *chemData) const;
	
	// Computing center of mass
	bool computeCenterOfMass(const ChemData* chemData, SbVec3f &cm) const;

  // Set the value using operator =
  SbResidue & operator =(const SbResidue &source);

  // Equality comparison operator
  friend int operator ==(const SbResidue &a, const SbResidue &b);
	friend int operator !=(const SbResidue &a, const SbResidue &b);

	SoSFEnum residueType;

	enum ResidueTypeBinding
	{
		RESIDUE_TYPE_UNKNOWN = 0,
		RESIDUE_TYPE_HELIX,
		RESIDUE_TYPE_SHEET,
		RESIDUE_TYPE_COIL,
		RESIDUE_TYPE_TURN,
		RESIDUE_TYPE_CUSTOM,
    RESIDUE_TYPE_DEFAULT = RESIDUE_TYPE_COIL
	};

	enum ResidueControlPointBinding
	{
		RESIDUE_CONTROL_POINT_NONE = -1,
		RESIDUE_CONTROL_POINT_CA = 0,
		RESIDUE_CONTROL_POINT_O = 1
	};

protected:
	short index;
	short number;
	SbString name;
	unsigned char chain;
  SoMFInt32 atomIndices; // Storage for atom indices
};

//////////////////////////////////////////////////////////////////////////////
//
//  Class: SFResidue
//
//  The class represents a single field which contains a single residue
//
//////////////////////////////////////////////////////////////////////////////

class SFResidue  : public SoSField
{
  typedef SoSField inherited;

  SO_SFIELD_HEADER(SFResidue, SbResidue, const SbResidue &);

public:
  static void initClass(void);
};

//////////////////////////////////////////////////////////////////////////////
//
//  Class: MFResidue
//
//  The class represents a multiple field container of residues
//
//////////////////////////////////////////////////////////////////////////////

class MFResidue : public SoMField
{
	// Use standard field stuff
	SO_MFIELD_HEADER(MFResidue, SbResidue, const SbResidue &);
public:
SoINTERNAL public:
	static void initClass();

	SbResidue& operator [](int index)
	{
		return values[index];
	}

};

#endif // !__RESIDUE_H__
