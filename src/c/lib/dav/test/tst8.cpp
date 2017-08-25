// WebDAV test

// include files ***********************************************************
#include <parsers/XercesDOMParser.hpp>
// #include <internal/MemBufInputSource.hpp>

#  include <iostream.h>
#  include <fstream.h>
#include "util/EcceURL.H"
#include "dsm/DavEDSI.H"

int main() {
  EcceURL url;
  url.set( "http", "brig.emsl.pnl.gov", 1080, "/Ecce/users/d3a303");
  DavEDSI myinterface(url);
  vector<MetaDataRequest> myRequests;
  MetaDataRequest myRequest;
  myRequest.name = "DAV:creationdate";
  myRequests.push_back(myRequest);
  vector<ResourceMetaDataResult> lsResults;
  bool isSuccessful = myinterface.listCollection(myRequests, lsResults);
  int numResults = lsResults.size();
  int numMetadata = 0;
  for (int i = 0; i != numResults; i++) {
    cout << lsResults[i].url.toString() << endl;
    cout << lsResults[i].resourcetype << endl;
    numMetadata = lsResults[i].metaData.size();
    for (int j = 0; j != numMetadata; j++) {
      cout << lsResults[i].metaData[j].name << " " << lsResults[i].metaData[j].value << endl;
    } 
  }
  return 0;
}

// end of test.cpp *********************************************************
