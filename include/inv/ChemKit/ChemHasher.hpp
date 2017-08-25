/*
 * ChemHasher.h
 *
 *     ChemHasher class: Uses a hash table to maintain a list of unique 2- or
 *     3-dimensional vectors. Adding a vector to the list checks to see if
 *     an identical vector has already been added and returns the old
 *     index, if so.
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

/////////////////////////////////////////////////////////////////////////////
//
// ChemHasher class: Uses a hash table to maintain a list of unique 2- or
// 3-dimensional vectors. Adding a vector to the list checks to see if
// an identical vector has already been added and returns the old
// index, if so.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef  __CHEM_HASHER_H__
#define  __CHEM_HASHER_H__

#include "inv/SbLinear.H"

class SbDict;
class SoMFVec2f;
class SoMFVec3f;

class ChemHasher {

  public:
    // Since this works with either 2- or 3-dimensional vectors, there
    // are two constructors. Each takes a field of the correct type in
    // which to store the results, minimum values and scale factors
    // used to set up a reasonable hash function, and the maximum
    // number of vectors to be added. The minimum values and scale
    // factors are used to bring the range of possible values of the
    // vectors into the interval (0,1) in all dimensions. Note that
    // nobody else should modify or access the passed field while the
    // ChemHasher instance is active, until finish() is called.
    ChemHasher(SoMFVec2f *field, int maxNumVectors,
	   const SbVec2f &min, const SbVec2f &scale);
    ChemHasher(SoMFVec3f *field, int maxNumVectors,
	   const SbVec3f &min, const SbVec3f &scale);

    // Destructor
    ~ChemHasher();

    // These add a vector to the field, or re-use an existing value in
    // it. The index of the value (new or old) is returned. Room is
    // made in the field, if necessary.
    int			addVector(const SbVec2f &newVector);
    int			addVector(const SbVec3f &newVector);

    // Finishes up
    void		finish();

  private:
    // These structures are stored in lists in the hash table
    // (external hashing) 
    struct HashEntry {
	int		index;		// Index into field
	HashEntry	*next;		// For creating hash lists
    };

    int			dimension;	// Dimension of vectors (2 or 3)
    SoMFVec2f		*field2;	// Given field
    SoMFVec3f		*field3;	// Given field
    SbVec2f		*vectors2;	// Pointer into field values 
    SbVec3f		*vectors3;	// Pointer into field values 
    SbVec3f		hashMin;	// Minimum coord values
    SbVec3f		hashScale;	// Hash scale factors (2 or 3 used)
    SbDict		*vectorDict;	// Dictionary storing vectors
    int			maxNum;		// Max number of vectors
    HashEntry		*entries;	// Array of HashEntry instances
    int			curEntry;	// Next entry to use

    // Stuff common to both constructors
    void		commonConstructor();

    // Adds the given vector to the hash table, if it is not already
    // there. Returns TRUE if the vector was added.
    SbBool		addIfNotThere(const float *newVector, int &index);

    // Makes room in the entries list and field for more values
    void		expandSize();

    // Hash function for finding duplicate vectors
    uint32_t		hashVector(const float *v);

    // Returns TRUE if the given vector matches the given existing vector
    SbBool		sameVector(const float *vector, int index);
};

#endif /* !__CHEM_HASHER_H__ */
