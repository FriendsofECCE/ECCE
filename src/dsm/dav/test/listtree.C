
// include files ***********************************************************
#  include <iostream.h>
#  include <fstream.h>
#include "util/CmdLineAuthListener.H"
#include "util/ProgressEvent.H"

#include "util/EcceURL.H"
#include "dsm/DavEDSI.H"
#include "dsm/EDSIFactory.H"

void usage() {
  cout << "usage listtree url " << endl;
  exit(-1);
}

void dumpResults(ResourceMetaDataResult& results)
{
  int numMetadata = results.metaData.size();
  for (int j = 0; j != numMetadata; j++) {
    cout << results.metaData[j].name << "\t" << results.metaData[j].value << endl;
  }
}

void getResults(EcceURL& url) 
{
  cout << "getting results" << endl;
  EDSI * myinterface = EDSIFactory::getEDSI(url);
  vector<ResourceMetaDataResult> lsResults;
  bool isSuccessful = myinterface->listCollection(lsResults);
  if (isSuccessful) {
    int numlsResults = lsResults.size();
    for (int index = 0; index != numlsResults; index++) {
      dumpResults(lsResults[index]);
      if ( lsResults[index].resourcetype != 0)
      {
        getResults(lsResults[index].url);
      }
//      cout << lsResults[index].url.toString() << endl;
    }
  } else {
    cout << "Unsuccessful " << myinterface->m_msgStack.getMessage() << endl;
  }

}


int main(int argc, char** argv) {
  if (argc  < 2 )
    usage();
  CmdLineAuthListener authhandler;
  EDSIFactory::addAuthEventListener(&authhandler);
  EcceURL projUrl(argv[1]);
  getResults (projUrl);
  return 0;
}

