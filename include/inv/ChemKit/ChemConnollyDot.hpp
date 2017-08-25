/*
 * ChemConnollyDot.h
 *
 *  This file defines the ChemConnollyDot node class.
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

#ifndef __CHEM_CONNOLLYDOT_H__
#define __CHEM_CONNOLLYDOT_H__

#include "inv/fields/SoSFColor.H"
#include "inv/fields/SoSFFloat.H"
#include "inv/nodes/SoPointSet.H"

#include "inv/ChemKit/ChemkitBasic.H"

class ChemBaseDataElement;
class ChemColorElement;
class ChemRadiiElement;

typedef struct {
   int shape;
   int start_point;
   int num_points;
   int yon;
   int victim;
   float coord[3];
   float alt[3];
} probe_descriptor;

typedef struct {
   int atom1;
   int atom2;
   int atom3;
   int shape;
   float area;
   float coord[3];
   float norm[3];
} sp_descriptor;

class CHEMKIT_DLL_API ChemConnollyDot : public SoPointSet {

    SO_NODE_HEADER(ChemConnollyDot);

  public:

    // Fields:
    SoSFFloat probeRadius;
    SoSFFloat densityOfPoints;
    SoSFEnum  colorBinding;
    SoSFColor overallSurfaceColor;
    SoSFColor contactSurfaceColor;
    SoSFColor saddleSurfaceColor;
    SoSFColor concaveSurfaceColor;

    enum ColorBinding {
        OVERALL,
        BY_ATOM,
        BY_SURFACE,
        DEFAULT = OVERALL,
    };

    // Constructor
    ChemConnollyDot();

    float getContactSurfaceArea() const;
    float getReentrantSurfaceArea() const;

  SoEXTENDER public:
    virtual void    GLRender(SoGLRenderAction *action);
    virtual void    rayPick(SoRayPickAction *action);

  SoINTERNAL public:
    static void initClass();
    virtual void notify(SoNotList *list);

  protected:
    virtual void    generatePrimitives(SoAction *action);
    virtual void    computeBBox(SoAction *action, SbBox3f &box,
                                SbVec3f &center);
    virtual ~ChemConnollyDot();

  private:
    ChemBaseDataElement  *lastChemBaseDataElement;
    ChemColorElement     *lastChemColorElement;
    ChemRadiiElement     *lastChemRadiiElement;

    SbBool regenerateFlag;

    float  contactSurfaceArea;
    float  reentrantSurfaceArea;

    int32_t *index;
    int8_t  *surfaceRequest;
    float   *coords;
    float   *radii;
    sp_descriptor *theSurface;

    void regenerate(SoAction *action);
    int surface(int32_t natoms, float probe_radius, float density_of_points,
        int8_t *surface_request, float *atom_radii, float *atom_coords,
        int32_t *atom_index, int32_t *total_returned,
        sp_descriptor **the_surface);
};

#endif  /* !__CHEM_CONNOLLYDOT_H__ */
