#  include <iostream.h>
#  include <fstream.h>
#include "util/ETimer.H"
#include "util/EcceURL.H"
#include "dsm/DavEDSI.H"
#include "dsm/EDSIFactory.H"
#include "util/CmdLineAuthListener.H"

// Global data

// 1024 bytes for dummy property data
const char *vtestdata = 
"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";

int vnumDocs = 1000;
int vnumProps = 50;

void usage() {
  cout << "usage: createTestData url \n";
  cout << "  used for benchmarking.  Makes collection with 50 documents ";
  cout << "  each with 5 1k application properties";
  cout << endl;
  exit(-1);
}

EcceURL *makeTestData(DavEDSI& myinterface)
{
  ETimer timer;
  timer.start();
  EcceURL *colURL;
  colURL = myinterface.makeCollection("benchmark");
  if (colURL) {

    char buf[32];
    EcceURL docURL;
    for (int doc=0; doc<vnumDocs; doc++) {
      sprintf(buf,"testdoc%d",doc);
      docURL = colURL->getChild(buf);
      myinterface.setURL(docURL);

      myinterface.putDataSet("test");

      // make some properties to add.  Don't really care what they are.
      vector<MetaDataResult> vmd;
      MetaDataResult md;
      for (int idx=0; idx<vnumProps; idx++) {
        sprintf(buf,"testMD%d",idx);
        md.name = buf;
        md.value = vtestdata;
        vmd.push_back(md);
      }
      bool isSuccessful = myinterface.putMetaData(vmd);
    } 
  } else {
    cout << "Unable to create collection" << endl;
  }
  timer.stop();
  cout << "elapsed time " << timer.elapsedTime() << endl;
  cout << "cpu time " << timer.CPUTime() << endl;
  return colURL;
}

bool getAllMD(EDSI& myinterface)
{
  // Make sure to include file open and close time
  ETimer timer;
  timer.start();
  vector<MetaDataRequest> request;  // leave it empty to get all
  vector<MetaDataResult> result;
  bool isSuccessful = myinterface.getMetaData(request,result);
  timer.stop();
  if (isSuccessful) {
      cout << "Successful " << endl;
  } else {
      cout << "Unsuccessful " << myinterface.m_msgStack.getMessage() << endl;
  }
  cout << "elapsed time " << timer.elapsedTime() << endl;
  cout << "cpu time " << timer.CPUTime() << endl;
  return isSuccessful;
}


bool queryMD(EDSI& myinterface)
{
  // Make sure to include file open and close time
  ETimer timer;
  timer.start();
  vector<MetaDataRequest> request;
  MetaDataRequest md;
  char buf[32];
  int which = 0;
  for (int idx=0; idx<vnumProps; idx++) {
    sprintf(buf,"testMD%d",which);
    md.name = buf;
    which+=5;
  }

  vector<MetaDataResult> result;
  bool isSuccessful = myinterface.getMetaData(request,result);
  timer.stop();
  if (isSuccessful) {
      cout << "Successful " << endl;
  } else {
      cout << "Unsuccessful " << myinterface.m_msgStack.getMessage() << endl;
  }
  cout << "elapsed time " << timer.elapsedTime() << endl;
  cout << "cpu time " << timer.CPUTime() << endl;
  return isSuccessful;
}

bool queryMDDepth1(EDSI& myinterface)
{
  // Make sure to include file open and close time
  ETimer timer;
  timer.start();

  // Set up request.  Request 5 non-consecutive properties.
  vector<MetaDataRequest> request;
  MetaDataRequest md;
  char buf[32];
  int which = 0;
  for (int idx=0; idx<vnumProps; idx++) {
    sprintf(buf,"testMD%d",which);
    md.name = buf;
    which+=5;
  }

  vector<ResourceMetaDataResult> result;
  bool isSuccessful = myinterface.listCollection(request,result);
  timer.stop();
  if (isSuccessful) {
      cout << "Successful " << endl;
  } else {
      cout << "Unsuccessful " << myinterface.m_msgStack.getMessage() << endl;
  }
  cout << "elapsed time " << timer.elapsedTime() << endl;
  cout << "cpu time " << timer.CPUTime() << endl;
  return isSuccessful;
}

