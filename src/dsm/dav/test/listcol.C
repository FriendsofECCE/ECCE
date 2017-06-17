
// include files ***********************************************************
#  include <iostream.h>
#  include <fstream.h>
#include "util/EcceURL.H"
#include "dsm/DavEDSI.H"

void usage() {
  cout << "usage listcol url [rr rmdr allrmdr] request1 request2 ..." << endl;
  exit(-1);
}

int main(int argc, char** argv) {
  if (argc  < 2 )
    usage();
  EcceURL projUrl(argv[1]);
  DavEDSI myinterface(projUrl);
  string resultType;
  if (argc >= 3 )
    resultType = argv[2];
  else 
    resultType = "brief";
  vector<MetaDataRequest> myRequests;
  MetaDataRequest myRequest;
  if (resultType.find("brief") != string::npos) {
    vector<ResourceResult> lsResults;
    bool isSuccessful = myinterface.listCollection(lsResults);
    cout << "listcol ret" << isSuccessful << endl;
    if (isSuccessful) {
      int numlsResults = lsResults.size();
      for (int index = 0; index != numlsResults; index++) {
        cout << lsResults[index].url.toString() << endl;
      }
    } else {
      cout << "Unsuccessful " << myinterface.m_msgStack.getMessage() << endl;
    }
    exit(0);

  }
  /*
  for (int index = 3; index < argc; index++) {
    myRequest.name = argv[index];
    myRequests.push_back(myRequest);
  }
  if (resultType.find("rmdr") != string::npos)  { 
    vector<ResourceMetaDataResult> lsResults;
    bool isSuccessful = myinterface.listCollection(myRequests, lsResults);
    if (isSuccessful) {
      cout << "results " << isSuccessful << endl;
      int numlsResults = lsResults.size();
      for (int index = 0; index != numlsResults; index++) {
        cout << lsResults[index].url.toString() << endl;
        cout << lsResults[index].resourcetype << endl;
        cout << lsResults[index].contenttype << endl;
        int numMetadata = lsResults[index].metaData.size();
        for (int j = 0; j != numMetadata; j++) {
          cout << lsResults[index].metaData[j].name << " " << lsResults[index].metaData[j].value << endl;
        }
      }
    } else {
      cout << "Unsuccessful " << myinterface.m_msgStack.getMessage() << endl;
    }
  } else if (resultType.find("rr") != string::npos) {
    vector<ResourceResult> lsResults;
    bool isSuccessful = myinterface.listCollection(lsResults);
    if (isSuccessful) {
      cout << "results " << isSuccessful << endl;
      int numlsResults = lsResults.size();
      for (int index = 0; index != numlsResults; index++) {
        cout << lsResults[index].url.toString() << endl;
        cout << lsResults[index].resourcetype << endl;
        cout << lsResults[index].contenttype << endl;
      }
    } else {
      cout << "Unsuccessful " << myinterface.m_msgStack.getMessage() << endl;
    }
  } else {
    vector<ResourceMetaDataResult> lsResults;
    bool isSuccessful = myinterface.listCollection(lsResults);
    if (isSuccessful) {
      cout << "results " << isSuccessful << endl;
      int numlsResults = lsResults.size();
      for (int index = 0; index != numlsResults; index++) {
        cout << lsResults[index].url.toString() << endl;
        cout << lsResults[index].resourcetype << endl;
        cout << lsResults[index].contenttype << endl;
        int numMetadata = lsResults[index].metaData.size();
        for (int j = 0; j != numMetadata; j++) {
          cout << lsResults[index].metaData[j].name << " " << lsResults[index].metaData[j].value << endl;
        }
      }
    } else {
      cout << "Unsuccessful " << myinterface.m_msgStack.getMessage() << endl;
    }
  }
  */
  return 0;
}

