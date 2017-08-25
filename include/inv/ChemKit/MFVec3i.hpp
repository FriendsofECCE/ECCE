/*
 * MFVec3i.h
 *
 *	This file defines the MFVec3i class.
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

#ifndef  __MF_VEC3I_H__
#define  __MF_VEC3I_H__

#include "inv/fields/SoSubField.H"
#include "inv/SbLinear.H"

#include "inv/ChemKit/SbVecI.H"
#include "inv/ChemKit/ChemkitBasic.H"

//////////////////////////////////////////////////////////////////////////////
//
//  MFVec3i subclass of SoMField.
//
//////////////////////////////////////////////////////////////////////////////

// C-api: prefix=SoMV3i
class CHEMKIT_DLL_API MFVec3i : public SoMField {
    // Use standard field stuff
    SO_MFIELD_HEADER(MFVec3i, SbVec3i, const SbVec3i &);

  public:

    //
    // Some additional convenience functions:
    //

    // Set values from array of arrays of 3 floats
    // C-api: name=SetIJKs
    void        setValues(int start, int num, const int32_t ijk[][3]);

    // Set one value from 3 floats
        // C-api: name=Set1I_J_K
    void        set1Value(int index, int32_t i, int32_t j, int32_t k);

    // Set one value from 3 floats in array
        // C-api: name=Set1IJK
    void        set1Value(int index, const int32_t ijk[3]);

    // Set to one value from 3 floats
        // C-api: name=SetI_J_K
    void        setValue(int32_t i, int32_t j, int32_t k);

    // Set to one value from 3 floats in array
        // C-api: name=SetIJK
    void        setValue(const int32_t ijk[3]);

  SoINTERNAL public:
    static void         initClass();

  private:

    // Write the values out as a block of data
    virtual void        writeBinaryValues(SoOutput *out) const;
    virtual SbBool      readBinaryValues(SoInput *in, int numToRead);
};

#endif /* !__MF_VEC3I_H__ */