bool queryMDLoop(EDSI& myinterface)
{
  // Make sure to include file open and close time
  ETimer timer;
  timer.start();
  vector<MetaDataRequest> request;
  MetaDataRequest md;
  int numProps = 50;
  char buf[32];
  int which = 0;
  for (int idx=0; idx<vnumProps; idx++) {
    sprintf(buf,"testMD%d",which);
    md.name = buf;
    which+=5;
  }

  vector<MetaDataResult> result;
  EcceURL projUrl = myinterface.getURL();
  EcceURL docURL;
  int numDocs = 50;
  bool isSuccessful = true;
  for (int doc=0; doc<vnumDocs && isSuccessful; doc++) {
    result.clear();
    sprintf(buf,"testdoc%d",doc);
    docURL = projUrl.getChild(buf);
    myinterface.setURL(docURL);
    isSuccessful = myinterface.getMetaData(request,result);
    if (isSuccessful) {
      //cout << "Successful " << endl;
    } else {
      cout << "Unsuccessful " << myinterface.m_msgStack.getMessage() << endl;
    }
  }
  timer.stop();
  cout << "elapsed time " << timer.elapsedTime() << endl;
  cout << "cpu time " << timer.CPUTime() << endl;
  return isSuccessful;
}

EcceURL *copy(EDSI& myinterface)
{
  EcceURL *ret = NULL;

  EcceURL target = myinterface.getURL();
  cout << target.toString() << endl;

  ETimer timer;
  timer.start();
  bool isSuccessful = myinterface.copyResource(target,EDSI::SORTOF);
  timer.stop();
  if (isSuccessful) {
      cout << "Successful " << endl;
      ret = new EcceURL(target);
  } else {
      cout << "Unsuccessful " << myinterface.m_msgStack.getMessage() << endl;
  }
  cout << "elapsed time " << timer.elapsedTime() << endl;
  cout << "cpu time " << timer.CPUTime() << endl;
  return ret;
}

bool remove(EDSI& myinterface)
{
  cout << "removing " << myinterface.getURL().toString() << endl;

  ETimer timer;
  timer.start();
  bool isSuccessful = myinterface.removeResource();
  timer.stop();
  if (isSuccessful) {
      cout << "Successful " << endl;
  } else {
      cout << "Unsuccessful " << myinterface.m_msgStack.getMessage() << endl;
  }
  cout << "elapsed time " << timer.elapsedTime() << endl;
  cout << "cpu time " << timer.CPUTime() << endl;
  return isSuccessful;
}

///

int main(int argc, char** argv) {
  if (argc != 2) {
    usage();
  }

  CmdLineAuthListener auth;
//  EDSIFactory::addListener(&auth);

  EcceURL projUrl(argv[1]);
  DavEDSI myinterface(projUrl);
  myinterface.addAuthEventListener(&auth);

  cout << "------ " << endl;
  cout << "STEP 1  making data " << endl;
  cout << "------ " << endl;
  EcceURL *colURL = makeTestData(myinterface);
  if (colURL) {
    cout << "------ " << endl;
    cout << "STEP 2  getting all metadata" << colURL->toString() << endl;
    cout << "------ " << endl;
    myinterface.setURL(colURL->getChild("testdoc0"));
    getAllMD(myinterface);

    cout << "------ " << endl;
    cout << "STEP 3 querying selected metadata" << endl;
    cout << "------ " << endl;
    queryMD(myinterface);

    cout << "------ " << endl;
    cout << "STEP 4 querying selected metadata depth=1" << endl;
    cout << "------ " << endl;
    myinterface.setURL(*colURL);
    queryMDDepth1(myinterface);

    cout << "------ " << endl;
    cout << "STEP 5 query MD loop" << endl;
    cout << "------ " << endl;
    queryMDLoop(myinterface);

    cout << "------ " << endl;
    cout << "STEP 6 copy" << endl;
    myinterface.setURL(*colURL);
    EcceURL *copied = copy(myinterface);
    cout << "------ " << endl;

    if (copied) {
      cout << "------ " << endl;
      cout << "STEP 7 remove" << endl;
      myinterface.setURL(*copied);
      remove(myinterface);
      cout << "------ " << endl;
    }

  } else {
    cout << "Unable to make test data" << endl;
  }


  return 0;
}

