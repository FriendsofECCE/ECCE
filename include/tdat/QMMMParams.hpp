/**
 * @file
 *
 *
 */
#ifndef QMMMPARAMS_HH
#define QMMMPARAMS_HH

#include <vector>

using std::vector;

#include "tdat/TAtm.H"

/**
  * Class for defining meta-dynamics potentials.
  */
class QMMMParams
{
  public:

    QMMMParams(string type, int natoms,
	       vector<string> *atypes, int npar, vector<double> *vals );
    QMMMParams(string type, int natoms,
	       vector<int> *atypes, QMMMParams *qmmm);
    QMMMParams(string type, int natoms,
	       vector<int> *aidx, int npar, vector<double> *vals);
    QMMMParams(const QMMMParams& rhs);
    virtual ~QMMMParams();

    virtual bool operator==(const QMMMParams& rhs);
    QMMMParams& operator=(const QMMMParams& rhs);

    string getQMMMType() const;
    int getNumAtoms() const;
    vector<string> *getQMMMPotentials() const;
    vector<int> *getAtomIndices() const;

    int getNumParams() const;
    vector<double> *getParams();
    void setParamValue(double val, int idx);
    QMMMParams *getQMMMParams();
    bool isZeroParams();

  protected:
    /**
     * Type of QM/MM term
     */
    string p_type;

    /**
     * Number of atoms associated with QM/MM term
     */
    int p_natoms;

    /**
     * The atom types associated with QM/MM term
     */
    vector<string> p_atypes;

    /**
     * The indices associated with QM/MM term
     */
    vector<int> p_indices;

    /**
     * Number of parameters associated with QM/MM term
     */
    int p_nparams;

    /** 
     * Parameters associated with QM/MM term
     */
    vector<double> p_params;

    /**
     *  Pointer to another QM/MM term containing parameters for this term
     */
    QMMMParams *p_qmmm;
};
#endif 
