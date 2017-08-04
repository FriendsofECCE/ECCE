/////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: DavCalculation.C
//
//
// DESIGN:
//
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h> // sprintf
#include <strstream>
  using std::istrstream;
#include <fstream>
  using std::ifstream;
#include <iostream>
  using std::cout;
  using std::endl;
  using std::ends;
  using std::ios;

#include <string>
   using std::string;
#include <vector>
   using std::vector;
   using std::find;

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMNamedNodeMap.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
using namespace xercesc;

#include "util/DavException.H"
#include "util/Ecce.H"
#include "util/EcceMap.H"
#include "util/ErrMsg.H"
#include "util/InvalidException.H"
#include "util/SFile.H"
#include "util/StringConverter.H"

#include "tdat/Fragment.H"
#include "tdat/GUIValues.H"
#include "tdat/TTheory.H"
#include "tdat/TRunType.H"

#include "dsm/BasicDOMParser.H"
#include "dsm/DavCalculation.H"
#include "dsm/EDSIGaussianBasisSetLibrary.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/ICalcUtils.H"
#include "dsm/JCode.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/Session.H"
#include "dsm/VDoc.H"
#include "dsm/TGBSConfig.H"


// Debug:
//#define DEBUG

/**
 * Protected because only EDSIFactory can create a calculation 
 */
DavCalculation::DavCalculation(const EcceURL& eurl) throw(InvalidException)  {
  p_url = eurl;
  p_edsi = 0;
  p_vdoc = 0;
}



/**
 * Big data member so returns a pointer to the gbsConfig.
 * User is responsible for deleting when finished.
 * Returns null on error.
 */
TGBSConfig* DavCalculation::gbsConfig(const string& theoryType) const
{
  TGBSConfig* config = new TGBSConfig; 

  // Set url to gbs file:
  EcceURL url; 
  if (DavCalculation::stringToTheoryType(theoryType) ==
        DavCalculation::GENERAL) {
    url = getVDoc()->getConfiguration(getVDoc()->getGeneralBasisSetName());
  }
  else if (DavCalculation::stringToTheoryType(theoryType) ==
             DavCalculation::SINGLE) {
    url = getVDoc()->getConfiguration(getVDoc()->getSinglePointBasisSetName());
  }
  else {
    url = getVDoc()->getConfiguration(getVDoc()->getBasisSetName());
  }
  getEDSI()->setURL(url);

  // Get config meta data:
  if (!getConfigMetaData(config)) {
    delete config;
    return (TGBSConfig*)0;
  } 

  // Clear message stack
  p_msgStack->clear();

  // Read the gbs data file from the dav server:
  ostrstream input;

  if (!getEDSI()->getDataSet(input) ||
      getEDSI()->m_msgStack.getMessage().find("404") != string::npos) {
    // TODO clean up this error handling.
    delete config;
    return (TGBSConfig*)0;
  }
  input << ends; // terminate the stream

  // Create a DOM document from the input stream
  const DOMDocument *doc = getXMLDocument(input.str());

#ifdef DEBUG
cout << input.str() << endl;
#endif
  // Cache of unique basis sets:
  map<string, TGaussianBasisSet*, less<string> > gbsCache;

  // Parse the Data element from the DOM document:
  parseConfigDataElement(*doc, config, gbsCache);
  
  // Parse the Map element from the DOM document:
  parseConfigMapElement(*doc, config, gbsCache);

  if (doc != 0) ((DOMDocument *) doc)->release();
  return config; 
}


/**
 * Returns null if the property does not exist or if there is
 * a dav error reading the URL.
 */
bool DavCalculation::gbsUseExpCoeff(void) const 
{
  return getProp(VDoc::getEcceNamespace() + ":useExpCoeff") == "true";
}


/**
 * Returns empty map if property does not exist or if there
 * is a dav error reading the URL.
 */
GUIValues *DavCalculation::guiparams(const string& theoryType) const {

  GUIValues *guiparams = new GUIValues();
  
  // Set url to params file:
  EcceURL paramsUrl; 
  if (DavCalculation::stringToTheoryType(theoryType) ==
        DavCalculation::GENERAL) {
    paramsUrl = getVDoc()->getConfiguration(
                           getVDoc()->getGeneralSetupParametersName());
  }
  else if (DavCalculation::stringToTheoryType(theoryType) ==
             DavCalculation::SINGLE) {
    paramsUrl = getVDoc()->getConfiguration(
                           getVDoc()->getSinglePointSetupParametersName());
  }
  else {
    paramsUrl = getVDoc()->getConfiguration(
                           getVDoc()->getSetupParametersName());
  }
  getEDSI()->setURL(paramsUrl);

  p_msgStack->clear();

  ostrstream ostr;
  if (getEDSI()->getDataSet(ostr)) {
    // we are responsible for adding end-of-string
    ostr << ends;
    char* tmp = ostr.str();
    *guiparams = tmp;
    delete tmp;
  } else {
    getEDSI()->setURL(getVDoc()->getBaseURL());

    vector<MetaDataRequest> requests(1);
    vector<MetaDataResult> results;
    requests[0].name = VDoc::getEcceNamespace() + ":guiparams";

    if (getEDSI()->getMetaData(requests, results)) {
      for (size_t i = 0; i < results.size(); i++)
        if (results[i].name == VDoc::getEcceNamespace() + ":guiparams")
          *guiparams = results[0].value;
    } else
      p_msgStack->add(getEDSI()->m_msgStack);
  }

  return guiparams;
}


void DavCalculation::parseConfigMapElement(
	       const DOMDocument& doc,
	       TGBSConfig* config,
	       map<string, TGaussianBasisSet*, less<string> >& gbsCache )
               const
{
  TGBSGroup* group = 0;
  string groupName;

  // to identify sharing of basis sets between groups
  map<string, TGaussianBasisSet*, less<string> > usedBasisSets;

  // Loop through all the groups in the map, creating groups from basis sets
  // in the cache:
  XMLCh *xStr = XMLString::transcode("Group");
  DOMNodeList *groupList = doc.getElementsByTagName(xStr);
  delete [] xStr;

  for (size_t groupIt = 0; groupIt < groupList->getLength(); groupIt++) {
    // Get group from XML document:
    DOMNamedNodeMap
      *groupAttributes = groupList->item(groupIt)->getAttributes();    

    // assign group name:
    xStr = XMLString::transcode("Name");
    char *tempStr =
        XMLString::transcode(groupAttributes->getNamedItem(xStr)->getNodeValue());
    groupName = tempStr;
    delete [] xStr;
    delete [] tempStr;

    group = new TGBSGroup;
    // group->name(groupName); we dont care about name anymore

    // Parse the group (group has 2 parts: ordered list and orbital mapping
    parseGroupGBSOrderedList(*(groupList->item(groupIt)), group, gbsCache,
			     usedBasisSets);
    parseGroupOrbitalMap(*(groupList->item(groupIt)), group, groupName);
    parseGroupContractionInfoMap(*(groupList->item(groupIt)), group);
    parseGroupTightMap(*(groupList->item(groupIt)), group);
    parseGroupDiffuseMap(*(groupList->item(groupIt)), group);

    // get group tag:
    xStr = XMLString::transcode("Tag");
    tempStr =
        XMLString::transcode(groupAttributes->getNamedItem(xStr)->getNodeValue());
    string tag = tempStr;
    delete [] xStr;
    delete [] tempStr;
    
    // add group to config:
#ifdef DEBUG
    cout << "inserting " << group->name() << " for tag " << tag << endl;
#endif
    config->insertGBSGroup(tag, group);
  }
}


void DavCalculation::parseGroupTightMap(const DOMNode& groupNode, 
					TGBSGroup* group) const 
{
  // Get current version:
  string currentversion = getProp(VDoc::getEcceNamespace() + ":currentVersion");

  if (currentversion != "ecce version 2.0") // this is the old format
    parseGroupFunctionMap(groupNode, group->getTightMap(), "TightMap");
}


void DavCalculation::parseGroupDiffuseMap(const DOMNode& groupNode, 
					  TGBSGroup* group) const 
{
  // Get current version:
  string currentversion = getProp(VDoc::getEcceNamespace() + ":currentVersion");

  if (currentversion != "ecce version 2.0") // this is the old format
    parseGroupFunctionMap(groupNode, group->getDiffuseMap(), "DiffuseMap");
}


