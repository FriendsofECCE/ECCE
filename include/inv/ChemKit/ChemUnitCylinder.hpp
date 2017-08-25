/*
 * ChemUnitCylinder.h
 *
 *     ChemUnitCylinder class
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

#ifndef  __CHEM_UNITCYLINDER_H__
#define  __CHEM_UNITCYLINDER_H__

#include "inv/SbLinear.H"
#include "inv/SbColor.H"
// --> EGB && SGB
#ifdef WIN32
#include <windows.h>
#endif
#ifndef __glu_h__
#include <GL/glu.h>
#endif /* __glu_h__ */
#include "inv/nodes/SoCylinder.H"
#include "inv/actions/SoGLRenderAction.H"

#include "inv/ChemKit/ChemDisplayParam.H"
// --> roundcap optimization
#include "inv/ChemKit/MFVec2i.H"
class ChemDisplay;
// <-- roundcap optimization
// <-- EGB && SGB


//////////////////////////////////////////////////////////////////////////////
//
//  Class: ChemUnitCylinder
//
//  Generates the vertices of a unit sphere.  Make Open GL rendering
//  calls to display the sphere.  
//
//////////////////////////////////////////////////////////////////////////////

class ChemUnitCylinder {
  public:

    enum CapType {
        NONE,
        FLATCAP,
        ROUNDCAP
    };

    ChemUnitCylinder(short capType, float complexity = 0.4);
    ~ChemUnitCylinder();

// --> EGB &&	SGB
//    void render(const SbVec3f &from, const SbVec3f &to, const float radius,
//        const SbBool doNormals, const SbBool doTextures) const;
		  void render(const SbVec3f &from, const SbVec3f &to, const float radius,
        const SbBool doNormals, const SbBool doTextures,
				int bondCylinderDisplayStyle,
				const SbVec2i fromTo,
				const ChemDisplay *cd, 
				SoGLRenderAction *action = NULL) const;
// <-- EGB && SGB

// --> EGB && SGB
//    void renderHalfBonded(const SbVec3f &from, const SbVec3f &to,
//        const float radius, const SbColor &colorFrom, const SbColor &colorTo,
//        const SbBool doNormals, const SbBool doTextures) const;
    void renderHalfBonded(const SbVec3f &from, const SbVec3f &to,
        const float radius, const SbColor &colorFrom, const SbColor &colorTo,
        const SbBool doNormals, const SbBool doTextures,
				int bondCylinderDisplayStyle, const SbVec2i fromTo,
				const ChemDisplay* cd, SoGLRenderAction *action) const;
// <-- EGB && SGB

// --> EGB && SGB
//    void renderWithInterpolatedColor(const SbVec3f &from, const SbVec3f &to,
//        const float radius, const SbColor &colorFrom, const SbColor &colorTo,
//        const SbBool doNormals, const SbBool doTextures) const;
    void renderWithInterpolatedColor(const SbVec3f &from, const SbVec3f &to,
        const float radius, const SbColor &colorFrom, const SbColor &colorTo,
        const SbBool doNormals, const SbBool doTextures,
				int bondCylinderDisplayStyle,
				const SbVec2i fromTo,
				const ChemDisplay *cd,
				SoGLRenderAction* action) const;
// <-- EGB && SGB

// --> EGB && SGB
		void renderCylinder(const SbMatrix &theVertexMatrix, 
			const SbBool doCaps, const SbBool doNormals, const SbBool doTextures,
			const int bondCylinderDisplayStyle,
			SoGLRenderAction *action) const;

		void renderRoundCap(const SbMatrix &theVertexMatrix,
			const SbBool bottom, const SbBool doNormals, const SbBool doTextures,
			const int numSections, const int numSides,
			const int cylinderBottomStartNorm, const int cylinderBottomStartVert,
			const SbVec3f *cylinderCapNormals, const SbVec3f *cylinderCapVertices,
			int bondCylinderDisplayStyle = ChemDisplayParam::BONDCYLINDER_ROUNDCAP) const;
// <-- EGB && SGB

