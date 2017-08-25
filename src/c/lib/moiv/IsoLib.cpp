/*
 * IsoLib.c++
 *
 *     Isosurfacing functions
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

#ident "$Revision: 22364 $"

//KLS include stdlib for g++ 3.2.3
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "inv/fields/SoMFShort.H"
#include "inv/fields/SoMFBool.H"
#include "inv/nodes/SoIndexedTriangleStripSet.H"

//  Fix compilation with Coin3D.	Daniel Soto	2002-02-05
#ifndef SO_END_STRIP_INDEX
#define SO_END_STRIP_INDEX	(-1)
#endif

//	Jamie Doornbos	2001/07/15
#include "inv/nodes/SoNormalBinding.H"
#include "inv/nodes/SoMaterialBinding.H"
#include "inv/SbColor.H"

#include "inv/ChemKit/ChemLattice3.H"
#include "inv/ChemKit/MFDouble.H"
#include "inv/ChemKit/MFInt8.H"

static void add(int v);
static int cube(short *edge, int v0, int v1);
static int link(int face);
static void getSheet(float *v, int z);
static int addEdge(float t, float v0, float v1, int i0, int j0, int k0,
                   int i1, int j1, int k1);
static void addVertex(int v);
static float getValue(int x, int y, int z);
static void gradient(int x, int y, int z, float g[3]);

static void lookupFromColorData(ChemLattice3 *colorLat);

static int interpColorData(SbVec3f index, float &dst);
static int interpColorMap(float index, uint32_t &dst);

#define EDGE_INCR 1024
#define VERT_INCR 1024

static int32_t gEdgeCount;
static int32_t gVertCount;
static int32_t gNpoints;
static int32_t gNmeshes;
static int32_t gNverts;
static SbBool  gGenerateNormals;
static SbBool  gFlipNormals;
static short gColorVariable;    // used in linear.inc
static short gSurfaceVariable;

static SbVec3f *gVertPtr;
static SbVec3f *gNormPtr;
static uint32_t *gColorPtr;
static int32_t *gCoordIndexPtr;

static ChemLattice3 *gDataLat;
static short gNumDataVar;
static LatticeDataType gDataType;
static SbVec3i gDataDims;
static const void *gDataValues;

static ChemLattice3 *gColorLat;
static short gNumColorVar;
static LatticeDataType gColorDataType;
static SbVec3i gColorDims;
static const void *gColorValues;

static int32_t gColorDataOffset[8];

static int32_t gNumColors;
static const uint32_t *gColorMapValues;
static float gColorMapMin;
static float gColorMapMax;

static SoIndexedTriangleStripSet *gSS;
static SoVertexProperty *gVP;

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Initialize variables for creating the isosurface
//

void
isoLibInitVolume(
    SbBool generateNormals, 
    SbBool flipNormals, 
    short dataVar,
    short colorVar,
    ChemLattice3 *dataLat,
    ChemLattice3 *colorLat,
    int32_t numColors,
    float minColor,
    float maxColor,
    const uint32_t *colorMap,
    SoIndexedTriangleStripSet *ss, 
    SoVertexProperty *vp)
//
////////////////////////////////////////////////////////////////////////
{
    gEdgeCount = gVertCount = 4;
    gNpoints = 1;
    gNmeshes = 0;
    gNverts = 0;
    gGenerateNormals = generateNormals;
    gFlipNormals = flipNormals;
    gSurfaceVariable = dataVar;
    gColorVariable = colorVar;

    gVertPtr = gNormPtr = NULL;
    gColorPtr = NULL;
    gCoordIndexPtr = NULL;

    gDataLat = dataLat;
    gNumDataVar = dataLat->nDataVar.getValue();
    gDataType = dataLat->getDataType();
    gDataDims = dataLat->dimension.getValue();
    switch (gDataType) {
// --> sas
      case LATTICE_DATA_BOOL:
        gDataValues = ((SoMFBool *)(dataLat->data))->getValues(0);
        break;
// <-- sas

      case LATTICE_DATA_INT8:
        gDataValues = ((MFInt8 *)(dataLat->data))->getValues(0);
        break;

      case LATTICE_DATA_INT16:
        gDataValues = ((SoMFShort *)(dataLat->data))->getValues(0);
        break;

      case LATTICE_DATA_INT32:
        gDataValues = ((SoMFInt32 *)(dataLat->data))->getValues(0);
        break;

      case LATTICE_DATA_FLOAT32:
        gDataValues = ((SoMFFloat *)(dataLat->data))->getValues(0);
        break;

      case LATTICE_DATA_FLOAT64:
        gDataValues = ((MFDouble *)(dataLat->data))->getValues(0);
        break;

      case LATTICE_DATA_BAD_TYPE:
        break;
    }

    gSS = ss;
    gVP = vp;

    if (generateNormals) {
        vp->normalBinding = SoNormalBinding::PER_VERTEX_INDEXED;
        vp->normal.setNum(gEdgeCount*EDGE_INCR);
        gNormPtr = vp->normal.startEditing();
    }
    else {
        vp->normalBinding = SoNormalBinding::OVERALL;
    }
    ss->coordIndex.setNum(gVertCount*VERT_INCR);
    gCoordIndexPtr = ss->coordIndex.startEditing();


// these code doesn't work with coin
// -> sas
#ifndef __coin
    // Set the first value to be the end of a strip.  Increment gNmeshes.
    gCoordIndexPtr[0] = SO_END_STRIP_INDEX;
     gNmeshes++;
#endif
// <-- sas
		

    // Start editing of the vertices.  Set the first value to the origin
    // here.  When the isosurfacing is finished, it will be set to
    // gVertPtr[1].
    vp->vertex.setNum(gEdgeCount*EDGE_INCR);
    gVertPtr = vp->vertex.startEditing();
    gVertPtr[0] = SbVec3f(0, 0, 0);

    if (colorLat) {
        gColorLat = colorLat;
        gNumColorVar = colorLat->nDataVar.getValue();
        gColorDataType = colorLat->getDataType();
        gColorDims = colorLat->dimension.getValue();
        switch (gColorDataType) {
          case LATTICE_DATA_INT8:
            gColorValues = ((MFInt8 *)(colorLat->data))->getValues(0);
            break;
    
          case LATTICE_DATA_INT16:
            gColorValues = ((SoMFShort *)(colorLat->data))->getValues(0);
            break;
    
          case LATTICE_DATA_INT32:
            gColorValues = ((SoMFInt32 *)(colorLat->data))->getValues(0);
            break;
    
          case LATTICE_DATA_FLOAT32:
            gColorValues = ((SoMFFloat *)(colorLat->data))->getValues(0);
            break;
    
          case LATTICE_DATA_FLOAT64:
            gColorValues = ((MFDouble *)(colorLat->data))->getValues(0);
            break;

          case LATTICE_DATA_BOOL:
          case LATTICE_DATA_BAD_TYPE:
            break;
        }

        vp->materialBinding = SoMaterialBinding::PER_VERTEX_INDEXED;
        vp->orderedRGBA.setNum(gEdgeCount*EDGE_INCR);
        gColorPtr = vp->orderedRGBA.startEditing();
        lookupFromColorData(colorLat);
        if (colorMap) {
            gNumColors = numColors;
            gColorMapMin = minColor;
            gColorMapMax = maxColor;
            gColorMapValues = colorMap;
        }
        else {
            gNumColors = 0;
            gColorMapValues = NULL;
        }
    }
    else {
        vp->materialBinding = SoMaterialBinding::OVERALL;
        gColorValues = NULL;
        gNumColors = 0;
        gColorMapValues = NULL;
    }
}

static int bitc[16] = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4 };
static int ilog2[64] = {
        0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5
};

// These macros work for up to 12-bit numbers
#define bitc12(X) \
        (bitc[(X) & 0xf] + bitc[((X) >> 4) & 0xf] + bitc[(X) >> 8])
#define ilog2b(X)       ((X) >> 6 ? 6 + ilog2[(X) >> 6] : ilog2[X])

// Order of intersections is 01 23 02 13 04 15 26 37 45 67 46 57
// Order of faces is xy xz yz
#define V(X)    (1 << (X))
    
static short f2v[6] = { // Face to vertices
        V(0) | V(1) | V(2) | V(3),      V(8) | V(9) | V(10) | V(11),
        V(0) | V(4) | V(5) | V(8),      V(1) | V(6) | V(7) | V(9),
        V(2) | V(4) | V(6) | V(10),     V(3) | V(5) | V(7) | V(11)
};
    
static int v2f[12] = {  // Vertex to faces
    5, 9, 17, 33, 20, 36, 24, 40, 6, 10, 18, 34
};
    
static short v2v[12] = {    // Shared face vertices
    0x13e, 0x2cd, 0x45b, 0x8a7,
    0x565, 0x999, 0x696, 0xa6a,
    0xe31, 0xdc2, 0xb54, 0x7a8
};
    
static short v2n[12] = {    /* neighbor vertices */
    V(4) | V(5) | V(2) | V(3),  V(2) | V(3) | V(6) | V(7),
    V(0) | V(1) | V(4) | V(6),  V(0) | V(1) | V(5) | V(7),
    V(0) | V(2) | V(8) | V(10), V(0) | V(3) | V(8) | V(11),
    V(1) | V(2) | V(9) | V(10), V(1) | V(3) | V(9) | V(11),
    V(4) | V(5) | V(10) | V(11),    V(6) | V(7) | V(10) | V(11),
    V(4) | V(6) | V(8) | V(9),  V(5) | V(7) | V(8) | V(9)
};
#undef V