void DavCalculation::parseGroupFunctionMap(const DOMNode& groupNode,
					   FunctionMap& fMap,
					   const string& nodeID) const
{
  
  DOMNodeList *childNodes = groupNode.getChildNodes();
  DOMNodeList *elementList = 0;
  bool elementFound = false;

  // First find the right node
  size_t childIt;
  string nodeName;
 
  for (childIt = 0; childIt < childNodes->getLength(); childIt++) {
    char *tempStr =
        XMLString::transcode(childNodes->item(childIt)->getNodeName());
    nodeName = tempStr;
    delete [] tempStr;

    if (nodeName == nodeID) {
      elementList = childNodes->item(childIt)->getChildNodes();
      elementFound = true;
      break;  
    }
  }	
  if (!elementFound)
    return;

  size_t elementIt;
  string elementName;
  string dataString;

  for (elementIt = 0; elementIt < elementList->getLength(); elementIt++) {
      DOMNode *element = elementList->item(elementIt);  
      char *tempStr = XMLString::transcode(element->getNodeName());
      nodeName = tempStr;
      delete [] tempStr;
      if (nodeName != "Element")
        continue;
 
      DOMNamedNodeMap *elAttributes = element->getAttributes();
      XMLCh *xStr = XMLString::transcode("tag");
      tempStr = 
          XMLString::transcode(elAttributes->getNamedItem(xStr)->getNodeValue());
      elementName = tempStr;
      delete [] xStr;
      delete [] tempStr;

      tempStr = 
          XMLString::transcode(element->getFirstChild()->getNodeValue());
      dataString = tempStr;
      delete [] tempStr;
      vector<int> vec(TGaussianBasisSet::maxShell + 1, 0);
      TGBSConfigTags values(dataString.c_str(), false, false);

      if (values.size() != vec.size())
        EE_ASSERT(false, EE_WARNING,"error - shell list is not the right size");

      for (size_t i = 0; i < values.size(); i++)
        sscanf(values[i].c_str(), "%d", &vec[i]);

      fMap[elementName] = vec;
  }
}


void DavCalculation::parseGroupContractionInfoMap(const DOMNode& groupNode, 
						  TGBSGroup* group) const
{  
  // Get current version:
  string currentversion = getProp(VDoc::getEcceNamespace() + ":currentVersion");

  if (currentversion == "ecce version 2.0") // this is the old format
    return;

  ElementToGBSInfoMap& elementMap = group->getContractionInfoMap();
  DOMNodeList *childNodes = groupNode.getChildNodes();
  DOMNodeList *elementList = 0;
  bool elementFound = false;

  // First find the right node
  size_t childIt;
  string nodeName;
    
  for (childIt = 0; childIt < childNodes->getLength(); childIt++) {
    char *tempStr = 
        XMLString::transcode(childNodes->item(childIt)->getNodeName());
    nodeName = tempStr;
    delete [] tempStr;

    if (nodeName == "ContractionInfoMap") {
      elementList = childNodes->item(childIt)->getChildNodes();
      elementFound = true;
      break;  
    }
  }

  if (!elementFound)
    return;

  size_t elementIt;
  size_t bsIt;
  string elementName;
  string uniqueKey;
  string dataString;

  for (elementIt = 0; elementIt < elementList->getLength(); elementIt++) {
    DOMNode *element = elementList->item(elementIt);  
    char *tempStr = XMLString::transcode(element->getNodeName());
    nodeName = tempStr;
    delete [] tempStr;

    // only evaluate recognized children
    if (nodeName != "Element")
      continue;

    GBSToContInfoMap newMap;

    DOMNodeList *bsList = element->getChildNodes();
    DOMNamedNodeMap *elAttributes = element->getAttributes();
    XMLCh *xStr = XMLString::transcode("tag");
    tempStr = 
        XMLString::transcode(elAttributes->getNamedItem(xStr)->getNodeValue());
    elementName = tempStr;
    delete [] xStr;
    delete [] tempStr;

    for (bsIt = 0; bsIt < bsList->getLength(); bsIt++) {
      DOMNode *gbs = bsList->item(bsIt);
      DOMNamedNodeMap *gbsAttributes = gbs->getAttributes();	
      tempStr = XMLString::transcode(gbs->getNodeName());
      nodeName = tempStr;
      delete [] tempStr;

      if (nodeName != "BasisSet")
        continue;

      ContractionInfo contInfo;
      xStr = XMLString::transcode("uniqueKey");
      tempStr = 
          XMLString::transcode(gbsAttributes->getNamedItem(xStr)->getNodeValue());
      uniqueKey = tempStr; 
      delete [] xStr;
      delete [] tempStr;
#ifdef DEBUG
cout << "parsing basis set " << uniqueKey << endl;
#endif
      DOMNodeList *flagsList = gbs->getChildNodes();
      for (size_t flagIt = 0; flagIt < flagsList->getLength(); flagIt++) {
        DOMNode *flag = flagsList->item(flagIt);
        tempStr = XMLString::transcode(flag->getNodeName());
        nodeName = tempStr;
        delete [] tempStr;

        if (nodeName == "DeleteFlags") {
          tempStr = XMLString::transcode(flag->getFirstChild()->getNodeValue());
          dataString = tempStr;
          delete [] tempStr;

          // use this class as a parser
          TGBSConfigTags values(dataString.c_str()); 
          for (size_t i = 0; i < values.size(); i++) {
#ifdef DEBUG
cout << "parsing value " << values[i] << endl;
#endif
            contInfo.deleteFlags[i] = values[i] == "true";
          }
        } else if (nodeName == "UncontractFlags") {
          tempStr = XMLString::transcode(flag->getFirstChild()->getNodeValue());
          dataString = tempStr;
          delete [] tempStr;

          // use this class as a parser
          TGBSConfigTags values(dataString.c_str());
          for (size_t i = 0; i < values.size(); i++) {
#ifdef DEBUG
cout << "parsing value " << values[i] << endl;
#endif
              contInfo.uncontractFlags[i] = values[i] == "true";
          }	  	       
        }
      }

      newMap[uniqueKey] = contInfo;
    }
    elementMap[elementName] = newMap;
  }
}


