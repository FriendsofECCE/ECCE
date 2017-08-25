///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: Theory.H
//
//
// CLASS SYNOPSIS:
//    Theory represents a theory represented as a category and name.
//
// EXPORTED TYPES:
//    Class Theory
//
// LOCAL TYPES:
//
// SEE ALSO:
//      
// DESCRIPTION:
//	A simple class that provides mainly a data structure around
//	the theory information used for computational experiments.
//	The theory data is essentially a dictionary of theories.
//	A given theory is a pair on names: the theory category and
//	the theory name.  Both names are required for unique
//	identification.
//
//			
///////////////////////////////////////////////////////////////////////////////
#ifndef TTHEORY_HH
#define TTHEORY_HH

#include <iostream>
  using std::ostream;


#include <vector>
#include <string>
  using std::vector;
  using std::string;


class TTheory
{

public:

  // Constructors
        TTheory(); // don't use; for ValVector instantiation only
        TTheory(const char *cat, const char *theory, const bool& = true);
        TTheory(const string& cat, const string& theory, const bool& = true);
        TTheory(const TTheory& rhs);

  // Virtual Destructor
        virtual ~TTheory();

  // Operators
        TTheory& operator=(const TTheory&);
        bool operator==(const TTheory&) const;
        bool operator!=(const TTheory&) const;


  // Modifiers
        void category(const string& rhs_cat);
        void name(const string& rhs_name);
        void needsBasis(const bool& rhs_needsBasis);
        void reset(const string& cat, const string& name,
                   const bool& needsBasis);

  // Accessors
        string category() const;
        string name() const;
        string toConciseString() const;
        bool needsBasis() const;


protected:

   // Member Objects

   // Derived Class Accessors
   bool validate_and_assign(const string& cat, const string& name,
                            const bool& needsBasis);

private:

   // State
   string p_category;
   string p_name;
   bool p_needsBasis;


   // Friends
   friend ostream& operator<< (ostream& os, const TTheory&);

}; // TTheory


#endif /* TTHEORY_HH */
