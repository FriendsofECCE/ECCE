//////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: TGBSConfig.C
//
//
// DESIGN:
//
///////////////////////////////////////////////////////////////////////////////

#include <unistd.h>
#include <ctype.h>

#include <iostream>
  using std::cout;
  using std::cerr;
  using std::endl;
  using std::ends;
  using std::ios;
#include <fstream>
  using std::ofstream;
  using std::ifstream;
#include <algorithm>
  using std::sort;
  using std::less;
#include <set>
  using std::set_difference;

#include "util/ErrMsg.H"
#include "util/Ecce.H"
#include "util/CompareOps.H"
#include "util/SFile.H"
#include "util/TempStorage.H"

#include "tdat/TTheory.H"
#include "tdat/TPerTab.H"

#include "dsm/TGBSConfig.H"
#include "dsm/TGaussianBasisSet.H"
#include "dsm/JCode.H"
#include "dsm/CodeFactory.H"

  
// Static conversion arrays

// used for counting functions & primitives:
short TGBSConfig::sph_mult[]  = { 1, 3, 5, 7, 9, 11, 13}; 
short TGBSConfig::cart_mult[] = { 1, 3, 6, 10, 15, 21, 28};

// converts config type to its corresponding string:
const char* TGBSConfig::configType_formatter[] = {
        "simple","element","atom", "UnknownConfigType"
};



/*******************************************************************
 Method : Constructor for TGBSConfigTags
 Summary: This class is used to separate a single string of tags
          into a vector of individual tags, where each tag is an
          element name (i.e., "H", "C", etc.).

          Note that if the input tag was "C H1 H2", the TGBSConfigTags
          default constructor would convert it to "C", "H", "H".
***************************************************** **************/
TGBSConfigTags::TGBSConfigTags
(const char* str, 
  bool sorted,     // defaults to false
  bool elementOnly // defaults to true; indicates whether to strip numerals
)
{
  char* tags = strdup(str);
  char* parseCharacters;
  if(elementOnly)
    parseCharacters = strdup("0123456789 \n");
  else
    parseCharacters = strdup(" \n");

  char* tag = strtok(tags, parseCharacters);
   while(tag) {
     //insert(begin(),tag);
     push_back(tag);
     tag = strtok((char*)0, parseCharacters);
  }

  if(sorted) sort(begin(),end(),less<string>()); // sorts in ascending order 
                                              // but doesnt remove duplicates
  free(tags);
  free(parseCharacters);
}

/*******************************************************************
 Method : findElement
 Summary: Determines whether an element is a tag for one of the groups..
*******************************************************************/
bool TGBSConfigTags::findElement(string elementName)
{
  TGBSConfigTags::iterator it;
  bool match = false;

  for (it = begin(); it != end(); it++)
    if ((*it) == elementName)
    {
      match = true;
      break;
    }
  return match;
}

/*******************************************************************
 Method : Default constructor for TGBSConfig
 Summary: Creates an empty, simple config.
*******************************************************************/
TGBSConfig::TGBSConfig()
{
  p_optimize = false;

  // default to Cartesian because all codes support Cartesian coordinates
  p_coordSys = TGaussianBasisSet::Cartesian;
  p_type = TGBSConfig::simple;

  // maps are empty
}

/*******************************************************************
 Method : Copy constructor for TGBSConfig
 Summary: Performs a deep copy.
*******************************************************************/
TGBSConfig::TGBSConfig(TGBSConfig& config)
{
  // Set general parameters:
  p_name = config.p_name;
  p_optimize = config.p_optimize;
  p_coordSys = config.p_coordSys;
  p_type = config.p_type;

  // Copy map, dynamically allocating memory for the TGBSGroups
  TGBSGroup* nextGroup;
  TGBSConfig::iterator it;
  for (it = config.begin(); it != config.end(); it++) {
    if ((*it).second != (TGBSGroup*) 0)
      nextGroup = new TGBSGroup(*(*it).second);
    else
      nextGroup = (TGBSGroup*) 0;

    TGBSConfig::value_type v((*it).first, nextGroup);
    insert(v);
  }
}

/*******************************************************************
 Method : Destructor
 Summary:
*******************************************************************/
TGBSConfig::~TGBSConfig()
{
  TGBSConfig::iterator it;
  for (it = begin(); it != end(); it++)
  {
    if ((*it).second != (TGBSGroup*) 0)
      delete (*it).second;
  }
}

// STATIC CONVERION METHODS

/*******************************************************************
 Method : ConfigType
 Summary:
*******************************************************************/
TGBSConfig::ConfigType TGBSConfig::strToConfigType(string ct)
{
  TGBSConfig::ConfigType configType;
  if (ct == "simple")
    configType = TGBSConfig::simple;
  else if (ct == "element")
    configType = TGBSConfig::element;
  else if (ct == "atom")
    configType = TGBSConfig::atom;
  else if (ct == "UnknownConfigType")
    configType = TGBSConfig::UnknownConfigType;
  else {
    string msg = "unrecognized config type: ";
    msg += ct;
    EE_RT_ASSERT(false, EE_WARNING, msg);
  }
  
  return configType;
}

//// MODIFIERS ////

/*******************************************************************
 Method : insertGBSGroup
 Summary:
*******************************************************************/
bool TGBSConfig::insertGBSGroup(const vector<string>& tags, 
                                TGBSGroup* newGroup) 
{
  string atmlist;
  for(size_t i = 0; i < tags.size(); i++) {
    if(i != 0) atmlist += " ";
    atmlist += tags[i];
  }
  return insertGBSGroup(atmlist, newGroup);
}

/*******************************************************************
 Method : insertGBSGroup
 Summary:
*******************************************************************/
bool TGBSConfig::insertGBSGroup(const string& tag, TGBSGroup* newGroup) {

  // delete the existing group for that tag, if necessary
  TGBSConfig::iterator it = find(tag);
  if (it != end() && (*it).second != (TGBSGroup*)0) { // prevent memory leaks
    delete (*it).second;
  }

  (*this)[tag] = newGroup;
  return true;
}



/*******************************************************************
 Method : clearType
 Summary: This method removes all Gaussian basis sets from all
          TGBSGroups that have the indicated gbs_type.
*******************************************************************/
void TGBSConfig::clearType(TGaussianBasisSet::GBSType gbs_type)
{
  TGBSConfig::iterator it;

  for(it = begin(); it != end(); it++)
  {
    if ((*it).second != (TGBSGroup*) 0)
      (*it).second->removeGBS(gbs_type);
  }
}

/*******************************************************************
 Method : removeMultipleOrbitals
 Summary: If any group has more than one orbital basis set, this
          function will remove the extra ones.  Used when switching
          between a code that supports multiple orbital basis sets
          and one that does not.
*******************************************************************/
void TGBSConfig::removeMultipleOrbitals() {
  TGBSConfig::iterator it;
  TGBSGroup* group;
  OrbitalMap::iterator orbIt;

  for(it = begin(); it != end(); it++) {
    group = (*it).second;

    if (group != 0) {
      for (orbIt = group->p_orbitals.begin();
           orbIt != group->p_orbitals.end(); orbIt++) {
        if (orbIt != group->p_orbitals.begin()) {
          group->removeOrbitalGBS((*orbIt).first);
        }
      }
    }
  }
}

/*******************************************************************
 Method : remove
 Summary: This function removes all TBGSGroups which support the
          element represented by elementName.  So, for example, if
          elementName = "H", then remove would erase all groups mapped to
          "C H" or "H1" or "H2", etc.

          Note that a removed group is completely deleted.
*******************************************************************/
void TGBSConfig::remove(const char* elementName) {
  TGBSConfig::iterator it;

  bool eraseFlag;

  for(it = begin(); it != end();) {
    TGBSConfigTags tags((*it).first.c_str()); // tags is a vector of strings

    eraseFlag = false;
    for(size_t i = 0; i < tags.size(); i++) {

      if(tags[i] == elementName) {
        if ((*it).second != (TGBSGroup*) 0) {
          delete (*it).second;
        }

        erase(it);
        eraseFlag = true;
        break;
      }
    }
    // don't advance iterator if we erased the current entry because
    // it has already been advanced to the next valid entry
    if (!eraseFlag)
      it++;
  }
}


/*******************************************************************
 Method : removeGBSGroup
 Summary: This function removes the group that matches the given
          tag.
*******************************************************************/
void TGBSConfig::removeGBSGroup(string tag)
{
  TGBSConfig::iterator it= find(tag);
  if(it != end())
  {
    if ((*it).second != (TGBSGroup*) 0)
      delete(*it).second;
    erase(it);
  }
  else {
    string msg = "Group " + tag + " not found in config!";
    EE_RT_ASSERT(false, EE_FATAL, msg);
  }
}

/*******************************************************************
 Method : removeAllGroups
 Summary: This function removes all groups from the config.
*******************************************************************/
void TGBSConfig::removeAllGroups(void)
{
  TGBSConfig::iterator it;

  for (it = begin(); it != end(); it++) {
    if ((*it).second != (TGBSGroup*) 0) {
      delete(*it).second;
    }
  }
  clear();
}

/*******************************************************************
 Method : resetAllGroups
 Summary: This function changes all the groups to match the
          tag and sets them to null;
*******************************************************************/
void TGBSConfig::resetAllGroups(TGBSConfig::ConfigType type, 
                                string tag,
                                TGBSGroup* group)
{
  removeAllGroups();
  if (type == TGBSConfig::simple) {
    insertGBSGroup(tag, (TGBSGroup*) 0);

  } else if (type == TGBSConfig::element){
    
    TGBSConfigTags tags(tag.c_str());
    for (size_t i = 0; i < tags.size(); i++) {
      insertGBSGroup(tags[i], (TGBSGroup*) 0);
    }
  }
}
/*******************************************************************
 Method : resetAllGroups
 Summary: This function resets the all the groups in the config to
          null;
*******************************************************************/
void TGBSConfig::resetAllGroups()
{
  TGBSConfig::iterator it;
  vector<string> groups;

  for (it = begin(); it != end(); it++) {
    groups.push_back((*it).first);
  }
  removeAllGroups();
  for (size_t i = 0; i < groups.size(); i++) {
    insertGBSGroup(groups[i], (TGBSGroup*) 0);
  }  
}

/*******************************************************************
 Method : resetGroup
 Summary: 
*******************************************************************/
void TGBSConfig::resetGroup(string tag, TGBSGroup* group)    
{
  // insertGBSGroup will automatically delete the previous group for the tag,
  // if one already exists
  if (group == (TGBSGroup*) 0)
    insertGBSGroup(tag, (TGBSGroup*) 0);
  else
    insertGBSGroup(tag, new TGBSGroup(*group));
}


