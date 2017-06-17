#include <iostream>
#include <fstream>
#include <strstream>
  using namespace std;

#include "util/CmdLineAuthListener.H"
#include "util/ProgressEvent.H"

#include "util/EcceURL.H"
#include "dsm/EDSIFactory.H"

#include "dsm/EcceDAVClient.H"

void usage() {
  cout << "usage putdataset url value-filename" << endl;
  exit(-1);
}

int main(int argc, char** argv) {

  // This script is in /msrc/www/htbin/ecce/ecce_accounts
  string url = "http://www.emsl.pnl.gov:80/cgi-bin/ecce/posttest.pl?action=exists&username=d39842";
  //string url = "http://www.emsl.pnl.gov:80/cgi-bin/ecce/ecce_accounts?action=exists&username=d39842";
  EcceDAVClient edc(url);

  ostrstream ostr;
  int status = edc.doPost(ostr);
  ostr << ends;
  char *cptr = ostr.str();

  cout << "-------------------------" << endl;
  cout << "results " << endl;
  cout << "-------------------------" << endl;
  cout << "status " << status << endl;
  cout << "content " << cptr << endl;

  delete [] cptr;

  return 0;
}

