#ifndef TRAJECTORYREADER_H_
#define TRAJECTORYREADER_H_

#include <string>
  using std::string;
#include <vector>
  using std::vector;

#include "util/IOException.H"


class SGContainer;


class TrajectoryReader
{
  public:
    TrajectoryReader(SGContainer * sg)
      : p_sg(sg),
        p_fileNames(),
        p_units("Nanometers"),
        p_vScale(0.5),
        p_fScale(0.001),
        p_hasSolvent(false),
        p_hasVelocities(false),
        p_hasForces(false),
        p_genBonds(true)
    {
    }

    virtual ~TrajectoryReader()
    {
    }

    // getters
    virtual int getSoluteAtomsCount() = 0;
    virtual long getStepCount() = 0;
    virtual string getCurrentFileName() = 0;
    virtual bool hasSolvent() { return p_hasSolvent; }
    virtual bool hasVelocities() { return p_hasVelocities; }
    virtual bool hasForces() { return p_hasForces; }
    virtual bool hasGenBonds() { return p_genBonds; }

    virtual bool readStep(const long& step,
            const bool& readVelocities=false,
            const bool& readForces=false) 
            throw (IOException) = 0;

    // setters
    virtual void setFileNames(const vector<string>& fileNames,
                              const bool& useSolvent=false)
            { p_fileNames = fileNames; }
    virtual void setUnits(const string& units) { p_units = units; }
    virtual void setVelocityScale(const double& scale) { p_vScale = scale; }
    virtual void setForceScale(const double& scale) { p_fScale = scale; }
    virtual void setGenBonds(const bool& genBonds) { p_genBonds = genBonds; }

    virtual void processVectors(const vector<bool>& atoms) = 0;

  protected:
    SGContainer * p_sg;
    vector<string> p_fileNames;
    string p_units;
    double p_vScale;
    double p_fScale;
    bool p_hasSolvent;
    bool p_hasVelocities;
    bool p_hasForces;
    bool p_genBonds;
};

#endif // TRAJECTORYREADER_H_
