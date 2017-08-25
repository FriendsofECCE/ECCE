//////////////////////////////////////////////////////////////////////////////
//
// HEADER FILENAME: TGBSAngFunc.H
//
//
// CLASS SYNOPSIS:
//    Contains a complete description of the angular momentum functions
//    associated with a particular code and basis type (cartesian or
//    spherical).
//      
// DESCRIPTION:
//    Used for displaying molecular orbitals and related quantities.
//
//////////////////////////////////////////////////////////////////////////////
#ifndef TGBSANGFUNC_HH
#define TGBSANGFUNC_HH

#include <string>
#include <vector>

  using std::string;
  using std::vector;

// Simple class containing data representing a single monomial
// in the angular part of a Gaussian basis set function.
class Monomial {
public:
  Monomial(void) {};
  ~Monomial(void) {};
  double m_coefficient;
  int m_l;     //X-exponent
  int m_m;     //Y-exponent
  int m_n;     //Z-exponent
  int m_k;     //R-exponent
};

typedef vector<Monomial> AngMomFunc;

class TGBSAngFunc {

public:

  enum BasisType {Cartesian, Spherical};

  // Constructor
  TGBSAngFunc(void);

  // Destructor
  ~TGBSAngFunc(void);

  // Accessors
  string code(void);
  TGBSAngFunc::BasisType basisType(void);
  int maxShells();
  int numFuncs(int shell);
  AngMomFunc getFunc(int shell, int index);

  // Modifiers
  void code(string codeName);
  void basisType(BasisType type);
  void addFunc(AngMomFunc func);
  void markShell();

  // Methods
  AngMomFunc parseString(string input);
  double evaluateAMFunc(int shell, int index, double x, double y,
                        double z);
  void getExponents(int shell, int index, int &l_exp, int &m_exp, int &n_exp);
  void getMaxExponents(int shell, int index, int &l_exp, int &m_exp, int &n_exp);

private:

  string p_code;
  TGBSAngFunc::BasisType p_basisType;
  
  vector<Monomial> p_monomial;
  vector<int>      p_AngMomFuncPtrs;
  vector<int>      p_AngMomShellPtrs;

};
#endif // TGBSANGFUNC_HH
