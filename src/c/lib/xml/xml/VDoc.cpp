/*
* @file
*
*
*/

#include <sstream>
  using std::istringstream;

#include <string>
  using std::string;

#include <algorithm>
#include <vector>

#include <xercesc/util/XMLString.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
  using namespace xercesc;

#include "util/DavException.H"
#include "util/ParseException.H"
#include "util/InvalidException.H"
#include "util/TypedFile.H"
#include "util/StringConverter.H"
#include "util/ResourceUtils.H"
#include "dsm/JCode.H"
#include "dsm/CodeFactory.H"
#include "dsm/BasicDOMParser.H"
#include "dsm/EDSI.H"
#include "dsm/EDSIFactory.H" 
#include "dsm/VDoc.H"


static string p_ecceNamespace = "http://www.emsl.pnl.gov/ecce";
static string p_cmcsNamespace = "http://purl.oclc.org/NET/cmcs/schema/";
static string p_dcNamespace = "http://purl.org/dc/elements/1.1/";
static string p_dcQualifierNamespace = "http://purl.org/dc/terms/";



/**
 * Constructor.
 * 
 * @param url Address of VDoc
 * @param createVdoc If true, indicates that a new VDoc should be created.
 *        Otherwise, load the VDoc located at the provided Url. 
 *
 * @throw DavException Indicates problem with access to DAV server.
 * @throw InvalidException  Url is not recognized as a valid VDoc.
 *
 */
VDoc::VDoc(const EcceURL& url, bool createVdoc) 
  throw(DavException, InvalidException)
{
  // Set base URL
  p_base = url;

  // Establish DAV server connection
  p_edsi = EDSIFactory::getEDSI(p_base);
  if (p_edsi == 0) {
    throw DavException("DAV server connection failed", WHERE);
  }

  try {

    p_currentVdoc = true;

    if (!createVdoc) {
      // Determine if URL references a Vdoc
      if (!isVdoc()) {
        throw InvalidException("URL not recognized as a Virtual Document", WHERE);
      }

      // Determine if vdoc >= version 4.0
      if (!isCurrentVdoc()) {
        p_currentVdoc = false;
      }
    }

    // Initialize/Create VDoc
    setVdoc(createVdoc);

  }
  catch (...) {
    if (p_edsi != 0) {
      delete p_edsi;
    }
    throw;
  }
}


/**
 * Destructor.
 */
VDoc::~VDoc()
{
  if (p_edsi != 0) {
    delete p_edsi;
  }

}


/**
 * Gets standard Ecce Namespace.
 *
 * @return Ecce Namespace.
 *
 */
string VDoc::getEcceNamespace()
{
  return p_ecceNamespace;
}


/**
 * Gets standard CMCS Namespace.
 *
 * @return CMCS Namespace.
 *
 */
string VDoc::getCmcsNamespace()
{
  return p_cmcsNamespace;
}


/**
 * Gets Dublin Core Namespace
 *
 * @return Dublin Core Namespace.
 *
 */
string VDoc::getDcNamespace()
{
  return p_dcNamespace;
}


/**
 * Gets Dublin Core qualifier Namespace
 *
 * @return Dublin Core qualifier Namespace.
 *
 */
string VDoc::getDcQualifierNamespace()
{
  return p_dcQualifierNamespace;
}


/**
 * Modifies the VDoc's base Url.
 *
 * @param url New base Url
 *
 */
void VDoc::setURL(const EcceURL& url)
{
  p_base = url;
}


/**
 * Gets a VDoc's base Url.  All VDoc resources are relative to this Url. 
 *
 * @return VDoc's base Url.
 *
 */
EcceURL VDoc::getBaseURL() const
{
  return p_base;
}


/**
 * Get's the name of the molecule file for a VDoc.
 *
 * @return Name of molecule resource.
 *
 */
string VDoc::getMoleculeName() const
{
  return p_molecule;
}


/**
 * Get's the name of the original molecule file for a VDoc.
 *
 * @return Name of original molecule resource.
 *
 */
string VDoc::getOriginalMoleculeName() const
{
  return p_originalMolecule;
}



/**
 * Get's the name of the input molecule file for a VDoc.
 *
 * @return Name of input molecule resource.
 *
 */
string VDoc::getInputMoleculeName() const
{
  return p_inputMolecule;
}


/**
 * Get's the name of the output molecule file for a VDoc.
 *
 * @return Name of output molecule resource.
 *
 */
string VDoc::getOutputMoleculeName() const
{
  return p_outputMolecule;
}


/**
 * Get's the name of the ESP Constraints file for a VDoc.
 *
 * @return Name of ESP Constraints file.
 *
 */
string VDoc::getEspConstraintsName() const
{
  return p_espConstraints;
}


/**
 * Get's the name of the Geometry Constraints file for a VDoc.
 *
 * @return Name of Geometry Constraints file.
 *
 */
string VDoc::getGeometryConstraintsName() const
{
  return p_geometryConstraints;
}


/**
 * Get the name of the Meta-Potentials file for a VDoc.
 *
 * @return Name of Meta-Potentials file.
 *
 */
string VDoc::getMetaPotentialsName() const
{
  return p_metaPotentials;
}


/**
 * Get the name of the QMMM terms file for a VDoc.
 *
 * @return Name of QMMM terms file.
 *
 */
string VDoc::getQMMMTermsName() const
{
  return p_QMMMTerms;
}


/**
 * Get's the name of the Basis Set file for a VDoc.
 *
 * @return Name of Basis Set file.
 *
 */
string VDoc::getBasisSetName() const
{
  return p_basisSet;
}


/**
 * Get's the name of the General Basis Set file 
 * for a DirDyVTST's VDoc.
 *
 * @return Name of the general Basis Set file.
 *
 */
string VDoc::getGeneralBasisSetName() const
{
  return p_generalBasisSet;
}


/**
 * Get's the name of the Single Point Basis Set file 
 * for a DirDyVTST's VDoc.
 *
 * @return Name of the single point Basis Set file.
 *
 */
string VDoc::getSinglePointBasisSetName() const
{
  return p_singlePointBasisSet;
}


/**
 * Get's the name of the Ecce Jobstore log file (run log) for a VDoc.
 *
 * @return Name of run log file.
 *
 */
string VDoc::getRunLogName() const
{
  return p_runLog;
}


/**
 * Get's the name of the Setup Parameters file for a VDoc.
 *
 * @return Name of Setup Parameters file.
 *
 */
string VDoc::getSetupParametersName() const
{
  return p_setupParameters;
}


/**
 * Get's the name of the General Setup Parameters file 
 * for a DirDyVTST's  VDoc.
 *
 * @return Name of Setup Parameters file.
 *
 */
string VDoc::getGeneralSetupParametersName() const
{
  return p_generalSetupParameters;
}


/**
 * Get's the name of the Single Point Setup Parameters file 
 * for a DirDyVTST's  VDoc.
 *
 * @return Name of Setup Parameters file.
 *
 */
string VDoc::getSinglePointSetupParametersName() const
{
  return p_singlePointSetupParameters;
}


/**
 * Get's the name of the MD Prepare file for a VDoc.
 *
 * @return Name of the MD Prepare file.
 *
 */
string VDoc::getMdPrepareName() const
{
  return p_mdPrepare;
}


/**
 * Get's the name of the MD Optimize file for a VDoc.
 *
 * @return Name of the MD Optimize file.
 *
 */
string VDoc::getMdOptimizeName() const
{
  return p_mdOptimize;
}


/**
 * Get's the name of the MD Energy file for a VDoc.
 *
 * @return Name of the MD Energy file.
 *
 */
string VDoc::getMdEnergyName() const
{
  return p_mdEnergy;
}


/**
 * Get's the name of the MD Dynamics file for a VDoc.
 *
 * @return Name of the MD Dynamics file.
 *
 */
string VDoc::getMdDynamicsName() const
{
  return p_mdDynamics;
}


/**
 * Get's the name of the thumbnail file for a VDoc.
 *
 * @return Name of the thumbnail file.
 *
 */
string VDoc::getThumbnailName() const
{
  return p_thumbnail;
}


/**
 * Get's the name of the NWDirdy file for a VDoc.
 *
 * @return Name of the NWDirdy file.
 *
 */
string VDoc::getNwDirdyName() const
{
  return p_nwDirdy;
}


/**
 * Get's the name of the Polyrate file for a VDoc.
 *
 * @return Name of the Polyrate file.
 *
 */
string VDoc::getPolyrateName() const
{
  return p_polyrate;
}


/**
 * Constructs and returns Url for a configuration resource
 * using the name provided in the call.  
 *
 * @return Url of configuration resource.
 *
 */
EcceURL VDoc::getConfiguration(const string& name)
{
  return createConfigurationUrl(name);
}


/**
 * Returns all existing configuration resources associated with a
 * VDoc.  
 *
 * @throw DavException DAV server access error
 * @throw ParseException XML parse error
 *
 * @return Configuration resource Url's.
 */
vector<EcceURL> VDoc::getConfigurations() throw(DavException, ParseException)
{
  vector<EcceURL> urls;

  if (p_currentVdoc) {

    p_edsi->setURL(p_base);

    vector<MetaDataRequest> requests(1);
    requests[0].name = p_configurationsProp;
    vector<MetaDataResult> results;
    if (!p_edsi->getMetaData(requests, results)) {
      throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
    }
    else {
      int i;
      for (i = 0; i < results.size(); i++) {
        if (results[i].name == p_configurationsProp) {
          urls = getVdocUrls(parseDoc(results[i].value));
        }
      }
    }

  }
  else {

    vector<ResourceResult> results;
    EcceURL url = getVdocUrl(p_configurationCollection);

    p_edsi->setURL(url);
    p_edsi->listCollection(results);
    int i;
    for (i = 0; i < results.size(); i++) {
      urls.push_back(results[i].url);
    }
  }

  return urls;
}


/**
 * Constructs and returns Url for an Input resource
 * using the name provided in the call.  
 *
 * @return Url of input resource.
 *
 */
EcceURL VDoc::getInput(const string& name)
{
  return createInputUrl(name);
}


/**
 * Returns all existing Input resources associated with a
 * VDoc.  
 *
 * @throw DavException DAV server access error
 * @throw ParseException XML parse error
 *
 * @return Input resource Url's.
 */
vector<EcceURL> VDoc::getInputs() throw(DavException, ParseException)
{
  vector<EcceURL> urls;

  if (p_currentVdoc) {

    p_edsi->setURL(p_base);

    vector<MetaDataRequest> requests(1);
    requests[0].name = p_inputsProp;
    vector<MetaDataResult> results;
    if (!p_edsi->getMetaData(requests, results)) {
      throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
    }
    else {
      int i;
      for (i = 0; i < results.size(); i++) {
        if (results[i].name == p_inputsProp) {
          urls = getVdocUrls(parseDoc(results[i].value));
        }
      }
    }
  }
  else {

    vector<ResourceResult> results;
    EcceURL url = getVdocUrl(p_inputCollection);

    p_edsi->setURL(url);
    p_edsi->listCollection(results);
    int i;
    for (i = 0; i < results.size(); i++) {
      if ((isInputFilename(results[i].url.getFilePathTail())) ||
          (isInputType(results[i].url))) {
        urls.push_back(results[i].url);
      }
    }
  }
  return urls;
}


/**
 * Constructs and returns Url for an Output resource
 * using the name provided in the call.  
 *
 * @return Url of Output resource.
 *
 */
EcceURL VDoc::getOutput(const string& name)
{
  return createOutputUrl(name);
}


/**
 * Returns all existing Output resources associated with a
 * VDoc.  
 *
 * @throw DavException DAV server access error
 * @throw ParseException XML parse error
 *
 * @return Output resource Url's.
 */
vector<EcceURL> VDoc::getOutputs() throw(DavException, ParseException)
{
  vector<EcceURL> urls;

  /*
  if (p_currentVdoc) {

    p_edsi->setURL(p_base);

    vector<MetaDataRequest> requests(1);
    requests[0].name = p_outputsProp;
    vector<MetaDataResult> results;
    if (!p_edsi->getMetaData(requests, results)) {
      throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
    }
    else {
      int i;
      for (i = 0; i < results.size(); i++) {
        if (results[i].name == p_outputsProp) {
          urls = getVdocUrls(parseDoc(results[i].value));
        }
      }
    }
  }
  else {
  */

  if (p_currentVdoc) {
    vector<ResourceResult> results;
    EcceURL url = getVdocUrl(p_outputCollection);

    p_edsi->setURL(url);
    p_edsi->listCollection(results);
    int i;
    for (i = 0; i < results.size(); i++) {
      urls.push_back(results[i].url);
    }
  }
  else {
    vector<ResourceResult> results;
    EcceURL url = getVdocUrl(p_outputCollection);

    p_edsi->setURL(url);
    p_edsi->listCollection(results);
    int i;
    for (i = 0; i < results.size(); i++) {
      if ((isOutputFilename(results[i].url.getFilePathTail())) ||
          (isOutputType(results[i].url))) {
        urls.push_back(results[i].url);
      }
    }
  }

  return urls;
}


