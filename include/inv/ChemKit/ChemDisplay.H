/*
 * ChemDisplay.h
 *
 *	This file defines the ChemDisplay node class.
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

#ifndef  __CHEM_DISPLAY_H__
#define  __CHEM_DISPLAY_H__

#include "inv/SbBox.H"
#include "inv/SbViewportRegion.H"

#include "inv/fields/SoSFBool.H"
#include "inv/fields/SoSFEnum.H"
#include "inv/fields/SoSFNode.H"
#include "inv/fields/SoMFBitMask.H"
#include "inv/fields/SoMFUInt32.H"

#include "inv/nodes/SoNonIndexedShape.H"

// --> schematic
#include "inv/fields/SoMFInt32.H"
// <-- schematic

// --> EGB && SGB
#include "inv/SbLinear.H"
// <-- EGB && SGB

#include "inv/ChemKit/MFVec2i.H"
#include "inv/ChemKit/ChemIntList.H"
#include "inv/ChemKit/ChemBaseData.H"
#include "inv/ChemKit/ChemBondBBox.H"
#include "inv/ChemKit/ChemDisplayParam.H"
#include "inv/ChemKit/ChemkitBasic.H"

class ChemAtomBBox;
class ChemLabelBBox;
class ChemStickBondBBox;
class ChemWireframeBondBBox;

class ChemRadii;
class ChemColor;
// --> lod
class ChemLOD;
// <-- lod
class ChemDisplay;
class ChemDisplayPathList;
class ChemBaseDataElement;
class ChemDisplayParamElement;
class ChemDisplaySelectionElement;
class ChemRadiiElement;

class ChemBitmapFontCache;
class ChemBondStuff;

class ChemUnitCylinder;
class ChemUnitSphere;
class SoClipPlaneElement;
class SoTextureCoordinateElement;
// --> CAstick
class ChemResidueStuff;
class ChemData;
// <-- CAstick

// --> schematic
class ChemSchematicChain;
// <-- schematic
// --> octree culling
class ChemOctreeNode;
// <-- octree culling
//////////////////////////////////////////////////////////////////////////////
//
//  Class: ChemDisplay
//
//  Performs the rendering of the atoms and bonds.
//
//////////////////////////////////////////////////////////////////////////////

#define CHEM_DISPLAY_USE_REST_OF_ATOMS -1
#define CHEM_DISPLAY_USE_REST_OF_BONDS -1
// --> Residue selection
#define CHEM_DISPLAY_USE_REST_OF_RESIDUES -1
// <-- Residue selection
// --> schematic
#define CHEM_DISPLAY_USE_REST_OF_SCHEMATICS -1
// <-- schematic


class CHEMKIT_DLL_API ChemDisplay : public SoNonIndexedShape {

    SO_NODE_HEADER(ChemDisplay);

  public:

    // Fields
    MFVec2i     atomIndex;      // which atoms to render
    MFVec2i     bondIndex;      // which bonds to render
    MFVec2i     atomLabelIndex; // which atom labels to render
    MFVec2i     bondLabelIndex; // which bond labels to render
// --> CAstick
		MFVec2i     residueIndex;		// which residues to render
		MFVec2i     residueLabelIndex;// which residue labels to render
// <-- CAstick
// --> schematic
    MFVec2i     schematicIndex;		// which residues to render
// <-- schematic

    // Constructor
    ChemDisplay();

    // Return the number of items being displayed.
    int32_t         getNumberOfAtoms(SoAction *action) const;
    int32_t         getNumberOfBonds(SoAction *action) const;
    int32_t         getNumberOfAtomLabels(SoAction *action) const;
    int32_t         getNumberOfBondLabels(SoAction *action) const;
// --> CAstick
		int32_t         getNumberOfResidues(SoAction *action) const;
    int32_t         getNumberOfResidueLabels(SoAction *action) const;
// <-- CAstick
// --> schematics
    int32_t         getNumberOfSchematics() const;
    void getResiduesForSchematicChain(MFVec2i* residues, int32_t* from, int32_t chain);
// <-- schematics
   

  SoEXTENDER public:
    // Implement actions
    virtual void    GLRender(SoGLRenderAction *action);
    virtual void    rayPick(SoRayPickAction *action);

  SoINTERNAL public:
    virtual void    notify(SoNotList *list);
    static void     initClass();

    // Return the individual bounding boxes of the items being displayed.
    void            getAtomBBoxes(SoAction *action, SbBool clipOnCenter,
                        ChemAtomBBox *&atomBBoxes);
    void            getBondBBoxes(SoAction *action, SbBool clipOnCenter,
                        ChemBondBBox *&bondBBoxes);
    void            getAtomLabelBBoxes(SoAction *action, SbBool clipOnCenter,
                        ChemLabelBBox *&atomLabelBBoxes);
    void            getBondLabelBBoxes(SoAction *action, SbBool clipOnCenter,
                        ChemLabelBBox *&bondLabelBBoxes);

// --> CAstick
		void            getResidueBBoxes(SoAction *action, SbBool clipOnCenter,
                        ChemBondBBox *&bondBBoxes);
    void            getResidueLabelBBoxes(SoAction *action, SbBool clipOnCenter,
                        ChemLabelBBox *&residueLabelBBoxes);
// <-- CAstick

    // Return various pieces of information regarding the direction of the
    // camera and the various matrices which influence the rendering of the
    // data.
    const SbVec3f    &getCurrentLineOfSight() const
        { return currentLineOfSight; }
    const SbRotation &getCurrentRotation() const 
        { return currentRotation; }
    const SbMatrix   &getCurrentModelMatrix() const
        { return currentModelMatrix; }
    const SbMatrix   &getCurrentMVPMatrix() const
        { return currentMVP; }

  protected:
    virtual void    generatePrimitives(SoAction *action);
    virtual void    computeBBox(SoAction *action, SbBox3f &box,
                                SbVec3f &center);

    virtual ~ChemDisplay();

  private:
    // Data number of atoms and bonds in the ChemBaseData-derived node
    int32_t      globalNumberOfAtoms;
    int32_t      globalNumberOfBonds;
    int32_t      localNumberOfAtoms;
    int32_t      localNumberOfBonds;
// --> CAstick
		int32_t      globalNumberOfResidues;
    int32_t      localNumberOfResidues;
// <-- CAstick
// --> schematic
		int32_t      globalNumberOfSchematics;
    int32_t      localNumberOfSchematics;
// <-- schematic

    // Data derived from the atom and bond indices
    ChemIntList  nonBondedAtoms;
    MFVec2i      normalNonBondedAtomIndex;
    MFVec2i      highlightNonBondedAtomIndex;

    // Which data to draw in the normal style
    MFVec2i      normalAtomIndex;
    MFVec2i      normalBondIndex;
    MFVec2i      normalAtomLabelIndex;
    MFVec2i      normalBondLabelIndex;
// --> CAstick
		MFVec2i     normalResidueIndex;
		MFVec2i     normalResidueLabelIndex;
// <-- CAstick
// --> schematic
    MFVec2i normalSchematicIndex;
// <-- schematic

    // Which data to draw in a highlighted style
    MFVec2i      highlightAtomIndex;
    MFVec2i      highlightBondIndex;
    MFVec2i      highlightAtomLabelIndex;
    MFVec2i      highlightBondLabelIndex;
// --> CAstick
		MFVec2i     highlightResidueIndex;
		MFVec2i     highlightResidueLabelIndex;
// <-- CAstick
// --> schematic
    MFVec2i highlightSchematicIndex;
// <-- schematic

    // Wireframe rendering lists
    SbIntList    singleNormal;
    SbIntList    singleHighlight;
    SbIntList    doubleNormal;
    SbIntList    doubleHighlight;
    SbIntList    tripleNormal;
    SbIntList    tripleHighlight;
    SbIntList    quadrupleNormal;
    SbIntList    quadrupleHighlight;
    SbIntList    resonanceNormal;
    SbIntList    resonanceHighlight;
    SbIntList    hbondNormal;
    SbIntList    hbondHighlight;
// --> CAstick
    SbIntList    residueNormal;
    SbIntList    residueHighlight;
// <-- CAstick

    // Bond attributes
    ChemBondStuff *bondAttributes;
// --> moiv doublebond error
	public:
		// global to local indexes
		int32_t *bondAttributesIndex;
	private:
// <-- moiv doublebond error

// --> CAstick
    // Residue attributes
    ChemResidueStuff *residueAttributes;
// <-- CAstick
// --> schematic
    ChemSchematicChain* schematicChains;
// <-- schematic

    // List of aromatic rings
    SbPList       aromaticRingList;

    // Copies of the elements we need to check
    ChemBaseDataElement  *lastChemBaseDataElement;
    ChemDisplayParamElement *lastChemDisplayParamElement;
    ChemRadiiElement     *lastChemRadiiElement;
    ChemDisplaySelectionElement *lastChemSelectionElement;

    // Cache for bounding box.  Used in computeBBox
    SbBox3f lastComputeBBoxBox;
    SbVec3f lastComputeBBoxCenter;

    // Font data
    ChemBitmapFontCache *normalFont;
    ChemBitmapFontCache *highlightFont;

    // Current rotation and line of sight as calculated in
    // calculateHemisphereRotation.  currentRotation is used to keep
    // hemispheres facing forward and currentLineOfSight is used as the
    // direction in which to translate labels so they are
    // always visible.
    SbRotation   currentRotation;
    SbVec3f      currentLineOfSight;
    SbMatrix     currentModelMatrix;
    SbMatrix     currentMVP;
    SbMatrix     currentViewMatrix;
    SbViewVolume currentViewVolume;
    SbViewportRegion currentVPR;

    // Need to keep track of changes in showHydrogens and showMultipleBonds
    SbBool      lastShowHydrogens;
    SbBool      lastShowMultipleBonds;
    SbBool      firstShowMultiple;

    // Flag for equality of nonBondedAtomBitmap and highlightAtomBitMap
    SbBool      bitmapsEqual;

    // The unit sphere data
    uint32_t    lastCacheContext;
    SbBool      lastSphereDoNormals;
    SbBool      lastSphereDoTextures;
    int         lastSphereDisplayStyle;
    float       lastSphereComplexity;
    ChemUnitSphere *unitSphere;
#ifdef DISPLAYLIST
    GLuint      unitSphereList;
#endif

    // The unit cylinder data
    SbBool      lastCylinderDoNormals;
    SbBool      lastCylinderDoTextures;
    float       lastCylinderComplexity;
    int         lastCylinderDisplayStyle;
    ChemUnitCylinder *unitCylinder;

// --> CAstick
		SoMFVec3f pl,pr; //ribbon points
		SoMFInt32 pIndex;//valid ribbon points

		SbBool      lastResidueCylinderDoNormals;
    SbBool      lastResidueCylinderDoTextures;
    float       lastResidueCylinderComplexity;
    int         lastResidueCylinderDisplayStyle;
		ChemUnitCylinder *residueUnitCylinder;
		
		void computeResidueAttributes(SoAction *action);
		void computeRibbonVectors(ChemData *chemData,ChemDisplayParam *cdp);
		
		void setupResidues(SoGLRenderAction *action,
                        SoState *&state,
                        ChemColor *&chemColor,
                        ChemDisplayParam *&cdp,
                        ChemBaseData *&chemData);

		void renderResiduesAsWireframe(SoGLRenderAction *action);
		void renderResiduesAsCylinders(SoGLRenderAction *action);
		void renderResiduesAsFlatRibbons(SoGLRenderAction *action);
		void renderResiduesAsSolidRibbons(SoGLRenderAction *action);
		
		void renderResiduesAsLineRibbons(SoGLRenderAction *action);
		void renderResidueLabels(SoGLRenderAction *action);
	
// --> schematic
		void computeBoundsForSchematic(SoAction *action);
		void computeSchematic(SoAction *action);
		void cleanSchematic();
		void setupSchematic(SoGLRenderAction *action,
                        SoState *&state,
                        ChemColor *&chemColor,
                        ChemDisplayParam *&cdp,
                        ChemBaseData *&chemData);

		void renderResiduesAsSchematic(SoGLRenderAction *action);
//		void normalSchematic(SoState *state);

		void normalSchematicOV(const MFVec2i &theSchematicIndex,
			SoState *state,
			const SbColor &chemColor,
			const ChemDisplayParam *cdp,
			const ChemData *chemData);

		void normalSchematicPST(const MFVec2i &theSchematicIndex,
			SoState *state,
			const ChemColor *chemColor,
			const ChemDisplayParam *cdp,
			const ChemData *chemData);

		void normalSchematicPE(const MFVec2i &theSchematicIndex,
			SoState *state,
			const ChemColor *chemColor,
			const ChemDisplayParam *cdp,
			const ChemData *chemData);

		void highlightSchematicOV(const MFVec2i &theSchematicIndex,
			SoState *state,
			const SbColor &chemColor,
			const ChemDisplayParam *cdp,
			const ChemData *chemData);

		void highlightSchematicPST(const MFVec2i &theSchematicIndex,
			SoState *state,
			const ChemColor *chemColor,
			const ChemDisplayParam *cdp,
			const ChemData *chemData);

		void highlightSchematicPE(const MFVec2i &theSchematicIndex,
			SoState *state,
			const ChemColor *chemColor,
			const ChemDisplayParam *cdp,
			const ChemData *chemData);
		
		void computeSchematicChains(ChemData* chemData, ChemDisplayParam* cdp);
		void computeSchematicCylinders(ChemData* chemData);
		void computeSchematicTubeControlPoints(ChemData* chemData);

		void computeSchematicArrowGeometry(int residue,bool ace = false, bool doNormals=true);
		void computeTubePath(const SbIntList &chain);
		void computeSchematicTubeGeometry(bool doNormals = TRUE);
				
		void renderSchematicArrow();
		void renderSchematicTube();

		void renderSchematicCylinders(const SbIntList& cylinders, SoState *state);
		void renderSchematicArrows(const SbIntList& arrows, SoState *state);
		void renderSchematicTubes(const SbIntList& tubes, SoState *state);
			
		void convertListToRange(SbIntList* list, MFVec2i* range);

		SbPList schematicCylinders;
		SbPList schematicArrows;
		SbPList schematicTubes;

      // KLS 6/21/06 Made public so accessible as per documentation
      public:
		SoMFInt32 schematicCylinderIndex;
		SoMFInt32 schematicArrowIndex;
		SoMFInt32 schematicTubeIndex;
      private:


		// vecpl and vecpr equivalent for schematic
		SoMFVec3f scpl,scpr;

		// bounds for schematic
		SoMFVec3f sctl,sctr, scbl, scbr;

		// schematic cylinder data
		// base points of cylinders
		SoMFVec3f scfrom;
		SoMFVec3f scto;
		ChemUnitCylinder *schematicUnitCylinder;

		// schematic arrow data
		// top/bottom left and right points of each arrow
		SoMFVec3f scrtl,scrtr, scrbl, scrbr;
		// up normals for top/bottom left and right points of each arrow
		SoMFVec3f scruntl,scruntr, scrunbl, scrunbr;
		// left normals for top/bottom left and right points of each arrow
		SoMFVec3f scrlntl,scrlntr, scrlnbl, scrlnbr;

		// tube  data
		SoMFVec3f scc;
		// vertices for each slice
		SoMFVec3f* scvtx;
		// normals for each slice
		SoMFVec3f* scnrm;
		// discrete path for a tube
		int32_t tubePathCnt;
		SoMFVec3f tubePath;
		// vertices for shape extruded
		SoMFVec3f scTubeVertices;

		// cached variables for avoiding redundant scene graph traversals
		SoSFFloat schematicCylinderComplexity;
		SoSFFloat schematicCylinderRadius;

		SoSFInt32 schematicArrowTesselationSegments;

		SoSFInt32 schematicTubeTesselationSegments;
		SoSFInt32 schematicTubeShapeSegments;
		SoSFFloat schematicTubeRadius;
		
		// global cnt used in all tesselations
		int schematicElementSegmentation;
// <-- schematic

		void normalResiduesAsWireframe(const MFVec2i &theResidueIndex,
			SoState *state,
			const ChemColor *chemColor,
			const ChemDisplayParam *cdp,
			const ChemData *chemData);

		void highlightResiduesAsWireframe(const MFVec2i &theResidueIndex,
			SoState *state,
			const ChemColor *chemColor,
			const ChemDisplayParam *cdp,
			const ChemData *chemData);

		void normalStickOV(const MFVec2i &theResidueIndex,
			SoState *state,
			const SbColor &chemColor,
			const ChemDisplayParam *cdp,
			const ChemData *chemData);

		void normalStickRPN(const MFVec2i &theResidueIndex,
			SoState *state,
			const ChemColor *chemColor,
			const ChemDisplayParam *cdp,
			const ChemData *chemData);

		void normalStickRPC(const MFVec2i &theResidueIndex,
			SoState *state,
			const ChemColor *chemColor,
			const ChemDisplayParam *cdp,
			const ChemData *chemData);

		void highlightStickOV(const MFVec2i &theResidueIndex,
			SoState *state,
			const ChemColor *chemColor,
			const ChemDisplayParam *cdp,
			const ChemData *chemData);

		void highlightStickRPN(const MFVec2i &theResidueIndex,
			SoState *state,
			const ChemColor *chemColor,
			const ChemDisplayParam *cdp,
			const ChemData *chemData);

		void highlightStickRPC(const MFVec2i &theResidueIndex,
			SoState *state,
			const ChemColor *chemColor,
			const ChemDisplayParam *cdp,
			const ChemData *chemData);

		// flat ribbon

		void normalFlatRibbonOV(const MFVec2i &theResidueIndex,
			SoState *state,
			const SbColor &chemColor,
			const ChemDisplayParam *cdp,
			const ChemData *chemData);

		void normalFlatRibbonRPN(const MFVec2i &theResidueIndex,
			SoState *state,
			const ChemColor *chemColor,
			const ChemDisplayParam *cdp,
			const ChemData *chemData);

		void normalFlatRibbonRPC(const MFVec2i &theResidueIndex,
			SoState *state,
			const ChemColor *chemColor,
			const ChemDisplayParam *cdp,
			const ChemData *chemData);

		void highlightFlatRibbonOV(const MFVec2i &theResidueIndex,
			SoState *state,
			const SbColor &chemColor,
			const ChemDisplayParam *cdp,
			const ChemData *chemData);

		void highlightFlatRibbonRPN(const MFVec2i &theResidueIndex,
			SoState *state,
			const ChemColor *chemColor,
			const ChemDisplayParam *cdp,
			const ChemData *chemData);

		void highlightFlatRibbonRPC(const MFVec2i &theResidueIndex,
			SoState *state,
			const ChemColor *chemColor,
			const ChemDisplayParam *cdp,
			const ChemData *chemData);

	  // solid ribbon
		
		void normalSolidRibbonOV(const MFVec2i &theResidueIndex,
			SoState *state,
			const SbColor &chemColor,
			const ChemDisplayParam *cdp,
			const ChemData *chemData);

		void normalSolidRibbonRPN(const MFVec2i &theResidueIndex,
			SoState *state,
			const ChemColor *chemColor,
			const ChemDisplayParam *cdp,
			const ChemData *chemData);

		void normalSolidRibbonRPC(const MFVec2i &theResidueIndex,
			SoState *state,
			const ChemColor *chemColor,
			const ChemDisplayParam *cdp,
			const ChemData *chemData);

		void highlightSolidRibbonOV(const MFVec2i &theResidueIndex,
			SoState *state,
			const SbColor &chemColor,
			const ChemDisplayParam *cdp,
			const ChemData *chemData);

		void highlightSolidRibbonRPN(const MFVec2i &theResidueIndex,
			SoState *state,
			const ChemColor *chemColor,
			const ChemDisplayParam *cdp,
			const ChemData *chemData);

		void highlightSolidRibbonRPC(const MFVec2i &theResidueIndex,
			SoState *state,
			const ChemColor *chemColor,
			const ChemDisplayParam *cdp,
			const ChemData *chemData);

		// line ribbon

		void normalLineRibbon(const MFVec2i &theResidueIndex,
			SoState *state,
			const ChemColor *chemColor,
			const ChemDisplayParam *cdp,
			const ChemData *chemData);

		void highlightLineRibbon(const MFVec2i &theResidueIndex,
			SoState *state,
			const ChemColor *chemColor,
			const ChemDisplayParam *cdp,
			const ChemData *chemData);
// <-- CAstick

// --> solid ribbon
		// bounds for solid ribbon
		SoMFVec3f tl,tr, bl, br;
		
		// geometry of line/flat/solid ribbon

		// Bezier basis for a curve
		SoMFFloat basisCoefficients;
		SoMFFloat* basisCoefficientsLOD;
		
		// line ribbon geometry for ONLY one residue
		SoMFVec3f* lr;

		// flat ribbon geometry and normals for ONLY one residue
		// left and right points of each flat ribbon
		SoMFVec3f fl, fr;
		// normals for left and right points of each flat ribbon
		SoMFVec3f ful, fur;

		// solid ribbon geometry and normals for ONLY one residue
		// top/bottom left and right points of each solid ribbon
		SoMFVec3f rtl,rtr, rbl, rbr;
		// up normals top/bottom left and right points of each solid ribbon
		SoMFVec3f runtl,runtr, runbl, runbr;
		// left normals top/bottom left and right points of each solid ribbon
		SoMFVec3f rlntl,rlntr, rlnbl, rlnbr;

		SbColor residueColor[3]; // i-1,i,i+1 colors
		SbBool residueInterpolateColor;

		double B(const int i, const int k, const float u, const float* knots);
		void computeBoundsForSolidRibbon(SoAction *action);
		void computeBasisCoefficients(const float t);
		void preComputeBasisCoefficients();
		
		void computeResidueColor(int residue,SoState* state);

		void computeLineRibbonGeometry(int residue);
		void computeFlatRibbonGeometry(int residue, bool computeNormals = true);
		void computeSolidRibbonGeometry(int residue, bool computeNormals = true);

		void renderLineRibbon(bool highlight = false);
		void renderFlatRibbon();
		void renderSolidRibbon();
		void renderSolidRibbonTop();
		void renderSolidRibbonBottom();
		void renderSolidRibbonLeft();
		void renderSolidRibbonRight();

		// cached variables for avoiding redundant scene graph traversals
		SoSFInt32 residueNumberOfThreads;
		SoSFBool solidRibbonSmoothNormals;
		int ribbonSegmentation;
// <-- solid ribbon

// --> residue labels
		void normalResidueLabelsOV(SoGLRenderAction *action, 
			const MFVec2i &theResidueLabelIndex);

		void highlightResidueLabelsOV(SoGLRenderAction *action, 
			const MFVec2i &theResidueLabelIndex);

		void normalResidueLabelsRPN(SoGLRenderAction *action, 
			const MFVec2i &theResidueLabelIndex);

		void highlightResidueLabelsRPN(SoGLRenderAction *action, 
			const MFVec2i &theResidueLabelIndex);

		void normalResidueLabelsRPC(SoGLRenderAction *action, 
			const MFVec2i &theResidueLabelIndex);

		void highlightResidueLabelsRPC(SoGLRenderAction *action, 
			const MFVec2i &theResidueLabelIndex);
// <-- residue labels

// --> residue box
void bBoxResiduesAsCylinders(SoState *state, ChemDisplayParam *cdp,
														 int32_t &count, SbBox3f &box, SbVec3f &center);
void bBoxResiduesAsRibbons(SoState *state, ChemDisplayParam *cdp,
														 int32_t &count, SbBox3f &box, SbVec3f &center);
void bBoxResiduesAsSchematic(SoState *state, ChemDisplayParam *cdp,
														 int32_t &count, SbBox3f &box, SbVec3f &center);
void bBoxResiduesNone(SoState *state, ChemDisplayParam *cdp,
							int32_t &count, SbBox3f &box,SbVec3f &center);
// <-- residue box


    // The unit ring data for drawing the circle in an aromatic ring
    int         lastAromaticRingNumSides;
    SbVec3f     *aromaticRingCoords;

    // Render functions
    void    setupWireframe(SoAction *action,
                           SoState *&state,
                           ChemColor *&chemColor,
                           ChemDisplayParam *&cdp,
                           ChemBaseData *&chemData);

    void    setupSpheres(SoGLRenderAction *action,
                         SoState *&state,
                         ChemColor *&chemColor,
                         ChemRadii *&chemRadii,
                         ChemDisplayParam *&cdp,
                         ChemBaseData *&chemData);

    void    setupCylinders(SoGLRenderAction *action,
                           SoState *&state,
                           ChemColor *&chemColor,
                           ChemDisplayParam *&cdp,
                           ChemBaseData *&chemData);

    void    renderAtomsAsSpheres(SoGLRenderAction *action,
                                 SbBool renderNormal,
                                 SbBool renderHighlight,
                                 const MFVec2i &normalIndex,
                                 const MFVec2i &highlightIndex);
    void    renderBondsAsCylinders(SoGLRenderAction *action);
    void    renderBondsAsWireframe(SoGLRenderAction *action);
    void    renderAtomLabelsWithWireframe(SoGLRenderAction *action);
    void    renderAtomLabelsWithCylinders(SoGLRenderAction *action);
    void    renderAtomLabelsWithSpheres(SoGLRenderAction *action);
    void    renderBondLabelsWithWireframe(SoGLRenderAction *action);
    void    renderBondLabelsWithCylinders(SoGLRenderAction *action);
    void    renderSingleAtoms(SoGLRenderAction *action);

    void    normalBondsAsWireframe(const MFVec2i &index,
                                   SoState *state,
                                   const ChemColor *chemColor,
                                   const ChemDisplayParam *cdp,
                                   const ChemBaseData *chemData);
    void    highlightBondsAsWireframe(const MFVec2i &index,
                                      SoState *state,
                                      const ChemColor *chemColor,
                                      const ChemDisplayParam *cdp,
                                      const ChemBaseData *chemData);

    void    normalBondLabelsWithWireframe(SoGLRenderAction *action,
                                          const MFVec2i &index);
    void    highlightBondLabelsWithWireframe(SoGLRenderAction *action,
                                             const MFVec2i &index);

    void    normalBondLabelsWithCylinders(SoGLRenderAction *action,
                                          const MFVec2i &index);
    void    highlightBondLabelsWithCylinders(SoGLRenderAction *action,
                                             const MFVec2i &index);


    // The following preprocessor-generated functions handle the various
    // ways to render the spheres.
    // RO ... RadiusOverall
    // RA ... RadiusPerAtom
    // RI ... RadiusPerAtomIndexed
    // CO ... ColorOverall
    // CA ... ColorPerAtom
    // CI ... ColorPerAtomIndexed

    void    normalSphereROCO(SoGLRenderAction *action,
                             const int displayStyle,
                             const SbBool doHydrogens,
                             const SbBool doHemi,
                             const float radScale,
                             const SbColor *atomColors,
                             const float *atomRadii,
                             const ChemBaseData *chemData,
                             const MFVec2i &index);
    void    normalSphereROCA(SoGLRenderAction *action,
                             const int displayStyle,
                             const SbBool doHydrogens,
                             const SbBool doHemi,
                             const float radScale,
                             const SbColor *atomColors,
                             const float *atomRadii,
                             const ChemBaseData *chemData,
                             const MFVec2i &index);
    void    normalSphereROCI(SoGLRenderAction *action,
                             const int displayStyle,
                             const SbBool doHydrogens,
                             const SbBool doHemi,
                             const float radScale,
                             const SbColor *atomColors,
                             const float *atomRadii,
                             const ChemBaseData *chemData,
                             const MFVec2i &index);
    void    normalSphereRACO(SoGLRenderAction *action,
                             const int displayStyle,
                             const SbBool doHydrogens,
                             const SbBool doHemi,
                             const float radScale,
                             const SbColor *atomColors,
                             const float *atomRadii,
                             const ChemBaseData *chemData,
                             const MFVec2i &index);
    void    normalSphereRACA(SoGLRenderAction *action,
                             const int displayStyle,
                             const SbBool doHydrogens,
                             const SbBool doHemi,
                             const float radScale,
                             const SbColor *atomColors,
                             const float *atomRadii,
                             const ChemBaseData *chemData,
                             const MFVec2i &index);
    void    normalSphereRACI(SoGLRenderAction *action,
                             const int displayStyle,
                             const SbBool doHydrogens,
                             const SbBool doHemi,
                             const float radScale,
                             const SbColor *atomColors,
                             const float *atomRadii,
                             const ChemBaseData *chemData,
                             const MFVec2i &index);
    void    normalSphereRICO(SoGLRenderAction *action,
                             const int displayStyle,
                             const SbBool doHydrogens,
                             const SbBool doHemi,
                             const float radScale,
                             const SbColor *atomColors,
                             const float *atomRadii,
                             const ChemBaseData *chemData,
                             const MFVec2i &index);
    void    normalSphereRICA(SoGLRenderAction *action,
                             const int displayStyle,
                             const SbBool doHydrogens,
                             const SbBool doHemi,
                             const float radScale,
                             const SbColor *atomColors,
                             const float *atomRadii,
                             const ChemBaseData *chemData,
                             const MFVec2i &index);
    void    normalSphereRICI(SoGLRenderAction *action,
                             const int displayStyle,
                             const SbBool doHydrogens,
                             const SbBool doHemi,
                             const float radScale,
                             const SbColor *atomColors,
                             const float *atomRadii,
                             const ChemBaseData *chemData,
                             const MFVec2i &index);

    void    highlightSphereROCO(SoGLRenderAction *action,
                                const int displayStyle,
                                const int highlightStyle,
                                const SbColor &highlightColor,
                                const SbBool doHydrogens,
                                const SbBool doHemi,
                                const float radScale,
                                const SbColor *atomColors,
                                const float *atomRadii,
                                const ChemBaseData *chemData,
                                const MFVec2i &index);
    void    highlightSphereROCA(SoGLRenderAction *action,
                                const int displayStyle,
                                const int highlightStyle,
                                const SbColor &highlightColor,
                                const SbBool doHydrogens,
                                const SbBool doHemi,
                                const float radScale,
                                const SbColor *atomColors,
                                const float *atomRadii,
                                const ChemBaseData *chemData,
                                const MFVec2i &index);
    void    highlightSphereROCI(SoGLRenderAction *action,
                                const int displayStyle,
                                const int highlightStyle,
                                const SbColor &highlightColor,
                                const SbBool doHydrogens,
                                const SbBool doHemi,
                                const float radScale,
                                const SbColor *atomColors,
                                const float *atomRadii,
                                const ChemBaseData *chemData,
                                const MFVec2i &index);
    void    highlightSphereRACO(SoGLRenderAction *action,
                                const int displayStyle,
                                const int highlightStyle,
                                const SbColor &highlightColor,
                                const SbBool doHydrogens,
                                const SbBool doHemi,
                                const float radScale,
                                const SbColor *atomColors,
                                const float *atomRadii,
                                const ChemBaseData *chemData,
                                const MFVec2i &index);
    void    highlightSphereRACA(SoGLRenderAction *action,
                                const int displayStyle,
                                const int highlightStyle,
                                const SbColor &highlightColor,
                                const SbBool doHydrogens,
                                const SbBool doHemi,
                                const float radScale,
                                const SbColor *atomColors,
                                const float *atomRadii,
                                const ChemBaseData *chemData,
                                const MFVec2i &index);
    void    highlightSphereRACI(SoGLRenderAction *action,
                                const int displayStyle,
                                const int highlightStyle,
                                const SbColor &highlightColor,
                                const SbBool doHydrogens,
                                const SbBool doHemi,
                                const float radScale,
                                const SbColor *atomColors,
                                const float *atomRadii,
                                const ChemBaseData *chemData,
                                const MFVec2i &index);
    void    highlightSphereRICO(SoGLRenderAction *action,
                                const int displayStyle,
                                const int highlightStyle,
                                const SbColor &highlightColor,
                                const SbBool doHydrogens,
                                const SbBool doHemi,
                                const float radScale,
                                const SbColor *atomColors,
                                const float *atomRadii,
                                const ChemBaseData *chemData,
                                const MFVec2i &index);
    void    highlightSphereRICA(SoGLRenderAction *action,
                                const int displayStyle,
                                const int highlightStyle,
                                const SbColor &highlightColor,
                                const SbBool doHydrogens,
                                const SbBool doHemi,
                                const float radScale,
                                const SbColor *atomColors,
                                const float *atomRadii,
                                const ChemBaseData *chemData,
                                const MFVec2i &index);
    void    highlightSphereRICI(SoGLRenderAction *action,
                                const int displayStyle,
                                const int highlightStyle,
                                const SbColor &highlightColor,
                                const SbBool doHydrogens,
                                const SbBool doHemi,
                                const float radScale,
                                const SbColor *atomColors,
                                const float *atomRadii,
                                const ChemBaseData *chemData,
                                const MFVec2i &index);

    // The following preprocessor-generated functions handle the various
    // ways to render the atom labels.
    // W  ... Wireframe
    // C  ... Cylinder
    // S  ... Sphere
    // LO ... LabelOverall
    // LA ... LabelPerAtom
    // LI ... LabelPerAtomIndexed
    // AO ... AtomOverall
    // AA ... AtomPerAtom
    // AI ... AtomPerAtomIndexed
    // RO ... RadiusOverall
    // RA ... RadiusPerAtom
    // RI ... RadiusPerAtomIndexed

    void    normalAtomLabelsWLO(SoGLRenderAction *action,
                                const MFVec2i &index);
    void    normalAtomLabelsWLL(SoGLRenderAction *action,
                                const MFVec2i &index);
    void    normalAtomLabelsWLI(SoGLRenderAction *action,
                                const MFVec2i &index);
    void    normalAtomLabelsWAO(SoGLRenderAction *action,
                                const MFVec2i &index);
    void    normalAtomLabelsWAA(SoGLRenderAction *action,
                                const MFVec2i &index);
    void    normalAtomLabelsWAI(SoGLRenderAction *action,
                                const MFVec2i &index);
    void    normalAtomLabelsCLO(SoGLRenderAction *action,
                                const MFVec2i &index);
    void    normalAtomLabelsCLL(SoGLRenderAction *action,
                                const MFVec2i &index);
    void    normalAtomLabelsCLI(SoGLRenderAction *action,
                                const MFVec2i &index);
    void    normalAtomLabelsCAO(SoGLRenderAction *action,
                                const MFVec2i &index);
    void    normalAtomLabelsCAA(SoGLRenderAction *action,
                                const MFVec2i &index);
    void    normalAtomLabelsCAI(SoGLRenderAction *action,
                                const MFVec2i &index);
    void    normalAtomLabelsSROLO(SoGLRenderAction *action,
                                  const MFVec2i &index);
    void    normalAtomLabelsSROLL(SoGLRenderAction *action,
                                  const MFVec2i &index);
    void    normalAtomLabelsSROLI(SoGLRenderAction *action,
                                  const MFVec2i &index);
    void    normalAtomLabelsSROAO(SoGLRenderAction *action,
                                  const MFVec2i &index);
    void    normalAtomLabelsSROAA(SoGLRenderAction *action,
                                  const MFVec2i &index);
    void    normalAtomLabelsSROAI(SoGLRenderAction *action,
                                  const MFVec2i &index);
    void    normalAtomLabelsSRALO(SoGLRenderAction *action,
                                  const MFVec2i &index);
    void    normalAtomLabelsSRALL(SoGLRenderAction *action,
                                  const MFVec2i &index);
    void    normalAtomLabelsSRALI(SoGLRenderAction *action,
                                  const MFVec2i &index);
    void    normalAtomLabelsSRAAO(SoGLRenderAction *action,
                                  const MFVec2i &index);
    void    normalAtomLabelsSRAAA(SoGLRenderAction *action,
                                  const MFVec2i &index);
    void    normalAtomLabelsSRAAI(SoGLRenderAction *action,
                                  const MFVec2i &index);
    void    normalAtomLabelsSRILO(SoGLRenderAction *action,
                                  const MFVec2i &index);
    void    normalAtomLabelsSRILL(SoGLRenderAction *action,
                                  const MFVec2i &index);
    void    normalAtomLabelsSRILI(SoGLRenderAction *action,
                                  const MFVec2i &index);
    void    normalAtomLabelsSRIAO(SoGLRenderAction *action,
                                  const MFVec2i &index);
    void    normalAtomLabelsSRIAA(SoGLRenderAction *action,
                                  const MFVec2i &index);
    void    normalAtomLabelsSRIAI(SoGLRenderAction *action,
                                  const MFVec2i &index);

    void    highlightAtomLabelsWLO(SoGLRenderAction *action,
                                   const MFVec2i &index);
    void    highlightAtomLabelsWLL(SoGLRenderAction *action,
                                   const MFVec2i &index);
    void    highlightAtomLabelsWLI(SoGLRenderAction *action,
                                   const MFVec2i &index);
    void    highlightAtomLabelsWAO(SoGLRenderAction *action,
                                   const MFVec2i &index);
    void    highlightAtomLabelsWAA(SoGLRenderAction *action,
                                   const MFVec2i &index);
    void    highlightAtomLabelsWAI(SoGLRenderAction *action,
                                   const MFVec2i &index);
    void    highlightAtomLabelsCLO(SoGLRenderAction *action,
                                   const MFVec2i &index);
    void    highlightAtomLabelsCLL(SoGLRenderAction *action,
                                   const MFVec2i &index);
    void    highlightAtomLabelsCLI(SoGLRenderAction *action,
                                   const MFVec2i &index);
    void    highlightAtomLabelsCAO(SoGLRenderAction *action,
                                   const MFVec2i &index);
    void    highlightAtomLabelsCAA(SoGLRenderAction *action,
                                   const MFVec2i &index);
    void    highlightAtomLabelsCAI(SoGLRenderAction *action,
                                   const MFVec2i &index);
    void    highlightAtomLabelsSROLO(SoGLRenderAction *action,
                                     const MFVec2i &index);
    void    highlightAtomLabelsSROLL(SoGLRenderAction *action,
                                     const MFVec2i &index);
    void    highlightAtomLabelsSROLI(SoGLRenderAction *action,
                                     const MFVec2i &index);
    void    highlightAtomLabelsSROAO(SoGLRenderAction *action,
                                     const MFVec2i &index);
    void    highlightAtomLabelsSROAA(SoGLRenderAction *action,
                                     const MFVec2i &index);
    void    highlightAtomLabelsSROAI(SoGLRenderAction *action,
                                     const MFVec2i &index);
    void    highlightAtomLabelsSRALO(SoGLRenderAction *action,
                                     const MFVec2i &index);
    void    highlightAtomLabelsSRALL(SoGLRenderAction *action,
                                     const MFVec2i &index);
    void    highlightAtomLabelsSRALI(SoGLRenderAction *action,
                                     const MFVec2i &index);
    void    highlightAtomLabelsSRAAO(SoGLRenderAction *action,
                                     const MFVec2i &index);
    void    highlightAtomLabelsSRAAA(SoGLRenderAction *action,
                                     const MFVec2i &index);
    void    highlightAtomLabelsSRAAI(SoGLRenderAction *action,
                                     const MFVec2i &index);
    void    highlightAtomLabelsSRILO(SoGLRenderAction *action,
                                     const MFVec2i &index);
    void    highlightAtomLabelsSRILL(SoGLRenderAction *action,
                                     const MFVec2i &index);
    void    highlightAtomLabelsSRILI(SoGLRenderAction *action,
                                     const MFVec2i &index);
    void    highlightAtomLabelsSRIAO(SoGLRenderAction *action,
                                     const MFVec2i &index);
    void    highlightAtomLabelsSRIAA(SoGLRenderAction *action,
                                     const MFVec2i &index);
    void    highlightAtomLabelsSRIAI(SoGLRenderAction *action,
                                     const MFVec2i &index);

    // The following preprocessor-generated functions handle the various
    // ways to render the cylinders.  These all have to do with how the
    // bonds are colored.
    // OV ... BondOverall or AtomOverall
    // BB ... BondPerBond
    // BI ... BondPerBondIndexed
    // BA ... BondPerAtom
    // BH ... BondPerAtomHalfBonded
    // AA ... AtomPerAtom
    // AI ... AtomPerAtomIndexed

    void    normalCylinderOV(const MFVec2i &index,
                             SoState *state,
                             const SbColor &chemColor,
                             const ChemDisplayParam *cdp,
                             const ChemBaseData *chemData);
    void    normalCylinderBB(const MFVec2i &index,
                             SoState *state,
                             const ChemColor *chemColor,
                             const ChemDisplayParam *cdp,
                             const ChemBaseData *chemData);
    void    normalCylinderBI(const MFVec2i &index,
                             SoState *state,
                             const ChemColor *chemColor,
                             const ChemDisplayParam *cdp,
                             const ChemBaseData *chemData);
// --> roundcap optimization
//    void    normalCylinderBAAA(const MFVec2i &index,
//                               SoState *state,
//                               const ChemColor *chemColor,
//                               const ChemDisplayParam *cdp,
//                               const ChemBaseData *chemData);
	    void    normalCylinderBAAA(const MFVec2i &index,
                               SoState *state,
                               const ChemColor *chemColor,
                               const ChemDisplayParam *cdp,
                               const ChemBaseData *chemData,
															 SoGLRenderAction* action);
// <-- roundcap optimization
// --> roundcap optimization
//    void    normalCylinderBAAI(const MFVec2i &index,
//                              SoState *state,
//                               const ChemColor *chemColor,
//                               const ChemDisplayParam *cdp,
//                               const ChemBaseData *chemData);
    void    normalCylinderBAAI(const MFVec2i &index,
                               SoState *state,
                               const ChemColor *chemColor,
                               const ChemDisplayParam *cdp,
                               const ChemBaseData *chemData,
															 SoGLRenderAction* action);
// <-- roundcap optimization
    void    normalCylinderBHAA(const MFVec2i &index,
                               SoState *state,
                               const ChemColor *chemColor,
                               const ChemDisplayParam *cdp,
                               const ChemBaseData *chemData);
    void    normalCylinderBHAI(const MFVec2i &index,
                               SoState *state,
                               const ChemColor *chemColor,
                               const ChemDisplayParam *cdp,
                               const ChemBaseData *chemData);

    void    highlightCylinderOV(const MFVec2i &index,
                                SoState *state,
                                const ChemColor *chemColor,
                                const ChemDisplayParam *cdp,
                                const ChemBaseData *chemData);
    void    highlightCylinderBB(const MFVec2i &index,
                                SoState *state,
                                const ChemColor *chemColor,
                                const ChemDisplayParam *cdp,
                                const ChemBaseData *chemData);
    void    highlightCylinderBI(const MFVec2i &index,
                                SoState *state,
                                const ChemColor *chemColor,
                                const ChemDisplayParam *cdp,
                                const ChemBaseData *chemData);
    void    highlightCylinderBAAA(const MFVec2i &index,
                                  SoState *state,
                                  const ChemColor *chemColor,
                                  const ChemDisplayParam *cdp,
                                  const ChemBaseData *chemData);
    void    highlightCylinderBAAI(const MFVec2i &index,
                                  SoState *state,
                                  const ChemColor *chemColor,
                                  const ChemDisplayParam *cdp,
                                  const ChemBaseData *chemData);
    void    highlightCylinderBHAA(const MFVec2i &index,
                                  SoState *state,
                                  const ChemColor *chemColor,
                                  const ChemDisplayParam *cdp,
                                  const ChemBaseData *chemData);
    void    highlightCylinderBHAI(const MFVec2i &index,
                                  SoState *state,
                                  const ChemColor *chemColor,
                                  const ChemDisplayParam *cdp,
                                  const ChemBaseData *chemData);

    // Raypicking functions
    void    pickAtomsAsSpheres(SoRayPickAction *action);
    void    pickAtomsWithinRadius(SoRayPickAction *action);
    void    pickBondsAsCylinders(SoRayPickAction *action);
    void    pickBondsAsWireframe(SoRayPickAction *action);
    void    pickAtomLabelsWithWireframe(SoRayPickAction *action);
    void    pickAtomLabelsWithCylinders(SoRayPickAction *action);
    void    pickAtomLabelsWithSpheres(SoRayPickAction *action);
    void    pickBondLabelsWithWireframe(SoRayPickAction *action);
    void    pickBondLabelsWithCylinders(SoRayPickAction *action);

// --> residue selection
		void pickResiduesAsWireframe(SoRayPickAction *action);
		void pickResiduesAsCylinders(SoRayPickAction *action);
		void pickResiduesAsFlatRibbons(SoRayPickAction *action);
		void pickResiduesAsSolidRibbons(SoRayPickAction *action);
		void pickResiduesLabels(SoRayPickAction *action);
// <-- residue selection
// --> schematic
    void pickSchematics(SoRayPickAction *action);
// <-- schematic

    // The following preprocessor-generated functions handle the various
    // ways to pick the spheres.
    // RO ... RadiusOverall
    // RA ... RadiusPerAtom
    // RI ... RadiusPerAtomIndexed

    void    pickSphereRO(SoRayPickAction *action);
    void    pickSphereRA(SoRayPickAction *action);
    void    pickSphereRI(SoRayPickAction *action);

    // The following preprocessor-generated functions handle the various
    // ways to pick the atom labels when using spheres.
    // RO ... RadiusOverall
    // RA ... RadiusPerAtom
    // RI ... RadiusPerAtomIndexed
    void    pickAtomLabelsWithSpheresRO(SoRayPickAction *action);
    void    pickAtomLabelsWithSpheresRA(SoRayPickAction *action);
    void    pickAtomLabelsWithSpheresRI(SoRayPickAction *action);

    // GeneratePrimitives functions
    virtual void    generateAtomsAsSpheres(SoAction *action);
    virtual void    generateBondsAsCylinders(SoAction *action);
    virtual void    generateBondsAsWireframe(SoAction *action);

    // The following preprocessor-generated functions handle the various
    // ways to generate the cylinders.  These all have to do with how the
    // bonds are colored.
    // OV ... BondOverall or AtomOverall
    // BB ... BondPerBond
    // BI ... BondPerBondIndexed
    // BA ... BondPerAtom
    // BH ... BondPerAtomHalfBonded
    // AA ... AtomPerAtom
    // AI ... AtomPerAtomIndexed

    void    generateCylinderOV(SoAction *action);
    void    generateCylinderBB(SoAction *action);
    void    generateCylinderBI(SoAction *action);
    void    generateCylinderBAAA(SoAction *action);
    void    generateCylinderBAAI(SoAction *action);
    void    generateCylinderBHAA(SoAction *action);
    void    generateCylinderBHAI(SoAction *action);

    // Output the primitives for cylinders
    void    spewCylinder(SoPrimitiveVertex &pv,
                SoAction *action,
                const SoTextureCoordinateElement *tce,
                const SbVec3f &from, const SbVec3f &to,
                const float radius, const SbBool genTexCoords,
                const ChemUnitCylinder *unitCylinder);
    void    spewCylinderRegular(SoPrimitiveVertex &pv,
                SoAction *action,
                const SoTextureCoordinateElement *tce,
                const SbMatrix &theVertexMatrix,
                const SbBool doCaps, const SbBool genTexCoords,
                const int32_t numSections, const int32_t numSides,
                const SbVec3f *cylNormals,
                const SbVec3f *cylVertices);
    void    spewCylinderRoundCap(SoPrimitiveVertex &pv,
                SoAction *action,
                const SoTextureCoordinateElement *tce,
                const SbMatrix &theVertexMatrix,
                const SbBool bottom,
                const SbBool genTexCoords,
                const int numRoundCapSections,
                const int numSides,
                const int cylinderBottomStartNorm,
                const int cylinderBottomStartVert,
                const SbVec3f *cylinderCapNormals,
                const SbVec3f *cylinderCapVertices);
    void    spewCylinderHalfBonded(SoPrimitiveVertex &pv,
                SoAction *action,
                const SoTextureCoordinateElement *tce,
                const SbVec3f &from, const SbVec3f &to,
                const float radius,
                const int32_t indexFrom,
                const int32_t indexTo,   
                const SbBool genTexCoords,
                const ChemUnitCylinder *unitCylinder);
    void    spewCylinderWithInterpolatedColor(SoPrimitiveVertex &pv,
                SoAction *action,
                const SoTextureCoordinateElement *tce,  
                const SbVec3f &from,
                const SbVec3f &to,
                const float radius,
                const int32_t indexFrom,
                const int32_t indexTo,
                const SbBool genTexCoords,
                const ChemUnitCylinder *unitCylinder);

    // ComputeBBox functions
    void    bBoxNone(SoState *state, ChemDisplayParam *cdp,
                     int32_t &count, SbBox3f &box,
                     SbVec3f &center);
    void    bBoxAtomsAsSpheres(SoState *state, ChemDisplayParam *cdp,
                               int32_t &count, SbBox3f &box,
                               SbVec3f &center);
    void    bBoxAtomsAsSpheresRO(SoState *state, ChemDisplayParam *cdp,
                                 int32_t &count, SbBox3f &box,
                                 SbVec3f &center);
    void    bBoxAtomsAsSpheresRA(SoState *state, ChemDisplayParam *cdp,
                                 int32_t &count, SbBox3f &box,
                                 SbVec3f &center);
    void    bBoxAtomsAsSpheresRI(SoState *state, ChemDisplayParam *cdp,
                                 int32_t &count, SbBox3f &box,
                                 SbVec3f &center);
    void    bBoxBondsAsCylinders(SoState *state, ChemDisplayParam *cdp,
                                 int32_t &count, SbBox3f &box,
                                 SbVec3f &center);
    void    bBoxBondsAsWireframe(SoState *state, ChemDisplayParam *cdp,
                                 int32_t &count, SbBox3f &box,
                                 SbVec3f &center);

    void    bBoxAtomLabelsWithWireframe(SoState *state,
                                        ChemDisplayParam *cdp,
                                        int32_t &count, SbBox3f &box,
                                        SbVec3f &center);
    void    bBoxAtomLabelsWithCylinders(SoState *state,
                                        ChemDisplayParam *cdp,
                                        int32_t &count, SbBox3f &box,
                                        SbVec3f &center);
    void    bBoxAtomLabelsWithSpheres(SoState *state,
                                      ChemDisplayParam *cdp,
                                      int32_t &count, SbBox3f &box,
                                      SbVec3f &center);
    void    bBoxAtomLabelsWithSpheresRO(SoState *state,
                                        ChemDisplayParam *cdp,
                                        int32_t &count, SbBox3f &box,
                                        SbVec3f &center);
    void    bBoxAtomLabelsWithSpheresRA(SoState *state,
                                        ChemDisplayParam *cdp,
                                        int32_t &count, SbBox3f &box,
                                        SbVec3f &center);
    void    bBoxAtomLabelsWithSpheresRI(SoState *state,
                                        ChemDisplayParam *cdp,
                                        int32_t &count, SbBox3f &box,
                                        SbVec3f &center);
    void    bBoxBondLabelsWithWireframe(SoState *state,
                                        ChemDisplayParam *cdp,
                                        int32_t &count, SbBox3f &box,
                                        SbVec3f &center);
    void    bBoxBondLabelsWithCylinders(SoState *state,
                                        ChemDisplayParam *cdp,
                                        int32_t &count, SbBox3f &box,
                                        SbVec3f &center);

    void    eachBBoxAtomsPickRadius(SoState *state,
                                    ChemDisplayParam *cdp,
                                    SbBool clipOnCenter,
                                    int32_t &count,
                                    ChemAtomBBox *atomBBoxes);
    void    eachBBoxAtomsAsSpheresRO(SoState *state,
                                     ChemDisplayParam *cdp,
                                     SbBool clipOnCenter,
                                     int32_t &count,
                                     ChemAtomBBox *atomBBoxes);
    void    eachBBoxAtomsAsSpheresRA(SoState *state,
                                     ChemDisplayParam *cdp,
                                     SbBool clipOnCenter,
                                     int32_t &count,
                                     ChemAtomBBox *atomBBoxes);
    void    eachBBoxAtomsAsSpheresRI(SoState *state,
                                     ChemDisplayParam *cdp,
                                     SbBool clipOnCenter,
                                     int32_t &count,
                                     ChemAtomBBox *atomBBoxes);

    void    eachBBoxBondsAsWireframe(SoState *state,
                                     ChemDisplayParam *cdp,
                                     SbBool clipOnCenter,
                                     int32_t &count,
                                     ChemWireframeBondBBox *bondBBoxes);
    void    eachBBoxBondsAsCylinders(SoState *state,
                                     ChemDisplayParam *cdp,
                                     SbBool clipOnCenter,
                                     int32_t &count,
                                     ChemStickBondBBox *bondBBoxes);

    void    eachBBoxAtomLabelsWithWireframe(SoState *state,
                                            ChemDisplayParam *cdp,
                                            SbBool clipOnCenter,
                                            int32_t &count,
                                            ChemLabelBBox *atomLabelBBoxes);
    void    eachBBoxAtomLabelsWithCylinders(SoState *state,
                                            ChemDisplayParam *cdp,
                                            SbBool clipOnCenter,
                                            int32_t &count,
                                            ChemLabelBBox *atomLabelBBoxes);
    void    eachBBoxAtomLabelsWithSpheresRO(SoState *state,
                                            ChemDisplayParam *cdp,
                                            SbBool clipOnCenter,
                                            int32_t &count,
                                            ChemLabelBBox *atomLabelBBoxes);
    void    eachBBoxAtomLabelsWithSpheresRA(SoState *state,
                                            ChemDisplayParam *cdp,
                                            SbBool clipOnCenter,
                                            int32_t &count,
                                            ChemLabelBBox *atomLabelBBoxes);
    void    eachBBoxAtomLabelsWithSpheresRI(SoState *state,
                                            ChemDisplayParam *cdp,
                                            SbBool clipOnCenter,
                                            int32_t &count,
                                            ChemLabelBBox *atomLabelBBoxes);
    void    eachBBoxBondLabelsWithWireframe(SoState *state,
                                            ChemDisplayParam *cdp,
                                            SbBool clipOnCenter,
                                            int32_t &count,
                                            ChemLabelBBox *bondLabelBBoxes);
    void    eachBBoxBondLabelsWithCylinders(SoState *state,
                                            ChemDisplayParam *cdp,
                                            SbBool clipOnCenter,
                                            int32_t &count,
                                            ChemLabelBBox *bondLabelBBoxes);
// --> EGB && SGB
//    void    highlightCylindersOverall(const ChemBaseData *chemData,
//                                      const ChemDisplayParam *cdp,
//                                      const MFVec2i &index,
//                                      int displayStyle,
//                                      SbBool doHydrogens,
//                                      SbBool doCaps,
//                                      float rad,
//                                      SbVec3f theCapScale);
    void    highlightCylindersOverall(const ChemBaseData *chemData,
                                      const ChemDisplayParam *cdp,
                                      const MFVec2i &index,
                                      int displayStyle,
                                      SbBool doHydrogens,
                                      SbBool doCaps,
                                      float rad,
                                      SbVec3f theCapScale,
																			SoState* state);
// <-- EGB && SGB
    void    highlightWireframeOverall(const ChemBaseData *chemData,
                                      const ChemDisplayParam *cdp,
                                      const MFVec2i &index,
                                      SbBool doHydrogens, 
                                      SbBool renderAsPoints);

    void    calculateHemisphereRotation(SoState *state);

    void    generateIndices(SoAction *action);
// --> schematics
// // --> CAstick
//    //void    resetIndices(int32_t numberOfAtoms, int32_t numberOfBonds);
//		void    resetIndices(int32_t numberOfAtoms, int32_t numberOfBonds, int32_t numberOfResidues);
//// <-- CAstick
    void    resetIndices(int32_t numberOfAtoms, int32_t numberOfBonds, int32_t numberOfResidues, int32_t numSchematics);
// <-- schematics
		void sortIndices(MFVec2i &listIndex, MFVec2i &newIndex, int32_t end_indicator, int32_t numberOfItems);

    void    removeIndices(MFVec2i &listIndex, MFVec2i &newIndex,
                          int32_t end_indicator, int32_t numberOfItems);

    void    generateDoubleBondList(ChemBaseData *chemData,
                                   ChemDisplayParam *cdp,
                                   int32_t localNumberOfBonds,
                                   ChemIntList *atch,
                                   ChemBondStuff *bondAttributes);
    void    generateHigherBondList(ChemBaseData *chemData,
                                   int32_t localNumberOfBonds,
                                   ChemIntList *atch,
                                   ChemBondStuff *bondAttributes);

    void    normalSixRing(SoState *state, const ChemColor *chemColor,
                          const ChemDisplayParam *cdp,
                          const ChemBaseData *chemData);
    void    generateHBonds(SoAction *action, SoState *state,
                           const ChemColor *chemColor,
                           const ChemDisplayParam *cdp,
                           const ChemBaseData *chemData);
    void    generateSixRing(SoAction *action, SoState *state,
                            const ChemColor *chemColor,
                            const ChemDisplayParam *cdp,
                            const ChemBaseData *chemData);
    void    highlightAtomsWithWireframe(SoGLRenderAction *action);
    void    highlightAtomsWithCylinders(SoGLRenderAction *action);
// --> EGB && SGB
		void		beginSpheres(const int displayStyle,ChemUnitSphere *unitSphere = NULL);
		void		endSpheres(const int displayStyle);
		void		beginCylinders(const int displayStyle,ChemUnitCylinder *unitCylinder = NULL);
		void		endCylinders(const int displayStyle);


// --> LOD && clipping
public:
		const ChemLOD* getLOD() const {return lodSelector;}
// --> octree culling
		const ChemResidueStuff* getResidueAttributes() const {return residueAttributes;}
// <-- octree culling
private:	
		SbBool clipAtoms;
		SbBool clipBonds;

	ChemLOD *lodSelector;
// --> octree culling
	ChemOctreeNode* octreenode;
	MFVec2i vnormalAtomIndex,vhighlightAtomIndex;
	MFVec2i vnormalNonBondedAtomIndex,vhighlightNonBondedAtomIndex;
	MFVec2i vnormalBondIndex,vhighlightBondIndex;
	MFVec2i vnormalResidueIndex,vhighlightResidueIndex;

	MFVec2i vnormalAtomLabelIndex,vhighlightAtomLabelIndex;
	MFVec2i vnormalBondLabelIndex,vhighlightBondLabelIndex;
	MFVec2i vnormalResidueLabelIndex,vhighlightResidueLabelIndex;
	friend class ChemOctreeNode;

  // duplicates
  bool* renderedBonds;
  bool* renderedResidues;
// <-- octree culling

		float computeSphereProjectedArea(const SbVec3f &sphereCoords,
			const float &rad) const;

		int computeCylinderLOD(const SbVec3f &coordFrom,
			const SbVec3f &coordTo,
			const float &rad) const;

		bool pointInScreen(const SbVec3f &coord) const;

		SbVec3f currentABCD[4]; // billboard vertices
		int currentView[4];
		SbVec3f currentCameraPosition;

		// RI
		void computeLODgSphereRI(const SbBool doHydrogens,
					const float radScale, const float *atomRadii,
					const ChemBaseData *chemData, const MFVec2i &theAtomIndex);

		void computeLODfSphereRI(const SbBool doHydrogens,
					const float radScale, const float *atomRadii,
					const ChemBaseData *chemData, const MFVec2i &theAtomIndex);

		// RO
		void computeLODgSphereRO(const SbBool doHydrogens,
					const float radScale, const float *atomRadii,
					const ChemBaseData *chemData, const MFVec2i &theAtomIndex);

		void computeLODfSphereRO(const SbBool doHydrogens,
					const float radScale, const float *atomRadii,
					const ChemBaseData *chemData, const MFVec2i &theAtomIndex);

		// RA
		void computeLODgSphereRA(const SbBool doHydrogens,
					const float radScale, const float *atomRadii,
					const ChemBaseData *chemData, const MFVec2i &theAtomIndex);

		void computeLODfSphereRA(const SbBool doHydrogens,
					const float radScale, const float *atomRadii,
					const ChemBaseData *chemData, const MFVec2i &theAtomIndex);

		// CI
		void renderLODgNormalSphereCI(const SbColor *atomColors,
			const ChemBaseData *chemData);

		void renderLODfNormalSphereCI(const SbColor *atomColors,
			const ChemBaseData *chemData);

		void renderLODgHighlightSphereCI(const int highlightStyle,
			const SbColor &highlightColor,const SbColor *atomColors,
			const ChemBaseData *chemData);

		void renderLODfHighlightSphereCI(const int highlightStyle,
			const SbColor &highlightColor,const SbColor *atomColors,
			const ChemBaseData *chemData);


		// CA
		void renderLODgNormalSphereCA(const SbColor *atomColors,
			const ChemBaseData *chemData);

		void renderLODfNormalSphereCA(const SbColor *atomColors,
			const ChemBaseData *chemData);

		void renderLODgHighlightSphereCA(const int highlightStyle,
			const SbColor &highlightColor,const SbColor *atomColors,
			const ChemBaseData *chemData);

		void renderLODfHighlightSphereCA(const int highlightStyle,
			const SbColor &highlightColor,const SbColor *atomColors,
			const ChemBaseData *chemData);

		// CO
		void renderLODgNormalSphereCO(const SbColor *atomColors,
			const ChemBaseData *chemData);

		void renderLODfNormalSphereCO(const SbColor *atomColors,
			const ChemBaseData *chemData);

		void renderLODgHighlightSphereCO(const int highlightStyle,
			const SbColor &highlightColor,const SbColor *atomColors,
			const ChemBaseData *chemData);

		void renderLODfHighlightSphereCO(const int highlightStyle,
			const SbColor &highlightColor,const SbColor *atomColors,
			const ChemBaseData *chemData);

		// RICI
		void LODgNormalSphereRICI(const SbBool doHydrogens,
												const float radScale, const float *atomRadii,
												const SbColor *atomColors,
												const ChemBaseData *chemData, const MFVec2i &theAtomIndex);

		void LODgHighlightSphereRICI(const int highlightStyle,
			const SbColor &highlightColor,const SbBool doHydrogens,
			const float radScale, const float *atomRadii,
			const SbColor *atomColors,
			const ChemBaseData *chemData, const MFVec2i &theAtomIndex);

		void LODfNormalSphereRICI(const SbBool doHydrogens,
					const float radScale, const float *atomRadii,const SbColor *atomColors,
					const ChemBaseData *chemData, const MFVec2i &theAtomIndex);

		void LODfHighlightSphereRICI(const int highlightStyle,
			const SbColor &highlightColor,const SbBool doHydrogens,
			const float radScale, const float *atomRadii,
			const SbColor *atomColors,
			const ChemBaseData *chemData, const MFVec2i &theAtomIndex);

		void clipNormalSphereRICI(const int displayStyle, const SbBool doHydrogens,
												const SbBool doHemi, const float radScale, 
												const float *atomRadii, const SbColor *atomColors,
												const ChemBaseData *chemData, const MFVec2i &theAtomIndex,
												SoGLRenderAction *action);


		// RICA
		void LODgNormalSphereRICA(const SbBool doHydrogens,
												const float radScale, const float *atomRadii,
												const SbColor *atomColors,
												const ChemBaseData *chemData, const MFVec2i &theAtomIndex);

		void LODgHighlightSphereRICA(const int highlightStyle,
			const SbColor &highlightColor,const SbBool doHydrogens,
			const float radScale, const float *atomRadii,
			const SbColor *atomColors,
			const ChemBaseData *chemData, const MFVec2i &theAtomIndex);

		
		void LODfNormalSphereRICA(const SbBool doHydrogens,
					const float radScale, const float *atomRadii,const SbColor *atomColors,
					const ChemBaseData *chemData, const MFVec2i &theAtomIndex);

		void LODfHighlightSphereRICA(const int highlightStyle,
			const SbColor &highlightColor,const SbBool doHydrogens,
			const float radScale, const float *atomRadii,
			const SbColor *atomColors,
			const ChemBaseData *chemData, const MFVec2i &theAtomIndex);

		void clipNormalSphereRICA(const int displayStyle, const SbBool doHydrogens,
												const SbBool doHemi, const float radScale, 
												const float *atomRadii, const SbColor *atomColors,
												const ChemBaseData *chemData, const MFVec2i &theAtomIndex,
												SoGLRenderAction *action);

		// RICO
		void LODfNormalSphereRICO(const SbBool doHydrogens,
					const float radScale, const float *atomRadii,const SbColor *atomColors,
					const ChemBaseData *chemData, const MFVec2i &theAtomIndex);


		void LODgNormalSphereRICO(const SbBool doHydrogens,
												const float radScale, const float *atomRadii,
												const SbColor *atomColors,
												const ChemBaseData *chemData, const MFVec2i &theAtomIndex);

		void LODfHighlightSphereRICO(const int highlightStyle,
			const SbColor &highlightColor,const SbBool doHydrogens,
			const float radScale, const float *atomRadii,
			const SbColor *atomColors,
			const ChemBaseData *chemData, const MFVec2i &theAtomIndex);

		void LODgHighlightSphereRICO(const int highlightStyle,
			const SbColor &highlightColor,const SbBool doHydrogens,
			const float radScale, const float *atomRadii,
			const SbColor *atomColors,
			const ChemBaseData *chemData, const MFVec2i &theAtomIndex);

		void clipNormalSphereRICO(const int displayStyle, const SbBool doHydrogens,
												const SbBool doHemi, const float radScale, 
												const float *atomRadii, const SbColor *atomColors,
												const ChemBaseData *chemData, const MFVec2i &theAtomIndex,
												SoGLRenderAction *action);

		// RACI
		void LODfNormalSphereRACI(const SbBool doHydrogens,
					const float radScale, const float *atomRadii,const SbColor *atomColors,
					const ChemBaseData *chemData, const MFVec2i &theAtomIndex);


		void LODgNormalSphereRACI(const SbBool doHydrogens,
												const float radScale, const float *atomRadii,
												const SbColor *atomColors,
												const ChemBaseData *chemData, const MFVec2i &theAtomIndex);

		void LODfHighlightSphereRACI(const int highlightStyle,
			const SbColor &highlightColor,const SbBool doHydrogens,
			const float radScale, const float *atomRadii,
			const SbColor *atomColors,
			const ChemBaseData *chemData, const MFVec2i &theAtomIndex);

		void LODgHighlightSphereRACI(const int highlightStyle,
			const SbColor &highlightColor,const SbBool doHydrogens,
			const float radScale, const float *atomRadii,
			const SbColor *atomColors,
			const ChemBaseData *chemData, const MFVec2i &theAtomIndex);

		void clipNormalSphereRACI(const int displayStyle, const SbBool doHydrogens,
												const SbBool doHemi, const float radScale, 
												const float *atomRadii, const SbColor *atomColors,
												const ChemBaseData *chemData, const MFVec2i &theAtomIndex,
												SoGLRenderAction *action);

		// RACA
		void LODfNormalSphereRACA(const SbBool doHydrogens,
					const float radScale, const float *atomRadii,const SbColor *atomColors,
					const ChemBaseData *chemData, const MFVec2i &theAtomIndex);


		void LODgNormalSphereRACA(const SbBool doHydrogens,
												const float radScale, const float *atomRadii,
												const SbColor *atomColors,
												const ChemBaseData *chemData, const MFVec2i &theAtomIndex);

		void LODfHighlightSphereRACA(const int highlightStyle,
			const SbColor &highlightColor,const SbBool doHydrogens,
			const float radScale, const float *atomRadii,
			const SbColor *atomColors,
			const ChemBaseData *chemData, const MFVec2i &theAtomIndex);

		void LODgHighlightSphereRACA(const int highlightStyle,
			const SbColor &highlightColor,const SbBool doHydrogens,
			const float radScale, const float *atomRadii,
			const SbColor *atomColors,
			const ChemBaseData *chemData, const MFVec2i &theAtomIndex);

		void clipNormalSphereRACA(const int displayStyle, const SbBool doHydrogens,
												const SbBool doHemi, const float radScale, 
												const float *atomRadii, const SbColor *atomColors,
												const ChemBaseData *chemData, const MFVec2i &theAtomIndex,
												SoGLRenderAction *action);

		// RACO
		void LODfNormalSphereRACO(const SbBool doHydrogens,
					const float radScale, const float *atomRadii,const SbColor *atomColors,
					const ChemBaseData *chemData, const MFVec2i &theAtomIndex);


		void LODgNormalSphereRACO(const SbBool doHydrogens,
												const float radScale, const float *atomRadii,
												const SbColor *atomColors,
												const ChemBaseData *chemData, const MFVec2i &theAtomIndex);

		void LODfHighlightSphereRACO(const int highlightStyle,
			const SbColor &highlightColor,const SbBool doHydrogens,
			const float radScale, const float *atomRadii,
			const SbColor *atomColors,
			const ChemBaseData *chemData, const MFVec2i &theAtomIndex);

		void LODgHighlightSphereRACO(const int highlightStyle,
			const SbColor &highlightColor,const SbBool doHydrogens,
			const float radScale, const float *atomRadii,
			const SbColor *atomColors,
			const ChemBaseData *chemData, const MFVec2i &theAtomIndex);

		void clipNormalSphereRACO(const int displayStyle, const SbBool doHydrogens,
												const SbBool doHemi, const float radScale, 
												const float *atomRadii, const SbColor *atomColors,
												const ChemBaseData *chemData, const MFVec2i &theAtomIndex,
												SoGLRenderAction *action);

		// ROCI
		void LODfNormalSphereROCI(const SbBool doHydrogens,
					const float radScale, const float *atomRadii,const SbColor *atomColors,
					const ChemBaseData *chemData, const MFVec2i &theAtomIndex);


		void LODgNormalSphereROCI(const SbBool doHydrogens,
												const float radScale, const float *atomRadii,
												const SbColor *atomColors,
												const ChemBaseData *chemData, const MFVec2i &theAtomIndex);

		void LODfHighlightSphereROCI(const int highlightStyle,
			const SbColor &highlightColor,const SbBool doHydrogens,
			const float radScale, const float *atomRadii,
			const SbColor *atomColors,
			const ChemBaseData *chemData, const MFVec2i &theAtomIndex);

		void LODgHighlightSphereROCI(const int highlightStyle,
			const SbColor &highlightColor,const SbBool doHydrogens,
			const float radScale, const float *atomRadii,
			const SbColor *atomColors,
			const ChemBaseData *chemData, const MFVec2i &theAtomIndex);

		void clipNormalSphereROCI(const int displayStyle, const SbBool doHydrogens,
												const SbBool doHemi, const float radScale, 
												const float *atomRadii, const SbColor *atomColors,
												const ChemBaseData *chemData, const MFVec2i &theAtomIndex,
												SoGLRenderAction *action);

		// ROCA
		void LODfNormalSphereROCA(const SbBool doHydrogens,
					const float radScale, const float *atomRadii,const SbColor *atomColors,
					const ChemBaseData *chemData, const MFVec2i &theAtomIndex);


		void LODgNormalSphereROCA(const SbBool doHydrogens,
												const float radScale, const float *atomRadii,
												const SbColor *atomColors,
												const ChemBaseData *chemData, const MFVec2i &theAtomIndex);

		void LODfHighlightSphereROCA(const int highlightStyle,
			const SbColor &highlightColor,const SbBool doHydrogens,
			const float radScale, const float *atomRadii,
			const SbColor *atomColors,
			const ChemBaseData *chemData, const MFVec2i &theAtomIndex);

		void LODgHighlightSphereROCA(const int highlightStyle,
			const SbColor &highlightColor,const SbBool doHydrogens,
			const float radScale, const float *atomRadii,
			const SbColor *atomColors,
			const ChemBaseData *chemData, const MFVec2i &theAtomIndex);

		void clipNormalSphereROCA(const int displayStyle, const SbBool doHydrogens,
												const SbBool doHemi, const float radScale, 
												const float *atomRadii, const SbColor *atomColors,
												const ChemBaseData *chemData, const MFVec2i &theAtomIndex,
												SoGLRenderAction *action);

		// ROCO
		void LODfNormalSphereROCO(const SbBool doHydrogens,
					const float radScale, const float *atomRadii,const SbColor *atomColors,
					const ChemBaseData *chemData, const MFVec2i &theAtomIndex);


		void LODgNormalSphereROCO(const SbBool doHydrogens,
												const float radScale, const float *atomRadii,
												const SbColor *atomColors,
												const ChemBaseData *chemData, const MFVec2i &theAtomIndex);

		void LODfHighlightSphereROCO(const int highlightStyle,
			const SbColor &highlightColor,const SbBool doHydrogens,
			const float radScale, const float *atomRadii,
			const SbColor *atomColors,
			const ChemBaseData *chemData, const MFVec2i &theAtomIndex);

		void LODgHighlightSphereROCO(const int highlightStyle,
			const SbColor &highlightColor,const SbBool doHydrogens,
			const float radScale, const float *atomRadii,
			const SbColor *atomColors,
			const ChemBaseData *chemData, const MFVec2i &theAtomIndex);

		void clipNormalSphereROCO(const int displayStyle, const SbBool doHydrogens,
												const SbBool doHemi, const float radScale, 
												const float *atomRadii, const SbColor *atomColors,
												const ChemBaseData *chemData, const MFVec2i &theAtomIndex,
												SoGLRenderAction *action);

		// OVERALL
		void LODhighlightCylindersOverall(const ChemBaseData *chemData,const ChemDisplayParam *cdp, 
																	 const MFVec2i &theBondIndex,int displayStyle, 
																	 SbBool doHydrogens, SbBool doCaps,float rad,
																	 SbVec3f theCapScale, SoState* state);

		// OV
		void LODnormalCylinderOV(const MFVec2i &index,
											 SoState *state,
											 const SbColor &chemColor,
											 const ChemDisplayParam *cdp,
											 const ChemBaseData *chemData);

	void LODhighlightCylinderOV(const MFVec2i &theBondIndex, 
												SoState *state,
												const ChemColor *chemColor,
												const ChemDisplayParam *cdp,
												const ChemBaseData *chemData);

		void clipNormalCylinderOV(const MFVec2i &index,
											 SoState *state,
											 const SbColor &chemColor,
											 const ChemDisplayParam *cdp,
											 const ChemBaseData *chemData);
		// BB
    void LODnormalCylinderBB(const MFVec2i &index,
                             SoState *state,
                             const ChemColor *chemColor,
                             const ChemDisplayParam *cdp,
                             const ChemBaseData *chemData);
	
		void LODhighlightCylinderBB(const MFVec2i &theBondIndex, 
															SoState *state,
															const ChemColor *chemColor,
															const ChemDisplayParam *cdp,
															const ChemBaseData *chemData);

		void clipNormalCylinderBB(const MFVec2i &index,
                             SoState *state,
                             const ChemColor *chemColor,
                             const ChemDisplayParam *cdp,
                             const ChemBaseData *chemData);

		// BI
    void LODnormalCylinderBI(const MFVec2i &index,
                             SoState *state,
                             const ChemColor *chemColor,
                             const ChemDisplayParam *cdp,
                             const ChemBaseData *chemData);
	
		void LODhighlightCylinderBI(const MFVec2i &theBondIndex, 
															SoState *state,
															const ChemColor *chemColor,
															const ChemDisplayParam *cdp,
															const ChemBaseData *chemData);

		void clipNormalCylinderBI(const MFVec2i &index,
                             SoState *state,
                             const ChemColor *chemColor,
                             const ChemDisplayParam *cdp,
                             const ChemBaseData *chemData);
		// BHAI
		void LODnormalCylinderBHAI(const MFVec2i &index,
											 SoState *state,
											 const ChemColor *chemColor,
											 const ChemDisplayParam *cdp,
											 const ChemBaseData *chemData);

		void LODhighlightCylinderBHAI(const MFVec2i &theBondIndex, 
																	SoState *state,
																	const ChemColor *chemColor,
																	const ChemDisplayParam *cdp,
																	const ChemBaseData *chemData);

		void clipNormalCylinderBHAI(const MFVec2i &index,
											 SoState *state,
											 const ChemColor *chemColor,
											 const ChemDisplayParam *cdp,
											 const ChemBaseData *chemData);

		// BHAA
		void LODnormalCylinderBHAA(const MFVec2i &index,
											 SoState *state,
											 const ChemColor *chemColor,
											 const ChemDisplayParam *cdp,
											 const ChemBaseData *chemData);

		void LODhighlightCylinderBHAA(const MFVec2i &theBondIndex, 
																	SoState *state,
																	const ChemColor *chemColor,
																	const ChemDisplayParam *cdp,
																	const ChemBaseData *chemData);

		void clipNormalCylinderBHAA(const MFVec2i &index,
											 SoState *state,
											 const ChemColor *chemColor,
											 const ChemDisplayParam *cdp,
											 const ChemBaseData *chemData);

		// BAAI
		void LODnormalCylinderBAAI(const MFVec2i &index,
											 SoState *state,
											 const ChemColor *chemColor,
											 const ChemDisplayParam *cdp,
											 const ChemBaseData *chemData);

		void LODhighlightCylinderBAAI(const MFVec2i &theBondIndex, 
															SoState *state,
															const ChemColor *chemColor,
															const ChemDisplayParam *cdp,
															const ChemBaseData *chemData);

		void clipNormalCylinderBAAI(const MFVec2i &index,
											 SoState *state,
											 const ChemColor *chemColor,
											 const ChemDisplayParam *cdp,
											 const ChemBaseData *chemData);

		// BAAA
		void LODnormalCylinderBAAA(const MFVec2i &index,
											 SoState *state,
											 const ChemColor *chemColor,
											 const ChemDisplayParam *cdp,
											 const ChemBaseData *chemData);

		void LODhighlightCylinderBAAA(const MFVec2i &theBondIndex, 
															SoState *state,
															const ChemColor *chemColor,
															const ChemDisplayParam *cdp,
															const ChemBaseData *chemData);

		void clipNormalCylinderBAAA(const MFVec2i &index,
											 SoState *state,
											 const ChemColor *chemColor,
											 const ChemDisplayParam *cdp,
											 const ChemBaseData *chemData);

// <-- LOD && clipping
// --> atom, bond and residue labels individual culling
		void computeAtomLabelsCulling(SoGLRenderAction *action);
		void computeBondLabelsCulling(SoGLRenderAction *action);
		void computeResidueLabelsCulling(SoGLRenderAction *action);

		// residue individual culling
		// ribbon culling
		void computeRibbonCulling();
		void computeRibbonLOD(const int index, const SbPlane* planes);

		void computeStickCulling(const MFVec2i &mf, ChemDisplayParam* cdp, ChemData* chemData);

// <-- atom, bond and residue labels clipping
// <-- EGB && SGB
// --> residue selection
		void eachBBoxResiduesAsWireframe(SoState *state, ChemDisplayParam *cdp,
			SbBool clipOnCenter, int32_t &bCount, ChemWireframeBondBBox *bondBBoxes);

		void eachBBoxResiduesAsCylinders(SoState *state, ChemDisplayParam *cdp,
			SbBool clipOnCenter, int32_t &bCount, ChemStickBondBBox *bondBBoxes);
// <-- residue selection
// --> residue labels selection
		void eachBBoxResidueLabels(SoState *state, ChemDisplayParam *cdp,
			SbBool clipOnCenter, int32_t &bCount, ChemLabelBBox *labelBBoxes);
// <-- residue labels selection
};

#endif /* !__CHEM_DISPLAY_H__ */
