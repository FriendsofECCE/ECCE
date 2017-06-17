//////////////////////////////////////////////////////////////////////////////
// SOURCE FILE: TGBSGroup.C
//
//
// DESIGN:
//
///////////////////////////////////////////////////////////////////////////////

#include <unistd.h>
#include <stdio.h>

#include <iostream>
  using std::cout;
  using std::endl;
  using std::ends;
#include <strstream>
  using std::istrstream;

#include <algorithm>
#include <set>

  using std::set_difference;
  using std::sort;

#include "util/ErrMsg.H"
#include "util/Ecce.H"

#include "tdat/Fragment.H"
#include "tdat/TTheory.H"

#include "dsm/TGBSConfig.H"
#include "dsm/TGaussianBasisSet.H"
#include "dsm/TGBSGroup.H"

/*******************************************************************
 Method : Default constructor
 Summary: 
*******************************************************************/
TGBSGroup::TGBSGroup()
{ // maps are empty
  // name is empty
  // p_gbsOrderedVector is empty

  p_ecp = (TGaussianBasisSet*) 0; // ecp is empty
}

/*******************************************************************
 Method : Copy constructor
 Summary: Performs a deep copy.
*******************************************************************/
TGBSGroup::TGBSGroup(TGBSGroup& tgbsGroup)
{
  for (size_t i = 0; i < tgbsGroup.p_gbsOrderedVector.size(); i++) {
    
    insertGBS(new TGaussianBasisSet(*(tgbsGroup.p_gbsOrderedVector[i])));
  }
  if (tgbsGroup.p_ecp != (TGaussianBasisSet*) 0)
    p_ecp = new TGaussianBasisSet(*(tgbsGroup.p_ecp));
  else
    p_ecp = (TGaussianBasisSet*) 0;

  p_name = tgbsGroup.p_name;

  // Copy metadata maps  
  // (shouldnt have to do a deep copy because these dont contain pointers)
  p_orbitals = tgbsGroup.p_orbitals;
  p_gbsCount = tgbsGroup.p_gbsCount;
  p_contractionInfoMap = tgbsGroup.p_contractionInfoMap;
  p_tightFunctions = tgbsGroup.p_tightFunctions;
  p_diffuseFunctions = tgbsGroup.p_diffuseFunctions;
}

/*******************************************************************
 Method : Destructor
 Summary: 
*******************************************************************/
TGBSGroup::~TGBSGroup()
{
  TGBSGroup::iterator it;
  for (it = begin(); it != end(); it++)
    if ((*it).second != (TGaussianBasisSet*) 0)
      delete (*it).second;

  if (p_ecp != (TGaussianBasisSet*) 0)
    delete p_ecp;
}

//// MODIFIERS ////

/*******************************************************************
 Method : getContractionInfoMap
 Summary: 
*******************************************************************/
ElementToGBSInfoMap& TGBSGroup::getContractionInfoMap()
{
  return p_contractionInfoMap;
}

/*******************************************************************
 Method : getOrbitalBasisSetList
 Summary: This function is used to get a list of all the orbital
          basis sets in the group.  This is necessary when 
          trying to reproduce the group.
*******************************************************************/
vector<string>* TGBSGroup::getOrbitalGBSIdentifiers() const 
{
  vector<string>* vec = new vector<string>;
  OrbitalMap::const_iterator it;

  for (it = p_orbitals.begin(); it != p_orbitals.end(); it++) {
    vec->push_back((*it).first);
  }
  return vec;
}

/*******************************************************************
 Method : getAuxGBSIdentifiers
 Summary: This function is used to get a list of all ALL augmenting
          basis sets (this includes polarization, diffuse, rydberg,
          exchange, charge, AND ecp) that were NOT added as part of 
          an orbital aggregate group
*******************************************************************/
vector<GBSKey>* TGBSGroup::getAuxGBSIdentifiers() const
{
  vector<GBSKey>* vec = new vector<GBSKey>;
  GBSKey keyObject;

  // how many orbitals does this aux Gbs belong to
  GBSCountMap::const_iterator cntIt; 
  TGBSGroup::const_iterator groupIt;
  string bsKey;

  // for each basis set in the group (don't forget ECP too)
  for(groupIt = begin(); groupIt != end(); groupIt++) {
    
    // If the basis set is NOT in any of the orbital basis set AGG groups
    // then add it to the list
    bsKey = (*groupIt).second->getUniqueKey();
    cntIt = p_gbsCount.find(bsKey);
    if (cntIt == p_gbsCount.end()) {
      keyObject.name = (*groupIt).second->p_name;
      keyObject.type = (*groupIt).second->p_type;
      vec->push_back(keyObject);
    }
  }
  // check ecp too
  if (p_ecp != 0) {
    bsKey = p_ecp->getUniqueKey();
    cntIt = p_gbsCount.find(bsKey);
    if (cntIt == p_gbsCount.end()) {
      keyObject.name = p_ecp->p_name;
      keyObject.type = p_ecp->p_type;
      vec->push_back(keyObject);
    }
  }
  return vec;
}    