/*******************************************************************
 Method : coordsys
 Summary: Sets p_coordSys value.
*******************************************************************/
void TGBSConfig::coordsys(TGaussianBasisSet::CoordinateSystem coordsys) {
  p_coordSys=coordsys;
}

/*******************************************************************
 Method : type
 Summary: Sets p_type to simple, atom, or element. 

          This function sets the type without error checking or
          config changes.  (This needs to happen externally because
          it requires access to the GaussianBasisSetLibrary.)
*******************************************************************/
void TGBSConfig::type(TGBSConfig::ConfigType type)
{
  p_type = type;
}

/*******************************************************************
 Method : optimize
 Summary: 
*******************************************************************/
void TGBSConfig::optimize(bool opt) {
  p_optimize = opt;
}

/*******************************************************************
 Method : name
 Summary: Probably don't need this function anymore.  I determine
          the name dynamically each time.
*******************************************************************/
void TGBSConfig::name(const string& name) {
  p_name = name;
}

//// ACCESSORS ////
set<string, less<string> > *TGBSConfig::elements() const
{
  set<string, less<string> > *ret = 0;
  if (!empty()) 
  {
    ret = new set<string, less<string> >;
    if (p_type == TGBSConfig::simple) 
    {
      // parse string
      TGBSConfigTags tagList((*begin()).first.c_str()); // strips numerals
      for (size_t i = 0; i < tagList.size(); i++)
        ret->insert(tagList[i]);
    } else {
      TGBSConfig::const_iterator it;
      for (it = begin(); it != end(); it++) 
      {
        TGBSConfigTags tagList((*it).first.c_str()); // strips numerals
        for (size_t i = 0; i < tagList.size(); i++)
          ret->insert(tagList[i]);
      }
    }
  }
  return ret;
}

/*******************************************************************
 Method : uniqueTagStr
 Summary: Returns a string of unique element symbols (in increasing
          alphabetic order) representing the fragment being used
          for this config.
*******************************************************************/
string TGBSConfig::uniqueTagStr(void) const
{
  string ret;

  if (empty())
    ret = "";

  else if (p_type == TGBSConfig::simple)
    ret = (*begin()).first;

  else
  {
    // set class is used because it automatically orders on insert
    // and removes duplicates
    set<string, less<string> > tagSet;
    set<string, less<string> >::iterator setIt;
    TGBSConfig::const_iterator it;
    string tagString;
    size_t i;

    for (it = begin(); it != end(); it++)
    {
      TGBSConfigTags tagList((*it).first.c_str()); // strips numerals
      for (i = 0; i < tagList.size(); i++)
        tagSet.insert(tagList[i]);
    }
   
    for (setIt = tagSet.begin(); setIt != tagSet.end(); setIt++)
    {
      if (setIt != tagSet.begin())
        tagString += " ";
      tagString += (*setIt);
    }
    ret = tagString;
  }
  return ret;
}

/*******************************************************************
 Method : isEqual
 Summary: Indicates if another config is the same as this one.  
          Equality is determined based on:
          1) Both configs have the same number of groups with the
             same tags
          2) Each group has the same number of basis sets with the
             same names
*******************************************************************/
bool TGBSConfig::isEqual(TGBSConfig* otherConfig) const
{
  // first check size (i.e., number of groups)
  if (size() != otherConfig->size()) {
    //cout << "size doesn't match" << endl;
    return false;
  }
  // next check the config meta data:
  if( p_name != otherConfig->p_name ||
      p_optimize != otherConfig->p_optimize ||
      p_coordSys != otherConfig->p_coordSys ||
      p_type != otherConfig->p_type) {
    //cout << "metadata doesn't match" << endl;
    return false;
  }

  // now check that group tags are identical and
  // all the basis sets in each equivalent group have the same name
  // and all the metadata maps for each group are the same:
  TGBSConfig::const_iterator it1, it2;
  const vector<TGaussianBasisSet*> *gbsList1, *gbsList2;
  const TGaussianBasisSet* ecp1, *ecp2;
  size_t i;

  for (it1 = begin(); it1 != end(); it1++)
  {
    it2 = otherConfig->find((*it1).first);

    // Check that each tag in 1 is also in 2
    if (it2 == otherConfig->end()) {
      //cout << "tags dont match" << endl;
      return false;
    }
    // Check for nulls so no seg faults
    else if ( ((*it1).second == 0 && (*it2).second != 0 
               && !(*it2).second->empty() ) ||
              ((*it1).second != 0 && !(*it1).second->empty() 
               && (*it2).second == 0) ) {
      //cout << "a group is now zero" << endl;
      return false;
    }
    else if ((*it1).second != 0 && (*it2).second != 0)
    {
      gbsList1 = (*it1).second->getOrderedList();
      gbsList2 = (*it2).second->getOrderedList();

      // Check that the same number of basis sets are in each group
      if (gbsList1->size() != gbsList2->size()) {
        //cout << "group has different number of basis sets" << endl;
        return false;
      }
      // Check that the basis set names match
      for (i = 0; i < gbsList1->size(); i++)
        if((*gbsList1)[i]->p_name != (*gbsList2)[i]->p_name) {
          //cout << "group has a different basis set" << endl;
          return false;
        }
      // check ecps too:
      ecp1 = (*it1).second->ecp();
      ecp2 = (*it2).second->ecp();

      if ( (ecp1 == 0 && ecp2 != 0) ||
           (ecp1 != 0 && ecp2 == 0) )
        return false;
 
      if (ecp1 != 0 && ecp1->p_name != ecp2->p_name)
        return false;
      
      // check the maps of metadata
      if ((*it1).second->p_gbsCount != (*it2).second->p_gbsCount) 
        return false;

      if ((*it1).second->p_orbitals != (*it2).second->p_orbitals)
        return false;

      if ((*it1).second->p_tightFunctions !=
          (*it2).second->p_tightFunctions)
        return false;

      if ((*it1).second->p_diffuseFunctions !=
          (*it2).second->p_diffuseFunctions)
        return false;
      
      if ((*it1).second->p_contractionInfoMap !=
          (*it2).second->p_contractionInfoMap)
        return false;
      
    }

  }
  return true;
}
/*******************************************************************
 Method : getGBSList()
 Summary: Returns a vector of all the GBSs (base and augmenting GBSs)
          found in the TGBSGroup that is mapped to the given tag
          (IN THE ORDER IN WHICH THEY WERE INSERTED). 
 
          NOTE: ecps and DFT fittings are NOT included in the list.

          The tag name is used as the key to return the
          appropriate GBS from the map.

          Note that this means if "H" and "H1" are separate keys,
          then the user must specify "H" or "H1" in the call to
          get the appropriate GBSGroup.

          Returns empty vector on error.

          The vector contents point to the basis sets contained within
          the class...do not delete.
*******************************************************************/
vector<const TGaussianBasisSet*> TGBSConfig::getGBSList(string element_) 
{
  TGBSGroup* group = findGroup(element_);
  const TGaussianBasisSet* gbs;
  vector<const TGaussianBasisSet*> result;

  if (group != 0) {
    for (size_t i = 0; i < group->p_gbsOrderedVector.size(); i++) {
      gbs = group->p_gbsOrderedVector[i];

      // Only include if not a DFT fitting:
      if (gbs->p_type != TGaussianBasisSet::charge && 
          gbs->p_type != TGaussianBasisSet::exchange)
        result.push_back(gbs);
    }
  }
  return result;
}

/**************************************************************************
 Method:  findGroup
 Summary: This function finds the group that contains the given element/atom
          (i.e., H or H1 are possible values)

          Returns 0 if element not found in config.
          DO NOT DELETE RETURN POINTER
***************************************************************************/
TGBSGroup* TGBSConfig::findGroup(string element_)
{
  TGBSConfig::iterator it;  // index of the group where the
                                  // element resides
  TGBSGroup* group = 0;

  if( !empty() ) {

    // Next find the group where the element resides:
    if (p_type == TGBSConfig::simple)  {
      TGBSConfigTags tagList((*begin()).first.c_str(), false,false);
      if (tagList.findElement(element_))
        it = begin();
      else
        it = end();
      
    } else {
      it = find(element_);  // can look for element or atom tag 
                            // (e.g., H or H1)  
    }
    if (it != end()) {
      group = (*it).second;
    }
  }

  if (!group) {
    // This isn't really an error
    /*
      EE_ASSERT(0, EE_WARNING, 
                element_ + " was not found in a group from this config.");
    */
  } 

  return group;
}

void TGBSConfig::contractionInfo(vector<int>& totalFrom, vector<int>& totalTo,
                                 const string& tag) {

  vector<const TGaussianBasisSet*> basis = getGBSList(tag);
  GBSToContInfoMap* infoMap = getContractionInfoMap(tag); 
  GBSToContInfoMap::iterator it;

  string uniqueKey;
  vector<int> from;
  vector<int> to;
  bool uncontractFlag;
  bool deleteFlag;

  // clear out old values
  totalFrom.clear();
  totalTo.clear();
  totalFrom.resize(TGaussianBasisSet::maxShell+1, 0);
  totalTo.resize(TGaussianBasisSet::maxShell+1, 0);

  // loop thru each basis set in the list
  for (size_t i=0; i < basis.size(); i++) {

    basis[i]->contractionInfo(tag,from,to);
    uniqueKey = basis[i]->getUniqueKey();

    if (infoMap != NULL) {
      it = infoMap->find(uniqueKey);
    } else {
      //cout << "infoMap is null for " << tag << endl;
    }

    /// loop thru each shell
    for (size_t j=0; j < from.size(); j++) {
      if (from[j] > 0) {
        if (infoMap != NULL && it != infoMap->end()) {

          uncontractFlag = (*it).second.uncontractFlags[j];
          deleteFlag = (*it).second.deleteFlags[j];

          if (deleteFlag == false) {   // then increment the total cont count
            totalFrom[j] += from[j];
            
            if (uncontractFlag == true) {
              totalTo[j] += from[j];

            } else {
              totalTo[j] += to[j];
            }
          }   
        }
      }
    }
  }

}

/**
 * Provides information about which shells have data for the tag.
 *
 * This is independent of the delete and uncontract flag and is useful
 * for GUIs that need to decide what to display.
 */
