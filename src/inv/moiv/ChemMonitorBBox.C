/*
 * ChemMonitorBBox.c++
 *
 *     Classes:  ChemMonitorBBox
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

#include "inv/ChemKit/ChemMonitorBBox.H"

//////////////////////////////////////////////////////////////////////////////
// 
// ChemMonitorBBox class - Holds all monitor bbox information.
//     Used in ChemBBoxAction, ChemMonitor and ChemSelection.
//
//////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Constructor
//
// Use: public

ChemMonitorBBox::ChemMonitorBBox(int32_t numDistanceMonitors,
    int32_t numAngleMonitors, int32_t numTorsionalMonitors)
//
////////////////////////////////////////////////////////////////////////
{
    distanceIndex = angleIndex = torsionalIndex = NULL;
    distanceBBox = angleBBox = torsionalBBox = NULL;
    distanceCenter = angleCenter = torsionalCenter = NULL;
    
    numberOfDistanceMonitors = numDistanceMonitors;
    numberOfAngleMonitors = numAngleMonitors;
    numberOfTorsionalMonitors = numTorsionalMonitors;

    if (numberOfDistanceMonitors != 0) {
        distanceIndex = new int32_t[numberOfDistanceMonitors];
        distanceBBox = new SbBox3f[numberOfDistanceMonitors];
        distanceCenter = new SbVec3f[numberOfDistanceMonitors];
    }

    if (numberOfAngleMonitors != 0) {
        angleIndex = new int32_t[numberOfAngleMonitors];
        angleBBox = new SbBox3f[numberOfAngleMonitors];
        angleCenter = new SbVec3f[numberOfAngleMonitors];
    }

    if (numberOfTorsionalMonitors != 0) {
        torsionalIndex = new int32_t[numberOfTorsionalMonitors];
        torsionalBBox = new SbBox3f[numberOfTorsionalMonitors];
        torsionalCenter = new SbVec3f[numberOfTorsionalMonitors];
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:  
//    Destructor
//
// Use: public

ChemMonitorBBox::~ChemMonitorBBox()
//
////////////////////////////////////////////////////////////////////////
{
    if (distanceIndex != NULL) delete [] distanceIndex;
    if (distanceBBox != NULL) delete [] distanceBBox;
    if (distanceCenter != NULL) delete [] distanceCenter;

    if (angleIndex != NULL) delete [] angleIndex;
    if (angleBBox != NULL) delete [] angleBBox;
    if (angleCenter != NULL) delete [] angleCenter;

    if (torsionalIndex != NULL) delete [] torsionalIndex;
    if (torsionalBBox != NULL) delete [] torsionalBBox;
    if (torsionalCenter != NULL) delete [] torsionalCenter;
}
