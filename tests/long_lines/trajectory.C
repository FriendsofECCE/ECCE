#include <cmath>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>
#include "inv/SoDB.H"
#include "inv/nodekits/SoNodeKit.H"
#include "inv/SoInteraction.H"
#include "wxviz/SGSelection.H"
#include "inv/ChemKit/ChemInit.H"
#include "viz/NodesInit.H"
#include "viz/AtomNodesInit.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"
#include "wxviz/TrajectoryReaderXyz.H"
#include "wxviz/TrajectoryReaderNWChem.H"

int main()
{
  SoDB::init();
  SoNodeKit::init();
  SoInteraction::init();
  ChemInit::initClasses();
  SGSelection::initClass();
  NodesInit::initClasses();
  AtomNodesInit::initClasses();
  SGFragment::initClass();
  SGContainer::initClass();
  SGContainer* scene = new SGContainer;
  scene->ref();
  for (int length : {20, 400}) {
    const std::string comment(length, 'x');
    {
      std::ofstream xyz("frames.xyz");
      for (int step = 0; step < 3; ++step)
        xyz << "2\n" << comment << "\nH " << step << " 0 0\nH 0 0 1\n";
    }
    scene->getFragment()->clear();
    TrajectoryReaderXyz reader(scene);
    reader.setUnits("Angstrom");
    reader.setGenBonds(false);
    reader.setFileNames({"frames.xyz"});
    if (reader.getStepCount() != 3 || reader.getSoluteAtomsCount() != 2) return 1;
    for (int step : {0, 2, 1}) {
      reader.readStep(step);
      if (std::abs(scene->getFragment()->atomRef(0)->coordinates()[0] - step) > 1e-12)
        return 1;
    }
    {
      std::ofstream trj("frames.trj");
      trj << comment << "\n" << std::setw(10) << 0 << std::setw(10) << 1
          << std::setw(10) << 0 << std::setw(10) << 0 << std::setw(10) << 0
          << "\nMOL   H   " << std::setw(6) << 1 << "\n";
      for (int step = 0; step < 3; ++step)
        trj << "frame\n" << comment << "\nline2\nline3\nline4\n"
            << "FFFFTFFF         0\n" << std::fixed << std::setprecision(3)
            << std::setw(8) << step * 0.1 << std::setw(8) << 0.0
            << std::setw(8) << 0.0 << "\n";
    }
    scene->getFragment()->clear();
    TrajectoryReaderNWChem nw(scene);
    nw.setFileNames({"frames.trj"});
    if (nw.getStepCount() != 3 || nw.getSoluteAtomsCount() != 1) return 1;
    for (int step : {0, 2, 1}) {
      nw.readStep(step);
      if (std::abs(scene->getFragment()->atomRef(0)->coordinates()[0] - step) > 1e-12)
        return 1;
    }
  }
  scene->unref();
  std::cout << "PASS XYZ and NWChem frames with long comments\n";
}
