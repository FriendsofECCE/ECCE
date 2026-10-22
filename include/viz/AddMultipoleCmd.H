#ifndef ADDMULTIPOLECMD__H
#define ADDMULTIPOLECMD__H

#include <vector>
#include "viz/AddVectorCmd.H"

class PropVector;

/**
 *   Quadrupole (QUADPOLE), octupole (OCTAPOLE) or hexadecapole (HEXPOLE) as
 *   the surface r(n) = s |f(n)| around the expansion origin, f(n) being the
 *   moment contracted with n; positive and negative f get two colours.
 *   "Amplitude" is s in Angstrom per D.Angstrom^(k-1); "Center" is the
 *   origin ("charge" or "mass"); "Code" names the code that wrote the
 *   moment, which fixes its units.
 */
class AddMultipoleCmd : public AddVectorCmd
{
   public:
      AddMultipoleCmd(const string& name, Receiver *, IPropCalculation *);
      ~AddMultipoleCmd();

      virtual bool execute();

      // Unused: this command draws no arrows.
      virtual bool getDisplacements(vector<double*>& displacements,
                                  vector<SIGN>& signs);

      // 2, 3, 4 for the three property keys, else 0.
      static int rankOf(const string& property);

      /**
       * The rank-k multipole in D.Angstrom^(k-1) with its traces removed.
       * The parsers store NWChem and Gaussian values in D.Angstrom^(k-1)
       * already and ORCA's in a.u.; "code" tells them apart.  False if the
       * property is absent or has the wrong number of components.
       */
      static bool tensor(const PropVector *vec, int rank, const string& code,
                         vector<double>& full);

      // Okabe-Ito blue and vermillion.
      static const char *POSITIVE_COLOR;
      static const char *NEGATIVE_COLOR;
};

#endif
