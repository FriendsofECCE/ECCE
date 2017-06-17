///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: FFDesc.H
//
//
// CLASS SYNOPSIS: Contains descriptors for an arbitrary number of ff 
//                 categories.
//
// Design
//    A forcefield will consist of a number of categories that can be 
//    uniquely described in a generic way.  This class provides access to
//    the set of categories defining a single force field.  It is intended
//    as readonly data except for the initial load.
///////////////////////////////////////////////////////////////////////////////
#ifndef FFDESC_HH
#define FFDESC_HH


#include <string>
#include <vector>
#include <stdexcept>

using std::string;
using std::vector;

#include "util/Serializable.H"
#include "tdat/FFCategoryDesc.H"

class FFDesc : protected vector<FFCategoryDesc*>,  public Serializable {

   public:

      FFDesc();
      FFDesc(const string& name);
      virtual ~FFDesc();

      /** set the name */
      void setName(const string& name);

      /** the name of this FFDesc */
      string getName() const;
      
      /** Does a Category of the given name exist? */
      bool exists(const string& name) const;

      /** The total number of categories. */
      using vector<FFCategoryDesc*>::size;

      /** all categories names. */
      vector<string> getCategoryNames() const;

      const FFCategoryDesc& getCategory(int index) const 
                   throw(std::range_error);

      const FFCategoryDesc& getCategoryByName(const string& name) const
                   throw(std::range_error);

      /** add category - no checking on uniqueness */
      void addCategory(FFCategoryDesc *category);

      /** remove category - range_error if name not defined. */
      void removeCategory(const string& name) throw(std::range_error);

      void clear();

   protected:

      /** index of name or -1 if not found. */
      int index(const string& name) const;

      string p_name;

};

#endif
