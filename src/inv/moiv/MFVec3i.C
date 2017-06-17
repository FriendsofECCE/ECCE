/*
 * MFVec3i.c++
 *
 *     Classes:  MFVec3i
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

#include <stdlib.h>

#include "inv/ChemKit/MFVec3i.H"

//////////////////////////////////////////////////////////////////////////////
//
// MFVec3i class
//
//////////////////////////////////////////////////////////////////////////////

// Use standard definitions of all basic methods
SO_MFIELD_SOURCE_MALLOC(MFVec3i, SbVec3i, const SbVec3i &);

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Sets values from array of arrays of 3 floats. This can be useful
//    in some applications that have vectors stored in this manner and
//    want to keep them that way for efficiency.
//
// Use: public

void
MFVec3i::setValues(int start,                 // Starting index
                   int num,                   // Number of values to set
                   const int32_t ijk[][3])    // Array of vector values
//
////////////////////////////////////////////////////////////////////////
{
    int newNum = start + num;
    int i;

    if (newNum > getNum())
        makeRoom(newNum);

    for (i = 0; i < num; i++)
        values[start + i].setValue(ijk[i]);

    valueChanged();
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Sets one vector value from 3 separate int32s. (Convenience function)
//
// Use: public

void
MFVec3i::set1Value(int index, int32_t i, int32_t j, int32_t k)
//
////////////////////////////////////////////////////////////////////////
{
    set1Value(index, SbVec3i(i, j, k));
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Sets one vector value from an array of 3 int32s. (Convenience function)
//
// Use: public

void
MFVec3i::set1Value(int index, const int32_t ijk[3])
//
////////////////////////////////////////////////////////////////////////
{
    set1Value(index, SbVec3i(ijk));
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Sets to one vector value from 3 separate int32s. (Convenience function)
//
// Use: public

void
MFVec3i::setValue(int32_t i, int32_t j, int32_t k)
//
////////////////////////////////////////////////////////////////////////
{
    setValue(SbVec3i(i, j, k));
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Sets to one vector value from array of 3 int32s. (Convenience function)
//
// Use: public

void
MFVec3i::setValue(const int32_t ijk[3])
//
////////////////////////////////////////////////////////////////////////
{
    setValue(SbVec3i(ijk));
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Reads one (indexed) value from file. Returns FALSE on error.
//
// Use: private

SbBool
MFVec3i::read1Value(SoInput *in, int index)
//
////////////////////////////////////////////////////////////////////////
{
    return (in->read(values[index][0]) &&
            in->read(values[index][1]) &&
            in->read(values[index][2]));
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Writes one (indexed) value to file.
//
// Use: private

void
MFVec3i::write1Value(SoOutput *out, int index) const
//
////////////////////////////////////////////////////////////////////////
{
    out->write(values[index][0]);

    if (! out->isBinary())
        out->write(' ');

    out->write(values[index][1]);

    if (! out->isBinary())
        out->write(' ');

    out->write(values[index][2]);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Writes array of binary values to file as one chunk.
//
// Use: private

void
MFVec3i::writeBinaryValues(SoOutput *out) const // Defines writing action

//
////////////////////////////////////////////////////////////////////////
{
#ifdef IV2_0
    out->writeBinaryArray((long *)values, 3*num);
#else
    out->writeBinaryArray((int32_t *)values, 3*num);
#endif
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Reads array of binary values from file as one chunk.
//
// Use: private

SbBool
MFVec3i::readBinaryValues(SoInput *in,    // Reading specification
                          int numToRead)  // Number of values to read
//
////////////////////////////////////////////////////////////////////////
{
#ifdef IV2_0
    return (in->readBinaryArray((long *)values, 3*numToRead));
#else
    return (in->readBinaryArray((int32_t *)values, 3*numToRead));
#endif
}



