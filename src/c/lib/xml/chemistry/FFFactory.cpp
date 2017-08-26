////////////////////////////////////////////////////////////////////////////////
// FFDesc Class
//
//
// DESIGN:  
//   This class is a container for FFCategoryDesc objects so as to provide
//   a wel defined interface.
//
////////////////////////////////////////////////////////////////////////////////
#include <fstream>
  using std::ifstream;

#include "util/SFile.hpp"
#include "util/Ecce.hpp"
#include "util/EcceMap.hpp"
#include "util/StringConverter.hpp"

#include "dsm/FFFactory.hpp"
#include "dsm/FFDescXMLSerializer.hpp"

/**
 * Default Constructor.
 */
FFFactory::FFFactory( )
{
}

FFFactory::~FFFactory( )
{
}

FFDesc *FFFactory::getFFDescriptor(const string& name) 
      throw (InvalidException,ParseException)
{
   // Generate file name path
   string path = Ecce::ecceDataPath();
   path += "/client/config/FF";
   path += name;
   path += "Desc.xml";

   SFile file(path);
   if (!file.exists()) {
      throw InvalidException("Force Field descriptor does not exist" + name,
                             WHERE);
   }

   string data;
   ifstream is(file.path().c_str());
   StringConverter::streamToText(is, data);
   is.close();

   // This can throw a ParseException
   FFDescXMLSerializer s;
   FFDesc *ret = new FFDesc();
   EcceMap properties;
   s.deserialize(data, *ret , properties);
   
   return ret;
}

