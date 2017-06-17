///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: NWChemDesc.C
//
//
// DESIGN:
//   
//
///////////////////////////////////////////////////////////////////////////////

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
  using std::cout;
  using std::endl;
  using std::ends;

#include <string>
  using std::string;

#include "util/STLUtil.H"
#include "util/StringTokenizer.H"

#include "tdat/FFItem.H"
#include "tdat/FFCategory.H"
#include "tdat/FFRules.H"
#include "tdat/ShapeData.H"
#include "tdat/TResTab.H"
#include "tdat/Residue.H"

#include "dsm/NWChemDesc.H"
#include "dsm/FFModel.H"

static const int NAME_END = 23;   // where 23 is size of name space
static const int FIRST_END = 33;  // NAME_END + 10  where 10 is size of FIRST
static const int SECOND_END = 45; // FIRST_END + 12 where 12 is size of SECOND
static const int THIRD_END = 50;  // SECOND_END + 5 where 5 is size of THIRD

// -----------------------
// Public Member Functions
// -----------------------

// ---------- Constructor ------------
NWChemDesc::NWChemDesc(void)
{
  setFFRulesName("NWChem");
}

// ---------- Virtual Destructor ------------
NWChemDesc::~NWChemDesc(void)
{
}

// ---------- Methods ----------------
////////////////////////////////////////////////////////////////////////////////
//  Description:
//    Method for creating a force field parameter file
////////////////////////////////////////////////////////////////////////////////
void NWChemDesc::serialize(const Serializable &sModel, string &output,
                           EcceMap &properties)
{
  // Hardwire the header information for now. We will need to pick this up from
  // the GUI at some point.

  FFModel &model = (FFModel&)sModel;
  output.clear();
  ostrstream ofile;
  ofile <<"#"<< endl;
  ofile <<"# This is the AMBER96 user defined parameter file for NWChem"<< endl;
  ofile <<"#"<< endl;
  ofile <<"Electrostatic 1-4 scaling factor     0.833333" << endl;
  ofile <<"Relative dielectric constant     1.000000" << endl;
  ofile <<"Parameters epsilon R*" << endl;
  vector<int> sources = model.selectedSources();
  exportFFAtoms(model,sources,ofile);
  exportFFBonds(model,sources,ofile);
  exportFFCross(model,sources,ofile);
  exportFFAngles(model,sources,ofile);
  exportFFTorsions(model,sources,ofile);
  exportFFImproper(model,sources,ofile);
  ofile << ends;
  output = ofile.str();
}
////////////////////////////////////////////////////////////////////////////////
//  Description:
//    Temporary implementation of other methods in Serializer
////////////////////////////////////////////////////////////////////////////////
void NWChemDesc::deserialize(const string &input, Serializable &model,
  EcceMap &map) throw(ParseException)
{}
string NWChemDesc::mimetype() const
{
   return "x/application-nwamber";
}

