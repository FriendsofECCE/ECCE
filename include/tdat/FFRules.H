///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: FFRules.H
//
//
// CLASS SYNOPSIS: Abstract class that defines rules particular to a given
//                 force field.
//
// DESCRIPTION:
//    Methods that must differentiate between different force field
//    types are collected here.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef FFRULES_HH
#define FFRULES_HH

// Library includes
#include <vector>
#include <string>
  using std::vector;
  using std::string;

class TAtm;
class TBond;

class FFRules
{
   public:

      virtual ~FFRules() {}

      virtual bool checkInversion(TAtm *atm, vector<TBond*> list) const = 0;

      virtual vector<bool> findInversions(TAtm *atm, vector<TBond*> list,
                                  vector<int> *ringAtoms) const = 0;
      void setFFRulesName(string);

      string getFFRulesName(void);

   private:

      string p_ffname;
};
#endif
