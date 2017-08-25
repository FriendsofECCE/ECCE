/*
 * SbVecI.c++
 *
 *     Classes:  SbVec2i, SbVec3i, SbVec4i
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

#include "inv/ChemKit/SbVecI.H"


//////////////////////////////////////////////////////////////////////////////
//
// Vec2i class
//
//////////////////////////////////////////////////////////////////////////////

//
// Returns dot (inner) product of vector and another vector
//

int32_t
SbVec2i::dot(const SbVec2i &v) const
{
    return vec[0] * v.vec[0] + vec[1] * v.vec[1];
}

//
// Returns 2 individual components
//

void
SbVec2i::getValue(int32_t &x, int32_t &y) const
{
    x = vec[0];
    y = vec[1];
}

//
// Negates each component of vector in place
//

void
SbVec2i::negate()
{
    vec[0] = -vec[0];
    vec[1] = -vec[1];
}

//
// Sets value of vector from array of 2 components
//

SbVec2i &
SbVec2i::setValue(const int32_t v[2])     
{
    vec[0] = v[0];
    vec[1] = v[1];

    return (*this);
}

//
// Sets value of vector from 2 individual components
//

SbVec2i &
SbVec2i::setValue(int32_t x, int32_t y)     
{
    vec[0] = x;
    vec[1] = y;

    return (*this);
}

//
// Component-wise scalar multiplication operator
//
SbVec2i &
SbVec2i::operator *=(int d)
{
    vec[0] *= d;
    vec[1] *= d;

    return *this;
}

SbVec2i &
SbVec2i::operator *=(double d)
{
    vec[0] = int32_t(vec[0] * d);
    vec[1] = int32_t(vec[1] * d);

    return *this;
}

//
// Component-wise scalar division operator
//

SbVec2i &
SbVec2i::operator /=(int d)
{
    vec[0] /= d;
    vec[1] /= d;

    return *this;
}

//
// Component-wise vector addition operator
//

SbVec2i &
SbVec2i::operator +=(const SbVec2i &u)
{
    vec[0] += u.vec[0];
    vec[1] += u.vec[1];

    return *this;
}

//
// Component-wise vector subtraction operator
//

SbVec2i &
SbVec2i::operator -=(const SbVec2i &u)
{
    vec[0] -= u.vec[0];
    vec[1] -= u.vec[1];

    return *this;
}

//
// Nondestructive unary negation - returns a new vector
//

SbVec2i
SbVec2i::operator -() const
{
    return SbVec2i(-vec[0], -vec[1]);
}


//
// Component-wise binary scalar multiplication operator
//

SbVec2i
operator *(const SbVec2i &v, int d)
{
    return SbVec2i(v.vec[0] * d, v.vec[1] * d);
}

SbVec2i
operator *(const SbVec2i &v, double d)
{
    return SbVec2i(int32_t(v.vec[0] * d), int32_t(v.vec[1] * d));
}

//
// Component-wise binary scalar division operator
//

SbVec2i
operator /(const SbVec2i &v, int d)
{
    return SbVec2i(v.vec[0] / d, v.vec[1] / d);
}

//
// Component-wise binary vector addition operator
//

SbVec2i
operator +(const SbVec2i &v1, const SbVec2i &v2)
{
    return SbVec2i(v1.vec[0] + v2.vec[0],
                  v1.vec[1] + v2.vec[1]);
}

//
// Component-wise binary vector subtraction operator
//

SbVec2i
operator -(const SbVec2i &v1, const SbVec2i &v2)
{
    return SbVec2i(v1.vec[0] - v2.vec[0],
                  v1.vec[1] - v2.vec[1]);
}

//
// Equality comparison operator
//

int
operator ==(const SbVec2i &v1, const SbVec2i &v2)
{
    return (v1.vec[0] == v2.vec[0] &&
            v1.vec[1] == v2.vec[1]);
}

//////////////////////////////////////////////////////////////////////////////
//
// Vec3i class
//
//////////////////////////////////////////////////////////////////////////////

//
// Returns dot (inner) product of vector and another vector
//

int32_t
SbVec3i::dot(const SbVec3i &v) const
{
    return (vec[0] * v.vec[0] +
            vec[1] * v.vec[1] +
            vec[2] * v.vec[2]);
}

//
// Returns 3 individual components
//

void
SbVec3i::getValue(int32_t &i, int32_t &j, int32_t &k) const
{
    i = vec[0];
    j = vec[1];
    k = vec[2];
}

//
// Negates each component of vector in place
//

void
SbVec3i::negate()
{
    vec[0] = -vec[0];
    vec[1] = -vec[1];
    vec[2] = -vec[2];
}

//
// Component-wise scalar multiplication operator
//
SbVec3i &
SbVec3i::operator *=(int d)
{
    vec[0] *= d;
    vec[1] *= d;
    vec[2] *= d;

    return *this;
}

SbVec3i &
SbVec3i::operator *=(double d)
{
    vec[0] = int32_t(vec[0] * d);
    vec[1] = int32_t(vec[1] * d);
    vec[2] = int32_t(vec[2] * d);

    return *this;
}

//
// Component-wise vector addition operator
//

SbVec3i &
SbVec3i::operator +=(SbVec3i v)
{
    vec[0] += v.vec[0];
    vec[1] += v.vec[1];
    vec[2] += v.vec[2];

    return *this;
}

//
// Component-wise vector subtraction operator
//

SbVec3i &
SbVec3i::operator -=(SbVec3i v)
{
    vec[0] -= v.vec[0];
    vec[1] -= v.vec[1];
    vec[2] -= v.vec[2];

    return *this;
}

//
// Nondestructive unary negation - returns a new vector
//

SbVec3i
SbVec3i::operator -() const
{
    return SbVec3i(-vec[0], -vec[1], -vec[2]);
}

//
// Component-wise binary scalar multiplication operator
//

SbVec3i
operator *(const SbVec3i &v, int d)
{
    return SbVec3i(v.vec[0] * d, v.vec[1] * d, v.vec[2] * d);
}

SbVec3i
operator *(const SbVec3i &v, double d)
{
    return SbVec3i(int32_t(v.vec[0] * d), int32_t(v.vec[1] * d), 
        int32_t(v.vec[2] * d));
}

//
// Component-wise binary vector addition operator
//

SbVec3i
operator +(const SbVec3i &v1, const SbVec3i &v2)
{
    return SbVec3i(v1.vec[0] + v2.vec[0],
                   v1.vec[1] + v2.vec[1],
                   v1.vec[2] + v2.vec[2]);
}

//
// Component-wise binary vector subtraction operator
//

SbVec3i
operator -(const SbVec3i &v1, const SbVec3i &v2)
{
    return SbVec3i(v1.vec[0] - v2.vec[0],
                   v1.vec[1] - v2.vec[1],
                   v1.vec[2] - v2.vec[2]);
}

//
// Equality comparison operator. Componenents must match exactly.
//

int
operator ==(const SbVec3i &v1, const SbVec3i &v2)
{
    return (v1.vec[0] == v2.vec[0] &&
            v1.vec[1] == v2.vec[1] &&
            v1.vec[2] == v2.vec[2]);
}


//////////////////////////////////////////////////////////////////////////////
//
// Vec4i class
//
//////////////////////////////////////////////////////////////////////////////

//
// Returns dot (inner) product of vector and another vector
//

int32_t
SbVec4i::dot(const SbVec4i &v) const
{
    return (vec[0] * v.vec[0] +
            vec[1] * v.vec[1] +
            vec[2] * v.vec[2] +
            vec[3] * v.vec[3]);
}

//
// Returns 3 individual components
//

void
SbVec4i::getValue(int32_t &i, int32_t &j, int32_t &k, int32_t &l) const
{
    i = vec[0];
    j = vec[1];
    k = vec[2];
    l = vec[3];
}

//
// Negates each component of vector in place
//

void
SbVec4i::negate()
{
    vec[0] = -vec[0];
    vec[1] = -vec[1];
    vec[2] = -vec[2];
    vec[3] = -vec[3];
}

//
// Component-wise scalar multiplication operator
//
SbVec4i &
SbVec4i::operator *=(int d)
{
    vec[0] *= d;
    vec[1] *= d;
    vec[2] *= d;
    vec[3] *= d;

    return *this;
}

SbVec4i &
SbVec4i::operator *=(double d)
{
    vec[0] = int32_t(vec[0] * d);
    vec[1] = int32_t(vec[1] * d);
    vec[2] = int32_t(vec[2] * d);
    vec[3] = int32_t(vec[3] * d);

    return *this;
}

//
// Component-wise vector addition operator
//

SbVec4i &
SbVec4i::operator +=(SbVec4i v)
{
    vec[0] += v.vec[0];
    vec[1] += v.vec[1];
    vec[2] += v.vec[2];
    vec[3] += v.vec[3];

    return *this;
}

//
// Component-wise vector subtraction operator
//

SbVec4i &
SbVec4i::operator -=(SbVec4i v)
{
    vec[0] -= v.vec[0];
    vec[1] -= v.vec[1];
    vec[2] -= v.vec[2];
    vec[3] -= v.vec[3];

    return *this;
}

//
// Nondestructive unary negation - returns a new vector
//

SbVec4i
SbVec4i::operator -() const
{
    return SbVec4i(-vec[0], -vec[1], -vec[2], -vec[3]);
}

//
// Component-wise binary scalar multiplication operator
//

SbVec4i
operator *(const SbVec4i &v, int d)
{
    return SbVec4i(v.vec[0] * d, v.vec[1] * d, v.vec[2] * d, v.vec[3] * d);
}

SbVec4i
operator *(const SbVec4i &v, double d)
{
    return SbVec4i(int32_t(v.vec[0] * d), int32_t(v.vec[1] * d), 
        int32_t(v.vec[2] * d), int32_t(v.vec[3] * d));
}

//
// Component-wise binary vector addition operator
//

SbVec4i
operator +(const SbVec4i &v1, const SbVec4i &v2)
{
    return SbVec4i(v1.vec[0] + v2.vec[0],
                   v1.vec[1] + v2.vec[1],
                   v1.vec[2] + v2.vec[2],
                   v1.vec[3] + v2.vec[3]);
}

//
// Component-wise binary vector subtraction operator
//

SbVec4i
operator -(const SbVec4i &v1, const SbVec4i &v2)
{
    return SbVec4i(v1.vec[0] - v2.vec[0],
                   v1.vec[1] - v2.vec[1],
                   v1.vec[2] - v2.vec[2],
                   v1.vec[3] - v2.vec[3]);
}

//
// Equality comparison operator. Componenents must match exactly.
//

int
operator ==(const SbVec4i &v1, const SbVec4i &v2)
{
    return (v1.vec[0] == v2.vec[0] &&
            v1.vec[1] == v2.vec[1] &&
            v1.vec[2] == v2.vec[2] &&
            v1.vec[3] == v2.vec[3]);
}