typedef struct Edge {
    int x,y,z;
} Edge;
        
static int ii,jj;
static Edge *e0,*e1,*e2;            /* edge pointers */
static int xverts,yverts,zverts;    /* sheet size in vertices */
static int xcells,ycells,zcells;    /* sheet size in cells */
static short *c0;
static int t0,t1;

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Generate the isosurface
//

void
isoLibIso(float t)
//
////////////////////////////////////////////////////////////////////////
{
	int	i,j,k;			/* loop variables */

	short	*cell;

	float	*sheet0,*sheet1;	/* value buffers */
	float	*s0,*s1,*s2,*s3;	/* value pointers */

	Edge	*edge0,*edge1;		/* edge buffers */

	// Get sizes
	xverts = gDataDims[0];
	yverts = gDataDims[1];
	zverts = gDataDims[2];
	xcells = xverts - 1;
	ycells = yverts - 1;
	zcells = zverts - 1;

	// Create sheet buffers
	sheet0 = (float *)malloc(xverts*yverts*sizeof(float));
	sheet1 = (float *)malloc(xverts*yverts*sizeof(float));

	// Create edge buffers
	edge0 = (Edge *)malloc(xverts*yverts*sizeof(Edge));
	edge1 = (Edge *)malloc(xverts*yverts*sizeof(Edge));

	// Create the cell edge flag array
	cell = (short *)malloc(xcells*ycells*sizeof(short));

	// Zero out the edge buffer
	/*
		Porting to Windows:
		Jamie Doornbos	2001/07/15
	*/
	size_t byteCount = xverts*yverts*sizeof(Edge);
	#ifdef WIN32
		memset(edge1, 0, byteCount);
	#else
		bzero(edge1, byteCount);
	#endif

	// Fill the bottom sheet
	getSheet(sheet1, 0);
	
	// Find the edges for the first row
	s0 = sheet1;
	s1 = sheet1 + 1;
	e0 = edge1;
	for ( i = 0; i < xcells; i++ ) {
		// Test for different values
		if ( (*s0 < t) ^ (*s1 < t) )
			e0->x = addEdge(t, *s0, *s1, i, 0, 0, i+1, 0, 0);
		
		s0++;	s1++;
		e0++;
	}

	// Do the remainder of the rows
	for ( i = 0; i < ycells; i++ ) {
		// Set up pointers
		s0 = sheet1 + i*xverts;
		s1 = s0 + xverts;
		s2 = s1 + 1;
		e0 = edge1 + i*xverts;
		e1 = e0 + xverts;

		/* check the first edge */
		if ( (*s0 < t) ^ (*s1 < t) )
			e0->y = addEdge(t,*s0,*s1,0,i,0,0,i+1,0);
		
		s0++;
		e0++;

		/* scan the row */
		for ( j = 0; j < xcells; j++ ) {
			/* check x direction */
			if ( (*s1 < t) ^ (*s2 < t) )
				e1->x = addEdge(t,*s1,*s2,j,i+1,0,j+1,i+1,0);
			
			/* check y direction */
			if ( (*s0 < t) ^ (*s2 < t) )
				e0->y = addEdge(t,*s0,*s2,j+1,i,0,j+1,i+1,0);
				
			s0++;	s1++;	s2++;
			e0++;	e1++;
		}
	}

	/* go through the sheets */
	for ( k = 0; k < zcells; k++ ) {
		/* swap the buffers */
		s0 = sheet0; sheet0 = sheet1; sheet1 = s0;
		e0 = edge0; edge0 = edge1; edge1 = e0;
		
		/* zero out the edge buffer */
		/*
			Porting to Windows:
			Jamie Doornbos	2001/07/15
		*/
		size_t byteCount = xverts*yverts*sizeof(Edge);
		#ifdef WIN32
			memset(edge1, 0, byteCount);
		#else
			bzero(edge1, byteCount);
		#endif

		/* get the next sheet */
		getSheet(sheet1,k + 1);

		/* find the edges on this sheet */
		s0 = sheet0;
		s1 = sheet1;
		s2 = s1;
		e0 = edge0;
		e1 = edge1;

		/* do origin between sheets */
		if ( (*s0 < t) ^ (*s2 < t) )
			e0->z = addEdge(t,*s0,*s2,0,0,k,0,0,k+1);
		
		s0++;	s2++;
		e0++;

		/* scan through row */
		for ( i = 0; i < xcells; i++ ) {
			if ( (*s0 < t) ^ (*s2 < t) )
				e0->z = addEdge(t,*s0,*s2,i+1,0,k,i+1,0,k+1);
			
			if ( (*s1 < t) ^ (*s2 < t) )
				e1->x = addEdge(t,*s1,*s2,i,0,k+1,i+1,0,k+1);

			s0++;	s1++;	s2++;
			e0++;	e1++;
		}

		c0 = cell;
		for ( i = 0; i < ycells; i++ ) {
			s0 = sheet0 + (i + 1)*xverts;
			s1 = sheet1 + i*xverts;
			s2 = s1 + xverts;
			s3 = s2;
			e0 = edge0 + (i + 1)*xverts;
			e1 = edge1 + i*xverts;
			e2 = e1 + xverts;

			if ( (*s0 < t) ^ (*s3 < t) )
				e0->z = addEdge(t,*s0,*s3,0,i+1,k,0,i+1,k+1);
			
			if ( (*s1 < t) ^ (*s3 < t) )
				e1->y = addEdge(t,*s1,*s3,0,i,k+1,0,i+1,k+1);
			
			s0++;	s1++;	s3++;
			e0++;	e1++;

			/* scan through the row */
			for ( j = 0; j < xcells; j++ ) {
				/* clear cell flag */
				*c0 = 0;

				if ( (*s0 < t) ^ (*s3 < t) ) {
					e0->z = addEdge(t,*s0,*s3,
						j+1,i+1,k,j+1,i+1,k+1);
					*c0 |= 1 << 7;
				}
				
				if ( (*s1 < t) ^ (*s3 < t) ) {
					e1->y = addEdge(t,*s1,*s3,
						j+1,i,k+1,j+1,i+1,k+1);
					*c0 |= 1 << 11;
				}
				
				if ( (*s2 < t) ^ (*s3 < t) ) {
					e2->x = addEdge(t,*s2,*s3,
						j,i+1,k+1,j+1,i+1,k+1);
					*c0 |= 1 << 9;
				}
				
				if ( (e0 - xverts - 1)->x ) *c0 |= 1 << 0;
				if ( (e0 - xverts - 1)->y ) *c0 |= 1 << 2;
				if ( (e0 - xverts - 1)->z ) *c0 |= 1 << 4;

				if ( (e0 - xverts)->y ) *c0 |= 1 << 3;
				if ( (e0 - xverts)->z ) *c0 |= 1 << 5;

				if ( (e0 - 1)->x ) *c0 |= 1 << 1;
				if ( (e0 - 1)->z ) *c0 |= 1 << 6;

				if ( (e1 - 1)->x ) *c0 |= 1 << 8;
				if ( (e1 - 1)->y ) *c0 |= 1 << 10;


#ifdef HOLES

				/* check for potential holes */
				if ( (*c0 & f2v[1]) == f2v[1] ) {
					if ( *s3 < t ) {
						addVertex(e1->y);
						addVertex((e1 - 1)->x);
						addVertex(e2->x);
						addVertex((e1 - 1)->y);
					}
					else {
						addVertex((e1 - 1)->x);
						addVertex(e1->y);
						addVertex((e1 - 1)->y);
						addVertex(e2->x);
					}
					addVertex(-1);
				}
				if ( (*c0 & f2v[3]) == f2v[3] ) {
					if ( *s3 < t ) {
						addVertex(e0->z);
						addVertex((e0 - xverts)->y);
						addVertex(e1->y);
						addVertex((e0 - xverts)->z);
					}
					else {
						addVertex((e0 - xverts)->y);
						addVertex(e0->z);
						addVertex((e0 - xverts)->z);
						addVertex(e1->y);
					}
					addVertex(-1);
				}
				if ( (*c0 & f2v[5]) == f2v[5] )
					;
#endif

				s0++;	s1++;	s2++;	s3++;
				e0++;	e1++;	e2++;
				c0++;
			}
		}

		/* now scan through the cells */
		for ( i = 0; i < ycells; i++ ) {
			s0 = sheet0 + i*xverts;
			s1 = sheet1 + i*xverts;

			for ( j = 0; j < xcells; j++ ) {
				ii = i;
				jj = j;

				while ( *(c0 = cell + i*xcells + j) ) {
					int	flag;
					int	v0,v1;

/*					printf("initiating %d %d 0x%x\n",jj,ii,*c0); */
					e0 = edge0 + i*xverts + j;
					e1 = edge1 + i*xverts + j;

					if ( 0 )
						;
#define IF(F) \
	else if ( (flag = *c0 & f2v[F]) && bitc12(flag) == 2 ) {\
		t0 = ilog2b(flag); \
		flag &= ~(1 << t0); \
		t1 = ilog2b(flag); \
	}

					IF(2)
					IF(4)
					IF(3)
					IF(5)
					IF(0)
					IF(1)
#undef IF
					else
						break;

					/* orient edge correctly */
					switch(t1)
					{
#define CASE(C,F) case C: flag = (*(F) < t); if ( gFlipNormals ) flag = !flag; if ( 0 ) ;
#define S(V0) \
	else if ( t0 == V0 ) { \
		if ( flag ) { v0 = t1; v1 = t0; } \
		else { v0 = t0; v1 = t1; } \
	}
#define N(V0) \
	else if ( t0 == V0 ) { \
		if ( flag ) { v0 = t0; v1 = t1; } \
		else { v0 = t1; v1 = t0; } \
	}
						CASE(0,s0)
							N(1) N(2) N(3)
							S(4) S(5) S(8)
							break;
						CASE(1,s0 + xverts)
							N(6) N(7) N(9)
							S(0) S(2) S(3)
							break;
						CASE(2,s0)
							N(4) N(6) N(10)
							S(0) S(1) S(3)
							break;
						CASE(3,s0 + 1)
							N(0) N(1) N(2)
							S(5) S(7) S(11)
							break;
						CASE(4,s0)
							N(0) N(5) N(8)
							S(2) S(6) S(10)
							break;
						CASE(5,s0 + 1)
							N(3) N(7) N(11)
							S(0) S(4) S(8)
							break;
						CASE(6,s0 + xverts)
							N(2) N(4) N(10)
							S(1) S(7) S(9)
							break;
						CASE(7,s0 + xverts + 1)
							N(1) N(6) N(9)
							S(3) S(5) S(11)
							break;
						CASE(8,s1)
							N(0) N(4) N(5)
							S(9) S(10) S(11)
							break;
						CASE(9,s1 + xverts)
							N(8) N(10) N(11)
							S(1) S(6) S(7)
							break;
						CASE(10,s1)
							N(8) N(9) N(11)
							S(2) S(4) S(6)
							break;
						CASE(11,s1 + 1)
							N(3) N(5) N(7)
							S(8) S(9) S(10)
							break;
#undef CASE
#undef S
#undef N
					}

					add(v0); add(v1);

					while ( link(cube(c0,v0,v1)) && *c0 ) {
/* printf("transfer to %d %d along %d %d\n",jj,ii,t0,t1);
printf("destination 0x%x\n",*c0); */
						v0 = t0; v1 = t1;
					}

					addVertex(-1);
				}

				s0++;	s1++;
			}
		}
	}

	free(sheet0);
	free(sheet1);
	free(edge0);
	free(edge1);
	free(cell);

        // Finish editing the vertex array and set the number of points.
        gVertPtr[0] = gVertPtr[1];
        gVP->vertex.finishEditing();
        gVP->vertex.setNum(gNpoints);

        gSS->coordIndex.finishEditing();
        gSS->coordIndex.setNum(gNmeshes + gNverts);

        // Set reasonable values for the zero'th orderedRGBA and zero'th
        // normal if necessary.  Otherwise, GL gets all screwed up.
        // Also finish editing the arrays.
        if (gVP->materialBinding.getValue() ==
            SoMaterialBinding::PER_VERTEX_INDEXED) {
            gColorPtr[0] = gColorPtr[1];
            gVP->orderedRGBA.finishEditing();
            gVP->orderedRGBA.setNum(gNpoints);
        }
        if (gVP->normalBinding.getValue() ==
            SoNormalBinding::PER_VERTEX_INDEXED) {
            gNormPtr[0] = gNormPtr[1];
            gVP->normal.finishEditing();
            gVP->normal.setNum(gNpoints);
        }
}