void DavCalculation::parseGroupOrbitalMap(const DOMNode& groupNode, 
					  TGBSGroup* group, string groupName) const
{   
  OrbitalMap orbitalMap;
  GBSCountMap countMap; 

  // Get current version:
  string currentversion = getProp(VDoc::getEcceNamespace() + ":currentVersion");

  if (currentversion == "ecce version 2.0") { // this is the old way

    // The version 2.0 data doesn't include the orbital map component
    // (this is the info that maps an orbital basis set with all the
    // basis sets that it includes (e.g., STO-3G* gets STO-3G and
    // STO-3G Polarization) )
    string orbitalName = groupName;
    string key;
    vector<string> bsNames;
    
    // Lookup mapping info from the library (unless it's Imported that is)!
    // This mapping info will help us create the orbital map, so it
    // can be converted to the new 2.1 format.
    EDSIServerCentral central;
    EDSIGaussianBasisSetLibrary lib(central.getDefaultBasisSetLibrary());
    vector<TGaussianBasisSet*> gbsList;

    if (orbitalName != "Imported" && 
        orbitalName != "imported")
      gbsList= lib.lookup(orbitalName.c_str());

    if (gbsList.size() != 0) {
      for (size_t i = 0; i < gbsList.size(); i++) {
        key = gbsList[i]->p_name + ":" +
              TGaussianBasisSet::gbs_type_formatter[gbsList[i]->p_type];
        bsNames.push_back(key);
        countMap[key] = 1; // keeps track of shared basis sets
      }
    } else { // don't know where this basis set came from, probably imported
             // so assume ALL the basis sets in the group are part of the 
             // orbital gbs
      DOMNodeList *orderedBSList = groupNode.getChildNodes();
      size_t bsIt;
      string nodeName;

      for (bsIt = 0; bsIt < orderedBSList->getLength(); bsIt++) {    
        char *tempStr = 
            XMLString::transcode(orderedBSList->item(bsIt)->getNodeName());
        nodeName = tempStr;
        delete [] tempStr;
      
        // only evaluate children with ecce data
        // (other meta data nodes are present in the list)
        if (nodeName != "BasisSet" && nodeName != "ECP")
          continue;
  
        DOMNamedNodeMap *gbsAttributes=orderedBSList->item(bsIt)->getAttributes();
    
        // key must concatenate gbs name with gbs type to provide unique value
        XMLCh *xStr = XMLString::transcode("Type");
        tempStr = 
            XMLString::transcode(gbsAttributes->getNamedItem(xStr)->getNodeValue());
        key = orbitalName;
        key += ":";
        key += tempStr;
        delete [] xStr;
        delete [] tempStr;

        bsNames.push_back(key);
        countMap[key] = 1;
      }
    }
    orbitalMap[orbitalName] = bsNames;
    
  } else { // this is the new way

    // First get the list of orbital basis sets
    DOMNodeList *orbitalList = 0;
    DOMNodeList *childNodes = groupNode.getChildNodes();
    size_t childIt;
    string nodeName;
    
    for (childIt = 0; childIt < childNodes->getLength(); childIt++) {
      char *tempStr = 
          XMLString::transcode(childNodes->item(childIt)->getNodeName());
      nodeName = tempStr;
      delete [] tempStr;

      if (nodeName == "OrbitalMapping") {
        orbitalList = childNodes->item(childIt)->getChildNodes();
        break;  
      }
    }

    size_t orbitalIt;
    size_t bsIt;
    string orbitalName;
    string key;

    for (orbitalIt = 0; orbitalIt < orbitalList->getLength(); orbitalIt++) {
      vector<string> bsNames;

      DOMNode *orbitalGBS = orbitalList->item(orbitalIt);
      DOMNodeList *bsList = orbitalGBS->getChildNodes();
      
      char *tempStr = 
          XMLString::transcode(orbitalGBS->getNodeName());
      nodeName = tempStr;
      delete [] tempStr;

      // only evaluate recognized children
      if (nodeName != "OrbitalBasisSet")
        continue;

      DOMNamedNodeMap *orbAttributes = orbitalGBS->getAttributes();
      XMLCh *xStr = XMLString::transcode("Name");
      tempStr = 
          XMLString::transcode(orbAttributes->getNamedItem(xStr)->getNodeValue());
      orbitalName = tempStr;
      delete [] xStr;
      delete [] tempStr;

      for (bsIt = 0; bsIt < bsList->getLength(); bsIt++) {
        DOMNode *gbs = bsList->item(bsIt);
        DOMNamedNodeMap *gbsAttributes = gbs->getAttributes();
	
        tempStr = 
            XMLString::transcode(gbs->getNodeName());
        nodeName = tempStr;
        delete [] tempStr;
        if (nodeName != "BasisSet" && nodeName != "ECP")
          continue;

        // find the key for each basis set in the list:    
        // (key must concatenate gbs name with gbs type to provide unique 
        // value)
        tempStr = XMLString::transcode(gbs->getFirstChild()->getNodeValue());
        key = tempStr;
        delete [] tempStr;
        key += ":";
        xStr = XMLString::transcode("Type");
        tempStr = 
          XMLString::transcode(gbsAttributes->getNamedItem(xStr)->getNodeValue());
        key += tempStr;
        delete [] xStr;
        delete [] tempStr;

        // Add gbs to the vector of basis set keys
        bsNames.push_back(key);

        // update the gbs count for removal purposes:
        GBSCountMap::iterator it = countMap.find (key);
        if (it != countMap.end())
          ((*it).second)++;
        else
          countMap[key] = 1;
      }
      
      // add bsNames to orbital map
      orbitalMap[orbitalName] = bsNames;
    }	    
  }

  group->setOrbitalGBSMappings(orbitalMap);
  group->setGBSCount(countMap);
}


void DavCalculation::parseGroupGBSOrderedList(const DOMNode& groupNode,
     TGBSGroup* group, map<string, TGaussianBasisSet*, less<string> >& gbsCache,
     map<string, TGaussianBasisSet*, less<string> >& usedBasisSets) const
{
  // Get current version:
  string currentversion = getProp(VDoc::getEcceNamespace() + ":currentVersion");

  size_t bsIt;
  DOMNodeList *orderedBSList = 0;

  // First determine whether we are using an old or new schema, and
  // find the right list of elements:
  if (currentversion == "ecce version 2.0")
    orderedBSList = groupNode.getChildNodes();
  else {
    DOMNodeList *childNodes = groupNode.getChildNodes();
    size_t childIt;
    string tmpNodeName;
    
    for (childIt = 0; childIt < childNodes->getLength(); childIt++) {
      char *tempStr = 
          XMLString::transcode(childNodes->item(childIt)->getNodeName());
      tmpNodeName = tempStr;
      delete [] tempStr;
      
      if (tmpNodeName == "OrderedList") {
        orderedBSList = childNodes->item(childIt)->getChildNodes();
        break;  
      }
    }
  }
  
  // Now create the ordered list (i.e., unique set) of real basis sets:
  string key;
  TGaussianBasisSet* gbs;
  string nodeName;

  for (bsIt = 0; bsIt < orderedBSList->getLength(); bsIt++) {    
    gbs = 0;
    char *tempStr = 
        XMLString::transcode(orderedBSList->item(bsIt)->getNodeName());
    nodeName = tempStr;
    delete [] tempStr;
      
    // only evaluate children with ecce data
    // (other meta data nodes are present in the list)
    if (nodeName != "BasisSet" && nodeName != "ECP")
      continue;
      
    // contents of the node are actually saved as the first child of that
    // node
    DOMNamedNodeMap *gbsAttributes= orderedBSList->item(bsIt)->getAttributes();
    
    // key must concatenate gbs name with gbs type to provide unique 
    // value
    tempStr = 
        XMLString::transcode(orderedBSList->item(bsIt)->getFirstChild()->getNodeValue());
    key = tempStr;
    delete [] tempStr;
    key += ":";
    XMLCh *xStr = XMLString::transcode("Type");
    tempStr = 
        XMLString::transcode(gbsAttributes->getNamedItem(xStr)->getNodeValue());
    key += tempStr;
    delete [] xStr;
    delete [] tempStr;
    
    map<string, TGaussianBasisSet*, less<string> >::iterator
      cacheIt = gbsCache.find(key);
    
    if (cacheIt != gbsCache.end()){
      gbs = (*cacheIt).second;
	
      // check to see if this gbs has been inserted to a group already
      // if so, then make a copy of it so no sharing of pointers occurs
      if (usedBasisSets.find(key) != usedBasisSets.end())
        gbs = new TGaussianBasisSet(*gbs); // if used, create a copy
      else
        usedBasisSets[key] = gbs; // add it to used list
      
#ifdef DEBUG
      cout << "inserting " << key << " into group" << endl;
#endif
      group->insertGBS(gbs); 
      // note that group is smart enough to determine the type of gbs
      // and insert an ecp into its special place
    } else {
      EE_RT_ASSERT(false, EE_WARNING, "Basis set name from TGBSConfig map "
                   "not found in basis set data section.");
    }
  }
}


void DavCalculation::parseConfigDataElement( const DOMDocument& doc,
                TGBSConfig* config,
                map<string, TGaussianBasisSet*, less<string> >& gbsCache) const
{
  string dataString = getXMLElementValue(doc,"Data");
  char* newString = new char[dataString.size() + 1];
  strcpy(newString, dataString.c_str());
  istrstream dataStream(newString);    
       
  char line[256]; 
  char typeStr[50];
  char category[50];
  char contType[50];
  char coordSys[50];
  TGaussianBasisSet* gbs = 0;

  string key; // used to identify unique basis sets in the master list

  while (dataStream.getline(line,255)) {
    if (strlen(line) == 0) // ignore empty lines:
      continue;

    if (strncmp(line,"BasisSet=",9) == 0) {
      gbs = new TGaussianBasisSet;
      gbs->p_name = &line[9];
    } else {
      sscanf(line,"Type=%s Category=%s Spherical=%s ContType=%s", 
	     typeStr, category, coordSys, contType);

      gbs->p_type = TGaussianBasisSet::strToType(typeStr);
      gbs->p_coordSys = TGaussianBasisSet::strToCoordSys(coordSys);
      gbs->p_category = TGaussianBasisSet::strToCategory(category);
      gbs->p_contType = TGaussianBasisSet::strToContType(contType);

      if (gbs->p_type == TGaussianBasisSet::ecp)
        parseEcpData(gbs, &dataStream);
      else
        parseGbsData(gbs, &dataStream);
 
      // Names are not unique for some of the aggregate ECP and
      // DFT sets, so type must be included in the key!
      key = gbs->p_name + ":" +
              TGaussianBasisSet::gbs_type_formatter[gbs->p_type];

      gbsCache[key] = gbs; // insert into cache
    }
  }
}



