/*
 * ChemDisplayParam.h
 *
 *	This file defines the ChemDisplayParam node class.
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

#ifndef  __CHEM_DISPLAY_PARAM_H__
#define  __CHEM_DISPLAY_PARAM_H__


#include "inv/fields/SoSFInt32.H"
#include "inv/fields/SoSFBool.H"
#include "inv/fields/SoSFColor.H"
#include "inv/fields/SoSFEnum.H"
#include "inv/fields/SoSFFloat.H"
#include "inv/fields/SoSFName.H"
#include "inv/fields/SoSFShort.H"
#include "inv/fields/SoSFString.H"
#include "inv/fields/SoSFUShort.H"
#include "inv/fields/SoMFFloat.H"
#include "inv/fields/SoMFNode.H"
#include "inv/nodes/SoSubNode.H"
// --> residue labels
#include "inv/fields/SoSFVec3f.H"
// <-- residue labels

#include "inv/ChemKit/MFInt8.H"
#include "inv/ChemKit/ChemkitBasic.H"

//////////////////////////////////////////////////////////////////////////////
//
//  Class: ChemDisplayParam
//
//  SoNode class that manages display parameters for chemical displays
//
//////////////////////////////////////////////////////////////////////////////

class CHEMKIT_DLL_API ChemDisplayParam : public SoNode {

    SO_NODE_HEADER(ChemDisplayParam);

  public:

    // Fields:
    SoSFEnum    displayStyle;
    SoSFBool    showHydrogens;

    // Atom Label Fields:
    SoSFBool    showAtomLabels;
    SoSFEnum    atomLabelLeftRightJustification;
    SoSFEnum    atomLabelTopBottomJustification;
    SoSFString  atomLabelString;

    // Atom Fields:
    SoSFFloat   atomPickRadius;
    SoSFFloat   atomRadiiScaleFactor;
    SoSFFloat   atomSphereComplexity;
    SoSFEnum    atomSphereDisplayStyle;

    // Bond Label Fields:
    SoSFBool    showBondLabels;
    SoSFEnum    bondLabelLeftRightJustification;
    SoSFEnum    bondLabelTopBottomJustification;
    SoSFString  bondLabelString;

    // Bond Fields:
    SoSFBool    showMultipleBonds;
    SoSFFloat   multipleBondSeparation;
    SoSFFloat   bondCylinderRadius;
    SoSFFloat   bondCylinderComplexity;
    SoSFEnum    bondCylinderDisplayStyle;

// --> CAstick
		// Residue Label Fields:
    SoSFBool    showResidueLabels;
    SoSFEnum    residueLabelLeftRightJustification;
    SoSFEnum    residueLabelTopBottomJustification;
    SoSFString  residueLabelString;
		SoSFVec3f   residueLabelOffset;
// <-- CAstick

    // Special Bond Fields:
    SoSFInt32   hydrogenBondStippleFactor;
    SoSFUShort  hydrogenBondStipplePattern;
    SoSFInt32   resonanceBondStippleFactor;
    SoSFUShort  resonanceBondStipplePattern;

    // Wireframe Fields:
    SoSFEnum    bondWireframeAntiAlias;
    SoSFFloat   bondWireframeLineWidth;

    // BallStick Fields:
    SoSFFloat   ballStickSphereScaleFactor;

    // Size of rings to search for
    SoSFInt32   maxRingSize;

    // Aromatic ring fields
    SoSFUShort  aromaticRingNumSides;
    SoSFColor   aromaticRingColor;
    SoSFFloat   aromaticRingScale;

    // Text Font Fields:
    SoSFName    fontName;
    SoSFFloat   fontSize;
    SoSFName    highlightFontName;
    SoSFFloat   highlightFontSize;

    // Highlight Fields:
    SoSFEnum    highlightStyle;
    SoSFColor   highlightColor; 
        // For lines in HIGHLIGHT_DRAWSTYLE mode
    SoSFInt32   highlightStippleFactor;
    SoSFUShort  highlightStipplePattern;
        // For highlighting atom selections when in DISPLAY_WIREFRAME or
        // DISPLAY_STICK modes.
    SoSFEnum    highlightAtomStyle;
    MFUInt8     highlightAtomBitmap;
    SoSFShort   highlightAtomBitmapWidth;
    SoSFShort   highlightAtomBitmapHeight;

    // NonBonded Atom Fields:
    SoSFEnum    nonBondedAtomStyle;
    MFUInt8     nonBondedAtomBitmap;
    SoSFShort   nonBondedAtomBitmapWidth;
    SoSFShort   nonBondedAtomBitmapHeight;

    // Type of fog, color of fog, density of fog, start of fog and 
    // visibility factor.  This is for depth-cueing.
    SoSFEnum    fogType;
    SoSFColor   fogColor;
    SoSFFloat   fogDensity;
    SoSFFloat   fogStart;
    SoSFFloat   fogVisibility;

    // Clipping planes
    SoMFNode    clipPlane;

// --> EGB && SGB
		SoSFBool clipAtoms;
		SoSFBool clipBonds;
// <-- EGB && SGB
// --> octree culling
		SoSFBool showOctree;
// <-- octree culling

    // Enum for how displayStyle can be set
    enum DisplayBinding {
        DISPLAY_NONE,
        DISPLAY_CPK,
        DISPLAY_STICK,
        DISPLAY_BALLSTICK,
        DISPLAY_WIREFRAME,
        DISPLAY_BALLWIRE,
        DISPLAY_DEFAULT = DISPLAY_WIREFRAME
    };

    // Enum for where labels are to be placed
    enum LabelLeftRightJustificationBinding {
        LABEL_LR_LEFT,
        LABEL_LR_RIGHT,
        LABEL_LR_CENTER,
        LABEL_LR_DEFAULT = LABEL_LR_LEFT
    };
    enum LabelTopBottomJustificationBinding {
        LABEL_TB_TOP,
        LABEL_TB_BOTTOM,
        LABEL_TB_MIDDLE,
        LABEL_TB_DEFAULT = LABEL_TB_BOTTOM
    };

    // Enum for how spheres can be drawn
    enum AtomSphereBinding {
        ATOMSPHERE_HEMISPHERES,
        ATOMSPHERE_FULLSPHERES,
// --> EGB && SGB
//        ATOMSPHERE_DEFAULT = ATOMSPHERE_HEMISPHERES,

				ATOMSPHERE_ARRAY_HEMISPHERES,
        ATOMSPHERE_ARRAY_FULLSPHERES,
				ATOMSPHERE_GLUSPHERE,
				ATOMSPHERE_BILLBOARD,
				ATOMSPHERE_SOSPHERE,
				ATOMSPHERE_LOD,
				ATOMSPHERE_LOD_GEOMETRY,
				ATOMSPHERE_DEFAULT = ATOMSPHERE_HEMISPHERES
// <-- EGB && SGB
    };

    // Enum for how to do antiAliasing (if any)
    enum WireframeAntiAliasBinding {
        WIREFRAME_ANTIALIAS_NONE,
        WIREFRAME_ANTIALIAS_WITH_DEPTH_COMPARISON,
        WIREFRAME_ANTIALIAS_NO_DEPTH_COMPARISON,
        WIREFRAME_ANTIALIAS_DEFAULT = WIREFRAME_ANTIALIAS_NONE
    };

    // Enum for how cylinders can be drawn
    enum BondCylinderBinding {
        BONDCYLINDER_NOCAP,
        BONDCYLINDER_FLATCAP,
        BONDCYLINDER_ROUNDCAP,
// --> EGB && SGB
//        BONDCYLINDER_DEFAULT = BONDCYLINDER_NOCAP,

				BONDCYLINDER_ARRAY_NOCAP,
        BONDCYLINDER_ARRAY_FLATCAP,
        BONDCYLINDER_ARRAY_ROUNDCAP,
				BONDCYLINDER_GLUCYLINDER_NOCAP,
        BONDCYLINDER_GLUCYLINDER_FLATCAP,
        BONDCYLINDER_GLUCYLINDER_ROUNDCAP,
				BONDCYLINDER_SOCYLINDER_NOCAP,
        BONDCYLINDER_SOCYLINDER_FLATCAP,
        BONDCYLINDER_SOCYLINDER_ROUNDCAP,
				BONDCYLINDER_SEMI_NOCAP,
        BONDCYLINDER_SEMI_FLATCAP,
        BONDCYLINDER_SEMI_ROUNDCAP,
				BONDCYLINDER_LOD_NOCAP,
        BONDCYLINDER_LOD_FLATCAP,
        BONDCYLINDER_LOD_ROUNDCAP,
				BONDCYLINDER_DEFAULT = BONDCYLINDER_NOCAP
// <-- EGB && SGB
    };

// --> CAstick
	SoSFEnum    residueDisplayStyle;

	SoSFFloat   residueCylinderRadius;
  SoSFFloat   residueCylinderComplexity;
	SoSFEnum    residueCylinderDisplayStyle;

	// Residue wireframe fields
	SoSFEnum    residueWireframeAntiAlias;
  SoSFFloat   residueWireframeLineWidth;

	// Line ribbon
	SoSFInt32 residueNumberOfThreads;

	//Residue ribbon width
	SoSFFloat residueTurnWidth;
	SoSFFloat residueCoilWidth;
	SoSFFloat residueSheetWidth;
	SoSFFloat residueHelixWidth;
	SoSFFloat residueHelixOffsetWidth;


	// Enum for how displayStyle can be set
  enum DisplayResiduesBinding 
	{
        DISPLAY_RESIDUES_NONE,
				DISPLAY_RESIDUES_CAWIRE,
        DISPLAY_RESIDUES_CASTICK,
				DISPLAY_RESIDUES_LINERIBBON,
				DISPLAY_RESIDUES_FLATRIBBON,
				DISPLAY_RESIDUES_SOLIDRIBBON,
				DISPLAY_RESIDUES_SCHEMATIC
	};

// <-- CAstick
// --> solid ribbon/schematic
	SoSFFloat residueHeigthFactor;
	SoSFBool residueInterpolateColor;
	SoSFBool residueSelectAtoms;
	SoSFFloat residueMaxDistance;

	SoSFBool solidRibbonSmoothNormals;
	
	SoSFFloat schematicCylinderComplexity;
	SoSFFloat schematicCylinderRadius;
	SoSFInt32 schematicArrowTesselationSegments;
	SoSFInt32 schematicTubeTesselationSegments;
	SoSFInt32 schematicTubeShapeSegments;
	SoSFFloat schematicTubeRadius;
	
	SoSFInt32 schematicCylinderMininumResidues;
	SoSFInt32 schematicArrowMininumResidues;

	SoSFInt32 ribbonHelixTesselationFactor; //deprecated
	SoSFInt32 flatRibbonTesselationSegments; //deprecated
	SoSFInt32 solidRibbonTesselationSegments; //deprecated

	// <-- solid ribbon/schematic

    // Enum for the type of highlighting
    enum HighlightBinding {
        HIGHLIGHT_NONE,
        HIGHLIGHT_EMISSIVE,
        HIGHLIGHT_DIFFUSE,
        HIGHLIGHT_DRAWSTYLE,
        HIGHLIGHT_DEFAULT = HIGHLIGHT_EMISSIVE
    };

    // Enum for how to highlight atoms when in DISPLAY_WIREFRAME or
    // DISPLAY_STICK modes.
    enum HighlightAtomBinding {
        HIGHLIGHT_ATOM_NONE,
        HIGHLIGHT_ATOM_BITMAP,
        HIGHLIGHT_ATOM_BBOX,
        HIGHLIGHT_ATOM_DEFAULT = HIGHLIGHT_ATOM_BITMAP
    };

    // Enum for how to display nonBonded atoms when in DISPLAY_WIREFRAME or
    // DISPLAY_STICK modes.
    enum NonBondedAtomBinding {
        NONBONDED_ATOM_NONE,
        NONBONDED_ATOM_BITMAP,
        NONBONDED_ATOM_DEFAULT = NONBONDED_ATOM_BITMAP
    };

    // Enum for how depth-cuing (fog) should be implemented
    enum FogBinding {
        FOG_NONE,
        FOG_HAZE,
        FOG_FOG,
        FOG_SMOKE,
        FOG_DEFAULT = FOG_NONE
    };

    // Constructor
    ChemDisplayParam();
    void setShowRings(SbBool showRings);
    SbBool getShowRings() const;

  SoEXTENDER public:
    virtual void    doAction(SoAction *action);
    virtual void    GLRender(SoGLRenderAction *action);
    virtual void    getBoundingBox(SoGetBoundingBoxAction *action);
    virtual void    callback(SoCallbackAction *action);
    virtual void    pick(SoPickAction *action);

  SoINTERNAL public:
    static void     initClass();

  protected:
    virtual ~ChemDisplayParam();
    SbBool  mShowRings;

  private:
    float           computeFogDensity(float visibility, SbBool squared);
    
};

#endif  /* !__CHEM_DISPLAY_PARAM_H__ */
