/*
 * ChemBBoxAction.c++
 *
 *     Classes:  ChemBBoxAction
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

#include "inv/errors/SoDebugError.H"

#include "inv/ChemKit/ChemBBoxAction.H"
// --> lod
#include "inv/ChemKit/ChemLOD.H"
// <-- lod
#include "inv/ChemKit/ChemDisplay.H"
#include "inv/ChemKit/ChemLabel.H"
#include "inv/ChemKit/ChemMonitor.H"

#include "inv/ChemKit/ChemAtomBBox.H"
#include "inv/ChemKit/ChemBondBBox.H"
#include "inv/ChemKit/ChemLabelBBox.H"
#include "inv/ChemKit/ChemMonitorBBox.H"
#include "inv/ChemKit/ChemStickBondBBox.H"
#include "inv/ChemKit/ChemWireframeBondBBox.H"


SO_ACTION_SOURCE(ChemBBoxAction);

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Constructor.
//
// Use: public

ChemBBoxAction::ChemBBoxAction()
//
////////////////////////////////////////////////////////////////////////
{
    SO_ACTION_CONSTRUCTOR(ChemBBoxAction);

    atomBBoxes = NULL;
    bondBBoxes = NULL;
    atomLabelBBoxes = bondLabelBBoxes = NULL;
    chemLabelBBoxes = NULL;
    chemMonitorBBoxes = NULL;

    atomClipOnCenter = TRUE;
    bondClipOnCenter = TRUE;
    atomLabelClipOnCenter = TRUE;
    bondLabelClipOnCenter = TRUE;
    chemLabelClipOnCenter = TRUE;
    chemMonitorClipOnCenter = TRUE;
// --> residue selection
		residueBBoxes = NULL;
		residueClipOnCenter = TRUE;
		residueLabelBBoxes = NULL;
		residueLabelClipOnCenter = TRUE;
		
// <-- residue selection

    reset(TRUE);
}

////////////////////////////////////////////////////////////////////////
// 
// Description:
//    Destructor.
//
// Use: public
 
ChemBBoxAction::~ChemBBoxAction()
//
////////////////////////////////////////////////////////////////////////
{
    nullEverything();
}

void
ChemBBoxAction::nullEverything()
{
    if (atomBBoxes != NULL) {
        delete atomBBoxes;
        atomBBoxes = NULL;
    }
    if (bondBBoxes != NULL) {
        if (bondBBoxes->bondBBoxType == ChemBondBBox::WIREFRAME) {
            delete (ChemWireframeBondBBox *)bondBBoxes;
        }
        else if (bondBBoxes->bondBBoxType == ChemBondBBox::STICK) {
            delete (ChemStickBondBBox *)bondBBoxes;
        }
        bondBBoxes = NULL;
    }
    if (atomLabelBBoxes != NULL) {
        delete atomLabelBBoxes;
        atomLabelBBoxes = NULL;
    }
    if (bondLabelBBoxes != NULL) {
        delete bondLabelBBoxes;
        bondLabelBBoxes = NULL;
    }
    if (chemLabelBBoxes != NULL) {
        delete chemLabelBBoxes;
        chemLabelBBoxes = NULL;
    }
    if (chemMonitorBBoxes != NULL) {
        delete chemMonitorBBoxes;
        chemMonitorBBoxes = NULL;
    }
// --> residue selection
		if (residueBBoxes != NULL)
		{
        if (residueBBoxes->bondBBoxType == ChemBondBBox::WIREFRAME) {
            delete (ChemWireframeBondBBox *)residueBBoxes;
        }
        else if (residueBBoxes->bondBBoxType == ChemBondBBox::STICK) {
            delete (ChemStickBondBBox *)residueBBoxes;
        }
        residueBBoxes = NULL;
    }
		if (residueLabelBBoxes != NULL)
		{
        delete residueLabelBBoxes;
        residueLabelBBoxes = NULL;
    }
// <-- residue selection
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Reset everything.
//
// Use: public

void
ChemBBoxAction::reset(SbBool doAll)
//
////////////////////////////////////////////////////////////////////////
{
    nullEverything();
    if (doAll) {
        parts = 0;
        atomClipOnCenter = TRUE;
        bondClipOnCenter = TRUE;
        atomLabelClipOnCenter = TRUE;
        bondLabelClipOnCenter = TRUE;
        chemLabelClipOnCenter = TRUE;
        chemMonitorClipOnCenter = TRUE;
// --> residue selection
				residueClipOnCenter = TRUE;
				residueLabelClipOnCenter = TRUE;
// <-- residue selection
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Initiates action on a graph.  This is where items are initialized
//    before traversal of the graph begins.
//
// Use: protected

void
ChemBBoxAction::beginTraversal(SoNode *node)
//
////////////////////////////////////////////////////////////////////////
{
    reset(FALSE);

    // Begin traversal at the given root node.
    traverse(node);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Implements the action for all non-relevant nodes.
//
// Use: private

void
ChemBBoxAction::callDoAction(SoAction *action, SoNode *node)
//
////////////////////////////////////////////////////////////////////////
{
    node->doAction(action);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Implements the action for all relevant nodes.  These would be
//    ChemDisplay nodes.
//
// Use: private

void
ChemBBoxAction::callBBoxAction(SoAction *action, SoNode *node)
//
////////////////////////////////////////////////////////////////////////
{
    ChemBBoxAction *bboxAction = (ChemBBoxAction *)action;

    int32_t parts = bboxAction->getParts();
    if (node->isOfType(ChemDisplay::getClassTypeId())) {
        ChemDisplay *chemDisplay = (ChemDisplay *)node;

        // Now fill out the bbox array.
        if ((parts & ChemBBoxAction::ATOMS) != 0) {
            chemDisplay->getAtomBBoxes(action, bboxAction->atomClipOnCenter,
                bboxAction->atomBBoxes);
        }
        if ((parts & ChemBBoxAction::BONDS) != 0) {
            chemDisplay->getBondBBoxes(action, bboxAction->bondClipOnCenter,
                bboxAction->bondBBoxes);
        }
        if ((parts & ChemBBoxAction::ATOMLABELS) != 0) {
            chemDisplay->getAtomLabelBBoxes(action,
                bboxAction->atomLabelClipOnCenter,
                bboxAction->atomLabelBBoxes);
        }
        if ((parts & ChemBBoxAction::BONDLABELS) != 0) {
            chemDisplay->getBondLabelBBoxes(action,
                bboxAction->bondLabelClipOnCenter,
                bboxAction->bondLabelBBoxes);
        }
// --> residue selection
				if ((parts & ChemBBoxAction::RESIDUES) != 0)
				{
            chemDisplay->getResidueBBoxes(action,
                bboxAction->residueClipOnCenter,
                bboxAction->residueBBoxes);
        }
				if ((parts & ChemBBoxAction::RESIDUELABELS) != 0)
				{
            chemDisplay->getResidueLabelBBoxes(action,
                bboxAction->residueLabelClipOnCenter,
                bboxAction->residueLabelBBoxes);
        }
// <-- residue selection
    }
    else if (node->isOfType(ChemLabel::getClassTypeId())) {
        ChemLabel *chemLabel = (ChemLabel *)node;
        if ((parts & ChemBBoxAction::CHEMLABELS) != 0) {
            chemLabel->getChemLabelBBoxes(action,
                bboxAction->chemLabelClipOnCenter,
                bboxAction->chemLabelBBoxes);
        }
    }
    else if (node->isOfType(ChemMonitor::getClassTypeId())) {
        ChemMonitor *chemMonitor = (ChemMonitor *)node;
        if ((parts & ChemBBoxAction::CHEMMONITORS) != 0) {
            chemMonitor->getChemMonitorBBoxes(action,
                bboxAction->chemMonitorClipOnCenter,
                bboxAction->chemMonitorBBoxes);
        }
    }
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Methods to get the current bbox data after the action has been
//    applied.
//
// Use: public

void
ChemBBoxAction::getAtomBBoxes(ChemAtomBBox *&_atomBBoxes) const
{
    _atomBBoxes = atomBBoxes;
}

void
ChemBBoxAction::getBondBBoxes(ChemBondBBox *&_bondBBoxes) const
{
    _bondBBoxes = bondBBoxes;
}

void
ChemBBoxAction::getAtomLabelBBoxes(ChemLabelBBox *&_atomLabelBBoxes) const
{
    _atomLabelBBoxes = atomLabelBBoxes;
}

void
ChemBBoxAction::getBondLabelBBoxes(ChemLabelBBox *&_bondLabelBBoxes) const
{
    _bondLabelBBoxes = bondLabelBBoxes;
}

void
ChemBBoxAction::getChemLabelBBoxes(ChemLabelBBox *&_chemLabelBBoxes) const
{
    _chemLabelBBoxes = chemLabelBBoxes;
}

void
ChemBBoxAction::getChemMonitorBBoxes(ChemMonitorBBox *&_chemMonitorBBoxes) const
{
    _chemMonitorBBoxes = chemMonitorBBoxes;
}

// --> residue selection
void ChemBBoxAction::getResidueBBoxes(ChemBondBBox *&_residueBBoxes) const
{
    _residueBBoxes = residueBBoxes;
}

void ChemBBoxAction::getResidueLabelBBoxes(ChemLabelBBox *&_residueLabelBBoxes) const
{
    _residueLabelBBoxes = residueLabelBBoxes;
}
// <-- residue selection
//
////////////////////////////////////////////////////////////////////////
