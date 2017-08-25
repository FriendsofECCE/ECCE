////////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: FFParamSet.H
//
//
// CLASS SYNOPSIS:
//
// EXPORTED TYPES:
//    Class FFParamSet
//
// SEE ALSO:
//
// DESCRIPTION:
//
////////////////////////////////////////////////////////////////////////////////
#ifndef FFPARAMSET_HH
#define FFPARAMSET_HH

// Library includes:
#include <string>
#include <vector>
  using std::string;
  using std::vector;

class FFParamSet  {

   private:
      vector<string> p_labels;
      vector<string> p_values;

      // modifiers
      void clear( void);

   public:
      FFParamSet( void); // Constructor
      ~FFParamSet(void); // Destructor

      // modifiers
      void reset();
      void addLabels( vector<string>&);
      void addValues( const vector<string>&);
      void addValue( string&);

      // accessors
      int    getValueCount( void) const;
      int    getLabelCount( void) const;
      string getLabel( const int) const;
      string getValue( const int) const;
      void   getLabels( vector<string>& );
      void   getValues( vector<string>& );
      void   print( void) const;
};

////////////////////////////////////////////////////////////////////////////////

#endif /* FFPARAMSET_HH */
