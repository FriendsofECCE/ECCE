///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: TOxidationState.H
//
//
// CLASS SYNOPSIS:
//   Data structure for element data that varies with formal charge.
//
// SEE ALSO:
//      
// DESCRIPTION:
//
//
///////////////////////////////////////////////////////////////////////////////
#ifndef TOXIDATIONSTATE_HH
#define TOXIDATIONSTATE_HH

#include <iostream>
  using std::ostream;

#include <string>
  using std::string;


class TOxidationState 
{

   public:

      // Constructors
      TOxidationState(const string& symbol, float charge);
      TOxidationState(const TOxidationState&);

      // Virtual Destructor
      virtual ~TOxidationState(void);

      // Operators
      TOxidationState& operator=(const TOxidationState&);
      bool operator==(const TOxidationState&) const;
      bool operator!=(const TOxidationState&) const;

      // Modifiers
      void vwr(float value);
      void electronConfiguration(const string& value);

      // Accessors
      string          symbol(void) const;
      float           charge(void) const;
      float           vwr(void) const;
      string          electronConfiguration(void) const;


   protected:


   private:

      // State
      string    p_symbol;
      float     p_charge;
      float     p_vwr;
      string    p_electronConfig;


      // Friends
      friend ostream& operator<< (ostream& os, const TOxidationState&);

}; // TOxidationState

#endif /* TOXIDATIONSTATE_HH */
