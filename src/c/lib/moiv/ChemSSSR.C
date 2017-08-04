/*
 * ChemSSSR.c++
 *
 *     Classes:  ChemSSSR, ChemPathMsg, ChemTNode, ChemHashSSSR
 *
 *     This code implements the algorithms described in:
 *     "Efficient Exact Solution of the Ring Perception Problem"
 *      Renzo Balducci and Robert S. Pearlman,
 *      J. Chem. Inf. Comput. Sci., 1994, 34, 822-831
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

#ident "$Revision: 22566 $"
#include <iostream>
using namespace std;

//#include <vector>
//  using std::vector;

#include <assert.h>
//#include <strstream.h>
#include <stdlib.h>

#include "inv/SbPList.H"

#include "inv/ChemKit/SparseBits.H"

// --> moiv doublebond error
#include "inv/ChemKit/ChemDisplay.H"
// <-- moiv doublebond error
#include "inv/ChemKit/ChemBaseData.H"
#include "inv/ChemKit/ChemIntList.H"
#include "inv/ChemKit/ChemSSSR.H"

#include "inv/ChemKit/ChemkitBasic.H"

////////////////////////////////////////////////////////////////////////
//
//    ChemPathMsg class:
//    These are the messages passed between the ChemTNodes
//
////////////////////////////////////////////////////////////////////////

class CHEMKIT_DLL_API ChemPathMsg {
  public:
    ChemPathMsg(int32_t numberOfEdges);
    ChemPathMsg(const ChemPathMsg *pathMsg);
    ~ChemPathMsg();
    
    void push(int32_t newNLast, int32_t newEdge);
 
    RWBitVec beep;
    int32_t  firstNode;
    int32_t  lastNode;
    int32_t  firstEdge;
};

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Constructor
//
// Use: public

ChemPathMsg::ChemPathMsg(int32_t numberOfEdges)
//
////////////////////////////////////////////////////////////////////////
{
    beep.resize(numberOfEdges);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Copy constructor
//
// Use: public

ChemPathMsg::ChemPathMsg(const ChemPathMsg *pathMsg)
//
////////////////////////////////////////////////////////////////////////
{
    beep = pathMsg->beep;
    firstNode = pathMsg->firstNode;
    lastNode = pathMsg->lastNode;
    firstEdge = pathMsg->firstEdge;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Destructor
//
// Use: public
    
ChemPathMsg::~ChemPathMsg()
//
////////////////////////////////////////////////////////////////////////
{
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Updates pathMsg with a new lastNode and new beep
//
// Use: public
    
void
ChemPathMsg::push(int32_t newNLast, int32_t newEdge)
//
////////////////////////////////////////////////////////////////////////
{    
    lastNode = newNLast;
    beep.setBit(newEdge);
}

////////////////////////////////////////////////////////////////////////
//
//    ChemTNode class:
//    These are the transceivers
//
////////////////////////////////////////////////////////////////////////

class CHEMKIT_DLL_API ChemTNode {
  public:
    ChemTNode();
    ~ChemTNode();
    
    SbPList *sendBuffer;
    SbPList *receiveBuffer;
};

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Constructor
//
// Use: public

ChemTNode::ChemTNode()
//
////////////////////////////////////////////////////////////////////////
{
    sendBuffer = new SbPList(100);
    receiveBuffer = new SbPList(100);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Destructor
//
// Use: public

ChemTNode::~ChemTNode()
//
////////////////////////////////////////////////////////////////////////
{
    delete sendBuffer;
    delete receiveBuffer;
}

////////////////////////////////////////////////////////////////////////
//
//    ChemHashSSSR class:
//    This is the implementation of the hash table used for ChemSSSR
//
////////////////////////////////////////////////////////////////////////

typedef struct _chemHashEntry {
    unsigned int key;
    RWBitVec bitvec;
} ChemHashEntry;

typedef struct _chemHashNode {
    ChemHashEntry item;
    struct _chemHashNode *next;
} ChemHashNode;

class CHEMKIT_DLL_API ChemHashSSSR {
  public:
    ChemHashSSSR();
    ~ChemHashSSSR();

    SbBool initialize(int32_t size);
    ChemHashEntry *search(ChemHashEntry item, SbBool *found);

  private:
    ChemHashEntry *build(ChemHashNode **last, ChemHashNode *next,
                         ChemHashEntry item);

    ChemHashNode **hashTable;
    unsigned int length;
    unsigned int m;
    unsigned int prcnt;
};

#define FACTOR 035761254233
#define BITSPERBYTE 8
#define SHIFT ((BITSPERBYTE * sizeof(int)) - m)

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Constructor
//
// Use: public

ChemHashSSSR::ChemHashSSSR()
//
////////////////////////////////////////////////////////////////////////
{
    hashTable = NULL;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Destructor
//
// Use: public

ChemHashSSSR::~ChemHashSSSR()
//
////////////////////////////////////////////////////////////////////////
{
    if (hashTable == NULL) return;

    int32_t i;
    ChemHashNode *p, *oldp;
    for (i = 0; i < length; i++) {
        if (hashTable[i] != (ChemHashNode *)NULL) {
            p = hashTable[i];
            while (p != (ChemHashNode *)NULL) {
                oldp = p;
                p = p->next;
                delete oldp;
            }
        }
    }
    delete [] hashTable;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Set up the initial values and hashTable
//
// Use: public

SbBool
ChemHashSSSR::initialize(int32_t size)
//
////////////////////////////////////////////////////////////////////////
{
    unsigned int unsize;

    if (size <= 0) return FALSE;

    unsize = size;
    length = 1;
    m = 0;

    while (unsize) {
        unsize >>= 1;
        length <<= 1;
        m++;
    }

    hashTable = new (ChemHashNode *[length]);

    if (hashTable != NULL) {
        for (unsize = 0; unsize < length; unsize++) {
            hashTable[unsize] = NULL;
        }
        return TRUE;
    }

    return FALSE;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Search for item in the hashTable.  Items are always added if 
//    they are not present.  "found" returns whether or not the item 
//    was initially in the hashTable.
//
// Use: public

ChemHashEntry *
ChemHashSSSR::search(ChemHashEntry item, SbBool *found)
//
////////////////////////////////////////////////////////////////////////
{
    ChemHashNode *p;        // searches through the linked list
    ChemHashNode **q;       // where to store the pointer to a new ChemHashNode
    unsigned int i; // result of hash
    int res;        // result of comparisons

    prcnt = 1;

    i = (unsigned int) ((item.key * FACTOR) >> SHIFT);  // hashTable[i] contains
                                               // list head

    if (hashTable[i] == (ChemHashNode *)NULL) { // List has not yet been begun
        *found = FALSE;
        return (build(&hashTable[i], (ChemHashNode *)NULL, item));
    }
    else {                      // List is not empty
        q = &hashTable[i];
        p = hashTable[i];
        //while (p != NULL && (res = (item.bitvec != p->item.bitvec))) {
        while (p != NULL && (res = (!item.bitvec.isEqual(p->item.bitvec)))) {
            prcnt++;
            q = &(p->next);
            p = p->next;
        }

        if (p != NULL && res == 0) {       // Item has been found
            *found = TRUE;
            return (&(p->item));
        }
        else {                  // Item is not yet on list
            *found = FALSE;
            return (build(q, p, item));
        }
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Creates a new entry in the hashTable
//
// Use: private

ChemHashEntry *
ChemHashSSSR::build(ChemHashNode **last, ChemHashNode *next,
    ChemHashEntry item)
//
////////////////////////////////////////////////////////////////////////
{
    ChemHashNode *p = new ChemHashNode;
    
    if (p != NULL) {
        p->item = item;
        *last = p;
        p->next = next;
        return (&(p->item));
    }
    else {
        return ((ChemHashEntry *)NULL);
    }
}

#undef FACTOR
#undef BITSPERBYTE
#undef SHIFT


////////////////////////////////////////////////////////////////////////
//
//    ChemSSSR class:
//    Finds rings in a system of nodes and edges as defined in atch
//
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Performs the compare for the qsort of the finalTable of bitvecs.
//
// Use: static

static int
cmpBitVec(const void *bv1, const void *bv2)
//
////////////////////////////////////////////////////////////////////////
{
    RWBitVec *tbv1 = (RWBitVec *)bv1;
    RWBitVec *tbv2 = (RWBitVec *)bv2;

    int32_t s1 = tbv1->sum();
    int32_t s2 = tbv2->sum();

    if (s1 < s2) return -1;
    if (s1 > s2) return 1;

    // So we have the same size ring.  Check firstTrue for each.
    // If still the same, return 0.
    int32_t ft1 = tbv1->firstTrue();
    int32_t ft2 = tbv2->firstTrue();
    if (ft1 < ft2) return -1;
    if (ft1 > ft2) return 1;
    return 0;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Constructor
//
// Use: public

ChemSSSR::ChemSSSR(const ChemBaseData *_chemData)
//
////////////////////////////////////////////////////////////////////////
{
    klshack = 0;
    // Initialize local values
    atch = NULL;
    bonds = NULL;
    fragAtch = NULL;
    fragBondList = NULL;
    tnodeList = NULL;
    aList = NULL;
    bList = NULL;
    bitVecTable = NULL;
    gaussElimTable = NULL;
    finalTable = NULL;
    pivots = NULL;
    hash = NULL;
    chemDisplay = NULL;

    ringBonds = NULL;

    // Store a pointer to _chemData in case we need to access it
    chemData = _chemData;

    // Get the number of rings possible
    numberOfAtoms = chemData->getNumberOfAtoms();
    numberOfBonds = chemData->getNumberOfBonds();

    // If there are less than 3 atoms or less than 3 bonds,
    // then there can't be any rings
    if ((numberOfAtoms < 3) || (numberOfBonds < 3)) {
        maxRing = 0;
        return;
    }

    // Make the atch array
    makeAtch();

    // See if there are any fragments.  The atoms in each fragment are
    // stored in fragmentList.  
    findFragments();

    // Using the number of atoms and bonds in each fragment, we can
    // now accurately determine the value of maxRing.
    maxRing = 0;
    int32_t i, j;
    int32_t tmpAtoms = 0, tmpBonds = 0, tmpRings =0;

// --> moiv doublebond error
		// this is done for avoiding memory faults in fragMaxRingList[i] = tmpRings
		// and fragMaxRingList[i] = 0;!!!
		// using append method the space is correctly allocated
		for (i = 0; i < numberOfFragments; i++) fragMaxRingList.append(0);
// <-- moiv doublebond error

    for (i = 0; i < numberOfFragments; i++) {
        RWBitVec *bv = (RWBitVec *)fragmentList[i];
        vector<int> setbits;
        bv->getSetBits(setbits);
        tmpAtoms = setbits.size();
        for (j = 0; j < tmpAtoms; j++) {
           tmpBonds += atch[setbits[j]].getLength() / 2;
        }

        // Since atch is symmetric, each bond is counted twice
        tmpBonds /= 2;
        tmpRings = tmpBonds - tmpAtoms + 1;
        if (tmpRings > 0) {
            fragMaxRingList[i] = tmpRings;
            maxRing += tmpRings;
        }
        else {
            fragMaxRingList[i] = 0;
        }
    }
    delete [] atch;
    atch = NULL;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Constructor
//
// Use: public

ChemSSSR::ChemSSSR(int32_t _numberOfAtoms, int32_t _numberOfBonds,
// --> makeAtch modify the pointer contents
    //const ChemIntList *_atch)
		ChemIntList *_atch,
		ChemDisplay* _chemDisplay)
// <--> makeAtch modify the pointer contents
//
////////////////////////////////////////////////////////////////////////
{
    klshack = 0;
    // Initialize local values
    atch = (ChemIntList *)_atch;
    bonds = NULL;

    numberOfAtoms = _numberOfAtoms;
    numberOfBonds = _numberOfBonds;

    chemData = NULL;
// --> moiv doublebond error
		chemDisplay = _chemDisplay;
// <-- moiv doublebond error
    fragAtch = NULL;
    fragBondList = NULL;
    tnodeList = NULL;
    aList = NULL;
    bList = NULL;
    bitVecTable = NULL;
    gaussElimTable = NULL;
    finalTable = NULL;
    pivots = NULL;
    hash = NULL;

    ringBonds = NULL;

    // If there are less than 3 atoms or less than 3 bonds,
    // then there can't be any rings
    if ((numberOfAtoms < 3) || (numberOfBonds < 3)) {
        maxRing = 0;
        return;
    }

    // See if there are any fragments.  The atoms in each fragment are
    // stored in fragmentList.  
    findFragments();

    // Using the number of atoms and bonds in each fragment, we can
    // now accurately determine the value of maxRing.

    maxRing = 0;
    int32_t i, j;
    int32_t tmpAtoms = 0, tmpBonds = 0, tmpRings =0;

// --> moiv doublebond error
		// this is done for avoiding memory faults in fragMaxRingList[i] = tmpRings
		// and fragMaxRingList[i] = 0;!!!
		// using append method the space is correctly allocated
		for (i = 0; i < numberOfFragments; i++) fragMaxRingList.append(0);
// <-- moiv doublebond error

    for (i = 0; i < numberOfFragments; i++) {
        RWBitVec *bv = (RWBitVec *)fragmentList[i];
        vector<int> setbits;
        bv->getSetBits(setbits);
        tmpAtoms = setbits.size();
        for (j = 0; j < tmpAtoms; j++) {
           tmpBonds += atch[setbits[j]].getLength() / 2;
        }

        // Since atch is symmetric, each bond is counted twice
        tmpBonds /= 2;
        tmpRings = tmpBonds - tmpAtoms + 1;
        if (tmpRings > 0) {
            fragMaxRingList[i] = tmpRings;
            maxRing += tmpRings;
        }
        else {
            fragMaxRingList[i] = 0;
        }
    }

    // maxRing is the maximum number of linearly independent rings
    // possible in the system
    if (maxRing <= 0) return;

    // Make a bond-based list
    bonds = new int32_t[2*numberOfBonds];

    int32_t k;
    int32_t atom1, atom2;
    for (i = 0; i < numberOfAtoms; i++) {
        for (j = 0; j < atch[i].getLength(); j+=2) {
            atom1 = i;
            atom2 = atch[i][j];
            if (atom1 > atom2) {
                atom2 = atom1;
                atom1 = atch[i][j];
            }
            // k is the bond index
            k = atch[i][j+1];
						// --> moiv doublebond error
						k = chemDisplay->bondAttributesIndex[k];
						assert(k>=0 && k<numberOfBonds);
						// <-- moiv doublebond error

            bonds[2*k]   = atom1;
            bonds[2*k+1] = atom2;
        }
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Destructor
//
// Use: public

ChemSSSR::~ChemSSSR()
//
////////////////////////////////////////////////////////////////////////
{
// --> moiv doublebond error
#ifdef __coin
		// this is done for avoiding memory leaks
		// it seems that space allocated for the pointers of fragmenList and ringBondList
		// aren't freed
		fragmentList.truncate(0,TRUE);
		ringBondList.truncate(0,TRUE);
		fragMaxRingList.truncate(0,TRUE);
#endif // _coin
// <-- moiv doublebond error

    if (maxRing <= 0) return;

    cleanUpTNodeList();
    cleanUpA();
    cleanUpB();

    if (bonds != NULL) delete [] bonds;
    if (fragAtch != NULL) delete [] fragAtch;
    if (fragBondList != NULL) delete fragBondList;

    if (tnodeList != NULL) delete [] tnodeList;
    if (aList != NULL) delete [] aList;
    if (bList != NULL) delete [] bList;

    if (bitVecTable != NULL) delete [] bitVecTable;
    if (gaussElimTable != NULL) delete [] gaussElimTable;
    if (finalTable != NULL) delete [] finalTable;
    if (pivots != NULL) delete [] pivots;

    if (hash != NULL) delete hash;

		RWBitVec *bv = NULL;
    for (int32_t i = 0; i < numberOfFragments; i++) {
				bv = (RWBitVec *)fragmentList[i];
        delete bv;

        bv = (RWBitVec *)ringBondList[i];
        delete bv;
    }

		if (ringBonds != NULL) delete ringBonds;    
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Make the full atch array
//
// Use: private

void
ChemSSSR::makeAtch()
//
////////////////////////////////////////////////////////////////////////
{
    if (atch != NULL) delete [] atch;
    atch = new ChemIntList[numberOfAtoms];

    int32_t i;
    int32_t atom1, atom2;
    for (i = 0; i < numberOfBonds; i++) {
        atom1 = chemData->getBondFrom(i);
        atom2 = chemData->getBondTo(i);
        atch[atom1].append(atom2);
        atch[atom2].append(atom1);

        // Keep the bond number in the atch list
        atch[atom1].append(i);
        atch[atom2].append(i);
    }
}
                
////////////////////////////////////////////////////////////////////////
//
// Description:
//    Make the fragment atch array
//
// Use: private

bool
ChemSSSR::makeFragAtch(int32_t which)
//
////////////////////////////////////////////////////////////////////////
{
    int32_t i;
    int32_t numAtomsAndBonds;

    // The number of bonds is the sum of the turned on bits in
    // fragRingBonds.  The maximum number of atoms involved in rings will
    // be this same number.
    RWBitVec *fragRingBonds = (RWBitVec *)ringBondList[which];
    numAtomsAndBonds = fragRingBonds->sum();
    // numAtomsAndBonds is the number of ring bonds so at this point we
    // already have filtered out atoms NOT in any type of ring.
    // Lots of systems have a value of 0 (solvents).
    // No sense creating and initializing memory for all of these.
    // 3 is the minimum you need for a ring.
    if (numAtomsAndBonds <= 3) return false;
    //cout << "number of ring bonds " << numAtomsAndBonds << endl;

    // Make a new attachment list with just the ring bonds and atoms
    assert(fragAtch == NULL);
    fragAtch = new ChemIntList[numAtomsAndBonds];

    int32_t atom1, atom2;
    int32_t index1, index2;
    int32_t *atomList = new int32_t[numberOfAtoms];
    for (i = 0; i < numberOfAtoms; i++) {
        atomList[i] = -1;
    }

    assert(fragBondList == NULL);
    fragBondList = new ChemIntList(numAtomsAndBonds);

    if (chemData != NULL) {
        fragNumberOfNodes = 0;
        fragNumberOfEdges = 0;
        int bondNo;
        vector<int> setbits;
        fragRingBonds->getSetBits(setbits);
        int nSetBits = setbits.size();;
        for (i = 0; i < nSetBits; i++) {
           bondNo = setbits[i];

           atom1 = chemData->getBondFrom(bondNo);
           atom2 = chemData->getBondTo(bondNo);
            
           if ((index1 = atomList[atom1]) == -1) {
               index1 = atomList[atom1] = fragNumberOfNodes;
               fragNumberOfNodes++;
           }
           if ((index2 = atomList[atom2]) == -1) {
               index2 = atomList[atom2] = fragNumberOfNodes;
               fragNumberOfNodes++;
           }
           fragAtch[index1].append(index2);
           fragAtch[index2].append(index1);
           fragAtch[index1].append(fragNumberOfEdges);
           fragAtch[index2].append(fragNumberOfEdges);
    
           fragNumberOfEdges++;
           fragBondList->append(bondNo);
        }
        delete [] atomList;
    }
    else if (atch != NULL) {
        // The bonds array was made in the constructor
        fragNumberOfNodes = 0;
        fragNumberOfEdges = 0;
        int bondNo;
        vector<int> setbits;
        fragRingBonds->getSetBits(setbits);
        int nSetBits = setbits.size();;
        for (i = 0; i < nSetBits; i++) {
           bondNo = setbits[i];

           atom1 = bonds[2*bondNo];
           atom2 = bonds[2*bondNo+1];
    
           if ((index1 = atomList[atom1]) == -1) {
               index1 = atomList[atom1] = fragNumberOfNodes;
               fragNumberOfNodes++;
           }
           if ((index2 = atomList[atom2]) == -1) {
               index2 = atomList[atom2] = fragNumberOfNodes;
               fragNumberOfNodes++;
           }
           fragAtch[index1].append(index2);
           fragAtch[index2].append(index1);
           fragAtch[index1].append(fragNumberOfEdges);
           fragAtch[index2].append(fragNumberOfEdges);

           fragNumberOfEdges++;
           fragBondList->append(bondNo);
        }
        delete [] atomList;
    }
    assert(fragNumberOfNodes <= numAtomsAndBonds);
    assert(fragNumberOfEdges == numAtomsAndBonds);
    return true;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Allocate the arrays and initialize variable in preparation for
//    finding rings
//
// Use: private

void
ChemSSSR::initialize()
//
////////////////////////////////////////////////////////////////////////
{
    if (fragMaxRing <= 0) return;

    int32_t i, j, numberOfConnections;

    // Create a list of all ChemTNodes
    assert(tnodeList == NULL);
    tnodeList = new ChemTNode[fragNumberOfNodes];

    // Create a table capable of holding fragMaxRing entries of bitvec with
    // each bitvec being fragNumberOfEdges in length
    assert(bitVecTable == NULL);
    bitVecTable = new RWBitVec[fragMaxRing+1];
 
    // Create the table used for the gaussian elimination tests
    assert(gaussElimTable == NULL);
    gaussElimTable = new RWBitVec[fragMaxRing+1];

    // Create a table to hold the pivots used in the Gaussian elimination
    assert(pivots == NULL);
    pivots = new int32_t[fragMaxRing+1];

    // Make the A and B arrays of ChemPathMsg's that are used by all ChemTNodes
    assert(aList == NULL);
    aList = new (ChemPathMsg *[2*fragNumberOfEdges]);
    assert(bList == NULL);
    bList = new SbPList[fragNumberOfNodes];

    for (i = 0; i < 2*fragNumberOfEdges; i++) {
        aList[i] = NULL;
    }

    // Create a hash table large enough to deal with maxRing entries
    assert(hash == NULL);
    hash = new ChemHashSSSR;
    if (fragMaxRing > 10) {
        hash->initialize(fragMaxRing);
    }
    else {
        hash->initialize(10);
    }

    // Initialize the tnode sendBuffers for the first "send"
    //cout << "Number of frag nodes " << fragNumberOfNodes << " " << fragNumberOfEdges << endl;
    int xcounter = 0;
    int counter = 0;
    for (i = 0; i < fragNumberOfNodes; i++) {
        // Get the number of connections to each node and make a
        // ChemPathMsg for each connection.  These ChemPathMsg's go into
        // the node's sendBuffer.

        numberOfConnections = fragAtch[i].getLength();
        //cout << "number of connections" << numberOfConnections << endl;

        for (j = 0; j < numberOfConnections; j+=2) {
           xcounter++;
            int bondNo = (*fragBondList)[fragAtch[i][j+1]];
            //cout << "bond number " << fragAtch[i][j+1] << " " << bondNo;

            if (klshack) {
             //  cout << "bondtype " << klshack->getBondType(bondNo);
            }
            //cout << endl;
            if (klshack) {
               int type1 = klshack->getBondType(bondNo);
               if (!(type1 == 2 || type1 == 5)) {
                  //cout << "skipping " << type1 << endl;
                  continue;
               }
            }
            ChemPathMsg *pathMsg = new ChemPathMsg(fragNumberOfEdges);
            pathMsg->firstNode = pathMsg->lastNode = fragAtch[i][j];
            //cout << "first node is " << fragAtch[i][j] << endl;
            pathMsg->firstEdge = fragAtch[i][j+1];
            pathMsg->beep.setBit(fragAtch[i][j+1]);
            tnodeList[i].sendBuffer->append((void *)pathMsg);
            counter++;
        }
    }
    //cout << "created chemPathMsg objects " << counter << " " << xcounter << endl;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Delete any pathMsgs in the aList
//
// Use: private

void
ChemSSSR::cleanUpA()
//
////////////////////////////////////////////////////////////////////////
{
    int32_t i;

    if (aList == NULL) return;

    for (i = 0; i < 2*fragNumberOfEdges; i++) {
        if (aList[i] != NULL) {
            delete aList[i];
            aList[i] = NULL;
        }
    }
    delete [] aList;
    aList = NULL;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Delete any pathMsgs in the bList
//
// Use: private

void
ChemSSSR::cleanUpB()
//
////////////////////////////////////////////////////////////////////////
{
    int32_t i, j, numberOfPaths;
    ChemPathMsg *pathMsg;

    if (bList == NULL) return;

    for (i = 0; i < fragNumberOfNodes; i++) {
        // Delete any pathMsgs in bList
        numberOfPaths = bList[i].getLength();
        for (j = 0; j < numberOfPaths; j++) {
            pathMsg = (ChemPathMsg *)bList[i][j];
            if (pathMsg != NULL) delete pathMsg;
        }
    }
    delete [] bList;
    bList = NULL;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Delete any pathMsgs in the tnodeList
//
// Use: private

void
ChemSSSR::cleanUpTNodeList()
//
////////////////////////////////////////////////////////////////////////
{
    if (tnodeList == NULL) return;

    int32_t i, j, numberOfPaths;
    ChemPathMsg *pathMsg;

    for (i = 0; i < fragNumberOfNodes; i++) {
        // Delete any pathMsgs in the tnode's sendBuffer
        numberOfPaths = tnodeList[i].sendBuffer->getLength();
        for (j = 0; j < numberOfPaths; j++) {
            pathMsg = (ChemPathMsg *)tnodeList[i].sendBuffer->get(j);
            if (pathMsg != NULL) delete pathMsg;
        }
        tnodeList[i].sendBuffer->truncate(0);

        // Delete any pathMsgs in the tnode's receiveBuffer
        numberOfPaths = tnodeList[i].receiveBuffer->getLength();
        for (j = 0; j < numberOfPaths; j++) {
            pathMsg = (ChemPathMsg *)tnodeList[i].receiveBuffer->get(j);
            if (pathMsg != NULL) delete pathMsg;
        }
        tnodeList[i].receiveBuffer->truncate(0);
    }
    delete [] tnodeList;
    tnodeList = NULL;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Find the rings in the system defined in fragAtch
//
// Use: public

void
ChemSSSR::findRings(int32_t maxRingSize, int32_t &numFound,
    int32_t &minSize, int32_t &maxSize, bool doSort /*, ChemBaseData *cbd*/)
