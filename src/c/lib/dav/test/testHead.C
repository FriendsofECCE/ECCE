#include <unistd.h>
#include <iostream.h>

#include "dsm/DavEDSI.H"
#include "util/CmdLineAuthListener.H"

#include <vector>
#include <string>
int main(int argc, char** argv) {
  if (argc != 2) {
    cout << "Usage: testHead <url>" << endl;
    exit (-1);
  }

  CmdLineAuthListener authhandler;

  EcceURL projUrl(argv[1]);
  DavEDSI myinterface(projUrl);
  myinterface.addAuthEventListener(&authhandler);
  for (int idx=0; idx<5; idx++) {
    bool status = myinterface.exists();
    string msg = myinterface.m_msgStack.getMessage();
    if (msg != "") {
      cout << "oh oh - something is wrong: " << msg << endl;
      cout << msg << endl;
    } else {
      cout << "exists? " << (int) status << endl;; 
    }
  }

}
