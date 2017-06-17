/////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: EDSIGaussianBasisSetLibrary.C
//
//
// DESIGN:
//
///////////////////////////////////////////////////////////////////////////////

#include <iostream>
  using std::cout;
  using std::cerr;
  using std::endl;
  using std::ends;
#include <strstream>
  using std::istrstream;

#include <stdio.h>

#include <algorithm>
   using std::sort;
   using std::includes;

#include "util/STLUtil.H"
#include "util/ErrMsg.H"
#include "util/Ecce.H"

#include "dsm/EDSI.H"
#include "dsm/EDSIFactory.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/EDSIGaussianBasisSetLibrary.H"
#include "dsm/TGBSConfig.H"


// Debug flags:
// #define DEBUG

///// static declarations ///////
const char * EDSIGaussianBasisSetLibrary::p_ns = 
          "http://www.emsl.pnl.gov/ecce:";  //DAV namespace

/*******************************************************************
 Method : Constructor
 Summary: 
*******************************************************************/
EDSIGaussianBasisSetLibrary::EDSIGaussianBasisSetLibrary(
						   const EcceURL& url) 
{
  p_gbsURLBase = url;
}

/*******************************************************************
 Method : Copy Constructor
 Summary: 
*******************************************************************/
EDSIGaussianBasisSetLibrary::EDSIGaussianBasisSetLibrary(
			       EDSIGaussianBasisSetLibrary& gbsl) 
{
  p_gbsURLBase = gbsl.p_gbsURLBase;

  GBSMetaData::iterator mapIt;
  vector<gbs_alias*>* nextVector;

  // copy internal cache
  for (mapIt = gbsl.p_gbsMetaData.begin(); mapIt != gbsl.p_gbsMetaData.end();
       mapIt++)
  {
    if ((*mapIt).second)
    {
      nextVector = new vector<gbs_alias*>;
      for (size_t i = 0; i < (*mapIt).second->size(); i++)
      {
	if ( (*(*mapIt).second)[i] )
	  nextVector->push_back(new gbs_alias(*(*(*mapIt).second)[i]));
        else
	  nextVector->push_back((gbs_alias*)NULL);
      }
    }
    else
      nextVector = (vector<gbs_alias*>*) 0;
    p_gbsMetaData[(*mapIt).first] = nextVector;
  }
}

/*******************************************************************
 Method : Destructor
 Summary: Deletes the dynamic map of gbs_aliases.
*******************************************************************/
EDSIGaussianBasisSetLibrary::~EDSIGaussianBasisSetLibrary(void)
{
  GBSMetaData::iterator mapIt;

  // free internal cache
  for (mapIt = p_gbsMetaData.begin(); mapIt != p_gbsMetaData.end();
       mapIt++)
    if ((*mapIt).second) {
      for (size_t i = 0; i < (*mapIt).second->size(); i++) {
	if ( (*(*mapIt).second)[i] )
	  delete (*(*mapIt).second)[i]; // delete the gbs_alias
      }
      delete (*mapIt).second; // delete the vector
  }
}

/*******************************************************************
 Method : checkStatus
 Summary: Returns false if the meta data that maps basis set names
          to files is missing from the GBSL default directory.

          Note that this doesn't check the validity of each individual
	  basis set, just access to the library in general.  It
	  checks the mapping meta data because without it, the library 
	  is hosed.
*******************************************************************/
bool EDSIGaussianBasisSetLibrary::checkStatus() 
{
  bool ret = false;
  const vector<gbs_alias*> *aliasList = 
    getAliasList(TGaussianBasisSet::pople);

  if (aliasList)
    ret = true;

  return ret;
}

/*******************************************************************
 Method : isSupported
 Summary: This function checks to see if the given basis set
          of the given type supports the list of atoms in tag.
*******************************************************************/
bool EDSIGaussianBasisSetLibrary::isSupported(const string& bsName,
                          TGaussianBasisSet::GBSType type,
                          const string& tag) {
  TGBSConfigTags tags(tag.c_str());
  vector<const char*> strTags;
  size_t i;
  bool ret = false;

  for (i=0; i < tags.size(); i++) {
    strTags.push_back(tags[i].c_str());
  }
  // find sub list containing tags
  sort(strTags.begin(),strTags.end(),ltstr());

  const gbs_alias* alias=getGbsAlias(bsName.c_str(), type);

  if (alias != 0) {    
    ret = includes( alias->atoms[0].begin(),
                    alias->atoms[0].end(),
                    strTags.begin(),
                    strTags.end(), ltstr());
    
  } else {
    string errMsg = "GBSL: no mapping meta data found for type ";
    errMsg += TGaussianBasisSet::gbs_type_formatter[type];
    EE_RT_ASSERT(false, EE_WARNING, errMsg);
  }

  return ret;
}

