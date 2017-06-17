#include <stdlib.h>

#include <iostream>
#include <fstream>
using namespace std;

#include "util/ETimer.H"
#include "util/EcceURL.H"
#include "util/CmdLineAuthListener.H"
#include "dsm/DavEDSI.H"
#include "dsm/EDSIFactory.H"


void usage() {
  cout << "usage: put <file> url \n";
  cout << "  used for benchmarking against ftp performance";
  cout << endl;
  exit(-1);
}

int main(int argc, char** argv) {
  if (argc != 3) {
    usage();
  }

  EcceURL projUrl(argv[2]);
  CmdLineAuthListener myauth;
  EDSIFactory::addAuthEventListener(&myauth);
  DavEDSI myinterface(projUrl);
  myinterface.addAuthEventListener(&myauth);
  myinterface.exists();

  // Make sure to include file open and close time
  ETimer timer;
  timer.start();
  ifstream  ifs(argv[1]);
  if (ifs) {


    bool isSuccessful = myinterface.putDataSet(ifs);
    ifs.close();
    timer.stop();
    if (isSuccessful) {
      cout << "Successful " << endl;
    } else {
      cout << "Unsuccessful " << myinterface.m_msgStack.getMessage() << endl;
    }
    cout << "elapsed time " << timer.elapsedTime() << endl;
  } else {
    cout << "Unable to open file " << argv[1] << endl;
  }
  return 0;
}

