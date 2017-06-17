#include <iostream>
using std::ios;
using std::cout;
using std::endl;

#include <stdio.h>
#include <string.h>

#include "util/STLUtil.H"

#include "tdat/FragmentSummary.H"
#include "tdat/TAtm.H"
#include "tdat/Fragment.H"

/**
 * Initialize all values to something harmless
 */
FragmentSummary::FragmentSummary()
{
  init();
}

FragmentSummary::~FragmentSummary()
{
}

void FragmentSummary::init()
{
  p_xmin = 0.0;
  p_xmax = 0.0;
  p_ymin = 0.0;
  p_ymax = 0.0;
  p_zmin = 0.0;
  p_zmax = 0.0;
  p_natom = 0;
  p_iatm1 = -1;
  p_iatm2 = -1;
  p_iatm3 = -1;
  p_iparent1 = -1;
  p_iparent2 = -1;
  p_iparent3 = -1;
  p_atnam1 = "";
  p_atnam2 = "";
  p_atnam3 = "";

  // The following are not part of the streams and so don't need to 
  // be in update() either.
  p_selectedModel = 1;
  p_altLocation = "";
  p_chain = "";
  p_name = "";
}

void FragmentSummary::update(Fragment *frag, vector<int> selection)
{
  init();
  double dims[6];
  frag->getBoxDimensions(dims);
  p_xmin = dims[0];
  p_xmax = dims[1];
  p_ymin = dims[2];
  p_ymax = dims[3];
  p_zmin = dims[4];
  p_zmax = dims[5];

  p_natom = selection.size();

  p_iatm1 = -1;
  p_iparent1 = -1;
  p_atnam1 = "";
  p_iatm2 = -1;
  p_iparent2 = -1;
  p_atnam2 = "";
  p_iatm3 = -1;
  p_iparent3 = -1;
  p_atnam3 = "";

  // Assign atoms 1,2, and 3 if 3 or less atoms are selected.
  if (p_natom > 0 && p_natom <= 3) {
    int idx = selection[0];
    TAtm *atm = frag->atomRef(idx);
    p_iatm1 = idx;
    p_atnam1 = atm->atomName();
    if (frag->numResidues() > 0) {
      p_iparent1 = atm->residueNumber();
    }
  }
  if (p_natom > 1 && p_natom <= 3) {
    int idx = selection[1];
    TAtm *atm = frag->atomRef(idx);
    p_iatm2 = idx;
    p_atnam2 = atm->atomName();
    if (frag->numResidues() > 0) {
      p_iparent2 = atm->residueNumber();
    }
  }
  if (p_natom == 3) {
    int idx = selection[2];
    TAtm *atm = frag->atomRef(idx);
    p_iatm3 = idx;
    p_atnam3 = atm->atomName();
    if (frag->numResidues() > 0) {
      p_iparent3 = atm->residueNumber();
    }
  }

  // Assign atoms 1 and 2 to the smallest and largest index of the
  // atoms selected if selection is 4 or more. Start by finding the
  // largest and smallest indices in selection
  int imin, imax;
  if (p_natom > 3) {
    imin = selection[0];
    imax = imin;
    for (int i=1; i<p_natom; i++) {
      if (selection[i] < imin)
        imin = selection[i];
      if (selection[i] > imax)
        imax = selection[i];
    }
    int idx = imin;
    TAtm *atm = frag->atomRef(idx);
    p_iatm1 = idx;
    p_atnam1 = atm->atomName();
    if (frag->numResidues() > 0) {
      p_iparent1 = atm->residueNumber();
    }

    // Get last selected atom
    idx = imax;
    atm = frag->atomRef(idx);
    p_iatm2 = idx;
    p_atnam2 = atm->atomName();
    if (frag->numResidues() > 0) {
      p_iparent2 = atm->residueNumber();
    }
  }
}

/**
 * Accessors
 */
void FragmentSummary::getDimensions(double *dimensions) const
{
  dimensions[0] = p_xmin;
  dimensions[1] = p_xmax;
  dimensions[2] = p_ymin;
  dimensions[3] = p_ymax;
  dimensions[4] = p_zmin;
  dimensions[5] = p_zmax;
}

int FragmentSummary::getNumSelected() const
{
  return p_natom;
}

int FragmentSummary::getAtom(int idx) const
{
  if (idx == 0) {
    return p_iatm1;
  } else if (idx == 1) {
    return p_iatm2;
  } else {
    return p_iatm3;
  }
}

