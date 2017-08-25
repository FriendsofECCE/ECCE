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
#include <assert.h>

#include "inv/SbPList.H"

#include "inv/ChemKit/ChemPDBImporter.H"
#include "inv/ChemKit/ChemData.H"
#include "inv/ChemKit/ChemAssociatedData.H"
#include "inv/ChemKit/ChemTheElements.H"
#include "inv/ChemKit/ChemCovRadius.H"

ChemPDBImporter::ChemPDBImporter()
{
}

ChemPDBImporter::~ChemPDBImporter()
{
}

bool ChemPDBImporter::openFile (const char* filename, ChemData* chemdata, ChemAssociatedData* chemassociateddata) const
{
  int i,j;
  char s[255];
  char label[80];
  int32_t numAtoms, numBonds;

  SbStringList resList;
  SbStringList asciiInfo;

  FILE *fp = fopen(filename,"r");

  if (fp==0) return false;

  numAtoms=0;
  while (!feof(fp))
  {
	fgets(s,255,fp);
	if (strlen(s)==0) continue;
	if (strncmp(s,"ATOM",4)==0 || strncmp(s,"HETATM",6)==0) numAtoms++;

	if (strncmp(s,"HELIX",5)==0 || strncmp(s,"TURN",4)==0 || strncmp(s,"SHEET",5)==0)
	{
		resList.append(new SbString(s));
		continue;
	}
  }

// --> change initial bond space allocation
//  numBonds=numAtoms*4;
  numBonds=(numAtoms+1)*4;
// <-- change initial bond space allocation

  rewind(fp);

  chemdata->numberOfAtoms.setValue(numAtoms);

  // For fast filling of the chemData fields, first set the size of the
  // ChemData fields.
  chemdata->atomicNumber.setNum(numAtoms);
  chemdata->atomId.setNum(numAtoms);
  chemdata->atomName.setNum(numAtoms);
  chemdata->atomIndex.setNum(numAtoms);
  chemdata->atomCoordinates.setNum(numAtoms);
  chemassociateddata->asciiData.setNum(numAtoms);
  chemassociateddata->binaryData.setNum(numAtoms);

  // Then "startEditing()" the fields
  short    *atomicNumber    = chemdata->atomicNumber.startEditing();
  int32_t  *atomId          = chemdata->atomId.startEditing();
  SbString *atomName        = chemdata->atomName.startEditing();
  int32_t  *atomIndex       = chemdata->atomIndex.startEditing();
  SbVec3f  *atomCoordinates = chemdata->atomCoordinates.startEditing();
  SbString *asciiData       = chemassociateddata->asciiData.startEditing();
 
  float *x  = new float [numAtoms];
  float *y  = new float [numAtoms];
  float *z  = new float [numAtoms];
  float *t  = new float [numAtoms];
  short *at = new short [numAtoms];
  float *xptr  = &x[0];
  float *yptr  = &y[0];
  float *zptr  = &z[0];
  float *tptr  = &t[0];
  short *atptr = &at[0];
  
  //int numsec = 0;

  i = 0;
  while (!feof(fp))
  {
     fgets(s,255,fp);

     if (strlen(s)==0) continue;
	 			 
	 if (strncmp(s,"ATOM",4)!=0 && strncmp(s,"HETATM",6)!=0)
		 continue;

     parseATOMLine(s, (*atptr), (*xptr), (*yptr), (*zptr), (*tptr), label);

     asciiInfo.append(new SbString(label));

     atomicNumber[i]    = (*atptr);
     atomIndex[i]       = (*atptr);
     atomId[i]          = i+1;
     atomName[i]        = theElements[(*atptr)];
     atomCoordinates[i] = SbVec3f((*xptr), (*yptr), (*zptr));
     asciiData [i]      = SbString (label);
     xptr++;
     yptr++;
     zptr++;
     tptr++;
     atptr++;
     i++;
  }
  fclose(fp);

  chemdata->atomicNumber.finishEditing();
  chemdata->atomId.finishEditing();
  chemdata->atomName.finishEditing();
  chemdata->atomIndex.finishEditing();
  chemdata->atomCoordinates.finishEditing();
  chemassociateddata->asciiData.finishEditing();

  // Thermic info must be scaled in order to put then as uint_8 
  float thermax=-1000.0f;
  float thermin= 1000.0f;
  
  tptr = &t[0];
  for (i=0;i<numAtoms;i++)
  {
      if ((*tptr)>thermax) thermax=(*tptr);
      if ((*tptr)<thermin) thermin=(*tptr);
	  tptr++;
  }

  float thercoef = 0;
  if (thermax-thermin != 0.0f)
	thercoef=255.0f/(thermax-thermin);
                                                                                
  uint8_t  *binaryData = chemassociateddata->binaryData.startEditing();
  tptr = &t[0];
  for (i=0;i<numAtoms;i++)
  {
      binaryData[i]    = (uint8_t) (((*tptr)-thermin)*thercoef);
	  tptr++;
  }
  chemassociateddata->binaryData.finishEditing();

  // Start processing bonds
  int * bf = new int [numBonds];
  int * bt = new int [numBonds];
  int * bi = new int [numBonds];
  int * by = new int [numBonds];

  i=0;
  float Xdif,Ydif,Zdif,dist,cutoff;
  for(int k=0; k<numAtoms; k++)
  {
     for(int j=k+1; j<numAtoms; j++)
	 {
                                                                                                                  
       // if differences in a certain axis are larger than 4.0 A don't compute sqrt!
                                                                                                                  
       Xdif=x[k]-x[j]; if (  (fabs(Xdif)>AXIAL_CUTOFF)  ) continue;
       Ydif=y[k]-y[j]; if (  (fabs(Ydif)>AXIAL_CUTOFF)  ) continue;
       Zdif=z[k]-z[j]; if (  (fabs(Zdif)>AXIAL_CUTOFF)  ) continue;
                                                                                                                  
       dist = sqrtf( (Xdif)*(Xdif) +
                    (Ydif)*(Ydif) +
                    (Zdif)*(Zdif) );
                                                                                                                  
       cutoff = 1.2f * (covRadius[at[j]]+covRadius[at[k]]);

       if (dist<cutoff) { //Is this a bond? (according atom distance)
           bf[i]  = k;
           bt[i]  = j;
           bi[i]  = i;
           by[i]  = 1;
           i++;

// --> bond dynamic allocation
//		   assert (i<numBonds);
		   if (i==numBonds)
		   {
				int* obf = bf;
				int* obt = bt;
				int* obi = bi;
				int* oby = by;
				int * nbf = new int [numBonds+1000];
				int * nbt = new int [numBonds+1000];
				int * nbi = new int [numBonds+1000];
				int * nby = new int [numBonds+1000];
				memcpy(nbf,bf,i*sizeof(int));
				memcpy(nbt,bt,i*sizeof(int));
				memcpy(nbi,bi,i*sizeof(int));
				memcpy(nby,by,i*sizeof(int));
				numBonds += 1000;
				bf = nbf;
				bt = nbt;
				bi = nbi;
				by = nby;
				delete [] obf;
				delete [] obt;
				delete [] obi;
				delete [] oby;
		   }
// <-- bond dynamic allocation
       }
     }
  }
  delete [] x;
  delete [] y;
  delete [] z;
  delete [] at;

  numBonds = i;

  chemdata->numberOfBonds.setValue(numBonds);
                                                                                                                            
  chemdata->bondFrom.setNum(numBonds);
  chemdata->bondTo.setNum(numBonds);
  chemdata->bondType.setNum(numBonds);
  chemdata->bondIndex.setNum(numBonds);
                                                                                                                            
  int32_t  *bondFrom        = chemdata->bondFrom.startEditing();
  int32_t  *bondTo          = chemdata->bondTo.startEditing();
  int32_t  *bondType        = chemdata->bondType.startEditing();
  int32_t  *bondIndex       = chemdata->bondIndex.startEditing();

  for (i=0;i<numBonds;i++) {
       bondFrom [i]  = bf[i];
       bondTo   [i]  = bt[i];
       bondIndex[i]  = bi[i];
       bondType [i]  = by[i];
  }

  chemdata->bondFrom.finishEditing();
  chemdata->bondTo.finishEditing();
  chemdata->bondType.finishEditing();
  chemdata->bondIndex.finishEditing();

  delete [] bf;
  delete [] bt;
  delete [] bi;
  delete [] by;


  // parse residues

  SbString line,resname,atomname,chain;
  int id=-1,numberId=0;
  for (i=0;i<numAtoms;i++)
  {
	  line = *asciiInfo[i];
	  resname = line.getSubString(8,10); 
	  chain = line.getSubString(11,12);
	  atomname = line.getSubString(4,7);
	  numberId = atoi(line.getSubString(line.getLength()-3).getString());

	  if (id>=0)//avoid silly searching, check old resId
	  {
		  if (!(chemdata->residues[id].getNumber()==numberId &&
			  chemdata->residues[id].getChain()==chain.getString()[0]))
			id=-1;
	  }
	  
	  if (id<0)
	  {
		  for (j=0;j<chemdata->residues.getNum();j++)
		  {
			  if (chemdata->residues[j].getNumber()==numberId &&
				  chemdata->residues[j].getChain()==chain.getString()[0])
			  {
				  id=j;
				  break;
			  }
		  }
	  }

	  if (id==-1) //not found
	  {
		  SbResidue myResidue;
		  myResidue.setName(resname);
		  myResidue.setChain(chain.getString()[0]);
		  myResidue.setNumber(numberId);

		  short j;
		  for (j=0;j<_RESNUM;j++)
		  {
			  if (strncmp(stdName3[j],resname.getString(),3)==0)
			  {
				  myResidue.setIndex(j);
				  break;
			  }
		  }
		  chemdata->residues.set1Value(chemdata->residues.getNum(),myResidue);
		  id=chemdata->residues.getNum()-1;
	  }
		// store indexes of CA and O atoms (C3' and C4' in DNA)
	  if (atomname ==" CA "     ||
		atomname ==" C3*"     ||
		atomname =="CA  "     ||    // mol2
		atomname =="C3* "           // mol2
								   )
		{
			chemdata->residues[id].addAtomIndex(i,SbResidue::RESIDUE_CONTROL_POINT_CA);
		}
		else
		if (atomname ==" O  "     ||
		atomname ==" C4*"     ||
		atomname =="O   "     ||   // mol2
		atomname =="C4* "          // mol2
								   )
		{
			chemdata->residues[id].addAtomIndex(i,SbResidue::RESIDUE_CONTROL_POINT_O);
		}
		else
			chemdata->residues[id].addAtomIndex(i);

  }
  chemdata->numberOfResidues.setValue(chemdata->residues.getNum());

  struct residueRange
  {
	  char ichain;
	  int itype;
	  int ibegin;
	  int iend;
  };

  residueRange* ranges = 0;
  if (resList.getLength()>0)
	ranges = new residueRange[resList.getLength()];

  // assign types
  for (i=0;i<resList.getLength();i++)
  {
	  line = *resList[i];
	  if (strncmp(line.getString(),"HELIX",5)==0)
	  {
		ranges[i].ichain = line.getString()[19];
		ranges[i].itype = SbResidue::RESIDUE_TYPE_HELIX;
		ranges[i].ibegin = atoi(line.getSubString(21,24).getString());
		ranges[i].iend   = atoi(line.getSubString(33,36).getString());
	  }
	  else if (strncmp(line.getString(),"SHEET",5)==0)
	  {
		ranges[i].ichain = line.getString()[21];
		ranges[i].itype = SbResidue::RESIDUE_TYPE_SHEET;
		ranges[i].ibegin = atoi(line.getSubString(22,25).getString());
		ranges[i].iend   = atoi(line.getSubString(33,36).getString());
	  }
	  else if (strncmp(line.getString(),"TURN",4)==0)
	  {
		ranges[i].ichain = line.getString()[19];
		ranges[i].itype = SbResidue::RESIDUE_TYPE_TURN;
		ranges[i].ibegin = atoi(line.getSubString(15,18).getString());
		ranges[i].iend   = atoi(line.getSubString(26,29).getString());
	  }
  }

  for (i=0;i<chemdata->numberOfResidues.getValue();i++)
  {
	  for (j=0;j<resList.getLength();j++)
	  {
		    if (chemdata->residues[i].getChain() == ranges[j].ichain &&
				chemdata->residues[i].getNumber()>= ranges[j].ibegin &&
				chemdata->residues[i].getNumber()<  ranges[j].iend)
			{
				chemdata->residues[i].residueType.setValue(ranges[j].itype);
				break;
			}

	  }
  }

  if (ranges!=0)
  {
	  delete[] ranges;
	  ranges=0;
  }

  // fill residue names table
  chemdata->residueName.setNum(_RESNUM);
  for (i=0;i<_RESNUM;i++)
	  chemdata->residueName.set1Value(i,stdName3[i]);

  //chemdata->residueName.setValues(0,_RESNUM,(const SbString*)stdName3);crash?

  // fill indices
  chemdata->residueColorIndex.setNum(_RESNUM);
  for (i=0;i<_RESNUM;i++)
	  chemdata->residueColorIndex.set1Value(i,i);
  
  return true;
}


