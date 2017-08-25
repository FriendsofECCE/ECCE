/*
 * ChemDisplayParam.c++
 *
 *     Classes:  ChemDisplayParam
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

/*
	Porting to Windows:
	Jamie Doornbos	2001/07/15
	Daniel Soto	2002/02/05
*/
#ifdef WIN32
#include <windows.h>
#endif
#ifndef __gl_h_
#include <GL/gl.h>
#endif /* __gl_h_ */

#include "inv/actions/SoGLRenderAction.H"
#include "inv/actions/SoCallbackAction.H"
#include "inv/actions/SoPickAction.H"
#include "inv/actions/SoGetBoundingBoxAction.H"
#include "inv/elements/SoGLClipPlaneElement.H"
#include "inv/elements/SoFontNameElement.H"
#include "inv/elements/SoFontSizeElement.H"
#include "inv/elements/SoViewVolumeElement.H"
#include "inv/nodes/SoClipPlane.H"

#include "inv/ChemKit/ChemDisplayParam.H"
#include "inv/ChemKit/ChemDisplayParamElement.H"
#include "inv/ChemKit/ChemOverrideElement.H"

/*
	Porting to Windows:
	Jamie Doornbos	2001/07/15
	Daniel Soto	2002/02/05
*/
#include "inv/ChemKit/FixFonts.H"

const uint8_t defaultHighlightAtomBitmap[] =
    {0xc0, 0xc0,  0x61, 0x80,
     0x33, 0x00,  0x1e, 0x00,  0x0c, 0x00,
     0x1e, 0x00,  0x33, 0x00,
     0x61, 0x80,  0xc0, 0xc0
};


SO_NODE_SOURCE(ChemDisplayParam);


////////////////////////////////////////////////////////////////////////
//
// Description:
//    Constructor
//
// Use: public

