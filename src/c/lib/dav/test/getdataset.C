
// include files ***********************************************************
#  include <iostream.h>
#  include <fstream.h>
#include "util/CmdLineAuthListener.H"
#include "util/ProgressEvent.H"
#include "util/EcceURL.H"
#include "dsm/EDSIFactory.H"

#include "dsm/DavEDSI.H"

void usage() {
  cout << "usage getdataset <url> <file> " << endl;
  exit(-1);
}

int main(int argc, char** argv) {
  if (argc != 3)
    usage();

  CmdLineAuthListener authhandler;
  EDSIFactory::addAuthEventListener(&authhandler);
//ADD EDSI FActory GET EDSI!!!!!!!!!!!!!!!!!
  EcceURL projUrl(argv[1]);
  EDSI *myinterface = EDSIFactory::getEDSI(projUrl);
  ofstream mystr(argv[2]);
  if (myinterface)
  {
  if (myinterface->getDataSet(mystr)) {
    cout << "should have saved file" << endl;
  } else {
    cout << "Unsuccessful " << myinterface->m_msgStack.getMessage() << endl;
  }
  mystr.close();
  }
  return 0;
}

