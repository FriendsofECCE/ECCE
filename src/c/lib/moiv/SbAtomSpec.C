/*
 * SbAtomSpec.c++
 *
 *     Classes:  SbAtomSpec
 *
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

#ident "$Revision: 22141 $"

#include "inv/ChemKit/SbAtomSpec.H"
#include "inv/ChemKit/ChemBaseData.H"
// --> lod
#include "inv/ChemKit/ChemLOD.H"
// <-- lod
#include "inv/ChemKit/ChemDisplay.H"

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Constructor
//
// Use: public

SbAtomSpec::SbAtomSpec()
//
////////////////////////////////////////////////////////////////////////
{
    chemData = NULL;
    chemDisplay = NULL;
    theAtom = -1;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Copy constructor
//
// Use: public

SbAtomSpec::SbAtomSpec(const SbAtomSpec &atomSpec)
//
////////////////////////////////////////////////////////////////////////
{
    setValue(atomSpec);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Set the values
//
// Use: public

void
SbAtomSpec::setValue(const SbAtomSpec &atomSpec)
//
////////////////////////////////////////////////////////////////////////
{
    atomSpec.getValue(chemData, chemDisplay, theAtom);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Set the values
//
// Use: public

void
SbAtomSpec::setValue(ChemBaseData *_chemData,
    ChemDisplay *_chemDisplay, int32_t _theAtom)
//
////////////////////////////////////////////////////////////////////////
{
    chemData = _chemData;
    chemDisplay = _chemDisplay;
    theAtom = _theAtom;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Get the values
//
// Use: public

void
SbAtomSpec::getValue(ChemBaseData *&_chemData,
    ChemDisplay *&_chemDisplay, int32_t &_theAtom) const
//
////////////////////////////////////////////////////////////////////////
{
    _chemData = chemData;
    _chemDisplay = chemDisplay;
    _theAtom = theAtom;
}

////////////////////////////////////////////////////////////////////////
// 
// Description:
//    Sets the value using operator =
//
// Use: public

SbAtomSpec &
SbAtomSpec::operator =(const SbAtomSpec &atomSpec)
// 
////////////////////////////////////////////////////////////////////////
{
    setValue(atomSpec);
    return (*this);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Equality comparison operator
//
// Use: public

int
operator ==(const SbAtomSpec &a1, const SbAtomSpec &a2)
//
////////////////////////////////////////////////////////////////////////
{
    return ((a1.chemData == a2.chemData) &&
            (a1.chemDisplay == a2.chemDisplay) &&
            (a1.theAtom == a2.theAtom));
}

//////////////////////////////////////////////////////////////////////////////
//
// Vec2AtomSpec class
//
//////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Constructor
//
// Use: public

SbVec2AtomSpec::SbVec2AtomSpec()
//
////////////////////////////////////////////////////////////////////////
{
    ChemBaseData *chemData = NULL;
    ChemDisplay *chemDisplay = NULL;
    int32_t theAtom = -1;

    vec[0].setValue(chemData, chemDisplay, theAtom);
    vec[1].setValue(chemData, chemDisplay, theAtom);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Copy constructor
//
// Use: public

SbVec2AtomSpec::SbVec2AtomSpec(const SbVec2AtomSpec &atomSpec)
//
////////////////////////////////////////////////////////////////////////
{
    setValue(atomSpec.getValue());
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Sets the value from another SbVec2AtomSpec
//

void
SbVec2AtomSpec::setValue(const SbVec2AtomSpec &atomSpec)
//
////////////////////////////////////////////////////////////////////////
{
    setValue(atomSpec.getValue());
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Sets value of vector from array of 2 components
//

SbVec2AtomSpec &
SbVec2AtomSpec::setValue(const SbAtomSpec v[2])     
//
////////////////////////////////////////////////////////////////////////
{
    vec[0] = v[0];
    vec[1] = v[1];

    return (*this);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Sets value of vector from 2 individual components
//

SbVec2AtomSpec &
SbVec2AtomSpec::setValue(const SbAtomSpec &v1, const SbAtomSpec &v2)     
//
////////////////////////////////////////////////////////////////////////
{
    vec[0] = v1;
    vec[1] = v2;

    return (*this);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Returns 2 individual components
//

void
SbVec2AtomSpec::getValue(SbAtomSpec &v1, SbAtomSpec &v2) const
//
////////////////////////////////////////////////////////////////////////
{
    v1 = vec[0];
    v2 = vec[1];
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Sets the value using operator =

SbVec2AtomSpec &
SbVec2AtomSpec::operator =(const SbVec2AtomSpec &atomSpec)
//
////////////////////////////////////////////////////////////////////////
{
    setValue(atomSpec.getValue());
    return (*this);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Equality comparison operator
//

int
operator ==(const SbVec2AtomSpec &v1, const SbVec2AtomSpec &v2)
//
////////////////////////////////////////////////////////////////////////
{
    return (v1.vec[0] == v2.vec[0] &&
            v1.vec[1] == v2.vec[1]);
}

//////////////////////////////////////////////////////////////////////////////
//
// Vec3AtomSpec class
//
//////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// 
// Description:
//    Constructor
// 
// Use: public

SbVec3AtomSpec::SbVec3AtomSpec()
//
////////////////////////////////////////////////////////////////////////
{
    ChemBaseData *chemData = NULL;
    ChemDisplay *chemDisplay = NULL;
    int32_t theAtom = -1;
  
    vec[0].setValue(chemData, chemDisplay, theAtom);
    vec[1].setValue(chemData, chemDisplay, theAtom);
    vec[2].setValue(chemData, chemDisplay, theAtom);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Copy constructor
//
// Use: public

SbVec3AtomSpec::SbVec3AtomSpec(const SbVec3AtomSpec &atomSpec)
//
////////////////////////////////////////////////////////////////////////
{
    setValue(atomSpec.getValue());
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Sets value of vector from an SbVec3AtomSpec
//

void
SbVec3AtomSpec::setValue(const SbVec3AtomSpec &atomSpec)
//
////////////////////////////////////////////////////////////////////////
{
    setValue(atomSpec.getValue());
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Sets value of vector from array of 3 components
//

SbVec3AtomSpec &
SbVec3AtomSpec::setValue(const SbAtomSpec v[3])     
//
////////////////////////////////////////////////////////////////////////
{
    vec[0] = v[0];
    vec[1] = v[1];
    vec[2] = v[2];

    return (*this);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Sets value of vector from 3 individual components
//

SbVec3AtomSpec &
SbVec3AtomSpec::setValue(const SbAtomSpec &v1, const SbAtomSpec &v2,
    const SbAtomSpec &v3)
//
////////////////////////////////////////////////////////////////////////
{
    vec[0] = v1;
    vec[1] = v2;
    vec[2] = v3;

    return (*this);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Returns 3 individual components
//

void
SbVec3AtomSpec::getValue(SbAtomSpec &i, SbAtomSpec &j, SbAtomSpec &k) const
//
////////////////////////////////////////////////////////////////////////
{
    i = vec[0];
    j = vec[1];
    k = vec[2];
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Set the value using the operator =
//

SbVec3AtomSpec &
SbVec3AtomSpec::operator =(const SbVec3AtomSpec &atomSpec)
//
////////////////////////////////////////////////////////////////////////
{
    setValue(atomSpec.getValue());
    return (*this);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Equality comparison operator. Componenents must match exactly.
//

int
operator ==(const SbVec3AtomSpec &v1, const SbVec3AtomSpec &v2)
//
////////////////////////////////////////////////////////////////////////
{
    return (v1.vec[0] == v2.vec[0] &&
            v1.vec[1] == v2.vec[1] &&
            v1.vec[2] == v2.vec[2]);
}


//////////////////////////////////////////////////////////////////////////////
//
// Vec4AtomSpec class
//
//////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// 
// Description:
//    Constructor
// 
// Use: public

SbVec4AtomSpec::SbVec4AtomSpec()
//
////////////////////////////////////////////////////////////////////////
{
    ChemBaseData *chemData = NULL;
    ChemDisplay *chemDisplay = NULL;
    int32_t theAtom = -1;
  
    vec[0].setValue(chemData, chemDisplay, theAtom);
    vec[1].setValue(chemData, chemDisplay, theAtom);
    vec[2].setValue(chemData, chemDisplay, theAtom);
    vec[3].setValue(chemData, chemDisplay, theAtom);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Copy constructor
//
// Use: public

SbVec4AtomSpec::SbVec4AtomSpec(const SbVec4AtomSpec &atomSpec)
//
////////////////////////////////////////////////////////////////////////
{
    setValue(atomSpec.getValue());
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Sets value of vector from an SbVec4AtomSpec
//

void
SbVec4AtomSpec::setValue(const SbVec4AtomSpec &atomSpec)
//
////////////////////////////////////////////////////////////////////////
{
    setValue(atomSpec.getValue());
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Sets value of vector from array of 4 components
//

SbVec4AtomSpec &
SbVec4AtomSpec::setValue(const SbAtomSpec v[4])     
//
////////////////////////////////////////////////////////////////////////
{
    vec[0] = v[0];
    vec[1] = v[1];
    vec[2] = v[2];
    vec[3] = v[3];

    return (*this);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Sets value of vector from 4 individual components
//

SbVec4AtomSpec &
SbVec4AtomSpec::setValue(const SbAtomSpec &v1, const SbAtomSpec &v2,
    const SbAtomSpec &v3, const SbAtomSpec &v4)
//
////////////////////////////////////////////////////////////////////////
{
    vec[0] = v1;
    vec[1] = v2;
    vec[2] = v3;
    vec[3] = v4;

    return (*this);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Returns 4 individual components
//

void
SbVec4AtomSpec::getValue(SbAtomSpec &i, SbAtomSpec &j, SbAtomSpec &k,
    SbAtomSpec &l) const
//
////////////////////////////////////////////////////////////////////////
{
    i = vec[0];
    j = vec[1];
    k = vec[2];
    l = vec[3];
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Sets the value using the operator =
//

SbVec4AtomSpec &
SbVec4AtomSpec::operator =(const SbVec4AtomSpec &atomSpec)
//
////////////////////////////////////////////////////////////////////////
{
    setValue(atomSpec.getValue());
    return (*this);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Equality comparison operator. Componenents must match exactly.
//

int
operator ==(const SbVec4AtomSpec &v1, const SbVec4AtomSpec &v2)
//
////////////////////////////////////////////////////////////////////////
{
    return (v1.vec[0] == v2.vec[0] &&
            v1.vec[1] == v2.vec[1] &&
            v1.vec[2] == v2.vec[2] &&
            v1.vec[3] == v2.vec[3]);
}