//
////////////////////////////////////////////////////////////////////////
{
   //klshack = cbd;
    /*
    cout << "using sparse bits - maxring " << maxRingSize << endl;
    */
    int32_t i, j, k, curNode;
    int32_t listLen;
    int32_t numConnections, nlast;
    ChemPathMsg *listMsg, *pathMsg, *pathMsg2;
    SbBool done = FALSE;

    numFound = minSize = maxSize = 0;

    if (maxRingSize >= 0 && maxRingSize < 3) return;
    if (maxRing <= 0) return;

    // For each fragment...
    for (int32_t fragLoop = 0; fragLoop < numberOfFragments; fragLoop++) {
        if ((fragMaxRing = fragMaxRingList[fragLoop]) <= 0) continue;


        // Make fragAtch
        if (!makeFragAtch(fragLoop)) continue;

        // Initialize various arrays
        initialize();

        // Set which iteration we are on and we position within 
        // bitVecTable and gaussElimTable to put possible new entries.
        // Iteration 1 actually takes place in "initialize()".
        iterationCounter = 1;
        tablePos = 0;

        // Each iteration, rings of size 2*iterationCounter-1 and
        // 2*iterationCounter are found.  Therefore, we will stop when
        // fragMaxRing's have been found or when the iterationCounter has
        // reached (maxRingSize + 1)/2.

        int32_t iterationDone;
        if (maxRingSize > 0) {
            iterationDone = (maxRingSize + 1) / 2;
        }
        else {
            iterationDone = 0;
        } 

        done = FALSE;
        while (!done) {
            iterationCounter++;

            // "Send" all ChemPathMsg's in each tnode's sendBuffer
            for (curNode = 0; curNode < fragNumberOfNodes; curNode++) {

                // For each Tnode (nr) connected to the current Tnode (ns) 
                // by edge (esr)
                //     if pathMsg->lastNode is not equal to nr
                //         make a new pathMsg from pathMsg updating lastNode 
                //             and beep with ns and esr respectively
                //         store the new pathMsg in the receive buffer of nr
        
                listLen = tnodeList[curNode].sendBuffer->getLength();
                numConnections = fragAtch[curNode].getLength();
                for (i = listLen-1; i >= 0; i--) {
                    pathMsg = (ChemPathMsg *)tnodeList[curNode].sendBuffer->get(i);
                    nlast = pathMsg->lastNode;

                    for (j = 0; j < numConnections; j+=2) {
                        if (nlast != fragAtch[curNode][j]) {
            if (klshack) {
               int bondNo = (*fragBondList)[fragAtch[curNode][j+1]];
               int type1 = klshack->getBondType(bondNo);
               if (!(type1 == 2 || type1 == 5)) {
                  //cout << "skipping " << type1 << endl;
                  continue;
               }
            }
                            pathMsg2 = new ChemPathMsg(pathMsg);
                            pathMsg2->push(curNode, fragAtch[curNode][j+1]);
                            tnodeList[fragAtch[curNode][j]].receiveBuffer->append(pathMsg2);
                        }
                    }
                    tnodeList[curNode].sendBuffer->remove(i);
                    delete pathMsg;
                }
                // The tnodeList[curNode].sendBuffer->remove(i) should have left
                // the sendBuffers empty, but just in case
                assert(tnodeList[curNode].sendBuffer->getLength() == 0);
                tnodeList[curNode].sendBuffer->truncate(0);
            }

            // "Receive" the ChemPathMsg's
            for (curNode = 0; curNode < fragNumberOfNodes; curNode++) {

                // The next section deals with those pathMsg's which have 
                // the same firstEdge.  If they also have the same firstNode, 
                // then this is a direct-edge collision.  Keep the first 
                // pathMsg found with a given firstEdge and firstNode.  
                // This is our "merge()".  If 2 pathMsg's have the same 
                // firstEdge but different firstNodes, then this is an 
                // inverse-edge collision.
        
                for (i = tnodeList[curNode].receiveBuffer->getLength()-1;
                     i >= 0; i--) {
                    pathMsg = (ChemPathMsg *)tnodeList[curNode].receiveBuffer->get(i);
        
                    if ((listMsg = aList[pathMsg->firstEdge]) != NULL) {
                        if (listMsg->firstNode == pathMsg->firstNode) {
                            // This is direct edge collision so remove the 
                            // duplicate
                            delete pathMsg;
                        }
                        else {
                            // Look at the second pathMsg which could be 
                            // involved in an inverse-edge collision.  If 
                            // there is an entry in aList, then get rid of 
                            // the current pathMsg.  Otherwise, add the 
                            // current pathMsg to aList.
                            if ((listMsg =
                                aList[pathMsg->firstEdge + fragNumberOfEdges])
                                    != NULL) {
                                if (listMsg->firstNode == pathMsg->firstNode) {
                                    // This is direct edge collision so remove
                                    // the duplicate
                                    delete pathMsg;
                                }
                                else {
                                    // We should never get here.  This means 
                                    // that there are 3 nodes sharing the same
                                    // edge.
                                    fprintf(stderr,
"For %d: pathMsg->firstNode != any firstNodes in aList[firstEdge]\n", i);
                                }
                            }   
                            else {   
                                aList[pathMsg->firstEdge + fragNumberOfEdges] =
                                    pathMsg;
                            }
                        }
                    }
                    else {
                        aList[pathMsg->firstEdge] = pathMsg;
                    }
                }
                // After the next statement, the only pointers to pathMsgs are
                // in aList and bList
                tnodeList[curNode].receiveBuffer->truncate(0);
            
                // join() any pair of path-messages exhibiting an inverse-edge 
                // collision (firstEdges are equal but not firstNodes) and 
                // forward the resulting BEER (an odd-sized ring) to the ring 
                // selector.  In our implementation, if aList[firstEdge] and 
                // aList[firstEdge + fragNumberOfEdges] are both non-NULL, then 
                // there is an inverse-edge collision.
        
                for (i = 0; i < fragNumberOfEdges; i++) {
                    if ((aList[i] != NULL) && (aList[i+fragNumberOfEdges]
                        != NULL)) {
			// Optimization from  Jamie Doornbos	2001/07/15

                        //RWBitVec beer = aList[i]->beep |
		//		aList[i+fragNumberOfEdges]->beep;
                        RWBitVec beer = aList[i]->beep;
				beer |= aList[i+fragNumberOfEdges]->beep;

                        if (beer.sum() == (2*iterationCounter - 1)) {
                            if (ringSelect(beer)) {
                                // Added a ring of size 2*iterationCounter - 1
                                done = !(tablePos < fragMaxRing);
                                if (done) {
                                    // At this point, all pointers are in
                                    // aList.  The send and receive buffers
                                    // of the tnode should be empty and
                                    // nothing has been copied to the bList.
                                    // We need to delete all pathMsgs pointed to
                                    // by the aList entries.

                                    assert(tnodeList[curNode].sendBuffer->getLength() == 0);
                                    assert(tnodeList[curNode].receiveBuffer->getLength() == 0);
                                    goto DONE;
                                }
                            }
                        }
                    }
                }

                // join() any pair of path-messages exhibiting a node collision 
                // (firstNodes are equal but not the firstEdges) and forward 
                // the resulting BEER (an even-sized ring) to the ring selector.
                // First, put the entries in aList into the appropriate slots
                // in bList.

                for (i = 0; i < 2*fragNumberOfEdges; i++) {
                    if (aList[i] != NULL) {
                        bList[aList[i]->firstNode].append(aList[i]);
                        aList[i] = NULL;
                    }
                }

                // Now any pointers to pathMsgs are in bList and aList should 
                // be all NULLs

                for (i = 0; i < fragNumberOfNodes; i++) {
                    listLen = bList[i].getLength();
                    if (listLen > 1) {
                        for (j = 0; j < listLen-1; j++) {
                            pathMsg = (ChemPathMsg *)bList[i][j];
                            for (k = j+1; k < listLen; k++) {
                                pathMsg2 = (ChemPathMsg *)bList[i][k];
				// Optimization from  Jamie Doornbos	2001/07/15
                                 //RWBitVec beer = pathMsg->beep |
                                 //	 pathMsg2->beep;
                                 RWBitVec beer = pathMsg->beep;
                                 	 beer |= pathMsg2->beep;

                                if (beer.sum() == (2*iterationCounter)) {
                                    if (ringSelect(beer)) {
                                        // Added a ring of size 
                                        // 2*iterationCounter
                                        done = !(tablePos < fragMaxRing);
                                        if (done) {
                                            // At this point, all pointers are
                                            // in bList.  The receive buffer
                                            // of the tnode should be empty but
                                            // there may be something in the
                                            // send buffer.
                                            // We need to delete all pathMsgs 
                                            // pointed to by the bList entries
                                            // and anything in the tnode's send
                                            // send buffer.
                                            assert(tnodeList[curNode].receiveBuffer->getLength() == 0);
                                            goto DONE;
                                        }
                                    }
                                }
                            }
                        }
                        // At this point, all pairs of pathMsgs in bList[i]
                        // have been processed, so delete the the pathMsgs
                        for (j = 0; j < listLen; j++) {
                            pathMsg = (ChemPathMsg *)bList[i][j];
                            delete pathMsg;
                            bList[i][j] = NULL;
                        }
                    }
                    else if (listLen == 1) {
                        // store any noncollided path-messages in the send 
                        // buffer of this node
                        tnodeList[curNode].sendBuffer->append(bList[i][0]);
                        bList[i][0] = NULL;
                    }
                    bList[i].truncate(0);
// --> coin, memory pointers aren't freed
#ifdef __coin
										bList[i].truncate(0,TRUE);
#endif
// <-- coin, memory pointers aren't freed
                }
            }
            if (!done) done = (iterationCounter == iterationDone);
        }
DONE:
        cleanUpTNodeList();
        cleanUpB();
        cleanUpA();

        // Keep the bitVecTable but remove the gaussElimTable.  Also get rid
        // of the hash table.  All pathMsgs should have been deleted by now
        // so the only thing taking up space should be the bitVecTable.
        delete [] gaussElimTable;
        gaussElimTable = NULL;
        delete [] pivots;
        pivots = NULL;
        delete hash;
        hash = NULL;

        // Move the stuff in bitVecTable to finalTable
        RWBitVec *tmpTable = new RWBitVec[numFound+tablePos];

        for (i = 0; i < numFound; i++) {
            tmpTable[i].resize(numberOfBonds);
            tmpTable[i] = finalTable[i];
        }
        for (i = 0; i < tablePos; i++) {
            tmpTable[i+numFound].resize(numberOfBonds);
            vector<int> setbits;
            bitVecTable[i].getSetBits(setbits);
            int nSetBits = setbits.size();
            for (j = 0; j < nSetBits; j++) {
               tmpTable[i+numFound].setBit(fragBondList->get(setbits[j]));
            }
        }
        numFound += tablePos;
        if (finalTable != NULL) delete [] finalTable;
        finalTable = tmpTable;

        delete [] bitVecTable;
        bitVecTable = NULL;

        // Clean up fragAtch and fragBondList
        delete [] fragAtch;
        fragAtch = NULL;
        delete fragBondList;
        fragBondList = NULL;
    }

    // All done so sort the finalTable
    if (doSort) {
       qsort(finalTable, numFound, sizeof(RWBitVec), cmpBitVec);
    }

    // Find min and max
    minSize = 999999;
    maxSize = -999999;
    for (i = 0; i < numFound; i++) {
        int32_t theSum = finalTable[i].sum();
        if (theSum < minSize) minSize = theSum;
        if (theSum > maxSize) maxSize = theSum;
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Determines whether or not a ring is unique and should be added.
//    Returns whether or not it added a ring.  "done" is set if we are
//    at maxRing limit
//
// Use: private

SbBool
ChemSSSR::ringSelect(const RWBitVec& beer)
//
////////////////////////////////////////////////////////////////////////
{
    ChemHashEntry item;
    SbBool found;

    item.key = beer.computeHashValue();
    item.bitvec = beer;

    // If this is the first time adding to the table, do so and then return
    if (tablePos == 0) {
        hash->search(item, &found);
        bitVecTable[tablePos] = beer;
        gaussElimTable[tablePos] = beer;
        tablePos++;
        return TRUE;
    }

    // If the bitvec is already in the table, return
    hash->search(item, &found);
    if (found) {
        return FALSE;
    }

    // We have a possible new ring.  See if it is a linear combination of
    // previously found rings.  If not, increment the position for the next 
    // possible entry.  The previous search already added the entry to the
    // table.
    if (!linearCombination(beer)) {
        bitVecTable[tablePos] = beer;
        tablePos++;
        return TRUE;
    }
    else {
        return FALSE;
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Checks to see if the current ring is a linear combination of
//    previously found rings
//
// Use: private

SbBool
ChemSSSR::linearCombination(const RWBitVec& beer)
//
////////////////////////////////////////////////////////////////////////
{
    int32_t i, j;
    int32_t thePivot;
    bool t;

    gaussElimTable[tablePos] = beer;

    // Determine the pivot for the last entry in the gaussElimTable
    i = tablePos - 1;
    thePivot = i;
    if (!gaussElimTable[i].testBit(i)) {
        for (j = i+1; j < fragNumberOfEdges; j++) {
            if (gaussElimTable[i].testBit(j)) {
                thePivot = j;
                break;
            }
        }
    }

    // Pivot if necessary
    if (thePivot != i) {
        gaussElimTable[i].setBit(i);
        gaussElimTable[i].clearBit(thePivot);
    }

    // Keep track of the pivot
    pivots[i] = thePivot;

    // Apply the previous pivots and perform the elimination on the
    // current bitvec
    for (i = 0; i < tablePos; i++) {
        thePivot = pivots[i];
        //KLS t = gaussElimTable[tablePos](thePivot);
        t = gaussElimTable[tablePos][thePivot];
        if (thePivot != i) {
            gaussElimTable[tablePos].setBitTo(thePivot, gaussElimTable[tablePos][i]);
            gaussElimTable[tablePos].setBitTo(thePivot,gaussElimTable[tablePos][i]);

        }
        if (t) {
            gaussElimTable[tablePos] ^= gaussElimTable[i];
        }
    }

    if (gaussElimTable[tablePos].sum() == 0) return TRUE;
    return FALSE;

}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Used by the depth-first search in findFragments
//
// Use: static

static void
dfsVisit(int32_t which, int32_t &depth,
// --> why not const ??
				 const ChemIntList *atch,
// <-- why not const ??
    RWBitVec &visitedAtoms, RWBitVec &visitedBonds, 
    int32_t *tmpRingAtoms, RWBitVec *tmpRingBonds, RWBitVec *fragment,
// --> moiv doublebond error
	  const ChemDisplay* chemDisplay
// <-- moiv doublebond error
		)
//
////////////////////////////////////////////////////////////////////////
{
    visitedAtoms.setBit(which);
    fragment->setBit(which);
    
    int32_t i, j, k;
    int32_t iend = atch[which].getLength();
    int32_t kend;
    int32_t attachedAtom;
    int32_t bond;

    depth += 1;

    for (i = 0; i < iend; i += 2) {
        bond = atch[which][i+1];
// --> moiv doublebond error
if (chemDisplay) {
				 bond = chemDisplay->bondAttributesIndex[bond];
}
// <-- moiv doublebond error
        if (!visitedBonds.testBit(bond)) {
            visitedBonds.setBit(bond);
            attachedAtom = atch[which][i];
            tmpRingAtoms[depth] = attachedAtom;
            if (!visitedAtoms.testBit(attachedAtom)) {
// --> moiv doublebond error
                //dfsVisit(attachedAtom, depth, atch, visitedAtoms, visitedBonds,
                //    tmpRingAtoms, tmpRingBonds, fragment);
								dfsVisit(attachedAtom, depth, atch, visitedAtoms, visitedBonds,
                    tmpRingAtoms, tmpRingBonds, fragment, chemDisplay);
// <-- moiv doublebond error
            }
            else {
                // If we get here, then we have found a closure.  By getting
                // here, we have found a bond that has not been visited but
                // the atom attached to the current atom by this unvisited
                // bond has already been visited.  We have already set the
                // bond as visited.  We need to trace back to figure out
                // how we got here.


                
                // Set the bit for the bond that make the closure.
                kend = atch[attachedAtom].getLength();
                for (k = 0; k < kend; k+=2) {
                    if (atch[attachedAtom][k] == tmpRingAtoms[depth-1]) {
										// --> moiv doublebond error
                       if (chemDisplay) {
                          bond = chemDisplay->bondAttributesIndex[atch[attachedAtom][k+1]];
                       } else {
                          bond = atch[attachedAtom][k+1];
                       }
                       tmpRingBonds->setBit(bond);
										// <-- moiv doublebond error
                        break;
                    }
                }

                // Set the bits for the rest of the bonds involved in the
                // cycle.
                for (j = depth-1; j > 0; j--) {
                    if (tmpRingAtoms[j] == attachedAtom) break;
                    kend = atch[tmpRingAtoms[j]].getLength();
                    for (k = 0; k < kend; k+=2) {
                        if (atch[tmpRingAtoms[j]][k] == tmpRingAtoms[j-1]) {                            
												// --> moiv doublebond error
                           if (chemDisplay) {
                              bond = chemDisplay->bondAttributesIndex[atch[tmpRingAtoms[j]][k+1]];
                           } else {
                              bond = atch[tmpRingAtoms[j]][k+1];
                           }
												tmpRingBonds->setBit(bond);
												// <-- moiv doublebond error
                        break;
                        }
                    }
                }
            }
        }
    }
    depth -= 1;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Finds all ring bonds for a given "atch" list
//
// Use: private

void
ChemSSSR::findFragments()
//
////////////////////////////////////////////////////////////////////////
{
    int32_t i;

    if (ringBonds != NULL) {
        delete ringBonds;  
        ringBonds = NULL;
    }

    // Check to see if we have atoms and bonds
    if (numberOfAtoms == 0 || numberOfBonds == 0) return;

    // Check to see if we have an atch
    if (atch == NULL) {
        if (chemData == NULL) {
            return;
        }
        else {
            makeAtch();
        }
    }

    // Allocate the RWBitVec for the ringBonds
    ringBonds = new RWBitVec(numberOfBonds, FALSE);

    // Set up arrays to hold visited atoms and bonds
// --> moiv doublebond error
//		RWBitVec visitedAtoms(numberOfAtoms, FALSE);
//    RWBitVec visitedBonds(numberOfBonds, FALSE);
    RWBitVec *visitedAtoms = new RWBitVec(numberOfAtoms, FALSE);
    RWBitVec *visitedBonds =new RWBitVec(numberOfBonds, FALSE);
// <-- moiv doublebond error
	
    int32_t  *tmpRingAtoms = new int32_t[numberOfAtoms];
    int32_t  depth = 0;

    // Perform a depth-first search.  Each non-recursive invocation of
    // "dfsVisit" corresponds to a different fragment.  Keep track of which
    // atoms are in which fragment by using bitvecs.
    numberOfFragments = 0;

   for (i = 0; i < numberOfAtoms; i++) {
// --> moiv doublebond error
//        if (!visitedAtoms.testBit(i)) {
				if (!visitedAtoms->testBit(i)) {
// <-- moiv doublebond error
            numberOfFragments += 1;
            RWBitVec *fragment = new RWBitVec(numberOfAtoms, FALSE);
            fragmentList.append(fragment);						
						
            RWBitVec *tmpRingBonds = new RWBitVec(numberOfBonds, FALSE);
            ringBondList.append(tmpRingBonds);
            tmpRingAtoms[0] = i;						
// --> moiv doublebond error
            //dfsVisit(i, depth, atch, *visitedAtoms, *visitedBonds, 
            //   tmpRingAtoms, tmpRingBonds, fragment);
						dfsVisit(i, depth, atch, *visitedAtoms, *visitedBonds, 
                tmpRingAtoms, tmpRingBonds, fragment, chemDisplay);
// <-- moiv doublebond error
            (*ringBonds) ^= (*tmpRingBonds);
        }
    }
    delete [] tmpRingAtoms;

// --> moiv doublebond error
		delete visitedAtoms;
		delete visitedBonds;
// <-- moiv doublebond error
}