string EDSIGaussianBasisSetLibrary::getNiceName(const string& bsName) {
  
  string ret;
  TGaussianBasisSet::GBSType type =
    TGaussianBasisSet::UnknownGBSType;
  const gbs_alias* alias = getGbsAlias(bsName.c_str(), type);
  if (alias != 0) {
    ret = alias->nicename;
  }
  return ret;
}

/*******************************************************************
 Method : gbsNameList
 Summary: Returns a list of GBS names.  Each GBS in the list must
          match the indicated type and also support all the atoms
          listed in the tags vector.
       
          Returns 0 if an error occurs reading the gbs_alias from
          the DAV server. 

          Return vector should be deleted by user, but NOT the
          char* contained within the vector.
*******************************************************************/
vector<const char*>* EDSIGaussianBasisSetLibrary::gbsNameList(
        TGaussianBasisSet::GBSType type, vector<const char*>* tags) 
{

  vector<const char*>* result = 0;

  // alias_list is a vector of meta data for all the GBSs in the
  // configuration
  // it points to data saved in the cache, so don't delete it
  const vector<gbs_alias*> *alias_list = getAliasList(type);
                                                
  if(alias_list) {
    result = new vector<const char*>;

    // find sub list containing tags
    if(tags) sort(tags->begin(),tags->end(),ltstr());
 
    bool hasTag = true;
    vector<gbs_alias*>::const_iterator it;
    gbs_alias* alias=0;
    for(it = (*alias_list).begin();
        it != (*alias_list).end();
        it++) {
      alias = (gbs_alias*)(*it);
      hasTag = true;
      if(tags) {
        sort(alias->atoms[0].begin(), alias->atoms[0].end(),ltstr());
        hasTag = includes( alias->atoms[0].begin(), 
                           alias->atoms[0].end(),
                           tags->begin(),
                           tags->end(),ltstr());
      }
      if(hasTag) result->push_back((*it)->nicename);
    }
  } else {
    string errMsg = "GBSL: no mapping meta data found for type ";
    errMsg += TGaussianBasisSet::gbs_type_formatter[type];
    EE_RT_ASSERT(alias_list, EE_WARNING, errMsg);
  }

  return result;
}

/*******************************************************************
 Method : getAliasList

 Summary: Returns a vector of gbs_aliases, one gbs_alias for each
          basis set in the given type category.  A gbs-alias is a
          a meta data structure used to map basis set names to files.

	  Specifically, each alias gives the
          basis set name, its supported atoms (one atoms list for each basis
	  set in the aggregate set), and a list of all the
	  files associated with that basis set (more than one for
	  aggregate basis sets).

	  DO NOT delete return vector as it is stored in the
          internal cache.

*******************************************************************/
const vector<gbs_alias*>* EDSIGaussianBasisSetLibrary::getAliasList(
                                          TGaussianBasisSet::GBSType type) 
{
  vector<gbs_alias*>* alias_list = 0;

  // need to put this up here, so its string buffer won't go out of scope
  vector<MetaDataResult> results;

  GBSMetaData::iterator cached_list = p_gbsMetaData.find(type);

  // retrieve from cache
  if(cached_list != p_gbsMetaData.end()) {
    alias_list =  (*cached_list).second;

  } else {
    // make dav connection
    EDSI *edsi = EDSIFactory::getEDSI(p_gbsURLBase);

    // set url to gbs type mapping file
    string url = p_gbsURLBase.toString();
    url = url + "/" + TGaussianBasisSet::gbs_type_formatter[(int)type];
    edsi->setURL(url);

    istream* input = edsi->getDataSet();

    // No file exists, so we are using the old way - stored as DAV properties
    if (input == 0) {
      edsi->setURL(p_gbsURLBase.toString().c_str());
      
      // get collection meta data
      vector<MetaDataRequest> requests(1);

      // Convert  type to a string:
      requests[0].name = string(p_ns) + TGaussianBasisSet::gbs_type_formatter[(int)type];    
  
      if( edsi->getMetaData(requests,results) && results.size() > 0 
	  && results[0].name == requests[0].name) {
	             
	// Note - we have to allocate vector<MetaDataResult> up above, because
	// we can't delete it until AFTER the istrstream goes out of
	// scope.  This is because the istrstream DOES NOT make a copy
	// of the char* that is passed into its constructor.
	istrstream* os = new istrstream(results[0].value.c_str());
	input = os;

      } else {
#ifdef DEBUG
	     cout << "property " << TGaussianBasisSet::gbs_type_formatter[(int)type] 
	          << " failed on propfind" << endl;
#endif
      }
    }

    if (input != 0) {
      alias_list = new vector<gbs_alias*>;
      gbs_alias* alias=0;
      char line[500];
      size_t i;

      while(!input->eof()) {
	input->getline(line,500);

	if(!strncmp(line,"name= ",6)) {
	  if(alias) {
	    alias_list->push_back(alias);
	  }
	  
	  alias = new gbs_alias;
	  alias->name = strdup(&line[6]);
     alias->nicename = strdup(&line[6]);

     // Convert to all lower case:
     for(i = 0; i < strlen(alias->name); i++) {
       alias->name[i] = tolower(alias->name[i]);
     }
	  
	} else if(!strncmp(line,"files= ",7)) {
	  
	  char* tok = strtok(&line[7]," ");
	  do {
	    alias->files.push_back(strdup(tok));
	  } while((tok = strtok((char*)0," ")));
  
	} else if(!strncmp(line,"atoms= ",7)) {
	  vector<char*> tmpVector;
	  
	  char* tok = strtok(&line[7]," ");
	  do {
	    tmpVector.push_back(strdup(tok));
	  } while((tok = strtok((char*)0," ")));
	  
          // sort the vector first:                  
          sort(tmpVector.begin(), tmpVector.end(),ltstr());
	  alias->atoms.push_back(tmpVector);
	}
      }
      // capture last alias
      alias_list->push_back(alias);
      
      // cache
      p_gbsMetaData[type] = alias_list;

      // free memory
      delete input;
    }
  }

  // convert alias_list to constant vector so user can't delete:
  const vector <gbs_alias*>* ret = alias_list;
  return ret;
}

