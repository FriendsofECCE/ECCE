/*
 * ChemDetail.h
 *
 *	This file defines the ChemDetail class.
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

#ifndef __CHEM_DETAIL_H__
#define __CHEM_DETAIL_H__

#include "inv/details/SoSubDetail.H"

#include "inv/ChemKit/ChemkitBasic.H"

//////////////////////////////////////////////////////////////////////////////
//
//  Class: ChemDetail
//
//  Detail containing the index of the picked atom or bond
//
//////////////////////////////////////////////////////////////////////////////

class CHEMKIT_DLL_API ChemDetail : public SoDetail {

    SO_DETAIL_HEADER(ChemDetail);

  public:
    // Constructor and destructor
    ChemDetail();
    virtual ~ChemDetail();

    // Returns the index of the atom and/or bond
    int32_t      getAtomIndex() const { return atomIndex; }
    int32_t      getBondIndex() const { return bondIndex; }
    int32_t      getAtomLabelIndex() const { return atomLabelIndex; }
    int32_t      getBondLabelIndex() const { return bondLabelIndex; }
// --> residue selection
		int32_t      getResidueIndex() const { return residueIndex; }
    int32_t      getResidueLabelIndex() const { return residueLabelIndex; }
// <-- residue selection
// --> schematic
    int32_t      getSchematicIndex() const { return schematicIndex; }
// <-- schematic

    virtual void getAtomBondIndex(int32_t &_atomIndex, 
                                  int32_t &_bondIndex) const
        { _atomIndex = atomIndex; _bondIndex = bondIndex; }
    virtual void getAtomBondLabelIndex(int32_t &_atomLabelIndex, 
                                       int32_t &_bondLabelIndex) const
        { _atomLabelIndex = atomLabelIndex; _bondLabelIndex = bondLabelIndex; }

    // Returns an instance that is a copy of this instance. The caller
    // is responsible for deleting the copy when done.
    virtual SoDetail * copy() const;

  SoEXTENDER public:
    // Sets the index of the atom and/or bond selected
    virtual void setAtomIndex(int32_t _index) { atomIndex = _index; }
    virtual void setBondIndex(int32_t _index) { bondIndex = _index; }
    virtual void setAtomLabelIndex(int32_t _index) { atomLabelIndex = _index; }
    virtual void setBondLabelIndex(int32_t _index) { bondLabelIndex = _index; }
    virtual void setAtomBondIndex(int32_t _atomIndex, int32_t _bondIndex)	
        { atomIndex = _atomIndex; bondIndex = _bondIndex; }
    virtual void setAtomBondLabelIndex(int32_t _atomLabelIndex, 
                                       int32_t _bondLabelIndex)	
        { atomLabelIndex = _atomLabelIndex; bondLabelIndex = _bondLabelIndex; }
// --> residue selection
		virtual void setResidueIndex(int32_t _index) { residueIndex = _index; }
		virtual void setResidueLabelIndex(int32_t _index) { residueLabelIndex = _index; }
// <-- residue selection
// --> schematic
    virtual void setSchematicIndex(int32_t _index) { schematicIndex = _index; }
// <-- schematic

  SoINTERNAL public:
    static void initClass();
    
  protected:
    int32_t atomIndex;
    int32_t bondIndex;
    int32_t atomLabelIndex;
    int32_t bondLabelIndex;
// --> residue selection
		int32_t residueIndex;
		int32_t residueLabelIndex;
// <-- residue selection
// --> schematic
    int32_t schematicIndex;
// <-- schematic
};

#endif /* !__CHEM_DETAIL_H__ */
