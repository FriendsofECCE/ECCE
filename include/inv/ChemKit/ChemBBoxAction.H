/*
 * ChemBBoxAction.h
 *
 *     Defines the ChemBBoxAction class.
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
        
#ifndef  __CHEM_BBOX_ACTION_H__
#define  __CHEM_BBOX_ACTION_H__
    
#include "inv/SbBox.H"
#include "inv/actions/SoSubAction.H"

#include "inv/ChemKit/ChemkitBasic.H"

class ChemAtomBBox;
class ChemBondBBox;
class ChemLabelBBox;
class ChemMonitorBBox;

//////////////////////////////////////////////////////////////////////////////
//
//  Class: ChemBBoxAction
//
//  Gets the individual bounding boxes of the objects in a ChemDisplay node.
//
//////////////////////////////////////////////////////////////////////////////
    
class CHEMKIT_DLL_API ChemBBoxAction : public SoAction {
        
    SO_ACTION_HEADER(ChemBBoxAction);

  public:
    ChemBBoxAction();
    virtual ~ChemBBoxAction();

    // These are the items whose bounding box can be retrieved.
    enum ChemPart {
        ATOMS = 0x01,
        BONDS = 0x02,
        ATOMLABELS = 0x04,
        BONDLABELS = 0x08,
        CHEMLABELS = 0x10,
        CHEMMONITORS = 0x20,
// --> residue selection
				RESIDUES = 0x40,
				RESIDUELABELS = 0x80,
				ALL = 0xFF
				//ALL = 0x3F,
// <-- residue selection
    };

    // Set which objects in the ChemDisplay for which to retrieve 
    // bounding boxes.
    void setParts(int32_t what) { parts = what; }

    // Set whether to check whether or not the clipping of the
    // bounding box should be based on the center of the box or not
    void setAtomClipOnCenter(SbBool value)
        { atomClipOnCenter = value; }
    void setBondClipOnCenter(SbBool value)
        { bondClipOnCenter = value; }
    void setAtomLabelClipOnCenter(SbBool value)
        { atomLabelClipOnCenter = value; }
    void setBondLabelClipOnCenter(SbBool value)
        { bondLabelClipOnCenter = value; }
    void setChemLabelClipOnCenter(SbBool value)
        { chemLabelClipOnCenter = value; }
    void setChemMonitorClipOnCenter(SbBool value)
        { chemMonitorClipOnCenter = value; }

// --> residue selection
		void setResidueClipOnCenter(SbBool value)
        { residueClipOnCenter = value; }
		void setResidueLabelClipOnCenter(SbBool value)
        { residueLabelClipOnCenter = value; }
// <-- residue selection

    // Return which objects in the ChemDisplay for which bounding
    // boxes are to be retrieved.
    int32_t getParts() const { return parts; }

    // Return the bounding box information.
    void getAtomBBoxes(ChemAtomBBox *&_atomBBoxes) const;

    void getBondBBoxes(ChemBondBBox *&_bondBBoxes) const;

    void getAtomLabelBBoxes(ChemLabelBBox *&_atomLabelBBoxes) const;

    void getBondLabelBBoxes(ChemLabelBBox *&_bondLabelBBoxes) const;

    void getChemLabelBBoxes(ChemLabelBBox *&_chemLabelBBoxes) const;

    void getChemMonitorBBoxes(ChemMonitorBBox *&_chemMonitorBBoxes) const;

// --> residue selection
		void getResidueBBoxes(ChemBondBBox *&_residueBBoxes) const;
		void getResidueLabelBBoxes(ChemLabelBBox *&_chemLabelBBoxes) const;
// <-- residue selection

    // Set Parts and numBoxes to 0 and delete the arrays.
    void reset(SbBool doAll = TRUE);
    void nullEverything();

  SoINTERNAL public:
    static void initClass();

  protected:
    virtual void beginTraversal(SoNode *node);

  private:
    static void callDoAction(SoAction *, SoNode *);
    static void callBBoxAction(SoAction *, SoNode *);

    int32_t parts;  // Whose bounding box to retrieve

    int32_t  numSphereVertices;
    SbVec3f  *sphereVertices;

    SbBool atomClipOnCenter;
    SbBool bondClipOnCenter;
    SbBool atomLabelClipOnCenter;
    SbBool bondLabelClipOnCenter;
    SbBool chemLabelClipOnCenter;
    SbBool chemMonitorClipOnCenter;

    ChemAtomBBox *atomBBoxes;
    ChemBondBBox *bondBBoxes;
    ChemLabelBBox *atomLabelBBoxes;
    ChemLabelBBox *bondLabelBBoxes;
    ChemLabelBBox *chemLabelBBoxes;
    ChemMonitorBBox *chemMonitorBBoxes;

// --> residue selection
		SbBool residueClipOnCenter;
		SbBool residueLabelClipOnCenter;
		ChemBondBBox *residueBBoxes;
		ChemLabelBBox *residueLabelBBoxes;
// <-- residue selection
};

#endif /* !__CHEM_BBOX_ACTION_H__ */
