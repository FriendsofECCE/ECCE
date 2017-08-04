#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMNamedNodeMap.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>
using namespace xercesc;

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include "util/EcceURL.H"
#include "tdat/CmdLineCacheAuthListener.H"
#include "tdat/AuthCache.H"
#include "dsm/EDSIFactory.H"
#include "dsm/DavEDSI.H"
#include "dsm/BasicDOMParser.H"
#include "dsm/ResourceDescriptor.H"
#include <strstream>
#include <fstream>
#include <set.h>
#include <map>
using namespace std;


// LOG FILE Name
char *logName = "/tmp/propParse.log";

// PRODUCTION server name
char *prodHost = "eccetera";

// TEST Flag
bool test_flag = false;



void usage() 
{
  cout << endl;
  cout << endl;
  cout << "Usage:" << endl;
  cout << "------" << endl;
  cout << endl;
  cout << "propParse -p -u <URL>" << endl;
  cout << "\tParse property files starting at <URL>, any property files that cannot be parsed " << endl;
  cout << "\twill be recorded in the file /tmp/parseInvalids.  This file can be used with the Copy (-c) option " << endl;
  cout << "\tto copy the invalid property files to a local DAV server so they can be be Modified " << endl;
  cout << "\t(-m) locally as a test before performing the modification on the source files." << endl;
  cout << "\tThe <URL> must be a collection" << endl;
  cout << endl;
  cout << "propParse -c -u <URL> -f <file>" << endl;
  cout << "\tCopy the property files listed in <file> to <URL>.  The <URL> must be a collection" << endl;
  cout << endl;
  cout << "propParse -m -f <file> -t" << endl;
  cout << "\tModify property files listed in <file> to fix missing closing tag.  The modification " << endl;
  cout << "\tis performed in-place.  The -t option indicates a test and the parse will take into account"  << endl;
  cout << "\tthe suffixes that have been added to the end of the file to make it unique. " << endl;
  cout << endl;
  cout << "propParse -l -f <file>" << endl;
  cout << "\tList distinct property file names in <file>.  <file> should be the results from the Parse operation. " << endl;
  cout << "\tThe list of distinct property file names is recorded in the log file (/tmp/propParse.log)" << endl;
  cout << endl;
  cout << endl;
  cout << "Results of propParse will be logged in the file /tmp/propParse.log" << endl;
  cout << endl;
  cout << endl;
  exit(-1);
}



bool is_valid_url(string url_arg) 
{
  bool ret = false;
  EDSI *source = EDSIFactory::getEDSI(url_arg);

  ret = source->exists();
  if (source != NULL) delete source;
 
  return ret;
}


bool is_valid_file(string file_arg) 
{
  ifstream source;
  bool ret = false;

  source.open(file_arg.c_str(), ios::in);
  ret = source.good();
  source.close();

  return ret;
}


bool is_prod_url(string url_arg) 
{
  string::size_type idx;
  EcceURL url(url_arg);
  string host;
  string::iterator it;
  bool ret = true;

  host = url.getHost();
  for (it = host.begin(); it != host.end(); it++) {
    *it = tolower(*it);
  }
  idx = host.find(prodHost);
  if (idx == string::npos) {
    ret = false;
  }

  return ret;
}



bool is_collection(string url)
{
  vector<MetaDataRequest> requests;
  ResourceMetaDataResult result;
  EDSI *edsi = EDSIFactory::getEDSI(url);
  bool ret = false;

  if (edsi->exists()) {
    edsi->getMetaData(requests, result);
    if ( (result.resourcetype == ResourceDescriptor::RT_COLLECTION) ||
         (result.resourcetype == ResourceDescriptor::RT_VIRTUAL_DOCUMENT)
       ) {
      ret = true;
    }
  }
  if (edsi != NULL) delete edsi;
  return ret;
}