bool DavCalculation::getConfigMetaData(TGBSConfig* config) const
{
  // Read THE TGBSCONFIG META DATA:
  vector<MetaDataRequest> requests(4);
  vector<MetaDataResult> results;
  string ns = VDoc::getEcceNamespace() + ":";  
  requests[0].name = ns + "name";
  requests[1].name = ns + "optimize";
  requests[2].name = ns + "coordsys";
  requests[3].name = ns + "gbs-type";

  if (getEDSI()->getMetaData(requests, results)) {
    for (size_t i = 0; i < results.size(); i++) {
      if (results[i].name == ns + "name")
        config->name(results[i].value);
      else if (results[i].name == ns + "optimize")
        config->optimize(results[i].value == "true");
      else if (results[i].name == ns + "coordsys")
        config->coordsys(TGaussianBasisSet::strToCoordSys(results[i].value));
      else if (results[i].name == ns + "gbs-type")
        config->type(TGBSConfig::strToConfigType(results[i].value));
    }
  } else
    p_msgStack->add(getEDSI()->m_msgStack);

  return true;
}


void DavCalculation::parseEcpData( TGaussianBasisSet* ecp,
				   istream* is ) const
{
  ContractionSet* potential=0;
  char element_name[10] = "";
  char line[500];
  while(!is->eof()) {
    is->getline(line,500);

    if(strncmp(line,"element=",8) == 0) { // new element
      // add the previous contraction set to the map except for 1st element:
      if (strcmp(element_name, "") != 0) {
#ifdef DEBUG
        cout << "inserting " << element_name 
             << " into ecp " << ecp->p_name << endl;
#endif
        ecp->p_contractions[element_name] = potential;
      }
      potential = new ContractionSet; // create new contraction set:
      sscanf(line,"element=%s ncore=%d lmax=%d",
                   element_name,&potential->ncore,&potential->lmax);
    } else if(strcmp(line, "EndBasisSet") == 0) { // end of basis set
      // add last contractions set to the map
#ifdef DEBUG
      cout << "inserting " << element_name 
           << " into ecp " << ecp->p_name << endl;
#endif
      ecp->p_contractions[element_name] = potential;
      break;
    } else if (strncmp(line,"ecp_potential%",14) == 0) {  // new contraction
                                 // in same set, with additional ecp data
      Contraction_* ecpp = new Contraction_;

      char* tok = strtok(&line[14],"%");
      do {
        if( !strncmp(tok,"l=",2) )
          sscanf(&tok[2],"%zu",&ecpp->l);

        else if( !strncmp(tok,"shell=",6) ) {
          // shells tok looks like this: "S-D POTENTIAL"
          string shellString = (&tok[6]);
          size_t potentialLocation = shellString.find("POTENTIAL");
          if (potentialLocation == string::npos)
            potentialLocation = shellString.find("potential");
          shellString = shellString.substr( 0, potentialLocation-1 );

          for (size_t i = 0; i < shellString.size(); i++)
          if (shellString[i] != '-' && shellString[i] != ' ')
            ecpp->shells.push_back(
                  TGaussianBasisSet::charToShell(shellString[i]));
        } else if( !strncmp(tok,"num_exponents=",14) )
          sscanf(&tok[14],"%zu",&ecpp->num_exponents);

        tok = strtok((char*)0,"%");
      } while( tok );
        
      ecpp->num_coefficients=1;

      ecpp->powers = new int[ecpp->num_exponents];
      ecpp->exponents = new double[ecpp->num_exponents];
      ecpp->coefficients=new double[ecpp->num_exponents]; // num_coef is 1
      char* tail;

      for(size_t primitive_counter=0; primitive_counter<ecpp->num_exponents;
          primitive_counter++) {
        is->getline(line,500);

        ecpp->powers[primitive_counter] = (int)strtol(line, &tail, 10);
        ecpp->exponents[primitive_counter] = strtod(tail, &tail);
        ecpp->coefficients[primitive_counter] = strtod(tail, NULL);
      }
      potential->push_back(ecpp);
    } /* end potential */
  }
}


/**
 * Loads up all the contractions for one gbs.
 */
void DavCalculation::parseGbsData( TGaussianBasisSet* gbs, 
				   istream* is ) const 
{
  ContractionSet* contractions = 0;
  string element_name = "";

  char line[500];
  while(!is->eof()) {
    is->getline(line,500);
    if(strncmp(line,"element=",8) == 0) { // new element

      // add the previous contraction set to the map:
      if (element_name != "") {
#ifdef DEBUG
        cout << "inserting " << element_name 
             << " into " << gbs->p_name << endl;
#endif
        gbs->p_contractions[element_name] = contractions;
      } 
      contractions = new ContractionSet(); // make new contraction set
      element_name = line+8;
    } else if(strcmp(line, "EndBasisSet") == 0) { // end of basis set

      if (element_name != "") { // this is true if no elements are there
        // add last contraction set to the map
        gbs->p_contractions[element_name]=contractions;
#ifdef DEBUG
        cout << "inserting " << element_name
             << " into " << gbs->p_name << endl;
#endif
      }
      break;
    }  else if(strncmp(line,"contraction",11) == 0) { // new contraction in
                                                      // same set
      Contraction_ *cont = new Contraction_;

      char buf[100];
      sscanf(line,"contraction shell=%s num_primitives=%zu num_coefficients=%zu",
        buf, &cont->num_exponents, &cont->num_coefficients);
      for (size_t i=0;i<cont->num_coefficients;i++) 
        cont->shells.push_back(TGaussianBasisSet::charToShell(buf[i]));
    
      cont->exponents = new double[cont->num_exponents];
      cont->coefficients = 
        new double[cont->num_coefficients*cont->num_exponents];

      int coefficient_counter=0;
      for(size_t primitive_counter=0; primitive_counter < cont->num_exponents;
          primitive_counter++) {
        is->getline(line,500);

        /* get exponent */
        char* tok = strtok(line," ");
        cont->exponents[primitive_counter] = strtod(tok, NULL);

        /* get coefficients */
        coefficient_counter=0;
        tok = strtok((char*)0," ");
        do {
          int index = primitive_counter*cont->num_coefficients + 
                      coefficient_counter;
          coefficient_counter++;
          char* tmp = strchr(tok,'D'); if(tmp) tmp[0] = 'E';
          cont->coefficients[index] = strtod(tok, NULL);
          tok = strtok((char*)0," ");
        } while(tok != (char*)0);
      }
      contractions->push_back(cont);
    } /* end contraction */
  }
}


/**
 * Returns null if the property does not exist or if there is
 * a dav error reading the URL.
 */
unsigned long DavCalculation::openShells(void) const 
{
  unsigned long ret = 0;
  string osStr = getProp(VDoc::getEcceNamespace() + ":openshells");
  if (!osStr.empty()) {
    ret = atol(osStr.c_str());
  }
  return ret;
}


/**
 * Returns "unknown" if the property does not exist or if there is
 * a dav error reading the URL.
 * (I'm not sure if that's what we want to return on error.)
 */
SpinMult::SpinMultEnum DavCalculation::spinMultiplicity(void) const 
{
  SpinMult::SpinMultEnum sm = SpinMult::unknown;
  string smStr = getProp(VDoc::getEcceNamespace() + ":spinmultiplicity");
  if (!smStr.empty()) {
    sm = SpinMult::toSpinMult( smStr );
  }
  return sm;
}


/**
 * Returns null if the property does not exist or if there is
 * a dav error reading the URL.
 */
TTheory* DavCalculation::theory(const string& theoryType) const 
{
  TTheory* ttheory = 0;
  string theoryStr = "";

  if (DavCalculation::stringToTheoryType(theoryType) ==
        DavCalculation::GENERAL) {
    theoryStr = getProp(VDoc::getEcceNamespace() + ":generalTheory");
  }
  else if (DavCalculation::stringToTheoryType(theoryType) ==
             DavCalculation::SINGLE) {
    theoryStr = getProp(VDoc::getEcceNamespace() + ":singlePointTheory");
  }
  else {
    theoryStr = getProp(VDoc::getEcceNamespace() + ":theory");
  }

  if (!theoryStr.empty()) {
    size_t idx = theoryStr.rfind('/', theoryStr.length());
    string category, theory;
    if (idx != string::npos) {
      category = theoryStr.substr(0, idx);
      theory   = theoryStr.substr(idx+1, theoryStr.size());
    } else {
      category = theoryStr;
      theory = "None";
    }
    ttheory = new TTheory(category,theory);
  }

  return ttheory;
}


/**
 * Returns default TRunType if the property does not exist or 
 * if there is a dav error reading the URL.
 */