/*******************************************************************
 Method : getRemovedAuxGBSIdentifiers
 Summary: This function is used to get a list of all ALL augmenting
          basis sets (this includes polarization, diffuse, rydberg,
          exchange, charge, AND ecp) that were REMOVED from 
          an orbital aggregate group
*******************************************************************/
vector<string>* TGBSGroup::getRemovedAuxGBSIdentifiers() const
{
  vector<string>* vec = new vector<string>;
  OrbitalMap::const_iterator orbIt;
  TGBSGroup::const_iterator groupIt;
  const vector<string>* keys;
  size_t i;
  bool match;

  // for each aggregate group
  for(orbIt = p_orbitals.begin(); orbIt != p_orbitals.end(); orbIt++) {
    
    keys = &((*orbIt).second);
    for (i = 0; i < keys->size(); i++) {
     
      match=false;

      // for each basis set in the group
      for(groupIt = begin(); groupIt != end() && !match; groupIt++) {
        
        // If the key isn't here, add it to the list:
        if ((*groupIt).second->getUniqueKey() == (*keys)[i]) {
          match = true;
        }
      }
      // (don't forget ECP too)
      if (p_ecp != 0 && p_ecp->getUniqueKey() == (*keys)[i]) {
        match = true;
      }
      if(!match) {
        vec->push_back((*keys)[i]);
      }
    }
  }
  return vec;
}    

/*******************************************************************
 Method : getTightMap
 Summary: 
*******************************************************************/
FunctionMap& TGBSGroup::getTightMap()
{
  return p_tightFunctions;
}

/*******************************************************************
 Method : getDiffuseMap
 Summary: 
*******************************************************************/
FunctionMap& TGBSGroup::getDiffuseMap()
{
  return p_diffuseFunctions;
}

/*******************************************************************
 Method : setOrbitalGBSMappings
 Summary: 
*******************************************************************/
void TGBSGroup::setOrbitalGBSMappings(const OrbitalMap& map)
{
  p_orbitals = map;
}

/*******************************************************************
 Method : setGBSCount
 Summary: 
*******************************************************************/
void TGBSGroup::setGBSCount(const GBSCountMap& map) 
{
  p_gbsCount = map;
}

/*******************************************************************
 Method : insertGBS
 Summary: Inserts a new GBS into the group and keeps track of
          insertion order.

          If inserting a charge or exchange dft fitting, and
          one already exists for this group, the charge or
          exchange will be replaced, and the old one will be
          deleted.

          If inserting an ecp and an ecp already exists for this 
          group, insert replaces the ecp and deletes the old one.

          Returns false if trying to insert an exact gbs that is already
          in the group.

          This keeps one set of basis sets with no duplicates.  This
          is the set that is used to build the input files to the
          chemical codes.

*******************************************************************/
bool TGBSGroup::insertGBS(TGaussianBasisSet* gbs, bool recompute) {
 
  EE_ASSERT(gbs, EE_FATAL, "Can't insert a null basis set into a group!");
  bool ret = true;

  // If gbs type is ecp, then add it to the group: 
  if(gbs->p_type == TGaussianBasisSet::ecp) {
    if (p_ecp != (TGaussianBasisSet*) 0)  {
                  
      if (gbs == p_ecp) {
        ret = false;
      }
      else {
        delete p_ecp;
      }
    }
    p_ecp = gbs;

  } else {
    // First check to see if the basis set already is in the group:
    TGBSGroup::iterator it = find(gbs->p_type);
    if(it != end()) {
      do {
        if (gbs->p_name == (*it).second->p_name) {
          ret = false; // don't add gbs because it already is in group
          break;
        }
        it++;
      } while(it != upper_bound(gbs->p_type));
    }

    if (ret != false) {
      // If inserting a charge or exchange fitting, delete old one, if it
      // exists (can only have 1)
      if(gbs->p_type == TGaussianBasisSet::charge ||
         gbs->p_type == TGaussianBasisSet::exchange)
        removeGBS(gbs->p_type);
      
      // Insert basis set into the map:
      TGBSGroup::value_type v(gbs->p_type, gbs);
      insert(v);      
      
      // Update the ordered vector:
      insertOrderedGBS(gbs);

      // update the map of contraction info if not exchange or charge
      if (gbs->p_type != TGaussianBasisSet::charge &&
          gbs->p_type != TGaussianBasisSet::exchange) {

        ContractionMap::iterator contIt;
        ElementToGBSInfoMap::iterator elIt;
        TightMap::iterator tIt;
        DiffuseMap::iterator dIt;
        GBSToContInfoMap* curMap;
        string gbsKey;
        string elementName;
        vector<int> from;
        vector<int> to;
        
        // for every element in the gbs, add it to the outermost map if not
        // already there, then add the gbs's contraction info to the map 
        // for that element
        for (contIt = gbs->p_contractions.begin();
             contIt != gbs->p_contractions.end(); contIt++) {
          elementName = (*contIt).first;

          // first do the contraction info map
          elIt = p_contractionInfoMap.find(elementName);
          
          if (elIt == p_contractionInfoMap.end()) {
            // make new gbs map

            GBSToContInfoMap newMap;
            p_contractionInfoMap[elementName] = newMap;
            elIt = p_contractionInfoMap.find(elementName);
            
          } 
          curMap = &((*elIt).second);     
          
          gbsKey = gbs->getUniqueKey();
          ContractionInfo info(false, false);

          // Figure out the default settings for uncontract flag for each shell
          // Switch this logic so this flag only indicates if user pushed
          // uncontract, not if it is uncontracted
          /*
          gbs->contractionInfo(elementName, from, to);

          for (int i = 0; i < from.size(); i++ ) {
            
            if (from[i] > 0 && from[i] == to[i]) {
              info.uncontractFlags[i] = true;
            }
          }
          */
          (*curMap)[gbsKey] = info;
        
          // then do the tight and diffuse maps
          if (gbs->p_type != TGaussianBasisSet::appended_tight &&
              gbs->p_type != TGaussianBasisSet::appended_diffuse) {

            int vecSize = (int) TGaussianBasisSet::maxShell + 1;
            
            tIt = p_tightFunctions.find(elementName);
            if (tIt == p_tightFunctions.end()) {
              vector<int> vec(vecSize, 0);
              p_tightFunctions[elementName] = vec;
            }
            dIt = p_diffuseFunctions.find(elementName);
            if (dIt == p_diffuseFunctions.end()) {
              vector<int> vec(vecSize, 0);
              p_diffuseFunctions[elementName] = vec;
            }
          }
        }
        if(recompute) { 
          recomputeDiffuse();
          recomputeTight();
        }
      }
    }
  }
  return ret;
}

