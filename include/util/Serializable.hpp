///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: Serializable.H
//
//
// CLASS SYNOPSIS: Top level class for all objects that can be serialized.
//
// DESCRIPTION:
//   For the most part, this class exists for the purpose of having a
//   common interface with the capability for dynamic casting.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef SERIALIZABLE_HH
#define SERIALIZABLE_HH

#include <string>

class Serializable
{
   public:
      virtual ~Serializable() {}
      virtual std::string mimetype() const { return p_mimetype; }
      virtual void mimetype( const std::string& type ) { p_mimetype = type; }

   protected:
      std::string p_mimetype;

   private:

}; 


#endif 