TRunType DavCalculation::runtype(void) const 
{
  TRunType runtype;

  runtype.name ( getProp(VDoc::getEcceNamespace() + ":runtype") );

  return runtype;
}



/**
 * Writes the contents of the TGBSConfig to file,
 * including the data for each GBS in each group, and the
 * mapping relationship between tags and groups.
 * Only the config meta data are saved as dav properties on the
 * file.  Everything else is saved in the file.
 */
bool DavCalculation::gbsConfig(TGBSConfig* config, const string& theoryType)
{
  bool ret = true;
  p_msgStack->clear(); 
  ostrstream output;
  string basisSetName = "";
  EcceURL basisSetUrl;

  // Set basis set name 
  if (DavCalculation::stringToTheoryType(theoryType) ==
        DavCalculation::GENERAL) {
    basisSetName = getVDoc()->getGeneralBasisSetName();
  }
  else if (DavCalculation::stringToTheoryType(theoryType) ==
             DavCalculation::SINGLE) {
    basisSetName = getVDoc()->getSinglePointBasisSetName();
  }
  else {
    basisSetName = getVDoc()->getBasisSetName();
  }

  if (config != (TGBSConfig*)0) {

    //////////////////////////////////
    // Write Basis Set File
    //////////////////////////////////
    output << "<MolecularConfig>\n";
    writeConfigMapElement(config, output);
    writeConfigDataElement(config, output);
    output << "</MolecularConfig>" << ends;
    char * cptr = output.str();
    string basisSetData = cptr;
    delete [] cptr;

    // Store Basis Set 
    try {
      istrstream is(basisSetData.c_str());
      basisSetUrl = getVDoc()->addConfiguration(basisSetName, &is);
    }
    catch (DavException& davException) {
      p_msgStack->add("UNABLE_TO_COMPLETE_REQUEST",
                      davException.what());
      ret = false;
    }


    //////////////////////////////////
    // Write Basis Set Meta Data
    //////////////////////////////////
    if (ret) {
      
      vector<MetaDataResult> config_results;
      MetaDataResult result;
      string ns = VDoc::getEcceNamespace() + ":";  

      // name property
      result.name  = ns + "name";
      result.value = config->name();
      config_results.push_back(result);

      // optimize property
      result.name  = ns + "optimize";
      if(config->optimize()) result.value = "true";
      else result.value = "false";
      config_results.push_back(result);

      // coordsys property
      result.name = ns + "coordsys";
      result.value = TGaussianBasisSet::coordSys_formatter[config->coordsys()];
      config_results.push_back(result);

      // gbs-type property
      result.name = ns + "gbs-type";
      if(config->type() == TGBSConfig::simple)
        result.value = "simple";
      else if(config->type() == TGBSConfig::element)
        result.value = "element";
      else if(config->type() == TGBSConfig::atom)
        result.value = "atom";
      config_results.push_back(result);

      // numFunctions and numPrimitives properties
      Fragment *frag = fragment();
      if (frag != 0) {
        TagCountMap *tagCounts = frag->tagCountsSTL();
        if (tagCounts != 0) {

          // numFunctions
          result.name = ns + "numFunctions";
          result.value = 
            StringConverter::toString(config->num_functions(*tagCounts));
          config_results.push_back(result);

          // numPrimitives
          result.name = ns + "numPrimitives";
          result.value = 
            StringConverter::toString(config->num_primitives(*tagCounts));
          config_results.push_back(result);

          delete tagCounts;
          tagCounts = 0;
        }
        delete frag;
        frag = 0;
      }

      // ecpName
      result.name = ns + "ecpName";
      result.value = config->ecpName();
      config_results.push_back(result);

      // chargeFittingName
      result.name = ns + "chargeFittingName";
      result.value = config->dftChargeFittingName();
      config_results.push_back(result);

      // exchangeFittingName
      result.name = ns + "exchangeFittingName";
      result.value = config->dftFittingName();
      config_results.push_back(result);

      // useExpCoef
      const JCode *codeCap = application();
      if (codeCap) {
        string val = "N";
        bool libnames = true;
        if (!codeCap->get_bool("LibraryNames", libnames)) {
          val = "Y";
        }
        result.name = ns + "useExpCoef";
        result.value = val;
        config_results.push_back(result);
      }

      getEDSI()->setURL(basisSetUrl);
      if (!getEDSI()->putMetaData(config_results)) {
        p_msgStack->add(getEDSI()->m_msgStack);
        ret = false;
      }

      // Need to update properties for this
      // calc to keep the cache in sync.  This 
      // is necessary because the properties are
      // being stored on the basis set file and
      // not on the calc.  If the proeprties were
      // being stored on the calc then would of
      // just used an addProps() call, which would
      // of updated the cache for us.
      updateProps();

    }

  }    

  else {

    //////////////////////////////////////////
    // Remove Basis Set -> if config is NULL
    //////////////////////////////////////////
    try {
      getVDoc()->removeConfiguration(basisSetName);
    }
    catch (DavException& davException) {
      p_msgStack->add("UNABLE_TO_COMPLETE_REQUEST",
                      davException.what());
      ret = false;
    }
  }
   

  // If the save was successful, update the version to the current Version
  // (this is a hack - versioning needs to be handled consistently 
  // across all applications)
  if ((ret) && (config != (TGBSConfig*)0)) {
    vector<MetaDataResult> result(1);
    result[0].name = VDoc::getEcceNamespace() + ":currentVersion";
    result[0].value = Ecce::ecceVersion();
    if (!addProps(result)) {
      ret &= false;
    }
  }

  return ret;
}



bool DavCalculation::gbsUseExpCoeff(bool useExpCoeff) {

  vector<MetaDataResult> results(1);
  results[0].name = VDoc::getEcceNamespace() + ":useExpCoeff";
  results[0].value = useExpCoeff? "true": "false";
  return addProps(results);
}



/**
 * Save the data and properties.
 * The url is presumed to have been set already.
 */
bool DavCalculation::save(istream& data, const EcceMap& props) const
{
  bool ret = true;
  if (!getEDSI()->putDataSet(data)) {
      p_msgStack->add(getEDSI()->m_msgStack);
      ret = false;
  } else {
     if (props.size() > 0) {
        vector<MetaDataResult> results(props.size());
        EcceMapIterator it = props.begin();
        int idx=0;
        while (it != props.end()) {
           results[idx].name = (*it).first;
           results[idx].value = (*it).second;
           idx++;
           it++;
        }
        if (!const_cast<DavCalculation*>(this)->addProps(results)) {
           ret = false;
        }
     }
  }
  return ret;
}


void DavCalculation::writeConfigMapElement(TGBSConfig* config,
					   ostrstream& output)
{
  TGBSConfig::iterator configIt;
  const vector<TGaussianBasisSet*>* orderedVector;

  // SAVE THE TGBSCONFIG GROUP MAPPING:
  output << "<Map>\n";

  // Write out all the groups:
  for (configIt = config->begin(); configIt != config->end(); configIt++) {
    output <<"<Group Tag=\"" << (*configIt).first << "\" Name=\"";

    if ((*configIt).second != (TGBSGroup*) 0)
      output << (*configIt).second->name();
    else
      output << "";

    output << "\">\n";
    output << "<OrderedList>\n";
    
    if ((*configIt).second != (TGBSGroup*) 0) {
    
      // Write out all the basis sets in each group IN ORDER:
      orderedVector = (*configIt).second->getOrderedList();

      for (size_t i = 0; i < orderedVector->size(); i++) {
        output << "<BasisSet Type=\""
               << TGaussianBasisSet::gbs_type_formatter[(*orderedVector)[i]->p_type]
               << "\">"
               << (*orderedVector)[i]->p_name 
               << "</BasisSet>\n";
      }
      // Write out the group's ECP (if one exists):
      if ( (*configIt).second->ecp() != NULL )
        output << "<ECP Type=\""
               << TGaussianBasisSet::gbs_type_formatter[(*configIt).second->ecp()->p_type]
               << "\">"
               << (*configIt).second->ecp()->p_name 
               << "</ECP>\n";
    }

    output << "</OrderedList>\n";
    output << "<OrbitalMapping>\n";

    if ((*configIt).second != (TGBSGroup*) 0) {
      const OrbitalMap* orbitalMap = 
        (*configIt).second->getOrbitalGBSMappings();
      OrbitalMap::const_iterator orbIt;
      vector<string> bsNames;
      string name;
      string type;
      size_t i, separatorLoc;

      for (orbIt = orbitalMap->begin(); orbIt != orbitalMap->end(); orbIt++) {
        output << "<OrbitalBasisSet ";
        output << "Name=\"" << (*orbIt).first << "\">\n";
	
        bsNames = (*orbIt).second;
        for (i = 0; i < bsNames.size(); i++) {
          separatorLoc = bsNames[i].find (":");
          name = bsNames[i].substr(0,separatorLoc);
          type = bsNames[i].substr(separatorLoc+1, string::npos);

          if (type == TGaussianBasisSet::gbs_type_formatter[TGaussianBasisSet::ecp])
            output << "<ECP Type=\"" << type << "\">" << name << "</ECP>\n";
          else
            output << "<BasisSet Type=\"" << type << "\">" << name <<
                      "</BasisSet>\n";
        }
        output << "</OrbitalBasisSet>\n";
      }
    }

    output << "</OrbitalMapping>\n";
    
    // Don't even write these elements for null groups:
    if ((*configIt).second != 0) {
      output << contractionInfoMap((*configIt).second);
      output << diffuseMap((*configIt).second);
      output << tightMap((*configIt).second);
    }
    output << "</Group>\n";
  }

  output << "</Map>\n" << "<Data>\n";  
  
}