/*******************************************************************
 Method : cleanMetadataMaps
 Summary: Used when switching between simple and by-element
          configurations.  Gets rid of metadata for all other
          elements but the given tag;
*******************************************************************/
void TGBSGroup::cleanMetadataMaps(const string& element) {

  TightMap::iterator tIt;
  TightMap newTight;
  DiffuseMap::iterator dIt;
  DiffuseMap newDiffuse;
  ElementToGBSInfoMap::iterator eIt;
  ElementToGBSInfoMap newContInfo;

  for (tIt = p_tightFunctions.begin(); tIt != p_tightFunctions.end();
       tIt++) {
    if ((*tIt).first == element) {
      newTight[element] = (*tIt).second;
      break;
    }
  }
  p_tightFunctions = newTight;

  for (dIt = p_diffuseFunctions.begin(); dIt != p_diffuseFunctions.end();
       dIt++) {
    if ((*dIt).first == element) {
      newDiffuse[element] = (*dIt).second;
      break;
    }
  }
  p_diffuseFunctions = newDiffuse;

  for (eIt = p_contractionInfoMap.begin(); 
       eIt != p_contractionInfoMap.end();
       eIt++) {
    if ((*eIt).first == element) {
      newContInfo[element] = (*eIt).second;
      break;
    }
  }
  p_contractionInfoMap = newContInfo;
}


/*******************************************************************
 Method : insertOrderedGBS
 Summary: Adds basis set to the ordered list.

          Ordering is as follows

          1) orbital basis sets in FIFO order
          2) auxiliary basis sets in FIFO order
          3) appended_diffuse basis sets
          4) appended_tight basis sets

*******************************************************************/
void TGBSGroup::insertOrderedGBS(TGaussianBasisSet* gbs) {
  vector<TGaussianBasisSet*>::iterator vecIt;
  
  for (vecIt = p_gbsOrderedVector.begin(); 
       vecIt != p_gbsOrderedVector.end(); vecIt ++) {
    
    if ((*vecIt)->p_category != TGaussianBasisSet::ORBITAL &&
        gbs->p_category == TGaussianBasisSet::ORBITAL) {
      break;
    
    } else if (gbs->p_type != TGaussianBasisSet::appended_diffuse &&
               gbs->p_type != TGaussianBasisSet::appended_tight &&
               ((*vecIt)->p_type == TGaussianBasisSet::appended_diffuse ||
                (*vecIt)->p_type == TGaussianBasisSet::appended_tight) ) {
      break;
  
    } else if (gbs->p_type == TGaussianBasisSet::appended_diffuse &&
               (*vecIt)->p_type == TGaussianBasisSet::appended_tight) {
      break;
    }
    
  }
  p_gbsOrderedVector.insert(vecIt, gbs);
    
}
     
/*******************************************************************
 Method : insertOrbitalGBS
 Summary: This function takes care of preserving the aggregate
          mapping that can be associated with some basis sets.

          This method inserts every pointer into the map (without
          making a copy) - so make sure you don't delete the pointers.
*******************************************************************/
void TGBSGroup::insertOrbitalGBS(string bsName, 
                                 vector<TGaussianBasisSet*>& gbsList,
                                 bool recompute) 
{
  vector<string> bsNames;
  string key;  // used to uniquely identify a basis set

  // Insert the real basis sets into the main map, removing duplicates
  for (size_t i = 0; i < gbsList.size(); i++) {
    // If inserting an aggregate group with both exchange and
    // charge fittings, then only insert the charge fitting
    // for efficiency purposes.
    if (gbsList[i]->p_type == TGaussianBasisSet::charge) {
      removeGBS(TGaussianBasisSet::exchange);
    }
    if (gbsList[i]->p_type != TGaussianBasisSet::exchange ||
        find(TGaussianBasisSet::charge) == end()) {

      // wont insert if the basis set is already in group
      insertGBS(gbsList[i], recompute);
      //insertGBS(new TGaussianBasisSet (*(gbsList[i])), recompute);

      // Names are not unique for some of the aggregate ECP and
      // DFT sets, so type must be included in the key!
      key = gbsList[i]->p_name + ":" +
              TGaussianBasisSet::gbs_type_formatter[gbsList[i]->p_type];

      // update the gbs count for removal purposes:
      GBSCountMap::iterator it = p_gbsCount.find (key);
      if (it != p_gbsCount.end()) {
        ((*it).second)++;
      } else {
        p_gbsCount[key] = 1;
      }

      // update the names vector
      bsNames.push_back(key);
    }
  }

  // Add the aggregate name to the aggregate map
  // (we dont need the unique key here because we know these are all
  //  orbital types so they will be unique)
  p_orbitals[bsName] = bsNames;

}