void
add(int v)
{
	// Save the vertices
	t0 = t1;	t1 = v;

	switch(v) {
		case 0: addVertex(e0->x);            break;
		case 1:	addVertex((e0 + xverts)->x); break;
		case 2:	addVertex(e0->y);            break;
		case 3:	addVertex((e0 + 1)->y);      break;
		case 4:	addVertex(e0->z);            break;
		case 5:	addVertex((e0 + 1)->z);      break;
		case 6:	addVertex((e0 + xverts)->z); break;
		case 7:	addVertex((e0 + xverts + 1)->z); break;
		case 8:	addVertex(e1->x);            break;
		case 9:	addVertex((e1 + xverts)->x); break;
		case 10: addVertex(e1->y);           break;
		case 11: addVertex((e1 + 1)->y);     break;
	}
}

int
link(int face)
{
	int	rval;

	switch( face ) {
		case 0:
		case 1:
			rval = 0;
			break;
		case 2:
			if ( --ii < 0 )
				return 0;

			if ( t0 == 4 || t0 == 5 )
				t0++;
			if ( t1 == 4 || t1 == 5 )
				t1++;
			
			t0++;	t1++;

			e0 -= xverts;
			e1 -= xverts;
			c0 -= xcells;

			rval = 1;
			break;
		case 3:
			if ( ++ii == ycells )
				return 0;

			if ( t0 == 6 || t0 == 7 )
				t0--;

			if ( t1 == 6 || t1 == 7 )
				t1--;

			t0--;	t1--;

			e0 += xverts;
			e1 += xverts;
			c0 += xcells;

			rval = 1;
			break;
		case 4:
			if ( --jj < 0 )
				return 0;

			t0++;	t1++;

			e0 -= 1;
			e1 -= 1;
			c0 -= 1;

			rval = 1;
			break;
		case 5:
			if ( ++jj == xcells )
				return 0;

			t0--;	t1--;

			e0 += 1;
			e1 += 1;
			c0 += 1;

			rval = 1;
			break;
	}

	return rval;
}