int FragmentSummary::getParent(int idx) const
{
  if (idx == 0) {
    return p_iparent1;
  } else if (idx == 1) {
    return p_iparent2;
  } else {
    return p_iparent3;
  }
}

string FragmentSummary::getAtomName(int idx) const
{
  if (idx == 0) {
    return p_atnam1;
  } else if (idx == 1) {
    return p_atnam2;
  } else {
    return p_atnam3;
  }
}

string FragmentSummary::getAtomString(int idx) const
{
  string ret, tmpStr;
  int ires;
  char buf[120];
  if (idx == 0) {
    tmpStr = p_atnam1;
    ires = p_iparent1;
  } else if (idx == 1) {
    tmpStr = p_atnam2;
    ires = p_iparent2;
  } else {
    tmpStr = p_atnam3;
    ires = p_iparent3;
  }
  if (tmpStr[0] == ' ') tmpStr[0] = '_';
  sprintf(buf,"%d:%s",ires,tmpStr.c_str());
  ret = buf;
  return ret;
}

int FragmentSummary::getSelectedModel() const
{
   return p_selectedModel;
}
string FragmentSummary::getAltLocation() const
{
   return p_altLocation;
}
string FragmentSummary::getChain() const
{
   return p_chain;
}
string FragmentSummary::getName() const
{
   return p_name;
}

/**
 * Modifiers
 */
void FragmentSummary::setDimensions(double *dimensions)
{
  p_xmin = dimensions[0];
  p_xmax = dimensions[1];
  p_ymin = dimensions[2];
  p_ymax = dimensions[3];
  p_zmin = dimensions[4];
  p_zmax = dimensions[5];
}

void FragmentSummary::setNumSelected(int num)
{
  p_natom = num;
}

void FragmentSummary::setAtom(int iatm, int idx)
{
  if (idx == 0) {
    p_iatm1 = iatm;
  } else if (idx == 1) {
    p_iatm2 = iatm;
  } else {
    p_iatm3 = iatm;
  }
}

void FragmentSummary::setParent(int ires, int idx)
{
  if (idx == 0) {
    p_iparent1 = ires;
  } else if (idx == 1) {
    p_iparent2 = ires;
  } else {
    p_iparent3 = ires;
  }
}

void FragmentSummary::setAtomName(string name, int idx)
{
  if (idx == 0) {
    p_atnam1 = name;
  } else if (idx == 1) {
    p_atnam2 = name;
  } else {
    p_atnam3 = name;
  }
}

void FragmentSummary::setSelectedModel(int model)
{
   p_selectedModel = model;
}
void FragmentSummary::setAltLocation(const string& loc)
{
   p_altLocation = loc;
}
void FragmentSummary::setChain(const string& chain)
{
   p_chain = chain;
}
void FragmentSummary::setName(const string& name)
{
   p_name = name;
}


/**
 * Do a quick scan of a PDB file to initialize FragmentSummary
 */
bool FragmentSummary::scanPDB(istream& infile)
{
  bool ret = true;
  char* cptr;
  char buf[1025];
  char record[7];
  char d8[9];
  double x,y,z;
  bool notFirst = false;
  p_xmin = 0.0;
  p_xmax = 0.0;
  p_ymin = 0.0;
  p_ymax = 0.0;
  p_zmin = 0.0;
  p_zmax = 0.0;

  record[6] = '\0';
  d8[8] = '\0';

  infile.seekg(0, ios::beg);
  infile.clear();
  if (!infile.good() || infile.eof()) return false;
 
  while (infile.getline(buf,1025)) {
    cptr = buf;
    strncpy(record,cptr,6);
    cptr += 6;

// Check to see if first six characters correspond to any of the records
// currently being supported (ATOM, HETATM)

    if (strncmp(record,"ATOM  ",6) == 0 ||
        strncmp(record,"HETATM",6) == 0) {

      /*  cptr += 6;
        cptr += 4;
        cptr += 1;
        cptr += 4;
        cptr += 1;
        cptr += 4;
        cptr += 4; */
        cptr += 24;
        strncpy(d8,cptr,8);
        x = atof(d8);
        cptr += 8;
        strncpy(d8,cptr,8);
        y = atof(d8);
        cptr += 8;
        strncpy(d8,cptr,8);
        z = atof(d8);
        cptr += 8;
        if (notFirst) {
          if (x < p_xmin) p_xmin = x;
          if (x > p_xmax) p_xmax = x;
          if (y < p_ymin) p_ymin = y;
          if (y > p_ymax) p_ymax = y;
          if (z < p_zmin) p_zmin = z;
          if (z > p_zmax) p_zmax = z;
        } else {
          notFirst = true;
          p_xmax = x;
          p_xmin = x;
          p_ymax = y;
          p_ymin = y;
          p_zmax = y;
          p_zmin = y;
        }
    }
  }

  return ret;
}

