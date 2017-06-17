/*
 * ChemAssociatedData.h
 *
 *      This file defines the ChemAssociatedData node class.
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

#ident "$Revision: 22148 $"

#ifndef  __CHEM_ASSOCIATED_DATA_H__
#define  __CHEM_ASSOCIATED_DATA_H__

#include "inv/fields/SoMFString.H"
#include "inv/nodes/SoSubNode.H"

#include "inv/ChemKit/ChemkitBasic.H"
#include "inv/ChemKit/ChemBaseData.H"
#include "inv/ChemKit/MFInt8.H"

//////////////////////////////////////////////////////////////////////////////
//
//  Class: ChemAssociatedData
//
//  This node contains non-Molecular Inventor data which is associated with
//  the chemical system
//
//////////////////////////////////////////////////////////////////////////////

class CHEMKIT_DLL_API ChemAssociatedData : public SoNode {

    SO_NODE_HEADER(ChemAssociatedData);

  public:

    // Fields
    SoMFString  description;
    SoMFString  asciiData;
    MFUInt8     binaryData;

    // Constructor
    ChemAssociatedData();

  SoEXTENDER public:

    // Implement actions
    virtual void    doAction(SoAction *action);
    virtual void    callback(SoCallbackAction *action);
    virtual void    GLRender(SoGLRenderAction *action);
    virtual void    pick(SoPickAction *action);
    virtual void    getBoundingBox(SoGetBoundingBoxAction *action);

  SoINTERNAL public:
    static void     initClass();

  protected:
    virtual ~ChemAssociatedData();
};

#endif /* !__CHEM_ASSOCIATED_DATA_H__ */