/*******************************************************************
 Method : removeGBS
 Summary: Removes the indicated basis set from the group.  No
          error checking is performed.  

*******************************************************************/
bool TGBSGroup::removeGBS(string gbsType, string gbsName)
{ 
  TGaussianBasisSet::GBSType 
    type = TGaussianBasisSet::strToType(gbsType);
  bool ret = false;
  TGaussianBasisSet* gbs;

  // First check if removing the ECP:
  if (type == TGaussianBasisSet::ecp) {

    if (p_ecp != (TGaussianBasisSet*) 0) {
      delete p_ecp;
      p_ecp = (TGaussianBasisSet*) 0;
      ret = true;
    }
  } else {

    TGBSGroup::iterator it = find(type);
    TGBSGroup::iterator tmpIt;
    if(it != end()) {
      do {
        if (gbsName == "All" || gbsName == (*it).second->p_name) {
          gbs = (*it).second;

          // clean up the maps of contraction meta data
          string uniqueKey = gbs->getUniqueKey();
          removeMetaDataFor(uniqueKey);

          // find next object to point to (after you delete "it", "it"
          // has trouble incrementing correctly to the next object in the
          // map - sometimes it goes backward!) 
          tmpIt = it;
          tmpIt++;

          // remove from map
          erase(it);
          
          // remove from ordered vector:
          removeOrderedGBS(gbs);
          
          // free memory
          if (gbs != 0) {
            delete gbs;
          }      
          ret = true;
          it = tmpIt;

        } else {
          it++;
        }

      } while(it != upper_bound(type));

      // now regenerate the appended tight and diffuse functions
      if (type != TGaussianBasisSet::charge &&
          type != TGaussianBasisSet::exchange &&
          type != TGaussianBasisSet::appended_tight &&
          type != TGaussianBasisSet::appended_diffuse) {
        recomputeTight();
        recomputeDiffuse();
      }
    }
  }
  return ret;
}

/******************************************************************
 Clears out the (uncontract/delete) meta data for that basis set, 
 since it is no longer in the group.
******************************************************************/
void TGBSGroup::removeMetaDataFor(string uniqueGBSKey) {
  ElementToGBSInfoMap::iterator it;

  for (it = p_contractionInfoMap.begin();
       it != p_contractionInfoMap.end(); it++) {
    
    (*it).second.erase(uniqueGBSKey);
  }

}

/******************************************************************
 Erases the basis set from the ordered list of basis sets
******************************************************************/
void TGBSGroup::removeOrderedGBS(TGaussianBasisSet* gbs) {

  vector<TGaussianBasisSet*>::iterator vecIt;
  for (vecIt = p_gbsOrderedVector.begin(); 
       vecIt != p_gbsOrderedVector.end(); vecIt++) {
    if ((*vecIt) == gbs) {
      p_gbsOrderedVector.erase(vecIt);
      break;
    }
  }

}

/*******************************************************************
 Method : removeGBS
 Summary: removes all basis sets of a given type from the group

          returns false if the type does not exist in the group

          Note that removing a GBS completely deletes the GBS object.
*******************************************************************/
bool TGBSGroup::removeGBS(TGaussianBasisSet::GBSType type) 
{
  string typeStr = TGaussianBasisSet::gbs_type_formatter[(int) type];
  return removeGBS(typeStr, "All");
}

/*******************************************************************
 Method : removeOrbitalGBS
 Summary: Removes an orbital gbs (which could include an aggregate
          group) by finding its name in the aggregate map, and
          then removing every basis set associated with that
          aggregate set.
*******************************************************************/
void TGBSGroup::removeOrbitalGBS(string bsName) {

  OrbitalMap::iterator it = p_orbitals.find(bsName);

  // get the aggregate list

  if (it != p_orbitals.end()) {
    
    vector<string> gbsList = (*it).second;
    string gbsName, gbsType;
    int separatorLoc;
    GBSCountMap::iterator countIt;

    for (size_t i = 0; i < gbsList.size(); i++) {
      countIt = p_gbsCount.find(gbsList[i]);

      if (countIt != p_gbsCount.end()) {
        if ((*countIt).second > 1) {
          ((*countIt).second)--;

        } else {
          separatorLoc = gbsList[i].find (":");
          gbsName = gbsList[i].substr(0,separatorLoc);
          gbsType = gbsList[i].substr(separatorLoc+1, string::npos);
          p_gbsCount.erase(countIt);
          removeGBS(gbsType, gbsName);
        }
      }
    }      
   
    p_orbitals.erase(it);
  }
}

/*******************************************************************
 Method : removeAll
 Summary: removes everything - group is empty after this call

          Note that removing a GBS completely deletes the GBS object.
*******************************************************************/
void TGBSGroup::removeAll(void)
{
  p_gbsOrderedVector.clear();

  if (p_ecp != (TGaussianBasisSet*) 0)
  {
    delete p_ecp;
    p_ecp = (TGaussianBasisSet*) 0;
  }
  TGBSGroup::iterator it;
  for (it = begin(); it != end(); it++)
    if((*it).second != (TGaussianBasisSet*) 0)
      delete (*it).second;
 
  clear();

  // erase name
  p_name = "";

  // erase aggregate mappings
  p_orbitals.clear();
  p_gbsCount.clear();

  // erase the meta data
  p_contractionInfoMap.clear();
  p_tightFunctions.clear();
  p_diffuseFunctions.clear();
}

