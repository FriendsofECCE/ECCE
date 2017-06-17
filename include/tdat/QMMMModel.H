/**
 * @file 
 *
 *
 */
#ifndef QMMMMODEL_HH
#define QMMMMODEL_HH

#include "util/Serializable.H"
#include "util/InvalidException.H"

#include "tdat/QMMMParams.H"

#include <vector>

class Fragment;
class QMMMParams;

/**
 * Class for building up a set of meta-dynamics potentials on a fragment.
 * Meta-dynamics potentials consist of a list of atoms that define the potential
 * and a list of parameters associated with the potential. See the QMMMParams
 * class for more information.
 */
class QMMMModel : public Serializable
{

  public:

    QMMMModel(const Fragment& frag);
    QMMMModel(const QMMMModel& rhs); 
    virtual ~QMMMModel();

    QMMMModel& operator=(const QMMMModel& rhs);

    int size() const;
    void clear();

    int setFragment(const Fragment& frag);
    int cleanUnmatchedTerms();
    const Fragment& getFragment() const;

    vector<QMMMParams*> *getQMMMTerms(string type) const;

    void addQMMMTerm(QMMMParams *term) throw(InvalidException);
    bool remove(const QMMMParams *term);
    int numQMMMTerms() const;
    QMMMParams* getQMMMTerm(int idx) const;
    void purgeParams(string type);

    // Do we need this one?
    //   bool removeLike(const QMMMParams& mpot);

  protected:

    vector<QMMMParams*>::iterator find(const QMMMParams *searchFor);
    //  void categorize(int& nFixedAtoms, int& nConstraints, 
    //                  int& nRestraints) const;
    //  vector<QMMMParams*> *getOfType(int numAtoms) const;
    void remove(int index);

  private:

    /**  The set of meta-dynamics potentials.  */
    vector<QMMMParams*> p_terms;

    /** * The system the user is operating on. */
    const Fragment *p_frag;
};


#endif 
