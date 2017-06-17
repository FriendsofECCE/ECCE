///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: FragUtil.H
//
//
// CLASS SYNOPSIS:
//    Utility routines for operating on Fragment class.
//      
// DESCRIPTION:
//   For viz.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef FRAGUTIL_HH
#define FRAGUTIL_HH

#include <iostream>
  using std::istream;

#include <string>
  using std::string;
#include <vector>
  using std::vector;
#include <map>
  using std::map;
  using std::less;


// application includes
#include "tdat/TAtm.H"
#include "tdat/TResChain.H"
#include "tdat/IBondList.H"
#include "util/Transformations.H"

class Fragment;
class TBond;
class TAtm;
class Residue;
class TResChain;
class TTagCountPair;

typedef map<string, int, less<string> > TagCountMap;

class FragUtil
{

   public:

      // Constructors
      FragUtil(Fragment *frag);

      //Destructor
      ~FragUtil(void);

      // Methods
      void transform(vector<double*> list, MMatrix &matrix);
      void changeGeometry(TAtm *orig_atom, const string& symbol,
            const string& newGeom);
      void nonMetalBonds(TAtm *atom, vector<TAtm*> &list);
      void nonMetalnonNubBonds(TAtm *atom, vector<TAtm*> &list);
      TAtm* addAtLocation(const char *element, const char *geom, double atomX,
            double atomY, double atomZ, TAtm *parent);
      void adjustExistingNubs(TAtm *orig_atom);

      // Building functions
      TAtm *addToNub( TAtm *atom1, TAtm *nub1,
            const char *element, const char *geom, double order);
      void bondParts(
            TAtm* mainAtm1, TAtm* nub1Atm, TAtm* main2Atm, TAtm* nub2Atm,
            double order, vector<TAtm*> *a2list);
      void doBond( vector<TAtm*>& atoms,
            TAtm *basenub, TAtm *baseparent,
            TAtm *fragparent, TAtm *fragnub);


      bool load(istream& is, const string& type, double mult, bool genBOs,
                int selectMdl = 1,                     // for pdb
                const string& selectAlt = " ",  // for pdb
                const string& selectChain = " ");      // for pdb



      void createResidues();

   private:

      Fragment *p_frag;
};
#endif
