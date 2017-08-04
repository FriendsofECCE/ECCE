/*
 * ChemUnitCylinder.c++
 *
 *     Unit Cylinder class
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

#include <assert.h>

#include <math.h>

/*
	Porting to Windows:
	Jamie Doornbos	2001/07/15
	Daniel Soto	2002/02/05
*/
#ifdef WIN32
#include <windows.h>
#endif

#include <GL/gl.h>

#include "inv/ChemKit/ChemDisplay.H"
#include "inv/ChemKit/ChemUnitCylinder.H"
// --> roundcap optimization
#include "inv/ChemKit/ChemDisplayParamElement.H"
// <-- roundcap optimization

// Returns S or T texture coord for point on top or bottom of
// cylinder, given x or z coord  (this is for flat caps)
#define BOT_TEX_S(x)    ((x) * .5 + .5)
#define BOT_TEX_T(z)    ((z) * .5 + .5)
#define TOP_TEX_S(x)    BOT_TEX_S(x)
#define TOP_TEX_T(z)    (1.0 - BOT_TEX_T(z))


// --> EGB && SGB
#define CALC_SEMICYLINDER_ROTATION \
if (bondCylinderDisplayStyle == ChemDisplayParam::BONDCYLINDER_SEMI_NOCAP || bondCylinderDisplayStyle == ChemDisplayParam::BONDCYLINDER_SEMI_FLATCAP || bondCylinderDisplayStyle  == ChemDisplayParam::BONDCYLINDER_SEMI_ROUNDCAP) { \
SbVec3f axis = to-from; \
axis.normalize(); \
SbVec3f right, normal; \
SbVec3f observer = camPos - from; \
observer.normalize(); \
if (perspective) right = axis.cross(observer);  \
else right = axis.cross(lookAtVector);  \
right.normalize(); \
normal = right.cross(axis); \
SbMatrix matrix; \
matrix[0][0] = normal[0]; \
matrix[0][1] = normal[1]; \
matrix[0][2] = normal[2]; \
matrix[0][3] = 0; \
matrix[1][0] = right[0]; \
matrix[1][1] = right[1]; \
matrix[1][2] = right[2]; \
matrix[1][3] = 0; \
matrix[2][0] = axis[0]; \
matrix[2][1] = axis[1]; \
matrix[2][2] = axis[2]; \
matrix[2][3] = 0; \
matrix[3][0] = 0; \
matrix[3][1] = 0; \
matrix[3][2] = 0; \
matrix[3][3] = 1; \
theRotation = SbRotation(matrix); \
}
// <-- EGB && SGB


// --> EGB && SGB
#include "inv/elements/SoModelMatrixElement.H"
#include "inv/elements/SoViewVolumeElement.H"
#include "inv/elements/SoViewingMatrixElement.H"
// <-- EGB && SGB

// --> roundcap optimization
#define PRE_RENDER_CAP(ATOM,BOOLEAN) \
BOOLEAN = true;  \
if (cdp->displayStyle.getValue() != ChemDisplayParam::DISPLAY_STICK) \
{ \
int __i; \
for (__i=0; __i<cd->atomIndex.getNum();__i++) \
{ \
const SbVec2i range = *cd->atomIndex.getValues(__i); \
if (range[1] == -1) BOOLEAN  = !(ATOM >= range[0]); \
else BOOLEAN = !(range[0]>=ATOM && ATOM<=range[1]); \
if (!BOOLEAN) break; \
} \
}
// <-- roundcap optimization

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Computes number of sides per circular cross-section and number
//    of sections, based on complexity, then computes ring of x,z
//    coordinates around cylinder and stores in ringCoords.
//
// Usage: static

static void
computeDepth(float complexity, int32_t &numSections, int32_t &numSides)
//
////////////////////////////////////////////////////////////////////////
{
    // If complexity is between 0 and .5 (inclusive), use 1 section
    // and between 3 and 16 sides:
    if (complexity <= 0.5) {
        numSections = 1;
        numSides    = (int) (complexity * 26.0 + 3.0);
    }

    // If complexity is between .5 and 1, use between 1 and 8 sections
    // and between 16 and 64 sides:
    else {
        numSections = (int) (14.0 * complexity - 6.0);
        numSides    = (int) (complexity * 96.0 - 32.0);
    }
}

// --> EGB && SGB
////////////////////////////////////////////////////////////////////////
//
// Description:
//    Computes number of sides per circular cross-section and number
//    of sections, based on complexity, then computes ring of x,z
//    coordinates around cylinder and stores in ringCoords.
//
// Usage: static

