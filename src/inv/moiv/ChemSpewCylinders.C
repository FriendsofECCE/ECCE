/*
 * ChemSpewCylinders.c++
 *
 *     Generates primitives for Open Inventor for the cylinders in
 *     ChemDisplay
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

#include <math.h>

#include "inv/SoPrimitiveVertex.H"
#include "inv/actions/SoAction.H"
#include "inv/elements/SoGLTextureCoordinateElement.H"
#include "inv/elements/SoTextureCoordinateElement.H"

// --> lod
#include "inv/ChemKit/ChemLOD.H"
// <-- lod
#include "inv/ChemKit/ChemDisplay.H"
#include "inv/ChemKit/ChemUnitCylinder.H"

// Returns S or T texture coord for point on top or bottom of
// cylinder, given x or z coord
#define BOT_TEX_S(x)    ((x) * .5 + .5)
#define BOT_TEX_T(z)    ((z) * .5 + .5)
#define TOP_TEX_S(x)    BOT_TEX_S(x)
#define TOP_TEX_T(z)    (1.0 - BOT_TEX_T(z))

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Generates primitives for Open Inventor
//

void
ChemDisplay::spewCylinder(SoPrimitiveVertex &pv,
    SoAction *action,
    const SoTextureCoordinateElement *tce,
    const SbVec3f &from, const SbVec3f &to,
    const float radius, const SbBool genTexCoords,
    const ChemUnitCylinder *unitCylinder)
//
////////////////////////////////////////////////////////////////////////
{
    SbMatrix theVertexMatrix;
    SbVec3f theTranslation;
    SbRotation theRotation;
    SbVec3f theScale;
    SbVec3f yAxis(0.0, 1.0, 0.0);

    int capType = unitCylinder->getCapType();
    const SbVec3f *cylNormals = unitCylinder->getCylinderNormals();
    const SbVec3f *cylVertices = unitCylinder->getCylinderVertices();
    int32_t numSections = unitCylinder->getNumberOfSections();
    int32_t numSides = unitCylinder->getNumberOfSides();

    // Set up the matrices
    SbVec3f tcoord = to - from;
    float height = tcoord.length() * 0.5f;
    theTranslation = tcoord * 0.5f + from;
    theRotation.setValue(yAxis, tcoord);
    theScale.setValue(radius, height, radius);
    theVertexMatrix.setTransform(theTranslation, theRotation, theScale);

    // Spit out the cylinder and flat caps if necessary
    spewCylinderRegular(pv, action, tce,
        theVertexMatrix, FALSE, genTexCoords,
        numSections, numSides, cylNormals, cylVertices);

    // Spit out the round caps if necessary
    if (capType == ChemUnitCylinder::ROUNDCAP) {
        int32_t numRoundCapSections =
            unitCylinder->getNumberOfRoundCapSections();
        int32_t cylBottomStartNorm =
            unitCylinder->getStartOfBottomNormals();
        int32_t cylBottomStartVert =
            unitCylinder->getStartOfBottomVertices();
        const SbVec3f *roundCapNormals =
            unitCylinder->getRoundCapNormals();
        const SbVec3f *roundCapVertices =
            unitCylinder->getRoundCapVertices();

        theScale.setValue(radius, radius, radius);
        theVertexMatrix.setTransform(from, theRotation, theScale);
        spewCylinderRoundCap(pv, action, tce,
            theVertexMatrix, TRUE, genTexCoords,
            numRoundCapSections, numSides,
            cylBottomStartNorm, cylBottomStartVert,
            roundCapNormals, roundCapVertices);

        theVertexMatrix.setTransform(to, theRotation, theScale);
        spewCylinderRoundCap(pv, action, tce,
            theVertexMatrix, FALSE, genTexCoords,
            numRoundCapSections, numSides,
            cylBottomStartNorm, cylBottomStartVert,
            roundCapNormals, roundCapVertices);
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Generate primitives for Open Inventor for the cylinder and 
//    flat caps if any
//
// Use: static   

void
ChemDisplay::spewCylinderRegular(SoPrimitiveVertex &pv,
    SoAction *action,
    const SoTextureCoordinateElement *tce,
    const SbMatrix &theVertexMatrix, 
    const SbBool doCaps, const SbBool genTexCoords,
    const int32_t numSections, const int32_t numSides, 
    const SbVec3f *cylNormals, const SbVec3f *cylVertices)
//
////////////////////////////////////////////////////////////////////////
{
    int32_t normCount, vertCount;
    int32_t section, side;
    float   s, ds, dt;
    float   tTop, tBot;
    SbVec3f norm, src, dst;
    SbVec4f tex;

    SbMatrix theNormalMatrix = theVertexMatrix.inverse().transpose();

    normCount = 0;
    vertCount = 0;
    tTop = 1.0;
    dt = -1.0 / numSections;
    ds = -1.0 / numSides;
    tex[2] = 0.0;
    tex[3] = 1.0;

    for (section = 0; section < numSections; section++) {
        s = 1.0;
        tBot = tTop + dt;
        beginShape(action, TRIANGLE_STRIP);
        for (side = 0; side < numSides; side++) {
            src = cylNormals[normCount++];
            theNormalMatrix.multDirMatrix(src, norm);
            norm.normalize();
            pv.setNormal(norm);
            src = cylVertices[vertCount++];
            theVertexMatrix.multVecMatrix(src, dst);
            pv.setPoint(dst);
            if (genTexCoords) {
                tex[0] = s;
                tex[1] = tBot;
            }
            else {
                tex = tce->get(dst, norm);
            }
            pv.setTextureCoords(tex);
            shapeVertex(&pv);

            src = cylVertices[vertCount++];
            theVertexMatrix.multVecMatrix(src, dst);
            pv.setPoint(dst);
            if (genTexCoords) {
                tex[0] = s;
                tex[1] = tTop;
            }
            else {
                tex = tce->get(dst, norm);
            }
            pv.setTextureCoords(tex);
            shapeVertex(&pv);
            s += ds;
        }
        s = 0.0;
        src = cylNormals[normCount++];
        theNormalMatrix.multDirMatrix(src, norm);
        norm.normalize();   
        pv.setNormal(norm);   
        src = cylVertices[vertCount++];
        theVertexMatrix.multVecMatrix(src, dst);
        pv.setPoint(dst);
        if (genTexCoords) {
            tex[0] = s;
            tex[1] = tBot;
        }
        else {
            tex = tce->get(dst, norm);
        }
        pv.setTextureCoords(tex);
        shapeVertex(&pv);
            
        src = cylVertices[vertCount++];
        theVertexMatrix.multVecMatrix(src, dst);
        pv.setPoint(dst);
        if (genTexCoords) {
            tex[0] = s;
            tex[1] = tTop;
        }
        else {
            tex = tce->get(dst, norm);
        }
        pv.setTextureCoords(tex);
        shapeVertex(&pv);
            
        endShape();
        tTop = tBot;
    }
    if (doCaps) {
        // TOP
        src = cylNormals[normCount++];
        theNormalMatrix.multDirMatrix(src, norm);
        norm.normalize();
        pv.setNormal(norm);
        for (section = numSections - 1; section > 0;
             --section) {
            beginShape(action, TRIANGLE_STRIP);
            for (side = 0; side < numSides; side++) {
                src = cylVertices[vertCount++];
                theVertexMatrix.multVecMatrix(src, dst);
                pv.setPoint(dst);
                if (genTexCoords) {
                    tex[0] = TOP_TEX_S(src[0]);
                    tex[1] = TOP_TEX_T(src[2]); 
                }
                else {
                    tex = tce->get(dst, norm);
                }
                pv.setTextureCoords(tex);
                shapeVertex(&pv);
            
                src = cylVertices[vertCount++];
                theVertexMatrix.multVecMatrix(src, dst);
                pv.setPoint(dst);
                if (genTexCoords) {
                    tex[0] = TOP_TEX_S(src[0]);
                    tex[1] = TOP_TEX_T(src[2]); 
                }
                else {
                    tex = tce->get(dst, norm);
                }
                pv.setTextureCoords(tex);
                shapeVertex(&pv);
            }
            src = cylVertices[vertCount++];
            theVertexMatrix.multVecMatrix(src, dst);
            pv.setPoint(dst);
            if (genTexCoords) {
                tex[0] = TOP_TEX_S(src[0]);
                tex[1] = TOP_TEX_T(src[2]);
            }
            else {
                tex = tce->get(dst, norm);
            } 
            pv.setTextureCoords(tex);
            shapeVertex(&pv);
        
            src = cylVertices[vertCount++];
            theVertexMatrix.multVecMatrix(src, dst);
            pv.setPoint(dst);
            if (genTexCoords) {
                tex[0] = TOP_TEX_S(src[0]);
                tex[1] = TOP_TEX_T(src[2]);
            }
            else {
                tex = tce->get(dst, norm);
            }
            pv.setTextureCoords(tex);
            shapeVertex(&pv);
                
            endShape();
        }
        beginShape(action, TRIANGLE_FAN);
        src = cylVertices[vertCount++];
        theVertexMatrix.multVecMatrix(src, dst);
        pv.setPoint(dst);
        if (genTexCoords) {
            tex[0] = tex[1] = 0.5;
        }
        else {
            tex = tce->get(dst, norm);    
        }
        pv.setTextureCoords(tex);
        shapeVertex(&pv);
                 
        for (side = numSides - 1; side >= 0; side--) {
            src = cylVertices[vertCount++];
            theVertexMatrix.multVecMatrix(src, dst);
            pv.setPoint(dst);
            if (genTexCoords) {
                tex[0] = TOP_TEX_S(src[0]);
                tex[1] = TOP_TEX_T(src[2]);
            }
            else {
                tex = tce->get(dst, norm);
            }
            pv.setTextureCoords(tex);
            shapeVertex(&pv);
        }
        src = cylVertices[vertCount++];
        theVertexMatrix.multVecMatrix(src, dst);
        pv.setPoint(dst);
        if (genTexCoords) {
            tex[0] = TOP_TEX_S(src[0]);
            tex[1] = TOP_TEX_T(src[2]);
        }
        else {
            tex = tce->get(dst, norm);
        }
        pv.setTextureCoords(tex);
        shapeVertex(&pv);
        endShape();
            
        // BOTTOM
        src = cylNormals[normCount++];
        theNormalMatrix.multDirMatrix(src, norm);
        norm.normalize();
        pv.setNormal(norm);
        for (section = numSections - 1; section > 0;
             --section) {
            beginShape(action, TRIANGLE_STRIP);
            for (side = numSides - 1; side >= 0; side--) {
                src = cylVertices[vertCount++];
                theVertexMatrix.multVecMatrix(src, dst);
                pv.setPoint(dst);
                if (genTexCoords) {
                    tex[0] = BOT_TEX_S(src[0]);
                    tex[1] = BOT_TEX_T(src[2]);
                }
                else {
                    tex = tce->get(dst, norm);  
                }
                pv.setTextureCoords(tex);
                shapeVertex(&pv);
                
                src = cylVertices[vertCount++];
                theVertexMatrix.multVecMatrix(src, dst);
                pv.setPoint(dst);
                if (genTexCoords) {
                    tex[0] = BOT_TEX_S(src[0]);
                    tex[1] = BOT_TEX_T(src[2]);
                }
                else {
                    tex = tce->get(dst, norm);
                }
                pv.setTextureCoords(tex);
                shapeVertex(&pv);
            }
            src = cylVertices[vertCount++];
            theVertexMatrix.multVecMatrix(src, dst);
            pv.setPoint(dst);
            if (genTexCoords) {
                tex[0] = BOT_TEX_S(src[0]);
                tex[1] = BOT_TEX_T(src[2]);
            }
            else { 
                tex = tce->get(dst, norm);
            }
            pv.setTextureCoords(tex);
            shapeVertex(&pv);
        
            src = cylVertices[vertCount++];
            theVertexMatrix.multVecMatrix(src, dst);
            pv.setPoint(dst);
            if (genTexCoords) {
                tex[0] = BOT_TEX_S(src[0]);
                tex[1] = BOT_TEX_T(src[2]);
            }
            else {
                tex = tce->get(dst, norm);
            }
            pv.setTextureCoords(tex);
            shapeVertex(&pv);
                
            endShape();
        }
        beginShape(action, TRIANGLE_FAN);
        src = cylVertices[vertCount++];
        theVertexMatrix.multVecMatrix(src, dst);
        pv.setPoint(dst);
        if (genTexCoords) {
            tex[0] = tex[1] = 0.5;
        }
        else {
            tex = tce->get(dst, norm);   
        }
        pv.setTextureCoords(tex);
        shapeVertex(&pv);
                 
        for (side = 0; side < numSides; side++) {
            src = cylVertices[vertCount++];
            theVertexMatrix.multVecMatrix(src, dst);
            pv.setPoint(dst);
            if (genTexCoords) {
                tex[0] = BOT_TEX_S(src[0]);
                tex[1] = BOT_TEX_T(src[2]);
            }
            else {
                tex = tce->get(dst, norm);
            }
            pv.setTextureCoords(tex);
            shapeVertex(&pv);
        }
        src = cylVertices[vertCount++];
        theVertexMatrix.multVecMatrix(src, dst);
        pv.setPoint(dst);
        if (genTexCoords) {
            tex[0] = BOT_TEX_S(src[0]);
            tex[1] = BOT_TEX_T(src[2]);
        }
        else {
            tex = tce->get(dst, norm);
        }
        pv.setTextureCoords(tex);
        shapeVertex(&pv);
        endShape();
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Generates primitives for Open Inventor for rounded cylinder caps
//

void
ChemDisplay::spewCylinderRoundCap(SoPrimitiveVertex &pv,
    SoAction *action,
    const SoTextureCoordinateElement *tce,
    const SbMatrix &theVertexMatrix,
    const SbBool bottom, const SbBool genTexCoords,
    const int numRoundCapSections, const int numSides,
    const int cylinderBottomStartNorm, const int cylinderBottomStartVert,
    const SbVec3f *cylinderCapNormals,
    const SbVec3f *cylinderCapVertices)
//
////////////////////////////////////////////////////////////////////////
{
    int32_t capNormCount, capVertCount;
    int32_t section, side;
    SbVec3f norm, src, dst;
    SbVec4f tex;

    SbMatrix theNormalMatrix = theVertexMatrix.inverse().transpose();

    if (bottom) {
        capNormCount = cylinderBottomStartNorm;
        capVertCount = cylinderBottomStartVert;
    }
    else {
        capNormCount = capVertCount = 0;
    }

    tex[2] = 0.0;
    tex[3] = 1.0;

    for (section = numRoundCapSections - 1; section > 0; --section) {
        beginShape(action, TRIANGLE_STRIP);
        for (side = 0; side < numSides; side++) {
            src = cylinderCapNormals[capNormCount++];
            theNormalMatrix.multDirMatrix(src, norm);
            norm.normalize();
            pv.setNormal(norm);
            src = cylinderCapVertices[capVertCount++];
            theVertexMatrix.multVecMatrix(src, dst);
            pv.setPoint(dst);
            if (genTexCoords) {
                if (bottom) {
                    tex[0] = BOT_TEX_S(src[0]);
                    tex[1] = BOT_TEX_T(src[2]);
                }
                else {
                    tex[0] = TOP_TEX_S(src[0]);
                    tex[1] = TOP_TEX_T(src[2]);
                }
            }
            else {
                tex = tce->get(dst, norm);
            }
            pv.setTextureCoords(tex);
            shapeVertex(&pv);
    
            src = cylinderCapNormals[capNormCount++];
            theNormalMatrix.multDirMatrix(src, norm);
            norm.normalize();
            pv.setNormal(norm);
            src = cylinderCapVertices[capVertCount++];
            theVertexMatrix.multVecMatrix(src, dst);
            pv.setPoint(dst);
            if (genTexCoords) {
                if (bottom) {
                    tex[0] = BOT_TEX_S(src[0]);
                    tex[1] = BOT_TEX_T(src[2]);
                }
                else {
                    tex[0] = TOP_TEX_S(src[0]);
                    tex[1] = TOP_TEX_T(src[2]);
                }
            }
            else {
                tex = tce->get(dst, norm);
            }
            pv.setTextureCoords(tex);
            shapeVertex(&pv);
        }
        src = cylinderCapNormals[capNormCount++];
        theNormalMatrix.multDirMatrix(src, norm);
        norm.normalize();
        pv.setNormal(norm);
        src = cylinderCapVertices[capVertCount++];
        theVertexMatrix.multVecMatrix(src, dst);
        pv.setPoint(dst);
        if (genTexCoords) {
            if (bottom) {
                tex[0] = BOT_TEX_S(src[0]);
                tex[1] = BOT_TEX_T(src[2]);
            }
            else {
                tex[0] = TOP_TEX_S(src[0]);
                tex[1] = TOP_TEX_T(src[2]);
            }
        }
        else {
            tex = tce->get(dst, norm);
        }
        pv.setTextureCoords(tex);
        shapeVertex(&pv);
    
        src = cylinderCapNormals[capNormCount++];
        theNormalMatrix.multDirMatrix(src, norm);
        norm.normalize();
        pv.setNormal(norm);
        src = cylinderCapVertices[capVertCount++];
        theVertexMatrix.multVecMatrix(src, dst);
        pv.setPoint(dst);
        if (genTexCoords) {
            if (bottom) {
                tex[0] = BOT_TEX_S(src[0]);
                tex[1] = BOT_TEX_T(src[2]);
            }
            else {
                tex[0] = TOP_TEX_S(src[0]);
                tex[1] = TOP_TEX_T(src[2]);
            }
        }
        else {
            tex = tce->get(dst, norm);
        }
        pv.setTextureCoords(tex);
        shapeVertex(&pv);
    
        endShape();
    }
    beginShape(action, TRIANGLE_FAN);
    src = cylinderCapNormals[capNormCount++];
    theNormalMatrix.multDirMatrix(src, norm);
    norm.normalize();
    pv.setNormal(norm);
    src = cylinderCapVertices[capVertCount++];
    theVertexMatrix.multVecMatrix(src, dst);
    pv.setPoint(dst);
    if (genTexCoords) {
        tex[0] = tex[1] = 0.5;
    }
    else {
        tex = tce->get(dst, norm);
    }
    pv.setTextureCoords(tex);
    shapeVertex(&pv);
    
    for (side = numSides - 1; side >= 0; side--) {
        src = cylinderCapNormals[capNormCount++];
        theNormalMatrix.multDirMatrix(src, norm);
        norm.normalize();
        pv.setNormal(norm);
        src = cylinderCapVertices[capVertCount++];
        theVertexMatrix.multVecMatrix(src, dst);
        pv.setPoint(dst);
        if (genTexCoords) {
            if (bottom) {
                tex[0] = BOT_TEX_S(src[0]);
                tex[1] = BOT_TEX_T(src[2]);
            }
            else {
                tex[0] = TOP_TEX_S(src[0]);
                tex[1] = TOP_TEX_T(src[2]);
            }
        }
        else {
            tex = tce->get(dst, norm);
        }
        pv.setTextureCoords(tex);
        shapeVertex(&pv);
    }
    src = cylinderCapNormals[capNormCount++];
    theNormalMatrix.multDirMatrix(src, norm);
    norm.normalize();
    pv.setNormal(norm);
    src = cylinderCapVertices[capVertCount++];
    theVertexMatrix.multVecMatrix(src, dst);
    pv.setPoint(dst);
    if (genTexCoords) {
        if (bottom) {
            tex[0] = BOT_TEX_S(src[0]);
            tex[1] = BOT_TEX_T(src[2]);
        }
        else {
            tex[0] = TOP_TEX_S(src[0]);
            tex[1] = TOP_TEX_T(src[2]);
        }
    }
    else {
        tex = tce->get(dst, norm);
    }
    pv.setTextureCoords(tex);
    shapeVertex(&pv);
    endShape();
}


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Generates primitives for Open Inventor for a cylinder with colors
//    half-bonded between the ends.
//

void
ChemDisplay::spewCylinderHalfBonded(SoPrimitiveVertex &pv,
    SoAction *action,
    const SoTextureCoordinateElement *tce,
    const SbVec3f &from, const SbVec3f &to,
    const float radius,
    const int32_t indexFrom, const int32_t indexTo,
    const SbBool genTexCoords,
    const ChemUnitCylinder *unitCylinder)
//
////////////////////////////////////////////////////////////////////////
{
    SbMatrix theVertexMatrix;
    SbVec3f theTranslation;
    SbRotation theRotation;
    SbVec3f theScale;
    SbVec3f yAxis(0.0, 1.0, 0.0);
    SbVec3f tcoord;
    float height;

    int capType = unitCylinder->getCapType();
    const SbVec3f *cylNormals = unitCylinder->getCylinderNormals();
    const SbVec3f *cylVertices = unitCylinder->getCylinderVertices();
    int32_t numSections = unitCylinder->getNumberOfSections();
    int32_t numSides = unitCylinder->getNumberOfSides();

    if (indexFrom == indexTo) {
        tcoord = to - from;
        height = tcoord.length() * 0.5f;
        theTranslation = tcoord * 0.5f + from;
        theRotation.setValue(yAxis, tcoord);
        theScale.setValue(radius, height, radius);
        theVertexMatrix.setTransform(theTranslation, theRotation, theScale);

        pv.setMaterialIndex(indexFrom);
        spewCylinderRegular(pv, action, tce,
            theVertexMatrix, FALSE, genTexCoords,
            numSections, numSides, cylNormals, cylVertices);

        if (capType == ChemUnitCylinder::ROUNDCAP) {
            int32_t numRoundCapSections =
                unitCylinder->getNumberOfRoundCapSections();
            int32_t cylBottomStartNorm =
                unitCylinder->getStartOfBottomNormals();
            int32_t cylBottomStartVert =
                unitCylinder->getStartOfBottomVertices();
            const SbVec3f *roundCapNormals =
                unitCylinder->getRoundCapNormals();
            const SbVec3f *roundCapVertices =
                unitCylinder->getRoundCapVertices();

            theScale.setValue(radius, radius, radius);
            theVertexMatrix.setTransform(from, theRotation, theScale);

            spewCylinderRoundCap(pv, action, tce,
                theVertexMatrix, TRUE, genTexCoords,
                numRoundCapSections, numSides,
                cylBottomStartNorm, cylBottomStartVert,
                roundCapNormals, roundCapVertices);

            theVertexMatrix.setTransform(to, theRotation, theScale);
            spewCylinderRoundCap(pv, action, tce,
                theVertexMatrix, FALSE, genTexCoords,
                numRoundCapSections, numSides,
                cylBottomStartNorm, cylBottomStartVert,
                roundCapNormals, roundCapVertices);
        }
    }
    else {
        tcoord = (to - from) * 0.5f + from;
        height = (tcoord - from).length() * 0.5f;
        theTranslation = (tcoord - from) * 0.5f + from;
        theRotation.setValue(yAxis, (to - from));
        theScale.setValue(radius, height, radius);
        theVertexMatrix.setTransform(theTranslation, theRotation, theScale);

        pv.setMaterialIndex(indexFrom);
        spewCylinderRegular(pv, action, tce,
            theVertexMatrix, FALSE, genTexCoords,
            numSections, numSides, cylNormals, cylVertices);

        theTranslation = (to - tcoord) * 0.5f + tcoord;
        theVertexMatrix.setTransform(theTranslation, theRotation, theScale);

        pv.setMaterialIndex(indexTo);
        spewCylinderRegular(pv, action, tce,
            theVertexMatrix, FALSE, genTexCoords,
            numSections, numSides, cylNormals, cylVertices);

        if (capType == ChemUnitCylinder::ROUNDCAP) {
            int32_t numRoundCapSections =
                unitCylinder->getNumberOfRoundCapSections();
            int32_t cylBottomStartNorm =
                unitCylinder->getStartOfBottomNormals();
            int32_t cylBottomStartVert =
                unitCylinder->getStartOfBottomVertices();
            const SbVec3f *roundCapNormals =
                unitCylinder->getRoundCapNormals();
            const SbVec3f *roundCapVertices =
                unitCylinder->getRoundCapVertices();

            theScale.setValue(radius, radius, radius);
            theVertexMatrix.setTransform(from, theRotation, theScale);
            pv.setMaterialIndex(indexFrom);

            spewCylinderRoundCap(pv, action, tce,
                theVertexMatrix, TRUE, genTexCoords,
                numRoundCapSections, numSides,
                cylBottomStartNorm, cylBottomStartVert,
                roundCapNormals, roundCapVertices);

            theVertexMatrix.setTransform(to, theRotation, theScale);
            pv.setMaterialIndex(indexTo);

            spewCylinderRoundCap(pv, action, tce,
                theVertexMatrix, FALSE, genTexCoords,
                numRoundCapSections, numSides,
                cylBottomStartNorm, cylBottomStartVert,
                roundCapNormals, roundCapVertices);
        }
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Generates primitives for Open Inventor for a cylinder with colors
//    interpolated between the ends.
//

void
ChemDisplay::spewCylinderWithInterpolatedColor(SoPrimitiveVertex &pv,
    SoAction *action,
    const SoTextureCoordinateElement *tce,
    const SbVec3f &from, const SbVec3f &to,
    const float radius,
    const int32_t indexFrom, const int32_t indexTo,
    const SbBool genTexCoords,
    const ChemUnitCylinder *unitCylinder)
//
////////////////////////////////////////////////////////////////////////
{
    if (indexFrom == indexTo) {
        pv.setMaterialIndex(indexFrom);
        spewCylinder(pv, action, tce, from, to,
            radius, genTexCoords, unitCylinder);
        return;
    }

    SbMatrix theVertexMatrix;
    SbVec3f theTranslation;
    SbRotation theRotation;
    SbVec3f theScale;
    //SbVec3f theCapScale;
    SbVec3f yAxis(0.0, 1.0, 0.0);

    SbVec3f tcoord = to - from;
    float height = tcoord.length() * 0.5f;
    theTranslation = tcoord * 0.5f + from;
    theRotation.setValue(yAxis, tcoord);
    theScale.setValue(radius, height, radius);
    theVertexMatrix.setTransform(theTranslation, theRotation, theScale);
    SbMatrix theNormalMatrix = theVertexMatrix.inverse().transpose();

    SbBool doCaps = (unitCylinder->getCapType() == ChemUnitCylinder::FLATCAP);

    const SbVec3f *cylNormals = unitCylinder->getCylinderNormals();
    const SbVec3f *cylVertices = unitCylinder->getCylinderVertices();
    int32_t numSections = unitCylinder->getNumberOfSections();
    int32_t numSides = unitCylinder->getNumberOfSides();

    int32_t normCount, vertCount;
    int32_t section, side;
    float   s, ds, dt;
    float   tTop, tBot;
    SbVec3f norm, src, dst;
    SbVec4f tex;

    normCount = 0;
    vertCount = 0;
    tTop = 1.0;
    dt = -1.0 / numSections;
    ds = -1.0 / numSides;
    tex[2] = 0.0;
    tex[3] = 1.0;

    for (section = 0; section < numSections; section++) {
        s = 1.0;
        tBot = tTop + dt;
        beginShape(action, TRIANGLE_STRIP);
        for (side = 0; side < numSides; side++) {
            src = cylNormals[normCount++]; 
            theNormalMatrix.multDirMatrix(src, norm);
            norm.normalize();
            pv.setNormal(norm);
            pv.setMaterialIndex(indexFrom);

            src = cylVertices[vertCount++];
            theVertexMatrix.multVecMatrix(src, dst);
            pv.setPoint(dst);
            if (genTexCoords) {
                tex[0] = s;
                tex[1] = tBot;
            }
            else {
                tex = tce->get(dst, norm);
            }
            pv.setTextureCoords(tex);
            shapeVertex(&pv);
         
            pv.setMaterialIndex(indexTo);

            src = cylVertices[vertCount++];
            theVertexMatrix.multVecMatrix(src, dst);
            pv.setPoint(dst);
            if (genTexCoords) {
                tex[0] = s;
                tex[1] = tTop;
            }
            else {
                tex = tce->get(dst, norm);
            }
            pv.setTextureCoords(tex);
            shapeVertex(&pv);
            s += ds;
        }
        s = 0.0;
        src = cylNormals[normCount++];
        theNormalMatrix.multDirMatrix(src, norm);
        norm.normalize();
        pv.setNormal(norm);

        pv.setMaterialIndex(indexFrom);

        src = cylVertices[vertCount++];
        theVertexMatrix.multVecMatrix(src, dst);
        pv.setPoint(dst);
        if (genTexCoords) {
            tex[0] = s;
            tex[1] = tBot;
        }
        else {
            tex = tce->get(dst, norm);
        }
        pv.setTextureCoords(tex);
        shapeVertex(&pv);
         
        pv.setMaterialIndex(indexTo);

        src = cylVertices[vertCount++];
        theVertexMatrix.multVecMatrix(src, dst);
        pv.setPoint(dst);
        if (genTexCoords) {
            tex[0] = s;
            tex[1] = tTop;
        }
        else {
            tex = tce->get(dst, norm);
        }
        pv.setTextureCoords(tex);
        shapeVertex(&pv);
            
        endShape();
        tTop = tBot;
    }
        
    if (doCaps) {
        // TOP
        src = cylNormals[normCount++];
        theNormalMatrix.multDirMatrix(src, norm);
        norm.normalize();
        pv.setNormal(norm);

        pv.setMaterialIndex(indexTo);

        for (section = numSections - 1; section > 0; --section) { 
            beginShape(action, TRIANGLE_STRIP);
            for (side = 0; side < numSides; side++) {
                src = cylVertices[vertCount++];
                theVertexMatrix.multVecMatrix(src, dst);
                pv.setPoint(dst);
                if (genTexCoords) {
                    tex[0] = TOP_TEX_S(src[0]);
                    tex[1] = TOP_TEX_T(src[2]);
                }
                else {
                    tex = tce->get(dst, norm);
                }
                pv.setTextureCoords(tex);
                shapeVertex(&pv);
        
                src = cylVertices[vertCount++];
                theVertexMatrix.multVecMatrix(src, dst);
                pv.setPoint(dst);
                if (genTexCoords) {
                    tex[0] = TOP_TEX_S(src[0]);
                    tex[1] = TOP_TEX_T(src[2]);
                }
                else {
                    tex = tce->get(dst, norm);
                }
                pv.setTextureCoords(tex);
                shapeVertex(&pv);
            }
            src = cylVertices[vertCount++];
            theVertexMatrix.multVecMatrix(src, dst);
            pv.setPoint(dst);
            if (genTexCoords) {
                tex[0] = TOP_TEX_S(src[0]);
                tex[1] = TOP_TEX_T(src[2]);
            }
            else {
                tex = tce->get(dst, norm);
            }
            pv.setTextureCoords(tex);
            shapeVertex(&pv);
        
            src = cylVertices[vertCount++];
            theVertexMatrix.multVecMatrix(src, dst);
            pv.setPoint(dst);
            if (genTexCoords) {
                tex[0] = TOP_TEX_S(src[0]);
                tex[1] = TOP_TEX_T(src[2]);
            }
            else {
                tex = tce->get(dst, norm);
            }
            pv.setTextureCoords(tex);
            shapeVertex(&pv);
                
            endShape();
        }
        beginShape(action, TRIANGLE_FAN);
        src = cylVertices[vertCount++];
        theVertexMatrix.multVecMatrix(src, dst);
        pv.setPoint(dst);
        if (genTexCoords) {
            tex[0] = tex[1] = 0.5;
        }
        else {
            tex = tce->get(dst, norm);   
        }
        pv.setTextureCoords(tex);
        shapeVertex(&pv);
                 
        for (side = numSides - 1; side >= 0; side--) {
            src = cylVertices[vertCount++];
            theVertexMatrix.multVecMatrix(src, dst);
            pv.setPoint(dst);
            if (genTexCoords) {
                tex[0] = TOP_TEX_S(src[0]);
                tex[1] = TOP_TEX_T(src[2]);
            }
            else {
                tex = tce->get(dst, norm);
            }
            pv.setTextureCoords(tex);
            shapeVertex(&pv);
        }
        src = cylVertices[vertCount++];
        theVertexMatrix.multVecMatrix(src, dst);
        pv.setPoint(dst);
        if (genTexCoords) {
            tex[0] = TOP_TEX_S(src[0]);
            tex[1] = TOP_TEX_T(src[2]);
        }
        else {
            tex = tce->get(dst, norm);
        }
        pv.setTextureCoords(tex);
        shapeVertex(&pv);
        endShape();
            
        // BOTTOM
        src = cylNormals[normCount++];
        theNormalMatrix.multDirMatrix(src, norm);
        norm.normalize();
        pv.setNormal(norm);

        pv.setMaterialIndex(indexFrom);

        for (section = numSections - 1; section > 0; --section) {
            beginShape(action, TRIANGLE_STRIP);
            for (side = numSides - 1; side >= 0; side--) {
                src = cylVertices[vertCount++];
                theVertexMatrix.multVecMatrix(src, dst);
                pv.setPoint(dst);
                if (genTexCoords) {
                    tex[0] = BOT_TEX_S(src[0]);
                    tex[1] = BOT_TEX_T(src[2]);
                }
                else {
                    tex = tce->get(dst, norm);
                } 
                pv.setTextureCoords(tex); 
                shapeVertex(&pv);
            
                src = cylVertices[vertCount++];
                theVertexMatrix.multVecMatrix(src, dst);
                pv.setPoint(dst);
                if (genTexCoords) {
                    tex[0] = BOT_TEX_S(src[0]);
                    tex[1] = BOT_TEX_T(src[2]);
                }
                else {
                    tex = tce->get(dst, norm);
                }
                pv.setTextureCoords(tex);
                shapeVertex(&pv);
            }
            src = cylVertices[vertCount++];
            theVertexMatrix.multVecMatrix(src, dst);
            pv.setPoint(dst);
            if (genTexCoords) {
                tex[0] = BOT_TEX_S(src[0]);
                tex[1] = BOT_TEX_T(src[2]);
            }
            else {
                tex = tce->get(dst, norm);
            }
            pv.setTextureCoords(tex);
            shapeVertex(&pv);
        
            src = cylVertices[vertCount++];
            theVertexMatrix.multVecMatrix(src, dst);
            pv.setPoint(dst);
            if (genTexCoords) {
                tex[0] = BOT_TEX_S(src[0]);
                tex[1] = BOT_TEX_T(src[2]);
            }
            else {
                tex = tce->get(dst, norm);
            }
            pv.setTextureCoords(tex);
            shapeVertex(&pv);
                
            endShape();
        }
        beginShape(action, TRIANGLE_FAN); 
        src = cylVertices[vertCount++];
        theVertexMatrix.multVecMatrix(src, dst);
        pv.setPoint(dst);
        if (genTexCoords) {
            tex[0] = tex[1] = 0.5;
        }
        else {
            tex = tce->get(dst, norm);   
        }
        pv.setTextureCoords(tex);
        shapeVertex(&pv);
                 
        for (side = 0; side < numSides; side++) {
            src = cylVertices[vertCount++];
            theVertexMatrix.multVecMatrix(src, dst);
            pv.setPoint(dst);
            if (genTexCoords) {
                tex[0] = BOT_TEX_S(src[0]);
                tex[1] = BOT_TEX_T(src[2]);
            }
            else {
                tex = tce->get(dst, norm);
            }
            pv.setTextureCoords(tex);
            shapeVertex(&pv);
        }
        src = cylVertices[vertCount++];
        theVertexMatrix.multVecMatrix(src, dst);
        pv.setPoint(dst);
        if (genTexCoords) {
            tex[0] = BOT_TEX_S(src[0]);
            tex[1] = BOT_TEX_T(src[2]);
        }
        else {
            tex = tce->get(dst, norm);
        }
        pv.setTextureCoords(tex);
        shapeVertex(&pv);
        endShape();
    }
}

#undef BOT_TEX_S
#undef BOT_TEX_T
#undef TOP_TEX_S
#undef TOP_TEX_T
