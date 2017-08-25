/*
 * ChemDisplaySelectionElement.c++
 *
 *     Classes:  ChemDisplaySelectionElement
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

#include "inv/errors/SoDebugError.H"

#include "inv/ChemKit/ChemDisplayPath.H"
#include "inv/ChemKit/ChemDisplayPathList.H"
#include "inv/ChemKit/ChemDisplaySelectionElement.H"

SO_ELEMENT_SOURCE(ChemDisplaySelectionElement);

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Initializes element.
//
// Use: public

void
ChemDisplaySelectionElement::init(SoState *state)
//
////////////////////////////////////////////////////////////////////////
{
   selectionList.truncate(0);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Destructor.
//
// Use: private

ChemDisplaySelectionElement::~ChemDisplaySelectionElement()
//
////////////////////////////////////////////////////////////////////////
{
    selectionList.truncate(0);
}
  
////////////////////////////////////////////////////////////////////////
//
// Description:
//    Overrides this method to return TRUE if the two
//    elements match.
//
// Use: public

SbBool
ChemDisplaySelectionElement::matches(const SoElement *elt) const
//
////////////////////////////////////////////////////////////////////////
{
    if (elt == NULL) return FALSE;

    const ChemDisplaySelectionElement *cse = (const ChemDisplaySelectionElement *)elt;

    if (cse->selectionList.getLength() != selectionList.getLength()) {
        return FALSE;
    }

    // Check that contents of the ChemDisplayPath's match
    for (int i = 0; i < selectionList.getLength(); i++) {
        if (selectionList[i]->isOfType(ChemDisplayPath::getClassTypeId())) {
            ChemDisplayPath *path0 = selectionList[i];
            ChemDisplayPath *path1 = cse->selectionList[i];
            if (!(*path0 == *path1)) {
                return FALSE;
            }
        }
    }

    return TRUE;
}
  
////////////////////////////////////////////////////////////////////////
//
// Description:
//     Create a copy of this instance suitable for calling matches()
//     on.
//
// Use: protected

SoElement *
ChemDisplaySelectionElement::copyMatchInfo() const
//
////////////////////////////////////////////////////////////////////////
{
    ChemDisplaySelectionElement *result =
        (ChemDisplaySelectionElement *)getTypeId().createInstance();
 
    result->selectionList.truncate(0);
    for (int i = 0; i < selectionList.getLength(); i++) {
        if (selectionList[i]->isOfType(ChemDisplayPath::getClassTypeId())) {
            ChemDisplayPath *newPath = selectionList[i]->copy();
// --> EGB memory leak
			newPath->ref();
// <-- EGB memory leak
            result->selectionList.append((void *)newPath);
        }
    }
 
    return result;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Sets the selectionList in element accessed from state.
//
// Use: public

void
ChemDisplaySelectionElement::set(SoState *state,
    const ChemDisplayPathList &_selectionList)
//
////////////////////////////////////////////////////////////////////////
{
    ChemDisplaySelectionElement        *elt;

    // Get an instance we can change (pushing if necessary)
    elt = (ChemDisplaySelectionElement *) getElement(state, classStackIndex);

    if (elt != NULL) {
        elt->selectionList = _selectionList;
    }
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Prints element for debugging.
//
// Use: public
//
////////////////////////////////////////////////////////////////////////

#ifdef DEBUG
void
ChemDisplaySelectionElement::print(FILE *fp) const
{
    SoReplacedElement::print(fp);
}
#else  /* DEBUG */
void
ChemDisplaySelectionElement::print(FILE *) const
{
}
#endif /* DEBUG */
