/*
 * ChemOverrideElement.c++   
 *
 *     Classes:  ChemOverrideElement base class
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
  
#include "inv/ChemKit/ChemOverrideElement.H"

SO_ELEMENT_SOURCE(ChemOverrideElement);

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Initializes element.  All flags are initialized to FALSE (0).
//
// Use: public

void
ChemOverrideElement::init(SoState *)
//
////////////////////////////////////////////////////////////////////////
{
    flags = 0;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Pushes element, copying flags from previous top instance.
//
// Use: public
void
ChemOverrideElement::push(SoState *state)
//
////////////////////////////////////////////////////////////////////////
{
    ChemOverrideElement *elt =
        (ChemOverrideElement *)getNextInStack();

    flags = elt->flags;
    elt->capture(state);  // Capture previous element
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Get ChemDisplayParamElement override flag
//
// Use: public, static
SbBool
ChemOverrideElement::getChemDisplayParamOverride(SoState *state)
//
////////////////////////////////////////////////////////////////////////
{
    const ChemOverrideElement *elt;
    elt = (const ChemOverrideElement *)
        getConstElement(state, classStackIndex);
    return elt->flags & CHEM_DISPLAYPARAM;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Get ChemColorElement override flag
//
// Use: public, static
SbBool
ChemOverrideElement::getChemColorOverride(SoState *state)
//
////////////////////////////////////////////////////////////////////////
{
    const ChemOverrideElement *elt;
    elt = (const ChemOverrideElement *)
        getConstElement(state, classStackIndex);
    return elt->flags & CHEM_COLOR;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Get ChemRadiiElement override flag
//
// Use: public, static
SbBool
ChemOverrideElement::getChemRadiiOverride(SoState *state)
//
////////////////////////////////////////////////////////////////////////
{
    const ChemOverrideElement *elt;
    elt = (const ChemOverrideElement *)
        getConstElement(state, classStackIndex);
    return elt->flags & CHEM_RADII;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Set ChemDisplayParamElement override flag
//
// Use: public, static
void
ChemOverrideElement::setChemDisplayParamOverride(SoState *state,
                                                 SbBool override)
//
////////////////////////////////////////////////////////////////////////
{
    ChemOverrideElement    *elt;
    elt = (ChemOverrideElement *)getElement(state, classStackIndex);
    if (override)
        elt->flags |= CHEM_DISPLAYPARAM;
    else
        elt->flags &= ~CHEM_DISPLAYPARAM;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Set ChemColorElement override flag
//
// Use: public, static
void
ChemOverrideElement::setChemColorOverride(SoState *state,
                                          SbBool override)
//
////////////////////////////////////////////////////////////////////////
{
    ChemOverrideElement    *elt;
    elt = (ChemOverrideElement *)getElement(state, classStackIndex);
    if (override)
        elt->flags |= CHEM_COLOR;
    else
        elt->flags &= ~CHEM_COLOR;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Set ChemRadiiElement override flag
//
// Use: public, static
void
ChemOverrideElement::setChemRadiiOverride(SoState *state,
                                          SbBool override)
//
////////////////////////////////////////////////////////////////////////
{
    ChemOverrideElement    *elt;
    elt = (ChemOverrideElement *)getElement(state, classStackIndex);
    if (override)
        elt->flags |= CHEM_RADII;
    else
        elt->flags &= ~CHEM_RADII;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//     Create a copy of this instance suitable for calling matches()
//     on.
//
// Use: protected

SoElement *
ChemOverrideElement::copyMatchInfo() const
//
////////////////////////////////////////////////////////////////////////
{
    ChemOverrideElement *result =
        (ChemOverrideElement *)getTypeId().createInstance();

    result->flags = flags;

    return result;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Returns TRUE if elements match
//
// Use: public

SbBool
ChemOverrideElement::matches(const SoElement *elt) const
//
////////////////////////////////////////////////////////////////////////
{
    return (flags == ((const ChemOverrideElement *) elt)->flags);
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
ChemOverrideElement::print(FILE *fp) const
{
    pFlag(fp, "CHEM_DISPLAYPARAM", CHEM_DISPLAYPARAM);
    pFlag(fp, "CHEM_COLOR", CHEM_COLOR);
    pFlag(fp, "CHEM_RADII", CHEM_RADII);
    putc('\n', fp);
}
#else  /* DEBUG */
void
ChemOverrideElement::print(FILE *) const
{
}
#endif /* DEBUG */

void
ChemOverrideElement::pFlag(FILE *fp, const char *flagName,
                                int flagBit) const
{
    if (flags & flagBit)
        fprintf(fp, "%s\t", flagName);
}
