/*
 * ContourLib.c++
 *
 *     Contour drawing functions
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

/* 
 * cntrLinear draws linearly interpolated contour lines on a rectangular grid
 * of size MxN. The grid is drawn from the array data, which has leading dim
 * ldA, and stride between points of strideA. 
 * dx, dy, and the x/y-start variables give the origin and offsets of grid points.
 * 
 * Do a single contour level at a time.
 *
 * Makes use of a global structure to send information to the displayLines
 * routine, including color, z-plane position, and non-uniform coords.
 */

#include <stdio.h>
#include <math.h>
#include <assert.h>

#include "inv/fields/SoMFInt32.H"
#include "inv/fields/SoMFFloat.H"
#include "inv/fields/SoMFShort.H"

#include "inv/nodes/SoIndexedLineSet.H"
#include "inv/nodes/SoMaterialBinding.H"
#include "inv/nodes/SoNormalBinding.H"

#include "inv/SbColor.H"

#include "inv/ChemKit/SbVecI.H"
#include "inv/ChemKit/MFDouble.H"
#include "inv/ChemKit/MFInt8.H"
#include "inv/ChemKit/ChemLattice3.H"
#include "inv/ChemKit/ChemCondenser.H"

static SoIndexedLineSet *lineSet;
static SoVertexProperty *lineSetVP;
static int32_t currentPoint;
static int32_t currentIndex;

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#define LINEBUFFER 2048

#define NUM_TRIS 4
#define NUM_VERT 5
#define VERT_PER_TRI 3
#define XYZ_3D 3

#define LINECASE_ABC \
            drawLine = 1; \
            setIntersectLoc( height, a, b, locs, aVert, bVert, xyBegin ); \
            setIntersectLoc( height, a, c, locs, aVert, cVert, xyEnd );
#define LINECASE_BAC \
            drawLine = 1; \
            setIntersectLoc( height, b, a, locs, bVert, aVert, xyBegin ); \
            setIntersectLoc( height, b, c, locs, bVert, cVert, xyEnd );
#define LINECASE_CBA \
            drawLine = 1; \
            setIntersectLoc( height, c, b, locs, cVert, bVert, xyBegin ); \
            setIntersectLoc( height, c, a, locs, cVert, aVert, xyEnd );

typedef   enum {
    orient_xy,                 /* contour in the X-Y plane */
    orient_xz,                 /* contour in the X-Z plane */
    orient_yz                  /* contour in the Y-Z plane */
} orientType;


typedef struct {
    orientType orientation;       /* XY, XZ, or YZ */
    float planeOffset;            /* coordinate of the plane in the */
                                  /* missing dimension, eg Z for X-Y */
    float RGBo[4];                /* 4-vector: Red, green, blue, opacity */
    const int32_t *dims;
    short coordType;
    const float *coord;

    short colorVar;
    ChemLattice3 *colorLat;
    const void *colorValues;
    int32_t numColors;
    float minValue;
    float maxValue;
    const uint32_t *colorMap;
    int32_t colorDataOffset[8];
} globalData;


static globalData glob;
  

#if 000
static float bilinearInterp( float dx, float dy, 
                             float x0y0, float x1y0, 
                             float x0y1, float x1y1 );
#endif
static void displayLines( int segmentCount, float *xyzBuf );
static void setIntersectLoc( float height, 
                             float a, 
                             float b, 
                             float locs[][2], 
                             int   aVert, 
                             int   bVert, 
                             float xyLoc[2] );
static void cntrLinear( short dataType,
                        const void *data, 
                        int ldA, 
                        int strideA,
                        int m, 
                        int n,
                        float height,
                        float dx, 
                        float dy, 
                        float xstart, 
                        float ystart );
static void contour( SbBool   iAxis,
                     SbBool   jAxis,
                     SbBool   kAxis,
                     short    nDim,
                     const int32_t  *dims,
                     int32_t  nDataVar,
                     short    dataType,
                     const void    *dataVals,
                     short    coordType,
                     const float   *coord,
                     int32_t  nLevel,
                     const float    *levels,
                     SoIndexedLineSet *theLineSet,
                     SoVertexProperty *theLineSetVP);

static void addColor(SoVertexProperty *theLineSetVP);
static int interpColorData(SbVec3f index, float &dst);
static int interpColorMap(float index, uint32_t &dst);