/**
 * Constructs and returns Url for a File resource
 * using the name provided in the call.  A File resource
 * may be either an Input, Output, or a misc. file that
 * has been stored in the VDoc.
 *
 * @return Url of File resource.
 *
 */
EcceURL VDoc::getFile(const string& name)
{
  EcceURL fileUrl;
  EcceURL *url;
  vector<EcceURL> iurls = getInputs();
  vector<EcceURL> ourls = getOutputs();

  if (isInputFilename(name)) {
    fileUrl = getInput(name);
  }
  else if ((url = findUrl(iurls, name))) {
    fileUrl = *url;
    delete url;
    url = 0;
  }
  else if (isOutputFilename(name)) {
    fileUrl = getOutput(name);
  }
  else if ((url = findUrl(ourls, name))) {
    fileUrl = *url;
    delete url;
    url = 0;
  }
  else {
    fileUrl = createFileUrl(name);
  }

  return fileUrl;
}


/**
 * Returns all existing File resources associated with a
 * VDoc.  This includes all Input, Output, and misc. File
 * resources.
 *
 * @throw DavException DAV server access error
 * @throw ParseException XML parse error
 *
 * @return File resource Url's.
 */
vector<EcceURL> VDoc::getFiles() throw(DavException, ParseException)
{
  vector<EcceURL>::iterator fileIt;
  int i;

  // Get input Url's
  vector<EcceURL> inputUrls = getInputs();

  // Get output Url's
  vector<EcceURL> outputUrls = getOutputs();

  // Get file Url's
  vector<EcceURL> fileUrls;
  vector<ResourceResult> results;
  p_edsi->setURL(getVdocUrl(p_fileCollection));
  p_edsi->listCollection(results);
  for (i = 0; i < results.size(); i++) {
    fileUrls.push_back(results[i].url);
  }

  // Add Inputs to return vector
  for (i = 0; i < inputUrls.size(); i++) { 
    fileIt = find(fileUrls.begin(), fileUrls.end(), inputUrls[i]);
    if (fileIt == fileUrls.end()) {
      fileUrls.push_back(inputUrls[i]);
    }
  }

  // Add Outputs to return vector
  for (i = 0; i < outputUrls.size(); i++) { 
    fileIt = find(fileUrls.begin(), fileUrls.end(), outputUrls[i]);
    if (fileIt == fileUrls.end()) {
      fileUrls.push_back(outputUrls[i]);
    }
  }

  return fileUrls;
}


/**
 * Constructs and returns Url for a Property resource
 * using the name provided in the call.  
 *
 * @return Url of Property resource.
 *
 */
EcceURL VDoc::getProperty(const string& name)
{
  return createPropertyUrl(name);
}


/**
 * Returns all existing Property resources associated with a
 * VDoc.  
 *
 * @throw DavException DAV server access error
 * @throw ParseException XML parse error
 *
 * @return Property resource Url's.
 */
vector<EcceURL> VDoc::getProperties() throw(DavException, ParseException)
{
  vector<EcceURL> propertyUrls;

  vector<ResourceResult> results;
  p_edsi->setURL(getVdocUrl(p_propertyCollection));
  p_edsi->listCollection(results);
  int i;
  for (i = 0; i < results.size(); i++) {
    propertyUrls.push_back(results[i].url);
  }

  return propertyUrls;
}


/**
 * Adds a Configuration resource to the VDoc.  If the resource exists it will
 * be overwritten.
 *
 * @param name Name of resource.
 * @param resource Input stream (resource contents).
 *
 * @throw DavException DAV server access error
 *
 * @return Url of added resource.
 */
EcceURL VDoc::addConfiguration(
                const string& name, 
                istream* resource) 
                throw(DavException)
{
  bool isNewResource = true;
  EcceURL configurationUrl;

  vector<EcceURL> urls = getConfigurations();
  EcceURL *url = findUrl(urls, name);
  if (url != 0) {
    isNewResource = false;
    configurationUrl = *url;
    delete url;
    url = 0;
  }
  else {
    configurationUrl = createConfigurationUrl(name);
  }

  p_edsi->setURL(configurationUrl);

  if (resource != 0) {
    if (!p_edsi->putDataSet(*resource)) {
      throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
    }
  }
  else {
    if (!p_edsi->putDataSet("")) {
      throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
    }
  }

  if (isNewResource) {
    // saveConfigurations() may throw an exception, the addConfiguration() call
    // will be considered as having failed if this occurs.
    urls.push_back(configurationUrl);
    saveConfigurations(urls);
  }

  return configurationUrl;
}


/**
 * Adds an Input resource to the VDoc.  If the resource exists it will
 * be overwritten.
 *
 * @param name Name of resource.
 * @param resource Input stream (resource contents).
 *
 * @throw DavException DAV server access error
 *
 * @return Url of added resource.
 */
EcceURL VDoc::addInput(
                const string& name, 
                istream* resource) 
                throw(DavException)
{
  bool isNewResource = true;
  EcceURL inputUrl;

  vector<EcceURL> urls = getInputs();
  EcceURL *url = findUrl(urls, name);
  if (url != 0) {
    isNewResource = false;
    inputUrl = *url;
    delete url;
    url = 0;
  }
  else {
    inputUrl = createInputUrl(name);
  }

  p_edsi->setURL(inputUrl);

  if (resource != 0) {
    if (!p_edsi->putDataSet(*resource)) {
      throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
    }
  }
  else {
    if (!p_edsi->putDataSet("")) {
      throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
    }
  }

  if (isNewResource) {
    // saveInputs() may throw an exception, the addInput() call
    // will be considered as having failed if this occurs.
    urls.push_back(inputUrl);
    saveInputs(urls);
  }

  return inputUrl;
}


/**
 * Adds an Output resource to the VDoc.  If the resource exists it will
 * be overwritten.
 *
 * @param name Name of resource.
 * @param resource Input stream (resource contents).
 *
 * @throw DavException DAV server access error
 *
 * @return Url of added resource.
 */
EcceURL VDoc::addOutput(
                const string& name, 
                istream* resource) 
                throw(DavException)
{
  EcceURL outputUrl;

#if 000
  bool isNewResource = true;

  // This code is not being executed because we currently don't use
  // hasOutputs. If we change and go back to using this, then we'll need
  // this code.  Actually, it's a big performance liability because as
  // the number of files increases, the time to execute findUrl goes
  // up as well.  Thus, this operation happens instantaneously with only
  // a few output files, but can take many seconds if there are 50+ files.
  vector<EcceURL> urls = getOutputs();
  EcceURL *url = findUrl(urls, name);
  if (url != 0) {
    isNewResource = false;
    outputUrl = *url;
    delete url;
    url = 0;
  }
  else {
    outputUrl = createOutputUrl(name);
  }
#else
  outputUrl = createOutputUrl(name);
#endif

  p_edsi->setURL(outputUrl);

  if (resource != 0) {
    if (!p_edsi->putDataSet(*resource)) {
      throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
    }
  }
  else {
    if (!p_edsi->putDataSet("")) {
      throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
    }
  }

#if 000
  // See commend above to see why this is commented out.
  if (isNewResource) {
    // saveOutputs() may throw an exception, the addOutput() call
    // will be considered as having failed if this occurs.
    urls.push_back(outputUrl);
    saveOutputs(urls);
  }
#endif

  return outputUrl;
}


/**
 * Adds a File resource to the VDoc.  If the resource exists it will
 * be overwritten. A File resource may be either an Input, Output, 
 * or misc. resource.  
 *
 * @param name Name of resource.
 * @param resource Input stream (resource contents).
 *
 * @throw DavException DAV server access error
 *
 * @return Url of added resource.
 */
EcceURL VDoc::addFile(
                const string& name, 
                istream* resource) 
                throw(DavException)
{
  EcceURL fileUrl;

  if (isInputFilename(name)) {
    fileUrl = addInput(name, resource);
  }
  else if (isOutputFilename(name)) {
    fileUrl = addOutput(name, resource);
  }
  else {
    fileUrl = createFileUrl(name);
    p_edsi->setURL(fileUrl);

    if (resource != 0) {
      if (!p_edsi->putDataSet(*resource)) {
        throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
      }
    }
    else {
      if (!p_edsi->putDataSet("")) {
        throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
      }
    }
  }

  return fileUrl;

}


/**
 * Adds a Property resource to the VDoc.  If the resource exists it will
 * be overwritten.
 *
 * @param name Name of resource.
 * @param resource Input stream (resource contents).
 *
 * @throw DavException DAV server access error
 *
 * @return Url of added resource.
 */
EcceURL VDoc::addProperty(
                const string& name, 
                istream* resource) 
                throw(DavException)
{
  EcceURL propertyUrl;

  propertyUrl = createPropertyUrl(name);
  p_edsi->setURL(propertyUrl);

  if (resource != 0) {
    if (!p_edsi->putDataSet(*resource)) {
      throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
    }
  }
  else {
    if (!p_edsi->putDataSet("")) {
      throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
    }
  }

  return propertyUrl;
}


/**
 * Moves a Configuration resource in the VDoc.  If the source resource
 * does not exist an exception is thrown.  If the target resource 
 * exists, a unique name is generated so the existing configuration
 * resource will not be overwritten.
 *
 * @param fromName Name of source resource.
 * @param toName Name of target resource.
 *
 * @throw DavException DAV server access error or if the From 
 *        resource does not exist.
 *
 * @return Url of moved resource.
 */
EcceURL VDoc::moveConfiguration(
                const string& fromName, 
                const string& toName) 
                throw(DavException)
{
  EcceURL *url;
  EcceURL fromUrl;
  EcceURL toUrl;
  vector<EcceURL> urls = getConfigurations();

  url = findUrl(urls, fromName);
  if (url == 0) {
    throw DavException("Move failed - FROM resource does not exist", WHERE);
  }
  else {
    fromUrl = *url;
    delete url;
    url = 0;
  }

  url = findUrl(urls, toName);
  if (url != 0) {
    EcceURL parentUrl = url->getParent();
    p_edsi->setURL(parentUrl);
    string newName = p_edsi->uniqueName(url->getFilePathTail());
    toUrl = parentUrl.getChild(newName.c_str());
    delete url;
    url = 0;
  }
  else {
    toUrl = createConfigurationUrl(toName);
  }

  p_edsi->setURL(fromUrl);

  if (p_currentVdoc) {

    if (!p_edsi->copyResource(toUrl)) {
      throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
    }

    int i;
    for (i = 0; i < urls.size(); i++) {
      if (fromUrl == urls[i]) {
        urls[i] = toUrl;
        break;
      }
    }
    // saveConfigurations() may throw an exception, the moveConfiguration() call
    // will be considered as having failed if this occurs.
    saveConfigurations(urls);

    // Can now go ahead and remove the From resource 
    p_edsi->setURL(fromUrl);
    p_edsi->removeResource();

  }
  else {
    if (!p_edsi->moveResource(toUrl)) {
      throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
    }
  }

  return toUrl;
}


/**
 * Moves an Input resource in the VDoc.  If the source resource
 * does not exist an exception is thrown.  If the target resource 
 * exists, a unique name is generated so the existing configuration
 * resource will not be overwritten.
 *
 * @param fromName Name of source resource.
 * @param toName Name of target resource.
 *
 * @throw DavException DAV server access error or if the From 
 *        resource does not exist.
 *
 * @return Url of moved resource.
 */