int
cube(short *edge, int v0, int v1)
{
	int   v2;		/* next intersection */
	short v2b;		/* bit representations for intersection */
	short mask;		/* mask for visited faces */
	short flag;

	/* test if this sheet has already been done */
	if ( !(*edge & (1 << v0)) )
		return 0;
	if ( !(*edge & (1 << v1)) )
		return 0;

/* printf("0x%x: ",*edge);
printf("%d %d ",v0,v1); */

	/* turn off these bits in the current voxel */
	*edge &= ~(1 << v0);
	*edge &= ~(1 << v1);

	/* mask off anything else on the shared face */
	mask = ~f2v[ilog2[v2f[v0] & v2f[v1]]];

	while ( 1 ) {
		/* get all the candidate vertices */
		flag = *edge & v2v[v0] & mask;

		/* test for one option */
#if 000
		if ( bitc12(v2b = flag) == 1 )
#else
		v2b = flag;
		if ( bitc12(v2b) == 1 )
#endif
			;
		/* test for in a face */
		else if ( (v2b = (flag &= v2n[v0]) & v2v[v1]) )
			;
		/* otherwise ambiguous */
		else if ( flag )
		{
			short tmp = v2n[v1] & *edge;

			if ( bitc12(tmp) == 1 )
			{
/*				printf("problem case "); */
				v2b = flag & v2v[ilog2b(tmp)];
			}
			else
			{
/*				printf("ambiguity "); */
				v2b = 1 << ilog2b(flag);
			}
		}
		else
		{
/*			printf("error\n"); */
			*edge = 0;
			return 0;
		}

		/* add this vertex */
		v2 = ilog2b(v2b);
		add(v2);
/* printf("%d ",v2); */
		/* remove this intersection */
		*edge &= ~v2b;

		/* test for termination */
		if ( v2b & v2v[v1] )
			break;

		/* mask out this face */
		mask &= ~f2v[ilog2[v2f[v0] & v2f[v2]]];

		/* move along */
		v0 = v1;	v1 = v2;
	}
/* printf("\n"); */

	return ilog2[v2f[v1] & v2f[v2]];
}