//// ACCESSORS ////

/*******************************************************************
 Method : orbitalGBS
 Summary: Returns a vector of the orbital, or base, GBSs for the group.
          Retuns null if no oribital basis sets are present.
         
          Internal pointers are constant so user can't delete them.
          User must delete the returned vector.

          Note that the orbital gbses are always stored as the first
          elements in the ordered vector of basis sets.

          Note that this is a unique list of real orbital basis sets
          (not a list of aggregate names).
*******************************************************************/
vector <const TGaussianBasisSet*>* TGBSGroup::getOrbitalGBSList(void) const 
{
  vector<const TGaussianBasisSet*>* ret;

  if (p_gbsOrderedVector.size() == 0 )
    ret = 0;
  else if (p_gbsOrderedVector[0]->p_category == TGaussianBasisSet::AUXILIARY)
    ret = 0;
  else {
    ret = new vector<const TGaussianBasisSet*>;
    
    for (size_t i = 0; i < p_gbsOrderedVector.size(); i++) {
      if (p_gbsOrderedVector[i]->p_category == TGaussianBasisSet::AUXILIARY)
        break;
      else
        ret->push_back(p_gbsOrderedVector[i]);
    }
  }
  return ret;
}

/*******************************************************************
 Method : hasContractions
 Summary: Returns true if some contractions are still left in the
 group's basis sets (i.e., they have not all been deleted)
*******************************************************************/
bool TGBSGroup::hasContractions(const string& tag) const
{      
  TGBSGroup::const_iterator groupIt;
  TGBSConfigTags tags(tag.c_str());
  bool ret = false;

  for (size_t i = 0; i < tags.size(); i++) {
    
    for(groupIt = begin(); groupIt != end(); groupIt++) {
 
      ret = ret || hasContractions(tags[i],(*groupIt).second);
    }
  }
  return ret;
}

/*******************************************************************
 Method : hasContractions
 Summary: Returns true if some contractions are still left in the
          specified basis set for the specified tag.
*******************************************************************/
bool TGBSGroup::hasContractions(const string& element,
                                const TGaussianBasisSet* gbs) const
{      
  bool ret = true;

  if (gbs->p_type != TGaussianBasisSet::exchange && 
      gbs->p_type != TGaussianBasisSet::charge &&
      gbs->p_type != TGaussianBasisSet::ecp) {

    ElementToGBSInfoMap::const_iterator elementIt;
    GBSToContInfoMap::const_iterator gbsIt;
    vector<int> from;
    vector<int> to; 
    int functionCount = 0;
    elementIt = p_contractionInfoMap.find(element);
    
    if (elementIt != p_contractionInfoMap.end()) {
      
      gbsIt = (*elementIt).second.find(gbs->getUniqueKey());   
      
      if (gbsIt != (*elementIt).second.end()) { 
        gbs->contractionInfo(element,from,to);
        
        for (size_t j = 0; j < from.size(); j++) {
          
          if ((*gbsIt).second.deleteFlags[j] == false) {
            if ((*gbsIt).second.uncontractFlags[j] == true) {
              functionCount += from[j];
            } else {
              functionCount += to[j];
            }
          }
        }
      }
    }
    ret = functionCount != 0;

  } else {
    ContractionMap::const_iterator contIt;
    contIt = gbs->p_contractions.find(element);
    if (contIt == gbs->p_contractions.end()) {
      ret = false;
    } else {
      ret = true;
    }
  }
  return ret;
}


/*******************************************************************
 Method : contains
 Summary: Returns true if this group already contains the indicated
          basis set.
*******************************************************************/
bool TGBSGroup::contains (string gbsName,
                          TGaussianBasisSet::GBSType gbsType) const
{
  bool ret = false;

  // First check ecp:
  if (gbsType == TGaussianBasisSet::ecp) {
    if (p_ecp != 0 && p_ecp->p_name == gbsName)
      ret = true;
  } else {
    TGBSGroup::const_iterator it = find(gbsType);
    if(it != end()) {
      do {
        if(gbsName == (*it).second->p_name) {
          ret = true;
          break;
        }
        it++;
      } while(it != upper_bound(gbsType));
    }
  }
  return ret;
}

