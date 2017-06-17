///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: FFParameterDesc.H
//
//
// CLASS SYNOPSIS:
//  FFParameterDesc encapsulates metadata about a single force field parameter.
//
//      
// DESCRIPTION:
//   FFParameterDesc contains metadata about a force field parameter that can
//   be used to drive validation and units conversion.
//   Note that values are intended to be kept in the default units set.
//   Conversion to user preferred units occurs at the presentation layer.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef FF_PARAMETER_DESC_HH
#define FF_PARAMETER_DESC_HH

#include <string>
#include <vector>

using std::string;
using std::vector;

class FFParameterDesc  {

   public:

      enum ParamType { eINT, eFLOAT, eSTRING, eBOOL, eELEMENT};

      FFParameterDesc( ParamType);
      virtual ~ FFParameterDesc( void);

      //-- Accessors -----------------------------------------------------------
      FFParameterDesc::ParamType getType() const;
      string          getLabel() const;
      string          getUnitClass() const;
      string          getDefaultUnit() const;
      vector<string>  getValidStrings() const;
      int             getMinInt() const;
      int             getMaxInt() const;
      float           getMinFloat() const;
      float           getMaxFloat() const;
      bool            isValid( const string&) const;
      bool            isValid( const int) const;
      bool            isValid( const float) const;
      string          getDefaultValue() const;


            
      //-- Modifiers -----------------------------------------------------------
      void            setLabel( const string& label);
      void            setUnitClass( const string& unitclass);
      void            setDefaultUnit( const string& unitname);
      void            setValidStrings( const vector<string>&);
      void            setMinInt( int min );
      void            setMaxInt( int max );
      void            setMinFloat( float min );
      void            setMaxFloat( float max );
      void            setDefaultValue( const string& value );

   private:

      ParamType        p_type;
      string           p_label;              // parameter label

      string           p_unitClass;
      string           p_defaultUnit;

      // Depending on p_type, one or more (but not all) of these will apply 
      // Too bad we don't have a class hierarchy!
      vector< string>  p_validStrings;
      int              p_minInt;      // in default units
      int              p_maxInt;      // in default units
      float            p_minFloat;    // in default units
      float            p_maxFloat;    // in default units
      // Store default as string so we don't have to have one
      // for each possible type.
      string          p_defaultValue;

      void initialize();
};

#endif // FF_PARAMETER_DESC_HH
