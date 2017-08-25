/*
 * ChemDisplayPath.c++
 *
 *     Classes:  ChemDisplayPath
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
#include "inv/ChemKit/ChemDisplayPath.H"
#include "inv/ChemKit/ChemBaseData.H"
// --> lod
#include "inv/ChemKit/ChemLOD.H"
// <-- lod
// --> residue selection
#include "inv/ChemKit/ChemData.H"
// <-- residue selection
#include "inv/ChemKit/ChemDisplay.H"


SoType ChemDisplayPath::classTypeId;

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Gets the path's typeId
//
// Use: public
    
SoType
ChemDisplayPath::getTypeId() const
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
ChemDisplayPath::createInstance()
//  
////////////////////////////////////////////////////////////////////////
{
    return (void *)(new ChemDisplayPath);
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
ChemDisplayPath::readInstance(SoInput *in)
#else
SbBool
ChemDisplayPath::readInstance(SoInput *in, unsigned short /* flags not used */)
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

ChemDisplayPath::ChemDisplayPath()
//
////////////////////////////////////////////////////////////////////////
{
    atomIndex.deleteValues(0);
    bondIndex.deleteValues(0);
    atomLabelIndex.deleteValues(0);
    bondLabelIndex.deleteValues(0);
// --> selection
    residueIndex.deleteValues(0);
    residueLabelIndex.deleteValues(0);
    schematicIndex.deleteValues(0);
// <-- selection
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Copy constructor
//
// Use: public

ChemDisplayPath::ChemDisplayPath(const ChemDisplayPath &chemPath)
//
////////////////////////////////////////////////////////////////////////
{
    setSoPath(chemPath.path);
    if (chemPath.atomIndex.getNum() > 0) {
        atomIndex.setValues(0, chemPath.atomIndex.getNum(),
                            chemPath.atomIndex.getValues(0));
    }
    else {
        atomIndex.deleteValues(0);
    }
    if (chemPath.bondIndex.getNum() > 0) {
        bondIndex.setValues(0, chemPath.bondIndex.getNum(),
                            chemPath.bondIndex.getValues(0));
    }
    else {
        bondIndex.deleteValues(0);
    }
    if (chemPath.atomLabelIndex.getNum() > 0) {
        atomLabelIndex.setValues(0, chemPath.atomLabelIndex.getNum(),
                            chemPath.atomLabelIndex.getValues(0));
    }
    else {
        atomLabelIndex.deleteValues(0);
    }
    if (chemPath.bondLabelIndex.getNum() > 0) {
        bondLabelIndex.setValues(0, chemPath.bondLabelIndex.getNum(),
                            chemPath.bondLabelIndex.getValues(0));
    }
    else {
        bondLabelIndex.deleteValues(0);
    }
// --> residue selection
		if (chemPath.residueIndex.getNum() > 0)
		{
        residueIndex.setValues(0, chemPath.residueIndex.getNum(),
                            chemPath.residueIndex.getValues(0));
    }
    else
		{
        residueIndex.deleteValues(0);
		}

		if (chemPath.residueLabelIndex.getNum() > 0)
		{
        residueLabelIndex.setValues(0, chemPath.residueLabelIndex.getNum(),
                            chemPath.residueLabelIndex.getValues(0));
    }
    else
		{
        residueLabelIndex.deleteValues(0);
		}
// <-- residue selection
// --> schematic
		if (chemPath.schematicIndex.getNum() > 0)
		{
        schematicIndex.setValues(0, chemPath.schematicIndex.getNum(),
                            chemPath.schematicIndex.getValues(0));
    }
    else
		{
        schematicIndex.deleteValues(0);
		}
// <-- residue selection
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Destructor
//
// Use: public

ChemDisplayPath::~ChemDisplayPath()
//
////////////////////////////////////////////////////////////////////////
{
    atomIndex.deleteValues(0);
    bondIndex.deleteValues(0);
    atomLabelIndex.deleteValues(0);
    bondLabelIndex.deleteValues(0);
// --> selection
    residueIndex.deleteValues(0);
    residueLabelIndex.deleteValues(0);
    schematicIndex.deleteValues(0);
// <-- selection
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Create a new ChemDisplayPath which is a copy of "this".
//
// Use: public

ChemDisplayPath *
ChemDisplayPath::copy() const
//
////////////////////////////////////////////////////////////////////////
{
    ChemDisplayPath *newCopy = new ChemDisplayPath(*this);
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
ChemDisplayPath::setPath(SoPath *newPath,
    const MFVec2i *newAtomIndex, const MFVec2i *newBondIndex,
    const MFVec2i *newAtomLabelIndex, const MFVec2i *newBondLabelIndex
// --> residue selection
		,const MFVec2i *newResidueIndex, const MFVec2i *newResidueLabelIndex
// <-- residue selection
// --> schematic
		,const MFVec2i *newSchematicIndex
// <-- schematic
	)
//
////////////////////////////////////////////////////////////////////////
{
    ChemBaseData *chemData;
    ChemDisplay *chemDisplay;

    // The path needs to have been set so that we can see if
    // the indices are valid.
    if (newPath == NULL) return FALSE;

    // If all of the indices are NULL, then this is not a valid ChemDisplayPath.
// --> selection
//    if (newAtomIndex == NULL && newBondIndex == NULL &&
//        newAtomLabelIndex == NULL && newBondLabelIndex == NULL) return FALSE;
    if (newAtomIndex == NULL && newBondIndex == NULL &&
        newAtomLabelIndex == NULL && newBondLabelIndex == NULL &&
        newResidueIndex == NULL && newResidueLabelIndex == NULL &&
        newSchematicIndex == NULL) 
        return FALSE;
// <-- selection

    // See if the last node is a ChemDisplay node
    if (!newPath->getTail()->isOfType(ChemDisplay::getClassTypeId())) {
        return FALSE;
    }
    else {
        chemDisplay = (ChemDisplay *)newPath->getTail();
    }

    // See if the path contains a ChemBaseData-derived node
    SoSearchAction sa;
    sa.setType(ChemBaseData::getClassTypeId());
    sa.setInterest(SoSearchAction::LAST);
    sa.apply(newPath);
    if (sa.getPath() == NULL) return FALSE;
    chemData = (ChemBaseData *)sa.getPath()->getTail();
    int32_t numAtoms = chemData->getNumberOfAtoms();
    int32_t numBonds = chemData->getNumberOfBonds();
// --> residue selection
		int32_t numResidues = ((ChemData*)chemData)->getNumberOfResidues();
// <-- residue selection

    // Path OK so ref it and set its value in this
    newPath->ref();
    path = newPath;

    // For each of the non-NULL indices, check to see if the indices are
    // valid for the path.

    if ((newAtomIndex != NULL) && (newAtomIndex->getNum() > 0)) {
        // Check that the ChemDisplay has values in its atomIndex 
        // since we cannot set a path to an item if it does not exist
        if (chemDisplay->atomIndex.getNum() == 0) return FALSE;

        // If the value is for all atoms, then set that
        // value in the path index.
        if ((*newAtomIndex)[0] == SbVec2i(0, CHEM_DISPLAY_USE_REST_OF_ATOMS)) {
            atomIndex = *newAtomIndex;
        }
        else {
            if (!makeIndices(chemDisplay->atomIndex, *newAtomIndex,
                             atomIndex, CHEM_DISPLAY_USE_REST_OF_ATOMS,
                             numAtoms)) {
                return FALSE;
            }
        }
    }

    if ((newBondIndex != NULL) && (newBondIndex->getNum() > 0)) {
        // Check that the ChemDisplay has values in its bondIndex
        // since we cannot set a path to an item if it does not exist
        if (chemDisplay->bondIndex.getNum() == 0) return FALSE;

        // If the value is for all bonds, then set that
        // value in the path index.
        if ((*newBondIndex)[0] ==
            SbVec2i(0, CHEM_DISPLAY_USE_REST_OF_BONDS)) {
            bondIndex = *newBondIndex;
        }
        else {
            if (!makeIndices(chemDisplay->bondIndex, *newBondIndex,
                             bondIndex, CHEM_DISPLAY_USE_REST_OF_BONDS,
                             numBonds)) {
                return FALSE;
            }
        }
    }

    if ((newAtomLabelIndex != NULL) && (newAtomLabelIndex->getNum() > 0)) {
        // Check that the ChemDisplay has values in its atomLabelIndex
        // since we cannot set a path to an item if it does not exist
        if (chemDisplay->atomLabelIndex.getNum() == 0) return FALSE;

        // If the value is for all atom labels, then set that
        // value in the path index.
        if ((*newAtomLabelIndex)[0] ==
            SbVec2i(0, CHEM_DISPLAY_USE_REST_OF_ATOMS)) {
            atomLabelIndex = *newAtomLabelIndex;
        }
        else {
            if (!makeIndices(chemDisplay->atomLabelIndex, *newAtomLabelIndex,
                             atomLabelIndex, CHEM_DISPLAY_USE_REST_OF_ATOMS,
                             numAtoms)) {
                return FALSE;
            }
        }
    }

    if ((newBondLabelIndex != NULL) && (newBondLabelIndex->getNum() > 0)) {
        // Check that the ChemDisplay has values in its bondLabelIndex
        // since we cannot set a path to an item if it does not exist
        if (chemDisplay->bondLabelIndex.getNum() == 0) return FALSE;

        // If the value is for all bond labels, then set that
        // value in the path index.
        if ((*newBondLabelIndex)[0] ==
            SbVec2i(0, CHEM_DISPLAY_USE_REST_OF_BONDS)) {
            bondLabelIndex = *newBondLabelIndex;
        }
        else {
            if (!makeIndices(chemDisplay->bondLabelIndex, *newBondLabelIndex,
                             bondLabelIndex, CHEM_DISPLAY_USE_REST_OF_BONDS,
                             numBonds)) {
                return FALSE;
            }
        }
    }

// --> residue selection
		if ((newResidueIndex != NULL) && (newResidueIndex->getNum() > 0)) {
        // Check that the ChemDisplay has values in its residueIndex 
        // since we cannot set a path to an item if it does not exist
        if (chemDisplay->residueIndex.getNum() == 0) return FALSE;

        // If the value is for all residue, then set that
        // value in the path index.
        if ((*newResidueIndex)[0] == SbVec2i(0, CHEM_DISPLAY_USE_REST_OF_RESIDUES)) {
            residueIndex = *newResidueIndex;
        }
        else {
            if (!makeIndices(chemDisplay->residueIndex, *newResidueIndex,
                             residueIndex, CHEM_DISPLAY_USE_REST_OF_RESIDUES,
                             numResidues)) {
                return FALSE;
            }
        }
    }

		if ((newResidueLabelIndex != NULL) && (newResidueLabelIndex->getNum() > 0)) {
        // Check that the ChemDisplay has values in its residueLabelIndex 
        // since we cannot set a path to an item if it does not exist
        if (chemDisplay->residueLabelIndex.getNum() == 0) return FALSE;

        // If the value is for all residue labels, then set that
        // value in the path index.
        if ((*newResidueLabelIndex)[0] == SbVec2i(0, CHEM_DISPLAY_USE_REST_OF_RESIDUES)) {
            residueLabelIndex = *newResidueLabelIndex;
        }
        else {
            if (!makeIndices(chemDisplay->residueLabelIndex, *newResidueLabelIndex,
                             residueLabelIndex, CHEM_DISPLAY_USE_REST_OF_RESIDUES,
                             numResidues)) {
                return FALSE;
            }
        }
    }
// <-- residue selection
// --> schematic
    int32_t numSchematics = chemDisplay->getNumberOfSchematics();
    if ((newSchematicIndex != NULL) && (newSchematicIndex ->getNum() > 0)) {
        // Check that the ChemDisplay has values in its schematicIndex 
        // since we cannot set a path to an item if it does not exist
        if (chemDisplay->schematicIndex.getNum() == 0) return FALSE;

        // If the value is for all residue, then set that
        // value in the path index.
        if ((*newSchematicIndex)[0] == SbVec2i(0, CHEM_DISPLAY_USE_REST_OF_SCHEMATICS)) {
            schematicIndex = *newSchematicIndex;
        }
        else {
            if (!makeIndices(chemDisplay->schematicIndex, *newSchematicIndex,
                             schematicIndex, CHEM_DISPLAY_USE_REST_OF_SCHEMATICS,
                             numSchematics)) {
                return FALSE;
            }
        }
    }
// <-- schematic

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
ChemDisplayPath::getAtomIndex() const
{
    return atomIndex;
}

const MFVec2i &
ChemDisplayPath::getBondIndex() const
{
    return bondIndex;
}

const MFVec2i &
ChemDisplayPath::getAtomLabelIndex() const
{
    return atomLabelIndex;
}

const MFVec2i &
ChemDisplayPath::getBondLabelIndex() const
{
    return bondLabelIndex;
}

// --> residue selection
const MFVec2i & ChemDisplayPath::getResidueIndex() const
{
    return residueIndex;
}

const MFVec2i & ChemDisplayPath::getResidueLabelIndex() const
{
    return residueLabelIndex;
}
// <-- residue selection
// --> schematic
const MFVec2i & ChemDisplayPath::getSchematicIndex() const
{
    return schematicIndex;
}
// <-- schematic

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Comparison operator tests path equality. Two paths are equal if
//    their chains are the same length and all node pointers in them
//    are the same.  And the indices are the same.
//
// Use: public

int
ChemDisplayPath::operator ==(const ChemPath &chemPath) const
//
////////////////////////////////////////////////////////////////////////
{
    return equals(chemPath);
}

SbBool
ChemDisplayPath::equals(const ChemPath &chemPath) const
{
    if (!chemPath.isOfType(ChemDisplayPath::getClassTypeId())) {
        return FALSE;
    }

    if (!ChemPath::equals(chemPath)) {
        return FALSE;
    }

    const ChemDisplayPath *cp = (const ChemDisplayPath *)&chemPath;

    if ((atomIndex == cp->atomIndex) &&
        (bondIndex == cp->bondIndex) &&
        (atomLabelIndex == cp->atomLabelIndex) &&
        (bondLabelIndex == cp->bondLabelIndex)
// --> residue selection
				&& (residueIndex == cp->residueIndex) &&
				(residueLabelIndex == cp->residueLabelIndex)
// <-- residue selection
// --> schematic
        && (schematicIndex == cp->schematicIndex)
// <-- schematic
		)
		{
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
ChemDisplayPath::makeIndices(const MFVec2i &chemDisplayIndex, 
    const MFVec2i &currentIndex, MFVec2i &newIndex,
    int32_t end_indicator, int32_t numberOfItems)
//
////////////////////////////////////////////////////////////////////////
{
    ChemIntList chemDisplayList, currentList;
    int32_t loop, numLoops, start, end, item;
    int32_t lastStart, lastIncr, count;
    int32_t index;

    // Make a list of all items in ChemDisplay
    numLoops = chemDisplayIndex.getNum();
    for (loop = 0; loop < numLoops; loop++) {
        chemDisplayIndex[loop].getValue(start, end);
        if (end == end_indicator) {
            end = numberOfItems;
        }
        else {
            end += start;
        }
        for (item = start; item < end; item++) {
            chemDisplayList.append(item);
        }
    }

    // Sort in preparation for the searches below.
    chemDisplayList.sort();

    // Now loop over the currentIndex values and see if they are in
    // chemDisplayList
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
            if ((index = chemDisplayList.search(&item)) == -1) {
                return FALSE;
            }
            else {
                currentList.append(chemDisplayList[index]);
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
