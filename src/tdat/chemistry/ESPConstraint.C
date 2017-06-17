////////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: ESPConstraint.C
//
//
// DESIGN:
//   Contains information to specify one ESP constraint
////////////////////////////////////////////////////////////////////////////////

//System includes
#include<set>
  using std::set;
  using std::less;
#include<iostream>
  using std::cout;
  using std::endl;

//Application includes
#include "tdat/TAtm.H"
#include "tdat/ESPConstraint.H"

vector<string> *ESPConstraint::p_typeLabels = new vector<string>;

// Constructor
ESPConstraint::ESPConstraint()
{

}

// Destructor
ESPConstraint::~ESPConstraint()
{
}

// Operator
bool ESPConstraint::operator==(const ESPConstraint& constraint) const
{
  int i,size1,size2;
  if (this->type() != constraint.type()) return false;
  if (this->group1().size() != constraint.group1().size()) return false;
  if (this->group2().size() != constraint.group2().size()) return false;

  // Groups 1 and 2 are the same size in both constraints. Check to see
  // if they have the same atoms.
  size1 = this->group1().size();
  if (size1 > 0) {
    set<int,less<int> > set1;
    for (i=0; i<size1; i++) {
      set1.insert(this->group1()[i]->index());
    }
    for (i=0; i<size1; i++) {
      if (set1.count(constraint.group1()[i]->index()) == 0) return false;
    }
  }
  size2 = this->group2().size();
  if (size2 > 0) {
    set<int,less<int> > set2;
    for (i=0; i<size2; i++) {
      set2.insert(this->group2()[i]->index());
    }
    for (i=0; i<size2; i++) {
      if (set2.count(constraint.group2()[i]->index()) == 0) return false;
    }
  }
  return true;
}

void ESPConstraint::initializeLabels()
{
  // Statically defined labels for the gui
  if (p_typeLabels->size() == 0) {
    p_typeLabels = new vector<string>;
    p_typeLabels->push_back("Fix atom charge");
    p_typeLabels->push_back("All charge equal");
    p_typeLabels->push_back("Total charge fixed");
    p_typeLabels->push_back("Constrain group1 to group2");
    p_typeLabels->push_back("Constrain Methyl group");
    p_typeLabels->push_back("Constrain special (Methyl type)");
  }
}

//Accessors
ESPConstraint::Type ESPConstraint::type() const
{
  return p_type;
}

vector<TAtm*> ESPConstraint::group1() const
{
  return p_group1;
}

vector<TAtm*> ESPConstraint::group2() const
{
  return p_group2;
}

double ESPConstraint::charge() const
{
  return p_charge;
}

string ESPConstraint::annotation() const
{
  return p_annotation;
}

bool ESPConstraint::chargeApplies() const
{
  bool ret = false;

  if (p_type == ESPConstraint::NET || p_type == ESPConstraint::FIXED) 
    ret = true;

  return ret;
}

bool ESPConstraint::group2Applies() const
{
  bool ret = false;

  if (p_type == ESPConstraint::GROUP || p_type == ESPConstraint::SPECIAL)
    ret = true;

  return ret;
}

//Modifiers
void ESPConstraint::type(ESPConstraint::Type type)
{
  p_type = type;
}

void ESPConstraint::group1(vector<TAtm*> list)
{
  p_group1 = list;
  // Compute group 2 automatically in some cases
  // For now do it just for atom 1
  if (p_type == ESPConstraint::METHYL && p_group1.size() == 1) {
    p_group2.clear();
    TAtm *atom = p_group1[0];
    vector<TAtm*> *toatoms = atom->connectedAtoms();
    for (int adx=0; adx<toatoms->size(); adx++) {
      if ((*toatoms)[adx]->atomicNumber() == 1) {
        p_group1.push_back((*toatoms)[adx]);
      }
    }
    delete toatoms;
  } else if ((p_type == ESPConstraint::METHYL ||
              p_type == ESPConstraint::SPECIAL) &&
             p_group1.size() > 1) {
    p_group2.clear();
  }
}

void ESPConstraint::group2(vector<TAtm*> list)
{
  p_group2 = list;
}

void ESPConstraint::charge(double charge)
{
  p_charge = charge;
}

void ESPConstraint::annotation(string comment)
{
  p_annotation = comment;
}

/**
 * Determine whether enough data is available to create a valid constraint.
 * @param emptyOk - if true errors, incomplete groups are not reported.
 * @return If constraint is valid, return the empty string "", otherwise 
 *         return a descriptive message.
 */
