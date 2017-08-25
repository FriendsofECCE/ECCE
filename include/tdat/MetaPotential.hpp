/**
 * @file
 *
 *
 */
#ifndef METAPOTENTIAL_HH
#define METAPOTENTIAL_HH

#include <vector>

using std::vector;

#include "tdat/TAtm.H"

/**
  * Class for defining meta-dynamics potentials.
  */
class MetaPotential
{
  public:

    MetaPotential(int natoms, vector<int> *ids, int npar, vector<double> *vals );
    MetaPotential(const MetaPotential& rhs);
    virtual ~MetaPotential();

    virtual bool operator==(const MetaPotential& rhs);
    MetaPotential& operator=(const MetaPotential& rhs);

    int getNumAtoms() const;
    int getNumParams() const;
    vector<int> *getAtoms() const;

    void   setPotentialValue(const double& value, int idx);
    vector<double> *getParams();

  protected:

    /**
     * Number of atoms associated with meta-dynamics potential
     */
    int p_natoms;

    /**
     * The atom IDs associated with the meta-dynamics potential
     */
    vector<int> p_IDs;

    /**
     * Number of paramters associated with meta-dynamics potential
     */
    int p_nparams;

    /** 
     * Parameters associated with meta-dynamics potential
     */
    vector<double> p_params;


};
#endif 
