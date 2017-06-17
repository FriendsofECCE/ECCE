/*
 * ChemHasher.c++
 *
 *     Classes:  ChemHasher
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

#include "inv/SbDict.H"
#include "inv/SbPList.H"
#include "inv/fields/SoMFVec2f.H"
#include "inv/fields/SoMFVec3f.H"

#include "inv/ChemKit/ChemAssert.H"
#include "inv/ChemKit/ChemHasher.H"

/////////////////////////////////////////////////////////////////////////////
//
// Constructors for 2- or 3-dimensional vectors.
//
/////////////////////////////////////////////////////////////////////////////

ChemHasher::ChemHasher(SoMFVec2f *field, int maxNumVectors,
	       const SbVec2f &min, const SbVec2f &scale)
{
    dimension = 2;

    field2 = field;
    field3 = NULL;

    hashMin[0] = min[0];
    hashMin[1] = min[1];
    hashMin[2] = 0.0;

    hashScale[0] = scale[0];
    hashScale[1] = scale[1];
    hashScale[2] = 1.0;

    maxNum = maxNumVectors;

    commonConstructor();
}

ChemHasher::ChemHasher(SoMFVec3f *field, int maxNumVectors,
	       const SbVec3f &min, const SbVec3f &scale)
{
    dimension = 3;

    field2 = NULL;
    field3 = field;

    hashMin = min;
    hashScale = scale;

    maxNum = maxNumVectors;

    commonConstructor();
}

/////////////////////////////////////////////////////////////////////////////
//
// Stuff common to both constructors.
//
/////////////////////////////////////////////////////////////////////////////

void
ChemHasher::commonConstructor()
{
    // Create an array of HashEntry instances
    entries = new HashEntry[maxNum];
    curEntry = 0;

    // Make room in the field and begin editing
    if (dimension == 2) {
	field2->setNum(maxNum);
	vectors2 = field2->startEditing();
	vectors3 = NULL;
    }
    else {
	field3->setNum(maxNum);
	vectors3 = field3->startEditing();
	vectors2 = NULL;
    }

    // Set up the dictionary
    vectorDict = new SbDict(1235);

    // Multiply the scale values by some arbitrary numbers to make
    // the hash function more random
    hashScale[0] *=    233;
    hashScale[1] *=  72091;
    hashScale[2] *= 453451;
}

/////////////////////////////////////////////////////////////////////////////
//
// Destructor.
//
/////////////////////////////////////////////////////////////////////////////

ChemHasher::~ChemHasher()
{
}

/////////////////////////////////////////////////////////////////////////////
//
// These add a vector to the list, returning its index. Room is
// made in the field, if necessary.
//
/////////////////////////////////////////////////////////////////////////////

int
ChemHasher::addVector(const SbVec2f &newVector)
{
    ASSERT(vectorDict != NULL);
    ASSERT(dimension == 2);

    // See if we need to add this one, or is it already there
    int index;
    SbBool notThere = addIfNotThere(newVector.getValue(), index);

    // If it's not there, we need to add it to the field
    if (notThere)
	vectors2[index] = newVector;

    return index;
}

int
ChemHasher::addVector(const SbVec3f &newVector)
{
    ASSERT(vectorDict != NULL);
    ASSERT(dimension == 3);

    // See if we need to add this one, or is it already there
    int index;
    SbBool notThere = addIfNotThere(newVector.getValue(), index);

    // If it's not there, we need to add it to the field
    if (notThere)
	vectors3[index] = newVector;

    return index;
}

/////////////////////////////////////////////////////////////////////////////
//
// Adds the given vector to the hash table, if it is not already
// there. Returns TRUE if the vector was added.
//
/////////////////////////////////////////////////////////////////////////////

SbBool
ChemHasher::addIfNotThere(const float *newVector, int &index)
{
    ASSERT(curEntry < maxNum - 1);

    // Find the hash key
    uint32_t key = hashVector(newVector);

    // See if there already is a list of vectors with the same key
    void *list;
    if (vectorDict->find(key, list)) {

	// If so, look through the list for an exact match
	HashEntry *entry, *prevEntry = NULL;
	for (entry = (HashEntry *) list; entry != NULL; entry = entry->next) {

	    ASSERT(entry->index < curEntry);

	    // If the vectors are the same, re-use the old one and stop
	    if (sameVector(newVector, entry->index)) {
		index = entry->index;
		return FALSE;
	    }

	    prevEntry = entry;
	}

	// If there were no matches, add the new vector to the end of
	// the list
	ASSERT(prevEntry != NULL);
	entry = &entries[curEntry];
	entry->index = curEntry;
	entry->next = NULL;
	prevEntry->next = entry;
    }

    // If there were no entries in the dictionary, start one
    else {
	HashEntry *entry = &entries[curEntry];
	entry->index = curEntry;
	entry->next = NULL;
	vectorDict->enter(key, entry);
    }

    index = curEntry++;
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//
// Hash function for finding duplicate vectors.
//
/////////////////////////////////////////////////////////////////////////////

uint32_t
ChemHasher::hashVector(const float *v)
{
    float sum = 0.0;
    for (int i = 0; i < dimension; i++)
	sum += (v[i] - hashMin[i]) * hashScale[i];
    return (int32_t) sum;
}

/////////////////////////////////////////////////////////////////////////////
//
// Returns TRUE if the given vector matches the given existing vector.
//
/////////////////////////////////////////////////////////////////////////////

SbBool
ChemHasher::sameVector(const float *vector, int index)
{
    if (dimension == 2) {
        return SbVec2f(vector).equals(vectors2[index], 0.000001f);
    }
    else {
        return SbVec3f(vector).equals(vectors3[index], 0.000001f);
    }
}

/////////////////////////////////////////////////////////////////////////////
//
// Finishes up.
//
/////////////////////////////////////////////////////////////////////////////

void
ChemHasher::finish()
{
    // Stop editing and set the field to contain the correct number of
    // values
    if (dimension == 2) {
	field2->finishEditing();
	field2->setNum(curEntry);
    }
    else {
	field3->finishEditing();
	field3->setNum(curEntry);
    }

    // Get rid of the dictionary and the HashEntry instances
    delete vectorDict;
    delete [] entries;

    // Make sure nobody uses this again
    vectorDict = NULL;
}
