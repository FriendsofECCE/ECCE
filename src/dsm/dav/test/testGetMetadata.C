#include <iostream>
  using namespace std;

#include <vector>
  using std::vector;
#include <string>
  using std::string;

#include "dsm/EDSI.H"
#include "dsm/EDSIFactory.H"
#include "util/CmdLineAuthListener.H"


int main(int argc, char** argv) 
{
   if (argc < 3) {
      cout << "Usage: testGetMetadata <url> <property>" << endl;
      cout << "\t testGetMetadata http://fletcher.pnl.gov:8080/sam/files/users/karen/xxx http://www.emsl.pnl.gov/ecce:hasConfigurations" << endl;
      exit(-1);
   }


    // arg0 = program name
    EcceURL url = argv[1];
    string property = argv[2];

cout << "url " << url.toString() << endl;
cout << "property " << property << endl;

    CmdLineAuthListener myauth;

    EDSI *edsi = EDSIFactory::getEDSI(url);
    edsi->addAuthEventListener(&myauth);

    MetaDataRequest mrq;
    MetaDataResult mrs;
    vector<MetaDataRequest> vmrq;
    vector<MetaDataResult> vmrs;
    vector<MetaDataResult>::iterator vmrsIt;

    mrq.name = property;
    vmrq.push_back(mrq);

    edsi->getMetaData(vmrq, vmrs);
    vmrsIt = vmrs.begin();
    while (vmrsIt != vmrs.end()) {
      cout <<  (*vmrsIt).name << "      " << (*vmrsIt).value << endl; 
      *vmrsIt++;
    }
    delete edsi;
}
