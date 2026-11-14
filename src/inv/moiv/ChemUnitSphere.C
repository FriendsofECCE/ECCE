/*
 * ChemUnitSphere.c++
 *
 *     ChemUnitSphere class
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
#include "inv/SbLinear.H"

#include "inv/ChemKit/ChemUnitSphere.H"

#ifdef OIV_COIN
#include <Inventor/C/glue/gl.h>
#include "inv/elements/SoCacheElement.H"
#endif

static float tridata[6][3] = {
    {1, 0, 0},
    {-1, 0, 0},
    {0, 1, 0},
    {0, -1, 0},
    {0, 0, 1},
    {0, 0, -1}
};

static int triindex[8][3] = {
    {4, 3, 0}, {4, 1, 3},
    {4, 2, 1}, {4, 0, 2},

    {5, 0, 2}, {5, 2, 1},
    {5, 1, 3}, {5, 3, 0}
};

GLuint ChemUnitSphere::sphereTexture = 0;
int ChemUnitSphere::instances = 0;
////////////////////////////////////////////////////////////////////////
//
// Description:
//    Computes sphere tesselation depth based on current complexity. 
//

static int32_t
computeDepth(float complexity)
//
////////////////////////////////////////////////////////////////////////
{
    int     depth;

    // In object space, compute tesselation depth based only on
    // complexity value. We want the following correspondences between
    // complexity and depth:  
    //      0.0 -> 1
    //      0.5 -> 5
    //      1.0 -> 30
    // So we'll use linear ramps from 0 to .5 and from .5 to 1

    if (complexity < 0.5)
        depth = 1   + (int) ((complexity) * 9.0);
    else
        depth = -20 + (int) ((complexity) * 50.0);

    return depth;
}

// --> EGB && SGB
////////////////////////////////////////////////////////////////////////
//
// Description:
//    Computes sphere tesselation depth based on current complexity. 
//

static int32_t
computeGLUDepth(float complexity)
//
////////////////////////////////////////////////////////////////////////
{
    int     depth;

    // In object space, compute tesselation depth based only on
    // complexity value. We want the following correspondences between
    // complexity and depth:  
    //      0.0 -> 4 slice, 4 stack
    //      0.5 -> 12 slices, 12 stack
    //      1.0 -> 18 slices, 18 stacks
    // So we'll use linear ramps from 0 to .5 and from .5 to 1

    if (complexity < 0.5f)
        depth =  4 + (int) (complexity*10.0f);
    else 
        depth =  6 + (int) (complexity*12.0f);
		
    return depth;
}
// <-- EGB && SGB

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Generates the vertices describing a unit sphere given a specified 
//    depth.  It allocates space for the vertices and also returns the
//    number of vertices generated in vertCount.
//

static void
generateVertices(int32_t depth, int32_t &vertCount, 
	SbVec3f *&vertices )
//
////////////////////////////////////////////////////////////////////////
{
    int     i, j, k, t;
    int     octSize;
    float   *v0, *v1, *v2;
    float   w[3][3];
    SbVec3f vec;

    // Create vertices for a unit sphere.
    // For the unit sphere, there will be 8*depth*depth + 12*depth - 2
    // vertices that make up the main triangle strip.

    if (vertices != NULL) delete [] vertices;
    octSize = 8*depth*depth + 12*depth - 2;

    vertices = new SbVec3f[octSize];

	vertCount = 0;
    for (i = 0; i < depth; i++) { // each level
        for (t = 0; t <= 3; t++) { 
            // across each orig triangle in sphere
            // set triangle verticies 
            v0 = tridata[triindex[t][0]];
            v1 = tridata[triindex[t][1]];
            v2 = tridata[triindex[t][2]];

            for (j = 0; i + j < depth; j++) { // do one piece of row
                k = depth - i - j;
                w[0][0] = (i*v0[0]     + j*v1[0] +     k*v2[0])/depth;
                w[0][1] = (i*v0[1]     + j*v1[1] +     k*v2[1])/depth;
                w[0][2] = (i*v0[2]     + j*v1[2] +     k*v2[2])/depth;
                w[1][0] = ((i+1)*v0[0] + j*v1[0] + (k-1)*v2[0])/depth;
                w[1][1] = ((i+1)*v0[1] + j*v1[1] + (k-1)*v2[1])/depth;
                w[1][2] = ((i+1)*v0[2] + j*v1[2] + (k-1)*v2[2])/depth;
                w[2][0] = (i*v0[0] + (j+1)*v1[0] + (k-1)*v2[0])/depth;
                w[2][1] = (i*v0[1] + (j+1)*v1[1] + (k-1)*v2[1])/depth;
                w[2][2] = (i*v0[2] + (j+1)*v1[2] + (k-1)*v2[2])/depth;

                if (j == 0) {  // first ttri
                    if ((t == 0))  {
                        // very first ttri of row - output 1st,2nd vert
                        vec.setValue(w[0][0], w[0][1], w[0][2]);
                        vec.normalize();
                        vertices[vertCount++] = vec;

                        vec.setValue(w[1][0], w[1][1], w[1][2]);
                        vec.normalize();
                        vertices[vertCount++] = vec;
                    }
                    else  {
                        // first ttri in a sub-row 
                        vec = vertices[vertCount-2];
                        vertices[vertCount++] = vec;
                    }
                }

                // output lower left vert of ttri - 3rd vert
                vec.setValue(w[2][0], w[2][1], w[2][2]);
                vec.normalize();
                vertices[vertCount++] = vec;

                // upside-down ttri 
                // - never one at the end of a sub-row 
                //   or at the top of the tri
                //
                if ((i < (depth - 1)) && ((i+j) < (depth - 1))) {
                    w[0][0] = ((i+1)*v0[0] + (j+1)*v1[0] + (k-2)*v2[0])/depth;
                    w[0][1] = ((i+1)*v0[1] + (j+1)*v1[1] + (k-2)*v2[1])/depth;
                    w[0][2] = ((i+1)*v0[2] + (j+1)*v1[2] + (k-2)*v2[2])/depth;

                    vec.setValue(w[0][0], w[0][1], w[0][2]);
                    vec.normalize();
                    vertices[vertCount++] = vec;
                }
            } // end for (j = 0 ...
        } // end for (t = 0 ...

        // if not on last row do a swap
        if (i < (depth - 1)) {
            vec = vertices[vertCount-2];
            vertices[vertCount++] = vec;
        }
    } // end for (i = 0 ...

    // Generate the back of the sphere.
    // Given the z+ hemisphere, mirror the vertices to z- for the other 
    // hemisphere
    j = vertCount;
    float x, y, z;
    for (i = 0; i < j; i++) {
        vertices[i].getValue(x, y, z);
        vertices[vertCount++].setValue(x, -y, -z);
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Constructor
//
// Use: public

ChemUnitSphere::ChemUnitSphere(float complexity)
//
////////////////////////////////////////////////////////////////////////
{
    init(complexity, computeDepth(complexity));
}

ChemUnitSphere::ChemUnitSphere(float complexity, int32_t theDepth)
{
    init(complexity, theDepth < 1 ? 1 : theDepth);
}

int32_t ChemUnitSphere::depthForComplexity(float complexity)
{
    return computeDepth(complexity);
}

void ChemUnitSphere::init(float complexity, int32_t theDepth)
{
    vertCount = 0;
    vertices = NULL;

    depth = theDepth;
    generateVertices(depth, vertCount, vertices);

// --> EGB && SGB
		GLUDepth = computeGLUDepth(complexity);
		qobj = gluNewQuadric();
		sphereTexCoords = NULL;
		//sphereTexture = 0;

		soSphere = new SoSphere;// default radius value 1.0

		sphereTexCoordsGenerated = false;
    instances++;
// <-- EGB && SGB
}

////////////////////////////////////////////////////////////////////////
//
// Description: 
//    Destructor
//
// Use: public  

ChemUnitSphere::~ChemUnitSphere()
// 
////////////////////////////////////////////////////////////////////////
{
    if (vertices != NULL) delete [] vertices;
// --> EGB && SGB
		if (qobj != NULL) gluDeleteQuadric(qobj);
		if (sphereTexCoords != NULL) delete [] sphereTexCoords;

    instances--;
    //printf("%d instances\n",instances);
    if (instances == 0 && sphereTexture>0)
    {
		  glDeleteTextures(1,&sphereTexture);
      sphereTexture = 0;
    }
// <-- EGB && SGB
}


// Computes S and T texture coordinates from point on surface
#define COMPUTE_S_T(point, s, t)                                              \
    s = atan2f(point[0], point[2]) * .159 + .5;                               \
    t = atan2f(point[1],                                                      \
           sqrtf(point[0]*point[0] + point[2]*point[2])) * .318 + .5

;   
// Adjusts S texture coordinate in unstable areas
#define ADJUST_S(s, vec)                                                      \
    if (s < .001 && (vec[0] < 0.0 && vec[2] > 0.0))                           \
        s = 1.0;                                                              \
    else if (s > .999 && (vec[0] < 0.0 && vec[2] < 0.0))                      \
        s = 0.0

// --> EGB && SGB
////////////////////////////////////////////////////////////////////////
//
// Description:
//    Generates the texture coordinates for a unit sphere given its vertices.
//    It also allocates space for the coords.

void ChemUnitSphere::generateSphereTexCoords(int32_t &vertCount)
//
////////////////////////////////////////////////////////////////////////
{
	assert(vertCount>0);
	
	float   s,t;
	
	if (sphereTexCoords != NULL) delete[] sphereTexCoords;
	sphereTexCoords = new SbVec2f[vertCount];

	int i=0;
	for (i = 0; i < vertCount; i++) 
	{
		COMPUTE_S_T(vertices[i].getValue(), s, t);
		ADJUST_S(s, vertices[i].getValue());
		sphereTexCoords[i].setValue(s,t);
	}

	sphereTexCoordsGenerated = true;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Generates a texture used for billboarding purposes

void ChemUnitSphere::generateBillboard(SoGLRenderAction *action)
{
	if (sphereTexture != 0)
		return;

	const int quad = 256;

#ifdef OIV_COIN
  // Texture construction must not clear the scene already in the window.
  const cc_glglue *glue = cc_glglue_instance(action->getCacheContext());
  if (!cc_glglue_has_framebuffer_objects(glue)) return;
  // Framebuffer bindings are immediate; scratch drawing must never be replayed.
  SoCacheElement::invalidate(action->getState());
  const bool separate = cc_glglue_glversion_matches_at_least(glue, 3, 0, 0);
  GLint readFbo = 0, drawFbo = 0, renderbuffer = 0;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &drawFbo);
  if (separate) glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFbo);
  else readFbo = drawFbo;
  glGetIntegerv(GL_RENDERBUFFER_BINDING, &renderbuffer);
  glPushAttrib(GL_ALL_ATTRIB_BITS);
  GLuint fbo = 0, buffers[2] = {0, 0};
  cc_glglue_glGenFramebuffers(glue, 1, &fbo);
  cc_glglue_glGenRenderbuffers(glue, 2, buffers);
  if (!fbo || !buffers[0] || !buffers[1]) {
    cc_glglue_glDeleteRenderbuffers(glue, 2, buffers);
    cc_glglue_glDeleteFramebuffers(glue, 1, &fbo);
    glPopAttrib();
    return;
  }
  cc_glglue_glBindFramebuffer(glue, GL_FRAMEBUFFER, fbo);
  cc_glglue_glBindRenderbuffer(glue, GL_RENDERBUFFER, buffers[0]);
  cc_glglue_glRenderbufferStorage(glue, GL_RENDERBUFFER, GL_RGBA8, quad, quad);
  cc_glglue_glFramebufferRenderbuffer(glue, GL_FRAMEBUFFER,
      GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, buffers[0]);
  cc_glglue_glBindRenderbuffer(glue, GL_RENDERBUFFER, buffers[1]);
  cc_glglue_glRenderbufferStorage(glue, GL_RENDERBUFFER,
      GL_DEPTH_COMPONENT24, quad, quad);
  cc_glglue_glFramebufferRenderbuffer(glue, GL_FRAMEBUFFER,
      GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, buffers[1]);
  glDrawBuffer(GL_COLOR_ATTACHMENT0);
  glReadBuffer(GL_COLOR_ATTACHMENT0);
  const auto restore = [&]() {
    if (separate) {
      cc_glglue_glBindFramebuffer(glue, GL_READ_FRAMEBUFFER, readFbo);
      cc_glglue_glBindFramebuffer(glue, GL_DRAW_FRAMEBUFFER, drawFbo);
    } else {
      cc_glglue_glBindFramebuffer(glue, GL_FRAMEBUFFER, drawFbo);
    }
    cc_glglue_glBindRenderbuffer(glue, GL_RENDERBUFFER, renderbuffer);
    cc_glglue_glDeleteRenderbuffers(glue, 2, buffers);
    cc_glglue_glDeleteFramebuffers(glue, 1, &fbo);
    glPopAttrib();
  };
  if (cc_glglue_glCheckFramebufferStatus(glue, GL_FRAMEBUFFER)
      != GL_FRAMEBUFFER_COMPLETE) {
    restore();
    return;
  }
  glDisable(GL_SCISSOR_TEST);
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glDepthMask(GL_TRUE);
#endif

	glGenTextures(1,&sphereTexture);

	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluPerspective(45,1,1,100);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	gluLookAt(0, 0, 3.1,     0, 0, 0,     0, 1, 0);
	//

	glPushAttrib(GL_VIEWPORT_BIT);

	glViewport(0, 0, quad,quad);

// --> Respect original color material state
	bool bColorMaterialActive = false;
	glGetBooleanv(GL_COLOR_MATERIAL,(GLboolean*)&bColorMaterialActive);
	if (!bColorMaterialActive)
// <-- Respect original color material state
		glEnable(GL_COLOR_MATERIAL);

// --> Respect original color
	float cur_color[4];
	glGetFloatv(GL_CURRENT_COLOR,(GLfloat*)&cur_color);
// <-- Respect original color
	glColor3f(1,1,1);
	glTranslatef(0,0,-10);
	GLUquadricObj *pObj = gluNewQuadric();
	gluQuadricNormals(pObj,GL_SMOOTH);
	gluSphere(pObj,5,64,64);
	gluDeleteQuadric(pObj);

// --> Respect original color
	glColor3f(cur_color[0],cur_color[1],cur_color[2]);
// <-- Respect original color

	glDisable(GL_COLOR_MATERIAL);
	//

	glBindTexture(GL_TEXTURE_2D,sphereTexture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, quad, quad, 0); // grab framebuffer

	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	unsigned char* pixels = NULL;
	pixels = new unsigned char[quad*quad*(32/8)];
	glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels); // pick framebuffer

	// generate alpha channel
	unsigned char *ptr = pixels;
	int i;
	for (i=0; i<quad*quad; i++, ptr+=4)
	{
		if (ptr[0] + ptr[1] + ptr[2] == 0)
			ptr[3] = 0; 
	}
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	gluBuild2DMipmaps(GL_TEXTURE_2D, 4, quad, quad, GL_RGBA,
   GL_UNSIGNED_BYTE, pixels);
	delete [] pixels;
	
// --> Respect original color material state
	if (!bColorMaterialActive)
// <-- Respect original color material state
		glDisable(GL_COLOR_MATERIAL);

	glPopAttrib();

	glPopMatrix(); // pop modelview
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
  glMatrixMode(GL_MODELVIEW);

#ifdef OIV_COIN
  restore();
#endif
}
//
// <-- EGB && SGB

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Generate the sphere display list containing either 1 triangle strip
//    if hemispheres are used or 2 triangle strips if the full sphere
//    is used.
//
// --> EGB && SGB
//void
//ChemUnitSphere::generateDisplay(SbBool doHemiSpheres, SbBool doNormals,
//    SbBool doTextures) const
void ChemUnitSphere::generateDisplay(SbBool doHemiSpheres, SbBool doNormals, 
			SbBool doTextures, 
			int atomSphereDisplayStyle,
			SoGLRenderAction* action
			)
// <-- EGB && SGB
//
////////////////////////////////////////////////////////////////////////
{
    int     i, vend;
    float   s,t;

    vend = vertCount/2;

// --> EGB && SGB
		if (atomSphereDisplayStyle == 
			ChemDisplayParam::ATOMSPHERE_HEMISPHERES ||
			atomSphereDisplayStyle == 
			ChemDisplayParam::ATOMSPHERE_FULLSPHERES)
		{
// <-- EGB && SGB
			// Don't do normals or textures
			if (!doNormals && !doTextures)
			{

						glBegin(GL_TRIANGLE_STRIP);
						for (i = 0; i < vend; i++) {
								glVertex3fv(vertices[i].getValue());
						}
						glEnd();
	        if (doHemiSpheres == FALSE) {
	            glBegin(GL_TRIANGLE_STRIP);
	            for (i = vend; i < vertCount; i++) {
	                glVertex3fv(vertices[i].getValue());
	            }
	            glEnd();
	        }
			// Do normals but not textures.  This is the most common case.
			}
			else if (doNormals && !doTextures)
			{

						glBegin(GL_TRIANGLE_STRIP);
						for (i = 0; i < vend; i++) {
								glNormal3fv(vertices[i].getValue());
								glVertex3fv(vertices[i].getValue());
						}
						glEnd();
	        if (doHemiSpheres == FALSE)
					{
	            glBegin(GL_TRIANGLE_STRIP);
	            for (i = vend; i < vertCount; i++) {
	                glNormal3fv(vertices[i].getValue());
	                glVertex3fv(vertices[i].getValue());
	            }
	            glEnd();
	        }

			// Don't do normals but do textures
			}
			else if (!doNormals && doTextures)
			{
						glBegin(GL_TRIANGLE_STRIP);
						for (i = 0; i < vend; i++) {
								COMPUTE_S_T(vertices[i].getValue(), s, t);
								ADJUST_S(s, vertices[i].getValue());
								glTexCoord2f(s, t);
								glNormal3fv(vertices[i].getValue());
								glVertex3fv(vertices[i].getValue());
						}
						glEnd();
	        if (doHemiSpheres == FALSE)
					{
	            glBegin(GL_TRIANGLE_STRIP);
	            for (i = vend; i < vertCount; i++) {
	                COMPUTE_S_T(vertices[i].getValue(), s, t);
	                ADJUST_S(s, vertices[i].getValue());
	                glTexCoord2f(s, t);
	                glNormal3fv(vertices[i].getValue());
	                glVertex3fv(vertices[i].getValue());
	            }
	            glEnd();
	        }

			// Do both normals and textures
			}
			else if (doNormals && doTextures)
			{
						glBegin(GL_TRIANGLE_STRIP);
						for (i = 0; i < vend; i++) {
								COMPUTE_S_T(vertices[i].getValue(), s, t);
								ADJUST_S(s, vertices[i].getValue());
								glTexCoord2f(s, t);
								glNormal3fv(vertices[i].getValue());
								glVertex3fv(vertices[i].getValue());
						}
						glEnd();
	        if (doHemiSpheres == FALSE)
					{
	            glBegin(GL_TRIANGLE_STRIP);
	            for (i = vend; i < vertCount; i++) {
	                COMPUTE_S_T(vertices[i].getValue(), s, t);
	                ADJUST_S(s, vertices[i].getValue());
	                glTexCoord2f(s, t);
	                glNormal3fv(vertices[i].getValue());
	                glVertex3fv(vertices[i].getValue());
	            }
	            glEnd();
	        }
			}
// --> EGB && SGB
		}
		else
		{
			// Don't do normals or textures
			if (!doNormals && !doTextures)
			{
					if (doHemiSpheres == FALSE)
					{
						switch (atomSphereDisplayStyle)
						{
							case ChemDisplayParam::ATOMSPHERE_ARRAY_FULLSPHERES:
							{
								glDrawArrays(GL_TRIANGLE_STRIP,0,vertCount);							
								break;
							}

							case ChemDisplayParam::ATOMSPHERE_GLUSPHERE:
							{
								gluSphere(qobj,1.0,GLUDepth,GLUDepth);
								break;
							}
							
							case ChemDisplayParam::ATOMSPHERE_SOSPHERE:
							{
								if (action != NULL)	soSphere->GLRender(action);
								break;
							}
							
						}
					} 
					else // if (doHemiSpheres == TRUE)
					{
						switch (atomSphereDisplayStyle)
						{
							case ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES:
							{
								glDrawArrays(GL_TRIANGLE_STRIP,0,vend);
								break;
							}
						}
					}
			// Do normals but not textures.  This is the most common case.
			}
			else if (doNormals && !doTextures)
			{
				if (doHemiSpheres == FALSE)
				{
						switch (atomSphereDisplayStyle)
						{
							case ChemDisplayParam::ATOMSPHERE_ARRAY_FULLSPHERES:
							{
								glDrawArrays(GL_TRIANGLE_STRIP,0,vertCount);						
								break;
							}

							case ChemDisplayParam::ATOMSPHERE_GLUSPHERE:
							{
								gluSphere(qobj,1.0,GLUDepth,GLUDepth);
								break;
							}
		
							case ChemDisplayParam::ATOMSPHERE_SOSPHERE:
							{
								if (action != NULL) soSphere->GLRender(action);
								break;
							}

							case ChemDisplayParam::ATOMSPHERE_BILLBOARD:
							{
								glBegin(GL_QUADS);
									glTexCoord2f(0,0);	glNormal3fv(normala.getValue()); glVertex3fv(a.getValue());
									glTexCoord2f(1,0);  glNormal3fv(normalb.getValue()); glVertex3fv(b.getValue());
									glTexCoord2f(1,1);	glNormal3fv(normalc.getValue()); glVertex3fv(c.getValue());
									glTexCoord2f(0,1);	glNormal3fv(normald.getValue()); glVertex3fv(d.getValue());
								glEnd();

								break;
							}
						}
					} 
					else // if (doHemiSpheres == TRUE)
					{
						switch (atomSphereDisplayStyle)
						{
							case ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES:
							{
								glDrawArrays(GL_TRIANGLE_STRIP,0,vend);
								break;
							}
						}
					}

			// Don't do normals but do textures
			}
			else if (!doNormals && doTextures)
			{
				if (doHemiSpheres == FALSE)
					{
						switch (atomSphereDisplayStyle)
						{
							case ChemDisplayParam::ATOMSPHERE_GLUSPHERE:
							{
								gluSphere(qobj,1.0,GLUDepth,GLUDepth);	
								break;
							}
							case ChemDisplayParam::ATOMSPHERE_ARRAY_FULLSPHERES:
							{
								glDrawArrays(GL_TRIANGLE_STRIP,0,vertCount);					
								break;
							}
							case ChemDisplayParam::ATOMSPHERE_SOSPHERE:
							{
								if (action!=NULL) soSphere->GLRender(action);
								break;
							}
						}
					} 
					else // if (doHemiSpheres == TRUE)
					{
						switch (atomSphereDisplayStyle)
						{
							case ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES:
							{
								glDrawArrays(GL_TRIANGLE_STRIP,0,vend);
								break;
							}
						}
					}

			// Do both normals and textures
			}
			else if (doNormals && doTextures)
			{
				if (doHemiSpheres == FALSE)
					{
						switch (atomSphereDisplayStyle)
						{
							case ChemDisplayParam::ATOMSPHERE_GLUSPHERE:
							{
								gluSphere(qobj,1.0,GLUDepth,GLUDepth);	
								break;
							}
							case ChemDisplayParam::ATOMSPHERE_ARRAY_FULLSPHERES:
							{
								glDrawArrays(GL_TRIANGLE_STRIP,0,vertCount);						
								break;
							}
							case ChemDisplayParam::ATOMSPHERE_SOSPHERE:
							{
								if (action!=NULL) soSphere->GLRender(action);
								break;
							}
							
						}
					} 
					else // if (doHemiSpheres == TRUE)
					{
						switch (atomSphereDisplayStyle)
						{
							case ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES:
							{
								glDrawArrays(GL_TRIANGLE_STRIP,0,vend);
								break;
							}
						}
					}
			} // if (doNormals && doTextures)
		}
// <-- EGB && SGB
}

#undef COMPUTE_S_T
#undef ADJUST_S
