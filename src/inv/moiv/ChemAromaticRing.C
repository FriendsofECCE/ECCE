/*
 * ChemAromaticRing.c++
 *
 *     Classes:  ChemAromaticRing
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

#include <math.h>

#include "inv/ChemKit/ChemIntList.H"
#include "inv/ChemKit/ChemAromaticRing.H"

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAXFLOAT    ((float)3.40282346638528860e+38)

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Constructor
//
// Use: private
       
ChemAromaticRing::ChemAromaticRing(const short numVerts)
//
////////////////////////////////////////////////////////////////////////
{
    numberOfVertices = numVerts;
    bonds = new int32_t[numVerts];
    vertices = new SbVec3f[numVerts];
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Destructor
//
// Use: private

ChemAromaticRing::~ChemAromaticRing()
//
////////////////////////////////////////////////////////////////////////
{
    delete bonds;
    delete vertices;
}

#define X 0
#define Y 1
#define Z 2

////////////////////////////////////////////////////////////////////////
//
// Description:
//    For the atoms in an aromatic ring, find the "best" plane.
//    From Graphics Gems III p. 231 "Newell's Method for Computing the
//    Plane Equation of a Polygon".  Actually, this will work for rings
//    of any size as the loop is over the numberOfVertices in the ring.
//
// Use: public

void
ChemAromaticRing::generateMatrix(float aromaticRingScale)
//
////////////////////////////////////////////////////////////////////////
{
    short i;
    SbVec3f refPt;

    // Compute the normal of the ring and a reference point on the plane.
    // The reference point is refPt / theRing.numberOfVertices.

    refPt[0] = refPt[1] = refPt[2] = 0.0;
    normal[0] = normal[1] = normal[2] = 0.0;

    for (i = 0; i < numberOfVertices; i++) {
        const SbVec3f &u = vertices[i];
        const SbVec3f &v = vertices[(i+1) % numberOfVertices];
        normal[X] += (u[Y] - v[Y]) * (u[Z] + v[Z]);
        normal[Y] += (u[Z] - v[Z]) * (u[X] + v[X]);
        normal[Z] += (u[X] - v[X]) * (u[Y] + v[Y]);
        refPt += u;
    }
       
    normal.normalize();
    refPt /= numberOfVertices;

    // Find the closest point on each bond to refPt
    float theDist = MAXFLOAT;
    SbLine tmpLine;
    SbVec3f tmpPt;
    for (i = 0; i < numberOfVertices; i++) {
        tmpLine.setValue(vertices[i], vertices[(i+1) % numberOfVertices]);
        tmpPt = tmpLine.getClosestPoint(refPt);
        float xx = refPt[0] - tmpPt[0];
        float yy = refPt[1] - tmpPt[1];
        float zz = refPt[2] - tmpPt[2];
        theDist = MIN(theDist, xx*xx + yy*yy + zz*zz);
    }
    theDist = sqrtf(theDist) * aromaticRingScale;

    // Generate the components of the matrix
    SbRotation theRotation;
    SbVec3f    theScale(theDist, theDist, theDist);
    SbVec3f    zAxis(0.0, 0.0, 1.0);

    theRotation.setValue(zAxis, normal);
    transform.setTransform(refPt, theRotation, theScale);
}

#undef X
#undef Y
#undef Z
#undef MAXFLOAT
#undef MIN