void
getSheet(float *v, int z)
{
    int i;
    int n;
    int nchan;
    void *ptr;

    nchan = gNumDataVar;
    n = gDataDims[0] * gDataDims[1];

    switch (gDataType) {
// --> sas
			case LATTICE_DATA_BOOL:
        ptr = (SbBool *)gDataValues + n*nchan*z + gSurfaceVariable;
        for ( i = 0; i < n; i++ ) { 
            *v++ = (float)*(SbBool *)ptr;
            ptr = (SbBool *)ptr + nchan;
        } 
        break;
// <-- sas
      case LATTICE_DATA_INT8:
        ptr = (uint8_t *)gDataValues + n*nchan*z + gSurfaceVariable;
        for ( i = 0; i < n; i++ ) { 
            *v++ = (float)*(uint8_t *)ptr;
            ptr = (uint8_t *)ptr + nchan;
        } 
        break;

      case LATTICE_DATA_INT16:
        ptr = (short *)gDataValues + n*nchan*z + gSurfaceVariable;
        for ( i = 0; i < n; i++ ) { 
            *v++ = (float)*(short *)ptr;
            ptr = (short *)ptr + nchan;
        } 
        break;

      case LATTICE_DATA_INT32:
        ptr = (int32_t *)gDataValues + n*nchan*z + gSurfaceVariable;
        for ( i = 0; i < n; i++ ) { 
            *v++ = (float)*(int32_t *)ptr;
            ptr = (int32_t *)ptr + nchan;
        } 
        break;

      case LATTICE_DATA_FLOAT32:
        ptr = (float *)gDataValues + n*nchan*z + gSurfaceVariable;
        for ( i = 0; i < n; i++ ) { 
            *v++ = (float)*(float *)ptr;
            ptr = (float *)ptr + nchan;
        } 
        break;

      case LATTICE_DATA_FLOAT64:
        ptr = (double *)gDataValues + n*nchan*z + gSurfaceVariable;
        for ( i = 0; i < n; i++ ) { 
            *v++ = (float)*(double *)ptr;
            ptr = (double *)ptr + nchan;
        } 
        break;

      case LATTICE_DATA_BAD_TYPE:
        break;
    }
}

