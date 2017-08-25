#include <iostream>
using namespace std;
#include <stdlib.h>
#include "util/EcceURL.H"
#include "tdat/CmdLineCacheAuthListener.H"
#include "tdat/AuthCache.H"
#include "dsm/EDSIFactory.H"
#include "dsm/VDoc.H"

int main(int argc, char** argv) {

CmdLineCacheAuthListener authhandler;
EDSIFactory::addAuthEventListener(&authhandler);

EcceURL source("http://cmcs.ca.sandia.gov:10080/files/public/Demo5/G3MP2-B3LYP-ch2o/");
EDSI *interface = EDSIFactory::getEDSI(source);
MetaDataRequest md;
vector<MetaDataRequest> mds;
MetaDataResult mdr;
vector<MetaDataResult> mdrs;
vector<MetaDataResult> returnVals;
mdr.name = VDoc::getEcceNamespace() + ":testprop";
mdr.value = "xyz";
md.name = mdr.name;
mds.push_back(md);
mdrs.push_back(mdr);
interface->putMetaData(mdrs);
interface->setURL(source);
interface->getMetaData(mds, returnVals);
int valsSize = returnVals.size();
 cerr << "Request key " << mds[0].name << endl;
for (int valsIndex = 0; valsIndex < valsSize; valsIndex++)
{
  cerr << "key " << returnVals[valsIndex].name;
  cerr << " value " << returnVals[valsIndex].value << endl; 
}

return 0;

}

