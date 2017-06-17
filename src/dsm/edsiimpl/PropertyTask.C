/**
 * @file
 *
 *
 */
#include <algorithm>
  using std::find;
#include <strstream>
  using std::ostrstream;
  using std::istrstream;
  using std::ends;
#include <vector>
  using std::vector;

#include <xercesc/util/XMLString.hpp>
using namespace xercesc;

#include "dsm/BasicDOMParser.H"
#include "dsm/BasicDOMWriter.H"
#include "dsm/EDSI.H"
#include "dsm/EDSIFactory.H"
#include "dsm/JCode.H"
#include "dsm/PropertyDoc.H"
#include "dsm/PropertyTask.H"
#include "dsm/PropFactory.H"
#include "dsm/ResourceType.H"
#include "dsm/Session.H"
#include "dsm/VDoc.H"

#include "tdat/PropString.H"
#include "tdat/PropTable.H"
#include "tdat/PropTSTable.H"
#include "tdat/PropTSVecString.H"
#include "tdat/PropTSVecTable.H"
#include "tdat/PropTSVector.H"
#include "tdat/PropTSVecVector.H"
#include "tdat/PropValue.H"
#include "tdat/PropVecString.H"
#include "tdat/PropVector.H"
#include "tdat/PropVecGrid.H"
#include "tdat/PropVecTable.H"
#include "tdat/TProperty.H"

#include "util/ErrMsg.H"
#include "util/ETimer.H"
#include "util/ResourceUtils.H"
#include "util/TypedFile.H"



/**
 * Constructor.
 */
PropertyTask::PropertyTask()
  : ChemistryTask()
  , IPropCalculation()
  , p_names()
  , p_properties()
{
  initializeXmlUtils();
}



/**
 * Destructor.
 */
PropertyTask::~PropertyTask() 
{
   flushPropertyCache(); 
}


/**
  * Loads up a vector of property names for all properties of this calc.
  * If the list of properties changes during the use
  * of this calculation, this list might not reflect current conditions.
  * This assumes the user will only
  * need to access names sequentially.  If name->property access is
  * required, this should be changed from a vector to a map.
  */
vector<string> PropertyTask::propertyNames()
{
   vector<string> names;
   vector<string>::iterator namesIt;
   string propName = "";
   vector<EcceURL> propertyUrls;

   ////////////////////////////////////////////////////////
   // Fill contents of names vector 
   // from the "Properties" collection
   // and from the Files collection (Property output files)
   ////////////////////////////////////////////////////////
   try {
     propertyUrls = getVDoc()->getProperties();
     for (size_t i = 0; i < propertyUrls.size(); i++) {
       propName = propertyUrls[i].getFilePathTail();
       if (propName != "scalars") {
         names.push_back(propName);
       }
     }
   }
   catch (...) {
     propertyUrls.clear();
   }

   // Get property output files, if any (e.g. CUBE)
   const JCode *codeCap = application();
   vector<TypedFile> codeFiles;
   if (codeCap) {
     codeFiles = codeCap->getCodeFiles(JCode::PROPERTY_OUTPUT);
     if (codeFiles.size() > 0) {
       vector<string> types = getOutputTypes();
       int numOutputs = types.size();
       int numCodeFiles = codeFiles.size();
       for (int i = 0; i < numOutputs; i++) { 
         string mimeType = types[i];
         for (int j = 0; j < numCodeFiles; j++) {
           if (mimeType == codeFiles[j].type()) {
             namesIt = find(names.begin(), names.end(), codeFiles[j].name());
             if (namesIt == names.end()) {
                names.push_back(codeFiles[j].name());
             }
           }
         }
       }
     }
   }

   // Calc info keys.  Only add them if our runstate is sufficient
   if (getState() > ResourceDescriptor::STATE_READY) {
      names.push_back("Application");
      names.push_back("Theory/Runtype");
      names.push_back("Basis Set");
      names.push_back("Launch Info");
      names.push_back("Run Statistics");
   }

   // Only report GEOMTRACE key if we have more than one step.
   vector<string>::iterator geokey =
          find(names.begin(), names.end(), "GEOMTRACE");
   if (geokey != names.end()) {
     PropTSVecTable *geotrace = (PropTSVecTable*)getProperty("GEOMTRACE");
     if (geotrace && geotrace->tables() <= 1) {
       names.erase(geokey);
     }
   }

   return names;
}


char* PropertyTask::getPropertyDoc(const string& name) 
{
  char* ret = NULL; 
  // Set the URL to the property:
  getEDSI()->setURL(getVDoc()->getProperty(name));

  // Clear message stack:
  p_msgStack->clear();

  ostrstream os;
  if (!getEDSI()->getDataSet(os)) 
  {
     p_msgStack->add(getEDSI()->m_msgStack);
  } else
  {
    os << ends;
    ret = os.str();
  }
  return ret;
}



/**
 * Updates the indicated property sent from a JMS message.
 * If the property does not currently exist in the cache
 * it will create the TProperty.
 */
