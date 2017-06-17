#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util/ErrMsg.H"
#include "util/STLUtil.H"
#include "util/StringConverter.H"

#include "tdat/Fragment.H"
#include "tdat/ShapeData.H"
#include "tdat/TResTab.H"
#include "tdat/FFRules.H"

#include "dsm/NWChemDesc.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/SegFactory.H"

#include <map>

using std::map;

///////////////////////////////////////////////////////////////////////////////
// Description:
//   Default constructor.
///////////////////////////////////////////////////////////////////////////////
SegFactory::SegFactory(void)
{
}

SegFactory::~SegFactory(void)
{
  int i, nsize;
  nsize = p_segmentList.size();
  for (i=0; i<nsize; i++) {
    delete p_segmentList[i];
  }
  p_segmentList.clear();
  p_unknownSegmentNames.clear();
}
///////////////////////////////////////////////////////////////////////////////
// Description:
//   Return a pointer to a segment corresponding to a particular residue name
//   (and coupling)
///////////////////////////////////////////////////////////////////////////////
Segment * SegFactory::getSegment(Residue *residue,
            const Residue::Coupling coupling, Fragment *frag, string fftype)
{

// Determine whether a suffix needs to be added to the residue name
// to INTERIOR, HEAD, TAIL, and MONOMER versions of the segment
// file. This can be done by checking the residue code and seeing if it
// is A, indicating that it was derived entirely from ATOM cards in a
// PDB file. If it is A, then it is assumed that the residue corresponds
// to one of the standard amino acid or nucleotide residues.

  string resname = residue->name();
  string suffix;
  TResTab restab;
  TResItem::Classification classification;
  classification = restab.verify(resname.c_str());
  size_t i,j;

// If residue classification is not Other, modify residue name.

  if (classification != TResItem::Other) {
    suffix.clear();

// Three non-blank characters found. Residue is an amino acid.

    if (classification == TResItem::AminoAcid) {

// Check to see if residue is isoleucine. If so, then check to
// see if there is an atom with the atom name " CD1". Change
// it to " CD " if it is present.

      if (resname == "ILE ") {

// Get list of atoms in residue

        vector<TAtm*> atomlist = frag->findResidueAtoms(residue);
        size_t size = atomlist.size();

// Check for atom named " CD1"

        for (i=0; i<size; i++) {
          TAtm *atm = atomlist[i];
          string atomName = atm->atomName().c_str();
          if (atomName == " CD1") {
            atm->atomName(" CD ");
            break;
          }
        }
      }

// Check to see if residue is cysteine. If so, then check to
// if the sulfur atom (atom name "SG") is bound to another sulfur.
// If it is, change residue name to CYX.

      if (resname == "CYS ") {

// Get list of atoms in residue

        vector<TAtm*> atomlist = frag->findResidueAtoms(residue);
        size_t size = atomlist.size();

// Check for atom named " SG "

        for (i=0; i<size; i++) {
          TAtm *atm = atomlist[i];
          string atomName = atm->atomName().c_str();
          if (atomName == " SG ") {

//  Look to see if SG is connected to another sulfur. If it is,
//  rename residue as "CYX".

            vector<TAtm*>* connectList = atm->connectedAtoms();
       
            for (j = 0; j < connectList->size(); j++) {

              TAtm* jatm = (*connectList)[j];
              int atomNum = jatm->atomicNumber();
              if (atomNum == 16) {
                residue->name("CYX ");
                resname[2] = 'X';
                break;
              }
            }
            delete connectList;
            break;
          }
        }
      }

// Check to see if residue is cysteine. If so, then check to see if
// protons have been added. If they have, then see if it is posible to
// determine protonation state and modify name accordingly.
      if (resname == "HIS ") {

// Get list of atoms in residue

        vector<TAtm*> atomlist = frag->findResidueAtoms(residue);
        size_t size = atomlist.size();

        // Check for atoms ND1 or NE2
        bool nd1_protonated = false;
        bool ne2_protonated = false;

        for (i=0; i<size; i++) {
          TAtm *atm = atomlist[i];
          string atomName = atm->atomName().c_str();

          // Find out if atom ND1 is protonated
          if (atomName == " ND1") {
            vector<TAtm*>* connectList = atm->connectedAtoms();
       
            if (connectList->size() == 3) {
              for (j = 0; j < connectList->size(); j++) {

                TAtm* jatm = (*connectList)[j];
                int atomNum = jatm->atomicNumber();
                if (atomNum == 1) {
                  nd1_protonated = true;
                  break;
                }
              }
            }
            delete connectList;
            break;
          }
          if (atomName == " NE2") {
            vector<TAtm*>* connectList = atm->connectedAtoms();
       
            if (connectList->size() == 3) {
              for (j = 0; j < connectList->size(); j++) {

                TAtm* jatm = (*connectList)[j];
                int atomNum = jatm->atomicNumber();
                if (atomNum == 1) {
                  ne2_protonated = true;
                  break;
                }
              }
            }
            delete connectList;
            break;
          }
        }

        if (nd1_protonated && ne2_protonated) {
          if (fftype == "Amber" || fftype == "NWChem") {
            residue->name("HIP ");
            resname[2] = 'P';
          } else if (fftype == "Charmm") {
            residue->name("HSP ");
            resname[1] = 'S';
            resname[2] = 'P';
          }
        } else if (!nd1_protonated && ne2_protonated) {
          if (fftype == "Amber" || fftype == "NWChem") {
            residue->name("HIE ");
            resname[2] = 'E';
          } else if (fftype == "Charmm") {
            residue->name("HSE ");
            resname[1] = 'S';
            resname[2] = 'E';
          }
        } else if (nd1_protonated && !ne2_protonated) {
          if (fftype == "Amber" || fftype == "NWChem") {
            residue->name("HID ");
            resname[2] = 'D';
          } else if (fftype == "Charmm") {
            residue->name("HSD ");
            resname[1] = 'S';
            resname[2] = 'D';
          }
        }
      }

      if (coupling == Residue::HEAD) suffix.append("_N");
      if (coupling == Residue::TAIL) suffix.append("_C");
      if (coupling == Residue::MONOMER) suffix.append("_M");
    }

// Residue is a DNA or RNA nucleotide.

    if (classification == TResItem::DNA ||
        classification == TResItem::RNA) {

// If residue is uracil or thymine, make sure it has preceding R or D

      if (resname == "  T ") {
        resname = " DT ";
        residue->name(resname.c_str());
      } else if (resname == "  U ") {
        resname = " RU ";
        residue->name(resname.c_str());
      } else if (resname == "THY ") {
        resname = " DT ";
        residue->name(resname.c_str());
      } else if (resname == "URA ") {
        resname = " RU ";
        residue->name(resname.c_str());
      }
      if (coupling == Residue::HEAD) suffix.append("_5");
      if (coupling == Residue::TAIL) suffix.append("_3");
      if (coupling == Residue::MONOMER) suffix.append("_M");
    }

// Residue is a DNA or RNA nucleotide. However, additional
// checking will be required to determine whether
// residue is RNA or DNA.

    if (classification == TResItem::RNAorDNA) {

// Residue is adenine, guanine, or cytosine. Check to see if O2* atom
// is present. If so, then residue belongs to an RNA strand, otherwise
// assume that it is DNA. First, change name from three characters to
// one, if necessary.

      if (resname == "CYT ") {
        resname = "  C ";
      } else if (resname == "GUA ") {
        resname = "  G ";
      } else if (resname == "ADE ") {
        resname = "  A ";
      }

      vector<TAtm*> atomlist = frag->findResidueAtoms(residue);
      size_t size = atomlist.size();

// Check for atom named O2*

      bool isRNA = false;
      for (i=0; i<size; i++) {
        string atomName = atomlist[i]->atomName().c_str();
        if (atomName == " O2*") isRNA = true;
      }
      if (isRNA) {
        resname[1] = 'R';
      } else {
        resname[1] = 'D';
      }
      residue->name(resname.c_str());
      STLUtil::stripLeadingAndTrailingWhiteSpace(resname);
      if (coupling == Residue::HEAD) resname.append("_5");
      if (coupling == Residue::TAIL) resname.append("_3");
      if (coupling == Residue::MONOMER) resname.append("_M");
    }
  }

// Remove leading and trailing blanks from residue name and then
// append suffix, if there is one
  STLUtil::stripLeadingAndTrailingWhiteSpace(resname);
  if (suffix.size() > 0) resname.append(suffix);

// Check to see if residue corresponds to an already existing segment

  size_t nsegs = p_segmentList.size();
  for (i=0; i<nsegs; i++) {
    Segment* seg = p_segmentList[i];
    EE_RT_ASSERT(seg!=(Segment*)0, EE_FATAL, "Null segment pointer");
    string segname = seg->name();
    if (segname == resname) {
#ifdef DEBUG
       cout << "Returning segment " << segname << endl;
#endif
       return seg;
    }
  }

// Check to see if residue corresponds to an unknown segment name.
// This should save time if a lot of unknown segments are present.

  nsegs = p_unknownSegmentNames.size();
  for (i=0; i<nsegs; i++) {
    string segname = p_unknownSegmentNames[i];
    if (segname == resname) {
#ifdef DEBUG
      cout << "No segment file found for " << resname << endl;
#endif
      return (Segment*)0;
    }
  }

// No segment found so far. Check directories for segment file.
// Get directory listing.

  size_t n_url = p_EDSIList.size();
  for (j=0; j<n_url; j++) {
    vector<ResourceResult> listing;
    p_EDSIList[j].listCollection(listing);
    if (listing.empty()) {
#ifdef DEBUG
      cout << "Listing not found" << endl;
#endif
      continue;
    }
    size_t ilist = listing.size();

// Scan through directory listing and check file names

    EcceURL segfile;
    string segname;
    size_t lastslsh;
    size_t lastdot;
    for (i = 0; i<ilist; i++) {
      if (listing[i].resourcetype == ResourceDescriptor::RT_DOCUMENT 
          && listing[i].contenttype == "sgm") {

// Found a segment file. Now check it against residue name to
// see if there is a match

        segfile = listing[i].url;
        segname = segfile.getFile();
        lastslsh = segname.rfind('/');
        lastdot = segname.rfind('.');
        segname = segname.substr(lastslsh+1,lastdot-lastslsh-1);

        if (segname == resname) {
          FileEDSI segEDSI(segfile);
          Segment *seg = readSegment(segEDSI,segname);
#ifdef DEBUG
          cout << "Segment file name " << segname  << endl;
#endif
          p_segmentList.push_back(seg);
          return seg;
        }
      }
    }

//  No segment file found. See if a fragment file exists.

    for (i = 0; i<ilist; i++) {
      if (listing[i].resourcetype == ResourceDescriptor::RT_DOCUMENT
          && listing[i].contenttype == "frg") {

// Found a fragment file. Now check it against residue name to
// see if there is a match

        segfile = listing[i].url;
        segname = segfile.getFile();
        lastslsh = segname.rfind('/');
        lastdot = segname.rfind('.');
        segname = segname.substr(lastslsh+1,lastdot-lastslsh-1);

        if (segname == resname) {
          FileEDSI segEDSI(segfile);
          Segment *seg = readFragment(segEDSI,segname);
#ifdef DEBUG
          cout << "Fragment file name " << segname  << endl;
#endif
          p_segmentList.push_back(seg);
          return seg;
        }
      }
    }
  }
#ifdef DEBUG
  cout << "No segment file found for " << resname << endl;
#endif
  p_unknownSegmentNames.push_back(resname);
  return (Segment*)0;
}
///////////////////////////////////////////////////////////////////////////////
// Description:
//   Read a .sgm (segment) file and create a new segment
///////////////////////////////////////////////////////////////////////////////
Segment* SegFactory::readSegment(FileEDSI & url, const string & segname)
{
#ifdef DEBUG
  cout << "read segment is called" << endl;
#endif
  istream* infile;
  Segment* seg;
  char segline[256];
  char i5[6];
  char a5[6];
  char a4[5];
  char d12[13];
  i5[5] = '\0';
  a5[5] = '\0';
  a4[4] = '\0';
  d12[12] = '\0';
  infile = url.getDataSet();
  if (infile == (istream*)0) {
    seg = (Segment*)0;
  } else {

// Ignore lines begining with $ or #. Get segment name from segname variable.

    bool use46 = false;
    seg = new Segment();
    bool comment = true;
    while (comment) {
      infile->getline(segline,255);
      if (segline[0] != '$' && segline[0] != '#') comment = false;
    }
    vector<string> values;
    StringConverter::toStringList(segline, values, " \t");
    if (values.size() == 1) {
      use46 = true;
    }
    if (use46) {
      infile->getline(segline,255);
    }
    char *cptr = segline;
    strncpy(i5,cptr,5);
    int natom = atoi(i5);
    cptr += 5;
    strncpy(i5,cptr,5);
    int nbonds = atoi(i5);
    cptr += 5;
    strncpy(i5,cptr,5);
    int nangles = atoi(i5);
    cptr += 5;
    strncpy(i5,cptr,5);
    int ntorsion = atoi(i5);
    cptr += 5;
    strncpy(i5,cptr,5);
    int nimproper = atoi(i5);
    int num_pset = 1, default_pset = 1;
    if (use46) {
      // skip over number of zmatrix definitions
      cptr += 10;
      if (strlen(cptr) >= 5) {
        strncpy(i5,cptr,5);
        num_pset = atoi(i5);
        cptr += 5;
      }
      if (strlen(cptr) >= 5) {
        strncpy(i5,cptr,5);
        default_pset = atoi(i5);
      } else {
        default_pset = 1;
      }
    }
    seg->name(segname);
    seg->natom(natom);
    seg->nbond(nbonds);
    seg->nangle(nangles);
    seg->ntorsion(ntorsion);
    seg->nimproper(nimproper);
// Read past line containing polarization energy correction
    int i,j;
    if (use46) {
      for (i=0; i<num_pset; i++) {
        infile->getline(segline,255);
      }
    }


// Get information from deck II
    string stemp;

    vector<int> atsq;
    vector<string> atnam;
    vector<string> atyp1;
    vector<string> atyp2;
    vector<string> atyp3;
    vector<char> dtyp1;
    vector<char> dtyp2;
    vector<char> dtyp3;
    vector<short> cgrp;
    vector<short> pgrp;
    vector<short> link;
    vector<int> etyp;
    vector<double> chrg1;
    vector<double> chrg2;
    vector<double> chrg3;
    vector<double> plrz1;
    vector<double> plrz2;
    vector<double> plrz3;

    double rval;
    for (i = 0; i<natom; i++) {
      if (use46) {
        infile->getline(segline,255);
        char* cptr = segline;
        strncpy(i5,cptr,5);
        atsq.push_back(atoi(i5));
        cptr += 5;

// Only use first four characters of atom name field
// (this field is actually six characters long)

        strncpy(a4,cptr,4);
        atnam.push_back(a4);
        cptr += 6;

        strncpy(i5,cptr,5);
        link.push_back(atoi(i5));
        cptr += 5;
        strncpy(i5,cptr,5);
        etyp.push_back(atoi(i5));
        cptr += 10;
        // Ignore q-hop index
        strncpy(i5,cptr,5);
        cgrp.push_back(atoi(i5));
        cptr += 5;
        strncpy(i5,cptr,5);
        pgrp.push_back(atoi(i5));

        // Get second line
        for (j=0; j<num_pset; j++) {
          infile->getline(segline,255);
          if (j == default_pset-1) {
            cptr = segline;
            cptr += 5;
            strncpy(a5,cptr,5);
            atyp1.push_back(a5);
            atyp2.push_back(a5);
            atyp3.push_back(a5);
            cptr += 5;
            dtyp1.push_back(*cptr);
            dtyp2.push_back(*cptr);
            dtyp3.push_back(*cptr);
            cptr++;
            
            strncpy(d12,cptr,12);
            rval = atof(d12);
            chrg1.push_back(rval);
            chrg2.push_back(rval);
            chrg3.push_back(rval);
            cptr += 12;
            strncpy(d12,cptr,12);
            rval = atof(d12);
            plrz1.push_back(rval);
            plrz2.push_back(rval);
            plrz3.push_back(rval);
          }
        }
      } else {

        infile->getline(segline,255);
        char* cptr = segline;
        strncpy(i5,cptr,5);
        atsq.push_back(atoi(i5));
        cptr += 5;

// Only use first four characters of atom name field
// (this field is actually six characters long)

        strncpy(a4,cptr,4);
        stemp = a4;
        atnam.push_back(stemp);
        cptr += 6;

        strncpy(a5,cptr,5);
        stemp = a5;
        atyp1.push_back(stemp);
        cptr += 5;
        dtyp1.push_back(*cptr);
        cptr++;
        strncpy(a5,cptr,5);
        stemp = a5;
        atyp2.push_back(stemp);
        cptr += 5;
        dtyp2.push_back(*cptr);
        cptr++;
        strncpy(a5,cptr,5);
        stemp = a5;
        atyp3.push_back(stemp);
        cptr += 5;
        dtyp3.push_back(*cptr);
        cptr++;
        strncpy(i5,cptr,5);
        cgrp.push_back(atoi(i5));
        cptr += 5;
        strncpy(i5,cptr,5);
        pgrp.push_back(atoi(i5));
        cptr += 5;
        strncpy(i5,cptr,5);
        link.push_back(atoi(i5));
        cptr += 5;
        strncpy(i5,cptr,5);
        etyp.push_back(atoi(i5));
        cptr += 5;

// second line

        infile->getline(segline,255);
        cptr = segline;
        strncpy(d12,cptr,12);
        rval = atof(d12);
        chrg1.push_back(rval);
        cptr += 12;
        strncpy(d12,cptr,12);
        rval = atof(d12);
        plrz1.push_back(rval);
        cptr += 12;
        strncpy(d12,cptr,12);
        rval = atof(d12);
        chrg2.push_back(rval);
        cptr += 12;
        strncpy(d12,cptr,12);
        rval = atof(d12);
        plrz2.push_back(rval);
        cptr += 12;
        strncpy(d12,cptr,12);
        rval = atof(d12);
        chrg3.push_back(rval);
        cptr += 12;
        strncpy(d12,cptr,12);
        rval = atof(d12);
        plrz3.push_back(rval);
        cptr += 12;
      }
    }
    seg->atSeqNum(atsq);
    seg->atomName(atnam);
    seg->atomType1(atyp1);
    seg->atomType2(atyp2);
    seg->atomType3(atyp3);
    seg->dynType1(dtyp1);
    seg->dynType2(dtyp2);
    seg->dynType3(dtyp3);
    seg->chrgGrp(cgrp);
    seg->plrzGrp(pgrp);
    seg->linkNum(link);
    seg->envType(etyp);
    seg->charge1(chrg1);
    seg->charge2(chrg2);
    seg->charge3(chrg3);
    seg->plrzblty1(plrz1);
    seg->plrzblty2(plrz2);
    seg->plrzblty3(plrz3);

// Get information from deck III

    vector<int> bsq;
    vector<int> idx;
    vector<int> jdx;
    vector<int> btyp;
    for (i=0; i<nbonds; i++) {
      infile->getline(segline,255);
      cptr = segline;
      strncpy(i5,cptr,5);
      bsq.push_back(atoi(i5));
      cptr +=5;
      strncpy(i5,cptr,5);
      idx.push_back(atoi(i5)-1);
      cptr +=5;
      strncpy(i5,cptr,5);
      jdx.push_back(atoi(i5)-1);
      cptr +=5;
      strncpy(i5,cptr,5);
      btyp.push_back(atoi(i5));
      cptr +=5;

// ignore second line (for now)

      for (j=0; j<num_pset; j++) {
        infile->getline(segline,255);
      }
    }
    seg->bndSeqNum(bsq);
    seg->iIndex(idx);
    seg->jIndex(jdx);
    seg->bndType(btyp);
  }
  return seg;
}
///////////////////////////////////////////////////////////////////////////////
// Description:
//   Read a .frg (fragment) file and create a new segment
///////////////////////////////////////////////////////////////////////////////
Segment* SegFactory::readFragment(FileEDSI & url, const string & segname)
{
#ifdef DEBUG
  cout << "read fragment is called" << endl;
#endif
  istream* infile;
  Segment* seg;
  char segline[256];
  char i5[6];
  char a5[6];
  char a4[5];
  char d12[13];
  i5[5] = '\0';
  a5[5] = '\0';
  a4[4] = '\0';
  d12[12] = '\0';
  bool use46 = false;
  infile = url.getDataSet();
  if (infile == (istream*)0) {
    seg = (Segment*)0;
  } else {

// Ignore lines begining with $ or #. Get segment name from segname variable.

    seg = new Segment();
    bool comment = true;
    while (comment) {
      infile->getline(segline,255);
      if (segline[0] != '$' && segline[0] != '#') comment = false;
    }
    char *cptr = segline;
    strncpy(i5,cptr,5);
    int natom = atoi(i5);
    seg->name(segname);
    seg->natom(natom);
    cptr += 5;

// Check to see if file is 4.6 format
    if (strlen(cptr) >= 5) {
      if (cptr[4] != ' ') {
        use46 = true;
      }
    }
    if (use46) {
      infile->getline(segline,255);
    }

// Get information from deck II
    string stemp;

    vector<int> atsq;
    vector<string> atnam;
    vector<string> atyp1;
    vector<char> dtyp1;
    vector<short> cgrp;
    vector<short> pgrp;
    vector<short> link;
    vector<int> etyp;
    vector<double> chrg1;
    vector<double> plrz1;

    int i;
    double rval;
    for (i = 0; i<natom; i++) {
      infile->getline(segline,255);
      char* cptr = segline;
      strncpy(i5,cptr,5);
      atsq.push_back(atoi(i5));
      cptr += 5;

// Only use first four characters of atom name field
// (this field is actually six characters long)

      strncpy(a4,cptr,4);
      stemp = a4;
      atnam.push_back(stemp);
      cptr += 6;

      strncpy(a5,cptr,5);
      stemp = a5;
      atyp1.push_back(stemp);
      cptr += 5;
      dtyp1.push_back(*cptr);
      cptr++;
      strncpy(i5,cptr,5);
      link.push_back(atoi(i5));
      cptr += 5;
      strncpy(i5,cptr,5);
      etyp.push_back(atoi(i5));
      cptr += 5;
      if (use46) {
        cptr += 5;
      }
      strncpy(i5,cptr,5);
      cgrp.push_back(atoi(i5));
      cptr += 5;
      strncpy(i5,cptr,5);
      pgrp.push_back(atoi(i5));
      cptr += 5;
      strncpy(d12,cptr,12);
      rval = atof(d12);
      chrg1.push_back(rval);
      cptr += 12;
      strncpy(d12,cptr,12);
      rval = atof(d12);
      plrz1.push_back(rval);
      cptr += 12;

    }
    seg->atSeqNum(atsq);
    seg->atomName(atnam);
    seg->atomType1(atyp1);
    seg->atomType2(atyp1);
    seg->atomType3(atyp1);
    seg->dynType1(dtyp1);
    seg->dynType2(dtyp1);
    seg->dynType3(dtyp1);
    seg->chrgGrp(cgrp);
    seg->plrzGrp(pgrp);
    seg->linkNum(link);
    seg->envType(etyp);
    seg->charge1(chrg1);
    seg->charge2(chrg1);
    seg->charge3(chrg1);
    seg->plrzblty1(plrz1);
    seg->plrzblty2(plrz1);
    seg->plrzblty3(plrz1);

// Get information from deck III

    vector<int> idxt;
    vector<int> jdxt;
    vector<int> bsq;
    vector<int> idx;
    vector<int> jdx;
    vector<int> btyp;
    int tbndcnt = 0;
    int length;
    int i1,i2;
    while (infile->getline(segline,255)) {
      length = strlen(segline);

      // Perform a few checks to see if line might be a comment.
      // Look for a carriage return or non-integer.

      if (length < 5) break;
      length = length/5;
      cptr = segline;
      strncpy(i5,cptr,5);
      if (strncmp(i5,"     ",5) == 0) break;
      char *tail = (char*)0;
      i1 = (int)strtol(i5,&tail,10);
      if (tail[0] != '\0' && tail[0] != ' ') break;
      cptr += 5;
      for (i=1; i<length; i++) {
        strncpy(i5,cptr,5);
        if (strncmp(i5,"     ",5) == 0) break;
        tbndcnt++;
        i2 = atoi(i5);
        cptr += 5;
        if (i1 < i2) {
          idxt.push_back(i1-1);
          jdxt.push_back(i2-1);
        } else {
          idxt.push_back(i2-1);
          jdxt.push_back(i1-1);
        }
        i1 = i2;
      }
    }
    if (tbndcnt == 0) {
      seg->nbond(0);
      return seg;
    }

//  Eliminate duplicate bonds
  
    int bndcnt = 0;
    bool duplicate;
    for (i=0; i<tbndcnt; i++) {
      duplicate = false;
      for (int j = i+1; j<tbndcnt; j++) {
        if (idxt[i] == idxt[j] && jdxt[i] == jdxt[j]) {
          duplicate = true;
          break;
        }
      }
      if (!duplicate) {
        bsq.push_back(bndcnt+1);
        idx.push_back(idxt[i]);
        jdx.push_back(jdxt[i]);
        btyp.push_back(0);
        bndcnt++;
      }
    }

    seg->bndSeqNum(bsq);
    seg->iIndex(idx);
    seg->jIndex(jdx);
    seg->bndType(btyp);
    seg->nbond(bndcnt);
  }
  return seg;
}
///////////////////////////////////////////////////////////////////////////////
// Description:
//   Write a .sgm (segment) file
///////////////////////////////////////////////////////////////////////////////
bool SegFactory::writeSegment(const Residue * residue, const FileEDSI & url,
                 const Fragment * frag, bool use46)
{

  vector<TAtm*> tmpAtmList = frag->findResidueAtoms(residue);

  // Scan list to remove any nubs
  size_t i,j,k;
  size_t size = tmpAtmList.size();
  vector<TAtm*> atmList;
  for (i=0; i<size; i++) {
    if (tmpAtmList[i]->atomicSymbol()!="Nub") {
      atmList.push_back(tmpAtmList[i]);
    }
  }

// First, check to see if atom types are available for all atoms. If not,
// then not enough information is available to create a segment file.

  map<int, int, less<int> > atMap;
  size = atmList.size();
  bool isComplete = true;
  for (i=0; i<size; i++) {
    TAtm* atm = atmList[i];
    string astr = atm->atomType();
    if (astr == "    " || astr == "")
      isComplete = false;
  }
  if (isComplete) {
    string putstream;
    char* cptr;
    char buf[120];
    double zero = 0.0;
    int izero = 0;

// Use residue name as segment name. Get rid of leading blank spaces
// first.

    string resname = residue->name();
    while (resname.size() > 0 && resname.at(0) == ' ') {
      resname = resname.erase(0,1);
    }

    if (resname.size() != 0) {
      cptr = buf;
      cptr[0] = '$';
      cptr++;
      strncpy(cptr,resname.c_str(),resname.size());
      cptr += resname.size();
      strncpy(cptr,"\n",1);
      cptr++;
      *cptr = '\0';
      putstream.append(buf);
    } else {
      EE_RT_ASSERT(false,EE_WARNING,"No Residue name specified");
      return false;
    }
    if (use46) {
      cptr = buf;
      sprintf(cptr,"%12.6f\n",4.6);
      putstream.append(buf);
    }

// Now need to compute total number of bonds, angles, torsions, and
// improper torsions to write out the next line. Start by making a
// list of unique bonds in the fragment.
    
    vector<size_t> ibnd;
    vector<size_t> jbnd;
    for (i=0; i<size; i++) {
      TAtm* iatm = atmList[i];
      vector<TAtm*>* clist = iatm->connectedAtoms();
      atMap[iatm->index()] = i;
   
      for (j=0; j < clist->size(); j++) {
        TAtm* jatm = (*clist)[j];

// Compare connected atom j with atoms in atmList and see if there
// are any correspondences. This will insure that only bonds between
// atoms within the fragment are included in the list.
 
        for (k=0; k<size; k++) {
          if (jatm == atmList[k]) {

// Only add bonds for i<k to avoid duplicates.

            if (i<k) {
              ibnd.push_back(i); 
              jbnd.push_back(k); 
            }
            break;
          }
        }
      }
      delete clist;
    }

// We now have a complete list of bonds. Construct a list of connected
// atoms for each atom using the local indices.

    size_t nbond = ibnd.size();
    vector<vector<int> > connectList;
    for (i=0; i<size; i++) {
      vector<int> list;
      connectList.push_back(list);
      for (j=0; j<nbond; j++) {
        if (ibnd[j] == i) {
          connectList[i].push_back(jbnd[j]);
        } else if (jbnd[j] == i) {
          connectList[i].push_back(ibnd[j]);
        }
      }
    }

// List of connected atoms is complete. Now construct list of unique
// bond angles.

    vector<int> iang;
    vector<int> jang;
    vector<int> kang;

    for (i=0; i<nbond; i++) {
      size_t iat = ibnd[i];
      size_t jat = jbnd[i];
      size_t kat;
      size_t cmax;
      vector<int> clist;
      clist = connectList[iat];
      cmax = clist.size();
      for (j=0; j<cmax; j++) {
        kat = clist[j];
        if (kat != jat && kat > jat) {
          iang.push_back(jat);
          jang.push_back(iat);
          kang.push_back(kat);
        }
      }
      clist = connectList[jat];
      cmax = clist.size();
      for (j=0; j<cmax; j++) {
        kat = clist[j];
        if (kat != iat && kat > iat) {
          iang.push_back(iat);
          jang.push_back(jat);
          kang.push_back(kat);
        }
      }
    }
    size_t nangle = iang.size();

// Construct list of unique torsions.

    vector<size_t> itors;
    vector<size_t> jtors;
    vector<size_t> ktors;
    vector<size_t> ltors;
    for (i=0; i<nangle; i++) {
      size_t iat = iang[i];
      size_t jat = jang[i];
      size_t kat = kang[i];
      size_t lat;
      size_t cmax;
      vector<int> clist;
      clist = connectList[iat];
      cmax = clist.size();
      for (j=0; j<cmax; j++) {
        lat = clist[j];
        if (lat != jat && lat != kat && kat < lat) {
          itors.push_back(kat);
          jtors.push_back(jat);
          ktors.push_back(iat);
          ltors.push_back(lat);
        }
      }
      clist = connectList[kat];
      cmax = clist.size();
      for (j=0; j<cmax; j++) {
        lat = clist[j];
        if (lat != iat && lat != jat && iat < lat) {
          itors.push_back(iat);
          jtors.push_back(jat);
          ktors.push_back(kat);
          ltors.push_back(lat);
        }
      }
    }
    size_t ntors = itors.size();

// Construct list of improper dihedrals

    vector<int> idihd;
    vector<int> jdihd;
    vector<int> kdihd;
    vector<int> ldihd;

    NWChemDesc rules;
    vector<int> rings = frag->findRingAtoms();

    for (i=0; i<size; i++) {
      int cmax;
      int jat;
      int kat;
      int lat;
      TAtm* atm = atmList[i];
      vector<TAtm*>* clist = atm->connectedAtoms();
      cmax = clist->size();
      if (cmax == 3) {
        vector<TBond*> blist = atm->bondList();
        if (rules.checkInversion(atm,blist)) {
          jat = atMap[(*clist)[0]->index()];
          kat = atMap[(*clist)[1]->index()];
          lat = atMap[(*clist)[2]->index()];
          vector<bool> check = rules.findInversions(atm,blist,&rings);
          if (check[0]) {
            idihd.push_back(kat);
            jdihd.push_back(lat);
            kdihd.push_back(i);
            ldihd.push_back(jat);
          }
          if (check[1]) {
            idihd.push_back(jat);
            jdihd.push_back(lat);
            kdihd.push_back(i);
            ldihd.push_back(kat);
          }
          if (check[2]) {
            idihd.push_back(jat);
            jdihd.push_back(kat);
            kdihd.push_back(i);
            ldihd.push_back(lat);
          }
        }
      }
    }
    size_t ndihd = idihd.size();

// Bond lists are complete. Now write out remaining file.

#if 000
    cptr = buf;
    sprintf(cptr,"%5d",size);
    cptr += 5;
    sprintf(cptr,"%5d",nbond);
    cptr += 5;
    sprintf(cptr,"%5d",nangle);
    cptr += 5;
    sprintf(cptr,"%5d",ntors);
    cptr += 5;
    sprintf(cptr,"%5d",ndihd);
    cptr += 5;
    cptr = '\n';
    cptr++;
    cptr = '\0';
#else
    if (use46) {
      sprintf(buf, "%5zu%5zu%5zu%5zu%5zu%5d%5d%5d\n", size, nbond, nangle,
              ntors, ndihd, 0, 1, 1);
    } else {
      sprintf(buf, "%5zu%5zu%5zu%5zu%5zu\n", size, nbond, nangle, ntors, ndihd);
    }
#endif
    putstream.append(buf);
    if (use46) {
      sprintf(buf, "%12.6f\n",0.0);
    }
    putstream.append(buf);

    short igrp;
    double chrg;
    float plrz;
    string attype;
    string atname;
    for (i=0; i<size; i++) {
      TAtm* atm = atmList[i];
      if (use46) {
        cptr = buf;
        atname = atm->atomName();
        sprintf(cptr,"%5zu",i+1);
        cptr += 5;
        strncpy(cptr,atname.c_str(),atname.size());
        cptr += 4;
        strncpy(cptr,"  ",2);
        cptr += 2;
        sprintf(cptr,"%5d%5d%5d%5d%5d",atm->connectionIndex(),
                                       atm->dihedralIndex(), 0,
                                       atm->chargeGroup(),
                                       atm->polarizationGroup());
        cptr +=25;
        strncpy(cptr,"\n",1);
        cptr++;
        *cptr = '\0';
        putstream.append(buf);

        cptr = buf;
        strncpy(cptr,"     ",5);
        cptr += 5;
        attype = atm->atomType();
        if (attype.size() < 5) {
          strcpy(cptr,attype.c_str());
          cptr += attype.size();
          for (j=0; j<5-attype.size(); j++) {
            strncpy(cptr," ",1);
            cptr++;
          }
        } else {
          strncpy(cptr,attype.c_str(),5);
          cptr += 5;
        }
        strncpy(cptr," ",1);
        cptr++;
        sprintf(cptr,"%12.6f%12.6f",atm->partialCharge(),
                                    atm->polarizability());
        cptr += 24;
        strncpy(cptr,"\n",1);
        cptr++;
        *cptr = '\0';
        putstream.append(buf);
      } else {
        cptr = buf;
        atname = atm->atomName();
        sprintf(cptr,"%5zu",i+1);
        cptr += 5;
        strncpy(cptr,atname.c_str(),atname.size());
        cptr += 4;
        strncpy(cptr,"  ",2);
        cptr += 2;

// Set 1

        attype = atm->atomType();
        if (attype.size() < 5) {
          strcpy(cptr,attype.c_str());
          cptr += attype.size();
          for (j=0; j<5-attype.size(); j++) {
            strncpy(cptr," ",1);
            cptr++;
          }
        } else {
          strncpy(cptr,attype.c_str(),5);
          cptr += 5;
        }
        strncpy(cptr," ",1);
        cptr++;

// Set 2

        if (attype.size() < 5) {
          strcpy(cptr,attype.c_str());
          cptr += attype.size();
          for (j=0; j<5-attype.size(); j++) {
            strncpy(cptr," ",1);
            cptr++;
          }
        } else {
          strncpy(cptr,attype.c_str(),5);
          cptr += 5;
        }
        strncpy(cptr," ",1);
        cptr++;

// Set 3

        if (attype.size() < 5) {
          strcpy(cptr,attype.c_str());
          cptr += attype.size();
          for (j=0; j<5-attype.size(); j++) {
            strncpy(cptr," ",1);
            cptr++;
          }
        } else {
          strncpy(cptr,attype.c_str(),5);
          cptr += 5;
        }
        strncpy(cptr," ",1);
        cptr++;

// Charge groups etc.

        igrp = atm->chargeGroup();
        sprintf(cptr,"%4d",igrp);
        cptr += 4;
        igrp = atm->polarizationGroup();
        sprintf(cptr,"%4d",igrp);
        cptr += 4;
        igrp = atm->connectionIndex();
        sprintf(cptr,"%4d",igrp);
        cptr += 4;
        igrp = atm->dihedralIndex();
        sprintf(cptr,"%4d",igrp);
        cptr += 4;
        strncpy(cptr,"\n",1);
        cptr++;
        *cptr = '\0';
        putstream.append(buf);
 
// Charges and polarizabilities.

        cptr = buf;
        chrg = atm->partialCharge();
        plrz = atm->polarizability();
        sprintf(cptr,"%12.6lf",chrg);
        cptr += 12;
        sprintf(cptr,"%12.6lf",plrz);
        cptr += 12;
        sprintf(cptr,"%12.6lf",chrg);
        cptr += 12;
        sprintf(cptr,"%12.6lf",plrz);
        cptr += 12;
        sprintf(cptr,"%12.6lf",chrg);
        cptr += 12;
        sprintf(cptr,"%12.6lf",plrz);
        cptr += 12;
        strncpy(cptr,"\n",1);
        cptr++;
        *cptr = '\0';
        putstream.append(buf);
      }
      
    }

// Print out bond list

    for (i=0; i<nbond; i++) {
      cptr = buf;
      sprintf(cptr,"%5zu",i+1);
      cptr += 5;
      sprintf(cptr,"%5zu",ibnd[i]+1);
      cptr += 5;
      sprintf(cptr,"%5zu",jbnd[i]+1);
      cptr += 5;
      sprintf(cptr,"%5d",izero);
      cptr += 5;
      sprintf(cptr,"%5d",izero);
      cptr += 5;
      strncpy(cptr,"\n",1);
      cptr++;
      *cptr = '\0';
      putstream.append(buf);

      cptr = buf;
      sprintf(cptr,"%12.6f",zero);
      cptr += 12;
      sprintf(cptr,"%12.5e",zero);
      cptr += 12;
      if (!use46) {
        sprintf(cptr,"%12.6f",zero);
        cptr += 12;
        sprintf(cptr,"%12.5e",zero);
        cptr += 12;
        sprintf(cptr,"%12.6f",zero);
        cptr += 12;
        sprintf(cptr,"%12.5e",zero);
        cptr += 12;
      }
      strncpy(cptr,"\n",1);
      cptr++;
      *cptr = '\0';
      putstream.append(buf);
    }

// Print out angle list

    for (i=0; i<nangle; i++) {
      cptr = buf;
      sprintf(cptr,"%5zu",i+1);
      cptr += 5;
      sprintf(cptr,"%5d",iang[i]+1);
      cptr += 5;
      sprintf(cptr,"%5d",jang[i]+1);
      cptr += 5;
      sprintf(cptr,"%5d",kang[i]+1);
      cptr += 5;
      sprintf(cptr,"%5d",izero);
      cptr += 5;
      sprintf(cptr,"%5d",izero);
      cptr += 5;
      strncpy(cptr,"\n",1);
      cptr++;
      *cptr = '\0';
      putstream.append(buf);

      cptr = buf;
      sprintf(cptr,"%10.6f",zero);
      cptr += 10;
      sprintf(cptr,"%12.5e",zero);
      cptr += 12;
      if (!use46) {
        sprintf(cptr,"%10.6f",zero);
        cptr += 10;
        sprintf(cptr,"%12.5e",zero);
        cptr += 12;
        sprintf(cptr,"%10.6f",zero);
        cptr += 10;
        sprintf(cptr,"%12.5e",zero);
        cptr += 12;
      }
      strncpy(cptr,"\n",1);
      cptr++;
      *cptr = '\0';
      putstream.append(buf);
    }

// Print out torsion list

    for (i=0; i<ntors; i++) {
      cptr = buf;
      sprintf(cptr,"%5zu",i+1);
      cptr += 5;
      sprintf(cptr,"%5zu",itors[i]+1);
      cptr += 5;
      sprintf(cptr,"%5zu",jtors[i]+1);
      cptr += 5;
      sprintf(cptr,"%5zu",ktors[i]+1);
      cptr += 5;
      sprintf(cptr,"%5zu",ltors[i]+1);
      cptr += 5;
      sprintf(cptr,"%5d",izero);
      cptr += 5;
      sprintf(cptr,"%5d",izero);
      cptr += 5;
      strncpy(cptr,"\n",1);
      cptr++;
      *cptr = '\0';
      putstream.append(buf);

      cptr = buf;
      sprintf(cptr,"%3d",izero);
      cptr += 3;
      sprintf(cptr,"%10.6f",zero);
      cptr += 10;
      sprintf(cptr,"%12.5e",zero);
      cptr += 12;
      if (!use46) {
        sprintf(cptr,"%3d",izero);
        cptr += 3;
        sprintf(cptr,"%10.6f",zero);
        cptr += 10;
        sprintf(cptr,"%12.5e",zero);
        cptr += 12;
        sprintf(cptr,"%3d",izero);
        cptr += 3;
        sprintf(cptr,"%10.6f",zero);
        cptr += 10;
        sprintf(cptr,"%12.5e",zero);
        cptr += 12;
      }
      strncpy(cptr,"\n",1);
      cptr++;
      *cptr = '\0';
      putstream.append(buf);
    }

// Print out improper dihedrals.

    for (i=0; i<ndihd; i++) {
      cptr = buf;
      sprintf(cptr,"%5zu",i+1);
      cptr += 5;
      sprintf(cptr,"%5d",idihd[i]+1);
      cptr += 5;
      sprintf(cptr,"%5d",jdihd[i]+1);
      cptr += 5;
      sprintf(cptr,"%5d",kdihd[i]+1);
      cptr += 5;
      sprintf(cptr,"%5d",ldihd[i]+1);
      cptr += 5;
      sprintf(cptr,"%5d",izero);
      cptr += 5;
      sprintf(cptr,"%5d",izero);
      cptr += 5;
      strncpy(cptr,"\n",1);
      cptr++;
      *cptr = '\0';
      putstream.append(buf);

      cptr = buf;
      sprintf(cptr,"%3d",izero);
      cptr += 3;
      sprintf(cptr,"%10.6f",zero);
      cptr += 10;
      sprintf(cptr,"%12.5e",zero);
      cptr += 12;
      if (!use46) {
        sprintf(cptr,"%3d",izero);
        cptr += 3;
        sprintf(cptr,"%10.6f",zero);
        cptr += 10;
        sprintf(cptr,"%12.5e",zero);
        cptr += 12;
        sprintf(cptr,"%3d",izero);
        cptr += 3;
        sprintf(cptr,"%10.6f",zero);
        cptr += 10;
        sprintf(cptr,"%12.5e",zero);
        cptr += 12;
      }
      strncpy(cptr,"\n",1);
      cptr++;
      *cptr = '\0';
      putstream.append(buf);
    }
    const char* output =  putstream.c_str();
    FileEDSI newurl(url);
    if (newurl.putDataSet(output)) {
      return true;
    } else {
      EE_RT_ASSERT(false,EE_WARNING,"Unable to create segment file");
      return false;
    }
  } else {
    return false;
  }
}
///////////////////////////////////////////////////////////////////////////////
// Description:
//   Write a .frg (fragment) file
///////////////////////////////////////////////////////////////////////////////
bool SegFactory::writeFragment(const Residue * residue, const FileEDSI & url,
                 const Fragment * frag, bool use46)
{
  vector<TAtm*> tmpAtmList = frag->findResidueAtoms(residue);

  // Scan list to remove any nubs
  size_t i,j,k;
  size_t size = tmpAtmList.size();
  vector<TAtm*> atmList;
  for (i=0; i<size; i++) {
    if (tmpAtmList[i]->atomicSymbol()!="Nub") {
      atmList.push_back(tmpAtmList[i]);
    }
  }

// First, check to see if atom types are available for all atoms. If not,
// then not enough information is available to create a fragment file.

  size = atmList.size();
  bool isComplete = true;
  for (i=0; i<size; i++) {
    TAtm* atm = atmList[i];
    string astr= atm->atomType();
    if (astr == "    " || astr == "")
      isComplete = false;
  }
  if (isComplete) {
    string putstream;
    char* cptr;
    char buf[120];

// Use residue name as segment name. Get rid of leading blank spaces
// first.

    string resname = residue->name();
    while (resname.size() > 0 && resname.at(0) == ' ') {
      resname = resname.erase(0,1);
    }

    if (resname.size() != 0) {
      cptr = buf;
      cptr[0] = '$';
      cptr++;
      strncpy(cptr,resname.c_str(),resname.size());
      cptr += resname.size();
      strncpy(cptr,"\n",1);
      cptr++;
      *cptr = '\0';
      putstream.append(buf);
    } else {
      EE_RT_ASSERT(false,EE_WARNING,"No Residue name specified");
      return false;
    }

// Now need to compute total number of bonds. Although not required,
// only unique bonds are specified.
    
    vector<size_t> ibnd;
    vector<size_t> jbnd;
    for (i=0; i<size; i++) {
      TAtm* iatm = atmList[i];
      vector<TAtm*>* clist = iatm->connectedAtoms();
  
      for (j=0; j < clist->size(); j++) {
        TAtm* jatm = (*clist)[j];

// Compare connected atom j with atoms in atmList and see if there
// are any correspondences. This will insure that only bonds between
// atoms within the fragment are included in the list.
 
        for (k=0; k<size; k++) {
          if (jatm == atmList[k]) {

// Only add bonds for i<k to avoid duplicates.

            if (i<k) {
              ibnd.push_back(i); 
              jbnd.push_back(k); 
            }
            break;
          }
        }
      }
      delete clist;
    }

// We now have a complete list of bonds. Construct a list of connected
// atoms for each atom using the local indices.

    size_t nbond = ibnd.size();
    vector<vector<size_t> > connectList;
    for (i=0; i<size; i++) {
      vector<size_t> list;
      connectList.push_back(list);
      for (j=0; j<nbond; j++) {
        if (ibnd[j] == i) {
          connectList[i].push_back(jbnd[j]);
        } else if (jbnd[j] == i) {
          connectList[i].push_back(ibnd[j]);
        }
      }
    }

// Bond list is complete. Now write out remaining file.

    if (use46) {
      sprintf(buf, "%5zu%5d%5d%5d\n", size,1,1,0);
      putstream.append(buf);
      cptr = buf;
      strncpy(cptr,resname.c_str(),resname.size());
      cptr += resname.size();
      strncpy(cptr,"\n",1);
      cptr++;
      *cptr = '\0';
      putstream.append(buf);
    } else {
      sprintf(buf, "%5zu\n", size);
      putstream.append(buf);
    }

    short igrp;
    double chrg;
    float plrz;
    string attype;
    string atname;
    for (i=0; i<size; i++) {
      TAtm* atm = atmList[i];
      atname = atm->atomName();
      cptr = buf;
      sprintf(cptr,"%5zu",i+1);
      cptr += 5;
      strncpy(cptr,atname.c_str(),atname.size());
      cptr += 4;
      strncpy(cptr,"  ",2);
      cptr += 2;

// Set 1

      attype = atm->atomType();
      if (attype.size() < 5) {
        strcpy(cptr,attype.c_str());
        cptr += attype.size();
        for (j=0; j<5-attype.size(); j++) {
          strncpy(cptr," ",1);
          cptr++;
        }
      } else {
        strncpy(cptr,attype.c_str(),5);
        cptr += 5;
      }
      strncpy(cptr," ",1);
      cptr++;

// Links, shape, charge groups etc.

      igrp = atm->connectionIndex();
      sprintf(cptr,"%5d",igrp);
      cptr += 5;
      igrp = atm->dihedralIndex();
      sprintf(cptr,"%5d",igrp);
      cptr += 5;
      if (use46) {
        igrp = 0;
        sprintf(cptr,"%5d",igrp);
        cptr += 5;
      }
      igrp = atm->chargeGroup();
      sprintf(cptr,"%5d",igrp);
      cptr += 5;
      igrp = atm->polarizationGroup();
      sprintf(cptr,"%5d",igrp);
      cptr += 5;
      chrg = atm->partialCharge();
      plrz = atm->polarizability();
      sprintf(cptr,"%12.6lf",chrg);
      cptr += 12;
      sprintf(cptr,"%12.6lf",plrz);
      cptr += 12;
      strncpy(cptr,"\n",1);
      cptr++;
      *cptr = '\0';
      putstream.append(buf);
    }

// Print out bond list

    for (i=0; i<nbond; i++) {
      cptr = buf;
      sprintf(cptr,"%5zu",ibnd[i]+1);
      cptr += 5;
      sprintf(cptr,"%5zu",jbnd[i]+1);
      cptr += 5;
      strncpy(cptr,"\n",1);
      cptr++;
      *cptr = '\0';
      putstream.append(buf);
    }
    const char* output =  putstream.c_str();
    FileEDSI newurl(url);
    if (newurl.putDataSet(output)) {
      return true;
    } else {
      EE_RT_ASSERT(false,EE_WARNING,"Unable to create fragment file");
      return false;
    }
  } else {
    return false;
  }
}
///////////////////////////////////////////////////////////////////////////////
// Description:
//   Add EDSI object to EDSI list
///////////////////////////////////////////////////////////////////////////////
void SegFactory::addEDSI(const char* filename)
{
  EcceURL url(filename);
  FileEDSI edsiURL(url);
  p_EDSIList.push_back(edsiURL);
}
///////////////////////////////////////////////////////////////////////////////
// Description:
//   Delete EDSI object from EDSI list
///////////////////////////////////////////////////////////////////////////////
void SegFactory::deleteEDSI(const FileEDSI url)
{
#ifdef DEBUG
       cout << "deleteEDSI called" << endl;
#endif
}
///////////////////////////////////////////////////////////////////////////////
// Description:
//   Clear EDSI list
///////////////////////////////////////////////////////////////////////////////
void SegFactory::clearEDSI()
{
  p_EDSIList.clear();
}