/*******************************************************************
 Method : name
 Summary: dynamically created

          If any of these special features are used:

          1) multiple orbitals
          2) contractions uncontracted or deleted
          3) tight or diffuse functions added
          4) polarization, diffuse, rydberg, charge, exchange,
             or ecp basis sets added that are NOT part of an aggregate 
             group
          5) polarization, diffuse, rydberg, charge, exchange,
             or ecp basis sets removed from an aggregate group
             
          Then the name of the basis set is "Custom".  Otherwise,
          it gets the name of the orbital basis set, plus "..." if
          auxilliary basis sets have been added.
*******************************************************************/
string TGBSGroup::name(void) const
{
  // First find out if any contractions have been deleted or
  // uncontracted:
  ElementToGBSInfoMap::const_iterator elIt;
  GBSToContInfoMap::const_iterator gbsIt;
  const GBSToContInfoMap* gbsInfoMap;
  const ContractionInfo* contInfo;
  bool custom = false;
  size_t i;
  
  // For each element:
  for (elIt =  p_contractionInfoMap.begin(); 
       elIt !=  p_contractionInfoMap.end() && custom == false; elIt++) {
    gbsInfoMap = &(*elIt).second;

    // For each basis set:
    for (gbsIt = gbsInfoMap->begin(); 
         gbsIt != gbsInfoMap->end() && custom == false;
         gbsIt++) {
      contInfo = &(*gbsIt).second;

      // Check the delete and uncontract flags;
      for (i = 0; i < contInfo->deleteFlags.size() && custom == false; i++) {
        custom = custom || contInfo->deleteFlags[i];
        custom = custom || contInfo->uncontractFlags[i]; // should be same size
 
      }
      
    }
  }

  // Now check tight and diffuse functions:
  TightMap::const_iterator tightIt;
  DiffuseMap::const_iterator diffIt;

  for (tightIt = p_tightFunctions.begin();
       tightIt != p_tightFunctions.end() && custom == false; tightIt++) {
    
    for (i = 0; i < (*tightIt).second.size() && custom == false; i++) {

      if ((*tightIt).second[i] > 0) {
         custom = true;
      }
    }
  }

  for (diffIt = p_diffuseFunctions.begin();
       diffIt != p_diffuseFunctions.end() && custom == false; diffIt++) {
    
    for (i = 0; i < (*diffIt).second.size() && custom == false; i++) {

      if ((*diffIt).second[i] > 0) {
         custom = true;
      }
    }
  }
 
  // Now check if extra auxilliary basis sets have been added:
  vector<GBSKey>* addedList = getAuxGBSIdentifiers();
  if (addedList->size() > 0) {
    custom = true;
  }
  delete addedList;

  // Now check if extra auxilliary basis sets have been deleted:
  vector<string>* removedList = getRemovedAuxGBSIdentifiers();
  if (removedList->size() > 0) {
      custom = true;
  }
  delete removedList;

  // 
  
  // Now check multiple orbitals:
  OrbitalMap::const_iterator it = p_orbitals.begin();
  string ret;
  if (it != p_orbitals.end()) {

    ret =  (*it).first;
    
    // If using multiple orbital basis sets, group name is "Custom"
    if(p_orbitals.size() > 1 || custom) {
      ret = "Custom";

    // If auxilliary basis sets have been added, indicate that more
    // that than the orbital exists by adding "..." to the end
    } else if((*it).second.size() < size() ) {
      ret += "...";
    }
  }
  
  return ret;
}

/*******************************************************************
 Method : ecp
 Summary: 
*******************************************************************/
const TGaussianBasisSet* TGBSGroup::ecp(void) const
{
  const TGaussianBasisSet* ret;
  ret = p_ecp;
  return ret;
}

/*******************************************************************
 Method : getOrbitalGBSMappings
 Summary: 
*******************************************************************/
const OrbitalMap* TGBSGroup::getOrbitalGBSMappings(void) const
{
  return &p_orbitals;
}


/*******************************************************************
 Method : getOrderedList
 Summary: Returns an ordered list of all the basis sets in the
          group (except the ecp).

          Note that the orbital GBS is always first, followed by
          the augmenting basis sets in the order in which they were
          inserted.
*******************************************************************/
const vector<TGaussianBasisSet*>* TGBSGroup::getOrderedList(void)
{
  const vector<TGaussianBasisSet*>* ret = &p_gbsOrderedVector;
  return ret;
}


bool TGBSGroup::addTight(const string& tag, const int& shell, const
                         int& numToAdd) {

  TightMap::iterator it = p_tightFunctions.find(tag);
  bool ret = false;
 
  // element should already be in the map
  if (it != p_tightFunctions.end()) {
    (*it).second[shell] = numToAdd;
    ret = recomputeTight(tag, shell, numToAdd);
    if (ret == false) {
      (*it).second[shell] = 0; // add failed, so reset to 0
    }
    
  } else {
    //cout << "cant find tag****" << tag << "***** in map" << endl;
  }
  return ret;
}
      
bool TGBSGroup::addDiffuse(const string& tag, const int& shell, const
                           int& numToAdd) {
  DiffuseMap::iterator it = p_diffuseFunctions.find(tag);
  bool ret = false;

  // element should already be in the map
  if (it != p_diffuseFunctions.end()) {
    (*it).second[shell] = numToAdd;
    ret = recomputeDiffuse(tag, shell, numToAdd);
    if (ret == false) {
      (*it).second[shell] = 0; // add failed, so reset to 0
    }
    
  } else {
    //cout << "cant find tag****" << tag << "***** in map" << endl;
  }
  return ret;
}


/*******************************************************************
 Method : recompute.....
 Summary: These  methods recalculate the appended diffuse/tight
          functions for the config.
*******************************************************************/

void TGBSGroup::recomputeDiffuse() {
  recomputeAppendedFuncs (p_diffuseFunctions, 
                          TGaussianBasisSet::appended_diffuse);
}

bool TGBSGroup::recomputeDiffuse(const string& element_, const int& shell, 
                               const int& numDiffuse) {

  return recomputeAppendedFuncs (element_, shell, numDiffuse, 
                                 p_diffuseFunctions,
                                 TGaussianBasisSet::appended_diffuse);
}

void TGBSGroup::recomputeTight() {
  recomputeAppendedFuncs (p_tightFunctions, 
                          TGaussianBasisSet::appended_tight);
}

bool TGBSGroup::recomputeTight(const string& element_, const int& shell, 
                               const int& numTight) {

  return recomputeAppendedFuncs (element_, shell, numTight, p_tightFunctions,
                                 TGaussianBasisSet::appended_tight);
}

