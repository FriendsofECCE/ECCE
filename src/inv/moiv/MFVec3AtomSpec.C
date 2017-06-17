/*
 * MFVec3AtomSpec.c++
 *
 *     Classes:  MFVec3AtomSpec
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

#include "inv/ChemKit/MFVec3AtomSpec.H"
#include "inv/ChemKit/ChemBaseData.H"
// --> lod
#include "inv/ChemKit/ChemLOD.H"
// <-- lod
#include "inv/ChemKit/ChemDisplay.H"

//////////////////////////////////////////////////////////////////////////////
//
// MFVec3AtomSpec class
//
//////////////////////////////////////////////////////////////////////////////

// --> now using correct macros
SO_MFIELD_SOURCE(MFVec3AtomSpec, SbVec3AtomSpec, const SbVec3AtomSpec &);
//// Use standard definitions of all required methods, but we have to
//// redefine all the rest to keep track of references and auditors.
//
//
//SO_MFIELD_REQUIRED_SOURCE(MFVec3AtomSpec);
//
///*
//	Jamie Doornbos	2001/07/27
//	Daniel Soto		2002/02/07
//*/
//#ifdef __coin
//SO_MFIELD_VALUE_SOURCE(MFVec3AtomSpec, SbVec3AtomSpec, const SbVec3AtomSpec &);
//#endif
//
//
//////////////////////////////////////////////////////////////////////////
////
//// Description:
////    Constructor
////
//// Use: public
//
//MFVec3AtomSpec::MFVec3AtomSpec()
////
//////////////////////////////////////////////////////////////////////////
//{
//    values = NULL;
//}
//
///////////////////////////////////////////////////////////////////////////
////
//// Description:
////    Destructor
////
//// Use: public
//
//MFVec3AtomSpec::~MFVec3AtomSpec()
////
//////////////////////////////////////////////////////////////////////////
//{
//    deleteAllValues();
//}
//
//
///*
//	Daniel Soto		2002/02/07
//	Alrady defined this functions in SO_MFIELD_VALUE_SOURCE() macro.
//*/
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
//MFVec3AtomSpec::find(const SbVec3AtomSpec &targetValue, SbBool addIfNotFound)
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
//
//void
//MFVec3AtomSpec::setValues(int start, int num, const SbVec3AtomSpec *newValues)
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
//MFVec3AtomSpec::set1Value(int index, const SbVec3AtomSpec &newValue)
//{
//    if (index >= getNum()) {
//        makeRoom(index + 1);
//    }
//    setVal(index, newValue);
//    valueChanged();
//}
//
// <-- now using correct macros
void
MFVec3AtomSpec::set1Value(int index, const SbAtomSpec &i,
    const SbAtomSpec &j, const SbAtomSpec &k)
{
    SbVec3AtomSpec newValue(i, j, k);
    if (index >= getNum()) {
        makeRoom(index + 1);
    }
    setVal(index, newValue);
    valueChanged();
}

void
MFVec3AtomSpec::set1Value(int index, const SbAtomSpec ijk[3])
{
    SbVec3AtomSpec newValue(ijk);
    if (index >= getNum()) {
        makeRoom(index + 1);
    }
    setVal(index, newValue);
    valueChanged();
}
// --> now using correct macros
//void
//MFVec3AtomSpec::setValue(const SbVec3AtomSpec &newValue)
//{
//    makeRoom(1);
//    setVal(0, newValue);
//    valueChanged();
//}
// <-- now using correct macros

void
MFVec3AtomSpec::setValue(const SbAtomSpec &i,
    const SbAtomSpec &j, const SbAtomSpec &k)
{
    SbVec3AtomSpec newValue(i, j, k);
    makeRoom(1);
    setVal(0, newValue);
    valueChanged();
}
// --> now using correct macros
//int
//MFVec3AtomSpec::operator ==(const MFVec3AtomSpec &f) const
//{
//    int i, num = getNum();
//    const SbVec3AtomSpec *myVals, *itsVals;
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
//MFVec3AtomSpec::deleteAllValues()
//{
//    allocValues(0);
//}
//
//void
//MFVec3AtomSpec::copyValue(int to, int from)
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
//MFVec3AtomSpec::allocValues(int newNum)
//{
//    int i, j;
//
//    if (values == NULL) {
//        if (newNum > 0) {
//            values = new SbVec3AtomSpec[newNum];
//        }
//    }
//    else {
//        SbVec3AtomSpec *oldValues = values;
//
//        if (newNum > 0) {
//            values = new SbVec3AtomSpec[newNum];
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
//            const SbAtomSpec *atomSpec;
//
//            // Remove auditors and references on unused values
//            for (i = newNum; i < num; i++) {
//                atomSpec = oldValues[i].getValue();
//                for (j = 0; j < 3; j++) {
//                    atomSpec[j].getValue(oldChemData, oldChemDisplay, oldAtom);
//                    if (oldChemData != NULL) {
//                        oldChemData->removeAuditor(this, SoNotRec::FIELD);
//                        oldChemData->unref();
//                    }
//                    if (oldChemDisplay != NULL) {
//                        oldChemDisplay->removeAuditor(this, SoNotRec::FIELD);
//                        oldChemDisplay->unref();
//                    }
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
//    Reads one (indexed) value from file. Returns FALSE on error.
//
// Use: private