TProperty* PropertyTask::updateProperty(const string& name, const string& value)
{
  TProperty *property = NULL;
  string localValue = value;
  bool isOutputFile = false;
  EcceURL fileUrl;


  // Check if property is in the map first:
  // Else, get the property from the dav server add it to the map 
  // and return so that we don't add a step twice.  All data should be 
  // on the DAV server prior to a message being sent.
  // 10/04/06 KLS Changed this to use getProperty instead of getTProperty
  // so that it does what the above comment says it does.
  property = getProperty(name);

  // If property type is bogus return with NULL value.
  if (!property) return 0;


  // This section of code is here to handle timing related issues with real
  // time monitoring of step data.  Note that the code only executes
  // for PropTSVector data.
  // Two potential problems occur:
  // 1) When a property is first loaded from the data server, it may have 
  //    picked up more steps and be ahead of messaging.  In this case, this 
  //    method should just return.  It used to append the data again leaving
  //    some duplicates at the beginning of the graph
  // 2) If the calcviewer is busy enough, particularly stuck processing such as
  //    updating trajectory files or waiting for a user to respond to a prompt,
  //    no events are processed.  Since we use UDP for messaging, data is lost.
  //    This chunk of code detects that and reloads the data from scratch.
  if (property) {
     if (property->classType() == TProperty::PROPTSVECTOR) {
        PropTSVector *tsvec = dynamic_cast<PropTSVector*>(property);
        // WARNING: we require this exact string to parse out the step number.
        // Extra spaces or whatever will break this code.  
        // Don't want to xml parse twice though.  The '13' is the length of the 
        // pattern + 1 for the presumed following double quote.
        // XML looks like: <step number="8356">2.66737E+04</step>
        size_t pos = value.find("step number=");
        if (pos != string::npos) {
           // have a step property
           char *tail = 0;
           const char *chunk = value.c_str();
           int step = (int)strtol(&chunk[pos+13],&tail,10); 

           // This can happen especially at applications startup time where
           // an updated came in, which forced getting the property which by 
           // then has the data in the message.  So just return
           if (tsvec->rows() >= step) return property;


           if (tsvec->rows()+1 < step) {
              // have to erase or getProperty fails....
              map<string, TProperty* , less<string> >::iterator cur = 
                 p_properties.find(name);
              p_properties.erase(cur);
              property = getProperty(name);
              if (property) {
                 // New one has been loaded and put in cache.  Delete old one
                 // and return without further processing - again to avoid
                 // double buffering
                 delete tsvec;
                 return property;
              } else {
                 // Put old one back and try again next time
                 property = tsvec;
                 putTProperty(name, tsvec);
              }
           }
        }

     }
  }



  //If property is not on DAV server then return null because this property is 
  //probably a time - series property that is being cached.
  // uhhhh what???
  if (localValue.empty()) {
    delete property;
    property = NULL;
    return property;
  }


  if (localValue.find("<?xml") == string::npos) {
    if (localValue.find("?start=") != string::npos) 
    {
      EcceURL valueUrl(localValue.substr(0, localValue.find("?") - 1));
      getEDSI()->setURL(valueUrl.getParent());
      //
      //  Parse the start and length parameters to get values 
      //
      string params = localValue.substr(localValue.find("?"), localValue.size()) ;   

      int startParamBegin = params.find("?start=") + strlen("?start=");
      int startParamEnd = params.find("&length=") - 1; 
      int lengthParamBegin = params.find("length=") + strlen("length=") ;
      int lengthParamEnd = params.find("&header=")  - 1 ;
      int headerParamBegin = params.find("header=") + strlen("header=");
      int headerParamEnd = params.size() - 1;

      int startParam = atoi(params.substr(startParamBegin, 
        startParamEnd - startParamBegin + 1).c_str());  
      int lengthParam = atoi(params.substr(lengthParamBegin, 
        lengthParamEnd - lengthParamBegin + 1).c_str());  
      ostrstream os;
      os << BasicDOMWriter::XMLDocumentDeclaration().c_str();
      os << params.substr(headerParamBegin, 
        headerParamEnd - headerParamBegin + 1);
      if (!getEDSI()->getDataSubSet(os, startParam, lengthParam)) {
        p_msgStack->add(getEDSI()->m_msgStack);
      } else {
        const Property_Ref* ref = PropFactory::getPropRef(name.c_str());
        os << "</step>";
        os << "</" << ref->storagetype().c_str() <<  ">" ; 
        os << ends;
        char* char_stream = os.str();
        localValue = char_stream;
        delete char_stream;
      }
    } 
    else {
      if (!isOutputFileProp(name)) {
        EcceURL valueUrl(localValue);
        getEDSI()->setURL(valueUrl);  
        ostrstream os;
        if (!getEDSI()->getDataSet(os)) {
          p_msgStack->add(getEDSI()->m_msgStack);
        } else {
          os << ends;
          char* char_stream = os.str();
          localValue = char_stream;
          delete char_stream;
        }
      }
      else {
        isOutputFile = true;
        fileUrl = localValue;
      }
    }
  }
  
  // This may change, but for now, PROPERTY_OUTPUT files are not parsed
  PropertyDoc doc;
  if (!isOutputFile) {
    doc.parse(localValue.c_str(), false);
  }

  if ( (doc.empty() && !isOutputFile) || !property ) {
    delete property;
    property = NULL;
  } 
  else {
    bool ok = true;
    // Retrieve the property contents:
    switch(property->classType())
    {
      case TProperty::PROPVALUE:
            ok=getPropValue((PropValue*) property, doc); break;
      case TProperty::PROPSTRING:
            ok=getPropString((PropString*) property, doc); break;
      case TProperty::PROPVECSTRING:
            ok=getPropVecString((PropVecString*) property, doc); break;
      case TProperty::PROPVECTOR:
            ok=getPropVector((PropVector*) property, doc); break;
      case TProperty::PROPTABLE:
            ok=getPropTable((PropTable*) property, doc); break;
      case TProperty::PROPVECTABLE:
            ok=getPropVecTable((PropVecTable*) property, doc); break;
      case TProperty::PROPVECGRID:
            ok=updatePropVecGrid((PropVecGrid*) property, fileUrl); break;
      case TProperty::PROPTSVECTABLE:
            ok=updatePropTSVecTable((PropTSVecTable*) property, doc); break;
      case TProperty::PROPTSVECSTRING:
            ok=updatePropTSVecString((PropTSVecString*) property, doc); break;
      case TProperty::PROPTSVECTOR:
            ok=updatePropTSVector((PropTSVector*) property, doc); break;
      case TProperty::PROPTSTABLE:
            ok=updatePropTSTable((PropTSTable*) property, doc); break;
      default: ok = false; break;
    };
    if (!ok) {
      delete property;
      property = NULL;
    } 
    else {
      putTProperty(name, property);
    }

  }
  return property;
}



/**
 * Get property from cache or if not in cache, load into cache and return.
 *
 * @param name - SHORTNAME of property (e.g. KEMDVEC) 
 * @return - property or possibly a 0 pointer if no such property is found
 *           or parsing of the data failed.  The latter normally happens
 *           if and update and read are occuring at the same time.
 */