void 
TGBSConfig::hasContractionInfo(vector<bool>& perShell, const string& tag) 
{

  vector<const TGaussianBasisSet*> basis = getGBSList(tag);
  GBSToContInfoMap* infoMap = getContractionInfoMap(tag); 
  GBSToContInfoMap::iterator it;

  string uniqueKey;
  vector<int> from;
  vector<int> to;

  // clear out old values
  perShell.resize(TGaussianBasisSet::maxShell+1, 0);

  // loop thru each basis set in the list
  for (size_t i=0; i < basis.size(); i++) {

    basis[i]->contractionInfo(tag,from,to);
    uniqueKey = basis[i]->getUniqueKey();

    if (infoMap != NULL) {
      it = infoMap->find(uniqueKey);
    } else {
      //cout << "infoMap is null for " << tag << endl;
    }

    /// loop thru each shell
    for (size_t j=0; j < from.size(); j++) {
      if (from[j] > 0) {
        if (infoMap != NULL && it != infoMap->end()) {

          perShell[j] = perShell[j] | (from[j] > 0);
          perShell[j] = perShell[j] | (to[j] > 0);
        }
      }
    }
  }
}

// DO NOT DELETE RETURN POINTER
vector<int>* TGBSConfig::getTightFunctionCount(const string& tag) {
  // tag is either an element or an atom name
  
  // first find what group the tag is in
  TGBSGroup* group = findGroup(tag);
  vector<int>* ret = 0;

  if (group != 0) {  
    // Now strip off the numerals (if any) because the group will
    // use the element name to map the config info, not the atom name
    TGBSConfigTags tags(tag.c_str());

    TightMap::iterator tIt = 
      group->p_tightFunctions.find(tags[0]); 

    if (tIt != group->p_tightFunctions.end()) {
      ret = &((*tIt).second);
    }
  }
    
  return ret;
}

// DO NOT DELETE RETURN POINTER
vector<int>* TGBSConfig::getDiffuseFunctionCount(const string& tag) {
  // tag is either an element or an atom name
  
  // first find what group the tag is in
  TGBSGroup* group = findGroup(tag);
  vector<int>* ret = 0;

  if (group != 0) {  
    // Now strip off the numerals (if any) because the group will
    // use the element name to map the config info, not the atom name
    TGBSConfigTags tags(tag.c_str());
    
    DiffuseMap::iterator dIt = 
      group->p_diffuseFunctions.find(tags[0]); 

    if (dIt != group->p_diffuseFunctions.end()) {
      ret = &((*dIt).second);
    }
  }
    
  return ret;
}

// DO NOT DELETE RETURN POINTER
GBSToContInfoMap* TGBSConfig::getContractionInfoMap(const string& tag)
{
  // tag is either an element or an atom name
  
  // first find what group the tag is in
  TGBSGroup* group = findGroup(tag);
  GBSToContInfoMap* ret = 0;

  if (group != 0) {  
    // Now strip off the numerals (if any) because the group will
    // use the element name to map the config info, not the atom name
    TGBSConfigTags tags(tag.c_str());

    ElementToGBSInfoMap::iterator elIt = 
      group->p_contractionInfoMap.find(tags[0]); 

    if (elIt != group->p_contractionInfoMap.end()) {
      ret = &((*elIt).second);
    }
  }
    
  return ret;
}

/*******************************************************************
 Method : hasMultipleOrbitals
 Summary: If any of the TGBSGroups have more than one orbital basis
          set, returns true.

*******************************************************************/
bool TGBSConfig::hasMultipleOrbitals(void) const
{
  bool ret = false;
  TGBSConfig::const_iterator it;
  TGBSGroup* group;

  for(it = begin(); it != end() && !ret; it++)
  {
    group = (*it).second;

    if(group == (TGBSGroup*) 0 || group->empty())
      continue;

    if (group->p_orbitals.size() > 1) {
        ret = true;
    }
  }
  return ret;
}

/*******************************************************************
 Method : getOrbitalGBSIdentifiers
 Summary: Return pointer guaranteed to not be null.  But caller
          must delete it.
*******************************************************************/
vector<string>* TGBSConfig::getOrbitalGBSIdentifiers(const string& groupTag) 
{
  vector<string>* ret = 0;

  TGBSConfig::iterator it = find (groupTag);
  if (it != end()) {
    if ((*it).second != 0) {
      ret = (*it).second->getOrbitalGBSIdentifiers();
    } else {
      ret = new vector<string>;
    }
  } else {
    ret = new vector<string>;
  }

  return ret;
}

/*******************************************************************
 Method : getAuxGBSIdentifiers
 Summary: Returns a list of all augmenting basis sets (this means
          polarization, diffuse, rydberg, exchange, charge, AND ecp) 
          that are not in an
          orbital aggregate group.  Each returned basis set is
          represented by a key, identifying the name and type.

          Return pointer guaranteed to not be null.  But caller
          must delete it.

          This function is used when switching between simple and
          by-element configurations.
*******************************************************************/
vector<GBSKey>* TGBSConfig::getAuxGBSIdentifiers(const string& groupTag) 
{
  vector<GBSKey>* ret = 0;

  TGBSConfig::iterator it = find (groupTag);
  if (it != end()) {
    if ((*it).second != 0) {
      ret = (*it).second->getAuxGBSIdentifiers();
    } else {
      ret = new vector<GBSKey>;
    }
  } else {
    ret = new vector<GBSKey>;
  }
  return ret;
}
  
/*******************************************************************
 Method : hasEmptyGroups
 Summary: If any of the TGBSGroups are null or empty, then returns true.

          NOTE: All of the data accessors in this class skip over
          empty groups, so you must check emptiness separately first
          using this function.
*******************************************************************/
bool TGBSConfig::hasEmptyGroups(void) const
{
  bool ret = false;
  TGBSConfig::const_iterator it;
  for(it = begin(); it != end(); it++)
  {
    if((*it).second == (TGBSGroup*) 0 || (*it).second->empty() ||
       !(*it).second->hasContractions((*it).first) )
      ret = true;
  }
  return ret;
}

/*******************************************************************
 Method : isEmpty
 Summary: Returns true if all the groups are null or empty..
*******************************************************************/
bool TGBSConfig::isEmpty(void) const
{
  TGBSConfig::const_iterator it;
  for(it = begin(); it != end(); it++) {
 
    if((*it).second != (TGBSGroup*) 0 && !(*it).second->empty()) {
      return false;
    }
  }
  return true;
}

/*******************************************************************
 Method : numUniqueGroups
 Summary: Returns the number of unique groups in the config.  Used
          when collapsing from by-element to simple configuration.
*******************************************************************/
int TGBSConfig::numUniqueGroups(void) const
{
  TGBSConfig::const_iterator groupIt;
  TGBSGroup::const_iterator bsIt;

  set<string, less<string> > groupDescriptors;
  TGBSGroup* group;
  TGaussianBasisSet* bs;
  const TGaussianBasisSet* ecp;

  for(groupIt = begin(); groupIt != end(); groupIt++) {

    group = (*groupIt).second; 
    vector<string> basisSetNames;

    if (group) {
      
      for (bsIt = group->begin(); bsIt != group->end(); bsIt++) {
        bs = (*bsIt).second;
        EE_RT_ASSERT(bs, EE_FATAL, "Null basis set in TGBSGroup!");

        basisSetNames.push_back(bs->p_name);
      }
      ecp = group->ecp();
      if(ecp) {
        basisSetNames.push_back(ecp->p_name);
      }
    
    } else {
      basisSetNames.push_back("");
    }

    // now sort the list using STL algorithm "sort"
    sort(basisSetNames.begin(), basisSetNames.end());

    // now concatenate the list into one long string
    string tmpString;
    for (size_t i = 0; i < basisSetNames.size(); i++) {
      tmpString += basisSetNames[i];
    }
    groupDescriptors.insert(groupDescriptors.end(),tmpString);
  }
  
  return groupDescriptors.size();
}


         
/*******************************************************************
 Method : name
 Summary: Always dynamically calculated.  Gives special name
          "Custom" to configurations using a Multiple Orbital Basis Set
          scenario (as would be used for Amica code in particular).  If not
          "Custom", then picks the heaviest atom and uses the basis set
          for that atom.  If the basis set is by-element, and different
          basis sets have been assigned for each group, then follow the
          name with "...".
*******************************************************************/
string TGBSConfig::name(void) const {
  string result;
  bool hasdata = false;

  if (!empty()) {
    
    // If simple, use first group to get name  
    if (p_type == simple) {
      if ((*begin()).second != 0) {
        result = (*begin()).second->name();
        hasdata = true;
      }

    } else { // atom or element
  
      // Find the heaviest atom by looking up atomic numbers
      // in the periodic table; use the group name for that atom
      // to get the config name.
      TGBSConfig::const_iterator it;
      TPerTab perTab(true);
      int maxWeight = 0;
      int curWeight = 0;
      size_t i;
      
      TGBSGroup* heaviestGroup = 0;
      
      
      for (it = begin(); it != end(); it++) {
        
        // skip null groups
        if ((*it).second == (TGBSGroup*) 0)
          continue;

        hasdata = true;
        if ((*it).second->name() == "Custom") {
          result = "Custom";
          break;
          
        } else {
          
          TGBSConfigTags tags((*it).first.c_str());
          for (i = 0; i < tags.size(); i++) {
            curWeight = perTab.atomicNumber(tags[i].c_str());
            if(curWeight > maxWeight){
              maxWeight = curWeight;
              heaviestGroup = (*it).second;
            }
          }
        }
      }
      if (result == "" && heaviestGroup != 0) {
        result = heaviestGroup->name();
        if (numUniqueGroups() > 1) {
          result += "...";
        }
      }
    }
  }
  // backward compatibility check for imported calculations
  if (result == "" && hasdata == true && p_name != "") {
    result = p_name;
  }

  return result;
} 

/*******************************************************************
 Method : groupTagOfHeaviestElement
 Summary: Returns the tag for the group that contains the heaviest
          element in the config.
*******************************************************************/
string TGBSConfig::groupTagOfHeaviestElement() const {

  string heaviestGroup;

  if (!empty()) {
    
    // If simple, use first group to get name  
    if (p_type == simple) {
      heaviestGroup = (*begin()).first;

    } else { // atom or element
  
      // Find the heaviest atom by looking up atomic numbers
      // in the periodic table;
      TGBSConfig::const_iterator it;
      TPerTab perTab(true);
      int maxWeight = 0;
      int curWeight = 0;
      size_t i;

      for (it = begin(); it != end(); it++) {

        TGBSConfigTags tags((*it).first.c_str());
        for (i = 0; i < tags.size(); i++) {
          curWeight = perTab.atomicNumber(tags[i].c_str());
          if(curWeight > maxWeight){
            maxWeight = curWeight;
            heaviestGroup = (*it).first;
          }
        }      
      }
    }
  }
  return heaviestGroup;
} 

