/*
 * ChemSAS.c++
 *
 *     Classes:  ChemSAS
 *
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


#ifdef WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>

#include <float.h> // FLT_MAX
#include <string.h> // memset

#include "inv/nodes/SoVertexProperty.H"
#include "inv/nodes/SoIndexedTriangleStripSet.H"

#include "inv/misc/SoState.H"
#include "inv/actions/SoGLRenderAction.H"
#include "inv/actions/SoRayPickAction.H"
#include "inv/actions/SoGetBoundingBoxAction.H"

#include "inv/bundles/SoMaterialBundle.H"

#include "inv/ChemKit/ChemSAS.H"
#include "inv/ChemKit/ChemBaseDataElement.H"
#include "inv/ChemKit/ChemDisplayParamElement.H"
#include "inv/ChemKit/ChemColorElement.H"
#include "inv/ChemKit/ChemRadiiElement.H"

// redefined here for compatibility with different compilers
#ifndef min
#define min(a,b) (((a)<(b))?(a):(b)) 
#endif
#ifndef max
#define max(a,b) (((a)>(b))?(a):(b)) 
#endif

SO_NODE_SOURCE(ChemSAS);

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Constructor
//
// Use: public

ChemSAS::ChemSAS()
//
////////////////////////////////////////////////////////////////////////
{
    SO_NODE_CONSTRUCTOR(ChemSAS);

/* 
do it for each field
  SO_NODE_ADD_FIELD(probeRadius, (0.0));
	....

	SO_NODE_DEFINE_ENUM_VALUE(ColorBinding, OVERALL);
	...

	SO_NODE_SET_SF_ENUM_TYPE(colorBinding, ColorBinding);
*/
  
		iso = NULL;
		lattice = NULL;
		regenerateFlag = FALSE;
    // Initialize private variables
    lastChemBaseDataElement = NULL;
    lastChemColorElement = NULL;
    lastChemRadiiElement = NULL;

    isBuiltIn = TRUE;
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Destructor
//
// Use: protected

ChemSAS::~ChemSAS()
//
////////////////////////////////////////////////////////////////////////
{
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    If a change was made to one of the fields, then we might need
//    to regenerate OUR SAS
//
// Use: internal

void
ChemSAS::notify(SoNotList *list)
//
////////////////////////////////////////////////////////////////////////
{
    SoShape::notify(list);
}

////////////////////////////////////////////////////////////////////////
// 
// Description:
//    Performs GL rendering of the SAS
//
// Use: public, extender

void
ChemSAS::GLRender(SoGLRenderAction *action)
// 
////////////////////////////////////////////////////////////////////////
{
		if (!shouldGLRender(action)) return;

		
		// render a mano
		
		if (iso==NULL)
		{
			 const ChemBaseDataElement *cbde =
       ChemBaseDataElement::getInstance(action->getState());
			 if (cbde->get(action->getState())->getNumberOfAtoms() == 0)
				 return;
				
		}
		regenerate(action);

    const int32_t* coords = iso->triStrip->coordIndex.getValues(0);
		const SbVec3f* vtx = ((SoVertexProperty*)iso->triStrip->vertexProperty.getValue())->vertex.getValues(0);			
		const SbVec3f* nml = ((SoVertexProperty*)iso->triStrip->vertexProperty.getValue())->normal.getValues(0);

		//
		//const uint32_t* col= ((SoVertexProperty*)iso->triStrip->vertexProperty.getValue())->orderedRGBA.getValues(0);
		//
		const int n = iso->triStrip->coordIndex.getNum();

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_COLOR_MATERIAL);
		glColor4f(1,0,0,0.5);
		
		glBegin(GL_TRIANGLE_STRIP);
		for (int i=0;i<n;i++)
		{
			// TODO: COLOR MODES
			//SbColor color;
			//float trans = 0;
			//color.setPackedValue(col[coords[i]],trans);
			//glColor3fv((float*)&color);
			// END TODO: COLOR MODES
			glNormal3fv((float*)&nml[coords[i]]);
			glVertex3fv((float*)&vtx[coords[i]]);
		}
		glEnd();
		//
		
		glDisable(GL_BLEND);

		//SoShape::GLRender(action);
}