#if 000
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
float bilinearInterp( float dx, float dy, 
                      float x0y0, float x1y0, 
                      float x0y1, float x1y1 )
{
    /* interpolate linearly on the X0, then on the interpolated points. */
    float x0Mid, x1Mid, ans;

    if (dy == 0.0) {
      x0Mid = x0y0;
      x1Mid = x1y0;
    }
    else {
      x0Mid = dy*(x0y1-x0y0) + x0y0;
      x1Mid = dy*(x1y1-x1y0) + x1y0;
    }

    if (dx == 0.0) 
      ans = x0Mid;
    else
      ans = dx*(x1Mid-x0Mid) + x0Mid;

    return( ans );
}         
#endif
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void displayLines( int segmentCount, float *xyzBuf )
{
    float *xyzPtr;
    int   i, j, ix, jx, jy, ky;
    float x, y;
    const float *iPerim, *jPerim, *kPerim;
    int32_t vertexList[XYZ_3D*LINEBUFFER];

    int32_t nPt = 2*segmentCount;
    int32_t nInd = 3*segmentCount;
    assert(nPt <= 2*XYZ_3D*LINEBUFFER);
    assert(nInd <= XYZ_3D*LINEBUFFER);

    xyzPtr = xyzBuf;

    if ( nPt > 0 && nInd > 0 ) {
        switch (glob.coordType) {
          case LATTICE_COORD_UNIFORM:
            /* fill  in the missing offset in z/y/x. */
    
            switch (glob.orientation) {
              case orient_xy:
                for (i=0; i<nPt; i++, xyzPtr += XYZ_3D) {
                    xyzPtr[2] = glob.planeOffset;     /* Set the z offset. */
                }
                break;
    
              case orient_xz:
                for (i=0; i<nPt; i++, xyzPtr += XYZ_3D) {
                    xyzPtr[2] = xyzPtr[1];            /* Move y to z */
                    xyzPtr[1] = glob.planeOffset;     /* Set the y offset. */
                }
                break;
      
              case orient_yz:
                for (i=0; i<nPt; i++, xyzPtr += XYZ_3D) {
                    xyzPtr[2] = xyzPtr[1];            /* Move y to z */
                    xyzPtr[1] = xyzPtr[0];            /* Move x to y */
                    xyzPtr[0] = glob.planeOffset;     /* Set the x offset. */
                }
                break;
            }
            break;

          case LATTICE_COORD_RECTILINEAR: /* The x/y/z locations are */
                                          /* fractional indices into the */
                                          /* perimeter coordinate array. */
                                          /* Lookup on the integer part */
                                          /* and then interpolate on the */
                                          /* fraction. */
            switch (glob.orientation) {
              case orient_xy:
                for (i=0; i<nPt; i++, xyzPtr += XYZ_3D) {
                    iPerim = glob.coord + 0;
                    jPerim = glob.coord + glob.dims[0];
                    x = xyzPtr[0];
                    ix = (int) x;
                    xyzPtr[0] = iPerim[ ix ]
                              + (x-ix) * (iPerim[ ix+1 ]-iPerim[ ix ]);
  
                    y = xyzPtr[1];
                    jy = (int) y;
                    xyzPtr[1] = jPerim[ jy ] 
                              + (y-jy) * (jPerim[ jy+1 ]-jPerim[ jy ]);
                                        
                    xyzPtr[2] = glob.planeOffset;
                }
                break;
        
              case orient_xz:                   /* IJK = XZY, watch */
                                                /* order of copying */
                for (i=0; i<nPt; i++, xyzPtr += XYZ_3D) {
                    iPerim = glob.coord + 0;
                    kPerim = glob.coord + glob.dims[0] + glob.dims[1];
                    x = xyzPtr[0];
                    ix = (int) x;
                    xyzPtr[0] = iPerim[ ix ] 
                              + (x-ix) * (iPerim[ ix+1 ]-iPerim[ ix ]);

                    y = xyzPtr[1];
                    ky = (int) y;
                    xyzPtr[2] = kPerim[ ky ] 
                              + (y-ky) * (kPerim[ ky+1 ]-kPerim[ ky ]);

                    xyzPtr[1] = glob.planeOffset;
                }
                break;

              case orient_yz:                   /* IJK = YZX Watch */
                                                /* order of copying */
                for (i=0; i<nPt; i++, xyzPtr += XYZ_3D) {
                    jPerim = glob.coord + glob.dims[0];
                    kPerim = glob.coord + glob.dims[0] + glob.dims[1];

                    y = xyzPtr[1];
                    ky = (int) y;
                    xyzPtr[2] = kPerim[ ky ] 
                              + (y-ky) * (kPerim[ ky+1 ]-kPerim[ ky ]);

                    x = xyzPtr[0];
                    jx = (int) x;
                    xyzPtr[1] = jPerim[ jx ] 
                              + (x-jx) * (jPerim[ jx+1 ]-jPerim[ jx ]);

                    xyzPtr[0] = glob.planeOffset;
                }
                break;
            }
            break;
        }

        // Update the values in the vertexList
        int32_t *vPtr = vertexList;
        int32_t nInd3 = nInd / 3;
        for (i=0, j=currentPoint; i<nInd3; i++) {
            *(vPtr++) = j++;
            *(vPtr++) = j++;
            *(vPtr++) = -1;
        }

#ifdef DONTEXECUTE
        for (i=0; i<nPt; i++) {
            fprintf(fPtr, " i %d XYZ: %6.2f %6.2f %6.2f\n", i, 
                 xyzPtr[XYZ_3D*i], xyzPtr[XYZ_3D*i+1], xyzPtr[XYZ_3D*i+2] );
        }
        for (i=0; i<nInd/3; i++) {
            fprintf(fPtr, " line i %d vrt: %d %d %d\n", i, 
                 vertexList[3*i], vertexList[3*i+1], vertexList[3*i+2] );
        }
#endif
        lineSetVP->vertex.setValues(currentPoint, nPt, 
                                   (const float (*)[3])xyzBuf);
        lineSet->coordIndex.setValues(currentIndex, nInd, vertexList);
        currentPoint += nPt;
        currentIndex += nInd;
    }
}
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void addColor(SoVertexProperty *theLineSetVP)
{
   int32_t i;
   int32_t nPt = theLineSetVP->vertex.getNum();
   float xx;
   SbVec3f p;
   const SbVec3f *coords = theLineSetVP->vertex.getValues(0);

   theLineSetVP->orderedRGBA.setNum(nPt);
   uint32_t *orderedRGBA = theLineSetVP->orderedRGBA.startEditing();

   for (i = 0; i < nPt; i++) {
      p = coords[i];

      interpColorData(p, xx);
      switch (glob.colorLat->getDataType()) {
          case LATTICE_DATA_INT8:
            if (glob.colorMap == NULL) {
                xx = xx / 255.0;
            }
            break;

          case LATTICE_DATA_INT16:
            if (glob.colorMap == NULL) {  
                xx = xx / 65535.0;
            }
            break;

          case LATTICE_DATA_INT32:
            if (glob.colorMap == NULL) {
                xx = xx / 255.0;
            }
            break;
      
          case LATTICE_DATA_FLOAT32:
            if (glob.colorMap == NULL) {
                xx = xx / 1.0;
            }
            break;

          case LATTICE_DATA_FLOAT64:
            if (glob.colorMap == NULL) {
                xx = xx / 1.0;
            }
            break;

          case LATTICE_DATA_BOOL:
          case LATTICE_DATA_BAD_TYPE:
            break;
        }
        /* convert to a color */
        uint32_t pc;
        if (glob.colorMap) {
            interpColorMap( xx, pc);
        }
        else {
            float tmp = 255*xx;
            SbColor c(tmp, tmp, tmp);
            pc = c.getPackedValue();
        }

        /* add the color to the list */
        orderedRGBA[i] = pc;
    }
    theLineSetVP->orderedRGBA.finishEditing();
}
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int
interpColorData(SbVec3f index, float &dst)
{
    int i;
    int dflag = 0;
    int32_t ix[32];
    float   fx[32];
            
    short numColorVar = glob.colorLat->nDataVar.getValue();
    SbVec3i dims = glob.colorLat->dimension.getValue();
    const float *coord = glob.colorLat->coord.getValues(0);

    switch (glob.colorLat->getCoordType()) {
      case LATTICE_COORD_UNIFORM:
        for (i = 0; i < 3; i++) { 
            float x = (dims[i] - 1) *
                      (index[i] - coord[2*i]) /
                      (coord[2*i+1] - coord[2*i]);
            ix[i] = (int)floor(x);
            fx[i] = x - ix[i];
        }
        break;
            
      case LATTICE_COORD_RECTILINEAR:
        for (i = 0; i < 3; i++) {  
            int32_t len = dims[i];  
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
                      
            if ( ix[i] == dims[i] - 1 ) {   
                fx[i] = 0.0;
                if ( index[i] > coord[ix[i]] )
                    dflag = 1;
            } 
            else {
                fx[i] = (index[i] - coord[ix[i]])/
                    (coord[ix[i]+1] - coord[ix[i]]);
            }
            
            coord += dims[i];
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
        else if (ix[i] >= dims[i] - 1) {
            if ( ix[i] >= dims[i] || fx[i] > 0.0 ) {
                dflag = 1;
            }
            ix[i] = dims[i] - 2;
            fx[i] = 1.0;
        }
        else if ( fx[i] < 0.0 ) {
            dflag = 1;
            fx[i] = 0.0;
        }
    }

    switch (glob.colorLat->getDataType()) {
      case LATTICE_DATA_INT8:
#define TYPE uint8_t
#define CXTYPE LATTICE_DATA_INT8
#include "clinear.inc"
        break;

      case LATTICE_DATA_INT16:
#define TYPE short
#define CXTYPE LATTICE_DATA_INT16
#include "clinear.inc"
        break;

      case LATTICE_DATA_INT32:
#define TYPE int32_t
#define CXTYPE LATTICE_DATA_INT32
#include "clinear.inc"
        break;

      case LATTICE_DATA_FLOAT32:
#define TYPE float
#define CXTYPE LATTICE_DATA_FLOAT32
#include "clinear.inc"
        break;

      case LATTICE_DATA_FLOAT64:
#define TYPE double
#define CXTYPE LATTICE_DATA_FLOAT64
#include "clinear.inc"
        break;

      default:
        break;
    }
    return dflag;
}
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int
interpColorMap(float index, uint32_t &dst)
{
    int dflag = 0;
    int32_t ix;
    float   fx;

    if (glob.numColors == 1) {
        dflag = 1;
        ix = 0;
        fx = 0.0;
    }
    else {
        float x = (glob.numColors - 1) *
                  (index - glob.minValue) /
                  (glob.maxValue - glob.minValue);
        ix = (int)floor(x);
        fx = x - ix;
    
        if (ix < 0) {
            dflag = 1;
            ix = 0;
            fx = 0.0;
        }
        else if (ix >= glob.numColors - 1) {
            if ( ix >= glob.numColors || fx > 0.0 ) {
                dflag = 1;
            }
            ix = glob.numColors - 2;
            fx = 1.0;
        }
        else if ( fx < 0.0 ) {
            dflag = 1;
            fx = 0.0;
        }
    }

    uint32_t pc;
    float trans0, trans1;
    SbColor c0, c1;

    pc = glob.colorMap[ix];
    c0.setPackedValue(pc, trans0);

    if (glob.numColors > 1) {
        pc = glob.colorMap[ix + 1];
        c1.setPackedValue(pc, trans1);

        c0[0] = (1.0 - fx)*c0[0] + fx*c1[0];
        c0[1] = (1.0 - fx)*c0[1] + fx*c1[1];
        c0[2] = (1.0 - fx)*c0[2] + fx*c1[2];
        trans0 = (1.0 - fx)*trans0 + fx*trans1;
    }

// printf ("%f %f %f\n", c0[0], c0[1], c0[2]);

//    pc = c0.getPackedValue(trans0);
    pc = c0.getPackedValue();

    dst = pc;

    return dflag;
}
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void setIntersectLoc( float height, 
                      float a, 
                      float b, 
                      float locs[][2], 
                      int   aVert, 
                      int   bVert, 
                      float xyLoc[2] )
{
    float t, t1;

    t = (height - a) / (b - a);
    t1 = 1.0-t;
    xyLoc[0] = locs[aVert][0]*t1 + locs[bVert][0]*t;
    xyLoc[1] = locs[aVert][1]*t1 + locs[bVert][1]*t;
}
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void cntrLinear( short dataType,
                 const void *data, 
                 int ldA, 
                 int strideA,
                 int m, 
                 int n,
                 float height,
                 float dx, 
                 float dy, 
                 float xstart, 
                 float ystart )
{

    int i, j;
    float  ht[5];
    /* ht is the local data value from:
     *       j         j+1
     *  i    1         4
     *            0     
     *  i+1  2         3
     */
  
    /* locs and offset refer to the vertex ordering */
    float locs[NUM_VERT][2];
  
    static float offset[NUM_VERT][2] = {
      { 0.5, 0.5},
      { 0.0, 0.0},
      { 1.0, 0.0},
      { 1.0, 1.0},
      { 0.0, 1.0} };
  
    /* tris makes up the triangles by listing vertices */

    /* Choose this triangle numbering to make a saddle point look nicer */
    /* than numbering with 0 in each triangle (hypotenuses radiating out */
    /* from 0 at center of square). */

    static int    tris[NUM_TRIS][VERT_PER_TRI] = { 
      { 1, 2, 0},
      { 2, 3, 0},
      { 3, 4, 0},
      { 4, 1, 0} };
  
    static int switBase0 = 1;
    static int switBase1 = 3;
    static int switBase2 = 9;
  
    float   xyzBuf[2*XYZ_3D*LINEBUFFER];

    int     segmentCount;
    float   *linePtr;
  
    float xs, ys;
  
    int iVert, it, swit, drawLine;
    int aVert, bVert, cVert;
    int *trisit;
    float a, b, c;
    float xyEnd[2], xyBegin[2];
    const void *dataj, *dataji;
    
  
    linePtr = xyzBuf;
    segmentCount = 0;

    for (j = 0; j < n-1; j++) {
        switch (dataType) {
          case LATTICE_DATA_INT8:
            dataj = (uint8_t *)data + j*ldA;
            break;

          case LATTICE_DATA_INT16:
            dataj = (short *)data + j*ldA;
            break;

          case LATTICE_DATA_INT32:
            dataj = (int32_t *)data + j*ldA;
            break;

          case LATTICE_DATA_FLOAT32:
            dataj = (float *)data + j*ldA;
            break;

          case LATTICE_DATA_FLOAT64:
            dataj = (double *)data + j*ldA;
            break;
        }

        ys = ystart + j*dy;
        for (i = 0; i < m-1; i++) {
            switch (dataType) {
              case LATTICE_DATA_INT8:
                dataji = (uint8_t *)dataj + i*strideA;
                break;
    
              case LATTICE_DATA_INT16:
                dataji = (short *)dataj + i*strideA;
                break;
    
              case LATTICE_DATA_INT32:
                dataji = (int32_t *)dataj + i*strideA;
                break;
    
              case LATTICE_DATA_FLOAT32:
                dataji = (float *)dataj + i*strideA;
                break;
    
              case LATTICE_DATA_FLOAT64:
                dataji = (double *)dataj + i*strideA;
                break;
            }
            xs = xstart + i*dx;

            switch (dataType) {
              case LATTICE_DATA_INT8:
                ht[1] = (float)*((uint8_t *)dataji);
                ht[2] = (float)*((uint8_t *)dataji + strideA);
                ht[3] = (float)*((uint8_t *)dataji + ldA + strideA);
                ht[4] = (float)*((uint8_t *)dataji + ldA);
                break;
    
              case LATTICE_DATA_INT16:
                ht[1] = (float)*((short *)dataji);
                ht[2] = (float)*((short *)dataji + strideA);
                ht[3] = (float)*((short *)dataji + ldA + strideA);
                ht[4] = (float)*((short *)dataji + ldA);
                break;
    
              case LATTICE_DATA_INT32:
                ht[1] = (float)*((int32_t *)dataji);
                ht[2] = (float)*((int32_t *)dataji + strideA);
                ht[3] = (float)*((int32_t *)dataji + ldA + strideA);
                ht[4] = (float)*((int32_t *)dataji + ldA);
                break;
    
              case LATTICE_DATA_FLOAT32:
                ht[1] = (float)*((float *)dataji);
                ht[2] = (float)*((float *)dataji + strideA);
                ht[3] = (float)*((float *)dataji + ldA + strideA);
                ht[4] = (float)*((float *)dataji + ldA);
                break;
    
              case LATTICE_DATA_FLOAT64:
                ht[1] = (float)*((double *)dataji);
                ht[2] = (float)*((double *)dataji + strideA);
                ht[3] = (float)*((double *)dataji + ldA + strideA);
                ht[4] = (float)*((double *)dataji + ldA);
                break;
            }

            if ((ht[1] < height) &&
                (ht[2] < height) &&
                (ht[3] < height) &&
                (ht[4] < height) ) {
                continue;
            }
            else {
                if ((ht[1] > height) &&
                    (ht[2] > height) &&
                    (ht[3] > height) &&
                    (ht[4] > height) ) {
                    continue;
                }
            }

      
            ht[0] = 0.25 * (ht[1] + ht[2] + ht[3] + ht[4]);
      
            for (iVert = 0; iVert < NUM_VERT; iVert++) {
                locs[iVert][0] = xs + dx * offset[iVert][0];
                locs[iVert][1] = ys + dy * offset[iVert][1];
            }

            for (it = 0; it < NUM_TRIS; it++) {
                swit = 0;
                trisit = tris[it];

                a = ht[aVert = trisit[0]];
                b = ht[bVert = trisit[1]];
                c = ht[cVert = trisit[2]];

                swit = 0;
                swit += (a < height ? 0 : 
                        (a == height ? switBase0 : 2*switBase0 ) );
                swit += (b < height ? 0 : 
                        (b == height ? switBase1 : 2*switBase1 ) );
                swit += (c < height ? 0 : 
                        (c == height ? switBase2 : 2*switBase2 ) );
        
                drawLine = 0;
        
                switch (swit) {
          
                  /* Set (x/y)-begin, (x/y)-end, and increment num segments
                     if a line segment is found. */
          
                  case (0 + 0*3 + 0*9):
                  case (1 + 0*3 + 0*9):
                    break;
                  case (2 + 0*3 + 0*9):
                    LINECASE_ABC
                    break;
                  case (0 + 1*3 + 0*9):
                    break;
                  case (1 + 1*3 + 0*9):
                    LINECASE_CBA
                    break;
                  case (2 + 1*3 + 0*9):
                    LINECASE_ABC
                    break;
                  case (0 + 2*3 + 0*9):
                    LINECASE_BAC
                    break;
                  case (1 + 2*3 + 0*9):
                    LINECASE_BAC
                    break;
                  case (2 + 2*3 + 0*9):
                    LINECASE_CBA
                    break;

                  case (0 + 0*3 + 1*9):
                    break;
                  case (1 + 0*3 + 1*9):
                    LINECASE_BAC
                    break;
                  case (2 + 0*3 + 1*9):
                    LINECASE_ABC
                    break;
                  case (0 + 1*3 + 1*9):
                    LINECASE_ABC
                    break;
                  case (1 + 1*3 + 1*9):
                    break;
                  case (2 + 1*3 + 1*9):
                    LINECASE_ABC
                    break;
                  case (0 + 2*3 + 1*9):
                    LINECASE_BAC
                    break;
                  case (1 + 2*3 + 1*9):
                    LINECASE_BAC
                    break;
                  case (2 + 2*3 + 1*9):
                    break;

                  case (0 + 0*3 + 2*9):
                    LINECASE_CBA
                    break;
                  case (1 + 0*3 + 2*9):
                    LINECASE_BAC
                    break;
                  case (2 + 0*3 + 2*9):
                    LINECASE_BAC
                    break;
                  case (0 + 1*3 + 2*9):
                    LINECASE_ABC
                    break;
                  case (1 + 1*3 + 2*9):
                    LINECASE_CBA
                    break;
                  case (2 + 1*3 + 2*9):
                    break;
                  case (0 + 2*3 + 2*9):
                    LINECASE_ABC
                    break;
                  case (1 + 2*3 + 2*9):
                  case (2 + 2*3 + 2*9):
                    break;
                }
        
                if (drawLine) {
                    *(linePtr++) = xyBegin[0];
                    *(linePtr++) = xyBegin[1];
                    *(linePtr++) = 0.0;
          
                    *(linePtr++) = xyEnd[0];
                    *(linePtr++) = xyEnd[1];
                    *(linePtr++) = 0.0;
          
                    if (++segmentCount >= LINEBUFFER) {
                      displayLines( segmentCount, xyzBuf );
                      linePtr = xyzBuf;
                      segmentCount = 0;
                    }
                }
            }
        }
    }
    if (segmentCount > 0 ) {
        displayLines( segmentCount, xyzBuf );
        linePtr = xyzBuf;
        segmentCount = 0;
    }
}
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void contour( 
        SbBool        iAxis,            /* plot this axis */
        SbBool        jAxis,            /* plot this axis */
        SbBool        kAxis,            /* plot this axis */
        short         nDim,
        const int32_t *dims,
        int32_t       nDataVar,
        short         dataType,
        const void    *dataVals,
        short         coordType,
        const float   *coord,
        int32_t       nLevel,
        const float   *levels,
        SoIndexedLineSet *theLineSet,
        SoVertexProperty *theLineSetVP)
{
    int32_t i, j;
    int ldd, stride;

    int32_t indexOffset;
    int32_t vertexOffset;
    int32_t nInd;
    int32_t *indices;

    const void   *datPtr;

    float dx, dy;
    float xs, ys;

    glob.coordType = coordType;
    glob.coord = coord;
    glob.dims = dims;

    ChemCondenser *condenser;

    /* handle 2D and 3D separately. */
    if (nDim == 2) {
        glob.orientation = orient_xy;
        glob.planeOffset = 0.0;                /* z location is 0.0 */
        switch (coordType) {
          case LATTICE_COORD_UNIFORM:
            if (dims[0] > 1)
              dx = (coord[1] - coord[0])/(dims[0] - 1);
            else
              dx = 1.0;
    
            if (dims[1] > 1)
              dy = (coord[3] - coord[2])/(dims[1] - 1);
            else
              dy = dx;
    
            xs = coord[0];
            ys = coord[2];
            break;

          case LATTICE_COORD_RECTILINEAR:
            dy = dx = 1.0;
            ys = xs = 0.0;
            break;
        }

        for (i=0; i<nLevel; i++) {
            ldd = (int) (dims[0] * nDataVar);
            stride = (int) nDataVar;
            cntrLinear( dataType, dataVals, ldd, stride, 
                        ((int)dims[0]), ((int)dims[1]), 
                        levels[i], dx, dy, xs, ys );
        }
    }
    else if (nDim == 3) {
        /* For each requested axis type, loop over all planes normal to */
        /* that axis and draw their contours. Make one contour call per */
        /* level to get colors right. */

        if (kAxis) {
            currentPoint = currentIndex = 0;
            glob.orientation = orient_xy;

            for (j=0; j<dims[2]; j++) {
                switch (coordType) {
                  case LATTICE_COORD_UNIFORM:
                    if (dims[2] > 1)
                        glob.planeOffset = coord[4] + 
                                           (coord[5]-coord[4])*j/(dims[2]-1);
                    else
                        glob.planeOffset = coord[4];
            
                    if (dims[0] > 1)
                        dx = (coord[1] - coord[0])/(dims[0] - 1);
                    else
                        dx = 1.0;
            
                    if (dims[1] > 1)
                        dy = (coord[3] - coord[2])/(dims[1] - 1);
                    else
                        dy = dx;
            
                    xs = coord[0];
                    ys = coord[2];
                    break;

                  case LATTICE_COORD_RECTILINEAR:
                    glob.planeOffset = coord[dims[0]+dims[1]+j];
                    dy = dx = 1.0;
                    ys = xs = 0.0;
                    break;
                }
          
                for (i=0; i<nLevel; i++) {
                    ldd = (int) (dims[0] * nDataVar);
                    stride = (int) nDataVar;
                    switch (dataType) {
                      case LATTICE_DATA_INT8:
                        datPtr = (uint8_t *)dataVals +
                                 (int)(j * nDataVar * dims[0] * dims[1]);
                        break;

                      case LATTICE_DATA_INT16:
                        datPtr = (short *)dataVals +
                                 (int)(j * nDataVar * dims[0] * dims[1]);
                        break;

                      case LATTICE_DATA_INT32:
                        datPtr = (int32_t *)dataVals +
                                 (int)(j * nDataVar * dims[0] * dims[1]);
                        break;

                      case LATTICE_DATA_FLOAT32:
                        datPtr = (float *)dataVals +
                                 (int)(j * nDataVar * dims[0] * dims[1]);
                        break;

                      case LATTICE_DATA_FLOAT64:
                        datPtr = (double *)dataVals +
                                 (int)(j * nDataVar * dims[0] * dims[1]);
                        break;
                    }
                    cntrLinear( dataType, datPtr, ldd, stride, 
                                ((int)dims[0]), ((int)dims[1]), 
                                levels[i], dx, dy, xs, ys );
                }
            }
            condenser = new ChemCondenser;
            condenser->condense(lineSet, FALSE, FALSE);
            delete condenser;

            vertexOffset = theLineSetVP->vertex.getNum();
            theLineSetVP->vertex.setValues(vertexOffset,
                lineSetVP->vertex.getNum(),
                lineSetVP->vertex.getValues(0));

            indices = lineSet->coordIndex.startEditing();
            nInd = lineSet->coordIndex.getNum();
            for (i = 0; i < nInd; i++) {
               if (indices[i] != -1) {
                   indices[i] += vertexOffset;
               }
            }
            lineSet->coordIndex.finishEditing();

            indexOffset = theLineSet->coordIndex.getNum();
            theLineSet->coordIndex.setValues(indexOffset, 
                lineSet->coordIndex.getNum(),
                lineSet->coordIndex.getValues(0));

            lineSet->coordIndex.deleteValues(0,-1);
            lineSetVP->vertex.deleteValues(0,-1);
        }
      
        if (jAxis) {
            currentPoint = currentIndex = 0;
            glob.orientation = orient_xz;
    
            for (j=0; j<dims[1]; j++) {
                switch (coordType) {
                  case LATTICE_COORD_UNIFORM:
                    if (dims[1] > 1)
                        glob.planeOffset = coord[2] + 
                                           (coord[3]-coord[2])*j/(dims[1]-1);
                    else
                        glob.planeOffset = coord[2];
                
                    if (dims[0] > 1)
                        dx = (coord[1] - coord[0])/(dims[0] - 1);
                    else
                        dx = 1.0;
                  
                    if (dims[2] > 1)
                        dy = (coord[5] - coord[4])/(dims[2] - 1);
                    else
                        dy = dx;
                  
                    xs = coord[0];
                    ys = coord[4];
                    break;

                  case LATTICE_COORD_RECTILINEAR:
                    glob.planeOffset = coord[dims[0]+j];
                    dy = dx = 1.0;
                    ys = xs = 0.0;
                    break;
                }

            
                for (i=0; i<nLevel; i++) {
                    ldd = (int) ((dims[0] * dims[1]) * nDataVar);
                    stride = (int) nDataVar;
                    switch (dataType) {
                      case LATTICE_DATA_INT8:
                        datPtr = (uint8_t *)dataVals +
                                 (int)(j * nDataVar * dims[0]);
                        break;

                      case LATTICE_DATA_INT16:
                        datPtr = (short *)dataVals +
                                 (int)(j * nDataVar * dims[0]);
                        break;

                      case LATTICE_DATA_INT32:
                        datPtr = (int32_t *)dataVals +
                                 (int)(j * nDataVar * dims[0]);
                        break;

                      case LATTICE_DATA_FLOAT32:
                        datPtr = (float *)dataVals +
                                 (int)(j * nDataVar * dims[0]);
                        break;

                      case LATTICE_DATA_FLOAT64:
                        datPtr = (double *)dataVals +
                                 (int)(j * nDataVar * dims[0]);
                        break;
                    }
                    cntrLinear( dataType, datPtr, ldd, stride, 
                                ((int)dims[0]), ((int)dims[2]), 
                                levels[i], dx, dy, xs, ys );
                }
            }
            condenser = new ChemCondenser;
            condenser->condense(lineSet, FALSE, FALSE);
            delete condenser;

            vertexOffset = theLineSetVP->vertex.getNum();
            theLineSetVP->vertex.setValues(vertexOffset,
                lineSetVP->vertex.getNum(),
                lineSetVP->vertex.getValues(0));

            indices = lineSet->coordIndex.startEditing();
            nInd = lineSet->coordIndex.getNum();
            for (i = 0; i < nInd; i++) {
               if (indices[i] != -1) {
                   indices[i] += vertexOffset;
               }
            }
            lineSet->coordIndex.finishEditing();

            indexOffset = theLineSet->coordIndex.getNum();
            theLineSet->coordIndex.setValues(indexOffset, 
                lineSet->coordIndex.getNum(),
                lineSet->coordIndex.getValues(0));

            lineSet->coordIndex.deleteValues(0,-1);
            lineSetVP->vertex.deleteValues(0,-1);
        }
      
        if (iAxis) {
            currentPoint = currentIndex = 0;
            glob.orientation = orient_yz;
    
            for (j=0; j<dims[0]; j++) {
                switch (coordType) {
                  case LATTICE_COORD_UNIFORM:
                    if (dims[0] > 1)
                        glob.planeOffset = coord[0] + 
                                           (coord[1]-coord[0])*j/(dims[0]-1);
                    else
                        glob.planeOffset = coord[0];
                
                    if (dims[1] > 1)
                        dx = (coord[3] - coord[2])/(dims[1] - 1);
                    else
                        dx = 1.0;
                
                    if (dims[2] > 1)
                        dy = (coord[5] - coord[4])/(dims[2] - 1);
                    else
                        dy = dx;
                
                    xs = coord[2];
                    ys = coord[4];
                    break;

                  case LATTICE_COORD_RECTILINEAR:
                    glob.planeOffset = coord[j];
                    dy = dx = 1.0;
                    ys = xs = 0.0;
                    break;
                }

              
                for (i=0; i<nLevel; i++) {
                    ldd = (int) ((dims[0] * dims[1]) * nDataVar);
                    stride = (int) (nDataVar * dims[0]);
                    switch (dataType) {
                      case LATTICE_DATA_INT8:
                        datPtr = (uint8_t *)dataVals +
                                 (int)(j * nDataVar);
                        break;

                      case LATTICE_DATA_INT16:
                        datPtr = (short *)dataVals +
                                 (int)(j * nDataVar);
                        break;

                      case LATTICE_DATA_INT32:
                        datPtr = (int32_t *)dataVals +
                                 (int)(j * nDataVar);
                        break;

                      case LATTICE_DATA_FLOAT32:
                        datPtr = (float *)dataVals +
                                 (int)(j * nDataVar);
                        break;

                      case LATTICE_DATA_FLOAT64:
                        datPtr = (double *)dataVals +
                                 (int)(j * nDataVar);
                        break;
                    }
                    cntrLinear( dataType, datPtr, ldd, stride, 
                                ((int)dims[1]), ((int)dims[2]), 
                                levels[i], dx, dy, xs, ys );
              
                }
            }
            condenser = new ChemCondenser;
            condenser->condense(lineSet, FALSE, FALSE);
            delete condenser;

            vertexOffset = theLineSetVP->vertex.getNum();
            theLineSetVP->vertex.setValues(vertexOffset,
                lineSetVP->vertex.getNum(),
                lineSetVP->vertex.getValues(0));

            indices = lineSet->coordIndex.startEditing();
            nInd = lineSet->coordIndex.getNum();
            for (i = 0; i < nInd; i++) {
               if (indices[i] != -1) {
                   indices[i] += vertexOffset;
               }
            }
            lineSet->coordIndex.finishEditing();

            indexOffset = theLineSet->coordIndex.getNum();
            theLineSet->coordIndex.setValues(indexOffset, 
                lineSet->coordIndex.getNum(),
                lineSet->coordIndex.getValues(0));

            lineSet->coordIndex.deleteValues(0,-1);
            lineSetVP->vertex.deleteValues(0,-1);
        }
    }
}
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void contourLibContour( 
                 SbBool  iAxis,           /* plot this axis */
                 SbBool  jAxis,           /* plot this axis */
                 SbBool  kAxis,           /* plot this axis */
                 short   dataVar,
                 ChemLattice3 *theLat,
                 short   nLevel,
                 const float   *levels,
                 const uint32_t *colors,
                 SoIndexedLineSet *theLineSet,
                 SoVertexProperty *theLineSetVP )
{
    int   i;
    int nDim = 3;

    int32_t nDataVar = theLat->nDataVar.getValue();
    short dataType = theLat->getDataType();
    short coordType = theLat->getCoordType();
    const float *coord = theLat->coord.getValues(0);
    const void *dataVals;

    int dims[3];
    SbVec3i vdim = theLat->dimension.getValue();
    dims[0] = vdim[0];
    dims[1] = vdim[1];
    dims[2] = vdim[2];

    switch (dataType) {
      case LATTICE_DATA_INT8:
        dataVals = ((MFInt8 *)(theLat->data))->getValues(0);
        dataVals = (uint8_t *)dataVals + dataVar;
        break;

      case LATTICE_DATA_INT16:
        dataVals = ((SoMFShort *)(theLat->data))->getValues(0);
        dataVals = (short *)dataVals + dataVar;
        break;

      case LATTICE_DATA_INT32:
        dataVals = ((SoMFInt32 *)(theLat->data))->getValues(0);
        dataVals = (int32_t *)dataVals + dataVar;
        break;

      case LATTICE_DATA_FLOAT32:
        dataVals = ((SoMFFloat *)(theLat->data))->getValues(0);
        dataVals = (float *)dataVals + dataVar;
        break;

      case LATTICE_DATA_FLOAT64:
        dataVals = ((MFDouble *)(theLat->data))->getValues(0);
        dataVals = (double *)dataVals + dataVar;
        break;
    }

    // Nullify color
    glob.colorLat = NULL;

    currentPoint = currentIndex = 0;
    lineSet = new SoIndexedLineSet;
    lineSet->ref();
    lineSetVP = new SoVertexProperty;
    lineSetVP->ref();
    lineSet->vertexProperty = lineSetVP;

    lineSet->coordIndex.deleteValues(0,-1);
    lineSetVP->vertex.deleteValues(0,-1);

    if (colors != NULL) {
        int32_t lastPolyLineCount = 0;
        theLineSetVP->materialBinding = SoMaterialBinding::PER_FACE_INDEXED;
        for (int theLevel = 0; theLevel < nLevel; theLevel++) {
            contour( iAxis, jAxis, kAxis, 
                     nDim, dims, nDataVar, dataType, dataVals, coordType, coord,
                     1, &levels[theLevel], theLineSet, theLineSetVP);
    
            // Count the number of polyLines
            int32_t numPolyLines = 0;
            int32_t end = theLineSet->coordIndex.getNum();
            const int32_t *coordIndex = theLineSet->coordIndex.getValues(0);
            for (i = 0; i < end; i++) {
               if (coordIndex[i] == SO_END_LINE_INDEX || (i == (end - 1))) {
                   numPolyLines++;
               }
            }
            // Add the color for this level numPolyLines-lastPolyLineCount times
            theLineSetVP->orderedRGBA.set1Value(theLevel, colors[theLevel]);
            theLineSet->materialIndex.setNum(numPolyLines);
            int32_t *materialIndex = theLineSet->materialIndex.startEditing();
            for (i = lastPolyLineCount; i < numPolyLines; i++) {
               materialIndex[i] = theLevel;
            }
            theLineSet->materialIndex.finishEditing();
            lastPolyLineCount += (numPolyLines - lastPolyLineCount);
        }
        if (theLineSet->materialIndex.getNum() == 0) {
            theLineSet->materialIndex = -1;
            theLineSetVP->materialBinding = SoMaterialBinding::OVERALL;
        }
    }
    else {
        contour( iAxis, jAxis, kAxis, 
                 nDim, dims, nDataVar, dataType, dataVals, coordType, coord,
                 nLevel, levels, theLineSet, theLineSetVP);
        theLineSetVP->materialBinding = SoMaterialBinding::OVERALL;
    }

    lineSet->unref();
    lineSetVP->unref();

    short condenseTest = 0;
    if (iAxis) condenseTest++;
    if (jAxis) condenseTest++;
    if (kAxis) condenseTest++;
    if (condenseTest > 1) {
       ChemCondenser condenser;
       condenser.condenseCoordinates(theLineSet);
    }
  
    theLineSetVP->normalBinding = SoNormalBinding::OVERALL;

    return;
}

void contourLibContour2( 
                 SbBool  iAxis,           /* plot this axis */
                 SbBool  jAxis,           /* plot this axis */
                 SbBool  kAxis,           /* plot this axis */
                 float   threshold,
                 short   dataVar,
                 short   colorVar,
                 ChemLattice3 *dataLat,
                 ChemLattice3 *colorLat,
                 int32_t numColors,
                 float minValue,
                 float maxValue,
                 const uint32_t *colorMap,
                 SoIndexedLineSet *theLineSet,
                 SoVertexProperty *theLineSetVP )
{
    int   i, j;
    int nDim = 3;

    int32_t nDataVar = dataLat->nDataVar.getValue();
    short dataType = dataLat->getDataType();
    short coordType = dataLat->getCoordType();
    const float *coord = dataLat->coord.getValues(0);
    const void *dataVals;

    int dims[3];
    SbVec3i vdim = dataLat->dimension.getValue();
    dims[0] = vdim[0];
    dims[1] = vdim[1];
    dims[2] = vdim[2];

    switch (dataType) {
      case LATTICE_DATA_INT8:
        dataVals = ((MFInt8 *)(dataLat->data))->getValues(0);
        dataVals = (uint8_t *)dataVals + dataVar;
        break;

      case LATTICE_DATA_INT16:
        dataVals = ((SoMFShort *)(dataLat->data))->getValues(0);
        dataVals = (short *)dataVals + dataVar;
        break;

      case LATTICE_DATA_INT32:
        dataVals = ((SoMFInt32 *)(dataLat->data))->getValues(0);
        dataVals = (int32_t *)dataVals + dataVar;
        break;

      case LATTICE_DATA_FLOAT32:
        dataVals = ((SoMFFloat *)(dataLat->data))->getValues(0);
        dataVals = (float *)dataVals + dataVar;
        break;

      case LATTICE_DATA_FLOAT64:
        dataVals = ((MFDouble *)(dataLat->data))->getValues(0);
        dataVals = (double *)dataVals + dataVar;
        break;
    }

    // Add color stuff
    glob.colorVar = colorVar;
    glob.colorLat = colorLat;
    glob.numColors = numColors;
    glob.minValue = minValue;
    glob.maxValue = maxValue;
    glob.colorMap = colorMap;

    if (colorLat != NULL) {
        switch (colorLat->getDataType()) {
          case LATTICE_DATA_INT8:
            glob.colorValues = ((MFInt8 *)(colorLat->data))->getValues(0);
            break;
    
          case LATTICE_DATA_INT16:
            glob.colorValues = ((SoMFShort *)(colorLat->data))->getValues(0);
            break;
    
          case LATTICE_DATA_INT32:
            glob.colorValues = ((SoMFInt32 *)(colorLat->data))->getValues(0);
            break;
    
          case LATTICE_DATA_FLOAT32:
            glob.colorValues = ((SoMFFloat *)(colorLat->data))->getValues(0);
            break;
    
          case LATTICE_DATA_FLOAT64:
            glob.colorValues = ((MFDouble *)(colorLat->data))->getValues(0);
            break;

          case LATTICE_DATA_BOOL:
          case LATTICE_DATA_BAD_TYPE:
            break;
        }
        SbVec3i colorDims = colorLat->dimension.getValue();
            
        for (i = 0; i < 8; i++) {
            int32_t offset = 0;
            int32_t stride = 1;
            for (j = 0; j < 3; j++) {
                if (i & (1 << j)) {
                    offset += stride;  
                }
                stride *= colorDims[j];
            }
            glob.colorDataOffset[i] = offset;
        }
    }

    currentPoint = currentIndex = 0;
    lineSet = new SoIndexedLineSet;
    lineSet->ref();
    lineSetVP = new SoVertexProperty;
    lineSetVP->ref();
    lineSet->vertexProperty = lineSetVP;

    lineSet->coordIndex.deleteValues(0,-1);
    lineSetVP->vertex.deleteValues(0,-1);

    contour( iAxis, jAxis, kAxis, 
             nDim, dims, nDataVar, dataType, dataVals, coordType, coord,
             1, &threshold, theLineSet, theLineSetVP);

    lineSet->unref();
    lineSetVP->unref();

    short condenseTest = 0;
    if (iAxis) condenseTest++;
    if (jAxis) condenseTest++;
    if (kAxis) condenseTest++;
    if (condenseTest > 1) {
       ChemCondenser condenser;
       condenser.condenseCoordinates(theLineSet);
    }

    theLineSetVP->normalBinding = SoNormalBinding::OVERALL;
    theLineSet->materialIndex = -1;
    theLineSetVP->materialBinding = SoMaterialBinding::PER_VERTEX_INDEXED;

    if (glob.colorLat != NULL) {
        theLineSetVP->materialBinding = SoMaterialBinding::PER_VERTEX_INDEXED;
        addColor(theLineSetVP);
    }
    else {
        theLineSetVP->materialBinding = SoMaterialBinding::OVERALL;
    }

    return;
}
