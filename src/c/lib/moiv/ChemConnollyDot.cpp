/*
 * ChemConnollyDot.c++
 *
 *     Classes:  ChemConnollyDot
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

//KLS include stdlib for g++ 3.2.3
#include <stdlib.h>

#include <stdio.h>

#include "inv/actions/SoGLRenderAction.H"
#include "inv/actions/SoRayPickAction.H"

#include "inv/ChemKit/ChemConnollyDot.H"
#include "inv/ChemKit/ChemBaseDataElement.H"
#include "inv/ChemKit/ChemColorElement.H"
#include "inv/ChemKit/ChemRadiiElement.H"

void
ChemConnollyDot::initClass()
{
    SO_NODE_INIT_CLASS(ChemConnollyDot, SoNonIndexedShape, "NonIndexedShape");
}

SO_NODE_SOURCE(ChemConnollyDot)

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Constructor
//
// Use: public

ChemConnollyDot::ChemConnollyDot()
//
////////////////////////////////////////////////////////////////////////
{
    SO_NODE_CONSTRUCTOR(ChemConnollyDot);

    SO_NODE_ADD_FIELD(probeRadius, (0.0));
    SO_NODE_ADD_FIELD(densityOfPoints, (10.0));
    SO_NODE_ADD_FIELD(colorBinding, (ChemConnollyDot::DEFAULT));
    SO_NODE_ADD_FIELD(overallSurfaceColor, (1.0, 1.0, 1.0));
    SO_NODE_ADD_FIELD(contactSurfaceColor, (1.0, 0.0, 0.0));
    SO_NODE_ADD_FIELD(saddleSurfaceColor,  (0.0, 1.0, 0.0));
    SO_NODE_ADD_FIELD(concaveSurfaceColor, (0.0, 0.0, 1.0));

    SO_NODE_DEFINE_ENUM_VALUE(ColorBinding, OVERALL);
    SO_NODE_DEFINE_ENUM_VALUE(ColorBinding, BY_ATOM);
    SO_NODE_DEFINE_ENUM_VALUE(ColorBinding, BY_SURFACE);
    SO_NODE_DEFINE_ENUM_VALUE(ColorBinding, DEFAULT);

    SO_NODE_SET_SF_ENUM_TYPE(colorBinding, ColorBinding);

    SoVertexProperty *vp = new SoVertexProperty;
    vp->ref();
    vertexProperty = vp;

    // Initialize private variables
    lastChemBaseDataElement = NULL;
    lastChemColorElement = NULL;
    lastChemRadiiElement = NULL;

    regenerateFlag = FALSE;

    contactSurfaceArea = 0.0;
    reentrantSurfaceArea = 0.0;

    index = NULL;
    surfaceRequest = NULL;
    coords = NULL;
    radii = NULL;
    theSurface = NULL;

    isBuiltIn = TRUE;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Destructor
//
// Use: protected

ChemConnollyDot::~ChemConnollyDot()
//
////////////////////////////////////////////////////////////////////////
{
    if (index != NULL) delete [] index;
    if (surfaceRequest != NULL) delete [] surfaceRequest;
    if (coords != NULL) delete [] coords;
    if (radii != NULL) delete [] radii;
    if (theSurface != NULL) free(theSurface);

    if (lastChemBaseDataElement != NULL) {
        delete (SoElement *)lastChemBaseDataElement;
    }
    if (lastChemColorElement != NULL) {
        delete (SoElement *)lastChemColorElement;
    }
    if (lastChemRadiiElement != NULL) {
        delete (SoElement *)lastChemRadiiElement;
    }

    SoVertexProperty *vp =
        (SoVertexProperty *)vertexProperty.getValue();
    if (vp != NULL) vp->unref();
}
    
////////////////////////////////////////////////////////////////////////
//
// Description:
//    If a change was made to one of the fields, then we might need
//    to regenerate the dot surface
//
// Use: internal

void
ChemConnollyDot::notify(SoNotList *list)
//
////////////////////////////////////////////////////////////////////////
{
    if ((list->getLastRec()->getType() == SoNotRec::CONTAINER)) {
        SoField *field = list->getLastField();
        if (field != &vertexProperty) regenerateFlag = TRUE;
    }
    SoShape::notify(list);
}

////////////////////////////////////////////////////////////////////////
// 
// Description:
//    Performs GL rendering of the dot surface
//
// Use: public, extender

void
ChemConnollyDot::GLRender(SoGLRenderAction *action)
// 
////////////////////////////////////////////////////////////////////////
{
    regenerate(action);
    SoPointSet::GLRender(action);
}

////////////////////////////////////////////////////////////////////////
// 
// Description:
//    Implements ray picking
//
// Use: public, extender

void
ChemConnollyDot::rayPick(SoRayPickAction *action)
// 
////////////////////////////////////////////////////////////////////////
{
    regenerate(action);
    SoPointSet::rayPick(action);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Generate primitives
//
// Use: protected

void
ChemConnollyDot::generatePrimitives(SoAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    regenerate(action);
    SoPointSet::generatePrimitives(action);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Computes the bounding box of the dot surface
//  
// Use: protected

void
ChemConnollyDot::computeBBox(SoAction *action, SbBox3f &box,
    SbVec3f &center)
//
////////////////////////////////////////////////////////////////////////
{
    regenerate(action);
    SoPointSet::computeBBox(action, box, center);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Generate the dot surface
//  
// Use: private

void
ChemConnollyDot::regenerate(SoAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    SoState *state = action->getState();

    SbBool dataChanged = FALSE;
    SbBool colorChanged = FALSE;
    SbBool radiiChanged = FALSE;

    if (regenerateFlag == FALSE) {
        // Determine if the data has changed since the last invocation
        const ChemBaseDataElement *cbde =
            ChemBaseDataElement::getInstance(state);
        if (cbde != NULL) {
            if (lastChemBaseDataElement == NULL) {
                dataChanged = TRUE;
                lastChemBaseDataElement =
                    (ChemBaseDataElement *)cbde->copyMatchInfo();
            }
            else {
                if (!lastChemBaseDataElement->matches(cbde)) {
                    dataChanged = TRUE;
                    delete (SoElement *)lastChemBaseDataElement;
                    lastChemBaseDataElement =
                        (ChemBaseDataElement *)cbde->copyMatchInfo();
                }
            }
        }

        // Determine if the atom colors have changed since the last invocation
        const ChemColorElement *cce =
            ChemColorElement::getInstance(state);
        if (cce != NULL) {
            if (lastChemColorElement == NULL) {
                colorChanged = TRUE;
                lastChemColorElement =
                    (ChemColorElement *)cce->copyMatchInfo();
            }
            else {
                if (!lastChemColorElement->matches(cce)) {
                    colorChanged = TRUE;
                    delete (SoElement *)lastChemColorElement;
                    lastChemColorElement =
                        (ChemColorElement *)cce->copyMatchInfo();
                }
            }
        }

        // Determine if the atom radii have changed since the last invocation
        const ChemRadiiElement *cre =
            ChemRadiiElement::getInstance(state);
        if (cre != NULL) {
            if (lastChemRadiiElement == NULL) {
                radiiChanged = TRUE;
                lastChemRadiiElement =
                    (ChemRadiiElement *)cre->copyMatchInfo();
            }
            else {
                if (!lastChemRadiiElement->matches(cre)) {
                    radiiChanged = TRUE;
                    delete (SoElement *)lastChemRadiiElement;
                    lastChemRadiiElement =
                        (ChemRadiiElement *)cre->copyMatchInfo();
                }
            }
        }
    }

    if ((dataChanged == FALSE) && (radiiChanged == FALSE) &&
        (colorChanged == FALSE) && (regenerateFlag == FALSE)) return;

    // Make sure regenerateFlag has been canceled out
    regenerateFlag = FALSE;

    ChemBaseData *chemData = ChemBaseDataElement::get(state);
    ChemColor *chemColor = ChemColorElement::get(state);
    ChemRadii *chemRadii = ChemRadiiElement::get(state);

    // Create the arrays needed by the Connolly calculation.
    int32_t numberOfAtoms = chemData->getNumberOfAtoms();
    if (numberOfAtoms <= 0) return;
    
    if (index != NULL) delete [] index;
    index = new int32_t[numberOfAtoms];

    if (surfaceRequest != NULL) delete [] surfaceRequest;
    surfaceRequest = new int8_t[numberOfAtoms];

    if (coords != NULL) delete [] coords;
    coords = new float[3*numberOfAtoms];

    if (radii != NULL) delete [] radii;
    radii = new float[numberOfAtoms];

    // theSurface is allocated in the Connolly code
    if (theSurface != NULL) free(theSurface);

    // Copy the information from chemData into the Connolly arrays
    int32_t i;
    float xx, yy, zz;
    short radiiBinding = chemRadii->atomRadiiBinding.getValue();
    const float *atomRadii = chemRadii->atomRadii.getValues(0);
    short atomColorBinding = chemColor->atomColorBinding.getValue();
    const SbColor *atomColors = chemColor->atomColor.getValues(0);

    for (i = 0; i < numberOfAtoms; i++) {
        index[i] = i+1;
        surfaceRequest[i] = 2;
        chemData->getAtomCoordinates(i).getValue(xx, yy, zz);
        coords[i*3] = xx;
        coords[i*3+1] = yy;
        coords[i*3+2] = zz;
        if (radiiBinding == ChemRadii::RADII_OVERALL) {
            radii[i] = atomRadii[0];
        }
        else if (radiiBinding == ChemRadii::RADII_PER_ATOM) {
            radii[i] = atomRadii[i];
        }
        else if (radiiBinding == ChemRadii::RADII_PER_ATOM_INDEXED) {
            int32_t theIndex = chemData->getAtomIndex(i);
            radii[i] = atomRadii[theIndex];
        }
    }

    int32_t totalReturned;
    int success = surface(numberOfAtoms, (probeRadius.getValue()),
        (densityOfPoints.getValue()), surfaceRequest,
        radii, coords, index, &totalReturned, &theSurface);

    // Get rid of stuff we don't need.  Everything is in theSurface.
    delete [] index; index = NULL;
    delete [] surfaceRequest; surfaceRequest = NULL;
    delete [] coords; coords = NULL;
    delete [] radii; radii = NULL;

    if (!success) return;

    SoVertexProperty *vp =
        (SoVertexProperty *)vertexProperty.getValue();

    vp->vertex.setNum(totalReturned);
    vp->normal.setNum(totalReturned);
    vp->normalBinding.setValue(SoVertexProperty::PER_VERTEX);

    short cBinding = colorBinding.getValue();
    uint32_t overallColor = overallSurfaceColor.getValue().getPackedValue();
    uint32_t contactColor = contactSurfaceColor.getValue().getPackedValue();
    uint32_t saddleColor = saddleSurfaceColor.getValue().getPackedValue();
    uint32_t concaveColor = concaveSurfaceColor.getValue().getPackedValue();

    if (cBinding == ChemConnollyDot::OVERALL) {
        vp->materialBinding.setValue(SoVertexProperty::OVERALL);
        vp->orderedRGBA.setValue(overallColor);
    }
    else if (cBinding == ChemConnollyDot::BY_ATOM) {
        // Create orderedRGBA from chemColor
        vp->materialBinding.setValue(SoVertexProperty::PER_VERTEX);
    }
    else if (cBinding == ChemConnollyDot::BY_SURFACE) {
        // Create 3 entries in orderedRGBA
        vp->materialBinding.setValue(SoVertexProperty::PER_VERTEX);
    }

    contactSurfaceArea = 0.0;
    reentrantSurfaceArea = 0.0;

    SbVec3f *vVertices = vp->vertex.startEditing();
    SbVec3f *vNorms = vp->normal.startEditing();
    uint32_t *vColors = NULL;
    if (cBinding != ChemConnollyDot::OVERALL) {
        vp->orderedRGBA.setNum(totalReturned);
        vColors = vp->orderedRGBA.startEditing();
    }
    for (i = 0; i < totalReturned; i++) {
        vVertices[i].setValue(theSurface[i].coord[0],
                              theSurface[i].coord[1],
                              theSurface[i].coord[2]);
        vNorms[i].setValue(theSurface[i].norm[0],
                           theSurface[i].norm[1],
                           theSurface[i].norm[2]);
        if (theSurface[i].shape == 1) {
            contactSurfaceArea += theSurface[i].area;
        }
        else {
            reentrantSurfaceArea += theSurface[i].area;
        }

        if (cBinding == ChemConnollyDot::BY_SURFACE) {
            if (theSurface[i].shape == 1) {
                vColors[i] = contactColor;
            }
            else if (theSurface[i].shape == 2) {
                vColors[i] = saddleColor;
            }
            else if (theSurface[i].shape == 3) {
                vColors[i] = concaveColor;
            }
        }
        else if (cBinding == ChemConnollyDot::BY_ATOM) {
            if (atomColorBinding == ChemColor::ATOM_OVERALL) {
                vColors[i] = atomColors[0].getPackedValue();
            }
            else if (atomColorBinding == ChemColor::ATOM_PER_ATOM) {
                vColors[i] = atomColors[theSurface[i].atom1-1].getPackedValue();;
            }
            else if (atomColorBinding == ChemColor::ATOM_PER_ATOM_INDEXED) {
                int32_t theIndex = chemData->getAtomIndex(theSurface[i].atom1-1);
                vColors[i] = atomColors[theIndex].getPackedValue();
            }
        }
    }
    vp->vertex.finishEditing();
    vp->normal.finishEditing();
    if (cBinding != ChemConnollyDot::OVERALL) {
        vp->orderedRGBA.finishEditing();
    }

    free(theSurface); theSurface = NULL;
}
