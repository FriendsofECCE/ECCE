#include <iostream>
  using std::cerr;
  using std::cout;
  using std::endl;

#include <fstream.h>
#include <unistd.h>

#include "util/EcceURL.H"
#include "dsm/Resource.H"
#include "dsm/EDSIFactory.H"

#include "dsm/FileEDSI.H"
#include "dsm/EDSIFactory.H"
#include "dsm/Resource.H"



int main(int argc, char** argv)
{

  //EcceURL url("/etc/hosts");
  EcceURL url("file:///");
  Resource *resource = EDSIFactory::getResource(url);

  if (resource != 0) {
    cout << "GOT THE RESOURCE !!!!!!" << endl;
  }
  else {
    cout << "GET REOSURCE FAILED !!!!! " << endl;
  }



  /*
  istream *is = NULL;
  if (is = edsi.getDataSet()) {
    char buf[256];
    cout << "dumping file" << endl;
    while (is->getline(buf,255)) {
      cout << buf << endl;
    }
  }

  EcceURL url1("/usr/include");
  FileEDSI edsi1(url1);
  string name = edsi1.uniqueName("listen.h");

  cout << endl;
  cout << "new string is " << name << endl;

  EcceURL url2("/msrc/tmp/lisong");
  FileEDSI edsi2(url2);
  EcceURL *coll = edsi2.makeCollection("wilma");
  if (coll) {
    cout << "made new collection " << coll->toString() << endl;
    delete coll;
  } else {
    cout << "Unable to make collection under " << url2.toString() << endl;
  }
  */


  return 0;
}
