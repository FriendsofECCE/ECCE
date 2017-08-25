#include <stdlib.h>
#include "util/EcceURL.H"
#include "tdat/CmdLineCacheAuthListener.H"
#include "tdat/AuthCache.H"
#include "dsm/EDSIFactory.H"
#include "dsm/G3MP2B3LYP.H"

int main(int argc, char** argv) {

CmdLineCacheAuthListener authhandler;
EDSIFactory::addAuthEventListener(&authhandler);

// EcceURL source("http://fringe.emsl.pnl.gov:8080/Ecce/share/SC02/G3MP2-B3LYP_wdata/");
EcceURL source("http://fringe.emsl.pnl.gov:8080/Ecce/share/SC02/G3MP2-B3LYP-ch2o/");
G3MP2B3LYP g3(source);
double E = g3.getEnergy();
double Hf = g3.getDeltaHf();
return 0;

}