TProperty* PropertyTask::getProperty(const string& name)
{
  TProperty* property = 0;
  bool isOutputFile = false;

  // Check if property is in the map first:
  property = getTProperty(name);

  // Else, get the property from the dav server and add it to the map:
  if (property) {
    return property;
  } 
  else {
    // NOTE may be 0 if unknown property
    // This should NOT happen unless it represents a property file
    property = PropFactory::getProperty(name);
  }

  // Determine if a prop file (in Properties collection) or an output 
  // property file (in Files collection)
  char* docString = NULL;

  if (!isOutputFileProp(name)) {
    docString = getPropertyDoc(name);
  }
  else {
    isOutputFile = true;
  }

  if (!docString && !isOutputFile) {
     if (property) {
        delete property;
        property = 0;
     }
    return property;
  } 

  // This may change, but for now, PROPERTY_OUTPUT files are not parsed
  PropertyDoc doc;
  if (!isOutputFile) {
    try {
    doc.parse(docString, false);
    } catch (...) {
       // This can happen if you request data while it is being updated.
       // The only recourse is to try again later.
       delete property;
       return 0;
    }
    delete [] docString;
  }

  if (doc.empty() && !isOutputFile) {
     if (property) {
        delete property;
        property = 0;
     }
  } 
  else if (property) {
    bool ok = true;
    // Retrieve the property contents:
    switch(property->classType()) {
      case TProperty::PROPVALUE:
              ok=getPropValue((PropValue*) property, doc); break;
      case TProperty::PROPSTRING:
              ok=getPropString((PropString*) property, doc); break;
      case TProperty::PROPVECSTRING:
              ok=getPropVecString((PropVecString*) property, doc); break;
      case TProperty::PROPVECTOR:
              ok=getPropVector((PropVector*) property, doc); break;
      case TProperty::PROPTABLE:
              ok=getPropTable((PropTable*) property, doc); break;
      case TProperty::PROPVECTABLE:
              ok=getPropVecTable((PropVecTable*) property, doc); break;
      case TProperty::PROPVECGRID:
              ok=getPropVecGrid((PropVecGrid*) property, name); break;
      case TProperty::PROPTSVECTABLE:
              ok=getPropTSVecTable((PropTSVecTable*) property, doc); break;
      case TProperty::PROPTSVECSTRING:
              ok=getPropTSVecString((PropTSVecString*) property, doc); break;
      case TProperty::PROPTSVECTOR:
              ok=getPropTSVector((PropTSVector*) property, doc); break;
      case TProperty::PROPTSTABLE:
              ok=getPropTSTable((PropTSTable*) property, doc); break;
      default: ok = false; break;
    };
    if (!ok) {
      delete property;
      property = NULL;
    } 
    else {
      putTProperty(name, property);
    }
  }
  return property;
}


bool PropertyTask::fragment(Fragment *frag)
{
  return ChemistryTask::fragment(frag);
}


Fragment* PropertyTask::fragment() const
{
  return ChemistryTask::fragment();
}


bool PropertyTask::getFragment(Fragment &frag)
{
  return ChemistryTask::getFragment(frag);
}


/** 
 * Loads the fragment into the fragment object and then overwrites
 * its coordinates with the coordinates of the specified step.
 * If step < 0, the last coordinates will be used.
 */
bool PropertyTask::getFragmentStep(Fragment *frag,int step)
{
  return loadStep(frag,step);
}



/**
 * Gets the fragment and then overwrites its coordinates
 * with the coordinates of the specified step.
 * If step < 0, the last coordinates will be used.
 */
Fragment* PropertyTask::getFragmentStep(int step)
{
   Fragment* frag = fragment() ;
   if (frag) {
     loadStep(frag,step);
   }
   return frag;
}



/**
 * Gets the fragment and then overwrites its coordinates
 * with the coordinates of the specified step.
 * If step < 0, the last coordinates will be used.
 */
Fragment* PropertyTask::getFragmentStep(int step, PropertyDoc& doc)
{
   Fragment* frag = fragment() ;
   if (frag) {
     loadStep(frag,step, doc);
   }
   return frag;
}


EcceURL PropertyTask::getURL() const
{
  return Resource::getURL();
}


bool PropertyTask::isReadOnly() const
{
  return isFragmentReadOnly()
          || getState() >= ResourceDescriptor::STATE_SUBMITTED;
}


bool PropertyTask::promptBeforeSave(Fragment *frag, string& msg, string& title)
{
  bool needPrompt = false;

  // begin special STTR logic
  Resource *parent = EDSIFactory::getResource(getURL().getParent());
  if (parent->getApplicationType() == ResourceDescriptor::AT_REACTION_STUDY) {
    string name = getName();
    // check if reactants or products have been defined yet
    Session *session = getSession();
    // force the linkbase to reload to be able to recognize newly
    // created reaction study component calculations
    session->loadLinkbase();
    vector<Resource*> roots = session->getRootMembers();
    if (name == "Transition-State") {
      size_t it;
      for (it = 0;
           it < roots.size()
           && roots[it]->getName() != "Reactant1"
           && roots[it]->getName() != "Product1";
           ++it);
      needPrompt = it < roots.size();
    } else if (name=="Reactant1"
            || name=="Reactant2"
            || name=="Product1"
            || name=="Product2") {
      needPrompt = true;
      // only check for the names we expect, all others just let
      // them pass through
    }

    if (needPrompt) {
      // yes, this is either a reactant or product, or a reactant or
      // product has been defined if it is a transition state
      needPrompt = frag->numAtoms() != p_atomTypes.size();
      if (!needPrompt) {
        vector<TAtm*> *atoms = frag->atoms();
        for (size_t it=0; it<frag->numAtoms(); ++it) {
          if ((*atoms)[it]->atomType() != p_atomTypes[it]) {
            needPrompt = true;
            break;
          }
        }
      }
    }

    if (needPrompt) {
      if (name == "Transition-State") {
        msg = "You have made changes to this reaction study"
              " transition state that may not be consistent"
              " with the reactant and product structures!"
              " Do you want to save these changes?";
        title = "ECCE Save Transition State Changes?";
      } else if (name == "Reactant1") {
        size_t it;
        for (it=0; it<roots.size() && roots[it]->getName()!="Reactant2"; ++it);
        if (it < roots.size()) {
          msg = "You have made changes to this reaction study"
                " reactant that may not be consistent with the"
                " other reactant and transition state structures!"
                " Do you want to save these changes?";
        } else {
          msg = "You have made changes to this reaction study"
                " reactant that may not be consistent with the"
                " transition state structure!"
                " Do you want to save these changes?";
        }
        title = "ECCE Save Reactant Changes?";
      } else if (name == "Reactant2") {
        msg = "You have made changes to this reaction study"
              " reactant that may not be consistent with the"
              " other reactant and transition state structures!"
              " Do you want to save these changes?";
        title = "ECCE Save Reactant Changes?";
      } else if (name == "Product1") {
        size_t it;
        for (it=0; it<roots.size() && roots[it]->getName()!="Product2"; ++it);
        if (it < roots.size()) {
          msg = "You have made changes to this reaction study"
                " product that may not be consistent with the"
                " other product and transition state structures!"
                " Do you want to save these changes?";
        } else {
          msg = "You have made changes to this reaction study"
                " product that may not be consistent with the"
                " transition state structure!"
                " Do you want to save these changes?";
        }
        title = "ECCE Save Product Changes?";
      } else if (name == "Product2") {
        msg = "You have made changes to this reaction study"
              " product that may not be consistent with the"
              " other product and transition state structures!"
              " Do you want to save these changes?";
        title = "ECCE Save Product Changes?";
      }
    }
  }

  return needPrompt;
}


