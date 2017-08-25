/*
 * SbAtomSpec.h
 *
 *	This file defines the SbAtomSpec class.
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

#ifndef __SB_ATOMSPEC_H__
#define __SB_ATOMSPEC_H__

#include "inv/SbBasic.H"

#include "inv/ChemKit/ChemkitBasic.H"

class ChemBaseData;
class ChemDisplay;

// -----------------------------------
//
// Types/classes defined in this file:
// 
// -----------------------------------

class SbVec2AtomSpec;
class SbVec3AtomSpec;
class SbVec4AtomSpec;

//////////////////////////////////////////////////////////////////////////////
//
//  Class: SbAtomSpec
//
//  Specifies an atom by its ChemBaseData and ChemDisplay "containers".
//  The ChemBaseData is used to get the properties of the atom and the
//  ChemDisplay is used to get any transformations applied to the atom.
//
//////////////////////////////////////////////////////////////////////////////

class CHEMKIT_DLL_API SbAtomSpec {
  public:

    // Default constructor
    SbAtomSpec();

    // Copy constructor
    SbAtomSpec(const SbAtomSpec &atomSpec);

    // Constructor given individual components
    SbAtomSpec(ChemBaseData *chemData,
               ChemDisplay *chemDisplay,
               int32_t theAtom) { setValue(chemData, chemDisplay, theAtom); }

    // Set the value from another SbAtomSpec
    void setValue(const SbAtomSpec &atomSpec);

    // Sets value of the 3 components
    void setValue(ChemBaseData *chemData,
                  ChemDisplay *chemDisplay,
                  int32_t theAtom);

    // Returns the 3 components
    void getValue(ChemBaseData *&chemData,
                  ChemDisplay *&chemDisplay,
                  int32_t &theAtom) const;

    // Set the value using operator =
    SbAtomSpec & operator =(const SbAtomSpec &atomSpec);

    // Equality comparison operator
    friend int operator ==(const SbAtomSpec &a1, const SbAtomSpec &a2);
    friend int operator !=(const SbAtomSpec &a1, const SbAtomSpec &a2)
        { return !(a1 == a2); }

  private:
    ChemBaseData *chemData;
    ChemDisplay  *chemDisplay;
    int32_t       theAtom;
};

class CHEMKIT_DLL_API SbVec2AtomSpec {

  public:

    // Default constructor
    SbVec2AtomSpec();

    // Copy constructor
    SbVec2AtomSpec(const SbVec2AtomSpec &atomSpec);

    // Constructor given an array of 2 components
    SbVec2AtomSpec(const SbAtomSpec v[2])
        { setValue(v); }

    // Constructor given 2 individual components
    SbVec2AtomSpec(const SbAtomSpec &v1, const SbAtomSpec &v2)
        { setValue(v1, v2); }

    // Sets the value of vector from an SbVec2AtomSpec
    void setValue(const SbVec2AtomSpec &atomSpec);

    // Sets value of vector from array of 2 components
    SbVec2AtomSpec &setValue(const SbAtomSpec v[2]);

    // Sets value of vector from 2 individual components
    SbVec2AtomSpec &setValue(const SbAtomSpec &v1, const SbAtomSpec &v2);

    // Returns pointer to array of 2 components
    const SbAtomSpec *getValue() const
        { return vec; }

    // Returns 2 individual components
    void getValue(SbAtomSpec &v1, SbAtomSpec &v2) const;

    // Accesses indexed component of vector
    SbAtomSpec       &operator [](int i)       { return vec[i]; }
    const SbAtomSpec &operator [](int i) const { return vec[i]; }

    SbVec2AtomSpec & operator =(const SbVec2AtomSpec &atomSpec);

    // Equality comparison operator
    friend int operator ==(const SbVec2AtomSpec &v1, const SbVec2AtomSpec &v2);
    friend int operator !=(const SbVec2AtomSpec &v1, const SbVec2AtomSpec &v2)
        { return !(v1 == v2); }

  protected:
    SbAtomSpec vec[2];
};

class CHEMKIT_DLL_API SbVec3AtomSpec {

  public:

    // Default constructor
    SbVec3AtomSpec();

    // Copy constructor
    SbVec3AtomSpec(const SbVec3AtomSpec &atomSpec);

    // Constructor given an array of 3 components
    SbVec3AtomSpec(const SbAtomSpec v[3])
        { setValue(v); }

    // Constructor given 3 individual components
    SbVec3AtomSpec(const SbAtomSpec &v1, const SbAtomSpec &v2,
                   const SbAtomSpec &v3)
        { setValue(v1, v2, v3); }

    // Sets value of vector from an SbVec3AtomSpec
    void setValue(const SbVec3AtomSpec &atomSpec);

    // Sets value of vector from array of 3 components
    SbVec3AtomSpec &setValue(const SbAtomSpec v[3]);

    // Sets value of vector from 3 individual components
    SbVec3AtomSpec &setValue(const SbAtomSpec &v1, const SbAtomSpec &v2,
                             const SbAtomSpec &v3);

    // Returns pointer to array of 3 components
    const SbAtomSpec *getValue() const
        { return vec; }

    // Returns 3 individual components
    void getValue(SbAtomSpec &v1, SbAtomSpec &v2, SbAtomSpec &v3) const;

    // Accesses indexed component of vector
    SbAtomSpec       &operator [](int i)       { return vec[i]; }
    const SbAtomSpec &operator [](int i) const { return vec[i]; }

    SbVec3AtomSpec & operator =(const SbVec3AtomSpec &atomSpec);

    // Equality comparison operator
    friend int operator ==(const SbVec3AtomSpec &v1, const SbVec3AtomSpec &v2);
    friend int operator !=(const SbVec3AtomSpec &v1, const SbVec3AtomSpec &v2)
        { return !(v1 == v2); }

  protected:
    SbAtomSpec vec[3];
};

class CHEMKIT_DLL_API SbVec4AtomSpec {

  public:

    // Default constructor
    SbVec4AtomSpec();

    // Copy constructor
    SbVec4AtomSpec(const SbVec4AtomSpec &atomSpec);

    // Constructor given an array of 4 components
    SbVec4AtomSpec(const SbAtomSpec v[4])
        { setValue(v); }

    // Constructor given 4 individual components
    SbVec4AtomSpec(const SbAtomSpec &v1, const SbAtomSpec &v2,
                   const SbAtomSpec &v3, const SbAtomSpec &v4)
        { setValue(v1, v2, v3, v4); }

    // Sets the value of vector from an SbVec4AtomSpec
    void setValue(const SbVec4AtomSpec &atomSpec);

    // Sets value of vector from array of 4 components
    SbVec4AtomSpec &setValue(const SbAtomSpec v[4]);

    // Sets value of vector from 4 individual components
    SbVec4AtomSpec &setValue(const SbAtomSpec &v1, const SbAtomSpec &v2,
                             const SbAtomSpec &v3, const SbAtomSpec &v4);

    // Returns pointer to array of 4 components
    const SbAtomSpec *getValue() const
        { return vec; }

    // Returns 4 individual components
    void getValue(SbAtomSpec &v1, SbAtomSpec &v2,
                  SbAtomSpec &v3, SbAtomSpec &v4) const;

    // Accesses indexed component of vector
    SbAtomSpec       &operator [](int i)       { return vec[i]; }
    const SbAtomSpec &operator [](int i) const { return vec[i]; }

    SbVec4AtomSpec & operator =(const SbVec4AtomSpec &atomSpec);

    // Equality comparison operator
    friend int operator ==(const SbVec4AtomSpec &v1, const SbVec4AtomSpec &v2);
    friend int operator !=(const SbVec4AtomSpec &v1, const SbVec4AtomSpec &v2)
        { return !(v1 == v2); }

  protected:
    SbAtomSpec vec[4];
};

#endif /* !__SB_ATOMSPEC_H__ */
