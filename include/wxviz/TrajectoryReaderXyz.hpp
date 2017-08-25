#ifndef TRAJECTORYREADERXYZ_HH
#define TRAJECTORYREADERXYZ_HH

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
 * Implementation of TrajectoryReader for .xyz files.
 */
class TrajectoryReaderXyz : public TrajectoryReader
{
  public:
    TrajectoryReaderXyz(SGContainer * sg);
    virtual ~TrajectoryReaderXyz();

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

    int p_lineSkip;
    int p_numSoluteAtoms;

    vector<bool> p_showVector;

    bool p_isHeaderRead;
    ifstream p_stream;
    string p_fileName;
    vector<string> p_stepToFileName;
    vector<int> p_stepToSeekPosition;
};

#endif // TRAJECTORYREADERXYZ_HH
