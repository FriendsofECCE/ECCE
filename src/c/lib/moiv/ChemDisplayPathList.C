/*
 * ChemDisplayPathList.c++
 *
 *     Classes:  ChemDisplayPathList
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

#include <assert.h>

#include "inv/SoPath.H"

#include "inv/actions/SoSearchAction.H"

#include "inv/ChemKit/ChemIntList.H"
#include "inv/ChemKit/ChemBaseData.H"
// --> lod
#include "inv/ChemKit/ChemLOD.H"
// <-- lod
// --> residues
#include "inv/ChemKit/ChemData.H"
// <-- residues
#include "inv/ChemKit/ChemDisplay.H"
#include "inv/ChemKit/ChemDisplayPath.H"
#include "inv/ChemKit/ChemDisplayPathList.H"


#define END_OF_LIST 9999999

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Merges a chemPath with the list.  This assumes that the
//    chemPath has already been checked for validity.  If the SoPath
//    is already in the pathList, then the indices are merged otherwise
//    the chemPath is added to the end of the list.  "toggle"
//    determines whether the indices are always added or whether the
//    indices are toggled on/off.
//
// Use: public

SbBool
ChemDisplayPathList::mergePath(ChemDisplayPath *newPath, SbBool toggle)
//
////////////////////////////////////////////////////////////////////////
{
    SbBool retVal = FALSE;
    int32_t index = findSoPath(newPath);
    if (index == -1) {
        SbPList::append((void *) newPath);
        newPath->ref();
        return TRUE;
    }
    else {
        ChemDisplayPath *listPath = (*this)[index];
        SoFullPath *theFullPath = (SoFullPath *)listPath->path;

        SoSearchAction sa;
        sa.setType(ChemBaseData::getClassTypeId());
        sa.setInterest(SoSearchAction::LAST);
        sa.apply(theFullPath);
        theFullPath = (SoFullPath *)sa.getPath();
        ChemBaseData *chemData = (ChemBaseData *)(theFullPath->getTail());
        assert(chemData->isOfType(ChemBaseData::getClassTypeId()));
        retVal = mergePaths(listPath, newPath, chemData, toggle);
    }
    return retVal;
}

////////////////////////////////////////////////////////////////////////
// 
// Description:
//    Delete all ChemDisplayPaths on the list from one with given index on,
//    inclusive.
//
// Use: public
 
void
ChemDisplayPathList::truncate(int32_t start)
//
////////////////////////////////////////////////////////////////////////
{
    int         i;

    for (i = start; i < getLength(); i++) {
        if ((*this)[i] != NULL) {
            (*this)[i]->unref();
        }
    }
 
    SbPList::truncate(start);
// --> coin,  memory pointers aren't freed
#ifdef __coin
		SbPList::truncate(start,TRUE);
#endif
// <-- coin,  memory pointers aren't freed
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Remove the ChemDisplayPath from the ChemDisplayPathList.
//
// Use: public

void
ChemDisplayPathList::removePath(ChemDisplayPath *chemPath)
//
////////////////////////////////////////////////////////////////////////
{
    int32_t index = findSoPath(chemPath);
    if (index == -1) return;

    ChemDisplayPath *listPath = (*this)[index];
    SoFullPath *theFullPath = (SoFullPath *)listPath->path;

    SoSearchAction sa;
    sa.setType(ChemBaseData::getClassTypeId());
    sa.setInterest(SoSearchAction::LAST);
    sa.apply(theFullPath);
    theFullPath = (SoFullPath *)sa.getPath();
    ChemBaseData *chemData = (ChemBaseData *)(theFullPath->getTail());
    assert(chemData->isOfType(ChemBaseData::getClassTypeId()));

    removePath(listPath, chemPath, chemData);
    if (listPath->atomIndex.getNum() == 0 &&
        listPath->bondIndex.getNum() == 0 &&
        listPath->atomLabelIndex.getNum() == 0 &&
        listPath->bondLabelIndex.getNum() == 0
// --> residues
				&& listPath->residueIndex.getNum() == 0
				&& listPath->residueLabelIndex.getNum() == 0
        && listPath->schematicIndex.getNum() == 0
// <-- residues
				) {

        remove(index);
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Remove the ChemDisplayPath with the given index from the 
//    ChemDisplayPathList.
//
// Use: public

void
ChemDisplayPathList::remove(int32_t which)
//
////////////////////////////////////////////////////////////////////////
{
    if (which < 0) return;
    if (which > (getLength() - 1)) return;
    if ((*this)[which]) {
        (*this)[which]->unref();
        SbPList::remove(which);
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Finds the SoPath part of the ChemDisplayPath.  It returns -1 if 
//    not found in the pathList.
//
// Use: public

int32_t
ChemDisplayPathList::findSoPath(const ChemPath *chemPath)
//
////////////////////////////////////////////////////////////////////////
{
    const SoPath *path = chemPath->path;
    return findSoPath(*path);
}

int32_t
ChemDisplayPathList::findSoPath(const SoPath &path)
{
    int32_t i;
    SoPath  *testPath;
    for(i = 0; i < getLength(); i++) {
        testPath = (*this)[i]->path;
        if (*testPath == path)
            return i;
    }
    return -1;  // not found
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Copy a list, keeping all reference counts correct.
//
// Use: public

void
ChemDisplayPathList::copy(const ChemDisplayPathList &chemPathList)
//
////////////////////////////////////////////////////////////////////////
{
    int i;
    truncate(0);

    for (i = 0; i < chemPathList.getLength(); i++) {
        if (chemPathList[i]) {
            chemPathList[i]->ref();
        }
    }
    SbPList::copy(chemPathList);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Merges the indices of the 2 ChemDisplayPaths.
//
// Use: private
//
////////////////////////////////////////////////////////////////////////

SbBool
ChemDisplayPathList::mergePaths(ChemDisplayPath *listPath,
    ChemDisplayPath *newPath, ChemBaseData *chemData, SbBool toggle)
{
    SbBool changed = FALSE;
    int32_t numberOfAtoms = chemData->getNumberOfAtoms();
    int32_t numberOfBonds = chemData->getNumberOfBonds();

    if (mergeIndices(listPath->atomIndex, newPath->atomIndex,
            CHEM_DISPLAY_USE_REST_OF_ATOMS, numberOfAtoms, toggle))
        changed = TRUE;

    if (mergeIndices(listPath->bondIndex, newPath->bondIndex,
            CHEM_DISPLAY_USE_REST_OF_BONDS, numberOfBonds, toggle))
        changed = TRUE;

    if (mergeIndices(listPath->atomLabelIndex, newPath->atomLabelIndex,
            CHEM_DISPLAY_USE_REST_OF_ATOMS, numberOfAtoms, toggle))
        changed = TRUE;

    if (mergeIndices(listPath->bondLabelIndex, newPath->bondLabelIndex,
            CHEM_DISPLAY_USE_REST_OF_BONDS, numberOfBonds, toggle))
        changed = TRUE;
// --> residues
		int32_t numberOfResidues = ((ChemData*)chemData)->getNumberOfResidues();

		if (mergeIndices(listPath->residueIndex, newPath->residueIndex,
            CHEM_DISPLAY_USE_REST_OF_RESIDUES, numberOfResidues, toggle))
        changed = TRUE;
		if (mergeIndices(listPath->residueLabelIndex, newPath->residueLabelIndex,
            CHEM_DISPLAY_USE_REST_OF_RESIDUES, numberOfResidues, toggle))
        changed = TRUE;

     assert(listPath->getSoPath()->getTail()->isOfType(ChemDisplay::getClassTypeId()));
     ChemDisplay* chemDisplay = (ChemDisplay *)(listPath->getSoPath()->getTail());
     assert(chemDisplay != NULL);

     int32_t numberOfSchematics = chemDisplay->getNumberOfSchematics();
     if (mergeIndices(listPath->schematicIndex, newPath->schematicIndex,
            CHEM_DISPLAY_USE_REST_OF_SCHEMATICS, numberOfSchematics, toggle))
        changed = TRUE;
// <-- residues

    return changed;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Merges the indices of newIndex into listIndex.
//
// Use: private

SbBool
ChemDisplayPathList::mergeIndices(MFVec2i &listIndex, MFVec2i &newIndex,
    int32_t end_indicator, int32_t numberOfItems, SbBool toggle)
//
////////////////////////////////////////////////////////////////////////
{
    SbBool changed = FALSE;
    ChemIntList theList(numberOfItems);
    int32_t loop, numLoops, start, end, item;
    int32_t lastStart, lastIncr, count;
    int32_t index;

    // If nothing to merge, then return.
    if (newIndex.getNum() == 0) {
        return FALSE;
    }

    // Make a list of all the entries in listIndex.
    if ((numLoops = listIndex.getNum()) > 0) {
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
    }

    // Sort the list before performing the searches.
    theList.sort();
    int32_t listStart = 0;
    int32_t listEnd = theList.getLength() - 1;

    // We already checked that newIndex has values, so loop over them and
    // see if they are in the list.  If not, add them.  If so, then leave
    // alone or remove depending on the value of toggle.
    numLoops = newIndex.getNum();
    for (loop = 0; loop < numLoops; loop++) {
        newIndex[loop].getValue(start, end);
        if (end == end_indicator) {
            end = numberOfItems;
        }
        else {
            end += start;
        }
        for (item = start; item < end; item++) {
            if ((index = theList.search(&item, listStart, listEnd))
                    == -1) {
                theList.append(item);
                changed = TRUE;
            }
            else if (toggle) {
                theList[index] = END_OF_LIST;
                listStart = index + 1;
                changed = TRUE;
            }
        }
    }

    // Remake listIndex using contents of theList.
    listIndex.deleteValues(0);

    // Sort theList.  All END_OF_LIST's should be at the end.  Find the first
    // of these and truncate the list.
    theList.sort();
    if ((index = theList.find(END_OF_LIST)) != -1) theList.truncate(index);

    // If nothing in the remaining list, then return.
    if ((end = theList.getLength()) == 0) return changed;

    // Fill out listIndex with values from theList.
    lastStart = -1;
    lastIncr = 0;
    count = 0;
    for (loop = 0; loop < end; loop++) {
        item = theList[loop];
        if (lastStart == -1) {
            lastStart = item;
            lastIncr = 1;
        }
        else if (item == (lastStart + lastIncr)) {
            lastIncr++;
        }
        else {
            listIndex.set1Value(count++, SbVec2i(lastStart, lastIncr));
            lastStart = item;
            lastIncr = 1;
        }
    }
    if (lastStart != -1) {
        listIndex.set1Value(count, SbVec2i(lastStart, lastIncr));
    }
    return changed;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Removes the indices of chemPath from listPath.
//
// Use: private

void
ChemDisplayPathList::removePath(ChemDisplayPath *listPath,
    ChemDisplayPath *newPath, ChemBaseData *chemData)
//
////////////////////////////////////////////////////////////////////////
{
    int32_t numberOfAtoms = chemData->getNumberOfAtoms();
    int32_t numberOfBonds = chemData->getNumberOfBonds();

    removeIndices(listPath->atomIndex, newPath->atomIndex,
        CHEM_DISPLAY_USE_REST_OF_ATOMS, numberOfAtoms);

    removeIndices(listPath->bondIndex, newPath->bondIndex,
        CHEM_DISPLAY_USE_REST_OF_BONDS, numberOfBonds);

    removeIndices(listPath->atomLabelIndex, newPath->atomLabelIndex,
        CHEM_DISPLAY_USE_REST_OF_ATOMS, numberOfAtoms);

    removeIndices(listPath->bondLabelIndex, newPath->bondLabelIndex,
        CHEM_DISPLAY_USE_REST_OF_BONDS, numberOfBonds);
// --> residues
		int32_t numberOfResidues = ((ChemData*)chemData)->getNumberOfResidues();

		removeIndices(listPath->residueIndex, newPath->residueIndex,
            CHEM_DISPLAY_USE_REST_OF_RESIDUES, numberOfResidues);
        
		removeIndices(listPath->residueLabelIndex, newPath->residueLabelIndex,
            CHEM_DISPLAY_USE_REST_OF_RESIDUES, numberOfResidues);

     assert(listPath->getSoPath()->getTail()->isOfType(ChemDisplay::getClassTypeId()));
     ChemDisplay* chemDisplay = (ChemDisplay *)(listPath->getSoPath()->getTail());
     assert(chemDisplay != NULL);

     int32_t numberOfSchematics = chemDisplay->getNumberOfSchematics();
     removeIndices(listPath->schematicIndex, newPath->schematicIndex,
            CHEM_DISPLAY_USE_REST_OF_SCHEMATICS, numberOfSchematics);
// <-- residues

}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Removes the indices of newIndex from listIndex.
//
// Use: private

void
ChemDisplayPathList::removeIndices(MFVec2i &listIndex, MFVec2i &newIndex,
    int32_t end_indicator, int32_t numberOfItems)
//
////////////////////////////////////////////////////////////////////////
{
    ChemIntList theList(numberOfItems);
    int32_t loop, numLoops, start, end, item;
    int32_t lastStart, lastIncr, count;
    int32_t index;

    // Make a list of all the unique entries in listIndex and newIndex.
    if ((numLoops = listIndex.getNum()) > 0) {
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

        // Sort the list before performing the searches.
        theList.sort();
        int32_t listStart = 0;
        int32_t listEnd = theList.getLength() - 1;

        if ((numLoops = newIndex.getNum()) > 0) {
            for (loop = 0; loop < numLoops; loop++) {
                newIndex[loop].getValue(start, end);
                if (end == end_indicator) {
                    end = numberOfItems;
                }
                else {
                    end += start;
                }
                for (item = start; item < end; item++) {
                    if ((index = theList.search(&item, listStart, listEnd))
                             != -1) {
                        theList[index] = END_OF_LIST;
                        listStart = index + 1;
                    }
                }
            }
        }

        // Sort theList.  All END_OF_LIST's should be at the end.
        // Find the first of these and truncate the list.
        theList.sort();
        if ((index = theList.find(END_OF_LIST)) != -1) theList.truncate(index);

        // Remake listIndex using contents of theList.
        lastStart = -1;
        lastIncr = 0;
        count = 0;
        end = theList.getLength();
        listIndex.deleteValues(0);
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
                listIndex.set1Value(count++, SbVec2i(lastStart, lastIncr));
                lastStart = item;
                lastIncr = 1;
            }
        }
        if (lastStart != -1) {
            listIndex.set1Value(count, SbVec2i(lastStart, lastIncr));
        }
    }
}
