#include <stdlib.h>
#include <iostream>
using namespace std;
#include "util/EcceURL.H"
#include "tdat/CmdLineCacheAuthListener.H"
#include "tdat/AuthCache.H"
#include "dsm/EDSIFactory.H"
#include "dsm/EDSIExporter.H"


int main(int argc, char** argv) {

   CmdLineCacheAuthListener authhandler;
   EDSIFactory::addAuthEventListener(&authhandler);

   EcceURL testURL;
   EcceURL source("http://fringe.emsl.pnl.gov:8080/Ecce/users/ecce1/Calculation");
   // EcceURL source("http://fringe.emsl.pnl.gov:8080/Ecce/users/d3a303/Calculation");
   //EcceURL source("http://fringe.emsl.pnl.gov:8080/Ecce/users/d3j766/Calculation2");
   //EcceURL source("http://cmcs.ca.sandia.gov:18081/slide/files/projects/demo_5/ActiveTablesSpecies");
   //EcceURL source("http://cmcs.ca.sandia.gov:18081/slide/files/projects/demo_5");
   //EcceURL source("http://cmcs.ca.sandia.gov:18081/slide/files/projects/dsmgmt");
   //EcceURL source("http://wv07611.pnl.gov/target/Calculation2_2");
   //EcceURL target("http://wv07611.pnl.gov/target");
   EcceURL target("http://cmcs.ca.sandia.gov:18081/slide/files/projects/demo_5");

   EDSIExporter myExport;
   myExport.setSourceURL(source);
   myExport.setTargetURL(target);

   try {
      cout << "BEFORE EXPORT" << endl;
      testURL = myExport.xport();
      cout << "Export Succeeded" << endl;
      cout << "Export returned: " << testURL.toString() << endl;
   }
   catch (EcceException& exportError) {
      cout << "Export failed - " << "Error: " << exportError.what() << endl;
   }
   return 0;

}