/*******************************************************************
 Method : recomputeAppendedFuncs
 Summary: Generic function that does either diffuse or tight
*******************************************************************/
void TGBSGroup::recomputeAppendedFuncs
( const FunctionMap& map,
  const TGaussianBasisSet::GBSType& type
) {
  // recomputes tight/diffuse functions for all groups

  FunctionMap::const_iterator it;

  for (it = map.begin(); it != map.end(); it++) {
    for (size_t i = 0; i < (*it).second.size(); i++) {
      if ((*it).second[i] > 0) {
        recomputeAppendedFuncs((*it).first, i, (*it).second[i], map, type);
      } 
    }
  }
}


bool TGBSGroup::recomputeAppendedFuncs 
( const string& element_, 
  const int& shell, 
  const int& numToAdd,
  const FunctionMap& map,
  const TGaussianBasisSet::GBSType& type
) {

  // TODO - convert the element_ into a true element_ identifier (the one
  // passed in could be an element_ or atom)

  string gbsName = "appendedFunctionFor:" + element_ +":" +
          TGaussianBasisSet::shell_formatter[shell] + "_shell";
    
  bool ret = true;

  // remove existing functions for this element/shell:
  removeGBS(TGaussianBasisSet::gbs_type_formatter[(int) type], gbsName);
    
  if (numToAdd > 0) {
    // make a new basis set
    TGaussianBasisSet* gbs;
    gbs = new TGaussianBasisSet; // what other info do i need to know?
    gbs->p_name = gbsName;
    gbs->p_type = type;
    gbs->p_category = TGaussianBasisSet::AUXILIARY;
    
    // make a new contraction set
    ContractionSet* contractions = new ContractionSet;
    Contraction_* cont;
    gbs->p_contractions[element_] = contractions;
    
    // find the default ratio to use    
    double endExponent;
    double ratio;
    if (type == TGaussianBasisSet::appended_tight) {
      computeRatio(element_, shell, endExponent, ratio, "upper");
    } else {
      computeRatio(element_, shell, endExponent, ratio, "lower");
    }
    // now add the number of tight functions to the config
    if (ratio > 0) {
      char sDouble[20];
      float tmpNum;

      for (int i = 0; i < numToAdd; i++) {
        cont = new Contraction_;
        cont->num_exponents = 1;
        cont->num_coefficients = 1;
        cont->shells.push_back((TGaussianBasisSet::AngularMomentum) shell);
        
        cont->exponents = new double[1];
        cont->coefficients = new double[1];
        endExponent = endExponent * ratio;
        sprintf(sDouble,"%4.3e",endExponent);
        
        sscanf(sDouble, "%f", &tmpNum);
        ostrstream os;
        os << tmpNum << ends;
        char* tmpStr = os.str();
        istrstream is(tmpStr);    
        is >> cont->exponents[0];
        delete tmpStr;
        cont->coefficients[0] = 1.0;
        
        contractions->push_back(cont);
      }
      // have to put this down here because of order of ops when inserted
      insertGBS(gbs); 
    } else {
      ret = false;
    }
  } else {
    ret = false;
  }
  return ret;
}

/*******************************************************************
 Method : computeRatio
 Summary: This function determines what the maximum exponent is
          for the given group, element_, and shell, and also what
          the default ratio is between any 2 consecutive exponents.

          Ratio is set to 0 if the tight function cannot be
          determined.  This occurs if the group currently has NO 
          contractions for the indicated shell, or if calculating
          a TIGHT function, and only 1 exponent is available for
          that shell.

          Note that DELETED contractions are not considered when
          computing the ratio.
*******************************************************************/
void TGBSGroup::computeRatio(const string& element_, const int& shell,
                             double& endExponent, double& ratio,
                             const string& bound) {
  TGBSGroup::iterator gIt;
  ContractionMap::iterator mIt;
  TGaussianBasisSet* gbs;
  ContractionSet* contractions;
  Contraction_* cont;
  ratio = 0;
  double secondToLast;
  double tmp;
  double startValue;

  if (bound == "upper") {
    startValue = 0.0;
  } else if (bound == "lower") {
    startValue = 20000.0;
  }
  endExponent = startValue;
  secondToLast = startValue;

  ElementToGBSInfoMap::iterator elIt = p_contractionInfoMap.find(element_);
  GBSToContInfoMap::iterator gbsIt;
  ContractionInfo* contInfo;
  bool deleteFlag;

  for (gIt = begin(); gIt != end(); gIt++) {
    gbs = (*gIt).second;
    
    if (gbs != 0 && gbs->p_type != TGaussianBasisSet::exchange &&
        gbs->p_type != TGaussianBasisSet::charge &&
        gbs->p_type != TGaussianBasisSet::appended_tight &&
        gbs->p_type != TGaussianBasisSet::appended_diffuse) {

      // find the contraction info for that element/gbs combination:
      contInfo = 0;
      if(elIt != p_contractionInfoMap.end()) {
        gbsIt = (*elIt).second.find(gbs->getUniqueKey());
        if (gbsIt != (*elIt).second.end()) {
          contInfo = &(*gbsIt).second;
        }
      }

      // find the contraction set for the element_
      mIt = gbs->p_contractions.find(element_);
      if (mIt != gbs->p_contractions.end()) {
        contractions = (*mIt).second;
        
        // for every contraction in the set
        for (size_t i = 0; i < contractions->size(); i++) {
          cont = (*contractions)[i];
          
          // See if this contraction is for the given shell
          for (size_t j = 0; j < cont->shells.size(); j++) {
      
            // if so, then use it to determine the maxExponent and ratio
            deleteFlag = contInfo == 0 || 
              contInfo->deleteFlags[cont->shells[j]] == true;
            if ((int) cont->shells[j] == shell && !deleteFlag) {

              for (size_t e = 0; e < cont->num_exponents; e++) {
                tmp = cont->exponents[e];
                
                if (bound == "upper") {

                  if (tmp > secondToLast && tmp < endExponent) {
                    secondToLast = tmp;
                    
                  } else if (tmp > endExponent) {
                    secondToLast = endExponent;
                    endExponent = tmp;
                  }
                } else if (bound == "lower") {
                  
                  if (tmp < secondToLast && tmp > endExponent) {
                    secondToLast = tmp;
                    
                  } else if (tmp < endExponent) {
                    secondToLast = endExponent;
                    endExponent = tmp;
                  }
                }
              }
              break;
            }
          }
        }
      }
    }
  }
  // now compute the ratio
  if (endExponent != startValue) { // if at least 1 exponent for this shell
    
    // if calculating a tight function and only 1 exponent available
    if (bound == "upper" && secondToLast == startValue) { 
      ratio = 0.0; // this is an error, so no tight function should
                   // be appended
      string errMsg = "only 1 exponent is available for the ";
      errMsg += TGaussianBasisSet::shell_formatter[shell]; 
      errMsg += " shell.";
      ErrMsg().message("EE_GBS_CANT_ADD_TIGHT",
                     EE_FATAL, WHERE, errMsg.c_str());   
 
    // if calculating a diffuse function and only 1 exponent available
    } else if (bound == "lower" && secondToLast == startValue) {
      ratio = getDiffuseSingleExponentRatio(element_, shell, endExponent);
      
    } else {
      ratio = endExponent / secondToLast;
    }   
  }

}