static void
computeGLUDepth(float complexity, int32_t &GLUslices, int32_t &GLUstacks)
//
////////////////////////////////////////////////////////////////////////
{
    // If complexity is between 0 and .5 (inclusive), use 1 section
    // and between 3 and 16 sides:
    if (complexity <= 0.5) {
        GLUstacks = 1;
        GLUslices    = (int) (complexity * 26.0 + 3.0);
    }

    // If complexity is between .5 and 1, use between 1 and 8 sections
    // and between 16 and 64 sides:
    else {
        GLUstacks = (int) (14.0 * complexity - 6.0);
        GLUslices    = (int) (complexity * 96.0 - 32.0);
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Generates the texture coordinates for a unit cylinder and related
//    topology

void ChemUnitCylinder::generateCylTexCoords(SbBool doCaps)
//
////////////////////////////////////////////////////////////////////////
//
{
	assert(numSections && numSides > 0);
	float s,t;
	float tTop = 1.0f;
	float dt = -1.0 / numSections;
	float ds = -1.0 / numSides;

	if (sidesTexCoords != NULL) delete [] sidesTexCoords;

	sidesTexCoords = new SbVec2f[numSections*(numSides*2 + 2)];

	int32_t section, side;
	int i=0;
	for (section = 0; section < numSections; section++) 
	{
			s = 1.0;
			float tBot = tTop + dt;
			for (side = 0; side < numSides; side++) 
			{
					sidesTexCoords[i++].setValue(s,tBot);//glTexCoord2f(s, tBot);
					sidesTexCoords[i++].setValue(s,tTop);//glTexCoord2f(s, tTop);
					s += ds;
			}
			s = 0.0;
			sidesTexCoords[i++].setValue(s,tBot);//glTexCoord2f(s, tBot);
			sidesTexCoords[i++].setValue(s,tTop);//glTexCoord2f(s, tTop);
			tTop = tBot;
	}

	if (doCaps) 
	{
		int vertCount = 0;
		SbVec3f vec = cylVertices[vertCount];
		
		// TOP
		if (topCapTexCoords != NULL) delete [] topCapTexCoords;
		topCapTexCoords = new SbVec2f[(numSections-1)*(numSides*2 + 2)];
		
		i=0;
		for (section = numSections - 1; section > 0; --section) 
		{
				for (side = 0; side < numSides; side++) 
				{
					// DISP_FLATCAP_TOP_TEXTURE;
					// DISP_FLATCAP_TOP_TEXTURE;
					vec = cylVertices[vertCount++];
					s = TOP_TEX_S(vec[0]);        
					t = TOP_TEX_T(vec[2]);
					topCapTexCoords[i++].setValue(s,t);//glTexCoord2f(s, t);
					vec = cylVertices[vertCount++];
					s = TOP_TEX_S(vec[0]);        
					t = TOP_TEX_T(vec[2]);
					topCapTexCoords[i++].setValue(s,t);//glTexCoord2f(s, t);
					//
				}
				//DISP_FLATCAP_TOP_TEXTURE;
				//DISP_FLATCAP_TOP_TEXTURE;
				vec = cylVertices[vertCount++];
				s = TOP_TEX_S(vec[0]);        
				t = TOP_TEX_T(vec[2]);
				topCapTexCoords[i++].setValue(s,t);//glTexCoord2f(s, t);
				vec = cylVertices[vertCount++];
				s = TOP_TEX_S(vec[0]);        
				t = TOP_TEX_T(vec[2]);
				topCapTexCoords[i++].setValue(s,t);//glTexCoord2f(s, t);
		}

		//glBegin(GL_TRIANGLE_FAN);
		topCapTexCoords[i++].setValue(0.5f,0.5f);//glTexCoord2f(0.5, 0.5);
		vec = cylVertices[vertCount++];//DISP_VERTEX;
		for (side = numSides - 1; side >= 0; side--) 
		{
				//DISP_FLATCAP_TOP_TEXTURE;
				vec = cylVertices[vertCount++];
				s = TOP_TEX_S(vec[0]);        
				t = TOP_TEX_T(vec[2]);
				topCapTexCoords[i++].setValue(s,t);//glTexCoord2f(s, t);
		}
		//DISP_FLATCAP_TOP_TEXTURE;
		vec = cylVertices[vertCount++];
		s = TOP_TEX_S(vec[0]);        
		t = TOP_TEX_T(vec[2]);
		topCapTexCoords[i++].setValue(s,t);//glTexCoord2f(s, t);
		//glEnd();

		// BOTTOM
		if (bottomCapTexCoords != NULL) delete [] bottomCapTexCoords;
		bottomCapTexCoords = new SbVec2f[(numSections-1)*(numSides*2 + 2)];

		i=0;
		for (section = numSections - 1; section > 0; --section) 
		{
				//glBegin(GL_TRIANGLE_STRIP);
				for (side = numSides - 1; side >= 0; side--) 
				{
						//DISP_FLATCAP_BOT_TEXTURE;
						//DISP_FLATCAP_BOT_TEXTURE;
					vec = cylVertices[vertCount++];
					s = BOT_TEX_S(vec[0]);        
					t = BOT_TEX_T(vec[2]);
					bottomCapTexCoords[i++].setValue(s,t);//glTexCoord2f(s, t);
					vec = cylVertices[vertCount++];
					s = BOT_TEX_S(vec[0]);        
					t = BOT_TEX_T(vec[2]);
					bottomCapTexCoords[i++].setValue(s,t);//glTexCoord2f(s, t);
				}
				//DISP_FLATCAP_BOT_TEXTURE;
				//DISP_FLATCAP_BOT_TEXTURE;
				vec = cylVertices[vertCount++];
				s = BOT_TEX_S(vec[0]);        
				t = BOT_TEX_T(vec[2]);
				bottomCapTexCoords[i++].setValue(s,t);//glTexCoord2f(s, t);
				vec = cylVertices[vertCount++];
				s = BOT_TEX_S(vec[0]);        
				t = BOT_TEX_T(vec[2]);
				bottomCapTexCoords[i++].setValue(s,t);//glTexCoord2f(s, t);
				//glEnd();
		}
		//glBegin(GL_TRIANGLE_FAN);
		bottomCapTexCoords[i++].setValue(0.5f,0.5f);//glTexCoord2f(0.5, 0.5);
		vec = cylVertices[vertCount++];//DISP_VERTEX;
		for (side = 0; side < numSides; side++) 
		{
				//DISP_FLATCAP_BOT_TEXTURE;
			vec = cylVertices[vertCount++];
			s = BOT_TEX_S(vec[0]);        
			t = BOT_TEX_T(vec[2]);
		}
		//DISP_FLATCAP_BOT_TEXTURE;
		vec = cylVertices[vertCount++];
		s = BOT_TEX_S(vec[0]);        
		t = BOT_TEX_T(vec[2]);
		//glEnd();
	}

	cylTexCoordsGenerated = true;
}
//
// <-- EGB && SGB

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Computes number of sides per circular cross-section and number
//    of sections, based on complexity, then computes ring of x,z
//    coordinates around cylinder and stores in ringCoords.
//
// Usage: static

static void
computeRingCoords(int numSides, SbVec2f *ringCoords)
//
////////////////////////////////////////////////////////////////////////
{
    float   theta, dTheta;
    int     side;

    // Compute x and z coordinates around ring
    theta  = 0.0;
    dTheta = 2.0 * M_PI / (numSides);
    for (side = 0; side < numSides; side++) {
        ringCoords[side].setValue(sin(theta), -cos(theta));
        theta += dTheta;
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Generate the unit cylinder.  This will generate enough storage
//    for the flat cap normals and vertices.
//
// Usage: static
// --> EGB && SGB
//static void
//generateCylVertices(int numSections, int numSides,
//	const SbVec2f *ringCoords,
//	int &cylinderNormCount, int &cylinderVertCount,
//	SbVec3f *cylNormals, SbVec3f *cylVertices)

static void
generateCylVertices(int numSections, int numSides,
	const SbVec2f *ringCoords,
	int &cylinderNormCount, int &cylinderVertCount,
	SbVec3f *cylNormals, SbVec3f *cylVertices,
	SbVec3f *arrayCylNormals = NULL)
// <-- EGB && SGB
//
////////////////////////////////////////////////////////////////////////
{
    int         side, section;
    float       yTop, yBot, dy;
    SbVec3f     pt, norm;

    cylinderNormCount = cylinderVertCount = 0;

    // Draw each section of sides as a triangle mesh, from top to bottom
    yTop = 1.0;
    dy   = -2.0 / numSections;

    for (section = 0; section < numSections; section++) {
        yBot = yTop + dy;
        for (side = 0; side < numSides; side++) {
            pt[0] = ringCoords[side][0];
            pt[2] = ringCoords[side][1];

            // Deal with normal
            norm.setValue(pt[0], 0.0, pt[2]);
            cylNormals[cylinderNormCount++] = norm;
// --> EGB && SGB
						if (arrayCylNormals != NULL)
						{
							arrayCylNormals[cylinderNormCount*2-2] = norm;
							arrayCylNormals[cylinderNormCount*2-1] = norm;
						}
// <-- EGB && SGB
            // Point at bottom of section
            pt[1] = yBot;
            cylVertices[cylinderVertCount++] = pt;

            // Point at top of section
            pt[1] = yTop;
            cylVertices[cylinderVertCount++] = pt;
        }

        // Join end of strip back to beginning
        side = 0;
        pt[0] = ringCoords[side][0];
        pt[2] = ringCoords[side][1];

        // Deal with normal
        norm.setValue(pt[0], 0.0, pt[2]);
        cylNormals[cylinderNormCount++] = norm;
// --> EGB && SGB
				if (arrayCylNormals != NULL)
				{
					arrayCylNormals[cylinderNormCount*2-2] = norm;
					arrayCylNormals[cylinderNormCount*2-1] = norm;
				}
// <-- EGB && SGB

        // Point at bottom of section
        pt[1] = yBot;
        cylVertices[cylinderVertCount++] = pt;

        // Point at top of section
        pt[1] = yTop;
        cylVertices[cylinderVertCount++] = pt;

        // Prepare for next section down
        yTop = yBot;
    }

}

// --> EGB && SGB flat caps array
static void generateFlatCapVerticesArray(int numSections, int numSides,
																				 const SbVec2f *ringCoords,int &cylinderVertCount,
																				 SbVec3f *cylNormals, SbVec3f *cylVertices)
{
    int side;
    SbVec3f pt, norm;

	// top cap (triangle fan)
		pt.setValue(0.0, 1.0, 0.0);
		norm.setValue(0.0, 1.0, 0.0);

		cylVertices[cylinderVertCount] = pt;
		cylNormals[cylinderVertCount++] = norm;

		for (side = 0; side < numSides; side++)
		{
			pt[0] = ringCoords[side][0]; // x
			//pt[1] = 1;	  						 // y
      pt[2] = ringCoords[side][1]; // z
			
			cylVertices[cylinderVertCount] = pt;
			cylNormals[cylinderVertCount++] = norm;
		}

		// Send first vertex again
    pt[0] = ringCoords[0][0];
    pt[2] = ringCoords[0][1];
    cylVertices[cylinderVertCount] = pt;
		cylNormals[cylinderVertCount++] = norm;

		// bottom cap (triangle fan)
		pt.setValue(0.0, -1.0, 0.0);
		norm.setValue(0.0, -1.0, 0.0);

		cylVertices[cylinderVertCount] = pt;
		cylNormals[cylinderVertCount++] = norm;

		for (side = 0; side < numSides; side++)
		{
			pt[0] = ringCoords[side][0]; // x
			//pt[1] = -1; 							 // y
      pt[2] = ringCoords[side][1]; // z
			
			cylVertices[cylinderVertCount] = pt;
			cylNormals[cylinderVertCount++] = norm;
		}

		// Send first vertex again
    pt[0] = ringCoords[0][0];
    pt[2] = ringCoords[0][1];
    cylVertices[cylinderVertCount] = pt;
		cylNormals[cylinderVertCount++] = norm;
}

static void generateFlatCapVerticesSemiCylinder(int numSides,
																				 int &cylinderVertCount,
																				 SbVec3f *cylNormals, SbVec3f *cylVertices)
{
    int side;
    SbVec3f pt, norm;
		float angle;
	
	// top cap (triangle fan)
		pt.setValue(0.0, 0.0, 2.0);
		norm.setValue(0.0, 0.0, 1.0);

		cylVertices[cylinderVertCount] = pt;
		cylNormals[cylinderVertCount++] = norm;

		angle = 0.0f;
		for (side = 0; side < numSides; side++)
		{
			pt[0] = sin(angle); // x
			pt[1] = cos(angle);	// y
      //pt[2] = 2.0;					// z
			
			cylVertices[cylinderVertCount] = pt;
			cylNormals[cylinderVertCount++] = norm;

			angle += (180.0f/(numSides/2-1)*0.01745329252f); //in radians
		}
		// Send first vertex again
    cylVertices[cylinderVertCount] = cylVertices[cylinderVertCount-side];
		cylNormals[cylinderVertCount++] = norm;

		// bottom cap (triangle fan)
		pt.setValue(0.0, 0.0, 0.0);
		norm.setValue(0.0, 0.0, -1.0);

		cylVertices[cylinderVertCount] = pt;
		cylNormals[cylinderVertCount++] = norm;

		angle = 0.0f;
		for (side = 0; side < numSides; side++)
		{
			pt[0] = sin(angle); // x
			pt[1] = cos(angle);	// y
      //pt[2] = 0;					// z
			
			cylVertices[cylinderVertCount] = pt;
			cylNormals[cylinderVertCount++] = norm;

			angle += (180.0f/(numSides/2-1)*0.01745329252f); //in radians
		}
		// Send first vertex again
    cylVertices[cylinderVertCount] = cylVertices[cylinderVertCount-side];
		cylNormals[cylinderVertCount++] = norm;
}
// <-- EGB && SGB flat caps array

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Generate the flat caps for the unit cylinder.  The storage for
//    the normals and vertices was created in generateCylVertices.
//
// Usage: static

// --> EGB && SGB
//static void
//generateFlatCapVertices(int numSections, int numSides,
//	const SbVec2f *ringCoords,
//	int &cylinderNormCount, int &cylinderVertCount,
//	SbVec3f *cylNormals, SbVec3f *cylVertices)
static void
generateFlatCapVertices(int numSections, int numSides,
	const SbVec2f *ringCoords,
	int &cylinderNormCount, int &cylinderVertCount,
	SbVec3f *cylNormals, SbVec3f *cylVertices,
	SbVec3f *arrayCylNormals = NULL)
// <-- EGB && SGB
//
////////////////////////////////////////////////////////////////////////
{
    // Draw top face as a series of concentric rings. The number of
    // rings is the same as the number of sections of the sides of the
    // cylinder.

    int         side, section;
    float       outerRadius, innerRadius, dRadius;
    SbVec3f     pt, norm;

    // TOP
    norm.setValue(0.0, 1.0, 0.0);
    pt[1] = 1.0;

    cylNormals[cylinderNormCount++] = norm;

    // Start at the outside and work in
    outerRadius = 1.0;
    dRadius     = -1.0 / numSections;

    for (section = numSections - 1; section > 0; --section) {

        innerRadius = outerRadius + dRadius;

        // Innermost ring will be a triangle fan calculated later.
        // Other rings are triangle strips.

        for (side = 0; side < numSides; side++) {
            // Send points on outer and inner rings
            pt[0] = outerRadius * ringCoords[side][0];
            pt[2] = outerRadius * ringCoords[side][1];
            cylVertices[cylinderVertCount++] = pt;
            pt[0] = innerRadius * ringCoords[side][0];
            pt[2] = innerRadius * ringCoords[side][1];
            cylVertices[cylinderVertCount++] = pt;
        }

        // Join end of strip back to beginning
        pt[0] = outerRadius * ringCoords[0][0];
        pt[2] = outerRadius * ringCoords[0][1];
        cylVertices[cylinderVertCount++] = pt;
        pt[0] = innerRadius * ringCoords[0][0];
        pt[2] = innerRadius * ringCoords[0][1];
        cylVertices[cylinderVertCount++] = pt;

        // Prepare for next ring
        outerRadius = innerRadius;
    }

    // Top innermost ring
    // Innermost ring is treated as a triangle fan. This not
    // only gets better shading (because the center vertex is
    // sent), but also avoids the problem of having a polygon
    // with too many vertices.

    outerRadius = -dRadius;

    // Center point comes first
    pt[0] = pt[2] = 0.0;
    cylVertices[cylinderVertCount++] = pt;

    // Send all vertices around ring. Go in reverse order
    // so that vertex ordering is correct
    for (side = numSides - 1; side >= 0; side--) {
        pt[0] = outerRadius * ringCoords[side][0];
        pt[2] = outerRadius * ringCoords[side][1];
        cylVertices[cylinderVertCount++] = pt;
    }
    // Send first vertex again
    pt[0] = outerRadius * ringCoords[numSides - 1][0];
    pt[2] = outerRadius * ringCoords[numSides - 1][1];
    cylVertices[cylinderVertCount++] = pt;

    // BOTTOM
    // Draw bottom face the same way as the top
    norm.setValue(0.0, -1.0, 0.0);
    pt[1] = -1.0;

    cylNormals[cylinderNormCount++] = norm;

    // Start at the outside and work in
    outerRadius = 1.0;
    dRadius     = -1.0 / numSections;
    for (section = numSections - 1; section > 0; --section) {

        innerRadius = outerRadius + dRadius;

        // Innermost ring will be a triangle fan calculated later.
        // Other rings are triangle strips.

        // Go in reverse order so that vertex ordering is correct
        for (side = numSides - 1; side >= 0; side--) {
            // Send points on outer and inner rings
            pt[0] = outerRadius * ringCoords[side][0];
            pt[2] = outerRadius * ringCoords[side][1];
            cylVertices[cylinderVertCount++] = pt;
            pt[0] = innerRadius * ringCoords[side][0];
            pt[2] = innerRadius * ringCoords[side][1];
            cylVertices[cylinderVertCount++] = pt;
        }

        // Join end of strip back to beginning
        side = numSides - 1;
        pt[0] = outerRadius * ringCoords[side][0];
        pt[2] = outerRadius * ringCoords[side][1];
        cylVertices[cylinderVertCount++] = pt;
        pt[0] = innerRadius * ringCoords[side][0];
        pt[2] = innerRadius * ringCoords[side][1];
        cylVertices[cylinderVertCount++] = pt;

        // Prepare for next ring
        outerRadius = innerRadius;
    } // end for (section = numSections - 1 ...

    // Bottom innermost ring.
    // Innermost ring is drawn as a triangle fan. This not
    // only gets better shading (because the center vertex is
    // sent), but also avoids the problem of having a polygon
    // with too many vertices.

    outerRadius = -dRadius;

    // Center point comes first
    pt[1] = -1.0;
    pt[0] = pt[2] = 0.0;
    cylVertices[cylinderVertCount++] = pt;

    // Send all vertices around ring
    for (side = 0; side < numSides; side++) {
        pt[0] = outerRadius * ringCoords[side][0];
        pt[2] = outerRadius * ringCoords[side][1];
        cylVertices[cylinderVertCount++] = pt;
    }
    // Send first vertex again
    pt[0] = outerRadius * ringCoords[0][0];
    pt[2] = outerRadius * ringCoords[0][1];
    cylVertices[cylinderVertCount++] = pt;

// --> EGB && SGB flat caps array
		generateFlatCapVerticesArray(numSections,numSides,
			ringCoords,cylinderVertCount,
			arrayCylNormals,cylVertices);
// <-- EGB && SGB flat caps array
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Generate the unit cylinder caps for rounded cylinders
//
// Usage: static

static void
generateRoundCapVertices(int numSections, int numSides,
	const SbVec2f *ringCoords,
	int &cylinderCapNormCount, int &cylinderCapVertCount,
	int &cylinderBottomStartNorm, int &cylinderBottomStartVert,
	SbVec3f *cylinderCapNormals, SbVec3f *cylinderCapVertices)
//
////////////////////////////////////////////////////////////////////////
{
    int     section;
    SbVec3f norm, pt;
    float   innerRadius, outerRadius;
    float   dY[3], dRadius[3];
    int     side;

    // Start at the outside and work in
    outerRadius = 1.0;
    dY[2]           = 0.0;
    dY[1]           = sin(M_PI / 6.0);
    dY[0]           = sin(M_PI / 3.0);
    dRadius[2] = 1.0;
    dRadius[1] = cos(M_PI / 6.0);
    dRadius[0] = cos(M_PI / 3.0);

    cylinderCapNormCount = cylinderCapVertCount = 0;

    for (section = numSections - 1; section > 0; --section) {

        outerRadius = dRadius[section];
        innerRadius = dRadius[section-1];

        // Innermost ring will be a triangle fan calculated later.
        // Other rings are triangle strips.

        for (side = 0; side < numSides; side++) {
            // Send points on outer and inner rings
            pt[0] = outerRadius * ringCoords[side][0];
            pt[1] = dY[section];
            pt[2] = outerRadius * ringCoords[side][1];
            norm.setValue(pt[0], pt[1], pt[2]);
            norm.normalize();
            cylinderCapNormals[cylinderCapNormCount++] = norm;
            cylinderCapVertices[cylinderCapVertCount++] = pt;

            pt[0] = innerRadius * ringCoords[side][0];
            pt[1] = dY[section-1];
            pt[2] = innerRadius * ringCoords[side][1];
            norm.setValue(pt[0], pt[1], pt[2]);
            norm.normalize();
            cylinderCapNormals[cylinderCapNormCount++] = norm;
            cylinderCapVertices[cylinderCapVertCount++] = pt;
        }

        // Join end of strip back to beginning
        pt[0] = outerRadius * ringCoords[0][0];
        pt[1] = dY[section];
        pt[2] = outerRadius * ringCoords[0][1];
        norm.setValue(pt[0], pt[1], pt[2]);
        norm.normalize();
        cylinderCapNormals[cylinderCapNormCount++] = norm;
        cylinderCapVertices[cylinderCapVertCount++] = pt;

        pt[0] = innerRadius * ringCoords[0][0];
        pt[1] = dY[section-1];;
        pt[2] = innerRadius * ringCoords[0][1];
        norm.setValue(pt[0], pt[1], pt[2]);
        norm.normalize();
        cylinderCapNormals[cylinderCapNormCount++] = norm;
        cylinderCapVertices[cylinderCapVertCount++] = pt;
    }

    // Cap innermost ring
    // Innermost ring is treated as a triangle fan. This not
    // only gets better shading (because the center vertex is
    // sent), but also avoids the problem of having a polygon
    // with too many vertices.

    // Center point comes first
    pt[0] = pt[2] = 0.0;
    pt[1] = 1.0;
    cylinderCapNormals[cylinderCapNormCount++] = pt;
    cylinderCapVertices[cylinderCapVertCount++] = pt;

    // Send all vertices around ring. Go in reverse order
    // so that vertex ordering is correct
    outerRadius = dRadius[0];
    pt[1] = dY[0];
    for (side = numSides - 1; side >= 0; side--) {
        pt[0] = outerRadius * ringCoords[side][0];
        pt[2] = outerRadius * ringCoords[side][1];
        norm.setValue(pt[0], pt[1], pt[2]);
        norm.normalize();
        cylinderCapNormals[cylinderCapNormCount++] = norm;
        cylinderCapVertices[cylinderCapVertCount++] = pt;
    }
    // Send first vertex again
    pt[0] = outerRadius * ringCoords[numSides - 1][0];
    pt[2] = outerRadius * ringCoords[numSides - 1][1];
    norm.setValue(pt[0], pt[1], pt[2]);
    norm.normalize();
    cylinderCapNormals[cylinderCapNormCount++] = norm;
    cylinderCapVertices[cylinderCapVertCount++] = pt;

    // BOTTOM
    // Draw bottom face the same way as the top
    // Start at the outside and work in

    cylinderBottomStartNorm = cylinderCapNormCount;
    cylinderBottomStartVert = cylinderCapVertCount;
    for (section = numSections - 1; section > 0; --section) {

        outerRadius = dRadius[section];
        innerRadius = dRadius[section-1];

        // Innermost ring will be a triangle fan calculated later.
        // Other rings are triangle strips.

        // Go in reverse order so that vertex ordering is correct
        for (side = numSides - 1; side >= 0; side--) {
            // Send points on outer and inner rings
            pt[0] = outerRadius * ringCoords[side][0];
            pt[1] = -dY[section];
            pt[2] = outerRadius * ringCoords[side][1];
            norm.setValue(pt[0], pt[1], pt[2]);
            norm.normalize();
            cylinderCapNormals[cylinderCapNormCount++] = norm;
            cylinderCapVertices[cylinderCapVertCount++] = pt;

            pt[0] = innerRadius * ringCoords[side][0];
            pt[1] = -dY[section-1];
            pt[2] = innerRadius * ringCoords[side][1];
            norm.setValue(pt[0], pt[1], pt[2]);
            norm.normalize();
            cylinderCapNormals[cylinderCapNormCount++] = norm;
            cylinderCapVertices[cylinderCapVertCount++] = pt;
        }

        // Join end of strip back to beginning
        side = numSides - 1;
        pt[0] = outerRadius * ringCoords[side][0];
        pt[1] = -dY[section];
        pt[2] = outerRadius * ringCoords[side][1];
        norm.setValue(pt[0], pt[1], pt[2]);
        norm.normalize();
        cylinderCapNormals[cylinderCapNormCount++] = norm;
        cylinderCapVertices[cylinderCapVertCount++] = pt;

        pt[0] = innerRadius * ringCoords[side][0];
        pt[1] = -dY[section-1];;
        pt[2] = innerRadius * ringCoords[side][1];
        norm.setValue(pt[0], pt[1], pt[2]);
        norm.normalize();
        cylinderCapNormals[cylinderCapNormCount++] = norm;
        cylinderCapVertices[cylinderCapVertCount++] = pt;

    } // end for (section = numSections - 1 ...

    // Bottom innermost ring.
    // Innermost ring is drawn as a triangle fan. This not
    // only gets better shading (because the center vertex is
    // sent), but also avoids the problem of having a polygon
    // with too many vertices.

    // Center point comes first
    pt[0] = pt[2] = 0.0;
    pt[1] = -1.0;
    cylinderCapNormals[cylinderCapNormCount++] = pt;
    cylinderCapVertices[cylinderCapVertCount++] = pt;

    // Send all vertices around ring
    outerRadius = dRadius[0];
    pt[1] = -dY[0];
    for (side = 0; side < numSides; side++) {
        pt[0] = outerRadius * ringCoords[side][0];
        pt[2] = outerRadius * ringCoords[side][1];
        norm.setValue(pt[0], pt[1], pt[2]);
        norm.normalize();
        cylinderCapNormals[cylinderCapNormCount++] = norm;
        cylinderCapVertices[cylinderCapVertCount++] = pt;
    }
    // Send first vertex again
    pt[0] = outerRadius * ringCoords[0][0];
    pt[2] = outerRadius * ringCoords[0][1];
    norm.setValue(pt[0], pt[1], pt[2]);
    norm.normalize();
    cylinderCapNormals[cylinderCapNormCount++] = norm;
    cylinderCapVertices[cylinderCapVertCount++] = pt;
}

#define DISP_ROUNDCAP_NORMAL                                                  \
theNormalMatrix.multDirMatrix(cylinderCapNormals[normCount++], vec);          \
glNormal3fv(vec.getValue());

#define DISP_ROUNDCAP_VERTEX                                                  \
theVertexMatrix.multVecMatrix(cylinderCapVertices[vertCount++], vec);         \
glVertex3fv(vec.getValue());

#define DISP_ROUNDCAP_TOP_TEXTURE                                             \
vec = cylinderCapVertices[vertCount++];                                       \
s = TOP_TEX_S(vec[0]);                                                        \
t = TOP_TEX_T(vec[2]);                                                        \
glTexCoord2f(s, t);                                                           \
theVertexMatrix.multVecMatrix(vec, tmpVec);                                   \
glVertex3fv(tmpVec.getValue());

#define DISP_ROUNDCAP_BOT_TEXTURE                                             \
vec = cylinderCapVertices[vertCount++];                                       \
s = BOT_TEX_S(vec[0]);                                                        \
t = BOT_TEX_T(vec[2]);                                                        \
glTexCoord2f(s, t);                                                           \
theVertexMatrix.multVecMatrix(vec, tmpVec);                                   \
glVertex3fv(tmpVec.getValue());

// --> EGB && SGB round caps array
////////////////////////////////////////////////////////////////////////
//
// Description:
//    Generate the unit cylinder caps for rounded cylinders
//
// Usage: static

static void
generateRoundCapVerticesArray(int numSections, int numSides,
	const SbVec2f *ringCoords,int &cylinderCapVertCount,
	SbVec3f *cylinderCapNormals, SbVec3f *cylinderCapVertices)
//
////////////////////////////////////////////////////////////////////////
{
    int     section;
    SbVec3f norm, pt;
    float   innerRadius, outerRadius;
    float   dY[3], dRadius[3];
    int     side;

    // Start at the outside and work in
    outerRadius = 1.0;
    dY[2]           = 0.0;
    dY[1]           = sin(M_PI / 6.0);
    dY[0]           = sin(M_PI / 3.0);
    dRadius[2] = 1.0;
    dRadius[1] = cos(M_PI / 6.0);
    dRadius[0] = cos(M_PI / 3.0);

    for (section = numSections - 1; section > 0; --section) {

        outerRadius = dRadius[section];
        innerRadius = dRadius[section-1];

        // Innermost ring will be a triangle fan calculated later.
        // Other rings are triangle strips.

        for (side = 0; side < numSides; side++) {
            // Send points on outer and inner rings
            pt[0] = outerRadius * ringCoords[side][0];
            pt[1] = dY[section];
            pt[2] = outerRadius * ringCoords[side][1];
            norm.setValue(pt[0], pt[1], pt[2]);
            norm.normalize();
            cylinderCapNormals[cylinderCapVertCount] = norm;
            cylinderCapVertices[cylinderCapVertCount++] = pt;

            pt[0] = innerRadius * ringCoords[side][0];
            pt[1] = dY[section-1];
            pt[2] = innerRadius * ringCoords[side][1];
            norm.setValue(pt[0], pt[1], pt[2]);
            norm.normalize();
            cylinderCapNormals[cylinderCapVertCount] = norm;
            cylinderCapVertices[cylinderCapVertCount++] = pt;
        }

        // Join end of strip back to beginning
        pt[0] = outerRadius * ringCoords[0][0];
        pt[1] = dY[section];
        pt[2] = outerRadius * ringCoords[0][1];
        norm.setValue(pt[0], pt[1], pt[2]);
        norm.normalize();
        cylinderCapNormals[cylinderCapVertCount] = norm;
        cylinderCapVertices[cylinderCapVertCount++] = pt;

        pt[0] = innerRadius * ringCoords[0][0];
        pt[1] = dY[section-1];
        pt[2] = innerRadius * ringCoords[0][1];
        norm.setValue(pt[0], pt[1], pt[2]);
        norm.normalize();
        cylinderCapNormals[cylinderCapVertCount] = norm;
        cylinderCapVertices[cylinderCapVertCount++] = pt;
    }

		// Top cap innmermost ring is treated as triangle strips
		for (side = numSides - 1; side >= 0; side--)
		{
			// Center point comes first
				pt[0] = pt[2] = 0.0;
				pt[1] = 1.0;
				cylinderCapNormals[cylinderCapVertCount] = pt;
				cylinderCapVertices[cylinderCapVertCount++] = pt;

        pt[0] = innerRadius * ringCoords[side][0];
				pt[1] = dY[0];
        pt[2] = innerRadius * ringCoords[side][1];
        norm.setValue(pt[0], pt[1], pt[2]);
        norm.normalize();
        cylinderCapNormals[cylinderCapVertCount] = norm;
        cylinderCapVertices[cylinderCapVertCount++] = pt;		
    }

		// Send first vertex again
		pt[0] = innerRadius * ringCoords[numSides - 1][0];
		pt[1] = dY[0];
    pt[2] = innerRadius * ringCoords[numSides - 1][1];
    norm.setValue(pt[0], pt[1], pt[2]);
    norm.normalize();
    cylinderCapNormals[cylinderCapVertCount] = norm;
    cylinderCapVertices[cylinderCapVertCount++] = pt;

		
    // BOTTOM
    // Draw bottom face the same way as the top
    // Start at the outside and work in

    for (section = numSections - 1; section > 0; --section) {

        outerRadius = dRadius[section];
        innerRadius = dRadius[section-1];

        // All rings are triangle strips.

        // Go in reverse order so that vertex ordering is correct
        for (side = numSides - 1; side >= 0; side--) {
            // Send points on outer and inner rings
            pt[0] = outerRadius * ringCoords[side][0];
            pt[1] = -dY[section];
            pt[2] = outerRadius * ringCoords[side][1];
            norm.setValue(pt[0], pt[1], pt[2]);
            norm.normalize();
            cylinderCapNormals[cylinderCapVertCount] = norm;
            cylinderCapVertices[cylinderCapVertCount++] = pt;

            pt[0] = innerRadius * ringCoords[side][0];
            pt[1] = -dY[section-1];
            pt[2] = innerRadius * ringCoords[side][1];
            norm.setValue(pt[0], pt[1], pt[2]);
            norm.normalize();
            cylinderCapNormals[cylinderCapVertCount] = norm;
            cylinderCapVertices[cylinderCapVertCount++] = pt;
        }

        // Join end of strip back to beginning
        side = numSides - 1;
        pt[0] = outerRadius * ringCoords[side][0];
        pt[1] = -dY[section];
        pt[2] = outerRadius * ringCoords[side][1];
        norm.setValue(pt[0], pt[1], pt[2]);
        norm.normalize();
        cylinderCapNormals[cylinderCapVertCount] = norm;
        cylinderCapVertices[cylinderCapVertCount++] = pt;

        pt[0] = innerRadius * ringCoords[side][0];
        pt[1] = -dY[section-1];;
        pt[2] = innerRadius * ringCoords[side][1];
        norm.setValue(pt[0], pt[1], pt[2]);
        norm.normalize();
        cylinderCapNormals[cylinderCapVertCount] = norm;
        cylinderCapVertices[cylinderCapVertCount++] = pt;

    } // end for (section = numSections - 1 ...

		// Bottom cap innmermost ring is treated as triangle strip

		for (side = 0; side < numSides; side++)
		{
			// Center point comes first
				pt[0] = pt[2] = 0.0;
				pt[1] = -1.0;
				cylinderCapNormals[cylinderCapVertCount] = pt;
				cylinderCapVertices[cylinderCapVertCount++] = pt;

        pt[0] = innerRadius * ringCoords[side][0];
				pt[1] = -dY[0];
        pt[2] = innerRadius * ringCoords[side][1];
        norm.setValue(pt[0], pt[1], pt[2]);
        norm.normalize();
        cylinderCapNormals[cylinderCapVertCount] = norm;
        cylinderCapVertices[cylinderCapVertCount++] = pt;
    }

		pt[0] = innerRadius * ringCoords[0][0];
		pt[1] = -dY[0];
    pt[2] = innerRadius * ringCoords[0][1];
    norm.setValue(pt[0], pt[1], pt[2]);
    norm.normalize();
    cylinderCapNormals[cylinderCapVertCount] = norm;
    cylinderCapVertices[cylinderCapVertCount++] = pt;
		/*
    // Bottom innermost ring.
    // Innermost ring is drawn as a triangle fan. This not
    // only gets better shading (because the center vertex is
    // sent), but also avoids the problem of having a polygon
    // with too many vertices.

    // Center point comes first
    pt[0] = pt[2] = 0.0;
    pt[1] = -1.0;
    cylinderCapNormals[cylinderCapNormCount++] = pt;
    cylinderCapVertices[cylinderCapVertCount++] = pt;

    // Send all vertices around ring
    outerRadius = dRadius[0];
    pt[1] = -dY[0];
    for (side = 0; side < numSides; side++) {
        pt[0] = outerRadius * ringCoords[side][0];
        pt[2] = outerRadius * ringCoords[side][1];
        norm.setValue(pt[0], pt[1], pt[2]);
        norm.normalize();
        cylinderCapNormals[cylinderCapNormCount++] = norm;
        cylinderCapVertices[cylinderCapVertCount++] = pt;
    }
    // Send first vertex again
    pt[0] = outerRadius * ringCoords[0][0];
    pt[2] = outerRadius * ringCoords[0][1];
    norm.setValue(pt[0], pt[1], pt[2]);
    norm.normalize();
    cylinderCapNormals[cylinderCapNormCount++] = norm;
    cylinderCapVertices[cylinderCapVertCount++] = pt;
		*/
}
// <-- EGB && SGB round caps array

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Issues GL calls to display rounded cylinder caps.
//

// --> EGB && SGB
//static void
//renderRoundCap(const SbMatrix &theVertexMatrix,
//    const SbBool bottom, const SbBool doNormals, const SbBool doTextures,
//	const int numSections, const int numSides,
//	const int cylinderBottomStartNorm, const int cylinderBottomStartVert,
//	const SbVec3f *cylinderCapNormals, const SbVec3f *cylinderCapVertices)
void
ChemUnitCylinder::renderRoundCap(const SbMatrix &theVertexMatrix,
  const SbBool bottom, const SbBool doNormals, const SbBool doTextures,
	const int numSections, const int numSides,
	const int cylinderBottomStartNorm, const int cylinderBottomStartVert,
	const SbVec3f *cylinderCapNormals, const SbVec3f *cylinderCapVertices,
	int bondCylinderDisplayStyle/* = ChemDisplayParam::BondCylinderBinding::BONDCYLINDER_ROUNDCAP*/) const
// <-- EGB && SGB
//
////////////////////////////////////////////////////////////////////////
{
// --> EGB && SGB round caps array
	switch (bondCylinderDisplayStyle)
	{
		case ChemDisplayParam::BONDCYLINDER_ARRAY_ROUNDCAP:
		{
			glPushMatrix();
				glMultMatrixf((const float*)&theVertexMatrix);
					glDrawArrays(GL_TRIANGLE_STRIP,cylTopArrayStartIndex,cylRoundCapArrayVertCount);
					glDrawArrays(GL_TRIANGLE_STRIP,cylTopArrayStartIndex+cylRoundCapArrayVertCount,cylRoundCapArrayVertCount);
			glPopMatrix();
			break;
		}
		case ChemDisplayParam::BONDCYLINDER_SEMI_ROUNDCAP:
		{
			glPushMatrix();
				glMultMatrixf((const float*)&theVertexMatrix);
					glDrawArrays(GL_TRIANGLE_STRIP,semiCylVertCount,cylRoundCapArrayVertCount);
					glDrawArrays(GL_TRIANGLE_STRIP,semiCylVertCount+cylRoundCapArrayVertCount,cylRoundCapArrayVertCount);
			glPopMatrix();
			break;
		}
		default:
		{
// <-- EGB && SGB round caps array
    int     side;
		int	    section;
    int     normCount;
    int     vertCount;
    float   s, t;
    SbVec3f vec;
    SbVec3f tmpVec;


    SbMatrix theNormalMatrix = theVertexMatrix.inverse().transpose();

    if (bottom) {
        normCount = cylinderBottomStartNorm;
        vertCount = cylinderBottomStartVert;
    }
    else {
        normCount = vertCount = 0;
    }


    // Don't do normals or textures
			if (!doNormals && !doTextures) {
				for (section = numSections-1; section > 0; --section) {
						glBegin(GL_TRIANGLE_STRIP);
						for (int side = 0; side < numSides; side++) {
									DISP_ROUNDCAP_VERTEX;
									DISP_ROUNDCAP_VERTEX;
						}
							DISP_ROUNDCAP_VERTEX;
							DISP_ROUNDCAP_VERTEX;
						glEnd();
				}
				glBegin(GL_TRIANGLE_FAN);
					DISP_ROUNDCAP_VERTEX;
				for (side = numSides - 1; side >= 0; side--) {
							DISP_ROUNDCAP_VERTEX;
				}
					DISP_ROUNDCAP_VERTEX;
				glEnd();
		}
			else if (doNormals && !doTextures) {
				for (section = numSections-1; section > 0; --section) {
						glBegin(GL_TRIANGLE_STRIP);
						for (int side = 0; side < numSides; side++) {
									DISP_ROUNDCAP_NORMAL;
									DISP_ROUNDCAP_VERTEX;

									DISP_ROUNDCAP_NORMAL;
									DISP_ROUNDCAP_VERTEX;
						}
							DISP_ROUNDCAP_NORMAL;
							DISP_ROUNDCAP_VERTEX;

							DISP_ROUNDCAP_NORMAL;
							DISP_ROUNDCAP_VERTEX;
						glEnd();
				}
				glBegin(GL_TRIANGLE_FAN);
					DISP_ROUNDCAP_NORMAL;
					DISP_ROUNDCAP_VERTEX;
				for (side = numSides - 1; side >= 0; side--) {
							DISP_ROUNDCAP_NORMAL;
							DISP_ROUNDCAP_VERTEX;
				}
					DISP_ROUNDCAP_NORMAL;
					DISP_ROUNDCAP_VERTEX;
				glEnd();
		}
			else if (!doNormals && doTextures) {
					if (bottom) {
    				for (section = numSections-1; section > 0; --section) {
    						glBegin(GL_TRIANGLE_STRIP);
    						for (int side = 0; side < numSides; side++) {
											DISP_ROUNDCAP_BOT_TEXTURE;
											DISP_ROUNDCAP_BOT_TEXTURE;
    						}
									DISP_ROUNDCAP_BOT_TEXTURE;
									DISP_ROUNDCAP_BOT_TEXTURE;
    						glEnd();
    				}
    				glBegin(GL_TRIANGLE_FAN);
							glTexCoord2f(0.5, 0.5);
							DISP_ROUNDCAP_VERTEX;
    				for (side = numSides - 1; side >= 0; side--) {
									DISP_ROUNDCAP_BOT_TEXTURE;
    				}
							DISP_ROUNDCAP_BOT_TEXTURE;
    				glEnd();
					}
					else {
    				for (section = numSections-1; section > 0; --section) {
    						glBegin(GL_TRIANGLE_STRIP);
    						for (int side = 0; side < numSides; side++) {
											DISP_ROUNDCAP_TOP_TEXTURE;
											DISP_ROUNDCAP_TOP_TEXTURE;
    						}
									DISP_ROUNDCAP_TOP_TEXTURE;
									DISP_ROUNDCAP_TOP_TEXTURE;
    						glEnd();
    				}
    				glBegin(GL_TRIANGLE_FAN);
							glTexCoord2f(0.5, 0.5);
							DISP_ROUNDCAP_VERTEX;
    				for (side = numSides - 1; side >= 0; side--) {
									DISP_ROUNDCAP_TOP_TEXTURE;
    				}
							DISP_ROUNDCAP_TOP_TEXTURE;
    				glEnd();
					}
		}
			else if (doNormals && doTextures) {
					if (bottom) {
    				for (section = numSections-1; section > 0; --section) {
    						glBegin(GL_TRIANGLE_STRIP);
    						for (int side = 0; side < numSides; side++) {
											DISP_ROUNDCAP_NORMAL;
											DISP_ROUNDCAP_BOT_TEXTURE;
    
											DISP_ROUNDCAP_NORMAL;
											DISP_ROUNDCAP_BOT_TEXTURE;
    						}
									DISP_ROUNDCAP_NORMAL;
									DISP_ROUNDCAP_BOT_TEXTURE;
    
									DISP_ROUNDCAP_NORMAL;
									DISP_ROUNDCAP_BOT_TEXTURE;
    						glEnd();
    				}
    				glBegin(GL_TRIANGLE_FAN);
							DISP_ROUNDCAP_NORMAL;
							glTexCoord2f(0.5, 0.5);
							DISP_ROUNDCAP_VERTEX;
    				for (side = numSides - 1; side >= 0; side--) {
									DISP_ROUNDCAP_NORMAL;
									DISP_ROUNDCAP_BOT_TEXTURE;
    				}
							DISP_ROUNDCAP_NORMAL;
							DISP_ROUNDCAP_BOT_TEXTURE;
    				glEnd();
					}
					else {
    				for (section = numSections-1; section > 0; --section) {
    						glBegin(GL_TRIANGLE_STRIP);
    						for (int side = 0; side < numSides; side++) {
											DISP_ROUNDCAP_NORMAL;
											DISP_ROUNDCAP_TOP_TEXTURE;
    
											DISP_ROUNDCAP_NORMAL;
											DISP_ROUNDCAP_TOP_TEXTURE;
    						}
									DISP_ROUNDCAP_NORMAL;
									DISP_ROUNDCAP_TOP_TEXTURE;
    
									DISP_ROUNDCAP_NORMAL;
									DISP_ROUNDCAP_TOP_TEXTURE;
    						glEnd();
    				}
    				glBegin(GL_TRIANGLE_FAN);
							DISP_ROUNDCAP_NORMAL;
							glTexCoord2f(0.5, 0.5);
							DISP_ROUNDCAP_VERTEX;
    				for (side = numSides - 1; side >= 0; side--) {
									DISP_ROUNDCAP_NORMAL;
									DISP_ROUNDCAP_TOP_TEXTURE;
    				}
							DISP_ROUNDCAP_NORMAL;
							DISP_ROUNDCAP_TOP_TEXTURE;
    				glEnd();
					}
				}
// --> EGB && SGB round caps array
			break;
			}
		} // switch (bondCylinderDisplayStyle)
// <-- EGB && SGB round caps array
}

#undef DISP_ROUNDCAP_NORMAL
#undef DISP_ROUNDCAP_VERTEX
#undef DISP_ROUNDCAP_TOP_TEXTURE
#undef DISP_ROUNDCAP_BOT_TEXTURE

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Constructor
//
// Use: public   
   
ChemUnitCylinder::ChemUnitCylinder(short theCapType, float complexity)
//
////////////////////////////////////////////////////////////////////////
{
    numSides = 0;
    numSections = 0;
    numRoundCapSections = 3;

// --> EGB && SGB
//    capType = theCapType;
		capType = theCapType % 3;
// <-- EGB && SGB
    cylBottomStartNorm = 0;
    cylBottomStartVert = 0;

    cylNormCount = 0;
    cylVertCount = 0;
    cylNormals = NULL;
    cylVertices = NULL;

    roundCapNormCount = 0;
    roundCapVertCount = 0;
    roundCapNormals = NULL;
    roundCapVertices = NULL;

    ringCoords = NULL;

    computeDepth(complexity, numSections, numSides);

// --> EGB && SGB
		cylArrayVertCount = 0;
		cylCapArrayVertCount = 0;
		cylRoundCapArrayVertCount = 0;
		semiCylVertCount = 0;

		cylTopArrayStartIndex = 0;
		
		arrayCylNormals = NULL;

		cylinder = gluNewQuadric();
		topCap = gluNewQuadric();
		bottomCap = gluNewQuadric();

		soCylinder = new SoCylinder;//default height = 2.0, default radius = 1.0
		if (capType == 0) soCylinder->parts = 0x1;

		cylTexCoordsGenerated = false;

		computeGLUDepth(complexity, GLUslices, GLUstacks);
// <-- EGB && SGB

    // Allocate memory based on numSections and numSides.

    // First the ringCoords
    ringCoords = new SbVec2f[numSides];

    int32_t size;
    // Now the cylinder.  This create enough room for the flat caps.
    // Allocate normals
    // Sides
    size = numSections*numSides + numSections;
    // Top and bottom flat caps
    size += 2;
    cylNormals = new SbVec3f[size];

    // Allocate vertices
    // Sides
// --> EGB && SGB
		cylArrayVertCount = 2*numSections*(numSides+1);
// <-- EGB && SGB
    size = 2*numSections*(numSides+1);
    // Top and bottom flat caps
    size += 2 * (2*(numSections-1) * (numSides + 1) + 2 + numSides);
// --> EGB && SGB flat caps array
		cylCapArrayVertCount = (numSides+2);
		size += 2*cylCapArrayVertCount;
// <-- EGB && SGB flat caps array
// --> EGB && SGB round caps array
		cylRoundCapArrayVertCount =  (numRoundCapSections-1)*(numSides*2+2) + (2*numSides+1);
		size += 2*cylRoundCapArrayVertCount;
// <-- EGB && SGB round caps array
    cylVertices = new SbVec3f[size];

		
// --> EGB && SGB
		arrayCylNormals = new SbVec3f[size];
		sidesTexCoords = NULL;
		topCapTexCoords = NULL;
		bottomCapTexCoords = NULL;
		roundCapTexCoords = NULL;
// <-- EGB && SGB

// --> EGB && SGB
		semiCylVertCount = numSides + numSides%2 + 2;
		int semiSize = semiCylVertCount;
// --> EGB && SGB flat caps array
		semiSize += 2*cylCapArrayVertCount;
// <-- EGB && SGB flat caps array
// --> EGB && SGB round caps array
		semiSize += 2*cylRoundCapArrayVertCount;
// <-- EGB && SGB round caps array

		semiCylVertices = new SbVec3f[semiSize];
		semiCylNormals = new SbVec3f[semiSize];
// <-- EGB && SGB

    // Generate the ringCoords
    computeRingCoords(numSides, ringCoords);

    // Generate the normals and vertices for the cylinder sides
// --> EGB && SGB
//    generateCylVertices(numSections, numSides, ringCoords,cylNormCount, cylVertCount, cylNormals, cylVertices);
		  generateCylVertices(numSections, numSides, ringCoords,cylNormCount, cylVertCount, cylNormals, cylVertices,arrayCylNormals);
// <-- EGB && SGB

// --> EGB && SGB
			generateSemiCylVertices();
// <-- EGB && SGB

    if (capType == ChemUnitCylinder::FLATCAP)
		{
// --> EGB && SGB
			cylTopArrayStartIndex = cylVertCount + 2 * (2*(numSections-1) * (numSides + 1) + 2 + numSides);
        //generateFlatCapVertices(numSections, numSides, ringCoords, cylNormCount, cylVertCount, cylNormals, cylVertices);
			generateFlatCapVertices(numSections, numSides, 
				ringCoords, cylNormCount, cylVertCount, 
				cylNormals, cylVertices,arrayCylNormals);

			// generate flat caps of semicylinder
			int semiCylVertCountIndex = semiCylVertCount;
			generateFlatCapVerticesSemiCylinder(numSides,semiCylVertCountIndex,
				semiCylNormals,semiCylVertices);
			
// <-- EGB && SGB
    }
    else if (capType == ChemUnitCylinder::ROUNDCAP) {
        // Allocate space for the normals and vertices
        size = 2*(numRoundCapSections-1)*numSides + 2*(numRoundCapSections-1);
        size += numSides + 2;
        roundCapNormals = new SbVec3f[2*size];
        roundCapVertices = new SbVec3f[2*size];

        // Create the normals and vertices
        generateRoundCapVertices(numRoundCapSections, numSides,
            ringCoords,
            roundCapNormCount, roundCapVertCount,
            cylBottomStartNorm, cylBottomStartVert,
            roundCapNormals, roundCapVertices);

// --> EGB && SGB round caps array
				cylTopArrayStartIndex = cylVertCount;
        generateRoundCapVerticesArray(numRoundCapSections, numSides,
					ringCoords,cylVertCount,arrayCylNormals, cylVertices);

				// generate round caps array for semicylinder
				int semiCylVertCountIndex = semiCylVertCount;
				generateRoundCapVerticesArray(numRoundCapSections, numSides,
					ringCoords,semiCylVertCountIndex,semiCylNormals, semiCylVertices);
// <-- EGB && SGB round caps array
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Constructor
//
// Use: public   
   
ChemUnitCylinder::~ChemUnitCylinder()
//
////////////////////////////////////////////////////////////////////////
{
    if (ringCoords != NULL) delete [] ringCoords;
    if (cylNormals != NULL) delete [] cylNormals;
    if (cylVertices != NULL) delete [] cylVertices;
    if (roundCapNormals != NULL) delete [] roundCapNormals;
    if (roundCapVertices != NULL) delete [] roundCapVertices;
// --> EGB && SGB
		if (arrayCylNormals != NULL) delete [] arrayCylNormals;
		if (sidesTexCoords != NULL) delete [] sidesTexCoords;
		if (topCapTexCoords != NULL) delete [] topCapTexCoords;
		if (bottomCapTexCoords != NULL) delete [] bottomCapTexCoords;
		if (roundCapTexCoords != NULL) delete [] roundCapTexCoords;

		if (semiCylVertices != NULL) delete [] semiCylVertices;
		if (semiCylNormals != NULL) delete [] semiCylNormals;

		if (cylinder !=	NULL) gluDeleteQuadric(cylinder);
		if (topCap !=	NULL) gluDeleteQuadric(topCap);
		if (bottomCap !=	NULL) gluDeleteQuadric(bottomCap);
// <-- EGB && SGB
}

#define DISP_NORMAL                                                           \
theNormalMatrix.multDirMatrix(cylNormals[normCount++], vec);                  \
glNormal3fv(vec.getValue());

#define DISP_VERTEX                                                           \
theVertexMatrix.multVecMatrix(cylVertices[vertCount++], vec);                 \
glVertex3fv(vec.getValue());

#define DISP_FLATCAP_TOP_TEXTURE                                              \
vec = cylVertices[vertCount++];                                               \
s = TOP_TEX_S(vec[0]);                                                        \
t = TOP_TEX_T(vec[2]);                                                        \
glTexCoord2f(s, t);                                                           \
theVertexMatrix.multVecMatrix(vec, tmpVec);                                   \
glVertex3fv(tmpVec.getValue());

#define DISP_FLATCAP_BOT_TEXTURE                                              \
vec = cylVertices[vertCount++];                                               \
s = BOT_TEX_S(vec[0]);                                                        \
t = BOT_TEX_T(vec[2]);                                                        \
glTexCoord2f(s, t);                                                           \
theVertexMatrix.multVecMatrix(vec, tmpVec);                                   \
glVertex3fv(tmpVec.getValue());

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Issue the GL calls to render the cylinder and flat caps if any
//
// Use: static   
// --> EGB && SGB
//static void
//renderCylinder(const SbMatrix &theVertexMatrix, 
//    const SbBool doCaps, const SbBool doNormals, const SbBool doTextures,
//    const int32_t numSections, const int32_t numSides,
//		const SbVec3f *cylNormals, const SbVec3f *cylVertices)
/*
void
renderCylinder(const SbMatrix &theVertexMatrix, 
    const SbBool doCaps, const SbBool doNormals, const SbBool doTextures,
    const int32_t numSections, const int32_t numSides,
		const SbVec3f *cylNormals, const SbVec3f *cylVertices,
		GLUquadricObj *cylinder = NULL, GLUquadricObj *topCap = NULL, 
		GLUquadricObj* bottomCap = NULL,
		const int32_t GLUnumSlices = 0, const int32_t GLUnumStacks = 0,
		int bondCylinderDisplayStyle = ChemDisplayParam::BondCylinderBinding::BONDCYLINDER_DEFAULT,
		const SbVec3f *arrayCylNormals = NULL)
*/
void ChemUnitCylinder::renderCylinder(const SbMatrix &theVertexMatrix, 
    const SbBool doCaps, const SbBool doNormals, const SbBool doTextures,
    const int bondCylinderDisplayStyle
		/*= ChemDisplayParam::BondCylinderBinding::BONDCYLINDER_DEFAULT*/,
		SoGLRenderAction *action ) const
//
////////////////////////////////////////////////////////////////////////
{
    int32_t normCount, vertCount;
    int32_t section, side;
    float   s, t;
    float   ds, dt;
    float   tTop, tBot;
    SbVec3f vec;
    SbVec3f tmpVec;

// --> EGB && SGB
		if (bondCylinderDisplayStyle == ChemDisplayParam::BONDCYLINDER_NOCAP 
			|| bondCylinderDisplayStyle == ChemDisplayParam::BONDCYLINDER_FLATCAP
			|| bondCylinderDisplayStyle == ChemDisplayParam::BONDCYLINDER_ROUNDCAP
			)
		{
// <-- EGB && SGB
			SbMatrix theNormalMatrix = theVertexMatrix.inverse().transpose();//THE GREAT CHAPU

			 // Don't do normals or textures
			if (!doNormals && !doTextures) {
					// Do the cylinder sides
					normCount = 0;
					vertCount = 0;
					for (section = 0; section < numSections; section++) {
							glBegin(GL_TRIANGLE_STRIP);
							for (side = 0; side < numSides; side++) {
									DISP_VERTEX;
									DISP_VERTEX;
							}
							DISP_VERTEX;
							DISP_VERTEX;
							glEnd();
					}

					if (doCaps) {
							// TOP
							for (section = numSections - 1; section > 0; --section) {
									glBegin(GL_TRIANGLE_STRIP);
									for (side = 0; side < numSides; side++) {
											DISP_VERTEX;
											DISP_VERTEX;
									}
									DISP_VERTEX;
									DISP_VERTEX;
									glEnd();
							}
							glBegin(GL_TRIANGLE_FAN);
							DISP_VERTEX;
							for (side = numSides - 1; side >= 0; side--) {
									DISP_VERTEX;
							}
							DISP_VERTEX;
							glEnd();

            
							// BOTTOM
							for (section = numSections - 1; section > 0; --section) {
									glBegin(GL_TRIANGLE_STRIP);
									for (side = numSides - 1; side >= 0; side--) {
											DISP_VERTEX;
											DISP_VERTEX;
									}
									DISP_VERTEX;
									DISP_VERTEX;
									glEnd();
							}
							glBegin(GL_TRIANGLE_FAN);
							DISP_VERTEX;
							for (side = 0; side < numSides; side++) {
									DISP_VERTEX;
							}
							DISP_VERTEX;
							glEnd();
					}
			}
			else if (doNormals && !doTextures) {
					// Do the cylinder sides
					normCount = 0;
					vertCount = 0;
					for (section = 0; section < numSections; section++) {
							glBegin(GL_TRIANGLE_STRIP);
							for (side = 0; side < numSides; side++) {
									DISP_NORMAL;
									DISP_VERTEX;
									DISP_VERTEX;
							}
							DISP_NORMAL;
							DISP_VERTEX;
							DISP_VERTEX;
							glEnd();
					}

					if (doCaps) {
							// TOP
							DISP_NORMAL;
							for (section = numSections - 1; section > 0; --section) {
									glBegin(GL_TRIANGLE_STRIP);
									for (side = 0; side < numSides; side++) {
											DISP_VERTEX;
											DISP_VERTEX;
									}
									DISP_VERTEX;
									DISP_VERTEX;
									glEnd();
							}
							glBegin(GL_TRIANGLE_FAN);
							DISP_VERTEX;
							for (side = numSides - 1; side >= 0; side--) {
									DISP_VERTEX;
							}
							DISP_VERTEX;
							glEnd();
            
							// BOTTOM
							DISP_NORMAL;
							for (section = numSections - 1; section > 0; --section) {
									glBegin(GL_TRIANGLE_STRIP);
									for (side = numSides - 1; side >= 0; side--) {
											DISP_VERTEX;
											DISP_VERTEX;
									}
									DISP_VERTEX;
									DISP_VERTEX;
									glEnd();
							}
							glBegin(GL_TRIANGLE_FAN);
							DISP_VERTEX;
							for (side = 0; side < numSides; side++) {
									DISP_VERTEX;
							}
							DISP_VERTEX;
							glEnd();
					}
			}
			else if (!doNormals && doTextures) {
					// Do the cylinder sides
					normCount = 0;
					vertCount = 0;
					tTop = 1.0;
					dt = -1.0 / numSections;
					ds = -1.0 / numSides;
					for (section = 0; section < numSections; section++) {
							s = 1.0;
							tBot = tTop + dt;
							glBegin(GL_TRIANGLE_STRIP);
							for (side = 0; side < numSides; side++) {
									glTexCoord2f(s, tBot);
									DISP_VERTEX;

									glTexCoord2f(s, tTop);
									DISP_VERTEX;

									s += ds;
							}
							s = 0.0;
							glTexCoord2f(s, tBot);
							DISP_VERTEX;

							glTexCoord2f(s, tTop);
							DISP_VERTEX;
							glEnd();

							tTop = tBot;
					}

					if (doCaps) {
							// TOP
							for (section = numSections - 1; section > 0; --section) {
									glBegin(GL_TRIANGLE_STRIP);
									for (side = 0; side < numSides; side++) {
											DISP_FLATCAP_TOP_TEXTURE;
											DISP_FLATCAP_TOP_TEXTURE;
									}
									DISP_FLATCAP_TOP_TEXTURE;
									DISP_FLATCAP_TOP_TEXTURE;
									glEnd();
							}
							glBegin(GL_TRIANGLE_FAN);
							glTexCoord2f(0.5, 0.5);
							DISP_VERTEX;
							for (side = numSides - 1; side >= 0; side--) {
									DISP_FLATCAP_TOP_TEXTURE;
							}
							DISP_FLATCAP_TOP_TEXTURE;
							glEnd();

            
							// BOTTOM
							for (section = numSections - 1; section > 0; --section) {
									glBegin(GL_TRIANGLE_STRIP);
									for (side = numSides - 1; side >= 0; side--) {
											DISP_FLATCAP_BOT_TEXTURE;
											DISP_FLATCAP_BOT_TEXTURE;
									}
									DISP_FLATCAP_BOT_TEXTURE;
									DISP_FLATCAP_BOT_TEXTURE;
									glEnd();
							}
							glBegin(GL_TRIANGLE_FAN);
							glTexCoord2f(0.5, 0.5);
							DISP_VERTEX;
							for (side = 0; side < numSides; side++) {
									DISP_FLATCAP_BOT_TEXTURE;
							}
							DISP_FLATCAP_BOT_TEXTURE;
							glEnd();
					}
			}
			else if (doNormals && doTextures) {
					// Do the cylinder sides
					normCount = 0;
					vertCount = 0;
					tTop = 1.0;
					dt = -1.0 / numSections;
					ds = -1.0 / numSides;
					for (section = 0; section < numSections; section++) {
							s = 1.0;
							tBot = tTop + dt;
							glBegin(GL_TRIANGLE_STRIP);
							for (side = 0; side < numSides; side++) {
									DISP_NORMAL;
									glTexCoord2f(s, tBot);
									DISP_VERTEX;

									glTexCoord2f(s, tTop);
									DISP_VERTEX;

									s += ds;
							}
							s = 0.0;
							DISP_NORMAL;
							glTexCoord2f(s, tBot);
							DISP_VERTEX;

							glTexCoord2f(s, tTop);
							DISP_VERTEX;
							glEnd();

							tTop = tBot;
					}

					if (doCaps) {
							// TOP
							DISP_NORMAL;
							for (section = numSections - 1; section > 0; --section) {
									glBegin(GL_TRIANGLE_STRIP);
									for (side = 0; side < numSides; side++) {
											DISP_FLATCAP_TOP_TEXTURE;
											DISP_FLATCAP_TOP_TEXTURE;
									}
									DISP_FLATCAP_TOP_TEXTURE;
									DISP_FLATCAP_TOP_TEXTURE;
									glEnd();
							}
							glBegin(GL_TRIANGLE_FAN);
							glTexCoord2f(0.5, 0.5);
							DISP_VERTEX;
							for (side = numSides - 1; side >= 0; side--) {
									DISP_FLATCAP_TOP_TEXTURE;
							}
							DISP_FLATCAP_TOP_TEXTURE;
							glEnd();

            
							// BOTTOM
							DISP_NORMAL;
							for (section = numSections - 1; section > 0; --section) {
									glBegin(GL_TRIANGLE_STRIP);
									for (side = numSides - 1; side >= 0; side--) {
											DISP_FLATCAP_BOT_TEXTURE;
											DISP_FLATCAP_BOT_TEXTURE;
									}
									DISP_FLATCAP_BOT_TEXTURE;
									DISP_FLATCAP_BOT_TEXTURE;
									glEnd();
							}
							glBegin(GL_TRIANGLE_FAN);
							glTexCoord2f(0.5, 0.5);
							DISP_VERTEX;
							for (side = 0; side < numSides; side++) {
									DISP_FLATCAP_BOT_TEXTURE;
							}
							DISP_FLATCAP_BOT_TEXTURE;
							glEnd();
					}
			}
/////////////////////////////////////////////////////////////////////////
// --> EGB && SGB
		}
		else
		{

			// Don't do normals or textures
			if (!doNormals && !doTextures)
			{
					// Do the cylinder sides

				switch (bondCylinderDisplayStyle)
				{
					case ChemDisplayParam::BONDCYLINDER_ARRAY_NOCAP:
					case ChemDisplayParam::BONDCYLINDER_ARRAY_FLATCAP:
					case ChemDisplayParam::BONDCYLINDER_ARRAY_ROUNDCAP:
					{
						glPushMatrix();
							glMultMatrixf((const float*)&theVertexMatrix);

								glDrawArrays(GL_TRIANGLE_STRIP,0,cylArrayVertCount);
								if (doCaps)
								{
									glDrawArrays(GL_TRIANGLE_FAN,cylTopArrayStartIndex,cylCapArrayVertCount);
									glDrawArrays(GL_TRIANGLE_FAN,cylTopArrayStartIndex+cylCapArrayVertCount,cylCapArrayVertCount);
								}
						glPopMatrix();

						break;
					}

					case ChemDisplayParam::BONDCYLINDER_SEMI_NOCAP:
					case ChemDisplayParam::BONDCYLINDER_SEMI_FLATCAP:
					case ChemDisplayParam::BONDCYLINDER_SEMI_ROUNDCAP:
					{
						glPushMatrix();
							glMultMatrixf((const float*)&theVertexMatrix);
								
							glDrawArrays(GL_TRIANGLE_STRIP,0,semiCylVertCount);
						
							if (doCaps)
							{
								glDrawArrays(GL_TRIANGLE_FAN,semiCylVertCount,cylCapArrayVertCount);
								glDrawArrays(GL_TRIANGLE_FAN,semiCylVertCount+cylCapArrayVertCount,cylCapArrayVertCount);
							}
						glPopMatrix();

						break;
					}

					case ChemDisplayParam::BONDCYLINDER_GLUCYLINDER_NOCAP:
					case ChemDisplayParam::BONDCYLINDER_GLUCYLINDER_FLATCAP:
					case ChemDisplayParam::BONDCYLINDER_GLUCYLINDER_ROUNDCAP:
					{ 
						glPushMatrix();
							glMultMatrixf((const float*)&theVertexMatrix);
							gluCylinder(cylinder,1,1,2,GLUslices,GLUstacks);
							if (doCaps)
							{
								gluDisk(topCap,0,1,GLUslices,1);

								// bottom cap is +2
								glTranslatef(theVertexMatrix[0][3],theVertexMatrix[1][3],theVertexMatrix[2][3]+2);
								gluDisk(bottomCap,0,1,GLUslices,1);
							}
						glPopMatrix();
						
						break;
					}
					
					case ChemDisplayParam::BONDCYLINDER_SOCYLINDER_NOCAP:
					case ChemDisplayParam::BONDCYLINDER_SOCYLINDER_FLATCAP:
					case ChemDisplayParam::BONDCYLINDER_SOCYLINDER_ROUNDCAP:
					{
						glPushMatrix();

							glMultMatrixf((const float*)&theVertexMatrix);
							if (action!= NULL) soCylinder->GLRender(action);
						
						glPopMatrix();

						break;
					}
				}
			}
			else if (doNormals && !doTextures)
			{
					// Do the cylinder sides
				switch (bondCylinderDisplayStyle)
				{
					case ChemDisplayParam::BONDCYLINDER_ARRAY_NOCAP:
					case ChemDisplayParam::BONDCYLINDER_ARRAY_FLATCAP:
					case ChemDisplayParam::BONDCYLINDER_ARRAY_ROUNDCAP:
					{
						glPushMatrix();
							glMultMatrixf((const float*)&theVertexMatrix);

								glDrawArrays(GL_TRIANGLE_STRIP,0,cylArrayVertCount);
								if (doCaps)
								{
									glDrawArrays(GL_TRIANGLE_FAN,cylTopArrayStartIndex,cylCapArrayVertCount);
									glDrawArrays(GL_TRIANGLE_FAN,cylTopArrayStartIndex+cylCapArrayVertCount,cylCapArrayVertCount);
								}
						glPopMatrix();

						break;
					}
					case ChemDisplayParam::BONDCYLINDER_SEMI_NOCAP:
					case ChemDisplayParam::BONDCYLINDER_SEMI_FLATCAP:
					case ChemDisplayParam::BONDCYLINDER_SEMI_ROUNDCAP:
					{
						glPushMatrix();
							glMultMatrixf((const float*)&theVertexMatrix);
								
							glDrawArrays(GL_TRIANGLE_STRIP,0,semiCylVertCount);
						
							if (doCaps)
							{
								glDrawArrays(GL_TRIANGLE_FAN,semiCylVertCount,cylCapArrayVertCount);
								glDrawArrays(GL_TRIANGLE_FAN,semiCylVertCount+cylCapArrayVertCount,cylCapArrayVertCount);
							}
						glPopMatrix();

						break;
					}

					case ChemDisplayParam::BONDCYLINDER_GLUCYLINDER_NOCAP:
					case ChemDisplayParam::BONDCYLINDER_GLUCYLINDER_FLATCAP:
					case ChemDisplayParam::BONDCYLINDER_GLUCYLINDER_ROUNDCAP:
					{
						glPushMatrix();									
							glMultMatrixf((const float*)&theVertexMatrix);
							gluQuadricNormals(cylinder,GL_SMOOTH);
							// the height must be 2 'cause height is divided in render()
							gluCylinder(cylinder,1,1,2,GLUslices,GLUstacks);
							if (doCaps)
							{
								gluQuadricNormals(topCap,GL_SMOOTH);
								gluDisk(topCap,0,1,GLUslices,1);
								// bottom cap is +2
								glTranslatef(theVertexMatrix[0][3],theVertexMatrix[1][3],theVertexMatrix[2][3]+2);
								gluQuadricNormals(bottomCap,GL_SMOOTH);
								gluDisk(bottomCap,0,1,GLUslices,1);
							}
						glPopMatrix();

						break;
					}

					case ChemDisplayParam::BONDCYLINDER_SOCYLINDER_NOCAP:
					case ChemDisplayParam::BONDCYLINDER_SOCYLINDER_FLATCAP:
					case ChemDisplayParam::BONDCYLINDER_SOCYLINDER_ROUNDCAP:
					{
						glPushMatrix();

							glMultMatrixf((const float*)&theVertexMatrix);
							if (action!= NULL) soCylinder->GLRender(action);
						
						glPopMatrix();

						break;
					}
				}

			}
			else if (!doNormals && doTextures)
			{
				switch (bondCylinderDisplayStyle)
				{
					case ChemDisplayParam::BONDCYLINDER_ARRAY_NOCAP:
					case ChemDisplayParam::BONDCYLINDER_ARRAY_FLATCAP:
					case ChemDisplayParam::BONDCYLINDER_ARRAY_ROUNDCAP:
					{
						glPushMatrix();
							glMultMatrixf((const float*)&theVertexMatrix);

								glDrawArrays(GL_TRIANGLE_STRIP,0,cylArrayVertCount);
								if (doCaps)
								{
									glDrawArrays(GL_TRIANGLE_FAN,cylTopArrayStartIndex,cylCapArrayVertCount);
									glDrawArrays(GL_TRIANGLE_FAN,cylTopArrayStartIndex+cylCapArrayVertCount,cylCapArrayVertCount);
								}
						glPopMatrix();

						break;
					}

					case ChemDisplayParam::BONDCYLINDER_SEMI_NOCAP:
					case ChemDisplayParam::BONDCYLINDER_SEMI_FLATCAP:
					case ChemDisplayParam::BONDCYLINDER_SEMI_ROUNDCAP:
					{
						glPushMatrix();
							glMultMatrixf((const float*)&theVertexMatrix);
								
							glDrawArrays(GL_TRIANGLE_STRIP,0,semiCylVertCount);
						
							if (doCaps)
							{
								glDrawArrays(GL_TRIANGLE_FAN,semiCylVertCount,cylCapArrayVertCount);
								glDrawArrays(GL_TRIANGLE_FAN,semiCylVertCount+cylCapArrayVertCount,cylCapArrayVertCount);
							}
						glPopMatrix();

						break;
					}

					case ChemDisplayParam::BONDCYLINDER_GLUCYLINDER_NOCAP:
					case ChemDisplayParam::BONDCYLINDER_GLUCYLINDER_FLATCAP:
					case ChemDisplayParam::BONDCYLINDER_GLUCYLINDER_ROUNDCAP:
					{
						glPushMatrix();									
							glMultMatrixf((const float*)&theVertexMatrix);
							gluQuadricTexture(cylinder,GL_TRUE);
							gluCylinder(cylinder,1,1,2,GLUslices,GLUstacks);
							if (doCaps)
							{
								gluQuadricTexture(topCap,GL_TRUE);
								gluDisk(topCap,0,1,GLUslices,1);

								// bottom cap is +2
								glTranslatef(theVertexMatrix[0][3],theVertexMatrix[1][3],theVertexMatrix[2][3]+2);
								gluQuadricTexture(bottomCap,GL_TRUE);
								gluDisk(bottomCap,0,1,GLUslices,1);
							}
						glPopMatrix();

						break;
					}

					case ChemDisplayParam::BONDCYLINDER_SOCYLINDER_NOCAP:
					case ChemDisplayParam::BONDCYLINDER_SOCYLINDER_FLATCAP:
					case ChemDisplayParam::BONDCYLINDER_SOCYLINDER_ROUNDCAP:
					{
						glPushMatrix();

							glMultMatrixf((const float*)&theVertexMatrix);
							if (action!= NULL) soCylinder->GLRender(action);
						
						glPopMatrix();

						break;
					}
				}
			}
			else if (doNormals && doTextures) 
			{
				switch (bondCylinderDisplayStyle)
				{
					case ChemDisplayParam::BONDCYLINDER_ARRAY_NOCAP:
					case ChemDisplayParam::BONDCYLINDER_ARRAY_FLATCAP:
					case ChemDisplayParam::BONDCYLINDER_ARRAY_ROUNDCAP:
					{
						glPushMatrix();
							glMultMatrixf((const float*)&theVertexMatrix);

								glDrawArrays(GL_TRIANGLE_STRIP,0,cylArrayVertCount);
								if (doCaps)
								{
									glDrawArrays(GL_TRIANGLE_FAN,cylTopArrayStartIndex,cylCapArrayVertCount);
									glDrawArrays(GL_TRIANGLE_FAN,cylTopArrayStartIndex+cylCapArrayVertCount,cylCapArrayVertCount);
								}
						glPopMatrix();

						break;
					}

					case ChemDisplayParam::BONDCYLINDER_SEMI_NOCAP:
					case ChemDisplayParam::BONDCYLINDER_SEMI_FLATCAP:
					case ChemDisplayParam::BONDCYLINDER_SEMI_ROUNDCAP:
					{
						glPushMatrix();
							glMultMatrixf((const float*)&theVertexMatrix);
								
							glDrawArrays(GL_TRIANGLE_STRIP,0,semiCylVertCount);
						
							if (doCaps)
							{
								glDrawArrays(GL_TRIANGLE_FAN,semiCylVertCount,cylCapArrayVertCount);
								glDrawArrays(GL_TRIANGLE_FAN,semiCylVertCount+cylCapArrayVertCount,cylCapArrayVertCount);
							}
						glPopMatrix();

						break;
					}

					case ChemDisplayParam::BONDCYLINDER_GLUCYLINDER_NOCAP:
					case ChemDisplayParam::BONDCYLINDER_GLUCYLINDER_FLATCAP:
					case ChemDisplayParam::BONDCYLINDER_GLUCYLINDER_ROUNDCAP:
					{
						glPushMatrix();									
							glMultMatrixf((const float*)&theVertexMatrix);
							gluQuadricTexture(cylinder,GL_TRUE);
							gluQuadricNormals(cylinder,GL_SMOOTH);
							gluCylinder(cylinder,1,1,2,GLUslices,GLUstacks);
							if (doCaps)
							{
								gluQuadricTexture(topCap,GL_TRUE);
								gluQuadricNormals(topCap,GL_SMOOTH);
								gluDisk(topCap,0,1,GLUslices,1);

								// bottom cap is +2
								glTranslatef(theVertexMatrix[0][3],theVertexMatrix[1][3],theVertexMatrix[2][3]+2);
								gluQuadricTexture(bottomCap,GL_TRUE);
								gluQuadricNormals(bottomCap,GL_SMOOTH);
								gluDisk(bottomCap,0,1,GLUslices,1);
							}
						glPopMatrix();

						break;
					}

					case ChemDisplayParam::BONDCYLINDER_SOCYLINDER_NOCAP:
					case ChemDisplayParam::BONDCYLINDER_SOCYLINDER_FLATCAP:
					case ChemDisplayParam::BONDCYLINDER_SOCYLINDER_ROUNDCAP:
					{
						if (action!= NULL)
							soCylinder->GLRender(action);

						break;
					}
				}
					
			}
		}
}
////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////
//
// Description:
//    Issues GL calls to display a cylinder.
//

// --> EGB && SGB
//void
//ChemUnitCylinder::render(const SbVec3f &from, const SbVec3f &to,
//    const float radius,
//    const SbBool doNormals, const SbBool doTextures) const
void
ChemUnitCylinder::render(const SbVec3f &from, const SbVec3f &to,
    const float radius,
    const SbBool doNormals, const SbBool doTextures,
		int bondCylinderDisplayStyle,
		const SbVec2i fromTo,
		const ChemDisplay *cd, 
		SoGLRenderAction *action /* = NULL */) const
// <-- EGB && SGB
//
////////////////////////////////////////////////////////////////////////
{
// --> cap optimization
	ChemDisplayParam* cdp = ChemDisplayParamElement::get(action->getState());
	bool renderFrom, renderTo;
	PRE_RENDER_CAP(fromTo[0],renderFrom);
	PRE_RENDER_CAP(fromTo[1],renderTo);
// <-- cap optimization
	SbMatrix theVertexMatrix;
  SbVec3f theTranslation;
  SbRotation theRotation;
  SbVec3f theScale;
  SbVec3f theCapScale;
  SbVec3f yAxis(0.0, 1.0, 0.0);
// --> EGB && SGB
	SbVec3f zAxis(0.0, 0.0, 1.0);
// <-- EGB && SGB

  SbVec3f tcoord = to - from;
  float height = tcoord.length() * 0.5;

// --> EGB && SGB
	SbBool bExtrudeY = false;
		bExtrudeY = (
		bondCylinderDisplayStyle !=
		ChemDisplayParam::BONDCYLINDER_GLUCYLINDER_NOCAP
		&&
		bondCylinderDisplayStyle !=
    ChemDisplayParam::BONDCYLINDER_GLUCYLINDER_FLATCAP
		&&
		bondCylinderDisplayStyle !=
    ChemDisplayParam::BONDCYLINDER_GLUCYLINDER_ROUNDCAP
		&& 
		bondCylinderDisplayStyle !=
		ChemDisplayParam::BONDCYLINDER_SEMI_NOCAP
		&&
		bondCylinderDisplayStyle !=
    ChemDisplayParam::BONDCYLINDER_SEMI_FLATCAP
		&&
		bondCylinderDisplayStyle !=
    ChemDisplayParam::BONDCYLINDER_SEMI_ROUNDCAP
   );

	if (bExtrudeY)
	{
// <-- EGB && SGB
		theTranslation = tcoord * 0.5f + from;
		theRotation.setValue(yAxis, tcoord);
		theScale.setValue(radius, height, radius);
// --> EGB && SGB
	}
	else
	{
		theTranslation = from;
		theRotation.setValue(zAxis, tcoord);
		theScale.setValue(radius, radius, height);
	}
// <-- EGB && SGB
// --> EGB && SGB
			CALC_SEMICYLINDER_ROTATION
// <-- EGB && SGB

	theVertexMatrix.setTransform(theTranslation, theRotation, theScale);

  SbBool doCaps = (capType == ChemUnitCylinder::FLATCAP);
// --> flatcap optimization
	doCaps = doCaps && (renderFrom == TRUE && renderTo == TRUE);
// <-- flatcap optimization
// --> EGB && SGB
//  renderCylinder(theVertexMatrix, doCaps, doNormals, doTextures,
//      numSections, numSides, cylNormals, cylVertices);
	  renderCylinder(theVertexMatrix, doCaps, doNormals, doTextures,
			bondCylinderDisplayStyle,action);
// <-- EGB && SGB

  if (capType == ChemUnitCylinder::ROUNDCAP) {
// --> EGB && SGB glu round cap error
			if (!bExtrudeY)
			{
				theRotation.setValue(yAxis, to-from);
			}
// <-- EGB && SGB glu round cap error
      theCapScale.setValue(radius, radius, radius);
			if (renderFrom)
			{
// <-- roundcap optimization
				theVertexMatrix.setTransform(from, theRotation, theCapScale);
	// --> EGB && SGB
	//      renderRoundCap(theVertexMatrix, TRUE,
	//          doNormals, doTextures,
	//          numRoundCapSections, numSides,
	//          cylBottomStartNorm, cylBottomStartVert,
	//          roundCapNormals, roundCapVertices);
				renderRoundCap(theVertexMatrix, TRUE,
						doNormals, doTextures,
						numRoundCapSections, numSides,
						cylBottomStartNorm, cylBottomStartVert,
						roundCapNormals, roundCapVertices,
						bondCylinderDisplayStyle);
	// <-- EGB && SGB
// --> roundcap optimization
			}
			if (renderTo)
			{
// <-- roundcap optimization
				theVertexMatrix.setTransform(to, theRotation, theCapScale);
	// --> EGB && SGB
	//      renderRoundCap(theVertexMatrix, FALSE,
	//          doNormals, doTextures,
	//          numRoundCapSections, numSides,
	//          cylBottomStartNorm, cylBottomStartVert,
	//          roundCapNormals, roundCapVertices);
				renderRoundCap(theVertexMatrix, FALSE,
						doNormals, doTextures,
						numRoundCapSections, numSides,
						cylBottomStartNorm, cylBottomStartVert,
						roundCapNormals, roundCapVertices,
						bondCylinderDisplayStyle);
	// <-- EGB && SGB
// --> roundcap optimization
			}
// <-- roundcap optimization
  }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Issues GL calls to display a cylinder with colors half-bonded
//    between the ends.
//

// --> EGB && SGB
//void
//ChemUnitCylinder::renderHalfBonded(
//    const SbVec3f &from, const SbVec3f &to,
//    const float radius,
//    const SbColor &colorFrom, const SbColor &colorTo,
//    const SbBool doNormals, const SbBool doTextures) const
void ChemUnitCylinder::renderHalfBonded(
    const SbVec3f &from, const SbVec3f &to,
    const float radius,
    const SbColor &colorFrom, const SbColor &colorTo,
    const SbBool doNormals, const SbBool doTextures,
		const int bondCylinderDisplayStyle,
		const SbVec2i fromTo,
		const ChemDisplay *cd, 
		SoGLRenderAction *action
		) const
// <-- EGB && SGB
//
////////////////////////////////////////////////////////////////////////
{
// --> cap optimization
	ChemDisplayParam* cdp = ChemDisplayParamElement::get(action->getState());
	bool renderFrom, renderTo;
        //KLS VERY SLOW
	PRE_RENDER_CAP(fromTo[0],renderFrom);
	PRE_RENDER_CAP(fromTo[1],renderTo);
// <-- cap optimization
	SbMatrix theVertexMatrix;
    SbVec3f theTranslation;
    SbRotation theRotation;
    SbVec3f theScale;
    SbVec3f theCapScale;
		SbVec3f yAxis(0.0, 1.0, 0.0);
// --> EGB && SGB
		SbVec3f zAxis(0.0, 0.0, 1.0);
		SbBool bExtrudeY = false;
		bExtrudeY = (
		bondCylinderDisplayStyle !=
		ChemDisplayParam::BONDCYLINDER_GLUCYLINDER_NOCAP
		&&
		bondCylinderDisplayStyle !=
    ChemDisplayParam::BONDCYLINDER_GLUCYLINDER_FLATCAP
		&&
		bondCylinderDisplayStyle !=
    ChemDisplayParam::BONDCYLINDER_GLUCYLINDER_ROUNDCAP
		&& 
		bondCylinderDisplayStyle !=
		ChemDisplayParam::BONDCYLINDER_SEMI_NOCAP
		&&
		bondCylinderDisplayStyle !=
    ChemDisplayParam::BONDCYLINDER_SEMI_FLATCAP
		&&
		bondCylinderDisplayStyle !=
    ChemDisplayParam::BONDCYLINDER_SEMI_ROUNDCAP
   );
// <-- EGB && SGB
    SbVec3f tcoord;
    float height;

    SbBool doCaps = (capType == ChemUnitCylinder::FLATCAP);
// --> flatcap optimization
	doCaps = doCaps && (renderFrom == TRUE && renderTo == TRUE);
// <-- flatcap optimization

    if (colorFrom == colorTo)
		{
      tcoord = to - from;
      height = tcoord.length() * 0.5f;
// --> EGB && SGB
			if (bExtrudeY)
			{
// <-- EGB && SGB
				theTranslation = tcoord * 0.5f + from;
				theRotation.setValue(yAxis, tcoord);
				theScale.setValue(radius, height, radius);
// --> EGB && SGB
			}
			else
			{
				theTranslation = from;
				theRotation.setValue(zAxis, tcoord);
				theScale.setValue(radius, radius, height);
			}
// <-- EGB && SGB
// --> EGB && SGB
			CALC_SEMICYLINDER_ROTATION
// <-- EGB && SGB
			theVertexMatrix.setTransform(theTranslation, theRotation, theScale);

      glColor3fv(colorFrom.getValue());
// --> EGB && SGB
//  renderCylinder(theVertexMatrix, doCaps, doNormals, doTextures,
//      numSections, numSides, cylNormals, cylVertices);
			renderCylinder(theVertexMatrix, doCaps, doNormals, doTextures,
				bondCylinderDisplayStyle,action);
// <-- EGB && SGB

      if (capType == ChemUnitCylinder::ROUNDCAP) 
			{
// --> EGB && SGB glu round cap error
					if (!bExtrudeY)
					{
						theRotation.setValue(yAxis, to-from);
					}
// <-- EGB && SGB glu round cap error
        theCapScale.setValue(radius, radius, radius);
// --> roundcap optimization
				if (renderFrom)
				{
// <-- roundcap optimization
					theVertexMatrix.setTransform(from, theRotation, theCapScale);
	// --> EGB && SGB
	//        renderRoundCap(theVertexMatrix, TRUE,
	//            doNormals, doTextures,
	//            numRoundCapSections, numSides,
	//            cylBottomStartNorm, cylBottomStartVert,
	//            roundCapNormals, roundCapVertices);
					renderRoundCap(theVertexMatrix, TRUE,
							doNormals, doTextures,
							numRoundCapSections, numSides,
							cylBottomStartNorm, cylBottomStartVert,
							roundCapNormals, roundCapVertices,
							bondCylinderDisplayStyle);
	// <-- EGB && SGB
// --> roundcap optimization
				}
				if (renderTo)
				{
// <-- roundcap optimization
					theVertexMatrix.setTransform(to, theRotation, theCapScale);
	// --> EGB && SGB
	//        renderRoundCap(theVertexMatrix, FALSE,
	//            doNormals, doTextures,
	//            numRoundCapSections, numSides,
	//            cylBottomStartNorm, cylBottomStartVert,
	//            roundCapNormals, roundCapVertices);
					renderRoundCap(theVertexMatrix, FALSE,
							doNormals, doTextures,
							numRoundCapSections, numSides,
							cylBottomStartNorm, cylBottomStartVert,
							roundCapNormals, roundCapVertices,
							bondCylinderDisplayStyle);
	// <-- EGB && SGB
// --> roundcap optimization
				}
// <-- roundcap optimization
			}
    }
    else 
		{
      tcoord = (to - from) * 0.5f + from;
      height = (tcoord - from).length() * 0.5f;
// --> EGB && SGB
			if (bExtrudeY)
			{
// <-- EGB && SGB
        theTranslation = (tcoord - from) * 0.5f + from;
        theRotation.setValue(yAxis, (to - from));
        theScale.setValue(radius, height, radius);
// --> EGB && SGB
			}
			else
			{
				theTranslation = from;
				theRotation.setValue(zAxis, (to - from));
				theScale.setValue(radius, radius, height);
			}
// <-- EGB && SGB

// --> EGB && SGB
			CALC_SEMICYLINDER_ROTATION
// <-- EGB && SGB

      theVertexMatrix.setTransform(theTranslation, theRotation, theScale);
      glColor3fv(colorFrom.getValue());
// --> EGB && SGB
//  renderCylinder(theVertexMatrix, doCaps, doNormals, doTextures,
//      numSections, numSides, cylNormals, cylVertices);
			renderCylinder(theVertexMatrix, doCaps, doNormals, doTextures,
				bondCylinderDisplayStyle,action);
// <-- EGB && SGB
// --> EGB && SGB
//			theTranslation = (to - tcoord) * 0.5 + tcoord;
			if (bExtrudeY)
        theTranslation = (to - tcoord) * 0.5f + tcoord;
			else
				theTranslation = tcoord;
// <-- EGB && SGB
      theVertexMatrix.setTransform(theTranslation, theRotation, theScale);
      glColor3fv(colorTo.getValue());
// --> EGB && SGB
//  renderCylinder(theVertexMatrix, doCaps, doNormals, doTextures,
//      numSections, numSides, cylNormals, cylVertices);
			renderCylinder(theVertexMatrix, doCaps, doNormals, doTextures,
				bondCylinderDisplayStyle,action);
// <-- EGB && SGB

      if (capType == ChemUnitCylinder::ROUNDCAP)
			{
// --> EGB && SGB glu round cap error
					if (!bExtrudeY)
					{
						theRotation.setValue(yAxis, to-from);
					}
// <-- EGB && SGB glu round cap error
          theCapScale.setValue(radius, radius, radius);
// --> roundcap optimization
					if (renderFrom)
					{
// <-- roundcap optimization
						theVertexMatrix.setTransform(from, theRotation, theCapScale);
						glColor3fv(colorFrom.getValue());
	// --> EGB && SGB
	//          renderRoundCap(theVertexMatrix, TRUE,
	//              doNormals, doTextures,
	//              numRoundCapSections, numSides,
	//              cylBottomStartNorm, cylBottomStartVert,
	//              roundCapNormals, roundCapVertices);
						renderRoundCap(theVertexMatrix, TRUE,
								doNormals, doTextures,
								numRoundCapSections, numSides,
								cylBottomStartNorm, cylBottomStartVert,
								roundCapNormals, roundCapVertices,
								bondCylinderDisplayStyle);
	// <-- EGB && SGB
// --> roundcap optimization
					}
					if (renderTo)
					{
// <-- roundcap optimization
						theVertexMatrix.setTransform(to, theRotation, theCapScale);
						glColor3fv(colorTo.getValue());
	// --> EGB && SGB
	//          renderRoundCap(theVertexMatrix, FALSE,
	//              doNormals, doTextures,
	//              numRoundCapSections, numSides,
	//              cylBottomStartNorm, cylBottomStartVert,
	//              roundCapNormals, roundCapVertices);
						renderRoundCap(theVertexMatrix, FALSE,
								doNormals, doTextures,
								numRoundCapSections, numSides,
								cylBottomStartNorm, cylBottomStartVert,
								roundCapNormals, roundCapVertices,
								bondCylinderDisplayStyle);
// <-- EGB && SGB
// --> roundcap optimization
					}
// <-- roundcap optimization
      }
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Issues GL calls to display a cylinder with colors interpolated
//    between the ends.
//
// --> EGB && SGB
//void
//ChemUnitCylinder::renderWithInterpolatedColor(
//    const SbVec3f &from, const SbVec3f &to,
//    const float radius,
//    const SbColor &colorFrom, const SbColor &colorTo,
//    const SbBool doNormals, const SbBool doTextures) const
void ChemUnitCylinder::renderWithInterpolatedColor(
    const SbVec3f &from, const SbVec3f &to,
    const float radius,
    const SbColor &colorFrom, const SbColor &colorTo,
    const SbBool doNormals, const SbBool doTextures,
		int bondCylinderDisplayStyle,
		const SbVec2i fromTo,
		const ChemDisplay *cd,
		SoGLRenderAction* action) const
// <-- EGB && SGB
//
////////////////////////////////////////////////////////////////////////
{
// --> cap optimization
	ChemDisplayParam* cdp = ChemDisplayParamElement::get(action->getState());
	bool renderFrom, renderTo;
	PRE_RENDER_CAP(fromTo[0],renderFrom);
	PRE_RENDER_CAP(fromTo[1],renderTo);
// <-- cap optimization
    if (colorFrom == colorTo) {
        glColor3fv(colorFrom.getValue());
// --> roundcap optimization
				//render(from, to, radius, doNormals, doTextures);
        render(from, to, radius, doNormals, doTextures,bondCylinderDisplayStyle, fromTo, cd,action);
// <-- roundcap optimization
        return;
    }

    int     normCount, vertCount;
    int     section, side;
    float   alpha;
    float   s, t;
    float   ds, dt;
    float   tTop, tBot;
    float   tmpFloat;
    SbVec3f vec;
    SbVec3f tmpVec;

    const float *lColor1;
    const float *lColor2;

	SbMatrix theVertexMatrix;
    SbVec3f theTranslation;
    SbRotation theRotation;
    SbVec3f theScale;
    SbVec3f theCapScale;
    SbVec3f yAxis(0.0, 1.0, 0.0);

    SbVec3f tcoord = to - from;
    float height = tcoord.length() * 0.5f;
    theTranslation = tcoord * 0.5f + from;
    theRotation.setValue(yAxis, tcoord);
    theScale.setValue(radius, height, radius);
    theVertexMatrix.setTransform(theTranslation, theRotation, theScale);

    SbMatrix theNormalMatrix = theVertexMatrix.inverse().transpose();
    SbBool doCaps = (capType == ChemUnitCylinder::FLATCAP);
// --> flatcap optimization
	doCaps = doCaps && (renderFrom == TRUE && renderTo == TRUE);
// <-- flatcap optimization

    alpha = 1.0 / (float)numSections;
    // Don't do normals or textures
    if (!doNormals && !doTextures) {
        // Do the cylinder sides
        normCount = 0;
        vertCount = 0;
        for (section = 0; section < numSections; section++) {
            tmpFloat = (numSections - section - 1)*alpha;
            lColor1 = ((1-tmpFloat)*colorFrom + (tmpFloat*colorTo)).getValue();
            tmpFloat += alpha;
            lColor2 = ((1-tmpFloat)*colorFrom + (tmpFloat*colorTo)).getValue();
            glBegin(GL_TRIANGLE_STRIP);
            for (side = 0; side < numSides; side++) {
                glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, lColor1);
                DISP_VERTEX;
                glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, lColor2);
                DISP_VERTEX;
            }
            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, lColor1);
            DISP_VERTEX;
            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, lColor2);
            DISP_VERTEX;
            glEnd();
        }

        if (doCaps) {
            lColor1 = colorFrom.getValue();
            lColor2 = colorTo.getValue();
            // TOP
            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, lColor2);
            for (section = numSections - 1; section > 0; --section) {
                glBegin(GL_TRIANGLE_STRIP);
                for (side = 0; side < numSides; side++) {
                    DISP_VERTEX;
                    DISP_VERTEX;
                }
                DISP_VERTEX;
                DISP_VERTEX;
                glEnd();
            }
            glBegin(GL_TRIANGLE_FAN);
            DISP_VERTEX;
            for (side = numSides - 1; side >= 0; side--) {
                DISP_VERTEX;
            }
            DISP_VERTEX;
            glEnd();

            
            // BOTTOM
            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, lColor1);
            for (section = numSections - 1; section > 0; --section) {
                glBegin(GL_TRIANGLE_STRIP);
                for (side = numSides - 1; side >= 0; side--) {
                    DISP_VERTEX;
                    DISP_VERTEX;
                }
                DISP_VERTEX;
                DISP_VERTEX;
                glEnd();
            }
            glBegin(GL_TRIANGLE_FAN);
            DISP_VERTEX;
            for (side = 0; side < numSides; side++) {
                DISP_VERTEX;
            }
            DISP_VERTEX;
            glEnd();
        }
    }
    else if (doNormals && !doTextures) {
        // Do the cylinder sides
        normCount = 0;
        vertCount = 0;
        for (section = 0; section < numSections; section++) {
            tmpFloat = (numSections - section - 1)*alpha;
            lColor1 = ((1-tmpFloat)*colorFrom + (tmpFloat*colorTo)).getValue();
            tmpFloat += alpha;
            lColor2 = ((1-tmpFloat)*colorFrom + (tmpFloat*colorTo)).getValue();
            glBegin(GL_TRIANGLE_STRIP);
            for (side = 0; side < numSides; side++) {
                DISP_NORMAL;
                glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, lColor1);
                DISP_VERTEX;
                glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, lColor2);
                DISP_VERTEX;
            }
            DISP_NORMAL;
            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, lColor1);
            DISP_VERTEX;
            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, lColor2);
            DISP_VERTEX;
            glEnd();
        }

        if (doCaps) {
            lColor1 = colorFrom.getValue();
            lColor2 = colorTo.getValue();
            // TOP
            DISP_NORMAL;
            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, lColor2);
            for (section = numSections - 1; section > 0; --section) {
                glBegin(GL_TRIANGLE_STRIP);
                for (side = 0; side < numSides; side++) {
                    DISP_VERTEX;
                    DISP_VERTEX;
                }
                DISP_VERTEX;
                DISP_VERTEX;
                glEnd();
            }
            glBegin(GL_TRIANGLE_FAN);
            DISP_VERTEX;
            for (side = numSides - 1; side >= 0; side--) {
                DISP_VERTEX;
            }
            DISP_VERTEX;
            glEnd();
            
            // BOTTOM
            DISP_NORMAL;
            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, lColor1);
            for (section = numSections - 1; section > 0; --section) {
                glBegin(GL_TRIANGLE_STRIP);
                for (side = numSides - 1; side >= 0; side--) {
                    DISP_VERTEX;
                    DISP_VERTEX;
                }
                DISP_VERTEX;
                DISP_VERTEX;
                glEnd();
            }
            glBegin(GL_TRIANGLE_FAN);
            DISP_VERTEX;
            for (side = 0; side < numSides; side++) {
                DISP_VERTEX;
            }
            DISP_VERTEX;
            glEnd();
        }
    }
    else if (!doNormals && doTextures) {
        // Do the cylinder sides
        normCount = 0;
        vertCount = 0;
        tTop = 1.0;
        dt = -1.0 / numSections;
        ds = -1.0 / numSides;
        for (section = 0; section < numSections; section++) {
            tmpFloat = (numSections - section - 1)*alpha;
            lColor1 = ((1-tmpFloat)*colorFrom + (tmpFloat*colorTo)).getValue();
            tmpFloat += alpha;
            lColor2 = ((1-tmpFloat)*colorFrom + (tmpFloat*colorTo)).getValue();

            s = 1.0;
            tBot = tTop + dt;
            glBegin(GL_TRIANGLE_STRIP);
            for (side = 0; side < numSides; side++) {
                glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, lColor1);
                glTexCoord2f(s, tBot);
                DISP_VERTEX;

                glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, lColor2);
                glTexCoord2f(s, tTop);
                DISP_VERTEX;

                s += ds;
            }
            s = 0.0;
            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, lColor1);
            glTexCoord2f(s, tBot);
            DISP_VERTEX;

            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, lColor2);
            glTexCoord2f(s, tTop);
            DISP_VERTEX;
            glEnd();

            tTop = tBot;
        }

        if (doCaps) {
            lColor1 = colorFrom.getValue();
            lColor2 = colorTo.getValue();
            // TOP
            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, lColor2);
            for (section = numSections - 1; section > 0; --section) {
                glBegin(GL_TRIANGLE_STRIP);
                for (side = 0; side < numSides; side++) {
                    DISP_FLATCAP_TOP_TEXTURE;
                    DISP_FLATCAP_TOP_TEXTURE;
                }
                DISP_FLATCAP_TOP_TEXTURE;
                DISP_FLATCAP_TOP_TEXTURE;
                glEnd();
            }
            glBegin(GL_TRIANGLE_FAN);
            glTexCoord2f(0.5, 0.5);
            DISP_VERTEX;
            for (side = numSides - 1; side >= 0; side--) {
                DISP_FLATCAP_TOP_TEXTURE;
            }
            DISP_FLATCAP_TOP_TEXTURE;
            glEnd();

            
            // BOTTOM
            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, lColor1);
            for (section = numSections - 1; section > 0; --section) {
                glBegin(GL_TRIANGLE_STRIP);
                for (side = numSides - 1; side >= 0; side--) {
                    DISP_FLATCAP_BOT_TEXTURE;
                    DISP_FLATCAP_BOT_TEXTURE;
                }
                DISP_FLATCAP_BOT_TEXTURE;
                DISP_FLATCAP_BOT_TEXTURE;
                glEnd();
            }
            glBegin(GL_TRIANGLE_FAN);
            glTexCoord2f(0.5, 0.5);
            DISP_VERTEX;
            for (side = 0; side < numSides; side++) {
                DISP_FLATCAP_BOT_TEXTURE;
            }
            DISP_FLATCAP_BOT_TEXTURE;
            glEnd();
        }
    }
    else if (doNormals && doTextures) {
        // Do the cylinder sides
        normCount = 0;
        vertCount = 0;
        tTop = 1.0;
        dt = -1.0 / numSections;
        ds = -1.0 / numSides;
        for (section = 0; section < numSections; section++) {
            tmpFloat = (numSections - section - 1)*alpha;
            lColor1 = ((1-tmpFloat)*colorFrom + (tmpFloat*colorTo)).getValue();
            tmpFloat += alpha;
            lColor2 = ((1-tmpFloat)*colorFrom + (tmpFloat*colorTo)).getValue();

            s = 1.0;
            tBot = tTop + dt;
            glBegin(GL_TRIANGLE_STRIP);
            for (side = 0; side < numSides; side++) {
                DISP_NORMAL;
                glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, lColor1);
                glTexCoord2f(s, tBot);
                DISP_VERTEX;

                glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, lColor2);
                glTexCoord2f(s, tTop);
                DISP_VERTEX;

                s += ds;
            }
            s = 0.0;
            DISP_NORMAL;
            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, lColor1);
            glTexCoord2f(s, tBot);
            DISP_VERTEX;

            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, lColor2);
            glTexCoord2f(s, tTop);
            DISP_VERTEX;
            glEnd();

            tTop = tBot;
        }

        if (doCaps) {
            lColor1 = colorFrom.getValue();
            lColor2 = colorTo.getValue();
            // TOP
            DISP_NORMAL;
            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, lColor2);
            for (section = numSections - 1; section > 0; --section) {
                glBegin(GL_TRIANGLE_STRIP);
                for (side = 0; side < numSides; side++) {
                    DISP_FLATCAP_TOP_TEXTURE;
                    DISP_FLATCAP_TOP_TEXTURE;
                }
                DISP_FLATCAP_TOP_TEXTURE;
                DISP_FLATCAP_TOP_TEXTURE;
                glEnd();
            }
            glBegin(GL_TRIANGLE_FAN);
            glTexCoord2f(0.5, 0.5);
            DISP_VERTEX;
            for (side = numSides - 1; side >= 0; side--) {
                DISP_FLATCAP_TOP_TEXTURE;
            }
            DISP_FLATCAP_TOP_TEXTURE;
            glEnd();

            
            // BOTTOM
            DISP_NORMAL;
            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, lColor1);
            for (section = numSections - 1; section > 0; --section) {
                glBegin(GL_TRIANGLE_STRIP);
                for (side = numSides - 1; side >= 0; side--) {
                    DISP_FLATCAP_BOT_TEXTURE;
                    DISP_FLATCAP_BOT_TEXTURE;
                }
                DISP_FLATCAP_BOT_TEXTURE;
                DISP_FLATCAP_BOT_TEXTURE;
                glEnd();
            }
            glBegin(GL_TRIANGLE_FAN);
            glTexCoord2f(0.5, 0.5);
            DISP_VERTEX;
            for (side = 0; side < numSides; side++) {
                DISP_FLATCAP_BOT_TEXTURE;
            }
            DISP_FLATCAP_BOT_TEXTURE;
            glEnd();
        }
    }

    if (capType == ChemUnitCylinder::ROUNDCAP) {
        theCapScale.setValue(radius, radius, radius);
// --> roundcap optimization
				if (renderFrom)
				{
// <-- roundcap optimization
					theVertexMatrix.setTransform(from, theRotation, theCapScale);
					glColor3fv(colorFrom.getValue());
	// --> EGB && SGB
	//        renderRoundCap(theVertexMatrix, TRUE,
	//            doNormals, doTextures,
	//            numRoundCapSections, numSides,
	//            cylBottomStartNorm, cylBottomStartVert,
	//            roundCapNormals, roundCapVertices);
					renderRoundCap(theVertexMatrix, TRUE,
							doNormals, doTextures,
							numRoundCapSections, numSides,
							cylBottomStartNorm, cylBottomStartVert,
							roundCapNormals, roundCapVertices,
							bondCylinderDisplayStyle);
	// <-- EGB && SGB
// --> roundcap optimization
				}
				if (renderTo)
				{
// <-- roundcap optimization
					theVertexMatrix.setTransform(to, theRotation, theCapScale);
					glColor3fv(colorTo.getValue());
	// --> EGB && SGB
	//        renderRoundCap(theVertexMatrix, FALSE,
	//            doNormals, doTextures,
	//            numRoundCapSections, numSides,
	//            cylBottomStartNorm, cylBottomStartVert,
	//            roundCapNormals, roundCapVertices);
					renderRoundCap(theVertexMatrix, FALSE,
							doNormals, doTextures,
							numRoundCapSections, numSides,
							cylBottomStartNorm, cylBottomStartVert,
							roundCapNormals, roundCapVertices,
							bondCylinderDisplayStyle);
	// <-- EGB && SGB
// --> roundcap optimization
				}
// <-- roundcap optimization
    }
}

