//#define DEBUG
#include <sys/types.h>
#include <time.h>

#include "util/ErrMsg.hpp"

#include "tdat/PropTSVecTable.hpp"   
#include "tdat/TAtm.hpp"   
#include "tdat/TBond.hpp"
#include "tdat/IBondList.hpp"

#include "inv/ChemKit/ChemKit.hpp"
#include "inv/ChemKit/ChemDisplay.hpp"
#include "inv/ChemKit/ChemDisplayParam.hpp"
#include "inv/ChemKit/ChemBaseDataElement.hpp"
#include "inv/ChemKit/ChemInit.hpp"

#include "viz/PropSGFragment.hpp"   

SO_NODE_SOURCE(PropSGFragment);


///////////////////////////////////////////////////////////////////////////
// Description
///////////////////////////////////////////////////////////////////////////
void PropSGFragment::initClass()
{
   SO_NODE_INIT_CLASS(PropSGFragment, SGFragment, "SGFragment");
}

///////////////////////////////////////////////////////////////////////////
// Descriptions
//  Default constructor - required for NODE but not allowed by
//  ECCE Subject hierary.  That is why we can get away with calling
//  the superclass with a 0 pointer.
///////////////////////////////////////////////////////////////////////////
PropSGFragment::PropSGFragment() : SGFragment(), p_propBondIndices(0)
{
  SO_NODE_CONSTRUCTOR(PropSGFragment);
  p_traceTable = 0;
  touchNumbers();
}

PropSGFragment::PropSGFragment(const char *name) : SGFragment(name),
                               p_propBondIndices(0)
{
  SO_NODE_CONSTRUCTOR(PropSGFragment);
  p_traceTable = 0;
  touchNumbers();
}



///////////////////////////////////////////////////////////////////////////
// Description
//   Construct object from a stream and do it even though the ECCE
//   Subject hierarrcy doesn't support default constructors.  We 
//   construct a tiny dummy object as a work around.  
///////////////////////////////////////////////////////////////////////////
PropSGFragment::PropSGFragment(istream& infile, const string& type) 
                        : SGFragment(), 
                          p_propBondIndices(0)
{

  SO_NODE_CONSTRUCTOR(PropSGFragment);
  p_traceTable = 0;
  touchNumbers();

}

PropSGFragment::PropSGFragment(const PropSGFragment& frag) 
         : SGFragment(frag), p_propBondIndices(0)
{
  SO_NODE_CONSTRUCTOR(PropSGFragment);
  
  PropTSVecTable* tmpTable = frag.getTraceTable();
  if (tmpTable)
     p_traceTable = new PropTSVecTable(*(tmpTable));
  else
     p_traceTable = 0;

  touchNumbers();
}

///////////////////////////////////////////////////////////////////////////
// Description
//   Have to initialize display styles.
///////////////////////////////////////////////////////////////////////////
PropSGFragment::PropSGFragment(const SGFragment& frag) 
         : SGFragment(frag), p_propBondIndices(0)
{
   SO_NODE_CONSTRUCTOR(PropSGFragment);

   p_traceTable = 0;

  touchNumbers();
}

//===========================================================================
    PropSGFragment::~PropSGFragment()
//===========================================================================
{
  if (p_propBondIndices) delete p_propBondIndices;
  if (p_traceTable) delete p_traceTable ;
}



void PropSGFragment::setTraceTable(PropTSVecTable *traceTable)
{
   if (p_traceTable) delete p_traceTable;

   if (traceTable)
   {
      p_traceTable = new PropTSVecTable(*traceTable);
   }
   else 
      p_traceTable = NULL;
}

PropTSVecTable* PropSGFragment::getTraceTable()
{
   return p_traceTable ;
}

PropTSVecTable* PropSGFragment::getTraceTable() const
{
   return p_traceTable ;
}
