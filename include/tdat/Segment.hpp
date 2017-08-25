///////////////////////////////////////////////////////////////////////////////
// Synopsis: The Segment class contains most of the information available in
//           the .sgm files used by NWChem and implements methods for
//           determining whether or not a particular residue corresponds to a
//           particular segment.
///////////////////////////////////////////////////////////////////////////////
#ifndef SEGMENT_HH
#define SEGMENT_HH

#include <vector>
#include <string>
  using std::string;
  using std::vector;

class Fragment;
#include <tdat/Residue.H>
class TAtm;

class Segment {
  public:

// Constructors
    Segment();
    Segment(vector<TAtm*> atomlist);
    Segment(const Segment & segment);

// Operators
    Segment& operator=(const Segment& rhs);

// Accessors

    string name() const;
    int natom() const;
    int nbond() const;
    int nangle() const;
    int ntorsion() const;
    int nimproper() const;
    vector<int> atSeqNum() const;
    vector<string> atomName() const;
    vector<string> atomType1() const;
    vector<string> atomType2() const;
    vector<string> atomType3() const;
    vector<char> dynType1() const;
    vector<char> dynType2() const;
    vector<char> dynType3() const;
    vector<short> chrgGrp() const;
    vector<short> plrzGrp() const;
    vector<short> linkNum() const;
    vector<int> envType() const;
    vector<double> charge1() const;
    vector<double> charge2() const;
    vector<double> charge3() const;
    vector<double> plrzblty1() const;
    vector<double> plrzblty2() const;
    vector<double> plrzblty3() const;
    vector<int> bndSeqNum() const;
    vector<int> iIndex() const;
    vector<int> jIndex() const;
    vector<int> bndType() const;

// Modifiers

    void name(const string & segname);
    void natom(const int & nat);
    void nbond(const int & nbnd);
    void nangle(const int & nangl);
    void ntorsion(const int & ntors);
    void nimproper(const int & nimp);
    void atSeqNum(const vector<int> & atsq);
    void atomName(const vector<string> & atnam);
    void atomType1(const vector<string> & atyp);
    void atomType2(const vector<string> & atyp);
    void atomType3(const vector<string> & atyp);
    void dynType1(const vector<char> & dtyp);
    void dynType2(const vector<char> & dtyp);
    void dynType3(const vector<char> & dtyp);
    void chrgGrp(const vector<short> & grp);
    void plrzGrp(const vector<short> & grp);
    void linkNum(const vector<short> & link);
    void envType(const vector<int> & etyp);
    void charge1(const vector<double> & chrg);
    void charge2(const vector<double> & chrg);
    void charge3(const vector<double> & chrg);
    void plrzblty1(const vector<double> & plrz);
    void plrzblty2(const vector<double> & plrz);
    void plrzblty3(const vector<double> & plrz);
    void bndSeqNum(const vector<int> & bsq);
    void iIndex(const vector<int> & idx);
    void jIndex(const vector<int> & idx);
    void bndType(const vector<int> & btyp);

// Methods

    Residue::ResidueStatus compare(Residue *Res,
               Fragment* frag);
    void assignResidueAtoms(Residue *Res, vector<TAtm*>
               atomlist);

  private:

// Deck I
    string p_segName;
    int p_natom;
    int p_nbond;
    int p_nangle;
    int p_ntorsion;
    int p_nimproper;

// Deck II
    vector<int> p_atSeqNum;
    vector<string> p_atomName;
    vector<string> p_atomType1;
    vector<string> p_atomType2;
    vector<string> p_atomType3;
    vector<char> p_dynType1;
    vector<char> p_dynType2;
    vector<char> p_dynType3;
    vector<short> p_chrgGrp;
    vector<short> p_plrzGrp;
    vector<short> p_linkNum;
    vector<int> p_envType;
    vector<double> p_charge1;
    vector<double> p_charge2;
    vector<double> p_charge3;
    vector<double> p_plrzblty1;
    vector<double> p_plrzblty2;
    vector<double> p_plrzblty3;

// Deck III
    vector<int> p_bndSeqNum;
    vector<int> p_iIndex;
    vector<int> p_jIndex;
    vector<int> p_bndType;
   
};
#endif