double TGBSGroup::getDiffuseSingleExponentRatio(const string& element_,
                                                const int& shell,
                                                const double& shellExponent)
{
  int previousShell1 = shell - 1;
  int previousShell2 = shell - 2;
  double ratio = 0.0;

  if (previousShell1 >= 0 && previousShell2 >= 0) {
   
    double dif_exp_shell1;
    double dif_exp_shell2;
    double dif_exp_shell;
    double tmpEndExponent;
    double tmpRatio;

    // first calculate diffuse exponent for previous shell 1
    computeRatio(element_, previousShell1, tmpEndExponent, tmpRatio, "lower");
    if (tmpRatio != 0.0) {
      dif_exp_shell1 = tmpEndExponent * tmpRatio;

      // then calculate diffuse exponent for previous shell 2
      computeRatio(element_, previousShell2, tmpEndExponent, tmpRatio, 
                   "lower");
      if (tmpRatio != 0.0) {
        dif_exp_shell2 = tmpEndExponent * tmpRatio;
        
        // then calculate the diffuse exponent for the indicated shell
        dif_exp_shell = dif_exp_shell1 * dif_exp_shell1 / dif_exp_shell2;
        
        // now calculate the ratio
        ratio = dif_exp_shell / shellExponent;
      } else {
        string errMsg = " all the contractions have been deleted from ";
        errMsg += TGaussianBasisSet::shell_formatter[previousShell2];
        ErrMsg().message("EE_GBS_CANT_ADD_DIFFUSE",
                         EE_FATAL, WHERE, errMsg.c_str());   
      }
    } else {
      string errMsg = " all the contractions have been deleted from ";
      errMsg += TGaussianBasisSet::shell_formatter[previousShell1];
      ErrMsg().message("EE_GBS_CANT_ADD_DIFFUSE",
                       EE_FATAL, WHERE, errMsg.c_str());   
    }
  } else {
    string errMsg = " L value of ";
    errMsg += TGaussianBasisSet::shell_formatter[shell];
    errMsg += " is too low.";
    ErrMsg().message("EE_GBS_CANT_ADD_DIFFUSE",
                     EE_FATAL, WHERE, errMsg.c_str());   
  }    
  return ratio;
}

//########################################################################

ContractionInfo::ContractionInfo() {
  resetVectors(false, false);
 
}

ContractionInfo::ContractionInfo(const ContractionInfo& c) {

  int vectorSize = TGaussianBasisSet::maxShell + 1;

  for(int i = 0; i < vectorSize; i++) {
    deleteFlags.push_back(c.deleteFlags[i]);
    uncontractFlags.push_back(c.uncontractFlags[i]);
  }

}
 
ContractionInfo::ContractionInfo(const bool& deleteValue, 
                                 const bool& uncontractValue) {
  resetVectors(deleteValue, uncontractValue);
}

bool ContractionInfo::operator==(const ContractionInfo& contInfo) const {
  return (deleteFlags == contInfo.deleteFlags &&
          uncontractFlags == contInfo.uncontractFlags); 
}


void ContractionInfo::resetVectors(const bool& deleteValue,
                              const bool& uncontractValue) {
  
  deleteFlags.clear();
  uncontractFlags.clear();

  int vectorSize = TGaussianBasisSet::maxShell + 1;
  deleteFlags.resize(vectorSize, deleteValue);
  uncontractFlags.resize(vectorSize, uncontractValue);

}


