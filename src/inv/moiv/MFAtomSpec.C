/*
 * MFAtomSpec.c++
 *
 *     Classes:  MFAtomSpec
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

#include "inv/ChemKit/MFAtomSpec.H"
#include "inv/ChemKit/ChemBaseData.H"
// --> lod
#include "inv/ChemKit/ChemLOD.H"
// <-- lod
#include "inv/ChemKit/ChemDisplay.H"

//////////////////////////////////////////////////////////////////////////////
//
// MFAtomSpec class
//
//////////////////////////////////////////////////////////////////////////////

// --> now using correct macros
SO_MFIELD_SOURCE(MFAtomSpec, SbAtomSpec, const SbAtomSpec &);
// // Use standard definitions of all required methods, but we have to
// // redefine all the rest to keep track of references and auditors.

// //SO_MFIELD_REQUIRED_SOURCE(MFAtomSpec);
//
///*
//	Jamie Doornbos	2001/07/27
//	Daniel Soto		2002/02/07
//*/
//#ifdef __coin
//SO_MFIELD_VALUE_SOURCE(MFAtomSpec, SbAtomSpec, const SbAtomSpec &);
//#endif
//
//////////////////////////////////////////////////////////////////////////
////
//// Description:
////    Constructor
////
//// Use: public
//MFAtomSpec::MFAtomSpec()
////
//////////////////////////////////////////////////////////////////////////
//{
//    values = NULL;
//}

//////////////////////////////////////////////////////////////////////////
////
//// Description:
////    Destructor
////
//// Use: public
//
//MFAtomSpec::~MFAtomSpec()
////
//////////////////////////////////////////////////////////////////////////
//{
//    deleteAllValues();
//}

///*
//	Daniel Soto		2002/02/07
//	Alrady defined this functions in SO_MFIELD_VALUE_SOURCE() macro.
//*/
///*
//#ifndef __coin
//
//////////////////////////////////////////////////////////////////////////
////
//// These all override the definitions in SO_MFIELD_VALUE_SOURCE() to
//// keep track of references and auditors.
////
//////////////////////////////////////////////////////////////////////////
//
//int
//MFAtomSpec::find(const SbAtomSpec &targetValue, SbBool addIfNotFound)
//{
//    int i, num = getNum();
//
//    for (i = 0; i < num; i++) {
//        if (values[i] == targetValue) {
//            return i;
//        }
//    }
//
//    if (addIfNotFound) {
//        set1Value(num, targetValue);
//    }
//
//    return -1;
//}
//
//void
//MFAtomSpec::setValues(int start, int num, const SbAtomSpec *newValues)
//{
//    int newNum = start + num, i;
//
//    if (newNum > getNum()) {
//        makeRoom(newNum);
//    }
//
//    for (i = 0; i < num; i++) {
//        setVal(start + i, newValues[i]);
//    }
//    valueChanged();
//}
//
//void
//MFAtomSpec::set1Value(int index, const SbAtomSpec &newValue)
//{
//    if (index >= getNum()) {
//        makeRoom(index + 1);
//    }
//    setVal(index, newValue);
//    valueChanged();
//}
//
//void
//MFAtomSpec::setValue(const SbAtomSpec &newValue)
//{
//    makeRoom(1);
//   setVal(0, newValue);
//    valueChanged();
//}
//
//int
//MFAtomSpec::operator ==(const MFAtomSpec &f) const
//{
//    int i, num = getNum();
//    const SbAtomSpec *myVals, *itsVals;
//
//    if (num != f.getNum()) {
//        return FALSE;
//    }
//
//    myVals  = getValues(0);
//    itsVals = f.getValues(0);
//
//    for (i = 0; i < num; i++) {
//        if (! (myVals[i] == itsVals[i])) {
//            return FALSE;
//        }
//    }
//
//    return TRUE;
//}
//
//void
//MFAtomSpec::deleteAllValues()
//{
//    allocValues(0);
//}
//
//void
//MFAtomSpec::copyValue(int to, int from)
//{
//    if (to != from) {
//        setVal(to, values[from]);
//    }
//}
//
//#endif  // __coin
//
//////////////////////////////////////////////////////////////////////////
////
//// This overrides the definition in SO_MFIELD_ALLOC_SOURCE() to
//// keep track of references and auditors.
////
//////////////////////////////////////////////////////////////////////////
//
//void
//MFAtomSpec::allocValues(int newNum)
//{
//    int i;
//
//    if (values == NULL) {
//        if (newNum > 0) {
//            values = new SbAtomSpec[newNum];
//        }
//    }
//    else {
//        SbAtomSpec *oldValues = values;
//
//        if (newNum > 0) {
//            values = new SbAtomSpec[newNum];
//            for (i = 0; i < num && i < newNum; i++) {
//                values[i] = oldValues[i];
//            }
//        }
//        else {
//            values = NULL;
//        }
//
//        // Free up any old stuff
//        if (oldValues != NULL) {
//            ChemBaseData *oldChemData;
//            ChemDisplay  *oldChemDisplay;
//            int32_t       oldAtom;
//
//            // Remove auditors and references on unused values
//            for (i = newNum; i < num; i++) {
//                oldValues[i].getValue(oldChemData, oldChemDisplay, oldAtom);
//                if (oldChemData != NULL) {
//                    oldChemData->removeAuditor(this, SoNotRec::FIELD);
//                    oldChemData->unref();
//                }
//                if (oldChemDisplay != NULL) {
//                    oldChemDisplay->removeAuditor(this, SoNotRec::FIELD);
//                    oldChemDisplay->unref();
//                }
//            }
//            delete [] oldValues;
//        }
//    }
//
//    num = maxNum = newNum;
//}
//
// <-- now using correct macros
////////////////////////////////////////////////////////////////////////
//
// Description:
//    Sets one value from 3 components. (Convenience function)
//
// Use: public

