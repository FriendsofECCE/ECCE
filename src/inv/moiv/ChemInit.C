/*
 * ChemInit.c++
 *
 *     Call initClasses for all node classes
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

#include "inv/elements/SoCacheElement.H" 
#include "inv/elements/SoClipPlaneElement.H" 
#include "inv/elements/SoFontNameElement.H"
#include "inv/elements/SoFontSizeElement.H"
#include "inv/elements/SoGLCacheContextElement.H"
#include "inv/elements/SoGLDrawStyleElement.H"
#include "inv/elements/SoModelMatrixElement.H"
#include "inv/elements/SoPickStyleElement.H"
#include "inv/elements/SoProjectionMatrixElement.H"
#include "inv/elements/SoSwitchElement.H"
#include "inv/elements/SoViewingMatrixElement.H"
#include "inv/elements/SoViewportRegionElement.H"
#include "inv/elements/SoViewVolumeElement.H"

#ifndef IV2_0
#include "inv/elements/SoGLLazyElement.H"
#include "inv/elements/SoLazyElement.H"
#endif
 
#include "inv/nodes/SoCamera.H"
#include "inv/nodes/SoClipPlane.H"
#include "inv/nodes/SoFont.H"
#include "inv/nodes/SoGroup.H"
#include "inv/nodes/SoPickStyle.H"
#include "inv/nodes/SoTransformation.H"

#include "inv/ChemKit/ChemInit.H"
#include "inv/ChemKit/ChemAssociatedData.H"
#include "inv/ChemKit/ChemAssociatedDataElement.H"
#include "inv/ChemKit/ChemBaseData.H"
#include "inv/ChemKit/ChemBaseDataElement.H"
#include "inv/ChemKit/ChemBBoxAction.H"
#include "inv/ChemKit/ChemColor.H"
#include "inv/ChemKit/ChemColorElement.H"
#include "inv/ChemKit/ChemData.H"
#include "inv/ChemKit/ChemDetail.H"
// --> lod
#include "inv/ChemKit/ChemLOD.H"
// <-- lod
#include "inv/ChemKit/ChemDisplay.H"
#include "inv/ChemKit/ChemDisplayKit.H"
#include "inv/ChemKit/ChemDisplayParam.H"
#include "inv/ChemKit/ChemDisplayParamElement.H"
#include "inv/ChemKit/ChemDisplayPath.H"
#include "inv/ChemKit/ChemDisplaySelectionElement.H"
#include "inv/ChemKit/ChemKit.H"
#include "inv/ChemKit/ChemLabel.H"
#include "inv/ChemKit/ChemLabelPath.H"
#include "inv/ChemKit/ChemLabelSelectionElement.H"
#include "inv/ChemKit/ChemMonitor.H"
#include "inv/ChemKit/ChemMonitorDetail.H"
#include "inv/ChemKit/ChemMonitorPath.H"
#include "inv/ChemKit/ChemMonitorSelectionElement.H"
#include "inv/ChemKit/ChemOverrideElement.H"
#include "inv/ChemKit/ChemPath.H"
#include "inv/ChemKit/ChemRadii.H"
#include "inv/ChemKit/ChemRadiiElement.H"
#include "inv/ChemKit/ChemSelection.H"
#include "inv/ChemKit/ChemSelectionElement.H"
#include "inv/ChemKit/ChemWireframeDetail.H"

#ifndef IV2_0
#include "inv/ChemKit/ChemContour.H"
#include "inv/ChemKit/ChemContour2.H"
#include "inv/ChemKit/ChemIso.H"
#include "inv/ChemKit/ChemLattice3.H"
#include "inv/ChemKit/ChemLattice3Element.H"
#endif

#include "inv/ChemKit/SFAtomSpec.H"
#include "inv/ChemKit/MFAtomSpec.H"

#include "inv/ChemKit/SFVec2AtomSpec.H"
#include "inv/ChemKit/MFVec2AtomSpec.H"
#include "inv/ChemKit/SFVec3AtomSpec.H"
#include "inv/ChemKit/MFVec3AtomSpec.H"
#include "inv/ChemKit/SFVec4AtomSpec.H"
#include "inv/ChemKit/MFVec4AtomSpec.H"

#include "inv/ChemKit/SFDouble.H"
#include "inv/ChemKit/MFDouble.H"
#include "inv/ChemKit/SFInt8.H"
#include "inv/ChemKit/MFInt8.H"
#include "inv/ChemKit/SFVec2i.H"
#include "inv/ChemKit/MFVec2i.H"
#include "inv/ChemKit/SFVec3i.H"
#include "inv/ChemKit/MFVec3i.H"
#include "inv/ChemKit/SFVec4i.H"
#include "inv/ChemKit/MFVec4i.H"

// --> CAstick
#include "inv/ChemKit/SbResidue.H"
#include "inv/ChemKit/SbChain.H"
#include "inv/ChemKit/ChemSAS.H"
// <-- CAstick

void
ChemInit::initClasses()
{
    static SbBool allInitialized = FALSE;

#ifdef DEBUG
printf ("In ChemInit::initClasses - allInitialized is %d\n", allInitialized);
#endif

    if (allInitialized)
        return;
    allInitialized = TRUE;

    if (ChemBaseDataElement::getClassTypeId() == SoType::badType())
        ChemBaseDataElement::initClass();

    if (SFAtomSpec::getClassTypeId() == SoType::badType())
        SFAtomSpec::initClass();

    if (MFAtomSpec::getClassTypeId() == SoType::badType())
        MFAtomSpec::initClass();

    if (SFVec2AtomSpec::getClassTypeId() == SoType::badType())
        SFVec2AtomSpec::initClass();

    if (MFVec2AtomSpec::getClassTypeId() == SoType::badType())
        MFVec2AtomSpec::initClass();

    if (SFVec3AtomSpec::getClassTypeId() == SoType::badType())
        SFVec3AtomSpec::initClass();

    if (MFVec3AtomSpec::getClassTypeId() == SoType::badType())
        MFVec3AtomSpec::initClass();

    if (SFVec4AtomSpec::getClassTypeId() == SoType::badType())
        SFVec4AtomSpec::initClass();

    if (MFVec4AtomSpec::getClassTypeId() == SoType::badType())
        MFVec4AtomSpec::initClass();

    if (SFDouble::getClassTypeId() == SoType::badType())
        SFDouble::initClass();

    if (MFDouble::getClassTypeId() == SoType::badType())
        MFDouble::initClass();

    if (SFInt8::getClassTypeId() == SoType::badType())
        SFInt8::initClass();

    if (SFUInt8::getClassTypeId() == SoType::badType())
        SFUInt8::initClass();

    if (MFInt8::getClassTypeId() == SoType::badType())
        MFInt8::initClass();

    if (MFUInt8::getClassTypeId() == SoType::badType())
        MFUInt8::initClass();

    if (SFVec2i::getClassTypeId() == SoType::badType())
        SFVec2i::initClass();

    if (MFVec2i::getClassTypeId() == SoType::badType())
        MFVec2i::initClass();

    if (SFVec3i::getClassTypeId() == SoType::badType())
        SFVec3i::initClass();

    if (MFVec3i::getClassTypeId() == SoType::badType())
        MFVec3i::initClass();

    if (SFVec4i::getClassTypeId() == SoType::badType())
        SFVec4i::initClass();

    if (MFVec4i::getClassTypeId() == SoType::badType())
        MFVec4i::initClass();

    if (ChemPath::getClassTypeId() == SoType::badType())
        ChemPath::initClass();

    if (ChemDisplayPath::getClassTypeId() == SoType::badType())
        ChemDisplayPath::initClass();

    if (ChemLabelPath::getClassTypeId() == SoType::badType())
        ChemLabelPath::initClass();

    if (ChemMonitorPath::getClassTypeId() == SoType::badType())
        ChemMonitorPath::initClass();

    if (ChemAssociatedDataElement::getClassTypeId() == SoType::badType())
        ChemAssociatedDataElement::initClass();

    if (ChemOverrideElement::getClassTypeId() == SoType::badType())
        ChemOverrideElement::initClass();

    if (ChemDisplayParamElement::getClassTypeId() == SoType::badType())
        ChemDisplayParamElement::initClass();

    if (ChemDisplaySelectionElement::getClassTypeId() == SoType::badType())
        ChemDisplaySelectionElement::initClass();

    if (ChemLabelSelectionElement::getClassTypeId() == SoType::badType())
        ChemLabelSelectionElement::initClass();

    if (ChemMonitorSelectionElement::getClassTypeId() == SoType::badType())
        ChemMonitorSelectionElement::initClass();

    if (ChemRadiiElement::getClassTypeId() == SoType::badType())
        ChemRadiiElement::initClass();

    if (ChemColorElement::getClassTypeId() == SoType::badType())
        ChemColorElement::initClass();

#ifndef IV2_0
    if (ChemLattice3Element::getClassTypeId() == SoType::badType())
        ChemLattice3Element::initClass();
#endif

    if (ChemSelectionElement::getClassTypeId() == SoType::badType())
        ChemSelectionElement::initClass();

    if (ChemBaseData::getClassTypeId() == SoType::badType())
        ChemBaseData::initClass();
 
    if (ChemAssociatedData::getClassTypeId() == SoType::badType())
        ChemAssociatedData::initClass();

    if (ChemData::getClassTypeId() == SoType::badType())
        ChemData::initClass();

    if (ChemDisplayParam::getClassTypeId() == SoType::badType())
        ChemDisplayParam::initClass();

    if (ChemLabel::getClassTypeId() == SoType::badType())
        ChemLabel::initClass();

    if (ChemMonitor::getClassTypeId() == SoType::badType())
        ChemMonitor::initClass();

    if (ChemRadii::getClassTypeId() == SoType::badType())
        ChemRadii::initClass();

    if (ChemSelection::getClassTypeId() == SoType::badType())
        ChemSelection::initClass();

    if (ChemColor::getClassTypeId() == SoType::badType())
        ChemColor::initClass();

    if (ChemDetail::getClassTypeId() == SoType::badType())
        ChemDetail::initClass();

    if (ChemMonitorDetail::getClassTypeId() == SoType::badType())
        ChemMonitorDetail::initClass();

    if (ChemWireframeDetail::getClassTypeId() == SoType::badType())
        ChemWireframeDetail::initClass();

    if (ChemDisplay::getClassTypeId() == SoType::badType())
        ChemDisplay::initClass();
  
    if (ChemDisplayKit::getClassTypeId() == SoType::badType())
        ChemDisplayKit::initClass();
  
    if (ChemKit::getClassTypeId() == SoType::badType())
        ChemKit::initClass();

    if (ChemBBoxAction::getClassTypeId() == SoType::badType())
        ChemBBoxAction::initClass();

#ifndef IV2_0
    if (ChemContour::getClassTypeId() == SoType::badType())
        ChemContour::initClass();

    if (ChemContour2::getClassTypeId() == SoType::badType())
        ChemContour2::initClass();

    if (ChemLattice3::getClassTypeId() == SoType::badType())
        ChemLattice3::initClass();
  
    if (ChemIso::getClassTypeId() == SoType::badType())
        ChemIso::initClass();
#endif
// --> CAstick
		if (SFResidue::getClassTypeId() == SoType::badType())
        SFResidue::initClass();
		if (MFResidue::getClassTypeId() == SoType::badType())
        MFResidue::initClass();
		if (SFChain::getClassTypeId() == SoType::badType())
        SFChain::initClass();
		if (MFChain::getClassTypeId() == SoType::badType())
        MFChain::initClass();
		if (ChemSAS::getClassTypeId() == SoType::badType())
        ChemSAS::initClass();

// <-- CAstick
}

void
ChemAssociatedData::initClass()
{   
    SO_NODE_INIT_CLASS(ChemAssociatedData, SoNode, "Node");
    
    SO_ENABLE(SoGLRenderAction, ChemAssociatedDataElement);
    SO_ENABLE(SoCallbackAction, ChemAssociatedDataElement);
    SO_ENABLE(SoGetBoundingBoxAction, ChemAssociatedDataElement);
    SO_ENABLE(SoPickAction,     ChemAssociatedDataElement);

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}
    
void
ChemAssociatedDataElement::initClass()
{
    SO_ELEMENT_INIT_CLASS(ChemAssociatedDataElement, SoReplacedElement);
}

void
ChemBaseData::initClass()
{
    SO_NODE_INIT_ABSTRACT_CLASS(ChemBaseData, SoNode, "Node");

    SO_ENABLE(SoWriteAction, ChemBaseDataElement);
    SO_ENABLE(SoGLRenderAction, ChemBaseDataElement);
    SO_ENABLE(SoPickAction, ChemBaseDataElement);
    SO_ENABLE(SoGetBoundingBoxAction, ChemBaseDataElement);
    SO_ENABLE(SoCallbackAction, ChemBaseDataElement);
}
    
void
ChemBaseDataElement::initClass()
{
    SO_ELEMENT_INIT_CLASS(ChemBaseDataElement, SoReplacedElement);
}

void
ChemBBoxAction::initClass()
{
    SO_ACTION_INIT_CLASS(ChemBBoxAction, SoAction);

    SO_ENABLE(ChemBBoxAction, ChemBaseDataElement);
    SO_ENABLE(ChemBBoxAction, ChemOverrideElement);
    SO_ENABLE(ChemBBoxAction, ChemDisplayParamElement);
    SO_ENABLE(ChemBBoxAction, ChemRadiiElement);
    SO_ENABLE(ChemBBoxAction, ChemColorElement);

    SO_ENABLE(ChemBBoxAction, SoCacheElement);
    SO_ENABLE(ChemBBoxAction, SoClipPlaneElement);
    SO_ENABLE(ChemBBoxAction, SoFontNameElement);
    SO_ENABLE(ChemBBoxAction, SoFontSizeElement);
    SO_ENABLE(ChemBBoxAction, SoGLCacheContextElement);
    SO_ENABLE(ChemBBoxAction, SoGLDrawStyleElement);
    SO_ENABLE(ChemBBoxAction, SoModelMatrixElement);
    SO_ENABLE(ChemBBoxAction, SoPickStyleElement);
    SO_ENABLE(ChemBBoxAction, SoProjectionMatrixElement);
    SO_ENABLE(ChemBBoxAction, SoSwitchElement);
    SO_ENABLE(ChemBBoxAction, SoViewingMatrixElement);
    SO_ENABLE(ChemBBoxAction, SoViewportRegionElement);
    SO_ENABLE(ChemBBoxAction, SoViewVolumeElement);
#ifndef IV2_0
        // MPM patch to prevent aplying this patch on SGI
#ifndef __sgi
	//  JAD FIX for COIN3D.
// --> Coin2 compatibility
	if (SoGLLazyElement::getClassTypeId() == SoType::badType())
// <-- Coin2 compatibility
	SoGLLazyElement::initClass();
#endif

    SO_ENABLE(ChemBBoxAction, SoGLLazyElement);
    SO_ENABLE(ChemBBoxAction, SoLazyElement);
#endif

    SO_ACTION_ADD_METHOD(SoNode, nullAction);

    SO_ACTION_ADD_METHOD(ChemDisplay, callBBoxAction);
    SO_ACTION_ADD_METHOD(ChemLabel, callBBoxAction);
    SO_ACTION_ADD_METHOD(ChemMonitor, callBBoxAction);

    SO_ACTION_ADD_METHOD(ChemBaseData,     callDoAction);
    SO_ACTION_ADD_METHOD(ChemDisplayParam, callDoAction);
    SO_ACTION_ADD_METHOD(ChemRadii,        callDoAction);
    SO_ACTION_ADD_METHOD(ChemColor,        callDoAction);

    SO_ACTION_ADD_METHOD(SoBaseKit,        callDoAction);
    SO_ACTION_ADD_METHOD(SoCamera,         callDoAction);
    SO_ACTION_ADD_METHOD(SoClipPlane,      callDoAction);
    SO_ACTION_ADD_METHOD(SoGroup,          callDoAction);
    SO_ACTION_ADD_METHOD(SoFont,           callDoAction);
    SO_ACTION_ADD_METHOD(SoPickStyle,      callDoAction);
    SO_ACTION_ADD_METHOD(SoTransformation, callDoAction);

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}

void
ChemColor::initClass()
{   
    SO_NODE_INIT_CLASS(ChemColor, SoNode, "Node");
    
    SO_ENABLE(SoGLRenderAction, ChemOverrideElement);
    SO_ENABLE(SoGLRenderAction, ChemColorElement);

    SO_ENABLE(SoCallbackAction, ChemOverrideElement);
    SO_ENABLE(SoCallbackAction, ChemColorElement);

    SO_ENABLE(SoGetBoundingBoxAction, ChemOverrideElement);
    SO_ENABLE(SoGetBoundingBoxAction, ChemColorElement);

    SO_ENABLE(SoPickAction,     ChemOverrideElement);
    SO_ENABLE(SoPickAction,     ChemColorElement);

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}

void
ChemColorElement::initClass()
{
    SO_ELEMENT_INIT_CLASS(ChemColorElement, SoReplacedElement);
}

#ifndef IV2_0
void
ChemContour::initClass()
{
    SO_NODE_INIT_CLASS(ChemContour, SoShape, "Shape");

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}

void
ChemContour2::initClass()
{
    SO_NODE_INIT_CLASS(ChemContour2, SoShape, "Shape");

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}
#endif
  
void
ChemData::initClass()
{ 
    if (ChemBaseDataElement::getClassTypeId() == SoType::badType())
        ChemBaseDataElement::initClass();

    if (ChemBaseData::getClassTypeId() == SoType::badType())
        ChemBaseData::initClass();

    SO_NODE_INIT_CLASS(ChemData, ChemBaseData, "ChemBaseData");

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}

void
ChemDetail::initClass()
{
	SO_DETAIL_INIT_CLASS(ChemDetail, SoDetail);
}

void
ChemMonitorDetail::initClass()
{
	SO_DETAIL_INIT_CLASS(ChemMonitorDetail, SoDetail);
}

void
ChemWireframeDetail::initClass()
{
	SO_DETAIL_INIT_CLASS(ChemWireframeDetail, ChemDetail);
}

void
ChemDisplay::initClass()
{ 
    SO_NODE_INIT_CLASS(ChemDisplay, SoNonIndexedShape, "NonIndexedShape");

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}
  
void
ChemDisplayKit::initClass()
{
    SO_KIT_INIT_CLASS(ChemDisplayKit, SoBaseKit, "BaseKit");
}

void
ChemDisplayParam::initClass()
{   
    SO_NODE_INIT_CLASS(ChemDisplayParam, SoNode, "Node");
    
    SO_ENABLE(SoGLRenderAction, ChemOverrideElement);
    SO_ENABLE(SoGLRenderAction, ChemDisplayParamElement);

    SO_ENABLE(SoCallbackAction, ChemOverrideElement);
    SO_ENABLE(SoCallbackAction, ChemDisplayParamElement);

    SO_ENABLE(SoGetBoundingBoxAction, ChemOverrideElement);
    SO_ENABLE(SoGetBoundingBoxAction, ChemDisplayParamElement);

    SO_ENABLE(SoPickAction,     ChemOverrideElement);
    SO_ENABLE(SoPickAction,     ChemDisplayParamElement);

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}
      
void
ChemDisplayParamElement::initClass()
{
    SO_ELEMENT_INIT_CLASS(ChemDisplayParamElement, SoReplacedElement);
}

void
ChemDisplayPath::initClass()
{       
    // Create SoType type
    classTypeId = SoType::createType(SoBase::getClassTypeId(),
                     "ChemDisplayPath",
                     ChemDisplayPath::createInstance);

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}

void
ChemDisplaySelectionElement::initClass()
{
    SO_ELEMENT_INIT_CLASS(ChemDisplaySelectionElement, SoElement);
}
  
#ifndef IV2_0
void
ChemIso::initClass()
{
    SO_NODE_INIT_CLASS(ChemIso, SoShape, "Shape");

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}
#endif

void
ChemKit::initClass()
{
    SO_KIT_INIT_CLASS(ChemKit, SoBaseKit, "BaseKit");

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}

void
ChemLabel::initClass()
{ 
    SO_NODE_INIT_CLASS(ChemLabel, SoNonIndexedShape, "NonIndexedShape");

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}

void
ChemLabelPath::initClass()
{       
    // Create SoType type
    classTypeId = SoType::createType(SoBase::getClassTypeId(),
                     "ChemLabelPath",
                     ChemLabelPath::createInstance);

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}

void
ChemLabelSelectionElement::initClass()
{
    SO_ELEMENT_INIT_CLASS(ChemLabelSelectionElement, SoElement);
}

#ifndef IV2_0
void
ChemLattice3::initClass()
{
    SO_NODE_INIT_CLASS(ChemLattice3, SoNode, "Node");

    SO_ENABLE(SoWriteAction, ChemLattice3Element);
    SO_ENABLE(SoGLRenderAction, ChemLattice3Element);
    SO_ENABLE(SoPickAction, ChemLattice3Element);
    SO_ENABLE(SoGetBoundingBoxAction, ChemLattice3Element);
    SO_ENABLE(SoCallbackAction, ChemLattice3Element);

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}

void
ChemLattice3Element::initClass()
{
    SO_ELEMENT_INIT_CLASS(ChemLattice3Element, SoReplacedElement);
}
#endif

void
ChemMonitor::initClass()
{ 
    SO_NODE_INIT_CLASS(ChemMonitor, SoNonIndexedShape, "NonIndexedShape");

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}

void
ChemMonitorPath::initClass()
{       
    // Create SoType type
    classTypeId = SoType::createType(SoBase::getClassTypeId(),
                     "ChemMonitorPath",
                     ChemMonitorPath::createInstance);

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}

void
ChemMonitorSelectionElement::initClass()
{
    SO_ELEMENT_INIT_CLASS(ChemMonitorSelectionElement, SoElement);
}

void
ChemOverrideElement::initClass()
{
    SO_ELEMENT_INIT_CLASS(ChemOverrideElement, SoElement);
}

void
ChemPath::initClass()
{       
    // Create SoType type
    classTypeId = SoType::createType(SoBase::getClassTypeId(), "ChemPath");

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}
  
void
ChemRadii::initClass()
{   
    SO_NODE_INIT_CLASS(ChemRadii, SoNode, "Node");
    
    SO_ENABLE(SoGLRenderAction, ChemOverrideElement);
    SO_ENABLE(SoGLRenderAction, ChemRadiiElement);

    SO_ENABLE(SoCallbackAction, ChemOverrideElement);
    SO_ENABLE(SoCallbackAction, ChemRadiiElement);

    SO_ENABLE(SoGetBoundingBoxAction, ChemOverrideElement);
    SO_ENABLE(SoGetBoundingBoxAction, ChemRadiiElement);

    SO_ENABLE(SoPickAction,     ChemOverrideElement);
    SO_ENABLE(SoPickAction,     ChemRadiiElement);

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}

void
ChemRadiiElement::initClass()
{
    SO_ELEMENT_INIT_CLASS(ChemRadiiElement, SoReplacedElement);
}
  
void
ChemSelection::initClass()
{ 
    if (ChemBaseDataElement::getClassTypeId() == SoType::badType())
        ChemBaseDataElement::initClass();

    if (ChemBaseData::getClassTypeId() == SoType::badType())
        ChemBaseData::initClass();

    SO_NODE_INIT_CLASS(ChemSelection, SoSeparator, "Separator");
    
    if (ChemDisplaySelectionElement::getClassTypeId() == SoType::badType())
        ChemDisplaySelectionElement::initClass();
    
    if (ChemLabelSelectionElement::getClassTypeId() == SoType::badType())
        ChemLabelSelectionElement::initClass();
    
    if (ChemMonitorSelectionElement::getClassTypeId() == SoType::badType())
        ChemMonitorSelectionElement::initClass();
    
    if (ChemSelectionElement::getClassTypeId() == SoType::badType())
        ChemSelectionElement::initClass();

    SO_ENABLE(SoGLRenderAction, ChemDisplaySelectionElement);
    SO_ENABLE(SoGetBoundingBoxAction, ChemDisplaySelectionElement);
//>> Ray pick actions must be enabled with ChemDisplaySelection elements
		SO_ENABLE(SoPickAction, ChemDisplaySelectionElement);
//<< Ray pick actions must be enabled with ChemDisplaySelection elements
    SO_ENABLE(SoGLRenderAction, ChemLabelSelectionElement);
    SO_ENABLE(SoGetBoundingBoxAction, ChemLabelSelectionElement);
    SO_ENABLE(SoGLRenderAction, ChemMonitorSelectionElement);
    SO_ENABLE(SoGetBoundingBoxAction, ChemMonitorSelectionElement);
    SO_ENABLE(SoPickAction, ChemSelectionElement);

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}

void
ChemSelectionElement::initClass()
{
    SO_ELEMENT_INIT_CLASS(ChemSelectionElement, SoInt32Element);
}

void
SFAtomSpec::initClass()
{
    // This macro takes the name of the class and the name of the
    // parent class
    SO_SFIELD_INIT_CLASS(SFAtomSpec, SoSField);

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}

void
MFAtomSpec::initClass()
{   
    // This macro takes the name of the class and the name of the
    // parent class
    SO_MFIELD_INIT_CLASS(MFAtomSpec, SoMField);

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}

void
SFVec2AtomSpec::initClass()
{
    // This macro takes the name of the class and the name of the
    // parent class
    SO_SFIELD_INIT_CLASS(SFVec2AtomSpec, SoSField);

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}

void
MFVec2AtomSpec::initClass()
{
    // This macro takes the name of the class and the name of the
    // parent class
    SO_MFIELD_INIT_CLASS(MFVec2AtomSpec, SoMField);

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}

void
SFVec3AtomSpec::initClass()
{
    // This macro takes the name of the class and the name of the
    // parent class
    SO_SFIELD_INIT_CLASS(SFVec3AtomSpec, SoSField);

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}

void
MFVec3AtomSpec::initClass()
{
    // This macro takes the name of the class and the name of the
    // parent class
    SO_MFIELD_INIT_CLASS(MFVec3AtomSpec, SoMField);

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}

void
SFVec4AtomSpec::initClass()
{
    // This macro takes the name of the class and the name of the
    // parent class
    SO_SFIELD_INIT_CLASS(SFVec4AtomSpec, SoSField);

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}

void
MFVec4AtomSpec::initClass()
{
    // This macro takes the name of the class and the name of the
    // parent class
    SO_MFIELD_INIT_CLASS(MFVec4AtomSpec, SoMField);

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}

void
SFDouble::initClass()
{
    // This macro takes the name of the class and the name of the
    // parent class
    SO_SFIELD_INIT_CLASS(SFDouble, SoSField);

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}

void
MFDouble::initClass()
{   
    // This macro takes the name of the class and the name of the
    // parent class
    SO_MFIELD_INIT_CLASS(MFDouble, SoMField);

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}

void
SFInt8::initClass()
{
    // This macro takes the name of the class and the name of the
    // parent class
    SO_SFIELD_INIT_CLASS(SFInt8, SoSField);

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}

void
SFUInt8::initClass()
{
    // This macro takes the name of the class and the name of the
    // parent class
    SO_SFIELD_INIT_CLASS(SFUInt8, SoSField);

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}

void
MFInt8::initClass()
{   
    // This macro takes the name of the class and the name of the
    // parent class
    SO_MFIELD_INIT_CLASS(MFInt8, SoMField);

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}

void
MFUInt8::initClass()
{   
    // This macro takes the name of the class and the name of the
    // parent class
    SO_MFIELD_INIT_CLASS(MFUInt8, SoMField);

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}

void
SFVec2i::initClass()
{
    // This macro takes the name of the class and the name of the
    // parent class
    SO_SFIELD_INIT_CLASS(SFVec2i, SoSField);

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}

void
MFVec2i::initClass()
{   
    // This macro takes the name of the class and the name of the
    // parent class
    SO_MFIELD_INIT_CLASS(MFVec2i, SoMField);

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}

void
SFVec3i::initClass()
{
    // This macro takes the name of the class and the name of the
    // parent class
    SO_SFIELD_INIT_CLASS(SFVec3i, SoSField);

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}

void
MFVec3i::initClass()
{   
    // This macro takes the name of the class and the name of the
    // parent class
    SO_MFIELD_INIT_CLASS(MFVec3i, SoMField);

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}

void
SFVec4i::initClass()
{
    // This macro takes the name of the class and the name of the
    // parent class
    SO_SFIELD_INIT_CLASS(SFVec4i, SoSField);

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}

void
MFVec4i::initClass()
{   
    // This macro takes the name of the class and the name of the
    // parent class
    SO_MFIELD_INIT_CLASS(MFVec4i, SoMField);

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}

// --> CAstick
void SFResidue::initClass(void)
{
	  // This macro takes the name of the class and the name of the
    // parent class
    SO_SFIELD_INIT_CLASS(SFResidue, SoSField);

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}

void MFResidue::initClass()
{   
    // This macro takes the name of the class and the name of the
    // parent class
    SO_MFIELD_INIT_CLASS(MFResidue, SoMField);

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}

void SFChain::initClass(void)
{
	  // This macro takes the name of the class and the name of the
    // parent class
    SO_SFIELD_INIT_CLASS(SFChain, SoSField);

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}

void MFChain::initClass()
{   
    // This macro takes the name of the class and the name of the
    // parent class
    SO_MFIELD_INIT_CLASS(MFChain, SoMField);

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}

void
ChemSAS::initClass()
{   
    SO_NODE_INIT_CLASS(ChemSAS, SoNode, "SAS");
    
/*    SO_ENABLE(SoGLRenderAction, ChemAssociatedDataElement);
    SO_ENABLE(SoCallbackAction, ChemAssociatedDataElement);
    SO_ENABLE(SoGetBoundingBoxAction, ChemAssociatedDataElement);
    SO_ENABLE(SoPickAction,     ChemAssociatedDataElement);*/

    // Initialize all other classes if necessary
    ChemInit::initClasses();
}

// <-- CAstick
