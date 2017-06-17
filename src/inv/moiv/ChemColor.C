/*
 * ChemColor.c++
 *
 *     Classes:  ChemColor
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

#include "inv/ChemKit/ChemColor.H"
#include "inv/ChemKit/ChemColorElement.H"
#include "inv/ChemKit/ChemOverrideElement.H"

const float defaultAtomColors[][3] =  {
    { 1.00f, 1.00f, 1.00f}, // Place holder
    { 1.00f, 1.00f, 1.00f}, // Hydrogen     
    { 1.00f, 1.00f, 1.00f}, // Helium       
    { 0.70f, 0.30f, 0.00f}, // Lithium      
    { 0.40f, 1.00f, 0.33f}, // Beryllium    
    { 0.36f, 0.63f, 1.00f}, // Boron        
    { 0.50f, 0.50f, 0.50f}, // Carbon       
    { 0.00f, 0.00f, 1.00f}, // Nitrogen     
    { 1.00f, 0.00f, 0.00f}, // Oxygen       
    { 0.70f, 0.30f, 0.90f}, // Fluorine     
    { 1.00f, 1.00f, 1.00f}, // Neon         
    { 0.70f, 0.30f, 0.00f}, // Sodium       
    { 0.40f, 1.00f, 0.33f}, // Magnesium    
    { 0.36f, 0.63f, 1.00f}, // Aluminum     
    { 0.00f, 0.20f, 1.00f}, // Silicon      
    { 0.00f, 1.00f, 0.00f}, // Phosphorus   
    { 0.90f, 0.90f, 0.00f}, // Sulfur       
    { 0.90f, 0.90f, 0.00f}, // Chlorine     
    { 1.00f, 1.00f, 1.00f}, // Argon        
    { 0.70f, 0.30f, 0.00f}, // Potassium    
    { 0.40f, 1.00f, 0.33f}, // Calcium      
    { 0.90f, 0.90f, 0.00f}, // Scandium     
    { 0.90f, 0.90f, 0.00f}, // Titanium     
    { 0.90f, 0.90f, 0.00f}, // Vanadium     
    { 0.90f, 0.90f, 0.00f}, // Chromium     
    { 0.90f, 0.90f, 0.00f}, // Manganese    
    { 0.90f, 0.90f, 0.00f}, // Iron         
    { 0.90f, 0.90f, 0.00f}, // Cobalt       
    { 0.90f, 0.90f, 0.00f}, // Nickel       
    { 0.90f, 0.90f, 0.00f}, // Copper       
    { 0.90f, 0.90f, 0.00f}, // Zinc         
    { 0.36f, 0.63f, 1.00f}, // Gallium      
    { 0.00f, 0.20f, 1.00f}, // Germanium    
    { 0.70f, 0.30f, 0.00f}, // Arsenic      
    { 0.00f, 0.60f, 0.00f}, // Selenium     
    { 0.70f, 0.30f, 0.90f}, // Bromium      
    { 1.00f, 1.00f, 1.00f}, // Krypton      
    { 0.70f, 0.30f, 0.00f}, // Rubidium     
    { 0.40f, 1.00f, 0.33f}, // Strontium    
    { 0.00f, 0.60f, 0.00f}, // Yttrium      
    { 0.00f, 0.60f, 0.00f}, // Zirconium    
    { 0.00f, 0.60f, 0.00f}, // Niobium      
    { 0.00f, 0.60f, 0.00f}, // Molybdenum   
    { 0.00f, 0.60f, 0.00f}, // Technetium   
    { 0.00f, 0.60f, 0.00f}, // Ruthenium    
    { 0.00f, 0.60f, 0.00f}, // Rhodium      
    { 0.00f, 0.60f, 0.00f}, // Palladium    
    { 0.00f, 0.60f, 0.00f}, // Silver       
    { 0.00f, 0.60f, 0.00f}, // Cadmium      
    { 0.36f, 0.63f, 1.00f}, // Indium       
    { 0.20f, 0.20f, 0.20f}, // Tin          
    { 0.70f, 0.30f, 0.00f}, // Antimony     
    { 0.00f, 0.60f, 0.00f}, // Tellurium    
    { 0.70f, 0.30f, 0.90f}, // Iodine       
    { 1.00f, 1.00f, 1.00f}, // Xenon        
    { 0.70f, 0.30f, 0.00f}, // Cesium       
    { 0.40f, 1.00f, 0.33f}, // Barium       
    { 0.90f, 0.90f, 0.00f}, // Lanthanum    
    { 0.00f, 0.60f, 0.00f}, // Cerium       
    { 0.00f, 0.60f, 0.00f}, // Praseodymium 
    { 0.00f, 0.60f, 0.00f}, // Neodymium    
    { 0.00f, 0.60f, 0.00f}, // Promethium   
    { 0.00f, 0.60f, 0.00f}, // Samarium     
    { 0.00f, 0.60f, 0.00f}, // Europium     
    { 0.00f, 0.60f, 0.00f}, // Gadolinium   
    { 0.00f, 0.60f, 0.00f}, // Terbium      
    { 0.00f, 0.60f, 0.00f}, // Dysprosium   
    { 0.00f, 0.60f, 0.00f}, // Holmium      
    { 0.00f, 0.60f, 0.00f}, // Erbium       
    { 0.00f, 0.60f, 0.00f}, // Thulium      
    { 0.00f, 0.60f, 0.00f}, // Ytterbium    
    { 0.00f, 0.60f, 0.00f}, // Lutetium     
    { 0.90f, 0.90f, 0.00f}, // Hafnium      
    { 0.90f, 0.90f, 0.00f}, // Tantalum     
    { 0.90f, 0.90f, 0.00f}, // Tungsten     
    { 0.90f, 0.90f, 0.00f}, // Rhenium      
    { 0.90f, 0.90f, 0.00f}, // Osmium       
    { 0.90f, 0.90f, 0.00f}, // Iridium      
    { 0.90f, 0.90f, 0.00f}, // Platinum     
    { 0.90f, 0.90f, 0.00f}, // Gold         
    { 0.90f, 0.90f, 0.00f}, // Mercury      
    { 0.36f, 0.63f, 1.00f}, // Thallium     
    { 0.20f, 0.20f, 0.20f}, // Lead         
    { 0.70f, 0.30f, 0.00f}, // Bismuth      
    { 0.00f, 0.60f, 0.00f}, // Polonium     
    { 0.70f, 0.30f, 0.90f}, // Astatine     
    { 1.00f, 1.00f, 1.00f}, // Radon        
    { 0.70f, 0.30f, 0.00f}, // Francium     
    { 0.40f, 1.00f, 0.33f}, // Radium       
    { 0.90f, 0.90f, 0.00f}, // Actinium     
    { 0.70f, 0.30f, 0.00f}, // Thorium      
    { 0.70f, 0.30f, 0.00f}, // Protactinium 
    { 0.70f, 0.30f, 0.00f}, // Uranium      
    { 0.70f, 0.30f, 0.00f}, // Neptunium    
    { 0.70f, 0.30f, 0.00f}, // Plutonium    
    { 0.70f, 0.30f, 0.00f}, // Americium    
    { 0.70f, 0.30f, 0.00f}, // Curium       
    { 0.70f, 0.30f, 0.00f}, // Berkelium    
    { 0.70f, 0.30f, 0.00f}, // Californium  
    { 0.70f, 0.30f, 0.00f}, // Einsteinium  
    { 0.70f, 0.30f, 0.00f}, // Fermium      
    { 0.70f, 0.30f, 0.00f}, // Mendelevium  
    { 0.70f, 0.30f, 0.00f}, // Nobelium     
    { 0.70f, 0.30f, 0.00f}  // Lawrencium   
};

SO_NODE_SOURCE(ChemColor);


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Constructor
//
// Use: public

ChemColor::ChemColor()
//
////////////////////////////////////////////////////////////////////////
{

    SO_NODE_CONSTRUCTOR(ChemColor);

    SO_NODE_ADD_FIELD(atomColor, (1.0, 1.0, 1.0));
    atomColor.deleteValues(0);
    SO_NODE_ADD_FIELD(atomColorBinding, (ATOM_DEFAULT));
    SO_NODE_ADD_FIELD(bondColorBinding, (BOND_DEFAULT));
    SO_NODE_ADD_FIELD(bondColor, (1.0, 1.0, 1.0));
    SO_NODE_ADD_FIELD(hydrogenBondColorBinding, (HBOND_DEFAULT));
    SO_NODE_ADD_FIELD(hydrogenBondColor, (1.0, 1.0, 1.0));
    SO_NODE_ADD_FIELD(atomLabelColorBinding, (ATOM_LABEL_DEFAULT));
    SO_NODE_ADD_FIELD(atomLabelColor, (1.0, 1.0, 1.0));
    SO_NODE_ADD_FIELD(bondLabelColorBinding, (BOND_LABEL_DEFAULT));
    SO_NODE_ADD_FIELD(bondLabelColor, (1.0, 1.0, 1.0));
// --> CA-stick
		SO_NODE_ADD_FIELD(residueColor, (1.0, 1.0, 1.0));
		residueColor.deleteValues(0);
		SO_NODE_ADD_FIELD(residueColorBinding, (RESIDUE_DEFAULT));
		SO_NODE_ADD_FIELD(residueLabelColor, (1.0, 1.0, 1.0));
		SO_NODE_ADD_FIELD(residueLabelColorBinding, (RESIDUE_LABEL_DEFAULT));

		residueColor.setValues(0,104, defaultAtomColors);
		residueColor.setDefault(TRUE);

		residueLabelColor.setValues(0,104, defaultAtomColors);
		residueLabelColor.setDefault(TRUE);
// <-- CA-stick
// --> schematic
		SO_NODE_ADD_FIELD(schematicColor, (1.0, 1.0, 1.0));
		schematicColor.deleteValues(0);
		SO_NODE_ADD_FIELD(schematicColorBinding, (SCHEMATIC_DEFAULT));
		schematicColor.setValues(0,104, defaultAtomColors);
		schematicColor.setDefault(TRUE);
// <-- schematic

    atomColor.setValues(0, 104, defaultAtomColors);

    // Make the values in the atomColor field the default
    atomColor.setDefault(TRUE);

    // Set up static info for enum type fields:

    // Atom color and binding
    SO_NODE_DEFINE_ENUM_VALUE(AtomBinding, ATOM_OVERALL);
    SO_NODE_DEFINE_ENUM_VALUE(AtomBinding, ATOM_PER_ATOM);
    SO_NODE_DEFINE_ENUM_VALUE(AtomBinding, ATOM_PER_ATOM_INDEXED);
    SO_NODE_DEFINE_ENUM_VALUE(AtomBinding, ATOM_DEFAULT);

    SO_NODE_SET_SF_ENUM_TYPE(atomColorBinding, AtomBinding);

    // Bond color and binding
    SO_NODE_DEFINE_ENUM_VALUE(BondBinding, BOND_OVERALL);
    SO_NODE_DEFINE_ENUM_VALUE(BondBinding, BOND_PER_ATOM);
    SO_NODE_DEFINE_ENUM_VALUE(BondBinding, BOND_PER_ATOM_HALF_BONDED);
    SO_NODE_DEFINE_ENUM_VALUE(BondBinding, BOND_PER_BOND);
    SO_NODE_DEFINE_ENUM_VALUE(BondBinding, BOND_PER_BOND_INDEXED);
    SO_NODE_DEFINE_ENUM_VALUE(BondBinding, BOND_DEFAULT);

    SO_NODE_SET_SF_ENUM_TYPE(bondColorBinding, BondBinding);

    // HBond color and binding
    SO_NODE_DEFINE_ENUM_VALUE(HydrogenBondBinding, HBOND_OVERALL);
    SO_NODE_DEFINE_ENUM_VALUE(HydrogenBondBinding, HBOND_PER_BOND);
    SO_NODE_DEFINE_ENUM_VALUE(HydrogenBondBinding, HBOND_PER_HBOND);
    SO_NODE_DEFINE_ENUM_VALUE(HydrogenBondBinding, HBOND_PER_HBOND_INDEXED);
    SO_NODE_DEFINE_ENUM_VALUE(HydrogenBondBinding, HBOND_DEFAULT);

    SO_NODE_SET_SF_ENUM_TYPE(hydrogenBondColorBinding, HydrogenBondBinding);

// --> CA-stick
		// Residue color and binding
    SO_NODE_DEFINE_ENUM_VALUE(ResidueBinding, RESIDUE_OVERALL);
    SO_NODE_DEFINE_ENUM_VALUE(ResidueBinding, RESIDUE_PER_INDEX);
    SO_NODE_DEFINE_ENUM_VALUE(ResidueBinding, RESIDUE_PER_CHAIN);
    SO_NODE_DEFINE_ENUM_VALUE(ResidueBinding, RESIDUE_DEFAULT );

    SO_NODE_SET_SF_ENUM_TYPE(residueColorBinding, ResidueBinding);
// <-- CA-stick

    // Atom label color and binding
    SO_NODE_DEFINE_ENUM_VALUE(AtomLabelBinding, ATOM_LABEL_OVERALL);
    SO_NODE_DEFINE_ENUM_VALUE(AtomLabelBinding, ATOM_LABEL_PER_ATOM);
    SO_NODE_DEFINE_ENUM_VALUE(AtomLabelBinding, ATOM_LABEL_PER_ATOM_LABEL);
    SO_NODE_DEFINE_ENUM_VALUE(AtomLabelBinding, 
                              ATOM_LABEL_PER_ATOM_LABEL_INDEXED);
    SO_NODE_DEFINE_ENUM_VALUE(AtomLabelBinding, ATOM_LABEL_DEFAULT);

    SO_NODE_SET_SF_ENUM_TYPE(atomLabelColorBinding, AtomLabelBinding);

    // Bond label color and binding
    SO_NODE_DEFINE_ENUM_VALUE(BondLabelBinding, BOND_LABEL_OVERALL);
    SO_NODE_DEFINE_ENUM_VALUE(BondLabelBinding, BOND_LABEL_PER_BOND);
    SO_NODE_DEFINE_ENUM_VALUE(BondLabelBinding, BOND_LABEL_PER_BOND_INDEXED);
    SO_NODE_DEFINE_ENUM_VALUE(BondLabelBinding, BOND_LABEL_PER_BOND_LABEL);
    SO_NODE_DEFINE_ENUM_VALUE(BondLabelBinding, 
                              BOND_LABEL_PER_BOND_LABEL_INDEXED);
    SO_NODE_DEFINE_ENUM_VALUE(BondLabelBinding, BOND_LABEL_DEFAULT);

    SO_NODE_SET_SF_ENUM_TYPE(bondLabelColorBinding, BondLabelBinding);


	// --> CA-stick
		// Residue color and binding
    SO_NODE_DEFINE_ENUM_VALUE(ResidueLabelBinding, RESIDUE_LABEL_OVERALL);
    SO_NODE_DEFINE_ENUM_VALUE(ResidueLabelBinding, RESIDUE_LABEL_PER_INDEX);
    SO_NODE_DEFINE_ENUM_VALUE(ResidueLabelBinding, RESIDUE_LABEL_PER_CHAIN);
    SO_NODE_DEFINE_ENUM_VALUE(ResidueLabelBinding, RESIDUE_LABEL_DEFAULT );

    SO_NODE_SET_SF_ENUM_TYPE(residueLabelColorBinding, ResidueLabelBinding);
// <-- CA-stick


    isBuiltIn = TRUE;

}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Destructor
//
// Use: private

ChemColor::~ChemColor()
//
////////////////////////////////////////////////////////////////////////
{
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Handles any action derived from SoAction.
//
// Use: extender

void
ChemColor::doAction(SoAction *action)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef ORIG
    ChemColorElement::set(action->getState(), this, this);
#else
    SoState *state = action->getState();
    if (!ChemOverrideElement::getChemColorOverride(state)) {
        if (isOverride()) {
            ChemOverrideElement::setChemColorOverride(state, TRUE);
        }
        ChemColorElement::set(state, this, this);
    }
#endif
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Does GL render action.
//
// Use: extender

void
ChemColor::GLRender(SoGLRenderAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    ChemColor::doAction(action);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Handles get bounding box action.
//
// Use: extender

void
ChemColor::getBoundingBox(SoGetBoundingBoxAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    ChemColor::doAction(action);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Does callback action thing.
//
// Use: extender

void
ChemColor::callback(SoCallbackAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    ChemColor::doAction(action);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Does pick action...
//
// Use: extender

void
ChemColor::pick(SoPickAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    ChemColor::doAction(action);
}
