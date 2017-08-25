///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: UnitFactory.H
//
//
// CLASS SYNOPSIS: This is a factory that returns a unit converter
//                 corresponding to whatever units need to be converted.
//
// Design
//   Returns a unit conversion module based on user request
//
// Example Usage:
//   UnitFactory& uf = UnitFactory::getInstance();
//   UnitConvert uc = UnitFactory::getInstance().getUnitConverter("xxx");
///////////////////////////////////////////////////////////////////////////////
#ifndef UNITFACTORY_HH
#define UNITFACTORY_HH
// Library includes
#include <map>
#include <string>
#include <vector>

  using std::string;
  using std::vector;
  using std::map;

#include "util/InvalidException.H"
#include "util/UnitConverter.H"

#include "util/UnitFamily.H"

/**
 *  The UnitFactory class will return the correct UnitConverter for a given
 *  unit class. For example, the UnitFactory will return an EnergyConverter
 *  if the getUnitConverter method is invoked with the argument "Energy".
 *  The EnergyConverter can then be used to convert a value between different
 *  energy units.
 *
 *  The UnitFactory can also be used to get an instance of a UnitFamily. The
 *  UnitFamily groups together units that are commonly used together. This
 *  allows application developers to provide easy conversions between commonly
 *  used unit groupings and also to set defaults for different applications.
 */  

class UnitFactory
{
   public:
      static UnitFactory& getInstance();

      virtual ~UnitFactory();

      UnitConverter& getUnitConverter(string units) throw(InvalidException);
      /**
       *  Provide a listing of all currently know converters. Useful for
       *  debugging.
       */
      vector<string> getConverterNames();

      UnitFamily& getUnitFamily(const string& name) throw(InvalidException);
      UnitFamily& getFamilyByFullName(const string & fullName) throw(InvalidException);

      /**
       *  Provide a listing of all know unit families. This should correspond
       *  to the number of .family files in the data/client/config directory.
       */
      vector<string> getFamilyNames();
      vector<string> getFullFamilyNames() ;

   protected:
      UnitFactory();

      void loadFamilies();
      /**
       *  This method must be updated every time a new UnitConverter class is
       *  created.
       */
      void loadConverters();

   private:
      // the singleton
      static UnitFactory *p_instance;

      // Map of unit converter names to converter objects
      map<string, UnitConverter*, less<string> > p_converters;

      // Map of family names to family objects
      map<string, UnitFamily*, less<string> > p_families;

};

#endif   //UNITFACTORY_HH
