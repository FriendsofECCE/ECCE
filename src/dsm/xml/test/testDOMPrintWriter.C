#include <iostream>
#include <fstream>
#include <strstream>
using namespace std;
#include <unistd.h>


#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMNode.hpp>

using namespace xercesc;

#include "util/ETimer.H"
#include "util/StringConverter.H"
#include "dsm/BasicDOMWriter.H"
#include "dsm/BasicDOMParser.H"



int main(int argc, char** argv)
{
  ETimer timer;

  try {
    XMLPlatformUtils::Initialize();
  } catch (const XMLException& toCatch) {
    cerr << "Error during initialization! :\n" << (toCatch.getMessage()) << endl;
    exit(1);
  }
      
  string file = "/tmp/test1.xml";
  ifstream is(file.c_str());
  string text;
  StringConverter::streamToText(is,text);

  cout << "parting " << text << endl;
  BasicDOMParser parser;
  DOMDocument *doc = parser.parse(text);

  if (doc == 0) {
    cout << "document is null " << endl;
  } else if (!doc->hasChildNodes()) {
    cout << "document is empty" << endl;
  }

  timer.start();
  BasicDOMWriter::write(*doc,cout, false, true);
  /*
  ostrstream os;
  BasicDOMWriter::write(*doc,os, false, true);
  os << ends;
  */
  timer.stop();
  cout << "Time to print " << timer.elapsedTime() << endl;

  return 0;
}
