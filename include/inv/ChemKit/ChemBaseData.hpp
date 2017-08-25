/*
 * ChemBaseData.h
 *
 *	This file defines the ChemBaseData node class.
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

#ifndef  __CHEM_BASE_DATA_H__
#define  __CHEM_BASE_DATA_H__

#include "inv/SbLinear.H"

#include "inv/fields/SoSFInt32.H"
#include "inv/fields/SoSFNode.H"
#include "inv/fields/SoMFEnum.H"

#include "inv/nodes/SoSubNode.H"

#include "inv/ChemKit/ChemkitBasic.H"

//////////////////////////////////////////////////////////////////////////////
//
//  Class: ChemBaseData
//
//  SoNode class which serves as the base class for all chemical data nodes
//
//////////////////////////////////////////////////////////////////////////////

class CHEMKIT_DLL_API ChemBaseData : public SoNode {

    SO_NODE_ABSTRACT_HEADER(ChemBaseData);

  public:
    enum BondType {
        NONE,
        SINGLE_BOND,
        DOUBLE_BOND,
        TRIPLE_BOND,
        QUADRUPLE_BOND,
        AROMATIC_BOND,
        HYDROGEN_BOND,
        RESONANCE_BOND
    };

    // Fields - Data not required by Molecular Inventor but which goes
    // along with the chemical system
    SoSFNode     associatedData;

  SoEXTENDER public:
    // Required functions to "get" the data from a ChemBaseData-derived node
    virtual int32_t getNumberOfAtoms() const = 0;
    virtual int32_t getNumberOfBonds() const = 0;

    virtual short   getAtomicNumber(int32_t index) const = 0;
    virtual int32_t getAtomId(int32_t index) const = 0;
    virtual SbString getAtomName(int32_t index) const = 0;
    virtual int32_t getAtomIndex(int32_t index) const = 0;
    virtual SbVec3f getAtomCoordinates(int32_t index) const = 0;

    virtual int32_t getBondFrom(int32_t index) const = 0;
    virtual int32_t getBondTo(int32_t index) const = 0;
    virtual int     getBondType(int32_t index) const = 0;
    virtual int32_t getBondIndex(int32_t index) const = 0;

    // Implement actions
    virtual void    doAction(SoAction *action);
    virtual void    callback(SoCallbackAction *action);
    virtual void    GLRender(SoGLRenderAction *action);
    virtual void    pick(SoPickAction *action);
    virtual void    getBoundingBox(SoGetBoundingBoxAction *action);


  SoINTERNAL public:
    static void     initClass();

  protected:
    // Constructor, destructor
    ChemBaseData();
    virtual ~ChemBaseData();
};

#endif /* !__CHEM_BASE_DATA_H__ */
