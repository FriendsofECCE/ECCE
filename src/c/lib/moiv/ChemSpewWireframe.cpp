/*
 * ChemSpewWireframe.c++
 *
 *     Classes:  ChemDisplay
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

#include <assert.h>
#include <ctype.h>
#include <math.h>

#include "inv/SoPrimitiveVertex.H"
#include "inv/actions/SoCallbackAction.H"
#include "inv/details/SoLineDetail.H"
#include "inv/details/SoPointDetail.H"
#include "inv/elements/SoGLDrawStyleElement.H"
#include "inv/elements/SoGLTextureCoordinateElement.H"
#include "inv/elements/SoGLTextureEnabledElement.H"
#include "inv/elements/SoGLTextureImageElement.H"

#ifdef IV2_0
#include "inv/bundles/SoMaterialBundle.H"
#else
#include "inv/elements/SoGLLazyElement.H"
#include "inv/elements/SoLazyElement.H"
#endif

#include "inv/ChemKit/ChemDetail.H"
// --> lod
#include "inv/ChemKit/ChemLOD.H"
// <-- lod
#include "inv/ChemKit/ChemDisplay.H"
#include "inv/ChemKit/ChemBaseDataElement.H"
#include "inv/ChemKit/ChemColorElement.H"
#include "inv/ChemKit/ChemDisplayParamElement.H"
#include "inv/ChemKit/ChemWireframeDetail.H"

#include "inv/ChemKit/ChemBondStuff.H"
#include "inv/ChemKit/ChemAromaticRing.H"

static void generateMulti(SoPrimitiveVertex *pvs, short &numPVS,
	SbVec3f coords1, SbVec3f coords2, int32_t theBond,
	float multiSep, float multiSep2,
	int32_t color1, int32_t color2, const ChemBondStuff *bondAttributes);

static void generateMultiHalfBonded(SoPrimitiveVertex *pvs, short &numPVS,
	SbVec3f coords1, SbVec3f coords2, int32_t theBond,
	float multiSep, float multiSep2, 
	int32_t color1, int32_t color2, const ChemBondStuff *bondAttributes);

//
// Start of Spew WIREFRAME section
//

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Generates lines representing the molecule.  Invoked by the
//	  generatePrimitives method.
//
// Use: protected

void
ChemDisplay::generateBondsAsWireframe(SoAction *action)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG
printf ("In ChemDisplay::generateBondsAsWireframe\n" );
#endif

	SoState *state;
	ChemColor *chemColor;
	ChemDisplayParam *cdp;
	ChemBaseData *chemData;
	
	setupWireframe(action, state, chemColor, cdp, chemData);

	state->push();

	int32_t			i, j;
	int32_t			numHBonds;
	short			numPVS;
	int32_t			from, to, theBond;
	SbVec3f 		coords1, coords2, tcoord;
	SoPrimitiveVertex	pvs[16];
	ChemWireframeDetail *detail;

	SbBool doHydrogens = cdp->showHydrogens.getValue();
	SbBool showMulti   = cdp->showMultipleBonds.getValue();
	float  multiSep    = cdp->multipleBondSeparation.getValue();
	float  multiSep2   = multiSep * 0.5;

	for (i = 0; i < 16; i++) {
		pvs[i].setTextureCoords(SbVec4f(0.0, 0.0, 0.0, 0.0));
		pvs[i].setNormal(SbVec3f(0.0, 0.0, 1.0));
	}

	numHBonds = 0;
	if (chemColor->bondColorBinding.getValue() == 
		ChemColor::BOND_OVERALL) {

		for (i = 0; i < localNumberOfBonds; i++) {
			// Skip hydrogen bonds as they will be dealt with later
			if (bondAttributes[i].type == ChemBaseData::HYDROGEN_BOND) {
				numHBonds++;
				continue;
			}

			theBond = bondAttributes[i].index;
			from = bondAttributes[i].from;
			to = bondAttributes[i].to;
	
			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}

			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			if (showMulti) {
				generateMulti(pvs, numPVS, coords1, coords2, i,
					multiSep, multiSep2, 0, 0, bondAttributes);

				for (j = 0; j < numPVS; j++) {
					invokeLineSegmentCallbacks(action,
						&pvs[j*2], &pvs[j*2+1]);
				}
			}
			else {
				// Create a detail for the specific shape
				detail = new ChemWireframeDetail;
				detail->setAtomBondIndex(-1, theBond);
				detail->setLineType(ChemWireframeDetail::SOLID);

				pvs[0].setDetail(detail);
				pvs[0].setMaterialIndex(0);
				pvs[0].setPoint(coords1.getValue());
				pvs[1].setDetail(detail);
				pvs[1].setMaterialIndex(0);
				pvs[1].setPoint(coords2.getValue());
				invokeLineSegmentCallbacks(action, &pvs[0], &pvs[1]);
			}
		}
	}
	else if (chemColor->bondColorBinding.getValue() == 
		ChemColor::BOND_PER_ATOM) {

		if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_OVERALL) {

			for (i = 0; i < localNumberOfBonds; i++) {
				// Skip hydrogen bonds as they will be dealt with later
				if (bondAttributes[i].type == ChemBaseData::HYDROGEN_BOND) {
					numHBonds++;
					continue;
				}

				theBond = bondAttributes[i].index;
				from = bondAttributes[i].from;
				to = bondAttributes[i].to;
	
				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}

				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				if (showMulti) {
					generateMulti(pvs, numPVS, coords1, coords2, i,
						multiSep, multiSep2, 0, 0, bondAttributes);

					for (j = 0; j < numPVS; j++) {
						invokeLineSegmentCallbacks(action,
							&pvs[j*2], &pvs[j*2+1]);
					}
				}
				else {
					// Create a detail for the specific shape
					detail = new ChemWireframeDetail;
					detail->setAtomBondIndex(-1, theBond);
					detail->setLineType(ChemWireframeDetail::SOLID);

					pvs[0].setDetail(detail);
					pvs[0].setMaterialIndex(0);
					pvs[0].setPoint(coords1.getValue());
					pvs[1].setDetail(detail);
					pvs[1].setMaterialIndex(0);
					pvs[1].setPoint(coords2.getValue());
					invokeLineSegmentCallbacks(action, &pvs[0], &pvs[1]);
				}
			}
		}
		else if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_PER_ATOM) {
			for (i = 0; i < localNumberOfBonds; i++) {
				// Skip hydrogen bonds as they will be dealt with later
				if (bondAttributes[i].type == ChemBaseData::HYDROGEN_BOND) {
					numHBonds++;
					continue;
				}

				theBond = bondAttributes[i].index;
				from = bondAttributes[i].from;
				to = bondAttributes[i].to;
	
				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}

				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				if (showMulti) {
					generateMulti(pvs, numPVS, coords1, coords2, i,
						multiSep, multiSep2, from, to, bondAttributes);

					for (j = 0; j < numPVS; j++) {
						invokeLineSegmentCallbacks(action,
							&pvs[j*2], &pvs[j*2+1]);
					}
				}
				else {
					// Create a detail for the specific shape
					detail = new ChemWireframeDetail;
					detail->setAtomBondIndex(-1, theBond);
					detail->setLineType(ChemWireframeDetail::SOLID);

					pvs[0].setDetail(detail);
					pvs[0].setMaterialIndex(from);
					pvs[0].setPoint(coords1.getValue());
					pvs[1].setDetail(detail);
					pvs[1].setMaterialIndex(to);
					pvs[1].setPoint(coords2.getValue());
					invokeLineSegmentCallbacks(action, &pvs[0], &pvs[1]);
				}
			}
		}
		else if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_PER_ATOM_INDEXED) {
			for (i = 0; i < localNumberOfBonds; i++) {
				// Skip hydrogen bonds as they will be dealt with later
				if (bondAttributes[i].type == ChemBaseData::HYDROGEN_BOND) {
					numHBonds++;
					continue;
				}

				theBond = bondAttributes[i].index;
				from = bondAttributes[i].from;
				to = bondAttributes[i].to;
	
				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}

				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				if (showMulti) {
					generateMulti(pvs, numPVS, coords1, coords2, i,
						multiSep, multiSep2,
						chemData->getAtomIndex(from),
						chemData->getAtomIndex(to),
						bondAttributes);

					for (j = 0; j < numPVS; j++) {
						invokeLineSegmentCallbacks(action,
							&pvs[j*2], &pvs[j*2+1]);
					}
				}
				else {
					// Create a detail for the specific shape
					detail = new ChemWireframeDetail;
					detail->setAtomBondIndex(-1, theBond);
					detail->setLineType(ChemWireframeDetail::SOLID);

					pvs[0].setDetail(detail);
					pvs[0].setMaterialIndex(chemData->getAtomIndex(from));
					pvs[0].setPoint(coords1.getValue());
					pvs[1].setDetail(detail);
					pvs[1].setMaterialIndex(chemData->getAtomIndex(to));
					pvs[1].setPoint(coords2.getValue());
					invokeLineSegmentCallbacks(action, &pvs[0], &pvs[1]);
				}
			}
		}
	}
	else if (chemColor->bondColorBinding.getValue() == 
		ChemColor::BOND_PER_ATOM_HALF_BONDED) {

		if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_OVERALL) {
			for (i = 0; i < localNumberOfBonds; i++) {
				// Skip hydrogen bonds as they will be dealt with later
				if (bondAttributes[i].type == ChemBaseData::HYDROGEN_BOND) {
					numHBonds++;
					continue;
				}

				theBond = bondAttributes[i].index;
				from = bondAttributes[i].from;
				to = bondAttributes[i].to;
		
				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();
	
				if (showMulti) {
					generateMulti(pvs, numPVS, coords1, coords2, i,
						multiSep, multiSep2, 0, 0, bondAttributes);

					for (j = 0; j < numPVS; j++) {
						invokeLineSegmentCallbacks(action,
							&pvs[j*2], &pvs[j*2+1]);
					}
				}
				else {
					// Create a detail for the specific shape
					detail = new ChemWireframeDetail;
					detail->setAtomBondIndex(-1, theBond);
					detail->setLineType(ChemWireframeDetail::SOLID);

					pvs[0].setDetail(detail);
					pvs[0].setMaterialIndex(0);
					pvs[0].setPoint(coords1.getValue());
					pvs[1].setDetail(detail);
					pvs[1].setMaterialIndex(0);
					pvs[1].setPoint(coords2.getValue());
					invokeLineSegmentCallbacks(action, &pvs[0], &pvs[1]);
				}
			}
		}
		else if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_PER_ATOM) {
			for (i = 0; i < localNumberOfBonds; i++) {
				// Skip hydrogen bonds as they will be dealt with later
				if (bondAttributes[i].type == ChemBaseData::HYDROGEN_BOND) {
					numHBonds++;
					continue;
				}

				theBond = bondAttributes[i].index;
				from = bondAttributes[i].from;
				to = bondAttributes[i].to;
		
				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();
				tcoord = (coords2 - coords1) * 0.5f + coords1;
	
				if (showMulti) {
					generateMultiHalfBonded(pvs, numPVS,
						coords1, coords2, i,
						multiSep, multiSep2, from, to, bondAttributes);

					// The point and materialIndex have been set in
					// generateMultiHalfBonded.  Fill in the detail
					// and invokeLineSegmentCallbacks.
					for (j = 0; j < numPVS; j++) {
						invokeLineSegmentCallbacks(action,
							&pvs[j*2], &pvs[j*2+1]);
					}
				}
				else {
					// Create a detail for the specific shape
					detail = new ChemWireframeDetail;
					detail->setAtomBondIndex(-1, theBond);
					detail->setLineType(ChemWireframeDetail::SOLID);

					pvs[0].setDetail(detail);
					pvs[1].setDetail(detail);

					pvs[0].setPoint(coords1.getValue());
					pvs[1].setPoint(tcoord.getValue());

					pvs[0].setMaterialIndex(from);
					pvs[1].setMaterialIndex(from);

					invokeLineSegmentCallbacks(action, &pvs[0], &pvs[1]);

					pvs[0].setPoint(tcoord.getValue());
					pvs[1].setPoint(coords2.getValue());

					pvs[0].setMaterialIndex(to);
					pvs[1].setMaterialIndex(to);

					invokeLineSegmentCallbacks(action, &pvs[0], &pvs[1]);
				}
			}
		}
		else if (chemColor->atomColorBinding.getValue() ==
			ChemColor::ATOM_PER_ATOM_INDEXED) {
			for (i = 0; i < localNumberOfBonds; i++) {
				// Skip hydrogen bonds as they will be dealt with later
				if (bondAttributes[i].type == ChemBaseData::HYDROGEN_BOND) {
					numHBonds++;
					continue;
				}

				theBond = bondAttributes[i].index;
				from = bondAttributes[i].from;
				to = bondAttributes[i].to;
		
				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();
				tcoord = (coords2 - coords1) * 0.5f + coords1;
	
				if (showMulti) {
					generateMultiHalfBonded(pvs, numPVS,
						coords1, coords2, i,
						multiSep, multiSep2,
						chemData->getAtomIndex(from),
						chemData->getAtomIndex(to),
						bondAttributes);

					// The point and materialIndex have been set in
					// generateMultiHalfBonded.  Fill in the detail
					// and invokeLineSegmentCallbacks.
					for (j = 0; j < numPVS; j++) {
						invokeLineSegmentCallbacks(action,
							&pvs[j*2], &pvs[j*2+1]);
					}
				}
				else {
					// Create a detail for the specific shape
					detail = new ChemWireframeDetail;
					detail->setAtomBondIndex(-1, theBond);
					detail->setLineType(ChemWireframeDetail::SOLID);

					pvs[0].setDetail(detail);
					pvs[1].setDetail(detail);

					pvs[0].setPoint(coords1.getValue());
					pvs[1].setPoint(tcoord.getValue());

					pvs[0].setMaterialIndex(chemData->getAtomIndex(from));
					pvs[1].setMaterialIndex(chemData->getAtomIndex(from));

					invokeLineSegmentCallbacks(action, &pvs[0], &pvs[1]);

					pvs[0].setPoint(tcoord.getValue());
					pvs[1].setPoint(coords2.getValue());

					pvs[0].setMaterialIndex(chemData->getAtomIndex(to));
					pvs[1].setMaterialIndex(chemData->getAtomIndex(to));

					invokeLineSegmentCallbacks(action, &pvs[0], &pvs[1]);
				}
			}
		}
	}
	else if (chemColor->bondColorBinding.getValue() ==
		ChemColor::BOND_PER_BOND) {

		for (i = 0; i < localNumberOfBonds; i++) {
			// Skip hydrogen bonds as they will be dealt with later
			if (bondAttributes[i].type == ChemBaseData::HYDROGEN_BOND) {
				numHBonds++;
				continue;
			}

			theBond = bondAttributes[i].index;
			from = bondAttributes[i].from;
			to = bondAttributes[i].to;
	
			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}

			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			if (showMulti) {
				generateMulti(pvs, numPVS, coords1, coords2, i,
					multiSep, multiSep2, theBond, theBond, bondAttributes);

				for (j = 0; j < numPVS; j++) {
					invokeLineSegmentCallbacks(action,
						&pvs[j*2], &pvs[j*2+1]);
				}
			}
			else {
				// Create a detail for the specific shape
				detail = new ChemWireframeDetail;
				detail->setAtomBondIndex(-1, theBond);
				detail->setLineType(ChemWireframeDetail::SOLID);

				pvs[0].setDetail(detail);
				pvs[0].setMaterialIndex(theBond);
				pvs[0].setPoint(coords1.getValue());
				pvs[1].setDetail(detail);
				pvs[1].setMaterialIndex(theBond);
				pvs[1].setPoint(coords2.getValue());
				invokeLineSegmentCallbacks(action, &pvs[0], &pvs[1]);
			}
		}
	}
	else if (chemColor->bondColorBinding.getValue() ==
		ChemColor::BOND_PER_BOND_INDEXED) {

		for (i = 0; i < localNumberOfBonds; i++) {
			// Skip hydrogen bonds as they will be dealt with later
			if (bondAttributes[i].type == ChemBaseData::HYDROGEN_BOND) {
				numHBonds++;
				continue;
			}

			theBond = bondAttributes[i].index;
			from = bondAttributes[i].from;
			to = bondAttributes[i].to;
	
			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}

			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			if (showMulti) {
				generateMulti(pvs, numPVS, coords1, coords2, i,
					multiSep, multiSep2,
					chemData->getBondIndex(theBond),
					chemData->getBondIndex(theBond),
					bondAttributes);

				for (j = 0; j < numPVS; j++) {
					invokeLineSegmentCallbacks(action,
						&pvs[j*2], &pvs[j*2+1]);
				}
			}
			else {
				// Create a detail for the specific shape
				detail = new ChemWireframeDetail;
				detail->setAtomBondIndex(-1, theBond);
				detail->setLineType(ChemWireframeDetail::SOLID);

				pvs[0].setDetail(detail);
				pvs[0].setMaterialIndex(chemData->getBondIndex(theBond));
				pvs[0].setPoint(coords1.getValue());
				pvs[1].setDetail(detail);
				pvs[1].setMaterialIndex(chemData->getBondIndex(theBond));
				pvs[1].setPoint(coords2.getValue());
				invokeLineSegmentCallbacks(action, &pvs[0], &pvs[1]);
			}
		}
	}
	state->pop();

	// Generate the primitives for any hydrogen bonds.
	if (numHBonds > 0) {
		generateHBonds(action, state, chemColor, cdp, chemData);
	}

	// Now generate the primitives for the circle inside the aromatic
	// rings if necessary.

	if ((aromaticRingList.getLength() > 0) &&
		(cdp->showMultipleBonds.getValue() == TRUE)) {
		generateSixRing(action, state, chemColor, cdp, chemData);
	}
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Generates lines representing the hydrogen bonds.
//	  Invoked by the generatePrimitives method.
//
// Use: protected

void
ChemDisplay::generateHBonds(SoAction *action, SoState *state,
	const ChemColor *chemColor, const ChemDisplayParam *cdp,
	const ChemBaseData *chemData)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG 
printf ("In ChemDisplay::generateHBonds\n" );
#endif

	state->push();

	int32_t i;
	int32_t	from, to, theBond;
	SbVec3f coords1, coords2, tcoord;
	SoPrimitiveVertex pvs[2];
	ChemWireframeDetail *detail;

	SbBool doHydrogens = cdp->showHydrogens.getValue();

	pvs[0].setTextureCoords(SbVec4f(0.0, 0.0, 0.0, 0.0));
	pvs[1].setTextureCoords(SbVec4f(0.0, 0.0, 0.0, 0.0));

	pvs[0].setNormal(SbVec3f(0.0, 0.0, 1.0));
	pvs[1].setNormal(SbVec3f(0.0, 0.0, 1.0));

	if (chemColor->hydrogenBondColorBinding.getValue() == 
		ChemColor::HBOND_OVERALL) {

		for (i = 0; i < localNumberOfBonds; i++) {
			if (bondAttributes[i].type != ChemBaseData::HYDROGEN_BOND) {
				continue;
			}
			theBond = bondAttributes[i].index;
			from = bondAttributes[i].from;
			to = bondAttributes[i].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			detail = new ChemWireframeDetail;
			detail->setAtomBondIndex(-1, theBond);
			detail->setLineType(ChemWireframeDetail::HBOND);

			pvs[0].setDetail(detail);
			pvs[0].setMaterialIndex(0);
			pvs[0].setPoint(coords1.getValue());
			pvs[1].setDetail(detail);
			pvs[1].setMaterialIndex(0);
			pvs[1].setPoint(coords2.getValue());
			invokeLineSegmentCallbacks(action, &pvs[0], &pvs[1]);
		}
	}
	else if (chemColor->hydrogenBondColorBinding.getValue() ==
		ChemColor::HBOND_PER_HBOND) {

		for (i = 0; i < localNumberOfBonds; i++) {
			if (bondAttributes[i].type != ChemBaseData::HYDROGEN_BOND) {
				continue;
			}
			theBond = bondAttributes[i].index;
			from = bondAttributes[i].from;
			to = bondAttributes[i].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			detail = new ChemWireframeDetail;
			detail->setAtomBondIndex(-1, theBond);
			detail->setLineType(ChemWireframeDetail::HBOND);

			pvs[0].setDetail(detail);
			pvs[0].setMaterialIndex(theBond);
			pvs[0].setPoint(coords1.getValue());
			pvs[1].setDetail(detail);
			pvs[1].setMaterialIndex(theBond);
			pvs[1].setPoint(coords2.getValue());
			invokeLineSegmentCallbacks(action, &pvs[0], &pvs[1]);
		}
	}
	else if (chemColor->hydrogenBondColorBinding.getValue() ==
		ChemColor::HBOND_PER_HBOND_INDEXED) {

		for (i = 0; i < localNumberOfBonds; i++) {
			if (bondAttributes[i].type != ChemBaseData::HYDROGEN_BOND) {
				continue;
			}
			theBond = bondAttributes[i].index;
			from = bondAttributes[i].from;
			to = bondAttributes[i].to;

			if (!doHydrogens) {
				if ((chemData->getAtomicNumber(from) == 1) || 
					(chemData->getAtomicNumber(to) == 1)) continue;
			}
			coords1 = chemData->getAtomCoordinates(from).getValue();
			coords2 = chemData->getAtomCoordinates(to).getValue();

			detail = new ChemWireframeDetail;
			detail->setAtomBondIndex(-1, theBond);
			detail->setLineType(ChemWireframeDetail::HBOND);

			pvs[0].setDetail(detail);
			pvs[0].setMaterialIndex(chemData->getBondIndex(theBond));
			pvs[0].setPoint(coords1.getValue());
			pvs[1].setDetail(detail);
			pvs[1].setMaterialIndex(chemData->getBondIndex(theBond));
			pvs[1].setPoint(coords2.getValue());
			invokeLineSegmentCallbacks(action, &pvs[0], &pvs[1]);
		}
	}
	else if (chemColor->hydrogenBondColorBinding.getValue() ==
		ChemColor::HBOND_PER_BOND) {
		if (chemColor->bondColorBinding.getValue() == 
			ChemColor::BOND_OVERALL) {
	
			for (i = 0; i < localNumberOfBonds; i++) {
				if (bondAttributes[i].type != ChemBaseData::HYDROGEN_BOND) {
					continue;
				}
				theBond = bondAttributes[i].index;
				from = bondAttributes[i].from;
				to = bondAttributes[i].to;
	
				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				detail = new ChemWireframeDetail;
				detail->setAtomBondIndex(-1, theBond);
				detail->setLineType(ChemWireframeDetail::HBOND);

				pvs[0].setDetail(detail);
				pvs[0].setMaterialIndex(0);
				pvs[0].setPoint(coords1.getValue());
				pvs[1].setDetail(detail);
				pvs[1].setMaterialIndex(0);
				pvs[1].setPoint(coords2.getValue());
				invokeLineSegmentCallbacks(action, &pvs[0], &pvs[1]);
			}
		}
		else if (chemColor->bondColorBinding.getValue() == 
			ChemColor::BOND_PER_ATOM) {
	
			if (chemColor->atomColorBinding.getValue() ==
				ChemColor::ATOM_OVERALL) {
	
				for (i = 0; i < localNumberOfBonds; i++) {
					if (bondAttributes[i].type != ChemBaseData::HYDROGEN_BOND) {
						continue;
					}
					theBond = bondAttributes[i].index;
					from = bondAttributes[i].from;
					to = bondAttributes[i].to;
	
					if (!doHydrogens) {
						if ((chemData->getAtomicNumber(from) == 1) || 
							(chemData->getAtomicNumber(to) == 1)) continue;
					}
					coords1 = chemData->getAtomCoordinates(from).getValue();
					coords2 = chemData->getAtomCoordinates(to).getValue();

					detail = new ChemWireframeDetail;
					detail->setAtomBondIndex(-1, theBond);
					detail->setLineType(ChemWireframeDetail::HBOND);

					pvs[0].setDetail(detail);
					pvs[0].setMaterialIndex(0);
					pvs[0].setPoint(coords1.getValue());
					pvs[1].setDetail(detail);
					pvs[1].setMaterialIndex(0);
					pvs[1].setPoint(coords2.getValue());
					invokeLineSegmentCallbacks(action, &pvs[0], &pvs[1]);
				}
			}
			else if (chemColor->atomColorBinding.getValue() ==
				ChemColor::ATOM_PER_ATOM) {
	
				for (i = 0; i < localNumberOfBonds; i++) {
					if (bondAttributes[i].type != ChemBaseData::HYDROGEN_BOND) {
						continue;
					}
					theBond = bondAttributes[i].index;
					from = bondAttributes[i].from;
					to = bondAttributes[i].to;
	
					if (!doHydrogens) {
						if ((chemData->getAtomicNumber(from) == 1) || 
							(chemData->getAtomicNumber(to) == 1)) continue;
					}
					coords1 = chemData->getAtomCoordinates(from).getValue();
					coords2 = chemData->getAtomCoordinates(to).getValue();

					detail = new ChemWireframeDetail;
					detail->setAtomBondIndex(-1, theBond);
					detail->setLineType(ChemWireframeDetail::HBOND);

					pvs[0].setDetail(detail);
					pvs[0].setMaterialIndex(from);
					pvs[0].setPoint(coords1.getValue());
					pvs[1].setDetail(detail);
					pvs[1].setMaterialIndex(to);
					pvs[1].setPoint(coords2.getValue());
					invokeLineSegmentCallbacks(action, &pvs[0], &pvs[1]);
				}
			}
			else if (chemColor->atomColorBinding.getValue() == 
				ChemColor::ATOM_PER_ATOM_INDEXED) {
	
				for (i = 0; i < localNumberOfBonds; i++) {
					if (bondAttributes[i].type != ChemBaseData::HYDROGEN_BOND) {
						continue;
					}
					theBond = bondAttributes[i].index;
					from = bondAttributes[i].from;
					to = bondAttributes[i].to;
	
					if (!doHydrogens) {
						if ((chemData->getAtomicNumber(from) == 1) || 
							(chemData->getAtomicNumber(to) == 1)) continue;
					}
					coords1 = chemData->getAtomCoordinates(from).getValue();
					coords2 = chemData->getAtomCoordinates(to).getValue();
	
					detail = new ChemWireframeDetail;
					detail->setAtomBondIndex(-1, theBond);
					detail->setLineType(ChemWireframeDetail::HBOND);

					pvs[0].setDetail(detail);
					pvs[0].setMaterialIndex(chemData->getAtomIndex(from));
					pvs[0].setPoint(coords1.getValue());
					pvs[1].setDetail(detail);
					pvs[1].setMaterialIndex(chemData->getAtomIndex(to));
					pvs[1].setPoint(coords2.getValue());
					invokeLineSegmentCallbacks(action, &pvs[0], &pvs[1]);
				}
			}
		}
		else if (chemColor->bondColorBinding.getValue() == 
			ChemColor::BOND_PER_ATOM_HALF_BONDED) {
	
			if (chemColor->atomColorBinding.getValue() ==
				ChemColor::ATOM_OVERALL) {
	
				for (i = 0; i < localNumberOfBonds; i++) {
					if (bondAttributes[i].type != ChemBaseData::HYDROGEN_BOND) {
						continue;
					}
					theBond = bondAttributes[i].index;
					from = bondAttributes[i].from;
					to = bondAttributes[i].to;
	
					if (!doHydrogens) {
						if ((chemData->getAtomicNumber(from) == 1) || 
							(chemData->getAtomicNumber(to) == 1)) continue;
					}
					coords1 = chemData->getAtomCoordinates(from).getValue();
					coords2 = chemData->getAtomCoordinates(to).getValue();

					detail = new ChemWireframeDetail;
					detail->setAtomBondIndex(-1, theBond);
					detail->setLineType(ChemWireframeDetail::HBOND);

					pvs[0].setDetail(detail);
					pvs[0].setMaterialIndex(0);
					pvs[0].setPoint(coords1.getValue());
					pvs[1].setDetail(detail);
					pvs[1].setMaterialIndex(0);
					pvs[1].setPoint(coords2.getValue());
					invokeLineSegmentCallbacks(action, &pvs[0], &pvs[1]);
				}
			}
			else if (chemColor->atomColorBinding.getValue() ==
				ChemColor::ATOM_PER_ATOM) {
				for (i = 0; i < localNumberOfBonds; i++) {
					if (bondAttributes[i].type != ChemBaseData::HYDROGEN_BOND) {
						continue;
					}
					theBond = bondAttributes[i].index;
					from = bondAttributes[i].from;
					to = bondAttributes[i].to;
	
					if (!doHydrogens) {
						if ((chemData->getAtomicNumber(from) == 1) || 
							(chemData->getAtomicNumber(to) == 1)) continue;
					}
					coords1 = chemData->getAtomCoordinates(from).getValue();
					coords2 = chemData->getAtomCoordinates(to).getValue();
					tcoord = (coords2 - coords1) * 0.5f + coords1;

					detail = new ChemWireframeDetail;
					detail->setAtomBondIndex(-1, theBond);
					detail->setLineType(ChemWireframeDetail::HBOND);

					pvs[0].setDetail(detail);
					pvs[1].setDetail(detail);

					pvs[0].setPoint(coords1.getValue());
					pvs[1].setPoint(tcoord.getValue());

					pvs[0].setMaterialIndex(from);
					pvs[1].setMaterialIndex(from);

					invokeLineSegmentCallbacks(action, &pvs[0], &pvs[1]);

					pvs[0].setPoint(tcoord.getValue());
					pvs[1].setPoint(coords2.getValue());

					pvs[0].setMaterialIndex(to);
					pvs[1].setMaterialIndex(to);

					invokeLineSegmentCallbacks(action, &pvs[0], &pvs[1]);
				}
			}
			else if (chemColor->atomColorBinding.getValue() ==
				ChemColor::ATOM_PER_ATOM_INDEXED) {
	
				for (i = 0; i < localNumberOfBonds; i++) {
					if (bondAttributes[i].type != ChemBaseData::HYDROGEN_BOND) {
						continue;
					}
					theBond = bondAttributes[i].index;
					from = bondAttributes[i].from;
					to = bondAttributes[i].to;
	
					if (!doHydrogens) {
						if ((chemData->getAtomicNumber(from) == 1) || 
							(chemData->getAtomicNumber(to) == 1)) continue;
					}
					coords1 = chemData->getAtomCoordinates(from).getValue();
					coords2 = chemData->getAtomCoordinates(to).getValue();
					tcoord = (coords2 - coords1) * 0.5f + coords1;
	
					detail = new ChemWireframeDetail;
					detail->setAtomBondIndex(-1, theBond);
					detail->setLineType(ChemWireframeDetail::HBOND);

					pvs[0].setDetail(detail);
					pvs[1].setDetail(detail);

					pvs[0].setPoint(coords1.getValue());
					pvs[1].setPoint(tcoord.getValue());

					pvs[0].setMaterialIndex(chemData->getAtomIndex(from));
					pvs[1].setMaterialIndex(chemData->getAtomIndex(from));

					invokeLineSegmentCallbacks(action, &pvs[0], &pvs[1]);

					pvs[0].setPoint(tcoord.getValue());
					pvs[1].setPoint(coords2.getValue());

					pvs[0].setMaterialIndex(chemData->getAtomIndex(to));
					pvs[1].setMaterialIndex(chemData->getAtomIndex(to));

					invokeLineSegmentCallbacks(action, &pvs[0], &pvs[1]);
				}
			}
		}
		else if (chemColor->bondColorBinding.getValue() ==
			ChemColor::BOND_PER_BOND) {
	
			for (i = 0; i < localNumberOfBonds; i++) {
				if (bondAttributes[i].type != ChemBaseData::HYDROGEN_BOND) {
					continue;
				}
				theBond = bondAttributes[i].index;
				from = bondAttributes[i].from;
				to = bondAttributes[i].to;

				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				detail = new ChemWireframeDetail;
				detail->setAtomBondIndex(-1, theBond);
				detail->setLineType(ChemWireframeDetail::HBOND);

				pvs[0].setDetail(detail);
				pvs[0].setMaterialIndex(theBond);
				pvs[0].setPoint(coords1.getValue());
				pvs[1].setDetail(detail);
				pvs[1].setMaterialIndex(theBond);
				pvs[1].setPoint(coords2.getValue());
				invokeLineSegmentCallbacks(action, &pvs[0], &pvs[1]);
			}
		}
		else if (chemColor->bondColorBinding.getValue() ==
			ChemColor::BOND_PER_BOND_INDEXED) {

			for (i = 0; i < localNumberOfBonds; i++) {
				if (bondAttributes[i].type != ChemBaseData::HYDROGEN_BOND) {
					continue;
				}
				theBond = bondAttributes[i].index;
				from = bondAttributes[i].from;
				to = bondAttributes[i].to;
	
				if (!doHydrogens) {
					if ((chemData->getAtomicNumber(from) == 1) || 
						(chemData->getAtomicNumber(to) == 1)) continue;
				}
				coords1 = chemData->getAtomCoordinates(from).getValue();
				coords2 = chemData->getAtomCoordinates(to).getValue();

				detail = new ChemWireframeDetail;
				detail->setAtomBondIndex(-1, theBond);
				detail->setLineType(ChemWireframeDetail::HBOND);

				pvs[0].setDetail(detail);
				pvs[0].setMaterialIndex(chemData->getBondIndex(theBond));
				pvs[0].setPoint(coords1.getValue());
				pvs[1].setDetail(detail);
				pvs[1].setMaterialIndex(chemData->getBondIndex(theBond));
				pvs[1].setPoint(coords2.getValue());
				invokeLineSegmentCallbacks(action, &pvs[0], &pvs[1]);
			}
		}
	}

	state->pop();
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//	  Generates lines representing the circles inside aromatic rings.
//	  Invoked by the generatePrimitives method.
//
// Use: protected

void
ChemDisplay::generateSixRing(SoAction *action, SoState *state,
	const ChemColor *chemColor, const ChemDisplayParam *cdp,
	const ChemBaseData *chemData)
//
////////////////////////////////////////////////////////////////////////
{
#ifdef DEBUG 
printf ("In ChemDisplay::generateSixRing\n" );
#endif

	state->push();

	int i, j;
	SbVec3f u, v;
	SoPrimitiveVertex pvs[2];
	ChemWireframeDetail *detail;

	detail = new ChemWireframeDetail;
	detail->setAtomBondIndex(-1, -1);
	detail->setLineType(ChemWireframeDetail::AROMATIC);

	// Set the texture coords and the material index.  We won't create
	// a detail for the ring.
	pvs[0].setTextureCoords(SbVec4f(0.0, 0.0, 0.0, 0.0));
	pvs[1].setTextureCoords(SbVec4f(0.0, 0.0, 0.0, 0.0));

	pvs[0].setMaterialIndex(0);
	pvs[1].setMaterialIndex(0);

	pvs[0].setDetail(detail);
	pvs[1].setDetail(detail);

	for (i = 0; i < aromaticRingList.getLength(); i++) {
		ChemAromaticRing *aromaticRing =
			(ChemAromaticRing *)aromaticRingList[i];

		// Set the normal for the segments of the circle to be the
		// normal of the circle.
		pvs[0].setNormal(aromaticRing->normal);
		pvs[1].setNormal(aromaticRing->normal);

		for (j = 0; j < lastAromaticRingNumSides; j++) {
			aromaticRing->transform.multVecMatrix(aromaticRingCoords[j], u);
			aromaticRing->transform.multVecMatrix(
				aromaticRingCoords[(j+1)%lastAromaticRingNumSides], v);
			pvs[0].setPoint(u.getValue());
			pvs[1].setPoint(v.getValue());
			invokeLineSegmentCallbacks(action, &pvs[0], &pvs[1]);
		}
	}

	state->pop();
}


static void
generateMulti(SoPrimitiveVertex *pvs, short &numPVS,
	SbVec3f coords1, SbVec3f coords2, int32_t theBond,
	float multiSep, float multiSep2,
	int32_t color1, int32_t color2, const ChemBondStuff *bondAttributes)
{
	SbVec3f tsep = bondAttributes[theBond].perpNormal * multiSep;
	SbVec3f tsep2 = bondAttributes[theBond].perpNormal * multiSep2;

	ChemWireframeDetail *detail, *detail2;

	numPVS = 0;

	if (bondAttributes[theBond].type == ChemBaseData::QUADRUPLE_BOND) {
		detail = new ChemWireframeDetail;
		detail->setAtomBondIndex(-1, theBond);
		detail->setLineType(ChemWireframeDetail::SOLID);
		coords1 += tsep2;
		coords2 += tsep2;
		pvs[0].setDetail(detail);
		pvs[0].setMaterialIndex(color1);
		pvs[0].setPoint(coords1.getValue());
		pvs[1].setDetail(detail);
		pvs[1].setMaterialIndex(color2);
		pvs[1].setPoint(coords2.getValue());
		numPVS++;

		coords1 += tsep;
		coords2 += tsep;
		pvs[2].setDetail(detail);
		pvs[2].setMaterialIndex(color1);
		pvs[2].setPoint(coords1.getValue());
		pvs[3].setDetail(detail);
		pvs[3].setMaterialIndex(color2);
		pvs[3].setPoint(coords2.getValue());
		numPVS++;

		coords1 -= 2.0f*tsep;
		coords2 -= 2.0f*tsep;
		pvs[4].setDetail(detail);
		pvs[4].setMaterialIndex(color1);
		pvs[4].setPoint(coords1.getValue());
		pvs[5].setDetail(detail);
		pvs[5].setMaterialIndex(color2);
		pvs[5].setPoint(coords2.getValue());
		numPVS++;

		coords1 -= tsep;
		coords2 -= tsep;
		pvs[6].setDetail(detail);
		pvs[6].setMaterialIndex(color1);
		pvs[6].setPoint(coords1.getValue());
		pvs[7].setDetail(detail);
		pvs[7].setMaterialIndex(color2);
		pvs[7].setPoint(coords2.getValue());
		numPVS++;
	}
	else if (bondAttributes[theBond].type == ChemBaseData::TRIPLE_BOND) {
		SbVec3f tcoord1, tcoord2;

		detail = new ChemWireframeDetail;
		detail->setAtomBondIndex(-1, theBond);
		detail->setLineType(ChemWireframeDetail::SOLID);

		tcoord1 = coords1 + tsep;
		tcoord2 = coords2 + tsep;
		pvs[0].setDetail(detail);
		pvs[0].setMaterialIndex(color1);
		pvs[0].setPoint(tcoord1.getValue());
		pvs[1].setDetail(detail);
		pvs[1].setMaterialIndex(color2);
		pvs[1].setPoint(tcoord2.getValue());
		numPVS++;

		pvs[2].setDetail(detail);
		pvs[2].setMaterialIndex(color1);
		pvs[2].setPoint(coords1.getValue());
		pvs[3].setDetail(detail);
		pvs[3].setMaterialIndex(color2);
		pvs[3].setPoint(coords2.getValue());
		numPVS++;

		tcoord1 = coords1 - tsep;
		tcoord2 = coords2 - tsep;
		pvs[4].setDetail(detail);
		pvs[4].setMaterialIndex(color1);
		pvs[4].setPoint(tcoord1.getValue());
		pvs[5].setDetail(detail);
		pvs[5].setMaterialIndex(color2);
		pvs[5].setPoint(tcoord2.getValue());
		numPVS++;
	}
	else if (bondAttributes[theBond].type == ChemBaseData::DOUBLE_BOND) {
		detail = new ChemWireframeDetail;
		detail->setAtomBondIndex(-1, theBond);
		detail->setLineType(ChemWireframeDetail::SOLID);
		if (bondAttributes[theBond].isSymmetric == FALSE) {
			pvs[0].setDetail(detail);
			pvs[0].setMaterialIndex(color1);
			pvs[0].setPoint(coords1.getValue());
			pvs[1].setDetail(detail);
			pvs[1].setMaterialIndex(color2);
			pvs[1].setPoint(coords2.getValue());
			numPVS++;

			coords1 = bondAttributes[theBond].start;
			coords2 = bondAttributes[theBond].end;
			pvs[2].setDetail(detail);
			pvs[2].setMaterialIndex(color1);
			pvs[2].setPoint(coords1.getValue());
			pvs[3].setDetail(detail);
			pvs[3].setMaterialIndex(color2);
			pvs[3].setPoint(coords2.getValue());
			numPVS++;
		}
		else {
			coords1 += tsep2;
			coords2 += tsep2;
			pvs[0].setDetail(detail);
			pvs[0].setMaterialIndex(color1);
			pvs[0].setPoint(coords1.getValue());
			pvs[1].setDetail(detail);
			pvs[1].setMaterialIndex(color2);
			pvs[1].setPoint(coords2.getValue());
			numPVS++;

			coords1 -= tsep;
			coords2 -= tsep;
			pvs[2].setDetail(detail);
			pvs[2].setMaterialIndex(color1);
			pvs[2].setPoint(coords1.getValue());
			pvs[3].setDetail(detail);
			pvs[3].setMaterialIndex(color2);
			pvs[3].setPoint(coords2.getValue());
			numPVS++;
		}
	}
	else if (bondAttributes[theBond].type == ChemBaseData::RESONANCE_BOND) {
		detail = new ChemWireframeDetail;
		detail->setAtomBondIndex(-1, theBond);
		detail->setLineType(ChemWireframeDetail::SOLID);
		detail2 = new ChemWireframeDetail;
		detail2->setAtomBondIndex(-1, theBond);
		detail2->setLineType(ChemWireframeDetail::RESONANCE);

		if (bondAttributes[theBond].isSymmetric == FALSE) {
			pvs[0].setDetail(detail);
			pvs[0].setMaterialIndex(color1);
			pvs[0].setPoint(coords1.getValue());
			pvs[1].setDetail(detail);
			pvs[1].setMaterialIndex(color2);
			pvs[1].setPoint(coords2.getValue());
			numPVS++;

			coords1 = bondAttributes[theBond].start;
			coords2 = bondAttributes[theBond].end;
			pvs[2].setDetail(detail2);
			pvs[2].setMaterialIndex(color1);
			pvs[2].setPoint(coords1.getValue());
			pvs[3].setDetail(detail2);
			pvs[3].setMaterialIndex(color2);
			pvs[3].setPoint(coords2.getValue());
			numPVS++;
		}
		else {
			coords1 += tsep2;
			coords2 += tsep2;
			pvs[0].setDetail(detail);
			pvs[0].setMaterialIndex(color1);
			pvs[0].setPoint(coords1.getValue());
			pvs[1].setDetail(detail);
			pvs[1].setMaterialIndex(color2);
			pvs[1].setPoint(coords2.getValue());
			numPVS++;

			coords1 -= tsep;
			coords2 -= tsep;
			pvs[2].setDetail(detail2);
			pvs[2].setMaterialIndex(color1);
			pvs[2].setPoint(coords1.getValue());
			pvs[3].setDetail(detail2);
			pvs[3].setMaterialIndex(color2);
			pvs[3].setPoint(coords2.getValue());
			numPVS++;
		}
	}
	else {
		detail = new ChemWireframeDetail;
		detail->setAtomBondIndex(-1, theBond);
		detail->setLineType(ChemWireframeDetail::SOLID);

		pvs[0].setDetail(detail);
		pvs[0].setMaterialIndex(color1);
		pvs[0].setPoint(coords1.getValue());
		pvs[1].setDetail(detail);
		pvs[1].setMaterialIndex(color2);
		pvs[1].setPoint(coords2.getValue());
		numPVS++;
	}
}

static void
generateMultiHalfBonded(SoPrimitiveVertex *pvs, short &numPVS,
	SbVec3f coords1, SbVec3f coords2, int32_t theBond,
	float multiSep, float multiSep2, 
	int32_t color1, int32_t color2, const ChemBondStuff *bondAttributes)
{
	SbVec3f tcoord, tc0, tc1, tc2;
	SbVec3f tsep = bondAttributes[theBond].perpNormal * multiSep;
	SbVec3f tsep2 = bondAttributes[theBond].perpNormal * multiSep2;

	ChemWireframeDetail *detail, *detail2;

	numPVS = 0;
	tcoord = (coords2 - coords1) * 0.5f + coords1;

	if (bondAttributes[theBond].type == ChemBaseData::QUADRUPLE_BOND) {
		detail = new ChemWireframeDetail;
		detail->setAtomBondIndex(-1, theBond);
		detail->setLineType(ChemWireframeDetail::SOLID);
		tcoord += tsep2;
		coords1 += tsep2;
		coords2 += tsep2;
		tc0 = tcoord + tsep;
		tc1 = coords1 + tsep;
		tc2 = coords2 + tsep;

		pvs[0].setDetail(detail);
		pvs[0].setMaterialIndex(color1);
		pvs[0].setPoint(coords1.getValue());
		pvs[1].setDetail(detail);
		pvs[1].setMaterialIndex(color1);
		pvs[1].setPoint(tcoord.getValue());
		numPVS++;

		pvs[2].setDetail(detail);
		pvs[2].setMaterialIndex(color1);
		pvs[2].setPoint(tc1.getValue());
		pvs[3].setDetail(detail);
		pvs[3].setMaterialIndex(color1);
		pvs[3].setPoint(tc0.getValue());
		numPVS++;

		pvs[4].setDetail(detail);
		pvs[4].setMaterialIndex(color2);
		pvs[4].setPoint(tcoord.getValue());
		pvs[5].setDetail(detail);
		pvs[5].setMaterialIndex(color2);
		pvs[5].setPoint(coords2.getValue());
		numPVS++;

		pvs[6].setDetail(detail);
		pvs[6].setMaterialIndex(color2);
		pvs[6].setPoint(tc0.getValue());
		pvs[7].setDetail(detail);
		pvs[7].setMaterialIndex(color2);
		pvs[7].setPoint(tc2.getValue());
		numPVS++;

		tcoord -= tsep;
		coords1 -= tsep;
		coords2 -= tsep;
		tc0 = tcoord - tsep;
		tc1 = coords1 - tsep;
		tc2 = coords2 - tsep;

		pvs[8].setDetail(detail);
		pvs[8].setMaterialIndex(color1);
		pvs[8].setPoint(coords1.getValue());
		pvs[9].setDetail(detail);
		pvs[9].setMaterialIndex(color1);
		pvs[9].setPoint(tcoord.getValue());
		numPVS++;

		pvs[10].setDetail(detail);
		pvs[10].setMaterialIndex(color1);
		pvs[10].setPoint(tc1.getValue());
		pvs[11].setDetail(detail);
		pvs[11].setMaterialIndex(color1);
		pvs[11].setPoint(tc0.getValue());
		numPVS++;

		pvs[12].setDetail(detail);
		pvs[12].setMaterialIndex(color2);
		pvs[12].setPoint(tcoord.getValue());
		pvs[13].setDetail(detail);
		pvs[13].setMaterialIndex(color2);
		pvs[13].setPoint(coords2.getValue());
		numPVS++;

		pvs[14].setDetail(detail);
		pvs[14].setMaterialIndex(color2);
		pvs[14].setPoint(tc0.getValue());
		pvs[15].setDetail(detail);
		pvs[15].setMaterialIndex(color2);
		pvs[15].setPoint(tc2.getValue());
		numPVS++;
	}
	else if (bondAttributes[theBond].type == ChemBaseData::TRIPLE_BOND) {
		detail = new ChemWireframeDetail;
		detail->setAtomBondIndex(-1, theBond);
		detail->setLineType(ChemWireframeDetail::SOLID);

		tc0 = tcoord + tsep;
		tc1 = coords1 + tsep;

		pvs[0].setDetail(detail);
		pvs[0].setMaterialIndex(color1);
		pvs[0].setPoint(tc1.getValue());
		pvs[1].setDetail(detail);
		pvs[1].setMaterialIndex(color1);
		pvs[1].setPoint(tc0.getValue());
		numPVS++;

		pvs[2].setDetail(detail);
		pvs[2].setMaterialIndex(color1);
		pvs[2].setPoint(coords1.getValue());
		pvs[3].setDetail(detail);
		pvs[3].setMaterialIndex(color1);
		pvs[3].setPoint(tcoord.getValue());
		numPVS++;

		tc0 = tcoord - tsep;
		tc1 = coords1 - tsep;
		pvs[4].setDetail(detail);
		pvs[4].setMaterialIndex(color1);
		pvs[4].setPoint(tc1.getValue());
		pvs[5].setDetail(detail);
		pvs[5].setMaterialIndex(color1);
		pvs[5].setPoint(tc0.getValue());
		numPVS++;

		tc2 = coords2 - tsep;
		pvs[6].setDetail(detail);
		pvs[6].setMaterialIndex(color2);
		pvs[6].setPoint(tc0.getValue());
		pvs[7].setDetail(detail);
		pvs[7].setMaterialIndex(color2);
		pvs[7].setPoint(tc2.getValue());
		numPVS++;

		pvs[8].setDetail(detail);
		pvs[8].setMaterialIndex(color2);
		pvs[8].setPoint(tcoord.getValue());
		pvs[9].setDetail(detail);
		pvs[9].setMaterialIndex(color2);
		pvs[9].setPoint(coords2.getValue());
		numPVS++;

		tc0 = tcoord + tsep;
		tc2 = coords2 + tsep;
		pvs[10].setDetail(detail);
		pvs[10].setMaterialIndex(color2);
		pvs[10].setPoint(tc0.getValue());
		pvs[11].setDetail(detail);
		pvs[11].setMaterialIndex(color2);
		pvs[11].setPoint(tc2.getValue());
		numPVS++;
	}
	else if (bondAttributes[theBond].type == ChemBaseData::DOUBLE_BOND) {
		detail = new ChemWireframeDetail;
		detail->setAtomBondIndex(-1, theBond);
		detail->setLineType(ChemWireframeDetail::SOLID);

		if (bondAttributes[theBond].isSymmetric == FALSE) {
			tc0 = tcoord + tsep;
			tc1 = bondAttributes[theBond].start;
			tc2 = bondAttributes[theBond].end;

			pvs[0].setDetail(detail);
			pvs[0].setMaterialIndex(color1);
			pvs[0].setPoint(coords1.getValue());
			pvs[1].setDetail(detail);
			pvs[1].setMaterialIndex(color1);
			pvs[1].setPoint(tcoord.getValue());
			numPVS++;

			pvs[2].setDetail(detail);
			pvs[2].setMaterialIndex(color1);
			pvs[2].setPoint(tc1.getValue());
			pvs[3].setDetail(detail);
			pvs[3].setMaterialIndex(color1);
			pvs[3].setPoint(tc0.getValue());
			numPVS++;

			pvs[4].setDetail(detail);
			pvs[4].setMaterialIndex(color2);
			pvs[4].setPoint(tcoord.getValue());
			pvs[5].setDetail(detail);
			pvs[5].setMaterialIndex(color2);
			pvs[5].setPoint(coords2.getValue());
			numPVS++;

			pvs[6].setDetail(detail);
			pvs[6].setMaterialIndex(color2);
			pvs[6].setPoint(tc0.getValue());
			pvs[7].setDetail(detail);
			pvs[7].setMaterialIndex(color2);
			pvs[7].setPoint(tc2.getValue());
			numPVS++;
		}
		else {

			tcoord += tsep2;
			coords1 += tsep2;
			coords2 += tsep2;
			tc0 = tcoord - tsep;
			tc1 = coords1 - tsep;
			tc2 = coords2 - tsep;

			pvs[0].setDetail(detail);
			pvs[0].setMaterialIndex(color1);
			pvs[0].setPoint(coords1.getValue());
			pvs[1].setDetail(detail);
			pvs[1].setMaterialIndex(color1);
			pvs[1].setPoint(tcoord.getValue());
			numPVS++;

			pvs[2].setDetail(detail);
			pvs[2].setMaterialIndex(color1);
			pvs[2].setPoint(tc1.getValue());
			pvs[3].setDetail(detail);
			pvs[3].setMaterialIndex(color1);
			pvs[3].setPoint(tc0.getValue());
			numPVS++;

			pvs[4].setDetail(detail);
			pvs[4].setMaterialIndex(color2);
			pvs[4].setPoint(tcoord.getValue());
			pvs[5].setDetail(detail);
			pvs[5].setMaterialIndex(color2);
			pvs[5].setPoint(coords2.getValue());
			numPVS++;

			pvs[6].setDetail(detail);
			pvs[6].setMaterialIndex(color2);
			pvs[6].setPoint(tc0.getValue());
			pvs[7].setDetail(detail);
			pvs[7].setMaterialIndex(color2);
			pvs[7].setPoint(tc2.getValue());
			numPVS++;
		}
	}
	else if (bondAttributes[theBond].type == ChemBaseData::RESONANCE_BOND) {
		detail = new ChemWireframeDetail;
		detail->setAtomBondIndex(-1, theBond);
		detail->setLineType(ChemWireframeDetail::SOLID);
		detail2 = new ChemWireframeDetail;
		detail2->setAtomBondIndex(-1, theBond);
		detail2->setLineType(ChemWireframeDetail::RESONANCE);

		if (bondAttributes[theBond].isSymmetric == FALSE) {
			tc0 = tcoord + tsep;
			tc1 = bondAttributes[theBond].start;
			tc2 = bondAttributes[theBond].end;

			pvs[0].setDetail(detail);
			pvs[0].setMaterialIndex(color1);
			pvs[0].setPoint(coords1.getValue());
			pvs[1].setDetail(detail);
			pvs[1].setMaterialIndex(color1);
			pvs[1].setPoint(tcoord.getValue());
			numPVS++;

			pvs[2].setDetail(detail2);
			pvs[2].setMaterialIndex(color1);
			pvs[2].setPoint(tc1.getValue());
			pvs[3].setDetail(detail2);
			pvs[3].setMaterialIndex(color1);
			pvs[3].setPoint(tc0.getValue());
			numPVS++;

			pvs[4].setDetail(detail);
			pvs[4].setMaterialIndex(color2);
			pvs[4].setPoint(tcoord.getValue());
			pvs[5].setDetail(detail);
			pvs[5].setMaterialIndex(color2);
			pvs[5].setPoint(coords2.getValue());
			numPVS++;

			pvs[6].setDetail(detail2);
			pvs[6].setMaterialIndex(color2);
			pvs[6].setPoint(tc0.getValue());
			pvs[7].setDetail(detail2);
			pvs[7].setMaterialIndex(color2);
			pvs[7].setPoint(tc2.getValue());
			numPVS++;
		}
		else {

			tcoord += tsep2;
			coords1 += tsep2;
			coords2 += tsep2;
			tc0 = tcoord - tsep;
			tc1 = coords1 - tsep;
			tc2 = coords2 - tsep;

			pvs[0].setDetail(detail);
			pvs[0].setMaterialIndex(color1);
			pvs[0].setPoint(coords1.getValue());
			pvs[1].setDetail(detail);
			pvs[1].setMaterialIndex(color1);
			pvs[1].setPoint(tcoord.getValue());
			numPVS++;

			pvs[2].setDetail(detail2);
			pvs[2].setMaterialIndex(color1);
			pvs[2].setPoint(tc1.getValue());
			pvs[3].setDetail(detail2);
			pvs[3].setMaterialIndex(color1);
			pvs[3].setPoint(tc0.getValue());
			numPVS++;

			pvs[4].setDetail(detail);
			pvs[4].setMaterialIndex(color2);
			pvs[4].setPoint(tcoord.getValue());
			pvs[5].setDetail(detail);
			pvs[5].setMaterialIndex(color2);
			pvs[5].setPoint(coords2.getValue());
			numPVS++;

			pvs[6].setDetail(detail2);
			pvs[6].setMaterialIndex(color2);
			pvs[6].setPoint(tc0.getValue());
			pvs[7].setDetail(detail2);
			pvs[7].setMaterialIndex(color2);
			pvs[7].setPoint(tc2.getValue());
			numPVS++;
		}
	}
	else {
		detail = new ChemWireframeDetail;
		detail->setAtomBondIndex(-1, theBond);
		detail->setLineType(ChemWireframeDetail::SOLID);

		pvs[0].setDetail(detail);
		pvs[0].setMaterialIndex(color1);
		pvs[0].setPoint(coords1.getValue());
		pvs[1].setDetail(detail);
		pvs[1].setMaterialIndex(color1);
		pvs[1].setPoint(tcoord.getValue());
		numPVS++;

		pvs[2].setDetail(detail);
		pvs[2].setMaterialIndex(color2);
		pvs[2].setPoint(tcoord.getValue());
		pvs[3].setDetail(detail);
		pvs[3].setMaterialIndex(color2);
		pvs[3].setPoint(coords2.getValue());
		numPVS++;
	}
}

//
// End of Spew WIREFRAME section
//