string DavCalculation::diffuseMap(TGBSGroup* group) {
  // Prerequisite:  group is NOT null

  string ret = "<DiffuseMap>\n";
  ret += appendedFunctionMap(group->getDiffuseMap());
  ret += "</DiffuseMap>\n";
  return ret;
}


string DavCalculation::tightMap(TGBSGroup* group) {
  // Prerequisite: group is NOT null

  string ret = "<TightMap>\n";
  ret += appendedFunctionMap(group->getTightMap());
  ret += "</TightMap>\n";
  return ret;
}


string DavCalculation::appendedFunctionMap(const FunctionMap& map) {
 
  FunctionMap::const_iterator elementIt;
  string ret;
  char buf[100];

  for (elementIt = map.begin(); elementIt != map.end(); elementIt++) {
    ret += "<Element tag=\"" + (*elementIt).first + "\">\n";

    for (size_t i = 0; i < (*elementIt).second.size(); i++) {
      sprintf(buf, "%d", (*elementIt).second[i]);
      ret += buf;
      ret += " ";
    }
    ret += "</Element>\n";
  }
  return ret;
}
    

string DavCalculation::contractionInfoMap(TGBSGroup* group) {
  // Prerequisite: group is NOT null

  string ret = "<ContractionInfoMap>\n";
  GBSToContInfoMap::iterator gbsIt;
  ElementToGBSInfoMap::iterator elementIt;

  // Map structure contains a set of basis sets for every element
  for (elementIt = group->getContractionInfoMap().begin();
       elementIt != group->getContractionInfoMap().end();
       elementIt++) {
    
    ret += "<Element tag=\"" + (*elementIt).first + "\">\n";
    
    for (gbsIt = (*elementIt).second.begin();
	      gbsIt != (*elementIt).second.end();
         gbsIt++) {
      ret += "<BasisSet uniqueKey=\"" + (*gbsIt).first + "\">\n";
      ret += "<DeleteFlags>";
      size_t i;
      for (i = 0; i < (*gbsIt).second.deleteFlags.size(); i++)
        if ((*gbsIt).second.deleteFlags[i] == true)
          ret += "true ";
        else 
          ret += "false ";

      ret += "</DeleteFlags>\n<UncontractFlags>\n";
      for (i = 0; i < (*gbsIt).second.uncontractFlags.size(); i++)
        if ((*gbsIt).second.uncontractFlags[i] == true)
          ret += "true ";
        else
          ret += "false ";

      ret += "</UncontractFlags>\n</BasisSet>\n";
    }
    ret += "</Element>\n";
  }

  ret += "</ContractionInfoMap>\n";
  return ret;
}


void DavCalculation::writeConfigDataElement(TGBSConfig* config,
					    ostrstream& output) 
{ 
  // BUILD MASTER SET OF BASIS SETS
  // Because 2 basis sets of different type can have the same name,
  // the key is formed by concatenating the name with the type;
  // for example, Stuttgart RLC ECP:ECPOrbital vs. 
  // Stuttgart RLC ECP:ecp

  TGBSConfig::iterator configIt;
  TGBSGroup::iterator groupIt;
  map <string, TGaussianBasisSet*, less<string> > masterList;
  map <string, TGaussianBasisSet*, less<string> >::iterator listIt;
  ContractionMap::iterator contMapIt;
  ContractionMap::const_iterator elementLoc;
  ContractionMap::const_iterator ecpContIt;     
  string key;

  // for all groups in the config
  for (configIt = config->begin(); configIt != config->end();
       configIt++) {
    if ((*configIt).second == (TGBSGroup*) 0)
      continue;

    // for all basis sets in each group
    for (groupIt = (*configIt).second->begin();
         groupIt != (*configIt).second->end(); groupIt++) {
      key = (*groupIt).second->p_name
          + ":"  
          + TGaussianBasisSet::gbs_type_formatter[(*groupIt).second->p_type];
      listIt = masterList.find(key);
      if (listIt == masterList.end()) {
        TGaussianBasisSet* bset = new TGaussianBasisSet(*((*groupIt).second));
        masterList[key] = bset; 
      } else { // listIt and groupIt point to the same basis set
        // check to make sure that basis set contains all elements:
        for (contMapIt = (*groupIt).second->p_contractions.begin();
             contMapIt != (*groupIt).second->p_contractions.end();
             contMapIt++) {
          elementLoc =(*listIt).second->p_contractions.find((*contMapIt).first);
          if (elementLoc == (*listIt).second->p_contractions.end())
            (*listIt).second->p_contractions[(*contMapIt).first]
              = new ContractionSet(*((*contMapIt).second));
        }
      }
    }

    // check ecp too
    if ((*configIt).second->ecp() != NULL) {
      key = (*configIt).second->ecp()->p_name
	  + ":"
	  + TGaussianBasisSet::gbs_type_formatter[(*configIt).second->ecp()->p_type];
      listIt = masterList.find(key);
      if (listIt == masterList.end())
        masterList[key] = new TGaussianBasisSet(*((*configIt).second->ecp()));
      else {
        // check to make sure that basis set contains all elements:   
        for (ecpContIt = (*configIt).second->ecp()->p_contractions.begin();
             ecpContIt != (*configIt).second->ecp()->p_contractions.end();
             ecpContIt++) {
          elementLoc =
            (*listIt).second->p_contractions.find((*ecpContIt).first);
          if (elementLoc == (*listIt).second->p_contractions.end())
            (*listIt).second->p_contractions[(*ecpContIt).first] =
               new ContractionSet(*((*ecpContIt).second));
        }
      }
    }  
  }

#ifdef DEBUG
  for (listIt = masterList.begin(); listIt != masterList.end(); listIt++) {
    cout << " basis set = " << (*listIt).second->p_name << ":"
         << TGaussianBasisSet::gbs_type_formatter[(*listIt).second->p_type]<< endl;

    for (contMapIt = (*listIt).second->p_contractions.begin();
         contMapIt != (*listIt).second->p_contractions.end();
         contMapIt++)
      cout << (*contMapIt).first << " ";

    cout << endl;
  }
#endif

  // WRITE OUT MASTER LIST OF BASIS SETS:
  vector<Contraction_*>::iterator contSetIt;       
  for(listIt = masterList.begin(); listIt != masterList.end(); listIt++) {
    if ((*listIt).second == NULL)
      continue;

    output << "BasisSet=" << (*listIt).second->p_name 
           << "\n"
           << "Type=" 
           << TGaussianBasisSet::gbs_type_formatter[(*listIt).second->p_type]
           << " Category="
           << TGaussianBasisSet::gbsCategory_formatter[(*listIt).second->p_category]
           << " Spherical="
           << TGaussianBasisSet::coordSys_formatter[(*listIt).second->p_coordSys]
           << " ContType="
           << TGaussianBasisSet::contractionType_formatter[(*listIt).second->p_contType]
           << "\n";
    
    // write the contractions for each basis set:
    for (contMapIt = (*listIt).second->p_contractions.begin();
         contMapIt != (*listIt).second->p_contractions.end();
         contMapIt++) {
      if ((*contMapIt).second == NULL)
        continue;

      if ((*listIt).second->p_type != TGaussianBasisSet::ecp) {
        output << "element=" << (*contMapIt).first << endl;
      
        for (contSetIt  = (*contMapIt).second->begin();
             contSetIt != (*contMapIt).second->end(); contSetIt++) {
          if ((*contSetIt) == NULL)
            continue;

          output << "contraction shell=";
          for (size_t i=0;i<(*contSetIt)->num_coefficients;i++) 
            output << TGaussianBasisSet::shell_formatter[(*contSetIt)->shells[i]];

          output << " num_primitives=" << (*contSetIt)->num_exponents;
          output << " num_coefficients=" << (*contSetIt)->num_coefficients;
          output << endl;
      
          for (size_t expIdx=0; expIdx < (*contSetIt)->num_exponents; expIdx++) {
            output.setf(ios::fixed|ios::showpoint);
            output.precision(12);
            output << (*contSetIt)->exponents[expIdx];
	    
            for (size_t coeffIdx=0;
                 coeffIdx < (*contSetIt)->num_coefficients; coeffIdx++) {
              int idx = expIdx*(*contSetIt)->num_coefficients + coeffIdx;
              output.setf(ios::fixed|ios::showpoint);
              output.precision(12);
              output << " " << (*contSetIt)->coefficients[idx];
            }
	        output << endl;
          } 
        } // end loop over contractions in each contraction set
      } else { // save ecp data
        output << "element=" << (*contMapIt).first
               << " ncore=" << (*contMapIt).second->ncore
               << " lmax=" << (*contMapIt).second->lmax << endl;

        for (contSetIt  = (*contMapIt).second->begin();
             contSetIt != (*contMapIt).second->end(); contSetIt++) {
          if ((*contSetIt) == NULL)
            continue;

          output << "ecp_potential%l=" << (*contSetIt)->l << "%shell="; 
          for (size_t i=0;i<(*contSetIt)->shells.size();i++) {
            if (i != 0)
              output << "-";
            output << TGaussianBasisSet::shell_formatter[(*contSetIt)->shells[i]];
          }

          output << " POTENTIAL%num_exponents=" << (*contSetIt)->num_exponents
                 << endl;

          output.setf(ios::fixed|ios::showpoint);
          output.precision(12);
          for (size_t primitive_counter = 0;
               primitive_counter < (*contSetIt)->num_exponents;
               primitive_counter++)
            output << (*contSetIt)->powers[primitive_counter] << " "
                   << (*contSetIt)->exponents[primitive_counter] << " "
                   << (*contSetIt)->coefficients[primitive_counter] << endl;
        } // end loop over contractions in each contraction set
      } // end ecp save 
    } // end loop over contraction sets, one for each element 

    output << "EndBasisSet" << endl;

  } // end loop over basis sets in master list

  output << "</Data>" << endl;

  // NEED TO FREE THE MASTER LIST POINTERS:
}



