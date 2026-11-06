#ifndef ADDDIPOLEMOMENTCMD__H
#define ADDDIPOLEMOMENTCMD__H

#include <utility>
#include <vector>
#include "viz/AddVectorCmd.H"
#include "tdat/BondDipoles.H"

class SGFragment;

/**
 *   The dipole moment as a plain arrow from the centre of negative partial
 *   charge to the centre of positive partial charge (IUPAC direction), drawn
 *   along mu where the two differ,.  Length
 *   follows the geometry, not |mu|.  No arrow, and , when
 *   mu vanishes.  Charges: CHELPG, else Merz-Kollman, else Mulliken.
 *   With the BondCharges parameter set to a charge property key, the
 *   bond dipoles are drawn as arrows on the bonds as well.
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
      // The viewer corner text for |mu| in Debye; `oppose`: the charges'
      // poles point against mu (the ChargesOppose parameter after execute).
      static string label(double mu, const string& chargeKey,
                          bool oppose = false);

      // Charge sets a calculation stores, for the bond dipoles' choice:
      // property keys in display order, and their names.
      static vector<string> bondChargeKeys(IPropCalculation *calc);
      static string bondChargeName(const string& key);

      // Bond dipoles (#849) from the charge set `key`; the point-charge sum
      // is taken about the centre of mass. bonds[k] = (negative atom, positive
      // atom), so mu points from first to second (negative to positive).
      // False when the charges or the geometry are missing.
      static bool bondDipoles(IPropCalculation *calc, SGFragment *frag,
                              const string& key,
                              std::vector<std::pair<int,int> >& bonds,
                              BondDipoles::Result& result);
};

#endif
