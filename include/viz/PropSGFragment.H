#ifndef  PROPSGFRAGMENT_HH
#define  PROPSGFRAGMENT_HH

#include <iostream>
  using std::istream;

#include "viz/SGFragment.H"

class PropVecTable;


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
    // PropVecTable, not PropTSVecTable: the only thing ever stored here is
    // the normal-mode animation table NModeTraceCmd builds from the VIB
    // property, and VIB is declared PropVecTable in
    // data/client/config/properties -- so PropFactory really does hand back
    // a PropVecTable.  It used to be declared PropTSVecTable and worked only
    // because the two copy-pasted classes happen to share a member layout
    // (GitHub issue #87).
    void setTraceTable(PropVecTable *geomtrace);
    PropVecTable* getTraceTable();
    PropVecTable* getTraceTable() const ;

SoINTERNAL public:
    static void		initClass();

  SoEXTENDER public:

protected :

   IBondList        *p_propBondIndices ;
   PropVecTable   *p_traceTable ;

};








#endif /* SGFRAGMENT__HH */