// --> EGB && SGB
void ChemUnitCylinder::generateSemiCylVertices()
{
	float angle = 0.0f;
	float x,y;
	unsigned int cnt = 0;
	
	int size = semiCylVertCount;
	for (int i=0; i<size/2;i++)
	{
		x = sin(angle);
		y = cos(angle);
		angle += (180.0f/(size/2-1)*0.01745329252f); //in radians
		float norm = 1/sqrt(x*x+y*y); // x and y never can be zero
		
		semiCylVertices[cnt].setValue(x,y,0);
		semiCylVertices[cnt+1].setValue(x,y,2);
		semiCylNormals[cnt].setValue(x*norm,y*norm,0);
		semiCylNormals[cnt+1] = semiCylNormals[cnt];
		cnt+=2;
	}
}
// <-- EGB && SGB

#undef DISP_NORMAL
#undef DISP_VERTEX
#undef DISP_FLATCAP_TOP_TEXTURE
#undef DISP_FLATCAP_BOT_TEXTURE

#undef BOT_TEX_S
#undef BOT_TEX_T
#undef TOP_TEX_S
#undef TOP_TEX_T
// --> EGB && SGB
#undef CALC_SEMICYLINDER_ROTATION
#undef PRE_RENDER_CAP
// <-- EGB && SGB
