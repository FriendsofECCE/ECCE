/*
 * ChemIntList.h
 *
 *	This contains the definition of the ChemIntList integer
 *      list class. A ChemIntList is a list of integers that
 *      allows easy insertion, removal, and other operations.
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

#ifndef __CHEM_INTLIST_H__
#define __CHEM_INTLIST_H__

#include "inv/SbBasic.H"
#include "inv/SbLinear.H"

#include "inv/ChemKit/ChemkitBasic.H"

//////////////////////////////////////////////////////////////////////////////
//
//  Class: ChemIntList
//
//  List of integers. ChemIntList allows random access,
//  insertion, and removal. It also invokes qsort and bsearch on the
//  list of integers
//
//////////////////////////////////////////////////////////////////////////////

class CHEMKIT_DLL_API ChemIntList {
  public:

    // Constructor
    ChemIntList();

    // Constructor allocates enough room for the given number of pointers
    ChemIntList(int32_t initSize);

    // Constructor for copying
    ChemIntList(const ChemIntList &cl);

    // Destructor
    ~ChemIntList();

    // Adds given integer to end of list
    void    append(int32_t integer)
        { if (nInts + 1 > theIntsSize) expand(nInts + 1);
          theInts[nInts++] = integer; }

    // Returns index of given integer in list, or -1 if not found
    int32_t find(const int32_t integer) const;

    // Inserts given integer in list before integer with given index
    void    insert(int32_t integer, int32_t addBefore);

    // Removes integer with given index
    void    remove(int32_t which);

    // Returns number of integers in list
    int32_t getLength() const       { return (int32_t) nInts;   }

    // Removes all integers after one with given index, inclusive
    // ??? should really be an error check in here
    void    truncate(int32_t start) { nInts = start; }

    // Copy a ChemIntList
    void    copy(const ChemIntList &cl);

    // Sorts the list using qsort.  If makeUnique is true, only unique
    // values are kept in the resultant list.
    void    sort(SbBool makeUnique = FALSE);

    // Make the values in the list unique.  Must have been sorted first.
    void    unique();

    // Performs a binary search on the list using bsearch.  start
    // allows the search to begin other than at the beginning.  end
    // says where the search should stop.  -1 means the end of the list.
    int32_t search(const int32_t *item, int32_t start = 0,
                   int32_t end = -1);

    ChemIntList &   operator =(const ChemIntList &cl)
        { copy(cl); return *this; }

    // Returns integer with given index
    int32_t & operator [](int32_t i) const
        { if (i >= nInts) grow(i); return theInts[i]; }

    // equality tests
    int32_t operator ==(const ChemIntList &cl) const
        { return cl.nInts == nInts ? compare(cl) : FALSE; }
    int32_t operator !=(const ChemIntList &cl) const
        { return cl.nInts == nInts ? ! compare(cl) : TRUE; }
  
// Internal versions of [] that do not check for bounds:
  SoINTERNAL public:
    int32_t get(int32_t i) const { return theInts[i]; }
    void    set(int32_t i, int32_t j) { theInts[i] = j; }

  private:

    // NOTE: this must be called only if the number of elements in the two
    // lists is the same, otherwise badness could result
    int32_t compare(const ChemIntList &cl) const;

    int32_t *theInts;       // The collection of integers
    int32_t nInts;          // Number of integers used
    int32_t theIntsSize;    // Number of integers allocated

    // There are three(!) methods to expand the list.  grow() is used
    // when space is dynamically created, and needs to be initialized
    // to NULL:
    void    grow(int32_t max) const;

    // setSize is used by grow and in other places internally where we
    // know that nothing needs to be initialized to NULL.
    void    setSize(int32_t size)
    { if (size > theIntsSize) expand(size); nInts = size; }

    // expand is the lowest level routine.  It just reallocates the
    // array and copies over the old values.
    void    expand(int32_t size);
};

#endif /* !__CHEM_INTLIST_H__ */
