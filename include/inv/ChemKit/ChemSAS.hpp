/*
 * ChemSAS.h
 Implements a Solvent Accessible Surface
 *
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

#ifndef __CHEM_SAS_H__
#define __CHEM_SAS_H__

#include "inv/nodes/SoVertexShape.H"

#include "inv/fields/SoMFInt32.H"
#include "inv/fields/SoMFVec3f.H"
#include "inv/fields/SoMFColor.H"
#include "inv/fields/SoSFColor.H"
#include "inv/fields/SoSFFloat.H"

#include "inv/ChemKit/ChemIso.H"
#include "inv/ChemKit/ChemLattice3.H"
#include "inv/ChemKit/ChemkitBasic.H"

class ChemBaseDataElement;
class ChemColorElement;
class ChemRadiiElement;
class SoGLRenderAction;
class SoRayPickAction;
class SoAction;
class SbBox3f;
class SbVec3f;


class CHEMKIT_DLL_API ChemSAS: public SoVertexShape
{
  SO_NODE_HEADER(ChemSAS);

  public:

    // Fields:
		
/*
    enum ColorBinding {
        OVERALL,
        BY_ATOM,
        BY_SURFACE,
        DEFAULT = OVERALL,
    };
*/

    // Constructor
    ChemSAS();

  SoEXTENDER public:
    virtual void    GLRender(SoGLRenderAction *action);
    virtual void    rayPick(SoRayPickAction *action);
		virtual void  notify (SoNotList *nl);

  SoINTERNAL public:
    static void initClass();

  protected:
    virtual void    generatePrimitives(SoAction *action);
    virtual void    computeBBox(SoAction *action, SbBox3f &box,
                                SbVec3f &center);
    virtual ~ChemSAS();

// extra methods

	public:
		// Add an atom index to the array
		void addAtomIndex(const int32_t index);

		// Remove array[index] value
		void removeAtomIndex(const int32_t index);

		// Get the atom index
		int32_t getAtomIndex(const int32_t index) const;

		// Set a new atom index to position 'index'
		void setAtomIndex(const int32_t index, const int32_t newAtomIndex);
	
		const SoMFInt32& getAtomIndices() const;


	protected:

		void CalcSurface(SoAction *action);
		// data
		SoMFInt32 atomIndices; // Storage for atom indices
		ChemIso *iso;
		ChemLattice3 *lattice;
		ChemLattice3 *color;


	private:

    ChemBaseDataElement  *lastChemBaseDataElement;
    ChemColorElement     *lastChemColorElement;
    ChemRadiiElement     *lastChemRadiiElement;

	  SbBool regenerateFlag;

    void regenerate(SoGLRenderAction *action);

};

#endif  /* !__CHEM_SAS_H__ */