int
addEdge(float t, float v0, float v1, int i0, int j0, int k0,
    int i1, int j1, int k1)
{
    int     index[3];   /* coord indices */
    float   w0,w1;      /* vertex weights */
    SbVec3f p0,  p1;    /* vertex coordinates */
    SbVec3f p;          /* intersection position */

    /* Reset the size of the arrays if necessary */
    if (gNpoints == gEdgeCount*EDGE_INCR) {
        gEdgeCount *= 2;
        int32_t newSize = gEdgeCount * EDGE_INCR;
        gVP->vertex.finishEditing();
        gVP->vertex.setNum(newSize);
        gVertPtr = gVP->vertex.startEditing();
        if (gVP->materialBinding.getValue() ==
            SoMaterialBinding::PER_VERTEX_INDEXED) {
            gVP->orderedRGBA.finishEditing();
            gVP->orderedRGBA.setNum(newSize);
            gColorPtr = gVP->orderedRGBA.startEditing();
        }
        if (gVP->normalBinding.getValue() ==
            SoNormalBinding::PER_VERTEX_INDEXED) {
            gVP->normal.finishEditing();
            gVP->normal.setNum(newSize);
            gNormPtr = gVP->normal.startEditing();
        }
    }

    /* compute weights */
    w1 = (t - v0)/(v1 - v0);
    w0 = 1.0 - w1;

    /* find the location */
    index[0] = i0;  index[1] = j0;  index[2] = k0;
    p0 = gDataLat->getCoord(index);

    index[0] = i1;  index[1] = j1;  index[2] = k1;
    p1 = gDataLat->getCoord(index);

    p[0] = w0*p0[0] + w1*p1[0];
    p[1] = w0*p0[1] + w1*p1[1];
    p[2] = w0*p0[2] + w1*p1[2];

    gVertPtr[gNpoints] = p;

    if ( gGenerateNormals )
    {
        float len;
        float g0[3],g1[3];

        /* get gradients */
        gradient(i0,j0,k0,g0);
        gradient(i1,j1,k1,g1);

        /* add weighted gradients */
        g0[0] = w0*g0[0] + w1*g1[0];
        g0[1] = w0*g0[1] + w1*g1[1];
        g0[2] = w0*g0[2] + w1*g1[2];
        
        /* normalize the result */
        len = sqrt(g0[0]*g0[0] + g0[1]*g0[1] + g0[2]*g0[2]);
        if ( len > 0.0 ) {
            if ( gFlipNormals )
                len = 1.0/len;
            else
                len = -1.0/len;
        }

        g0[0] *= len;
        g0[1] *= len;
        g0[2] *= len;

        /* add the normal vector to the list */
        gNormPtr[gNpoints] = g0;
    }

    if (gColorValues) {
        float xx;

        /* read lookup data */
        interpColorData( p, xx );
        switch (gColorDataType) {
          case LATTICE_DATA_INT8:
            if (gColorMapValues == NULL) {
                xx = xx / 255.0;
            }
            break;

          case LATTICE_DATA_INT16:
            if (gColorMapValues == NULL) {
                xx = xx / 65535.0;
            }
            break;

          case LATTICE_DATA_INT32:
            if (gColorMapValues == NULL) {
                xx = xx / 255.0;
            }
            break;

          case LATTICE_DATA_FLOAT32:
            if (gColorMapValues == NULL) {
                xx = xx / 1.0;
            }
            break;

          case LATTICE_DATA_FLOAT64:
            if (gColorMapValues == NULL) {
                xx = xx / 1.0;
            }
            break;

          case LATTICE_DATA_BOOL:
          case LATTICE_DATA_BAD_TYPE:
            break;
        }
        /* convert to a color */
        uint32_t pc;
        if (gColorMapValues) {
            interpColorMap( xx, pc);
        }
        else {
            float tmp = 255*xx;
            SbColor c(tmp, tmp, tmp);
            pc = c.getPackedValue();
        }

        /* add the color to the list */
        gColorPtr[gNpoints] = pc;
    }

    return gNpoints++;
}
    