void
MFAtomSpec::set1Value(int index, ChemBaseData *chemData,
    ChemDisplay *chemDisplay, int32_t theAtom)
//
////////////////////////////////////////////////////////////////////////
{
    set1Value(index, SbAtomSpec(chemData, chemDisplay, theAtom));
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Sets to one vector value from 3 separate int32s. (Convenience function)
//
// Use: public

void
MFAtomSpec::setValue(ChemBaseData *chemData,
    ChemDisplay *chemDisplay, int32_t theAtom)
//
////////////////////////////////////////////////////////////////////////
{
    setValue(SbAtomSpec(chemData, chemDisplay, theAtom));
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Reads one (indexed) value from file. Returns FALSE on error.
//
// Use: private

SbBool
MFAtomSpec::read1Value(SoInput *in, int index)
//
////////////////////////////////////////////////////////////////////////
{
    SoBase *base;
    SbName  name;
    char    c;

    SbAtomSpec as;
    ChemBaseData *chemData = NULL;
    ChemDisplay  *chemDisplay = NULL;
    int32_t       theAtom = -1;

    // Read the parentheses
    if (! in->isBinary()) {
        if (! (in->read(c) && c == '(')) return FALSE;
    }

    // Get the ChemBaseData node
    if (in->read(name)) {
        if (name != "NULL") {
            in->putBack(name.getString());
            if (! SoBase::read(in, base, SoNode::getClassTypeId())) {
                return FALSE;
            }
            chemData = (ChemBaseData *)base;
        }
    }
    else {
        return FALSE;
    }

    // Get the ChemDisplay node
    if (in->read(name)) {
        if (name != "NULL") {
            in->putBack(name.getString());
            if (! SoBase::read(in, base, SoNode::getClassTypeId())) {
                return FALSE;
            }
            chemDisplay = (ChemDisplay *)base;
        }
    }
    else {
        return FALSE;
    }

    // Get theAtom
    if (!in->read(theAtom)) return FALSE;

    // Read the close parentheses
    if (! in->isBinary()) {
        if (! (in->read(c) && c == ')')) return FALSE;
    }

    // Set value of the SbAtomSpec
    as.setValue(chemData, chemDisplay, theAtom);

    setVal(index, as);
    return TRUE;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Overrides SoField::countWriteRefs() to maintain write-references
//    in nodes.
//
// Use: private

void
MFAtomSpec::countWriteRefs(SoOutput *out) const
//
////////////////////////////////////////////////////////////////////////
{
    // Do standard counting for field
    SoField::countWriteRefs(out);

    ChemBaseData *chemData = NULL;
    ChemDisplay  *chemDisplay = NULL;
    int32_t       theAtom = -1;

    // Count nodes
    int i;
    for (i = 0; i < num; i++) {
        values[i].getValue(chemData, chemDisplay, theAtom);
        if (chemData != NULL) {
            chemData->writeInstance(out);
        }
        if (chemDisplay != NULL) {
            chemDisplay->writeInstance(out);
        }
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Writes value of field to file.
//
// Use: private

void
MFAtomSpec::write1Value(SoOutput *out, int index) const
//
////////////////////////////////////////////////////////////////////////
{
    ChemBaseData *chemData = NULL;
    ChemDisplay  *chemDisplay = NULL;
    int32_t       theAtom = -1;

    values[index].getValue(chemData, chemDisplay, theAtom);

    // Write out the parentheses
    if (! out->isBinary()) out->write("( ");

    // Write out the ChemBaseData
    if (chemData != NULL) {
        int referenceId = 0;//out->findReference(chemData);
        out->write("USE");
        if (! out->isBinary()) out->write(' ');

        SbString t;
        const SbName &myName = chemData->getName();
        if (myName.getLength() != 0) {
            t += myName.getString();
        }
        t += "+";
        t += SbString(referenceId);
        out->write(t.getString());
    }
    else {
        out->write("NULL");
    }
    if (! out->isBinary()) out->write(' ');

    // Write out the ChemDisplay
    if (chemDisplay != NULL) {
        int referenceId = 0;//out->findReference(chemDisplay);
        out->write("USE");
        if (! out->isBinary()) out->write(' ');

        SbString t;
        const SbName &myName = chemDisplay->getName();
        if (myName.getLength() != 0) {
            t += myName.getString();
        }
        t += "+";
        t += SbString(referenceId);
        out->write(t.getString());
    }
    else {
        out->write("NULL");
    }
    if (! out->isBinary()) out->write(' ');

    // Write out theAtom
    out->write(theAtom);

    // Write out the parentheses
    if (! out->isBinary()) out->write(" )");
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Changes value in field without doing other notification stuff.
//    Keeps track of references and auditors.
//
// Use: private

void
MFAtomSpec::setVal(short index, const SbAtomSpec &newValue)
//
////////////////////////////////////////////////////////////////////////
{
    // The logic here is similar to that for SoSFNode::setVal.
    // Essentially we need to add and remove auditors so that
    // the ChemBaseData and ChemDisplay nodes can be kept track of
    // correctly and read/written correctly.
    // A comment from SoSFNode says:
    // "Play it safe if we are replacing one pointer with the same pointer"
    // which refers to the multiple ref'ing and unref'ing.

    ChemBaseData *chemData = NULL;
    ChemDisplay  *chemDisplay = NULL;
    int32_t       theAtom;

    ChemBaseData *oldChemData = NULL;
    ChemDisplay  *oldChemDisplay = NULL;
    int32_t       oldAtom = -1;

    newValue.getValue(chemData, chemDisplay, theAtom);
    if (chemData != NULL) {
        chemData->ref();
    }
    if (chemDisplay != NULL) {
        chemDisplay->ref();
    }

    values[index].getValue(oldChemData, oldChemDisplay, oldAtom);

    if (oldChemData != NULL) {
        oldChemData->removeAuditor(this, SoNotRec::FIELD);
        oldChemData->unref();
    }
    if (oldChemDisplay != NULL) {
        oldChemDisplay->removeAuditor(this, SoNotRec::FIELD);
        oldChemDisplay->unref();
    }

    if (chemData != NULL) {
        chemData->ref();
        chemData->addAuditor(this, SoNotRec::FIELD);
    }
    if (chemDisplay != NULL) {
        chemDisplay->ref();
        chemDisplay->addAuditor(this, SoNotRec::FIELD);
    }

    if (chemData != NULL) {
        chemData->unref();
    }
    if (chemDisplay != NULL) {
        chemDisplay->unref();
    }

    values[index].setValue(newValue);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Update a copied field to use the copy of each node if there is
//    one.
//
// Use: internal

void
MFAtomSpec::fixCopy(SbBool copyConnections)
//
////////////////////////////////////////////////////////////////////////
{
    ChemBaseData *chemData;
    ChemBaseData *chemDataCopy;
    ChemDisplay  *chemDisplay;
    ChemDisplay  *chemDisplayCopy;
    int32_t       theAtom;

    SbAtomSpec atomSpecCopy;
    for (int i = 0; i < num; i++) {
        chemData = NULL;
        chemDataCopy = NULL;
        chemDisplay = NULL;
        chemDisplayCopy = NULL;
        theAtom = -1;

        values[i].getValue(chemData, chemDisplay, theAtom);

        if (chemData != NULL) {
            chemDataCopy = (ChemBaseData *)
                SoFieldContainer::findCopy(chemData, copyConnections);
        }
        if (chemDisplay != NULL) {
            chemDisplayCopy = (ChemDisplay *)
                SoFieldContainer::findCopy(chemDisplay, copyConnections);
        }

        if ((chemDataCopy != NULL) || (chemDisplayCopy != NULL)) {
            atomSpecCopy.setValue(chemDataCopy, chemDisplayCopy, theAtom);
            setVal(i, atomSpecCopy);
        }
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Override this to also check the stored nodes.
//
// Use: internal, virtual

SbBool
MFAtomSpec::referencesCopy() const
//
////////////////////////////////////////////////////////////////////////
{
    // Do the normal test
    if (SoMField::referencesCopy()) {
        return TRUE;
    }

    ChemBaseData *chemData = NULL;
    ChemDisplay  *chemDisplay = NULL;
    int32_t       theAtom = -1;

    for (int i = 0; i < num; i++) {
        values[i].getValue(chemData, chemDisplay, theAtom);
        if ((chemData != NULL) &&
            (SoFieldContainer::checkCopy(chemData) != NULL)) {
            return TRUE;
        }
        if ((chemDisplay != NULL) &&
            (SoFieldContainer::checkCopy(chemDisplay) != NULL)) {
            return TRUE;
        }
    }

    return FALSE;
}
