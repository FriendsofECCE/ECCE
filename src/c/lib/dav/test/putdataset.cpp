
// include files ***********************************************************
#  include <iostream>
#  include <fstream>
#  include <strstream>
using namespace std;
#include "util/CmdLineAuthListener.hpp"
#include "util/ProgressEvent.hpp"

#include "util/EcceURL.hpp"
#include "dsm/EDSIFactory.hpp"

#include "dsm/DavEDSI.hpp"

void usage() {
  cout << "usage putdataset url value-filename" << endl;
  exit(-1);
}

int main(int argc, char** argv) {
  if (argc != 3)
    usage();
  EcceURL projUrl(argv[1]);
  CmdLineAuthListener authhandler;
  EDSIFactory::addAuthEventListener(&authhandler);

  EDSI *myinterface = EDSIFactory::getEDSI(projUrl);

  string stringbuf;
  ifstream  ifs(argv[2]);
  if (!ifs)
  {
    return false;
  }
  ostrstream ostr;
  ostr << ifs.rdbuf();
  ostr << ends;
  stringbuf = ostr.str();
  bool isSuccessful = myinterface->putDataSet(stringbuf.c_str());
  if (isSuccessful) {
    cout << "Successful " << endl;
  } else {
    cout << "Unsuccessful " << myinterface->m_msgStack.getMessage() << endl;
  }
  return 0;
}

