#include <fstream>
using std::ofstream;
using std::ifstream;
#include <iostream>
using std::ostream;
using std::istream;
  using std::cout;
  using std::cerr;
  using std::endl;
  using std::ios;

#include <stdlib.h>
#include <string.h>

#include <xercesc/util/PlatformUtils.hpp>

#include "util/EcceURL.H"
#include "util/IOException.H"
#include "util/TempStorage.H"
#include "util/SFile.H"

#include "dsm/EDSI.H"
#include "dsm/EDSIFactory.H"
#include "dsm/DavEDSI.H"
#include "dsm/EDSIExporter.H"
#include "dsm/ResourceDescriptor.H"


// Restrict meta data exports to non DAV name spaces
static string DAV_NAMESPACE = "DAV";


EDSIExporter::EDSIExporter()
{
}


EDSIExporter::EDSIExporter(const EcceURL &sourceURL, const EcceURL &targetURL) 
{
  setSourceURL(sourceURL);
  setTargetURL(targetURL);
}


EDSIExporter::~EDSIExporter()
{
}


EcceURL EDSIExporter::getSourceURL() const
{
  return p_sourceURL;
}


EcceURL EDSIExporter::getTargetURL() const
{
  return p_targetURL;
}


void EDSIExporter::setSourceURL(const EcceURL& sourceURL)
{
  p_sourceURL = sourceURL;
}


void EDSIExporter::setTargetURL(const EcceURL& targetURL)
{
  p_targetURL = targetURL;
}


/**
 * Exports the src url to the target url.
 * Note that if the export fails, it can fail part way through but since
 * an IOException is used, no return url can be communicated.  So
 * upstream callers can't show or delete partial exported urls.
 */
EcceURL EDSIExporter::xport() throw (IOException)
{

  // Create source and target EDSI
  EDSI *source = EDSIFactory::getEDSI(getSourceURL());
  EDSI *target = EDSIFactory::getEDSI(getTargetURL());

  // Check Source EDSI creation
  if (source == NULL) {
    throw IOException("EDSI Source Server creation Failed", WHERE);
  }

  // Check Target  EDSI creation
  if (target == NULL) {
    throw IOException("EDSI Target Server creation Failed", WHERE);
  }

  // Check Source Server
  if (!source->checkServer()) {
    throw IOException("Check Source Server Failed", WHERE);
  }

  // Check Target Server
  if (!target->checkServer()) {
    throw IOException("Check Target Server Failed", WHERE);
  }

  // Source must exist
  if (!source->exists()) {
    throw IOException("Source URL does not exist", WHERE);
  }

  // Target must exist
  if (!target->exists()) {
    throw IOException("Target URL does not exist", WHERE);
  }

  // Target must be a Collection
  if (!isCollection(*target)) {
    throw IOException("Target must be a collection", WHERE);
  }

  // Must be a DAV -> DAV export
  if (!isDAVExport(*source, *target)) {
    throw IOException("Must be a DAV->DAV Export", WHERE);
  }

  // Prepare/Determine export type
  initializeExport();
  if (isCollection(*source)) {
    if (!exportCollection(*source, *target)) {
       throw IOException("Export failed - could not export collection", WHERE);
    }
  }
  else { // Resource
    if (!exportResource(*source, *target, true)) {
       throw IOException("Export failed - could not export resource", WHERE);
    }
  }

  p_endTime = XMLPlatformUtils::getCurrentMillis();

  logExport();
  return target->getURL();
}


bool EDSIExporter::exportResource(EDSI &source, EDSI &target, bool checkName)
      throw(IOException)
{
  string resourceName;
  EcceURL *checkURL;
  EcceURL noCheckURL;
  bool ret = false;

  SFile * sfile = TempStorage::getTempFile();
  ofstream ofs(sfile->path().c_str() );
  source.getDataSet(ofs);
  ofs.close();
  ifstream resource(sfile->path().c_str());

  ///////////////////////////////////////////////////////
  // Avoid eccejobstore.xml export - cmcs slide problem 
  string problemResource = "eccejobstorelog.xml";
  string tempResourceName = "thisissilly";
  EcceURL problemURL; 
  bool isProblemResource = false;
  ///////////////////////////////////////////////////////

  resourceName = source.getURL().getFilePathTail();

  // Return if is a .htaccess file
  //  These are not included in an export
  if (strcmp(resourceName.c_str(), ".htaccess") == 0) {
    return true;
  }

  if (strcmp(resourceName.c_str(), problemResource.c_str()) == 0) {
    resourceName = tempResourceName;
    isProblemResource = true; 
  }
  
  if (checkName) {
    checkURL = target.makeDataSet(resourceName);
    if (checkURL != 0) {
      target.setURL(*checkURL);
      if (target.putDataSet(resource)) {
        ret = true;
      }
      delete checkURL;
    }
  }
  else {
    noCheckURL = target.getURL();
    noCheckURL = noCheckURL.getChild(resourceName.c_str());      
    target.setURL(noCheckURL);
    if (target.putDataSet(resource)) {
      ret = true;
    }
  }

  if (ret) {
    if (isProblemResource) {
      problemURL = target.getURL();
      problemURL = problemURL.getParent().getChild(problemResource.c_str());      
      target.moveResource(problemURL); 
    }
    exportMetaData(source, target);
  }
  
  if (ret) {
    p_exportSize += source.getDataSetSize();
    p_resourceCount++;
  }

  sfile->remove();
  delete sfile;

  return ret;
}


