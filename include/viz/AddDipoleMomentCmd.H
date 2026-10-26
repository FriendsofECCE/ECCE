#ifndef ADDDIPOLEMOMENTCMD__H
#define ADDDIPOLEMOMENTCMD__H

#include "viz/AddVectorCmd.H"

/**
 *   The dipole moment as a plain arrow from the centre of negative partial
 *   charge to the centre of positive partial charge (IUPAC direction), drawn
 *   along mu where the two differ, with "mu = x.xx D" beside it.  Length
 *   follows the geometry, not |mu|.  No arrow, and the label "mu = 0", when
 *   mu vanishes.  Charges: CHELPG, else Merz-Kollman, else Mulliken.
 */
class AddDipoleMomentCmd : public AddVectorCmd
{
   public:
      AddDipoleMomentCmd(const string& name, Receiver *, IPropCalculation *);
      ~AddDipoleMomentCmd();

      virtual bool execute();
      virtual bool getDisplacements(vector<double*>& displacements,
                                  vector<SIGN>& signs);

      // Property key of the charges used ("ESPFIT", "MKFIT", "MULLIKEN"),
      // or "" when the calculation has none.
      static string chargeKey(IPropCalculation *calc);
      // Name of those charges for the user.
      static string chargeName(const string& key);
      // The label for |mu| in Debye.
      static string label(double mu);
};

#endif