/**
 * The fragment has its coordinates overwritten with data
 * from the specified step.  -1 means to use the last step.
 */
bool PropertyTask::loadStep(Fragment *frag, int step)
{
  bool ret = false;
  PropTSVecTable *geotrace = (PropTSVecTable*)getProperty("GEOMTRACE");
  if (geotrace) {
    // Reset the Fragment coords with the last step
    ret = true;
    int numStep = step;
    EE_ASSERT((step==-1|| step<geotrace->tables()),EE_WARNING,"Bad step #");
    if (numStep < 0) numStep = geotrace->tables()-1;
    size_t numRows = frag->numAtoms();
    const vector<double>& xvalues = geotrace->values(numStep);
    EE_ASSERT((3*numRows == xvalues.size()),EE_WARNING,"Bad Geomtrace");
    TAtm *atom;
    int ptr=0;
    for (size_t idx=0; idx<numRows; idx++) {
      atom = frag->atomRef(idx);
      atom->coordinates(xvalues[ptr],xvalues[ptr+1], xvalues[ptr+2]);
      ptr += 3;
    }

  }
  return ret;
}



/**
 * The fragment has its coordinates overwritten with data
 * from the specified step.  -1 means to use the last step.
 */
bool PropertyTask::loadStep(Fragment *frag, int step, PropertyDoc& doc)
{


  bool ret = false;
  TProperty *property = PropFactory::getProperty("GEOMTRACE");
  if (getPropTSVecTable((PropTSVecTable*) property, doc))
  {

    PropTSVecTable *geotrace = (PropTSVecTable*)property;
    if (geotrace)
    {
    // Reset the Fragment coords with the last step
      ret = true;
      int numStep = step;
      EE_ASSERT((step==-1|| step<geotrace->tables()),EE_WARNING,"Bad step #");
      if (numStep < 0) numStep = geotrace->tables()-1;
      size_t numRows = frag->numAtoms();
      const vector<double>& xvalues = geotrace->values(numStep);
       EE_ASSERT((3*numRows == xvalues.size()),EE_WARNING,"Bad Geomtrace");
      TAtm *atom;
      int ptr=0;
      for (size_t idx=0; idx<numRows; idx++)
      {
        atom = frag->atomRef(idx);
        atom->coordinates(xvalues[ptr],xvalues[ptr+1], xvalues[ptr+2]);
        ptr += 3;
      }
    }
  }
  return ret;
}



/**
 * Sets property* to NULL if property does not exist.
 * URL is set before this function is called.
 *
 * Since PropVector is a write-once class, the data is stored as the contents
 * of a single file on the dav server.  Because PropVector is write-once, it
 * is assumed that the entire property data will be present, and, hence, no
 * error checking is performed during the file read.
 */
bool PropertyTask::updatePropVector(PropVector* property, PropertyDoc&  doc)
{
  bool ret = true;
    //
    //Now that we have all data let's stuff it into a transient property.
    //
    char * cp = NULL;

    cp = doc.values();
    if (cp)
    {
      vector<double> valueVector;
      parseValues(cp, valueVector);
      property->values(valueVector);
      delete cp;
    }

   cp = doc.rows();
    //Validation checking to make sure essential parts of the property are present;
   long prop_size = property->values().size();
   if ((prop_size == 0) || (prop_size != atol(cp)))
     ret = false;
   delete cp;
  return ret;
}



/**
 * Sets property* to NULL if property does not exist.
 * URL is set before this function is called.
 *
 * Since PropVector is a write-once class, the data is stored as the contents
 * of a single file on the dav server.  Because PropVector is write-once, it
 * is assumed that the entire property data will be present, and, hence, no
 * error checking is performed during the file read.
 */
bool PropertyTask::getPropVector(PropVector* property, PropertyDoc&  doc)
{
   bool ret = true;

   //
   //Now that we have all data let's stuff it into a transient property.
   // 
   char * cp = doc.rowLabel();
   if (cp) {
      property->rowLabel(cp);
      delete cp;
   }

   cp = doc.units();
   if (cp) {
      property->units(cp);
      delete cp;
   }

   cp = doc.rowLabels();
   if (cp) { 
      vector<string> labels;
      parseValues(cp, labels);
      property->rowLabels(&labels); 
      delete cp;
   }

   cp = doc.values();
   if (cp) { 
      vector<double> valueVector;
      parseValues(cp, valueVector);
      property->values(valueVector); 
      delete cp;
   }

   // Validate - check to make sure essential parts of the property are present
   cp = doc.rows();
   long prop_size = property->values().size();
   if (property->type().empty()         || 
         (prop_size == 0)   ||
         (prop_size != atol(cp)) ) { 
      ret = false;
   }
   delete cp;

   return ret;
}



