/*
 * ChemPath.c++
 *
 *     Classes:  ChemPath
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

#ident "$Revision: 22147 $"

#include "inv/actions/SoSearchAction.H"

#include "inv/ChemKit/ChemIntList.H"
#include "inv/ChemKit/ChemPath.H"
#include "inv/ChemKit/ChemBaseData.H"
// --> lod
#include "inv/ChemKit/ChemLOD.H"
// <-- lod
#include "inv/ChemKit/ChemDisplay.H"

SoType ChemPath::classTypeId;

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Gets the path's typeId
//
// Use: public
    
SoType
ChemPath::getTypeId() const
//
////////////////////////////////////////////////////////////////////////
{
    return classTypeId;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Reads stuff into empty instance of SoPath. Returns FALSE on
//    error.
//
// Use: private

#ifdef IV2_0
SbBool
ChemPath::readInstance(SoInput *in)
#else
SbBool
ChemPath::readInstance(SoInput *in, unsigned short /* flags not used */)
#endif
//  
////////////////////////////////////////////////////////////////////////
{
    return TRUE;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Default constructor
//
// Use: protected

ChemPath::ChemPath()
//
////////////////////////////////////////////////////////////////////////
{
    path = NULL;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Destructor
//
// Use: protected

ChemPath::~ChemPath()
//
////////////////////////////////////////////////////////////////////////
{
    path->unref();
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Method to set the SoPath portion of ChemPath.
//
// Use: protected
//
////////////////////////////////////////////////////////////////////////
SbBool
ChemPath::setSoPath(SoPath *_path)
{
    if (_path == NULL) return FALSE;
    _path->ref();
    path = _path;
    return TRUE;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Methods to return const pointers to the private data.
//
// Use: public
//
////////////////////////////////////////////////////////////////////////

const SoPath *
ChemPath::getSoPath() const
{
    return path;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Comparison operator tests path equality. Two paths are equal if
//    their chains are the same length and all node pointers in them
//    are the same.
//
// Use: public

int
ChemPath::operator ==(const ChemPath &chemPath) const
//
////////////////////////////////////////////////////////////////////////
{
    return equals(chemPath);
}

SbBool
ChemPath::equals(const ChemPath &chemPath) const
{
    SoPath *path2 = chemPath.path;

    // Check to see if any of the 2 paths are NULL.  I guess if both
    // of the paths are NULL, then the ChemDisplayPaths are equal.
    if ((path == NULL) && (path2 == NULL)) return TRUE;
    if ((path == NULL) && (path2 != NULL)) return FALSE;
    if ((path != NULL) && (path2 == NULL)) return FALSE;
  
    return (*path == *path2);
}
