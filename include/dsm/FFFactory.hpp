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

#include "util/InvalidException.hpp"
#include "util/ParseException.hpp"
#include "tdat/FFDesc.hpp"

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