////////////////////////////////////////////////////////////////////////////////
//  Description:
//    Export the atom parameters
////////////////////////////////////////////////////////////////////////////////
void NWChemDesc::exportFFAtoms(const FFModel &model, vector<int> sources,
                               ostrstream &ofile) const
{
  int ncat = model.getCategoryCount();
  int icat;
  char buf[120];
  char *cptr;
  string param, atype;
  FFItem *item;
  int i,j;
  for (icat=0; icat<ncat; icat++) {
    if (model.getCategoryLabel(icat) == "Atoms") {
      ofile << "Atoms" <<endl;
      vector<FFItem*> items;
      vector<string> atypes;
      model.getFFItems(icat,sources,atypes,items);
      int nitems = atypes.size();
      int atnum;
      double mass, epsilon, sigma;
      for (i=0; i<nitems; i++) {
        item = items[i];
        atype = atypes[i];
        // Assemble first line of output
        cptr = buf;
        int length = atype.length();
        for (j=0; j<length; j++) {
          cptr[j] = atype[j];
        }
        cptr += length;
        for (j=length; j<6; j++) {
          *cptr = ' ';
          cptr++;
        }
        mass = atof(item->getParam(0).c_str());
        epsilon = atof(item->getParam(1).c_str());
        sigma = atof(item->getParam(2).c_str());
        sprintf(cptr,"%9.5f %7.5e %7.5e",mass,epsilon,sigma);
        cptr += 33;
        strncpy(cptr,"                            1 1111111111",40);
        cptr += 40;
        *cptr = '\0';
        ofile << buf << endl;
        // Assemble second line of output
        cptr = buf;
        strncpy(cptr,"      ",6);
        cptr += 6;
        if (item->getParam(4) == "yes") {
          *cptr = 'Q';
        } else {
          *cptr = ' ';
        }
        cptr++;
        atnum = atoi(item->getParam(3).c_str());
        sprintf(cptr,"%8d %7.5e %7.5e",atnum,epsilon,sigma);
        cptr += 32;
        strncpy(cptr,"                              ",30);
        cptr += 30;
        param = item->getAnnotation();
        if (param.length() > 0) strcpy(cptr,param.c_str());
        else *cptr = '\0';
        ofile << buf << endl;
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
//  Description:
//    Export the cross interaction parameters
////////////////////////////////////////////////////////////////////////////////
void NWChemDesc::exportFFCross(const FFModel &model, vector<int> sources,
                               ostrstream &ofile) const
{
  int ncat = model.getCategoryCount();
  int icat;
  char buf[120];
  char *cptr;
  string param, atype;
  FFItem *item;
  int i,j;
  for (icat=0; icat<ncat; icat++) {
    if (model.getCategoryLabel(icat) == "Cross") {
      ofile << "Cross" <<endl;
      vector<FFItem*> items;
      vector<string> atypes;
      model.getFFItems(icat,sources,atypes,items);
      int nitems = atypes.size();
      string a1, a2;
      double epsilon, sigma;
      for (i=0; i<nitems; i++) {
        atype = atypes[i];
        item = items[i];
        // Parse atype to get individual atom types
        int length = atype.length();
        int icnt = 0;
        a1.clear();
        a2.clear();
        for (j=0; j<length; j++) {
          if (icnt == 0 && atype[j] != '-') {
            a1.append(1,atype[j]);
          } else if (atype[j] == '-') {
            icnt++;
          } else if (icnt == 1 && atype[j] != '-') {
            a2.append(1,atype[j]);
          }
        }
        // Assemble first line of output
        cptr = buf;
        length = a1.length();
        for (j=0; j<length; j++) {
          cptr[j] = a1[j];
        }
        cptr += length;
        for (j=length; j<6; j++) {
          *cptr = ' ';
          cptr++;
        }
        length = a2.length();
        for (j=0; j<length; j++) {
          cptr[j] = a2[j];
        }
        cptr += length;
        for (j=length; j<8; j++) {
          *cptr = ' ';
          cptr++;
        }
        epsilon = atof(item->getParam(0).c_str());
        sigma = atof(item->getParam(1).c_str());
        sprintf(cptr,"%7.5e %7.5e",epsilon,sigma);
        cptr += 23;
        *cptr = '\0';
        ofile << buf << endl;
        cptr = buf;
        strncpy(cptr,"              ",14);
        cptr += 14;
        sprintf(cptr,"%7.5e %7.5e",epsilon,sigma);
        cptr += 23;
        strncpy(cptr,"                                ",32);
        cptr += 32;
        param = item->getAnnotation();
        if (param.length() > 0) strcpy(cptr,param.c_str());
        else *cptr = '\0';
        ofile << buf << endl;
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
//  Description:
//    Export the bond parameters
////////////////////////////////////////////////////////////////////////////////
void NWChemDesc::exportFFBonds(const FFModel &model, vector<int> sources,
                               ostrstream &ofile) const
{
  int ncat = model.getCategoryCount();
  int icat;
  char buf[120];
  char *cptr;
  string param, atype;
  FFItem *item;
  int i,j;
  for (icat=0; icat<ncat; icat++) {
    if (model.getCategoryLabel(icat) == "Bonds") {
      ofile << "Bonds" <<endl;
      vector<FFItem*> items;
      vector<string> atypes;
      model.getFFItems(icat,sources,atypes,items);
      int nitems = atypes.size();
      string a1, a2;
      double fconst, req, dipole;
      for (i=0; i<nitems; i++) {
        atype = atypes[i];
        item = items[i];
        // Parse atype to get individual atom types
        int length = atype.length();
        int icnt = 0;
        a1.clear();
        a2.clear();
        for (j=0; j<length; j++) {
          if (icnt == 0 && atype[j] != '-') {
            a1.append(1,atype[j]);
          } else if (atype[j] == '-') {
            icnt++;
          } else if (icnt == 1 && atype[j] != '-') {
            a2.append(1,atype[j]);
          }
        }
        // Assemble first line of output
        cptr = buf;
        length = a1.length();
        for (j=0; j<length; j++) {
          cptr[j] = a1[j];
        }
        cptr += length;
        for (j=length; j<5; j++) {
          *cptr = ' ';
          cptr++;
        }
        *cptr = '-';
        cptr++;
        length = a2.length();
        for (j=0; j<length; j++) {
          cptr[j] = a2[j];
        }
        cptr += length;
        for (j=length; j<8; j++) {
          *cptr = ' ';
          cptr++;
        }
        req = atof(item->getParam(0).c_str());
        fconst = atof(item->getParam(1).c_str());
        dipole = atof(item->getParam(2).c_str());
        sprintf(cptr,"%7.5f %7.5e",req,fconst);
        cptr += 19;
        if (dipole != 0.0) {
          sprintf(cptr,"%12.6f",dipole);
        } else {
          strncpy(cptr,"            ",12);
        }
        cptr += 12;
        strncpy(cptr,"             ",13);
        cptr += 13;
        param = item->getAnnotation();
        if (param.length() > 0) strcpy(cptr,param.c_str());
        else *cptr = '\0';
        ofile << buf << endl;
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
//  Description:
//    Export the angle parameters
////////////////////////////////////////////////////////////////////////////////
void NWChemDesc::exportFFAngles(const FFModel &model, vector<int> sources,
                                ostrstream &ofile) const
{
  int ncat = model.getCategoryCount();
  int icat;
  char buf[120];
  char *cptr;
  string param, atype;
  FFItem *item;
  int i,j;
  for (icat=0; icat<ncat; icat++) {
    if (model.getCategoryLabel(icat) == "Angles") {
      ofile << "Angles" <<endl;
      vector<FFItem*> items;
      vector<string> atypes;
      model.getFFItems(icat,sources,atypes,items);
      int nitems = atypes.size();
      string a1, a2, a3;
      double  fconst, aeq;
      for (i=0; i<nitems; i++) {
        atype = atypes[i];
        item = items[i];
        // Parse atype to get individual atom types
        int length = atype.length();
        int icnt = 0;
        a1.clear();
        a2.clear();
        a3.clear();
        for (j=0; j<length; j++) {
          if (icnt == 0 && atype[j] != '-') {
            a1.append(1,atype[j]);
          } else if (icnt == 1 && atype[j] != '-') {
            a2.append(1,atype[j]);
          } else if (icnt == 2 && atype[j] != '-') {
            a3.append(1,atype[j]);
          } else if (atype[j] == '-') {
            icnt++;
          }
        }
        // Assemble first line of output
        cptr = buf;
        length = a1.length();
        for (j=0; j<length; j++) {
          cptr[j] = a1[j];
        }
        cptr += length;
        for (j=length; j<5; j++) {
          *cptr = ' ';
          cptr++;
        }
        *cptr = '-';
        cptr++;
        length = a2.length();
        for (j=0; j<length; j++) {
          cptr[j] = a2[j];
        }
        cptr += length;
        for (j=length; j<5; j++) {
          *cptr = ' ';
          cptr++;
        }
        *cptr = '-';
        cptr++;
        length = a3.length();
        for (j=0; j<length; j++) {
          cptr[j] = a3[j];
        }
        cptr += length;
        for (j=length; j<8; j++) {
          *cptr = ' ';
          cptr++;
        }
        aeq = atof(item->getParam(0).c_str());
        fconst = atof(item->getParam(1).c_str());
        sprintf(cptr,"%7.5f %7.5e",aeq,fconst);
        cptr += 19;
        *cptr = ' ';
        cptr++;
        param = item->getAnnotation();
        if (param.length() > 0) strcpy(cptr,param.c_str());
        else *cptr = '\0';
        ofile << buf << endl;
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
//  Description:
//    Export the torsion parameters
////////////////////////////////////////////////////////////////////////////////
void NWChemDesc::exportFFTorsions(const FFModel &model, vector<int> sources,
                                  ostrstream &ofile) const
{
  int ncat = model.getCategoryCount();
  int icat;
  char buf[120], tag[60];
  char *cptr;
  string param, atype;
  FFItem *item;
  int i,j, npar;
  int nwild;
  for (icat=0; icat<ncat; icat++) {
    if (model.getCategoryLabel(icat) == "Torsions") {
      ofile << "Proper dihedrals" <<endl;
      vector<FFItem*> items;
      vector<string> atypes;
      model.getFFItems(icat,sources,atypes,items);
      int nitems = atypes.size();
      string a1, a2, a3, a4;
      double  fconst, phi_eq;
      int iconst;
      for (nwild=2; nwild<=4; nwild++) {
        for (i=0; i<nitems; i++) {
          atype = atypes[i];
          item = items[i];
          // Parse atype to get individual atom types
          int length = atype.length();
          int icnt = 0;
          a1.clear();
          a2.clear();
          a3.clear();
          a4.clear();
          for (j=0; j<length; j++) {
            if (icnt == 0 && atype[j] != '-' && atype[j] != ' ') {
              a1.append(1,atype[j]);
            } else if (icnt == 1 && atype[j] != '-' && atype[j] != ' ') {
              a2.append(1,atype[j]);
            } else if (icnt == 2 && atype[j] != '-' && atype[j] != ' ') {
              a3.append(1,atype[j]);
            } else if (icnt == 3 && atype[j] != '-' && atype[j] != ' ') {
              a4.append(1,atype[j]);
            } else if (atype[j] == '-') {
              icnt++;
            }
          }
          // Check number of wild-card entries to see if this line should
          // be printed
          icnt = 0;
          if (!a1.empty()) icnt++;
          if (!a2.empty()) icnt++;
          if (!a3.empty()) icnt++;
          if (!a4.empty()) icnt++;
          if (icnt != nwild) continue;
          // Assemble dihedral name
          cptr = tag;
          length = a1.length();
          for (j=0; j<length; j++) {
            cptr[j] = a1[j];
          }
          cptr += length;
          for (j=length; j<5; j++) {
            *cptr = ' ';
            cptr++;
          }
          *cptr = '-';
          cptr++;
          length = a2.length();
          for (j=0; j<length; j++) {
            cptr[j] = a2[j];
          }
          cptr += length;
          for (j=length; j<5; j++) {
            *cptr = ' ';
            cptr++;
          }
          *cptr = '-';
          cptr++;
          length = a3.length();
          for (j=0; j<length; j++) {
            cptr[j] = a3[j];
          }
          cptr += length;
          for (j=length; j<5; j++) {
            *cptr = ' ';
            cptr++;
          }
          *cptr = '-';
          cptr++;
          length = a4.length();
          for (j=0; j<length; j++) {
            cptr[j] = a4[j];
          }
          cptr += length;
          for (j=length; j<8; j++) {
            *cptr = ' ';
            cptr++;
          }
          // construct individual lines
          npar =  item->getParamCount();
          npar /= 3;
          for (j=0; j<npar; j++) {
            cptr = buf;
            strncpy(cptr,tag,26);
            cptr += 26;
            phi_eq = atof(item->getParam(j*3).c_str());
            fconst = atof(item->getParam(j*3+1).c_str());
            iconst = atoi(item->getParam(j*3+2).c_str());
            if (j<npar-1) iconst = -iconst;
            if (fconst >= 0) {
              sprintf(cptr,"%7.5f %7.5e %4d",phi_eq,fconst,iconst);
            } else {
              sprintf(cptr,"%7.5f%8.5e %4d",phi_eq,fconst,iconst);
            }
            cptr += 24;
            *cptr = ' ';
            cptr++;
            param = item->getAnnotation();
            if (param.length() > 0) strcpy(cptr,param.c_str());
            else *cptr = '\0';
            ofile << buf << endl;
          }
        }
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
//  Description:
//    Export the improper torsion parameters
////////////////////////////////////////////////////////////////////////////////
void NWChemDesc::exportFFImproper(const FFModel &model, vector<int> sources,
                                  ostrstream &ofile) const
{
  int ncat = model.getCategoryCount();
  int icat;
  char buf[120], tag[60];
  char *cptr;
  string param, atype;
  FFItem *item;
  int i,j, npar;
  int nwild;
  for (icat=0; icat<ncat; icat++) {
    if (model.getCategoryLabel(icat) == "Improper") {
      ofile << "Improper dihedrals" <<endl;
      vector<FFItem*> items;
      vector<string> atypes;
      model.getFFItems(icat,sources,atypes,items);
      int nitems = atypes.size();
      string a1, a2, a3, a4;
      double  fconst, phi_eq;
      int iconst;
      for (nwild=2; nwild<=4; nwild++) {
        for (i=0; i<nitems; i++) {
          atype = atypes[i];
          item = items[i];
          // Parse atype to get individual atom types
          int length = atype.length();
          int icnt = 0;
          a1.clear();
          a2.clear();
          a3.clear();
          a4.clear();
          for (j=0; j<length; j++) {
            if (icnt == 0 && atype[j] != '-' && atype[j] != ' ') {
              a1.append(1,atype[j]);
            } else if (icnt == 1 && atype[j] != '-' && atype[j] != ' ') {
              a2.append(1,atype[j]);
            } else if (icnt == 2 && atype[j] != '-' && atype[j] != ' ') {
              a3.append(1,atype[j]);
            } else if (icnt == 3 && atype[j] != '-' && atype[j] != ' ') {
              a4.append(1,atype[j]);
            } else if (atype[j] == '-') {
              icnt++;
            }
          }
          // Check number of wild-card entries to see if this line should
          // be printed
          icnt = 0;
          if (!a1.empty()) icnt++;
          if (!a2.empty()) icnt++;
          if (!a3.empty()) icnt++;
          if (!a4.empty()) icnt++;
          if (icnt != nwild) continue;
          // Assemble dihedral name
          cptr = tag;
          length = a1.length();
          for (j=0; j<length; j++) {
            cptr[j] = a1[j];
          }
          cptr += length;
          for (j=length; j<5; j++) {
            *cptr = ' ';
            cptr++;
          }
          *cptr = '-';
          cptr++;
          length = a2.length();
          for (j=0; j<length; j++) {
            cptr[j] = a2[j];
          }
          cptr += length;
          for (j=length; j<5; j++) {
            *cptr = ' ';
            cptr++;
          }
          *cptr = '-';
          cptr++;
          length = a3.length();
          for (j=0; j<length; j++) {
            cptr[j] = a3[j];
          }
          cptr += length;
          for (j=length; j<5; j++) {
            *cptr = ' ';
            cptr++;
          }
          *cptr = '-';
          cptr++;
          length = a4.length();
          for (j=0; j<length; j++) {
            cptr[j] = a4[j];
          }
          cptr += length;
          for (j=length; j<8; j++) {
            *cptr = ' ';
            cptr++;
          }
          // construct individual lines
          npar =  item->getParamCount();
          npar /= 3;
          for (j=0; j<npar; j++) {
            cptr = buf;
            strncpy(cptr,tag,26);
            cptr += 26;
            phi_eq = atof(item->getParam(j*3).c_str());
            fconst = atof(item->getParam(j*3+1).c_str());
            iconst = atoi(item->getParam(j*3+2).c_str());
            if (j<npar-1) iconst = -iconst;
            if (fconst >= 0) {
              sprintf(cptr,"%7.5f %7.5e %4d",phi_eq,fconst,iconst);
            } else {
              sprintf(cptr,"%7.5f%8.5e %4d",phi_eq,fconst,iconst);
            }
            cptr += 24;
            *cptr = ' ';
            cptr++;
            param = item->getAnnotation();
            if (param.length() > 0) strcpy(cptr,param.c_str());
            else *cptr = '\0';
            ofile << buf << endl;
          }
        }
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
//  Description:
//    Check to see if an atom center has improper torsions
////////////////////////////////////////////////////////////////////////////////
bool NWChemDesc::checkInversion(TAtm *atm, vector<TBond*> list) const
{
  int ienv = atm->dihedralIndex();
  if (ienv == 0) return false;
  int nbnd = list.size();
  if (nbnd != 3) return false;
  ShapeData::Shape shape = atm->shape();
  double obnd21, obnd31, obnd41;
  if (!(shape == ShapeData::pyramidal
      || shape == ShapeData::trigonalPlanar)) return false;
  // check to make sure that center atom is either in N or O column of the
  // periodic table for pyramidal geometries or that it is a C atom with
  // sp2 geometry.
  int atomNumber = atm->atomicNumber();
  if ((shape == ShapeData::pyramidal && (atomNumber == 7 ||
       atomNumber == 8 || atomNumber == 15 || atomNumber == 16 ||
       atomNumber == 33 || atomNumber == 34 || atomNumber == 51 ||
       atomNumber == 52 || atomNumber == 83 || atomNumber == 84)) ||
       (shape == ShapeData::trigonalPlanar && (atomNumber == 6 ||
       atomNumber == 7))) {
    // check to make sure that there are no bonds with bond order less than
    // 1.0
    obnd21 = list[0]->order();
    obnd31 = list[1]->order();
    obnd41 = list[2]->order();
    if (obnd21 < 0.99 || obnd31 < 0.99 || obnd41 < 0.99) return false;
//    if (atomNumber == 7) {
//      if (resTab.verify(atm->getResidue()->name().c_str())
//          == TResItem::AminoAcid) {
//        if (atm->atomicName() == "C") {
//          string res = atm->getResidue()->name();
//        }
//      }
//    }
    return true;
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////
//  Description:
//    Check atoms to find out which ones are part of rings. Only the inversions
//    for which the atom in the fourth position is not part of a ring are
//    considered.
////////////////////////////////////////////////////////////////////////////////
vector<bool> NWChemDesc::findInversions(TAtm *atm, vector<TBond*> list,
                                        vector<int> *ringAtoms) const
{
  TBond* bond;
  int iat, jat, kat, iat1, iat2, iat3;
  TAtm *jatm, *katm, *atm1, *atm2, *atm3;
  int terminal_count = 0;
  ShapeData::Shape shape;
  string atnam = atm->atomName();
  string atsym = atm->atomicSymbol();
  STLUtil::stripLeadingAndTrailingWhiteSpace(atnam);
  bool hasRings = true;

  if (ringAtoms == (vector<int>*)0 || ringAtoms->size() == 0)
    hasRings = false;

  TResTab resTab;
  bool protein_bb = false;
  if (resTab.verify(atm->getResidue()->name().c_str())
          == TResItem::AminoAcid) {
     if (atnam == "C" || atnam == "N" || atnam == "CA") protein_bb = true;
  }

  iat = atm->index();
//  cout <<"Atom: "<<iat<<" Sym: "<<atsym<<" Name: "<<atnam<<" Ring: "
//    << (*ringAtoms)[iat] <<endl;
  bond = list[0];
  jatm = bond->atom1();
  katm = bond->atom2();
  jat = jatm->index();
  kat = katm->index();
  if (jat == iat) {
    iat1 = kat;
    atm1 = katm;
  } else {
    iat1 = jat;
    atm1 = jatm;
  }
  shape = atm1->shape();
  if (shape == ShapeData::terminal) terminal_count++;

  bond = list[1];
  jatm = bond->atom1();
  katm = bond->atom2();
  jat = jatm->index();
  kat = katm->index();
  if (jat == iat) {
    iat2 = kat;
    atm2 = katm;
  } else {
    iat2 = jat;
    atm2 = jatm;
  }
  shape = atm2->shape();
  if (shape == ShapeData::terminal) terminal_count++;

  bond = list[2];
  jatm = bond->atom1();
  katm = bond->atom2();
  jat = jatm->index();
  kat = katm->index();
  if (jat == iat) {
    iat3 = kat;
    atm3 = katm;
  } else {
    iat3 = jat;
    atm3 = jatm;
  }
  shape = atm3->shape();
  if (shape == ShapeData::terminal) terminal_count++;

  // The rules for assigning improper torsions:
  //   If central atom is in a ring and central atom is not
  //   part of protein backbone:
  //     *If central atom is carbon and there is only one terminal
  //      atom hanging off of it, then central atom and terminal
  //      atom form an improper torsion
  //     *If atom is a nitrogen and there are two hydrogens or one
  //      terminal atom hanging off of it then central atom forms
  //      two improper torsions with the hydrogens or one improper
  //      torsion with the terminal atom
  //     *If there is only one terminal atom then it forms an
  //      improper torsion
  //   If the central atom is part of a protein backbone:
  //     *If central atom is named "C" or "N" then it forms an
  //      improper torsion with "O" or "H", respectively. This is
  //      not true if "C" is bonded to two oxygens, in this case
  //      (terminal carboxylic acid) no improper torsions are on the
  //      carbon.
  //   If the central atom is part of a ring
  //     *If the central atom attached to a terminal atom or an atom
  //      is non-planar it forms an improper torsion with the
  //      attached atom.
  //     
  vector<bool> ret;
  shape = atm1->shape();
  if ((hasRings && (*ringAtoms)[iat] == 0) && !protein_bb) {
    string at1sym = atm1->atomicSymbol();
    if (atsym == "C") {
      if (terminal_count > 1)
        ret.push_back(false);
      else if (shape == ShapeData::terminal)
        ret.push_back(true);
      else
        ret.push_back(false);
    } else if (atsym == "N") {
      if ((terminal_count > 1 && at1sym == "H") ||
          (terminal_count == 1 && shape == ShapeData::terminal))
        ret.push_back(true);
      else
        ret.push_back(false);
    } else {
      if (terminal_count == 1 && shape == ShapeData::terminal)
        ret.push_back(true);
      else
        ret.push_back(false);
    }
  } else if (protein_bb) {
    string at1nam = atm1->atomName();
    STLUtil::stripLeadingAndTrailingWhiteSpace(at1nam);
    if (atnam == "C" && at1nam == "O" && terminal_count == 1) {
      ret.push_back(true);
    } else if (atnam == "N" && at1nam == "H") {
      ret.push_back(true);
    } else {
      ret.push_back(false);
    }
  } else if ((hasRings && (*ringAtoms)[iat1] == 0)
      || shape == ShapeData::tetrahedral || shape == ShapeData::pyramidal) {
    ret.push_back(true);
  } else {
    ret.push_back(false);
  }

  shape = atm2->shape();
  if ((hasRings && (*ringAtoms)[iat] == 0) && !protein_bb) {
    string at2sym = atm2->atomicSymbol();
    if (atsym == "C") {
      if (terminal_count > 1)
        ret.push_back(false);
      else if (shape == ShapeData::terminal)
        ret.push_back(true);
      else
        ret.push_back(false);
    } else if (atsym == "N") {
      if ((terminal_count > 1 && at2sym == "H") ||
          (terminal_count == 1 && shape == ShapeData::terminal))
        ret.push_back(true);
      else
        ret.push_back(false);
    } else {
      if (terminal_count == 1 && shape == ShapeData::terminal)
        ret.push_back(true);
      else
        ret.push_back(false);
    }
  } else if (protein_bb) {
    string at2nam = atm2->atomName();
    STLUtil::stripLeadingAndTrailingWhiteSpace(at2nam);
    if (atnam == "C" && at2nam == "O" && terminal_count == 1) {
      ret.push_back(true);
    } else if (atnam == "N" && at2nam == "H") {
      ret.push_back(true);
    } else {
      ret.push_back(false);
    }
  } else if ((hasRings && (*ringAtoms)[iat2] == 0)
      || shape == ShapeData::tetrahedral || shape == ShapeData::pyramidal) {
    ret.push_back(true);
  } else {
    ret.push_back(false);
  }

  shape = atm3->shape();
  if ((hasRings && (*ringAtoms)[iat] == 0) && !protein_bb) {
    string at3sym = atm3->atomicSymbol();
    if (atsym == "C") {
      if (terminal_count > 1)
        ret.push_back(false);
      else if (shape == ShapeData::terminal)
        ret.push_back(true);
      else
        ret.push_back(false);
    } else if (atsym == "N") {
      if ((terminal_count > 1 && at3sym == "H") ||
          (terminal_count == 1 && shape == ShapeData::terminal))
        ret.push_back(true);
      else
        ret.push_back(false);
    } else {
      if (terminal_count == 1 && shape == ShapeData::terminal)
        ret.push_back(true);
      else
        ret.push_back(false);
    }
  } else if (protein_bb) {
    string at3nam = atm3->atomName();
    STLUtil::stripLeadingAndTrailingWhiteSpace(at3nam);
    if (atnam == "C" && at3nam == "O" && terminal_count == 1) {
      ret.push_back(true);
    } else if (atnam == "N" && at3nam == "H") {
      ret.push_back(true);
    } else {
      ret.push_back(false);
    }
  } else if ((hasRings && (*ringAtoms)[iat3] == 0)
      || shape == ShapeData::tetrahedral || shape == ShapeData::pyramidal) {
    ret.push_back(true);
  } else {
    ret.push_back(false);
  }

  return ret;
}


////////////////////////////////////////////////////////////////////////////////
//  Description:
//    Load parameters from the source files listed in sources
////////////////////////////////////////////////////////////////////////////////
void NWChemDesc::loadCategories(FFModel *model, vector<string>& sources)
                 throw(IOException)
{

   FFSources ffSources;
   ffSources.loadSources(sources);
   int sourceCount = ffSources.getSourceCount();
   string ioerrmsg;

   string sourcePath;
   string stopTag = "Atom";
   const char commentChar = '#';
   string electroVariable = "Electrostatic";
   string relatVariable = "Relative";
   string paramVariable = "Parameters";
   string electroValue;
   string relatValue;
   string param1Value, param2Value;

   FFCategoryDesc ffcatDesc;
   FFItem *ffitem = (FFItem*)0;
   FFCategory::eCategoryTypes  ffcat_type = FFCategory::INVALID_TYPE;
   int    ffcatID = FFCategory::INVALID_INDEX;
   int    fftypeID = FFType::INVALID_ID;

   const int BUFSIZE = 256;  // TODO confirm this covers all possibilities
   char buffer[BUFSIZE];
   char *tok, *ptr;
   unsigned long lineEnd = 0;
   bool postCommentFlag;

   for ( int sid=0;sid<sourceCount;sid++)  {

      sourcePath = ffSources.getSourceName( sid);
      const char * path = sourcePath.c_str();
      ifstream sfile( path);
      postCommentFlag = false;

      if ( sfile.good()) {                // continue only if file is good

         // get the next line in the file
         while ( sfile.getline( buffer, BUFSIZE))  {

            ptr = buffer;
            lineEnd = (unsigned long)buffer + strlen( buffer);
            fftypeID = FFType::INVALID_ID;

            // grab first non-blank character string
            if (( tok = strtok( ptr, " ")) != 0)  {  // skip blank lines

               //---------------------------------------------
               // look for end tag and break
               //---------------------------------------------
               if ( !strcmp( ptr, stopTag.c_str())) {
                  ffcat_type = FFCategory::INVALID_TYPE;
                  break;                      
               }
               //---------------------------------------------
               // ignore comment lines
               //---------------------------------------------
               else if ( ptr[0] == commentChar)  {
               }
               else if ( !strcmp( tok, electroVariable.c_str()))  {
                 // set flag indicating that we are passed any comments
                 postCommentFlag = true;

                 // test size of vector to make sure only add one per file
                 if ( model->numElectrostaticStrings() == sid) {
                   // Assumes more that one space delimited string in line
                    int len = strlen( tok);
                    ptr[len] = ' ';  // replace null inserted by strtok()
                    string dup(ptr);
                    StringTokenizer token(dup);
                    vector<string> tokens = token.tokenize(" \t");
                    if (tokens.size() > 4) {
                      if (!electroValue.empty()) {
                        if (STLUtil::compareString(tokens[4],electroValue)!=0) {
                          string errmsg = "A mismatch in the 1-4 electrostatic"
                                 " scaling parameter between successive"
                                 " parameter files was found. Check"
                                 " parameter file headers to make sure they"
                                 " have the same values for the electrostatic"
                                 " scaling parameter.";
                          throw IOException(errmsg, WHERE);
                        }
                      } else {
                        electroValue = tokens[4];
                      }
                    }
                    model->addElectrostaticString( ptr);
                 }
               }
               //---------------------------------------------
               // look for category name
               //---------------------------------------------
               else if (postCommentFlag) {
                  try  {
                     ffcatDesc = model->getDesc().getCategoryByName( string(tok));
                     // found one, put in reading category state
                     ffcatID = model->getCategoryID( ffcatDesc.getName());
                     ffcat_type = model->getCategoryType( ffcatDesc.getName());
                  }
                  catch( std::range_error& e)  {
                     // not a category type, ...
                     //---------------------------------------------
                     // look for variable start, put in reading variable state
                     //---------------------------------------------
                     if ( !strcmp( tok, relatVariable.c_str()))  {
                       // test size of vector to make sure only add one per file
                       if ( model->numRelativeStrings() == sid)  {
                         // Assumes more that one space delimited string in line
                          int len = strlen( tok);
                          ptr[len] = ' ';
                          string dup(ptr);
                          StringTokenizer token(dup);
                          vector<string> tokens = token.tokenize(" \t");
                          if (tokens.size() > 3) {
                            if (!relatValue.empty()) {
                              if (STLUtil::compareString(tokens[3],relatValue)!=0) {
                                string errmsg = "A mismatch in the dielectric"
                                       " constant between successive"
                                       " parameter files was found. Check"
                                       " parameter file headers to make sure"
                                       " they have the same values for the"
                                       " dielectric constant.";
                                throw IOException(errmsg,WHERE);
                              }
                            } else {
                              relatValue = tokens[3];
                            }
                          }
                          model->addRelativeString( ptr);
                       }
                     }
                     else if ( !strcmp( tok, paramVariable.c_str()))  {
                       // test size of vector to make sure only add one per file
                       if ( model->numParametersStrings() == sid)  {
                         // Assumes more that one space delimited string in line
                          int len = strlen( tok);
                          ptr[len] = ' ';
                          string dup(ptr);
                          StringTokenizer token(dup);
                          vector<string> tokens = token.tokenize(" \t");
                          if (tokens.size() > 2) {
                            if (!param1Value.empty() && !param2Value.empty()) {
                              if (STLUtil::compareString(tokens[1],param1Value) != 0
                                || STLUtil::compareString(tokens[2],param2Value) != 0) {
                                string errmsg = "A mismatch in the parameter "
                                       " types between successive"
                                       " parameter files was found. Check"
                                       " parameter file headers to make sure"
                                       " they have the same values for the"
                                       " parameter types.";
                                throw IOException(errmsg, WHERE);
                              }
                            } else {
                              param1Value = tokens[1];
                              param2Value = tokens[2];
                            }
                          }
                          model->addParametersString( ptr);
                       }
                     }
                     //---------------------------------------------
                     // Assume a data line for last tagged category
                     //---------------------------------------------
                     else if ( ffcat_type == FFCategory::ATOM_TYPE)  {
                        // capture the force field item
                        //------------------------------------------------
                        ffitem = parseNWChemAtomData(sfile, model, tok, BUFSIZE,
                                                     sid, ffcatID, &fftypeID);
                        //------------------------------------------------
                        model->getCategory(ffcatID)->addFFItem(fftypeID,ffitem);
                     }
                     else if ( ffcat_type == FFCategory::CROSS_TYPE)  {

                        // capture the force field item
                        //------------------------------------------------
                        ffitem = parseNWChemCrossData(sfile, model, tok, BUFSIZE, 
                                                      sid, ffcatID, &fftypeID);
                        // save it
                        //------------------------------------------------
                        model->getCategory(ffcatID)->addFFItem(fftypeID, ffitem);
                     }
                     else if ( ffcat_type == FFCategory::BOND_TYPE)  {
      
                       // capture the force field item and save it if successful
                       //------------------------------------------------
                        ffitem = parseNWChemBondData(tok, model, lineEnd, 
                                                     sid, ffcatID, &fftypeID);
      
                        model->getCategory(ffcatID)->addFFItem(fftypeID,ffitem);
                     }
                     else if ( ffcat_type == FFCategory::ANGLE_TYPE)  {

                       // capture the force field item and save it if successful
                       //------------------------------------------------
                        ffitem = parseNWChemAngleData(tok, model, lineEnd, 
                                                      sid, ffcatID, &fftypeID);

                        model->getCategory(ffcatID)->addFFItem(fftypeID,ffitem);
                     }
                     else if ( ffcat_type == FFCategory::PROPER_TYPE || 
                               ffcat_type == FFCategory::IMPROPER_TYPE)   {

                       // capture the force field item and save it if successful
                       //------------------------------------------------
                       ffitem = parseNWChemTorsionData(sfile, model, buffer, BUFSIZE,
                                                       sid, ffcatID, &fftypeID);
                        model->getCategory(ffcatID)->addFFItem(fftypeID,ffitem);
                     }
                  } // end of catch
               } // end of else not a comment or stop tagimproperCat
            } // end of not a blank to start the line
         } // end of while get a line
      }    // end if file is good
      else  {
        // Track errors but keep processing
        if (ioerrmsg.size() == 0) 
        ioerrmsg = "Unable to open parameter file(s): ";
        ioerrmsg += sourcePath + " ";
      }
      sfile.close();
   }  // end of processing this SFile

   // If any files failed to load, throw an exception and let caller decide
   // if it really matters
   if (ioerrmsg.size() > 0) {
      throw IOException(ioerrmsg, WHERE);
   }
}


////////////////////////////////////////////////////////////////////////////////
// Description:
//
////////////////////////////////////////////////////////////////////////////////
FFItem* NWChemDesc::parseNWChemAtomData( ifstream& sfile, FFModel *model,
                                         char* tok, const int BUFSIZE, 
                                         int sid, int catID, int*typeID)
{
   char* buffer = tok;  // remember pointer to buffer so can reuse
   FFItem *item = (FFItem*)0;

   // First tok is name of type. Use it to get type id 
   //------------------------------------------------
   *typeID = model->getCategory(catID)->getTypeID( tok, true);

   // capture the parameters - must match order in FFParameterDesc
   //------------------------------------------------
   if (( tok = strtok( 0, " ")) != 0) {   // get next string token
   
      item = new FFItem( sid);

      item->addParameter( tok);  // add mass as first  params

      // ASSUMES 2 parameters follow mass
      int count = 0;
      while (count < 2 && ( tok=strtok(0," ")) != 0)  {
         item->addParameter( tok);
         count++;
      }

      //------------------------------------------------
      // no annotation for the first line of atom parameters
      //------------------------------------------------

      // get second line of atom parameters
      //------------------------------------------------
      if ( sfile.getline( buffer, BUFSIZE)) {

         unsigned long lineEnd = (unsigned long)buffer + strlen( buffer);//remember the last address
         if (( tok = strtok( buffer, " ")) != 0) {

            // Check to see if first atom is supposed to be a quantum atom.
            //------------------------------------------------
            // Note quantum status for appending after symbolic name.
            bool quantum = false;
            if ( tok[0] == 'Q') {
               quantum = true;
               // advance to next token if found 'Q'
               tok = strtok( 0, " ");
            }

            //  retrieve next parameter (atomic number)
            //------------------------------------------------
            item->addParameter( tok);

            // now add quantum information to parameter set
            //------------------------------------------------
            if ( quantum == true)  {
               item->addParameter( "yes");// must be valid string for parameter
            }
            else {
               item->addParameter("no"); // must be valid string for parameter
            }

            // ASSUMES 2 parameters follow atomic number
            //------------------------------------------------
            count = 0;
            while ( count < 2 && (tok=strtok(0," ")) != 0)  {
               // ignore these two params,  ffitem->addParameter( tok);
               count++;
            }

            // capture annotation, if it exists
            //------------------------------------------------
            if ((tok=strtok(0," ")) != 0)  {
               // replace the null with a blank if the annotation has > one word
               if ( ((unsigned long)tok + strlen(tok)) < lineEnd)
                  tok[strlen(tok)] = ' ';
               item->setAnnotation( tok);
            }
         }  // endif found new token
         else  {
            delete item;
            item = (FFItem*)0;  // if second line is blank, don't want this item
         }
      }     // if got next line
      else  {
         delete item;
         item = (FFItem*)0; //if failed to get second line, don't want this item
      }
   }
   return item;
}
////////////////////////////////////////////////////////////////////////////////
// Description:
//
////////////////////////////////////////////////////////////////////////////////
FFItem* NWChemDesc::parseNWChemCrossData( ifstream& sfile, FFModel *model,
                                          char* tok, const int BUFSIZE,
                                          int sid, int catID, int* typeID)
{

   char* buffer = tok;
   FFItem *item = (FFItem*)0;

   // capture type name
   //------------------------------------------------
   string typeName = tok;
   typeName.append("-");
   if (( tok = strtok( 0, " ")) != 0)  {
      typeName.append( tok);

      // retrieve and set ff type id from ff type name
      //------------------------------------------------
      *typeID = model->getCategory(catID)->getTypeID( typeName, true);

      // create new force field item ( deleted by eventual owner)
      //------------------------------------------------

      // ignore remaining characters on this line
      // assume no annotation for the first line of cross parameters
      //------------------------------------------------

      // get second line of cross parameters
      // capture the parameters - must match order in FFParameterDesc
      //------------------------------------------------
      if ( sfile.getline( buffer, BUFSIZE)) {

         item = new FFItem( sid);

         unsigned long lineEnd = (unsigned long)buffer + strlen( buffer);//remember the last address
         string name = model->getCategory(catID)->getName();
         int max = model->getDesc().getCategoryByName(name).getNumParams();

         // continue to parse way through the line
         //------------------------------------------------
         int count = 0;
         while ( (tok = strtok( 0, " ")) != 0) { 
            // retrieve next <max> strings as parameters
            if ( count < max)  {
               item->addParameter( tok);
               count++;
            }
            else  {  // The rest of the line is an annotation - may have 
                     //   embedded blanks.
               // Replace the inserted null with a blank if the annotation 
               //    has > one word (the null could be the actual end of string)
               if ( ((unsigned long)tok + strlen(tok)) < lineEnd)
                  tok[strlen(tok)] = ' ';
               item->setAnnotation( tok);
               break;
            }
         }
      }
   }
   return item;
}

////////////////////////////////////////////////////////////////////////////////
// Description:
//
////////////////////////////////////////////////////////////////////////////////
FFItem* NWChemDesc::parseNWChemBondData( char* tok, FFModel *model,
                                         const unsigned long lineEnd, 
                                         int sid, int catID, int* typeID)
{
   FFItem *item = (FFItem*)0;
   string name = model->getCategory(catID)->getName();
   int max = model->getDesc().getCategoryByName( name).getNumParams();

   // capture type name
   //------------------------------------------------
   string typeName = tok;
   if (( tok = strtok( 0, " ")) != 0)  { // must have second part - at least "-"
      typeName.append( tok);
   
      // retrieve and set ff type id from ff type name
      //------------------------------------------------
      *typeID = model->getCategory(catID)->getTypeID( typeName, true);

      // create new force field item ( deleted by eventual owner)
      //------------------------------------------------
      item = new FFItem( sid);

      // capture parameters and annotation
      //------------------------------------------------
      int paramCount = 0;
      while ( ( tok = strtok( 0, " ")) != 0) {
         if ( !isalpha( *tok) && paramCount < max)  {
            item->addParameter( tok);
            paramCount++;
         }
         //else, found annotation, capture it - from tok + to end of line
         else {
            if ( ((unsigned long)tok + strlen(tok)) < lineEnd)  {
               tok[strlen(tok)] = ' ';
               item->setAnnotation( tok);
               break;
            }
         }
      }
      // pad undefined parameter values, if necessary
      while ( paramCount < max)  {
         item->addParameter( "0.0");
         paramCount++;
      } 
   }
   return item;
}

////////////////////////////////////////////////////////////////////////////////
// Description:
//
////////////////////////////////////////////////////////////////////////////////
FFItem* NWChemDesc::parseNWChemAngleData( char* tok, FFModel *model,
                                          unsigned long lineEnd,
                                          int sid, int catID, int* typeID)
{
   FFItem *item = (FFItem*)0;
   string name = model->getCategory(catID)->getName();
   int max = model->getDesc().getCategoryByName( name).getNumParams();

   // capture type name
   //------------------------------------------------
   string typeName = tok;
   if (( tok = strtok( 0, " ")) != 0)  { // must have second part - at least "-"
      typeName.append( tok);
      if (( tok = strtok( 0, " ")) != 0)  { // must have third part
         typeName.append( tok);
   
         // retrieve and set ff type id from ff type name
         //------------------------------------------------
         *typeID = model->getCategory(catID)->getTypeID( typeName, true);

         // create new force field item ( deleted by eventual owner)
         //------------------------------------------------
         item = new FFItem( sid);

         // capture parameters and annotation
         //------------------------------------------------
         int paramCount = 0;
         while ( ( tok = strtok( 0, " ")) != 0) {
            if ( !isalpha( *tok) && paramCount < max)  {
               item->addParameter( tok);
               paramCount++;
            }
            //else, found annotation, capture it - from tok + to end of line
            else {
               if ( ((unsigned long)tok + strlen(tok)) < lineEnd)  {
                  tok[strlen(tok)] = ' ';
                  item->setAnnotation( tok);
                  break;
               }
            }
         }
         // pad undefined parameter values, if necessary
         while ( paramCount < max)  {
            item->addParameter( "0.0");
            paramCount++;
         } 
      }
   }
   return item;
}

////////////////////////////////////////////////////////////////////////////////
// Description:
//
////////////////////////////////////////////////////////////////////////////////
FFItem* NWChemDesc::parseNWChemTorsionData( ifstream& sfile, FFModel *model,
                                            char* buffer, int BUFSIZE,
                                            int sid, int catID, int* typeID)
{
   FFItem *item = new FFItem( sid);

   string name = model->getCategory(catID)->getName();

   // restore null to blank ( to pre-strtok call)
   buffer[strlen(buffer)] = ' ';

   // parse line as long as get next line and parse returns true
   int myTypeID = -2;
   while ( parseTorsionLine( buffer, model, &item, catID, typeID )) {
      if ( myTypeID == -2 )
         myTypeID = *typeID;
      else if ( myTypeID != *typeID)  { // mismatched typeIDs per line
        item = (FFItem*)0;
        break;
      }
      // get next line
      if ( sfile.getline( buffer, BUFSIZE) == 0) {
         break;
      }
   }
   return item;
}

////////////////////////////////////////////////////////////////////////////////
// Description:
//
////////////////////////////////////////////////////////////////////////////////
bool NWChemDesc::parseTorsionLine( const char* buffer, FFModel *model,
                                   FFItem **item, int catID, int* typeID)
{
   bool ret = false;
   int lineEnd = strlen(buffer);

   // capture type name 
   string typeName;
   int i = 0;
   while ( i < NAME_END && i < lineEnd)  {
      if ( buffer[i] != ' ')
         typeName.append( 1, buffer[i]);
      i++;
   }
   *typeID = model->getCategory(catID)->getTypeID( typeName, true);

   // retrieve first param
   string param;
   while ( i < FIRST_END && i < lineEnd)  {
      if ( buffer[i] != ' ')
         param.append( 1, buffer[i]);
      i++;
   }
   (*item)->addParameter( param);
   param.clear();

   // retrieve second param
   while ( i < SECOND_END && i < lineEnd)  {
      if ( buffer[i] != ' ')
         param.append( 1, buffer[i]);
      i++;
   }
   (*item)->addParameter( param);
   param.clear();

   // retrieve third param
   while ( i < THIRD_END && i < lineEnd)  {
      if ( buffer[i] != ' ')  {
         if ( buffer[i] == '-')
            ret = true;
         else  
            param.append( 1, buffer[i]);
      }
      i++;
   }
   (*item)->addParameter( param);
   param.clear();

   // retrieve annotation, if exists
   string annot = buffer;
   i++;  // advance by one character (past blank delimiter)
   int len = lineEnd - i + 1;
   if ( len > 0)  {
      annot = annot.substr( i, lineEnd - i + 1);
      (*item)->setAnnotation( annot);
   }
   return ret;
}
