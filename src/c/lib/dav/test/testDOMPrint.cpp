// include files ***********************************************************
#  include <iostream.h>
#  include <fstream.h>
#include "util/EcceURL.H"
#include "dsm/BasicDOMWriter.H"
#include "dsm/BasicDOMParser.H"

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>


int main(int argc, char** argv) {
  if (argc != 2) {
    cout << "usage testDOMPrint <file>" << endl;
    exit(-1);
  }

  bool ret = true;
  try
  {
    XMLPlatformUtils::Initialize();
  }
  catch (const XMLException& toCatch)
  {
    cerr << "Error during parser initialization! :\n"
      << XMLString::transcode(toCatch.getMessage()) << endl;
    ret = false;
  }

  ifstream ifs(argv[1]);
  char buf[513];
  string data;
  while (ifs.getline(buf,512)) {
    data.append(buf);
    data.append("\n");
  }
  //cout << "data " << data << endl;
  // Doing this the stupid way.
  BasicDOMParser parser;


  cout << "Using MemBufInputSource version" << endl;
  MemBufInputSource* memBufIS = new MemBufInputSource (
      ( const XMLByte*)data.data(),
      data.length(),
      "whatever",
      false);
  DOMDocument *doc = parser.parse(*memBufIS);

  BasicDOMWriter::write(*doc,cout,false);

  return 0;
}

