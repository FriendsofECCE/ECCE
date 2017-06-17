/*
 * MFVec2i.c++
 *
 *     Classes:  MFVec2i
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
#include <limits.h>

#include "inv/ChemKit/MFVec2i.H"
// --> util
#include "inv/ChemKit/ChemIntList.H"
// <-- util

//////////////////////////////////////////////////////////////////////////////
//
// MFVec2i class
//
//////////////////////////////////////////////////////////////////////////////

// Use standard definitions of all basic methods
SO_MFIELD_SOURCE_MALLOC(MFVec2i, SbVec2i, const SbVec2i &);

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Sets values from array of arrays of 2 floats. This can be useful
//    in some applications that have vectors stored in this manner and
//    want to keep them that way for efficiency.
//
// Use: public

void
MFVec2i::setValues(int start,                 // Starting index
                   int num,                   // Number of values to set
                   const int32_t ij[][2])     // Array of vector values
//
////////////////////////////////////////////////////////////////////////
{
    int newNum = start + num;
    int i;

    if (newNum > getNum())
        makeRoom(newNum);

    for (i = 0; i < num; i++)
        values[start + i].setValue(ij[i]);

    valueChanged();
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Sets one vector value from 2 separate int32s. (Convenience function)
//
// Use: public

void
MFVec2i::set1Value(int index, int32_t i, int32_t j)
//
////////////////////////////////////////////////////////////////////////
{
    set1Value(index, SbVec2i(i, j));
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Sets one vector value from an array of 2 int32s. (Convenience function)
//
// Use: public

void
MFVec2i::set1Value(int index, const int32_t ij[2])
//
////////////////////////////////////////////////////////////////////////
{
    set1Value(index, SbVec2i(ij));
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Sets to one vector value from 2 separate int32s. (Convenience function)
//
// Use: public

void
MFVec2i::setValue(int32_t i, int32_t j)
//
////////////////////////////////////////////////////////////////////////
{
    setValue(SbVec2i(i, j));
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Sets to one vector value from array of 2 int32s. (Convenience function)
//
// Use: public

void
MFVec2i::setValue(const int32_t ij[2])
//
////////////////////////////////////////////////////////////////////////
{
    setValue(SbVec2i(ij));
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Reads one (indexed) value from file. Returns FALSE on error.
//
// Use: private

SbBool
MFVec2i::read1Value(SoInput *in, int index)
//
////////////////////////////////////////////////////////////////////////
{
    return (in->read(values[index][0]) &&
            in->read(values[index][1]));
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Writes one (indexed) value to file.
//
// Use: private

void
MFVec2i::write1Value(SoOutput *out, int index) const
//
////////////////////////////////////////////////////////////////////////
{
    out->write(values[index][0]);

    if (! out->isBinary())
        out->write(' ');

    out->write(values[index][1]);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Writes array of binary values to file as one chunk.
//
// Use: private

void
MFVec2i::writeBinaryValues(SoOutput *out) const // Defines writing action

//
////////////////////////////////////////////////////////////////////////
{
#ifdef IV2_0
    out->writeBinaryArray((long *)values, 2*num);
#else
    out->writeBinaryArray((int32_t *)values, 2*num);
#endif
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Reads array of binary values from file as one chunk.
//
// Use: private

SbBool
MFVec2i::readBinaryValues(SoInput *in,    // Reading specification
                          int numToRead)  // Number of values to read
//
////////////////////////////////////////////////////////////////////////
{
#ifdef IV2_0
    return (in->readBinaryArray((long *)values, 2*numToRead));
#else
    return (in->readBinaryArray((int32_t *)values, 2*numToRead));
#endif
}

// --> util 
int MFVec2i::find(const int index, const int globalNumber) const
{
	int i,start,end;
	for (i=0;i<getNum();i++)
	{
		values[i].getValue(start,end);
		if (end==-1)
		{
			end=start+globalNumber;
		}
		else end = start+end;

		if (index >=start && index < end)
			return i;
	}
	return -1;
}

int MFVec2i::count(int globalNumber) const
{
	int i,start,end, cnt=0;
	for (i=0;i<getNum();i++)
	{
		values[i].getValue(start,end);
		if (end==-1)
		{
			end=globalNumber-start;
		}
		cnt+=end;
	}
	return cnt;
}

void MFVec2i::sortIndices(MFVec2i &listIndex, MFVec2i &newIndex,
	int32_t end_indicator, int32_t numberOfItems)
//
////////////////////////////////////////////////////////////////////////
{
	//newIndex.deleteValues(0);
	newIndex.setNum(0);
	if (listIndex.getNum() == 0) return;

	ChemIntList theList(numberOfItems);

	int32_t loop, numLoops, start, end, item;
	int32_t lastStart, lastIncr, count;

	// Make a list of all the entries in listIndex.
	numLoops = listIndex.getNum();
	for (loop = 0; loop < numLoops; loop++) {
		listIndex[loop].getValue(start, end);
		if (end == end_indicator) {
			end = numberOfItems;
		}
		else {
			end += start;
		}
		for (item = start; item < end; item++) {
			theList.append(item);
		}
	}

	// Sort theList and make it unique.
	theList.sort(TRUE);

	if (theList.getLength() == 0) return;

	// Make newIndex using contents of theList.
	lastStart = -1;
	lastIncr = 0;
	count = 0;
	end = theList.getLength();
	for (loop = 0; loop < end; loop++) {
		item = theList[loop];
		if (lastStart == -1) {
			lastStart = item;
			lastIncr = 1;
		}
		else if (item == lastStart + lastIncr) {
			lastIncr++;
		}
		else {
			newIndex.set1Value(count++, SbVec2i(lastStart, lastIncr));
			lastStart = item;
			lastIncr = 1;
		}
	}
	if (lastStart != -1) {
		newIndex.set1Value(count, SbVec2i(lastStart, lastIncr));
	}
}

void MFVec2i::setNum(const int num)
{
  // Don't use getNum(), as that could trigger evaluate(), which is
  // _not_ supposed to be called recursively (which means setNum()
  // wouldn't have been available from within an evaluate() session).
  int oldnum = this->num;

  // Note: this method is implemented in terms of the virtual methods
  // deleteValues() and insertSpace() so the more "complex" fields
  // (like SoMFNode and SoMFEngine) can also handle setNum() in a
  // correct way.

  if (num < oldnum) {
		this->num=num;
    //this->deleteValues(num, -1);
    // deleteValues() also handles notification.
  }
  else if (num > oldnum) {
    this->insertSpace(oldnum, num - oldnum);
    // insertSpace() also handles notification.
  }
  // else no change.
}

// <-- util