/*******************************************************************
 Method : getGbsAlias
 Summary: Returns a gbs_alias for a specific GBS (specified by
          name and type).  See getGbsAliasList for a description
          of the gbs_alias.
          
*******************************************************************/
const gbs_alias* EDSIGaussianBasisSetLibrary::getGbsAlias(
             const char* gbs_name, TGaussianBasisSet::GBSType& type) 
{ 
  const gbs_alias* alias = 0;

  // Convert gbs_name to lower case for lookup
  string lowername = gbs_name;
  STLUtil::toLower(lowername);

  // If type is unknown, we have to check through all basis set categories
  // to find a match.  Note that priority is given to orbital basis sets,
  // in case there is a naming conflict (which happens on occasion - 
  // e.g., DFT aggregate groups)

  if (type == TGaussianBasisSet::UnknownGBSType) {
    bool match = false;

    for (int i = 0; i < 12; i++) { 
  
      const vector<gbs_alias*> *alias_list = 
	getAliasList((TGaussianBasisSet::GBSType)i);
      
      if (alias_list) {
	vector<gbs_alias*>::const_iterator alias_it;
	for(alias_it  = alias_list->begin();
	    alias_it != alias_list->end(); alias_it++) {
	  if( !strcmp((*alias_it)->name, lowername.c_str()) ) {
	    alias = (*alias_it);
	    match = true;
	    type = (TGaussianBasisSet::GBSType)i;
	    break;
	  } 
	}
	if (match == true)
	  break;
      } else {
	string errMsg = "GBSL: no mapping meta data found for type ";
	errMsg += TGaussianBasisSet::gbs_type_formatter[(TGaussianBasisSet::GBSType)i];
	EE_RT_ASSERT(alias_list, EE_WARNING, errMsg);
      }
    }
  } else {
    const vector<gbs_alias*> *alias_list = getAliasList(type);
    
    if(alias_list) {
      
      vector<gbs_alias*>::const_iterator alias_it;
      for(alias_it  = alias_list->begin();
	  alias_it != alias_list->end(); alias_it++) {
	if( !strcmp((*alias_it)->name, lowername.c_str()) ) {
	  alias = (*alias_it);
	}
      }
      
    } else {
      string errMsg = "GBSL: no mapping meta data found for type ";
      errMsg += TGaussianBasisSet::gbs_type_formatter[type];
      EE_RT_ASSERT(alias_list, EE_WARNING, errMsg);
    }
  }
  return alias;
}
/**********************************************************************
 Method:  simpleLookup

 Summary: This function looks up an orbital basis set (which could
          be an aggregate set), and creates a simple TGBSConfig out of
          it.  Note that tag must be provided so the group can be
          inserted into the TGBSConfig map.
	  
          If the aggregate set is a DFT set, then only the charge
	  fitting will be added to the group (no exchange) for
	  calculation efficiency.

	  User is responsible for deleting the returned pointer.
**********************************************************************/
TGBSConfig* EDSIGaussianBasisSetLibrary::simpleLookup
( const char* gbs_name,
  const char* tag
)
{
  TGBSConfig* config = new TGBSConfig;  // defaults are Spherical,
                                        // max_l_code = i_shell,
                                        // optimize = true,
                                        // type = simple 

  TGBSGroup* group = new TGBSGroup; // only one group since simple
  
  vector<TGaussianBasisSet*> gbsList = 
    lookup(gbs_name, TGaussianBasisSet::UnknownGBSType, tag);
  
 
  group->insertOrbitalGBS(gbs_name, gbsList);

  config->insertGBSGroup(tag, group);

  return config;
}

