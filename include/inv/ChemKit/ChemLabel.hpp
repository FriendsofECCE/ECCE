/*
 * ChemLabel.h
 *
 *	This file defines the ChemLabel node class.
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

#ifndef  __CHEM_LABEL_H__
#define  __CHEM_LABEL_H__

#include "inv/SbViewportRegion.H"

#include "inv/fields/SoMFColor.H"
#include "inv/fields/SoMFEnum.H"
#include "inv/fields/SoMFInt32.H"
#include "inv/fields/SoMFString.H"
#include "inv/fields/SoMFVec3f.H"

#include "inv/fields/SoSFBool.H"
#include "inv/fields/SoSFColor.H"
#include "inv/fields/SoSFName.H"
#include "inv/fields/SoSFFloat.H"

#include "inv/nodes/SoNonIndexedShape.H"

// Manuel Pastor (MPM)  2002/03/08
#include "inv/SoLists.H"
// Jamie Doornbos	2001/07/15
//#include "inv/lists/SoPickedPointList.H"

#include "inv/ChemKit/MFVec2i.H"
#include "inv/ChemKit/ChemkitBasic.H"

class ChemBitmapFontCache;
class ChemLabelBBox;
class ChemLabelSelectionElement;

//////////////////////////////////////////////////////////////////////////////
//
//  Class: ChemLabel
//
//  SoNode class that displays multiple pieces of 2D text
//
//////////////////////////////////////////////////////////////////////////////

class CHEMKIT_DLL_API ChemLabel : public SoNonIndexedShape {

    SO_NODE_HEADER(ChemLabel);

  public:

    // Fields:
    SoSFName   fontName;
    SoSFFloat  fontSize;

    SoMFColor  color;
    SoMFInt32  colorIndex;
    SoSFEnum   colorBinding;

    SoSFBool   doHighlighting;
    SoSFColor  highlightColor;

    SoMFEnum   leftRightJustification;
    SoMFInt32  leftRightJustificationIndex;
    SoSFEnum   leftRightJustificationBinding;

    SoMFEnum   topBottomJustification;
    SoMFInt32  topBottomJustificationIndex;
    SoSFEnum   topBottomJustificationBinding;

    SoMFVec3f  position;
    SoMFString text;

    // Enum for how to index into the various MF fields
    enum Binding {
        OVERALL,
        PER_LABEL,
        PER_LABEL_INDEXED
    };

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
    ChemLabel();

  SoEXTENDER public:
    virtual void    GLRender(SoGLRenderAction *action);
    virtual void    rayPick(SoRayPickAction *action);

  SoINTERNAL public:
    virtual void    notify(SoNotList *list);
    static void     initClass();
    void getPickedPoints(SoRayPickAction *action, SoNode *parentNode,
                         SoPickedPointList *ptList);
    void getChemLabelBBoxes(SoAction *action, SbBool clipOnCenter,
                            ChemLabelBBox *&chemLabelBBoxes);

    const SbMatrix   &getCurrentModelMatrix() const
        { return currentModelMatrix; }
    const SbMatrix   &getCurrentMVPMatrix() const
        { return currentMVP; }

  protected:
    // This method is a no-op for ChemLabel since there are no
    // graphics primitives it can generate
    virtual void    generatePrimitives(SoAction *action);
    virtual void    computeBBox(SoAction *action, SbBox3f &box,
                                SbVec3f &center);
    virtual ~ChemLabel();

  private:
    ChemBitmapFontCache *normalFont;

    MFVec2i normalLabelIndex;
    MFVec2i highlightLabelIndex;

    SbMatrix     currentMVP;
    SbMatrix     currentModelMatrix;
    SbViewVolume currentViewVolume;
    SbViewportRegion currentVPR;

    ChemLabelSelectionElement *lastSelectionElement;

    void resetIndices();
    void generateIndices(SoAction *action);
    void commonPick(SoRayPickAction *action, SbBool fillPickList,
                    SoNode *parentNode, SoPickedPointList *ptList);
    
};

#endif  /* !__CHEM_LABEL_H__ */
