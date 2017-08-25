/*
 * ChemLattice3.h
 *
 *	This file defines the ChemLattice3 node class.
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

#ifndef  __CHEM_LATTICE3_H__
#define  __CHEM_LATTICE3_H__

#include "inv/fields/SoSFName.H"
#include "inv/fields/SoSFShort.H"
#include "inv/fields/SoSFVec3f.H"
#include "inv/fields/SoMFFloat.H"
#include "inv/nodes/SoSubNode.H"

#include "inv/ChemKit/SFVec3i.H"
#include "inv/ChemKit/ChemkitBasic.H"

enum LatticeDataType {
// --> sas
	  LATTICE_DATA_BOOL,
// <-- sas
    LATTICE_DATA_INT8,
    LATTICE_DATA_INT16,
    LATTICE_DATA_INT32,
    LATTICE_DATA_FLOAT32,
    LATTICE_DATA_FLOAT64,
    LATTICE_DATA_BAD_TYPE
};

enum LatticeCoordType {
    LATTICE_COORD_UNIFORM,
    LATTICE_COORD_RECTILINEAR,
    LATTICE_COORD_BAD_TYPE
};

//////////////////////////////////////////////////////////////////////////////
//
//  Class: ChemLattice3
//
//  SoNode class that contains a 3D grid of data
//
//////////////////////////////////////////////////////////////////////////////

class CHEMKIT_DLL_API ChemLattice3 : public SoNode {

    SO_NODE_HEADER(ChemLattice3);

  public:

    // Fields:
    SFVec3i   dimension;  // number of data points in the 3 dimensions
    SoSFShort nDataVar;   // number of data variables at each point
    SoMField  *data;      // data values
    SoMFFloat coord;      // coordinate values
                          //     uniform - minX, maxX, minY, maxY, minZ, maxZ
                          //     rectilinear - all X, all Y, all Z

    // Methods
    //    Constructor
    ChemLattice3(LatticeDataType, LatticeCoordType);

    LatticeDataType  getDataType() { return _dataType; }
    LatticeCoordType getCoordType() { return _coordType; }
    SbVec3f          getCoord(int32_t index[3]) const;

  SoEXTENDER public:
    // Implement actions
    virtual void    doAction(SoAction *action);
    virtual void    callback(SoCallbackAction *action);
    virtual void    GLRender(SoGLRenderAction *action);
    virtual void    pick(SoPickAction *action);
    virtual void    getBoundingBox(SoGetBoundingBoxAction *action);

    // ChemLattice3 must write out its data type before writing the
    // actual data
    virtual void    write(SoWriteAction *action);

  SoINTERNAL public:
    static void     initClass();

  protected:
    virtual SbBool readInstance(SoInput *in, unsigned short flags);
    /* 
       virtual ~ChemLattice3(); was moved from private mode by fabien fontaine
       the 13/12/2000 to avoid g++ compiler warning. 
    */
    virtual ~ChemLattice3();

  private:
    // Constructor
    ChemLattice3();
    // Destructor
    /* 
       Moved to protected mode by fabien fontaine the 13/12/2000 to avoid 
       g++ compiler warning
       virtual ~ChemLattice3();
    */
    void setup(LatticeDataType dataType);
    void setup(SoType inputType);

    LatticeDataType  _dataType;
    LatticeCoordType _coordType;

    SoType theSoType;
    SoSFName typeField;          // used when reading/writing
    SoFieldData *theFieldData;


};

#endif  /* !__CHEM_LATTICE3_H__ */