/*******************************************************************
 Method : lookup
 Summary: This function looks up a basis set from the EMSL
          library stored on the DAV server.  It returns a vector of
	  basis sets BECAUSE if the basis set is aggregate
	  (such as STO-3G*), then the basis set actually includes
	  a set of basis sets (in this case, the orbital basis
	  set STO-3G, plus an additional polarization basis set).

	  Returns empty vector on error.

          Both gbs_type and tag are optional.  If no type is entered
	  lookup finds the orbital basis set with the same name.
          If no tag is specified, lookup returns contractions for
          all the elements that the basis set supports.  Otherwise,
          only those elements in tag are returned (if they are
	  supported).

          User is responsible for deleting the returned pointers.
*******************************************************************/
vector<TGaussianBasisSet*> EDSIGaussianBasisSetLibrary::lookup
( const char* gbs_name,
  TGaussianBasisSet::GBSType gbs_type,
  const char* tag
) 
{
  vector<TGaussianBasisSet*> result;

  // look up basis set file information
  const gbs_alias* alias;
  alias = getGbsAlias(gbs_name, gbs_type);

  if (!alias) {
    // Check to make sure the gbs name was found:
    string errorMsg = "Basis set ";
           errorMsg += gbs_name; 
           errorMsg += " not found in library!";          
    EE_ASSERT(alias, EE_WARNING, errorMsg);
    return result;
  }
  
  string ns = p_ns;  // p_ns can't be string due to static init problems

  // set up general requests for GBS data:
  vector<MetaDataRequest> requests(4);
  requests[0].name = ns + "name";
  requests[1].name = ns + "type";
  requests[2].name = ns + "spherical";
  requests[3].name = ns + "contraction_type";
  
  // get files' contents & add each gbs to group
  TGaussianBasisSet* gbs = 0;
  
  // List of files (one file for each basis set if an aggregate group)
  const vector<char*>* file_list = &alias->files;
  if (!file_list) {
    EE_ASSERT(file_list, EE_WARNING, "files_list is null!");
    return result;
  }

#ifdef DEBUG
  cout << "Loading gbs: " << gbs_name << endl;
#endif

  // First we need to figure out if the basis set has an *-AGG.*
  // dummy file.  (Most of the aggregate basis sets have one, except
  // for ECPs and DFTs...although some of those have one too.)
  // If the basis set has an AGG file, then we want to IGNORE it
  // when mapping atoms lists to file names.
  // (THE DUMMY AGGREGATE PLACEHOLDER DOES NOT EXIST FOR MOST ECP ORBITAL AND
  //  DFT ORBITAL AGGREGATE GROUPS!  THIS IS INCONSISTENCY IS 
  //  BAD AND SHOULD BE CHANGED!)
  size_t numDummyFile = 0;     // dummy file will have "-AGG." in the name

  if (file_list->size() > 1 ) {
    string firstFileName = (*file_list)[0];
    if (firstFileName.find("-AGG.") != string::npos) {
      numDummyFile = 1;
    }
  }
  // loop through all the GBSs in the set, adding them to the list:
  for(size_t f = 0; f < file_list->size(); f++) {

    // skip the dummy aggregate placeholder file, if it exists:
    if (numDummyFile == 1 && f == 0) {
      continue;
    }

    // connect to server and get file
    string url = p_gbsURLBase.toString() + string("/") + (*file_list)[f];
#ifdef DEBUG
    cout << "file request " << url << endl;
#endif
    EDSI *edsi = EDSIFactory::getEDSI(url.c_str());
    vector<MetaDataResult> mDataResults;

    if (!edsi->getMetaData(requests,mDataResults) || mDataResults.size() == 0)
    {
      string msg = "Unable to retrieve meta data for: " + url;
      EE_RT_ASSERT(false, EE_WARNING, msg);
      continue;
    }
    istream* is = edsi->getDataSet();
    if (!is)
    {
      string msg = "Unable to read data file for: " + url;
      EE_RT_ASSERT(false, EE_WARNING, msg);
      continue;
    }
    delete edsi;

    char *name, *type, *spherical, *contraction_type;

    string ns = p_ns;  // p_ns can't be string due to static init problems

    // set the gbs characteristics
    for (size_t i = 0; i < mDataResults.size(); i++)
    {
      if (mDataResults[i].name == ns + "name")
	name=     strdup(mDataResults[i].value.c_str());
      else if (mDataResults[i].name == ns + "type")
	type=     strdup(mDataResults[i].value.c_str());
      else if (mDataResults[i].name == ns + "spherical")
	spherical= strdup(mDataResults[i].value.c_str());
      else if (mDataResults[i].name == ns + "contraction_type")
	contraction_type= strdup(mDataResults[i].value.c_str());
    }
#ifdef DEBUG
    cout << "file :             " << (*file_list)[f] << endl
	 << "name :             " << name << endl
	 << "type :             " << type << endl;
#endif
    // set the meta data properties:
    gbs = new TGaussianBasisSet;
    
    if( !strncmp(type,"ecp",3) )    // ECP
      setAttributes(gbs, name, type);
    else 
    {
      setAttributes(gbs, name, type, spherical, contraction_type);
      free (spherical);
      free (contraction_type);
    }
#ifdef DEBUG
    cout << "parse data begin" << endl;
#endif

    // Determine the right list of atoms supported by that basis set:
    const vector<char*>* atoms_list;

    // This is a backward compatible hack for old dav gbs format
    if (f-numDummyFile >= alias->atoms.size()) {
      atoms_list = &(alias->atoms[0]);  // Make it compatible w/ old data
                                        // where there was only 1 atom mapping
                                        // for all the files

    } else {
      atoms_list = &(alias->atoms[f-numDummyFile]);  // if there is a 
                                                     // dummy file, start at
                                                     // index 1, else 0  
    }

    // Determine the appropriate elements to extract based on the tag
    // and whether that basis set supports that tag:
    string supportedAtoms;
      
    if (tag == (char*) 0) // list of tags not provided, so get everything
      for (size_t i = 0; i < atoms_list->size(); i++) {
	if(i != 0)
	  supportedAtoms += " ";
	supportedAtoms += (*atoms_list)[i];
      }
    else {
      TGBSConfigTags tagAtoms(tag);
      for (size_t i = 0; i < atoms_list->size(); i++) {
	if (tagAtoms.findElement((*atoms_list)[i])) {
	  if (i != 0)
	    supportedAtoms += " ";
	  supportedAtoms += (*atoms_list)[i];
	}
      }
    }
    if(gbs->p_type == TGaussianBasisSet::ecp) 
      parseEcpData(gbs,is, supportedAtoms.c_str());
    else
      parseGbsData(gbs,is, supportedAtoms.c_str());
 
#ifdef DEBUG    
    cout << "inserting gbs: "  << gbs->p_name 
	 << " into list for: "<< gbs_name << endl;
    cout << "parse data end" << endl;
#endif

    result.push_back(gbs);    
    delete is;
    free (name);
    free (type);
  }

  return result;
}