void
addVertex(int v)
{
    int32_t total = gNmeshes + gNverts;

    if (total == gVertCount*VERT_INCR) {
        gVertCount *= 2;
        gSS->coordIndex.finishEditing();
        gSS->coordIndex.setNum(gVertCount*VERT_INCR);
        gCoordIndexPtr = gSS->coordIndex.startEditing();
    }
    gCoordIndexPtr[total] = v;

    if ( v < 0 ) {
        gNmeshes++; 
    }
    else {
        gNverts++;
    }
}

float
getValue(int x, int y, int z)
{
    void *ptr;

    switch (gDataType) {
// --> sas
      case LATTICE_DATA_BOOL:
        ptr = (SbBool *)gDataValues + gSurfaceVariable + 
            gNumDataVar*(x + gDataDims[0]*(y + gDataDims[1]*z)); 
        return (float)*(SbBool *)ptr;
// <-- sas

      case LATTICE_DATA_INT8:
        ptr = (uint8_t *)gDataValues + gSurfaceVariable + 
            gNumDataVar*(x + gDataDims[0]*(y + gDataDims[1]*z)); 
        return *(uint8_t *)ptr;

      case LATTICE_DATA_INT16:
        ptr = (short *)gDataValues + gSurfaceVariable + 
            gNumDataVar*(x + gDataDims[0]*(y + gDataDims[1]*z)); 
        return *(short *)ptr;

      case LATTICE_DATA_INT32:
        ptr = (int32_t *)gDataValues + gSurfaceVariable + 
            gNumDataVar*(x + gDataDims[0]*(y + gDataDims[1]*z)); 
        return (float)*(int32_t *)ptr;
   
      case LATTICE_DATA_FLOAT32:
        ptr = (float *)gDataValues + gSurfaceVariable + 
            gNumDataVar*(x + gDataDims[0]*(y + gDataDims[1]*z)); 
        return *(float *)ptr;
   
      case LATTICE_DATA_FLOAT64:
        ptr = (double *)gDataValues + gSurfaceVariable + 
            gNumDataVar*(x + gDataDims[0]*(y + gDataDims[1]*z)); 
        return *(double *)ptr;

      case LATTICE_DATA_BAD_TYPE:
         break;
    }
    return 0.0;
}

void
gradient(int x, int y, int z, float g[3])
{
    int index[3];
    float   v0,v1;
    float   len;
    SbVec3f p0, p1;

    /* initialize gradient */
    g[0] = g[1] = g[2] = 0.0;

#define GET(X,Y,Z,V,P) { \
    V = getValue(X,Y,Z); \
    index[0] = X; index[1] = Y; index[2] = Z; \
    P = gDataLat->getCoord(index); \
    }

#define ADD \
    p1[0] -= p0[0]; p1[1] -= p0[1]; p1[2] -= p0[2]; \
    len = p1[0]*p1[0] + p1[1]*p1[1] + p1[2]*p1[2]; \
    if ( len > 0.0 ) len = 1.0/len; \
    g[0] += (v1 - v0)*p1[0]*len; \
    g[1] += (v1 - v0)*p1[1]*len; \
    g[2] += (v1 - v0)*p1[2]*len;

    /* get lower value */
    if ( x > 0 ) GET(x - 1,y,z,v0,p0)
    else GET(x,y,z,v0,p0)

    /* get upper value */
    if ( x < gDataDims[0] - 1 ) GET(x + 1,y,z,v1,p1)
    else GET(x,y,z,v1,p1)

    ADD

    /* get lower value */
    if ( y > 0 ) GET(x,y - 1,z,v0,p0)
    else GET(x,y,z,v0,p0)

    /* get upper value */
    if ( y < gDataDims[1] - 1 ) GET(x,y + 1,z,v1,p1)
    else GET(x,y,z,v1,p1)

    ADD

    /* get lower value */
    if ( z > 0 ) GET(x,y,z - 1,v0,p0)
    else GET(x,y,z,v0,p0)

    /* get upper value */
    if ( z < gDataDims[2] - 1 ) GET(x,y,z + 1,v1,p1)
    else GET(x,y,z,v1,p1)

    ADD
