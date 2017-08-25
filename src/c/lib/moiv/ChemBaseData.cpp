/*
 * ChemBaseData.c++
 *
 *     Classes:  ChemBaseData
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

#include "inv/actions/SoCallbackAction.H"
#include "inv/actions/SoGetBoundingBoxAction.H"
#include "inv/actions/SoGLRenderAction.H"
#include "inv/actions/SoPickAction.H"
#include "inv/actions/SoWriteAction.H"

#include "inv/ChemKit/ChemBaseData.H"
#include "inv/ChemKit/ChemBaseDataElement.H"

SO_NODE_ABSTRACT_SOURCE(ChemBaseData);

ChemBaseData::ChemBaseData()
{
    SO_NODE_CONSTRUCTOR(ChemBaseData);

    SO_NODE_ADD_FIELD(associatedData, (NULL));

    SO_NODE_DEFINE_ENUM_VALUE(BondType, NONE);
    SO_NODE_DEFINE_ENUM_VALUE(BondType, SINGLE_BOND);
    SO_NODE_DEFINE_ENUM_VALUE(BondType, DOUBLE_BOND);
    SO_NODE_DEFINE_ENUM_VALUE(BondType, TRIPLE_BOND);
    SO_NODE_DEFINE_ENUM_VALUE(BondType, QUADRUPLE_BOND);
    SO_NODE_DEFINE_ENUM_VALUE(BondType, AROMATIC_BOND);
    SO_NODE_DEFINE_ENUM_VALUE(BondType, HYDROGEN_BOND);
    SO_NODE_DEFINE_ENUM_VALUE(BondType, RESONANCE_BOND);

    isBuiltIn = TRUE;

}

ChemBaseData::~ChemBaseData()
{
}

void
ChemBaseData::doAction(SoAction *action)
{
    ChemBaseDataElement::set(action->getState(), this, this);
}

void
ChemBaseData::GLRender(SoGLRenderAction *action)
{
    ChemBaseData::doAction(action);
}

void
ChemBaseData::pick(SoPickAction *action)
{
    ChemBaseData::doAction(action);
}

void
ChemBaseData::callback(SoCallbackAction *action)
{
    ChemBaseData::doAction(action);
}

void
ChemBaseData::getBoundingBox(SoGetBoundingBoxAction *action)
{
    ChemBaseData::doAction(action);
}