////////////////////////////////////////////////////////////////////////
// 
// Description:
//    Implements ray picking
//
// Use: public, extender

void
ChemSAS::rayPick(SoRayPickAction *action)
// 
////////////////////////////////////////////////////////////////////////
{
//    regenerate(action);
    SoShape::rayPick(action);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Generate primitives
//
// Use: protected

void
ChemSAS::generatePrimitives(SoAction *action)
//
////////////////////////////////////////////////////////////////////////
{
    //regenerate(action);
	if (iso!=NULL)
	iso->generatePrimitives(action);
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Computes the bounding box of the SAS
//  
// Use: protected

void
ChemSAS::computeBBox(SoAction *action, SbBox3f &box,
    SbVec3f &center)
//
////////////////////////////////////////////////////////////////////////
{ 
}

////////////////////////////////////////////////////////////////////////
//
// Description:
//    Generate the SAS
//  
// Use: private

void
ChemSAS::regenerate(SoGLRenderAction *action)
//
////////////////////////////////////////////////////////////////////////
{

    SoState *state = action->getState();

    SbBool dataChanged = FALSE;
    SbBool colorChanged = FALSE;
    SbBool radiiChanged = FALSE;

    if (regenerateFlag == FALSE) {
        // Determine if the data has changed since the last invocation
        const ChemBaseDataElement *cbde =
            ChemBaseDataElement::getInstance(state);
        if (cbde != NULL) {
            if (lastChemBaseDataElement == NULL) {
                dataChanged = TRUE;
                lastChemBaseDataElement =
                    (ChemBaseDataElement *)cbde->copyMatchInfo();
            }
            else {
                if (!lastChemBaseDataElement->matches(cbde)) {
                    dataChanged = TRUE;
                    delete (SoElement *)lastChemBaseDataElement;
                    lastChemBaseDataElement =
                        (ChemBaseDataElement *)cbde->copyMatchInfo();
                }
            }
        }

        // Determine if the atom colors have changed since the last invocation
        const ChemColorElement *cce =
            ChemColorElement::getInstance(state);
        if (cce != NULL) {
            if (lastChemColorElement == NULL) {
                colorChanged = TRUE;
                lastChemColorElement =
                    (ChemColorElement *)cce->copyMatchInfo();
            }
            else {
                if (!lastChemColorElement->matches(cce)) {
                    colorChanged = TRUE;
                    delete (SoElement *)lastChemColorElement;
                    lastChemColorElement =
                        (ChemColorElement *)cce->copyMatchInfo();
                }
            }
        }

        // Determine if the atom radii have changed since the last invocation
        const ChemRadiiElement *cre =
            ChemRadiiElement::getInstance(state);
        if (cre != NULL) {
            if (lastChemRadiiElement == NULL) {
                radiiChanged = TRUE;
                lastChemRadiiElement =
                    (ChemRadiiElement *)cre->copyMatchInfo();
            }
            else {
                if (!lastChemRadiiElement->matches(cre)) {
                    radiiChanged = TRUE;
                    delete (SoElement *)lastChemRadiiElement;
                    lastChemRadiiElement =
                        (ChemRadiiElement *)cre->copyMatchInfo();
                }
            }
        }
    }

    if ((dataChanged == FALSE) && (radiiChanged == FALSE) &&
        (colorChanged == FALSE) && (regenerateFlag == FALSE)) return;

    // Make sure regenerateFlag has been canceled out
    regenerateFlag = FALSE;

		CalcSurface(action);
}

void ChemSAS::addAtomIndex(const int32_t index)
{
	atomIndices.set1Value(atomIndices.getNum(),index);
}

void ChemSAS::removeAtomIndex(const int32_t index)
{
	atomIndices.deleteValues(index,1);
}

int32_t ChemSAS::getAtomIndex(const int32_t index) const
{
	return atomIndices[index];
}

void ChemSAS::setAtomIndex(const int32_t index, const int32_t newAtomIndex)
{
	atomIndices.set1Value(index,newAtomIndex);
}

const SoMFInt32& ChemSAS::getAtomIndices() const
{
	return atomIndices;
}


void
ChemSAS::CalcSurface(SoAction* action)
{
	int i,j,k, pos;
	int32_t dims[3];
	float bBox[6];
	double value;

	SoState* state = action->getState();

	ChemBaseData *chemData = ChemBaseDataElement::get(state);
	ChemDisplayParam *cdp= ChemDisplayParamElement::get(state);
  ChemColor *chemColor = ChemColorElement::get(state);
  ChemRadii *chemRadii = ChemRadiiElement::get(state);

	lattice = new ChemLattice3(LATTICE_DATA_FLOAT32, LATTICE_COORD_UNIFORM);
	lattice->nDataVar = 1;

	//
	//const SbColor *atomColors = chemColor->atomColor.getValues(0);
	color = new ChemLattice3(LATTICE_DATA_INT32, LATTICE_COORD_UNIFORM);
	color->nDataVar = 1;
	//

	// calc dimensions of scene

	// not really necessary, but sogetbounding box needs it
	SbViewportRegion vpreg;

	SoSeparator* root = (SoSeparator*)SoNode::getByName("root");
	SoGetBoundingBoxAction* act = new SoGetBoundingBoxAction(vpreg);
	act->apply((SoNode*)root);
	SbBox3f BBox = act->getBoundingBox();
	//SbVec3f bcenter = act->getCenter();
	
	bBox[0] = BBox.getMin()[0];
	bBox[1] = BBox.getMax()[0];
	bBox[2] = BBox.getMin()[1];
	bBox[3] = BBox.getMax()[1];
	bBox[4] = BBox.getMin()[2];
	bBox[5] = BBox.getMax()[2];

	// specify number of cubies in each direction
	// TODO: determine better complexity 
	int K = 2; // number of segments per angstrom

	float dx = fabs(bBox[0]) + fabs(bBox[1]);
	float dy = fabs(bBox[2]) + fabs(bBox[3]);
	float dz = fabs(bBox[4]) + fabs(bBox[5]);
	dims[0] = (long)(dx * K);
	dims[1] = (long)(dy * K) ;
	dims[2] = (long)(dz * K);
	
	lattice->dimension.setValue(dims);

	lattice->coord.setValues(0, 6, bBox);

	((SoMFFloat *)(lattice->data))->setNum(dims[0]*dims[1]*dims[2]);

  float *latticeData = ((SoMFFloat *)(lattice->data))->startEditing();

	// reset
	memset(latticeData, 1, dims[0]*dims[1]*dims[2]*sizeof(float));
	for (pos = 0; pos < dims[0]*dims[1]*dims[2]; pos++)
		latticeData[pos] = (float)27;

	//
	color->dimension.setValue(dims);
	color->coord.setValues(0, 6, bBox);
	((SoMFInt32 *)(color->data))->setNum(dims[0]*dims[1]*dims[2]);
  int32_t *colorData = ((SoMFInt32*)(color->data))->startEditing();
	memset(colorData, 0, dims[0]*dims[1]*dims[2]*sizeof(int32_t));
	//

	float radScale = cdp->atomRadiiScaleFactor.getValue();

	const float *atomRadii = chemRadii->atomRadii.getValues(0);
	int localAtomIndex;
	float rad;
	//
	SbVec3f position;
	int index[3];
	int a;
	float threshold = 25;
	float factor = 0;
	int r;
	const float *perim = lattice->coord.getValues(0);
	float   lower;
	float   upper;

	int fromTo[6];

	// for each sphere
	printf("filling lattice...\n");
	for (a=0;a<chemData->getNumberOfAtoms();a++)
	// for (b=0;b<atomIndices.getNum(); b++)
	{
		// a = atomIndices[b];
		localAtomIndex = chemData->getAtomIndex(a);
		rad = atomRadii[localAtomIndex];
		rad *= radScale;

		factor = threshold/(rad*rad);

		//printf("atom %d/%d\n", a, chemData->getNumberOfAtoms());
		for (r=0;r<3;r++)
		{
			lower = perim[2*r];
			upper = perim[2*r + 1];
			fromTo[r*2] = (int)(( (chemData->getAtomCoordinates(a)[r] - rad - lower)*(float)(dims[r]-1) ) / (upper - lower));
			fromTo[(r*2)+1] = (int)(( (chemData->getAtomCoordinates(a)[r] + rad - lower)*(float)(dims[r]-1) ) / (upper - lower));
		}
	
		// calc radii and normalize by threshold
		for (i=fromTo[0];i<fromTo[1];i++)
		for (j=fromTo[2];j<fromTo[3];j++)
		for (k=fromTo[4];k<fromTo[5];k++)
		{
			
			index[0] = i;index[1] = j;index[2] = k;
			position = lattice->getCoord(index);

#ifdef __coin
			float len = (chemData->getAtomCoordinates(a)-position).sqrLength();
#else
			// SbVec3f from SGI's OIV hasn't a member function sqrLength
			float len = (chemData->getAtomCoordinates(a)-position).length();
			len*=len;
#endif
			value = len * factor;
			pos= i + dims[0]*(j + dims[1]*k);
			if (latticeData[pos] > value)
			{
					latticeData[pos] = (float)value;
					//
					colorData[pos] = localAtomIndex;
					//
			}

		}
	}

	printf("lattice full ...\n");

/*	printf("smoothing ...\n");

	float smooth = 0.0f;
	for (int iter = 0;iter<10;iter++)
	for (i=1;i<dims[0]-2;i++)
	for (j=1;j<dims[1]-2;j++)
	for (k=1;k<dims[2]-2;k++)
	{
		smooth = 0.0f;
		const float T = 1/26;

		for (int r= -1 ; r<=1; r++)
		for (int s= -1 ; r<=1; r++)
		for (int t= -1 ; r<=1; r++)
		{
			if (r == 0 && s==0 && t == 0) continue;
			pos= (i+r) + dims[0]*((j+s) + dims[1]*(k+t));
			smooth += latticeData[pos]*T;
		}
		//

		pos= (i) + dims[0]*((j) + dims[1]*(k));
		latticeData[pos] += smooth;
	}
*/

	printf("way to go...\n");

	((SoMFFloat *)(lattice->data))->finishEditing();


	iso = new ChemIso;
	iso->data = lattice;
	//
	//iso->color = lattice;
	iso->color = color;
	//
	iso->threshold.setValue(threshold);
	iso->flipNormals = TRUE;
	iso->dataVar = 0;
	iso->colorVar = 0;
	//
	//iso->minValue = -10;
	//iso->maxValue = 10;
	iso->minValue = 0;
	iso->maxValue = (float)chemColor->atomColor.getNum()-1;
	//


	//iso->orderedRGBA.setNum(1);
	iso->orderedRGBA.setNum(chemColor->atomColor.getNum());
	uint32_t *tmpMap = iso->orderedRGBA.startEditing();
	//for (i = 0; i < 1; i++) {
	for (i = 0; i < chemColor->atomColor.getNum(); i++) {
//			float red   = i%2;
//			float green = (i+1)%2;
//			float blue  = 0;
//			float alpha = 0.0;
//			tmpMap[i] = SbColor(red, green, blue).getPackedValue(alpha - 1.0);
				tmpMap[i] = chemColor->atomColor[i].getPackedValue(0.0 - 1.0);
	}
	iso->orderedRGBA.finishEditing();

	// force regenerate
	iso->regenerate(FALSE);


}
