#include <iostream.h>
#include <fstream.h>

#include <string>
#include <vector>
  using std::string;
  using std::vector;
  using std::find;

#include "tdat/CmdLineCacheAuthListener.H"
#include "tdat/AuthCache.H"

#include "util/EcceURL.H"
#include "dsm/DavEDSI.H"
#include "dsm/EDSIFactory.H"


void usage() {
  cout << "usage getmetadata url [mdr rmdr] request1 request2 ..." << endl;
  exit(-1);
}

int main(int argc, char** argv) 
{

  if (argc < 3) {
    usage();
  }

cout << "111Enter getmetadata .... " << endl;

  CmdLineCacheAuthListener authhandler;
  EDSIFactory::addAuthEventListener(&authhandler);
  EcceURL projUrl(argv[1]);
  EDSI *myinterface =  EDSIFactory::getEDSI(projUrl);

cout << "Enter getmetadata .... " << endl;

  string resultType = argv[2];
  vector<MetaDataRequest> myRequests;
  MetaDataRequest myRequest;
  for (int index = 3; index != argc; index++) {
cout << "Request Name :: " << argv[index] << endl;
    myRequest.name = argv[index];
    myRequests.push_back(myRequest);
  }



  if (resultType.find("rmdr") != string::npos)  { 
cout << "Before request meta data (rmdr) " << endl;
    ResourceMetaDataResult lsResults;
    bool isSuccessful = myinterface->getMetaData(myRequests, lsResults);
    if (isSuccessful) {
      cout << "results " << isSuccessful << endl;
      cout << lsResults.url.toString() << endl;
      cout << lsResults.resourcetype << endl;
      cout << "Number of results :: " << lsResults.metaData.size() << endl;
      int numMetadata = lsResults.metaData.size();
      for (int j = 0; j != numMetadata; j++) {
        cout << lsResults.metaData[j].name << " " << lsResults.metaData[j].value << endl;
      }
    } 
    else {
      cout << "Unsuccessful " << myinterface->m_msgStack.getMessage() << endl;
    }
  } 
  else if (resultType.find("mdr") != string::npos)  { 
cout << "Before request meta data (mdr) " << endl;
    MetaDataResult mrs;
    vector<MetaDataResult> vmrs;
    vector<MetaDataResult>::iterator vmrsIt;
    bool isSuccessful = myinterface->getMetaData(myRequests, vmrs);
    if (isSuccessful) {
      vmrsIt = vmrs.begin();
      while (vmrsIt != vmrs.end()) {
        cout <<  (*vmrsIt).name << "      " << (*vmrsIt).value << endl;
        *vmrsIt++;
      }
    } else {
      cout << "Unsuccessful " << myinterface->m_msgStack.getMessage() << endl;
    }
  }
  else {
cout << "Before request meta data (dmd) " << endl;
    MetaDataResult mrs;
    vector<MetaDataResult> vmrs;
    vector<MetaDataResult>::iterator vmrsIt;
    bool isSuccessful = myinterface->describeMetaData(vmrs);
    if (isSuccessful) {
      vmrsIt = vmrs.begin();
      while (vmrsIt != vmrs.end()) {
        cout <<  (*vmrsIt).name << "      " << (*vmrsIt).value << endl;
        *vmrsIt++;
      }
    } else {
      cout << "Unsuccessful " << myinterface->m_msgStack.getMessage() << endl;
    }



  }
cout << "Leaving program ... " << endl;
  return 0;
}

