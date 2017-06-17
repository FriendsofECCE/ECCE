#include <errno.h>

#include <fstream>
  using std::ifstream;
#include <string>
  using std::string;
#include <vector>
  using std::vector;

#include <iostream>
  using std::cout;
  using std::endl;

#define MAX_LINE 256

#define VELOCITY 1
#if VELOCITY
//#include "inv/nodes/SoSwitch.H"
#endif

#include "tdat/Residue.H"
#include "tdat/TAtm.H"
#include "tdat/TBond.H"

#include "util/StringConverter.H"
#include "util/StringTokenizer.H"

#include "viz/SGContainer.H"
#include "viz/SGFragment.H"
#include "viz/VRVector.H"

#include "wxviz/TrajectoryReaderXyz.H"



/**
 * Constructor.
 * The goal of this class was to separate reading from user interaction
 * but this proved too much work for now at least.
 */
TrajectoryReaderXyz::TrajectoryReaderXyz(SGContainer *sg)
  : TrajectoryReader(sg),
    p_lineSkip(0),
    p_showVector(),
    p_isHeaderRead(false),
    p_stream(),
    p_fileName(""),
    p_stepToFileName(),
    p_stepToSeekPosition()
{
}


/**
 * Destructor.
 */
TrajectoryReaderXyz::~TrajectoryReaderXyz()
{
}


int TrajectoryReaderXyz::getSoluteAtomsCount()
{
  return p_numSoluteAtoms;
}


long TrajectoryReaderXyz::getStepCount()
{
  return p_stepToFileName.size();
}


string TrajectoryReaderXyz::getCurrentFileName()
{
  return p_fileName;
}


bool TrajectoryReaderXyz::readStep(const long& step,
        const bool& readVelocities, const bool& readForces)
        throw (IOException)
{
  if (step >= getStepCount()) {
    // don't bother if the requested step is outside valid range
    return false;
  }

  changeStreamForStep(step);

  SGFragment * frag = p_sg->getFragment();

  bool ret = false;

  char buf[MAX_LINE+1];

  double mult;
  if (p_units == "Bohr") {
    // This is the value NIST is reporting in 2002
    mult = 0.5291772083;
  } else if (p_units == "Picometers") {
    mult = 0.01;
  } else if (p_units == "Nanometers") {
    mult = 10.0;
  } else {
    mult = 1.0;
  }

  // get total number of atoms

  int ipos = p_stream.tellg();
  int natoms;
  if (!p_stream.getline(buf,MAX_LINE)) { 
    throw IOException("Unable to read line containing numatoms", WHERE);
  }
  // find first line that corresponds to a single integer (start of frame)
  // the logic in the next line is flawed because it won't support the
  // element being specified by a number instead of a symbol GDB 9/30/09
  //while(!StringConverter::toInt(buf,natoms)) {
  char *endp;
  natoms = (int)strtol(buf, &endp, 10);
  while (buf==endp || errno==ERANGE || *endp!='\0') { 
    ipos = p_stream.tellg();
    if (!p_stream.getline(buf,MAX_LINE)) {
      throw IOException("Unable to find first single integer line", WHERE);
    }
    natoms = (int)strtol(buf, &endp, 10);
  }
  //cout << "first line " << buf << endl;

  if (natoms < 0 ) {
    throw IOException("number of atoms was < 0",WHERE);
  }

  if (natoms == frag->numAtoms()) {
  // Skip title line

    p_stream.getline(buf,MAX_LINE);
#if VELOCITY
    p_sg->getTopAtomVectors()->removeAllChildren();
    if (p_showVector.size() == 0) {
      for (int i=0; i<natoms; i++) {
        p_showVector.push_back(true);
      }
    }
#endif

  // Read in atom coordinates (skip atom names, these have already been parsed).

    char* coordinate;
    double coords[3];
    for (int i=0; i<natoms; i++) {
      p_stream.getline(buf,MAX_LINE);
      (void)strtok(buf," ");
      coordinate = strtok(0," \t");
      if (coordinate == NULL) return ret;
      coords[0] = mult*atof(coordinate);
      coordinate = strtok(0," \t");
      if (coordinate == NULL) return ret;
      coords[1] = mult*atof(coordinate);
      coordinate = strtok(0," \t");
      if (coordinate == NULL) return ret;
      coords[2] = mult*atof(coordinate);
      TAtm* atm = frag->atomRef(i);
      atm->coordinates(coords);
#if VELOCITY
      bool check = true;
      double vel[3];
      double frc[3];
      // Get velocities, if they exist
      if (readVelocities) {
        coordinate = strtok(0," \t");
        if (coordinate == NULL) check = false;
        else vel[0] = p_vScale*atof(coordinate);
        if (check) {
          coordinate = strtok(0," \t");
          if (coordinate == NULL) check = false;
          else vel[1] = p_vScale*atof(coordinate);
        }
        if (check) {
          coordinate = strtok(0," \t");
          if (coordinate == NULL) check = false;
          else vel[2] = p_vScale*atof(coordinate);
        }
        if (check && p_showVector[i]) {
          VRVector *vec = new VRVector;
          vec->setName("Velocity");
          vec->position(coords[0],coords[1],coords[2]);
          vec->direction(vel[0],vel[1],vel[2]);
          vec->setColor(0.0,0.0,1.0);
          p_sg->getTopAtomVectors()->addChild(vec);
        }
      }
      // Read force vector, if it exists
      if (readForces) {
        if (check) {
          coordinate = strtok(0," \t");
          if (coordinate == NULL) check = false;
          else vel[0] = p_fScale*atof(coordinate);
        }
        if (check) {
          coordinate = strtok(0," \t");
          if (coordinate == NULL) check = false;
          else vel[1] = p_fScale*atof(coordinate);
        }
        if (check) {
          coordinate = strtok(0," \t");
          if (coordinate == NULL) check = false;
          else vel[2] = p_fScale*atof(coordinate);
        }
        if (check && p_showVector[i]) {
          VRVector *vec = new VRVector;
          vec->setName("Force");
          vec->position(coords[0],coords[1],coords[2]);
          vec->direction(frc[0],frc[1],frc[2]);
          vec->setColor(1.0,1.0,0.0);
          p_sg->getTopAtomVectors()->addChild(vec);
        }
      }
#endif
    }
  } else {
    frag->clear();
    // reset istream pointer to line that was just read
    p_stream.seekg(ipos);

    if (!frag->restoreXYZ(p_stream, mult, p_genBonds)) {
      throw IOException("Unable to restore XYZ", WHERE);
    } 
    for(int i = 0; i < frag->numAtoms(); i++) {
      TAtm* atm = frag->atomRef(i);
      atm->displayStyle(DisplayStyle(DisplayStyle::CPK));
    }
    ret = true;
  }
  return ret;
}


