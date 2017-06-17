/*
 * ChemSelection.c++
 *
 *     Classes:  ChemSelection
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

#ident "$Revision: 22177 $"

#include <assert.h>
#include <math.h>

/*
	Porting to Windows:
	Jamie Doornbos	2001/07/15
	Daniel Soto	2002/02/05
*/
#ifndef WIN32
#include <X11/Xlib.h>
#endif

#ifdef __sgi
#include <X11/extensions/SGIStereo.h>
#endif

#include "inv/SoDB.H"
#include "inv/SoInput.H"
#include "inv/SoInteraction.H"
#include "inv/SoOutput.H"
#include "inv/SoPickedPoint.H"

#include "inv/actions/SoGetBoundingBoxAction.H"
#include "inv/actions/SoGetMatrixAction.H"
#include "inv/actions/SoGLRenderAction.H"
#include "inv/actions/SoHandleEventAction.H"
#include "inv/actions/SoSearchAction.H"
#include "inv/actions/SoWriteAction.H"
#include "inv/actions/SoPickAction.H"

#include "inv/details/SoTextDetail.H"

#include "inv/elements/SoModelMatrixElement.H"
#include "inv/elements/SoProjectionMatrixElement.H"
#include "inv/elements/SoViewingMatrixElement.H"
#include "inv/elements/SoViewportRegionElement.H"
#include "inv/elements/SoViewVolumeElement.H"

#include "inv/errors/SoDebugError.H"

#include "inv/events/SoEvent.H"
#include "inv/events/SoKeyboardEvent.H"
#include "inv/events/SoLocation2Event.H"
#include "inv/events/SoMouseButtonEvent.H"

#include "inv/fields/SoFieldData.H"

#include "inv/misc/SoCallbackList.H"

#include "inv/nodes/SoCallback.H"
#include "inv/nodes/SoComplexity.H"
#include "inv/nodes/SoCoordinate3.H"
#include "inv/nodes/SoColorIndex.H"
#include "inv/nodes/SoDrawStyle.H"
#include "inv/nodes/SoFont.H"
#include "inv/nodes/SoLightModel.H"
#include "inv/nodes/SoLineSet.H"
#include "inv/nodes/SoMaterial.H"
#include "inv/nodes/SoPickStyle.H"
#include "inv/nodes/SoSphere.H"
#include "inv/nodes/SoSubNode.H"
#include "inv/nodes/SoSwitch.H"
#include "inv/nodes/SoText2.H"
#include "inv/nodes/SoTransform.H"

#include "inv/sensors/SoFieldSensor.H"

/*
	Porting to Windows:
	Jamie Doornbos	2001/07/15
	Daniel Soto	2002/02/05
*/
#include "inv/ChemKit/FixFonts.H"

#include "inv/ChemKit/SFVec2i.H"
#include "inv/ChemKit/MFVec2i.H"
#include "inv/ChemKit/ChemAtomBBox.H"
#include "inv/ChemKit/ChemBaseData.H"
#include "inv/ChemKit/ChemBBoxAction.H"
#include "inv/ChemKit/ChemBondBBox.H"
#include "inv/ChemKit/ChemDetail.H"
// --> lod
#include "inv/ChemKit/ChemLOD.H"
// <-- lod
#include "inv/ChemKit/ChemDisplay.H"
#include "inv/ChemKit/ChemDisplayParam.H"
#include "inv/ChemKit/ChemDisplayPath.H"
#include "inv/ChemKit/ChemDisplayPathList.H"
#include "inv/ChemKit/ChemDisplaySelectionElement.H"
#include "inv/ChemKit/ChemLabel.H"
#include "inv/ChemKit/ChemLabelBBox.H"
#include "inv/ChemKit/ChemLabelPath.H"
#include "inv/ChemKit/ChemLabelSelectionElement.H"
#include "inv/ChemKit/ChemMonitor.H"
#include "inv/ChemKit/ChemMonitorBBox.H"
#include "inv/ChemKit/ChemMonitorDetail.H"
#include "inv/ChemKit/ChemMonitorPath.H"
#include "inv/ChemKit/ChemMonitorSelectionElement.H"
#include "inv/ChemKit/ChemSelection.H"
#include "inv/ChemKit/ChemSelectionElement.H"
#include "inv/ChemKit/ChemStickBondBBox.H"
#include "inv/ChemKit/ChemWireframeBondBBox.H"

// --> residue atoms selection
#include "inv/ChemKit/ChemData.H"
#include "inv/ChemKit/ChemBaseDataElement.H"
#include "inv/ChemKit/ChemDisplayParamElement.H"
// <-- residue atoms selection

//#define TIMEIT
#ifdef TIMEIT
SbTime now = 0.0;
#define RESET_TIMER()   now = SbTime::getTimeOfDay()
#define MARK_TIME(MSG) \
{                               \
    if (now == 0.0) RESET_TIMER();      \
    SbTime tmp = SbTime::getTimeOfDay() - now;   \
    fprintf(stderr, "%f : %s\n", tmp.getValue(), MSG); \
    RESET_TIMER(); \
}
#define LAP_TIME(MSG) \
{                               \
    if (now == 0.0) RESET_TIMER();      \
    SbTime tmp = SbTime::getTimeOfDay() - now;   \
    fprintf(stderr, "%f : \t%s\n", tmp.getValue(), MSG); \
}
#else
#define RESET_TIMER()
#define MARK_TIME(X)
#define LAP_TIME(X)
#endif

#define SELECT(PARTS, PART)   ((PARTS & (PART)) != 0)
 
SO_NODE_SOURCE(ChemSelection);

SoSearchAction *ChemSelection::searchAction = NULL;
ChemBBoxAction *ChemSelection::chemBBoxAction = NULL;

#define RINGSIDES 16
SbVec2f ChemSelection::ringCoords[RINGSIDES];

////////////////////////////////////////////////////////////////////////
// 
// Description:
//    Returns the square of the distance between 2 points.  This
//    is so we can avoid the square root you have to do when the
//    SbVec3f length method is used.
// 
// Use: internal, static

static float
distSquared(const SbVec3f &v1, const SbVec3f &v2)
// 
////////////////////////////////////////////////////////////////////////
{
    SbVec3f tvec = v1 - v2;
    return (tvec[0]*tvec[0] + tvec[1]*tvec[1] + tvec[2]*tvec[2]);
}

static float
distSquared2D(const SbVec3f &v1, const SbVec3f &v2)
// 
////////////////////////////////////////////////////////////////////////
{
    SbVec3f tvec = v1 - v2;
    return (tvec[0]*tvec[0] + tvec[1]*tvec[1]);
}

////////////////////////////////////////////////////////////////////////
// 
// Description:
//    Static helper routine; given a vector in object space and a
//    transformation matrix to screen (normalized-device coordinate)
//    space returns the 2D coordinates of the vector.  This
//    actually returns a 3D vector; the z value is just the NDC z value.
// 
// Use: internal, static

