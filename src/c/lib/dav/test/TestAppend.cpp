
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

void usage() 
{
  cout << "Usage:\n\t TestAppend url value-filename bytes-to-overwrite" << endl;
  exit(-1);
}

int main(int argc, char** argv) 
{
  if (argc < 3 || argc > 4) {
    usage();
  }
  int bytesToOverwrite = 0;


  EcceURL url(argv[1]);
  CmdLineAuthListener authhandler;
  EDSIFactory::addAuthEventListener(&authhandler);

  EDSI *myinterface = EDSIFactory::getEDSI(url);

  ifstream  ifs(argv[2]);

  if (argc == 4) {
     bytesToOverwrite = atoi(argv[3]);
  }


  cout << "Append at " << url.toString() << " from " << argv[2] << " over " << bytesToOverwrite << endl;
  if (ifs) {

     bool isSuccessful = myinterface->appendDataSet(ifs,bytesToOverwrite);

     if (isSuccessful) {
        cout << "Successful " << endl;
     } else {
        cout << "Unsuccessful " << myinterface->m_msgStack.getMessage() << endl;
     }
  } else {
     cout << "Could not read input file" << endl;
  }
  return 0;

}

