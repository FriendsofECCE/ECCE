///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: TBond.H
//
//
// CLASS SYNOPSIS:
//    Bond Objects Represent a Connection Between Two Atoms
//
// SEE ALSO:
//    Class TAtm
//      
// DESCRIPTION:
//    Bonds track the connection between pairs of atoms.  The pair of atoms
//    is tracked and the "strength" of bond is also tracked.  Bond order
//    is captured via a double value with values less than 1
//    representing non-covalent bonds.  Order 1, 1.5, 2, and 3 stand for
//    Single, Aromatic, Double, and Triple.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef TBOND_HH
#define TBOND_HH

#include <iostream>
  using std::ostream;

#include <string>
  using std::string;

#include "tdat/DisplayStyle.H"

class TAtm;

// application includes


class TBond 
{
public:

    static const double Single;
    static const double Aromatic;
    static const double Double;
    static const double Triple;
    static const double Hydrogen;
    static const double Metal;
    static double       strToOrder(const string& str);
    static const char*  orderToString(double order);
    static string orderToSmallImage(double order);
    static string orderToImage(double order);

  // Constructors
    TBond(const TAtm* atom1,const TAtm* atom2,double bondOrder=TBond::Single);
    TBond(const TAtm* atom1,const TAtm* atom2, DisplayStyle display_style,
	  double bondOrder=TBond::Single);
    TBond(const TBond& bond);

  // Virtual Destructor
    virtual ~TBond(void);

  // Operators
    TBond& operator=(const TBond& bond);
    bool operator==(const TBond& bond) const;
    bool operator!=(const TBond& bond) const;
    bool operator<(const TBond& bond) const;
    static unsigned long hFun(const TBond& a);

  // Modifiers
  //  void order(double order);
    void displayStyle(const DisplayStyle);
    void order(const double& order);	
    void index(const int& value);
				
  // Accessors			
    TAtm*        atom1(void) const;
    TAtm*        atom2(void) const;
    double       order(void) const;
    DisplayStyle displayStyle(void) const ;
    int          index(void) const;

  // General
    double           length(void) const;
    bool             contains(const TAtm& atom) const;

protected:			
  				
  // Protected Modifiers		
    void atom1(const TAtm* atom1);
    void atom2(const TAtm* atom2);

private:

  // State
    TAtm         *p_atom1;
    TAtm         *p_atom2;

    float        p_order;

    DisplayStyle p_displayStyle;

    // Having the index in here is not really a good design since it is
    // the containing class that cares about indices.  However, its
    // allows us to do what we need to do with good performance.
    int           p_index;

}; // TBond

struct ltTBondPtr {
  bool operator()(const TBond* b1, const TBond* b2) const {
    return (*b1) < (*b2);
  }
};

ostream& operator<<(ostream& os, const TBond& bond);

#endif /* TBOND_HH */
