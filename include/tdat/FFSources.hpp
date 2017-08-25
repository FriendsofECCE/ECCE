////////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: FFSources.H
//
//
// CLASS SYNOPSIS:
//
// EXPORTED TYPES:
//    Class FFSources
//
// SEE ALSO:
//
// DESCRIPTION:  Handles source information. Assumes that the order of the 
//               input file names reflects the order of supersession intended 
//               by the user.  That is, the force field parameters in each file
//               supercede any values given in preceeding files (files with a 
//               lower index).
//
////////////////////////////////////////////////////////////////////////////////
#ifndef FFSOURCES_HH
#define FFSOURCES_HH

#include <string>
#include <vector>
using std::string;
using std::vector;

#include "util/IndexOutOfRangeException.H"
#include "util/SFile.H"

#include "tdat/FFCategory.H"

////////////////////////////////////////////////////////////////////////////////
class FFSources 
{

   public:
      // Constructor Methods----------------------------------------------------
      FFSources( void); 

      // Destructor Method------------------------------------------------------
      ~FFSources(void);

      // Modifier Methods-------------------------------------------------------
      void loadSources( vector<string>&);

      // Accessor Methods-------------------------------------------------------
      int    getSourceID( string&) const;
      int    getNewSourceID( void) const;
      int    getSourceCount( void) const;
      string getSourceName( int) const throw (IndexOutOfRangeException);
      string getNewSourceName() const;

      static const char* NEW_SOURCE;

   private:
      // Variables--------------------------------------------------------------
      vector<SFile*> p_sources;       // last source is the superceding one

      // Methods----------------------------------------------------------------
      void   clear( void);

}; // FFSources

#endif /* FFSOURCES_HH */