/**
 * Sets property* to NULL if property does not exist.
 * URL is set before this function is called.
 *
 * Since PropVecString is a write-once class, the data is stored as the contents
 * of a single file on the dav server.  Because PropVecString is write-once, it
 * is assumed that the entire property data will be present, and, hence, no
 * error checking is performed during the file read.
 */
bool PropertyTask::updatePropVecString(PropVecString* property , 
                                             PropertyDoc&  doc)
{
   bool ret = true;

   //
   //Now that we have all data let's stuff it into a transient property.
   //
   char *cp = doc.values();

   if (cp) {
      vector<string> valueVector;
      parseValues(cp, valueVector);
      property->values(valueVector);
      delete cp;
   }


   // Validate - check to make sure essential parts of the property are present
   cp = doc.rows();
   long prop_size = property->values().size();
   if ((prop_size == 0) ||
         (prop_size != atol(cp))) {
      ret = false;
   }
   delete cp;
   return ret;

}



/**
 * Sets property* to NULL if property does not exist.
 * URL is set before this function is called.
 *
 * Since PropVecString is a write-once class, the data is stored as the contents
 * of a single file on the dav server.  Because PropVecString is write-once, it
 * is assumed that the entire property data will be present, and, hence, no
 * error checking is performed during the file read.
 */
bool PropertyTask::getPropVecString(PropVecString* property , PropertyDoc&  doc)
{
   bool ret = true;

   //
   //Now that we have all data let's stuff it into a transient property.
   //
   char * cp = doc.rowLabel();
   if (cp) {
      property->rowLabel(cp);
      delete cp;
   }

   cp = doc.units();
   if (cp) {
      property->units(cp);
      delete cp;
   }

   cp = doc.values();
   if (cp) {
      vector<string> valueVector;
      parseValues(cp, valueVector);
      property->values(valueVector);
      delete cp;
   }

   cp = doc.rowLabels();
   if (cp) {
      vector<string> labelVector;
      parseValues(cp, labelVector);
      property->rowLabels(&labelVector);
      delete cp;
   } 

   // Validate - check to make sure essential parts of the property are present
   cp = doc.rows();
   long prop_size = property->values().size();
   if (property->type().empty()         || 
         (prop_size == 0) ||
         (prop_size != atol(cp))) { 
      ret = false;
   }
   delete cp;

   return ret;
}



/**
 * Sets property* to NULL if property does not exist.
 * URL is set before this function is called.
 * 
 * Since PropTable is a write-once class, the data is stored as the contents
 * of a single file on the dav server.  Because PropTable is write-once, it
 * is assumed that the entire property data will be present, and, hence, no
 * error checking is performed during the file read.
 */
bool PropertyTask::updatePropTable(PropTable* property, PropertyDoc&  doc)
{
   bool ret = true;

   // Clear message stack:
   p_msgStack->clear();

   //
   //Now that we have all data let's stuff it into a transient property.
   //
   char * cp = doc.rows();
   unsigned long numRows = atol(cp);
   delete cp;

   cp = doc.columns();
   unsigned long numColumns = atol(cp);
   delete cp;

   cp = doc.values();
   if (cp) {
      vector<double> valueVector;
      parseValues(cp, valueVector);
      property->values(numRows, numColumns, valueVector);
      delete cp;
   }

   // Validate - check to make sure essential parts of the property are present
   if ((property->values().size() == 0) ||
         (property->values().size() != (numRows*numColumns))) {
      ret = false;
   }

   return ret;
}



/**
 * Sets property* to NULL if property does not exist.
 * URL is set before this function is called.
 *
 * Since PropTable is a write-once class, the data is stored as the contents
 * of a single file on the dav server.  Because PropTable is write-once, it
 * is assumed that the entire property data will be present, and, hence, no
 * error checking is performed during the file read.
 */
bool PropertyTask::getPropTable(PropTable* property, PropertyDoc&  doc)
{
   bool ret = true;

   // Clear message stack:
   p_msgStack->clear();

   //
   //Now that we have all data let's stuff it into a transient property.
   //

   char * cp = doc.rowLabel();
   if (cp) {
      property->rowLabel(cp);
      delete cp;
   }

   cp = doc.units();
   if (cp) {
      property->units(cp);
      delete cp;
   }

   cp = doc.columnLabel();
   if (cp) {
      property->columnLabel(cp);
      delete cp;
   }

   cp = doc.rows();
   unsigned long numRows = atol(cp);
   delete cp;

   cp = doc.columns();
   unsigned long numColumns = atol(cp);
   delete cp;

   cp = doc.values();
   if (cp) {
      vector<double> valueVector;
      parseValues(cp, valueVector);
      property->values(numRows, numColumns, valueVector);
      delete cp;
   } 

   vector<string> rlabelVector;
   vector<string> clabelVector;

   cp = doc.rowLabels();
   if (cp) {
      parseValues(cp, rlabelVector);
      delete cp;
   }

   cp = doc.columnLabels();
   if (cp) {
      parseValues(cp, clabelVector);
      delete cp;
   }
   property->labels(&rlabelVector, &clabelVector);

   // Validate - check to make sure essential parts of the property are present
   if (property->type().empty()         || 
       (property->values().size() == 0) ||  
       (property->values().size() != (numRows*numColumns)) ) { 
      ret = false;
   }

   return ret;
}


/**
 * Sets property* to NULL if property does not exist.
 * URL is set before this function is called.
 *
 * PropVecTable properties are stored as a directory of tables, where
 * each table is written to a separate file.  PropVecTables are write-many,
 * so each table file may be appended separately over time.
 */
bool PropertyTask::updatePropVecTable(PropVecTable* property, 
                                            PropertyDoc&  doc)
{

  bool ret = true;

  // Clear message stack:
  p_msgStack->clear();

  //
  //Now that we have all data let's stuff it into a transient property.
  //
  char * cp = 0;

  cp = doc.rows();
  unsigned long numRows = atol(cp);
  delete cp;

  cp = doc.columns();
  unsigned long numColumns = atol(cp);
  delete cp;

  if (ret == true) {
    vector<vector<double> >* vVector = doc.doubleVecTable();
    if (vVector->size() > 0) {
      property->setValues(vVector, numRows, numColumns);
    } else {
      ret = false;
    }
  }

  return ret;
}