bool is_problem_prop(string propType, string &close_tag, bool is_test) 
{
  bool ret = false;

  if (is_test) {
    if ( (propType.find("DELTAE") != string::npos) ||
         (propType.find("DEWVEC") != string::npos) || 
         (propType.find("EAM1VEC") != string::npos) || 
         (propType.find("EGRAD") != string::npos) || 
         (propType.find("ESCFVEC") != string::npos) || 
         (propType.find("EWVEC") != string::npos) || 
         (propType.find("TEVEC") != string::npos)
       ) {
      close_tag = "</tsvector>";
      ret = true;
    }
    if (propType.find("GEOMTRACE") != string::npos) {
      close_tag = "</tsvectable>";
      ret = true;
    }
  }
  else { 
    if ( (propType == "DELTAE") ||
         (propType == "DEWVEC") || 
         (propType == "EAM1VEC") || 
         (propType == "EGRAD") || 
         (propType == "ESCFVEC") || 
         (propType == "EWVEC") || 
         (propType == "TEVEC")
       ) {
      close_tag = "</tsvector>";
      ret = true;
    }
    if (propType == "GEOMTRACE") {
      close_tag = "</tsvectable>";
      ret = true;
    }
  }
  
  return ret;
} 



void parseProp(string url_arg) {

  // Source URL
  EcceURL sourceUrl(url_arg);

  // Create source EDSI
  EDSI *source = EDSIFactory::getEDSI(sourceUrl);

  vector<EcceURL> findResults;
  vector<EcceURL> invalidProps;
  BasicDOMParser parser;
  DOMDocument *propDoc;
  char *destbuf;
  char *sourcebuf;
  unsigned long startTime;
  unsigned long endTime;



  startTime = XMLPlatformUtils::getCurrentMillis();
  endTime = startTime;
  bool status = source->efindProp(sourceUrl, findResults);
  if (status == false) {
    cout << "FIND FAILED" << "\n"; 
  }
  else {
    int i = 0;
    cout << "FIND SUCCEEDED" << "\n";
    while (i < findResults.size() ) {
      try {
        propDoc = 0;
        source->setURL(findResults[i]);
        if (source->exists()) {
          ostrstream dest;
          if (source->getDataSet(dest)) {
            dest << ends;
            destbuf = dest.rdbuf()->str();
            sourcebuf = strdup(destbuf);
            istrstream isource(sourcebuf);
            propDoc = parser.parse(isource);
          }
        } 
      }
      catch (const EcceException& toCatch) {
        invalidProps.push_back(findResults[i]);
      }
      delete propDoc;
      if (sourcebuf) {
        free(sourcebuf);
      }
      if (destbuf) {
        free(destbuf);
      }
      i++;
    }
  }
  if (source != NULL) delete source;
  endTime = XMLPlatformUtils::getCurrentMillis();


  // Log Results
  char *logFile = 0;
  logFile = strdup(logName);
  ofstream log(logFile,ios::app);

  log << "\n";
  log << "---------------------------------- \n";
  log << "PARSE RESULTS " << "\n";
  log << "\n";
  log << "Source: " << sourceUrl.toString().c_str() << "\n";
  log << "---------------------------------- \n";
  log << "\n";
  log << "\n";
  log << "         Number of Property Resources: " << findResults.size() << "\n";
  log << " Number of Invalid Property Resources: " << invalidProps.size() << "\n";
  log << "                     Percent Invalids: " << ((findResults.size() > 0) ? (((float)invalidProps.size()/(float)findResults.size())*100.0) : 0) << "\n";
  log << "               Elapsed time (minutes): " << (((endTime - startTime)/1000.0)/60.0) << "\n";
  log << "\n";
  log << "\n";
  log << "---------------------------------- \n";
  log << "---------------------------------- \n";
  log << "---------------------------------- \n";
  log << "\n";
  log << "\n";
  log << "\n";
  log << "\n";
  log.flush();
  log.close();
  if (logFile) {
    free(logFile);
  }

  // Log Invalids
  char *invalidsName = "/tmp/parseInvalids";
  char *invalidsFile = 0;
  invalidsFile = strdup(invalidsName);
  ofstream invalids(invalidsFile,ios::trunc);
  int j = 0;
  while (j < invalidProps.size() ) {
    invalids << invalidProps[j].toString().c_str() << "\n";
    j++;
  }
  invalids.flush();
  invalids.close();
  if (invalidsFile) {
    free(invalidsFile);
  }
}