//! Parses a single line of the PDB file
/*! Parses a PDB line which contains keywords ATOM  
    or HETATM. Looks for the atom number, the atom 
    cartesian coordinates and the AA label in the 
    case of a protein with alfa carbons. Also will
    label the WAT or HOH types. It could be enlarged
    to recognize other residue types.
*/
void ChemPDBImporter::parseATOMLine (char* s, short &atnum, float &x, float &y, float &z, float &therm, char* Label) const
{
  char  line[200]; 
  char  AALabel[20];
  char  atom[3];
  char  t[10];
  char  *sp;
  char  l12,l13;
  int   ilen;

  strncpy (line, s, 199);

  if ((strlen(line)) > 78) {
     if(line[76]==' ') {
        atom[0] = line[77];
        atom[1] = '\0';
     }
     else {
        atom[0] = line[76];
        if(line[76] == 'H' || line[76] == 'D') {
          atom[1]='\0';               // Hidrogen or deuterium are not right justified
        }
        else {
          atom[1] = line[77];
          atom[2] = '\0';
        }
     }

     for (ilen = 1; ilen < 104; ilen++) {
        if (!strcmp (atom, theElements[ilen] )) { atnum = ilen; break;}
     }
     if(!strcmp (atom, "D")) {
        atnum = 1;
        ilen  = 1;
     } //It's a Deuterium

  }
  else { 
     ilen = 104;
  } // If length of line is less than 78 looks for atom type in position 13-14

  if (ilen == 104) {
     l12 = line[12];
     l13 = line[13];
     if(l12==' ') {
        atom[0] = l13;
        atom[1] = '\0';
     } 
     else {
        atom[0] = l12;
             if(l12 == 'H' || l12 == 'D') {
               atom[1]='\0';
        } 
        else if (l12 == '1' || l12 == '2' || l12 == '3') {
               atom[0] = l13;
               atom[1] = '\0';
        }
        else {
               atom[1] = l13;
               atom[2] = '\0';
        }
     }
     for (ilen = 1; ilen < 104; ilen++) {
        if (!strcmp (atom, theElements[ilen] )) { 
           atnum = ilen; 
           break;
        } 
     }

     if(!strcmp (atom, "D")) {
        atnum = 1; 
        ilen = 1; 
     } // It's a Deuterium

     if (ilen==104) {
        atnum = 6;
     }  // Default Value is Carbon
  }
     
  // atom number in column 8-11
  AALabel[0] = line[7];
  AALabel[1] = line[8];
  AALabel[2] = line[9];
  AALabel[3] = line[10];

  // atom name in column 13-15
  AALabel[4] = line[12];
  AALabel[5] = line[13];
  AALabel[6] = line[14];
  AALabel[7] = line[15];

  // res name in column 18-20
  AALabel[8] = line[17];
  AALabel[9] = line[18];
  AALabel[10] = line[19];

  // chain in column 22
  AALabel[11] = line[21];

  // res number in column 24-26
  AALabel[12] = line[23];
  AALabel[13] = line[24];
  AALabel[14] = line[25];

  // end
  AALabel[15] = '\0';

  strcpy(Label,AALabel);

  // Gets the coordinates
  sp=line+30; strncpy( t, sp, 9 ); t[8]='\0';
  x = atof(t);
  sp=line+38; strncpy( t, sp, 9 ); t[8]='\0';
  y = atof(t);
  sp=line+46; strncpy( t, sp, 9 ); t[8]='\0';
  z = atof(t);

  // Gets the Thermal factor, here some programs can put chemiometric properties
  // This is stored in positions 61-66 as a float 6.2 (PDB Contents Guide v.2.1)
  if (strlen(line)>65) {
     sp=line+60; strncpy (t, sp, 6); t[6]='\0';
     therm=atof(t);
  }
  else {
     therm=0.0f;
  }
}