/*******************************************************************
 Method : isOrbital
 Summary: static method that tells you if the basis set type
          represents an orbital basis set
*******************************************************************/
bool EDSIGaussianBasisSetLibrary::isOrbital(TGaussianBasisSet::GBSType type)
{
  if (type == TGaussianBasisSet::pople || 
      type == TGaussianBasisSet::other_segmented ||
      type == TGaussianBasisSet::correlation_consistent ||
      type == TGaussianBasisSet::other_generally_contracted ||
      type == TGaussianBasisSet::ECPOrbital ||
      type == TGaussianBasisSet::DFTOrbital )
    return true;
  else
    return false;
}

/*******************************************************************
 Method : setAttributes
 Summary: 
*******************************************************************/
void EDSIGaussianBasisSetLibrary::setAttributes(
  TGaussianBasisSet* gbs,
  const char* name,
  const char* type,
  const char* spherical,
  const char* contraction_type
) {

  gbs->p_name = name;
  if (spherical == 0)
    gbs->p_coordSys = TGaussianBasisSet::UnknownCoordSys;
  else
    gbs->p_coordSys = TGaussianBasisSet::strToCoordSys(spherical);
  if (contraction_type == 0)
    gbs->p_contType = TGaussianBasisSet::UnknownContType;
  else
     gbs->p_contType = TGaussianBasisSet::strToContType(contraction_type);
  gbs->p_type = TGaussianBasisSet::strToType(type);
  if (isOrbital(gbs->p_type))
    gbs->p_category = TGaussianBasisSet::ORBITAL;
  else
    gbs->p_category = TGaussianBasisSet::AUXILIARY;
}

/*******************************************************************
 Method : atoms
 Summary: Delete returned vector, but not the pointers that are
          in the vector.
*******************************************************************/
vector<const char*>* EDSIGaussianBasisSetLibrary::atoms(
                                                 const char* name,
                                 TGaussianBasisSet::GBSType type) 
{
  const gbs_alias* alias = getGbsAlias(name,type);
  vector<const char*>* ret = new vector<const char*>;
  for (size_t i = 0; i < alias->atoms[0].size(); i++)
     ret->push_back(alias->atoms[0][i]);

  return ret;
}