/*******************************************************************
 Method : type
 Summary: 
*******************************************************************/
TGBSConfig::ConfigType TGBSConfig::type(void) const {
  return p_type;
}

/*******************************************************************
 Method : coordsys
 Summary: 
*******************************************************************/
TGaussianBasisSet::CoordinateSystem TGBSConfig::coordsys(void) const {
  return p_coordSys;
}

/*******************************************************************
 Method : optimize
 Summary: 
*******************************************************************/
bool TGBSConfig::optimize(void) const {
  return p_optimize;
}

/*******************************************************************
 Method : max_l_value
 Summary: Finds the largest shell value (i_shell is the max possible) 
          out of all of the contractions for all of the GBSs for all of
          the TGBSGroups in the config.

          Returns -1 if no contractions are present
*******************************************************************/
int TGBSConfig::max_l_value(void) const 
{
  int result = -1, curVal;
  size_t i;
  TGBSConfig::const_iterator configIt;

  // for each group in the map
  for(configIt = begin(); configIt != end(); configIt++) 
  { 
    TGBSConfigTags tags((*configIt).first.c_str());
    // tags separates the elements into a vector of single elements
    // removing any numbers added for by-atom elements (i.e., "H1")

    for(i = 0; i < tags.size(); i++) // for each element in the group's tag
    {
      // find that maxlvalue for that element
      curVal = max_l_value(tags[i]);
      if (curVal > result)
        result = curVal;
    }   
  }
  return result;
}

/*******************************************************************
 Method : max_l_value
 Summary: Finds the largest shell value (i_shell is the max possible) 
          out of all of the contractions for all of the GBSs for all of
          the TGBSGroups in the config.

          Returns -1 if no contractions are present
*******************************************************************/
int TGBSConfig::max_l_value(const string& element_) const 
{
  int result = -1, curVal;
  const TGaussianBasisSet* gbs;
  const TGBSGroup* group;
  TGBSConfig::const_iterator configIt;
  TGBSGroup::const_iterator groupIt;

  // for each group in the map
  for(configIt = begin(); configIt != end(); configIt++) 
  { 
    group = (*configIt).second;
    if (group == (TGBSGroup*) 0)
      continue;

    // for each basis set in the group except dft fittings
    for (groupIt = group->begin(); groupIt != group->end(); groupIt++)
    {
      gbs = (*groupIt).second;
      if (gbs->p_type == TGaussianBasisSet::charge ||
          gbs->p_type == TGaussianBasisSet::exchange)
        continue;
   
      // find that maxlvalue for that element
      curVal = (int) gbs->maxlvalue(element_);
      if (curVal > result) {
        result = curVal;
      }
    }
  }
  return result;
}

/*******************************************************************
 Method : max_primitives
 Summary: User is responsible for deleting the returned pointer.
          Returns a vector with the max number of exponents for
          each shell (s_shell to i_shell), out of all the contractions
          for all the base (i.e., orbital) GBSs of all the TGBSGroups
          in the map.

          Note that ecps are not included in this value, as expected.
*******************************************************************/
vector<int>* TGBSConfig::max_primitives(void) const {

  vector<int> *maxprim = 0;
  size_t i;

  TGBSConfig::const_iterator it;
  vector <const TGaussianBasisSet*>* orbitalGBSList;
  const TGaussianBasisSet* orbitalGBS;

  for(it = begin();it != end();it++) // loop thru all groups
  {
    if ((*it).second == (TGBSGroup*) 0)
      continue; // skip over null groups

    TGBSConfigTags tags((*it).first.c_str());
    TGBSConfigTags::iterator tag_it;
    orbitalGBSList = (*it).second->getOrbitalGBSList();

    if (orbitalGBSList != 0)
    {
      for (i = 0; i < orbitalGBSList->size(); i++) {
        orbitalGBS = (*orbitalGBSList)[i];

        for(tag_it = tags.begin(); tag_it != tags.end(); tag_it++) {
          ContractionMap::const_iterator cont_list = 
            orbitalGBS->p_contractions.find(*tag_it);

          if(cont_list != orbitalGBS->p_contractions.end()) {
            if (maxprim == 0)
              // only create the vector one time
              maxprim = new vector<int>(TGaussianBasisSet::i_shell+1, 0);
            
            ContractionSet::iterator cont_it;
            for(cont_it=(*cont_list).second->begin();
                cont_it!=(*cont_list).second->end();cont_it++) {
              vector<TGaussianBasisSet::AngularMomentum> funcs 
                = (*cont_it)->shells;
              int rows = (*cont_it)->num_exponents;
              int cols = (*cont_it)->num_coefficients;
              for(int f=0;f<cols;f++) {
                if((*maxprim)[(int)funcs[f]] <= rows) 
                  (*maxprim)[(int)funcs[f]] = rows; 
              }
            }
          }
        }
      }
      delete orbitalGBSList;
    }
  }
  return maxprim;
}

/*******************************************************************
 Method : dftChargeFittingName
 Summary: (See dftFitting Name below)
*******************************************************************/
string TGBSConfig::dftChargeFittingName(void) const { 
  string ret;

  TGBSConfig::const_iterator it;
  TGBSGroup::iterator match;
  for(it = begin(); it != end(); it++) {
    if((*it).second) {
      match = (*it).second->find(TGaussianBasisSet::charge);
      if (match != (*it).second->end()) {
        if (ret.empty()) {
          ret = (*match).second->p_name;
        } else {
          ret += "...";
          break;
        }
      }
    }
  }
  return ret;
}

/*******************************************************************
 Method : hasChargeFitting
 Summary: "some" returns whether a charge GBS exists for any element
          in the config
          "all" returns whether a charge GBS exists for all the
          elements in the config

          Note that it only checks non-null groups, so null groups
          need to be identified before-hand using hasEmptyGroups().
*******************************************************************/
void TGBSConfig::hasChargeFitting(bool& some, bool& all) const
{
  some = false;
  all = true;
  TGBSConfig::const_iterator it;

  for(it = begin(); it != end(); it++) {
    if ((*it).second == (TGBSGroup*) 0)
      continue;

    TGBSConfigTags tags((*it).first.c_str());
    TGBSGroup::iterator gbs_it =
      (*it).second->find(TGaussianBasisSet::charge);

    if (gbs_it == (*it).second->end()) // this group doesnt have charge fitting
      all = false;

    else // check every element to make sure it is supported by the 
         // charge fitting
    {
      for (size_t i = 0; i < tags.size(); i++)
      {
        if((*gbs_it).second->p_contractions.find(tags[i]) !=
           (*gbs_it).second->p_contractions.end())
          some = true;
        else
          all = false;
      }
    }
  }
}

