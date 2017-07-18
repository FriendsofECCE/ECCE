#include <iostream>

#include <fstream>

#include <strstream>
  using namespace std;

#include "util/CmdLineAuthListener.H"
#include "util/ProgressEvent.H"
#include "util/EcceURL.H"
#include "util/SFile.H"
#include "util/ETimer.H"
#include "util/StringConverter.H"
#include "dsm/EDSIFactory.H"
#include "dsm/DavEDSI.H"



void usage() {
  cout << "usage <num files | int> <file size | '1|2|3'> <target | url>" << endl;
  exit(-1);
}


int main(int argc, char** argv) {

  if (argc != 5) {
    usage();
  }

  int numFiles = atoi(argv[1]); 
  string fileSize = argv[2];
  string file1 = "/home/d3j766/ecce_dev/devroot/src/libs/dsm/dav/benchmarks/500kb";
  string file2 = "/home/d3j766/ecce_dev/devroot/src/libs/dsm/dav/benchmarks/20mb";
  string file3 = "/home/d3j766/ecce_dev/devroot/src/libs/dsm/dav/benchmarks/200mb";

  CmdLineAuthListener authhandler;
  EDSIFactory::addAuthEventListener(&authhandler);

  SFile uploadFile;
  if (fileSize == "1") {
    uploadFile = file1;
  }
  if (fileSize == "2") {
    uploadFile = file2;
  }
  if (fileSize == "3") {
    uploadFile = file3;
  }

  EcceURL projUrl(argv[3]);
  string fileName = argv[4];
  projUrl = projUrl.getChild(fileName);
  EDSI *myinterface = EDSIFactory::getEDSI(projUrl);

cout << "Upload file :: " << uploadFile.path() << endl;
cout << "Upload file name :: " << uploadFile.filename() << endl;
cout << "Target URL :: " << projUrl.toString() << endl;

  // force authentication
  myinterface->putDataSet("test");

  ETimer timer;
  timer.start();
  for (int i = 0; i < numFiles; i++) {

    // make sure file name is unique
    projUrl = projUrl.getParent();
    projUrl = projUrl.getChild(fileName + "-" + StringConverter::toString(i));
    myinterface->setURL(projUrl);

    ifstream is(uploadFile.path().c_str());
    if (! myinterface->putDataSet(is)) {
      cout << "PUT FAILED" << endl;
    }

    is.close();
  }
  timer.stop();
  cout << "Elapsed time for PUT operation :: " << timer.elapsedTime() << endl;

  return 0;
}