EcceURL VDoc::moveInput(
                const string& fromName, 
                const string& toName) 
                throw(DavException)
{
  EcceURL *url;
  EcceURL fromUrl;
  EcceURL toUrl;
  vector<EcceURL> urls = getInputs();

  url = findUrl(urls, fromName);
  if (url == 0) {
    throw DavException("Move failed - FROM resource does not exist", WHERE);
  }
  else {
    fromUrl = *url;
    delete url;
    url = 0;
  }

  url = findUrl(urls, toName);
  if (url != 0) {
    EcceURL parentUrl = url->getParent();
    p_edsi->setURL(parentUrl);
    string newName = p_edsi->uniqueName(url->getFilePathTail());
    toUrl = parentUrl.getChild(newName.c_str());
    delete url;
    url = 0;
  }
  else {
    toUrl = createInputUrl(toName);
  }

  p_edsi->setURL(fromUrl);

  if (p_currentVdoc) {

    if (!p_edsi->copyResource(toUrl)) {
      throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
    }

    int i;
    for (i = 0; i < urls.size(); i++) {
      if (fromUrl == urls[i]) {
        urls[i] = toUrl;
        break;
      }
    }
    // saveInputs() may throw an exception, the moveInput() call
    // will be considered as having failed if this occurs.
    saveInputs(urls);

    // Can now go ahead and remove the From resource 
    p_edsi->setURL(fromUrl);
    p_edsi->removeResource();

  }
  else {
    if (!p_edsi->moveResource(toUrl)) {
      throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
    }
  }

  return toUrl;
}


/**
 * Moves an Output resource in the VDoc.  If the source resource
 * does not exist an exception is thrown.  If the target resource 
 * exists, a unique name is generated so the existing configuration
 * resource will not be overwritten.
 *
 * @param fromName Name of source resource.
 * @param toName Name of target resource.
 *
 * @throw DavException DAV server access error or if the From 
 *        resource does not exist.
 *
 * @return Url of moved resource.
 */
EcceURL VDoc::moveOutput(
                const string& fromName, 
                const string& toName) 
                throw(DavException)
{
  EcceURL *url;
  EcceURL fromUrl;
  EcceURL toUrl;
  vector<EcceURL> urls = getOutputs();

  url = findUrl(urls, fromName);
  if (url == 0) {
    string msg = "Move failed - FROM resource (" + fromName +
                 ") does not exist";
    throw DavException(msg, WHERE);
  }
  else {
    fromUrl = *url;
    delete url;
    url = 0;
  }

  url = findUrl(urls, toName);
  if (url != 0) {
    EcceURL parentUrl = url->getParent();
    p_edsi->setURL(parentUrl);
    string newName = p_edsi->uniqueName(url->getFilePathTail());
    toUrl = parentUrl.getChild(newName.c_str());
    delete url;
    url = 0;
  }
  else {
    toUrl = createOutputUrl(toName);
  }

  p_edsi->setURL(fromUrl);

  if (p_currentVdoc) {

    if (!p_edsi->copyResource(toUrl)) {
      throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
    }

    int i;
    for (i = 0; i < urls.size(); i++) {
      if (fromUrl == urls[i]) {
        urls[i] = toUrl;
        break;
      }
    }
    // saveOutputs() may throw an exception, the moveOutput() call
    // will be considered as having failed if this occurs.
    saveOutputs(urls);

    // Can now go ahead and remove the From resource 
    p_edsi->setURL(fromUrl);
    p_edsi->removeResource();

  }
  else {
    if (!p_edsi->moveResource(toUrl)) {
      throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
    }
  }

  return toUrl;
}


/**
 * Moves a File resource in the VDoc.  A File resource may
 * be either an Input, Output, or a misc. file resource.
 * If the source resource  does not exist an 
 * exception is thrown.  If the target resource 
 * exists, a unique name is generated so the existing configuration
 * resource will not be overwritten.
 *
 * @param fromName Name of source resource.
 * @param toName Name of target resource.
 *
 * @throw DavException DAV server access error or if the From 
 *        resource does not exist.
 *
 * @return Url of moved resource.
 */
EcceURL VDoc::moveFile(
                const string& fromName, 
                const string& toName) 
                throw(DavException)
{
  EcceURL fromUrl;
  EcceURL toUrl;
  EcceURL *url = 0;
  EcceURL *inputUrl = findUrl(getInputs(), fromName);
  EcceURL *outputUrl = findUrl(getOutputs(), fromName);

  try {
    if (inputUrl != 0) {
      delete inputUrl;
      inputUrl = 0;
      toUrl = moveInput(fromName, toName);
    }
    else if (outputUrl != 0) {
      delete outputUrl;
      outputUrl = 0;
      toUrl = moveOutput(fromName, toName);
    }
    else {
      vector<EcceURL> urls = getFiles();

      url = findUrl(urls, fromName);
      if (url ==0) {
        throw DavException("Move failed - FROM resource does not exist", WHERE);
      }
      else {
        fromUrl = *url;
        delete url;
        url = 0;
      }

      url = findUrl(urls, toName);
      if (url != 0) {
        EcceURL parentUrl = url->getParent();
        p_edsi->setURL(parentUrl);
        string newName = p_edsi->uniqueName(url->getFilePathTail());
        toUrl = parentUrl.getChild(newName.c_str());
        delete url;
        url = 0;
      }
      else {
        toUrl = createFileUrl(toName);
      }

      p_edsi->setURL(fromUrl);
      if (!p_edsi->moveResource(toUrl)) {
        throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
      }
    }
  }
  catch (DavException&) {
    if (inputUrl != 0) {
      delete inputUrl;
      inputUrl = 0;
    }
    if (outputUrl != 0) {
      delete outputUrl;
      outputUrl = 0;
    }
    throw;
  }
  if (inputUrl != 0) {
    delete inputUrl;
    inputUrl = 0;
  }
  if (outputUrl != 0) {
    delete outputUrl;
    outputUrl = 0;
  }

  return toUrl;
}


/**
 * Moves a Property resource in the VDoc.  If the source resource
 * does not exist an exception is thrown.  If the target resource 
 * exists, a unique name is generated so the existing configuration
 * resource will not be overwritten.
 *
 * @param fromName Name of source resource.
 * @param toName Name of target resource.
 *
 * @throw DavException DAV server access error or if the From 
 *        resource does not exist.
 *
 * @return Url of moved resource.
 */
EcceURL VDoc::moveProperty(
                const string& fromName, 
                const string& toName) 
                throw(DavException)
{
  EcceURL *url;
  EcceURL fromUrl;
  EcceURL toUrl;
  vector<EcceURL> urls = getProperties();

  url = findUrl(urls, fromName);
  if (url == 0) {
    throw DavException("Move failed - FROM resource does not exist", WHERE);
  }
  else {
    fromUrl = *url;
    delete url;
    url = 0;
  }

  url = findUrl(urls, toName);
  if (url != 0) {
    EcceURL parentUrl = url->getParent();
    p_edsi->setURL(parentUrl);
    string newName = p_edsi->uniqueName(url->getFilePathTail());
    toUrl = parentUrl.getChild(newName.c_str());
    delete url;
    url = 0;
  }
  else {
    toUrl = createPropertyUrl(toName);
  }

  p_edsi->setURL(fromUrl);
  if (!p_edsi->moveResource(toUrl)) {
    throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
  }

  return toUrl;
}


/**
 * Removes a Configuration resource from the VDoc.
 *
 * @param name Name of resource.
 *
 * @throw DavException DAV server access error
 *
 */
void VDoc::removeConfiguration(const string& name) throw(DavException)
{
  vector<EcceURL> urls = getConfigurations();
  EcceURL *url = findUrl(urls, name);

  if (url != 0) {
    EcceURL configurationUrl = *url;
    delete url;
    url = 0;

    p_edsi->setURL(configurationUrl);

    if (p_currentVdoc) {

      vector<EcceURL>::iterator urlIt = urls.begin();
      while (urlIt != urls.end()) {
        if (configurationUrl == (*urlIt)) {
          urls.erase(urlIt);
          urlIt = urls.end();
        }
        else {
          urlIt++;
        }
      }

      saveConfigurations(urls);
      p_edsi->removeResource();

    }
    else {
      if (!p_edsi->removeResource()) {
        throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
      }
    }
  }
}


/**
 * Removes all Configuration resources from the VDoc.
 *
 * @throw DavException DAV server access error
 *
 */
void VDoc::removeConfigurations() throw(DavException)
{
  vector<EcceURL> urls = getConfigurations();
  vector<EcceURL> noUrls;

  if (p_currentVdoc) {
    noUrls.clear();
    saveConfigurations(noUrls);
    p_edsi->removeResources(urls);
  }
  else {
    if (!p_edsi->removeResources(urls)) {
      throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
    }
  }
}


/**
 * Removes an Input resource from the VDoc.
 *
 * @param name Name of resource.
 *
 * @throw DavException DAV server access error
 *
 */
void VDoc::removeInput(const string& name) throw(DavException)
{
  vector<EcceURL> urls = getInputs();
  EcceURL *url = findUrl(urls, name);

  if (url != 0) {
    EcceURL inputUrl = *url;
    delete url;
    url = 0;

    if (p_currentVdoc) {

      vector<EcceURL>::iterator urlIt = urls.begin();
      while (urlIt != urls.end()) {
        if (inputUrl == (*urlIt)) {
          urls.erase(urlIt);
          urlIt = urls.end();
        }
        else {
          urlIt++;
        }
      }
      saveInputs(urls);
      p_edsi->setURL(inputUrl);
      p_edsi->removeResource();

    }
    else {
      p_edsi->setURL(inputUrl);
      if (!p_edsi->removeResource()) {
        throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
      }
    }
  }
}


/**
 * Removes all Input resources from the VDoc.
 *
 * @throw DavException DAV server access error
 *
 */
void VDoc::removeInputs() throw(DavException)
{
  vector<EcceURL> urls = getInputs();
  vector<EcceURL> noUrls;

  if (p_currentVdoc) {
    noUrls.clear();
    saveInputs(noUrls);
    p_edsi->removeResources(urls);
  }
  else {
    if (!p_edsi->removeResources(urls)) {
      throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
    }
  }
}


/**
 * Removes an Output resource from the VDoc.
 *
 * @param name Name of resource.
 *
 * @throw DavException DAV server access error
 *
 */
void VDoc::removeOutput(const string& name) throw(DavException)
{
  vector<EcceURL> urls = getOutputs();
  EcceURL *url = findUrl(urls, name);

  if (url != 0) {
    EcceURL outputUrl = *url;
    delete url;
    url = 0;

    if (p_currentVdoc) {

      vector<EcceURL>::iterator urlIt = urls.begin();
      while (urlIt != urls.end()) {
        if (outputUrl == (*urlIt)) {
          urls.erase(urlIt);
          urlIt = urls.end();
        }
        else {
          urlIt++;
        }
      }
      saveOutputs(urls);
      p_edsi->setURL(outputUrl);
      p_edsi->removeResource();

    }
    else {
      p_edsi->setURL(outputUrl);
      if (!p_edsi->removeResource()) {
        throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
      }
    }
  }
}


/**
 * Removes all Output resources from the VDoc.
 *
 * @throw DavException DAV server access error
 *
 */
void VDoc::removeOutputs() throw(DavException)
{
  vector<EcceURL> urls = getOutputs();
  vector<EcceURL> noUrls;

  if (p_currentVdoc) {
    noUrls.clear();
    saveOutputs(noUrls);
    p_edsi->removeResources(urls);
  }
  else {
    if (!p_edsi->removeResources(urls)) {
      throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
    }
  }
}


/**
 * Removes a File resource from the VDoc.  A File resource may
 * be either an Input, Output, or misc. file resource.
 *
 * @param name Name of resource.
 *
 * @throw DavException DAV server access error
 *
 */
void VDoc::removeFile(const string& name) throw(DavException)
{
  // Remove input - if exists
  removeInput(name);

  // Remove output - if exists
  removeOutput(name);

  // Remove file - if exists
  vector<EcceURL> urls = getFiles();
  EcceURL *url = findUrl(urls, name);

  if (url != 0) {
    EcceURL fileUrl = *url;
    delete url;
    url = 0;

    p_edsi->setURL(fileUrl);
    if (!p_edsi->removeResource()) {
      throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
    }
  }
}


/**
 * Removes all File resources from the VDoc.  This includes
 * all Input, Output, and misc. file resources.
 *
 * @throw DavException DAV server access error
 *
 */
void VDoc::removeFiles() throw(DavException)
{
  vector<EcceURL> fileUrls = getFiles();
  removeInputs();
  removeOutputs();
  if (!p_edsi->removeResources(fileUrls)) {
    throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
  }
}


/**
 * Removes a Property resource from the VDoc.
 *
 * @param name Name of resource.
 *
 * @throw DavException DAV server access error
 *
 */
