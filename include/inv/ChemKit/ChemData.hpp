/*
 * ChemData.h
 *
 *	This file defines the ChemData node class.
 *
 * Copyright 1996, 1997, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED
 *
 * UNPUBLISHED -- Rights reserved under the copyright laws of the United
 * States.   Use of a copyright notice is precautionary only and does not
 * imply publication or disclosure.
 *
 * U.S. GOVERNMENT RESTRICTED RIGHTS LEGEND:
 * Use, duplication or disclosure by the Government is subject to restrictions
 * as set forth in FAR 52.227.19(c)(2) or subparagraph (c)(1)(ii) of the Rights
 * in Technical Data and Computer Software clause at DFARS 252.227-7013 and/or
 * in similar or successor clauses in the FAR, or the DOD or NASA FAR
 * Supplement.  Contractor/manufacturer is Silicon Graphics, Inc.,
 * 2011 N. Shoreline Blvd. Mountain View, CA 94039-7311.
 *
 * THE CONTENT OF THIS WORK CONTAINS CONFIDENTIAL AND PROPRIETARY
 * INFORMATION OF SILICON GRAPHICS, INC. ANY DUPLICATION, MODIFICATION,
 * DISTRIBUTION, OR DISCLOSURE IN ANY FORM, IN WHOLE, OR IN PART, IS STRICTLY
 * PROHIBITED WITHOUT THE PRIOR EXPRESS WRITTEN PERMISSION OF SILICON
 * GRAPHICS, INC.
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

#ident "$Revision: 22148 $"

#ifndef  __CHEM_DATA_H__
#define  __CHEM_DATA_H__


#include "inv/fields/SoSFInt32.H"
#include "inv/fields/SoMFInt32.H"
#include "inv/fields/SoMFEnum.H"
#include "inv/fields/SoMFShort.H"
#include "inv/fields/SoMFString.H"
#include "inv/fields/SoMFVec3f.H"
#include "inv/nodes/SoSubNode.H"

#include "inv/ChemKit/ChemBaseData.H"
// --> CAstick
#include "inv/ChemKit/SbResidue.H"
// <-- CAstick
// --> schematic
#include "inv/ChemKit/MFVec2i.H"
// <-- schematic

//////////////////////////////////////////////////////////////////////////////
//
//  Class: ChemData
//
//  Derived from ChemBaseData, this class stores information about the
//  chemical system
//
//////////////////////////////////////////////////////////////////////////////

class CHEMKIT_DLL_API ChemData : public ChemBaseData {

    SO_NODE_HEADER(ChemData);

  public:

    // Fields - number of atoms and bonds
    SoSFInt32   numberOfAtoms;
    SoSFInt32   numberOfBonds;

    // Fields - Atoms
    SoMFShort   atomicNumber;
    SoMFInt32   atomId;
    SoMFString  atomName;
    SoMFInt32   atomIndex;
    SoMFVec3f   atomCoordinates;

    // Fields - Bonds
    SoMFInt32   bondFrom;
    SoMFInt32   bondTo;
    SoMFEnum    bondType;
    SoMFInt32   bondIndex;

// --> CAstick
		SoSFInt32   numberOfResidues;
		SoMFString  residueName;
		SoMFInt32   residueColorIndex; // color index per residue index
		SoMFInt32   residueChainIndex; // color index per residue chain
		MFResidue residues;
// <-- CAstick

    // Constructor
    ChemData();

  SoEXTENDER public:
    // Required functions to "get" the data from a Chem Data node
    virtual int32_t getNumberOfAtoms() const
        {return numberOfAtoms.getValue(); }
    virtual int32_t getNumberOfBonds() const
        {return numberOfBonds.getValue(); }

    virtual short   getAtomicNumber(int32_t index) const
        {return atomicNumber[index];}
    virtual int32_t getAtomId(int32_t index) const
        {return atomId[index];}
    virtual SbString getAtomName(int32_t index) const
        {return atomName[index];}
    virtual int32_t getAtomIndex(int32_t index) const
        {return atomIndex[index];}
    virtual SbVec3f getAtomCoordinates(int32_t index) const
        {return atomCoordinates[index];}
 
    virtual int32_t getBondFrom(int32_t index) const
        {return bondFrom[index];}
    virtual int32_t getBondTo(int32_t index) const
        {return bondTo[index];}
    virtual int     getBondType(int32_t index) const
        {return bondType[index];}
    virtual int32_t getBondIndex(int32_t index) const
        {return bondIndex[index];}

// --> CAstick
		int32_t getNumberOfResidues() const
        {return numberOfResidues.getValue(); }
    virtual SbString getResidueName(int32_t index) const
    {
				if (residues[index].getIndex() < 0)
				{
					SbString unknown = "Unknown";
					return unknown;
				}
				else return residueName[residues[index].getIndex()];
		}
		 virtual SbString   getCustomResidueName(int32_t index) const
        {return residues[index].getName();}
    virtual short getResidueIndex(int32_t index) const
        {return residues[index].getIndex();}
    virtual short getResidueNumber(int32_t index) const
        {return residues[index].getNumber();}
		virtual char getResidueChain(int32_t index) const
        {return residues[index].getChain();}
// <-- CAstick


    // Reset all values to 0
    virtual void    reset();

    // Implement actions
    virtual void    doAction(SoAction *action);
    virtual void    callback(SoCallbackAction *action);
    virtual void    GLRender(SoGLRenderAction *action);
    virtual void    pick(SoPickAction *action);
    virtual void    getBoundingBox(SoGetBoundingBoxAction *action);

  SoINTERNAL public:
    static void     initClass();

  protected:
    virtual ~ChemData();
};

#endif /* !__CHEM_DATA_H__ */
