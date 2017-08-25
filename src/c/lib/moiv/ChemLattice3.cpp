/*
 * ChemLattice3.c++
 *
 *     Classes:  ChemLattice3
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

#include "inv/actions/SoGLRenderAction.H"
#include "inv/actions/SoCallbackAction.H"
#include "inv/actions/SoPickAction.H"
#include "inv/actions/SoGetBoundingBoxAction.H"
#include "inv/actions/SoWriteAction.H"

#include "inv/errors/SoDebugError.H"
#include "inv/errors/SoReadError.H"

#include "inv/fields/SoMFBool.H"
#include "inv/fields/SoMFShort.H"
#include "inv/fields/SoMFInt32.H"
#include "inv/fields/SoMFFloat.H"

#include "inv/ChemKit/ChemLattice3.H"
#include "inv/ChemKit/ChemLattice3Element.H"
#include "inv/ChemKit/MFDouble.H"
#include "inv/ChemKit/MFInt8.H"

/*
	Jamie Doornbos	2001/07/27
	Daniel Soto		2002/02/07

#ifdef __coin
SO_NODE_SOURCE(ChemLattice3);
#else
SO__NODE_VARS(ChemLattice3);
#endif
*/
SO_NODE_SOURCE(ChemLattice3);

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Constructor
//
// Use: public

ChemLattice3::ChemLattice3(LatticeDataType dataType, 
    LatticeCoordType coordType)
//
////////////////////////////////////////////////////////////////////////
{
    SO_NODE_CONSTRUCTOR(ChemLattice3);

    SO_NODE_ADD_FIELD(dimension, (0, 0, 0));
    SO_NODE_ADD_FIELD(nDataVar, (0));
    SO_NODE_ADD_FIELD(coord, (0.0));

    theFieldData = new SoFieldData(fieldData);
    coord.deleteValues(0,-1);
    data = NULL;
    _coordType = coordType;

    setup(dataType);
    typeField.setValue(theSoType.getName());

    isBuiltIn = TRUE;

}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Constructor, used only when reading from a file
//
// Use: private

