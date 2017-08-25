/*
 * ChemIso.h
 *
 *	This file defines the ChemIso class.
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

#ifndef __CHEM_ISO_H__
#define __CHEM_ISO_H__

#include "inv/SbLinear.H"

#include "inv/fields/SoSFBool.H"
#include "inv/fields/SoSFEnum.H"
#include "inv/fields/SoSFFloat.H"
#include "inv/fields/SoSFNode.H"
#include "inv/fields/SoSFShort.H"
#include "inv/fields/SoMFUInt32.H"

#include "inv/nodes/SoShape.H"

#include "inv/ChemKit/ChemkitBasic.H"

class SoIndexedTriangleStripSet;
class SoChildList;

class CHEMKIT_DLL_API ChemIso : public SoShape {

    SO_NODE_HEADER(ChemIso);

  public:
    // Fields
    SoSFFloat threshold;       // value of the data to contour
    SoSFBool  generateNormals; // generate normals?
    SoSFBool  flipNormals;     // flip the sense of the normals?
    SoSFShort dataVar;         // which of the data variables in the data
                               //     lattice to use
    SoSFShort colorVar;        // which of the data variables in the color
                               //     lattice to use
    SoSFNode  data;            // the lattice to isosurface
    SoSFNode  color;           // the lattice used to color the isosurface
    SoSFFloat minValue;
    SoSFFloat maxValue;
    SoMFUInt32 orderedRGBA;

    // Constructor
    ChemIso();

    void regenerate(SbBool forceIt);

  SoEXTENDER public:
    virtual void doAction(SoAction *action);
    virtual void GLRender(SoGLRenderAction *action);
    virtual void callback(SoCallbackAction *action);
    virtual void getBoundingBox(SoGetBoundingBoxAction *action);
    virtual void getMatrix(SoGetMatrixAction *action);
    virtual void handleEvent(SoHandleEventAction *action);
    virtual void pick(SoPickAction *action);
    virtual void rayPick(SoRayPickAction *action);
    virtual void search(SoSearchAction *action);
    virtual void write(SoWriteAction *action);

  SoINTERNAL public:
    static void initClass();   // initialize the class
    virtual SoChildList *getChildren() const;

  protected:
    virtual void generatePrimitives(SoAction *action);
    virtual void computeBBox(SoAction *action, SbBox3f &box,
                             SbVec3f &center);

    // Destructor
    virtual ~ChemIso();

  private:
    SbBool  first;
    SbBool  lastGenerateNormals;
    SbBool  lastFlipNormals;
    float   lastThreshold;
    short   lastDataVar;  
    short   lastColorVar;

    SoChildList *children;
    SoIndexedTriangleStripSet *triStrip;

		friend class ChemSAS;

};

#endif /* !__CHEM_ISO_H__ */