void TrajectoryReaderXyz::setFileNames(const vector<string>& fileNames,
                                       const bool& useSolvent)
{
  TrajectoryReader::setFileNames(fileNames);
  p_isHeaderRead = false;
  indexFiles();
  readHeader();
}


/**
 * Not supported - does nothing.
 */
void TrajectoryReaderXyz::processVectors(const vector<bool>& atoms)
{
  p_showVector = atoms;
}


void TrajectoryReaderXyz::readHeader() throw (IOException)
{
  if (p_isHeaderRead) {
    return;
  }

  changeStreamForStep(0);

  p_isHeaderRead = true;

  SGFragment * frag = p_sg->getFragment();

  double mult;
  if (p_units == "Bohr") {
    // This is the value NIST is reporting in 2002
    mult = 0.5291772083;
  } else if (p_units == "Picometers") {
    mult = 0.01;
  } else if (p_units == "Nanometers") {
    mult = 10.0;
  } else {
    mult = 1.0;
  }

  if (!frag->restoreXYZ(p_stream,mult,p_genBonds)) {
    throw IOException("Unable to read XYZ header", __FILE__, __LINE__);
  }

  // Assume every atom is a solute atom and that the number of atoms
  // won't drastically change between steps, although it certainly could.
  // This value is used to determine the display style to use in conjunction
  // with the wireframe limit
  p_numSoluteAtoms = frag->numAtoms();

  // Rewind since there really isn't a header and we just read the first step
  p_stream.seekg(0, std::ios::beg);
  p_stream.clear();
  p_lineSkip = frag->numAtoms() + 2;
}


void TrajectoryReaderXyz::indexFiles()
{
  // clear old indices
  p_stepToFileName.clear();
  p_stepToSeekPosition.clear();
  p_hasVelocities = false;
  p_hasForces = false;

  int natoms;
  int pos;
  char *endp;
  char buf[256];
  ifstream stream;
  vector<string>::iterator fileName;
  vector<string> tokens;
  for (fileName = p_fileNames.begin();
       fileName != p_fileNames.end();
       ++fileName) {
    stream.open(fileName->c_str());

    while (stream.good()) {
      pos = stream.tellg();
      stream.getline(buf, 256);
      // find first line that corresponds to a single integer (start of frame)
      // the logic in the next line is flawed because it won't support the
      // element being specified by a number instead of a symbol GDB 9/30/09
      //if (StringConverter::toInt(line.c_str(),natoms)) {
      natoms = (int)strtol(buf, &endp, 10);
      if (buf!=endp && errno!=ERANGE && *endp=='\0') { 
        p_stepToFileName.push_back(*fileName);
        p_stepToSeekPosition.push_back(pos);

        stream.getline(buf, 256);
        if (stream.good()) { 
          tokens = StringTokenizer(buf).tokenize(" \t");
          p_hasVelocities |= tokens.size() == 7;
          p_hasForces |= tokens.size() == 10;

          // skip passed natoms lines to prepare for next frame
          for (pos=1; pos<natoms && stream.good(); pos++)
            // ignore seems a bit faster than getline
            //stream.getline(buf, 256);
            stream.ignore(256, '\n');
        }
      }
    }
    stream.close();
    stream.clear();
  }
}


void TrajectoryReaderXyz::changeStreamForStep(const long& step)
        throw (IOException)
{
  if (p_fileName != p_stepToFileName[step]) {
    p_fileName = p_stepToFileName[step];
    if (p_stream.is_open()) {
      p_stream.close();
    }
    p_stream.open(p_fileName.c_str());
  }
  if (!p_stream.good()) {
    throw IOException("could not open file", WHERE);
  }
  p_stream.seekg(p_stepToSeekPosition[step]);
  if (!p_stream.good()) {
    throw IOException("could not seek file", WHERE);
  }
}

