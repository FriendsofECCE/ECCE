/*
 * ChemUnitSphere.h
 *
 *     ChemUnitSphere class
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

#ifndef  __CHEM_UNITSPHERE_H__
#define  __CHEM_UNITSPHERE_H__

#include "inv/SbLinear.H"
// --> EGB && SGB
#ifdef WIN32
#include <windows.h>
#endif
#ifndef __glu_h__
#include <GL/glu.h>
#endif /* __glu_h__ */

#include "inv/fields/SoSFEnum.H"
#include "inv/nodes/SoSphere.H"
#include "inv/actions/SoGLRenderAction.H"
// <-- EGB && SGB

#include "inv/ChemKit/ChemDisplayParam.H"

//////////////////////////////////////////////////////////////////////////////
//
//  Class: ChemUnitSphere
//
//  Generates the vertices of a unit sphere.  Make Open GL rendering
//  calls to display the sphere.  
//
//////////////////////////////////////////////////////////////////////////////

class ChemUnitSphere {
  public:
    ChemUnitSphere(float complexity = 0.4);
    ~ChemUnitSphere();

// --> EGB && SGB
//    void generateDisplay(SbBool doHemiSpheres, SbBool doNormals,
//        SbBool doTextures) const;
		void generateDisplay(SbBool doHemiSpheres, SbBool doNormals, 
			SbBool doTextures, 
			int atomSphereDisplayStyle = ChemDisplayParam::ATOMSPHERE_DEFAULT,
			SoGLRenderAction* action = NULL);
// <-- EGB && SGB

    int32_t getDepth() const { return depth; }
// --> EGB && SGB
		//
		SbVec3f a,b,c,d,normala,normalb,normalc,normald;
		//
		int32_t getGLUDepth() const { return GLUDepth; }
		GLuint getSphereTextureId() const { return sphereTexture;} 
		const SbVec2f *getTexCoords() const { return sphereTexCoords; }
		GLUquadricObj *getQuadric() const { return qobj; }
// <-- EGB && SGB
    int32_t getNumberOfVertices() const { return vertCount; }
    const SbVec3f *getVertices() const { return vertices; }
// --> EGB && SGB
	public:
		void generateSphereTexCoords(int32_t &vertCount);
		static void generateBillboard();
// <-- EGB && SGB

  private:
    int32_t depth;
    int32_t vertCount;
    SbVec3f *vertices;
// --> EGB && SGB
		int32_t GLUDepth;
		GLUquadricObj *qobj;
		SbVec2f *sphereTexCoords;

		SoSphere *soSphere;

		bool sphereTexCoordsGenerated;

    // static variables
    static GLuint sphereTexture;
    static int instances;
// <-- EGB && SGB
};
#endif /* !__CHEM_UNITSPHERE_H__ */