/*******************************************************************
 Method : details
 Summary: User is responsible for deleting returned pointer. 
*******************************************************************/
gbs_details* EDSIGaussianBasisSetLibrary::details(
                                           const char* gbs_name,
                                           TGaussianBasisSet::GBSType type) {
  gbs_details* result = 0;
  const gbs_alias* alias = getGbsAlias(gbs_name,type);

  if (!alias) {
 
    // Check to make sure the gbs name was found:
    string errorMsg = "Basis set ";
         errorMsg += gbs_name;
         errorMsg += " not found in library!  Meta data is missing.";
    EE_ASSERT(false, EE_WARNING, errorMsg);

  } else {

    // if type is an ecp augmenting basis set, then
    // lookup the alias for the ecp orbital basis set instead
    if(type == TGaussianBasisSet::ecp) {

      const char* file = alias->files[0];

      const vector<gbs_alias*>* alias_list = 
	getAliasList(TGaussianBasisSet::ECPOrbital);

      vector<gbs_alias*>::const_iterator alias_it;
      for(alias_it  = alias_list->begin();
	  alias_it != alias_list->end(); alias_it++) {

	vector<char*>::const_iterator file_it;
	for(file_it  = (*alias_it)->files.begin();
	    file_it != (*alias_it)->files.end(); file_it++) {
	  if( !strcmp(file, (*file_it)) ) {
	    alias = *alias_it;
	  }
	}
      }
    }

    result = new gbs_details;
    // details info contains :
    //             1. references
    //             2. info - description
    //             3. path to image file



    // Lookup this info in a backwards-compatible way:
    EDSI* edsi = EDSIFactory::getEDSI(p_gbsURLBase);
    bool   newWay = false;

    // First decide if we are using new data or old data:
    // (If *.meta file exists, then we are using new data)
    edsi->setURL(p_gbsURLBase.toString() + string("/") + alias->files[0] + 
                 ".meta");
    if (edsi->exists()) {
      newWay = true;
    }

    // Now get the data from property or file
    vector<char*>::const_iterator it;
    string url;
    
    for(it = alias->files.begin(); it != alias->files.end(); it++) {
      
      vector<MetaDataRequest>* requests;
      vector<MetaDataResult> mDataResults;      
    
      if (newWay) {
        requests = new vector<MetaDataRequest>(1);
      } else {
        requests = new vector<MetaDataRequest>(3);
      }

      string ns = p_ns;  // p_ns can't be string due to static init problems
      
      (*requests)[0].name = ns + "name";

      if (!newWay) {
        (*requests)[1].name = ns + "references";
        (*requests)[2].name = ns + "info";
      }

      // Get metadata from properties:
      url = p_gbsURLBase.toString() + string("/") + *it;
      edsi->setURL(url);

      if (!edsi->getMetaData(*requests, mDataResults))
	cerr << ("UNABLE_TO_READ" + edsi->getURL().toString()) << endl;
      
      // Now get metadata from file, if necessary:
      if (newWay) {

        url += ".meta";
        edsi->setURL(url);
        delete requests;
        requests = new vector<MetaDataRequest>(2);
        (*requests)[0].name = "references";
        (*requests)[1].name = "info";
        getBigProperty(edsi, *requests, mDataResults);
      }

      // evaluate meta data:
      string name;
      string reference;

      for (size_t i = 0; i < mDataResults.size(); i++) {
   
	// get name and references
	if (mDataResults[i].name == ns + "references" ||
            mDataResults[i].name == "references")
	  reference = mDataResults[i].value;
	else if (mDataResults[i].name == ns + "name")
	  name = mDataResults[i].value;
	else if (mDataResults[i].name == ns + "info" || 
                 mDataResults[i].name == "info")
	  // get info
	  if(it == alias->files.begin())
	    result->info = mDataResults[i].value.c_str();
      }
      if (reference != "") {
	result->reference += "References for: \"" + name + "\"\n";
	result->reference += reference + "\n";
      }
    }
    delete edsi;


    // get image name
    char file[128], full_path[128];
    int loc=0;
    int len = strlen(gbs_name);
    for (int j=0; j<len; j++) {
      if (gbs_name[j] == '.' ||
	  gbs_name[j] == '(' ||
	  gbs_name[j] == ')')
        ;      // eliminate these
      else if (gbs_name[j] == '*')
        file[loc++] = 'S';
      else if (gbs_name[j] == ' ' || gbs_name[j] == ',')
        file[loc++] = '-';
      else
        file[loc++] = gbs_name[j];
    }
    file[loc] = '\0';
    sprintf(full_path, "%s/data/client/bstimages/%s.gif", 
        Ecce::ecceHome(), file);
    result->image_path = full_path;

  }
    // not null if you get some data
    // not all data will be there for all basis sets.
  return result;

}