/*******************************************************************
 Method : hasExchangeFitting
 Summary: "some" returns whether an exchange GBS exists for any element
          in the config
          "all" returns whether an exchange GBS exists for all the
          elements in the config

          Note that it only checks non-null groups, so null groups
          need to be identified before-hand using hasEmptyGroups().
*******************************************************************/
void TGBSConfig::hasExchangeFitting(bool& some, bool& all) const
{
  some = false;
  all = true;
  TGBSConfig::const_iterator it;

  for(it = begin(); it != end(); it++) {
    if ((*it).second == (TGBSGroup*) 0)
      continue;

    TGBSConfigTags tags((*it).first.c_str());
    TGBSGroup::iterator gbs_it =
      (*it).second->find(TGaussianBasisSet::exchange);

    if (gbs_it == (*it).second->end()) // this group doesnt have charge fitting
      all = false;

    else // check every element to make sure it is supported by the 
         // charge fitting
    {
      for (size_t i = 0; i < tags.size(); i++)
      {
        if((*gbs_it).second->p_contractions.find(tags[i]) !=
           (*gbs_it).second->p_contractions.end())
          some = true;
        else
          all = false;
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
//  method: dftFittingName
//
//    Returns the name of the dft fitting used in the configuration.  Since
//    different elements/atoms can have different fitting sets or none at
//    all, this method currently returns the first one it finds
//    followed by ... if more may be present.  In addition, it only 
//    returns the name of the exchange fitting set.  Usually both are the
//    same but...
//
///////////////////////////////////////////////////////////////////////////////
string TGBSConfig::dftFittingName(void) const { 
  string ret;

  TGBSConfig::const_iterator configIt;
  TGBSGroup::iterator match;
  for(configIt = begin(); configIt != end(); configIt++) {
    if((*configIt).second) {
      match = (*configIt).second->find(TGaussianBasisSet::exchange);
      if (match != (*configIt).second->end()) {
        if (ret.empty()) {
          ret = (*match).second->p_name;
        } else {
          ret += "...";
          break;
        }
      }
    }
  }
  return ret;
}

/*******************************************************************
 Method : hasDFTs
 Summary: Returns true if either an exchange or a charge GBS are
          present in any TGBSGroup.
*******************************************************************/
bool TGBSConfig::hasDFTs(void) const {
  TGBSConfig::const_iterator configIt;
  for(configIt = begin(); configIt != end(); configIt++) {
    if ((*configIt).second == (TGBSGroup*) 0)
      continue;
    if((*configIt).second->find(TGaussianBasisSet::exchange) != 
       (*configIt).second->end() ||
       (*configIt).second->find(TGaussianBasisSet::charge) != 
       (*configIt).second->end())
      return true;
  }
  return false;
}

///////////////////////////////////////////////////////////////////////////////
//  method: ecpName
//    Returns the name of the ecp used in the configuration.  Since
//    different elements/atoms can have different ecps or no ecps at
//    all, this method currently returns the first one it finds
//    followed by ... if more may be present.
///////////////////////////////////////////////////////////////////////////////
string TGBSConfig::ecpName(void) const { 
  string ret;
  TGBSConfig::const_iterator configIt;
  for(configIt = begin(); configIt != end(); configIt++) {
    if((*configIt).second && (*configIt).second->ecp()) {
      if (!ret.empty()) {
        ret += "...";
        break;
      } else {
        ret = (*configIt).second->ecp()->p_name;
      }
    }
  }
  return ret;
}

/*******************************************************************
 Method : hasECPs
 Summary: Returns true if any TGBSGroup contains an ecp
*******************************************************************/
bool TGBSConfig::hasECPs(void) const {
  TGBSConfig::const_iterator configIt;
  for(configIt = begin(); configIt != end(); configIt++) {
    if((*configIt).second && (*configIt).second->ecp())
      return true;
  }
  return false;
}

/*******************************************************************
 Method : hasSpherical
 Summary: Returns true if a GBS from any group should use
          spherical components.
         
          If considerLValues is true, hasSpherical checks if there are
          any basis sets that were designed for spherical components
          that also have  L values of d or greater.

          If considerLValues is false, hasSpherical only checks if
          there are any basis sets that were designed for spherical
          components.

          Note that ecps and dft fittings are not evaluated as part
          of the group.
*******************************************************************/
bool TGBSConfig::hasSpherical(bool considerLValues) const {

  bool result = false;
  TGBSConfig::const_iterator configIt;
  TGBSGroup::iterator groupIt;
  TGBSGroup* group;
  TGaussianBasisSet* gbs;
  size_t i;

  // for each group in the config:
  for(configIt = begin(); configIt != end(); configIt++) 
  {
    if (result == true)
      break;

    group = (*configIt).second;
    if (group == (TGBSGroup*) 0)
      continue;
    TGBSConfigTags tags((*configIt).first.c_str());

    for (groupIt = group->begin(); groupIt != group->end(); groupIt++)
    {
      if (result == true)
        break;
      gbs = (*groupIt).second;   
      if (gbs->p_type == TGaussianBasisSet::exchange ||
          gbs->p_type == TGaussianBasisSet::charge)
        continue;

      if (considerLValues)
      {
        for(i = 0; i < tags.size(); i++) // for each element in the group's tag
          result = result || gbs->useSpherical(tags[i]); // checks l value
      }
      else
        result = result || (gbs->p_coordSys == TGaussianBasisSet::Spherical);
    }
  }
  return result;
}

/*******************************************************************
 Method : hasGeneralContractions
 Summary: Returns true if an orbital GBS from any group has
          a contraction type of GC.
*******************************************************************/
bool TGBSConfig::hasGeneralContractions(void) const {
  TGBSConfig::const_iterator configIt;
  vector<const TGaussianBasisSet*>* orbitalGBSList;
  size_t i;

  for(configIt = begin(); configIt != end(); configIt++) {
    if((*configIt).second) {
      orbitalGBSList = (*configIt).second->getOrbitalGBSList();
      if (orbitalGBSList != 0) {
        for (i = 0; i < orbitalGBSList->size(); i++) {
          if ((*orbitalGBSList)[i]->p_contType == TGaussianBasisSet::GC) {
            delete orbitalGBSList;
            return true;
          }
        }
        delete orbitalGBSList;
      }
    }
  }
  return false;
}

/* ================================================================
   COUNTING FUNCTIONS

       * num_functions
       * num_primitives
       * eff_num_primitives
   ================================================================ */

/*******************************************************************
 Method : num_functions
 Summary: Counts the total number of functions for all elements in
          all contractions in all TGaussianBasisSets in all TGBSGroups
          using coordinate system multipliers (which increase the
          function count according to the shell type);

          Note that the functions in any auxiliary ecp basis sets
          or dft fittings (if present) are not counted.
*******************************************************************/
int TGBSConfig::num_functions
(
 TagCountMap tagCounts           // tells how many atoms of each element
                                 // are in the molecule
) 

{
  return count(TGBSConfig::NumFunctions, tagCounts);
}

int TGBSConfig::num_ecpCoreElectrons
(
 TagCountMap tagCounts
) 
{
  return count(TGBSConfig::NumEcpCoreElectrons, tagCounts);
}

/*******************************************************************
 Method : num_primitives
 Summary: Counts the total number of coefficients for all contractions
          in all TGaussianBasisSets in all TGBSGroups in the config.
          If the gbs's contraction_type is Segmented, then the coefficients
          for that gbs are multplied by a conversion factor determined
          by the particular shell (s_shell, p_shell, etc.).

*******************************************************************/
int TGBSConfig::num_primitives
(
  TagCountMap tagCounts          // tells how many of
                                  // each atom in the molecule
) 
{
  return count(TGBSConfig::NumPrimitives, tagCounts);
}

/*******************************************************************
 Method : eff_num_primitives
 Summary: 
*******************************************************************/
int TGBSConfig::eff_num_primitives
(
  TagCountMap tagCounts,
  bool gc_code
) 
{
  return count(TGBSConfig::EffNumPrimitives, tagCounts, gc_code);
}

/*******************************************************************
 Method : countEcpCoreElectrons
 Summary: 
*******************************************************************/
int TGBSConfig::countEcpCoreElectrons
(
  const TGaussianBasisSet* ecp,
  int elementCount,
  string element_
)
{
  int numCoreElectrons = 0;
  ContractionMap::const_iterator map_it =
    ecp->p_contractions.find(element_);

  if (map_it != ecp->p_contractions.end())
  { 
    // debug condition
    if ((*map_it).second == (ContractionSet*) 0)
    {
      string msg = "No contraction set defined for ";
      msg += element_;
      msg += ", ";
      msg += ecp->p_name;
      msg += "!!!";
      EE_RT_ASSERT(false, EE_WARNING, msg);
      return 0;
    }
    numCoreElectrons += (*map_it).second->ncore * elementCount;     
  }
  else {
    string msg = "Element " + element_ + " not found in " +
                 ecp->p_name + "!";
    EE_RT_ASSERT(false, EE_WARNING, msg);
  }

  return numCoreElectrons;

}
/*******************************************************************
 Method : countFunctions
 Summary: 
*******************************************************************/
int TGBSConfig::countFunctions
(
  const TGaussianBasisSet* gbs,
  int elementCount,
  string element_
)
{  
  // Determine the multiplier based on spherical or cartesian:
  // (cart_mult and sph_mult are arrays defined in the header)
  short *mult;
  if (p_coordSys == TGaussianBasisSet::Spherical)
    mult = sph_mult;
  else if (p_coordSys == TGaussianBasisSet::Cartesian)
    mult = cart_mult;
  else  {// coord sys is unknown
  
    EE_RT_ASSERT(false, EE_WARNING,
                 "Unknown coordinate system: function count indeterminable.");
    return 0;
  }
     
  int numFunctions = 0; 
  GBSToContInfoMap* infoMap = getContractionInfoMap(element_);
  string uniqueKey = gbs->getUniqueKey();
  GBSToContInfoMap::iterator it;
  vector<int> from;
  vector<int> to;

  if (infoMap != 0) {
    it = infoMap->find(uniqueKey);   
    
    if (it != infoMap->end()){ 
      gbs->contractionInfo(element_,from,to);
      
      for (size_t i = 0; i < from.size(); i++) {

        if ((*it).second.deleteFlags[i] == false) {
          if ((*it).second.uncontractFlags[i] == true) {
            numFunctions += from[i] * mult[i] * elementCount;
          } else {
            numFunctions += to[i] * mult[i] * elementCount;
          }
        }
      }
    } else {
      string msg = "GBS " + gbs->p_name + " not found in " +
        "its group's contraction info map for element " + element_ ;
      EE_RT_ASSERT(false, EE_WARNING, msg);
    }
  } else {
    string msg = "Element " + element_ + " not found in contraction info map!";
    EE_RT_ASSERT(false, EE_WARNING, msg);
  }

  return numFunctions;
}

/*******************************************************************
 Method : countPrimitives
 Summary: 
*******************************************************************/
int TGBSConfig::countPrimitives
(
 const TGaussianBasisSet* gbs,
 int elementCount,
 string element_)
{
  // Determine the multiplier based on spherical or cartesian:
  // (cart_mult and sph_mult are arrays defined in the header)
  short *mult;
  if (p_coordSys == TGaussianBasisSet::Spherical)
    mult = sph_mult;
  else if (p_coordSys == TGaussianBasisSet::Cartesian)
    mult = cart_mult;
  else  {// coord sys is unknown
  
    EE_RT_ASSERT(false, EE_WARNING,
                 "Unknown coordinate system: primitive count indeterminable.");
    return 0;
  }
  
  int numPrimitives = 0; 
  GBSToContInfoMap* infoMap = getContractionInfoMap(element_);
  string uniqueKey = gbs->getUniqueKey();
  GBSToContInfoMap::iterator it;
  vector<int> from;
  vector<int> to;

  if (infoMap != 0) {
    it = infoMap->find(uniqueKey);   
    
    if (it != infoMap->end()){ 
      gbs->contractionInfo(element_,from,to);
      
      for (size_t i = 0; i < from.size(); i++) {

        if ((*it).second.deleteFlags[i] == false) {
            numPrimitives += from[i] * mult[i] * elementCount;
        }
      }
    } else {
      string msg = "GBS " + gbs->p_name + " not found in " +
        "its group's contraction info map for element " + element_ ;
      EE_RT_ASSERT(false, EE_WARNING, msg);
    }
  } else {
    string msg = "Element " + element_ + " not found in contraction info map!";
    EE_RT_ASSERT(false, EE_WARNING, msg);
  }

  return numPrimitives;
}


/*******************************************************************
 Method : countEffPrimitives
 Summary: 
*******************************************************************/
int TGBSConfig::countEffPrimitives
(
 const TGaussianBasisSet* gbs,
 int elementCount,
 string element_,
 bool gc_code) 
{
  int result = 0;

  // ##### General Contraction Basis Set ####
  // (these are the only ones that are special for effPrimitives,
  // and they only differ if the code doesnt support GC)
  
  // ##### Code doesn't sup0port GC ####
  if (gbs->p_contType == TGaussianBasisSet::GC && !gc_code) {
  
    // Determine the multiplier based on spherical or cartesian:
    // (cart_mult and sph_mult are arrays defined in the header)
    short *mult; 
    if (p_coordSys == TGaussianBasisSet::Spherical)
      mult = sph_mult;
    else if (p_coordSys == TGaussianBasisSet::Cartesian)
      mult = cart_mult;
    else { // coord sys is unknown    
      EE_RT_ASSERT(false, EE_WARNING,
           "Unknown coordinate system: function count indeterminable.");
      return 0;
    }

    // Get all the metadata we need:
    GBSToContInfoMap* infoMap = getContractionInfoMap(element_);
    string uniqueKey = gbs->getUniqueKey();
    GBSToContInfoMap::iterator infoIt;
    
    ContractionMap::const_iterator contractions = 
      gbs->p_contractions.find(element_);
    ContractionSet::const_iterator it;
    
    int maxShell = (int) TGaussianBasisSet::i_shell;
    
    if (infoMap != 0) {
      infoIt = infoMap->find(uniqueKey);  
      if (infoIt != infoMap->end() &&
          contractions != gbs->p_contractions.end()) {
        
        for (it=(*contractions).second->begin();
             it != (*contractions).second->end(); it++) {
        
          vector<TGaussianBasisSet::AngularMomentum> funcs = (*it)->shells;
        
          double* coeffs = (*it)->coefficients;
          
          int cols = (*it)->num_coefficients;
          int rows = (*it)->num_exponents;
          
          
          // ##### Code doesn't support GC, and optimize is false ####
          if (!p_optimize) {
            
            for (int r=0; r<rows; r++) {
              for (int c=0; c<cols; c++) {
                if ((int)funcs[c] > maxShell)
                  continue;
                if (coeffs[(cols*r)+c] != 0.0 &&
                    (*infoIt).second.deleteFlags[(int)funcs[c]] == false)
                  result += (mult[(int)funcs[c]] * elementCount);
              }
            }
          } else  {
            // ##### Code doesn't support GC, and optimize is true #### 
            
            int optCol = -1;

            vector<bool> printableCols = (*it)->getPrintableColumns();
            for (int r=0; r<rows; r++) {
              optCol = (*it)->getOptimizeColumn(printableCols, r);
              if (optCol != -1) {
                 // This row will be optimized so only count once
                 if ((int)funcs[optCol] <= maxShell &&
                     (*infoIt).second.deleteFlags[(int)funcs[optCol]] == false) {
                    result += (mult[(int)funcs[optCol]] * elementCount);
                 }
              } else {
                 for (int c=0; c<cols; c++) {
                    if ((int)funcs[c] > maxShell) continue;
                    if (coeffs[(cols*r)+c] != 0.0 &&
                          (*infoIt).second.deleteFlags[(int)funcs[c]] == false )
                       result += (mult[(int)funcs[c]] * elementCount);
                 }
              }
            }
          }
        }
      }
    }
    // ##### For all other cases effPrimitives = numPrimitives
  } else {
    result = countPrimitives(gbs, elementCount, element_);  
  }
  return result;
}

/*******************************************************************
 Method : count
 Summary: 
*******************************************************************/
int TGBSConfig::count
(
 TGBSConfig::CountType ct,
 TagCountMap& tagCounts,
 bool gc_code
) 
{
  int result=0;
  map<string, const TGaussianBasisSet*, less<string> > remainingCount;
  TGBSConfig::iterator config_it;
  TagCountMap::iterator tagCntIt;
  ContractionMap::const_iterator conts;
  TGBSGroup::iterator group_it;

  // Loop through all the groups in the config:
  for(config_it = begin(); config_it != end(); config_it++) {

    // Vector of individual elements
    TGBSConfigTags tags((*config_it).first.c_str());

    // Vector of individual atom tags (they include numerals)
    TGBSConfigTags atomTags((*config_it).first.c_str(), false, false);

    TGBSGroup *group = (*config_it).second;

    if(group)
    {
      for(size_t tag_it = 0; tag_it < tags.size(); tag_it++)
      {
            
        // determine the count of that element in the molecule
        int numAtoms = 0;
        bool countElement = false;
            
        tagCntIt = tagCounts.find(tags[tag_it]);

        if (tagCntIt != tagCounts.end())
        {
          if (p_type == TGBSConfig::atom &&
              tags[tag_it] != atomTags[tag_it]) // specific atoms 
          {
            numAtoms = 1;
            (*tagCntIt).second--;
            countElement = true;
          }
          else if (p_type == TGBSConfig::atom) // base atom
            countElement = false;
          
          else // not by-atom config
          {
            numAtoms = (*tagCntIt).second;
            countElement = true;
          }
        }

        // loop through all the basis sets in each group if not
        // counting ECP core electrons:
        if (ct != TGBSConfig::NumEcpCoreElectrons)
        {
          for(group_it  = group->begin();
              group_it != group->end(); group_it++)
          {
            const TGaussianBasisSet* gbs = (*group_it).second;

            // Don't count DFT Fittings!
            if (gbs->p_type == TGaussianBasisSet::exchange ||
                gbs->p_type == TGaussianBasisSet::charge)
              continue;

            if(countElement)
            {
              // look up the contraction for that element:
              conts = gbs->p_contractions.find(tags[tag_it]);
              if(conts != gbs->p_contractions.end() )
              {
                if (ct == TGBSConfig::EffNumPrimitives)
                  result += 
                    countEffPrimitives(gbs, numAtoms, tags[tag_it], gc_code);
                else if (ct == TGBSConfig::NumFunctions)
                  result += countFunctions(gbs, numAtoms, tags[tag_it]);
                else // counting NumPrimitives
                  result += countPrimitives(gbs, numAtoms, tags[tag_it]);
              }
            }
            else
              remainingCount[tags[tag_it]] = gbs;
          }
        }
        else // counting ecp core electrons
        {
          const TGaussianBasisSet* ecp = group->ecp();
          if (ecp != (TGaussianBasisSet*) 0 && countElement)
          {
            // look up the contraction for that element:
            conts = ecp->p_contractions.find(tags[tag_it]);
            if(conts != ecp->p_contractions.end() )
              result += 
                  countEcpCoreElectrons(ecp, numAtoms, tags[tag_it]);
          }     
          else if (ecp != (TGaussianBasisSet*) 0)
            remainingCount[tags[tag_it]] = ecp;
     
        }
      }
    }
  }
  // count the remaining functions if by atom configuration:
  if (p_type == TGBSConfig::atom)
  {
    int numAtoms;
    map<string, const TGaussianBasisSet*, less<string> >::iterator rIt;

    for (rIt = remainingCount.begin(); rIt != remainingCount.end();
         rIt++)
    {
      numAtoms = 0;
      tagCntIt = tagCounts.find((*rIt).first);
      if (tagCntIt != tagCounts.end())
        numAtoms = (*tagCntIt).second;
      if (ct == EffNumPrimitives)
        result += 
          countEffPrimitives((*rIt).second, numAtoms, (*rIt).first, gc_code);
      else if (ct == NumFunctions)
        result += countFunctions((*rIt).second, numAtoms, (*rIt).first);
      else if (ct == NumPrimitives) // counting NumPrimitives
        result += countPrimitives((*rIt).second, numAtoms, (*rIt).first);  
      else if (ct == NumEcpCoreElectrons)
        result += 
          countEcpCoreElectrons((*rIt).second, numAtoms, (*rIt).first);   
    }
  }
  return result;
}



/* =================================================================
   Methods for GBS Export
   ============================================================== */

/*******************************************************************
 Method : dump
 Summary: Prints out exponents and coefficients in format specified
          by code_name.
          
          User is responsible to free memory from return pointer.
*******************************************************************/
// top level export method - public
const char* TGBSConfig::dump(const char* code_name, bool useNames) 
{
  ostrstream orbital_os;
  ostrstream orbitalName_os;
  ostrstream exchange_os;
  ostrstream exchangeName_os;
  ostrstream charge_os;
  ostrstream chargeName_os;
  ostrstream ecp_os;
  ostrstream ecpName_os;
  ostrstream* gbs_os;
 
  TGBSConfig::iterator conf_it;
  TGBSConfigTags::iterator tag_it;
  OrbitalMap::const_iterator orbitalMapIt;
  
  // Create code object
  const JCode* code = CodeFactory::lookup(code_name);

  // for every group in the config
  for(conf_it = begin(); conf_it != end(); conf_it++) {

    // skip null groups
    if ((*conf_it).second == (TGBSGroup*) 0)
      continue;

    // (get ordered list of basis sets in the group)
    const vector<TGaussianBasisSet*>* gbsList = 
      (*conf_it).second->getOrderedList();

    // Get list of elements in group
    TGBSConfigTags tags((*conf_it).first.c_str());

   
    // for every element in the group's tag
    for(tag_it = tags.begin(); tag_it != tags.end(); tag_it++)
    {
      ostrstream taiorbital_os;     // need to put here to reallocate buffer

      // for every basis set in the group
      for(size_t i = 0; i < gbsList->size(); i++)
      {
        if((*gbsList)[i]->p_type == TGaussianBasisSet::exchange) {
          gbs_os = 
            print_contraction((*gbsList)[i],(*tag_it).c_str(), code);
          exchange_os << gbs_os->str();
          delete gbs_os;
          
          exchangeName_os << *tag_it << " library \"" 
                          << (*gbsList)[i]->p_name << "\"" << endl;

        } else if((*gbsList)[i]->p_type == TGaussianBasisSet::charge) {
          gbs_os = 
            print_contraction((*gbsList)[i],(*tag_it).c_str(), code);
          charge_os << gbs_os->str();
          delete gbs_os;

          chargeName_os << *tag_it << " library \"" 
                        << (*gbsList)[i]->p_name << "\"" << endl;

        } else if((*gbsList)[i]->p_type == TGaussianBasisSet::polarization ||
            (*gbsList)[i]->p_type == TGaussianBasisSet::rydberg ||
            (*gbsList)[i]->p_type == TGaussianBasisSet::diffuse ||
            (*gbsList)[i]->p_type == TGaussianBasisSet::appended_diffuse ||
            (*gbsList)[i]->p_type == TGaussianBasisSet::appended_tight) {
          gbs_os = 
            print_contraction((*gbsList)[i],(*tag_it).c_str(), code);
          taiorbital_os << gbs_os->str();
          delete gbs_os;

          // Don't print the aux basis set name here, as we are printing
          // orbital basis set names from the orbital map instead
          

        } else { // it is an orbital basis set
          gbs_os = 
            print_contraction((*gbsList)[i],(*tag_it).c_str(), code);
          orbital_os << gbs_os->str();
          delete gbs_os;

          // Don't print the basis set name here, as we are printing
          // orbital basis set names from the orbital map instead
        }
      }

      // Print the groups orbital basis sets for the name basis     
      const OrbitalMap* orbitalMappings = 
        (*conf_it).second->getOrbitalGBSMappings();
      for (orbitalMapIt = orbitalMappings->begin();
           orbitalMapIt != orbitalMappings->end(); orbitalMapIt++) {
        
        orbitalName_os << *tag_it << " library \"" 
                       << (*orbitalMapIt).first << "\"" << endl;
      }

      // Concatenate aux basis sets with orbital basis set for numeric basis
      taiorbital_os << ends;
      orbital_os << taiorbital_os.str();
      
      // if the group has an ecp, print it too
      if((*conf_it).second->ecp()) {
        gbs_os = 
          print_potential(
              (*conf_it).second->ecp(),
              (*tag_it).c_str());
        ecp_os <<  gbs_os->str();

        // Only print the name if the ecp has data for that element:
        if (strcmp(gbs_os->str(), "") != 0) {
          ecpName_os << *tag_it << " library \"" 
                     << (*conf_it).second->ecp()->p_name << "\"" << endl;
        }
        delete gbs_os;
      }
    }
  }

  // Put the pieces together to make the final input to script:
  ostrstream os;
  ostrstream osName;
  const char* coords = (p_coordSys == TGaussianBasisSet::Cartesian) 
    ? "cartesian" : "spherical";

  os << "NumericalBasis" << endl;
  osName << "NameBasis" << endl;

  if(orbital_os.pcount() > 0) {
    os << "basis \"ao basis\" " << coords << " print" << endl;
    orbital_os << ends;
    os << orbital_os.str();
    os << "END" << endl;

    osName << "basis \"ao basis\" " << coords << " print" << endl;
    orbitalName_os << ends;
    osName << orbitalName_os.str();
    osName << "END" << endl;;
  }

  if(exchange_os.pcount() > 0) {
    os << endl << "basis \"xc basis\" " << coords << " print" << endl;
    exchange_os << ends;
    os <<  exchange_os.str();
    os << "END" << endl;

    osName << endl << "basis \"xc basis\" " << coords << " print" << endl;
    exchangeName_os << ends;
    osName <<  exchangeName_os.str();
    osName << "END" << endl;
  }

  if(charge_os.pcount() > 0) {
    os << endl << "basis \"cd basis\" " << coords << " print" << endl;
    charge_os << ends;
    os <<  charge_os.str();
    os << "END" << endl;

    osName << endl << "basis \"cd basis\" " << coords << " print" << endl;
    chargeName_os << ends;
    osName << chargeName_os.str();
    osName << "END" << endl;
  }

  if(ecp_os.pcount() > 0) {
    os << endl << "ECP" << endl;
    ecp_os << ends;
    os <<  ecp_os.str();
    os << "END" << endl;

    osName << endl << "ECP" << endl;
    ecpName_os << ends;
    osName << ecpName_os.str();
    osName << "END" << endl;
  }

  os << "EndNumericalBasis";
  os << ends;

  osName << "EndNameBasis";
  osName << ends;

  ostrstream combinedFile;
 
  // Only print out the name basis if no fancy stuff (i.e., multiple orbitals,
  // appended functions, deleted/uncontracted contractions) have been used.
  string configName = name();
  if (useNames && configName != "Custom" && 
      configName != "Imported") {
    string tmp(osName.str());
    // Call the method that checks which basis sets to always override the
    // user setting for named basis sets for, such as CRENBL and CRENBS
    if (!alwaysUseExp(tmp)) {
      combinedFile << tmp;
    }
  }
  combinedFile << endl << endl << os.str();
  combinedFile << ends;

  // Pass data through nethe appropriate script for formatting:
  SFile* sourceFile = TempStorage::getTempFile();
  SFile* resultsFile = TempStorage::getTempFile();
  string codeName = code->getScript("GBSExport");

  EE_ASSERT(codeName != "", EE_WARNING, "Unable to find script");

  // Write exp/coef data to temp file:
  ofstream sourceFS(sourceFile->path().c_str());
  sourceFS << combinedFile.str();
  sourceFS.close();

  // Exec the script:
  string command = codeName + " < " + sourceFile->path() + " > " +
    resultsFile->path();

  if(system(command.c_str()) < 0) {
    EE_ASSERT(0, EE_WARNING, "Cannot execute script: " + codeName);
    sourceFile->remove();
    resultsFile->remove();
    return 0;
  }

  // Read results from temp file:
  ifstream resultsFS(resultsFile->path().c_str());
  ostrstream resultsStr;

  // Read whole file into the buffer of the ostrstream
  resultsFS.get(*(resultsStr.rdbuf()), '\0');
  resultsStr << ends; // end buffer with null character
  resultsFS.close();

  // Clean up temp files:
  sourceFile->remove();
  resultsFile->remove();

  return resultsStr.str();
}


bool TGBSConfig::alwaysUseExp(const string& name) const
{
  bool ret = false;

  string infile = Ecce::ecceDataPrefPath();
  infile += "/BasisSetUseExp";
  ifstream is(infile.c_str());

  char cbuf[256];

  while (is.getline(cbuf, 255)) {
    if (cbuf[0]!='#' && cbuf[0]!='\0' && name.find(cbuf)!=string::npos) {
      ret = true;
      break;
    }
  }

  is.close();

  return ret;
}


/*******************************************************************
 Method : print_contraction

 Summary: Prints the exponents and coefficients for one basis set.
          Tag represents the tag for the group in which the basis set
          resides.  The basis set may not support all the elements in
          the tag.

          Exact format of the dump is dependent upon the contraction
          type of the gbs, as well as whether the code supports
          general contractions.

          It is assumed that no basis set can be assigned to the
          config if the maxlvalue for the code would be exceeded.
          Therefore, no checking of maxlvalue occurs here, and
          all contractions are printed.
*******************************************************************/
ostrstream* TGBSConfig::print_contraction(const TGaussianBasisSet* gbs,
                                          const char* tag,
                                          const JCode* code) 
{
  // Check preconditions
  EE_ASSERT(gbs, EE_FATAL, "Trying to print null basis set!")
  EE_ASSERT(tag, EE_FATAL, "Tag not defined!")
  EE_ASSERT(code, EE_FATAL, "No code defined!")

  // Find out if the code supports general contractions:
  bool gcSupported;
  code->get_bool("GeneralContractions", gcSupported);

  // For segmented and unknown contraction types, print everything
  if (gbs->p_contType == TGaussianBasisSet::Segmented ||
      gbs->p_contType == TGaussianBasisSet::UnknownContType)
    return print_all_contractions(gbs, tag);

  // If uncontracted, optimize contractions
  else if (gbs->p_contType == TGaussianBasisSet::Uncontracted)
    return print_contractions_optimize_gc(gbs, tag);

  else if (gbs->p_contType == TGaussianBasisSet::GC)
  {
    if (p_optimize == false)
    {
      // If gc supported by code, print everything
      if (gcSupported)
        return print_all_contractions(gbs, tag);

      else // consolidate contractions in a different way than optimize
        return print_contractions_noop_gcNotSupported(gbs, tag);
    }
    else // optimize is true, so go ahead and optimize
      return print_contractions_optimize_gc(gbs, tag);
  }
  return (ostrstream*) 0;
}

/*******************************************************************
 Method : print_all_contractions

 Summary: Prints all the exponents and coefficients for one basis set,
          just as they appear in the contraction set.  No attempt at
          consolidation is made.
*******************************************************************/
ostrstream* TGBSConfig::print_all_contractions
(const TGaussianBasisSet* gbs, 
 const char* tag) 
{
  ostrstream *result = new ostrstream();
  ContractionMap::const_iterator cont_list_it = gbs->p_contractions.find(tag);
  

  // find the vector of contractions for that symbol
  if(cont_list_it != gbs->p_contractions.end()) {

    ContractionSet* cont_list = (*cont_list_it).second;
    ContractionSet::const_iterator cont_it;

    // Get the delete/uncontract meta data for that element and basis set
    GBSToContInfoMap* infoMap = getContractionInfoMap(tag);
    string uniqueKey = gbs->getUniqueKey();
    GBSToContInfoMap::iterator infoIt;
    ContractionInfo* contInfo = 0;

    if (infoMap != 0) {
      infoIt = infoMap->find(uniqueKey);   
      if (infoIt != infoMap->end()){ 
        contInfo = &((*infoIt).second);
      }
    }

    // Loop through all the contractions in the contracted set
    for(cont_it  = cont_list->begin();
        cont_it != cont_list->end(); cont_it++)
    {
      Contraction_* cont = (*cont_it);
      bool uncontracted = false;
      vector<int> uniqueShells;
      vector<bool> shells(TGaussianBasisSet::maxShell +1, false);
      int lastShell = -1;
      int curShell;

      // find out what shells belong in the contraction, and if
      // one of the shells is uncontracted:
      // (only include shells that are not deleted!)
      for(size_t shell_idx=0; shell_idx<cont->shells.size(); shell_idx++) {
        curShell = (int)cont->shells[shell_idx];
        if (curShell != lastShell) {
          if (contInfo == 0 || contInfo->deleteFlags[curShell] == false) {
            uniqueShells.push_back(curShell);
            shells[curShell] = true;
          }
          lastShell = curShell;
        }
        if (contInfo != 0 && contInfo->uncontractFlags[curShell] == true) {
          uncontracted = true;
        }
      }

      if (contInfo == 0) {
        printNormalContraction(result, cont, shells, tag);

      } else if (uncontracted == false) { // print like normal
        
        printNormalContraction(result, cont, shells, tag);

      } else if (uniqueShells.size() == 1) { // only 1 shell represented
        
        printUncontractedContraction(result, cont, uniqueShells[0], tag);

      } else { // shared-shell contraction needs to be split up
        vector<bool> tmp(TGaussianBasisSet::maxShell + 1, false);
        
        for (size_t i = 0; i < uniqueShells.size(); i++) {
          
          if (contInfo->uncontractFlags[uniqueShells[i]] == false) {
            tmp[i] = true;
            printNormalContraction(result, cont, tmp, tag);
            tmp[i] = false;

          } else {
            printUncontractedContraction(result, cont, uniqueShells[i], tag);
          }
        }
      }
    }
  }
  (*result) << ends;
  return result;
}
//####################################################################
void TGBSConfig::printNormalContraction(ostrstream* result, 
                                        Contraction_* cont, 
                                        const vector<bool>& shells,
                                        const char* tag)
{
  int num_coef = cont->num_coefficients;
  int num_exp = cont->num_exponents;

  // Print element label
  (*result) << tag << "    ";
  
  // Print shell label
  for(size_t i = 0; i < shells.size(); i++) {
    if (shells[i] == true) {
      (*result) << TGaussianBasisSet::shell_formatter[i];
    }
  }
  (*result) << endl;

  // Print exps and coefs:
  for(int j=0;j<num_exp;j++) {
    (*result).setf(ios::fixed|ios::showpoint|ios::right);
    (*result).width(23); (*result).precision(12);
    (*result) << cont->exponents[j];
          
    for(int l=0;l< num_coef;l++) {
      if (shells[(int)cont->shells[l]] == true) {
        (*result).setf(ios::fixed|ios::showpoint|ios::right);
        (*result).width(23); (*result).precision(12);
        (*result) << cont->coefficients[l + (num_coef*j)];
      }
    }
    (*result) << endl;
  }
}

//###################################################################
void TGBSConfig::printUncontractedContraction(ostrstream* result, 
                                              Contraction_* cont, 
                                              const int& shell,
                                              const char* tag)
{
  // assume this will effect only 1 shell for the time being

  int num_exp = cont->num_exponents;

  // print every gaussian primitive as a separate contraction
   
  for(int j = 0; j < num_exp; j++) {
      
    (*result) << tag << "    "
              << TGaussianBasisSet::shell_formatter[shell] << endl;
    // print exp 
    (*result).setf(ios::fixed|ios::showpoint|ios::right);
    (*result).width(23); (*result).precision(12);
    (*result) << cont->exponents[j];
    
    // Print out 1.0 for the coefficient, since the coefficients for
    // each function must normalize to 1.0
    (*result).setf(ios::fixed|ios::showpoint|ios::right);
    (*result).width(23); (*result).precision(12);
    (*result) << 1.0;
    (*result) << endl;      
  }
}


/*******************************************************************
 Method : print_contractions_optmize_gc

 Summary: Use this format if the gbs is Uncontracted or if
          the gbs is GC (generally contracted) AND optimize is
          TRUE.
*******************************************************************/
ostrstream* TGBSConfig::print_contractions_optimize_gc
(const TGaussianBasisSet* gbs, 
 const char* tag) 
{
  ostrstream *result = new ostrstream();
  ostrstream *firstSet;  // non-optimized rows
  ostrstream *secondSet; // optimized rows
  char* temp;

  ContractionMap::const_iterator cont_list_it = gbs->p_contractions.find(tag);

  // find the vector of contractions for that symbol
  if(cont_list_it != gbs->p_contractions.end()) {

    ContractionSet* cont_list = (*cont_list_it).second;
    ContractionSet::const_iterator cont_it;

    // Get the delete/uncontract meta data for that element and basis set
    GBSToContInfoMap* infoMap = getContractionInfoMap(tag);
    string uniqueKey = gbs->getUniqueKey();
    GBSToContInfoMap::iterator infoIt;
    ContractionInfo* contInfo = 0;

    if (infoMap != 0) {
      infoIt = infoMap->find(uniqueKey);   
      if (infoIt != infoMap->end()){ 
        contInfo = &((*infoIt).second);
      }
    }

    Contraction_* cont = 0; 
    int num_coef;
    int num_exp;
    int j,f;

    // loop through each contraction in the contracted set
    for(cont_it  = cont_list->begin();
        cont_it != cont_list->end(); cont_it++) {
      cont = (*cont_it);
      firstSet = new ostrstream;
      secondSet = new ostrstream;

      // First determine if the contraction has been deleted:
      // (we can use shells[0] to represent the shell for the whole
      // contraction, as this type of basis set won't have shared shell
      // contractions)
      if (contInfo != 0 &&
          (contInfo->deleteFlags[cont->shells[0]] == true) ) {
        continue;
      }

      num_coef = cont->num_coefficients;
      num_exp = cont->num_exponents;

      // first determine what columns can be printed:
      vector<bool> printableCols = cont->getPrintableColumns();


      // Loop through each row in the table of exps and coefs
      for (j = 0; j < num_exp; j++) {

         int optCol = cont->getOptimizeColumn(printableCols, j);

         if (optCol == -1) {
            // print all the coefficients that are not zero

            if (j == 0 || 
                  (contInfo != 0 && 
                   contInfo->uncontractFlags[cont->shells[0]] == true)) {

               // Print the first contraction/shell label:
               (*firstSet) << tag << "    ";
               (*firstSet) << TGaussianBasisSet::shell_formatter[cont->shells[0]];
               (*firstSet) << endl;
            }

            // Print exponent for row
            firstSet->setf(ios::fixed|ios::showpoint|ios::right);
            firstSet->width(23); firstSet->precision(12);
            (*firstSet) << cont->exponents[j];

            if (contInfo != 0 && 
                  contInfo->uncontractFlags[cont->shells[0]] == true) {
               firstSet->setf(ios::fixed|ios::showpoint|ios::right);
               firstSet->width(23); firstSet->precision(12);
               (*firstSet) << 1.0;

            } else {
               // Print coefficients for row
               for (f = 0; f < num_coef; f++) {
                  if (printableCols[f]) {
                     firstSet->setf(ios::fixed|ios::showpoint|ios::right);
                     firstSet->width(23); firstSet->precision(12);
                     (*firstSet) << cont->coefficients[f + (num_coef*j)];
                  }
               }
            }
            (*firstSet) << endl;

         } else { 

            // Print the contraction/shell label:
            (*secondSet) << tag << "    ";
            (*secondSet) << TGaussianBasisSet::shell_formatter[cont->shells[optCol]];
            (*secondSet) << endl;

            // Print exponent
            secondSet->setf(ios::fixed|ios::showpoint|ios::right);
            secondSet->width(23); secondSet->precision(12);
            (*secondSet) << cont->exponents[j];

            // Print the coefficient
            secondSet->setf(ios::fixed|ios::showpoint|ios::right);
            secondSet->width(23); secondSet->precision(12);
            (*secondSet) << cont->coefficients[optCol + (num_coef*j)];
            (*secondSet) << endl;

         }
      }
      (*firstSet) << ends;
      (*secondSet) << ends;
      temp = firstSet->str();
      (*result) << temp;
      delete temp;
      temp = secondSet->str();
      (*result) << temp;
      delete temp;
      delete firstSet;
      delete secondSet;    
    }
  }
  (*result) << ends;
  return result;
}
/*******************************************************************
 Method : print_contractions_noop_gcNotSupported

 Summary: We are not optimizing, but code doesn't support general
          contractions.  Therefore, they can't be printed as is.
          A special consolidation routine is performed, different
          from optimize.

          Don't print any contractions that have a coefficient of 0.
*******************************************************************/
ostrstream* TGBSConfig::print_contractions_noop_gcNotSupported
(const TGaussianBasisSet* gbs, 
 const char* tag)
{
  ostrstream *result = new ostrstream();
  ContractionMap::const_iterator cont_list_it = gbs->p_contractions.find(tag);

  // find the vector of contractions for that symbol
  if(cont_list_it != gbs->p_contractions.end()) {

    ContractionSet* cont_list = (*cont_list_it).second;
    ContractionSet::const_iterator cont_it;

    // Get the delete/uncontract meta data for that element and basis set
    GBSToContInfoMap* infoMap = getContractionInfoMap(tag);
    string uniqueKey = gbs->getUniqueKey();
    GBSToContInfoMap::iterator infoIt;
    ContractionInfo* contInfo = 0;

    if (infoMap != 0) {
      infoIt = infoMap->find(uniqueKey);   
      if (infoIt != infoMap->end()){ 
        contInfo = &((*infoIt).second);
      }
    }

    int num_coef;
    int num_exp;
    bool uncontractFlag;
    bool optimizableCol;
    int f,j;

    // loop through each contraction in the contracted set
    for(cont_it  = cont_list->begin();
        cont_it != cont_list->end(); cont_it++)
    {
      Contraction_* cont = (*cont_it);

      // If this contraction has been deleted, go to next contraction:
      // (we can use shells[0] to represent the shell for the whole
      // contraction, as this type of basis set won't have shared shell
      // contractions)
      if (contInfo != 0 &&
          contInfo->deleteFlags[cont->shells[0]] == true) {
        continue;
      }

      num_coef = cont->num_coefficients;
      num_exp = cont->num_exponents;

      // Loop through each column (shell) in the table of exps and coefs
      for (f = 0; f < num_coef; f++)
      {
        uncontractFlag = (contInfo != 0 && 
                          contInfo->uncontractFlags[cont->shells[f]] == true);

        // first find out if the column is optimizable
        optimizableCol = false;
  
        for (j=0; j<num_exp; j++) {
          if (cont->coefficients[f + (num_coef*j)] == 0.0) {
            optimizableCol = true;
            break;
          }
        }

        // Only print the exponents if the coef for that column is not 0:
        for(j=0;j<num_exp;j++)
        {           
          if (j == 0 || (uncontractFlag && !optimizableCol))
          {
            // Print the contraction/shell label:
            (*result) << tag << "    ";
            (*result) << TGaussianBasisSet::shell_formatter[(int)cont->shells[f]];
            (*result) << endl;        
          }

          if (cont->coefficients[f + (num_coef*j)] == 0.0)
            continue;

          // Print exponent:
          (*result).setf(ios::fixed|ios::showpoint|ios::right);
          (*result).width(23); (*result).precision(12);
          (*result) << cont->exponents[j];
          
          // Print coefficient:
          (*result).setf(ios::fixed|ios::showpoint|ios::right);
          (*result).width(23); (*result).precision(12);
          
          if (uncontractFlag) {
            (*result) << 1.0;
          } else {
            (*result) << cont->coefficients[f + (num_coef*j)];
          }
          (*result) << endl;          
        }
      }
    }
  }
  (*result) << ends;
  return result;
}

/*******************************************************************
 Method : print_potential
 Summary: ECP is a TGaussianBasisSet in which the mapping of
          element name to vector of contractions
          is replaced by a mapping to a vector of ecp_parts.  Each
          ecp_part is a contraction with additional data members.
*******************************************************************/
ostrstream* TGBSConfig::print_potential(const TGaussianBasisSet* ecp, 
                                        const char* tag) 
{
  ostrstream* os = new ostrstream();
  ContractionMap::const_iterator potential_it = ecp->p_contractions.find(tag);

  // find the vector of contractions for that symbol
  if(potential_it != ecp->p_contractions.end()) {

    ContractionSet* potential = (*potential_it).second;
    (*os) << tag << " nelec " << potential->ncore << endl;

    ContractionSet::const_iterator part_it;
    string shell; 
    int num_exp;

    for(part_it  = potential->begin();
        part_it != potential->end(); part_it++) {
      Contraction_* part = (*part_it);
 
      if (part_it == potential->begin())
        shell = "ul";
      else
        shell = tolower(TGaussianBasisSet::shell_formatter[part->l][0]);

      (*os) << " " << tag << " " << shell << endl;

      num_exp = part->num_exponents;
      for(int j=0;j<num_exp;j++) {
        os->width(5); os->setf(ios::right);
        (*os) << part->powers[j];
        os->setf(ios::fixed|ios::showpoint|ios::right);
        os->width(24); os->precision(12); 
        (*os) << part->exponents[j];
        os->setf(ios::fixed|ios::showpoint|ios::right);
        os->width(24); os->precision(12); 
        (*os) << part->coefficients[j];
        (*os) << endl;
      }
    }
  }
  (*os) << ends;
  return os;
}

