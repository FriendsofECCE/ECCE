
// include files ***********************************************************
#  include <iostream.h>
#  include <fstream.h>
#include "util/CmdLineAuthListener.hpp"
#include "util/ProgressEvent.hpp"
#include "util/EcceURL.hpp"
#include "dsm/EDSIFactory.hpp"

#include "dsm/DavEDSI.hpp"

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

