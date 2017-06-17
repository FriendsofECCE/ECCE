
// include files ***********************************************************
#  include <iostream.h>
#  include <fstream.h>
#include "util/EcceURL.H"
#include "dsm/DavEDSI.H"

void usage() {
  cout << "usage putmetadata url key value-filename" << endl;
  exit(-1);
}

int main(int argc, char** argv) {
  if (argc != 4)
    usage();
  EcceURL projUrl(argv[1]);
  DavEDSI myinterface(projUrl);
  string resultType = argv[2];
  MetaDataResult mrs;
  vector<MetaDataResult> vmrs;

  ifstream  ifs(argv[3]);
  if (!ifs)
  {
    return false;
  }
  ifs >> mrs.value; 
  mrs.name = argv[2]; 
  vmrs.push_back(mrs);
  bool isSuccessful = myinterface.putMetaData(vmrs);
  if (isSuccessful) {
    cout << "Successful " << endl;
  } else {
    cout << "Unsuccessful " << myinterface.m_msgStack.getMessage() << endl;
  }
  return 0;
}

