/*
 * ChemColor.h
 *
 *	This file defines the ChemColor node class
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

#ifndef  __CHEM_COLOR_H__
#define  __CHEM_COLOR_H__

#include "inv/fields/SoSFEnum.H"
#include "inv/fields/SoMFColor.H"
#include "inv/nodes/SoSubNode.H"

#include "inv/ChemKit/ChemkitBasic.H"

//////////////////////////////////////////////////////////////////////////////
//
//  Class: ChemColor
//
//  SoNode class that manages atom and bond colors for chemical displays
//
//////////////////////////////////////////////////////////////////////////////

class CHEMKIT_DLL_API ChemColor : public SoNode {

    SO_NODE_HEADER(ChemColor);

  public:

    // Fields:
    SoSFEnum        atomColorBinding;   // how to index into the colors
    SoMFColor       atomColor;          // colors to use for atoms
    SoSFEnum        bondColorBinding;   // how to index into the colors
    SoMFColor       bondColor;          // colors to use for bonds
    SoSFEnum        hydrogenBondColorBinding;  // how to index into the colors
    SoMFColor       hydrogenBondColor;         // colors to use for hbonds
    SoSFEnum        atomLabelColorBinding;   // how to index into the colors
    SoMFColor       atomLabelColor;          // colors to use for atom labels
    SoSFEnum        bondLabelColorBinding;   // how to index into the colors
    SoMFColor       bondLabelColor;          // colors to use for bond labels
// -->CAstick
		SoSFEnum        residueColorBinding;   // how to color residues
    SoMFColor       residueColor;          // colors to use for residues
		SoSFEnum        residueLabelColorBinding; // how to color residue labels
    SoMFColor       residueLabelColor;        // colors to use for residue labels
// <--CAstick

// --> schematic
		SoSFEnum        schematicColorBinding; // how to color schematic
		SoMFColor       schematicColor;          // colors to use for residues
// <-- schematic

    enum AtomBinding {
        ATOM_OVERALL,
        ATOM_PER_ATOM,
        ATOM_PER_ATOM_INDEXED,
        ATOM_DEFAULT = ATOM_PER_ATOM_INDEXED
    };

    enum BondBinding {
        BOND_OVERALL,
        BOND_PER_ATOM,
        BOND_PER_ATOM_HALF_BONDED,
        BOND_PER_BOND,
        BOND_PER_BOND_INDEXED,
        BOND_DEFAULT = BOND_PER_ATOM_HALF_BONDED
    };

    enum HydrogenBondBinding {
        HBOND_OVERALL,
        HBOND_PER_BOND,
        HBOND_PER_HBOND,
        HBOND_PER_HBOND_INDEXED,
        HBOND_DEFAULT = HBOND_OVERALL
    };

// -->CAstick
    enum ResidueBinding {
        RESIDUE_OVERALL,
        RESIDUE_PER_INDEX,
        RESIDUE_PER_CHAIN,
        RESIDUE_DEFAULT = RESIDUE_OVERALL
    };
// <--CAstick

// --> schematic
		 enum SchematicBinding {
			SCHEMATIC_OVERALL,
			SCHEMATIC_PER_SECONDARY_TYPE,
			SCHEMATIC_PER_ELEMENT,
			SCHEMATIC_DEFAULT = SCHEMATIC_PER_SECONDARY_TYPE
		};
// <-- schematic
    enum AtomLabelBinding {
        ATOM_LABEL_OVERALL,
        ATOM_LABEL_PER_ATOM,
        ATOM_LABEL_PER_ATOM_LABEL,
        ATOM_LABEL_PER_ATOM_LABEL_INDEXED,
        ATOM_LABEL_DEFAULT = ATOM_LABEL_OVERALL
    };

    enum BondLabelBinding {
        BOND_LABEL_OVERALL,
        BOND_LABEL_PER_BOND,
        BOND_LABEL_PER_BOND_INDEXED,
        BOND_LABEL_PER_BOND_LABEL,
        BOND_LABEL_PER_BOND_LABEL_INDEXED,
        BOND_LABEL_DEFAULT = BOND_LABEL_OVERALL
    };

// -->CAstick
    enum ResidueLabelBinding {
        RESIDUE_LABEL_OVERALL,
        RESIDUE_LABEL_PER_INDEX,
        RESIDUE_LABEL_PER_CHAIN,
        RESIDUE_LABEL_DEFAULT = RESIDUE_LABEL_OVERALL
    };
// <--CAstick


    // Constructor
    ChemColor();

  SoEXTENDER public:
    virtual void    doAction(SoAction *action);
    virtual void    GLRender(SoGLRenderAction *action);
    virtual void    getBoundingBox(SoGetBoundingBoxAction *action);
    virtual void    callback(SoCallbackAction *action);
    virtual void    pick(SoPickAction *action);

  SoINTERNAL public:
    static void     initClass();

  protected:
    virtual ~ChemColor();

};

#endif  /* !__CHEM_COLOR_H__ */
