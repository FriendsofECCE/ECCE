/**
 * @file
 *
 *
 */

#include <math.h>

#include "util/Ecce.H"
#include "util/SDirectory.H"
#include "util/KeyValueReader.H"
#include "util/UnitFactory.H"
#include "util/EnergyConverter.H"
#include "util/LengthConverter.H"
#include "util/AngleConverter.H"
#include "util/PressureConverter.H"
#include "util/BondFrcConstConverter.H"
#include "util/BendFrcConstConverter.H"
#include "util/TrsnFrcConstConverter.H"
#include "util/TimeConverter.H"
#include "util/MassConverter.H"
#include "util/TemperatureConverter.H"
#include "util/CompressibilityConverter.H"
#include "util/DriftConverter.H"
#include "util/NullConverter.H"

// Converter helper types
typedef map<string, UnitConverter*, less<string> >::value_type ConverterMapPair;
typedef map<string, UnitConverter*, less<string> >::const_iterator ConverterIterator;

// Family helper types
typedef map<string, UnitFamily*, less<string> >::value_type FamilyMapPair;
typedef map<string, UnitFamily*, less<string> >::const_iterator FamilyIterator;


UnitFactory* UnitFactory::p_instance = 0;

/**
 * Inializes set of converters and unit families.
 */
UnitFactory::UnitFactory()
{

  loadConverters();
  loadFamilies();

}


UnitFactory::~UnitFactory()
{
   // TODO Delete Unit Converters  and Families in a destory method

}

/**
 * Hand over the singleton UnitFactory
 */
UnitFactory& UnitFactory::getInstance()
{
  if (p_instance == (UnitFactory*)0) {
    p_instance = new UnitFactory;
  }
  return *p_instance;
}

UnitConverter& UnitFactory::getUnitConverter(string units)
  throw (InvalidException)
{
   ConverterIterator it = p_converters.find(units);

   if (it == p_converters.end()) {
     throw InvalidException(units + " is an unknown units class.", WHERE);
   }

   return *(*it).second;
}

vector<string> UnitFactory::getConverterNames() 
{
   vector<string> names;

   ConverterIterator it = p_converters.begin();
   while ( it != p_converters.end() ) {
      names.push_back( (*it).first );
      it++;
   }
   return names;
}

UnitFamily& UnitFactory::getUnitFamily(const string& family) 
             throw(InvalidException)
{
   FamilyIterator it = p_families.find(family);

   if (it == p_families.end()) {
     throw InvalidException(family + " is an unknown units family.", WHERE);
   }

   return *(*it).second;
}


UnitFamily& UnitFactory::getFamilyByFullName(const string & fullName)
  throw(InvalidException)
{
  FamilyIterator it = p_families.begin();
  while ( it != p_families.end() ) {
    if (it->second->getFullName() == fullName)
      return *(it->second);
    it++;
  }
  throw InvalidException(fullName + " is an unknown units family.", WHERE);
}


vector<string> UnitFactory::getFullFamilyNames() 
{
  vector<string> names;

  FamilyIterator it = p_families.begin();
  while ( it != p_families.end() ) {
    names.push_back(it->second->getFullName());
    it++;
  }
  return names;
}


vector<string> UnitFactory::getFamilyNames() 
{
   vector<string> names;

   FamilyIterator it = p_families.begin();
   while ( it != p_families.end() ) {
      names.push_back( (*it).first );
      it++;
   }
   return names;
}

/**
 * Load definitions of unit families from *.familes files.
 */
void UnitFactory::loadFamilies()
{
   if (p_families.size() == 0) {
      string unitsPath(Ecce::ecceDataPrefPath());

      // Get listing of all files in config directory
      SDirectory dir(unitsPath);
      vector<SFile> files;
      dir.get_files(files, false);

      string key,value;
      for (size_t idx=0; idx<files.size(); idx++) {
         string name = files[idx].filename();

         // Load any file that has .family in it.
         // Might be nice to require that it ends in .family
         if (name.find(".family") != string::npos) {
            string family = name.substr(0,name.find(".family"));
            UnitFamily *uf = new UnitFamily(family);
            KeyValueReader kvr(files[idx].path());
            while (kvr.getpair(key,value)) {
              if (key == "FullName")
                uf->setFullName(value);
              uf->set(key,value);
            }

            // Now add to map
            p_families.insert( FamilyMapPair(family, uf) );
         }
      }
   }
}

/**
 * Converters are basically hardwired entities right now - just create the map.
 */
void UnitFactory::loadConverters()
{
   p_converters.insert( ConverterMapPair("Energy", new EnergyConverter()) );
   p_converters.insert( ConverterMapPair("Length", new LengthConverter()) );
   p_converters.insert( ConverterMapPair("Angle", new AngleConverter()) );
   p_converters.insert( ConverterMapPair("Pressure", new PressureConverter()) );
   p_converters.insert( ConverterMapPair("Bond Force Constant", new BondFrcConstConverter()) );
   p_converters.insert( ConverterMapPair("Bend Force Constant", new BendFrcConstConverter()) );
   p_converters.insert( ConverterMapPair("Torsion Force Constant", new TrsnFrcConstConverter()) );
   p_converters.insert( ConverterMapPair("Time", new TimeConverter()) );
   p_converters.insert( ConverterMapPair("Temperature", new TemperatureConverter()) );
   p_converters.insert( ConverterMapPair("Compressibility", new CompressibilityConverter()) );
   p_converters.insert( ConverterMapPair("Drift", new DriftConverter()) );
   p_converters.insert( ConverterMapPair("Mass", new MassConverter()) );
   p_converters.insert( ConverterMapPair("Null", new NullConverter()) );
}