string ESPConstraint::isValid() const
{
  string ret = "";
  // check that atoms in group 1 and group 2 are complementary
  if (p_group2.size() > 0) {
    int i,j;
    for (i=0; i<p_group1.size(); i++) {
      for (j=0; j<p_group2.size(); j++) {
        if (p_group1[i] == p_group2[j]) {
          ret = "Groups 1 and 2 share some atoms.";
          return ret;
        }
      }
    }
  }
  if (p_type == ESPConstraint::NET) {
    if (p_group1.size() == 0) {
      ret = "There are no atoms in group 1.";
    } else {
      // Check to make sure that atoms are in consecutive order
      int amin = p_group1[0]->index();
      int amax = amin;
      int i;
      for (i=1; i<p_group1.size(); i++) {
        if (amin > p_group1[i]->index()) {
          amin = p_group1[i]->index();
        }
        if (amax < p_group1[i]->index()) {
          amax = p_group1[i]->index();
        }
      }
      if (amax - amin + 1 != p_group1.size()) {
        ret = "Group does not consist of a block of consective atoms.";
      }
    }
  } else if ( p_type == ESPConstraint::FIXED) {
    if (p_group1.size() != 1)
      ret = "Only one atom should be specified.";
  } else if (p_type == ESPConstraint::EQUAL) {
    if (p_group1.size() < 2)
      ret = "More than 1 atom is required.";
  } else if (p_type == ESPConstraint::GROUP) {
    if (p_group1.size() == 0 || p_group1.size() != p_group2.size())
      return "Groups 1 and 2 must be the same size.";
    // Check to make sure that atoms are in consecutive order
    int amin = p_group1[0]->index();
    int amax = amin;
    int i;
    for (i=1; i<p_group1.size(); i++) {
      if (amin > p_group1[i]->index()) {
        amin = p_group1[i]->index();
      }
      if (amax < p_group1[i]->index()) {
        amax = p_group1[i]->index();
      }
    }
    if (amax - amin + 1 != p_group1.size()) {
      ret = "Group 1 does not consist of a block of consective atoms.";
    }
    amin = p_group2[0]->index();
    amax = amin;
    for (i=1; i<p_group2.size(); i++) {
      if (amin > p_group2[i]->index()) {
        amin = p_group2[i]->index();
      }
      if (amax < p_group2[i]->index()) {
        amax = p_group2[i]->index();
      }
    }
    if (amax - amin + 1 != p_group2.size()) {
      ret = "Group 2 does not consist of a block of consective atoms.";
    }
    // Check that atoms in group 1 correspond to atoms in group 2
    for (i=0; i<p_group1.size(); i++) {
      if (p_group1[i]->atomicNumber() != p_group2[i]->atomicNumber()) {
        ret = "Groups 1 and 2 must contain the same elements. Also,";
        ret.append(" check to make sure that atoms in both groups were");
        ret.append(" selected in the same order.");
        return ret;
      }
    }
  } else if (p_type == ESPConstraint::SPECIAL ||
             p_type == ESPConstraint::METHYL) {
//    if (p_group1.size() != 1 || p_group2.size() == 0) {
//      ret = "Only one atom allowed in group 1 and at least 1 atom";
//      ret.append(" is required in group 2. Did you select only one atom");
//      ret.append(" when selecting groups?");
//      return ret;
//      cout<< "ret: "<<ret<<endl;
//    }
    // Check to see if atoms in group 2 are attached to the single atom in
    // group 1
    if (p_type == ESPConstraint::SPECIAL) {
      if (p_group1.size() != 1) {
        ret = "Only 1 atom allowed in group 1";
      } else if (p_group2.size() == 0) {
        ret = "Some atoms required in group 2";
      }
    }

    vector<TAtm*>* neighbors = p_group1[0]->connectedAtoms();
    set<int, less<int> > neighborIndices;
    int i;
    for (i=0; i<neighbors->size(); i++) {
      neighborIndices.insert((*neighbors)[i]->index());
    }
    set<int, less<int> >::iterator it = neighborIndices.end();
    if (p_type == ESPConstraint::METHYL) {
      for (i=1; i<p_group1.size(); i++) {
        if (neighborIndices.find(p_group1[i]->index()) == it) {
          ret = "First atom in group 1 not attached to remaining atoms";
        }
      }
    } else {
      for (i=0; i<p_group2.size(); i++) {
        if (neighborIndices.find(p_group2[i]->index()) == it) {
          ret = "First atom in group 1 not attached to atoms in group 2";
        }
      }
    }
    delete neighbors;

    if (p_type == ESPConstraint::METHYL) {
      // Check to see if atom in group 1 is a carbon and all atoms in group 2
      // are hydrogen
      if (p_group1[0]->atomicNumber() != 6) {
        ret = "Central atom must be a carbon";
        return ret;
      }
      for (i=1; i<p_group1.size(); i++) {
        if (p_group1[i]->atomicNumber() != 1) {
          ret = "Attached atoms must be hydrogens";
          return ret;
        }
      }
    }
  }
  return ret;
}

