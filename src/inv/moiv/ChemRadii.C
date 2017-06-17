/*
 * ChemRadii.c++
 *
 *     Classes:  ChemRadii
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

#include "inv/ChemKit/ChemRadii.H"
#include "inv/ChemKit/ChemRadiiElement.H"
#include "inv/ChemKit/ChemOverrideElement.H"

const float defaultAtomRadii[] =
    { 1.00f, // Place holder
      1.08f, // Hydrogen     
      1.00f, // Helium       
      1.00f, // Lithium      
      1.00f, // Beryllium    
      1.00f, // Boron        
      1.54f, // Carbon       
      1.48f, // Nitrogen     
      1.36f, // Oxygen       
      1.30f, // Fluorine     
      1.00f, // Neon         
      2.30f, // Sodium       
      1.00f, // Magnesium    
      2.86f, // Aluminum     
      2.10f, // Silicon      
      1.75f, // Phosphorus   
      1.70f, // Sulfur       
      1.65f, // Chlorine     
      1.00f, // Argon        
      1.00f, // Potassium    
      2.75f, // Calcium      
      1.00f, // Scandium     
      1.00f, // Titanium     
      1.00f, // Vanadium     
      1.00f, // Chromium     
      1.00f, // Manganese    
      2.27f, // Iron         
      1.00f, // Cobalt       
      1.00f, // Nickel       
      1.40f, // Copper       
      1.40f, // Zinc         
      1.00f, // Gallium      
      1.00f, // Germanium    
      1.00f, // Arsenic      
      1.00f, // Selenium     
      1.80f, // Bromium      
      1.00f, // Krypton      
      1.00f, // Rubidium     
      1.00f, // Strontium    
      1.00f, // Yttrium      
      1.00f, // Zirconium    
      1.00f, // Niobium      
      1.00f, // Molybdenum   
      1.00f, // Technetium   
      1.00f, // Ruthenium    
      1.00f, // Rhodium      
      1.00f, // Palladium    
      1.00f, // Silver       
      1.00f, // Cadmium      
      1.00f, // Indium       
      1.00f, // Tin          
      1.00f, // Antimony     
      1.00f, // Tellurium    
      2.00f, // Iodine       
      1.00f, // Xenon        
      1.00f, // Cesium       
      1.00f, // Barium       
      1.00f, // Lanthanum    
      1.00f, // Cerium       
      1.00f, // Praseodymium 
      1.00f, // Neodymium    
      1.00f, // Promethium   
      1.00f, // Samarium     
      1.00f, // Europium     
      1.00f, // Gadolinium   
      1.00f, // Terbium      
      1.00f, // Dysprosium   
      1.00f, // Holmium      
      1.00f, // Erbium       
      1.00f, // Thulium      
      1.00f, // Ytterbium    
      1.00f, // Lutetium     
      1.00f, // Hafnium      
      1.00f, // Tantalum     
      1.00f, // Tungsten     
      1.00f, // Rhenium      
      1.00f, // Osmium       
      1.00f, // Iridium      
      1.00f, // Platinum     
      1.00f, // Gold         
      1.00f, // Mercury      
      1.00f, // Thallium     
      1.00f, // Lead         
      1.00f, // Bismuth      
      1.00f, // Polonium     
      1.00f, // Astatine     
      1.00f, // Radon        
      1.00f, // Francium     
      1.00f, // Radium       
      1.00f, // Actinium     
      1.00f, // Thorium      
      1.00f, // Protactinium 
      1.00f, // Uranium      
      1.00f, // Neptunium    
      1.00f, // Plutonium    
      1.00f, // Americium    
      1.00f, // Curium       
      1.00f, // Berkelium    
      1.00f, // Californium  
      1.00f, // Einsteinium  
      1.00f, // Fermium      
      1.00f, // Mendelevium  
      1.00f, // Nobelium     
      1.00f  // Lawrencium 
};

SO_NODE_SOURCE(ChemRadii);


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Constructor
//
// Use: public

ChemRadii::ChemRadii()
//
////////////////////////////////////////////////////////////////////////
{

    SO_NODE_CONSTRUCTOR(ChemRadii);

    SO_NODE_ADD_FIELD(atomRadiiBinding, (ChemRadii::RADII_DEFAULT));
    SO_NODE_ADD_FIELD(atomRadii, (0));
    atomRadii.deleteValues(0);
    atomRadii.setValues(0, 104, defaultAtomRadii);
    atomRadii.setDefault(TRUE);

    SO_NODE_DEFINE_ENUM_VALUE(RadiiBinding, RADII_OVERALL);
    SO_NODE_DEFINE_ENUM_VALUE(RadiiBinding, RADII_PER_ATOM);
    SO_NODE_DEFINE_ENUM_VALUE(RadiiBinding, RADII_PER_ATOM_INDEXED);
    SO_NODE_DEFINE_ENUM_VALUE(RadiiBinding, RADII_DEFAULT);
    
    SO_NODE_SET_SF_ENUM_TYPE(atomRadiiBinding, RadiiBinding);

    isBuiltIn = TRUE;

}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Destructor
//
// Use: private

ChemRadii::~ChemRadii()
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
ChemRadii::doAction(SoAction *action)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef ORIG
    ChemRadiiElement::set(action->getState(), this, this);
#else
    SoState *state = action->getState();
    if (!ChemOverrideElement::getChemRadiiOverride(state)) {
        if (isOverride()) {
            ChemOverrideElement::setChemRadiiOverride(state, TRUE);
        }
        ChemRadiiElement::set(state, this, this);
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
ChemRadii::GLRender(SoGLRenderAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    ChemRadii::doAction(action);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Handles get bounding box action.
//
// Use: extender

void
ChemRadii::getBoundingBox(SoGetBoundingBoxAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    ChemRadii::doAction(action);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Does callback action thing.
//
// Use: extender

void
ChemRadii::callback(SoCallbackAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    ChemRadii::doAction(action);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Does pick action...
//
// Use: extender

void
ChemRadii::pick(SoPickAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    ChemRadii::doAction(action);
}
