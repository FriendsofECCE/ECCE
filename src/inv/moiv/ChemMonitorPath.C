/*
 * ChemMonitorPath.c++
 *
 *     Classes:  ChemMonitorPath
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
#include "inv/ChemKit/ChemMonitorPath.H"
#include "inv/ChemKit/ChemBaseData.H"
#include "inv/ChemKit/ChemMonitor.H"

SoType ChemMonitorPath::classTypeId;

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Gets the path's typeId
//
// Use: public
    
SoType
ChemMonitorPath::getTypeId() const
//
////////////////////////////////////////////////////////////////////////
{
    return classTypeId;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    This is called by SoType::createInstance to create a new
//    instance of a path.
//
// Use: private
            
void *
ChemMonitorPath::createInstance()
//  
////////////////////////////////////////////////////////////////////////
{
    return (void *)(new ChemMonitorPath);
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
ChemMonitorPath::readInstance(SoInput *in)
#else
SbBool
ChemMonitorPath::readInstance(SoInput *in, unsigned short /* flags not used */)
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
// Use: public

ChemMonitorPath::ChemMonitorPath()
//
////////////////////////////////////////////////////////////////////////
{
    distanceIndex.deleteValues(0);
    angleIndex.deleteValues(0);
    torsionalIndex.deleteValues(0);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Copy constructor
//
// Use: public

ChemMonitorPath::ChemMonitorPath(const ChemMonitorPath &chemPath)
//
////////////////////////////////////////////////////////////////////////
{
    setSoPath(chemPath.path);
    if (chemPath.distanceIndex.getNum() > 0) {
        distanceIndex.setValues(0, chemPath.distanceIndex.getNum(),
                                chemPath.distanceIndex.getValues(0));
    }
    else {
        distanceIndex.deleteValues(0);
    }
    if (chemPath.angleIndex.getNum() > 0) {
        angleIndex.setValues(0, chemPath.angleIndex.getNum(),
                             chemPath.angleIndex.getValues(0));
    }
    else {
        angleIndex.deleteValues(0);
    }
    if (chemPath.torsionalIndex.getNum() > 0) {
        torsionalIndex.setValues(0, chemPath.torsionalIndex.getNum(),
                                 chemPath.torsionalIndex.getValues(0));
    }
    else {
        torsionalIndex.deleteValues(0);
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Destructor
//
// Use: public

ChemMonitorPath::~ChemMonitorPath()
//
////////////////////////////////////////////////////////////////////////
{
    distanceIndex.deleteValues(0);
    angleIndex.deleteValues(0);
    torsionalIndex.deleteValues(0);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Create a new ChemMonitorPath which is a copy of "this".
//
// Use: public

ChemMonitorPath *
ChemMonitorPath::copy() const
//
////////////////////////////////////////////////////////////////////////
{
    ChemMonitorPath *newCopy = new ChemMonitorPath(*this);
    return newCopy;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Set the path and indices.  Check to see if the path and indices are
//    valid.
//
// Use: public

SbBool
ChemMonitorPath::setPath(SoPath *newPath,
    const MFVec2i *newDistanceIndex, const MFVec2i *newAngleIndex,
    const MFVec2i *newTorsionalIndex)
//
////////////////////////////////////////////////////////////////////////
{
    ChemMonitor *chemMonitor;

    // The path needs to have been set so that we can see if
    // the indices are valid.
    if (newPath == NULL) return FALSE;

    // If all of the indices are NULL, then this is not a valid ChemMonitorPath.
    if (newDistanceIndex == NULL && newAngleIndex == NULL &&
        newTorsionalIndex == NULL) return FALSE;

    // See if the last node is a ChemMonitor node
    if (!newPath->getTail()->isOfType(ChemMonitor::getClassTypeId())) {
        return FALSE;
    }
    else {
        chemMonitor = (ChemMonitor *)newPath->getTail();
    }

    // Path OK so ref it and set its value in this
    newPath->ref();
    path = newPath;

    // For each of the non-NULL indices, check to see if the indices are
    // valid for the path.

    int32_t numIndices;
    if ((newDistanceIndex != NULL) && (newDistanceIndex->getNum() > 0)) {
        // Check that the ChemMonitor has values in its distanceIndex 
        // since we cannot set a path to an item if it does not exist

        numIndices = chemMonitor->distance.getNum();
        if (numIndices == 0) return FALSE;

        // If the value is for all distance monitors, then set that
        // value in the path index.
        if ((*newDistanceIndex)[0] == SbVec2i(0, -1)) {
            distanceIndex = *newDistanceIndex;
        }
        else {
            if (!makeIndices(*newDistanceIndex, distanceIndex,
                             -1, numIndices)) {
                return FALSE;
            }
        }
    }

    if ((newAngleIndex != NULL) && (newAngleIndex->getNum() > 0)) {
        // Check that the ChemMonitor has values in its angleIndex
        // since we cannot set a path to an item if it does not exist

        numIndices = chemMonitor->angle.getNum();
        if (numIndices == 0) return FALSE;

        // If the value is for all bonds, then set that
        // value in the path index.
        if ((*newAngleIndex)[0] == SbVec2i(0, -1)) {
            angleIndex = *newAngleIndex;
        }
        else {
            if (!makeIndices(*newAngleIndex, angleIndex, -1, numIndices)) {
                return FALSE;
            }
        }
    }

    if ((newTorsionalIndex != NULL) && (newTorsionalIndex->getNum() > 0)) {
        // Check that the ChemMonitor has values in its torsionalIndex
        // since we cannot set a path to an item if it does not exist

        numIndices = chemMonitor->torsional.getNum();
        if (numIndices == 0) return FALSE;

        // If the value is for all atom labels, then set that
        // value in the path index.
        if ((*newTorsionalIndex)[0] == SbVec2i(0, -1)) {
            torsionalIndex = *newTorsionalIndex;
        }
        else {
            if (!makeIndices(*newTorsionalIndex, torsionalIndex,
                             -1, numIndices)) {
                return FALSE;
            }
        }
    }

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

const MFVec2i &
ChemMonitorPath::getDistanceIndex() const
{
    return distanceIndex;
}

const MFVec2i &
ChemMonitorPath::getAngleIndex() const
{
    return angleIndex;
}

const MFVec2i &
ChemMonitorPath::getTorsionalIndex() const
{
    return torsionalIndex;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Comparison operator tests path equality. Two paths are equal if
//    their chains are the same length and all node pointers in them
//    are the same.  And the indices are the same.
//
// Use: public

int
ChemMonitorPath::operator ==(const ChemPath &chemPath) const
//
////////////////////////////////////////////////////////////////////////
{
    return equals(chemPath);
}

SbBool
ChemMonitorPath::equals(const ChemPath &chemPath) const
{
    if (!chemPath.isOfType(ChemMonitorPath::getClassTypeId())) {
        return FALSE;
    }

    if (!ChemPath::equals(chemPath)) {
        return FALSE;
    }

    const ChemMonitorPath *cp = (const ChemMonitorPath *)&chemPath;

    if ((distanceIndex == cp->distanceIndex) &&
        (angleIndex == cp->angleIndex) &&
        (torsionalIndex == cp->torsionalIndex)) {
        return TRUE;
    }
    return FALSE;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Checks to see if the indices exist in the path and if so updates the
//    internal index with what is passed in.
//
// Use: private

SbBool
ChemMonitorPath::makeIndices(const MFVec2i &currentIndex, MFVec2i &newIndex,
    int32_t end_indicator, int32_t numberOfItems)
//
////////////////////////////////////////////////////////////////////////
{
    ChemIntList currentList;
    int32_t loop, numLoops, start, end, item;
    int32_t lastStart, lastIncr, count;

    // Loop over the currentIndex values and see if they are in
    // chemMonitorList

    currentList.truncate(0);
    numLoops = currentIndex.getNum();
    for (loop = 0; loop < numLoops; loop++) {
        currentIndex[loop].getValue(start, end);
        if (end == end_indicator) {
            end = numberOfItems;
        }
        else {
            end += start;
        }
        for (item = start; item < end; item++) {
            if (item >= numberOfItems) {
                return FALSE;
            }
            else {
                currentList.append(item);
            }
        }
    }
    // Now make the newIndex for this
    newIndex.deleteValues(0);

    // Sort the currentList
    currentList.sort();
    end = currentList.getLength();

    // Fill out newIndex with values from currentList
    lastStart = -1;
    lastIncr = 0;
    count = 0;
    for (loop = 0; loop < end; loop++) {
        item = currentList[loop];
        if (lastStart == -1) {
            lastStart = item;
            lastIncr = 1;
        }
        else if (item == (lastStart + lastIncr)) {
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
    return TRUE;
}