/**
 * Sets property* to NULL if property does not exist.
 * URL is set before this function is called.
 *
 * PropVecTable properties are stored as a directory of tables, where
 * each table is written to a separate file.  PropVecTables are write-many,
 * so each table file may be appended separately over time.
 */
bool PropertyTask::getPropVecTable(PropVecTable* property, PropertyDoc&  doc)
{
   bool ret = true;

   // Clear message stack:
   p_msgStack->clear();

   //
   //Now that we have all data let's stuff it into a transient property.
   //
   char * cp = 0;

   if (property->type().empty()) {
      ret = false; 

   } else {

      cp = doc.rowLabel();
      if (cp) {
         property->rowLabel(cp);
         delete cp;
      }

      cp = doc.units();
      if (cp) {
        property->units(cp);
        delete cp;
      }

      cp = doc.columnLabel();
      if (cp) {
         property->columnLabel(cp);
         delete cp;
      }

      vector<string> vlabelVector;
      vector<string> rlabelVector;
      vector<string> clabelVector;

      cp = doc.vectorLabels();
      if (cp) {
         parseValues(cp, vlabelVector);
         delete cp;
      }
      cp = doc.rowLabels();
      if (cp) {
         parseValues(cp, rlabelVector);
         delete cp;
      }
      cp = doc.columnLabels();
      if (cp) {
         parseValues(cp, clabelVector);
         delete cp;
      }

      property->labels(&rlabelVector, &clabelVector, &vlabelVector);

      vector<double> valueVector;

      cp = doc.rows();
      unsigned long numRows = atol(cp);
      delete cp;

      cp = doc.columns();
      unsigned long numColumns = atol(cp);
      delete cp;


      if (ret == true) {
         vector<vector<double> >* vVector = doc.doubleVecTable();
         if (vVector->size() > 0) {
            property->setValues(vVector, numRows, numColumns);
         } else {
            ret = false;
         }
      }

   }

   return ret;
}



/**
 * Adds property to a PROPVECGRID structure.  
 * 
 * @param[in,out] property Pointer to a PropVecGrid object
 * @name[in] fileUrl URL to output file property 
 *
 * @return Returns if property file was successfully added, 
 *         otherwise returns false.
 */
bool PropertyTask::updatePropVecGrid(PropVecGrid* property, 
                                           EcceURL fileUrl)
{
  bool ret = false;
  EcceURL origUrl = getURL();
  string fileName = fileUrl.getFilePathTail();
  
  ostrstream os;
  getEDSI()->setURL(fileUrl);  
  if (!getEDSI()->getDataSet(os)) {
    p_msgStack->add(getEDSI()->m_msgStack);
  } else {
    os << ends;
    char* propStream = os.str();
    istrstream is(propStream);
    ret = property->readGrid(is);
    delete propStream;
  }

  getEDSI()->setURL(origUrl);
  return ret;
}




/**
 * Gets all property output files and add to a PROPVECGRID structure.  
 * 
 * @param[in,out] property Pointer to a PropVecGrid object
 * @name[in] name Name of property ("short name" from properties file)
 *
 * @return Returns true if all property files were successfully added, 
 *         otherwise returns false.
 */
bool PropertyTask::getPropVecGrid(PropVecGrid* property, string name)
{
  bool ret = true;
  vector<EcceURL> propUrls;
  vector<string> propTypes;
  EcceURL propUrl;
  size_t i =0;

  // Get all PROPERTY_OUTPUT file URLs for property type (propName)
  const JCode *codeCap = application();
  vector<TypedFile> codeFiles;
  if (codeCap) {

    // Get Code's mime types for this property
    codeFiles = codeCap->getCodeFiles(JCode::PROPERTY_OUTPUT);
    if (codeFiles.size() > 0) {
      size_t numCodeFiles = codeFiles.size();
      for (i = 0; i < numCodeFiles; i++) {
        if (codeFiles[i].name() == name) {
          propTypes.push_back(codeFiles[i].type());
        }
      }
    }

    // Compare Code's mime type with data file's - if match then
    // it's what we're looking for
    vector<EcceURL> propOutputUrls = getDataFiles(JCode::PROPERTY_OUTPUT);
    for (i = 0; i < propTypes.size(); i++) {
      vector<EcceURL>::iterator propIt = propOutputUrls.begin();
      while (propIt != propOutputUrls.end()) {
        if (getMimeType(*propIt) == propTypes[i])  {
          propUrls.push_back(*propIt);
        }
        propIt++;
      }
    }

  }

  // Update PROPVECGRID with property output files
  for (i = 0; i < propUrls.size(); i++) {    
    if (!updatePropVecGrid(property, propUrls[i])) {
      ret = false;
    }
  }
  return ret;
}



/**
 * Sets property* to NULL if property does not exist.
 * URL is set before this function is called.
 *
 * PropTSTable properties are stored as a directory of tables, where
 * each table is written to a separate file.  PropTSTables are write-many,
 * so each table file may be appended separately over time.  This container
 * has not been implemented yet.
 */
bool PropertyTask::updatePropTSTable(PropTSTable* property, PropertyDoc&  doc)
{

  bool ret = false;
  return ret;
}


/**
 * Sets property* to NULL if property does not exist.
 * URL is set before this function is called.
 *
 * PropTSTable properties are stored as a directory of tables, where
 * each table is written to a separate file.  PropTSTables are write-many,
 * so each table file may be appended separately over time.  This container
 * has not been implemented yet.
 */
bool PropertyTask::getPropTSTable(PropTSTable* property, PropertyDoc& doc)
{

  bool ret = false;
  return ret;
}


/**
 * Sets property* to NULL if property does not exist.
 * URL is set before this function is called.
 *
 * PropTSVecTable properties are stored as a directory of tables, where
 * each table is written to a separate file.  PropTSVecTables are write-many,
 * so each table file may be appended separately over time.
 */