ChemDisplayParam::ChemDisplayParam()
//
////////////////////////////////////////////////////////////////////////
{
    mShowRings = TRUE;

    SO_NODE_CONSTRUCTOR(ChemDisplayParam);

    // General Fields:
    SO_NODE_ADD_FIELD(displayStyle, (ChemDisplayParam::DISPLAY_DEFAULT));
    SO_NODE_ADD_FIELD(showHydrogens, (TRUE));

    // Atom Label Fields:
    SO_NODE_ADD_FIELD(showAtomLabels, (FALSE));
    SO_NODE_ADD_FIELD(atomLabelLeftRightJustification,
                      (ChemDisplayParam::LABEL_LR_DEFAULT));
    SO_NODE_ADD_FIELD(atomLabelTopBottomJustification,
                      (ChemDisplayParam::LABEL_TB_DEFAULT));
    SO_NODE_ADD_FIELD(atomLabelString, ("%a"));

    // Atom Fields:
    SO_NODE_ADD_FIELD(atomPickRadius, (0.2f));
    SO_NODE_ADD_FIELD(atomRadiiScaleFactor, (1.0));
    SO_NODE_ADD_FIELD(atomSphereComplexity, (0.5));
    SO_NODE_ADD_FIELD(atomSphereDisplayStyle, 
                      (ChemDisplayParam::ATOMSPHERE_DEFAULT));

    // Bond Label Fields:
    SO_NODE_ADD_FIELD(showBondLabels, (FALSE));
    SO_NODE_ADD_FIELD(bondLabelLeftRightJustification,
                      (ChemDisplayParam::LABEL_LR_DEFAULT));
    SO_NODE_ADD_FIELD(bondLabelTopBottomJustification,
                      (ChemDisplayParam::LABEL_TB_DEFAULT));
    SO_NODE_ADD_FIELD(bondLabelString, ("%d"));

    // Bond Fields:
    SO_NODE_ADD_FIELD(showMultipleBonds, (TRUE));
    SO_NODE_ADD_FIELD(multipleBondSeparation, (0.15f));
    SO_NODE_ADD_FIELD(bondCylinderRadius, (0.15f));
    SO_NODE_ADD_FIELD(bondCylinderComplexity, (0.04f));
    SO_NODE_ADD_FIELD(bondCylinderDisplayStyle,
                      (ChemDisplayParam::BONDCYLINDER_DEFAULT));

    // Special Bond Fields:
    SO_NODE_ADD_FIELD(hydrogenBondStippleFactor, (1));
    SO_NODE_ADD_FIELD(hydrogenBondStipplePattern, (0x0F0F));
    SO_NODE_ADD_FIELD(resonanceBondStippleFactor, (1));
    SO_NODE_ADD_FIELD(resonanceBondStipplePattern, (0x0F0F));

    // Wireframe Fields:
    SO_NODE_ADD_FIELD(bondWireframeAntiAlias,
        (ChemDisplayParam::WIREFRAME_ANTIALIAS_DEFAULT));
    SO_NODE_ADD_FIELD(bondWireframeLineWidth, (1.0));

    // Size of rings to search for:
    SO_NODE_ADD_FIELD(maxRingSize, (6));

    // Aromatic Ring Fields:
    SO_NODE_ADD_FIELD(aromaticRingNumSides, (16));
    SO_NODE_ADD_FIELD(aromaticRingColor, (0.50, 0.50, 0.50));
    SO_NODE_ADD_FIELD(aromaticRingScale, (0.80f));

    // BallStick Fields:
    SO_NODE_ADD_FIELD(ballStickSphereScaleFactor, (0.20f));

    // Highlight Fields:
    SO_NODE_ADD_FIELD(highlightStyle, (ChemDisplayParam::HIGHLIGHT_DEFAULT));
    SO_NODE_ADD_FIELD(highlightColor, (0.3f, 0.3f, 0.0f));
    SO_NODE_ADD_FIELD(highlightStippleFactor, (1));
    SO_NODE_ADD_FIELD(highlightStipplePattern, (0x0F0F));
    SO_NODE_ADD_FIELD(highlightAtomStyle,
                     (ChemDisplayParam::HIGHLIGHT_ATOM_DEFAULT));
		
    SO_NODE_ADD_FIELD(highlightAtomBitmap, (0));
    highlightAtomBitmap.deleteValues(0);
    highlightAtomBitmap.setValues(0, 18, defaultHighlightAtomBitmap);
    SO_NODE_ADD_FIELD(highlightAtomBitmapWidth, (10));
    SO_NODE_ADD_FIELD(highlightAtomBitmapHeight, (9));

    // NonBonded Atom Fields:
    SO_NODE_ADD_FIELD(nonBondedAtomStyle,
                     (ChemDisplayParam::NONBONDED_ATOM_DEFAULT));
    SO_NODE_ADD_FIELD(nonBondedAtomBitmap, (0));
    nonBondedAtomBitmap.deleteValues(0);
    nonBondedAtomBitmap.setValues(0, 18, defaultHighlightAtomBitmap);
    SO_NODE_ADD_FIELD(nonBondedAtomBitmapWidth, (10));
    SO_NODE_ADD_FIELD(nonBondedAtomBitmapHeight, (9));

/*
	Porting to Windows:
	Jamie Doornbos	2001/07/15
	Daniel Soto	2002/02/05
*/
    // Text Font Fields:
    SO_NODE_ADD_FIELD(fontName, (DEFAULT_FONT));
// --> multiplatform fonts
//    SO_NODE_ADD_FIELD(fontSize, (10.0));
		 SO_NODE_ADD_FIELD(fontSize, (12.0));
// <-- multiplatform fonts
    SO_NODE_ADD_FIELD(highlightFontName, (HIGHLIGHT_DEFAULT_FONT));
    SO_NODE_ADD_FIELD(highlightFontSize, (10.0));

    // Depth-cueing (fog)
    SO_NODE_ADD_FIELD(fogType, (ChemDisplayParam::FOG_DEFAULT));
    SO_NODE_ADD_FIELD(fogColor, (0.0, 0.0, 0.0));
    SO_NODE_ADD_FIELD(fogDensity, (1.0));
    SO_NODE_ADD_FIELD(fogStart, (0.0));
    SO_NODE_ADD_FIELD(fogVisibility, (0.0));

    // Clipping planes
    SO_NODE_ADD_FIELD(clipPlane, (NULL));

// --> residues
		// Residues add fields
		// Residue Label Fields:
    SO_NODE_ADD_FIELD(showResidueLabels, (FALSE));
    SO_NODE_ADD_FIELD(residueLabelLeftRightJustification,
                      (ChemDisplayParam::LABEL_LR_DEFAULT));
    SO_NODE_ADD_FIELD(residueLabelTopBottomJustification,
                      (ChemDisplayParam::LABEL_TB_DEFAULT));
    SO_NODE_ADD_FIELD(residueLabelString, ("%i"));
		SO_NODE_ADD_FIELD(residueLabelOffset, (0.0, 0.0, -5.0));

		// Residue fields

    SO_NODE_ADD_FIELD(residueCylinderRadius, (1.0f));
    SO_NODE_ADD_FIELD(residueCylinderComplexity, (0.4f));
		SO_NODE_ADD_FIELD(residueDisplayStyle,
			(ChemDisplayParam::DISPLAY_RESIDUES_CAWIRE));
		SO_NODE_ADD_FIELD(residueCylinderDisplayStyle,
			(ChemDisplayParam::BONDCYLINDER_ROUNDCAP));

		// Wireframe Fields:
    SO_NODE_ADD_FIELD(residueWireframeAntiAlias,
        (ChemDisplayParam::WIREFRAME_ANTIALIAS_DEFAULT));
    SO_NODE_ADD_FIELD(residueWireframeLineWidth, (1.0));

		SO_NODE_ADD_FIELD(residueTurnWidth, (1.5));
		SO_NODE_ADD_FIELD(residueCoilWidth, (1.0));
		SO_NODE_ADD_FIELD(residueSheetWidth, (3.0));
		SO_NODE_ADD_FIELD(residueHelixWidth, (3.0));
		SO_NODE_ADD_FIELD(residueHelixOffsetWidth, (3.0));

// <-- residues
// --> line/flat/solid ribbon
		SO_NODE_ADD_FIELD(residueNumberOfThreads,(5));
		SO_NODE_ADD_FIELD(residueHeigthFactor, (0.25));
		SO_NODE_ADD_FIELD(residueInterpolateColor, (TRUE));
		SO_NODE_ADD_FIELD(residueSelectAtoms, (FALSE));
		SO_NODE_ADD_FIELD(residueMaxDistance, (8.0f));

		SO_NODE_ADD_FIELD(solidRibbonSmoothNormals,(TRUE));
// <-- line/flat/solid ribbon
// --> schematic
		SO_NODE_ADD_FIELD(schematicCylinderComplexity,(0.7f));
		SO_NODE_ADD_FIELD(schematicCylinderRadius,(2.3f));
		SO_NODE_ADD_FIELD(schematicArrowTesselationSegments, (5));
		SO_NODE_ADD_FIELD(schematicTubeTesselationSegments, (5));
		SO_NODE_ADD_FIELD(schematicTubeShapeSegments, (8));
		SO_NODE_ADD_FIELD(schematicTubeRadius, (0.7f));
		SO_NODE_ADD_FIELD(schematicCylinderMininumResidues,(3));
		SO_NODE_ADD_FIELD(schematicArrowMininumResidues,(2));
// <-- schematic
/*
	Porting to Windows:
	Jamie Doornbos	2001/07/15

	This function has been removed from TGS and is now private in Coin3D.
*/
#if 0
    clipPlane.deleteValues(0);
#endif

    // Set up static info for enum type fields:

    SO_NODE_DEFINE_ENUM_VALUE(DisplayBinding, DISPLAY_NONE);
    SO_NODE_DEFINE_ENUM_VALUE(DisplayBinding, DISPLAY_CPK);
    SO_NODE_DEFINE_ENUM_VALUE(DisplayBinding, DISPLAY_STICK);
    SO_NODE_DEFINE_ENUM_VALUE(DisplayBinding, DISPLAY_BALLSTICK);
    SO_NODE_DEFINE_ENUM_VALUE(DisplayBinding, DISPLAY_WIREFRAME);
    SO_NODE_DEFINE_ENUM_VALUE(DisplayBinding, DISPLAY_BALLWIRE);
    SO_NODE_DEFINE_ENUM_VALUE(DisplayBinding, DISPLAY_DEFAULT);

    SO_NODE_SET_SF_ENUM_TYPE(displayStyle, DisplayBinding);

    SO_NODE_DEFINE_ENUM_VALUE(LabelLeftRightJustificationBinding,
                              LABEL_LR_LEFT);
    SO_NODE_DEFINE_ENUM_VALUE(LabelLeftRightJustificationBinding,
                              LABEL_LR_RIGHT);
    SO_NODE_DEFINE_ENUM_VALUE(LabelLeftRightJustificationBinding,
                              LABEL_LR_CENTER);
    SO_NODE_DEFINE_ENUM_VALUE(LabelLeftRightJustificationBinding,
                              LABEL_LR_DEFAULT);

    SO_NODE_DEFINE_ENUM_VALUE(LabelTopBottomJustificationBinding,
                              LABEL_TB_TOP);
    SO_NODE_DEFINE_ENUM_VALUE(LabelTopBottomJustificationBinding,
                              LABEL_TB_BOTTOM);
    SO_NODE_DEFINE_ENUM_VALUE(LabelTopBottomJustificationBinding,
                              LABEL_TB_MIDDLE);
    SO_NODE_DEFINE_ENUM_VALUE(LabelTopBottomJustificationBinding,
                              LABEL_TB_DEFAULT);

    SO_NODE_SET_SF_ENUM_TYPE(atomLabelLeftRightJustification, 
                             LabelLeftRightJustificationBinding);
    SO_NODE_SET_SF_ENUM_TYPE(atomLabelTopBottomJustification, 
                             LabelTopBottomJustificationBinding);

    SO_NODE_SET_SF_ENUM_TYPE(bondLabelLeftRightJustification, 
                             LabelLeftRightJustificationBinding);
    SO_NODE_SET_SF_ENUM_TYPE(bondLabelTopBottomJustification, 
                             LabelTopBottomJustificationBinding);

    SO_NODE_DEFINE_ENUM_VALUE(AtomSphereBinding, ATOMSPHERE_HEMISPHERES);
    SO_NODE_DEFINE_ENUM_VALUE(AtomSphereBinding, ATOMSPHERE_FULLSPHERES);
// --> EGB && SGB
		SO_NODE_DEFINE_ENUM_VALUE(AtomSphereBinding, ATOMSPHERE_ARRAY_HEMISPHERES);
		SO_NODE_DEFINE_ENUM_VALUE(AtomSphereBinding, ATOMSPHERE_ARRAY_FULLSPHERES);
		SO_NODE_DEFINE_ENUM_VALUE(AtomSphereBinding, ATOMSPHERE_GLUSPHERE);
		SO_NODE_DEFINE_ENUM_VALUE(AtomSphereBinding, ATOMSPHERE_BILLBOARD);
		SO_NODE_DEFINE_ENUM_VALUE(AtomSphereBinding, ATOMSPHERE_SOSPHERE);
		SO_NODE_DEFINE_ENUM_VALUE(AtomSphereBinding, ATOMSPHERE_LOD);
		SO_NODE_DEFINE_ENUM_VALUE(AtomSphereBinding, ATOMSPHERE_LOD_GEOMETRY);
// <-- EGB && SGB

    SO_NODE_SET_SF_ENUM_TYPE(atomSphereDisplayStyle, AtomSphereBinding);

    SO_NODE_DEFINE_ENUM_VALUE(WireframeAntiAliasBinding,
        WIREFRAME_ANTIALIAS_NONE);
    SO_NODE_DEFINE_ENUM_VALUE(WireframeAntiAliasBinding,
        WIREFRAME_ANTIALIAS_WITH_DEPTH_COMPARISON);
    SO_NODE_DEFINE_ENUM_VALUE(WireframeAntiAliasBinding,
        WIREFRAME_ANTIALIAS_NO_DEPTH_COMPARISON);

    SO_NODE_SET_SF_ENUM_TYPE(bondWireframeAntiAlias, WireframeAntiAliasBinding);

    SO_NODE_DEFINE_ENUM_VALUE(BondCylinderBinding, BONDCYLINDER_NOCAP);
    SO_NODE_DEFINE_ENUM_VALUE(BondCylinderBinding, BONDCYLINDER_FLATCAP);
    SO_NODE_DEFINE_ENUM_VALUE(BondCylinderBinding, BONDCYLINDER_ROUNDCAP);
// --> EGB && SGB
		SO_NODE_DEFINE_ENUM_VALUE(BondCylinderBinding, BONDCYLINDER_ARRAY_NOCAP);
    SO_NODE_DEFINE_ENUM_VALUE(BondCylinderBinding, BONDCYLINDER_ARRAY_FLATCAP);
    SO_NODE_DEFINE_ENUM_VALUE(BondCylinderBinding, BONDCYLINDER_ARRAY_ROUNDCAP);
		SO_NODE_DEFINE_ENUM_VALUE(BondCylinderBinding, BONDCYLINDER_GLUCYLINDER_NOCAP);
    SO_NODE_DEFINE_ENUM_VALUE(BondCylinderBinding, BONDCYLINDER_GLUCYLINDER_FLATCAP);
    SO_NODE_DEFINE_ENUM_VALUE(BondCylinderBinding, BONDCYLINDER_GLUCYLINDER_ROUNDCAP);
		SO_NODE_DEFINE_ENUM_VALUE(BondCylinderBinding, BONDCYLINDER_SOCYLINDER_NOCAP);
    SO_NODE_DEFINE_ENUM_VALUE(BondCylinderBinding, BONDCYLINDER_SOCYLINDER_FLATCAP);
    SO_NODE_DEFINE_ENUM_VALUE(BondCylinderBinding, BONDCYLINDER_SOCYLINDER_ROUNDCAP);
		SO_NODE_DEFINE_ENUM_VALUE(BondCylinderBinding, BONDCYLINDER_SEMI_NOCAP);
    SO_NODE_DEFINE_ENUM_VALUE(BondCylinderBinding, BONDCYLINDER_SEMI_FLATCAP);
    SO_NODE_DEFINE_ENUM_VALUE(BondCylinderBinding, BONDCYLINDER_SEMI_ROUNDCAP);
		SO_NODE_DEFINE_ENUM_VALUE(BondCylinderBinding, BONDCYLINDER_LOD_NOCAP);
    SO_NODE_DEFINE_ENUM_VALUE(BondCylinderBinding, BONDCYLINDER_LOD_FLATCAP);
    SO_NODE_DEFINE_ENUM_VALUE(BondCylinderBinding, BONDCYLINDER_LOD_ROUNDCAP);
// <-- EGB && SGB

    SO_NODE_SET_SF_ENUM_TYPE(bondCylinderDisplayStyle, BondCylinderBinding);

// --> CAstick
		SO_NODE_DEFINE_ENUM_VALUE(DisplayResiduesBinding, DISPLAY_RESIDUES_NONE);
    SO_NODE_DEFINE_ENUM_VALUE(DisplayResiduesBinding, DISPLAY_RESIDUES_CAWIRE);
		SO_NODE_DEFINE_ENUM_VALUE(DisplayResiduesBinding, DISPLAY_RESIDUES_CASTICK);
		SO_NODE_DEFINE_ENUM_VALUE(DisplayResiduesBinding, DISPLAY_RESIDUES_LINERIBBON);
		SO_NODE_DEFINE_ENUM_VALUE(DisplayResiduesBinding, DISPLAY_RESIDUES_FLATRIBBON);
		SO_NODE_DEFINE_ENUM_VALUE(DisplayResiduesBinding, DISPLAY_RESIDUES_SOLIDRIBBON);
		SO_NODE_DEFINE_ENUM_VALUE(DisplayResiduesBinding, DISPLAY_RESIDUES_SCHEMATIC);
				
		SO_NODE_SET_SF_ENUM_TYPE(residueDisplayStyle, DisplayResiduesBinding);
		SO_NODE_SET_SF_ENUM_TYPE(residueCylinderDisplayStyle,BondCylinderBinding);
// <-- CAstick

    SO_NODE_DEFINE_ENUM_VALUE(HighlightBinding, HIGHLIGHT_NONE);
    SO_NODE_DEFINE_ENUM_VALUE(HighlightBinding, HIGHLIGHT_EMISSIVE);
    SO_NODE_DEFINE_ENUM_VALUE(HighlightBinding, HIGHLIGHT_DIFFUSE);
    SO_NODE_DEFINE_ENUM_VALUE(HighlightBinding, HIGHLIGHT_DRAWSTYLE);
    SO_NODE_DEFINE_ENUM_VALUE(HighlightBinding, HIGHLIGHT_DEFAULT);

    SO_NODE_SET_SF_ENUM_TYPE(highlightStyle, HighlightBinding);

    SO_NODE_DEFINE_ENUM_VALUE(HighlightAtomBinding, HIGHLIGHT_ATOM_NONE);
    SO_NODE_DEFINE_ENUM_VALUE(HighlightAtomBinding, HIGHLIGHT_ATOM_BITMAP);
    SO_NODE_DEFINE_ENUM_VALUE(HighlightAtomBinding, HIGHLIGHT_ATOM_BBOX);
    SO_NODE_DEFINE_ENUM_VALUE(HighlightAtomBinding, HIGHLIGHT_ATOM_DEFAULT);

    SO_NODE_SET_SF_ENUM_TYPE(highlightAtomStyle, HighlightAtomBinding);

    SO_NODE_DEFINE_ENUM_VALUE(NonBondedAtomBinding, NONBONDED_ATOM_NONE);
    SO_NODE_DEFINE_ENUM_VALUE(NonBondedAtomBinding, NONBONDED_ATOM_BITMAP);
    SO_NODE_DEFINE_ENUM_VALUE(NonBondedAtomBinding, NONBONDED_ATOM_DEFAULT);

    SO_NODE_SET_SF_ENUM_TYPE(nonBondedAtomStyle, NonBondedAtomBinding);

    SO_NODE_DEFINE_ENUM_VALUE(FogBinding, FOG_NONE);
    SO_NODE_DEFINE_ENUM_VALUE(FogBinding, FOG_HAZE);
    SO_NODE_DEFINE_ENUM_VALUE(FogBinding, FOG_FOG);
    SO_NODE_DEFINE_ENUM_VALUE(FogBinding, FOG_SMOKE);
    SO_NODE_DEFINE_ENUM_VALUE(FogBinding, FOG_DEFAULT);

    SO_NODE_SET_SF_ENUM_TYPE(fogType, FogBinding);

    isBuiltIn = TRUE;

// --> EGB && SGB
    SO_NODE_ADD_FIELD(clipAtoms, (TRUE));
	SO_NODE_ADD_FIELD(clipBonds, (TRUE));
// <-- EGB && SGB
// --> octree culling
	SO_NODE_ADD_FIELD(showOctree, (FALSE));
// <-- octree culling
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Destructor
//
// Use: private

ChemDisplayParam::~ChemDisplayParam()
//
////////////////////////////////////////////////////////////////////////
{
}

void ChemDisplayParam::setShowRings(SbBool showRings)
{
   mShowRings = showRings;
}

SbBool ChemDisplayParam::getShowRings() const
{
   return mShowRings;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Handles any action derived from SoAction.
//
// Use: extender

void
ChemDisplayParam::doAction(SoAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    SoState *state = action->getState();
#ifdef IV2_0
    SoFontNameElement::set(state, this, fontName.getValue());
    SoFontSizeElement::set(state, this, fontSize.getValue());
#else
#ifdef IV_STRICT
    SoFontNameElement::set(state, this, fontName.getValue());
    SoFontSizeElement::set(state, fontSize.getValue());
#else
    SoFontNameElement::set(state, this, fontName.getValue());
    SoFontSizeElement::set(state, this, fontSize.getValue());
#endif
#endif
    if (!action->isOfType(SoGetBoundingBoxAction::getClassTypeId())) {
        SoClipPlane *clip;
        for (int i = 0; i < clipPlane.getNum(); i++) {
            clip = (SoClipPlane *)clipPlane[i];
            if (clip != NULL) {
                if (!clip->on.isIgnored() && clip->on.getValue() == FALSE) {
                    continue;
                }
                if (!clip->plane.isIgnored()) {
                    SoClipPlaneElement::add(action->getState(), this, 
                        clip->plane.getValue());
                }
            }
        }
    }
#ifdef ORIG
    ChemDisplayParamElement::set(action->getState(), this, this);
#else
    if (!ChemOverrideElement::getChemDisplayParamOverride(state)) {
        if (isOverride()) {
            ChemOverrideElement::setChemDisplayParamOverride(state, TRUE);
        }
        ChemDisplayParamElement::set(state, this, this);
    }
#endif
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Does GL render action.
//
// Use: extender

void
ChemDisplayParam::GLRender(SoGLRenderAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    // Set up fog
    int type = fogType.getValue();
          
    if (type == FOG_NONE)
        glDisable(GL_FOG);
            
    else {
        float start = fogStart.getValue();
        float visibility = fogVisibility.getValue();
            
        // Check for start of 0, which is the default value - this
        // means that we should use the near plane of the current view
        // volume as the start.
        if (type == FOG_HAZE) {
            if (start == 0.0) {
                const SbViewVolume &vol =
                    SoViewVolumeElement::get(action->getState());
                start = vol.getNearDist();
            }
        }

        // Check for visibility of 0, which is the default value - this
        // means that we should use the far plane of the current view
        // volume as the visibility.
        if (visibility == 0.0) {
            const SbViewVolume &vol =
                SoViewVolumeElement::get(action->getState());
            visibility = vol.getNearDist() + vol.getDepth();
        }

        glEnable(GL_FOG);
        glFogfv(GL_FOG_COLOR, fogColor.getValue().getValue());

        switch (type) {

          case FOG_NONE:
            // Can't get here!
            break;

          case FOG_HAZE:
            // Set up linear ramp based on visibility
            glFogf(GL_FOG_MODE,  GL_LINEAR);
            glFogf(GL_FOG_START, start);
            glFogf(GL_FOG_END,   visibility);
            break;

          case FOG_FOG:
            glFogf(GL_FOG_DENSITY, fogDensity.getValue());
            glFogf(GL_FOG_MODE,    GL_EXP);
            glFogf(GL_FOG_DENSITY, computeFogDensity(visibility, FALSE));
            break;

          case FOG_SMOKE:
            glFogf(GL_FOG_DENSITY, fogDensity.getValue());
            glFogf(GL_FOG_MODE,    GL_EXP2);
            glFogf(GL_FOG_DENSITY, computeFogDensity(visibility, TRUE));
            break;
        }
    }
    ChemDisplayParam::doAction(action);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Handles get bounding box action.
//
// Use: extender

void
ChemDisplayParam::getBoundingBox(SoGetBoundingBoxAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    ChemDisplayParam::doAction(action);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Does callback action thing.
//
// Use: extender

void
ChemDisplayParam::callback(SoCallbackAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    ChemDisplayParam::doAction(action);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Does pick action...
//
// Use: extender

void
ChemDisplayParam::pick(SoPickAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    ChemDisplayParam::doAction(action);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Computes fog density based on visibility.
//
// Use: private
        
float
ChemDisplayParam::computeFogDensity(float visibility, SbBool squared)
//
////////////////////////////////////////////////////////////////////////
{
    //   
    // We want nearly total opacity at a distance of "visibility" from
    // the eye. The exponential GL fog function is:
    //
    //          f = e ** (-density * distance)
    //
    // (the exponent is squared in the SMOKE case)
    //
    // Since this function approaches 0 asymptotically, we have to
    // choose a reasonable cutoff point that approximates total
    // opacity. e ** -4 is about 0.018, so all we have to do is make
    // the exponent equal to -4 at a distance of "visibility".
    //
            
    if (squared)
        return (float)(2.0 / visibility);
            
    else
        return (float)(4.0 / visibility);
}