SbBool
MFVec3AtomSpec::read1Value(SoInput *in, int index)
//
////////////////////////////////////////////////////////////////////////
{
    SbAtomSpec as1, as2, as3;

    if (!readAtomSpec(in, as1)) return FALSE;
    if (!readAtomSpec(in, as2)) return FALSE;
    if (!readAtomSpec(in, as3)) return FALSE;

    SbVec3AtomSpec as123(as1, as2, as3);
    setVal(index, as123);
    return TRUE;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Performs the actual read of an atomSpec.  See also SFAtomSpec.
//
// Use: private

SbBool
MFVec3AtomSpec::readAtomSpec(SoInput *in, SbAtomSpec &as)
//
////////////////////////////////////////////////////////////////////////
{
    SoBase *base;
    SbName  name;
    char    c;

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
MFVec3AtomSpec::countWriteRefs(SoOutput *out) const
//
////////////////////////////////////////////////////////////////////////
{
    // Do standard counting for field
    SoField::countWriteRefs(out);

    ChemBaseData *chemData = NULL;
    ChemDisplay  *chemDisplay = NULL;
    int32_t       theAtom = -1;

    // Count nodes
    int i, j;
    const SbAtomSpec *atomSpec;
    for (i = 0; i < num; i++) {
        atomSpec = values[i].getValue();
        for (j = 0; j < 3; j++) {
            atomSpec[j].getValue(chemData, chemDisplay, theAtom);
            if (chemData != NULL) {
                chemData->writeInstance(out);
            }
            if (chemDisplay != NULL) {
                chemDisplay->writeInstance(out);
            }
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
MFVec3AtomSpec::write1Value(SoOutput *out, int index) const
//
////////////////////////////////////////////////////////////////////////
{
    SbAtomSpec as1, as2, as3;

    values[index].getValue(as1, as2, as3);

    writeAtomSpec(as1, out);
    if (! out->isBinary()) out->write(' ');
    writeAtomSpec(as2, out);
    if (! out->isBinary()) out->write(' ');
    writeAtomSpec(as3, out);
}

///////////////////////////////////////////////////////////////////////
//
// Description:
//    Performs tha actual write of an SbAtomSpec to a file.
//    This code is the same as for SFAtomSpec::writeValue so any changes
//    there should be reflected here and vice versa.
//
// Use: private

void
MFVec3AtomSpec::writeAtomSpec(const SbAtomSpec &as, SoOutput *out) const
//
////////////////////////////////////////////////////////////////////////
{
    ChemBaseData *chemData = NULL;
    ChemDisplay  *chemDisplay = NULL;
    int32_t       theAtom = -1;

    as.getValue(chemData, chemDisplay, theAtom);

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
MFVec3AtomSpec::setVal(short index, const SbVec3AtomSpec &newValue)
// 
////////////////////////////////////////////////////////////////////////
{
    ChemBaseData *chemData = NULL;
    ChemDisplay  *chemDisplay = NULL;
    int32_t       theAtom;

    ChemBaseData *oldChemData = NULL;
    ChemDisplay  *oldChemDisplay = NULL;
    int32_t       oldAtom = -1;

    const SbAtomSpec *atomSpec = newValue.getValue();
    const SbAtomSpec *oldAtomSpec = values[index].getValue();
    for (int i = 0; i < 3; i++) {
        atomSpec[i].getValue(chemData, chemDisplay, theAtom);

        if (chemData != NULL) {
            chemData->ref();
        }
        if (chemDisplay != NULL) {
            chemDisplay->ref();
        }

        oldAtomSpec[i].getValue(oldChemData, oldChemDisplay, oldAtom);

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
    }
    values[index].setValue(atomSpec);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Update a copied field to use the copy of each node if there is
//    one.
//
// Use: internal
    
void
MFVec3AtomSpec::fixCopy(SbBool copyConnections)
//
////////////////////////////////////////////////////////////////////////
{
    ChemBaseData *chemData1;
    ChemBaseData *chemDataCopy1;
    ChemDisplay  *chemDisplay1;
    ChemDisplay  *chemDisplayCopy1;
    int32_t       theAtom1;

    ChemBaseData *chemData2;
    ChemBaseData *chemDataCopy2;
    ChemDisplay  *chemDisplay2;
    ChemDisplay  *chemDisplayCopy2;
    int32_t       theAtom2;

    ChemBaseData *chemData3;
    ChemBaseData *chemDataCopy3;
    ChemDisplay  *chemDisplay3;
    ChemDisplay  *chemDisplayCopy3;
    int32_t       theAtom3;
    
    int i;
    const SbAtomSpec *atomSpecValue;
    SbAtomSpec atomSpecCopy1;
    SbAtomSpec atomSpecCopy2;
    SbAtomSpec atomSpecCopy3;
    SbVec3AtomSpec atomVec3;

    for (i = 0; i < num; i++) {
        chemData1 = chemData2 = chemData3 = NULL;
        chemDataCopy1 = chemDataCopy2 = chemDataCopy3 = NULL;
        chemDisplay1 = chemDisplay2 = chemDisplay3 = NULL;
        chemDisplayCopy1 = chemDisplayCopy2 = chemDisplayCopy3 = NULL;

        atomSpecValue = values[i].getValue();
        atomSpecValue[0].getValue(chemData1, chemDisplay1, theAtom1);
        atomSpecValue[1].getValue(chemData2, chemDisplay2, theAtom2);
        atomSpecValue[2].getValue(chemData3, chemDisplay3, theAtom3);

        if (chemData1 != NULL) {
            chemDataCopy1 = (ChemBaseData *)
                SoFieldContainer::findCopy(chemData1, copyConnections);
        }
        if (chemDisplay1 != NULL) {
            chemDisplayCopy1 = (ChemDisplay *)
                SoFieldContainer::findCopy(chemDisplay1, copyConnections);
        }
        if (chemData2 != NULL) {
            chemDataCopy2 = (ChemBaseData *)
                SoFieldContainer::findCopy(chemData2, copyConnections);
        }
        if (chemDisplay2 != NULL) {
            chemDisplayCopy2 = (ChemDisplay *)
                SoFieldContainer::findCopy(chemDisplay2, copyConnections);
        }
        if (chemData3 != NULL) {
            chemDataCopy3 = (ChemBaseData *)
                SoFieldContainer::findCopy(chemData3, copyConnections);
        }
        if (chemDisplay3 != NULL) {
            chemDisplayCopy3 = (ChemDisplay *)
                SoFieldContainer::findCopy(chemDisplay3, copyConnections);
        }
        if ((chemDataCopy1 != NULL) || (chemDisplayCopy1 != NULL) ||
            (chemDataCopy2 != NULL) || (chemDisplayCopy2 != NULL) ||
            (chemDataCopy3 != NULL) || (chemDisplayCopy3 != NULL)) {
            atomSpecCopy1.setValue(chemDataCopy1, chemDisplayCopy1, theAtom1);
            atomSpecCopy2.setValue(chemDataCopy2, chemDisplayCopy2, theAtom2);
            atomSpecCopy3.setValue(chemDataCopy3, chemDisplayCopy3, theAtom3);
            atomVec3.setValue(atomSpecCopy1, atomSpecCopy2, atomSpecCopy3);
            setVal(i, atomVec3);
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
MFVec3AtomSpec::referencesCopy() const
//
////////////////////////////////////////////////////////////////////////
{
    // Do the normal test
    if (SoMField::referencesCopy()) {
        return TRUE;
    }

    ChemBaseData *chemData1, *chemData2, *chemData3;
    ChemDisplay  *chemDisplay1, *chemDisplay2, *chemDisplay3;
    int32_t       theAtom1, theAtom2, theAtom3;

    const SbAtomSpec *atomSpecValue;
    for (int i = 0; i < num; i++) {
        chemData1 = chemData2 = chemData3 = NULL;
        chemDisplay1 = chemDisplay2 = chemDisplay3 = NULL;

        atomSpecValue = values[i].getValue();
        atomSpecValue[0].getValue(chemData1, chemDisplay1, theAtom1);
        atomSpecValue[1].getValue(chemData2, chemDisplay2, theAtom2);
        atomSpecValue[2].getValue(chemData3, chemDisplay3, theAtom3);

        if ((chemData1 != NULL) &&
            (SoFieldContainer::checkCopy(chemData1) != NULL)) {
            return TRUE;
        }
        if ((chemDisplay1 != NULL) &&
            (SoFieldContainer::checkCopy(chemDisplay1) != NULL)) {
            return TRUE;
        }

        if ((chemData2 != NULL) &&
            (SoFieldContainer::checkCopy(chemData2) != NULL)) {
            return TRUE;
        }
        if ((chemDisplay2 != NULL) &&
            (SoFieldContainer::checkCopy(chemDisplay2) != NULL)) {
            return TRUE;
        }

        if ((chemData3 != NULL) &&
            (SoFieldContainer::checkCopy(chemData3) != NULL)) {
            return TRUE;
        }
        if ((chemDisplay3 != NULL) &&
            (SoFieldContainer::checkCopy(chemDisplay3) != NULL)) {
            return TRUE;
        }
    }

    return FALSE;
}
