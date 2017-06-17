/**
 * @file
 *
 *
 */

#include "util/UnitFamily.H"

/**
 * Constructor.
 */
UnitFamily::UnitFamily(const string& name) : p_name(name)
{
  p_fullName = "";
  // Null conversion is always allowed.
  set("Null", "Null");
}


/**
 * Destructor.
 */
UnitFamily::~UnitFamily()
{
}

/**
 * Get the name of this unit family (example SI).
 */
string UnitFamily::familyName() const
{
   return p_name;
}

/**
 * Comparison - equal if names are equal.
 */
bool UnitFamily::operator==(const UnitFamily& rhs)
{
   return rhs.familyName() == familyName();
}

/**
 * Assignment.
 * Family name and all conversions copied.
 */
UnitFamily& UnitFamily::operator=(const UnitFamily& rhs)
{
   p_name = rhs.familyName();

   // copy the map
   EcceMap::operator=(rhs.toString());

   return *this;
}


/**
 * Get the units to use for the specified unitclass for this family.
 *
 * @throw InvalidException if unitclass is not defined for this family.
 */
string UnitFamily::get(const string& unitclass) const throw(InvalidException)
{
   string ret;
#if 000
   if ( !findValue(unitclass, ret))
      throw InvalidException(unitclass + " not found in UnitFamily", WHERE);
#else
   (void)findValue(unitclass, ret);
#endif

   return ret;
}

/**
 * Set the units to use for the specified unitclass for this family.
 *
 * No checking is done on either parameter.
 */
void UnitFamily::set(const string& unitclass, const string& units)
{
   map<string, string, less<string> >::insert( EcceMapPair(unitclass,units) );
}


void UnitFamily::setFullName(const string & fullName)
{
  p_fullName = fullName;
}


string UnitFamily::getFullName() const
{
  return p_fullName;
}
