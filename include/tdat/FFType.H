/**
 *  @file
 *
 */
#ifndef FFTYPE_HH
#define FFTYPE_HH

#include <vector>
  using std::vector;
#include <string>
  using std::string;
#include <algorithm>
  using std::find;

#include "util/IndexOutOfRangeException.H"
#include "util/InvalidException.H"
#include "tdat/FFItem.H"

/** The force field type class (FFType) contains information and methods 
 *  for handling force field types.  Example force field types of the
 *  "Atoms" category are "HO", "HS", "C", "CA".  Force field types may have 
 *  as many member items (essentially, parameter sets) as there are input 
 *  sources (plus one and only one "new" item defined by the user.)  
 *  An FFType's items are added to a vector of items in the order they are 
 *  instantiated. Since each item contains a complete set of parameters to
 *  characterize the FFType, only one item can apply at any point in time.
 *  The last item on the FFType's item vector is considered to be the 
 *  "current" item.
 *
 *  @see FFCategory
 *  @see FFItem
 */
class FFType {

 public:
   enum TYPE_SRC_STATUS { NEWLY_ASSIGNED, SINGLE_ASSIGNED,  MULTI_ASSIGNED,
                           NO_ASSIGNED, INVALID_ASSIGNED};

            FFType( const string& typeName);  //FFType instance must have a name
   virtual ~FFType(void);

         int     getItemCount( void) const;
         int     getItemID( int index) const throw(IndexOutOfRangeException);
   const string& getName( void) const;
         void    print( void) const;
         int     getLastItemID( void) const throw (InvalidException);

         void    addItemID( int ffitemID);
         void    removeItemID( int index)  throw(IndexOutOfRangeException);

   static const int INVALID_ID;

   private:
         // Variables----------------------------------------------------------
         string      p_name;
         vector<int> p_ffItemIDs;

}; // FFType
#endif /* FFTYPE_HH */