void copyProp(string url_arg, string file_arg) {

  ifstream invalids;
  EcceURL targetUrl(url_arg);
  EDSI *target = EDSIFactory::getEDSI(targetUrl);
  EDSI *source = EDSIFactory::getEDSI(targetUrl);
  string propUrl;
  char *destbuf;
  char *sourcebuf;
  bool copiedUrl;
  int sourceUrlCount = 0;
  int copiedUrlCount = 0;
  vector<string> notCopiedUrl;
  vector<string>::iterator it;
  unsigned long startTime;
  unsigned long endTime;


  startTime = XMLPlatformUtils::getCurrentMillis();
  endTime = startTime;
  if (target->exists() && is_collection(target->getURL().toString())) {
    invalids.open(file_arg.c_str(), ios::in);
    if (invalids.good()) {
      //Get Invalids 
      while (!invalids.eof() && !invalids.fail()) {
        copiedUrl = false;
        propUrl = "";
        invalids >> propUrl;
        if (!invalids.eof()) sourceUrlCount++;
        if (is_valid_url(propUrl)) {
          EcceURL sourceUrl(propUrl);
          source->setURL(sourceUrl);
          if (source->exists() && !is_collection(source->getURL().toString())) {
            ostrstream dest;
            string sourceName = sourceUrl.getFilePathTail();
            if (source->getDataSet(dest)) {
              dest << ends;
              destbuf = dest.rdbuf()->str();
              sourcebuf = strdup(destbuf);
              istrstream isource(sourcebuf);
              // Copy property file - make sure it has a unique name on target
              string newSourceName = target->uniqueName(sourceName);
              target->setURL(targetUrl.getChild(newSourceName.c_str()));
              if (target->putDataSet(isource)) {
                copiedUrl = true;
              }
              // Restore to target collection for next copy
              target->setURL(targetUrl);
              if (destbuf) free(destbuf);
              if (sourcebuf) free(sourcebuf);
            }
          }
        }
        if (copiedUrl) {
          copiedUrlCount++;
        }
        else {
          if (!invalids.eof()) notCopiedUrl.push_back(propUrl);
        }
      } 
    }
  }
  if (target != NULL) delete target;
  if (source != NULL) delete source;
  endTime = XMLPlatformUtils::getCurrentMillis();


  // Log results
  char *logFile = 0;
  logFile = strdup(logName);
  ofstream log(logFile,ios::app);

  log << "\n";
  log << "---------------------------------- \n";
  log << "COPY RESULTS " << "\n";
  log << "\n";
  log << "Source: " << file_arg << "\n";
  log << "Target: " << url_arg << "\n";
  log << "---------------------------------- \n";
  log << "\n";
  log << "\n";
  log << "   Number of Property Url's Listed: " << sourceUrlCount << "\n";
  log << "                     Number copied: " << copiedUrlCount << "\n";
  log << "   Number NOT copied (i.e. error?): " << notCopiedUrl.size() << "\n";
  log << "            Elapsed time (minutes): " << (((endTime - startTime)/1000.0)/60.0) << "\n";
  log << "\n";
  log << "\n";
  log << " Url's NOT Copied" << "\n";
  log << " ----------------" << "\n";
  for (it = notCopiedUrl.begin(); it != notCopiedUrl.end(); it++) {
    log << "  " << *it << endl;
  }
  log << "\n";
  log << "---------------------------------- \n";
  log << "---------------------------------- \n";
  log << "---------------------------------- \n";
  log << "\n";
  log << "\n";
  log << "\n";
  log << "\n";

  log.flush();
  log.close();
  if (logFile) {
    free(logFile);
  }

}