ChemLattice3::ChemLattice3()
//
////////////////////////////////////////////////////////////////////////
{
    SO_NODE_CONSTRUCTOR(ChemLattice3);

    SO_NODE_ADD_FIELD(dimension, (0, 0, 0));
    SO_NODE_ADD_FIELD(nDataVar, (0));
    SO_NODE_ADD_FIELD(coord, (0.0));

    theFieldData = new SoFieldData(fieldData);
    data = NULL;

    isBuiltIn = TRUE;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Destructor
//
// Use: protected since changes made by fabien fontaine the 13/12/2000 to avoid
// g++ compiler warning

ChemLattice3::~ChemLattice3()
//
////////////////////////////////////////////////////////////////////////
{
    delete theFieldData;
    if (data != NULL) {
        delete data;
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Create the data field from a LatticeDataType
//
// Use: private

void
ChemLattice3::setup(LatticeDataType dataType)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
    if (data != NULL) {
        SoDebugError::post("ChemLattice3::setup",
                           "Already initialized!");
    }
#endif

    switch (dataType) {
// --> sas
      case LATTICE_DATA_BOOL:
        theSoType = SoType::fromName(SbName("SoMFBool"));
        _dataType = dataType;
        break;
// <-- sas
      case LATTICE_DATA_INT8:
        theSoType = SoType::fromName(SbName("MFInt8"));
        _dataType = dataType;
        break;

      case LATTICE_DATA_INT16:
        theSoType = SoType::fromName(SbName("SoMFShort"));
        _dataType = dataType;
        break;

      case LATTICE_DATA_INT32:
        theSoType = SoType::fromName(SbName("SoMFInt32"));
        _dataType = dataType;
        break;

      case LATTICE_DATA_FLOAT32:
        theSoType = SoType::fromName(SbName("SoMFFloat"));
        _dataType = dataType;
        break;

      case LATTICE_DATA_FLOAT64:
        theSoType = SoType::fromName(SbName("MFDouble"));
        _dataType = dataType;
        break;

      default:
        theSoType = SoType::badType();
        _dataType = LATTICE_DATA_BAD_TYPE;
#ifdef DEBUG
        SoDebugError::post("ChemLattice3::setup",
            "Couldn't create field of dataType %d", dataType);
#endif
        break;
    }

    if (theSoType.isDerivedFrom(SoMField::getClassTypeId())) {
        data = (SoMField *)theSoType.createInstance();
    }
    else {
        data = NULL;
    }
    if (data == NULL) {
#ifdef DEBUG
        SoDebugError::post("ChemLattice3::setup",
            "Couldn't create field of dataType %d", dataType);
#endif
        theSoType = SoType::badType();
    }
    else {
        data->setContainer(this);
        theFieldData->addField(this, "data", data);
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Create the data field from an SoType
//
// Use: private

void
ChemLattice3::setup(SoType inputType)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
    if (data != NULL) {
        SoDebugError::post("ChemLattice3::setup",
                           "Already initialized!");
    }
#endif

    if (inputType.isDerivedFrom(SoMField::getClassTypeId())) {
// --> sas
//        if (inputType == MFInt8::getClassTypeId()) {
			if (inputType == SoMFBool::getClassTypeId()) {
				    _dataType = LATTICE_DATA_BOOL;
            theSoType = inputType;
            data = (SoMField *)theSoType.createInstance();
			}
			else if (inputType == MFInt8::getClassTypeId()) {
// <-- sas
            _dataType = LATTICE_DATA_INT8;
            theSoType = inputType;
            data = (SoMField *)theSoType.createInstance();
        }
        else if (inputType == SoMFShort::getClassTypeId()) {
            _dataType = LATTICE_DATA_INT16;
            theSoType = inputType;
            data = (SoMField *)theSoType.createInstance();
        }
        else if (inputType == SoMFInt32::getClassTypeId()) {
            _dataType = LATTICE_DATA_INT32;
            theSoType = inputType;
            data = (SoMField *)theSoType.createInstance();
        }
        else if (inputType == SoMFFloat::getClassTypeId()) {
            _dataType = LATTICE_DATA_FLOAT32;
            theSoType = inputType;
            data = (SoMField *)theSoType.createInstance();
        }
        else if (inputType == MFDouble::getClassTypeId()) {
            _dataType = LATTICE_DATA_FLOAT64;
            theSoType = inputType;
            data = (SoMField *)theSoType.createInstance();
        }
        else {
            _dataType = LATTICE_DATA_BAD_TYPE;
            theSoType = SoType::badType();
            data = NULL;
#ifdef DEBUG
            SoDebugError::post("ChemLattice3::setup",
                "Couldn't create field of type %s", 
                 inputType.getName().getString());
#endif
        }
    }
    else {
        _dataType = LATTICE_DATA_BAD_TYPE;
        theSoType = SoType::badType();
        data = NULL;
#ifdef DEBUG
        SoDebugError::post("ChemLattice3::setup",
            "Couldn't create field of type %s", 
             inputType.getName().getString());
#endif
    }
    if (data != NULL) {
        data->setContainer(this);
        theFieldData->addField(this, "data", data);
    }
}
    
////////////////////////////////////////////////////////////////////////
//
// Description:
//    Given a node index, return the coordinate value.
//
// Use: public
//

SbVec3f
ChemLattice3::getCoord(int32_t index[3]) const
//
////////////////////////////////////////////////////////////////////////
{
    int32_t i;
    float   v[3];
    SbVec3f rCoord;
    SbVec3i dims = dimension.getValue();

    v[0] = v[1] = v[2] = 0.0;
    // If there are no coordinates, then return the indices as the coord
    if (coord.getNum() == 0) {
        for (i = 0; i < 3; i++) {
            v[i] = (float)index[i];
        }
        rCoord.setValue(v);
    }
    else {
        const float *perim = coord.getValues(0);
        switch(_coordType) {
          case LATTICE_COORD_UNIFORM:
            for ( i = 0; i < 3; i++ ) {
                float   lower = perim[2*i];
                float   upper = perim[2*i + 1];
 
                // Interpolate the bounding box coordinates
                if ( dims[i] > 1 ) {
                    v[i] = lower + (upper - lower)*
                           index[i]/(dims[i] - 1);
                }
                else {
                    v[i] = lower;
                }
            }
            break;

          case LATTICE_COORD_RECTILINEAR:
            for ( i = 0; i < 3; i++ ) {
                /* look up the appropriate perimeter value */
                v[i] = perim[index[i]];
                perim += dims[i];
            }
            break;

          case LATTICE_COORD_BAD_TYPE:
            break;
        }
        rCoord.setValue(v);
    }
    return rCoord;
}


/*
	Daniel Soto		2002/02/07
	Alrady defined these functions in SO_NODE_SOURCE() macro.

#ifndef __coin

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Returns type id.
//
// Use: public
//

SoType
ChemLattice3::getTypeId() const
//
////////////////////////////////////////////////////////////////////////
{
    return classTypeId;
}

////////////////////////////////////////////////////////////////////////
// 
// Description:
//    Returns field data
// 
// Use: internal
//    

const SoFieldData *
ChemLattice3::getFieldData() const
// 
////////////////////////////////////////////////////////////////////////
{
    return theFieldData;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Create an instance of the class, using private constructor.
//    Used only by database when reading in from file.
//
// Use: internal
//    

void *
ChemLattice3::createInstance()
//
////////////////////////////////////////////////////////////////////////
{
    return new ChemLattice3;
}

#endif  // __coin
*/
////////////////////////////////////////////////////////////////////////
// 
// Description: 
//    Special read code needed to read in type of ChemLattice3 data.
//
// Use: private
  
SbBool
ChemLattice3::readInstance(SoInput *in, unsigned short flags)
//
////////////////////////////////////////////////////////////////////////
{
    SbName typeName;
    if (!in->read(typeName, TRUE) ||
                typeName != "type" || !typeField.read(in, "type")) {

        SoReadError::post(in, "ChemLattice3 is missing type field");
        return FALSE;
    }
  
    SoType inputType = SoType::fromName(typeField.getValue());
    if (! inputType.isDerivedFrom(SoMField::getClassTypeId())) {
        SoReadError::post(in, "\"%s\" is not a type of MField",
                          typeField.getValue().getString());
        return FALSE;
    }
 
    setup(inputType);

    return SoNode::readInstance(in, flags);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Implements write action for ChemLattice3 nodes.
//
// Use: extender

void
ChemLattice3::write(SoWriteAction *action)  
//
////////////////////////////////////////////////////////////////////////
{
    if (action->getOutput()->getStage() == SoOutput::COUNT_REFS)
        addWriteReference(action->getOutput());

    // We assume this node would not be traversed if is has not been counted
    else if (! writeHeader(action->getOutput(), FALSE, FALSE)) {
  
        // Write type info
        typeField.write(action->getOutput(), "type");

        const SoFieldData *fieldData = getFieldData();

        fieldData->write(action->getOutput(), this);

        writeFooter(action->getOutput());
    }
} 


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Handles any action derived from SoAction.
//
// Use: extender

void
ChemLattice3::doAction(SoAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    ChemLattice3Element::set(action->getState(), this, this);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Does GL render action.
//
// Use: extender

void
ChemLattice3::GLRender(SoGLRenderAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    ChemLattice3::doAction(action);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Handles get bounding box action.
//
// Use: extender

void
ChemLattice3::getBoundingBox(SoGetBoundingBoxAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    ChemLattice3::doAction(action);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Does callback action thing.
//
// Use: extender

void
ChemLattice3::callback(SoCallbackAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    ChemLattice3::doAction(action);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Does pick action...
//
// Use: extender

void
ChemLattice3::pick(SoPickAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    ChemLattice3::doAction(action);
}
