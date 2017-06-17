#ifndef  UNITFAMILY_HH
#define  UNITFAMILY_HH

#include <string>

#include "util/EcceMap.H"
#include "util/InvalidException.H"

using std::string;

/**
 *  This class is used to collect groups of units that are commonly used
 *  together. These families are defined in configuration files in
 *  data/client/config using the naming convention NAM.family. The
 *  UnitFamily is obtained using the UnitFactory class and the family
 *  name NAM. An example would be the CGS family which groups the units
 *       Energy: KCal/Mole
 *       Length: Angstrom
 *       Time: ps
 *       Temperature: Kelvin
 *  together since these units are often preferred by people doing molecular
 *  dynamics. By comparison, the AU.family uses Hartree for Energy and Bohr
 *  for distance, since these values are often preferred by people doing
 *  electronic structure. The primary function of the UnitFamily is to return
 *  the preferred units, defined in the NAM.family file, for a given unit
 *  class. This is done with the get method.
 */

class UnitFamily : protected EcceMap {

   public:
      virtual ~UnitFamily();

      bool  operator==(const UnitFamily& rhs);
      UnitFamily&  operator=(const UnitFamily& rhs);

      string familyName() const;

      string getFullName() const;

      string get(const string& unitclass) const throw(InvalidException);

   protected:

      UnitFamily(const string& name);
      void set(const string& unitclass, const string& value);
      void setFullName(const string& name);

      string p_name;
      string p_fullName;


  private:

    friend class UnitFactory;
};


#endif