void VDoc::removeProperty(const string& name) throw(DavException)
{
  vector<EcceURL> urls = getProperties();
  EcceURL *url = findUrl(urls, name);

  if (url != 0) {
    EcceURL propertyUrl = *url;
    delete url;
    url = 0;
    p_edsi->setURL(propertyUrl);
    if (!p_edsi->removeResource()) {
      throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
    }
  }
}


/**
 * Removes all Property resources from the VDoc.
 *
 * @throw DavException DAV server access error
 *
 */
void VDoc::removeProperties() throw(DavException)
{
  vector<EcceURL> urls = getProperties();
  if (!p_edsi->removeResources(urls)) {
    throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
  }
}


/**
 * Determines if the base Url identifies a valid VDoc.
 *
 * @throw DavException DAV server access error
 *
 * @return True if is a valid VDoc, otherwise false.
 *
 */
bool VDoc::isVdoc() throw(DavException)
{
  bool ret = false;
  string resourceProp = VDoc::getEcceNamespace() + ":resourcetype";
  ResourceDescriptor::RESOURCETYPE resourceType = 
    ResourceDescriptor::RT_UNDEFINED;
  string contentProp = VDoc::getEcceNamespace() + ":contenttype";
  ResourceDescriptor::CONTENTTYPE contentType = 
    ResourceDescriptor::CT_UNDEFINED;

  vector<MetaDataRequest> requests(2);
  requests[0].name = resourceProp;
  requests[1].name = contentProp;
  vector<MetaDataResult> results;
  if (!p_edsi->getMetaData(requests, results)) {
    throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
  }
  else {
    int i;
    for (i = 0; i < results.size(); i++) {
      if (results[i].name == resourceProp) {
        resourceType =  
          ResourceUtils::stringToResourceType(results[i].value);
      }
      if (results[i].name == contentProp) {
        contentType = 
          ResourceUtils::stringToContentType(results[i].value);
      }
    }
  }

  if ( (resourceType == ResourceDescriptor::RT_VIRTUAL_DOCUMENT) && 
       (contentType != ResourceDescriptor::CT_UNDEFINED) ) {
    ret = true;
  }

  return ret;
}


/**
 * Determines if the VDoc was created with an Ecce  client version 
 * greater than or equal to Version 4.0, indicating that it is 
 * a "current" VDoc.  The VDoc will be accessed differently based
 * on this designation.  That is, prior to 4.0 implies hardcoded 
 * collection lookups/listings.  4.0 and later implies resource 
 * listings provided in DAV properties.
 *
 * @throw DavException DAV server access error.
 * @throw InvalidException Ecce client version could not be determined
 *
 * @return True if it is a current VDoc, otherwise false.
 */
bool VDoc::isCurrentVdoc() throw(DavException, InvalidException)
{
  bool ret = false;
  int version;
  string createdWithProp = VDoc::getEcceNamespace() + ":createdWith";
  string createdWithVal = "";

  vector<MetaDataRequest> requests(1);
  requests[0].name = createdWithProp;
  vector<MetaDataResult> results;
  if (!p_edsi->getMetaData(requests, results)) {
    throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
  }
  else {
    int i;
    for (i = 0; i < results.size(); i++) {
      if (results[i].name == createdWithProp) {

        createdWithVal = results[i].value;

        // Remove 'v'
        if (!createdWithVal.empty()) {
          createdWithVal.erase(0,1); // remove 'v'
        }

        // Erase all char's except primary version number
        if (createdWithVal.size() >= 2) {
          createdWithVal.erase(1); 
        }

      }
    }
  }

  if( (!createdWithVal.empty()) && 
      (StringConverter::toInt(createdWithVal, version))) {
    if (version >= 4) {
      ret = true;
    }
  }
  else {
    //throw InvalidException("Could not determine VDoc version.", WHERE);
    ret = false;
  }

  return ret;
}


/**
 * Performs the necessary steps to either create a new VDoc or 
 * load an existing VDoc.  Default resource and collection names
 * are initialized here.
 *
 * @param createVdoc True indicates a new VDoc should be created.
 *
 * @throw DavException DAV server access error.
 * @throw InvalidException  Url was not recognized as a valid VDoc.
 *
 */
void VDoc::setVdoc(bool createVdoc) 
       throw(DavException, InvalidException)
{
  // DAV Property names
  p_moleculeProp = VDoc::getEcceNamespace() + ":moleculeDocument";
  p_originalMoleculeProp = VDoc::getEcceNamespace() + ":originalMoleculeDocument";
  p_espConstraintsProp = VDoc::getEcceNamespace() + ":espConstraintsDocument";
  p_geometryConstraintsProp = VDoc::getEcceNamespace() + ":geometryConstraintsDocument";
  p_metaPotentialsProp = VDoc::getEcceNamespace() + ":metaPotentialsDocument";
  p_QMMMTermsProp = VDoc::getEcceNamespace() + ":qmmmTermsDocument";
  p_basisSetProp = VDoc::getEcceNamespace() + ":basisSetDocument";
  p_runLogProp = VDoc::getEcceNamespace() + ":runLogDocument";
  p_setupParametersProp = VDoc::getEcceNamespace() + ":setupParametersDocument";
  p_mdPrepareProp = VDoc::getEcceNamespace() + ":mdPrepareDocument";
  p_mdOptimizeProp = VDoc::getEcceNamespace() + ":mdOptimizeDocument";
  p_mdEnergyProp = VDoc::getEcceNamespace() + ":mdEnergyDocument";
  p_mdDynamicsProp = VDoc::getEcceNamespace() + ":mdDynamicsDocument";
  p_thumbnailProp = VDoc::getEcceNamespace() + ":thumbnailDocument";
  p_configurationCollectionProp = VDoc::getEcceNamespace() + ":configurationCollection";
  p_inputCollectionProp = VDoc::getEcceNamespace() + ":inputCollection";
  p_outputCollectionProp = VDoc::getEcceNamespace() + ":outputCollection";
  p_fileCollectionProp = VDoc::getEcceNamespace() + ":fileCollection";
  p_propertyCollectionProp = VDoc::getEcceNamespace() + ":propertyCollection";
  p_configurationsProp = VDoc::getCmcsNamespace() + ":hasConfigurations";
  p_inputsProp = VDoc::getEcceNamespace() + ":hasinputs";
  p_outputsProp = VDoc::getEcceNamespace() + ":hasoutputs";
  p_propertiesProp = VDoc::getEcceNamespace() + ":hasProperties";

  // Default Resource names
  p_molecule = "chemsys.mvm";
  p_originalMolecule = "chemsys_orig.pdb";
  p_inputMolecule = "chemsys.pdb";
  p_outputMolecule = "chemsys.pdb";
  p_espConstraints = "espConstraints.ecst";
  p_geometryConstraints = "GeomConstraintModel.gcst";
  p_metaPotentials = "MetaPotentialModel.mpot";
  p_QMMMTerms = "QMMMModel.qmmm";
  p_basisSet = "BasisSet.ecce_basisset";
  p_generalBasisSet = "GeneralBasisSet.ecce_basisset";
  p_singlePointBasisSet = "SingleBasisSet.ecce_basisset";
  p_runLog = "eccejobstorelog.ecce_run_log";
  p_setupParameters = "SetupParams";
  p_generalSetupParameters = "GeneralSetupParams";
  p_singlePointSetupParameters = "SingleSetupParams";
  p_mdPrepare = "mdPrepare.xml";
  p_mdOptimize = "mdOptimize.xml"; 
  p_mdEnergy =  "mdEnergy.xml";
  p_mdDynamics =  "mdDynamics.xml";
  p_thumbnail = "Thumbnail.jpeg";
  p_nwDirdy =  "nwDirdy.xml";
  p_polyrate =  "polyrate.xml";

  // Default collection names
  if (!p_currentVdoc) {
    p_configurationCollection = "";
    p_inputCollection = "Files";
    p_outputCollection = "Files";
    p_fileCollection = "Files";
    p_propertyCollection = "Properties";
  }
  else {
    p_configurationCollection = "Parameters";
    p_inputCollection = "Inputs";
    p_outputCollection = "Outputs";
    p_fileCollection = "Misc";
    p_propertyCollection = "Props";
  }

  // Load/Create the Virtual Document
  if (!createVdoc) {
    loadMoleculeName();
    loadOriginalMoleculeName();
    loadEspConstraintsName();
    loadGeometryConstraintsName();
    loadMetaPotentialsName();
    loadQMMMTermsName();
    loadBasisSetName();
    loadRunLogName();
    loadSetupParametersName();
    loadConfigurationCollection();
    loadInputCollection();
    loadOutputCollection();
    loadFileCollection();
    loadPropertyCollection();
  }
  else {
    createMoleculeName();
    createOriginalMoleculeName();
    createEspConstraintsName();
    createGeometryConstraintsName();
    createMetaPotentialsName();
    createQMMMTermsName();
    createBasisSetName();
    createRunLogName();
    createSetupParametersName();
    createConfigurationCollection();
    createInputCollection();
    createOutputCollection();
    createPropertyCollection();
  }
}


/**
 * Loads the molecule resource name for the VDoc.  The name is retrieved
 * from a DAV property.  If the property does not exist or the property
 * value is empty - an exception will be thrown.  For older VDocs (pre-4.0),
 * the name is assigned based on the previous naming convention.
 *
 * @throw DavException DAV server access error
 * @throw InvalidException Name of resource could not be determined.
 *
 */
void VDoc::loadMoleculeName() throw(DavException, InvalidException)
{
  if (p_currentVdoc) {

    p_molecule = "";

    p_edsi->setURL(p_base);

    vector<MetaDataRequest> requests(1);
    requests[0].name = p_moleculeProp;
    vector<MetaDataResult> results;
    if (!p_edsi->getMetaData(requests, results)) {
      throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
    }
    else {
      int i;
      for (i = 0; i < results.size(); i++) {
        if (results[i].name == p_moleculeProp) {
          p_molecule = results[i].value;
        }
      }
    }

    if (p_molecule.empty()) {
      throw InvalidException("Could not determine Molecule resource name.",
                             WHERE);
    }
  }
  else {
    // Set resource name if it exists, otherwise default name
    // will be used.
    string doc1 = "Chemsys";
    string doc2 = "Chemsys.ecce_mvm";
    EcceURL url1 = getVdocUrl(doc1);
    EcceURL url2 = getVdocUrl(doc2);

    if (resourceExists(url1)) {
      p_molecule = doc1;
    } 
    if (resourceExists(url2)) {
      p_molecule = doc2;
    }
  }
}


/**
 * Loads the original molecule resource name for the VDoc.  The name is retrieved
 * from a DAV property.  If the property does not exist or the property
 * value is empty - an exception will be thrown.
 *
 * @throw DavException DAV server access error
 * @throw InvalidException Name of resource could not be determined.
 *
 */
void VDoc::loadOriginalMoleculeName() throw(DavException, InvalidException)
{
  p_originalMolecule = "";

  p_edsi->setURL(p_base);

  vector<MetaDataRequest> requests(1);
  requests[0].name = p_originalMoleculeProp;
  vector<MetaDataResult> results;
  if (!p_edsi->getMetaData(requests, results)) {
    throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
  }
  else {
    int i;
    for (i = 0; i < results.size(); i++) {
      if (results[i].name == p_originalMoleculeProp) {
        p_originalMolecule = results[i].value;
      }
    }
  }

  if (p_molecule.empty()) {
    throw InvalidException("Could not determine Original Molecule resource name.",
                           WHERE);
  }
}


/**
 * Loads the ESP constraints resource name for the VDoc.  The name is retrieved
 * from a DAV property.  If the property does not exist or the property
 * value is empty - an exception will be thrown.  For older VDocs (pre-4.0),
 * the name is assigned based on the previous naming convention.
 *
 * @throw DavException DAV server access error
 * @throw InvalidException Name of resource could not be determined.
 *
 */
void VDoc::loadEspConstraintsName() throw(DavException, InvalidException)
{
  if (p_currentVdoc) {

    p_espConstraints = "";

    p_edsi->setURL(p_base);

    vector<MetaDataRequest> requests(1);
    requests[0].name = p_espConstraintsProp;
    vector<MetaDataResult> results;
    if (!p_edsi->getMetaData(requests, results)) {
      throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
    }
    else {
      int i;
      for (i = 0; i < results.size(); i++) {
        if (results[i].name == p_espConstraintsProp) {
          p_espConstraints = results[i].value;
        }
      }
    }
    if (p_espConstraints.empty()) {
      throw InvalidException("Could not determine ESP "
                             "constraints resource name.",
                             WHERE);
    }
  }
  else {
    // Set resource name if it exists, otherwise default name
    // will be used.
    string doc1 = "ESPModel";
    EcceURL url1 = getVdocUrl(doc1);

    if (resourceExists(url1)) {
      p_espConstraints = doc1;
    } 
  }
}


