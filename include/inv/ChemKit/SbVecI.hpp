/*
 * SbVecI.h
 *
 *	This file defines the SbVec2i, SbVec3i and SbVec4i classes.
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

#ifndef __SB_VECI_H__
#define __SB_VECI_H__

#include <math.h>
#include "inv/SbBasic.H"

#include "inv/ChemKit/ChemkitBasic.H"

// -----------------------------------
//
// Types/classes defined in this file:
// 
// -----------------------------------

class SbVec2i;
class SbVec3i;
class SbVec4i;


//////////////////////////////////////////////////////////////////////////////
//
//  Class: SbVec2i
//
//  2D vector used to represet points or directions. Each component of
//  the vector is a (int32) integer.
//
//////////////////////////////////////////////////////////////////////////////

class CHEMKIT_DLL_API SbVec2i {
  public:

    // Default constructor
    SbVec2i()                                           { }

    // Constructor given an array of 2 components
    SbVec2i(const int32_t v[2])                         { setValue(v); }

    // Constructor given 2 individual components
    SbVec2i(int32_t x, int32_t y)                       { setValue(x, y); }

    // Returns dot (inner) product of vector and another vector
    int32_t     dot(const SbVec2i &v) const;

    // Returns pointer to array of 2 components
    const int32_t *getValue() const                     { return vec; }

    // Returns 2 individual components
    void        getValue(int32_t &x, int32_t &y) const;

    // Negates each component of vector in place
    void        negate();

    // Sets value of vector from array of 2 components
    SbVec2i &   setValue(const int32_t v[2]);

    // Sets value of vector from 2 individual components
    SbVec2i &   setValue(int32_t x, int32_t y);

    // Accesses indexed component of vector
    int32_t &       operator [](int i)        { return (vec[i]); }
    const int32_t & operator [](int i) const  { return (vec[i]); }

    // Component-wise scalar multiplication and division operators
    SbVec2i &   operator *=(int d);
    SbVec2i &   operator *=(double d);

    SbVec2i &   operator /=(int d);
    SbVec2i &   operator /=(double d)
        { return *this *= (1.0 / d); }

    // Component-wise vector addition and subtraction operators
    SbVec2i &   operator +=(const SbVec2i &u);
    SbVec2i &   operator -=(const SbVec2i &u);

    // Nondestructive unary negation - returns a new vector
    SbVec2i     operator -() const;

    // Component-wise binary scalar multiplication and division operators
    friend SbVec2i      operator *(const SbVec2i &v, int d);
    friend SbVec2i      operator *(const SbVec2i &v, double d);
    friend SbVec2i      operator *(int d, const SbVec2i &v)
        { return v * d; }
    friend SbVec2i      operator *(double d, const SbVec2i &v)
        { return v * d; }
    friend SbVec2i      operator /(const SbVec2i &v, int d);
    friend SbVec2i      operator /(const SbVec2i &v, double d)
        { return v * (1.0 / d); }

    // Component-wise binary vector addition and subtraction operators
    friend SbVec2i      operator +(const SbVec2i &v1, const SbVec2i &v2);
    friend SbVec2i      operator -(const SbVec2i &v1, const SbVec2i &v2);

    // Equality comparison operator
    friend int          operator ==(const SbVec2i &v1, const SbVec2i &v2);
    friend int          operator !=(const SbVec2i &v1, const SbVec2i &v2)
        { return !(v1 == v2); }

  protected:
    int32_t     vec[2];         // Storage for vector components
};

//////////////////////////////////////////////////////////////////////////////
//
//  Class: SbVec3i
//
//  3D vector used to represent points or directions. Each component of
//  the vector is a (int32_t) integer.
//
//  WARNING!!!!!  Transcription of arrays of this class assume that the
//                only data stored in this class are three consecutive values.
//                Do not add any extra data members!!!
//
//////////////////////////////////////////////////////////////////////////////

class CHEMKIT_DLL_API SbVec3i {
  public:
    // Default constructor
    SbVec3i()                                           { }

    // Constructor given an array of 3 components
    SbVec3i(const int32_t v[3])
         { vec[0] = v[0]; vec[1] = v[1]; vec[2] = v[2]; }

    // Constructor given 3 individual components
    SbVec3i(int32_t x, int32_t y, int32_t z)
         { vec[0] = x; vec[1] = y; vec[2] = z; }

    // Returns dot (inner) product of vector and another vector
    int32_t     dot(const SbVec3i &v) const;

    // Returns pointer to array of 3 components
    const int32_t *getValue() const                     { return vec; }

    // Returns 3 individual components
    void        getValue(int32_t &x, int32_t &y, int32_t &z) const;

    // Negates each component of vector in place
    void        negate();

    // Sets value of vector from array of 3 components
    SbVec3i &   setValue(const int32_t v[3])
         { vec[0] = v[0]; vec[1] = v[1]; vec[2] = v[2]; return *this; }

    // Sets value of vector from 3 individual components
    SbVec3i &   setValue(int32_t x, int32_t y, int32_t z)
         { vec[0] = x; vec[1] = y; vec[2] = z; return *this; }

    // Accesses indexed component of vector
    int32_t &       operator [](int i)          { return (vec[i]); }
    const int32_t & operator [](int i) const    { return (vec[i]); }

    // Component-wise scalar multiplication and division operators
    SbVec3i &   operator *=(int d);
    SbVec3i &   operator *=(double d);

    SbVec3i &   operator /=(int d);
    SbVec3i &   operator /=(double d)
        { return *this *= (1.0 / d); }

    // Component-wise vector addition and subtraction operators
    SbVec3i &   operator +=(SbVec3i v);
    SbVec3i &   operator -=(SbVec3i v);

    // Nondestructive unary negation - returns a new vector
    SbVec3i     operator -() const;

    // Component-wise binary scalar multiplication and division operators
    friend SbVec3i      operator *(const SbVec3i &v, int d);
    friend SbVec3i      operator *(const SbVec3i &v, double d);
    friend SbVec3i      operator *(int d, const SbVec3i &v)
        { return v * d; }
    friend SbVec3i      operator *(double d, const SbVec3i &v)
        { return v * d; }
    friend SbVec3i      operator /(const SbVec3i &v, int d);
    friend SbVec3i      operator /(const SbVec3i &v, double d)
        { return v * (1.0 / d); }

    // Component-wise binary vector addition and subtraction operators
    friend SbVec3i      operator +(const SbVec3i &v1, const SbVec3i &v2);
    friend SbVec3i      operator -(const SbVec3i &v1, const SbVec3i &v2);

    // Equality comparison operator
    friend int          operator ==(const SbVec3i &v1, const SbVec3i &v2);
    friend int          operator !=(const SbVec3i &v1, const SbVec3i &v2)
        { return !(v1 == v2); }

  protected:
    int32_t     vec[3];         // Storage for vector components
};

//////////////////////////////////////////////////////////////////////////////
//
//  Class: SbVec4i
//
//  4D vector used to represent points or directions. Each component of
//  the vector is a (int32_t) integer.
//
//  WARNING!!!!!  Transcription of arrays of this class assume that the
//                only data stored in this class are four consecutive values.
//                Do not add any extra data members!!!
//
//////////////////////////////////////////////////////////////////////////////

class CHEMKIT_DLL_API SbVec4i {
  public:
    // Default constructor
    SbVec4i()                                           { }

    // Constructor given an array of 3 components
    SbVec4i(const int32_t v[4])
         { vec[0] = v[0]; vec[1] = v[1]; vec[2] = v[2]; vec[3] = v[3]; }

    // Constructor given 4 individual components
    SbVec4i(int32_t i, int32_t j, int32_t k, int32_t l)
         { vec[0] = i; vec[1] = j; vec[2] = k; vec[3] = l; }

    // Returns dot (inner) product of vector and another vector
    int32_t     dot(const SbVec4i &v) const;

    // Returns pointer to array of 4 components
    const int32_t *getValue() const { return vec; }

    // Returns 4 individual components
    void        getValue(int32_t &i, int32_t &j, int32_t &k, int32_t &l) const;

    // Negates each component of vector in place
    void        negate();

    // Sets value of vector from array of 4 components
    SbVec4i &   setValue(const int32_t v[4])
         { vec[0] = v[0]; vec[1] = v[1]; vec[2] = v[2]; vec[3] = v[3];
           return *this; }

    // Sets value of vector from 4 individual components
    SbVec4i &   setValue(int32_t i, int32_t j, int32_t k, int32_t l)
         { vec[0] = i; vec[1] = j; vec[2] = k; vec[3] = l; return *this; }

    // Accesses indexed component of vector
    int32_t &       operator [](int i)          { return (vec[i]); }
    const int32_t & operator [](int i) const    { return (vec[i]); }

    // Component-wise scalar multiplication and division operators
    SbVec4i &   operator *=(int d);
    SbVec4i &   operator *=(double d);

    SbVec4i &   operator /=(int d);
    SbVec4i &   operator /=(double d)
        { return *this *= (1.0 / d); }

    // Component-wise vector addition and subtraction operators
    SbVec4i &   operator +=(SbVec4i v);
    SbVec4i &   operator -=(SbVec4i v);

    // Nondestructive unary negation - returns a new vector
    SbVec4i     operator -() const;

    // Component-wise binary scalar multiplication and division operators
    friend SbVec4i      operator *(const SbVec4i &v, int d);
    friend SbVec4i      operator *(const SbVec4i &v, double d);
    friend SbVec4i      operator *(int d, const SbVec4i &v)
        { return v * d; }
    friend SbVec4i      operator *(double d, const SbVec4i &v)
        { return v * d; }
    friend SbVec4i      operator /(const SbVec4i &v, int d);
    friend SbVec4i      operator /(const SbVec4i &v, double d)
        { return v * (1.0 / d); }

    // Component-wise binary vector addition and subtraction operators
    friend SbVec4i      operator +(const SbVec4i &v1, const SbVec4i &v2);
    friend SbVec4i      operator -(const SbVec4i &v1, const SbVec4i &v2);

    // Equality comparison operator
    friend int          operator ==(const SbVec4i &v1, const SbVec4i &v2);
    friend int          operator !=(const SbVec4i &v1, const SbVec4i &v2)
        { return !(v1 == v2); }

  protected:
    int32_t     vec[4];         // Storage for vector components
};

#endif /* !__SB_VECI_H__ */
