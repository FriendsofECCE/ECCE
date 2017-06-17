
// include files ***********************************************************
#  include <iostream.h>
#  include <fstream.h>
#include "util/EcceURL.H"
#include "dsm/DavEDSI.H"

void usage() {
  cout << "usage describemetadata url "<< endl;
  exit(-1);
}

int main(int argc, char** argv) {
  if (argc != 2)
    usage();
  int idx;
  EcceURL projUrl(argv[1]);
  DavEDSI myinterface(projUrl);
  MetaDataResult mrs;
  vector<MetaDataResult> vmrs;
  vector<MetaDataResult>::iterator vmrsIt;
  for (idx=0; idx<50; idx++ ) {
    cout << "INDEX " << idx << endl;
  //  cout << "doing head" << endl;
  //  myinterface.exists();
  //  cout << "doing head" << endl;
  //  myinterface.exists();
    cout << "describe" << endl;
  bool isSuccessful = myinterface.describeMetaData(vmrs);
  if (isSuccessful) {
    vmrsIt = vmrs.begin();
    while (vmrsIt != vmrs.end()) {
      cout <<  (*vmrsIt).name << "      " << (*vmrsIt).value << endl;
      *vmrsIt++;
    }
  } else {
    cout << "Unsuccessful " << myinterface.m_msgStack.getMessage() << endl;
  }
  }
  return 0;
}

