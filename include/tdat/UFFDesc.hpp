///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: UFFDesc.H
//
//
// CLASS SYNOPSIS:
//
//   Represents a class for managing UFF-specific code
//
// EXPORTED TYPES:
//    Class UFFDesc
//
// DESCRIPTION:
//
///////////////////////////////////////////////////////////////////////////////
#ifndef UFFDESC_HH
#define UFFDESC_HH


#include <vector>
using std::vector;


#include "tdat/FFRules.hpp"
#include "tdat/TAtm.hpp"
#include "tdat/TBond.hpp"

class UFFDesc: public FFRules
{

   public:
      UFFDesc(void);

      virtual ~UFFDesc(void);


      // Force field rules
      bool checkInversion(TAtm *atm, vector<TBond*> list) const;
      vector<bool> findInversions(TAtm *atm, vector<TBond*> list,
            vector<int> *ringAtoms) const;

   private:

};

#endif 