bool EDSIExporter::exportCollection(EDSI &source, EDSI &target)
         throw(IOException)
{
  string collectionName;
  EcceURL *colURL;
  vector<ResourceResult> results;
  bool ret = false;

  collectionName = source.getURL().getFilePathTail();
  colURL = target.makeCollection(collectionName);

  if (colURL != 0) {

    EcceURL collectionURL(*colURL);
    delete colURL;

    p_collectionCount++;
    target.setURL(collectionURL);
    if (target.exists()) {
      ret = true;
      exportMetaData(source, target);
      if (source.listCollection(results)) {
        int i = 0;
        while (ret && (i < results.size()) ) {
          source.setURL(results[i].url);
          if (isCollection(source)) {
            ret = exportCollection(source, target);
          }
          else {
            ret = exportResource(source, target);
          }
          target.setURL(collectionURL);
          i++;
        }
      }
    }
  }

  return ret;
}


void EDSIExporter::exportMetaData(EDSI &source, EDSI &target) throw(IOException)
{
  vector<MetaDataRequest> requests;
  vector<MetaDataResult> sourceProperties;
  vector<MetaDataResult> targetProperties;

  if (!source.getMetaData(requests, sourceProperties, false)) {
       throw IOException(target.m_msgStack.getMessage(), WHERE);
  }
  removeLiveProperties(sourceProperties);  
  if (sourceProperties.size() > 0) {
    p_propertyCount += sourceProperties.size();

    // Encapsulate values - avoid namespace replacement, for 
    // properties like :hasinputs
    for (int i = 0; i < sourceProperties.size(); i++) {
      sourceProperties[i].value = "<![CDATA[" + sourceProperties[i].value + "]]>";
    }
    if (!target.putMetaData(sourceProperties)) {
       throw IOException(target.m_msgStack.getMessage(), WHERE);
    }
  }
}


void EDSIExporter::removeLiveProperties(vector<MetaDataResult>& source)
{
  vector<MetaDataResult>::iterator j;

  j = source.begin();
  while (j != source.end()) {
    if ( j->name.find(DAV_NAMESPACE) != string::npos ) {
      j = source.erase(j);
    }
    else {
      j++;
    }
  }
}


bool EDSIExporter::isResource(EDSI &edsi)
{
  vector<MetaDataRequest> requests;
  ResourceMetaDataResult result;
  bool ret = false;

  if (edsi.exists()) {
    edsi.getMetaData(requests, result, false);
    if (result.resourcetype == ResourceDescriptor::RT_DOCUMENT) {
      ret = true;
    }
  }

  return ret;
}


bool EDSIExporter::isCollection(EDSI &edsi)
{
  vector<MetaDataRequest> requests;
  ResourceMetaDataResult result;
  bool ret = false;

  if (edsi.exists()) {
    edsi.getMetaData(requests, result, false);
    if ( (result.resourcetype == ResourceDescriptor::RT_COLLECTION) ||
         (result.resourcetype == ResourceDescriptor::RT_VIRTUAL_DOCUMENT)
       ) {
      ret = true;
    }
  }

  return ret;
}


bool EDSIExporter::isDAVExport(EDSI &source, EDSI &target)
{
  bool ret = false;
  string davServer = "DavEDSI";

  if ( (source.getClassName() == davServer) &&
       (target.getClassName() == davServer) ) {
    ret = true;
  }

  return ret;
}


void EDSIExporter::initializeExport()
{
  p_collectionCount = 0;
  p_resourceCount = 0;
  p_propertyCount = 0;
  p_exportSize = 0;
  p_startTime = XMLPlatformUtils::getCurrentMillis();
  p_endTime = p_startTime;

}


void EDSIExporter::logExport()
{
  char *logFile = 0; 

  logFile = strdup("/tmp/davexport");
  ofstream log(logFile,ios::app);

  log << "\n";
  log << "---------------------------------- \n";
  log << "EXPORT START " << "\n";
  log << "Source: " << p_sourceURL.toString().c_str() << "\n";
  log << "Target: " << p_targetURL.toString().c_str() << "\n";
  log << "---------------------------------- \n";
  log << "\n";
  log << " Number of Collections: " << p_collectionCount << "\n";
  log << "   Number of Resources: " << p_resourceCount << "\n";
  log << "  Number of Properties: " << p_propertyCount << "\n";
  log << "   Export size (bytes): " << p_exportSize  << "\n";
  log << "Elapsed time (seconds): " << ((p_endTime - p_startTime)/1000.0) << "\n";
  log << "---------------------------------- \n";
  log << "EXPORT END " << "\n";
  log << "---------------------------------- \n";
  log << "\n";
  log << "\n";
  log.flush();
  log.close();

  if (logFile) {
    free(logFile);
  }

}

