#ifndef TRAJECTORYREADERNWCHEM_HH
#define TRAJECTORYREADERNWCHEM_HH

#include <fstream>
  using std::ifstream;
#include <string>
  using std::string;
#include <vector>
  using std::vector;

#include "util/IOException.H"

#include "wxviz/TrajectoryReader.H"


class SGContainer;


/**
 * Implementation of TrajectoryReader for NWChem .trj files.
 */
class TrajectoryReaderNWChem : public TrajectoryReader
{
  public:
    TrajectoryReaderNWChem(SGContainer * sg);
    virtual ~TrajectoryReaderNWChem();

    virtual int getSoluteAtomsCount();
    virtual long getStepCount();
    virtual string getCurrentFileName();

    virtual bool readStep(const long& step,
            const bool& readVelocities=false,
            const bool& readForces=false)
            throw (IOException);

    virtual void setFileNames(const vector<string>& fileNames,
                              const bool& useSolvent=false);
    virtual void processVectors(const vector<bool>& atoms);

  protected:
    void readHeader() throw (IOException);
    void indexFiles();
    void changeStreamForStep(const long& step) throw (IOException);

    // Molecule configuration data. This needs to be stored during
    // the trajectory to manage parsing and display of the trajectory
    // file.
    bool         p_useSolvent;  // KLS move to controller?
    int          p_soluteAtoms;
    int          p_soluteBonds;
    int          p_solventAtoms;
    int          p_solventBonds;
    int          p_maxSolvMol;
    vector<bool> p_useAtom;
    int          p_numUsedAtoms;

    vector<bool> p_showVector;

    bool p_isHeaderRead;
    ifstream p_stream;
    string p_fileName;
    vector<string> p_stepToFileName;
    vector<int> p_stepToSeekPosition;
};

#endif // TRAJECTORYREADERNWCHEM_HH