bool PropertyTask::updatePropTSVecTable(PropTSVecTable* property, 
                                              PropertyDoc&  doc)
{
   bool ret = true;

   //
   //Now that we have all data let's stuff it into a transient property.
   //
   char * cp = 0;

   cp = doc.rows();
   unsigned long numRows = atol(cp);
   delete cp;

   cp = doc.columns();
   unsigned long numColumns = atol(cp);
   delete cp;

   vector<double> valVec = doc.doubleVector(doc.lastStep() - 1);
   if (((numRows * numColumns) == valVec.size()) &&
         (valVec.size() > 0)) {
      property->appendTable(numRows, numColumns, valVec);
   } else {
      ret = false;
   }

   return ret;

}


/**
 * Sets property* to NULL if property does not exist.
 * URL is set before this function is called.
 *
 * PropTSVecTable properties are stored as a directory of tables, where
 * each table is written to a separate file.  PropTSVecTables are write-many,
 * so each table file may be appended separately over time.
 */
bool PropertyTask::getPropTSVecTable(PropTSVecTable* property, 
                                           PropertyDoc&  doc)
{
   bool ret = true;

   //
   //Now that we have all data let's stuff it into a transient property.
   //
   char * cp = 0;

   if (property->type().empty()) {
      ret = false; 
   } else {

      cp = doc.rowLabel();
      if (cp) {
         property->rowLabel(cp);
         delete cp;
      }

      cp = doc.columnLabel();
      if (cp) {
         property->columnLabel(cp);
         delete cp;
      }

      cp = doc.units();
      if (cp) {
         property->units(cp);
         delete cp;
      }

      vector<string> rlabelVector;
      vector<string> clabelVector;
      vector<string> vlabelVector;

      cp = doc.vectorLabels();
      if (cp) {
         parseValues(cp, vlabelVector);
         delete cp;
      }

      cp = doc.rowLabels();
      if (cp) {
         parseValues(cp, rlabelVector);
         delete cp;
      }

      cp = doc.columnLabels();
      if (cp) {
         parseValues(cp, clabelVector);
         delete cp;
      }

      property->labels(&rlabelVector, &clabelVector, &vlabelVector);

      vector<double> valueVector;
      cp = doc.rows();
      unsigned long numRows = atol(cp);
      delete cp;

      cp = doc.columns();
      unsigned long numColumns = atol(cp);
      delete cp;


      if (ret == true) {
         vector<vector<double> >* vVector = doc.doubleVecTable();
         if (vVector) {
            property->setValues(vVector, numRows, numColumns);
         } else{
            ret = false;
         }
      }
   }

   return ret;
}

/**
 * Sets property* to NULL if property does not exist.
 * URL is set before this function is called.
 *
 * PropTSVecStrings are stored as dav properties of a file.  Each string
 * in the vector can be appended as a new property.
 */
bool PropertyTask::updatePropTSVecString(PropTSVecString* property, 
                                               PropertyDoc&  doc)
{
   bool ret = true;

   //
   //Now that we have all data let's stuff it into a transient property.
   //
   char *cp = 0;

   // Validate check to make sure essential parts of the property are present
   unsigned long step = 0;
   cp =  doc.values(step);
   if (cp ) {
      property->appendValue(cp);
      delete cp;
   } else {
      ret = false;
   }

   return ret;

}

/**
 * Sets property* to NULL if property does not exist.
 * URL is set before this function is called.
 *
 * PropTSVecStrings are stored as dav properties of a file.  Each string
 * in the vector can be appended as a new property.
 */
bool PropertyTask::getPropTSVecString(PropTSVecString* property, 
                                            PropertyDoc&  doc)
{
   bool ret = true;


   unsigned long numRows = doc.lastStep();
   // Validate checke to make sure essential parts of the property are present
   if (property->type().empty() || 
       (numRows != 0)) {
      ret = false;
   }

   //
   //Now that we have all data let's stuff it into a transient property.
   //
   char * cp = 0;
   for (size_t idx = 1; idx < doc.lastStep(); idx++) {
      if ((cp = doc.values(idx))) {
         property->appendValue(cp);
         delete cp;
      }
   }

   return ret;
}


/**
 * Sets property* to NULL if property does not exist.
 * URL is set before this function is called.
 *
 * PropTSVectors are stored as dav properties of a file.  Each value
 * in the vector can be appended as a new property.
 */
bool PropertyTask::updatePropTSVector(PropTSVector* property , 
                                            PropertyDoc&  doc)
{
   bool ret = true;

   //
   //Now that we have all data let's stuff it into a transient property.
   //
   unsigned long step = 0;
   char *cp = doc.values(step);

   if (cp) {
      property->appendValue(atof(cp));
      delete cp;
   } else {
      ret = false;
   }

   return ret;
}



/**
 * Sets property* to NULL if property does not exist.
 * URL is set before this function is called.
 *
 * PropTSVectors are stored as dav properties of a file.  Each value
 * in the vector can be appended as a new property.
 */
bool PropertyTask::getPropTSVector(PropTSVector* property , 
                                         PropertyDoc&  doc)
{
   bool ret = true;

   //
   //Now that we have all data let's stuff it into a transient property.
   //
   char* cp = 0;

   cp = doc.units();
   if (cp) {
      property->units(cp);
      delete cp;
   }

   cp = doc.rowLabel();
   if (cp) {
      property->rowLabel(cp);
      delete cp;  // Added kls 3/04
   }

   cp = doc.rowLabels();
   if (cp) {
      vector<string> labelVector;
      parseValues(cp, labelVector);
      property->rowLabels(&labelVector);
      delete cp;
   }
   unsigned long numRows = doc.lastStep();

   // Validate check to make sure essential parts of the property are present
   if (property->type().empty()         ||
       (numRows == 0)) {
      ret = false;
   }
   vector<double> dvalues = doc.doubleVector();
   property->insertValues(dvalues);

   return ret;
}


/**
 * Sets property* to NULL if property does not exist.
 * URL is set before this function is called.
 *
 * PropStrings are stored as dav properties of the "scalars" file. Each
 * PropString is written as one separate dav property.
 */
bool PropertyTask::updatePropString(PropString* property, 
                                          PropertyDoc&  doc)
{
   bool ret = true;

   //
   //Now that we have all data let's stuff it into a transient property.
   //
   char* cp = doc.values();
   if (cp) {
      property->value(cp);
      delete cp;
   }

   return ret;
}