bool DavCalculation::openShells(unsigned long openshell) 
{
  vector<MetaDataResult> results(1);
  char temp[100];

  results[0].name = VDoc::getEcceNamespace() + ":openshells";
  sprintf(temp, "%ld", openshell);
  results[0].value = temp;
  
  return addProps(results);
}



bool DavCalculation::spinMultiplicity(SpinMult::SpinMultEnum sm) 
{
  vector<MetaDataResult> results(1);

  results[0].name = VDoc::getEcceNamespace() + ":spinmultiplicity";
  results[0].value = SpinMult::toString(sm);
  return addProps(results);
}



bool DavCalculation::guiparams(GUIValues *params, const string& theoryType) {

  bool ret = true;

  string setupParametersName = "";
  EcceURL setupParametersUrl;

  // Set parameters name
  if (DavCalculation::stringToTheoryType(theoryType) ==
        DavCalculation::GENERAL) {
    setupParametersName = getVDoc()->getGeneralSetupParametersName();
  }
  else if (DavCalculation::stringToTheoryType(theoryType) ==
             DavCalculation::SINGLE) {
    setupParametersName = getVDoc()->getSinglePointSetupParametersName();
  }
  else {
    setupParametersName = getVDoc()->getSetupParametersName();
  }

  p_msgStack->clear();

  try {
    string paramStr = params->toString();
    istrstream is(paramStr.c_str());
    setupParametersUrl = getVDoc()->addConfiguration(setupParametersName, &is);
  }
  catch (DavException& davException) {
    p_msgStack->add("UNABLE_TO_COMPLETE_REQUEST",
                    davException.what());
    ret = false;
  }

  if (ret) {
    getEDSI()->setURL(getVDoc()->getBaseURL());
    vector<MetaDataRequest> mdr(1);
    mdr[0].name = VDoc::getEcceNamespace() + ":guiparams";
    getEDSI()->removeMetaData(mdr);
  }

  return ret;
}



bool DavCalculation::theory(TTheory* theory, const string& theoryType)
{
  bool ret = false;
  string propName = "";
  string propValue = "";

  if (DavCalculation::stringToTheoryType(theoryType) ==
        DavCalculation::GENERAL) {
    propName = VDoc::getEcceNamespace() + ":generalTheory";
  }
  else if (DavCalculation::stringToTheoryType(theoryType) ==
             DavCalculation::SINGLE) {
    propName = VDoc::getEcceNamespace() + ":singlePointTheory";
  }
  else {
    propName = VDoc::getEcceNamespace() + ":theory";
  }

  if (!propName.empty()) {
    if (theory != 0) {
      propValue = theory->category();
      if (theory->name() != "None") {
        propValue += '/';
        propValue += theory->name();
      }
    }
    ret = addProp(propName, propValue);
  }

  return ret;
}



bool DavCalculation::runtype(const TRunType& runtype) 
{
  vector<MetaDataResult> results(1);
  results[0].name = VDoc::getEcceNamespace() + ":runtype";
  results[0].value = runtype.name();
  return addProps(results);
}



/* =========================================================================
   private functions -- for parsing xml documents within xml'ized data types
                        for a calculation
   ========================================================================= */

string DavCalculation::getXMLElementValue(
  const DOMDocument& doc,
  const string& name
) const {

  string ret;

  XMLCh *xStr = XMLString::transcode(name.c_str());
  DOMNodeList *nodes = doc.getElementsByTagName(xStr);
  delete [] xStr;

  if (nodes->getLength() != 0 && nodes->item(0)->hasChildNodes()) {
    DOMNode *child = nodes->item(0)->getFirstChild();
    if (child->getNodeType() == DOMNode::TEXT_NODE) {
      char *tempStr = XMLString::transcode(child->getNodeValue());
      ret = tempStr;
      delete [] tempStr;
    }
  }

  return ret;
}



const DOMDocument* DavCalculation::getXMLDocument( const string& docTail) const
{

  // set up document
  string xmlDoc =   "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n";
  string xmlbegin = "<EcceValueDoc>\n";
  string xmlend =   "</EcceValueDoc>\n";
  xmlDoc = xmlbegin ;
  xmlDoc += docTail;
  xmlDoc += xmlend;
  xmlDoc += "\n";

  // translate into membuf
  const char * gMemBufId = "Dav Response";
  MemBufInputSource* memBufIS = new MemBufInputSource (
        (const XMLByte*)xmlDoc.c_str(), xmlDoc.length(), gMemBufId, false);

  // parse document
  BasicDOMParser parser;
  DOMDocument *doc = parser.parse(*memBufIS);
  delete memBufIS;

#if 0
  if (doc.isNull() || !doc.hasChildNodes()) {
    // documentation says that you should not new DOMDocuments
    // DOM has garbage collection -- lets see what happens when
    // doc loses scope!!

    ret = &doc;
  }
#endif

  return doc;
}


/** 
 * Writes the indicated fragment to file on the dav server.
 * If frag is NULL, then it erases the fragment from the dav
 * server.
 *
 * @param frag Fragment pointer
 *
 * @return Returns true if the fragment was successfully stored, 
 *         otherwise false is returned.
 * 
 * @todo Need to augment the Fragment class to put
 *       the state information requested in the Fragment class.
 *       Need logic to get all Molecule formats...
 */
bool DavCalculation::fragment(Fragment* frag)
{
  bool ret = true;
  p_msgStack->clear();

  // set url to molecule
  string moleculeName = getVDoc()->getMoleculeName();
  EcceURL moleculeUrl;

  if (frag && frag->numAtoms()>0) { // save fragment to file
    ret = ChemistryTask::fragment(frag);
  }
  else { // Remove the resource
    try {
      getVDoc()->removeConfiguration(moleculeName);
    }
    catch (DavException& davException) {
      p_msgStack->add("UNABLE_TO_COMPLETE_REQUEST",
                      davException.what());
      ret = false;
    }

    (void)gbsConfig(0);
    GUIValues guivals;
    (void)guiparams(&guivals);
  }
  return ret;
}


