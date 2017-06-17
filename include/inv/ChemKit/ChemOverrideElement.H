/*
 * ChemOverrideElement.h
 *
 *  This file defines the ChemOverrideElement node class
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

#ident "$Revision: 22148 $"

#ifndef  __CHEM_OVERRIDE_ELEMENT_H__
#define  __CHEM_OVERRIDE_ELEMENT_H__

#include "inv/elements/SoSubElement.H"

#include "inv/ChemKit/ChemkitBasic.H"

//////////////////////////////////////////////////////////////////////////////
//
//  Class: ChemOverrideElement
//
//  Element that stores a flag for each type of element which can be
//  overridden.  Nodes implement override by setting the appropriate
//  bit if their override flag is on, and ignoring overridden elements
//  if the corresponding bit in the state's ChemOverrideElement is set.
//  
//////////////////////////////////////////////////////////////////////////////

SoEXTENDER class CHEMKIT_DLL_API ChemOverrideElement : public SoElement {

    SO_ELEMENT_HEADER(ChemOverrideElement);

    enum {
        CHEM_DISPLAYPARAM = 0x1,
        CHEM_COLOR        = 0x2,
        CHEM_RADII        = 0x3,
    };

  public:
    // Initializes element
    virtual void        init(SoState *state);

    // Comparison based on the nodeId and the value of "data"
    virtual SbBool      matches(const SoElement *elt) const;

    // Overrides push() method to copy values from next instance in
    // the stack, and set up cache dependencies correctly
    virtual void        push(SoState *state);

    //
    // "get" methods for each element which can be overridden.
    //

    // Returns TRUE iff ChemDisplayParamElement is overridden.
    static SbBool       getChemDisplayParamOverride(SoState *state);

    // Returns TRUE iff ChemColorElement is overridden.
    static SbBool       getChemColorOverride(SoState *state);

    // Returns TRUE iff ChemRadiiElement is overridden.
    static SbBool       getChemRadiiOverride(SoState *state);

    //
    // "set" methods for each element which can be overridden.
    //

    // set override flag for ChemDisplayParamElement.
    static void         setChemDisplayParamOverride(SoState *state,
                                                    SbBool override);
    
    // set override flag for ChemColorElement.
    static void         setChemColorOverride(SoState *state,
                                             SbBool override);
    
    // set override flag for ChemRadiiElement.
    static void         setChemRadiiOverride(SoState *state,
                                             SbBool override);

    // Prints element (for debugging)
    virtual void        print(FILE *fp) const;

  SoINTERNAL public:
    // Initializes the SoTextureOverrideElement class
    static void         initClass();

    // Copy method, copies flags
    virtual SoElement   *copyMatchInfo() const;
    
  private:
    // Used by ::print method
    void pFlag(FILE *, const char *, int) const;

    uint32_t flags;
};

#endif /* !__CHEM_OVERRIDE_ELEMENT_H__ */
