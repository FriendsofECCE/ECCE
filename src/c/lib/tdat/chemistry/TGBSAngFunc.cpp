//////////////////////////////////////////////////////////////////////////////
//
// FILENAME: TGBSAngFunc.C
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
#include <math.h>

#include "util/ErrMsg.H"
#include "tdat/TGBSAngFunc.H"


// Constructor
TGBSAngFunc::TGBSAngFunc(void) {
  p_code = "";
  p_basisType = TGBSAngFunc::Cartesian;
}

// Destructor
TGBSAngFunc::~TGBSAngFunc(void) {
  // This function will require more thought to make sure that memory is
  // properly cleaned up.
}

// Accessors
string TGBSAngFunc::code(void) {
  return p_code;
}

TGBSAngFunc::BasisType TGBSAngFunc::basisType(void) {
  return p_basisType;
}

int TGBSAngFunc::maxShells(void) {
  return p_AngMomShellPtrs.size();
}

int TGBSAngFunc::numFuncs(int shell) {
  EE_ASSERT(shell<p_AngMomShellPtrs.size(), EE_FATAL,
      "Shell is not available");
  if (shell == 0) {
    return 1;
  } else {
    return (p_AngMomShellPtrs[shell]-p_AngMomShellPtrs[shell-1]);
  }
}

AngMomFunc TGBSAngFunc::getFunc(int shell, int index) {
  int firstm, lastm;
  // Find first monomial in shell
  if (shell > 0) {
    firstm = p_AngMomShellPtrs[shell-1] + 1;
  } else  {
    firstm = 0;
  }
  lastm = p_AngMomFuncPtrs[firstm+index];
  firstm = firstm+index;
  if (firstm > 0) {
    firstm = p_AngMomFuncPtrs[firstm-1] + 1;
  }
  vector<Monomial> ret;
  for (int i=firstm; i<=lastm; i++) {
    ret.push_back(p_monomial[i]);
  }
  return ret;
}

// Modifiers
void TGBSAngFunc::code(string codeName) {
  p_code = codeName;
  return;
}

void TGBSAngFunc::basisType(TGBSAngFunc::BasisType type) {
  p_basisType = type;
  return;
}

void TGBSAngFunc::addFunc(AngMomFunc func) {
  int i;
  for (i=0; i<func.size(); i++) {
    p_monomial.push_back(func[i]);
  }
  p_AngMomFuncPtrs.push_back(p_monomial.size()-1);
  return;
}

void TGBSAngFunc::markShell() {
  p_AngMomShellPtrs.push_back(p_AngMomFuncPtrs.size()-1);
}

// Methods

////////////////////////////////////////////////////////////////////////////////
// Description:
//   Parse a text string and convert it into a set of coefficients and
//   exponents describing an angular momentum function.
////////////////////////////////////////////////////////////////////////////////
AngMomFunc TGBSAngFunc::parseString(string input) {
  int length = input.size();
  char coord;
  int i,ex,numerator,denominator,sign;
  int l,n,m,k;
  Monomial component;
  double coef;
  AngMomFunc ret;
  string nstring, dstring, xstring;
  bool numflag, denflag, beginflag;

  numerator = 1;
  denominator = 1;
  beginflag = true;
  sign = 1;
  numflag = true;
  denflag = false;
  coord = ' ';
  l = 0;
  m = 0;
  n = 0;
  k = 0;
  for (i=0; i<length; i++) {
    // If character is a digit, append it to appropriate string.
    if (isdigit(input[i]) != 0) {
      beginflag = false;
      if (numflag) nstring.append(1,input[i]);
      else if (denflag) dstring.append(1,input[i]);
      else xstring.append(1,input[i]);
    // If character is a coordinate then either increment the
    // corresponding exponent if it is not followed by a numeric
    // character or add the character to xstring.
    } else if (isalpha(input[i]) != 0) {
      beginflag = false;
      numflag = false;
      // increment exponent
      if (xstring.size() > 0) {
        ex = atoi(xstring.c_str())-1;
        if (coord == 'x') l += ex;
        else if (coord == 'y') m += ex;
        else if (coord == 'z') n += ex;
        else if (coord == 'r') k += ex;
        xstring = "";
      }
      coord = input[i];
      if (coord == 'x') l++;
      else if (coord == 'y') m++;
      else if (coord == 'z') n++;
      else if (coord == 'r') k++;
    } else if (input[i] == '/') {
      numflag = false;
      denflag = true;
    }
    if (input[i] == '+' || input[i] == '-' || i == length-1) {
    // Character is a plus or minus sign or it is at the end of the
    // string. Add monomial to ret and initialize parameters for new
    // monomial.
      // Check to see if this is not the start of the string
      if (!beginflag) {
        numflag = true;
        if (nstring.size()>0) numerator = atoi(nstring.c_str());
        if (dstring.size()>0) denominator = atoi(dstring.c_str());
        if (xstring.size()>0) ex = atoi(xstring.c_str())-1;
        coef = (double)sign * (double)numerator / (double)denominator;
        if (xstring.size() > 0) {
          if (coord == 'x') l += ex;
          else if (coord == 'y') m += ex;
          else if (coord == 'z') n += ex;
          else if (coord == 'r') k += ex;
        }
        component.m_coefficient = coef;
        component.m_l = l;
        component.m_m = m;
        component.m_n = n;
        component.m_k = k;
        ret.push_back(component);
        // if there are more non-blank characters in string, reset
        // parameters
        if (i < length - 1) {
          numerator = 1;
          denominator = 1;
          numflag = true;
          denflag = false;
          coord = ' ';
          l = 0;
          m = 0;
          n = 0;
          k = 0;
          nstring = "";
          dstring = "";
          xstring = "";
        }
      }
      if (input[i] == '+') {
        sign = 1;
      } else if (input[i] == '-') {
        sign = -1;
      }
    // Division symbol encountered. Start evaluating denominator.
    }
  }
  return ret;
}