/**
 * Loads the Geometry constraints resource name for the VDoc.  The name is retrieved
 * from a DAV property.  If the property does not exist or the property
 * value is empty - an exception will be thrown.  For older VDocs (pre-4.0),
 * the name is assigned based on the previous naming convention.
 *
 * @throw DavException DAV server access error
 * @throw InvalidException Name of resource could not be determined.
 *
 */
void VDoc::loadGeometryConstraintsName() throw(DavException, InvalidException)
{
  if (p_currentVdoc) {

    p_geometryConstraints = "";

    p_edsi->setURL(p_base);

    vector<MetaDataRequest> requests(1);
    requests[0].name = p_geometryConstraintsProp;
    vector<MetaDataResult> results;
    if (!p_edsi->getMetaData(requests, results)) {
      throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
    }
    else {
      int i;
      for (i = 0; i < results.size(); i++) {
        if (results[i].name == p_geometryConstraintsProp) {
          p_geometryConstraints = results[i].value;
        }
      }
    }
    if (p_geometryConstraints.empty()) {
      throw InvalidException("Could not determine Geometry "
                             "constraints resource name.",
                             WHERE);
    }
  }
  else {
    // Set resource name if it exists, otherwise default name
    // will be used.
    string doc1 = "GeomConstraintModel.gcst";
    EcceURL url1 = getVdocUrl(doc1);

    if (resourceExists(url1)) {
      p_geometryConstraints = doc1;
    } 
  }
}


/**
 * Loads the meta-potentials resource name for the VDoc.  The name is retrieved
 * from a DAV property.  If the property does not exist or the property
 * value is empty - an exception will be thrown.  For older VDocs (pre-4.0),
 * the name is assigned based on the previous naming convention.
 *
 * @throw DavException DAV server access error
 * @throw InvalidException Name of resource could not be determined.
 *
 */
void VDoc::loadMetaPotentialsName() throw(DavException, InvalidException)
{
  if (p_currentVdoc) {

    p_metaPotentials = "";

    p_edsi->setURL(p_base);

    vector<MetaDataRequest> requests(1);
    requests[0].name = p_metaPotentialsProp;
    vector<MetaDataResult> results;
    if (!p_edsi->getMetaData(requests, results)) {
      throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
    }
    else {
      int i;
      for (i = 0; i < results.size(); i++) {
        if (results[i].name == p_metaPotentialsProp) {
          p_metaPotentials = results[i].value;
        }
      }
    }
#if 0
    if (p_metaPotentials.empty()) {
      throw InvalidException("Could not determine meta-Potential "
                             "resource name.",
                             WHERE);
    }
#endif
  }
  else {
    // Set resource name if it exists, otherwise default name
    // will be used.
    string doc1 = "MetaPotentialModel.mpot";
    EcceURL url1 = getVdocUrl(doc1);

    if (resourceExists(url1)) {
      p_metaPotentials = doc1;
    } 
  }
}


/**
 * Loads the QMMM terms resource name for the VDoc.  The name is retrieved
 * from a DAV property.  If the property does not exist or the property
 * value is empty - an exception will be thrown.  For older VDocs (pre-4.0),
 * the name is assigned based on the previous naming convention.
 *
 * @throw DavException DAV server access error
 * @throw InvalidException Name of resource could not be determined.
 *
 */
void VDoc::loadQMMMTermsName() throw(DavException, InvalidException)
{
  if (p_currentVdoc) {

    p_QMMMTerms = "";

    p_edsi->setURL(p_base);

    vector<MetaDataRequest> requests(1);
    requests[0].name = p_QMMMTermsProp;
    vector<MetaDataResult> results;
    if (!p_edsi->getMetaData(requests, results)) {
      throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
    }
    else {
      int i;
      for (i = 0; i < results.size(); i++) {
        if (results[i].name == p_QMMMTermsProp) {
          p_QMMMTerms = results[i].value;
        }
      }
    }
#if 0
    if (p_QMMMTerms.empty()) {
      throw InvalidException("Could not determine QMMM terms "
                             "resource name.",
                             WHERE);
    }
#endif
  }
  else {
    // Set resource name if it exists, otherwise default name
    // will be used.
    string doc1 = "QMMMModel.qmmm";
    EcceURL url1 = getVdocUrl(doc1);

    if (resourceExists(url1)) {
      p_QMMMTerms = doc1;
    } 
  }
}


/**
 * Loads the Basis Set resource name for the VDoc.  The name is retrieved
 * from a DAV property.  If the property does not exist or the property
 * value is empty - an exception will be thrown.  For older VDocs (pre-4.0),
 * the name is assigned based on the previous naming convention.
 *
 * @throw DavException DAV server access error
 * @throw InvalidException Name of resource could not be determined.
 *
 */
void VDoc::loadBasisSetName() throw(DavException, InvalidException)
{
  if (p_currentVdoc) {

    p_basisSet = "";

    p_edsi->setURL(p_base);

    vector<MetaDataRequest> requests(1);
    requests[0].name = p_basisSetProp;
    vector<MetaDataResult> results;
    if (!p_edsi->getMetaData(requests, results)) {
      throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
    }
    else {
      int i;
      for (i = 0; i < results.size(); i++) {
        if (results[i].name == p_basisSetProp) {
          p_basisSet = results[i].value;
        }
      }
    }

    if (p_basisSet.empty()) {
      throw InvalidException("Could not determine Basis Set resource name.",
                             WHERE);
    }
  }
  else {
    // Set resource name if it exists, otherwise default name
    // will be used.
    string doc1 = "BasisSet";
    string doc2 = "BasisSet.ecce_basisset";
    EcceURL url1 = getVdocUrl(doc1);
    EcceURL url2 = getVdocUrl(doc2);

    if (resourceExists(url1)) {
      p_basisSet = doc1;
    } 
    if (resourceExists(url2)) {
      p_basisSet = doc2;
    }
  }
}


/**
 * Loads the Run Log resource name for the VDoc.  The name is retrieved
 * from a DAV property.  If the property does not exist or the property
 * value is empty - an exception will be thrown.  For older VDocs (pre-4.0),
 * the name is assigned based on the previous naming convention.
 *
 * @throw DavException DAV server access error
 * @throw InvalidException Name of resource could not be determined.
 *
 */
void VDoc::loadRunLogName() throw(DavException, InvalidException)
{
  if (p_currentVdoc) {

    p_runLog = "";

    p_edsi->setURL(p_base);

    vector<MetaDataRequest> requests(1);
    requests[0].name = p_runLogProp;
    vector<MetaDataResult> results;
    if (!p_edsi->getMetaData(requests, results)) {
      throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
    }
    else {
      int i;
      for (i = 0; i < results.size(); i++) {
        if (results[i].name == p_runLogProp) {
          p_runLog = results[i].value;
        }
      }
    }

    if (p_runLog.empty()) {
      throw InvalidException("Could not determine Run Log resource name.",
                             WHERE);
    }
  }
  else {
    // Set resource name if it exists, otherwise default name
    // will be used.
    string doc1 = "eccejobstorelog.xml";
    string doc2 = "eccejobstorelog.ecce_run_log";
    EcceURL url1 = getVdocUrl(doc1);
    EcceURL url2 = getVdocUrl(doc2);

    if (resourceExists(url1)) {
      p_runLog = doc1;
    } 
    if (resourceExists(url2)) {
      p_runLog = doc2;
    }
  }
}


/**
 * Loads the Setup Parameters resource name for the VDoc.  The name is retrieved
 * from a DAV property.  If the property does not exist or the property
 * value is empty - an exception will be thrown.  For older VDocs (pre-4.0),
 * the name is assigned based on the previous naming convention.
 *
 * @throw DavException DAV server access error
 * @throw InvalidException Name of resource could not be determined.
 *
 */
void VDoc::loadSetupParametersName() throw(DavException, InvalidException)
{
  if (p_currentVdoc) {

    p_setupParameters = "";

    p_edsi->setURL(p_base);

    vector<MetaDataRequest> requests(1);
    requests[0].name = p_setupParametersProp;
    vector<MetaDataResult> results;
    if (!p_edsi->getMetaData(requests, results)) {
      throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
    }
    else {
      int i;
      for (i = 0; i < results.size(); i++) {
        if (results[i].name == p_setupParametersProp) {
          p_setupParameters = results[i].value;
        }
      }
    }
    if (p_setupParameters.empty()) {
      throw InvalidException("Could not determine Setup "
                             "Parameters resource name.",
                             WHERE);
    }
  }
  else {
    // Set resource name if it exists, otherwise default name
    // will be used.
    string doc1 = "SetupParams";
    EcceURL url1 = getVdocUrl(doc1);

    if (resourceExists(url1)) {
      p_setupParameters = doc1;
    } 
  }
}


/**
 * Loads the Md Prepare resource name for the VDoc.  The name is retrieved
 * from a DAV property.  If the property does not exist or the property
 * value is empty - an exception will be thrown.
 *
 * @throw DavException DAV server access error
 * @throw InvalidException Name of resource could not be determined.
 */
void VDoc::loadMdPrepareName() throw(DavException, InvalidException)
{
  p_mdPrepare = "";

  p_edsi->setURL(p_base);

  vector<MetaDataRequest> requests(1);
  requests[0].name = p_mdPrepareProp;
  vector<MetaDataResult> results;
  if (!p_edsi->getMetaData(requests, results)) {
    throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
  }
  else {
    int i;
    for (i = 0; i < results.size(); i++) {
      if (results[i].name == p_mdPrepareProp) {
        p_mdPrepare = results[i].value;
      }
    }
  }
  if (p_mdPrepare.empty()) {
    throw InvalidException("Could not determine MD "
                           "Prepare resource name.",
                           WHERE);
  }
}


/**
 * Loads the Md Optimize resource name for the VDoc.  The name is retrieved
 * from a DAV property.  If the property does not exist or the property
 * value is empty - an exception will be thrown.  
 *
 * @throw DavException DAV server access error
 * @throw InvalidException Name of resource could not be determined.
 */
void VDoc::loadMdOptimizeName() throw(DavException, InvalidException)
{
  p_mdOptimize = "";

  p_edsi->setURL(p_base);

  vector<MetaDataRequest> requests(1);
  requests[0].name = p_mdOptimizeProp;
  vector<MetaDataResult> results;
  if (!p_edsi->getMetaData(requests, results)) {
    throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
  }
  else {
    int i;
    for (i = 0; i < results.size(); i++) {
      if (results[i].name == p_mdOptimizeProp) {
        p_mdOptimize = results[i].value;
      }
    }
  }
  if (p_mdOptimize.empty()) {
    throw InvalidException("Could not determine MD "
                           "Optimize resource name.",
                           WHERE);
  }
}


/**
 * Loads the Md Energy resource name for the VDoc.  The name is retrieved
 * from a DAV property.  If the property does not exist or the property
 * value is empty - an exception will be thrown.  
 *
 * @throw DavException DAV server access error
 * @throw InvalidException Name of resource could not be determined.
 */
void VDoc::loadMdEnergyName() throw(DavException, InvalidException)
{
  p_mdEnergy = "";

  p_edsi->setURL(p_base);

  vector<MetaDataRequest> requests(1);
  requests[0].name = p_mdEnergyProp;
  vector<MetaDataResult> results;
  if (!p_edsi->getMetaData(requests, results)) {
    throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
  }
  else {
    int i;
    for (i = 0; i < results.size(); i++) {
      if (results[i].name == p_mdEnergyProp) {
        p_mdEnergy = results[i].value;
      }
    }
  }
  if (p_mdEnergy.empty()) {
    throw InvalidException("Could not determine MD "
                           "Energy resource name.",
                           WHERE);
  }
}


/**
 * Loads the Md Dynamics resource name for the VDoc.  The name is retrieved
 * from a DAV property.  If the property does not exist or the property
 * value is empty - an exception will be thrown.  
 *
 * @throw DavException DAV server access error
 * @throw InvalidException Name of resource could not be determined.
 */
