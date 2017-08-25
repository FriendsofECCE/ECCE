///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: PointGroup.C
//
//
// DESIGN:
//   This class just provides access to the Reference point group which
//   are stored in a file.  Its used mainly for validation at this point
//   and is not well tested.
//
///////////////////////////////////////////////////////////////////////////////

#include <iostream>
  using std::ostream;

#include <fstream>
  using std::ifstream;

#include "util/ErrMsg.H"
#include "util/KeyValueReader.H"
#include "util/LineReader.H"
#include "util/Ecce.H"
#include "util/STLUtil.H"
#include "util/StringTokenizer.H"

#include "tdat/PointGroup.H"

vector<PointGroup*> *PointGroup::p_extent = 0;
vector<string> PointGroup::p_spnames;

// -----------------------
// Public Member Functions
// -----------------------

// ---------- Constructors ------------
PointGroup::PointGroup(const string& groupName) : p_irreps(0)
{
  initialize();
  EE_ASSERT(PointGroup::isGroup(groupName),EE_FATAL,groupName+" is invalid");
  p_groupName = groupName;
  PointGroup const *ref = get(groupName);
  if (ref) {
    *this = *ref;
  }
 
}

PointGroup::PointGroup(const PointGroup& rhs) : p_irreps(0)
{
  *this = rhs;
}

 
// ---------- Virtual Destructor ------------
PointGroup::~PointGroup(void)
{
  if (p_irreps) delete p_irreps;
}


// ---------- Operators ----------
PointGroup& PointGroup::operator=(const PointGroup& rhs)
{
  if (this != &rhs) {
    p_groupName = rhs.p_groupName;
    int cnt = rhs.p_irreps->size();
    if (!p_irreps) p_irreps = new vector<string>;
    for (int idx=0; idx<cnt; idx++) {
       p_irreps->push_back((*rhs.p_irreps)[idx]);
    }
  }
  return *this;
}

bool PointGroup::operator==(const PointGroup& rhs) const
{
  return p_groupName == rhs.p_groupName;
}

bool PointGroup::operator!=(const PointGroup& rhs) const
{
  return !(*this==rhs);
}


// ------------ Modifiers ------------

// ------------ Accessors ------------
string PointGroup::name(void) const
{ return p_groupName; }

bool PointGroup::isValid(const string& irrepName) const
{
  bool ret=false;
  for (int idx=p_irreps->size()-1; idx>=0; idx--) {
    if (STLUtil::compareString((*p_irreps)[idx],irrepName) == 0) {
      ret = true;
      break;
    }
  }
  return ret; 
}

// ---------- General Methods  ----------
bool PointGroup::isGroup(const string& name)
{
  initialize();
  bool ret=false;
  for (int idx=p_extent->size()-1; idx>=0; idx--) {
    if (STLUtil::compareString((*p_extent)[idx]->name(),name)==0) {
      ret = true;
      break;
    }
  }
  return ret;
}

vector<string> PointGroup::getGroups()
{
  initialize();
   vector<string> names;
   int numGroups = p_extent->size();
   for (int idx=0; idx<numGroups; idx++) {
      names.push_back((*p_extent)[idx]->name());
   }

   return names;
}

vector<string> PointGroup::getSpaceGroups()
{
   initialize();
   return p_spnames;
}

bool PointGroup::isSpaceGroup(const string& name)
{
  initialize();
  bool ret=false;
  for (int idx=p_spnames.size()-1; idx>=0; idx--) {
    if (p_spnames[idx] == name) {
      ret = true;
      break;
    }
  }
  return ret;
}



PointGroup const *PointGroup::get(const string& name) const
{
  PointGroup const *ret=0;
  for (int idx=p_extent->size()-1; idx>=0; idx--) {
    if (STLUtil::compareString((*p_extent)[idx]->name(),name)==0) {
      ret = (*p_extent)[idx];
      break;
    }
  }
  return ret;
}

// --------------------------
// Protected Member Functions
// --------------------------

// ---------- Derived Class Accessors ----------



// ------------------------
// Private Member Functions
// ------------------------
PointGroup::PointGroup(void) : p_irreps(0)
{
}

void  PointGroup::initialize(void)
{
  if (p_extent == (vector<PointGroup*>*)0) {
    p_extent = new vector<PointGroup*>;
    string path = Ecce::ecceDataPrefPath();
    path += "/PointGroups";
    ifstream inFile(path.c_str());
    EE_RT_ASSERT(inFile, EE_FATAL, path+": unable to access");
    if (inFile) {
      KeyValueReader reader(inFile);
      string key,value;
      // Can't use regular constructor since it checks agaist extent
      while (reader.getpair(key,value)) {
        PointGroup *pg = new PointGroup();
        pg->p_groupName = key;
        StringTokenizer tokenizer(value);
        string token;
        pg->p_irreps = new vector<string>;
        while (!(token=tokenizer.next()).empty()) {
          pg->p_irreps->push_back(token);
        }
        p_extent->push_back(pg);

      }
      inFile.close();
    }
    //dump();
  }
  
  if (p_spnames.empty()) {
     string path = Ecce::ecceDataPrefPath();
     path += "/SpaceGroups";
     ifstream inFile(path.c_str());
     EE_RT_ASSERT(inFile, EE_FATAL, path+": unable to access");
     if (inFile) {
        LineReader reader(inFile);
        string line;
        // Can't use regular constructor since it checks agaist extent
        while (reader.getLine(line)) {
           StringTokenizer tokenizer(line," \t,");
           string token;
           while (!(token=tokenizer.next()).empty()) {
              p_spnames.push_back(token);
           }

        }
        inFile.close();
     }
  }
}

void PointGroup::dump(void) 
{
  for (int idx=0; idx<p_extent->size(); idx++) {
    PointGroup *pg = (*p_extent)[idx];
    std::cout << *pg << std::endl;
  }
}

// ---------- Friends ----------
ostream& operator<<(ostream& os, const PointGroup& rhs)
{
  os << rhs.p_groupName << "{";
  int cnt = rhs.p_irreps->size();
  for (int idx=0; idx<cnt; idx++) {
    os << (*rhs.p_irreps)[idx]; 
    if (idx != cnt-1) os << ",";
  }
  os << "}";
  return os;
}


// ---------- Encapsulated Behavior ----------

///////////////////////////////////////////////////////////////////////////////
///  man
//
//  Description
//
//  Implementation
//
///////////////////////////////////////////////////////////////////////////////