    int32_t getCapType() const { return capType; }
    int32_t getNumberOfSections() const { return numSections; }
    int32_t getNumberOfSides() const { return numSides; }
    int32_t getNumberOfRoundCapSections() const { return numRoundCapSections; }

// --> EGB && SGB
    int32_t getNumberOfGLUSlices() const { return GLUslices; }
    int32_t getNumberOfGLUStacks() const { return GLUstacks; }
		const SbVec3f *getArrayCylinderNormals() const { return arrayCylNormals; }
		const SbVec3f *getSemiCylinderVertices() const { return semiCylVertices; }
		const SbVec3f *getSemiCylinderNormals() const { return semiCylNormals; }
// <-- EGB && SGB

    int32_t getStartOfBottomNormals() const { return cylBottomStartNorm; }
    int32_t getStartOfBottomVertices() const { return cylBottomStartVert; }

    int32_t getNumberOfRingCoords() const { return numSides; }
    int32_t getNumberOfCylinderNormals() const { return cylNormCount; }
    int32_t getNumberOfCylinderVertices() const { return cylVertCount; }
    int32_t getNumberOfRoundCapNormals() const { return roundCapNormCount; }
    int32_t getNumberOfRoundCapVertices() const { return roundCapVertCount; }

    const SbVec2f *getRingCoords() const { return ringCoords; }
    const SbVec3f *getCylinderNormals() const { return cylNormals; }
    const SbVec3f *getCylinderVertices() const { return cylVertices; }
    const SbVec3f *getRoundCapNormals() const { return roundCapNormals; }
    const SbVec3f *getRoundCapVertices() const { return roundCapVertices; }


// --> EGB && SGB
// --> semicylinders
		// used to know how orient semicylinders
		bool perspective;
		// used to orient semicylinders without perspective
		SbVec3f lookAtVector;
		// used to orient semicylinders with perspective
		SbVec3f camPos;
// <-- semicylinders

		GLUquadricObj *getCylinderQuadric() const { return	cylinder;}
		GLUquadricObj *getTopCapQuadric() const { return	topCap;}
		GLUquadricObj *getBottomCapQuadric() const{ return	bottomCap;}

	void generateCylTexCoords(SbBool doCaps);
private:

	// cylinder array vertices count
	int32_t cylArrayVertCount;
	// cylinder flat caps array vertices count
	int32_t cylCapArrayVertCount;

	// cylinder round caps array vertices count
	int32_t cylRoundCapArrayVertCount;

	// cylinder array cap index
	int32_t cylTopArrayStartIndex;
	
// semicylinder stuff
	int32_t semiCylVertCount;
	
	void generateSemiCylVertices();
// <-- EGB && SGB

private:

    short   capType;

    int32_t numSections;
    int32_t numSides;
    int32_t numRoundCapSections;


    int32_t cylBottomStartNorm;
    int32_t cylBottomStartVert;
    int32_t cylNormCount;
    int32_t cylVertCount;
    int32_t roundCapNormCount;
    int32_t roundCapVertCount;

    SbVec2f *ringCoords;
    SbVec3f *cylNormals;
    SbVec3f *cylVertices;
    SbVec3f *roundCapNormals;
    SbVec3f *roundCapVertices;

// --> EGB && SGB
		SbVec3f *semiCylVertices;
		SbVec3f *semiCylNormals;

		SbVec3f *arrayCylNormals;
		SbVec2f *sidesTexCoords;
		SbVec2f *topCapTexCoords;
		SbVec2f *bottomCapTexCoords;
		SbVec2f *roundCapTexCoords;
		
		int32_t GLUslices;
		int32_t GLUstacks;
		GLUquadricObj *cylinder;
		GLUquadricObj *topCap;
		GLUquadricObj *bottomCap;
		SoCylinder *soCylinder;

		bool cylTexCoordsGenerated;
// <-- EGB && SGB
};
#endif /* !__CHEM_UNITCYLINDER_H__ */