/*******************************************************************
 Method : comments
 Summary: 
*******************************************************************/
string EDSIGaussianBasisSetLibrary::comments( 
  const char* gbs_name,
  TGaussianBasisSet::GBSType type  ) 
{

  string result;
  const gbs_alias* alias = getGbsAlias(gbs_name, type);

  // if type is an ecp augmenting basis set, then
  // lookup the alias for the ecp orbital basis set instead
  if(type == TGaussianBasisSet::ecp) {

    // get the file for gbs_name
    const char* file = alias->files[0];
    const vector<gbs_alias*>* alias_list =
           getAliasList(TGaussianBasisSet::ECPOrbital);
    vector<gbs_alias*>::const_iterator alias_it;

    for(alias_it  = alias_list->begin();
        alias_it != alias_list->end(); alias_it++) {

      vector<char*>::const_iterator file_it;
      for(file_it  = (*alias_it)->files.begin();
          file_it != (*alias_it)->files.end(); file_it++) {
        if( !strcmp(file, (*file_it)) ) {
          alias = *alias_it;
        }
      }
    }
  }

  // Now concatenate all the comments from all the basis sets contained
  // in this aggregate group 
  //    (maintain backward compatibility)
  //    OLD WAY: all metadata saved as properties
  //    NEW WAY: big metadata (info, comments, references) saved in *.meta file
  //
  EDSI* edsi = EDSIFactory::getEDSI(p_gbsURLBase);
  bool   newWay = false;

  // First decide if we are using new data or old data:
  // (If *.meta file exists, then we are using new data)
  edsi->setURL(p_gbsURLBase.toString() + string("/") + alias->files[0] + 
               ".meta");
  if (edsi->exists()) {
    newWay = true;
  }

  vector<MetaDataRequest> requests(1);
  vector<MetaDataResult> mDataResults;
  vector<char*>::const_iterator it;
  string url;

  if (newWay) {
    requests[0].name = "comments";
  } else {
    requests[0].name = string(p_ns) + "comments";
  }

  for(it = alias->files.begin(); it != alias->files.end(); it++) {

    url = p_gbsURLBase.toString() + string("/") + *it;

    if (newWay) {
      url += ".meta";
    }
    edsi->setURL(url.c_str());
    
    if (newWay) {
      getBigProperty(edsi, requests, mDataResults);
      
    } else {
      edsi->getMetaData(requests,mDataResults);
    }
    
    // get name and references
    result += mDataResults[0].value;

    // error checking
    string msg = string("Comments not found for ") + url;
    EE_ASSERT(!mDataResults.empty(),EE_WARNING, msg);
  }

  delete edsi;
  return result;
}

void EDSIGaussianBasisSetLibrary::getBigProperty
(EDSI* edsi, 
 vector<MetaDataRequest>& requests,
 vector<MetaDataResult>& results) {

  istream* is = edsi->getDataSet();

  if (!is) {
    EE_RT_ASSERT(false, EE_WARNING, "Unable to read *.meta file");

  } else {
    
    char line[501];
    vector<MetaDataResult> tmp;
    MetaDataResult result;

    while(is->getline(line,500)) {
 
      result.name = "";
      for (size_t i = 0; i < requests.size(); i++) {
           
        if(requests[i].name == line) {
          result.name = line;
          break;
        }
      }
      if (result.name != "") { 
        result.value = "";

        while(is->getline(line,500) && line != result.name) {
          result.value += string(line) + "\n";
        }
        tmp.push_back(result);
      }
    }
    for (size_t j = 0; j < tmp.size(); j++) {
      results.push_back(tmp[j]);
    }
  }
}
        