void VDoc::loadMdDynamicsName() throw(DavException, InvalidException)
{
  p_mdDynamics = "";

  p_edsi->setURL(p_base);

  vector<MetaDataRequest> requests(1);
  requests[0].name = p_mdDynamicsProp;
  vector<MetaDataResult> results;
  if (!p_edsi->getMetaData(requests, results)) {
    throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
  }
  else {
    int i;
    for (i = 0; i < results.size(); i++) {
      if (results[i].name == p_mdDynamicsProp) {
        p_mdDynamics = results[i].value;
      }
    }
  }
  if (p_mdDynamics.empty()) {
    throw InvalidException("Could not determine MD "
                           "Dynamics resource name.",
                           WHERE);
  }
}


/**
 * Loads the thumbnail resource name for the VDoc.  The name is retrieved
 * from a DAV property.  If the property does not exist or the property
 * value is empty - an exception will be thrown.  
 *
 * @throw DavException DAV server access error
 * @throw InvalidException Name of resource could not be determined.
 */
void VDoc::loadThumbnailName() throw(DavException, InvalidException)
{
  p_thumbnail = "";

  p_edsi->setURL(p_base);

  vector<MetaDataRequest> requests(1);
  requests[0].name = p_thumbnailProp;
  vector<MetaDataResult> results;
  if (!p_edsi->getMetaData(requests, results)) {
    throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
  }
  else {
    int i;
    for (i = 0; i < results.size(); i++) {
      if (results[i].name == p_thumbnailProp) {
        p_thumbnail = results[i].value;
      }
    }
  }
  if (p_thumbnail.empty()) {
    throw InvalidException("Could not determine thumbnail resource name.",
                           WHERE);
  }
}


/**
 * Loads the Configuration collection name for the VDoc.  The name is retrieved
 * from a DAV property.  If the property does not exist 
 * an exception will be thrown.  For older VDocs (pre-4.0),
 * the name is assigned based on the previous naming convention.
 *
 * @throw DavException DAV server access error
 * @throw InvalidException Name of collection could not be determined.
 *
 * @todo remove check for old property name, only done so
 *       older 4.0 developer calcs aren't broken.
 */
void VDoc::loadConfigurationCollection() throw(DavException, InvalidException)
{
  if (p_currentVdoc) {

    if (propertyExists(p_base, VDoc::getEcceNamespace() + ":hasConfigurations")) {
      p_configurationsProp = VDoc::getEcceNamespace() + ":hasConfigurations";
    }

    if (propertyExists(p_base, p_configurationCollectionProp)) {

      p_configurationCollection = "";

      p_edsi->setURL(p_base);

      vector<MetaDataRequest> requests(1);
      requests[0].name = p_configurationCollectionProp;
      vector<MetaDataResult> results;
      if (!p_edsi->getMetaData(requests, results)) {
        throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
      }
      else {
        int i;
        for (i = 0; i < results.size(); i++) {
          if (results[i].name == p_configurationCollectionProp) {
            p_configurationCollection = results[i].value;
          }
        }
      }
    }
    else {
      throw InvalidException("DAV property missing. Could not "
                             "determine collection name for "
                             "configuration resources.",
                             WHERE);
    }
  }
}


/**
 * Loads the Input collection name for the VDoc.  The name is retrieved
 * from a DAV property.  If the property does not exist 
 * an exception will be thrown.  For older VDocs (pre-4.0),
 * the name is assigned based on the previous naming convention.
 *
 * @throw DavException DAV server access error
 * @throw InvalidException Name of collection could not be determined.
 *
 * @todo remove checks for old property names, only done so
 *       older 4.0 developer calcs aren't broken.
 */
void VDoc::loadInputCollection() throw(DavException, InvalidException)
{
  if (p_currentVdoc) {

    // Check for old property names
    string oldCmcsNamespace = "http://purl.oclc.org/NET/cmcs/schema";
    if (propertyExists(p_base, VDoc::getCmcsNamespace() + ":hasInputs")) {
      p_inputsProp = VDoc::getCmcsNamespace() + ":hasInputs";
    }
    if (propertyExists(p_base, VDoc::getCmcsNamespace() + ":hasinputs")) {
      p_inputsProp = VDoc::getCmcsNamespace() + ":hasinputs";
    }
    if (propertyExists(p_base, oldCmcsNamespace + ":hasInputs")) {
      p_inputsProp = oldCmcsNamespace + ":hasInputs";
    }
    if (propertyExists(p_base, oldCmcsNamespace + ":hasinputs")) {
      p_inputsProp = oldCmcsNamespace + ":hasinputs";
    }

    if (propertyExists(p_base, p_inputCollectionProp)) {

      p_inputCollection = "";

      p_edsi->setURL(p_base);

      vector<MetaDataRequest> requests(1);
      requests[0].name = p_inputCollectionProp;
      vector<MetaDataResult> results;
      if (!p_edsi->getMetaData(requests, results)) {
        throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
      }
      else {
        int i;
        for (i = 0; i < results.size(); i++) {
          if (results[i].name == p_inputCollectionProp) {
            p_inputCollection = results[i].value;
          }
        }
      }
    }
    else {
      throw InvalidException("DAV property missing. Could not "
                             "determine collection name for "
                             "input resources.",
                             WHERE);
    }
  }
}


/**
 * Loads the Output collection name for the VDoc.  The name is retrieved
 * from a DAV property.  If the property does not exist 
 * an exception will be thrown.  For older VDocs (pre-4.0),
 * the name is assigned based on the previous naming convention.
 *
 * @throw DavException DAV server access error
 * @throw InvalidException Name of collection could not be determined.
 *
 * @todo remove check for old :hasOutputs property name, only done so
 *       older 4.0 developer calcs aren't broken.
 *
 */
void VDoc::loadOutputCollection() throw(DavException, InvalidException)
{
  if (p_currentVdoc) {

    // Check for old property names
    string oldCmcsNamespace = "http://purl.oclc.org/NET/cmcs/schema";
    if (propertyExists(p_base, VDoc::getCmcsNamespace() + ":hasOutputs")) {
      p_outputsProp = VDoc::getCmcsNamespace() + ":hasOutputs";
    }
    if (propertyExists(p_base, VDoc::getCmcsNamespace() + ":hasoutputs")) {
      p_outputsProp = VDoc::getCmcsNamespace() + ":hasoutputs";
    }
    if (propertyExists(p_base, oldCmcsNamespace + ":hasOutputs")) {
      p_outputsProp = oldCmcsNamespace + ":hasOutputs";
    }
    if (propertyExists(p_base, oldCmcsNamespace + ":hasoutputs")) {
      p_outputsProp = oldCmcsNamespace + ":hasoutputs";
    }


    if (propertyExists(p_base, p_outputCollectionProp)) {

      p_outputCollection = "";

      p_edsi->setURL(p_base);

      vector<MetaDataRequest> requests(1);
      requests[0].name = p_outputCollectionProp;
      vector<MetaDataResult> results;
      if (!p_edsi->getMetaData(requests, results)) {
        throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
      }
      else {
        int i;
        for (i = 0; i < results.size(); i++) {
          if (results[i].name == p_outputCollectionProp) {
            p_outputCollection = results[i].value;
          }
        }
      }
    }
    else {
      throw InvalidException("DAV property missing. Could not "
                             "determine collection name for "
                             "output resources.",
                             WHERE);
    }
  }
}


/**
 * Loads the File collection name for the VDoc.  The name is retrieved
 * from a DAV property.  If the property does not exist 
 * an exception will be thrown.  For older VDocs (pre-4.0),
 * the name is assigned based on the previous naming convention.
 *
 * @throw DavException DAV server access error
 * @throw InvalidException Name of collection could not be determined.
 *
 */
void VDoc::loadFileCollection() throw(DavException, InvalidException)
{
  if (p_currentVdoc) {

    p_fileCollection = "";

    if (propertyExists(p_base, p_fileCollectionProp)) {

      p_edsi->setURL(p_base);

      vector<MetaDataRequest> requests(1);
      requests[0].name = p_fileCollectionProp;
      vector<MetaDataResult> results;
      if (!p_edsi->getMetaData(requests, results)) {
        throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
      }
      else {
        int i;
        for (i = 0; i < results.size(); i++) {
          if (results[i].name == p_fileCollectionProp) {
            p_fileCollection = results[i].value;
          }
        }
      }
    }
  }
}


/**
 * Loads the Property collection name for the VDoc.  The name is retrieved
 * from a DAV property.  If the property does not exist 
 * an exception will be thrown.  For older VDocs (pre-4.0),
 * the name is assigned based on the previous naming convention.
 *
 * @throw DavException DAV server access error
 * @throw InvalidException Name of collection could not be determined.
 *
 */
void VDoc::loadPropertyCollection() throw(DavException, InvalidException)
{
  if (p_currentVdoc) {

    if (propertyExists(p_base, p_propertyCollectionProp)) {

      p_propertyCollection = "";

      p_edsi->setURL(p_base);

      vector<MetaDataRequest> requests(1);
      requests[0].name = p_propertyCollectionProp;
      vector<MetaDataResult> results;
      if (!p_edsi->getMetaData(requests, results)) {
        throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
      }
      else {
        int i;
        for (i = 0; i < results.size(); i++) {
          if (results[i].name == p_propertyCollectionProp) {
            p_propertyCollection = results[i].value;
          }
        }
      }
    }
    else {
      throw InvalidException("DAV property missing. Could not "
                             "determine collection name for "
                             "property resources.",
                             WHERE);
    }
  }
}


/**
 * For a new VDoc, creates and initializes DAV property 
 * used to store the molecule resource name.
 * 
 * @throw DavException DAV server access error.
 *
 */
void VDoc::createMoleculeName() throw(DavException)
{
  p_edsi->setURL(p_base);
  vector<MetaDataResult> results(1);
  results[0].name = p_moleculeProp;
  results[0].value = p_molecule;
  if (!p_edsi->putMetaData(results)) {
    throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
  }
}


/**
 * For a new VDoc, creates and initializes DAV property 
 * used to store the original molecule resource name.
 * 
 * @throw DavException DAV server access error.
 *
 */
void VDoc::createOriginalMoleculeName() throw(DavException)
{
  p_edsi->setURL(p_base);
  vector<MetaDataResult> results(1);
  results[0].name = p_originalMoleculeProp;
  results[0].value = p_originalMolecule;
  if (!p_edsi->putMetaData(results)) {
    throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
  }
}


/**
 * For a new VDoc, creates and initializes DAV property 
 * used to store the ESP Constraints resource name.
 * 
 * @throw DavException DAV server access error.
 *
 */
void VDoc::createEspConstraintsName() throw(DavException)
{
  p_edsi->setURL(p_base);
  vector<MetaDataResult> results(1);
  results[0].name = p_espConstraintsProp;
  results[0].value = p_espConstraints;
  if (!p_edsi->putMetaData(results)) {
    throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
  }
}


/**
 * For a new VDoc, creates and initializes DAV property 
 * used to store the Geometry Constraints resource name.
 * 
 * @throw DavException DAV server access error.
 *
 */
void VDoc::createGeometryConstraintsName() throw(DavException)
{
  p_edsi->setURL(p_base);
  vector<MetaDataResult> results(1);
  results[0].name = p_geometryConstraintsProp;
  results[0].value = p_geometryConstraints;
  if (!p_edsi->putMetaData(results)) {
    throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
  }
}


/**
 * For a new VDoc, creates and initializes DAV property 
 * used to store the meta-potentials resource name.
 * 
 * @throw DavException DAV server access error.
 *
 */
void VDoc::createMetaPotentialsName() throw(DavException)
{
  p_edsi->setURL(p_base);
  vector<MetaDataResult> results(1);
  results[0].name = p_metaPotentialsProp;
  results[0].value = p_metaPotentials;
  if (!p_edsi->putMetaData(results)) {
    throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
  }
}


/**
 * For a new VDoc, creates and initializes DAV property 
 * used to store the QMMM terms resource name.
 * 
 * @throw DavException DAV server access error.
 *
 */
void VDoc::createQMMMTermsName() throw(DavException)
{
  p_edsi->setURL(p_base);
  vector<MetaDataResult> results(1);
  results[0].name = p_QMMMTermsProp;
  results[0].value = p_QMMMTerms;
  if (!p_edsi->putMetaData(results)) {
    throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
  }
}


/**
 * For a new VDoc, creates and initializes DAV property 
 * used to store the Basis Set resource name.
 * 
 * @throw DavException DAV server access error.
 *
 */
