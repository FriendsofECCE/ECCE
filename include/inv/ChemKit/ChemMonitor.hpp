/*
 * ChemMonitor.h
 *
 *	This file defines the ChemMonitor node class.
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

#ifndef  __CHEM_MONITOR_H__
#define  __CHEM_MONITOR_H__

#include "inv/fields/SoSFBool.H"
#include "inv/fields/SoSFColor.H"
#include "inv/fields/SoSFEnum.H"
#include "inv/fields/SoSFFloat.H"
#include "inv/fields/SoSFName.H"
#include "inv/fields/SoSFString.H"
#include "inv/fields/SoSFUShort.H"
#include "inv/fields/SoMFBool.H"

#include "inv/misc/SoChildList.H"

#include "inv/nodes/SoNonIndexedShape.H"

#include "inv/ChemKit/MFVec2i.H"
#include "inv/ChemKit/MFVec2AtomSpec.H"
#include "inv/ChemKit/MFVec3AtomSpec.H"
#include "inv/ChemKit/MFVec4AtomSpec.H"
#include "inv/ChemKit/ChemkitBasic.H"

class ChemLabel;
class ChemMonitorBBox;
class ChemMonitorSelectionElement;

//////////////////////////////////////////////////////////////////////////////
//
//  Class: ChemMonitor
//
//  SoNode class that displays distances, angles and torsionals
//
//////////////////////////////////////////////////////////////////////////////

class CHEMKIT_DLL_API ChemMonitor : public SoNonIndexedShape {

    SO_NODE_HEADER(ChemMonitor);

  public:

    // Fields:
    SoSFEnum distanceLabelLeftRightJustification;
    SoSFEnum distanceLabelTopBottomJustification;
    SoSFEnum angleLabelLeftRightJustification;
    SoSFEnum angleLabelTopBottomJustification;
    SoSFEnum torsionalLabelLeftRightJustification;
    SoSFEnum torsionalLabelTopBottomJustification;

    SoSFString distanceLabelFormat;
    SoSFName   distanceLabelFontName;
    SoSFFloat  distanceLabelFontSize;
    SoSFColor  distanceLabelColor;

    SoSFString angleLabelFormat;
    SoSFName   angleLabelFontName;
    SoSFFloat  angleLabelFontSize;
    SoSFColor  angleLabelColor;

    SoSFString torsionalLabelFormat;
    SoSFName   torsionalLabelFontName;
    SoSFFloat  torsionalLabelFontSize;
    SoSFColor  torsionalLabelColor;

    SoSFColor distanceGeomColor;
    SoSFColor angleGeomColor;
    SoSFColor torsionalGeomColor;

    SoSFBool  doHighlighting;
    SoSFColor highlightColor;

    SoSFInt32  distanceStippleFactor;
    SoSFUShort distanceStipplePattern;
    SoSFInt32  angleStippleFactor;
    SoSFUShort angleStipplePattern;
    SoSFInt32  torsionalStippleFactor;
    SoSFUShort torsionalStipplePattern;

    SoMFBool drawDistanceLine;
    SoMFBool drawAngleLines;
    SoMFBool drawTorsionalLines;

    SoSFFloat angleArcScale;
    SoSFFloat torsionalArcScale;

    MFVec2AtomSpec distance;
    MFVec3AtomSpec angle;
    MFVec4AtomSpec torsional;

    // Enum for where text is to be placed
    enum LeftRightJustification {
        LR_LEFT,
        LR_RIGHT,
        LR_CENTER,
        LR_DEFAULT = LR_LEFT
    };
    enum TopBottomJustification {
        TB_TOP,
        TB_BOTTOM,
        TB_MIDDLE,
        TB_DEFAULT = TB_BOTTOM
    };

    // Constructor
    ChemMonitor();

  SoEXTENDER public:
    virtual void    GLRender(SoGLRenderAction *action);
    virtual void    rayPick(SoRayPickAction *action);

  SoINTERNAL public:
    virtual void    notify(SoNotList *list);
    static void     initClass();
    void getChemMonitorBBoxes(SoAction *action, SbBool clipOnCenter,
                              ChemMonitorBBox *&chemMonitorBBoxes);

    const SbMatrix   &getCurrentModelMatrix() const
        { return currentModelMatrix; }
    const SbMatrix   &getCurrentMVPMatrix() const
        { return currentMVP; }

  protected:
    virtual void    generatePrimitives(SoAction *action);
    virtual void    computeBBox(SoAction *action, SbBox3f &box,
                                SbVec3f &center);
    virtual ~ChemMonitor();

  private:
    SoChildList *children;
    ChemLabel   *distanceLabels;
    ChemLabel   *angleLabels;
    ChemLabel   *torsionalLabels;

    MFVec2i normalDistanceIndex;
    MFVec2i normalAngleIndex;
    MFVec2i normalTorsionalIndex;

    MFVec2i highlightDistanceIndex;
    MFVec2i highlightAngleIndex;
    MFVec2i highlightTorsionalIndex;

    SbMatrix    currentMVP;
    SbMatrix    currentModelMatrix;
    SbMatrix    currentModelMatrixInverse;

    ChemMonitorSelectionElement *lastSelectionElement;
    
    void renderDistance(SoGLRenderAction *action);
    void renderAngle(SoGLRenderAction *action);
    void renderTorsional(SoGLRenderAction *action);

    void computeDistanceBBox(SoAction *action, SbBox3f &box,
                             SbVec3f &center);
    void computeAngleBBox(SoAction *action, SbBox3f &box,
                          SbVec3f &center);
    void computeTorsionalBBox(SoAction *action, SbBox3f &box,
                              SbVec3f &center);

    void pickDistanceGeoms(SoRayPickAction *action, SbVec3f &norm);
    void pickAngleGeoms(SoRayPickAction *action, SbVec3f &norm);
    void pickTorsionalGeoms(SoRayPickAction *action, SbVec3f &norm);

    void doDistanceRender(const MFVec2i &index, const SbColor &labelColor);
    void doAngleRender(const MFVec2i &index, const SbColor &labelColor);
    void doTorsionalRender(const MFVec2i &index, const SbColor &labelColor);

    void generateIndices(SoAction *action);
    void resetIndices();
    void resetLabels();
};

#endif  /* !__CHEM_MONITOR_H__ */
