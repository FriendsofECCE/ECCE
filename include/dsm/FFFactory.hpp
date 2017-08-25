///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: FFFactory.H
//
//
// CLASS SYNOPSIS: Factory (creator) methods for forcefields.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef FFFACTORY_HH
#define FFFACTORY_HH


#include <string>

using std::string;

#include "util/InvalidException.H"
#include "util/ParseException.H"
#include "tdat/FFDesc.H"

class FFFactory {

   public:

      FFFactory();
      virtual ~FFFactory();

      /**
       * Creates a FFDesc object for the specified force field.
       * Caller currently responsible for memory.
       */
      FFDesc *getFFDescriptor(const string& name) 
               throw (InvalidException, ParseException);

   protected:


};

#endif
