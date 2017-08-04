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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "inv/SbString.H" 
#include "inv/SbPList.H" 

#include "inv/ChemKit/ChemMOLImporter.H"
#include "inv/ChemKit/ChemData.H"
#include "inv/ChemKit/ChemAssociatedData.H"
#include "inv/ChemKit/ChemTheElements.H"

ChemMOLImporter::ChemMOLImporter()
{
}

ChemMOLImporter::~ChemMOLImporter()
{
}

bool ChemMOLImporter::openFile (const char* filename, ChemData* chemdata, ChemAssociatedData* chemassociateddata) const
{
    int i, sc;
    char buf[132];

    SbString record;

    // Open the file
    FILE *fp = fopen(filename, "r");

	if (fp == NULL) return false;

    // Get the name of the molecule
    fgets(buf, 132, fp);
	buf[strlen(buf)-1] = '\0';

    // Skip past the next 2 lines
    fgets(buf, 132, fp);
    fgets(buf, 132, fp);

    // Get the number of atoms and bonds
    fgets(buf, 132, fp);
    record = buf;
    sc = 0;

    int32_t numAtoms = atoi(record.getSubString(sc,sc+2).getString());
    sc+=3;

    int32_t numBonds = atoi(record.getSubString(sc,sc+2).getString());

    // Set the numberOfAtoms and numberOfBonds in chemdata

	//printf("numAtoms %d i numBonds %d\n",numAtoms, numBonds);

    chemdata->numberOfAtoms.setValue(numAtoms);
    chemdata->numberOfBonds.setValue(numBonds);

    // For fast filling of the chemdata fields, first set the size of the
    // chemdata fields.  
    chemdata->atomicNumber.setNum(numAtoms);
    chemdata->atomId.setNum(numAtoms);
    chemdata->atomName.setNum(numAtoms);
    chemdata->atomIndex.setNum(numAtoms);
    chemdata->atomCoordinates.setNum(numAtoms);

    chemdata->bondFrom.setNum(numBonds);
    chemdata->bondTo.setNum(numBonds);
    chemdata->bondType.setNum(numBonds);
    chemdata->bondIndex.setNum(numBonds);

    // Then "startEditing()" the fields
    short    *atomicNumber    = chemdata->atomicNumber.startEditing();
    int32_t  *atomId          = chemdata->atomId.startEditing();
    SbString *atomName        = chemdata->atomName.startEditing();
    int32_t  *atomIndex       = chemdata->atomIndex.startEditing();
    SbVec3f  *atomCoordinates = chemdata->atomCoordinates.startEditing();

    int32_t  *bondFrom        = chemdata->bondFrom.startEditing();
    int32_t  *bondTo          = chemdata->bondTo.startEditing();
    int32_t  *bondType        = chemdata->bondType.startEditing();
    int32_t  *bondIndex       = chemdata->bondIndex.startEditing();

    // Read in the atom data
    // Need atomicNumber, atomIndex, atomId, atomName 
    // and atomCoordinates

    short atnum;
    float xx, yy, zz;
    SbString atomString;
    //short rindex = 0;
    SbString atomTypeString;

    for (i = 0; i < numAtoms; i++)
    {
        fgets(buf, 132, fp);
        parseAtomRecord(buf, xx,yy,zz,&atomString);
        stringToAtnum(atomString.getString(), atnum);

        atomicNumber[i]    = atnum;
        atomIndex[i]       = atnum;
        atomId[i]          = i+1;
        atomName[i]        = atomString;
        atomCoordinates[i] = SbVec3f(xx, yy, zz);
    }

    chemdata->atomicNumber.finishEditing();
    chemdata->atomId.finishEditing();
    chemdata->atomName.finishEditing();
    chemdata->atomIndex.finishEditing();
    chemdata->atomCoordinates.finishEditing();

    // Read in the bond data
    // Need bondFrom, bondTo, bondType, bondIndex
    int32_t from, to;
    short type;

    for (i = 0; i < numBonds; i++)
    {
        fgets(buf, 132, fp);
        parseBondRecord(buf, from, to, type);

        bondFrom[i]  = from;
        bondTo[i]    = to;
        bondType[i]  = type;
        bondIndex[i] = i;
    }

    chemdata->bondFrom.finishEditing();
    chemdata->bondTo.finishEditing();
    chemdata->bondType.finishEditing();
    chemdata->bondIndex.finishEditing();

    fclose(fp);

    return true;
}

void ChemMOLImporter::parseAtomRecord(const char *buffer,
									  float &xx,float &yy,float &zz,
									  SbString* atomName) const
{
//  ATOMFORMAT "(f10.4,f10.4,f10.4,1x,a3);
    int i;
    SbString record(buffer);
    //int len = record.getLength() - 1;
    int sc = 0;

    xx = yy = zz = 0.0f;
    *atomName = "";

    // Get the X, Y and Z coordinates
    xx = (float)atof(record.getSubString(sc,sc+9).getString()); sc+=10;
    yy = (float)atof(record.getSubString(sc,sc+9).getString()); sc+=10;
    zz = (float)atof(record.getSubString(sc,sc+9).getString()); sc+=11;

    *atomName = record.getSubString(sc,sc+2).getString(); sc+=2;

    // Trim off trailing blanks from the atomName
    const char *tmpName = atomName->getString();
    for (i = atomName->getLength()-1; i > 0; i--)
	{
        if (tmpName[i] != ' ') break;
    }
    if ((i+1) < atomName->getLength()) atomName->deleteSubString(i+1);
}

void ChemMOLImporter::parseBondRecord(const char *buffer, int32_t &from, int32_t &to,
									  short &type ) const
{   
//  BONDFORMAT "(3i3)"
    SbString record(buffer);
    //int len = record.getLength() - 1;
    int sc = 0;
        
    // Get the "from" atom, the "to" atom and the bond type
    from = atoi(record.getSubString(sc,sc+2).getString()); sc+=3;
    to = atoi(record.getSubString(sc,sc+2).getString()); sc+=3;
    type = (short)atoi(record.getSubString(sc,sc+2).getString());

    // Subtract 1 from "from" and "to" since atom indexing starts at 0
    // and the "from" and "to" atoms in the file start with atom 1
    from--;
    to--;
}

void ChemMOLImporter::stringToAtnum(const char *string, short &atnum) const
{
    // Make a duplicate of the string and make sure it is 
    // NULL terminated

    char *tmpstr = strdup(string);
    tmpstr[strcspn(tmpstr, " ")] = '\0';
   
    atnum = -1;

    // Compare the temporary string with what is in the table 
    // of element names

    int ilen;
    for (ilen = 1; ilen < 104; ilen++) {
        if (!strcmp (tmpstr, theElements[ilen] )) {
            atnum = ilen;
            free(tmpstr);
            return;
        }
    }
    free(tmpstr);
}
