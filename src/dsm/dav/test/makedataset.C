
// include files ***********************************************************
#  include <iostream.h>
#  include <fstream.h>
#include "util/EcceURL.H"
#include "dsm/DavEDSI.H"

void usage() {
  cout << "usage makedataset url value-filename" << endl;
  exit(-1);
}

int main(int argc, char** argv) {
  if (argc != 2)
    usage();
  EcceURL projUrl(argv[1]);
  cout << "URLS is " << projUrl.toString().data() << endl;
  DavEDSI myinterface(projUrl);
  cout << "calling make data set" << endl;

  EcceURL *newdoc = myinterface.makeDataSet("empty");
  if (newdoc) {
    cout << "Successful " << newdoc->toString().data() << endl;
    delete newdoc;
  } else {
    cout << "Unsuccessful " << myinterface.m_msgStack.getMessage().data() << endl;
  }
  return 0;
}

