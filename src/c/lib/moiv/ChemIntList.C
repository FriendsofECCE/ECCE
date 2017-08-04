/*
 * ChemIntList.c++
 *
 * 	   Classes:  ChemIntList
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

#ident "$Revision: 22177 $"


#include <stdlib.h>

#include "inv/errors/SoDebugError.H"

#include "inv/ChemKit/ChemIntList.H"

#define DEFAULT_INITIAL_SIZE    40
#define END_OF_LIST 9999999

static int32_t compareInts(const void *int1, const void *int2);

//
// Constructor
//

ChemIntList::ChemIntList()
{
    theInts  = NULL;
    nInts = theIntsSize = 0;

    setSize(0);
}

//
// Constructor allocates enough room for the given number of integers.
//

ChemIntList::ChemIntList(int32_t initSize)
{
    theInts  = NULL;
    nInts = theIntsSize = 0;

    setSize(initSize);          // Makes enough room for initSize entries
    setSize(0);                 // Sets nInts back to 0
}

//
// Initialize one ChemIntList from another
//

ChemIntList::ChemIntList(const ChemIntList &cl)
{
    int32_t i;

    nInts = cl.nInts;
    theIntsSize = cl.theIntsSize;
    theInts = new int32_t[theIntsSize];

    for (i = 0; i < nInts; i++)
        theInts[i] = cl.theInts[i];
}

//
// Destructor
//

ChemIntList::~ChemIntList()
{
    if (theInts != NULL)
        delete [] theInts;
}

//
// Returns index of given integer in list, or -1 if not found
//

int32_t
ChemIntList::find(const int32_t integer) const
{
    int32_t i;

    for (i = 0; i < nInts; i++)
        if (theInts[i] == integer)
            return(i);

    return -1;  // Not found
}

//
// Inserts given integer in list before integer with given index
//

void
ChemIntList::insert(int32_t integer, int32_t addBefore)
{
    int32_t i;

    // If addBefore is off the end of the list, grow the list (and
    // initialize any new elements to NULL)
    if (addBefore > nInts) grow(addBefore);

    // Make room for one more
    setSize(nInts + 1);

    // Move integers forward to make room
    for (i = nInts - 1; i > addBefore; --i)
        theInts[i] = theInts[i - 1];

    // Insert the new one
    theInts[addBefore] = integer;
}

//
// Removes integer with given index
//

void
ChemIntList::remove(int32_t which)
{
    int32_t i;

    // Move all integers after the ith one backward
    for (i = which; i < nInts - 1; i++)
        theInts[i] = theInts[i + 1];

    // Shrink the list
    setSize(nInts - 1);
}

//
// Copies a integer list
//

void
ChemIntList::copy(const ChemIntList &cl)
{
    int32_t i;

    setSize(cl.nInts);

    for (i = 0; i < nInts; i++)
        theInts[i] = cl.theInts[i];
}

//
// Makes the list big enough for "newSize" elements, and initializes
// any newly created elements to NULL.
//

void
ChemIntList::grow(int32_t max) const
{
    int32_t newSize = max+1;
    int32_t oldSize = nInts;

#ifdef DEBUG
    if (newSize <= oldSize)
        SoDebugError::post("(internal) ChemIntList::grow", "newSize <= oldSize!");
#endif /* DEBUG */

    // Get around the const thing:
    ChemIntList *me = (ChemIntList *)this;

    me->setSize(newSize);
    
    for (int32_t i = oldSize; i < newSize; i++) {
      /*
	 NULL is casted in int32_t to avoid compiler warning
	 changed by fabien fontaine the 14/12/2000
       */
      me->theInts[i] = (int32_t) 0;
    }
}


//
// Changes size of list to be new size. If the new size is larger than
// the old one, it allocates more space.
//

void
ChemIntList::expand(int32_t size)
{
    int32_t *newInts;
    int32_t i;

    if (theIntsSize == 0)
        theIntsSize = DEFAULT_INITIAL_SIZE;

    while (size > theIntsSize) {
#ifdef DEBUG
        // check for overflow
        int32_t     oldIntsSize = theIntsSize;
        theIntsSize *= 2;
        if (theIntsSize < oldIntsSize)
            SoDebugError::post("ChemIntList::expand",
                               "Attempt to expand list beyond capacity;\n"
                               "A core dump is likely");
#else
        theIntsSize *= 2;
#endif
    }

    newInts = new int32_t[theIntsSize];

    if (theInts != NULL) {
        for (i = 0; i < nInts; i++) {
            newInts[i] = theInts[i];
        }
        delete [] theInts;
    }

    theInts = newInts;
}

// NOTE: this must be called only if the number of elements in the two
// lists is the same, otherwise badness could result

int32_t
ChemIntList::compare(const ChemIntList &cl) const
{
    int32_t i;
    for (i = 0; i < nInts; i++) {
        if ((*this)[i] != cl[i]) {
            return FALSE;
        }
    }

    return TRUE;
}

// Invokes qsort on the list

void
ChemIntList::sort(SbBool makeUnique)
{
    if (theInts != NULL && nInts > 0) {
        qsort(theInts, nInts, sizeof(int32_t), compareInts);
        if (makeUnique) this->unique();
    }
}

// Performs a binary search on the list by invoking bsearch

int32_t
ChemIntList::search(const int32_t *item, int32_t start, int32_t end)
{
    if (end == -1 || end > nInts) end = nInts - 1;
    int32_t nElem = end - start + 1;

    if ((theInts != NULL) && (nInts > 0) && (nElem > 0)) {
        int32_t *result = (int32_t *)bsearch(item, &theInts[start], nElem,
                                             sizeof(int32_t), compareInts);
        if (result == NULL) {
            return -1;
        }
        else {
            return (result - theInts);
        }
    }
    else {
        return -1;
    }
}

// Makes the elements of theInts unique.  theInts must have been sorted
// before invoking unique.

void
ChemIntList::unique()
{
    if (theInts == NULL || nInts <= 0) return;

    int32_t i, j;
    int32_t newListCount;
    int32_t *newInts = new int32_t[theIntsSize];

    i = 0;
    j = 1;
    newListCount = 0;
    newInts[newListCount++] = theInts[0];

    while (i+j < nInts) {
        if (theInts[i] != theInts[i+j]) {
            i += j;
            j = 1;
            newInts[newListCount++] = theInts[i];
        }
        else {
            j++;
        }
    }
    delete [] theInts;
    theInts = newInts;
    nInts = newListCount;
}

int32_t
compareInts(const void *int1, const void *int2)
{
    if (*(int32_t *)int1 < *(int32_t *)int2) return -1;
    if (*(int32_t *)int1 > *(int32_t *)int2) return 1;
    return 0;
}
