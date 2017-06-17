/*
 * ChemCondenser.c++
 *
 *     Classes:  ChemCondenser
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

#include "inv/SbBox.H"
#include "inv/SbDict.H"
#include "inv/nodes/SoIndexedLineSet.H"
#include "inv/nodes/SoMaterialBinding.H"
#include "inv/nodes/SoNormal.H"
#include "inv/nodes/SoTextureCoordinate2.H"

#include "inv/ChemKit/ChemAssert.H"
#include "inv/ChemKit/ChemCondenser.H"
#include "inv/ChemKit/ChemHasher.H"

/////////////////////////////////////////////////////////////////////////////
//
// Constructor. Initializes data structures.
//
/////////////////////////////////////////////////////////////////////////////

ChemCondenser::ChemCondenser()
{
    lineSet = NULL;
}

/////////////////////////////////////////////////////////////////////////////
//
// Destructor.
//
/////////////////////////////////////////////////////////////////////////////

ChemCondenser::~ChemCondenser()
{
}

/////////////////////////////////////////////////////////////////////////////
//
// This takes a scene graph produced by flattening a graph and
// condenses vertices and material indices, if possible, by removing
// duplicates.
//
/////////////////////////////////////////////////////////////////////////////

void
ChemCondenser::condense(SoIndexedLineSet *_lineSet, SbBool doNormals,
    SbBool doTexCoords)
{
    lineSet = _lineSet;
    ASSERT(lineSet->coordIndex.getNum() % 3 == 0);
    numLines = lineSet->coordIndex.getNum() / 3;

    // See what we all have in lineSet
    haveNormals   = (doNormals && lineSet->normalIndex.getNum() > 1);
    haveTexCoords = (doTexCoords && lineSet->textureCoordIndex.getNum() > 1);
    haveMaterials = (lineSet->materialIndex.getNum() > 1);

    // Remove duplicate coordinates, normals, and texture coordinates
    condenseCoordinates();

    if (doNormals)
	condenseNormals();

    if (doTexCoords)
	condenseTextureCoordinates();

    // Condense the material indices if possible
    condenseMaterials();

    //
    createVertexList();
    createLineList();

    vertexPtrList = new SbPList(numLines * 2);

    createPolyLines();
    adjustIndices();

    delete [] vertices;
    delete [] lines;
    delete vertexPtrList;
}

/////////////////////////////////////////////////////////////////////////////
//
// Removes duplicate coordinates, updating the coordinate indices in
// the line set.
//
/////////////////////////////////////////////////////////////////////////////
void
ChemCondenser::condenseCoordinates(SoIndexedLineSet *_lineSet)
{
    lineSet = _lineSet;
    condenseCoordinates();
}

void
ChemCondenser::condenseCoordinates()
{
    SoVertexProperty *vp =
        (SoVertexProperty *)lineSet->vertexProperty.getValue();
    int		  numCoords = vp->vertex.getNum();
    const SbVec3f *coords   = vp->vertex.getValues(0);

    // Determine the 3D extent of the coordinates so the hash
    // function can normalize coordinates into the range 0 to 1.
    SbBox3f box;
    // int i = 0 put out of the for loop by fabien fontaine the 15/12/2000
    // to avoid compiler warning
    int i = 0 ;
    for ( ; i < numCoords; i++)
	box.extendBy(coords[i]);
    SbVec3f scale;
    box.getSize(scale[0],scale[1], scale[2]);
    for (i = 0; i < 3; i++)
	scale[i] = (scale[i] == 0.0f ? 1.0f : 1.0f / scale[i]);

    // Create a new field in which to store the uniquified coordinates
    SoMFVec3f uniqueCoords;
    uniqueCoords.setContainer(NULL);

    // Add all the coordinates, replacing the old indices with the new
    // ones
    int	 numIndices  = lineSet->coordIndex.getNum();
    int32_t *indices = lineSet->coordIndex.startEditing();

    // Create a ChemHasher to store the coordinates
    ChemHasher coordHasher(&uniqueCoords, numIndices, box.getMin(), scale);

    for (i = 0; i < numIndices; i++)
	if (indices[i] >= 0)
	    indices[i] = coordHasher.addVector(coords[indices[i]]);

    lineSet->coordIndex.finishEditing();
    coordHasher.finish();

    // Replace the coordinates with the unique ones
    vp->vertex = uniqueCoords;
}

/////////////////////////////////////////////////////////////////////////////
//
// Removes duplicate normals, updating the normal indices in
// the line set.
//
/////////////////////////////////////////////////////////////////////////////

void
ChemCondenser::condenseNormals()
{
    SoVertexProperty *vp =
        (SoVertexProperty *)lineSet->vertexProperty.getValue();
    const SbVec3f *normals   = vp->normal.getValues(0);

    // Normals always lie between -1 and 1 in all dimensions
// --> compatibility issues
//    SbVec3f min(-1.0, -1.0, -1.0);
	SbVec3f vmin(-1.0, -1.0, -1.0);
// <-- compatibility issues
    SbVec3f scale(0.5, 0.5, 0.5);

    // Create a new field in which to store the uniquified normals
    SoMFVec3f uniqueNormals;
    uniqueNormals.setContainer(NULL);

    // Add all the normals, replacing the old indices with the new
    // ones
    int	 numIndices  = lineSet->normalIndex.getNum();
    int32_t *indices = lineSet->normalIndex.startEditing();

    // Create a ChemHasher to store the normals
// --> compatibility issues
//    ChemHasher normalHasher(&uniqueNormals, numIndices, min, scale);
	ChemHasher normalHasher(&uniqueNormals, numIndices, vmin, scale);
// <-- compatibility issues

    for (int i = 0; i < numIndices; i++)
	if (indices[i] >= 0)
	    indices[i] = normalHasher.addVector(normals[indices[i]]);

    lineSet->normalIndex.finishEditing();
    normalHasher.finish();

    // Replace the normals with the unique ones
    vp->normal = uniqueNormals;

    // If the normal indices are now the same as the coordinate
    // indices, we can get rid of them
    if (sameIndices(&lineSet->coordIndex, &lineSet->normalIndex))
	lineSet->normalIndex = -1;
}

/////////////////////////////////////////////////////////////////////////////
//
// Removes duplicate texture coordinates, updating the texture
// coordinate indices in the line set.
//
/////////////////////////////////////////////////////////////////////////////

void
ChemCondenser::condenseTextureCoordinates()
{
    SoVertexProperty *vp =
        (SoVertexProperty *)lineSet->vertexProperty.getValue();
    const SbVec2f *texCoords   = vp->texCoord.getValues(0);

    // Texture Coordinates always lie between 0 and 1 in both dimensions
// --> compatibility issues
//    SbVec2f min(0.0, 0.0);
	SbVec2f vmin(0.0, 0.0);
// <-- compatibility issues
    SbVec2f scale(1.0, 1.0);

    // Create a new field in which to store the uniquified texture coordinates
    SoMFVec2f uniqueTexCoords;
    uniqueTexCoords.setContainer(NULL);

    // Add all the texture coordinates, replacing the old indices with
    // the new ones
    int	 numIndices  = lineSet->textureCoordIndex.getNum();
    int32_t *indices = lineSet->textureCoordIndex.startEditing();

    // Create a ChemHasher to store the texture coordinates
// --> compatibility issues
//    ChemHasher texCoordHasher(&uniqueTexCoords, numIndices, min, scale);
    ChemHasher texCoordHasher(&uniqueTexCoords, numIndices, vmin, scale);
// <-- compatibility issues

    for (int i = 0; i < numIndices; i++)
	if (indices[i] >= 0)
	    indices[i] = texCoordHasher.addVector(texCoords[indices[i]]);

    lineSet->textureCoordIndex.finishEditing();
    texCoordHasher.finish();

    // Replace the texture coordinates with the unique ones
    vp->texCoord = uniqueTexCoords;

    // If the texture coordinate indices are now the same as the
    // coordinate indices, we can get rid of them
    if (sameIndices(&lineSet->coordIndex, &lineSet->textureCoordIndex))
	lineSet->textureCoordIndex = -1;
}

/////////////////////////////////////////////////////////////////////////////
//
// Condenses the material indices if possible.
//
/////////////////////////////////////////////////////////////////////////////

void
ChemCondenser::condenseMaterials()
{
    SoVertexProperty *vp =
        (SoVertexProperty *)lineSet->vertexProperty.getValue();
    const int32_t *mtlIndices = lineSet->materialIndex.getValues(0);
    int numIndices = lineSet->materialIndex.getNum();

    // See if all material indices are 0
    SbBool allZero = TRUE;
    for (int i = 0; i < numIndices; i++) {
	if (mtlIndices[i] > 0) {
	    allZero = FALSE;
	    break;
	}
    }

    // If so, clean up the material indices
    if (allZero) {
	// Get rid of the material indices. Note that we store a 0
	// here to differentiate this case (overall material) from the
	// case below where we share material indices.
	lineSet->materialIndex = 0;

	// Set the binding to overall
    	vp->materialBinding.setValue(SoMaterialBinding::OVERALL);
    }

    // Otherwise, see if the material indices are the same as the
    // coordinate indices
    else if (sameIndices(&lineSet->coordIndex, &lineSet->materialIndex))
	lineSet->materialIndex = -1;
}

/////////////////////////////////////////////////////////////////////////////
//
// Returns TRUE if the two sets of indices are the same.
//
/////////////////////////////////////////////////////////////////////////////

SbBool
ChemCondenser::sameIndices(const SoMFInt32 *indexField1,
		       const SoMFInt32 *indexField2)
{
    int num1 = indexField1->getNum();
    int num2 = indexField2->getNum();

    if (num1 != num2)
	return FALSE;

    const int32_t *ind1 = indexField1->getValues(0);
    const int32_t *ind2 = indexField2->getValues(0);

    for (int i = 0; i < num1; i++)
	if (ind1[i] != ind2[i])
	    return FALSE;

    return TRUE;
}

////////////////////////////////////////////////////////////////////////
//
// Fills in StripVertex structures that represent line
// vertices. Returns the number of distinct vertices.
//
////////////////////////////////////////////////////////////////////////
    
void
ChemCondenser::createVertexList()
{
    // Step 0: Access the fields we will need to use
    int         numCoordIndices  = lineSet->coordIndex.getNum();
    const int32_t *coordIndices  = lineSet->coordIndex.getValues(0);
    const int32_t *normalIndices = (!haveNormals ? NULL :
                                   lineSet->normalIndex.getValues(0));
    const int32_t *texCoordIndices = (!haveTexCoords ? NULL :
                                   lineSet->textureCoordIndex.getValues(0));
    const int32_t *mtlIndices = (!haveMaterials ? NULL :
                                   lineSet->materialIndex.getValues(0));
    
    // Step 1: Assume the worst: all coordinates are distinct
    int maxNumVertices = numLines * 2;
    
    // Step 2: Allocate an array of StripVertex instances big enough
    // to hold all of them
    StripVertex *allVertices = new StripVertex[maxNumVertices];
    
    // Step 3: Create an array of pointers to StripVertex instances, 
    // indexed by coordinate index. This will allow us to find
    // duplicates very easily. (Note that the maximum coordinate index
    // we can find is the number of coordinate values - 1.)
    SoVertexProperty *vp =
        (SoVertexProperty *)lineSet->vertexProperty.getValue();
    int numCoords = vp->vertex.getNum();
    StripVertex **table = new StripVertex *[numCoords];
    // int i = 0 put out of the for loop by fabien fontaine the 15/12/2000
    // to avoid compiler warning
    int i = 0 ;
    for ( ; i < numCoords; i++)
        table[i] = NULL;

    // Step 4: Also create an array of indices into the StripVertex
    // instances, indexed by the index into the coordIndex field. This
    // is used later on to create polyLines from the StripVertex
    // instances.
    vertexMap = new int [numCoordIndices];

    // Step 5: Fill in the StripVertex instances with actual indices,
    // checking for duplicates. If we find a duplicate, we don't save   
    // it.
    StripVertex *curVert = allVertices;
    for (i = 0; i < numCoordIndices; i++) {
  
        int coordIndex = coordIndices[i];
    
        if (coordIndex < 0) {
            vertexMap[i] = -1;
            continue;
        }
    
        // Fill in the next StripVertex instance
        curVert->coordIndex     = coordIndex;
        curVert->normIndex      = (haveNormals ?     normalIndices[i] : -1);
        curVert->texCoordIndex  = (haveTexCoords ? texCoordIndices[i] : -1);
        curVert->mtlIndex       = (haveMaterials ?      mtlIndices[i] : -1);
        curVert->uniqueID       = curVert - allVertices;
    
        // See if we have any entries at that slot in the table
        StripVertex *oldVert;
        for (oldVert = table[coordIndex]; oldVert != NULL;   
             oldVert = oldVert->next) {   
    
            if (oldVert->normIndex     == curVert->normIndex &&
                oldVert->texCoordIndex == curVert->texCoordIndex &&
                oldVert->mtlIndex      == curVert->mtlIndex)   
                break;
        }
    
        // If we found a match, re-use the old vertex
        if (oldVert != NULL)
            vertexMap[i] = oldVert->uniqueID;
    
        // Otherwise, store this vertex in the table and prepare for
        // the next vertex
        else {
            curVert->next = table[coordIndex];
            table[coordIndex] = curVert;
            vertexMap[i] = curVert->uniqueID;
            curVert++;
        }
    }
    
    // Step 6: The number of vertices we stored is the number of
    // distinct vertices
    numVertices = curVert - allVertices;
    
    // Step 7: Copy the distinct vertices into the real array
    vertices = new StripVertex[numVertices];
    for (i = 0; i < numVertices; i++) {
        vertices[i] = allVertices[i];
        vertices[i].curLine = 0;
    }
            
    // Step 8: Get rid of the stuff we don't need any more
    delete [] table;
    delete [] allVertices;
}

////////////////////////////////////////////////////////////////////////
// 
// Creates structures that will be easier to deal with when creating
// polyLines.
//  
////////////////////////////////////////////////////////////////////////
        
void
ChemCondenser::createLineList()
{   
    //const int32_t *coordIndices = lineSet->coordIndex.getValues(0);
                
    // Allocate lines
    lines = new StripLine[numLines];
         
    // Fill in line structures, using the vertexMap created in
    // createVertexList
    for (int i = 0; i < numLines; i++) {
            
        StripLine *line = &lines[i];
        
        //ASSERT(coordIndices[3*i + 0] >= 0);
        int vertexIndex0 = vertexMap[3*i + 0];
        int vertexIndex1 = vertexMap[3*i + 1];
        ASSERT(vertexIndex0 >= 0);
        ASSERT(vertexIndex1 >= 0);
        ASSERT(vertexIndex0 < numVertices);
        ASSERT(vertexIndex1 < numVertices);

        line->isUsed = FALSE;
        if (vertexIndex0 == vertexIndex1) {
            line->isUsed = TRUE;
        }
        else {
            // Look through the vertex with the most lines to see if
            // the 2 current vertices have already been used in a line
            // and if so, then mark the line as used.
            if (vertices[vertexIndex0].lines.getLength() >= 
                vertices[vertexIndex1].lines.getLength()) {
                for (int j = 0; j < vertices[vertexIndex0].lines.getLength(); 
                     j++) {
                    StripLine *tLine = 
                        (StripLine *)vertices[vertexIndex0].lines[j];
                    StripVertex *v0 = tLine->v[0];
                    StripVertex *v1 = tLine->v[1];
                    if ((v0 == &vertices[vertexIndex0] &&
                         v1 == &vertices[vertexIndex1]) ||
                        (v1 == &vertices[vertexIndex0] && 
                         v0 == &vertices[vertexIndex1])) {
                        line->isUsed = TRUE;
                    }
                }
            }
            else {
                for (int j = 0; j < vertices[vertexIndex1].lines.getLength();
                     j++) {
                    StripLine *tLine = 
                        (StripLine *)vertices[vertexIndex1].lines[j];
                    StripVertex *v0 = tLine->v[0];
                    StripVertex *v1 = tLine->v[1];
                    if ((v0 == &vertices[vertexIndex0] && 
                         v1 == &vertices[vertexIndex1]) ||
                        (v1 == &vertices[vertexIndex0] &&
                         v0 == &vertices[vertexIndex1])) {
                        line->isUsed = TRUE;
                    }
                }   
            }
            if (!line->isUsed) {
                line->v[0] = &vertices[vertexIndex0];
                line->v[1] = &vertices[vertexIndex1];
                vertices[vertexIndex0].lines.append(line);
                vertices[vertexIndex1].lines.append(line);
        
                // Not used in a real strip yet
                line->isUsed = FALSE;
            }
        }
    }

    // We don't need this any more
    delete [] vertexMap;
}

void
ChemCondenser::createPolyLines()
{
    int32_t i;
    StripLine *line;
    StripVertex *vp;

    // Pick a line, any line.
    // Pick one of its vertices (call it A).
    // Insert A into the vertexPtrList.
    // Find the next line that has A as a vertex.
    // If none, 
    //     Go back to the original line and pick the other vertex (B).
    //     Append B to the vertexPtrlist.
    //     Find the next line that has B as a vertex.
    //     If none,
    //         Add NULL to the vertexPtrList.
    //     Else
    //         Add B's partner to the vertexPtrList
    //         Find the next line that has B's partner as a vertex
    //         ...
    // Else
    //     Insert A's partner into the list
    //     Find the next line that has A's partner as a vertex.
    //     ...

    int32_t vp_insert = 0;
    for (i = 0; i < numLines; i++) {
        StripLine *lineStart;
        lineStart = line = &lines[i];
        if (!line->isUsed) {
            line->isUsed = TRUE;
            vertexPtrList->append(lineStart->v[0]);
            vp = lineStart->v[0];
            for (;;) {
                if (vp->curLine == vp->lines.getLength()) break;
                line = (StripLine *)vp->lines[vp->curLine++];
                if (!line->isUsed) {
                    if (line->v[0]->uniqueID == vp->uniqueID) {
                        vp = line->v[1];
                    }
                    else if (line->v[1]->uniqueID == vp->uniqueID) {
                        vp = line->v[0];
                    }
                    vertexPtrList->insert(vp, vp_insert);
                    line->isUsed = TRUE;
                }
            }

            line = lineStart;
            vertexPtrList->append(lineStart->v[1]);
            vp = lineStart->v[1];
            for (;;) {
                if (vp->curLine == vp->lines.getLength()) break;
                line = (StripLine *)vp->lines[vp->curLine++];
                if (!line->isUsed) {
                    if (line->v[0]->uniqueID == vp->uniqueID) {
                        vp = line->v[1];
                    }
                    else if (line->v[1]->uniqueID == vp->uniqueID) {
                        vp = line->v[0];
                    }
                    vertexPtrList->append(vp);
                    line->isUsed = TRUE;
                }
            }
            vertexPtrList->append(NULL);
        }
        vp_insert = vertexPtrList->getLength();
    }
}

void
ChemCondenser::adjustIndices()
{
    int32_t numVertices = vertexPtrList->getLength();

    lineSet->coordIndex.setNum(numVertices);
    int32_t *indices = lineSet->coordIndex.startEditing();
    for (int32_t i = 0; i < numVertices; i++) {
        StripVertex *vp = (StripVertex *)(*vertexPtrList)[i];
        indices[i] = (vp == NULL ? -1 : vp->coordIndex);
    }
    lineSet->coordIndex.finishEditing();
}