double TGBSAngFunc::evaluateAMFunc(int shell, int index, double x,
                                   double y, double z)
{
  int firstm, lastm;
  // Find first monomial in shell
  if (shell > 0) {
    firstm = p_AngMomShellPtrs[shell-1] + 1;
  } else  {
    firstm = 0;
  }
  lastm = p_AngMomFuncPtrs[firstm+index];
  firstm = firstm+index;
  if (firstm > 0) {
    firstm = p_AngMomFuncPtrs[firstm-1] + 1;
  }
  int mdx, k;
  double r, ret;
  Monomial mfunc;
  ret = 0.0;
  for (mdx=firstm; mdx<=lastm; mdx++) {
    mfunc = p_monomial[mdx];
    if (mfunc.m_k > 0) {
      k = mfunc.m_k/2;
      r = x*x + y*y + z*z;
      ret += mfunc.m_coefficient * pow(x,mfunc.m_l)
        * pow(y,mfunc.m_m) * pow(z,mfunc.m_n) * pow(r, k);
    } else {
      ret += mfunc.m_coefficient * pow(x,mfunc.m_l)
        * pow(y,mfunc.m_m) * pow(z,mfunc.m_n);
    }
  }
  return ret;
}

void TGBSAngFunc::getExponents(int shell, int index, int &l, int &m,
    int &n)
{
  int firstm;
  // Find first monomial in shell
  if (shell > 0) {
    firstm = p_AngMomShellPtrs[shell-1] + 1;
  } else  {
    firstm = 0;
  }
  firstm = firstm+index;
  if (firstm > 0) {
    firstm = p_AngMomFuncPtrs[firstm-1] + 1;
  }
  Monomial AMFunc = p_monomial[firstm];
  l = AMFunc.m_l;
  m = AMFunc.m_m;
  n = AMFunc.m_n;
}
void TGBSAngFunc::getMaxExponents(int shell, int index, int &l, int &m,
    int &n)
{
  int firstm, lastm;
  // Find first monomial in shell
  if (shell > 0) {
    firstm = p_AngMomShellPtrs[shell-1] + 1;
  } else  {
    firstm = 0;
  }
  lastm = p_AngMomFuncPtrs[firstm+index];
  firstm = firstm+index;
  if (firstm > 0) {
    firstm = p_AngMomFuncPtrs[firstm-1] + 1;
  }
  Monomial AMFunc;
  l=0; m=0; n=0;
  for (int i=firstm; i<=lastm; i++) {
    AMFunc = p_monomial[i];
    if (AMFunc.m_l > l) l=AMFunc.m_l;
    if (AMFunc.m_m > m) m=AMFunc.m_m;
    if (AMFunc.m_n > n) n=AMFunc.m_n;
  }
  return;
}