/**
 * Overwrite streaming operators to import and export internal FragmentSummary
 * state
 */
ostream& operator<<(ostream& os, const FragmentSummary& summary)
{
  double dimensions[6];
  summary.getDimensions(dimensions);
  int natom = summary.getNumSelected();
  os <<dimensions[0]<<" "<<dimensions[1]<<" ";
  os <<dimensions[2]<<" "<<dimensions[3]<<" ";
  os <<dimensions[4]<<" "<<dimensions[5]<<" ";
  os <<natom;
  /**
   * Add underscores around atom name to protect against blank names
   * and replace remaining blanks with underscores
   */
  if (natom > 0) {
    string atom = summary.getAtomName(0);
    for (int i=0; i<atom.size(); i++) {
      if (atom[i] == ' ') atom[i] = '_';
    }
    os <<" "<<summary.getAtom(0)<<" "<<summary.getParent(0)
       <<" _"<<atom<<"_";
  }
  if (natom > 1) {
    string atom = summary.getAtomName(1);
    for (int i=0; i<atom.size(); i++) {
      if (atom[i] == ' ') atom[i] = '_';
    }
    os <<" "<<summary.getAtom(1)<<" "<<summary.getParent(1)
       <<" _"<<atom<<"_";
  }
  if (natom == 3) {
    string atom = summary.getAtomName(2);
    for (int i=0; i<atom.size(); i++) {
      if (atom[i] == ' ') atom[i] = '_';
    }
    os <<" "<<summary.getAtom(2)<<" "<<summary.getParent(2)
       <<" _"<<atom<<"_";
  }
  os <<endl;
  return os;
}

istream& operator>>(istream& is, FragmentSummary& summary)
{
  double dimensions[6];
  int natom;
  is >> dimensions[0] >> dimensions[1];
  is >> dimensions[2] >> dimensions[3];
  is >> dimensions[4] >> dimensions[5];
  is >> natom;
  summary.setDimensions(dimensions);
  summary.setNumSelected(natom);
  if (natom > 0) {
    int idx, iparent;
    string name;
    is >> idx >> iparent >> name;
    /**
     * Remove underscores from end either end of name
     */
    string newName;
    if (name.size() > 2) {
      newName = name.substr(1,name.size()-2);
    } else {
      newName = "";
    }
    summary.setAtom(idx,0);
    summary.setParent(iparent,0);
    /**
     * Replace remaining underscores with blanks
     */
    for (int i=0; i<newName.size(); i++) {
      if (newName[i] == '_') newName[i] = ' ';
    }
    summary.setAtomName(newName,0);
  }
  if (natom > 1) {
    int idx, iparent;
    string name;
    is >> idx >> iparent >> name;
    /**
     * Remove underscores from end either end of name
     */
    string newName;
    if (name.size() > 2) {
      newName = name.substr(1,name.size()-2);
    } else {
      newName = "";
    }
    summary.setAtom(idx,1);
    summary.setParent(iparent,1);
    /**
     * Replace remaining underscores with blanks
     */
    for (int i=0; i<newName.size(); i++) {
      if (newName[i] == '_') newName[i] = ' ';
    }
    summary.setAtomName(newName,1);
  }
  if (natom == 3) {
    int idx, iparent;
    string name;
    is >> idx >> iparent >> name;
    /**
     * Remove underscores from end either end of name
     */
    string newName;
    if (name.size() > 2) {
      newName = name.substr(1,name.size()-2);
    } else {
      newName = "";
    }
    summary.setAtom(idx,2);
    summary.setParent(iparent,2);
    /**
     * Replace remaining underscores with blanks
     */
    for (int i=0; i<newName.size(); i++) {
      if (newName[i] == '_') newName[i] = ' ';
    }
    summary.setAtomName(newName,2);
  }
  is.clear(ios::badbit|is.rdstate());             // clear read error, if any
  return is;
}
