#include <string>
#include <vector>
  using std::string;
  using std::vector;
#include "tdat/Segment.H"
#include "tdat/Fragment.H"
#include "tdat/TPerTab.H"
#include "tdat/FragUtil.H"

///////////////////////////////////////////////////////////////////////////////
// Description:
//   Default constructor
///////////////////////////////////////////////////////////////////////////////
Segment::Segment()
{
}
Segment::Segment(vector<TAtm*> atomlist)
{
}
///////////////////////////////////////////////////////////////////////////////
// Description:
//   Copy constructor
///////////////////////////////////////////////////////////////////////////////
Segment::Segment(const Segment & rhs)
{
   *this = rhs;
}
///////////////////////////////////////////////////////////////////////////////
// Description:
//   Assignment operator
///////////////////////////////////////////////////////////////////////////////
Segment& Segment::operator=(const Segment & rhs)
{
    if (this != &rhs) {
      p_segName = rhs.p_segName;
      p_natom = rhs.p_natom;
      p_nbond = rhs.p_nbond;
      p_nangle = rhs.p_nangle;
      p_ntorsion = rhs.p_ntorsion;
      p_nimproper = rhs.p_nimproper;

      p_atSeqNum = rhs.p_atSeqNum;
      p_atomName = rhs.p_atomName;
      p_atomType1 = rhs.p_atomType1;
      p_atomType2 = rhs.p_atomType2;
      p_atomType3 = rhs.p_atomType3;
      p_dynType1 = rhs.p_dynType1;
      p_dynType2 = rhs.p_dynType2;
      p_dynType3 = rhs.p_dynType3;
      p_chrgGrp = rhs.p_chrgGrp;
      p_plrzGrp = rhs.p_plrzGrp;
      p_linkNum = rhs.p_linkNum;
      p_envType = rhs.p_envType;
      p_charge1 = rhs.p_charge1;
      p_charge2 = rhs.p_charge2;
      p_charge3 = rhs.p_charge3;
      p_plrzblty1 = rhs.p_plrzblty1;
      p_plrzblty2 = rhs.p_plrzblty2;
      p_plrzblty3 = rhs.p_plrzblty3;

      p_bndSeqNum = rhs.p_bndSeqNum;
      p_iIndex = rhs.p_iIndex;
      p_jIndex = rhs.p_jIndex;
      p_bndType = rhs.p_bndType;
    }
    return *this;
}
///////////////////////////////////////////////////////////////////////////////
// Description:
//   Compare residue and segment to see if residue has a complete set of atoms
///////////////////////////////////////////////////////////////////////////////
Residue::ResidueStatus Segment::compare(Residue *Res, Fragment *frag)
{
#ifdef DEBUG
  //cout << "Compare segment called" << endl;
#endif
  vector<TAtm*> atomlist = frag->findResidueAtoms(Res);
  int ratoms = atomlist.size();
  int satoms = p_natom;
  vector<int> rstat(ratoms);
  vector<int> sstat(satoms);
  TPerTab pertab;
  int inub = pertab.nubAtom();
  int i,j,k;
  for (i=0; i<ratoms; i++) {
    rstat[i] = 0;
  }
  for (i=0; i<satoms; i++) {
    sstat[i] = 0;
  }

// Scan over atoms in residue

  string resnam = Res->name();
  for (i=0; i<ratoms; i++) {

// Compare residue atoms with atoms in segment to see if there are any
// correspondences. Get atom name (need temporary kluge to get around
// fact that TAtm accessors provide an RWCString).

    TAtm *atm = atomlist[i];

// Hack to change O in HOH to OW so that it matches standard name in
// force fields.
    string ratomName = atm->atomName();
    if (resnam == "HOH " && ratomName == " O  ") {
      ratomName = " OW ";
      atm->atomName(ratomName.c_str());
    }

    for (j=0; j<satoms; j++) {
      string satomName = p_atomName[j];
      if (ratomName == satomName) {
        rstat[i] = 1;
        sstat[j] = 1;
        break;
      }
    }
  }

// Determine status of residue. If some atoms in residue are not
// in segment, and they correspond to heavy atoms (not hydrogen),
// then residue is unknown.

  bool unkn = false;
  bool unknHyd = false;
  TAtm* atm;
  for (i=0; i<ratoms; i++) {
    if (rstat[i] == 0) { 
      atm = atomlist[i];
      int atomnum = atm->atomicNumber();
      if (atomnum != inub) {
        if (atomnum != 1) {
          unkn = true;
        } else {
          unknHyd = true;
        }
      }
    }
  }
  if (unkn) return Residue::UNKNOWN;

// If unknown hydrogens are present, try and identify them by
// comparing bond list from the residue with bond list in segment
// file.

  if (unknHyd) {

//  Get lists of hydrogens from segment file and from residue

    int ires = 0;
    int iseg = 0;
    vector<int> rlist;
    vector<int> slist;
    vector<int> rename_i;
    vector<int> rename_j;
    vector<bool> rfound;
    vector<bool> sfound;
    int atomnum;
    for (i=0; i<ratoms; i++) {
      atm = atomlist[i];
      atomnum  = atm->atomicNumber();
      if (atomnum == 1) {
        rlist.push_back(i);
        rfound.push_back(false);
        ires++;
      }
    }
    for (i=0; i<satoms; i++) {
      const char* cptr = p_atomName[i].c_str();
      if (cptr[1] == 'H') {
        slist.push_back(i);
        sfound.push_back(false);
        iseg++;
      }
    }

// Scan through list of residue hydrogens and find the atom to which they
// are bonded. See if there is one hydrogen in segment list that is bonded
// to an atom with the same atom name.

    for (i=0; i<ires; i++) {
      int iat = rlist[i];
      atm = atomlist[iat];
      string rptr;
      vector<TAtm*>* catm = atm->connectedAtoms();
      if (catm->size() > 0) {
        rptr = (*catm)[0]->atomName();
        delete catm;
      } else {
        delete catm;
        return Residue::UNKNOWN;
      }
      for (j=0; j<iseg; j++) {
        if (!sfound[j]) {
          int jat = slist[j];

// Scan list of segment bonds

          int bsize = p_iIndex.size();
          int ibnd,jbnd;
          int kat = -1;
          for (k=0; k<bsize; k++) {
            ibnd = p_iIndex[k];
            jbnd = p_jIndex[k];
            if (ibnd == jat || jbnd == jat) {
              kat =  k;
              break;
            }
          }
          string sptr;
          if (kat > -1) {
            if (jat == p_iIndex[kat]) {
              sptr = p_atomName[p_jIndex[kat]].c_str();
            } else {
              sptr = p_atomName[p_iIndex[kat]].c_str();
            }
            if (rptr == sptr) {
               sfound[j] = true;
               rfound[i] = true;
               rename_i.push_back(iat);
               rename_j.push_back(jat);
               break;
            }
          }
        }
      }
    }

// Check to see if any hydrogens on residue were left unassigned. If
// so, then give up and label residue as UNKNOWN. Otherwise, go back
// and rename all hydrogens so that they have the same names as in the
// segment file.

    bool allFound = true;
    for (i=0; i<ires; i++) {
      if (!rfound[i]) allFound = false;
    }
    if (allFound) {
      for (i=0; i<ires; i++) {
        int iat = rename_i[i];
        int jat = rename_j[i];
        atm = atomlist[rlist[i]];
        string atName = atomName()[jat].c_str();
        const char* cptr = atName.c_str();
        atm->atomName(cptr);
        rstat[iat] = 1;
        sstat[jat] = 1;
      }
    } else {
      return Residue::UNKNOWN;
    }
  }

// Residue is complete, incomplete, or generatable. Before determining
// status, modify atoms to include atom types, partial charges,
// polarizabilities, charge groups, and polarization groups obtained from
// segment.

  for (i=0; i<ratoms; i++) {
    atm = atomlist[i]; 
    string ratomName = atm->atomName();
    for (j=0; j<satoms; j++) {
      string satomName = p_atomName[j]; 
      if (ratomName == satomName) {
        atm->atomType(p_atomType1[j].c_str());
        atm->partialCharge(p_charge1[j]);
        atm->polarizability(p_plrzblty1[j]);
        atm->chargeGroup(p_chrgGrp[j]);
        atm->polarizationGroup(p_plrzGrp[j]);
        atm->connectionIndex(p_linkNum[j]);
        atm->dihedralIndex(p_envType[j]);
        break;
      }
    }
  }

// Finish determining residue status

  bool cmplt = true;
  bool incmplt = false;
  bool gnrtbl = true;

  for (i=0; i<satoms; i++) {
    if (sstat[i] == 0) {

// Check to see if missing atom is a hydrogen

      string atsym = p_atomName[i];
      if ((atsym[0] == ' ' || isdigit(atsym[0]) != 0) &&
           atsym[1] == 'H') {
        cmplt = false;
      } else {
        cmplt = false;
        incmplt = true;
        gnrtbl = false;
      }
    }
  }
  Residue::ResidueStatus status;
  if (cmplt) status = Residue::COMPLETE;
  if (gnrtbl && !cmplt) status = Residue::GENERATABLE;
  if (incmplt) return Residue::INCOMPLETE;
  if (!cmplt && !gnrtbl) return Residue::UNKNOWN;

// Residue is either COMPLETE or GENERATABLE. Check to see if all bonds
// are present. If not add them. Start by constructing a map between
// the segment atoms and the available residue atoms.

  FragUtil *fragutil = new FragUtil(frag);
  vector<int> smap;
// ratnam is a kluge since atomName returns an RWCString.
  string ratnam;
  for (i = 0; i<satoms; i++) {
    smap.push_back(-1);
    for (j = 0; j<ratoms; j++) {
      ratnam = atomlist[j]->atomName();
      if (ratnam == p_atomName[i]) smap[i] = j;
    }
  }

// Now check to make sure that bonds that are suppose to be present are
// actually present.

  TAtm *atm1, *atm2;
  int nubs1, nubs2, catoms;
  bool bondExists;
  for (k = 0; k<p_nbond; k++) {
    i = p_iIndex[k];
    j = p_jIndex[k];
    if (smap[i] >= 0 && smap[j] >= 0) {
      atm1 = atomlist[smap[i]];
      atm2 = atomlist[smap[j]];
      vector<TAtm*>* catm1 = atm1->connectedAtoms();
      vector<TAtm*>* catm2 = atm2->connectedAtoms();
      nubs1 = 0;
      nubs2 = 0;
      bondExists = false;
      catoms = catm1->size();
      for (i=0; i<catoms; i++) {
        atm = (*catm1)[i];
        if (atm2 == atm) {
          bondExists = true;
          break;
        }
        if (atm->atomicSymbol() == "Nub") nubs1++;
      }
      catoms = catm2->size();
      for (i=0; i<catoms && !bondExists; i++) {
        atm = (*catm2)[i];
        if (atm1 == atm) {
          bondExists = true;
          break;
        }
        if (atm->atomicSymbol() == "Nub") nubs2++;
      }
      if (!bondExists) {
        i = atm1->index();
        j = atm2->index();
        frag->addBond(i,j,1.0);

// remove all nubs and then reset atom shape

        if (nubs1 > 0) {
          frag->deleteNubs(atm1);
          fragutil->changeGeometry(atm1,
             atm1->atomicSymbol(),
             atm1->shapeString());
        }
        if (nubs2 > 0) {
          frag->deleteNubs(atm2);
          fragutil->changeGeometry(atm2,
             atm2->atomicSymbol(),
             atm2->shapeString());
        }
      }
      delete catm1;
      delete catm2;
    }
  } 
  delete fragutil;
  return status;
}
///////////////////////////////////////////////////////////////////////////////
// Description:
//   Compare atom list with segment and determine if any of the atoms
//   can be assigned atom names and atom types.
///////////////////////////////////////////////////////////////////////////////
void Segment::assignResidueAtoms(Residue* res, vector<TAtm*> atomlist)
{

// Find atoms with no atom names.

  int size = atomlist.size();
  vector<bool> assigned;
  int i,j,k,l;
  int ilen;
  bool namchk;
  string name;
  TPerTab pertab;
  string resname = res->name();
  int resnum = res->number();
  char icode = res->insertCode();
  char ichain = res->chain();
  for (i=0; i<size; i++) {
    TAtm* atm = atomlist[i];
    name = atm->atomName();
    ilen = name.length();
    namchk = false;
    for (j=0; j<ilen; j++) {
      if (name[j] != ' ') namchk = true;
    }
    if (namchk || atm->atomicSymbol() == "Nub") {
      assigned.push_back(true);
    } else {
      assigned.push_back(false);
    }
  }

// Atoms without names have been found. Now try and assign names.
// Start by scanning through list of residue atoms and comparing
// the list of known residue atoms with segment atoms. Label
// segment atoms that already appear in list of residue atoms as
// already found. Also extract atomic symbols from atom names of
// all atoms in segment.

  vector<bool> found;
  vector<string> sgsymbol;
  string rptr;
  string sptr;
  for (j=0; j<p_natom; j++) {
    found.push_back(false);
    sptr = p_atomName[j];
    char atsym[3];
    if (sptr[0] == ' ' || isdigit(sptr[0]) != 0) {
      atsym[0] = toupper(sptr[1]);
      atsym[1] = '\0';
    } else {
      atsym[0] = toupper(sptr[0]);
      atsym[1] = tolower(sptr[1]);
      atsym[2] = '\0';
    }
    string symbol(atsym);
    sgsymbol.push_back(symbol);
  }
  for (i=0; i<size; i++) {
    if (assigned[i]) {
      TAtm* atm = atomlist[i];
      rptr = atm->atomName();
      for (j=0; j<p_natom; j++) {
        sptr = p_atomName[j];
        if (rptr == sptr) {
          found[j] = true;
        } 
      }
    }
  }

// Start comparing residue atoms that haven't been assigned with
// segment atoms that have not already been used and see if
// there are any correspondences.

  bool madechange = true;
  while (madechange) {
    madechange = false;
    for (i=0; i<size; i++) {
      if (!assigned[i]) {
        TAtm* atm = atomlist[i];
        rptr = atm->atomicSymbol();

// First check to see how many atoms of the same element are
// available.

        vector<int> sametype;
        vector<int> stillcompatible;
        for (j=0; j<p_natom; j++) {
          if (!found[j]) {
            sptr = sgsymbol[j];
            if (rptr == sptr) {
              sametype.push_back(j);
            }
          }
        }

// Create list of element types to which residue atom is bounded.
// Create two integer arrays corresponding to the atomic elements.

        int psize = pertab.numAtoms();
        vector<int> rsym;
        vector<int> ssym;
        for (j=0; j<150; j++) {
          rsym.push_back(0);
          ssym.push_back(0);
        }

        vector<TAtm*>* rbonded = atm->connectedAtoms();
        int rsize = rbonded->size();
        int iat,jat,kat;

// Find identities of all atoms that are bonded to the residue atom.
// Only include bonded neighbors that are on the same residue. Ignore
// Nubs.

        for (j=0; j<rsize; j++) {
          TAtm* catm = (*rbonded)[j];
          iat = catm->atomicNumber();
          rptr = catm->residueName();
          int ires = catm->residueNumber();
          char ic = catm->icode();
          char ch = catm->chain();
          sptr = catm->atomicSymbol();
          if (resname == rptr && ires == resnum &&
              ic == icode && ch == ichain && sptr != "Nub") {
            rsym[iat]++;
          }
        }
        

// Scan through segment atoms that have the same atomic symbol
// and see if segment atom has compatible bonding pattern.

        for (j=0; j<sametype.size(); j++) {
          jat = sametype[j];

          for (k=0; k<=psize; k++) {
            ssym[k] = 0;
          }
          
// Form list of all segment atoms that are bonded to jat.

          vector<int> connected;
          int ibond,jbond;
          for (k=0; k<p_nbond; k++) {
            ibond = p_iIndex[k];
            jbond = p_jIndex[k];
            if (jat == ibond) {
              connected.push_back(jbond);
            } else if (jat == jbond) {
              connected.push_back(ibond);
            }
          }
          
          for (k=0; k<connected.size(); k++) {
            kat = connected[k];
            sptr = sgsymbol[kat];
            string atsym(sptr.c_str());
            ssym[pertab.atomicNumber(atsym)]++;
          }

// First check to see of residue atom is bonded to more atoms then
// segment atom.

          bool compatible=true;
          for (k=0; k<psize; k++) {
            if (rsym[k] > ssym[k]) compatible = false;
          }

// Next, check to see if residue atom is bonded to an atom with a name
// that does not correspond to a bonded neighbor in the segment
// file.
 
          if (compatible) {
            for (k=0; k<rsize; k++) {
              TAtm* catm = (*rbonded)[k];

// Only check if neighbor bonded to residue atom is also on residue.

              rptr = catm->residueName();
              int ires = catm->residueNumber();
              char ic = catm->icode();
              char ch = catm->chain();
              bool notfound = false;
              if (resname == rptr && ires == resnum &&
                  ic == icode && ch == ichain) {
                if (catm->atomName().length()>0) {
                  bool blank = true;
                  for (l=0; l<catm->atomName().length(); l++) {
                    if (catm->atomName()[l] != ' ') blank = false;
                  }
                  if (!blank) {
                    notfound = true;
                    rptr = catm->atomName();
                    for (l=0; l<connected.size(); l++) {
                      kat = connected[l];
                      string sptr = p_atomName[kat];
                      if (rptr == sptr) notfound=false;
                    }
                  }
                  if (notfound) compatible = false;
                } 
              }
            }
          }

// Mark atom if it is still compatible

          if (compatible) stillcompatible.push_back(jat);
        }

// We have now found all atoms in the segment that are compatible
// with this residue atom. If there is only one atom in the segment
// that is compatible, then we are done. If more than one atom is
// compatible, then we need to do some more checking to see if we can
// can safely make an assignment. 

        if (stillcompatible.size() == 1) {
          jat = stillcompatible[0];
          atm->atomName(p_atomName[jat].c_str());
          assigned[i] = true;
          madechange = true;
          found[jat] = true;
        } else if (stillcompatible.size() > 1) {

// Look at compatible segment atoms and see if one of the bonded
// neighbor atoms in the segment corresponds to an assigned atom in
// the residue. If so, check to see if the residue atom is bonded to
// the same atom. Assign the residue atom if there is a match.

          for (k=0; k<stillcompatible.size(); k++) {
            jat = stillcompatible[k];
            int ibond,jbond;
            int kat;
            for (l=0; l<p_nbond; l++) {
              ibond = p_iIndex[l];
              jbond = p_jIndex[l];
              bool kfnd = false;
              if (jat == ibond) {
                kat = jbond;
                kfnd = true;
              } else if (jat == jbond) {
                kat = ibond;
                kfnd = true;
              }
              if (kfnd && found[kat]) {
                bool match = false;
                string sptr(p_atomName[kat].c_str());
                for (j=0; j<rsize; j++) {
                  TAtm* catm=(*rbonded)[j];
                  string rptr(catm->atomName().c_str());
                  if (sptr == rptr) {
                    match = true;
                    break;
                  }
                }
                if (match) {
                  atm->atomName(p_atomName[jat].c_str());
                  assigned[i] = true;
                  found[jat] = true;
                  madechange = true;
                  break;
                }
              }
            }
            if (assigned[i]) break;
          }
          if (assigned[i]) break;
        }
        delete rbonded;
      }
    }
  }
  return;
}
///////////////////////////////////////////////////////////////////////////////
// Description:
//   Accessor functions for segment data
///////////////////////////////////////////////////////////////////////////////
string Segment::name() const
{
    return p_segName;
}
int Segment::natom() const
{
    return p_natom;
}
int Segment::nbond() const
{
    return p_nbond;
}
int Segment::nangle() const
{
    return p_nangle;
}
int Segment::ntorsion() const
{
    return p_ntorsion;
}
int Segment::nimproper() const
{
    return p_nimproper;
}
vector<int> Segment::atSeqNum() const
{
    return p_atSeqNum;
}
vector<string> Segment::atomName() const
{
    return p_atomName;
}
vector<string> Segment::atomType1() const
{
    return p_atomType1;
}
vector<string> Segment::atomType2() const
{
    return p_atomType2;
}
vector<string> Segment::atomType3() const
{
    return p_atomType3;
}
vector<char> Segment::dynType1() const
{
    return p_dynType1;
}
vector<char> Segment::dynType2() const
{
    return p_dynType2;
}
vector<char> Segment::dynType3() const
{
    return p_dynType3;
}
vector<short> Segment::chrgGrp() const
{
    return p_chrgGrp;
}
vector<short> Segment::plrzGrp() const
{
    return p_plrzGrp;
}
vector<short> Segment::linkNum() const
{
    return p_linkNum;
}
vector<int> Segment::envType() const
{
    return p_envType;
}
vector<double> Segment::charge1() const
{
    return p_charge1;
}
vector<double> Segment::charge2() const
{
    return p_charge2;
}
vector<double> Segment::charge3() const
{
    return p_charge3;
}
vector<double> Segment::plrzblty1() const
{
    return p_plrzblty1;
}
vector<double> Segment::plrzblty2() const
{
    return p_plrzblty2;
}
vector<double> Segment::plrzblty3() const
{
    return p_plrzblty3;
}
vector<int> Segment::bndSeqNum() const
{
    return p_bndSeqNum;
}
vector<int> Segment::iIndex() const
{
    return p_iIndex;
}
vector<int> Segment::jIndex() const
{
    return p_jIndex;
}
vector<int> Segment::bndType() const
{
    return p_bndType;
}
///////////////////////////////////////////////////////////////////////////////
// Description:
//   Modifier functions for segment data
///////////////////////////////////////////////////////////////////////////////
void Segment::name(const string & segname)
{
    p_segName = segname;
    return;
}
void Segment::natom(const int & nat)
{
    p_natom = nat;
    return;
}
void Segment::nbond(const int & nbnd)
{
    p_nbond = nbnd;
    return;
}
void Segment::nangle(const int & nangl)
{
    p_nangle = nangl;
    return;
}
void Segment::ntorsion(const int & ntors)
{
    p_ntorsion = ntors;
    return;
}
void Segment::nimproper(const int & nimp)
{
    p_nimproper = nimp;
    return;
}
void Segment::atSeqNum(const vector<int> & atsq)
{
    p_atSeqNum = atsq;
    return;
}
void Segment::atomName(const vector<string> & atnam)
{
    p_atomName = atnam;
    return;
}
void Segment::atomType1(const vector<string> & atyp)
{
    p_atomType1 = atyp;
    return;
}
void Segment::atomType2(const vector<string> & atyp)
{
    p_atomType2 = atyp;
    return;
}
void Segment::atomType3(const vector<string> & atyp)
{
    p_atomType3 = atyp;
    return;
}
void Segment::dynType1(const vector<char> & dtyp)
{
    p_dynType1 = dtyp;
    return;
}
void Segment::dynType2(const vector<char> & dtyp)
{
    p_dynType2 = dtyp;
    return;
}
void Segment::dynType3(const vector<char> & dtyp)
{
    p_dynType3 = dtyp;
    return;
}
void Segment::chrgGrp(const vector<short> & grp)
{
    p_chrgGrp = grp;
    return;
}
void Segment::plrzGrp(const vector<short> & grp)
{
    p_plrzGrp = grp;
    return;
}
void Segment::linkNum(const vector<short> & link)
{
    p_linkNum = link;
    return;
}
void Segment::envType(const vector<int> & etyp)
{
    p_envType = etyp;
    return;
}
void Segment::charge1(const vector<double> & chrg)
{
    p_charge1 = chrg;
    return;
}
void Segment::charge2(const vector<double> & chrg)
{
    p_charge2 = chrg;
    return;
}
void Segment::charge3(const vector<double> & chrg)
{
    p_charge3 = chrg;
    return;
}
void Segment::plrzblty1(const vector<double> & plrz)
{
    p_plrzblty1 = plrz;
    return;
}
void Segment::plrzblty2(const vector<double> & plrz)
{
    p_plrzblty2 = plrz;
    return;
}
void Segment::plrzblty3(const vector<double> & plrz)
{
    p_plrzblty3 = plrz;
    return;
}
void Segment::bndSeqNum(const vector<int> & bsq)
{
    p_bndSeqNum = bsq;
    return;
}
void Segment::iIndex(const vector<int> & idx)
{
    p_iIndex = idx;
    return;
}
void Segment::jIndex(const vector<int> & idx)
{
    p_jIndex = idx;
    return;
}
void Segment::bndType(const vector<int> & btyp)
{
    p_bndType = btyp;
    return;
}
