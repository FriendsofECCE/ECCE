/*
 * ChemSSSR.h
 *
 *  This file defines the ChemSSSR node class
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

#ifndef __CHEM_SSSR_H__
#define __CHEM_SSSR_H__

#include "inv/SbBasic.H"
#include "inv/SbPList.H"

#include "inv/ChemKit/ChemIntList.H"
#include "inv/ChemKit/ChemkitBasic.H"

class ChemPathMsg;
class ChemTNode;
// The SparseBits class acts like a bit vector without consuming
// so much space.  Map it to RWBitVec to make it easier to switch back
// and forth while minimizing ifdefs
class SparseBits;
class ChemHashSSSR;
class ChemBaseData;
class ChemDisplay;

class CHEMKIT_DLL_API ChemSSSR {

  public:
    ChemSSSR(const ChemBaseData *chemData);
    ~ChemSSSR();

    void findRings(int32_t maxRingSize, int32_t &numFound,
        int32_t &minSize, int32_t &maxSize, bool doSort=false
        /*, ChemBaseData *cbd=0*/);

    const RWBitVec *getRings() const
        { return finalTable; }
    const RWBitVec *getRingBonds() const
        { return ringBonds; }
    const SbPList &getFragments() const
        { return fragmentList; }


  private:
    ChemSSSR(int32_t numberOfAtoms, int32_t numberOfBonds,
// --> makeAtch modify the pointer contents
    //         const ChemIntList *_atch);
		ChemIntList *_atch,
// <-- makeAtch modify the pointer contents
// --> moiv doublebond error
	  ChemDisplay* chemDisplay
// <-- moiv doublebond error
		);


    void initialize();
    void cleanUpA();
    void cleanUpB();
    void cleanUpTNodeList();
    void makeAtch();
    bool makeFragAtch(int32_t which);

    void   findFragments();
    SbBool ringSelect(const RWBitVec& beer);
    SbBool linearCombination(const RWBitVec& beer);

    int32_t numberOfAtoms;        // #atoms in entire structure
    int32_t numberOfBonds;        // #bonds in entire structure
    int32_t numberOfFragments;    // #fragments in structure

    int32_t   fragNumberOfNodes;  // # of ring-only nodes in the current fragment
    int32_t   fragNumberOfEdges;  // # of ring-only edges in the current fragment
    int32_t   fragMaxRing;        // max. #rings in the current fragment
    ChemIntList *fragAtch;        // ring-only atch list for the current fragment
    ChemIntList *fragBondList;    // bond reordering for the current fragment

    SbPList   fragmentList;       // List of fragments
    SbIntList fragMaxRingList;    // max. #rings in each fragment

    SbPList   ringBondList;       // list of ring bonds in each fragment
    RWBitVec *ringBonds;          // all ring bonds in the entire structure

    int32_t iterationCounter;
    int32_t tablePos;
    int32_t maxRing;

    const ChemBaseData *klshack;
    const ChemBaseData *chemData;
// --> moiv doublebond error
		const ChemDisplay* chemDisplay;
// <-- moiv doublebond error
    ChemIntList *atch;
    
    int32_t     *bonds;
    ChemTNode   *tnodeList;
    ChemPathMsg **aList;
    SbPList     *bList;

    RWBitVec *bitVecTable;
    RWBitVec *gaussElimTable;
    RWBitVec *finalTable;
    int32_t  *pivots;

    ChemHashSSSR *hash;

  friend class ChemDisplay;
};

#endif /* !__CHEM_SSSR_H__ */