static SbVec3f
fromObjectSpace(const SbVec3f &vector, const SbMatrix &matrix)
//
////////////////////////////////////////////////////////////////////////
{
    // First, transform to NDC (-1 to 1)
    SbVec3f ndc;
    matrix.multVecMatrix(vector, ndc);
    
    // And do the viewport transformation:
    SbVec3f result;
    result[0] = (ndc[0]+1.0) * 0.5;
    result[1] = (ndc[1]+1.0) * 0.5;
    // Leave the z coordinate alone
    result[2] = ndc[2];

    return result;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Static helper routine; given a vector in object space, a
//    transformation matrix to screen (normalized-device coordinate)
//    space, and a viewportRegion, returns the 2D pixel coordinates of
//    the vector (relative to the origin of the viewport).  This
//    actually returns a 3D vector; the z value is just the NDC z value.
//
// Use: internal, static
 
static SbVec3f
fromObjectSpaceToPixels(const SbVec3f &vector, const SbMatrix &matrix,
                const SbViewportRegion &vpr)
//
////////////////////////////////////////////////////////////////////////
{           
    // First, transform to NDC (-1 to 1 in viewport)
    SbVec3f ndc;
    matrix.multVecMatrix(vector, ndc);

    // And do the viewport transformation:
    SbVec2s vpSize = vpr.getViewportSizePixels();
    SbVec3f result;
    result[0] = (ndc[0]+1.0)*vpSize[0]/2.0;
    result[1] = (ndc[1]+1.0)*vpSize[1]/2.0;
    // Leave the z coordinate alone
    result[2] = ndc[2];

    return result;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Calculate the rotation portion of the direction in which to translate
//    the label used in the sphere selection.
//
// Use: internal, static

static SbRotation
calculateRotation(SbMatrix modelMatrix, SbViewVolume viewVolume,
    SbMatrix viewMatrix)
//
////////////////////////////////////////////////////////////////////////
{
    float angle;
    SbVec3f axis, normal;
    axis.setValue(0.0, 1.0, 0.0);
    normal.setValue(0.0, 0.0, 1.0);
  
    SbVec3f lineOfSight = viewVolume.getProjectionDirection();

    SbVec3f  translate, scale;
    SbRotation  rotation, scaleOrientation;
    modelMatrix.getTransform(translate, rotation, scale, scaleOrientation);
    SbRotation answer = rotation.inverse();
 
 
    // rotate the normal to point in the direction opposite to LOS
    SbRotation      orientation;
    orientation.setValue(normal.getValue(), -lineOfSight);
            
    // calculate the new position of the up vector
    SbVec3f newUp;
    orientation.multVec(axis.getValue(), newUp);
        
    // This is what the up vector should be lined up with
    // It becomes +Y after the viewing xform
    //    (0, 1, 0) = desiredUp * viewMatrix
    //    desiredUp = (0, 1, 0) * (viewMatrix)^-1
    SbVec3f desiredUp;

    // we know that viewMatrix is orthonormal
    desiredUp[0] = viewMatrix[0][1];
    desiredUp[1] = viewMatrix[1][1];
    desiredUp[2] = viewMatrix[2][1];
 
    // calculate the angle between newUp and desiredUp
    desiredUp.normalize();
    newUp.normalize();
    angle = acos(newUp.dot(desiredUp));
    if ((newUp.cross(desiredUp)).dot(lineOfSight) < 0.0)
        angle = -angle;

    // rotate about lineOfSight to line newUp with desiredUp
    SbRotation      twist;
    twist.setValue(lineOfSight, angle);

    return (orientation * twist * answer);
}

#define X 0
#define Y 1
#define Z 2

////////////////////////////////////////////////////////////////////////
//
// Description:
//    From GGemsIV, ptinpoly.c
//
//    "Shoot a test ray along +X axis.  The strategy, from MacMartin, is to
//    compare vertex Y values to the testing point's Y and quickly discard
//    edges which are entirely to one side of the test ray."
//
// Use: internal, static

static SbBool
pointInLasso(const short &lassoType, 
    const SbVec3f &lassoMin, const SbVec3f &lassoMax,
    const SbVec3f *vertices, const int32_t numVertices, 
    const SbVec3f &thePoint)
//
////////////////////////////////////////////////////////////////////////
{
    int32_t j, yflag0, yflag1, inside_flag, xflag0;
    SbVec3f theVertex0, theVertex1;
    int32_t vCount = 0;

    if ((thePoint[X] < lassoMin[X]) || (thePoint[X] > lassoMax[X]))
        return FALSE;
    if ((thePoint[Y] < lassoMin[Y]) || (thePoint[Y] > lassoMax[Y]))
        return FALSE;

    // The lassoBBox is the same as the DRAGGER.  Therefore, there is no
    // need for further checks if we are using a DRAGGER.
    if (lassoType == ChemSelection::DRAGGER) {
        return TRUE;
    }

    theVertex0 = vertices[numVertices-1];
    theVertex1 = vertices[vCount++];
    yflag0 = (theVertex0[Y] >= thePoint[Y]);
    inside_flag = 0;

    for (j = numVertices+1; --j; ) {
        yflag1 = (theVertex1[Y] >= thePoint[Y]);

        // "check if endpoints straddle (are on opposite sides) of X axis
        // (i.e. the Y's differ); if so, +X ray could intersect this edge."

        if (yflag0 != yflag1) {
            xflag0 = (theVertex0[X] >= thePoint[X]);

            // "check if endpoints are on same side of the Y axis (i.e. X's
            // are the same); if so, it's easy to test if edge hits or misses."

            if (xflag0 == (theVertex1[X] >= thePoint[X])) {
                // if edge's X values both right of the point, must hit"
                if ( xflag0 ) inside_flag = !inside_flag ;
            }
            else {
                // "compute intersection of pgon segment with +X ray, note
                //  if >= point's X; if so, the ray hits it."
                if ( (theVertex1[X] - (theVertex1[Y]-thePoint[Y])*
                     ( theVertex0[X]-theVertex1[X])/
                       (theVertex0[Y]-theVertex1[Y])) >= thePoint[X] ) {
                    inside_flag = !inside_flag ;
                }
            }
        }

        // "move to next pair of vertices, retaining info as possible"
        yflag0 = yflag1;
        theVertex0 = theVertex1;
        vCount++;
        // The following check is mostly so Purify won't complain.
        if (vCount == numVertices) break;
        theVertex1 = vertices[vCount];
    }
    return inside_flag;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    From GGemsIII, insectc.c
//
//    "Faster Line Segment Intersection" by Franklin Antonio.
//
// Use: internal, static

static SbBool
lineIntersectsLasso(const SbVec3f *vertices,
    const int32_t numVertices, const SbVec3f &point1, const SbVec3f &point2)
//
////////////////////////////////////////////////////////////////////////
{
    float Ax, Bx, Cx;
    float Ay, By, Cy;
    float d, e, f;
    float x1Lo, x1Hi, y1Lo, y1Hi;

    Ax = point2[X] - point1[X];
    Ay = point2[Y] - point1[Y];
    
    // For X bounding box check
    if (Ax < 0.0) {
        x1Lo = point2[X];
        x1Hi = point1[X];
    }
    else {
        x1Hi = point2[X];
        x1Lo = point1[X];
    }
    
    // For Y bounding box check
    if (Ay < 0.0) {
        y1Lo = point2[Y];
        y1Hi = point1[Y];
    }
    else {
        y1Hi = point2[Y];
        y1Lo = point1[Y];
    }

    SbVec3f theVertex0, theVertex1;
    int32_t vCount = 0;
    int32_t j;
    theVertex0 = vertices[numVertices-1];
    theVertex1 = vertices[vCount++];

    // Start loop over the lasso lines using the algorithm from GGemsIII.
    for (j = numVertices+1; --j; ) {
        Bx = theVertex0[X] - theVertex1[X];

        if (Bx > 0.0) {
            if (x1Hi < theVertex1[X] || theVertex0[X] < x1Lo) {
                theVertex0 = theVertex1;
                theVertex1 = vertices[vCount++];
                continue;
            }
        }
        else {
            if (x1Hi < theVertex0[X] || theVertex1[X] < x1Lo) {
                theVertex0 = theVertex1;
                theVertex1 = vertices[vCount++];
                continue;
            }
        }
    
        By = theVertex0[Y] - theVertex1[Y];
    
        if (By > 0.0) {
            if (y1Hi < theVertex1[Y] || theVertex0[Y] < y1Lo) {
                theVertex0 = theVertex1;
                theVertex1 = vertices[vCount++];
                continue;
            }
        }
        else {
            if (y1Hi < theVertex0[Y] || theVertex1[Y] < y1Lo) {
                theVertex0 = theVertex1;
                theVertex1 = vertices[vCount++];
                continue;
            }
        }
    
        Cx = point1[X] - theVertex0[X];
        Cy = point1[Y] - theVertex0[Y];
        d  = By*Cx - Bx*Cy;        // Numerator for alpha
        f  = Ay*Bx - Ax*By;        // Denominator for both alpha and beta
    
        // Tests on alpha
        if ( f > 0.0) {
            if (d < 0.0 || d > f) {
                theVertex0 = theVertex1;
                theVertex1 = vertices[vCount++];
                continue;
            }
        }
        else {
            if (d > 0.0 || d < f) {
                theVertex0 = theVertex1;
                theVertex1 = vertices[vCount++];
                continue;
            }
        }
    
        e = Ax*Cy - Ay*Cx;         // Numerator for beta;
    
        // Tests on beta
        if (f > 0.0) {
            if (e < 0.0 || e > f) {
                theVertex0 = theVertex1;
                theVertex1 = vertices[vCount++];
                continue;
            }
        }
        else {
            if (e > 0.0 || e < f) {
                theVertex0 = theVertex1;
                theVertex1 = vertices[vCount++];
                continue;
            }
        }
    
        // At this point the lines either intersect or are collinear
        return TRUE;
    }
    return FALSE;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    See if the lasso intersects the given circle.  This is from
//    "A Programmer's Geometry" by Adrian Bowyer and John Woodwark, p. 26
//
// Use: internal, static

static SbBool
lassoIntersectsCircle(const SbVec3f *vertices, const int32_t numVertices, 
    const SbVec3f &circleCenter, float circleRadius)
//
////////////////////////////////////////////////////////////////////////
{
    SbVec3f theVertex0, theVertex1;
    SbVec3f gVec;
    SbVec3f theDirection;
    int32_t j;
    int32_t vcount = 0;
    float circleRadiusSq = circleRadius * circleRadius;

    theVertex0 = vertices[numVertices - 1];
    theVertex1 = vertices[vcount++];

    float a, b, c, d, t1, t2, theLength;
    for (j = numVertices+1; --j; ) {
        theDirection = theVertex1 - theVertex0;
        theDirection.normalize();
        gVec = theVertex0 - circleCenter;
        a = theDirection.dot(theDirection);
        b = 2.0 * theDirection.dot(gVec);
        c = (gVec.dot(gVec)) - circleRadiusSq;
        d = b*b - 4.0*a*c;
        if (d >= 0.0) {
            d = sqrt(d);
            a = 1.0 / (2.0*a);
            // t1 and t2 are the parameters which give the points of
            // intersection on the v0-v1 line.
            t1 = (-b + d) * a;
            t2 = (-b - d) * a;
            // Check to see if the points specified by the parameters
            // are between theVertex0 and theVertex1.
            theLength = distSquared2D(theVertex0, theVertex1);
            if (t1 > 0.0 && (t1*t1) <= theLength) return TRUE;
            if (t2 > 0.0 && (t2*t2) <= theLength) return TRUE;
        }

        theVertex0 = theVertex1;
        theVertex1 = vertices[vcount++];
    }

    // Since there is no intersection, check to see if all of the points
    // of the lasso are within the radius of the circle.
    
    SbBool ret = TRUE;
    for (j = 0; j < numVertices; j++) {
        if (distSquared2D(vertices[j], circleCenter) > circleRadiusSq) {
            ret = FALSE;
            break;
        }
    }
    return ret;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    From GGemsI, p. 335
//    See if box and sphere intersect
//
// Use: internal, static

static SbBool
boxSphereIntersect(const SbBox3f &bbox, 
    const SbMatrix &modelMatrix,
    const SbVec3f &center, const float &radius)
//
////////////////////////////////////////////////////////////////////////
{
    float distMin = 0.0;
    SbVec3f bMin = bbox.getMin();
    SbVec3f bMax = bbox.getMax();

    modelMatrix.multVecMatrix(bMin, bMin);
    modelMatrix.multVecMatrix(bMax, bMax);
    for (int i = 0; i < 3; i++) {
        if (center[i] < bMin[i]) {
            distMin += (center[i] - bMin[i])*(center[i] - bMin[i]);
        }
        else {
            if (center[i] > bMax[i]) {
                distMin += (center[i] - bMax[i])*(center[i] - bMax[i]);
            }
        }
    }
    if (distMin < (radius*radius)) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Check to see if the lasso goes through the center of any label bboxes.
//    This is used for both atom and bond labels.
//
// Use: static, internal

static void
checkLassoCenter(
    const float orthoCamXres, const float orthoCamYres,
    const SbMatrix &curMVP,
    const int32_t numberOfLabels,
    const int32_t *index,
    const SbVec3f *center,
    const short theLassoType, const SbBox3f &lassoBBox,
    const int32_t numLassoVertices, 
    const SbVec3f *lassoVertices, int32_t &lassoedIncr, 
    SbIntList &lassoedList)
//
////////////////////////////////////////////////////////////////////////
{
    int32_t loop;
    SbVec3f tmpCoord;

    const SbVec3f &lassoMin = lassoBBox.getMin();
    const SbVec3f &lassoMax = lassoBBox.getMax();

    for (loop = 0; loop < numberOfLabels; loop++) {
        tmpCoord = fromObjectSpace(center[loop], curMVP);
        tmpCoord[0] *= orthoCamXres;
        tmpCoord[1] *= orthoCamYres;
        if (pointInLasso(theLassoType, lassoMin, lassoMax,
                         lassoVertices, numLassoVertices, tmpCoord)) {
// --> EGB && SGB lasso selection error
//        lassoedList[lassoedIncr++] = index[loop];
					lassoedIncr++;
          lassoedList.append(index[loop]);
// <-- EGB && SGB
        }
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Check to see if the lasso goes through any label bboxes.
//    This is used for both atom and bond labels.
//    We want to make a rectangle in lasso space using the bbox of
//    the label.  This rectangle should face toward the camera.
//
// Use: static, internal

static void
checkLassoThruBBox(
    const float orthoCamXres, const float orthoCamYres,
    const SbMatrix &curMVP,
    const int32_t numberOfLabels,
    const int32_t *index,
    const SbBox3f *bbox,
    const SbVec3f *center,
    const short theLassoType, const SbBox3f &lassoBBox,
    const int32_t numLassoVertices, 
    const SbVec3f *lassoVertices, int32_t &lassoedIncr, 
    SbIntList &lassoedList)
//
////////////////////////////////////////////////////////////////////////
{
    int32_t loop;
    //SbVec3f minCoord, maxCoord, edge;
    SbVec3f minCoord, maxCoord;
    SbVec3f pt0, pt1, pt2, pt3;

    const SbVec3f &lassoMin = lassoBBox.getMin();
    const SbVec3f &lassoMax = lassoBBox.getMax();

    for (loop = 0; loop < numberOfLabels; loop++) {
        // Get the min and max and set their Z component to be the same as
        // the center.
        minCoord = bbox[loop].getMin();
        maxCoord = bbox[loop].getMax();

        // Put min and max into screen space
        pt0 = fromObjectSpace(minCoord, curMVP);
        pt2 = fromObjectSpace(maxCoord, curMVP);
        pt0[X] *= orthoCamXres;
        pt0[Y] *= orthoCamYres;
        pt2[X] *= orthoCamXres;
        pt2[Y] *= orthoCamYres;

        // See if this box intersects the lassoBBox.  If they don't intersect
        // then don't process this label.
        if (!((lassoMax[X] >= pt0[X]) && (lassoMin[X] <= pt2[X]) &&
              (lassoMax[Y] >= pt0[Y]) && (lassoMin[Y] <= pt2[Y]))) {
            continue;
        }
        
        // Set the values of the other corners of the rectangle.
        pt0[Z] = pt1[Z] = pt2[Z] = pt3[Z] = 0.0;
        pt1[X] = pt2[X];
        pt1[Y] = pt0[Y];
        pt3[X] = pt0[X];
        pt3[Y] = pt2[Y];

        // If any of the bbox coords are in the lasso, then
        // the lasso goes through the bbox.
        if ((pointInLasso(theLassoType, lassoMin, lassoMax,
                          lassoVertices, numLassoVertices, pt0)) ||
            (pointInLasso(theLassoType, lassoMin, lassoMax,
                          lassoVertices, numLassoVertices, pt1)) ||
            (pointInLasso(theLassoType, lassoMin, lassoMax,
                          lassoVertices, numLassoVertices, pt2)) ||
            (pointInLasso(theLassoType, lassoMin, lassoMax,
                          lassoVertices, numLassoVertices, pt3))) {
// --> EGB && SGB lasso selection error
//        lassoedList[lassoedIncr++] = index[loop];
					lassoedIncr++;
          lassoedList.append(index[loop]);
// <-- EGB && SGB
        }
        else {
            // None of the points are inside the lasso, so
            // we need to check for line intersections between
            // the bbox and the lasso lines.
            if (lineIntersectsLasso(lassoVertices,
                numLassoVertices, pt0, pt1) ||

                lineIntersectsLasso(lassoVertices,
                numLassoVertices, pt1, pt2) ||

                lineIntersectsLasso(lassoVertices,
                numLassoVertices, pt2, pt3) ||

                lineIntersectsLasso(lassoVertices,
                numLassoVertices, pt3, pt0)) {

// --> EGB && SGB lasso selection error
//        lassoedList[lassoedIncr++] = index[loop];
					lassoedIncr++;
          lassoedList.append(index[loop]);
// <-- EGB && SGB
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Check to see if the lasso encloses all points of a bbox.
//    This is used for both atom and bond labels.
//    We want to make a rectangle in lasso space using the bbox of
//    the label.  This rectangle should face toward the camera.
//
// Use: static, internal

static void
checkLassoEntireBBox(
    const float orthoCamXres, const float orthoCamYres,
    const SbMatrix &curMVP,
    const int32_t numberOfLabels,
    const int32_t *index,
    const SbBox3f *bbox,
    const SbVec3f *center,
    const short theLassoType, const SbBox3f &lassoBBox,
    const int32_t numLassoVertices, 
    const SbVec3f *lassoVertices, int32_t &lassoedIncr, 
    SbIntList &lassoedList)
//
////////////////////////////////////////////////////////////////////////
{
    int32_t loop;
    //SbVec3f minCoord, maxCoord, edge;
    SbVec3f minCoord, maxCoord;
    SbVec3f pt0, pt1, pt2, pt3;

    const SbVec3f &lassoMin = lassoBBox.getMin();
    const SbVec3f &lassoMax = lassoBBox.getMax();

    for (loop = 0; loop < numberOfLabels; loop++) {
        // Get the min and max and set their Z component to be the same as
        // the center.
        minCoord = bbox[loop].getMin();
        maxCoord = bbox[loop].getMax();

        // Put min and max into screen space
        pt0 = fromObjectSpace(minCoord, curMVP);
        pt2 = fromObjectSpace(maxCoord, curMVP);
        pt0[X] *= orthoCamXres;
        pt0[Y] *= orthoCamYres;
        pt2[X] *= orthoCamXres;
        pt2[Y] *= orthoCamYres;

        // See if this box intersects the lassoBBox.  If they don't intersect
        // then don't process this label.
        if (!((lassoMax[X] >= pt0[X]) && (lassoMin[X] <= pt2[X]) &&
              (lassoMax[Y] >= pt0[Y]) && (lassoMin[Y] <= pt2[Y]))) {
            continue;
        }
        
        // Set the values of the other corners of the rectangle.
        pt0[Z] = pt1[Z] = pt2[Z] = pt3[Z] = 0.0;
        pt1[X] = pt2[X];
        pt1[Y] = pt0[Y];
        pt3[X] = pt0[X];
        pt3[Y] = pt2[Y];

        if ((pointInLasso(theLassoType, lassoMin, lassoMax,
                          lassoVertices, numLassoVertices, pt0)) &&
            (pointInLasso(theLassoType, lassoMin, lassoMax,
                          lassoVertices, numLassoVertices, pt1)) &&
            (pointInLasso(theLassoType, lassoMin, lassoMax,
                          lassoVertices, numLassoVertices, pt2)) && 
            (pointInLasso(theLassoType, lassoMin, lassoMax,
                          lassoVertices, numLassoVertices, pt3))) {
// --> EGB && SGB lasso selection error
//        lassoedList[lassoedIncr++] = index[loop];
					lassoedIncr++;
          lassoedList.append(index[loop]);
// <-- EGB && SGB
        }
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Check to see if the lasso goes through any atom bboxes.  This is
//    done by making a circle and checking if any of the lasso line
//    segments intersects any of the circle line segments.
//
// Use: static, internal

static void
checkLassoThruAtomBBox(
    const float orthoCamXres, const float orthoCamYres,
    const SbMatrix &curMVP, const SbRotation &curRot,
    const SbBool doHydrogens, const ChemBaseData *chemData,
    const ChemAtomBBox *atomBBoxes,
    const short theLassoType, const SbBox3f &lassoBBox,
    const int32_t numLassoVertices, 
    const SbVec3f *lassoVertices, int32_t &lassoedIncr, 
    SbIntList &lassoedList)
//
////////////////////////////////////////////////////////////////////////
{
    int32_t loop;
    float   rad;
    //SbVec3f minCoord, maxCoord, edge;
    SbVec3f minCoord, maxCoord;
    SbVec3f pcenter;
    //SbVec3f pt0, pt1;
    SbVec3f pt0;
    //SbVec3f radVec;
    SbMatrix rotMatrix;

    if (atomBBoxes == NULL) return;

    const SbVec3f &lassoMin = lassoBBox.getMin();
    const SbVec3f &lassoMax = lassoBBox.getMax();

    rotMatrix.makeIdentity();
    rotMatrix.setRotate(curRot);

    // Loop over all of the atom bboxes
    for (loop = 0; loop < atomBBoxes->numberOfAtoms; loop++) {
        // Check to see if this is a hydrogen and if we are to select it
        // or not.
        if (!doHydrogens) {
            if (chemData->getAtomicNumber(atomBBoxes->index[loop]) == 1) {
                continue;
            }
        }

        // What the following code is doing is determining a point in the X
        // direction a distance of radius from the origin.  This point is
        // transformed by the direction of the camera (contained in rotMatrix)
        // so that it is perpendicular to the line of sight.

        pt0.setValue(atomBBoxes->radius[loop], 0.0, 0.0);
        rotMatrix.multVecMatrix(pt0, pt0);
        pt0 += atomBBoxes->center[loop];

        // Transform center and edge by the MVP matrix.
        pcenter = fromObjectSpace(atomBBoxes->center[loop], curMVP);
        pt0 = fromObjectSpace(pt0, curMVP);

        // Scale by the camera resolution.
        pcenter[X] *= orthoCamXres;
        pcenter[Y] *= orthoCamYres;
        pcenter[Z] = 0.0;
        pt0[X] *= orthoCamXres;
        pt0[Y] *= orthoCamYres;

        // We can now get the radius of the circle to use for the intersection
        // tests.
        rad = (pt0[X] - pcenter[X]);

        minCoord = maxCoord = pcenter;
        minCoord[X] -= rad;
        minCoord[Y] -= rad;
        maxCoord[X] += rad;
        maxCoord[Y] += rad;

        // See if this box intersects the lassoBBox.  If they don't intersect
        // then don't process this atom.
        if (!((lassoMax[X] >= minCoord[X]) && (lassoMin[X] <= maxCoord[X]) &&
              (lassoMax[Y] >= minCoord[Y]) && (lassoMin[Y] <= maxCoord[Y]))) {
            continue;
        }

        // Check to see if the center of the circle is within the lasso
        if (pointInLasso(theLassoType, lassoMin, lassoMax,
                         lassoVertices, numLassoVertices, pcenter)) {
// --> EGB && SGB lasso selection error
//            lassoedList[lassoedIncr++] = atomBBoxes->index[loop];
					lassoedIncr++;
          lassoedList.append(atomBBoxes->index[loop]);
// <-- EGB && SGB
        }
        // Check to see if the lasso intersects the circle
        else if (lassoIntersectsCircle(lassoVertices, numLassoVertices,
                                       pcenter, rad)) {
// --> EGB && SGB lasso selection error
//            lassoedList[lassoedIncr++] = atomBBoxes->index[loop];
					lassoedIncr++;
          lassoedList.append(atomBBoxes->index[loop]);
// <-- EGB && SGB
        }
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Check to see if the lasso encloses any atom bboxes.  This is
//    done by making a circle and checking if all of the points of
//    the circle are within the lasso.
//
// Use: static, internal

static void
checkLassoEntireAtomBBox(
    const float orthoCamXres, const float orthoCamYres,
    const SbVec2f *ringCoords,
    const SbMatrix &curMVP, const SbRotation &curRot,
    const SbBool doHydrogens, const ChemBaseData *chemData,
    const ChemAtomBBox *atomBBoxes,
    const short theLassoType, const SbBox3f &lassoBBox,
    const int32_t numLassoVertices, 
    const SbVec3f *lassoVertices, int32_t &lassoedIncr, 
    SbIntList &lassoedList)
//
////////////////////////////////////////////////////////////////////////
{
    int32_t loop, side;
    float   rad;
    //SbVec3f minCoord, maxCoord, edge;
    SbVec3f minCoord, maxCoord;
    //SbVec3f pcenter, pedge;
    SbVec3f pcenter;
    //SbVec3f pt0, pt1;
    SbVec3f pt0;
    SbBool  allIn;
    SbMatrix rotMatrix;

    if (atomBBoxes == NULL) return;

    const SbVec3f &lassoMin = lassoBBox.getMin();
    const SbVec3f &lassoMax = lassoBBox.getMax();

    rotMatrix.makeIdentity();
    rotMatrix.setRotate(curRot);

    // See checkLassoThruAtomBBox for a description of what is going on
    // here in determining pcenter, pedge and radius.
    for (loop = 0; loop < atomBBoxes->numberOfAtoms; loop++) {
        // Check to see if this is a hydrogen and if we are to select it
        // or not.
        if (!doHydrogens) {
            if (chemData->getAtomicNumber(atomBBoxes->index[loop]) == 1) {
                continue;
            }
        }
        // What the following code is doing is determining a point in the X
        // direction a distance of radius from the origin.  This point is
        // transformed by the direction of the camera (contained in rotMatrix)
        // so that it is perpendicular to the line of sight.

        pt0.setValue(atomBBoxes->radius[loop], 0.0, 0.0);
        rotMatrix.multVecMatrix(pt0, pt0);
        pt0 += atomBBoxes->center[loop];

        // Transform center and edge by the MVP matrix.
        pcenter = fromObjectSpace(atomBBoxes->center[loop], curMVP);
        pt0 = fromObjectSpace(pt0, curMVP);

        // Scale by the camera resolution.
        pcenter[X] *= orthoCamXres;
        pcenter[Y] *= orthoCamYres;
        pcenter[Z] = 0.0;
        pt0[X] *= orthoCamXres;
        pt0[Y] *= orthoCamYres;

        // We can now get the radius of the circle to use for the intersection
        // tests.
        rad = (pt0[X] - pcenter[X]);

        minCoord = maxCoord = pcenter;
        minCoord[X] -= rad;
        minCoord[Y] -= rad;
        maxCoord[X] += rad;
        maxCoord[Y] += rad;

        // See if this box intersects the lassoBBox.  If they don't intersect
        // then don't process this atom.
        if (!((lassoMax[X] >= minCoord[X]) && (lassoMin[X] <= maxCoord[X]) &&
              (lassoMax[Y] >= minCoord[Y]) && (lassoMin[Y] <= maxCoord[Y]))) {
            continue;
        }

        allIn = TRUE;
        // Loop through the ring points and see if they are in the lasso.
        for (side = 0; side < RINGSIDES; side++) {
            pt0[0] = rad * ringCoords[side][0] + pcenter[0];
            pt0[1] = rad * ringCoords[side][1] + pcenter[1];
            if (!pointInLasso(theLassoType, lassoMin, lassoMax, 
                              lassoVertices, numLassoVertices, pt0)) {
                allIn = FALSE;
                break;
            }
        }
        if (allIn) {
// --> EGB && SGB lasso selection error
//            lassoedList[lassoedIncr++] = atomBBoxes->index[loop];
					lassoedIncr++;
					lassoedList.append(atomBBoxes->index[loop]);
// <-- EGB && SGB lasso selection error
        }
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Check to see if the lasso goes through any bond bboxes.
//
// Use: static, internal

static void
checkLassoThruStickBondBBox(
    const float orthoCamXres, const float orthoCamYres,
    const SbMatrix &curMVP,
    const SbBool doHydrogens,
    const ChemBaseData *chemData,
    const ChemStickBondBBox *bondBBoxes,
    const short theLassoType, const SbBox3f &lassoBBox,
    const int32_t numLassoVertices, 
    const SbVec3f *lassoVertices, int32_t &lassoedIncr, 
    SbIntList &lassoedList)
//
////////////////////////////////////////////////////////////////////////
{
    if (bondBBoxes == NULL) return;

    if (bondBBoxes->numberOfBonds == 0) return;

    int32_t i, loop;
    SbVec3f bbox2[4];
    SbBox3f lbondBBox, lbondBBox2;
    SbVec3f *verts0 = new SbVec3f[bondBBoxes->numberOfVertices];
    SbVec3f *verts1 = new SbVec3f[bondBBoxes->numberOfVertices];
    SbVec3f endBboxVerts0[4];
    SbVec3f endBboxVerts1[4];
    SbVec3f theVertex0, theVertex1;

    const SbVec3f &lassoMin = lassoBBox.getMin();
    const SbVec3f &lassoMax = lassoBBox.getMax();

    // Set up the vertices to be used to extend the bounding box
    // to include the ends of the cylinders.
    endBboxVerts0[0][X] = 1.0;
    endBboxVerts0[0][Y] = 1.0;
    endBboxVerts0[0][Z] = 0.0;

    endBboxVerts0[1][X] = 0.0;
    endBboxVerts0[1][Y] = 1.0;
    endBboxVerts0[1][Z] = 1.0;

    endBboxVerts0[2][X] = -1.0;
    endBboxVerts0[2][Y] = 1.0;
    endBboxVerts0[2][Z] = 0.0;

    endBboxVerts0[3][X] = 0.0;
    endBboxVerts0[3][Y] = 1.0;
    endBboxVerts0[3][Z] = -1.0;

    endBboxVerts1[0] = endBboxVerts0[0];
    endBboxVerts1[1] = endBboxVerts0[1];
    endBboxVerts1[2] = endBboxVerts0[2];
    endBboxVerts1[3] = endBboxVerts0[3];

    endBboxVerts1[0][Y] = endBboxVerts1[1][Y] = -1.0;
    endBboxVerts1[2][Y] = endBboxVerts1[3][Y] = -1.0;

    for (loop = 0; loop < bondBBoxes->numberOfBonds; loop++) {
        // Check to see if either atom is a hydrogen and whether are to
        // to select bonds with hydrogens.
        if (!doHydrogens) {
            int32_t from, to, theBond;
            theBond = bondBBoxes->index[loop];
            from = chemData->getBondFrom(theBond);
            to = chemData->getBondTo(theBond);
            if ((chemData->getAtomicNumber(from) == 1) ||
                (chemData->getAtomicNumber(to) == 1)) continue;
        }
        lbondBBox.makeEmpty();
        // Get the 2D projections of the bounding box
        for (i = 0; i < 4; i++) {
            bbox2[i] = fromObjectSpace(bondBBoxes->silhouette[loop][i], curMVP);
            bbox2[i][X] *= orthoCamXres;
            bbox2[i][Y] *= orthoCamYres;
            bbox2[i][Z] = 0.0;
            lbondBBox.extendBy(bbox2[i]);
        }
        // Make the bbox slightly bigger to account for the ends of the
        // cylinders.  This is just a rough estimate but should suffice to
        //  exclude interactions with lassos that are way out of range.
        SbMatrix localMVP = curMVP;
        localMVP = localMVP.multLeft(bondBBoxes->transform[loop]);
        lbondBBox2 = lbondBBox;
        for (i = 0; i < 4; i++) {
            theVertex0 = fromObjectSpace(endBboxVerts0[i], localMVP);
            theVertex0[X] *= orthoCamXres;
            theVertex0[Y] *= orthoCamYres;
            theVertex0[Z] = 0.0;
            lbondBBox2.extendBy(theVertex0);

            theVertex0 = fromObjectSpace(endBboxVerts1[i], localMVP);
            theVertex0[X] *= orthoCamXres;
            theVertex0[Y] *= orthoCamYres;
            theVertex0[Z] = 0.0;
            lbondBBox2.extendBy(theVertex0);
        }

        // Check to see if the local bond BBox intersects the lasso bbox.
        const SbVec3f &lbondBBoxMin = lbondBBox2.getMin();
        const SbVec3f &lbondBBoxMax = lbondBBox2.getMax();
        if (!((lassoMax[X] >= lbondBBoxMin[X]) &&
              (lassoMin[X] <= lbondBBoxMax[X]) &&
              (lassoMax[Y] >= lbondBBoxMin[Y]) &&
              (lassoMin[Y] <= lbondBBoxMax[Y]))) {
            continue;
        }

        // Check to see if any of the bounding box points are contained in
        // the lasso.
        if ((pointInLasso(theLassoType, lassoMin, lassoMax,
                          lassoVertices, numLassoVertices, bbox2[0])) ||
            (pointInLasso(theLassoType, lassoMin, lassoMax,
                          lassoVertices, numLassoVertices, bbox2[1])) ||
            (pointInLasso(theLassoType, lassoMin, lassoMax,
                          lassoVertices, numLassoVertices, bbox2[2])) ||
            (pointInLasso(theLassoType, lassoMin, lassoMax,
                          lassoVertices, numLassoVertices, bbox2[3]))) {

// --> EGB && SGB lasso selection error
//            lassoedList[lassoedIncr++] = bondBBoxes->index[loop];
					lassoedIncr++;
					lassoedList.append(bondBBoxes->index[loop]);
// <-- EGB && SGB lasso selection error
            continue;
        }

        // Check for intersections of the edges with the lasso.
        // Check sides first as they are the most likely
        if ((lineIntersectsLasso(lassoVertices, numLassoVertices,
                                 bbox2[0], bbox2[1])) ||
            (lineIntersectsLasso(lassoVertices, numLassoVertices,
                                 bbox2[1], bbox2[2])) ||
            (lineIntersectsLasso(lassoVertices, numLassoVertices,
                                 bbox2[2], bbox2[3])) ||
            (lineIntersectsLasso(lassoVertices, numLassoVertices,
                                 bbox2[3], bbox2[0]))) {
// --> EGB && SGB lasso selection error
//            lassoedList[lassoedIncr++] = bondBBoxes->index[loop];
					lassoedIncr++;
					lassoedList.append(bondBBoxes->index[loop]);
// <-- EGB && SGB lasso selection error
            continue;
        }
        
        // Check to see if the lasso intersects the interior of the 
        // bbox.  This case would only occur if the lasso is drawn entirely
        // within the 4 lines comprising the box and therefore didn't cross
        // any of them.  Therefore, if any point on the lasso is outside of the
        // bbox, then there is no intersection.
        SbBool lassoInBBox = TRUE;
        for (i = 0; i < numLassoVertices; i++) {
            if (!pointInLasso(ChemSelection::LASSO,
                              lbondBBox.getMin(), lbondBBox.getMax(),
                              bbox2, 4, lassoVertices[i])) {
                lassoInBBox = FALSE;
                break;
            }
        }
        if (lassoInBBox) {
// --> EGB && SGB lasso selection error
//            lassoedList[lassoedIncr++] = bondBBoxes->index[loop];
					lassoedIncr++;
					lassoedList.append(bondBBoxes->index[loop]);
// <-- EGB && SGB lasso selection error
            continue;
        }

        // If we get here, then we need to check to see if the lasso
        // goes through the ends of the cylinder.  The stick bondBBox
        // holds the vertices of the circle forming the end of the
        // cylinder (numberOfVertices and vertices).
        SbBool foundOne = FALSE;
        for (i = 0; i < bondBBoxes->numberOfVertices; i++) {
            verts0[i] = bondBBoxes->vertices[i];
            verts1[i] = bondBBoxes->vertices[i];
            verts0[i][Y] =  1.0;
            verts1[i][Y] = -1.0;
            verts0[i] = fromObjectSpace(verts0[i], localMVP);
            verts0[i][X] *= orthoCamXres;
            verts0[i][Y] *= orthoCamYres;
            verts0[i][Z] = 0.0;
            if (pointInLasso(theLassoType, lassoMin, lassoMax,
                             lassoVertices, numLassoVertices, verts0[i])) {
// --> EGB && SGB lasso selection error
//            lassoedList[lassoedIncr++] = bondBBoxes->index[loop];
								lassoedIncr++;
								lassoedList.append(bondBBoxes->index[loop]);
// <-- EGB && SGB lasso selection error
                foundOne = TRUE;
                break;
            }
            verts1[i] = fromObjectSpace(verts1[i], localMVP);
            verts1[i][X] *= orthoCamXres;
            verts1[i][Y] *= orthoCamYres;
            verts1[i][Z] = 0.0;
            if (pointInLasso(theLassoType, lassoMin, lassoMax,
                             lassoVertices, numLassoVertices, verts1[i])) {
// --> EGB && SGB lasso selection error
//            lassoedList[lassoedIncr++] = bondBBoxes->index[loop];
								lassoedIncr++;
								lassoedList.append(bondBBoxes->index[loop]);
// <-- EGB && SGB lasso selection error
                foundOne = TRUE;
                break;
            }
        }
        // If this cylinder was selected, continue on to the next one
        if (foundOne) continue;

        // None of the vertices is within the lasso, but they are all
        // converted to pixel space.  Now check for line intersections.
        int32_t vcount = 0;
        theVertex0 = verts0[bondBBoxes->numberOfVertices - 1];
        theVertex1 = verts0[vcount++];
        for (i = bondBBoxes->numberOfVertices+1; --i; ) {
            if (lineIntersectsLasso(lassoVertices, numLassoVertices,
                                    theVertex0, theVertex1)) {
// --> EGB && SGB lasso selection error
//            lassoedList[lassoedIncr++] = bondBBoxes->index[loop];
								lassoedIncr++;
								lassoedList.append(bondBBoxes->index[loop]);
// <-- EGB && SGB lasso selection error

                foundOne = TRUE;
                break;
            }
            theVertex0 = theVertex1;
            theVertex1 = verts0[vcount++];
        }
        if (foundOne) continue;

        vcount = 0;
        theVertex0 = verts1[bondBBoxes->numberOfVertices - 1];
        theVertex1 = verts1[vcount++];
        for (i = bondBBoxes->numberOfVertices+1; --i; ) {
            if (lineIntersectsLasso(lassoVertices, numLassoVertices,
                                    theVertex0, theVertex1)) {
// --> EGB && SGB lasso selection error
//            lassoedList[lassoedIncr++] = bondBBoxes->index[loop];
								lassoedIncr++;
								lassoedList.append(bondBBoxes->index[loop]);
// <-- EGB && SGB lasso selection error

                foundOne = TRUE;
                break;
            }
            theVertex0 = theVertex1;
            theVertex1 = verts1[vcount++];
        }
        if (foundOne) continue;

        // The last case is if all the points in the lasso are within the
        // vertices of the ends.
        lassoInBBox = TRUE;
        for (i = 0; i < numLassoVertices; i++) {
            if (!pointInLasso(ChemSelection::LASSO,
                              lbondBBox2.getMin(), lbondBBox2.getMax(),
                              verts0, bondBBoxes->numberOfVertices,
                              lassoVertices[i])) {
                lassoInBBox = FALSE;
                break;
            }
        }
        if (!lassoInBBox) {
            lassoInBBox = TRUE;
            for (i = 0; i < numLassoVertices; i++) {
                if (!pointInLasso(ChemSelection::LASSO,
                                  lbondBBox2.getMin(), lbondBBox2.getMax(),
                                  verts1, bondBBoxes->numberOfVertices,
                                  lassoVertices[i])) {
                    lassoInBBox = FALSE;
                    break;
                }
            }
        }
        if (lassoInBBox) {
// --> EGB && SGB lasso selection error
//            lassoedList[lassoedIncr++] = bondBBoxes->index[loop];
						lassoedIncr++;
						lassoedList.append(bondBBoxes->index[loop]);
// <-- EGB && SGB lasso selection error
            continue;
        }
    }

    delete [] verts0;
    delete [] verts1;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Check to see if the lasso encloses any bond bboxes.
//
// Use: static, internal

static void
checkLassoEntireStickBondBBox(
    const float orthoCamXres, const float orthoCamYres,
    const SbMatrix &curMVP,
    const SbBool doHydrogens,
    const ChemBaseData *chemData,
    const ChemStickBondBBox *bondBBoxes,
    const short theLassoType, const SbBox3f &lassoBBox,
    const int32_t numLassoVertices, 
    const SbVec3f *lassoVertices, int32_t &lassoedIncr, 
    SbIntList &lassoedList)
//
////////////////////////////////////////////////////////////////////////
{
    if (bondBBoxes == NULL) return;

    if (bondBBoxes->numberOfBonds == 0) return;

    int32_t loop;
    SbVec3f bbox2[4];
    SbBox3f lbondBBox, lbondBBox2;
    SbVec3f *verts0 = new SbVec3f[bondBBoxes->numberOfVertices];
    SbVec3f *verts1 = new SbVec3f[bondBBoxes->numberOfVertices];
    SbVec3f endBboxVerts0[4];
    SbVec3f endBboxVerts1[4];
    //SbVec3f theVertex0, theVertex1;
    SbVec3f theVertex0;


    const SbVec3f &lassoMin = lassoBBox.getMin();
    const SbVec3f &lassoMax = lassoBBox.getMax();

    // Set up the vertices to be used to extend the bounding box
    // to include the ends of the cylinders.
    endBboxVerts0[0][X] = 1.0;
    endBboxVerts0[0][Y] = 1.0;
    endBboxVerts0[0][Z] = 0.0;

    endBboxVerts0[1][X] = 0.0;
    endBboxVerts0[1][Y] = 1.0;
    endBboxVerts0[1][Z] = 1.0;

    endBboxVerts0[2][X] = -1.0;
    endBboxVerts0[2][Y] = 1.0;
    endBboxVerts0[2][Z] = 0.0;

    endBboxVerts0[3][X] = 0.0;
    endBboxVerts0[3][Y] = 1.0;
    endBboxVerts0[3][Z] = -1.0;

    endBboxVerts1[0] = endBboxVerts0[0];
    endBboxVerts1[1] = endBboxVerts0[1];
    endBboxVerts1[2] = endBboxVerts0[2];
    endBboxVerts1[3] = endBboxVerts0[3];

    endBboxVerts1[0][Y] = endBboxVerts1[1][Y] = -1.0;
    endBboxVerts1[2][Y] = endBboxVerts1[3][Y] = -1.0;

    for (loop = 0; loop < bondBBoxes->numberOfBonds; loop++) {
        // Check to see if either atom is a hydrogen and whether are to
        // to select bonds with hydrogens.
        if (!doHydrogens) {
            int32_t from, to, theBond;
            theBond = bondBBoxes->index[loop];
            from = chemData->getBondFrom(theBond);
            to = chemData->getBondTo(theBond);
            if ((chemData->getAtomicNumber(from) == 1) ||
                (chemData->getAtomicNumber(to) == 1)) continue;
        }

        lbondBBox.makeEmpty();
        // Get the 2D projections of the bounding box
	// int i = 0 put out of the for loop by fabien fontaine the 15/12/2000
	// to avoid compiler warning
	int i = 0 ;
        for ( ; i < 4; i++) {
            bbox2[i] = fromObjectSpace(bondBBoxes->silhouette[loop][i], curMVP);
            bbox2[i][0] *= orthoCamXres;
            bbox2[i][1] *= orthoCamYres;
            bbox2[i][2] = 0.0;
            lbondBBox.extendBy(bbox2[i]);
        }
        // Make the bbox slightly bigger to account for the ends of the
        // cylinders.  This is just a rough estimate but should suffice to
        //  exclude interactions with lassos that are way out of range.
        SbMatrix localMVP = curMVP;
        localMVP = localMVP.multLeft(bondBBoxes->transform[loop]);
        lbondBBox2 = lbondBBox;
        for (i = 0; i < 4; i++) {
            theVertex0 = fromObjectSpace(endBboxVerts0[i], localMVP);
            theVertex0[X] *= orthoCamXres;
            theVertex0[Y] *= orthoCamYres;
            theVertex0[Z] = 0.0;
            lbondBBox2.extendBy(theVertex0);

            theVertex0 = fromObjectSpace(endBboxVerts1[i], localMVP);
            theVertex0[X] *= orthoCamXres;
            theVertex0[Y] *= orthoCamYres;
            theVertex0[Z] = 0.0;
            lbondBBox2.extendBy(theVertex0);
        }

        // Check to see if the local bond BBox intersects the lasso bbox.
        const SbVec3f &lbondBBoxMin = lbondBBox2.getMin();
        const SbVec3f &lbondBBoxMax = lbondBBox2.getMax();
        if (!((lassoMax[X] >= lbondBBoxMin[X]) &&
              (lassoMin[X] <= lbondBBoxMax[X]) &&
              (lassoMax[Y] >= lbondBBoxMin[Y]) &&
              (lassoMin[Y] <= lbondBBoxMax[Y]))) {
            continue;
        }

        SbBool allIn = FALSE;
        // Check to see if all of the bounding box points are contained in
        // the lasso.
        if ((pointInLasso(theLassoType, lassoMin, lassoMax,
                          lassoVertices, numLassoVertices, bbox2[0])) &&
            (pointInLasso(theLassoType, lassoMin, lassoMax,
                          lassoVertices, numLassoVertices, bbox2[1])) &&
            (pointInLasso(theLassoType, lassoMin, lassoMax,
                          lassoVertices, numLassoVertices, bbox2[2])) &&
            (pointInLasso(theLassoType, lassoMin, lassoMax,
                          lassoVertices, numLassoVertices, bbox2[3]))) {

            allIn = TRUE;
        }

        // If all of the silhouette points were in the lasso, then we
        // need to check that all of the end vertices are in the lasso.
        if (allIn) {
            for (i = 0; i < bondBBoxes->numberOfVertices; i++) {
                verts0[i] = bondBBoxes->vertices[i];
                verts1[i] = bondBBoxes->vertices[i];
                verts0[i][Y] =  1.0;
                verts1[i][Y] = -1.0;
                verts0[i] = fromObjectSpace(verts0[i], localMVP);
                verts0[i][X] *= orthoCamXres;
                verts0[i][Y] *= orthoCamYres;
                verts0[i][Z] = 0.0;
                if (!pointInLasso(theLassoType, lassoMin, lassoMax,
                                  lassoVertices, numLassoVertices, verts0[i])) {
// --> EGB && SGB lasso selection error
//            lassoedList[lassoedIncr++] = bondBBoxes->index[loop];
										lassoedIncr++;
										lassoedList.append(bondBBoxes->index[loop]);
// <-- EGB && SGB lasso selection error

                    allIn = FALSE;
                    break;
                }
                verts1[i] = fromObjectSpace(verts1[i], localMVP);
                verts1[i][X] *= orthoCamXres;
                verts1[i][Y] *= orthoCamYres;
                verts1[i][Z] = 0.0;
                if (!pointInLasso(theLassoType, lassoMin, lassoMax,
                                  lassoVertices, numLassoVertices, verts1[i])) {
                    allIn = FALSE;
                    break;
                }
            }
        }
        if (allIn) {
// --> EGB && SGB lasso selection error
//            lassoedList[lassoedIncr++] = bondBBoxes->index[loop];
					lassoedIncr++;
					lassoedList.append(bondBBoxes->index[loop]);
// <-- EGB && SGB lasso selection error

        }
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Check to see if the lasso goes through any bond bboxes.
//
// Use: static, internal

static void
checkLassoThruWireframeBondBBox(
    const float orthoCamXres, const float orthoCamYres,
    const SbMatrix &curMVP,
    const SbBool doHydrogens,
    const ChemBaseData *chemData,
    const ChemWireframeBondBBox *bondBBoxes,
    const short theLassoType, const SbBox3f &lassoBBox,
    const int32_t numLassoVertices, 
    const SbVec3f *lassoVertices, int32_t &lassoedIncr, 
    SbIntList &lassoedList)
//
////////////////////////////////////////////////////////////////////////
{
    if (bondBBoxes == NULL) return;

    if (bondBBoxes->numberOfBonds == 0) return;

    int32_t i, loop, theBond;
    SbVec3f bbox2[4];
    SbBox3f lbondBBox;

    const SbVec3f &lassoMin = lassoBBox.getMin();
    const SbVec3f &lassoMax = lassoBBox.getMax();

    for (loop = 0; loop < bondBBoxes->numberOfBonds; loop++) {
        // Check to see if either atom is a hydrogen and whether are to
        // to select bonds with hydrogens.
        theBond = bondBBoxes->index[loop];
        if (!doHydrogens) {
            int32_t from, to;
            from = chemData->getBondFrom(theBond);
            to = chemData->getBondTo(theBond);
            if ((chemData->getAtomicNumber(from) == 1) ||
                (chemData->getAtomicNumber(to) == 1)) continue;
        }

        // Check to see what type of bond this is.  If it is a single
        // bond, then we only have to check for line intersections with
        // the lasso.
        lbondBBox.makeEmpty();
        if (bondBBoxes->bondType[loop] == ChemBaseData::SINGLE_BOND) {
            for (i = 0; i < 2; i++) {
                bbox2[i] =
                    fromObjectSpace(bondBBoxes->silhouette[loop][i], curMVP);
                bbox2[i][X] *= orthoCamXres;
                bbox2[i][Y] *= orthoCamYres;
                bbox2[i][Z] = 0.0;
                lbondBBox.extendBy(bbox2[i]);
            }
            // Check to see if the local bond BBox intersects the lasso bbox.
            const SbVec3f &lbondBBoxMin = lbondBBox.getMin();
            const SbVec3f &lbondBBoxMax = lbondBBox.getMax();
            if (!((lassoMax[X] >= lbondBBoxMin[X]) &&
                  (lassoMin[X] <= lbondBBoxMax[X]) &&
                  (lassoMax[Y] >= lbondBBoxMin[Y]) &&
                  (lassoMin[Y] <= lbondBBoxMax[Y]))) {
                continue;
            }
            if ((pointInLasso(theLassoType, lassoMin, lassoMax,
                              lassoVertices, numLassoVertices, bbox2[0])) ||
                (pointInLasso(theLassoType, lassoMin, lassoMax,
                              lassoVertices, numLassoVertices, bbox2[1]))) {
// --> EGB && SGB lasso selection error
//            lassoedList[lassoedIncr++] = bondBBoxes->index[loop];
								lassoedIncr++;
								lassoedList.append(bondBBoxes->index[loop]);
// <-- EGB && SGB lasso selection error

                continue;
            }
            if (lineIntersectsLasso(lassoVertices, numLassoVertices,
                                     bbox2[0], bbox2[1])) {
// --> EGB && SGB lasso selection error
//            lassoedList[lassoedIncr++] = bondBBoxes->index[loop];
								lassoedIncr++;
								lassoedList.append(bondBBoxes->index[loop]);
// <-- EGB && SGB lasso selection error

                continue;
            }
        }
        else {
            // Get the 2D projections of the bounding box
            for (i = 0; i < 4; i++) {
                bbox2[i] = fromObjectSpace(bondBBoxes->silhouette[loop][i], curMVP);
                bbox2[i][X] *= orthoCamXres;
                bbox2[i][Y] *= orthoCamYres;
                bbox2[i][Z] = 0.0;
                lbondBBox.extendBy(bbox2[i]);
            }
            // Check to see if the local bond BBox intersects the lasso bbox.
            const SbVec3f &lbondBBoxMin = lbondBBox.getMin();
            const SbVec3f &lbondBBoxMax = lbondBBox.getMax();
            if (!((lassoMax[X] >= lbondBBoxMin[X]) &&
                  (lassoMin[X] <= lbondBBoxMax[X]) &&
                  (lassoMax[Y] >= lbondBBoxMin[Y]) &&
                  (lassoMin[Y] <= lbondBBoxMax[Y]))) {
                continue;
            }
    
            // Check to see if any of the bounding box points are contained in
            // the lasso.
            if ((pointInLasso(theLassoType, lassoMin, lassoMax,
                              lassoVertices, numLassoVertices, bbox2[0])) ||
                (pointInLasso(theLassoType, lassoMin, lassoMax,
                              lassoVertices, numLassoVertices, bbox2[1])) ||
                (pointInLasso(theLassoType, lassoMin, lassoMax,
                              lassoVertices, numLassoVertices, bbox2[2])) ||
                (pointInLasso(theLassoType, lassoMin, lassoMax,
                              lassoVertices, numLassoVertices, bbox2[3]))) {
    
// --> EGB && SGB lasso selection error
//            lassoedList[lassoedIncr++] = bondBBoxes->index[loop];
								lassoedIncr++;
								lassoedList.append(bondBBoxes->index[loop]);
// <-- EGB && SGB lasso selection error
                continue;
            }
    
            // Check for intersections of the edges with the lasso.
            // Check sides first as they are the most likely
            if ((lineIntersectsLasso(lassoVertices, numLassoVertices,
                                     bbox2[0], bbox2[1])) ||
                (lineIntersectsLasso(lassoVertices, numLassoVertices,
                                     bbox2[1], bbox2[2])) ||
                (lineIntersectsLasso(lassoVertices, numLassoVertices,
                                     bbox2[2], bbox2[3])) ||
                (lineIntersectsLasso(lassoVertices, numLassoVertices,
                                     bbox2[3], bbox2[0]))) {
// --> EGB && SGB lasso selection error
//            lassoedList[lassoedIncr++] = bondBBoxes->index[loop];
								lassoedIncr++;
								lassoedList.append(bondBBoxes->index[loop]);
// <-- EGB && SGB lasso selection error
                continue;
            }
            
            // Check to see if the lasso intersects the interior of the 
            // bbox.  This case would only occur if the lasso is drawn entirely
            // within the 4 lines comprising the box and therefore didn't cross
            // any of them.  Therefore, if any point on the lasso is outside of the
            // bbox, then there is no intersection.
            SbBool lassoInBBox = TRUE;
            for (i = 0; i < numLassoVertices; i++) {
                if (!pointInLasso(ChemSelection::LASSO, 
                                  lbondBBoxMin, lbondBBoxMax,
                                  bbox2, 4, lassoVertices[i])) {
                    lassoInBBox = FALSE;
                    break;
                }
            }
            if (lassoInBBox) {
// --> EGB && SGB lasso selection error
//            lassoedList[lassoedIncr++] = bondBBoxes->index[loop];
							lassoedIncr++;
							lassoedList.append(bondBBoxes->index[loop]);
// <-- EGB && SGB lasso selection error
                continue;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Check to see if the lasso encloses any bond bboxes.
//
// Use: static, internal

static void
checkLassoEntireWireframeBondBBox(
    const float orthoCamXres, const float orthoCamYres,
    const SbMatrix &curMVP,
    const SbBool doHydrogens,
    const ChemBaseData *chemData,
    const ChemWireframeBondBBox *bondBBoxes,
    const short theLassoType, const SbBox3f &lassoBBox,
    const int32_t numLassoVertices, 
    const SbVec3f *lassoVertices, int32_t &lassoedIncr, 
    SbIntList &lassoedList)
//
////////////////////////////////////////////////////////////////////////
{
    if (bondBBoxes == NULL) return;

    if (bondBBoxes->numberOfBonds == 0) return;

    int32_t loop, theBond;
    SbVec3f bbox2[4];
    SbBox3f lbondBBox;

    const SbVec3f &lassoMin = lassoBBox.getMin();
    const SbVec3f &lassoMax = lassoBBox.getMax();

    for (loop = 0; loop < bondBBoxes->numberOfBonds; loop++) {
        // Check to see if either atom is a hydrogen and whether are to
        // to select bonds with hydrogens.
        theBond = bondBBoxes->index[loop];
        if (!doHydrogens) {
            int32_t from, to;
            from = chemData->getBondFrom(theBond);
            to = chemData->getBondTo(theBond);
            if ((chemData->getAtomicNumber(from) == 1) ||
                (chemData->getAtomicNumber(to) == 1)) continue;
        }

        lbondBBox.makeEmpty();
        // Get the 2D projections of the bounding box
        if (bondBBoxes->bondType[loop] == ChemBaseData::SINGLE_BOND) {
            for (int i = 0; i < 2; i++) {
                bbox2[i] =
                    fromObjectSpace(bondBBoxes->silhouette[loop][i], curMVP);
                bbox2[i][0] *= orthoCamXres;
                bbox2[i][1] *= orthoCamYres;
                bbox2[i][2] = 0.0;
                lbondBBox.extendBy(bbox2[i]);
            }
        }
        else {
            for (int i = 0; i < 4; i++) {
                bbox2[i] =
                    fromObjectSpace(bondBBoxes->silhouette[loop][i], curMVP);
                bbox2[i][0] *= orthoCamXres;
                bbox2[i][1] *= orthoCamYres;
                bbox2[i][2] = 0.0;
                lbondBBox.extendBy(bbox2[i]);
            }
        }
        // Check to see if the local bond BBox intersects the lasso bbox.
        const SbVec3f &lbondBBoxMin = lbondBBox.getMin();
        const SbVec3f &lbondBBoxMax = lbondBBox.getMax();
        if (!((lassoMax[X] >= lbondBBoxMin[X]) &&
              (lassoMin[X] <= lbondBBoxMax[X]) &&
              (lassoMax[Y] >= lbondBBoxMin[Y]) &&
              (lassoMin[Y] <= lbondBBoxMax[Y]))) {
            continue;
        }

        // Check to see if all of the bounding box points are contained in
        // the lasso.
        if (bondBBoxes->bondType[loop] == ChemBaseData::SINGLE_BOND) {
            if ((pointInLasso(theLassoType, lassoMin, lassoMax,
                              lassoVertices, numLassoVertices, bbox2[0])) &&
                (pointInLasso(theLassoType, lassoMin, lassoMax,
                              lassoVertices, numLassoVertices, bbox2[1]))) {
// --> EGB && SGB lasso selection error
//            lassoedList[lassoedIncr++] = bondBBoxes->index[loop];
								lassoedIncr++;
								lassoedList.append(bondBBoxes->index[loop]);
// <-- EGB && SGB lasso selection error
            }
        }
        else {
            if ((pointInLasso(theLassoType, lassoMin, lassoMax,
                              lassoVertices, numLassoVertices, bbox2[0])) &&
                (pointInLasso(theLassoType, lassoMin, lassoMax,
                              lassoVertices, numLassoVertices, bbox2[1])) &&
                (pointInLasso(theLassoType, lassoMin, lassoMax,
                              lassoVertices, numLassoVertices, bbox2[2])) &&
                (pointInLasso(theLassoType, lassoMin, lassoMax,
                              lassoVertices, numLassoVertices, bbox2[3]))) {    
// --> EGB && SGB lasso selection error
//            lassoedList[lassoedIncr++] = bondBBoxes->index[loop];
							lassoedIncr++;
							lassoedList.append(bondBBoxes->index[loop]);
// <-- EGB && SGB lasso selection error
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    See if bond bbox and sphere intersect.
//    Most of this is from SbLine::intersect.
//
// Use: internal, static

static SbBool
bondBBoxSphereIntersect(const Silhouette &bbox, 
    const SbMatrix &modelMatrix,
    const SbVec3f &bondCenter, const SbVec3f &sphereCenter,
    const float &sphereRadiusSquared)
//
////////////////////////////////////////////////////////////////////////
{
    // Check to see of the distance between the center of the bbox and
    // the center of the sphere is less than the sphereRadius.  If so,
    // then the center of the bond is within the radius and the bond is
    // selected.

    SbVec3f tmpBondCenter;
    modelMatrix.multVecMatrix(bondCenter, tmpBondCenter);
    float bondSphereDistSquared = distSquared(tmpBondCenter, sphereCenter);
    if (bondSphereDistSquared <= sphereRadiusSquared) return TRUE;

    int i;
    Silhouette tmpBox;
    for (i = 0; i < 4; i++) {
        modelMatrix.multVecMatrix(bbox[i], tmpBox[i]);
    }

    // Now check to see if any of the corners of the bbox are within the
    // sphereRadius.  If so, then the bond is selected.
    if ((distSquared(tmpBox[0], sphereCenter) <= sphereRadiusSquared) ||
        (distSquared(tmpBox[1], sphereCenter) <= sphereRadiusSquared) ||
        (distSquared(tmpBox[2], sphereCenter) <= sphereRadiusSquared) ||
        (distSquared(tmpBox[3], sphereCenter) <= sphereRadiusSquared)) {
        return TRUE;
    }
    return FALSE;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Select items from the lassoedList.
//
// Use: static, internal   

static void
selectItems(SbIntList lassoedList, MFVec2i &theIndex)
//
////////////////////////////////////////////////////////////////////////
{
    int32_t atomLoop, atomEnd, theAtom;
    int32_t lastAtomStart = -1;
    int32_t lastAtomIncr  = 0;
    int32_t count = 0;

    // Go through the list of atoms that have been lassoed and 
    // add to the chemPath.
    atomEnd = lassoedList.getLength();
    for (atomLoop = 0; atomLoop < atomEnd; atomLoop++) {
        theAtom = lassoedList[atomLoop];
        if (lastAtomStart == -1) {
            lastAtomStart = theAtom;
            lastAtomIncr = 1;
        }
        else if (theAtom == (lastAtomStart + lastAtomIncr)) {
            lastAtomIncr++;
        }
        else {
            theIndex.set1Value(count++,
                SbVec2i(lastAtomStart, lastAtomIncr));
            lastAtomStart = theAtom;
            lastAtomIncr = 1;
        }
    }

    if (lastAtomStart != -1) {
        theIndex.set1Value(count,
            SbVec2i(lastAtomStart, lastAtomIncr));
    }
}

// --> residue atoms selection
static void selectResidueAtoms(SbIntList &lassoedList, MFVec2i &atomIndex,
							   ChemData *chemData, bool residueSelectAtoms)
{
	SbIntList residueAtomsList;
	if (residueSelectAtoms)
	{
		residueAtomsList.truncate(0);
		int residueLoopEnd = lassoedList.getLength();
		int i,j;
		int residueIndex = 0, atomIndex = 0;
		for (i = 0; i< residueLoopEnd; i++)
		{
			residueIndex = lassoedList[i];
			assert(residueIndex < chemData->residues.getNum());
			for (j = 0; j<chemData->residues[residueIndex].getAtomIndices().getNum(); j++)
			{
				atomIndex = chemData->residues[residueIndex].getAtomIndices()[j];
				if (atomIndex >= 0)
					residueAtomsList.append(atomIndex);
			}
		}
	}

	if (residueAtomsList.getLength() > 0)
		selectItems(residueAtomsList, atomIndex);
}
// <-- residue atoms selection

#undef X
#undef Y
#undef Z

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Constructor initialization
//
// Use: private

void
ChemSelection::constructorCommon()
//
////////////////////////////////////////////////////////////////////////
{
    SO_NODE_CONSTRUCTOR(ChemSelection);

    SO_NODE_ADD_FIELD(parts, (ChemSelection::ALL));
    SO_NODE_ADD_FIELD(togglePolicy, (ChemSelection::SHIFT));
    SO_NODE_ADD_FIELD(atomLassoPolicy, (ChemSelection::CENTER));
    SO_NODE_ADD_FIELD(bondLassoPolicy, (ChemSelection::CENTER));
    SO_NODE_ADD_FIELD(atomLabelLassoPolicy, (ChemSelection::CENTER));
    SO_NODE_ADD_FIELD(bondLabelLassoPolicy, (ChemSelection::CENTER));
    SO_NODE_ADD_FIELD(chemLabelLassoPolicy, (ChemSelection::CENTER));
    SO_NODE_ADD_FIELD(chemMonitorLassoPolicy, (ChemSelection::CENTER));
// --> residue selection
		SO_NODE_ADD_FIELD(residueLassoPolicy, (ChemSelection::CENTER));
		SO_NODE_ADD_FIELD(residueLabelLassoPolicy, (ChemSelection::CENTER));
// <-- residue selection
    SO_NODE_ADD_FIELD(selectInvisibleHydrogens, (FALSE));

    // Default lasso line parameters
    SO_NODE_ADD_FIELD(lassoSelectCutoff, (0.1f));
    SO_NODE_ADD_FIELD(lassoType, (ChemSelection::NOLASSO));
    SO_NODE_ADD_FIELD(lassoLineWidth, (1));
    SO_NODE_ADD_FIELD(lassoLinePattern, (0xffff));
    SO_NODE_ADD_FIELD(lassoLineColorIndex, (1));

    // The coordinates of the lasso
#ifdef IV2_0
    SoCoordinate3 *coords = new SoCoordinate3;
    coords->ref();
    SO_NODE_ADD_FIELD(lassoPoints, ((SoNode *)coords));
#else
    SoVertexProperty *vertexProperty = new SoVertexProperty;
    vertexProperty->ref();
    SO_NODE_ADD_FIELD(lassoPoints, ((SoNode *)vertexProperty));
#endif

    // Fields for the radiusSelect option
    SO_NODE_ADD_FIELD(radiusSelect, (FALSE));
    SO_NODE_ADD_FIELD(radiusSelectCutoff, (0.001f));
    SO_NODE_ADD_FIELD(radiusSelectSphereColor, (1.0, 1.0, 1.0));
    SO_NODE_ADD_FIELD(radiusSelectSphereTransparency, (0.0));
    SO_NODE_ADD_FIELD(radiusSelectSphereDrawStyle, (ChemSelection::FILLED));
    SO_NODE_ADD_FIELD(radiusSelectSphereComplexity, (0.5));

    SO_NODE_ADD_FIELD(radiusSelectLabelColor, (1.0, 0.0, 0.0));
    SO_NODE_ADD_FIELD(radiusSelectLabelFontName, (DEFAULT_FONT));
    SO_NODE_ADD_FIELD(radiusSelectLabelFontSize, (10.0));

    // Set up static info for enumerated type field
    SO_NODE_DEFINE_ENUM_VALUE(TogglePolicy, SINGLE);
    SO_NODE_DEFINE_ENUM_VALUE(TogglePolicy, TOGGLE);
    SO_NODE_DEFINE_ENUM_VALUE(TogglePolicy, SHIFT);

    // Set up info in enumerated type field
    SO_NODE_SET_SF_ENUM_TYPE(togglePolicy, TogglePolicy);

    // Set up static info for enumerated type field
    SO_NODE_DEFINE_ENUM_VALUE(LassoType, NOLASSO);
    SO_NODE_DEFINE_ENUM_VALUE(LassoType, LASSO);
    SO_NODE_DEFINE_ENUM_VALUE(LassoType, DRAGGER);

    // Set up info in enumerated type field
    SO_NODE_SET_SF_ENUM_TYPE(lassoType, LassoType);

    // Enum for what to select
    SO_NODE_DEFINE_ENUM_VALUE(ChemPart, ATOMS);
    SO_NODE_DEFINE_ENUM_VALUE(ChemPart, BONDS);   
    SO_NODE_DEFINE_ENUM_VALUE(ChemPart, ATOMLABELS);
    SO_NODE_DEFINE_ENUM_VALUE(ChemPart, BONDLABELS);
    SO_NODE_DEFINE_ENUM_VALUE(ChemPart, CHEMLABELS);
    SO_NODE_DEFINE_ENUM_VALUE(ChemPart, CHEMMONITORS);
// --> residue selection
		SO_NODE_DEFINE_ENUM_VALUE(ChemPart, RESIDUES);
    SO_NODE_DEFINE_ENUM_VALUE(ChemPart, RESIDUELABELS);
// <-- residue selection
    SO_NODE_DEFINE_ENUM_VALUE(ChemPart, ALL);

    // Set up info in enumerated type field
    SO_NODE_SET_SF_ENUM_TYPE(parts, ChemPart);

    // Enum for how to lasso atoms and labels
    SO_NODE_DEFINE_ENUM_VALUE(LassoPolicy, CENTER);
    SO_NODE_DEFINE_ENUM_VALUE(LassoPolicy, THRU_BBOX);
    SO_NODE_DEFINE_ENUM_VALUE(LassoPolicy, ENTIRE_BBOX);

    // Set up info in enumerated type fields
    SO_NODE_SET_SF_ENUM_TYPE(atomLassoPolicy, LassoPolicy);
    SO_NODE_SET_SF_ENUM_TYPE(bondLassoPolicy, LassoPolicy);
    SO_NODE_SET_SF_ENUM_TYPE(atomLabelLassoPolicy, LassoPolicy);
    SO_NODE_SET_SF_ENUM_TYPE(bondLabelLassoPolicy, LassoPolicy);
    SO_NODE_SET_SF_ENUM_TYPE(chemLabelLassoPolicy, LassoPolicy);
    SO_NODE_SET_SF_ENUM_TYPE(chemMonitorLassoPolicy, LassoPolicy);
// --> residue selection
		SO_NODE_SET_SF_ENUM_TYPE(residueLassoPolicy, LassoPolicy);
		SO_NODE_SET_SF_ENUM_TYPE(residueLabelLassoPolicy, LassoPolicy);
// <-- residue selection

    // Enum for how to draw the radiusSelect sphere
    SO_NODE_DEFINE_ENUM_VALUE(RadiusSelectSphereDrawStyle, FILLED);
    SO_NODE_DEFINE_ENUM_VALUE(RadiusSelectSphereDrawStyle, LINES);
    SO_NODE_DEFINE_ENUM_VALUE(RadiusSelectSphereDrawStyle, POINTS);

    // Set up radiusSelectSphereDrawStyle
    SO_NODE_SET_SF_ENUM_TYPE(radiusSelectSphereDrawStyle,
                             RadiusSelectSphereDrawStyle);


    //////////////////////////////////////////////////
    //
    // Defaults for non-fields

#ifndef __nogl
    viewer = NULL;
#endif
    lassoActive = FALSE;
    currentEvent = NULL;

    // selection callback functions and user data
    selCBList = NULL;
    deselCBList = NULL;
    startCBList = NULL;
    finishCBList = NULL;  
    lassoStartCBList = NULL;
    lassoFinishCBList = NULL;  
    changeCBList = NULL;
    
    pickCBFunc = NULL;
    pickCBData = NULL;
    
    mouseDownPickPath = NULL;
    pickMatching = TRUE;

    //////////////////////////////////////////////////
    //
    // Local sceneGraph for rendering of the lasso geometry into
    // the overlay plane.
    //
    lassoRoot = new SoSeparator;
    lassoRoot->ref();

    // Create an orthoGraphic camera
    orthoCam = new SoOrthographicCamera;
    orthoCam->position.setValue(0.0, 0.0, 1.0);
    orthoCam->nearDistance = 0.5;
    lassoRoot->addChild(orthoCam);

#ifdef IV2_0
    // Add the lasso coordinates
    lassoRoot->addChild(coords);
#endif

    // Add a pickStyle to make the lasso UNPICKABLE
    SoPickStyle *ps = new SoPickStyle;
    ps->style = SoPickStyle::UNPICKABLE;
    lassoRoot->addChild(ps);

    // Add a colorIndex
    lassoColorIndex = new SoColorIndex;
    lassoColorIndex->index = lassoLineColorIndex.getValue();
    lassoRoot->addChild(lassoColorIndex);

    // Set the drawStyle of the lasso
    lassoDrawStyle = new SoDrawStyle;
    lassoDrawStyle->style = SoDrawStyle::LINES;
    lassoDrawStyle->lineWidth = lassoLineWidth.getValue();
    lassoDrawStyle->linePattern = lassoLinePattern.getValue();
    lassoRoot->addChild(lassoDrawStyle);

    // Add a lineSet
#ifdef IV2_0
    SoLineSet *ls = new SoLineSet;
    ls->startIndex = 0;
    ls->numVertices = SO_LINE_SET_USE_REST_OF_VERTICES;
    lassoRoot->addChild(ls);
#else
    SoLineSet *ls = new SoLineSet;
    ls->startIndex = 0;
    ls->vertexProperty = (SoNode *)vertexProperty;
    lassoRoot->addChild(ls);
#endif

    //////////////////////////////////////////////////
    //
    // Local sceneGraph for rendering of the radius selection
    // sphere and text.
    //
    // Switch to display the sphere scene graph or not.
    sceneSwitch = new SoSwitch;
    sceneSwitch->ref();

    // Make the switch a child of this ChemSelection node.
    addChild(sceneSwitch);

    // Turn off the display of the sphere scene graph.
    sceneSwitch->whichChild.setValue(SO_SWITCH_NONE);

    // Make the sphereRoot portion of the scene graph.
    sphereRoot = new SoSeparator;
    sceneSwitch->addChild(sphereRoot);

    // Make things UNPICKABLE
    SoPickStyle *ps2 = new SoPickStyle;
    ps2->style = SoPickStyle::UNPICKABLE;
    sphereRoot->addChild(ps2);

    // Add a Material so we can change color and transparency
    sphereMaterial = new SoMaterial;
    sphereRoot->addChild(sphereMaterial);
    sphereMaterial->diffuseColor = radiusSelectSphereColor.getValue();
    sphereMaterial->transparency = radiusSelectSphereTransparency.getValue();

    // Add a DrawStyle
    sphereDrawStyle = new SoDrawStyle;
    sphereRoot->addChild(sphereDrawStyle);
    sphereDrawStyle->style = radiusSelectSphereDrawStyle.getValue();

    // Add a Complexity
    sphereComplexity = new SoComplexity;
    sphereRoot->addChild(sphereComplexity);
    sphereComplexity->value = radiusSelectSphereComplexity.getValue();

    // Add a Transform so we can position the sphere on an atom
    sphereTransform = new SoTransform;
    sphereRoot->addChild(sphereTransform);

    // Add the sphere
    sphere = new SoSphere;
    sphereRoot->addChild(sphere);

    // Add a Material so we can change the color of the label
    labelMaterial = new SoMaterial;
    sphereRoot->addChild(labelMaterial);
    labelMaterial->diffuseColor = radiusSelectLabelColor.getValue();

    labelFont = new SoFont;
    sphereRoot->addChild(labelFont);
    labelFont->name = radiusSelectLabelFontName.getValue();
    labelFont->size = radiusSelectLabelFontSize.getValue();

    // Add a Transform so we can position the label
    labelTransform = new SoTransform;
    sphereRoot->addChild(labelTransform);

    labelText = new SoText2;
    sphereRoot->addChild(labelText);

    // Set up sensors for lasso and radiusSelect fields
    lassoLineWidthSensor = new SoFieldSensor(selectSensorCB, this);
    lassoLineWidthSensor->setPriority(0);
    lassoLineWidthSensor->attach(&lassoLineWidth);

    lassoLinePatternSensor = new SoFieldSensor(selectSensorCB, this);
    lassoLinePatternSensor->setPriority(0);
    lassoLinePatternSensor->attach(&lassoLinePattern);

    lassoLineColorIndexSensor = new SoFieldSensor(selectSensorCB, this);
    lassoLineColorIndexSensor->setPriority(0);
    lassoLineColorIndexSensor->attach(&lassoLineColorIndex);

    radiusSelectSphereColorSensor = new SoFieldSensor(selectSensorCB, this);
    radiusSelectSphereColorSensor->setPriority(0);
    radiusSelectSphereColorSensor->attach(&radiusSelectSphereColor);

    radiusSelectSphereTransparencySensor =
        new SoFieldSensor(selectSensorCB, this);
    radiusSelectSphereTransparencySensor->setPriority(0);
    radiusSelectSphereTransparencySensor->
        attach(&radiusSelectSphereTransparency);

    radiusSelectSphereDrawStyleSensor = new SoFieldSensor(selectSensorCB, this);
    radiusSelectSphereDrawStyleSensor->setPriority(0);
    radiusSelectSphereDrawStyleSensor->attach(&radiusSelectSphereDrawStyle);

    radiusSelectSphereComplexitySensor =
        new SoFieldSensor(selectSensorCB, this);
    radiusSelectSphereComplexitySensor->setPriority(0);
    radiusSelectSphereComplexitySensor->attach(&radiusSelectSphereComplexity);

    radiusSelectLabelColorSensor = new SoFieldSensor(selectSensorCB, this);
    radiusSelectLabelColorSensor->setPriority(0);
    radiusSelectLabelColorSensor->attach(&radiusSelectLabelColor);

    radiusSelectLabelFontNameSensor = new SoFieldSensor(selectSensorCB, this);
    radiusSelectLabelFontNameSensor->setPriority(0);
    radiusSelectLabelFontNameSensor->attach(&radiusSelectLabelFontName);

    radiusSelectLabelFontSizeSensor = new SoFieldSensor(selectSensorCB, this);
    radiusSelectLabelFontSizeSensor->setPriority(0);
    radiusSelectLabelFontSizeSensor->attach(&radiusSelectLabelFontSize);

    isBuiltIn = TRUE;

    if (SO_NODE_IS_FIRST_INSTANCE()) {
        float theta = 0.0f;
        float dTheta = (float)(2.0 * M_PI / RINGSIDES);
        for (int side = 0; side < RINGSIDES; side++) {
            ringCoords[side].setValue(sin(theta), -cos(theta));
            theta += dTheta;
        }
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Constructors
//
// Use: public

ChemSelection::ChemSelection()
{
    constructorCommon();
}

ChemSelection::ChemSelection(int32_t numChildren) : SoSeparator(numChildren+1)
{
    constructorCommon();
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Destructor.
//
// Use: protected

ChemSelection::~ChemSelection()
//
////////////////////////////////////////////////////////////////////////
{
    delete selCBList;
    delete deselCBList;
    delete startCBList;
    delete finishCBList;  
    delete lassoStartCBList;
    delete lassoFinishCBList;  
    delete changeCBList;
    
    if (mouseDownPickPath != NULL) {
        mouseDownPickPath->unref();
    }

    lassoRoot->unref();
    sceneSwitch->unref();
    SoGroup::removeChild(sceneSwitch);
    getLassoPoints()->unref();

    // Delete the SoFieldSensor's
    delete lassoLineWidthSensor;
    delete lassoLinePatternSensor;
    delete lassoLineColorIndexSensor;
    delete radiusSelectSphereColorSensor;
    delete radiusSelectSphereTransparencySensor;
    delete radiusSelectSphereDrawStyleSensor;
    delete radiusSelectSphereComplexitySensor;
    delete radiusSelectLabelColorSensor;
    delete radiusSelectLabelFontNameSensor;
    delete radiusSelectLabelFontSizeSensor;
}

////////////////////////////////////////////////////////////////////////
//    
// Description:
//    This adds a child as the last one in the group.
//
// Use: public

void
ChemSelection::addChild(SoNode *child)
//    
////////////////////////////////////////////////////////////////////////
{
    SoGroup::addChild(child);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    This inserts a child into a group so that it will have the given
//    index.
//
// Use: public

void
ChemSelection::insertChild(SoNode *child, int newChildIndex)
//    
////////////////////////////////////////////////////////////////////////
{
    SoGroup::insertChild(child, newChildIndex+1);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Returns child with the given index
//
// Use: public

SoNode *
ChemSelection::getChild(int index) const
//    
////////////////////////////////////////////////////////////////////////
{
    return SoGroup::getChild(index+1);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Returns the first index (starting with 0) of a child that
//    matches the given node pointer, or -1 if no such child is found.
//
// Use: public

int
ChemSelection::findChild(const SoNode *child) const
//    
////////////////////////////////////////////////////////////////////////
{
    return SoGroup::findChild(child) - 1;
}

////////////////////////////////////////////////////////////////////////
//  
// Description:
//    Returns number of children
//
// Use: public

int
ChemSelection::getNumChildren() const
//    
////////////////////////////////////////////////////////////////////////
{
// --> Coin2 compatibility
//    return SoGroup::getNumChildren() - 1;
		return SoGroup::getNumChildren();
// <-- Coin2 compatibility
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Removes the child with the given index from the group.
//
// Use: public

void
ChemSelection::removeChild(int index)
//    
////////////////////////////////////////////////////////////////////////
{
    SoGroup::removeChild(index+1);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Removes the child from the group.
//
// Use: public

void
ChemSelection::removeChild(SoNode *child)
//
////////////////////////////////////////////////////////////////////////
{
    SoGroup::removeChild(child);
}

////////////////////////////////////////////////////////////////////////
// 
// Description:
//    Removes all children from the group.
// 
// Use: public

void
ChemSelection::removeAllChildren()
//    
////////////////////////////////////////////////////////////////////////
{
// --> Coin2 compatibility
// First node: sceneSwitch
//    int lastChild = getNumChildren();
    int lastChild = getNumChildren()-1;
// <-- Coin2 compatibility
    for (int i = lastChild; i >= 1; i--) {
        SoGroup::removeChild(i);
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Replaces child with given index with new child.
// 
// Use: public

void
ChemSelection::replaceChild(int index, SoNode *newChild)
//    
////////////////////////////////////////////////////////////////////////
{
    SoGroup::replaceChild(index+1, newChild);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Replaces old child with new child.
// 
// Use: public

void
ChemSelection::replaceChild(SoNode *oldChild, SoNode *newChild)
//
////////////////////////////////////////////////////////////////////////
{
    SoGroup::replaceChild(oldChild, newChild);
}

////////////////////////////////////////////////////////////////////////
// 
// Description:
//    Turn off notification on fields to avoid notification when
//    reading, so that caching works properly:
//
// Use: protected

#ifdef IV2_0
ChemSelection::readInstance(SoInput *in)
#else
SbBool
ChemSelection::readInstance(SoInput *in, unsigned short flags)
#endif
//
////////////////////////////////////////////////////////////////////////
{
    SoFieldList myFields;
    getFields(myFields);
    // int i = 0 put out of the for loop by fabien fontaine the 15/12/2000
    // to avoid compiler warning
    int i = 0 ;
    for ( ; i < myFields.getLength(); i++) {
        if (myFields[i] == &renderCaching ||
            myFields[i] == &boundingBoxCaching ||
            myFields[i] == &renderCulling ||
            myFields[i] == &pickCulling) {

            myFields[i]->enableNotify(FALSE);
        }
    }

#ifdef IV2_0
    SbBool result = SoGroup::readInstance(in);
#else
    SbBool result = SoGroup::readInstance(in, flags); 
#endif

    for (i = 0; i < myFields.getLength(); i++) {
        myFields[i]->enableNotify(TRUE);
    }

    return result;  
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Implements pick action.
//
// Use: extender
        
void
ChemSelection::pick(SoPickAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    ChemSelectionElement::set(action->getState(),
        (ChemSelectionElement::ChemPart)(parts.getValue()));
    SoGroup::pick(action);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Implements write action.
//
// Use: extender
        
void
ChemSelection::write(SoWriteAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    SoGroup::removeChild(sceneSwitch);
    SoGroup::write(action);
    SoGroup::insertChild(sceneSwitch, 0);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Does the getBoundingBox action
//
// Use: extender

void
ChemSelection::getBoundingBox(SoGetBoundingBoxAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    ChemDisplaySelectionElement::set(action->getState(), displaySelectionList);
    ChemLabelSelectionElement::set(action->getState(), labelSelectionList);
    ChemMonitorSelectionElement::set(action->getState(), monitorSelectionList);
    SoSeparator::getBoundingBox(action);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Does the GL render action
//
// Use: extender
 
void
ChemSelection::GLRender(SoGLRenderAction *action)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("Setting ChemDisplaySelectionElement in GLRender\n");
#endif
    SoState *state = action->getState();

    currentModelMatrix = SoModelMatrixElement::get(state);
    currentViewVolume = SoViewVolumeElement::get(state);
    currentViewMatrix = SoViewingMatrixElement::get(state);

    ChemDisplaySelectionElement::set(state, displaySelectionList);
    ChemLabelSelectionElement::set(action->getState(), labelSelectionList);
    ChemMonitorSelectionElement::set(action->getState(), monitorSelectionList);
    SoSeparator::GLRender(action);
}

#ifndef IV2_0
////////////////////////////////////////////////////////////////////////
//
// Description:
//    Does the GL render action
//
// Use: extender

void
ChemSelection::GLRenderBelowPath(SoGLRenderAction *action)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("Setting ChemDisplaySelectionElement in GLRenderBelowPath\n");
#endif
    SoState *state = action->getState();

    currentModelMatrix = SoModelMatrixElement::get(state);
    currentViewVolume = SoViewVolumeElement::get(state);
    currentViewMatrix = SoViewingMatrixElement::get(state);

    ChemDisplaySelectionElement::set(state, displaySelectionList);
    ChemLabelSelectionElement::set(action->getState(), labelSelectionList);
    ChemMonitorSelectionElement::set(action->getState(), monitorSelectionList);
    SoSeparator::GLRenderBelowPath(action);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Does the GL render action 
//
// Use: extender

void
ChemSelection::GLRenderInPath(SoGLRenderAction *action)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("Setting ChemDisplaySelectionElement in GLRenderInPath\n");
#endif
    SoState *state = action->getState();

    currentModelMatrix = SoModelMatrixElement::get(state);
    currentViewVolume = SoViewVolumeElement::get(state);
    currentViewMatrix = SoViewingMatrixElement::get(state);

    ChemDisplaySelectionElement::set(state, displaySelectionList);
    ChemLabelSelectionElement::set(action->getState(), labelSelectionList);
    ChemMonitorSelectionElement::set(action->getState(), monitorSelectionList);
    SoSeparator::GLRenderInPath(action);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Does the GL render action
//
// Use: extender   

void
ChemSelection::GLRenderOffPath(SoGLRenderAction *action)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("Setting ChemDisplaySelectionElement in GLRenderOffPath\n");
#endif
    ChemDisplaySelectionElement::set(action->getState(), displaySelectionList);
    ChemLabelSelectionElement::set(action->getState(), labelSelectionList);
    ChemMonitorSelectionElement::set(action->getState(), monitorSelectionList);
    SoSeparator::GLRenderOffPath(action);
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
// Description:
//    Add this path to the selection list.  Path should include this
//    node. The path will be referenced by the ChemPathList (SoBaseList) class.
//    A touch is performed in order to update the elements.
//
// Use: public

void
ChemSelection::select(const ChemPath *chemPath)
//
//////////////////////////////////////////////////////////////////////////////
{
    // Deselect everything.  Don't use deselectAll since it performs a
    // touch as does merge.

    int32_t i;
    if (chemPath->isOfType(ChemDisplayPath::getClassTypeId())) {
        for (i = displaySelectionList.getLength() - 1; i >= 0; i--) {
            removeDisplayPath(i);
        }
    }
    else if (chemPath->isOfType(ChemLabelPath::getClassTypeId())) {
        for (i = labelSelectionList.getLength() - 1; i >= 0; i--) {
            removeLabelPath(i);
        }
    }
    else if (chemPath->isOfType(ChemMonitorPath::getClassTypeId())) {
        for (i = monitorSelectionList.getLength() - 1; i >= 0; i--) {
            removeMonitorPath(i);
        }
    }
    merge(chemPath);
}

//////////////////////////////////////////////////////////////////////////////
//
// Description:
//    Remove a path by finding the path and then removing it by index.
//    A touch is performed in order to update the elements.
//
//  Use: public

void
ChemSelection::deselect(ChemPath *chemPath)
//
//////////////////////////////////////////////////////////////////////////////
{
    if (chemPath != NULL) {
        if (chemPath->isOfType(ChemDisplayPath::getClassTypeId())) {
            displaySelectionList.removePath((ChemDisplayPath *)chemPath);
        }
        else if (chemPath->isOfType(ChemLabelPath::getClassTypeId())) {
            labelSelectionList.removePath((ChemLabelPath *)chemPath);
        }
        else if (chemPath->isOfType(ChemMonitorPath::getClassTypeId())) {
            monitorSelectionList.removePath((ChemMonitorPath *)chemPath);
        }

        if (deselCBList != NULL)
            deselCBList->invokeCallbacks(chemPath);
        
        if (changeCBList != NULL)
            changeCBList->invokeCallbacks(this);

        touch();
    }

#ifdef DEBUG
    else {
        SoDebugError::post("ChemSelection::deselect", "Passed path is NULL");
    }
#endif
}

//////////////////////////////////////////////////////////////////////////////
//
// Description:
//    Toggle adds path if it's not in the SelectionList, removes path if it is.
//    A touch is performed in order to update the elements.
//
//  Use: public

void
ChemSelection::toggle(const ChemPath *path)
//
//////////////////////////////////////////////////////////////////////////////
{
    // get a path whose head is this selection node
    ChemPath *selPath = copyFromThis(path);
    if ((selPath != NULL) && (selPath->path->getLength() > 1)) {
        selPath->ref();
        addPath(selPath, TRUE);
        selPath->unref();
        touch();
    }
#ifdef DEBUG
    else {
        SoDebugError::post("ChemSelection::toggle",
        "Path does not pass through this selection node.");
    }
#endif
}

//////////////////////////////////////////////////////////////////////////////
//
// Description:
//    Merge adds path if it's not in the SelectionList, leaves it alone if 
//    it is.  A touch is performed in order to update the elements.
//
//  Use: public

void
ChemSelection::merge(const ChemPath *path)
//
//////////////////////////////////////////////////////////////////////////////
{
    // get a path whose head is this selection node
    ChemPath *selPath = copyFromThis(path);
    if ((selPath != NULL) && (selPath->path->getLength() > 1)) {
        selPath->ref();
        addPath(selPath, FALSE);
        selPath->unref();
        touch();
    }
#ifdef DEBUG
    else {
        SoDebugError::post("ChemSelection::merge",
        "Path does not pass through this selection node.");
    }
#endif
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Removes every path in the selection list.
//    A touch is performed in order to update the elements.
//
// Use: public

void
ChemSelection::deselectAll()
//
////////////////////////////////////////////////////////////////////////
{
    int32_t i;
    for (i = displaySelectionList.getLength() - 1; i >= 0; i--) {
        removeDisplayPath(i);
    }
    for (i = labelSelectionList.getLength() - 1; i >= 0; i--) {
        removeLabelPath(i);
    }
    for (i = monitorSelectionList.getLength() - 1; i >= 0; i--) {
        removeMonitorPath(i);
    }
    touch();
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Removes every path in the display selection list.
//    A touch is performed in order to update the elements.
//
// Use: public

void
ChemSelection::deselectAllDisplays()
//
////////////////////////////////////////////////////////////////////////
{
    int32_t i;
    for (i = displaySelectionList.getLength() - 1; i >= 0; i--) {
        removeDisplayPath(i);
    }
    touch();
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Removes every path in the label selection list.
//    A touch is performed in order to update the elements.
//
// Use: public

void
ChemSelection::deselectAllLabels()
//
////////////////////////////////////////////////////////////////////////
{
    int32_t i;
    for (i = labelSelectionList.getLength() - 1; i >= 0; i--) {
        removeLabelPath(i);
    }
    touch();
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Removes every path in the monitor selection list.
//    A touch is performed in order to update the elements.
//
// Use: public

void
ChemSelection::deselectAllMonitors()
//
////////////////////////////////////////////////////////////////////////
{
    int32_t i;
    for (i = monitorSelectionList.getLength() - 1; i >= 0; i--) {
        removeMonitorPath(i);
    }
    touch();
}

//////////////////////////////////////////////////////////////////////////////
//
// Description:
//    Returns TRUE if the SoPath portion of the ChemPath has already been
//    selected.
//
// Use: public

SbBool
ChemSelection::isSelected(const ChemPath *path) const
//
//////////////////////////////////////////////////////////////////////////////
{
    return (findPath(path) != -1);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Search for the passed node under this, then return whether it
//    is selected.
//
// Use: public

SbBool
ChemSelection::isSelected(SoNode *node) const
//
////////////////////////////////////////////////////////////////////////
{
    SbBool itIs = FALSE;
    
    if (node != NULL) {
        node->ref();
        
        if (searchAction == NULL)
            searchAction = new SoSearchAction;
        else
            searchAction->reset();
        searchAction->setInterest(SoSearchAction::FIRST);
        searchAction->setFind(SoSearchAction::NODE);
        searchAction->setNode(node);
        searchAction->apply((ChemSelection *) this);
        if (searchAction->getPath() != NULL) {
            ChemPath *chemPath = new ChemPath;
            chemPath->ref();
            chemPath->setSoPath(searchAction->getPath());
            itIs = isSelected(chemPath);
            chemPath->unref();
        }
        
        node->unref();
    }
    
    return itIs;
}

//////////////////////////////////////////////////////////////////////////////
//
// Description:
//    Get the i'th path in the displaySelectionList.
//
// Use: public

ChemDisplayPath *
ChemSelection::getDisplayPath(int32_t index) const
//
//////////////////////////////////////////////////////////////////////////////
{
    ChemDisplayPath *p;
    
#ifdef DEBUG
    if ((index >= displaySelectionList.getLength()) || (index < 0)) {
        SoDebugError::post("ChemSelection::getDisplayPath", "Index out of range.  Index = %d, numSelected = %d",
            index, displaySelectionList.getLength());
        p = NULL;
    }
    else
#endif
        p = (ChemDisplayPath *)displaySelectionList[index];
        
    return p;
}

//////////////////////////////////////////////////////////////////////////////
//
// Description:
//    Get the i'th path in the labelSelectionList.
//
// Use: public

ChemLabelPath *
ChemSelection::getLabelPath(int32_t index) const
//
//////////////////////////////////////////////////////////////////////////////
{
    ChemLabelPath *p;
    
#ifdef DEBUG
    if ((index >= labelSelectionList.getLength()) || (index < 0)) {
        SoDebugError::post("ChemSelection::getLabelPath", "Index out of range.  Index = %d, numSelected = %d",
            index, labelSelectionList.getLength());
        p = NULL;
    }
    else
#endif
        p = (ChemLabelPath *)labelSelectionList[index];
        
    return p;
}

//////////////////////////////////////////////////////////////////////////////
//
// Description:
//    Get the i'th path in the monitorSelectionList.
//
// Use: public

ChemMonitorPath *
ChemSelection::getMonitorPath(int32_t index) const
//
//////////////////////////////////////////////////////////////////////////////
{
    ChemMonitorPath *p;
    
#ifdef DEBUG
    if ((index >= monitorSelectionList.getLength()) || (index < 0)) {
        SoDebugError::post("ChemSelection::getMonitorPath", "Index out of range.  Index = %d, numSelected = %d",
            index, monitorSelectionList.getLength());
        p = NULL;
    }
    else
#endif
        p = (ChemMonitorPath *)monitorSelectionList[index];
        
    return p;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Methods for setting callback functions.
//
// Use: public
//
//
////////////////////////////////////////////////////////////////////////

void
ChemSelection::addSelectionCallback(ChemSelectionPathCB *f, void *userData)
{
    if (selCBList == NULL)
        selCBList = new SoCallbackList;
    selCBList->addCallback((SoCallbackListCB *) f, userData);
}
void
ChemSelection::removeSelectionCallback(ChemSelectionPathCB *f, void *userData)
{
    if (selCBList != NULL)
        selCBList->removeCallback((SoCallbackListCB *) f, userData);
}

void
ChemSelection::addDeselectionCallback(ChemSelectionPathCB *f, void *userData)
{
    if (deselCBList == NULL)
        deselCBList = new SoCallbackList;
    deselCBList->addCallback((SoCallbackListCB *) f, userData);
}
void
ChemSelection::removeDeselectionCallback(ChemSelectionPathCB *f, void *userData)
{
    if (deselCBList != NULL)
        deselCBList->removeCallback((SoCallbackListCB *) f, userData);
}

void
ChemSelection::addStartCallback(ChemSelectionClassCB *f, void *userData)
{
    if (startCBList == NULL)
        startCBList = new SoCallbackList;
    startCBList->addCallback((SoCallbackListCB *) f, userData);
}
void
ChemSelection::removeStartCallback(ChemSelectionClassCB *f, void *userData)
{
    if (startCBList != NULL)
        startCBList->removeCallback((SoCallbackListCB *) f, userData);
}

void
ChemSelection::addFinishCallback(ChemSelectionClassCB *f, void *userData)
{
    if (finishCBList == NULL)
        finishCBList = new SoCallbackList;
    finishCBList->addCallback((SoCallbackListCB *) f, userData);
}
void
ChemSelection::removeFinishCallback(ChemSelectionClassCB *f, void *userData)
{
    if (finishCBList != NULL)
        finishCBList->removeCallback((SoCallbackListCB *) f, userData);
}

void
ChemSelection::addLassoStartCallback(ChemSelectionClassCB *f, void *userData)
{
    if (lassoStartCBList == NULL)
        lassoStartCBList = new SoCallbackList;
    lassoStartCBList->addCallback((SoCallbackListCB *) f, userData);
}
void
ChemSelection::removeLassoStartCallback(ChemSelectionClassCB *f, void *userData)
{
    if (lassoStartCBList != NULL)
        lassoStartCBList->removeCallback((SoCallbackListCB *) f, userData);
}

void
ChemSelection::addLassoFinishCallback(ChemSelectionClassCB *f, void *userData)
{
    if (lassoFinishCBList == NULL)
        lassoFinishCBList = new SoCallbackList;
    lassoFinishCBList->addCallback((SoCallbackListCB *) f, userData);
}
void
ChemSelection::removeLassoFinishCallback(ChemSelectionClassCB *f,
    void *userData)
{
    if (lassoFinishCBList != NULL)
        lassoFinishCBList->removeCallback((SoCallbackListCB *) f, userData);
}

void
ChemSelection::addChangeCallback(ChemSelectionClassCB *f, void *userData)
{
    if (changeCBList == NULL)
        changeCBList = new SoCallbackList;
    changeCBList->addCallback((SoCallbackListCB *) f, userData);
}
void
ChemSelection::removeChangeCallback(ChemSelectionClassCB *f, void *userData)
{
    if (changeCBList != NULL)
        changeCBList->removeCallback((SoCallbackListCB *) f, userData);
}

void
ChemSelection::setPickFilterCallback(
    ChemSelectionPickCB *f,
    void *userData, 
    SbBool callOnlyIfSelectable)
{
    pickCBFunc = f;
    pickCBData = userData;
    callPickCBOnlyIfSelectable = callOnlyIfSelectable;
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Save the currentEvent in case its needed and invokes reallyHandleEvent
//    to actually handle the event.
//
// Use: protected

void
ChemSelection::handleEvent(SoHandleEventAction *action)
//
////////////////////////////////////////////////////////////////////////
{    
    // Save event in case it's needed
    currentEvent = action->getEvent();

    reallyHandleEvent(action);   
    
    // We no longer have an event
    currentEvent = NULL;
}

////////////////////////////////////////////////////////////////////////
// 
// Description:
//    If a change was made to a ChemBaseData or ChemDisplay node, then
//    the displaySelectionList is no longer valid.  This resets the displaySelectionList
//    if necessary and invokes SoSeparator::notify.
//    
// Use: internal, public

void
ChemSelection::notify(SoNotList *list)
//
////////////////////////////////////////////////////////////////////////
{
    int32_t i;
    // See if the notification is due to a child notifying its parent.
    if ((list->getFirstRecAtNode()->getType() == SoNotRec::PARENT)) {
        // If the child is of type ChemBaseData or of type ChemDisplay, 
        // find all ChemDisplayPaths in displaySelectionList which contain the 
        // node and deselect those ChemDisplayPaths.
        SoBase *base = list->getFirstRecAtNode()->getBase();
        if ((base->isOfType(ChemBaseData::getClassTypeId())) ||
            (base->isOfType(ChemDisplay::getClassTypeId()))) {
            // Check if the displaySelectionList has anything in it.
            if (displaySelectionList.getLength() > 0) {
                if (searchAction == NULL) {
                    searchAction = new SoSearchAction;
                }
                else {
                    searchAction->reset();
                }
                searchAction->setInterest(SoSearchAction::FIRST);
                searchAction->setFind(SoSearchAction::NODE);
                searchAction->setNode((SoNode *)base);
                for(i = displaySelectionList.getLength() - 1; i >= 0; i--) {
                    searchAction->apply(displaySelectionList[i]->path);
                    if (searchAction->getPath() != NULL) {
                        removeDisplayPath(i);
                    }
                }
            }
        }
        // If the child is of type ChemLabel, 
        // find all ChemLabelPaths in labelSelectionList which contain the 
        // node and deselect those ChemLabelPaths.
        else if (base->isOfType(ChemLabel::getClassTypeId())) {
            // Check if the labelSelectionList has anything in it.
            if (labelSelectionList.getLength() > 0) {
                if (searchAction == NULL) {
                    searchAction = new SoSearchAction;
                }
                else {
                    searchAction->reset();
                }
                searchAction->setInterest(SoSearchAction::FIRST);
                searchAction->setFind(SoSearchAction::NODE);
                searchAction->setNode((SoNode *)base);
                for(i = labelSelectionList.getLength() - 1; i >= 0; i--) {
                    searchAction->apply(labelSelectionList[i]->path);
                    if (searchAction->getPath() != NULL) {
                        removeLabelPath(i);
                    }
                }
            }
        }
        else if (base->isOfType(ChemMonitor::getClassTypeId())) {
            // Check if the monitorSelectionList has anything in it.
            if (monitorSelectionList.getLength() > 0) {
                if (searchAction == NULL) {
                    searchAction = new SoSearchAction;
                }
                else {
                    searchAction->reset();
                }
                searchAction->setInterest(SoSearchAction::FIRST);
                searchAction->setFind(SoSearchAction::NODE);
                searchAction->setNode((SoNode *)base);
                for(i = monitorSelectionList.getLength() - 1; i >= 0; i--) {
                    searchAction->apply(monitorSelectionList[i]->path);
                    if (searchAction->getPath() != NULL) {
                        removeMonitorPath(i);
                    }
                }
            }
        }
    }
    SoSeparator::notify(list);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Invoke SoSeparator's handleEvent and the handles the lasso or radius
//    selection.  It invokes action->setHandled if appropriate.
//
// Use: private
    
void
ChemSelection::reallyHandleEvent(SoHandleEventAction *action)
//  
////////////////////////////////////////////////////////////////////////
{
    // Let SoSeparator traverse the children
    SoSeparator::handleEvent(action);

    // If any children handled the event, then return.  This will include
    // manipulators and draggers, which will also invoke setGrabber.  We
    // could also check getGrabber != NULL or getGrabber != this.
    if (action->isHandled()) return;

    const SoEvent *event = action->getEvent();

    // On Button1 press, see who got picked.
    // On Button1 release, see who got picked.
    // If pick-matching is on, and they match,
    // then select the object that got picked.

    SbBool shiftDown = FALSE;
    SbBool ctrlDown  = FALSE;
    if (event->isOfType(SoMouseButtonEvent::getClassTypeId())) {
        const SoMouseButtonEvent *be = (const SoMouseButtonEvent *) event;

        if (be->getButton() != SoMouseButtonEvent::BUTTON1)
            return;

        shiftDown = be->wasShiftDown();
        ctrlDown = be->wasCtrlDown();
        const SoPickedPoint *pickedPoint = action->getPickedPoint();
        ChemPath *pickChemPath = NULL;
        if (be->getState() == SoButtonEvent::DOWN ||
            be->getState() == SoButtonEvent::UP) {

            // Get the pick path
            if (pickedPoint != NULL) {
                // If the pick callback exists, let it tell us what path to 
                // pick
                if (pickCBFunc != NULL) {
                    if (callPickCBOnlyIfSelectable) {
                        // Make sure pick passes through this
                        if (pickedPoint->getPath()->containsNode(this))
                            pickChemPath = (*pickCBFunc)(pickCBData, pickedPoint);
                    }
                    else pickChemPath = (*pickCBFunc)(pickCBData, pickedPoint);
                }
                // Else no pick CB - use the picked path as is.
                // processPickedPoint will return NULL if the last node in
                // the path is not a ChemDisplay.
                else {
                    pickChemPath = processPickedPoint(pickedPoint);
                }
            }
        }

        if (be->getState() == SoButtonEvent::DOWN) {

//
// this checking would require to include the SoXt component
// since the SGI STEREO extensions are not universaly valid for
// the platforms used by SoQt we would remove this part of the code
// when SoQt is used instead of SoXt
//
// MPM 11/2001
//
			float  xResOffset = 1.0;
#ifndef __nogl
            // See if the viewer is set.  If so, determine if the
            // viewer is currently using the SGI stereo extensions to X.
            if (viewer != NULL) {
                // See if using a stereo buffer (stereo-in-a-window)
#ifdef __sgi
                if (!viewer->isStereoBuffer()) {
                    int first_event, first_error;
                    if (XSGIStereoQueryExtension(viewer->getDisplay(),
                        &first_event, &first_error)) {
                        int stereoMode =
                            XSGIQueryStereoMode(viewer->getDisplay(),
                                viewer->getNormalWindow());
                        if ((stereoMode == STEREO_BOTTOM) ||
                            (stereoMode == STEREO_TOP)) {
                            xResOffset = 2.0;
                        }
                    }
                }
#endif  // !__sgi
            }
#endif  // !__nogl

            // Set up orthoCam
            const SbViewVolume &viewVol =
                SoViewVolumeElement::get(action->getState());
            orthoCamXres = viewVol.getWidth() * xResOffset;
            orthoCamYres = viewVol.getHeight();
            orthoCam->height = orthoCamYres;
            orthoCam->aspectRatio = orthoCamXres / orthoCamYres;
            orthoCam->position.setValue(orthoCamXres * 0.5,
                                        orthoCamYres * 0.5, 1.0);

            if (pickMatching) {
                if (mouseDownPickPath != NULL) {
                    mouseDownPickPath->unref(); // out with the old...
                    mouseDownPickPath = NULL;
                }

                useRadiusSelection = FALSE;
                mouseDownPickPath = copyFromThis(pickChemPath);

                if ((mouseDownPickPath != NULL) &&
                    (mouseDownPickPath->isOfType(ChemDisplayPath::getClassTypeId()))) {

                    mouseDownPickPath->ref();   // ...in with the new
                    if ((radiusSelect.getValue() == TRUE) &&
                        (((ChemDisplayPath *)mouseDownPickPath)->getAtomIndex().getNum() > 0)) {
                        startRadiusSelection((ChemDisplayPath *)mouseDownPickPath);
                    }
                }
            }

#ifdef DEBUG
fprintf(stderr, "Starting lasso\n");
#endif
            // Turn off notification so that the user's scene graph will not
            // be traversed during the lasso.
            if (!useRadiusSelection) {
                enableNotify(FALSE);
            }
            if (lassoType.getValue() != ChemSelection::NOLASSO) {
#ifdef IV2_0
                getLassoPoints()->point.deleteValues(0);
#else
                getLassoPoints()->vertex.deleteValues(0);
#endif
                lassoBBox.makeEmpty();
                lassoActive = TRUE;  
                if (lassoStartCBList != NULL)
                    lassoStartCBList->invokeCallbacks(this);
            }
            return;
        }
        else if (be->getState() == SoButtonEvent::UP) {
            // Else button release.
            SbVec3f lassoMin, lassoMax;
            float lassoLength = 0.0;
            if (lassoType.getValue() != ChemSelection::NOLASSO) {
                if (!lassoBBox.isEmpty()) {
                    lassoMin = lassoBBox.getMin();
                    lassoMax = lassoBBox.getMax();
                    if (lassoMin != lassoMax) {
                        SoState *state = action->getState();
                        SbViewportRegion vpr;
                        SbMatrix viewMat, projMat;

                        vpr = SoViewportRegionElement::get(state);
                        SbViewVolume viewVol = orthoCam->getViewVolume();
                        viewVol.getMatrices(viewMat, projMat);

                        lassoMin[2] = lassoMax[2] = 0.0;
                        lassoMin =
                            fromObjectSpaceToPixels(lassoMin, projMat, vpr);
                        lassoMax =
                            fromObjectSpaceToPixels(lassoMax, projMat, vpr);
                        lassoMin[2] = lassoMax[2] = 0.0;
                        lassoLength = (lassoMax - lassoMin).length();
                    }
                }
            }

            // First check to see if the selection should be made by
            // the sphere selection mechanism
            if (useRadiusSelection &&
                selectedRadius >= radiusSelectCutoff.getValue()) {
                stopRadiusSelection();
                doRadiusPick(shiftDown, ctrlDown);
                if (lassoFinishCBList != NULL)
                    lassoFinishCBList->invokeCallbacks(this);
                action->setHandled();
            }

            // The sphere selection mechanism didn't want to handle it
            // so see if the lasso wants to deal with it.
            else if ((lassoType.getValue() != ChemSelection::NOLASSO) &&
                (lassoLength >= lassoSelectCutoff.getValue())) {
                doLassoPick(shiftDown, ctrlDown);
                if (lassoFinishCBList != NULL)
                    lassoFinishCBList->invokeCallbacks(this);
                action->setHandled();
            }
            else {
                // For button release, get the pick path and see if it matches
                // the button press pick path.
                //
                // If they match, invoke the selection policy.
                // If they do NOT match, do nothing.
            
                if (pickChemPath == NULL) {
                    // If nothing was picked, pass NULL to 
                    // invokeSelectionPolicy.
                    if ((! pickMatching) || (mouseDownPickPath == NULL)) {
                        invokeSelectionPolicy(NULL, shiftDown, ctrlDown, TRUE);
                    }
                    // else paths do not match - ignore event
                }
                else {
                    pickChemPath->ref();
                
                    if ((! pickMatching) || (mouseDownPickPath != NULL)) {
                        // Mouse down pick hit something.
                        // Was it same as mouse up pick?
                        // See if the pick path passes through this node
                        ChemPath *mouseUpPickPath = copyFromThis(pickChemPath);
                        if (mouseUpPickPath != NULL) {
                            mouseUpPickPath->ref();
                    
                            // If paths match, invoke the selection policy. 
                            if ((! pickMatching) ||
                                (*mouseDownPickPath == *mouseUpPickPath)) {
                                if (mouseUpPickPath->path->getLength() == 1) {
                                    // Path is to ONLY the selection node.
                                    // Act as if nothing were picked, but
                                    // allow traversal to continue.
                                    invokeSelectionPolicy(NULL, shiftDown,
                                                          ctrlDown, TRUE);
                                }
                                else {
                                    // Alter selection and halt traversal.
                                    invokeSelectionPolicy(mouseUpPickPath,
                                        shiftDown, ctrlDown, TRUE);
                                    action->setHandled();
                                }
                            }
                            // else paths do not match - ignore event
                        
                            mouseUpPickPath->unref();
                        }
                        // else path does not pass through this node -
                        // ignore event
                    }
                    // else paths do not match - ignore event
                
                    pickChemPath->unref();
                }
            
                // All done with mouse down pick path
                if (mouseDownPickPath != NULL) {
                    mouseDownPickPath->unref();
                    mouseDownPickPath = NULL;
                }
            }
            // button may be in unknown state, erase lasso and return
#ifdef IV2_0
            getLassoPoints()->point.deleteValues(0);
#else
            getLassoPoints()->vertex.deleteValues(0);
#endif

            lassoBBox.makeEmpty();
            lassoActive = FALSE;
            
            // Turn notification back on and apply a touch so that the scene
            // graph will be traversed with the new selection info in the
            // state.
            enableNotify(TRUE);
            touch();
            return;
        }
    }
    else if (event->isOfType(SoLocation2Event::getClassTypeId()) &&
             lassoActive) {
        addPointToLasso(action);
        action->setHandled();
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Invoke the appropriate routine to implement the current
//    selection policy. 
//
// Use: protected

void
ChemSelection::invokeSelectionPolicy(ChemPath *chemPath, SbBool shiftDown,
    SbBool ctrlDown, SbBool notify)
//
////////////////////////////////////////////////////////////////////////
{
    // employ the current selection policy
    switch (togglePolicy.getValue()) {
        case SINGLE:
            performSingleSelection(chemPath, notify);
            break;
        case TOGGLE:
            performToggleSelection(chemPath, FALSE, notify);
            break;
        case SHIFT:
            if (shiftDown) {
                 performToggleSelection(chemPath, ctrlDown, notify);
            }
            else {
                performSingleSelection(chemPath, notify); 
            }
            break;
        default:
#ifdef DEBUG
            SoDebugError::post("ChemSelection::invokeSelectionPolicy",
            "Unknown selection togglePolicy %d", togglePolicy.getValue());
#endif
            break;
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Clear the selection, then select the picked object.
//    If no object is picked, then nothing is selected.
//
// Use: protected

void
ChemSelection::performSingleSelection(ChemPath *chemPath, SbBool notify)
//
////////////////////////////////////////////////////////////////////////
{
    SbBool needFinishCB = FALSE;
    
    // let app know (if and only if) user is changing the selection
    if (notify) {
        if ((getNumDisplaysSelected() > 0) || 
            (getNumLabelsSelected() > 0) ||
            (getNumMonitorsSelected() > 0) ||
            (chemPath != NULL)) {
            if (startCBList != NULL)
                startCBList->invokeCallbacks(this);
            needFinishCB = TRUE;
        }
    }

    // Deselect everything.  Not using deselectAll since it does a "touch".
    int32_t i;
    for (i = displaySelectionList.getLength() - 1; i >= 0; i--) {
        removeDisplayPath(i);
    }
    for (i = labelSelectionList.getLength() - 1; i >= 0; i--) {
        removeLabelPath(i);
    }
    for (i = monitorSelectionList.getLength() - 1; i >= 0; i--) {
        removeMonitorPath(i);
    }
    if (chemPath != NULL) {
        addPath(chemPath, FALSE);
    }
    
    // let app know user is done changing the selection
    if (notify) {
        if (needFinishCB) {
            if (finishCBList != NULL)
                finishCBList->invokeCallbacks(this);
        }
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Depending on the value of ctrlDown, selection is made either by
//    toggling or by merging.  Toggling means that if an object is 
//    already selected, then it is deselected.  Merging means that if
//    it is already selected, it stays selected.
//
// Use: protected

void
ChemSelection::performToggleSelection(ChemPath *chemPath, SbBool ctrlDown,
    SbBool notify)
//
////////////////////////////////////////////////////////////////////////
{
    if (chemPath != NULL) {
        // let app know user is changing the selection
        if (startCBList != NULL)
            startCBList->invokeCallbacks(this);
        
        // toggle the picked object
        addPath(chemPath, !ctrlDown);

        // let app know user is done changing the selection
        if (finishCBList != NULL)
            finishCBList->invokeCallbacks(this);
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Find 'this' in the path, then copy the path from 'this' down to
//    the path tail. If 'this' is not in the path, return NULL.
//
// Use: private

ChemPath *
ChemSelection::copyFromThis(const ChemPath *chemPath) const
//
////////////////////////////////////////////////////////////////////////
{
    if (chemPath == NULL)
        return NULL;
        
    SoNode *node;
    int32_t i, indexToThis = -1;
    ChemPath *cp = NULL;

    SoFullPath *fullInPath = (SoFullPath *) chemPath->path;

    for (i = 0; i < fullInPath->getLength(); i++) {
        node = fullInPath->getNode(i);
	/* 
	   cast (SoNode *) added by fabien fontaine the 14/12/2000 to avoid 
	   compiler warning
	*/
        if (node == (SoNode *) this) {
            indexToThis = i;
            break;
        }
    }
    
    if (indexToThis != -1) {
        if (chemPath->isOfType(ChemDisplayPath::getClassTypeId())) {
            ChemDisplayPath *tcp = ((ChemDisplayPath *)chemPath)->copy();
            cp = tcp;
        }
        else if (chemPath->isOfType(ChemLabelPath::getClassTypeId())) {
            ChemLabelPath *tcp = ((ChemLabelPath *)chemPath)->copy();
            cp = tcp;
        }
        else if (chemPath->isOfType(ChemMonitorPath::getClassTypeId())) {
            ChemMonitorPath *tcp = ((ChemMonitorPath *)chemPath)->copy();
            cp = tcp;
        }
        if (cp != NULL) {
            cp->setSoPath(fullInPath->copy(indexToThis));
        }
    }

    return cp;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Add path to the selection list. It is assumed that path->head
//    is this - DO NOT CALL THIS FUNCTION unless this is true. Also, the
//    path must include more than just the selection node by itself.
//
// Use: protected

void
ChemSelection::addPath(ChemPath *chemPath, SbBool toggle)
//
////////////////////////////////////////////////////////////////////////
{
    SbBool changeNotify = FALSE;

    // Add the path to the selection list,
    // and notify the app that this path has been selected
    chemPath->ref();

    if (chemPath->isOfType(ChemDisplayPath::getClassTypeId())) {
        if (displaySelectionList.mergePath((ChemDisplayPath *)chemPath, toggle)) {
            changeNotify = TRUE;
        }
    }
    else if (chemPath->isOfType(ChemLabelPath::getClassTypeId())) {
        if (labelSelectionList.mergePath((ChemLabelPath *)chemPath, toggle)) {
            changeNotify = TRUE;
        }
    }
    else if (chemPath->isOfType(ChemMonitorPath::getClassTypeId())) {
        if (monitorSelectionList.mergePath((ChemMonitorPath *)chemPath, toggle)) {
            changeNotify = TRUE;
        }
    }

    if (selCBList != NULL)
        selCBList->invokeCallbacks(chemPath);
    chemPath->unref();

    if (changeNotify) {
        if (changeCBList != NULL)
            changeCBList->invokeCallbacks(this);
    }
}

//////////////////////////////////////////////////////////////////////////////
//
// Description:
//    This method takes the path indexed by `which' out of the selection list.
//    The path will be unreferenced by the ChemPathList (SoBaseList) class.
//
// Use: protected

void
ChemSelection::removeDisplayPath(int32_t which)
//
//////////////////////////////////////////////////////////////////////////////
{
    // Remove the path from the selection list,
    // and notify the app that this path has been deselected.
    if (which >= 0) {
        ChemDisplayPath *p = displaySelectionList[which];
        p->ref();
        displaySelectionList.remove(which);
        if (deselCBList != NULL)
            deselCBList->invokeCallbacks(p);
        p->unref();     
        
        if (changeCBList != NULL)
            changeCBList->invokeCallbacks(this);
    }
}

//////////////////////////////////////////////////////////////////////////////
//
// Description:
//    This method takes the path indexed by `which' out of the selection list.
//    The path will be unreferenced by the ChemPathList (SoBaseList) class.
//
// Use: protected

void
ChemSelection::removeLabelPath(int32_t which)
//
//////////////////////////////////////////////////////////////////////////////
{
    // Remove the path from the selection list,
    // and notify the app that this path has been deselected.
    if (which >= 0) {
        ChemLabelPath *p = labelSelectionList[which];
        p->ref();
        labelSelectionList.remove(which);
        if (deselCBList != NULL)
            deselCBList->invokeCallbacks(p);
        p->unref();     
        
        if (changeCBList != NULL)
            changeCBList->invokeCallbacks(this);
    }
}

//////////////////////////////////////////////////////////////////////////////
//
// Description:
//    This method takes the path indexed by `which' out of the selection list.
//    The path will be unreferenced by the ChemPathList (SoBaseList) class.
//
// Use: protected

void
ChemSelection::removeMonitorPath(int32_t which)
//
//////////////////////////////////////////////////////////////////////////////
{
    // Remove the path from the selection list,
    // and notify the app that this path has been deselected.
    if (which >= 0) {
        ChemMonitorPath *p = monitorSelectionList[which];
        p->ref();
        monitorSelectionList.remove(which);
        if (deselCBList != NULL)
            deselCBList->invokeCallbacks(p);
        p->unref();     
        
        if (changeCBList != NULL)
            changeCBList->invokeCallbacks(this);
    }
}

//////////////////////////////////////////////////////////////////////////////
//
// Description:
//    Returns the index of the path in the selection list.
//
// Use: protected

int
ChemSelection::findPath(const ChemPath *chemPath) const
//
//////////////////////////////////////////////////////////////////////////////
{
    int32_t index = -1;
    if (chemPath != NULL) {
        ChemPath *selPath = NULL;
	/* 
	   cast (SoNode *) added by fabien fontaine the 14/12/2000 to avoid 
	   compiler warning
	*/
        if (chemPath->path->getHead() != (SoNode *) this)
             selPath = copyFromThis(chemPath);
        else selPath = (ChemPath *)chemPath; // const cast away
        
        // selPath still not NULL? (copyFromThis() might have returned NULL)
        if (selPath != NULL) {
            selPath->ref();
			index = ((ChemDisplayPathList)displaySelectionList).findSoPath(selPath);
            if (index == -1) {
                index = ((ChemLabelPathList)labelSelectionList).findSoPath(selPath);
                if (index == -1) {
                    index = ((ChemMonitorPathList)monitorSelectionList).findSoPath(selPath);
                }
            }
            selPath->unref();
        }
    }
    
    return index;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Perform a lasso pick operation using the current set of lassoPoints
//
// Use: private       
        
void
ChemSelection::doLassoPick(SbBool shiftDown, SbBool ctrlDown)
//   
////////////////////////////////////////////////////////////////////////
{
#ifdef IV2_0
    int32_t numLassoVertices = getLassoPoints()->point.getNum();
#else
    int32_t numLassoVertices = getLassoPoints()->vertex.getNum();
#endif
    if (numLassoVertices < 3) {
        return;
    } 
    // The code to determine if a point is within the polygon automatically
    // connects the first and last point.  If the DRAGGER is used, the first
    // and last vertex points are the same, so set the number of vertices to
    // be 4 instead of 5.
    if (lassoType.getValue() == ChemSelection::DRAGGER) {
        assert(numLassoVertices == 5);
        numLassoVertices = 4;
    }
    
    short theLassoType = (short)lassoType.getValue();
    const SbVec3f &lassoMin = lassoBBox.getMin();
    const SbVec3f &lassoMax = lassoBBox.getMax();
    SbPList selectedItemList;

    int32_t what = parts.getValue();
    if ((SELECT(what, ChemSelection::ATOMS)) ||
        (SELECT(what, ChemSelection::BONDS)) ||
        (SELECT(what, ChemSelection::ATOMLABELS)) ||
        (SELECT(what, ChemSelection::BONDLABELS))
// --> residue selection
				|| (SELECT(what, ChemSelection::RESIDUES)
				|| (SELECT(what, ChemSelection::RESIDUELABELS)))
// <-- residue selection
				) {
        lassoChemDisplay(numLassoVertices, theLassoType,
            lassoMin, lassoMax, &selectedItemList);
    }

    if (SELECT(what, ChemSelection::CHEMLABELS)) {
        lassoChemLabel(numLassoVertices, theLassoType,
            lassoMin, lassoMax, &selectedItemList);
    }

    if (SELECT(what, ChemSelection::CHEMMONITORS)) {
        lassoChemMonitor(numLassoVertices, theLassoType,
            lassoMin, lassoMax, &selectedItemList);
    }

    // Invoke the startCBList if necessary
    if (startCBList != NULL) {
        startCBList->invokeCallbacks(this);
    }

    // Add the first chemPath according to the values of shiftDown and
    // crtlDown
    ChemPath *chemPath = (ChemPath *)selectedItemList[0];
    invokeSelectionPolicy(chemPath, shiftDown, ctrlDown, FALSE);
    if (chemPath != NULL) chemPath->unref();
    
    // Loop over the rest of seletedItemList
    int32_t i;
    int32_t endIndex = selectedItemList.getLength();
    for (i = 1; i < endIndex; i++) {
        chemPath = (ChemPath *)selectedItemList[i];
        invokeSelectionPolicy(chemPath, TRUE, ctrlDown, FALSE);
        if (chemPath != NULL) chemPath->unref();
    }
    selectedItemList.truncate(0);
// --> coin, memory pointers aren't freed
#ifdef __coin
		selectedItemList.truncate(0,TRUE);
#endif
// <-- coin, memory pointers aren't freed

    // Invoke the finishCBList if necessary
    if (finishCBList != NULL) {
        finishCBList->invokeCallbacks(this);
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Perform a lasso pick operation of items in ChemDisplay nodes
//    using the current set of lassoPoints.  The selected items are
//    appended to the selectedItemList.
//
// Use: private       

void
ChemSelection::lassoChemDisplay(int32_t numLassoVertices, short theLassoType,
    const SbVec3f &lassoMin, const SbVec3f &lassoMax, SbPList *selectedItemList)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef IV2_0
    const SbVec3f *lassoVertices = getLassoPoints()->point.getValues(0);
#else
    const SbVec3f *lassoVertices = getLassoPoints()->vertex.getValues(0);
#endif

    // Get paths to all of the ChemDisplays which are children of this
    // ChemSelection node.  Note that "pl" needs to be freed before leaving
    // this method.
    SoPathList *pl = getPathList(ChemDisplay::getClassTypeId());
    
    if (pl == NULL) {
        return;
    }
            
    ////////////////////////////////////////////
    //
    // Go through the pathList and add the ChemDisplay pointers to
    // pickedNodeArray.  Only unique references will be added.
    //
    
    int32_t i;
    uint32_t endIndex = pl->getLength();

    ChemDisplay *chemDisplay;
    ChemBaseData *chemData;
    ChemDisplayParam *cdp;
    SbIntList lassoedList(50);
    int32_t lassoedIncr;

    // Create a new searchAction if necessary
    if (searchAction == NULL) {
        searchAction = new SoSearchAction;
    }

    ChemDisplayPath *chemPath;
    int32_t what = parts.getValue();
    int32_t chemBBoxWhat = 0;

    SbBool atomClipOnCenter = (atomLassoPolicy.getValue() == ChemSelection::CENTER);
    SbBool bondClipOnCenter = (bondLassoPolicy.getValue() == ChemSelection::CENTER);
    SbBool atomLabelClipOnCenter =
        (atomLabelLassoPolicy.getValue() == ChemSelection::CENTER);
    SbBool bondLabelClipOnCenter =
        (bondLabelLassoPolicy.getValue() == ChemSelection::CENTER);

// --> residue selection
		SbBool residueClipOnCenter = (residueLassoPolicy.getValue() == ChemSelection::CENTER);
		SbBool residueLabelClipOnCenter = (residueLabelLassoPolicy.getValue() == ChemSelection::CENTER);
// <-- residue selection

    // Figure out what to get with the ChemBBoxAction
    if (SELECT(what, ChemSelection::ATOMS)) {
        chemBBoxWhat |= ChemBBoxAction::ATOMS;
    }
    if (SELECT(what, ChemSelection::BONDS)) {
        chemBBoxWhat |= ChemBBoxAction::BONDS;
    }
    if (SELECT(what, ChemSelection::ATOMLABELS)) {
        chemBBoxWhat |= ChemBBoxAction::ATOMLABELS;
    }
    if (SELECT(what, ChemSelection::BONDLABELS)) {
        chemBBoxWhat |= ChemBBoxAction::BONDLABELS;
    }

// --> residue selection
		if (SELECT(what, ChemSelection::RESIDUES)) {
        chemBBoxWhat |= ChemBBoxAction::RESIDUES;
    }
		if (SELECT(what, ChemSelection::RESIDUELABELS)) {
        chemBBoxWhat |= ChemBBoxAction::RESIDUELABELS;
    }
// <-- residue selection

    // Set up ChemBBoxAction
    if (chemBBoxAction == NULL) {
        chemBBoxAction = new ChemBBoxAction;
    }

    int32_t loop;
    int32_t theItem;
    int32_t thePolicy;

    // For atoms
    ChemAtomBBox *atomBBoxes;

    // For bonds
    ChemBondBBox *bondBBoxes;

    // For labels
    ChemLabelBBox *atomLabelBBoxes;
    ChemLabelBBox *bondLabelBBoxes;

// --> residue selection
		// For residues
    ChemBondBBox *residueBBoxes;
		// For labels
    ChemLabelBBox *residueLabelBBoxes;
// <-- residue selection

    for (i = 0; i < endIndex; i++) {
        // Make sure we have the full path to the ChemDisplay node.
        SoFullPath *path = (SoFullPath *)(*pl)[i];
        assert(path->getTail()->isOfType(ChemDisplay::getClassTypeId()));
        chemDisplay = (ChemDisplay *)(path->getTail());
        assert(chemDisplay != NULL);

        chemPath = new ChemDisplayPath;
        chemPath->setSoPath(path);
        chemPath->ref();

        // Search the path for the ChemDisplayParam node.
        searchAction->reset();
        searchAction->setType(ChemDisplayParam::getClassTypeId());
        searchAction->setInterest(SoSearchAction::LAST);
        searchAction->apply(path);
        SoFullPath *thePath = (SoFullPath *)searchAction->getPath();
        cdp = (ChemDisplayParam *)(thePath->getTail());

        // Search the path for the ChemBaseData node.  The reset needs
        // to be done before each apply or else the result will be appended
        // to the list of returned paths.
        searchAction->reset();
        searchAction->setType(ChemBaseData::getClassTypeId());
        searchAction->setInterest(SoSearchAction::LAST);
        searchAction->apply(path);

        // Do the appropriate ChemBBoxAction
        if (chemBBoxWhat != 0) {
            chemBBoxAction->reset();
            chemBBoxAction->setParts(chemBBoxWhat);
            chemBBoxAction->setAtomClipOnCenter(atomClipOnCenter);
            chemBBoxAction->setBondClipOnCenter(bondClipOnCenter);
            chemBBoxAction->setAtomLabelClipOnCenter(atomLabelClipOnCenter);
            chemBBoxAction->setBondLabelClipOnCenter(bondLabelClipOnCenter);
// --> residue selection
						chemBBoxAction->setResidueClipOnCenter(residueClipOnCenter);
						chemBBoxAction->setResidueLabelClipOnCenter(residueLabelClipOnCenter);
// <-- residue selection
            chemBBoxAction->apply(path);
        }
        const SbMatrix &displayMVPMatrix = chemDisplay->getCurrentMVPMatrix();
        const SbRotation &displayRotation = chemDisplay->getCurrentRotation();

        // Loop through all of the atoms in the current ChemDisplay node
        // and determine if they have been lassoed.
        thePath = (SoFullPath *)searchAction->getPath();
        chemData = (ChemBaseData *)(thePath->getTail());
        //int32_t numberOfAtoms = chemData->getNumberOfAtoms();
        //int32_t numberOfBonds = chemData->getNumberOfBonds();
// --> residues
	//int32_t numberOfResidues = ((ChemData*)chemData)->getNumberOfResidues();
// <-- residues
        SbBool  doHydrogens = cdp->showHydrogens.getValue();
        if ((!doHydrogens) && (selectInvisibleHydrogens.getValue() == TRUE)) {
            doHydrogens = TRUE;
        }

        if (SELECT(what, ChemSelection::ATOMS)) {
            chemBBoxAction->getAtomBBoxes(atomBBoxes);
            if ((atomBBoxes != NULL) && (atomBBoxes->numberOfAtoms > 0)) {
                thePolicy = atomLassoPolicy.getValue();
                lassoedList.truncate(0);
                lassoedIncr = 0;
                if (thePolicy == ChemSelection::CENTER) {
                    for (loop = 0; loop < atomBBoxes->numberOfAtoms; loop++) {
                        theItem = atomBBoxes->index[loop];
                        if (!doHydrogens) {
                            if (chemData->getAtomicNumber(theItem) == 1) {
                                continue;
                            }
                        }
                        SbVec3f tmpCoord =
                            fromObjectSpace(atomBBoxes->center[loop],
                                            displayMVPMatrix);
                        tmpCoord[0] *= orthoCamXres;
                        tmpCoord[1] *= orthoCamYres;
                        if (pointInLasso(theLassoType, lassoMin, lassoMax,
                                         lassoVertices, numLassoVertices,
                                         tmpCoord)) {
                            // The atom has been lassoed so add it to the list
                            // of lassoed atoms which will be processed later.
// --> EGB && SGB lasso selection error
//            lassoedList[lassoedIncr++] = theItem;
															lassoedIncr++;
															lassoedList.append(theItem);
// <-- EGB && SGB lasso selection error
                        }
                    }
                }
                else if (thePolicy == ChemSelection::THRU_BBOX) {
                    checkLassoThruAtomBBox(orthoCamXres, orthoCamYres,
                        displayMVPMatrix, displayRotation, 
                        doHydrogens, chemData, atomBBoxes, 
                        theLassoType, lassoBBox,
                        numLassoVertices, lassoVertices, 
                        lassoedIncr, lassoedList);
                }
                else if (thePolicy == ChemSelection::ENTIRE_BBOX) {
                    checkLassoEntireAtomBBox(orthoCamXres, orthoCamYres,
                        ringCoords,
                        displayMVPMatrix, displayRotation,
                        doHydrogens, chemData, atomBBoxes,
                        theLassoType, lassoBBox,
                        numLassoVertices, lassoVertices, 
                        lassoedIncr, lassoedList);
                }
                selectItems(lassoedList, chemPath->atomIndex);
            }
        } // end of selection of ATOMS 

        if (SELECT(what, ChemSelection::BONDS)) {
            int displayStyle = cdp->displayStyle.getValue();
            if ((displayStyle != ChemDisplayParam::DISPLAY_CPK) &&
                (displayStyle != ChemDisplayParam::DISPLAY_NONE)) {
                chemBBoxAction->getBondBBoxes(bondBBoxes);
                if ((bondBBoxes != NULL) && (bondBBoxes->numberOfBonds > 0)) {
                    SbBool isWireframe =
                        ((displayStyle == ChemDisplayParam::DISPLAY_WIREFRAME) ||
                         (displayStyle == ChemDisplayParam::DISPLAY_BALLWIRE));
        
                    thePolicy = bondLassoPolicy.getValue();
                    lassoedList.truncate(0);
                    lassoedIncr = 0;
                    if (thePolicy == ChemSelection::CENTER) {
                        for (loop = 0; loop < bondBBoxes->numberOfBonds; loop++) {
                            theItem = bondBBoxes->index[loop];
                            int32_t from = chemData->getBondFrom(theItem);
                            int32_t to = chemData->getBondTo(theItem);
        
                            if (!doHydrogens) {
                                if ((chemData->getAtomicNumber(from) == 1) ||
                                    (chemData->getAtomicNumber(to) == 1)) {
                                    continue;
                                }
                            }

                            SbVec3f tmpCoord =
                                fromObjectSpace(bondBBoxes->center[loop],
                                                displayMVPMatrix);
                            tmpCoord[0] *= orthoCamXres;
                            tmpCoord[1] *= orthoCamYres;
                            if (pointInLasso(theLassoType, lassoMin, lassoMax,
                                             lassoVertices, numLassoVertices,
                                             tmpCoord)) {
                                // The bond has been lassoed so add it to the list
                                // of lassoed bonds which will be processed later.
// --> EGB && SGB lasso selection error
//            lassoedList[lassoedIncr++] = theItem;
															lassoedIncr++;
															lassoedList.append(theItem);
// <-- EGB && SGB lasso selection error
                            }
                        }
                    }
                    else if (thePolicy == ChemSelection::THRU_BBOX) {
                        if (isWireframe) {
                            checkLassoThruWireframeBondBBox(
                                orthoCamXres, orthoCamYres,
                                displayMVPMatrix,
                                doHydrogens, chemData,
                                (ChemWireframeBondBBox *)bondBBoxes, 
                                theLassoType, lassoBBox,
                                numLassoVertices, lassoVertices, 
                                lassoedIncr, lassoedList);
                        }
                        else {
                            checkLassoThruStickBondBBox(orthoCamXres, orthoCamYres,
                                displayMVPMatrix,
                                doHydrogens, chemData,
                                (ChemStickBondBBox *)bondBBoxes, 
                                theLassoType, lassoBBox,
                                numLassoVertices, lassoVertices, 
                                lassoedIncr, lassoedList);
                        }
                    }
                    else if (thePolicy == ChemSelection::ENTIRE_BBOX) {
                        if (isWireframe) {
                            checkLassoEntireWireframeBondBBox(
                                orthoCamXres, orthoCamYres,
                                displayMVPMatrix,
                                doHydrogens, chemData,
                                (ChemWireframeBondBBox *)bondBBoxes,
                                theLassoType, lassoBBox,
                                numLassoVertices, lassoVertices, 
                                lassoedIncr, lassoedList);
                        }
                        else {
                            checkLassoEntireStickBondBBox(orthoCamXres, orthoCamYres,
                                displayMVPMatrix,
                                doHydrogens, chemData,
                                (ChemStickBondBBox *)bondBBoxes,
                                theLassoType, lassoBBox,
                                numLassoVertices, lassoVertices, 
                                lassoedIncr, lassoedList);
                        }
                    }

                    selectItems(lassoedList, chemPath->bondIndex);
                }
            }
        } // end of selection of BONDS 
    
        if (SELECT(what, ChemSelection::ATOMLABELS)) {
            chemBBoxAction->getAtomLabelBBoxes(atomLabelBBoxes);
            if ((atomLabelBBoxes != NULL) &&
                (atomLabelBBoxes->numberOfLabels > 0)) {
                thePolicy = atomLabelLassoPolicy.getValue();
                lassoedList.truncate(0);
                lassoedIncr = 0;
                if (thePolicy == ChemSelection::CENTER) {
                    checkLassoCenter(orthoCamXres, orthoCamYres,
                        displayMVPMatrix,
                        atomLabelBBoxes->numberOfLabels,
                        atomLabelBBoxes->index,
                        atomLabelBBoxes->center,
                        theLassoType, lassoBBox,
                        numLassoVertices, lassoVertices, 
                        lassoedIncr, lassoedList);
                }
                else if (thePolicy == ChemSelection::THRU_BBOX) {
                    checkLassoThruBBox(orthoCamXres, orthoCamYres,
                        displayMVPMatrix,
                        atomLabelBBoxes->numberOfLabels,
                        atomLabelBBoxes->index,
                        atomLabelBBoxes->bbox,
                        atomLabelBBoxes->center,
                        theLassoType, lassoBBox,
                        numLassoVertices, lassoVertices, 
                        lassoedIncr, lassoedList);
                }
                else if (thePolicy == ChemSelection::ENTIRE_BBOX) {
                    checkLassoEntireBBox(orthoCamXres, orthoCamYres,
                        displayMVPMatrix,
                        atomLabelBBoxes->numberOfLabels,
                        atomLabelBBoxes->index,
                        atomLabelBBoxes->bbox,
                        atomLabelBBoxes->center,
                        theLassoType, lassoBBox,
                        numLassoVertices, lassoVertices, 
                        lassoedIncr, lassoedList);
                }
                selectItems(lassoedList, chemPath->atomLabelIndex);
            } // end of (atomLabelBBoxes != NULL)
        } // end of selection of ATOMLABELS

        if (SELECT(what, ChemSelection::BONDLABELS)) {
            chemBBoxAction->getBondLabelBBoxes(bondLabelBBoxes);
            if ((bondLabelBBoxes != NULL) &&
                (bondLabelBBoxes->numberOfLabels > 0)) {
                thePolicy = bondLabelLassoPolicy.getValue();
                lassoedList.truncate(0);
                lassoedIncr = 0;
                if (thePolicy == ChemSelection::CENTER) {
                    checkLassoCenter(orthoCamXres, orthoCamYres,
                        displayMVPMatrix,
                        bondLabelBBoxes->numberOfLabels,
                        bondLabelBBoxes->index,
                        bondLabelBBoxes->center,
                        theLassoType, lassoBBox,
                        numLassoVertices, lassoVertices, 
                        lassoedIncr, lassoedList);
                }
                else if (thePolicy == ChemSelection::THRU_BBOX) {
                    checkLassoThruBBox(orthoCamXres, orthoCamYres,
                        displayMVPMatrix,
                        bondLabelBBoxes->numberOfLabels,
                        bondLabelBBoxes->index,
                        bondLabelBBoxes->bbox,
                        bondLabelBBoxes->center,
                        theLassoType, lassoBBox,
                        numLassoVertices, lassoVertices, 
                        lassoedIncr, lassoedList);
                }
                else if (thePolicy == ChemSelection::ENTIRE_BBOX) {
                    checkLassoEntireBBox(orthoCamXres, orthoCamYres,
                        displayMVPMatrix,
                        bondLabelBBoxes->numberOfLabels,
                        bondLabelBBoxes->index,
                        bondLabelBBoxes->bbox,
                        bondLabelBBoxes->center,
                        theLassoType, lassoBBox,
                        numLassoVertices, lassoVertices, 
                        lassoedIncr, lassoedList);
                }
                selectItems(lassoedList, chemPath->bondLabelIndex);
            } // end of (bondLabelBBoxes != NULL)
        } // end of selection of BONDLABELS

// --> residue selection
		if (SELECT(what, ChemSelection::RESIDUES))
		{
            int displayStyle = cdp->residueDisplayStyle.getValue();
            if (displayStyle == ChemDisplayParam::DISPLAY_RESIDUES_CAWIRE ||
								displayStyle == ChemDisplayParam::DISPLAY_RESIDUES_CASTICK)
			{
                chemBBoxAction->getResidueBBoxes(residueBBoxes);
                if ((residueBBoxes != NULL) && (residueBBoxes->numberOfBonds > 0))
								{
                    SbBool isWireframe =
                        (displayStyle == ChemDisplayParam::DISPLAY_RESIDUES_CAWIRE);
        
                    thePolicy = residueLassoPolicy.getValue();
                    lassoedList.truncate(0);
                    lassoedIncr = 0;
                    if (thePolicy == ChemSelection::CENTER)
										{
                        for (loop = 0; loop < residueBBoxes->numberOfBonds; loop++)
												{
                            theItem = residueBBoxes->index[loop];
        
                            SbVec3f tmpCoord =
                                fromObjectSpace(residueBBoxes->center[loop],
                                                displayMVPMatrix);
                            tmpCoord[0] *= orthoCamXres;
                            tmpCoord[1] *= orthoCamYres;
                            if (pointInLasso(theLassoType, lassoMin, lassoMax,
                                             lassoVertices, numLassoVertices,
                                             tmpCoord))
														{
                                // The residue bond has been lassoed so add it to the list
                                // of lassoed bonds which will be processed later.
															lassoedIncr++;
															assert(theItem < numberOfResidues);
															lassoedList.append(theItem);
                            }
                        }
                    }
                    else if (thePolicy == ChemSelection::THRU_BBOX)
										{
                        if (isWireframe)
												{
                            checkLassoThruWireframeBondBBox(
                                orthoCamXres, orthoCamYres,
                                displayMVPMatrix,
                                doHydrogens, chemData,
                                (ChemWireframeBondBBox *)residueBBoxes, 
                                theLassoType, lassoBBox,
                                numLassoVertices, lassoVertices, 
                                lassoedIncr, lassoedList);
                        }
                        else
												{
                            checkLassoThruStickBondBBox(orthoCamXres, orthoCamYres,
                                displayMVPMatrix,
                                doHydrogens, chemData,
                                (ChemStickBondBBox *)residueBBoxes, 
                                theLassoType, lassoBBox,
                                numLassoVertices, lassoVertices, 
                                lassoedIncr, lassoedList);
                        }
                    }
                    else if (thePolicy == ChemSelection::ENTIRE_BBOX)
										{
                        if (isWireframe)
												{
                            checkLassoEntireWireframeBondBBox(
                                orthoCamXres, orthoCamYres,
                                displayMVPMatrix,
                                doHydrogens, chemData,
                                (ChemWireframeBondBBox *)residueBBoxes,
                                theLassoType, lassoBBox,
                                numLassoVertices, lassoVertices, 
                                lassoedIncr, lassoedList);
                        }
                        else
												{
                            checkLassoEntireStickBondBBox(orthoCamXres, orthoCamYres,
                                displayMVPMatrix,
                                doHydrogens, chemData,
                                (ChemStickBondBBox *)residueBBoxes,
                                theLassoType, lassoBBox,
                                numLassoVertices, lassoVertices, 
                                lassoedIncr, lassoedList);
                        }
                    }
// --> residue atoms selection
									 if (SELECT(what, ChemSelection::ATOMS))
											selectResidueAtoms(lassoedList,chemPath->atomIndex,(ChemData*)chemData,cdp->residueSelectAtoms.getValue());
// <-- residue atoms selection
                    selectItems(lassoedList, chemPath->residueIndex);
                }
						} // cawire, castick
						else if (displayStyle == ChemDisplayParam::DISPLAY_RESIDUES_LINERIBBON ||
							displayStyle == ChemDisplayParam::DISPLAY_RESIDUES_FLATRIBBON || 
							displayStyle == ChemDisplayParam::DISPLAY_RESIDUES_SOLIDRIBBON)
						{
						 chemBBoxAction->getResidueBBoxes(residueBBoxes);
						 thePolicy = residueLassoPolicy.getValue();
						 lassoedList.truncate(0);
						 lassoedIncr = 0;
						 if ((residueBBoxes != NULL) && (residueBBoxes->numberOfBonds > 0))
						 {
							 if (thePolicy == ChemSelection::CENTER)
							 {
									for (loop = 0; loop < residueBBoxes->numberOfBonds; loop++)
									{
										SbVec3f tmpCoord =
												fromObjectSpace(residueBBoxes->center[loop],
																				displayMVPMatrix);
										tmpCoord[0] *= orthoCamXres;
										tmpCoord[1] *= orthoCamYres;
										if (pointInLasso(theLassoType, lassoMin, lassoMax,
																		 lassoVertices, numLassoVertices,
																		 tmpCoord))
										{
												// The residue has been lassoed so add it to the list
												// of lassoed residues which will be processed later.
											theItem = residueBBoxes->index[loop];
											assert(theItem < numberOfResidues);
											lassoedIncr++;
											lassoedList.append(theItem);
										}
									}
							 }
							 else if (thePolicy == ChemSelection::THRU_BBOX)
							 {
								// TO DO:
								 SoDebugError::post("ChemSelection::lassoChemDisplay","selection THRU_BBOX not implemented");
							 }
							 else // if (thePolicy == ChemSelection::ENTIRE_BBOX)
							 {
								 // TO DO:
								 SoDebugError::post("ChemSelection::lassoChemDisplay","selection ENTIRE_BBOX not implemented");
							 }
// --> residue atoms selection
							 if (SELECT(what, ChemSelection::ATOMS))
									selectResidueAtoms(lassoedList,chemPath->atomIndex,(ChemData*)chemData,cdp->residueSelectAtoms.getValue());
// <-- residue atoms selection
							 selectItems(lassoedList, chemPath->residueIndex);
						}
				} // ribbons
		} // end of selection of RESIDUES

		if (SELECT(what, ChemSelection::RESIDUELABELS))
		{
			chemBBoxAction->getResidueLabelBBoxes(residueLabelBBoxes);
      if ((residueLabelBBoxes != NULL) && (residueLabelBBoxes->numberOfLabels> 0))
			{
          thePolicy = residueLabelLassoPolicy.getValue();
          lassoedList.truncate(0);
          lassoedIncr = 0;
          if (thePolicy == ChemSelection::CENTER)
					{
						checkLassoCenter(orthoCamXres, orthoCamYres,
							displayMVPMatrix,
							residueLabelBBoxes->numberOfLabels,
							residueLabelBBoxes->index,
							residueLabelBBoxes->center,
							theLassoType, lassoBBox,
							numLassoVertices, lassoVertices, 
							lassoedIncr, lassoedList);
					}
          else if (thePolicy == ChemSelection::THRU_BBOX)
					{
						checkLassoThruBBox(orthoCamXres, orthoCamYres,
								displayMVPMatrix,
								residueLabelBBoxes->numberOfLabels,
								residueLabelBBoxes->index,
								residueLabelBBoxes->bbox,
								residueLabelBBoxes->center,
								theLassoType, lassoBBox,
								numLassoVertices, lassoVertices, 
								lassoedIncr, lassoedList);
          }
          else if (thePolicy == ChemSelection::ENTIRE_BBOX)
					{
	          checkLassoEntireBBox(orthoCamXres, orthoCamYres,
              displayMVPMatrix,
              residueLabelBBoxes->numberOfLabels,
              residueLabelBBoxes->index,
              residueLabelBBoxes->bbox,
              residueLabelBBoxes->center,
              theLassoType, lassoBBox,
              numLassoVertices, lassoVertices, 
              lassoedIncr, lassoedList);
          }
					selectItems(lassoedList, chemPath->residueLabelIndex);
				}
    } // end of selection of RESIDUELABELS
// <-- residue selection

        selectedItemList->append(chemPath);
    }

    if (chemBBoxAction != NULL) {
        chemBBoxAction->reset();
    }
    delete pl;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Perform a lasso pick operation of labels in ChemLabel nodes
//    using the current set of lassoPoints.  The selected labels are
//    appended to the selectedItemList.
//
// Use: private       

void
ChemSelection::lassoChemLabel(int32_t numLassoVertices, short theLassoType,
    const SbVec3f &lassoMin, const SbVec3f &lassoMax, SbPList *selectedItemList)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef IV2_0
    const SbVec3f *lassoVertices = getLassoPoints()->point.getValues(0);
#else
    const SbVec3f *lassoVertices = getLassoPoints()->vertex.getValues(0);
#endif

    // Get paths to all of the ChemLabels which are children of this
    // ChemSelection node.  Note that "pl" needs to be freed before leaving
    // this method.
    SoPathList *pl = getPathList(ChemLabel::getClassTypeId());
    
    if (pl == NULL) {
        return;
    }
            
    ////////////////////////////////////////////
    //
    // Go through the pathList and add the ChemLabel pointers to
    // pickedNodeArray.  Only unique references will be added.
    //
    
    int32_t i;
    uint32_t endIndex = pl->getLength();

    ChemLabel *chemLabel;
    SbIntList lassoedList(50);
    int32_t lassoedIncr;

    // Create a new searchAction if necessary
    if (searchAction == NULL) {
        searchAction = new SoSearchAction;
    }

    ChemLabelPath *chemPath;
    int32_t what = parts.getValue();
    int32_t chemBBoxWhat = 0;

    SbBool chemLabelClipOnCenter =
        (chemLabelLassoPolicy.getValue() == ChemSelection::CENTER);

    // Figure out what to get with the ChemBBoxAction
    if (SELECT(what, ChemSelection::CHEMLABELS)) {
        chemBBoxWhat |= ChemBBoxAction::CHEMLABELS;
    }
    // Set up ChemBBoxAction
    if (chemBBoxAction == NULL) {
        chemBBoxAction = new ChemBBoxAction;
    }
    int32_t thePolicy;

    // For labels
    ChemLabelBBox *chemLabelBBoxes;

    for (i = 0; i < endIndex; i++) {
        // Make sure we have the full path to the ChemLabel node.
        SoFullPath *path = (SoFullPath *)(*pl)[i];
        assert(path->getTail()->isOfType(ChemLabel::getClassTypeId()));
        chemLabel = (ChemLabel *)(path->getTail());
        assert(chemLabel != NULL);

        chemPath = new ChemLabelPath;
        chemPath->setSoPath(path);
        chemPath->ref();

        // Do the appropriate ChemBBoxAction
        if (chemBBoxWhat != 0) {
            chemBBoxAction->reset();
            chemBBoxAction->setParts(chemBBoxWhat);
            chemBBoxAction->setChemLabelClipOnCenter(chemLabelClipOnCenter);
            chemBBoxAction->apply(path);
        }
        const SbMatrix &labelMVPMatrix = chemLabel->getCurrentMVPMatrix();

        // Loop through all of the labels in the current ChemLabel node
        // and determine if they have been lassoed.
        chemBBoxAction->getChemLabelBBoxes(chemLabelBBoxes);
        if ((chemLabelBBoxes != NULL) &&
            (chemLabelBBoxes->numberOfLabels > 0)) {
            thePolicy = chemLabelLassoPolicy.getValue();
            lassoedList.truncate(0);
            lassoedIncr = 0;
            if (thePolicy == ChemSelection::CENTER) {
                checkLassoCenter(orthoCamXres, orthoCamYres,
                    labelMVPMatrix,
                    chemLabelBBoxes->numberOfLabels,
                    chemLabelBBoxes->index,
                    chemLabelBBoxes->center,
                    theLassoType, lassoBBox,
                    numLassoVertices, lassoVertices, 
                    lassoedIncr, lassoedList);
            }
            else if (thePolicy == ChemSelection::THRU_BBOX) {
                checkLassoThruBBox(orthoCamXres, orthoCamYres,
                    labelMVPMatrix,
                    chemLabelBBoxes->numberOfLabels,
                    chemLabelBBoxes->index,
                    chemLabelBBoxes->bbox,
                    chemLabelBBoxes->center,
                    theLassoType, lassoBBox,
                    numLassoVertices, lassoVertices, 
                    lassoedIncr, lassoedList);
            }
            else if (thePolicy == ChemSelection::ENTIRE_BBOX) {
                checkLassoEntireBBox(orthoCamXres, orthoCamYres,
                    labelMVPMatrix,
                    chemLabelBBoxes->numberOfLabels,
                    chemLabelBBoxes->index,
                    chemLabelBBoxes->bbox,
                    chemLabelBBoxes->center,
                    theLassoType, lassoBBox,
                    numLassoVertices, lassoVertices, 
                    lassoedIncr, lassoedList);
            }
    
            selectItems(lassoedList, chemPath->labelIndex);
            selectedItemList->append(chemPath);

        } // end of (chemLabelBBoxes != NULL)
    } // end of loop over paths to ChemLabel nodes

    if (chemBBoxAction != NULL) {
        chemBBoxAction->reset();
    }
    delete pl;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Perform a lasso pick operation of monitors in ChemMonitor nodes
//    using the current set of lassoPoints.  The selected monitors are
//    appended to the selectedItemList.
//
// Use: private       

void
ChemSelection::lassoChemMonitor(int32_t numLassoVertices, short theLassoType,
    const SbVec3f &lassoMin, const SbVec3f &lassoMax, SbPList *selectedItemList)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef IV2_0
    const SbVec3f *lassoVertices = getLassoPoints()->point.getValues(0);
#else
    const SbVec3f *lassoVertices = getLassoPoints()->vertex.getValues(0);
#endif

    // Get paths to all of the ChemMonitors which are children of this
    // ChemSelection node.  Note that "pl" needs to be freed before leaving
    // this method.
    SoPathList *pl = getPathList(ChemMonitor::getClassTypeId());
    
    if (pl == NULL) {
        return;
    }
            
    ////////////////////////////////////////////
    //
    // Go through the pathList and add the ChemMonitor pointers to
    // pickedNodeArray.  Only unique references will be added.
    //
    
    int32_t i;
    uint32_t endIndex = pl->getLength();

    ChemMonitor *chemMonitor;
    SbIntList lassoedList(50);
    int32_t lassoedIncr;

    // Create a new searchAction if necessary
    if (searchAction == NULL) {
        searchAction = new SoSearchAction;
    }

    ChemMonitorPath *chemPath;
    int32_t what = parts.getValue();
    int32_t chemBBoxWhat = 0;

    SbBool chemMonitorClipOnCenter =
        (chemMonitorLassoPolicy.getValue() == ChemSelection::CENTER);

    // Figure out what to get with the ChemBBoxAction
    if (SELECT(what, ChemSelection::CHEMMONITORS)) {
        chemBBoxWhat |= ChemBBoxAction::CHEMMONITORS;
    }
    // Set up ChemBBoxAction
    if (chemBBoxAction == NULL) {
        chemBBoxAction = new ChemBBoxAction;
    }

    int32_t thePolicy;

    // For monitors
    ChemMonitorBBox *chemMonitorBBoxes;

    for (i = 0; i < endIndex; i++) {
        // Make sure we have the full path to the ChemMonitor node.
        SoFullPath *path = (SoFullPath *)(*pl)[i];
        assert(path->getTail()->isOfType(ChemMonitor::getClassTypeId()));
        chemMonitor = (ChemMonitor *)(path->getTail());
        assert(chemMonitor != NULL);

        chemPath = new ChemMonitorPath;
        chemPath->setSoPath(path);
        chemPath->ref();

        // Do the appropriate ChemBBoxAction
        if (chemBBoxWhat != 0) {
            chemBBoxAction->reset();
            chemBBoxAction->setParts(chemBBoxWhat);
            chemBBoxAction->setChemMonitorClipOnCenter(chemMonitorClipOnCenter);
            chemBBoxAction->apply(path);
        }
        const SbMatrix &monitorMVPMatrix = chemMonitor->getCurrentMVPMatrix();

        // Loop through all of the monitors in the current ChemMonitor node
        // and determine if they have been lassoed.
        chemBBoxAction->getChemMonitorBBoxes(chemMonitorBBoxes);
        if (chemMonitorBBoxes != NULL) {
            thePolicy = chemMonitorLassoPolicy.getValue();
            if (thePolicy == ChemSelection::CENTER) {
                lassoedList.truncate(0);
                lassoedIncr = 0;
                checkLassoCenter(orthoCamXres, orthoCamYres,
                    monitorMVPMatrix,
                    chemMonitorBBoxes->numberOfDistanceMonitors,
                    chemMonitorBBoxes->distanceIndex,
                    chemMonitorBBoxes->distanceCenter,
                    theLassoType, lassoBBox,
                    numLassoVertices, lassoVertices, 
                    lassoedIncr, lassoedList);

                selectItems(lassoedList, chemPath->distanceIndex);

                lassoedList.truncate(0);
                lassoedIncr = 0;
                checkLassoCenter(orthoCamXres, orthoCamYres,
                    monitorMVPMatrix,
                    chemMonitorBBoxes->numberOfAngleMonitors,
                    chemMonitorBBoxes->angleIndex,
                    chemMonitorBBoxes->angleCenter,
                    theLassoType, lassoBBox,
                    numLassoVertices, lassoVertices, 
                    lassoedIncr, lassoedList);

                selectItems(lassoedList, chemPath->angleIndex);

                lassoedList.truncate(0);
                lassoedIncr = 0;
                checkLassoCenter(orthoCamXres, orthoCamYres,
                    monitorMVPMatrix,
                    chemMonitorBBoxes->numberOfTorsionalMonitors,
                    chemMonitorBBoxes->torsionalIndex,
                    chemMonitorBBoxes->torsionalCenter,
                    theLassoType, lassoBBox,
                    numLassoVertices, lassoVertices, 
                    lassoedIncr, lassoedList);

                selectItems(lassoedList, chemPath->torsionalIndex);
            }
            else if (thePolicy == ChemSelection::THRU_BBOX) {
                lassoedList.truncate(0);
                lassoedIncr = 0;
                checkLassoThruBBox(orthoCamXres, orthoCamYres,
                    monitorMVPMatrix,
                    chemMonitorBBoxes->numberOfDistanceMonitors,
                    chemMonitorBBoxes->distanceIndex,
                    chemMonitorBBoxes->distanceBBox,
                    chemMonitorBBoxes->distanceCenter,
                    theLassoType, lassoBBox,
                    numLassoVertices, lassoVertices, 
                    lassoedIncr, lassoedList);

                selectItems(lassoedList, chemPath->distanceIndex);

                lassoedList.truncate(0);
                lassoedIncr = 0;
                checkLassoThruBBox(orthoCamXres, orthoCamYres,
                    monitorMVPMatrix,
                    chemMonitorBBoxes->numberOfAngleMonitors,
                    chemMonitorBBoxes->angleIndex,
                    chemMonitorBBoxes->angleBBox,
                    chemMonitorBBoxes->angleCenter,
                    theLassoType, lassoBBox,
                    numLassoVertices, lassoVertices, 
                    lassoedIncr, lassoedList);

                selectItems(lassoedList, chemPath->angleIndex);

                lassoedList.truncate(0);
                lassoedIncr = 0;
                checkLassoThruBBox(orthoCamXres, orthoCamYres,
                    monitorMVPMatrix,
                    chemMonitorBBoxes->numberOfTorsionalMonitors,
                    chemMonitorBBoxes->torsionalIndex,
                    chemMonitorBBoxes->torsionalBBox,
                    chemMonitorBBoxes->torsionalCenter,
                    theLassoType, lassoBBox,
                    numLassoVertices, lassoVertices, 
                    lassoedIncr, lassoedList);

                selectItems(lassoedList, chemPath->torsionalIndex);
            }
            else if (thePolicy == ChemSelection::ENTIRE_BBOX) {
                lassoedList.truncate(0);
                lassoedIncr = 0;
                checkLassoEntireBBox(orthoCamXres, orthoCamYres,
                    monitorMVPMatrix,
                    chemMonitorBBoxes->numberOfDistanceMonitors,
                    chemMonitorBBoxes->distanceIndex,
                    chemMonitorBBoxes->distanceBBox,
                    chemMonitorBBoxes->distanceCenter,
                    theLassoType, lassoBBox,
                    numLassoVertices, lassoVertices, 
                    lassoedIncr, lassoedList);

                selectItems(lassoedList, chemPath->distanceIndex);

                lassoedList.truncate(0);
                lassoedIncr = 0;
                checkLassoEntireBBox(orthoCamXres, orthoCamYres,
                    monitorMVPMatrix,
                    chemMonitorBBoxes->numberOfAngleMonitors,
                    chemMonitorBBoxes->angleIndex,
                    chemMonitorBBoxes->angleBBox,
                    chemMonitorBBoxes->angleCenter,
                    theLassoType, lassoBBox,
                    numLassoVertices, lassoVertices, 
                    lassoedIncr, lassoedList);

                selectItems(lassoedList, chemPath->angleIndex);

                lassoedList.truncate(0);
                lassoedIncr = 0;
                checkLassoEntireBBox(orthoCamXres, orthoCamYres,
                    monitorMVPMatrix,
                    chemMonitorBBoxes->numberOfTorsionalMonitors,
                    chemMonitorBBoxes->torsionalIndex,
                    chemMonitorBBoxes->torsionalBBox,
                    chemMonitorBBoxes->torsionalCenter,
                    theLassoType, lassoBBox,
                    numLassoVertices, lassoVertices, 
                    lassoedIncr, lassoedList);

                selectItems(lassoedList, chemPath->torsionalIndex);
            }
    
            selectedItemList->append(chemPath);

        } // end of (chemMonitorBBoxes != NULL)
    } // end of loop over paths to ChemMonitor nodes

    if (chemBBoxAction != NULL) {
        chemBBoxAction->reset();
    }
    delete pl;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Perform a radius pick operation using the selectedRadius.
//
// Use: private       

void
ChemSelection::doRadiusPick(SbBool shiftDown, SbBool ctrlDown)
//
////////////////////////////////////////////////////////////////////////
{
    int32_t what = parts.getValue();
    SbPList selectedItemList;

    if ((SELECT(what, ChemSelection::ATOMS)) ||
        (SELECT(what, ChemSelection::BONDS)) ||
        (SELECT(what, ChemSelection::ATOMLABELS)) ||
        (SELECT(what, ChemSelection::BONDLABELS))) {
        radiusPickChemDisplay(&selectedItemList);
    }

    if (SELECT(what, ChemSelection::CHEMLABELS)) {
        radiusPickChemLabel(&selectedItemList);
    }

    if (SELECT(what, ChemSelection::CHEMMONITORS)) {
        radiusPickChemMonitor(&selectedItemList);
    }

    // Invoke the startCBList if necessary
    if (startCBList != NULL) {
        startCBList->invokeCallbacks(this);
    }

    // Add the first chemPath according to the values of shiftDown and
    // crtlDown
    ChemPath *chemPath = (ChemPath *)selectedItemList[0];
    invokeSelectionPolicy(chemPath, shiftDown, ctrlDown, FALSE);
    chemPath->unref();

    // Loop over the rest of tmpPathList
    int32_t i;
    int32_t endIndex = selectedItemList.getLength();
    for (i = 1; i < endIndex; i++) {
        chemPath = (ChemPath *)selectedItemList[i];
        invokeSelectionPolicy(chemPath, TRUE, ctrlDown, FALSE);
        chemPath->unref();
    }

		selectedItemList.truncate(0);
// --> coin,  memory pointers aren't freed
#ifdef __coin
		selectedItemList.truncate(0,TRUE);
#endif
// <-- coin,  memory pointers aren't freed

    // Invoke the finishCBList if necessary
    if (finishCBList != NULL) {
        finishCBList->invokeCallbacks(this);
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Perform a radius pick operation on items in ChemDisplay nodes
//    using the selectedRadius.  The seleted items are appended to
//    the selectedItemList.
//
// Use: private       

void
ChemSelection::radiusPickChemDisplay(SbPList *selectedItemList)
//
////////////////////////////////////////////////////////////////////////
{
    // Get paths to all of the ChemDisplays which are children of this
    // ChemSelection node.  Note that "pl" needs to be freed before leaving
    // this method.
    SoPathList *pl = getPathList(ChemDisplay::getClassTypeId());
    
    if (pl == NULL) {
        return;
    }
            
    ////////////////////////////////////////////
    //
    // Go through the pathList and add the ChemDisplay pointers to
    // pickedNodeArray.  Only unique references will be added.
    //
    
    int32_t i;
    uint32_t endIndex = pl->getLength();

    ChemDisplay *chemDisplay;
    ChemBaseData *chemData;
    ChemDisplayParam *cdp;
    SbIntList lassoedList(50);
    int32_t lassoedIncr;
    
    // Create a new searchAction if necessary
    if (searchAction == NULL) {
        searchAction = new SoSearchAction;
    }

    ChemDisplayPath *chemPath;
    int32_t what = parts.getValue();
    int32_t chemBBoxWhat = 0;

    SbBool atomClipOnCenter = (atomLassoPolicy.getValue() == ChemSelection::CENTER);
    SbBool bondClipOnCenter = (bondLassoPolicy.getValue() == ChemSelection::CENTER);
    SbBool atomLabelClipOnCenter =
        (atomLabelLassoPolicy.getValue() == ChemSelection::CENTER);
    SbBool bondLabelClipOnCenter =
        (bondLabelLassoPolicy.getValue() == ChemSelection::CENTER);
// --> residue selection
		SbBool residueClipOnCenter = (residueLassoPolicy.getValue() == ChemSelection::CENTER);
		SbBool residueLabelClipOnCenter = (residueLabelLassoPolicy.getValue() == ChemSelection::CENTER);
// <-- residue selection

    // Figure out what to get with the ChemBBoxAction
    if (SELECT(what, ChemSelection::ATOMS)) {
        chemBBoxWhat |= ChemBBoxAction::ATOMS;
    }
    if (SELECT(what, ChemSelection::BONDS)) {
        chemBBoxWhat |= ChemBBoxAction::BONDS;
    }
    if (SELECT(what, ChemSelection::ATOMLABELS)) {
        chemBBoxWhat |= ChemBBoxAction::ATOMLABELS;
    }
    if (SELECT(what, ChemSelection::BONDLABELS)) {
        chemBBoxWhat |= ChemBBoxAction::BONDLABELS;
    }
// --> residue selection
		if (SELECT(what, ChemSelection::RESIDUES)) {
        chemBBoxWhat |= ChemBBoxAction::RESIDUES;
    }
		if (SELECT(what, ChemSelection::RESIDUELABELS)) {
        chemBBoxWhat |= ChemBBoxAction::RESIDUELABELS;
    }
// <-- residue selection

    // Set up ChemBBoxAction
    if (chemBBoxAction == NULL) {
        chemBBoxAction = new ChemBBoxAction;
    }

    int32_t loop;


    int32_t theItem;
    int32_t thePolicy;



    // For atoms
    ChemAtomBBox *atomBBoxes;

    // For bonds
    ChemBondBBox *bondBBoxes;

    // For labels
    ChemLabelBBox *atomLabelBBoxes;
    ChemLabelBBox *bondLabelBBoxes;

// --> residue selection
		// For residues
    ChemBondBBox *residueBBoxes;
		// For labels
    ChemLabelBBox *residueLabelBBoxes;
// <-- residue selection

    for (i = 0; i < endIndex; i++) {
        // Make sure we have the full path to the ChemDisplay node.
        SoFullPath *path = (SoFullPath *)(*pl)[i];
        assert(path->getTail()->isOfType(ChemDisplay::getClassTypeId()));
        chemDisplay = (ChemDisplay *)(path->getTail());
        assert(chemDisplay != NULL);

        SbMatrix modelMatrix = chemDisplay->getCurrentModelMatrix();

        chemPath = new ChemDisplayPath;
        chemPath->setSoPath(path);
        chemPath->ref();

        // Search the path for the ChemDisplayParam node.
        searchAction->reset();
        searchAction->setType(ChemDisplayParam::getClassTypeId());
        searchAction->setInterest(SoSearchAction::LAST);
        searchAction->apply(path);
        SoFullPath *thePath = (SoFullPath *)searchAction->getPath();
        cdp = (ChemDisplayParam *)(thePath->getTail());

        // Search the path for the ChemBaseData node.  The reset needs
        // to be done before each apply or else the result will be appended
        // to the list of returned paths.
        searchAction->reset();
        searchAction->setType(ChemBaseData::getClassTypeId());
        searchAction->setInterest(SoSearchAction::LAST);
        searchAction->apply(path);

        // Do the appropriate ChemBBoxAction
        if (chemBBoxWhat != 0) {
            chemBBoxAction->reset();
            chemBBoxAction->setParts(chemBBoxWhat);
            chemBBoxAction->setAtomClipOnCenter(atomClipOnCenter);
            chemBBoxAction->setBondClipOnCenter(bondClipOnCenter);
            chemBBoxAction->setAtomLabelClipOnCenter(atomLabelClipOnCenter);
            chemBBoxAction->setBondLabelClipOnCenter(bondLabelClipOnCenter);
// --> residue selection
						chemBBoxAction->setResidueClipOnCenter(residueClipOnCenter);
						chemBBoxAction->setResidueLabelClipOnCenter(residueLabelClipOnCenter);
// <-- residue selection

            chemBBoxAction->apply(path);
        }

        // Loop through all of the atoms in the current ChemDisplay node
        // and determine if they have been lassoed.
        thePath = (SoFullPath *)searchAction->getPath();
        chemData = (ChemBaseData *)(thePath->getTail());
        //int32_t numberOfAtoms = chemData->getNumberOfAtoms();
        //int32_t numberOfBonds = chemData->getNumberOfBonds();

        SbBool  doHydrogens = cdp->showHydrogens.getValue();

        if (SELECT(what, ChemSelection::ATOMS)) {
            chemBBoxAction->getAtomBBoxes(atomBBoxes);
            if ((atomBBoxes != NULL) && (atomBBoxes->numberOfAtoms > 0)) {
                thePolicy = atomLassoPolicy.getValue();
                lassoedList.truncate(0);
                lassoedIncr = 0;
                if (thePolicy == ChemSelection::CENTER) {
                    for (loop = 0; loop < atomBBoxes->numberOfAtoms; loop++) {
                        theItem = atomBBoxes->index[loop];
                        if (!doHydrogens) {
                            if (chemData->getAtomicNumber(theItem) == 1) {
                                continue;
                            }
                        }
                        SbVec3f theCoord;
                        modelMatrix.multVecMatrix(atomBBoxes->center[loop],
                            theCoord);
                        if (distSquared(theCoord, selectedAtomCoordX) <=
                             selectedRadiusSquared) {
                            // The atom has been lassoed so add it to the list
                            // of lassoed atoms which will be processed later.
// --> sphere selection error
//                            lassoedList[lassoedIncr++] = theItem;
													lassoedIncr++;
													lassoedList.append(theItem);
// <-- sphere selection error
                        }
                    }
                }
                else if (thePolicy == ChemSelection::THRU_BBOX) {
                    for (loop = 0; loop < atomBBoxes->numberOfAtoms; loop++) {
                        // If the distance between the atom center and the
                        // selectedAtom center is less than the sum of the
                        // atom radius and the selectedRadius, then the spheres
                        // overlap (intersect).
                        SbVec3f theCenter;
                        modelMatrix.multVecMatrix(atomBBoxes->center[loop],
                            theCenter);
                        float theSep =
                            (theCenter - selectedAtomCoordX).length();
                        if (theSep < 
                            (atomBBoxes->radius[loop] + selectedRadius)) {
// --> sphere selection error
//                            lassoedList[lassoedIncr++] = atomBBoxes->index[loop];
													lassoedIncr++;
													lassoedList.append(atomBBoxes->index[loop]);
// <-- sphere selection error
                        }
                    }
                }
                else if (thePolicy == ChemSelection::ENTIRE_BBOX) {
                    for (loop = 0; loop < atomBBoxes->numberOfAtoms; loop++) {
                        // If the (distance between the atomCenter and the
                        // selectedAtom center) + the atom radius is <=
                        // selectedRadius, then the atom is entirely within
                        // the selection sphere.
                        SbVec3f theCenter;
                        modelMatrix.multVecMatrix(atomBBoxes->center[loop],
                            theCenter);
                        float theSep =
                            (theCenter - selectedAtomCoordX).length();
        
                        if ((theSep + atomBBoxes->radius[loop]) <=
                             selectedRadius) {
// --> sphere selection error
//                            lassoedList[lassoedIncr++] = atomBBoxes->index[loop];
													lassoedIncr++;
													lassoedList.append(atomBBoxes->index[loop]);
// <-- sphere selection error
                        }
                    }
                }
                selectItems(lassoedList, chemPath->atomIndex);
            }
        } // end of selection of ATOMS 

        if (SELECT(what, ChemSelection::BONDS)) {
            int displayStyle = cdp->displayStyle.getValue();
            if ((displayStyle != ChemDisplayParam::DISPLAY_CPK) &&
                (displayStyle != ChemDisplayParam::DISPLAY_NONE)) {
                chemBBoxAction->getBondBBoxes(bondBBoxes);
                if ((bondBBoxes != NULL) && (bondBBoxes->numberOfBonds > 0)) {
                    thePolicy = bondLassoPolicy.getValue();
                    lassoedList.truncate(0);
                    lassoedIncr = 0;
                    if (thePolicy == ChemSelection::CENTER) {
                        for (loop = 0; loop < bondBBoxes->numberOfBonds; loop++) {
                            // The center of the bond must be within the selectedRadius
                            theItem = bondBBoxes->index[loop];
                            int32_t from = chemData->getBondFrom(theItem);
                            int32_t to = chemData->getBondTo(theItem);
        
                            if (!doHydrogens) {
                                if ((chemData->getAtomicNumber(from) == 1) ||
                                    (chemData->getAtomicNumber(to) == 1)) {
                                    continue;
                                }
                            }
        
                            SbVec3f theCoord;
                            modelMatrix.multVecMatrix(bondBBoxes->center[loop],
                                theCoord);
        
                            if (distSquared(theCoord, selectedAtomCoordX) <=
                                 selectedRadiusSquared) {
                                // The bond has been lassoed so add it to the list
                                // of lassoed bonds which will be processed later.
// --> sphere selection error
//                            lassoedList[lassoedIncr++] = theItem;
													lassoedIncr++;
													lassoedList.append(theItem);
// <-- sphere selection error
                            }
                        }
                    }
                    else if (thePolicy == ChemSelection::THRU_BBOX) {
                        for (loop = 0; loop < bondBBoxes->numberOfBonds; loop++) {
                            // If the bbox and sphere intersect, add the bond
                            // to the lassoed list.
                            if (bondBBoxSphereIntersect(
                                    bondBBoxes->silhouette[loop],
                                    modelMatrix,
                                    bondBBoxes->center[loop],
                                    selectedAtomCoordX,
                                    selectedRadiusSquared)) {
// --> sphere selection error
//                            lassoedList[lassoedIncr++] = bondBBoxes->index[loop];
															lassoedIncr++;
															lassoedList.append(bondBBoxes->index[loop]);
// <-- sphere selection error
                            }
                        }
                    }
                    else if (thePolicy == ChemSelection::ENTIRE_BBOX) {
                        // All 4 points of the bounding box must be within the
                        // selectedRadius
                        for (loop = 0; loop < bondBBoxes->numberOfBonds; loop++) {
                            Silhouette tmpBox;
                            for (i = 0; i < 4; i++) {
                                modelMatrix.multVecMatrix(
                                    bondBBoxes->silhouette[loop][i], tmpBox[i]);
                            }
                            if ((distSquared(tmpBox[0], selectedAtomCoordX) <=
                                 selectedRadiusSquared) &&
                                (distSquared(tmpBox[1], selectedAtomCoordX) <=
                                 selectedRadiusSquared) &&
                                (distSquared(tmpBox[2], selectedAtomCoordX) <=
                                 selectedRadiusSquared) &&
                                (distSquared(tmpBox[3], selectedAtomCoordX) <=
                                 selectedRadiusSquared)) {
// --> sphere selection error
//                            lassoedList[lassoedIncr++] = bondBBoxes->index[loop];
															lassoedIncr++;
															lassoedList.append(bondBBoxes->index[loop]);
// <-- sphere selection error

                            }
                        }
                    }
                    selectItems(lassoedList, chemPath->bondIndex);
                }
            }
        } // end of selection of BONDS 

        if (SELECT(what, ChemSelection::ATOMLABELS)) {
            chemBBoxAction->getAtomLabelBBoxes(atomLabelBBoxes);
            if ((atomLabelBBoxes != NULL) &&
                (atomLabelBBoxes->numberOfLabels > 0)) {
                thePolicy = atomLabelLassoPolicy.getValue();
                lassoedList.truncate(0);
                lassoedIncr = 0;
                if (thePolicy == ChemSelection::CENTER) {
                    for (loop = 0; loop < atomLabelBBoxes->numberOfLabels; loop++) {
                        SbVec3f theCoord;
                        modelMatrix.multVecMatrix(atomLabelBBoxes->center[loop],
                            theCoord);
                        if (distSquared(theCoord, selectedAtomCoordX) <=
                             selectedRadiusSquared) {
// --> sphere selection error
//                            lassoedList[lassoedIncr++] = atomLabelBBoxes->index[loop];
															lassoedIncr++;
															lassoedList.append(atomLabelBBoxes->index[loop]);
// <-- sphere selection error   
                        }
                    }
                }
                else if (thePolicy == ChemSelection::THRU_BBOX) {
                    for (loop = 0; loop < atomLabelBBoxes->numberOfLabels; loop++) {
                        // If the bbox and sphere intersect, add the atom label
                        // to the lassoed list.
                        if (boxSphereIntersect(atomLabelBBoxes->bbox[loop],
                                modelMatrix,
                                selectedAtomCoordX, selectedRadius)) {
// --> sphere selection error
//                            lassoedList[lassoedIncr++] = atomLabelBBoxes->index[loop];
															lassoedIncr++;
															lassoedList.append(atomLabelBBoxes->index[loop]);
// <-- sphere selection error   

                        }
                    }
                }
                else if (thePolicy == ChemSelection::ENTIRE_BBOX) {
                    for (loop = 0; loop < atomLabelBBoxes->numberOfLabels; loop++) {
                        SbVec3f minCoord;
                        SbVec3f maxCoord;
                        modelMatrix.multVecMatrix(
                            atomLabelBBoxes->bbox[loop].getMin(), minCoord);
                        modelMatrix.multVecMatrix(
                            atomLabelBBoxes->bbox[loop].getMax(), maxCoord);
                        if ((distSquared(minCoord, selectedAtomCoordX) <=
                             selectedRadiusSquared) &&
                            (distSquared(maxCoord, selectedAtomCoordX) <=
                             selectedRadiusSquared)) {
// --> sphere selection error
//                            lassoedList[lassoedIncr++] = atomLabelBBoxes->index[loop];
															lassoedIncr++;
															lassoedList.append(atomLabelBBoxes->index[loop]);
// <-- sphere selection error   
                        }
                    }
                }

                selectItems(lassoedList, chemPath->atomLabelIndex);

            } // end of (atomLabelBBoxes != NULL)
        } // end of selection of ATOMLABELS

        if (SELECT(what, ChemSelection::BONDLABELS)) {
            chemBBoxAction->getBondLabelBBoxes(bondLabelBBoxes);
            if ((bondLabelBBoxes != NULL) &&
                (bondLabelBBoxes->numberOfLabels > 0)) {
                lassoedList.truncate(0);
                lassoedIncr = 0;
                thePolicy = bondLabelLassoPolicy.getValue();
                if (thePolicy == ChemSelection::CENTER) {
                    for (loop = 0; loop < bondLabelBBoxes->numberOfLabels; loop++) {
                        SbVec3f theCoord;
                        modelMatrix.multVecMatrix(bondLabelBBoxes->center[loop],
                            theCoord);
                        if (distSquared(theCoord, selectedAtomCoordX) <=
                             selectedRadiusSquared) {
// --> sphere selection error
//                            lassoedList[lassoedIncr++] = bondLabelBBoxes->index[loop];
															lassoedIncr++;
															lassoedList.append(bondLabelBBoxes->index[loop]);
// <-- sphere selection error
                        }
                    }
                }
                else if (thePolicy == ChemSelection::THRU_BBOX) {
                    for (loop = 0; loop < bondLabelBBoxes->numberOfLabels; loop++) {
                        // If the bbox and sphere intersect, add the bond label
                        // to the lassoed list.
                        if (boxSphereIntersect(bondLabelBBoxes->bbox[loop],
                                modelMatrix,
                                selectedAtomCoordX, selectedRadius)) {
// --> sphere selection error
//                            lassoedList[lassoedIncr++] = bondLabelBBoxes->index[loop];
															lassoedIncr++;
															lassoedList.append(bondLabelBBoxes->index[loop]);
// <-- sphere selection error
                        }
                    }
                }
                else if (thePolicy == ChemSelection::ENTIRE_BBOX) {
                    for (loop = 0; loop < bondLabelBBoxes->numberOfLabels; loop++) {
                        SbVec3f minCoord;
                        SbVec3f maxCoord;
                        modelMatrix.multVecMatrix(
                            bondLabelBBoxes->bbox[loop].getMin(), minCoord);
                        modelMatrix.multVecMatrix(
                            bondLabelBBoxes->bbox[loop].getMax(), maxCoord);
                        if ((distSquared(minCoord, selectedAtomCoordX) <=
                             selectedRadiusSquared) &&
                            (distSquared(maxCoord, selectedAtomCoordX) <=
                             selectedRadiusSquared)) {
// --> sphere selection error
//                            lassoedList[lassoedIncr++] = bondLabelBBoxes->index[loop];
															lassoedIncr++;
															lassoedList.append(bondLabelBBoxes->index[loop]);
// <-- sphere selection error
                        }
                    }
                }

                selectItems(lassoedList, chemPath->bondLabelIndex);

            } // end of (bondLabelBBoxes != NULL)
        } // end of selection of BONDLABELS
// --> residue selection
				if (SELECT(what, ChemSelection::RESIDUES))
				{
            int displayStyle = cdp->residueDisplayStyle.getValue();
            if (displayStyle == ChemDisplayParam::DISPLAY_RESIDUES_CAWIRE ||
								displayStyle == ChemDisplayParam::DISPLAY_RESIDUES_CASTICK)
						{
                chemBBoxAction->getResidueBBoxes(residueBBoxes);
                if ((residueBBoxes != NULL) && (residueBBoxes->numberOfBonds > 0))
								{        
                    thePolicy = residueLassoPolicy.getValue();
                    lassoedList.truncate(0);
                    lassoedIncr = 0;
                    if (thePolicy == ChemSelection::CENTER)
										{
                        for (loop = 0; loop < residueBBoxes->numberOfBonds; loop++)
												{
                            theItem = residueBBoxes->index[loop];
        
                            SbVec3f theCoord;
		                        modelMatrix.multVecMatrix(residueBBoxes->center[loop],
                            theCoord);
														if (distSquared(theCoord, selectedAtomCoordX) <=
                             selectedRadiusSquared)
														{
															lassoedIncr++;
															lassoedList.append(theItem);
                            }
                        }
                    }
                    else if (thePolicy == ChemSelection::THRU_BBOX)
										{
											// TO DO:
											SoDebugError::post("ChemSelection::radiusPickChemDisplay","CA selection THRU_BBOX not implemented");
                    }
                    else //if (thePolicy == ChemSelection::ENTIRE_BBOX)
										{
											// TO DO:
											SoDebugError::post("ChemSelection::radiusPickChemDisplay","CA selection ENTIRE_BBOX not implemented");
                    }
                }
// --> residue atoms selection
								if (SELECT(what, ChemSelection::ATOMS))
								selectResidueAtoms(lassoedList,chemPath->atomIndex,(ChemData*)chemData,cdp->residueSelectAtoms.getValue());
// <-- residue atoms selection
						} // castick,cawire
						else if (displayStyle == ChemDisplayParam::DISPLAY_RESIDUES_LINERIBBON ||
							displayStyle == ChemDisplayParam::DISPLAY_RESIDUES_FLATRIBBON || 
							displayStyle == ChemDisplayParam::DISPLAY_RESIDUES_SOLIDRIBBON)
						{
							chemBBoxAction->getResidueBBoxes(residueBBoxes);
							thePolicy = residueLassoPolicy.getValue();
							lassoedList.truncate(0);
							lassoedIncr = 0;
							if ((residueBBoxes != NULL) && (residueBBoxes->numberOfBonds > 0))
							{
									if (thePolicy == ChemSelection::CENTER)
									{
											for (loop = 0; loop < residueBBoxes->numberOfBonds; loop++)
											{
												theItem = residueBBoxes->index[loop];

												SbVec3f theCoord;
												modelMatrix.multVecMatrix(residueBBoxes->center[loop],
												theCoord);
												if (distSquared(theCoord, selectedAtomCoordX) <=
												selectedRadiusSquared)
												{
													lassoedIncr++;
													lassoedList.append(theItem);
												}
											}
									}
									else if (thePolicy == ChemSelection::THRU_BBOX)
									{
										// TO DO:
										SoDebugError::post("ChemSelection::radiusPickChemDisplay","ribbon selection THRU_BBOX not implemented");
									}
									else // if (thePolicy == ChemSelection::ENTIRE_BBOX)
									{
										// TO DO:
										SoDebugError::post("ChemSelection::radiusPickChemDisplay","ribbon selection ENTIRE_BBOX not implemented");
									}
									// --> residue atoms selection
									if (SELECT(what, ChemSelection::ATOMS))
										selectResidueAtoms(lassoedList,chemPath->atomIndex,(ChemData*)chemData,cdp->residueSelectAtoms.getValue());
									// <-- residue atoms selection
							} // bboxes != NULL
					} //ribbons
					
					if (displayStyle != ChemDisplayParam::DISPLAY_RESIDUES_SCHEMATIC)
					{
						selectItems(lassoedList, chemPath->residueIndex);
					}
        } // end of selection of RESIDUES

				if (SELECT(what, ChemSelection::RESIDUELABELS))
				{
            chemBBoxAction->getResidueLabelBBoxes(residueLabelBBoxes);
            if ((residueLabelBBoxes != NULL) &&
                (residueLabelBBoxes->numberOfLabels > 0))
						{
                thePolicy = residueLabelLassoPolicy.getValue();
                lassoedList.truncate(0);
                lassoedIncr = 0;
                if (thePolicy == ChemSelection::CENTER)
								{
                    for (loop = 0; loop < residueLabelBBoxes->numberOfLabels; loop++)
										{
                        SbVec3f theCoord;
                        modelMatrix.multVecMatrix(residueLabelBBoxes->center[loop],
                            theCoord);
                        if (distSquared(theCoord, selectedAtomCoordX) <=
                             selectedRadiusSquared) {
															lassoedIncr++;
															lassoedList.append(residueLabelBBoxes->index[loop]);
                        }
                    }
                }
                else if (thePolicy == ChemSelection::THRU_BBOX)
								{
                    for (loop = 0; loop < residueLabelBBoxes->numberOfLabels; loop++)
										{
                        // If the bbox and sphere intersect, add the residue label
                        // to the lassoed list.
                        if (boxSphereIntersect(residueLabelBBoxes->bbox[loop],
                                modelMatrix,
                                selectedAtomCoordX, selectedRadius)) {
															lassoedIncr++;
															lassoedList.append(residueLabelBBoxes->index[loop]);
                        }
                    }
                }
                else if (thePolicy == ChemSelection::ENTIRE_BBOX)
								{
                    for (loop = 0; loop < residueLabelBBoxes->numberOfLabels; loop++)
										{
                        SbVec3f minCoord;
                        SbVec3f maxCoord;
                        modelMatrix.multVecMatrix(
                            atomLabelBBoxes->bbox[loop].getMin(), minCoord);
                        modelMatrix.multVecMatrix(
                            atomLabelBBoxes->bbox[loop].getMax(), maxCoord);
                        if ((distSquared(minCoord, selectedAtomCoordX) <=
                             selectedRadiusSquared) &&
                            (distSquared(maxCoord, selectedAtomCoordX) <=
                             selectedRadiusSquared))
												{
															lassoedIncr++;
															lassoedList.append(residueLabelBBoxes->index[loop]);
                        }
                    }
                }

                selectItems(lassoedList, chemPath->residueLabelIndex);

            } // end of (residueLabelBBoxes != NULL)
        } // end of selection of RESIDUELABELS
// <-- residue selection

        selectedItemList->append(chemPath);

    }

    if (chemBBoxAction != NULL) {
        chemBBoxAction->reset();
    }
    delete pl;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Perform a radius pick operation on labels in ChemLabel nodes
//    using the selectedRadius.  The seleted labels are appended to
//    the selectedItemList.
//
// Use: private       

void
ChemSelection::radiusPickChemLabel(SbPList *selectedItemList)
//
////////////////////////////////////////////////////////////////////////
{
    // Get paths to all of the ChemLabels which are children of this
    // ChemSelection node.  Note that "pl" needs to be freed before leaving
    // this method.
    SoPathList *pl = getPathList(ChemLabel::getClassTypeId());
    
    if (pl == NULL) {
        return;
    }
            
    ////////////////////////////////////////////
    //
    // Go through the pathList and add the ChemLabel pointers to
    // pickedNodeArray.  Only unique references will be added.
    //
    
    int32_t i;
    uint32_t endIndex = pl->getLength();

    ChemLabel *chemLabel;
    SbIntList lassoedList(50);
    int32_t lassoedIncr;
    
    // Create a new searchAction if necessary
    if (searchAction == NULL) {
        searchAction = new SoSearchAction;
    }

    ChemLabelPath *chemPath;
    int32_t what = parts.getValue();
    int32_t chemBBoxWhat = 0;

    SbBool chemLabelClipOnCenter =
        (chemLabelLassoPolicy.getValue() == ChemSelection::CENTER);

    // Figure out what to get with the ChemBBoxAction
    if (SELECT(what, ChemSelection::CHEMLABELS)) {
        chemBBoxWhat |= ChemBBoxAction::CHEMLABELS;
    }
    // Set up ChemBBoxAction
    if (chemBBoxAction == NULL) {
        chemBBoxAction = new ChemBBoxAction;
    }

    int32_t loop;
    int32_t thePolicy;
    

    // For labels
    ChemLabelBBox *chemLabelBBoxes;

    for (i = 0; i < endIndex; i++) {
        // Make sure we have the full path to the ChemDisplay node.
        SoFullPath *path = (SoFullPath *)(*pl)[i];
        assert(path->getTail()->isOfType(ChemLabel::getClassTypeId()));
        chemLabel = (ChemLabel *)(path->getTail());

        SbMatrix modelMatrix = chemLabel->getCurrentModelMatrix();

        chemPath = new ChemLabelPath;
        chemPath->setSoPath(path);
        chemPath->ref();

        // Do the appropriate ChemBBoxAction
        if (chemBBoxWhat != 0) {
            chemBBoxAction->reset();
            chemBBoxAction->setParts(chemBBoxWhat);
            chemBBoxAction->setChemLabelClipOnCenter(chemLabelClipOnCenter);
            chemBBoxAction->apply(path);
        }

        // Loop through all of the labels in the current ChemLabel node
        // and determine if they have been lassoed.
        chemBBoxAction->getChemLabelBBoxes(chemLabelBBoxes);
        if ((chemLabelBBoxes != NULL) &&
            (chemLabelBBoxes->numberOfLabels > 0)) {
            thePolicy = chemLabelLassoPolicy.getValue();
            lassoedList.truncate(0);
            lassoedIncr = 0;
            if (thePolicy == ChemSelection::CENTER) {
                for (loop = 0; loop < chemLabelBBoxes->numberOfLabels; loop++) {
                    SbVec3f theCoord;
                    modelMatrix.multVecMatrix(chemLabelBBoxes->center[loop],
                        theCoord);
                    if (distSquared(theCoord, selectedAtomCoordX) <=
                         selectedRadiusSquared) {
                        lassoedList[lassoedIncr++] =
                            chemLabelBBoxes->index[loop];
                    }
                }
            }
            else if (thePolicy == ChemSelection::THRU_BBOX) {
                for (loop = 0; loop < chemLabelBBoxes->numberOfLabels; loop++) {
                    // If the bbox and sphere intersect, add the chem label
                    // to the lassoed list.
                    if (boxSphereIntersect(chemLabelBBoxes->bbox[loop],
                            modelMatrix,
                            selectedAtomCoordX, selectedRadius)) {
                        lassoedList[lassoedIncr++] = 
                            chemLabelBBoxes->index[loop];
                    }
                }
            }
            else if (thePolicy == ChemSelection::ENTIRE_BBOX) {
                for (loop = 0; loop < chemLabelBBoxes->numberOfLabels; loop++) {
                    SbVec3f minCoord;
                    SbVec3f maxCoord;
                    modelMatrix.multVecMatrix(
                        chemLabelBBoxes->bbox[loop].getMin(), minCoord);
                    modelMatrix.multVecMatrix(
                        chemLabelBBoxes->bbox[loop].getMax(), maxCoord);
                    if ((distSquared(minCoord, selectedAtomCoordX) <=
                         selectedRadiusSquared) &&
                        (distSquared(maxCoord, selectedAtomCoordX) <=
                         selectedRadiusSquared)) {
                        lassoedList[lassoedIncr++] =
                            chemLabelBBoxes->index[loop];
                    }
                }
            }

            selectItems(lassoedList, chemPath->labelIndex);
            selectedItemList->append(chemPath);

        } // end of (chemLabelBBoxes != NULL)
    } // end of loop over paths to ChemLabel nodes

    if (chemBBoxAction != NULL) {
        chemBBoxAction->reset();
    }
    delete pl;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Perform a radius pick operation on monitors in ChemMonitor nodes
//    using the selectedRadius.  The seleted monitors are appended to
//    the selectedItemList.
//
// Use: private       

void
ChemSelection::radiusPickChemMonitor(SbPList *selectedItemList)
//
////////////////////////////////////////////////////////////////////////
{
    // Get paths to all of the ChemMonitors which are children of this
    // ChemSelection node.  Note that "pl" needs to be freed before leaving
    // this method.
    SoPathList *pl = getPathList(ChemMonitor::getClassTypeId());
    
    if (pl == NULL) {
        return;
    }
            
    ////////////////////////////////////////////
    //
    // Go through the pathList and add the ChemMonitor pointers to
    // pickedNodeArray.  Only unique references will be added.
    //
    
    int32_t i;
    uint32_t endIndex = pl->getLength();

    ChemMonitor *chemMonitor;
    SbIntList lassoedList(50);
    int32_t lassoedIncr;
    
    // Create a new searchAction if necessary
    if (searchAction == NULL) {
        searchAction = new SoSearchAction;
    }

    ChemMonitorPath *chemPath;
    int32_t what = parts.getValue();
    int32_t chemBBoxWhat = 0;

    SbBool chemMonitorClipOnCenter =
        (chemMonitorLassoPolicy.getValue() == ChemSelection::CENTER);

    // Figure out what to get with the ChemBBoxAction
    if (SELECT(what, ChemSelection::CHEMMONITORS)) {
        chemBBoxWhat |= ChemBBoxAction::CHEMMONITORS;
    }
    // Set up ChemBBoxAction
    if (chemBBoxAction == NULL) {
        chemBBoxAction = new ChemBBoxAction;
    }

    int32_t loop;
    int32_t thePolicy;
    
    // For monitors
    ChemMonitorBBox *chemMonitorBBoxes;

    for (i = 0; i < endIndex; i++) {
        // Make sure we have the full path to the ChemDisplay node.
        SoFullPath *path = (SoFullPath *)(*pl)[i];
        assert(path->getTail()->isOfType(ChemMonitor::getClassTypeId()));
        chemMonitor = (ChemMonitor *)(path->getTail());

        SbMatrix modelMatrix = chemMonitor->getCurrentModelMatrix();

        chemPath = new ChemMonitorPath;
        chemPath->setSoPath(path);
        chemPath->ref();

        // Do the appropriate ChemBBoxAction
        if (chemBBoxWhat != 0) {
            chemBBoxAction->reset();
            chemBBoxAction->setParts(chemBBoxWhat);
            chemBBoxAction->setChemMonitorClipOnCenter(chemMonitorClipOnCenter);
            chemBBoxAction->apply(path);
        }

        // Loop through all of the monitors in the current ChemMonitor node
        // and determine if they have been lassoed.
        chemBBoxAction->getChemMonitorBBoxes(chemMonitorBBoxes);
        if (chemMonitorBBoxes != NULL) {
            thePolicy = chemMonitorLassoPolicy.getValue();
            if (thePolicy == ChemSelection::CENTER) {
                lassoedList.truncate(0);
                lassoedIncr = 0;
                for (loop = 0; loop < chemMonitorBBoxes->numberOfDistanceMonitors; loop++) {
                    SbVec3f theCoord;
                    modelMatrix.multVecMatrix(chemMonitorBBoxes->distanceCenter[loop],
                        theCoord);
                    if (distSquared(theCoord, selectedAtomCoordX) <=
                         selectedRadiusSquared) {
                        lassoedList[lassoedIncr++] =
                            chemMonitorBBoxes->distanceIndex[loop];
                    }
                }
                selectItems(lassoedList, chemPath->distanceIndex);

                lassoedList.truncate(0);
                lassoedIncr = 0;
                for (loop = 0; loop < chemMonitorBBoxes->numberOfAngleMonitors; loop++) {
                    SbVec3f theCoord;
                    modelMatrix.multVecMatrix(chemMonitorBBoxes->angleCenter[loop],
                        theCoord);
                    if (distSquared(theCoord, selectedAtomCoordX) <=
                         selectedRadiusSquared) {
                        lassoedList[lassoedIncr++] =
                            chemMonitorBBoxes->angleIndex[loop];
                    }
                }
                selectItems(lassoedList, chemPath->angleIndex);

                lassoedList.truncate(0);
                lassoedIncr = 0;
                for (loop = 0; loop < chemMonitorBBoxes->numberOfTorsionalMonitors; loop++) {
                    SbVec3f theCoord;
                    modelMatrix.multVecMatrix(chemMonitorBBoxes->torsionalCenter[loop],
                        theCoord);
                    if (distSquared(theCoord, selectedAtomCoordX) <=
                         selectedRadiusSquared) {
                        lassoedList[lassoedIncr++] =
                            chemMonitorBBoxes->torsionalIndex[loop];
                    }
                }
                selectItems(lassoedList, chemPath->torsionalIndex);
            }
            else if (thePolicy == ChemSelection::THRU_BBOX) {
                lassoedList.truncate(0);
                lassoedIncr = 0;
                for (loop = 0; loop < chemMonitorBBoxes->numberOfDistanceMonitors; loop++) {
                    // If the bbox and sphere intersect, add the chem monitor
                    // to the lassoed list.
                    if (boxSphereIntersect(chemMonitorBBoxes->distanceBBox[loop],
                            modelMatrix,
                            selectedAtomCoordX, selectedRadius)) {
                        lassoedList[lassoedIncr++] = 
                            chemMonitorBBoxes->distanceIndex[loop];
                    }
                }
                selectItems(lassoedList, chemPath->distanceIndex);

                lassoedList.truncate(0);
                lassoedIncr = 0;
                for (loop = 0; loop < chemMonitorBBoxes->numberOfAngleMonitors; loop++) {
                    // If the bbox and sphere intersect, add the chem monitor
                    // to the lassoed list.
                    if (boxSphereIntersect(chemMonitorBBoxes->angleBBox[loop],
                            modelMatrix,
                            selectedAtomCoordX, selectedRadius)) {
                        lassoedList[lassoedIncr++] = 
                            chemMonitorBBoxes->angleIndex[loop];
                    }
                }
                selectItems(lassoedList, chemPath->angleIndex);

                lassoedList.truncate(0);
                lassoedIncr = 0;
                for (loop = 0; loop < chemMonitorBBoxes->numberOfTorsionalMonitors; loop++) {
                    // If the bbox and sphere intersect, add the chem monitor
                    // to the lassoed list.
                    if (boxSphereIntersect(chemMonitorBBoxes->torsionalBBox[loop],
                            modelMatrix,
                            selectedAtomCoordX, selectedRadius)) {
                        lassoedList[lassoedIncr++] = 
                            chemMonitorBBoxes->torsionalIndex[loop];
                    }
                }
                selectItems(lassoedList, chemPath->torsionalIndex);
            }
            else if (thePolicy == ChemSelection::ENTIRE_BBOX) {
                lassoedList.truncate(0);
                lassoedIncr = 0;
                for (loop = 0; loop < chemMonitorBBoxes->numberOfDistanceMonitors; loop++) {
                    SbVec3f minCoord;
                    SbVec3f maxCoord;
                    modelMatrix.multVecMatrix(
                        chemMonitorBBoxes->distanceBBox[loop].getMin(), minCoord);
                    modelMatrix.multVecMatrix(
                        chemMonitorBBoxes->distanceBBox[loop].getMax(), maxCoord);
                    if ((distSquared(minCoord, selectedAtomCoordX) <=
                         selectedRadiusSquared) &&
                        (distSquared(maxCoord, selectedAtomCoordX) <=
                         selectedRadiusSquared)) {
                        lassoedList[lassoedIncr++] =
                            chemMonitorBBoxes->distanceIndex[loop];
                    }
                }
                selectItems(lassoedList, chemPath->distanceIndex);

                lassoedList.truncate(0);
                lassoedIncr = 0;
                for (loop = 0; loop < chemMonitorBBoxes->numberOfAngleMonitors; loop++) {
                    SbVec3f minCoord;
                    SbVec3f maxCoord;
                    modelMatrix.multVecMatrix(
                        chemMonitorBBoxes->angleBBox[loop].getMin(), minCoord);
                    modelMatrix.multVecMatrix(
                        chemMonitorBBoxes->angleBBox[loop].getMax(), maxCoord);
                    if ((distSquared(minCoord, selectedAtomCoordX) <=
                         selectedRadiusSquared) &&
                        (distSquared(maxCoord, selectedAtomCoordX) <=
                         selectedRadiusSquared)) {
                        lassoedList[lassoedIncr++] =
                            chemMonitorBBoxes->angleIndex[loop];
                    }
                }
                selectItems(lassoedList, chemPath->angleIndex);

                lassoedList.truncate(0);
                lassoedIncr = 0;
                for (loop = 0; loop < chemMonitorBBoxes->numberOfTorsionalMonitors; loop++) {
                    SbVec3f minCoord;
                    SbVec3f maxCoord;
                    modelMatrix.multVecMatrix(
                        chemMonitorBBoxes->torsionalBBox[loop].getMin(), minCoord);
                    modelMatrix.multVecMatrix(
                        chemMonitorBBoxes->torsionalBBox[loop].getMax(), maxCoord);
                    if ((distSquared(minCoord, selectedAtomCoordX) <=
                         selectedRadiusSquared) &&
                        (distSquared(maxCoord, selectedAtomCoordX) <=
                         selectedRadiusSquared)) {
                        lassoedList[lassoedIncr++] =
                            chemMonitorBBoxes->torsionalIndex[loop];
                    }
                }
                selectItems(lassoedList, chemPath->torsionalIndex);
            }

            selectedItemList->append(chemPath);

        } // end of (chemMonitorBBoxes != NULL)
    } // end of loop over paths to ChemMonitor nodes

    if (chemBBoxAction != NULL) {
        chemBBoxAction->reset();
    }
    delete pl;
}
                      
////////////////////////////////////////////////////////////////////////
//
// Description:
//  add the point in the passed action to the lassoPoints list
//
// Use: private
         
void    
ChemSelection::addPointToLasso(SoHandleEventAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    char tmpString[64];
    const SbVec2f &point = getModelCoordinates(action);
#ifdef IV2_0
    SoCoordinate3 *coords = getLassoPoints();
    int32_t n = coords->point.getNum();
#else
    SoVertexProperty *vp = getLassoPoints();
    int32_t n = vp->vertex.getNum();
#endif
    static SbBool sphereSwitchOn = FALSE;
                            
    // If "radiusSelect" is TRUE and the user selected an atom, then
    // "useRadiusSelection" will be on and will override LASSO and DRAGGER
    // selection.
    if (useRadiusSelection) {
        if (radiusSelectionFirstPoint) {
            sphereSwitchOn = FALSE;
            radiusSelectionFirstPoint = FALSE;
            selectedRadius = 0.0;
            selectedRadiusSquared = 0.0;
            sphereBBox.makeEmpty();
            sphere->radius.setValue(selectedRadius);
            sphereTransform->translation.setValue(sphereCoord);
            labelTransform->translation.setValue(0, 0, 0);
            labelText->string.set1Value(0, "");
        }
        else {
            const SbViewVolume &viewVol =
                SoViewVolumeElement::get(action->getState());
            SbVec2f tmpPoint;
            SbLine  theLine;
            SbVec3f toward;

            if (!sphereSwitchOn) {
                sphereSwitchOn = TRUE;
                sceneSwitch->whichChild.setValue(0);
            }
            tmpPoint[0] = point[0] / orthoCamXres;
            tmpPoint[1] = point[1] / orthoCamYres;
            viewVol.projectPointToLine(tmpPoint, theLine);

#ifdef KEEP
            if (viewVol.getProjectionType() == SbViewVolume::PERSPECTIVE) {
                toward = viewVol.getProjectionPoint() - selectedAtomCoordX;
                toward.normalize();
            }
            else {
                toward = -viewVol.getProjectionDirection();
            }
#endif

            SbVec3f closestPoint = theLine.getClosestPoint(selectedAtomCoordX);
            selectedRadius = (closestPoint - selectedAtomCoordX).length();
            selectedRadiusSquared = selectedRadius * selectedRadius;

            sphere->radius.setValue(selectedRadius);
            SbVec3f tmpRad(selectedRadius, selectedRadius, selectedRadius);
            sphereBBox.setBounds(selectedAtomCoordX - tmpRad,
                selectedAtomCoordX + tmpRad);

            SbMatrix   theMatrix;
            SbVec3f    theScale;
            SbVec3f    theZAxis(0, 0, 1);
            SbVec3f    theOrigin(0, 0, 0);

            float rad = selectedRadius + 0.1;
            theScale.setValue(rad, rad, rad);
            theMatrix.setTransform(theOrigin, currentRotation, theScale);
            theMatrix.multVecMatrix(theZAxis, toward);
            labelTransform->translation.setValue(toward);

#ifdef KEEP
            labelTransform->translation.setValue(
                toward * (selectedRadius + 0.1));
#endif

            sprintf(tmpString, "%7.3f", selectedRadius);
            labelText->string.set1Value(0, tmpString);
        }
    }
    else if (lassoType.getValue() == ChemSelection::LASSO) {
        // don't add a point if it is coincident after the space xform
#ifdef IV2_0
        if (n > 0 &&
            (coords->point[n-1][0] == point[0] &&
             coords->point[n-1][1] == point[1]))
            return;
            
        SbVec3f p(point[0], point[1], 0.0);
        coords->point.set1Value(n, p);
        lassoBBox.extendBy(p);
#else
        if (n > 0 &&
            (vp->vertex[n-1][0] == point[0] &&
             vp->vertex[n-1][1] == point[1]))
            return;
            
        // grrr. Have to do this because getLassoPoints()->point is a vec3f
        SbVec3f p(point[0], point[1], 0.0);
        vp->vertex.set1Value(n, p);
        lassoBBox.extendBy(p);
#endif
    }
    else if (lassoType.getValue() == ChemSelection::DRAGGER) {
        SbVec3f p(point[0], point[1], 0.0);
#ifdef IV2_0
        if (n == 0) {
            coords->point.set1Value(0, p);
        }
        else {
            const SbVec3f vertexPoint = coords->point[0];
            SbVec3f tmpPoint;

            lassoBBox.makeEmpty();
            if (p == vertexPoint) {
                coords->point.deleteValues(1);
                return;
            }
            lassoBBox.extendBy(vertexPoint);
            tmpPoint[0] = vertexPoint[0];
            tmpPoint[1] = p[1];
            tmpPoint[2] = 0.0;
            coords->point.set1Value(1, tmpPoint);
            coords->point.set1Value(2, p);
            tmpPoint[0] = p[0];
            tmpPoint[1] = vertexPoint[1];
            tmpPoint[2] = 0.0;
            coords->point.set1Value(3, tmpPoint);
            coords->point.set1Value(4, vertexPoint);
            lassoBBox.extendBy(p);
        }
#else
        if (n == 0) {
            vp->vertex.set1Value(0, p);
        }
        else {
            const SbVec3f vertexPoint = vp->vertex[0];
            SbVec3f tmpPoint;

            lassoBBox.makeEmpty();
            if (p == vertexPoint) {
                vp->vertex.deleteValues(1);
                return;
            }
            lassoBBox.extendBy(vertexPoint);
            tmpPoint[0] = vertexPoint[0];
            tmpPoint[1] = p[1];
            tmpPoint[2] = 0.0;
            vp->vertex.set1Value(1, tmpPoint);
            vp->vertex.set1Value(2, p);
            tmpPoint[0] = p[0];
            tmpPoint[1] = vertexPoint[1];
            tmpPoint[2] = 0.0;
            vp->vertex.set1Value(3, tmpPoint);
            vp->vertex.set1Value(4, vertexPoint);
            lassoBBox.extendBy(p);
        }
#endif
    }
}

////////////////////////////////////////////////////////////////////////
// 
// Description:
//  return a list of paths to all nodes of classTypeId.  If none exist
//  return NULL.
//              
// Use: protected
    
SoPathList *   
ChemSelection::getPathList(SoType classTypeId)
//                  
////////////////////////////////////////////////////////////////////////
{

    if (searchAction == NULL)
        searchAction = new SoSearchAction;
    else
        searchAction->reset();

    searchAction->setType(classTypeId);
    searchAction->setInterest(SoSearchAction::ALL);
    searchAction->apply(this);
    return (searchAction->getPaths().getLength() > 0) ?
        new SoPathList(searchAction->getPaths()) : NULL;
}
  
////////////////////////////////////////////////////////////////////////  
//
// Description:
//    Convert the passed mouse coordinates to model coords.  These are 0 to 1   
//    along the shorter axis and values of equal aspect centered at 0.5 along
//    the longer axis.
//
// Use: protected

SbVec2f
ChemSelection::getModelCoordinates(SoHandleEventAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    const SoEvent      *event = action->getEvent();
    const SbViewportRegion &vprgn = action->getViewportRegion();
    const SbVec2f &np = event->getNormalizedPosition(vprgn);
    SbVec2f p(np[0] * orthoCamXres, np[1] * orthoCamYres);

    return p;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    If the user picks a single point, process that point to see what
//    was selected.
//
// Use: private   

ChemPath *
ChemSelection::processPickedPoint(const SoPickedPoint *pp)
//
////////////////////////////////////////////////////////////////////////
{
    // Get the path of the picked point.

    // If the pickedPoint or the pickedPoint path are NULL, then return.
    if (pp == NULL) return NULL;
    SoFullPath *path = (SoFullPath *)pp->getPath();
    if (path == NULL) return NULL;

    int32_t what = parts.getValue();

    // Make sure that the picked node was a ChemDisplay.
    if (path->getTail()->isOfType(ChemDisplay::getClassTypeId())) {
        if ((SELECT(what, ChemSelection::ATOMS)) ||
            (SELECT(what, ChemSelection::BONDS)) ||
            (SELECT(what, ChemSelection::ATOMLABELS)) ||
            (SELECT(what, ChemSelection::BONDLABELS))
// --> residue selection
						|| (SELECT(what, ChemSelection::RESIDUES))
						|| (SELECT(what, ChemSelection::RESIDUELABELS))
// <-- residue selection
						) {
            ChemDisplayPath *chemPath;
            chemPath = new ChemDisplayPath;
            chemPath->setSoPath(path);

            // Get the ChemDetail which will tell us what was picked.  Add this
            // information to the ChemDisplayPath.
            const ChemDetail *chemDetail = (const ChemDetail *)pp->getDetail();
            if (chemDetail != NULL) {
                int32_t atomIndex, bondIndex;
                int32_t atomLabelIndex, bondLabelIndex;
                chemDetail->getAtomBondIndex(atomIndex, bondIndex);
                chemDetail->getAtomBondLabelIndex(atomLabelIndex,
                                                  bondLabelIndex);
// --> residue selection
								int32_t residueIndex = chemDetail->getResidueIndex();
								int32_t residueLabelIndex = chemDetail->getResidueLabelIndex();
// <-- residue selection
// --> schematic
                int32_t schematicIndex = chemDetail->getSchematicIndex();
// <-- schematic
                if (SELECT(what, ChemSelection::ATOMS) && (atomIndex != -1)) {
                    chemPath->atomIndex.set1Value(0, SbVec2i(atomIndex, 1));
                }
                if (SELECT(what, ChemSelection::BONDS) && (bondIndex != -1)) {
                    chemPath->bondIndex.set1Value(0, SbVec2i(bondIndex, 1));
                }
                if (SELECT(what, ChemSelection::ATOMLABELS) &&
                    (atomLabelIndex != -1)) {
                    chemPath->atomLabelIndex.set1Value(0, SbVec2i(atomLabelIndex, 1));
                }
                if (SELECT(what, ChemSelection::BONDLABELS) &&
                    (bondLabelIndex != -1)) {
                    chemPath->bondLabelIndex.set1Value(0, SbVec2i(bondLabelIndex, 1));
                }
// --> residue selection
								if (SELECT(what, ChemSelection::RESIDUES))
								{
                  if (residueIndex != -1)
                  {
                      chemPath->residueIndex.set1Value(0, SbVec2i(residueIndex, 1));
// --> residue atoms selection
                      if (SELECT(what, ChemSelection::ATOMS))
                      {
	                      ChemData *chemData = NULL;
	                      ChemDisplayParam *cdp = NULL;
	                      SoFullPath *thePath = NULL;
	                      if (searchAction == NULL)
		                      searchAction = new SoSearchAction;
	                      // Search the path for the ChemDisplayParam node.
	                      searchAction->reset();
	                      searchAction->setType(ChemDisplayParam::getClassTypeId());
	                      searchAction->setInterest(SoSearchAction::LAST);
	                      searchAction->apply(path);
	                      thePath = (SoFullPath *)searchAction->getPath();
	                      cdp = (ChemDisplayParam *)(thePath->getTail());
	                      // Search the path for the ChemBaseData node.
	                      searchAction->reset();
	                      searchAction->setType(ChemBaseData::getClassTypeId());
	                      searchAction->setInterest(SoSearchAction::LAST);
	                      searchAction->apply(path);
	                      thePath = (SoFullPath *)searchAction->getPath();
	                      chemData = (ChemData*)(thePath->getTail());

	                      if (chemData != NULL && cdp != NULL)
	                      {
		                      SbIntList list;
		                      list.append(residueIndex);
		                      selectResidueAtoms(list,chemPath->atomIndex,chemData,cdp->residueSelectAtoms.getValue());
	                      }
                      }
// <-- residue atoms selection
                  } // residueIndex != -1
// --> schematic
                  if (schematicIndex != -1)
                  {
                     chemPath->schematicIndex.set1Value(0, SbVec2i(schematicIndex, 1));
                  }
// <-- schematic
                } // select residues
								if (SELECT(what, ChemSelection::RESIDUELABELS) &&
									(residueLabelIndex != -1))
								{
									chemPath->residueLabelIndex.set1Value(0, SbVec2i(residueLabelIndex, 1));
								}
// <-- residue selection
            }
            return chemPath;
        }
    }
    else if (path->getTail()->isOfType(ChemLabel::getClassTypeId())) {
        if (SELECT(what, ChemSelection::CHEMLABELS)) {
            ChemLabelPath *chemPath;
            chemPath = new ChemLabelPath;
            chemPath->setSoPath(path);

            // Get the SoTextDetail which will tell us which label was picked.
            // Add this information to the ChemLabelPath.
            const SoTextDetail *textDetail =
                (const SoTextDetail *)pp->getDetail();
            if (textDetail != NULL) {
                int32_t whichLabel = textDetail->getStringIndex();
                if (whichLabel >= 0) {
                    chemPath->labelIndex.set1Value(0, SbVec2i(whichLabel, 1));
                }
            }
            return chemPath;
        }
    }
    else if (path->getTail()->isOfType(ChemMonitor::getClassTypeId())) {
        if (SELECT(what, ChemSelection::CHEMMONITORS)) {
            ChemMonitorPath *chemPath;
            chemPath = new ChemMonitorPath;
            chemPath->setSoPath(path);

            // Get the ChemMonitorDetail which will tell us which monitor was 
            // picked.  Add this information to the ChemMonitorPath.
            const ChemMonitorDetail *detail =
                (const ChemMonitorDetail *)pp->getDetail();
            if (detail != NULL) {
                int32_t distanceIndex, angleIndex, torsionalIndex;
                detail->getAllIndices(distanceIndex, angleIndex, torsionalIndex);
                if (distanceIndex != -1) {
                    chemPath->distanceIndex.set1Value(0, SbVec2i(distanceIndex, 1));
                }
                if (angleIndex != -1) {
                    chemPath->angleIndex.set1Value(0, SbVec2i(angleIndex, 1));
                }
                if (torsionalIndex != -1) {
                    chemPath->torsionalIndex.set1Value(0, SbVec2i(torsionalIndex, 1));
                }
            }
            return chemPath;
        }
    }
    return NULL;
}

#define SQRT_3_INV 0.57735027

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Get the coordinates of the selected atom.  This is the position at
//    which to start the radius selection.
//
// Use: private

void
ChemSelection::getSelectedAtomCoord(ChemDisplayPath *chemPath)
//
////////////////////////////////////////////////////////////////////////
{
    ChemBaseData *chemData;
    ChemDisplay *chemDisplay;

    // Set up the search action to look for ChemBaseData nodes.
    if (searchAction == NULL)
        searchAction = new SoSearchAction;
    else
        searchAction->reset();
    searchAction->setType(ChemBaseData::getClassTypeId());
    searchAction->setInterest(SoSearchAction::LAST);

    // Search the path for the ChemBaseData node.
    searchAction->apply(chemPath->path);
    SoFullPath *thePath = (SoFullPath *)searchAction->getPath();
    chemData = (ChemBaseData *)(thePath->getTail());

    // The selected atom is the first atom in the first atomIndex.
    // chemPath's tail node should be a ChemDisplay node.  We need
    // to get that node's modelMatrix so that we can transform the
    // selection sphere appropriately.
    int32_t atomStart, atomEnd;
    chemPath->atomIndex[0].getValue(atomStart, atomEnd);
    thePath = (SoFullPath *)(chemPath->path);
    assert(thePath->getTail()->isOfType(ChemDisplay::getClassTypeId()));
    chemDisplay = (ChemDisplay *)(thePath->getTail());
    assert(chemDisplay != NULL);

    selectedAtomCoord = chemData->getAtomCoordinates(atomStart);

    SbVec3f theTranslation, theScale;
    SbRotation theRotation, theScaleOrientation;
 
    currentModelMatrixInverse = currentModelMatrix.inverse();
    currentModelMatrixInverse.getTransform(theTranslation, theRotation,
      theScale, theScaleOrientation);

    sphereRadiusScaleFactor = theScale.length() * SQRT_3_INV;

    SbMatrix tmpMatrix = chemDisplay->getCurrentModelMatrix();
    tmpMatrix.multVecMatrix(selectedAtomCoord, selectedAtomCoordX);
    currentModelMatrixInverse.multVecMatrix(selectedAtomCoordX, sphereCoord);

    currentRotation = calculateRotation(currentModelMatrix, currentViewVolume,
        currentViewMatrix);
}

#undef SQRT_3_INV

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Initialize the starting values for the radiusSelection mechanism
//    and get the coordinate of the selected atom.
//
// Use: private

void
ChemSelection::startRadiusSelection(ChemDisplayPath *mouseDownPickPath)
//
////////////////////////////////////////////////////////////////////////
{
    useRadiusSelection = TRUE;
    getSelectedAtomCoord(mouseDownPickPath);
    radiusSelectionFirstPoint = TRUE;
    selectedRadius = 0.0;
    selectedRadiusSquared = 0.0;
    sphereBBox.makeEmpty();
}

void
ChemSelection::stopRadiusSelection()
{
    useRadiusSelection = FALSE;
    sceneSwitch->whichChild.setValue(SO_SWITCH_NONE);
}

void
ChemSelection::selectSensorCB(void *data, SoSensor *sensor)
{
    ChemSelection *cs = (ChemSelection *)data;
    SoField *theField = ((SoFieldSensor *)sensor)->getAttachedField();

    if (theField == &cs->lassoLineWidth) {
        cs->lassoDrawStyle->lineWidth = cs->lassoLineWidth.getValue();
    }

    else if (theField == &cs->lassoLinePattern) {
        cs->lassoDrawStyle->linePattern = cs->lassoLinePattern.getValue();
    }

    else if (theField == &cs->lassoLineColorIndex) {
        cs->lassoColorIndex->index = cs->lassoLineColorIndex.getValue();
    }

    else if (theField == &cs->radiusSelectSphereColor) {
        cs->sphereMaterial->diffuseColor =
            cs->radiusSelectSphereColor.getValue();
    }

    else if (theField == &cs->radiusSelectSphereTransparency) {
        cs->sphereMaterial->transparency =
            cs->radiusSelectSphereTransparency.getValue();
    }

    else if (theField == &cs->radiusSelectSphereComplexity) {
        cs->sphereComplexity->value =
            cs->radiusSelectSphereComplexity.getValue();
    }

    else if (theField == &cs->radiusSelectSphereDrawStyle) {
        cs->sphereDrawStyle->style = cs->radiusSelectSphereDrawStyle.getValue();
    }

    else if (theField == &cs->radiusSelectLabelFontName) {
        cs->labelFont->name = cs->radiusSelectLabelFontName.getValue();
    }

    else if (theField == &cs->radiusSelectLabelFontSize) {
        cs->labelFont->size = cs->radiusSelectLabelFontSize.getValue();
    }

    else if (theField == &cs->radiusSelectLabelColor) {
        cs->labelMaterial->diffuseColor = cs->radiusSelectLabelColor.getValue();
    }
}

#undef SELECT