#undef GET
#undef ADD
}

void
lookupFromColorData(ChemLattice3 *colorLat)
{
    int i, j;
    SbVec3i dims = colorLat->dimension.getValue();

    int32_t np = 8;
    for (i = 0; i < np; i++) {
        int32_t offset = 0;
        int32_t stride = 1;
        for (j = 0; j < 3; j++) {
            if (i & (1 << j)) {
                offset += stride;
            }
            stride *= dims[j];
        }
        gColorDataOffset[i] = offset;
    }
}

int
interpColorData(SbVec3f index, float &dst)
{
    int i;
    int dflag = 0;
    int32_t ix[32];
    float   fx[32];

    const float *coord = gColorLat->coord.getValues(0);
    switch (gColorLat->getCoordType()) {
      case LATTICE_COORD_UNIFORM:
        for (i = 0; i < 3; i++) {
            float x = (gColorDims[i] - 1) *
                      (index[i] - coord[2*i]) /
                      (coord[2*i+1] - coord[2*i]);
            ix[i] = (int)floor(x);
            fx[i] = x - ix[i];
        }
        break;

      case LATTICE_COORD_RECTILINEAR:
        for (i = 0; i < 3; i++) {
            int32_t len = gColorDims[i];
            const float *start = coord;

            /* find the interval by binary search */
            ix[i] = 0;
            while ( len > 1 ) {
                if ( index[i] >= start[len/2] ) {
                    start += len/2;
                    ix[i] += len/2;
                    len = len - len/2;
                }
                else {
                    len = len/2;
                }
            }

            if ( ix[i] == gColorDims[i] - 1 ) {
                fx[i] = 0.0;
                if ( index[i] > coord[ix[i]] )
                    dflag = 1;
            }
            else {
                fx[i] = (index[i] - coord[ix[i]])/
                    (coord[ix[i]+1] - coord[ix[i]]);
            }

            coord += gColorDims[i];
        }
        break;

      case LATTICE_COORD_BAD_TYPE:
        break;
    }

    for (i = 0; i < 3; i++) {
        if (ix[i] < 0) {
            dflag = 1;
            ix[i] = 0;
            fx[i] = 0.0;
        }
        else if (ix[i] >= gColorDims[i] - 1) {
            if ( ix[i] >= gColorDims[i] || fx[i] > 0.0 ) {
                dflag = 1;
            }
            ix[i] = gColorDims[i] - 2;
            fx[i] = 1.0;
        }
        else if ( fx[i] < 0.0 ) {
            dflag = 1;
            fx[i] = 0.0;
        }
    }

    switch (gColorDataType) {
      case LATTICE_DATA_INT8:
#define TYPE uint8_t
#define CXTYPE LATTICE_DATA_INT8
#include "linear.inc"
        break;

      case LATTICE_DATA_INT16:
#define TYPE short
#define CXTYPE LATTICE_DATA_INT16
#include "linear.inc"
        break;

      case LATTICE_DATA_INT32:
#define TYPE int32_t
#define CXTYPE LATTICE_DATA_INT32
#include "linear.inc"
        break;

      case LATTICE_DATA_FLOAT32:
#define TYPE float
#define CXTYPE LATTICE_DATA_FLOAT32
#include "linear.inc"
        break;

      case LATTICE_DATA_FLOAT64:
#define TYPE double
#define CXTYPE LATTICE_DATA_FLOAT64
#include "linear.inc"
        break;

      default:
        break;
    }
    return dflag;
}

int
interpColorMap(float index, uint32_t &dst)
{
    int dflag = 0;
    int32_t ix;
    float   fx;

    float x = (gNumColors - 1) *
              (index - gColorMapMin) /
              (gColorMapMax - gColorMapMin);
    ix = (int)floor(x);
    fx = x - ix;

    if (ix < 0) {
        dflag = 1;
        ix = 0;
        fx = 0.0;
    }
    else if (ix >= gNumColors - 1) {
        if ( ix >= gNumColors || fx > 0.0 ) {
            dflag = 1;
        }
        ix = gNumColors - 2;
        fx = 1.0;
    }
    else if ( fx < 0.0 ) {
        dflag = 1;
        fx = 0.0;
    }

    uint32_t pc;
    float trans0, trans1;
    //SbColor c0, c1, cf;
    SbColor c0, c1;

    pc = gColorMapValues[ix];
    c0.setPackedValue(pc, trans0);

    pc = gColorMapValues[ix + 1];
    c1.setPackedValue(pc, trans1);

    c0[0] = (1.0 - fx)*c0[0] + fx*c1[0];
    c0[1] = (1.0 - fx)*c0[1] + fx*c1[1];
    c0[2] = (1.0 - fx)*c0[2] + fx*c1[2];
    trans0 = (1.0 - fx)*trans0 + fx*trans1;

//    printf ("%f %f %f %f\n", c0[0], c0[1], c0[2], trans0);

     pc = c0.getPackedValue(trans0);
//    pc = c0.getPackedValue();

    dst = pc;

    return dflag;
}