void modifyProp(string file_arg) {

  ifstream invalids;
  string propUrl;
  string closeTag;
  bool modifiedUrl;
  int urlCount = 0;
  int modifiedUrlCount = 0;
  vector<string> notModifiedUrl;
  vector<string>::iterator it;
  char *destbuf;
  char *sourcebuf;
  BasicDOMParser parser;
  DOMDocument *propDoc = 0;
  unsigned long startTime;
  unsigned long endTime;


  startTime = XMLPlatformUtils::getCurrentMillis();
  endTime = startTime;
  invalids.open(file_arg.c_str(), ios::in);
  if (invalids.good()) {
    //Get Invalids 
    while (!invalids.eof() && !invalids.fail()) {
      modifiedUrl = false;
      propUrl = "";
      invalids >> propUrl;
      if (!invalids.eof()) urlCount++;
      if (is_valid_url(propUrl)) {
        EcceURL targetUrl(propUrl);
        string targetName = targetUrl.getFilePathTail();
        closeTag = "";
        EDSI *target = EDSIFactory::getEDSI(targetUrl);
        if (target->exists() && is_problem_prop(targetName, closeTag, test_flag)) {
          // Attempt to parse, if an exception is thrown then 
          //  append closing tag to end of file
          try {
            propDoc = 0;
            ostrstream dest;
            if (target->getDataSet(dest)) {
              dest << ends;
              destbuf = dest.rdbuf()->str();
              sourcebuf = strdup(destbuf);
              istrstream isource(sourcebuf);
              propDoc = parser.parse(isource);
            }
          }
          catch (const EcceException& toCatch) {
            if (target->appendDataSet(closeTag.c_str())) {
              modifiedUrl = true;
            }
          }
          // Clean Up
          delete propDoc;
          if (sourcebuf) free(sourcebuf);
          if (destbuf) free(destbuf);
        }
        if (target != NULL) delete target;
      }
      if (modifiedUrl) {
        modifiedUrlCount++;
      }
      else {
        if (!invalids.eof()) notModifiedUrl.push_back(propUrl);
      }
    }
  } 
  endTime = XMLPlatformUtils::getCurrentMillis();


  // Log results
  char *logFile = 0;
  logFile = strdup(logName);
  ofstream log(logFile,ios::app);

  log << "\n";
  log << "---------------------------------- \n";
  log << "MODIFY RESULTS " << "\n";
  log << "\n";
  log << "Source File: " << file_arg << "\n";
  log << "---------------------------------- \n";
  log << "\n";
  log << "\n";
  log << "   Number of Property Url's Listed: " << urlCount << "\n";
  log << "                   Number modified: " << modifiedUrlCount << "\n";
  log << " Number NOT modified (i.e. error?): " << notModifiedUrl.size() << "\n";
  log << "            Elapsed time (minutes): " << (((endTime - startTime)/1000.0)/60.0) << "\n";
  log << "\n";
  log << "\n";
  log << " Url's NOT Modified" << "\n";
  log << " ------------------" << "\n";
  for (it = notModifiedUrl.begin(); it != notModifiedUrl.end(); it++) {
    log << "  " << *it << endl;
  }
  log << "\n";
  log << "---------------------------------- \n";
  log << "---------------------------------- \n";
  log << "---------------------------------- \n";
  log << "\n";
  log << "\n";
  log << "\n";
  log << "\n";

  log.flush();
  log.close();
  if (logFile) {
    free(logFile);
  }

}