/**
 * Sets property* to NULL if property does not exist.
 * URL is set before this function is called.
 *
 * PropStrings are stored as dav properties of the "scalars" file. Each
 * PropString is written as one separate dav property.
 */
bool PropertyTask::getPropString(PropString* property, PropertyDoc&  doc)
{
   bool ret = true;

   //
   //Now that we have all data let's stuff it into a transient property.
   //
   char* cp = 0;

   cp = doc.values();
   if (cp) { 
      property->value(cp);
      delete cp;
   }
   cp = doc.units();
   if (cp) {
      property->units(cp);
      delete cp;
   }

   // Validate check to make sure essential parts of the property are present;
   if ( property->type().empty()) {
      ret = false;
   }

   return ret;
}



/**
 * Sets property* to NULL if property does not exist.
 * URL is set before this function is called.
 *
 * PropValues are stored as dav properties of the "scalars" file. Each
 * PropValue is written as one separate dav property.
 */
bool PropertyTask::updatePropValue(PropValue* property, PropertyDoc&  doc)
{
   bool ret = true;

   //
   //Now that we have all data let's stuff it into a transient property.
   //
   char* cp = doc.values();
   if (cp) {
      property->value(atof(cp));
      delete cp;
   }

   return ret;
}


/**
 * Sets property* to NULL if property does not exist.
 * URL is set before this function is called.
 *
 * PropValues are stored as dav properties of the "scalars" file. Each
 * PropValue is written as one separate dav property.
 */
bool PropertyTask::getPropValue(PropValue* property, PropertyDoc&  doc)
{
   bool ret = true;

   //
   //Now that we have all data let's stuff it into a transient property.
   //
   char* cp = doc.values();
   if (cp) {
      property->value(atof(cp));
      delete cp;
   }

   cp = doc.units();
   if (cp) {
      property->units(cp);
      delete cp;
   }

   // Validate check to make sure essential parts of the property are present
   if (property->type().empty()) {
      ret = false;
   }

   return ret;

}

/**
 * Does nothing right now....may never be implemented.
 */
bool PropertyTask::putProperty( TProperty* prop )
{
  EE_ASSERT(prop, EE_FATAL, "Property is null!")
  return true;
}



/**
 * Empty the property cache.
 */
void PropertyTask::flushPropertyCache()
{
   map<string, TProperty* , less<string> >::iterator cur = p_properties.begin();

   while (cur != p_properties.end()) {
     delete (*cur).second;
     cur++;
   }
   p_properties.clear();
}



/**
 * Deletes all the properties for the calculation and re-creates an
 * empty calcprop directory.  Returns false if an error occurs 
 * during deletion.
 */
bool PropertyTask::deleteProperties()
{
  bool ret = true;

  try {
    getVDoc()->removeProperties();
    flushPropertyCache();
  }
  catch (DavException& davException) {
     ret = false;
     p_msgStack->add("UNABLE_TO_COMPLETE_REQUEST",
                     davException.what());
  }

  return ret;
}



/**
 * Look for named property in the cache.
 * @return TProperty or a 0 pointer if not in cache.
 */
TProperty *PropertyTask::getTProperty (const string& name) 
{
  TProperty* ret = 0;

  ret = p_properties[name];  

  return ret;
}



void PropertyTask::putTProperty (const string& name, TProperty* property) 
{
  p_properties[name] = property;
}



void PropertyTask::parseValues(char* values, vector<double>& valVec) 
{
  char *token = strtok(values," \n\r"); 
  while (token) { 
    valVec.push_back(atof(token)); 
    token = strtok(NULL," \n\r"); 
  } 

}



void PropertyTask::parseValues(char* values, vector<string>& valVec) 
{
  char *token = strtok(values," \n\r"); 

  while (token) { 
    valVec.push_back(token); 
    token = strtok(NULL," \n\r"); 
  } 
}



bool PropertyTask::initializeXmlUtils()
{
  bool ret = true;
  try {
    XMLPlatformUtils::Initialize();

  } catch (const XMLException& toCatch) {
    string msg = "Error during initialization! :\n";
    char *xStr = XMLString::transcode(toCatch.getMessage());
    msg +=  xStr;
    delete [] xStr;
    EE_RT_ASSERT(false, EE_FATAL, msg);
    ret = false;
  }
  return ret;
}


/**
 * Determines if the property is an output file property (i.e. 
 * stored in the Files collection along with the other
 * output files of the calculation).
 *
 * @param[in] propName Name of the property, should be the
 *                     "short" name from the config/properties file.
 * @return Returns true if the property is an output file property. 
           otherwise returns false.
 * 
 */
bool PropertyTask::isOutputFileProp(const string& propName) 
{
  bool ret = false;
  const JCode *codeCap = application();
  vector<TypedFile> names = codeCap->getCodeFiles(JCode::PROPERTY_OUTPUT);
  size_t i;

  for (i = 0; i < names.size(); i++) {
    if (names[i].name() == propName) {
      ret = true;
      break;
    }
  }
  return ret;
}


bool PropertyTask::resetForRerun(const ResourceDescriptor::RUNSTATE& tostate)
{
  return deleteProperties() && TaskJob::resetForRerun(tostate);
}


long PropertyTask::getFragmentModifiedDate() const
{
  long ret = 0;

  MetaDataRequest request;
  vector<MetaDataRequest> requests;
  vector<MetaDataResult> molecule_results;
  vector<MetaDataResult> geomtrace_results;

  request.name = "DAV:getlastmodified";
  requests.push_back(request);

  getEDSI()->setURL(getVDoc()->getConfiguration(getVDoc()->getMoleculeName()));
  getEDSI()->getMetaData(requests, molecule_results);

  getEDSI()->setURL(getVDoc()->getProperty("GEOMTRACE"));
  getEDSI()->getMetaData(requests, geomtrace_results);

  if (molecule_results.size() > 0) {
    ret = TDateTime::toSeconds(molecule_results[0].value.c_str());
  }

  if (geomtrace_results.size() > 0) {
    long geomSecs = TDateTime::toSeconds(geomtrace_results[0].value.c_str());
    if (geomSecs > ret) {
      ret = geomSecs;
    }
  }

  return ret;
}