/*******************************************************************
 Method : parseGbsData
 Summary: Loads up all the contractions for one gbs.
*******************************************************************/
void EDSIGaussianBasisSetLibrary::parseGbsData(
  TGaussianBasisSet* gbs,
  istream* is,
  const char* tag) 
{
  ContractionSet* contractions = 0;
  string atom_name = "";
  TGBSConfigTags* tags;

  if (tag != (char*) 0)
    tags = new TGBSConfigTags(tag);
  else
    tags = (TGBSConfigTags*) 0;

  char line[500];
  while(!is->eof()) {
    is->getline(line,500);

    if(strncmp(line,"atom=",5) == 0) {
      if (atom_name != "" && 
	  (tags == (TGBSConfigTags*) 0 || 
	   (tags != (TGBSConfigTags*) 0 && tags->findElement(atom_name)) ) )
      { 
	gbs->p_contractions[atom_name] = contractions;
      }
      contractions = new ContractionSet();
      atom_name = line+5;
    }

    else if(strncmp(line,"contraction",11) == 0) {
      Contraction_ *cont = new Contraction_;

      char buf[100];
      sscanf(line,"contraction shell=%s num_primitives=%zu num_coefficients=%zu",
        buf, &cont->num_exponents, &cont->num_coefficients);

      for(size_t i=0;i<cont->num_coefficients;i++) {
	if (i >= strlen(buf)) {	  
	  string msg = "--Shells list ";
	  msg += buf;
	  msg += " is not long enough.\nContraction has more";
	  msg += " coefficients than this.\n";
	  msg += "Notify Dave Feller - this is a typo.\n";
	  
	  EE_RT_ASSERT(false, EE_WARNING, msg);
	  int last = strlen(buf) - 1;
	  cont->shells.push_back(TGaussianBasisSet::charToShell(buf[last]));
	  
	} else {
	  cont->shells.push_back(TGaussianBasisSet::charToShell(buf[i]));
	}
      }
      cont->exponents = new double[cont->num_exponents];
      cont->coefficients = 
	new double[cont->num_coefficients*cont->num_exponents];

      int coefficient_counter=0;
      for(size_t primitive_counter=0;
          primitive_counter<cont->num_exponents;
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

  // add last contractions vector to the map
  if (atom_name != "" && 
      (tags == 0 || (tags != 0 && tags->findElement(atom_name)) )
     )
    gbs->p_contractions[atom_name]=contractions;
}

/*******************************************************************
 Method : parseEcpData
 Summary: 
*******************************************************************/
void EDSIGaussianBasisSetLibrary::parseEcpData(
  TGaussianBasisSet* ecp,
  istream* is,
  const char* tag) 
{
  ContractionSet* potential=0;
  char element_name[10] = "";
  char line[500];

  TGBSConfigTags* tags;

  if (tag != (char*) 0)
    tags = new TGBSConfigTags(tag);
  else
    tags = 0;

  while(!is->eof()) {
    is->getline(line,500);

    if(strncmp(line,"atom=",5) == 0) { // new element

      // add the previous contraction set to the map except for 1st element:
      if (strcmp(element_name, "") != 0 && 
	  (tags == 0 || (tags != 0 && tags->findElement(element_name)) )
	 )
	ecp->p_contractions[element_name] = potential;

      potential = new ContractionSet; // create new contraction set:
      sscanf(line,"atom=%s ncore=%d lmax=%d",
                   element_name,&potential->ncore,&potential->lmax);
    }
    else if(strncmp(line,"ecp_potential%",14) == 0) {  // new contraction
                                 // in same set, with additional ecp data
      Contraction_* ecpp = new Contraction_;

      char* tok = strtok(&line[14],"%");
      do {

        if( !strncmp(tok,"l=",2) )
          sscanf(&tok[2],"%zu",&ecpp->l);

        else if( !strncmp(tok,"shell=",6) ) {

	  // shells tok looks like this: "s-d POTENTIAL"
          string shellString = (&tok[6]);
          size_t potentialLocation = shellString.find("POTENTIAL");
          if (potentialLocation == string::npos)
	    potentialLocation = shellString.find("potential");
	  shellString = shellString.substr( 0, potentialLocation-1 );

	  for (size_t i = 0; i < shellString.size(); i++)
	    if (shellString[i] != '-' && shellString[i] != ' ')
	      ecpp->shells.push_back(
			    TGaussianBasisSet::charToShell(shellString[i]));
	}
        else if( !strncmp(tok,"num_exponents=",14) )
          sscanf(&tok[14],"%zu",&ecpp->num_exponents);

        tok = strtok((char*)0,"%");
      } while( tok );
        
      ecpp->num_coefficients=1;

      ecpp->powers = new int[ecpp->num_exponents];
      ecpp->exponents = new double[ecpp->num_exponents];
      ecpp->coefficients=new double[ecpp->num_exponents]; // num_coef is 1
      char* tail;

      for(size_t primitive_counter=0;
          primitive_counter<ecpp->num_exponents;
          primitive_counter++) {
        is->getline(line,500);

        ecpp->powers[primitive_counter] = (int)strtol(line, &tail, 10);
        ecpp->exponents[primitive_counter] = strtod(tail, &tail);
        ecpp->coefficients[primitive_counter] = strtod(tail, NULL);
      }
      potential->push_back(ecpp);
    } /* end potential */
  }
  // add last contractions set to the map (if one exists)
  if (strcmp(element_name, "") != 0 && 
      (tags == 0 || (tags != 0 && tags->findElement(element_name)) )
      )
    ecp->p_contractions[element_name] = potential;
}


//// GBS_ALIAS METHODS ////

/*******************************************************************
 Method : Constructor
 Summary: 
*******************************************************************/
gbs_alias::gbs_alias(void)
{
  name = 0;
  nicename=0;
  // files and atoms vectors are empty
}

/*******************************************************************
 Method : Copy Constructor
 Summary: 
*******************************************************************/
gbs_alias::gbs_alias(gbs_alias& alias)
{
  if (alias.name != (char*) 0)
    name = strdup(alias.name);
  else
    name = NULL;

  if (alias.nicename != (char*) 0)
    nicename = strdup(alias.nicename);
  else
    nicename = NULL;

  size_t i;
  for (i = 0; i < alias.files.size(); i++)
  {
    if (alias.files[i] != (char*) 0)
      files.push_back(strdup(alias.files[i]));
    else
      files.push_back((char*)NULL);
  }

  for (i = 0; i < alias.atoms.size(); i++)
  {
    vector<char*> tmpVector;

    for (size_t j = 0; j < alias.atoms[i].size(); j++) {
      if (alias.atoms[i][j] != (char*)NULL)
	tmpVector.push_back(strdup(alias.atoms[i][j]));
      else
	tmpVector.push_back((char*)NULL);
    }
    atoms.push_back(tmpVector);
  }
}

/*******************************************************************
 Method : Destructor
 Summary: 
*******************************************************************/
gbs_alias::~gbs_alias(void)
{
  if (name != NULL)
    delete name;

  if (nicename != NULL)
    delete nicename;

  size_t i;
  for (i = 0; i < files.size(); i++)
    if (files[i] != (char*) NULL)
      delete files[i];

  for (i = 0; i < atoms.size(); i++) {
    for (size_t j = 0; j < atoms[i].size(); j++) {
      if (atoms[i][j] != (char*)NULL)
	delete atoms[i][j];
    }
  }
}
   