void listProp(string file_arg) {

  ifstream invalids; 
  string propUrl;
  string propName;
  map<string, int> propNames;
  map<string, int>::iterator mit;
  set<string> invalidUrls;
  set<string>::iterator it;
  int numInvalids = 0;
  unsigned long startTime;
  unsigned long endTime;


  startTime = XMLPlatformUtils::getCurrentMillis();
  endTime = startTime;
  // Open file and retrieve property file names
  invalids.open(file_arg.c_str(), ios::in);
  if (invalids.good()) {
    //Get Invalids 
    while (!invalids.eof() && !invalids.fail()) {
      propUrl = "";
      invalids >> propUrl;
      if (is_valid_url(propUrl)) {
        EcceURL url(propUrl);
        propName = url.getFilePathTail();
        propNames[propName]++; 
        numInvalids++;
      }
      else {
        if (propUrl != "") {
          invalidUrls.insert(propUrl);
        }
      }
    } 
  }
  endTime = XMLPlatformUtils::getCurrentMillis();

  // Log results
  char *logFile = 0;
  logFile = strdup(logName);
  ofstream log(logFile,ios::app);

  log << "\n";
  log << "---------------------------------- \n";
  log << "LIST RESULTS " << "\n";
  log << "\n";
  log << "Source: " << file_arg << "\n";
  log << "---------------------------------- \n";
  log << "\n";
  log << "\n";
  log << "   Number of Property Url's Listed: " << numInvalids << "\n";
  log << "  Number of Invalid Property Url's: " << invalidUrls.size() << "\n";
  log << " Number of Distinct Property Names: " << propNames.size() << "\n";
  log << "            Elapsed time (minutes): " << (((endTime - startTime)/1000.0)/60.0) << "\n";
  log << "\n";
  log << " Distinct Property Names " << "\n";
  log << " -----------------------" << "\n";
  for (mit = propNames.begin(); mit != propNames.end(); mit++) {
    log << "  " << mit->first << "  (" << mit->second << ")" << endl;
  }
  log << "\n";
  log << " Invalid Url's " << "\n";
  log << " -------------" << "\n";
  for (it = invalidUrls.begin(); it != invalidUrls.end(); it++) {
    log << "  " << *it << endl;
  }
  log << "\n";
  log << "---------------------------------- \n";
  log << "---------------------------------- \n";
  log << "---------------------------------- \n";
  log << "\n";
  log << "\n";
  log << "\n";
  log << "\n";

  log.flush();
  log.close();
  if (logFile) {
    free(logFile);
  }
}





int main(int argc, char** argv) {

  CmdLineCacheAuthListener authhandler;
  EDSIFactory::addAuthEventListener(&authhandler);

  // Get program options and arguments
  int parse_flag = 0;
  int copy_flag  = 0;
  int modify_flag  = 0;
  int list_flag  = 0;
  int error_flag = 0;
  string url_arg;
  string file_arg;
  int c;
  char buffer[10];
  while ((c = getopt(argc, argv, "pcmltu:f:")) != -1) {
    switch (c) {  
         case 'p': 
           if (copy_flag || modify_flag || list_flag) {
             error_flag++;
           }
           else {
             parse_flag++;
           }
           break;
         case 'c': 
           if (parse_flag || modify_flag || list_flag) {
             error_flag++;
           }
           else {
             copy_flag++;
           }
           break;
         case 'm': 
           if (copy_flag || parse_flag || list_flag) {
             error_flag++;
           }
           else {
             modify_flag++;
           }
           break;
         case 'l':
           if (copy_flag || modify_flag || parse_flag) {
             error_flag++;
           }
           else {
             list_flag++;
           }
           break;
         case 't':
           test_flag = true;
           break;
         case 'u': 
           url_arg = optarg;
           break;
         case 'f': 
           file_arg = optarg;
           break;
         case ':':
           error_flag++;
           break;
         case '?': 
           error_flag++;
           break;
         default:
           error_flag++;
           break;
    }
  }

  // Verify options and arguments
  if ( (error_flag) || 
       (!(parse_flag || copy_flag || modify_flag || list_flag)) ) {
    usage();
  }

  // Parse
  if (parse_flag) {
    if (!(is_valid_url(url_arg))) { 
      cout << "Must specify a valid <URL>" << endl; 
      usage();
    }
    else {
      parseProp(url_arg);
    }
  }

  // Copy
  if (copy_flag) {
    if (!(is_valid_url(url_arg) && is_valid_file(file_arg))) { 
      cout << "Must specify a valid <URL> and valid <file>" << endl; 
      usage();
    }
    else {
      if (is_prod_url(url_arg)) {
        cout << "Cannot use " << prodHost << " as a target for the copy" << endl; 
        usage();
      }
      else {
        copyProp(url_arg, file_arg);
      }
    }
  }


  // Modify
  if (modify_flag) {
    if (!(is_valid_file(file_arg))) { 
      cout << "Must specify a valid <file>" << endl; 
      usage();
    }
    else {
      modifyProp(file_arg);
    }
  }


  // List
  if (list_flag) {
    if (!(is_valid_file(file_arg))) { 
      cout << "Must specify a valid <file>" << endl; 
      usage();
    }
    else {
      listProp(file_arg);
    }
  }

}