void VDoc::createBasisSetName() throw(DavException)
{
  p_edsi->setURL(p_base);
  vector<MetaDataResult> results(1);
  results[0].name = p_basisSetProp;
  results[0].value = p_basisSet;
  if (!p_edsi->putMetaData(results)) {
    throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
  }
}


/**
 * For a new VDoc, creates and initializes DAV property 
 * used to store the Run Log resource name.
 * 
 * @throw DavException DAV server access error.
 *
 */
void VDoc::createRunLogName() throw(DavException)
{
  p_edsi->setURL(p_base);
  vector<MetaDataResult> results(1);
  results[0].name = p_runLogProp;
  results[0].value = p_runLog;
  if (!p_edsi->putMetaData(results)) {
    throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
  }
}


/**
 * For a new VDoc, creates and initializes DAV property 
 * used to store the Setup Parameters resource name.
 * 
 * @throw DavException DAV server access error.
 *
 */
void VDoc::createSetupParametersName() throw(DavException)
{
  p_edsi->setURL(p_base);
  vector<MetaDataResult> results(1);
  results[0].name = p_setupParametersProp;
  results[0].value = p_setupParameters;
  if (!p_edsi->putMetaData(results)) {
    throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
  }
}


/**
 * For a new VDoc, creates and initializes DAV property 
 * used to store the MD Prepare resource name.
 * 
 * @throw DavException DAV server access error.
 *
 */
void VDoc::createMdPrepareName() throw(DavException)
{
  p_edsi->setURL(p_base);
  vector<MetaDataResult> results(1);
  results[0].name = p_mdPrepareProp;
  results[0].value = p_mdPrepare;
  if (!p_edsi->putMetaData(results)) {
    throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
  }
}


/**
 * For a new VDoc, creates and initializes DAV property 
 * used to store the MD Optimize resource name.
 * 
 * @throw DavException DAV server access error.
 *
 */
void VDoc::createMdOptimizeName() throw(DavException)
{
  p_edsi->setURL(p_base);
  vector<MetaDataResult> results(1);
  results[0].name = p_mdOptimizeProp;
  results[0].value = p_mdOptimize;
  if (!p_edsi->putMetaData(results)) {
    throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
  }
}


/**
 * For a new VDoc, creates and initializes DAV property 
 * used to store the MD Energy resource name.
 * 
 * @throw DavException DAV server access error.
 *
 */
void VDoc::createMdEnergyName() throw(DavException)
{
  p_edsi->setURL(p_base);
  vector<MetaDataResult> results(1);
  results[0].name = p_mdEnergyProp;
  results[0].value = p_mdEnergy;
  if (!p_edsi->putMetaData(results)) {
    throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
  }
}


/**
 * For a new VDoc, creates and initializes DAV property 
 * used to store the MD Dynamics resource name.
 * 
 * @throw DavException DAV server access error.
 *
 */
void VDoc::createMdDynamicsName() throw(DavException)
{
  p_edsi->setURL(p_base);
  vector<MetaDataResult> results(1);
  results[0].name = p_mdDynamicsProp;
  results[0].value = p_mdDynamics;
  if (!p_edsi->putMetaData(results)) {
    throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
  }
}


/**
 * For a new VDoc, creates and initializes DAV property 
 * used to store the thumbnail resource name.
 * 
 * @throw DavException DAV server access error.
 *
 */
void VDoc::createThumbnailName() throw(DavException)
{
  p_edsi->setURL(p_base);
  vector<MetaDataResult> results(1);
  results[0].name = p_thumbnailProp;
  results[0].value = p_thumbnail;
  if (!p_edsi->putMetaData(results)) {
    throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
  }
}


/**
 * For a new VDoc, creates and initializes DAV property 
 * used to store the Configuration collection name.
 * 
 * @throw DavException DAV server access error.
 *
 */
void VDoc::createConfigurationCollection() throw(DavException)
{
  createVdocCollection(p_configurationCollection);

  p_edsi->setURL(p_base);
  vector<MetaDataResult> results(1);
  results[0].name = p_configurationCollectionProp;
  results[0].value = p_configurationCollection;
  if (!p_edsi->putMetaData(results)) {
    throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
  }
}


/**
 * For a new VDoc, creates and initializes DAV property 
 * used to store the Input collection name.
 * 
 * @throw DavException DAV server access error.
 *
 */
void VDoc::createInputCollection() throw(DavException)
{
  createVdocCollection(p_inputCollection);

  p_edsi->setURL(p_base);
  vector<MetaDataResult> results(1);
  results[0].name = p_inputCollectionProp;
  results[0].value = p_inputCollection;
  if (!p_edsi->putMetaData(results)) {
    throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
  }
}


/**
 * For a new VDoc, creates and initializes DAV property 
 * used to store the Output collection name.
 * 
 * @throw DavException DAV server access error.
 *
 */
void VDoc::createOutputCollection() throw(DavException)
{
  createVdocCollection(p_outputCollection);

  p_edsi->setURL(p_base);
  vector<MetaDataResult> results(1);
  results[0].name = p_outputCollectionProp;
  results[0].value = p_outputCollection;
  if (!p_edsi->putMetaData(results)) {
    throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
  }
}


/**
 * For a new VDoc, creates and initializes DAV property 
 * used to store the Property collection name.
 * 
 * @throw DavException DAV server access error.
 *
 */
void VDoc::createPropertyCollection() throw(DavException)
{
  createVdocCollection(p_propertyCollection);

  p_edsi->setURL(p_base);
  vector<MetaDataResult> results(1);
  results[0].name = p_propertyCollectionProp;
  results[0].value = p_propertyCollection;
  if (!p_edsi->putMetaData(results)) {
    throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
  }
  saveProperties();
}


/**
 * Parses XML document.  Document will be one of either Configurations, Inputs,
 * or Outputs DAV property values - which contains an XML document listing
 * the resources for a particular category. 
 *
 * @param The xml document.
 * 
 * @throw ParseException Error encountered when parsing document.
 *
 * @return List of resources identified by the document.
 */
vector<string> VDoc::parseDoc(const string& document) throw(ParseException)
{
  vector<string> ret;

  BasicDOMParser parser;
  DOMDocument *domDocument = 0;

  istringstream is(document);
  domDocument = parser.parse(is);

  ret = getDocItems(domDocument);

  return ret;
}


/**
 * Gets resources from the DOM document.  
 *
 * @param domDocument DOM document.
 *
 * @return List of resources identified by the document.
 *
 * @todo remove checks for old ecce:href element, only done so
 *       older 4.0 developer calcs aren't broken.
 */
vector<string> VDoc::getDocItems(DOMDocument *domDocument) 
{
  vector<string> ret;

  // Get Root element
  DOMElement *root = domDocument->getDocumentElement();

  // Get rdf:href elements
  XMLCh *hrefElementName = XMLString::transcode("rdf:href");
  DOMNodeList *hrefElements = root->getElementsByTagName(hrefElementName);
  delete [] hrefElementName;
  hrefElementName = 0;

  // Get old ecce:href elements
  XMLCh *old_hrefElementName = XMLString::transcode("ecce:href");
  DOMNodeList *old_hrefElements = root->getElementsByTagName(old_hrefElementName);
  delete [] old_hrefElementName;
  old_hrefElementName = 0;

  // Get href attributes (xlink)
  XMLCh *hrefAttName = XMLString::transcode("xlink:href");
  char *hrefAttValue = 0;
  int i;
  for (i = 0; i < hrefElements->getLength(); i++) {
    DOMElement *hrefElement = (DOMElement*)hrefElements->item(i);
    hrefAttValue = XMLString::transcode(hrefElement->getAttribute(hrefAttName));
    if (hrefAttValue != 0) {
      string val = hrefAttValue;
      ret.push_back(val);
      delete [] hrefAttValue;        
      hrefAttValue = 0;
    }
  }

  // Get old ecce:href links
  for (i = 0; i < old_hrefElements->getLength(); i++) {
    DOMElement *hrefElement = (DOMElement*)old_hrefElements->item(i);
    hrefAttValue = XMLString::transcode(hrefElement->getAttribute(hrefAttName));
    if (hrefAttValue != 0) {
      string val = hrefAttValue;
      ret.push_back(val);
      delete [] hrefAttValue;        
      hrefAttValue = 0;
    }
  }
  delete [] hrefAttName;
  hrefAttName = 0;

  return ret;
}


/**
 * Gets the application JCode.  
 * 
 * Based on the value of the :application DAV property a pointer to 
 * a JCode object is returned.
 *
 * @return Pointer to a JCode.  Null returned if no Value for :application.
 *
 */
const JCode* VDoc::getApplication() throw(DavException)
{
  const JCode* tcode = 0;  

  EcceURL url = p_base;
  p_edsi->setURL(url);

  vector<MetaDataRequest> requests(1);
  vector<MetaDataResult> results;
  requests[0].name = VDoc::getEcceNamespace() + ":application";

  if (!p_edsi->getMetaData(requests, results)) {
    throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);    
  }
  else if (results.size() > 0) {
    tcode = CodeFactory::lookup(results[0].value.c_str());
  }

  return tcode;
}


/**
 * Based on the application, determines if the provided name
 * is an Input filename.
 *
 * @param name Name of file.
 *
 * @return True if it is an Input filename, otherwise false.
 *
 */
bool VDoc::isInputFilename(const string& name) 
{
  bool ret = false;
  const JCode *codeCap = getApplication();
  vector<TypedFile> codeFiles; 

  if (codeCap) {

    codeFiles = codeCap->getCodeFiles(JCode::INPUT);
 
    int i;
    for (i = 0; i < codeFiles.size(); i++) {
      if (name == codeFiles[i].name()) {
        ret = true;
        break;
      }
    }
  }

  return ret;
}


/**
 * Based on the application, determines if the provided url
 * has an input mime-type
 *
 * @param url URL of resouce
 *
 * @return True if it is an Input URL, otherwise false.
 *
 */
bool VDoc::isInputType(const EcceURL& url) 
{
  bool ret = false;
  const JCode *codeCap = getApplication();
  vector<TypedFile> codeFiles; 

  p_edsi->setURL(url);
  string mimeType = "";

  if (codeCap && p_edsi->exists()) {

    codeFiles = codeCap->getCodeFiles(JCode::INPUT);

    // Get mime-type
    vector<MetaDataRequest> requests(1);
    string propName = "DAV:getcontenttype";
    requests[0].name = propName;
    vector<MetaDataResult> results;
    if (p_edsi->getMetaData(requests, results)) {
      for (int i = 0; i < results.size(); i++) {
        if (results[i].name == propName) {
          mimeType = results[i].value;
        }
      }
    }
 
    // Compare registered mie-types with resources mime-type
    for (int i = 0; i < codeFiles.size(); i++) {
      if (mimeType == codeFiles[i].type()) {
        ret = true;
        break;
      }
    }
  }
  return ret;
}


/**
 * Based on the application, determines if the provided name
 * is an Output filename.
 *
 * @param name Name of file.
 *
 * @return True if it is an Output filename, otherwise false.
 *
 */
bool VDoc::isOutputFilename(const string& name) 
{
  bool ret = false;
  const JCode *codeCap = getApplication();
  vector<TypedFile> codeFiles; 

  if (codeCap) {

    codeFiles = codeCap->getCodeFiles(JCode::OUTPUT);
 
    int i;
    for (i = 0; i < codeFiles.size(); i++) {
      if (name == codeFiles[i].name()) {
        ret = true;
        break;
      }
    }
    // Check if it's the run log 
    if (!ret) {
      if (name == p_runLog) {
        ret = true;
      }
    }
  }

  return ret;
}


/**
 * Based on the application, determines if the provided url
 * has an input mime-type
 *
 * @param url URL of resouce
 *
 * @return True if it is an Input URL, otherwise false.
 *
 */
bool VDoc::isOutputType(const EcceURL& url) 
{
  bool ret = false;
  const JCode *codeCap = getApplication();
  vector<TypedFile> codeFiles; 

  p_edsi->setURL(url);
  string mimeType = "";

  if (codeCap && p_edsi->exists()) {

    codeFiles = codeCap->getCodeFiles(JCode::OUTPUT);

    // Get mime-type
    vector<MetaDataRequest> requests(1);
    string propName = "DAV:getcontenttype";
    requests[0].name = propName;
    vector<MetaDataResult> results;
    if (p_edsi->getMetaData(requests, results)) {
      for (int i = 0; i < results.size(); i++) {
        if (results[i].name == propName) {
          mimeType = results[i].value;
        }
      }
    }
 
    // Compare registered mie-types with resources mime-type
    for (int i = 0; i < codeFiles.size(); i++) {
      if (mimeType == codeFiles[i].type()) {
        ret = true;
        break;
      }
    }
  }
  return ret;
}


