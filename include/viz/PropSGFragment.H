#ifndef  PROPSGFRAGMENT_HH
#define  PROPSGFRAGMENT_HH

#include <iostream>
  using std::istream;

#include "viz/SGFragment.H"

class PropTSVecTable;


class  PropSGFragment : public SGFragment {
  
    SO_NODE_HEADER(PropSGFragment);


public:

    PropSGFragment();
    PropSGFragment(const char *name);
    PropSGFragment(istream& istr, const string& type);
    PropSGFragment(const PropSGFragment& frag);
    PropSGFragment(const SGFragment& frag);
    virtual  ~PropSGFragment(void);

    // access
    void setTraceTable(PropTSVecTable *geomtrace);
    PropTSVecTable* getTraceTable();
    PropTSVecTable* getTraceTable() const ;

SoINTERNAL public:
    static void		initClass();

  SoEXTENDER public:

protected :

   IBondList        *p_propBondIndices ;
   PropTSVecTable   *p_traceTable ;

};








#endif /* SGFRAGMENT__HH */