////////////////////////////////////////////////////////////////////////////////
//  Method: makeValid
//
//  Description:
//    This is used when converting a constraint from one type to another. If
//    existing data is incompatible with the requirements of the new constraint
//    then it is discarded.
////////////////////////////////////////////////////////////////////////////////
void ESPConstraint::makeValid(void)
{
  if (p_type == ESPConstraint::NET) {
    p_group2.clear();
  } else if (p_type == ESPConstraint::FIXED) {
    if (p_group1.size() > 0) {
      p_group1.clear();
    }
    p_group2.clear();
  } else if (p_type == ESPConstraint::EQUAL) {
    p_charge = 0.0;
    p_group2.clear();
  } else if (p_type == ESPConstraint::GROUP) {
    p_charge = 0.0;
    if (p_group1.size() != p_group2.size()) p_group2.clear();
    // Check that atoms in group 1 correspond to atoms in group 2
    int i;
    for (i=0; i<p_group1.size() && p_group2.size() > 0; i++) {
      if (p_group1[i]->atomicNumber() != p_group2[i]->atomicNumber()) {
        p_group2.clear();
      }
    }
  } else if (p_type == ESPConstraint::SPECIAL ||
             p_type == ESPConstraint::METHYL) {
    p_charge = 0.0;
    if (p_group1.size() != 1) {
      p_group1.clear();
      p_group2.clear();
    }
    // Check to see if atoms in group 2 are attached to the single atom in
    // group 1
    if (p_group1.size() > 0) {
      set<int, less<int> > groupAtoms;
      int i;
      for (i=0; i<p_group2.size(); i++) {
        groupAtoms.insert(p_group2[i]->index());
      }
      vector<TAtm*>* neighbors = p_group1[0]->connectedAtoms();
      for (i=0; i<neighbors->size() && p_group2.size() > 0; i++) {
        p_group2.clear();
      }
      if (p_type == ESPConstraint::METHYL && p_group1.size() > 0 &&
          p_group2.size() > 0) {
      // Check to see if atom in group 1 is a carbon and all atoms in group 2
      // are hydrogen
        if (p_group1[0]->atomicNumber() != 6) {
          p_group1.clear();
          p_group2.clear();
        }
        for (i=0; i<p_group2.size(); i++) {
          if (p_group2[i]->atomicNumber() != 1) {
            p_group2.clear();
          }
        }
      }
    }
  }
  return;
}

bool ESPConstraint::hasAtoms() const
{
  return p_group1.size() > 0 || p_group2.size() > 0; 
}

////////////////////////////////////////////////////////////////////////////////
//  Method: hasAtoms
//
//  Description:
//    Determine whether any of the atoms in the argument list are in involved
//    in the constraint
////////////////////////////////////////////////////////////////////////////////
bool ESPConstraint::hasAtoms(const vector<int>& list) const
{
  // Create hash map of atoms in group
  set<int, less<int> > groupAtoms;
  int icnt = 0, i, nsize;
  nsize = p_group1.size();
  for (i=0; i<nsize; i++) {
    groupAtoms.insert(p_group1[i]->index());
    icnt++;
  }
  nsize = p_group2.size();
  for (i=0; i<nsize; i++) {
    groupAtoms.insert(p_group2[i]->index());
    icnt++;
  }
  // Check to see if any atoms in list are also in groupAtoms
  set<int, less<int> >::iterator it;
  it = groupAtoms.end();
  nsize = list.size();
  bool ret = false;
  for (i=0; i<nsize; i++) {
    if (groupAtoms.find(list[i]) != it) {
      ret = true;
      break;
    }
  }
  return ret;
}

string ESPConstraint::typeToLabel(ESPConstraint::Type type)
{
  initializeLabels();
  if (type >= 0 && (int)type < p_typeLabels->size())
    return (*p_typeLabels)[type];
  else
    return "Undefined";
}

ESPConstraint::Type ESPConstraint::labelToType(const string& type)
{
  initializeLabels();
  ESPConstraint::Type ret = NET;  // Should we default to bad value like -1?
  for (int idx=p_typeLabels->size()-1; idx>=0; idx--) {
    if (type == (*p_typeLabels)[idx]) {
      ret = (ESPConstraint::Type)idx;
      break;
    }
  }
  return ret;
}

////////////////////////////////////////////////////////////////////////////////
//  Method: getDescription
//
//  Description:
//    Return a string containing a short description of the constraint
////////////////////////////////////////////////////////////////////////////////
string ESPConstraint::getDescription(ESPConstraint::Type type)
{
  string ret;
  if (type == ESPConstraint::NET) {
    ret = "The net charge of the selected atoms\n"
          "is constrained to have a specified\n"
          "value. The atoms in the group must be\n"
          "contained in a block of consecutive atoms.";
  } else if (type == ESPConstraint::EQUAL) {
    ret = "All selected atoms are required to be equal.";
  } else if (type == ESPConstraint::GROUP) {
    ret = "The constraints in group 2 are\n"
          "equal to the constraints in group 1.\n"
          "Group 1 and group 2 must have the same\n"
          "number of atoms.";
  } else if (type == ESPConstraint::FIXED) {
    ret = "The selected atom is required to have a fixed charge.";
  } else if (type == ESPConstraint::METHYL) {
    ret = "The net charge of the methyl group is zero\n"
          "and all attached hydrogens are constrained\n"
          "to have the same charge. Select only the\n"
          "central carbon atom for this constraint.";
  } else if (type == ESPConstraint::SPECIAL) {
    ret = "The net charge of the group is zero. All atoms in\n"
          "group 2 are constrained to have the same charge.\n";
  }
  return ret;
}