/**
 * Saves the urls to a DAV property as an xml document, listing the configuration
 * resources for the VDoc.  This method does not apply to older VDoc's (pre-4.0). 
 *
 * @param urls The configuration resource Urls for the VDoc to be saved.
 *
 * @throw DavException DAV server access error.
 *
 */
void VDoc::saveConfigurations(const vector<EcceURL>& urls) throw(DavException)
{
  // Need to restore setting when done
  EcceURL origUrl = p_edsi->getURL();

  if (p_currentVdoc) {
    string document = "";

    if (urls.size() > 0) {
      startDoc(document);
      int i;
      for (i = 0; i < urls.size(); i++) {
        addDocItem(document, getVdocPath(urls[i].getFile()));
      }
      endDoc(document);
    }

    p_edsi->setURL(p_base);
    vector<MetaDataResult> results(1);
    results[0].name = p_configurationsProp;
    results[0].value = document;
    if (!p_edsi->putMetaData(results)) {
      throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
    }
  }
  p_edsi->setURL(origUrl);
}


/**
 * Saves the urls to a DAV property as an xml document, listing the Input
 * resources for the VDoc.  This method does not apply to older VDoc's (pre-4.0). 
 *
 * @param urls The input resource Urls for the VDoc to be saved.
 *
 * @throw DavException DAV server access error.
 *
 */
void VDoc::saveInputs(const vector<EcceURL>& urls) throw(DavException)
{
  // Need to restore setting when done
  EcceURL origUrl = p_edsi->getURL();

  if (p_currentVdoc) {
    string document = "";

    if (urls.size() > 0) {
      startDoc(document);
      int i;
      for (i = 0; i < urls.size(); i++) {
        addDocItem(document, getVdocPath(urls[i].getFile()));
      }
      endDoc(document);
    }

    p_edsi->setURL(p_base);
    vector<MetaDataResult> results(1);
    results[0].name = p_inputsProp;
    results[0].value = document;
    if (!p_edsi->putMetaData(results)) {
      throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
    }
  }
  p_edsi->setURL(origUrl);
}


/**
 * Saves the urls to a DAV property as an xml document, listing the Output
 * resources for the VDoc.  This method does not apply to older VDoc's (pre-4.0). 
 *
 * @param urls The output resource Urls for the VDoc to be saved.
 *
 * @throw DavException DAV server access error.
 *
 */
void VDoc::saveOutputs(const vector<EcceURL>& urls) throw(DavException)
{
  // Need to restore setting when done
  EcceURL origUrl = p_edsi->getURL();

  if (p_currentVdoc) {
    string document = "";

    if (urls.size() > 0) {
      startDoc(document);
      int i;
      for (i = 0; i < urls.size(); i++) {
        addDocItem(document, getVdocPath(urls[i].getFile()));
      }
      endDoc(document);
    }

    p_edsi->setURL(p_base);
    vector<MetaDataResult> results(1);
    results[0].name = p_outputsProp;
    results[0].value = document;
    if (!p_edsi->putMetaData(results)) {
      throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
    }
  }
  p_edsi->setURL(origUrl);
}


/**
 * Saves the property collection name to a DAV property as 
 * an xml document. This method does not apply to older VDoc's (pre-4.0). 
 *
 * @throw DavException DAV server access error.
 *
 */
void VDoc::saveProperties() throw(DavException)
{
  // Need to restore setting when done
  EcceURL origUrl = p_edsi->getURL();

  if (p_currentVdoc) {
    string document = "";

    startDoc(document);
    addDocItem(document, p_propertyCollection);
    endDoc(document);

    p_edsi->setURL(p_base);
    vector<MetaDataResult> results(1);
    results[0].name = p_propertiesProp;
    results[0].value = document;
    if (!p_edsi->putMetaData(results)) {
      throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
    }
  }
  p_edsi->setURL(origUrl);
}


/**
 * Initializes/starts the xml document.  Adds namespaces and creates
 * the rdf:Bag element.  
 *
 * WARNING: SAM2 (actually jdom) has trouble with whitespace between the
 * end of the CDATA and the closing element tag so all whitespace removed.
 *
 * @param document Document that is initialized.
 *
 */
void VDoc::startDoc(string& document)
{
  document.clear();
  // Wrap in CDATA because we have paths in the urls and '/' is an
  // XML character???  Seemed to be needed for mod_dav but not sam
  // Originally we thought we needed the xml header too but I think
  // this was a different problem.
  //document += "<![CDATA[<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
  document += "<![CDATA["
              "<rdf:Bag "
              "xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\" "
              "xmlns:xlink=\"http://www.w3.org/1999/xlink\" > ";
}


/**
 * Adds an item to the xml document.  The item is a path relative to the
 * base Url of the VDoc.  An xlink href element within an RDF container
 * is used to document the resource.
 *
 * WARNING: SAM2 (actually jdom) has trouble with whitespace between the
 * end of the CDATA and the closing element tag so all whitespace removed.
 *
 * @param document Document the item will be added to.
 * @param path Path relative to the VDoc's base Url, identifying the resource.
 *
 */
void VDoc::addDocItem(string& document, const string& path)
{
  document += "<rdf:li>"
              "<rdf:href xlink:href=\"" + path + "\" xlink:type=\"simple\"></rdf:href>"
              "</rdf:li>";
}


/**
 * Ends the xml document.  The rdf:Bag end tag is added.
 *
 * WARNING: SAM2 (actually jdom) has trouble with whitespace between the
 * end of the CDATA and the closing element tag so all whitespace removed.
 *
 * @param document Document that is ended.
 *
 */
void VDoc::endDoc(string& document)
{
  document += "</rdf:Bag>]]>"; 
  //document += "</rdf:Bag>"; 
}


/**
 * Determines if the url exists.  
 *
 * @param url Url of resource to check if exists.
 *
 * @throw DavException DAV server access error
 *
 * @return True if url exists, otherwise false.
 *
 */
bool VDoc::resourceExists(const EcceURL& url) throw(DavException)
{
  bool ret = false;

  p_edsi->setURL(url);
  ret = p_edsi->exists();
  if (p_edsi->m_msgStack.messages()) {
    throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
  }

  return ret;
}


/**
 * Determines if the DAV property exists.  Needed this because the getMetadata calls 
 * of DavEDSI will not return a property if it does not have a value - even
 * though the property exists.  
 *
 * @todo Look into modifying the getMetaData calls to return properties
 *       even if their values are empty.
 * 
 * @param url Url of resource containing DAV property.
 * @param property DAV property to check if exists.
 *
 * @throw DavException DAV server access error
 *
 * @return True if property exists, otherwise false.
 *
 */
bool VDoc::propertyExists(const EcceURL& url, const string& property) throw(DavException)
{
  bool ret = false;

  vector<MetaDataResult> results;

  p_edsi->setURL(url);
  if (!p_edsi->describeMetaData(results)) {
    throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
  }
  else {
    int i;
    for (i = 0; i < results.size(); i++) {
      if (property == results[i].name) {
        ret = true;
        break;
      }
    }
  }

  return ret;
}


/**
 * Creates a path relative to the VDoc's base url.
 *
 * @param collection Name of collection
 * @param resource Name of resource - default value is Null
 * 
 * @return A path relative to the VDoc's base Url.
 *
 */
string VDoc::getVdocPath(const string& collection, const string& resource) const
{
  string vdocPath = "";
  string basePath = p_base.getFile();
  string collectionPath = collection;
  string resourceName = resource;

  // Remove leading and trailing slashes
  trimSlash(basePath);
  trimSlash(collectionPath);
  trimSlash(resourceName);

  // Create relative Path
  vdocPath = collectionPath + "/" + resourceName;
  trimSlash(vdocPath);
  string::size_type idx = vdocPath.find(basePath);

  if ( (idx != string::npos) && 
       (idx == 0) ) {
    vdocPath = vdocPath.erase(0, basePath.size());
  }
  trimSlash(vdocPath);
  return vdocPath;
}


/**
 * Trims leading and trailing slashes from the string.
 * 
 * @param value String to trim
 *
 */
void VDoc::trimSlash(string& value) const
{
  bool foundSlash;

  // Remove leading "/"'s
  foundSlash = true;
  while ( (foundSlash) && (value.size() > 0) ) {
    if (value[0] == '/') {
      value.erase(0,1);
    }
    else {
      foundSlash = false;
    }
  }

  // Remove trailing "/"'s
  foundSlash = true;
  while ( (foundSlash) && (value.size() > 0) ) {
    if (value[value.size() - 1] == '/') {
      value.erase(value.size() - 1, 1);
    }
    else {
      foundSlash = false;
    }
  }
}


/**
 * Using the provided path, creates a Url relative
 * to the VDoc's base Url.
 *
 * @param path String containing relative path.
 *
 * @return An EcceURL
 *
 */
EcceURL VDoc::getVdocUrl(const string& path) const
{
  EcceURL vdocUrl = p_base.getChild(path.c_str());
  return vdocUrl;
}


/**
 * Using the provided paths, creates Urls relative
 * to the VDoc's base Url.
 *
 * @param paths List of strings, each string contains 
          a path relative to the VDoc's base Url.
 *
 * @return A vector of EcceURL
 *
 */
vector<EcceURL> VDoc::getVdocUrls(const vector<string>& paths) const
{
  vector<EcceURL> ret;
  int i;
  for (i = 0; i < paths.size(); i++) {
    ret.push_back(getVdocUrl(paths[i]));
  }
  return ret;
}


/**
 * Creates a collection for the VDoc.
 *
 * @param collectionName Name of collection.
 *
 * @throw DavException DAV server access error.
 *
 */
void VDoc::createVdocCollection(const string& collectionName) throw (DavException)
{
  if (!collectionName.empty()) {
    EcceURL collectionUrl = getVdocUrl(collectionName);
    EcceURL collectionParentUrl = collectionUrl.getParent();
    if (!resourceExists(collectionUrl)) {
      p_edsi->setURL(collectionParentUrl);
      if (!p_edsi->makeCollection(collectionName)) {
        throw DavException(p_edsi->m_msgStack.getMessage(), WHERE);
      }
    }
  }
}



/**
 * Searches a vector of Urls for a specific resource name.  
 *
 * @param urls Urls to search.
 * @param name Name of resource to search for.
 *
 * @return Pointer to an EcceURL, caller responsible for memory cleanup.
 *
 */
EcceURL* VDoc::findUrl(const vector<EcceURL>& urls, const string& name) const
{
  EcceURL *url = 0;

  int i;
  for (i = 0; i < urls.size(); i++) {
    if (name == urls[i].getFilePathTail()) {
      url = new EcceURL(urls[i]);
      break;
    }
  }

  return url;
}


/**
 * Creates a configuration Url.
 *
 * @param name Name of configuration resource.
 *
 * @return The configuration Url.
 *
 */
EcceURL VDoc::createConfigurationUrl(const string& name) const
{
  EcceURL configurationUrl = 
    getVdocUrl(getVdocPath(p_configurationCollection, name));

  return configurationUrl;
}


/**
 * Creates an Input Url.
 *
 * @param name Name of input resource.
 *
 * @return The input Url.
 *
 */
EcceURL VDoc::createInputUrl(const string& name) const
{
  EcceURL inputUrl = 
    getVdocUrl(getVdocPath(p_inputCollection, name));

  return inputUrl;
}


/**
 * Creates an Output Url.
 *
 * @param name Name of output resource.
 *
 * @return The output Url.
 *
 */
EcceURL VDoc::createOutputUrl(const string& name) const
{
  EcceURL outputUrl = 
    getVdocUrl(getVdocPath(p_outputCollection, name));

  return outputUrl;
}


/**
 * Creates a File Url.
 *
 * @param name Name of file resource.
 *
 * @return The file Url.
 *
 */
EcceURL VDoc::createFileUrl(const string& name) const
{
  EcceURL fileUrl = 
    getVdocUrl(getVdocPath(p_fileCollection, name));

  return fileUrl;
}


/**
 * Creates a Property Url.
 *
 * @param name Name of property resource.
 *
 * @return The property Url.
 *
 */
EcceURL VDoc::createPropertyUrl(const string& name) const
{
  EcceURL propertyUrl = 
    getVdocUrl(getVdocPath(p_propertyCollection, name));

  return propertyUrl;
}