/**
 * Big data member so returns a pointer to the fragment.
 * User is responsible for deleting when finished.
 * Returns null on error.  (Not all fragment data may get set
 * if some properties are missing.)
 *
 * @return Returns a Fragment pointer, user responsible for memory
 *         management.
 */
Fragment* DavCalculation::fragment() const
{
  return ChemistryTask::fragment();
}


/**
 * Indicates if the Calculation's input provider (i.e. other Tasks 
 * within a Session) can be modified/removed.  For a Calculation task this 
 * is determined by the ResourceDescriptor::RUNSTATE - if it has not been SUBMITTED 
 * then it may be altered.
 * 
 * @return bool indicates that the Task's input provider
 *         may be altered within a Session
 */
bool DavCalculation::canModifyInputProvider()
{
  return (getState() < ResourceDescriptor::STATE_SUBMITTED);
}



/**
 * Returns string value for a theory type.  If the theory type
 * is not recognized, an empty string is returned.
 *
 * @param theory the type of theory
 *
 * @return string value of theory type.
 */
string DavCalculation::theoryTypeToString(DavCalculation::TheoryType theory)
{
  string theoryStr = "";

  if (theory == DavCalculation::GENERAL) {
    theoryStr = "general";
  }
  else if (theory == DavCalculation::SINGLE) {
    theoryStr = "single";
  }

  return theoryStr;
}


/**
 * Returns the type of theory for a string value.  If the string value
 * does not map to a theory type then the UNDEFINED theory
 * type is returned.
 *
 * @param str string value
 *
 * @return returns the type of theory for a string value.
 */
DavCalculation::TheoryType DavCalculation::stringToTheoryType(const string& str)
{
  DavCalculation::TheoryType
    theory = DavCalculation::UNDEFINED_THEORY;

  if (str == "general") {
    theory = DavCalculation::GENERAL;
  }
  else if (str == "single") {
    theory = DavCalculation::SINGLE;
  }
  else {
    theory = DavCalculation::UNDEFINED_THEORY;
  }

  return theory;
}


/**
 * Imports a .frag, and a .gbs file making use of a .param file and
 * also invokes the processImportParameters
 * method.  These files are removed at the end.
 */
string DavCalculation::import( const string& dir, const string& parseFileName) 
   throw(EcceException)
{
   string message;

   string fullFileRoot = dir + "/" + parseFileName;
   size_t index = fullFileRoot.find_last_of('.');
   if (index != string::npos) fullFileRoot = fullFileRoot.substr(0, index);

   // define , basis, and param file names here so they are
   // scoped for the rm command at the end
   string gbsFileName = fullFileRoot + ".gbs";
   string fragFileName = fullFileRoot + ".frag";
   string paramFileName = fullFileRoot + ".param";

   // load .param file
   Preferences params(paramFileName, true);
   if (!params.isValid()) {
      throw EcceException("Setup parse script command "  
         " did not create parameter file.", WHERE);
   }


   // delegate fragment import to superclass and add basis set processing.
   // There is a minor performance hit because the param file will be loaded
   // twice
   ChemistryTask::importFragment(fragFileName, params);

   string theorycat;
   params.getString("Category", theorycat);

   const JCode *codeCap = application();
   if (codeCap->theoryCategoryNeedsBasis(theorycat)) {
      // import .gbs file

      SFile bfile(gbsFileName);
      if (bfile.exists()) {

         // import basis set
         ifstream basisStream(gbsFileName.c_str());

         TGBSConfig *basis = ICalcUtils::importConfig(basisStream);
         if (basis == (TGBSConfig*)0) {
            throw EcceException(
                  "Unable to import basis set from file created by setup "
                  "parse script.", WHERE);
         }
         basisStream.close();

         // spherical is correct even when there is no SphericalBasis key
         bool spherical = false;
         params.getBool("SphericalBasis", spherical);
         basis->coordsys(spherical? TGaussianBasisSet::Spherical:
               TGaussianBasisSet::Cartesian);

         gbsConfig(basis);
         delete basis;
      } else {
         message = "Could not parse basis set from calculation output file "
                   "(attempting import without it).";

         // to prevent attempted delete
         gbsFileName = "";
      }
   }

   // Subclasses may have other stuff in the parameter file.
   processImportParameters(params);

   /* remove temparay files .param .frag files */
   // don't do this because it is useful debugging information and with a
   // successful import the parent directory will be deleted anyway if
   // $ECCE_JOB_LOGMODE is not set
   // string cmd = "cd " + dir + "; ";
   // cmd += "/bin/rm -f ";
   // cmd += paramFileName;
   // cmd += " ";
   // if (theorycat != "SE" && gbsFileName != "") {
   //    cmd += " ";
   //    cmd += gbsFileName+"*";
   // }
   // system(cmd.c_str());

   return message;
}




/**
 * Generate parse script args for electronic structure calcs.
 * All scripts get the same args.
 */
string DavCalculation::getParseScriptArgs() const
{
   string parseArgs;

   // set "key" as current directory, "." to shorten command line
   parseArgs = " . \"";

   parseArgs.append(runtype().name());
   parseArgs.append("\" \"");

   // checking theory first avoids a very nasty bug (infinite loop variety)
   // with calculations that don't have a theory like the MD tasks
   if (theory()) {
      if (!theory()->category().empty()) {
         parseArgs.append(theory()->category());
         parseArgs.append("\" \"");
      }

      if (!theory()->name().empty())
         parseArgs.append(theory()->name());
   }

   char tmpbuf[6];
   sprintf(tmpbuf, "%lu", openShells());
   parseArgs.append("\" \"");
   parseArgs.append(tmpbuf);
   parseArgs.append("\"");


   return parseArgs;
}



/**
 * Overrides ChemistryTask method to also support:
 * <li>Category
 * <li>Theory
 * <li>RunType
 * <li> ES.ChemSys.Multiplicity
 */
void DavCalculation::processImportParameters(const Preferences& params) 
      throw(IOException)
{
   ChemistryTask::processImportParameters(params);

   string theorycat;
   string rwstring, rwstring2;

   if (params.getString("Category", theorycat) &&
       params.getString("Theory", rwstring2)) {
      TTheory atheory(theorycat, rwstring2);
      theory(&atheory);
   } else {
      // Old code set message but didn't use it - may want it later
      //message = "Unable to import level of theory from setup parameter file.";
      //return ret;
   }

   if (params.getString("RunType", rwstring)) {
      // HACK:  Hardwire noSpherical to false for now
      TRunType aruntype(rwstring, false);
      runtype(aruntype);
   } else {
      // Old code set message but didn't use it - may want it later
      //message = "Unable to import run type from setup parameter file.";
      //return ret;
   }

   int number;
   if (params.getInt("ES.ChemSys.Multiplicity", number)) {
      spinMultiplicity((SpinMult::SpinMultEnum)number);
   }

}


/**
 * Removes calculation from dav server.  If the resource is in a session,
 * the resource is removed from there as well.
 */
bool DavCalculation::remove()
{
  bool ret = false;
  p_msgStack->clear();
  
  Session *session = getSession();
  if (session != 0) {
    session->removeMember(this);
    session->saveLinkbase();
    if (Resource::remove()) {
      ret = true;
    }
  }
  else {
    ret = Resource::remove();
  }

  return ret;
}


/**
 * Copies calculation to target URL.  Copying a session based
 * calculation is not supported.
 */
bool DavCalculation::copy(EcceURL& target)
{
  bool ret = false;
  p_msgStack->clear();

  Session *session = getSession();
  if (session != 0) {
    p_msgStack->add("UNABLE_TO_COPY",
                    "\nCopy of a session based calculation is not supported!");
    session = 0;
  }
  else {
    ret = Resource::copy(target);
  }

  return ret;

}


/**
 * Moves calculation to target URL.  Only renames are supported
 * for session based calculations (ie moves within the same collection).
 */
bool DavCalculation::move(EcceURL& target)
{
  bool ret = false;
  p_msgStack->clear();

  Session *session = getSession();

  if (session != 0) {

    // Detect whether it is a rename
    if (target.getParent() != getURL().getParent()) {
      p_msgStack->add("UNABLE_TO_MOVE",
                      "\nMove of a session based calculation is not supported!");
    }
    else {
      if (Resource::move(target)) {
        session->saveLinkbase();
        ret = true;
      }
    }
  }
  else {
    ret = Resource::move(target);
  }

  setURL(target);
  return ret;
}


