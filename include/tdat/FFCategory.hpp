////////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: FFCategory.H
//
//
// CLASS SYNOPSIS:
//
// EXPORTED TYPES:
//    Class FFCategory
//
// SEE ALSO:
//
// DESCRIPTION:
//
////////////////////////////////////////////////////////////////////////////////
#ifndef FFCATEGORY_HH
#define FFCATEGORY_HH

// Library includes:
#include <vector>
#include <string>
#include <stdexcept>
#include <map>
  using std::vector;
  using std::string;
  using std::less;
  using std::map;
  using std::find;
  using std::range_error;
  using std::invalid_argument;

#include "util/InvalidException.H"
#include "util/IndexOutOfRangeException.H"

#include "FFItem.H"
#include "FFType.H"
#include "FFParamSet.H"
#include "FFCategoryDesc.H"


////////////////////////////////////////////////////////////////////////////////
class FFCategory {

   //---------------------------------------------------------------------------
   public:
   //---------------------------------------------------------------------------
     // must be in same order as FFModel
      enum eCategoryTypes { ATOM_TYPE, CROSS_TYPE, BOND_TYPE, ANGLE_TYPE, 
                     PROPER_TYPE, IMPROPER_TYPE, CAT_TYPE_COUNT, INVALID_TYPE};

      // Constructors-----------------------------------------------------------
      FFCategory( string&);

      // Destructor-------------------------------------------------------------
      ~FFCategory(void);

      // Accessors--------------------------------------------------------------
            string    getName( void) const;
       eCategoryTypes getCategoryType( void) ;
      const FFItem*   getFFItem( int, int)  throw(std::range_error);
            void      getFFItems( vector<int>& srcIDs, vector<string>&,
                                                              vector<FFItem*>&);
            int       getItemSourceID( int) const  throw(std::range_error);
            int       getTypeCount( void) const;
            int       getTypeID( string, bool flag = false);
            int       getMatchingTypeID( const string&);
            void      getTypeSourceIDs( int, vector<int>&)  const;
            string    getTypeName( int) const throw(std::range_error);
            void      getTypeNameComponents( const string&,vector<string>&, int);
            string    getTypeNameAndSrcStatus( int, int, int*) const;
            FFType::TYPE_SRC_STATUS getTypeSrcStatus( int, int) const;
            int      getAtomicNo( int typeID) throw (InvalidException, 
	                                             IndexOutOfRangeException);
            int      getTypeItemID( int, int) const throw(std::range_error);
            int      getTypeItemCount( int) const;

            void     print( void) const;

      // Modifiers--------------------------------------------------------------
            void addFFType( FFType*);
            void addFFItem( int, FFItem*);
            void addFFItem( const string&, FFItem*);
            bool removeFFItem( const string&, int) throw(std::invalid_argument);
            bool removeFFType( int)  throw(std::invalid_argument);
            void rebuildTypeNameMap();
            void sortByTypeName( void);
	    void setAtomicNoID( int atomicNoID);

      // Parses name like CV-NB-CV into its parts.  Guarenteed to have the
      // correct number of parts for the type (ie --CB will have 3 parts).
      static void  parseTypeName( const string& str, vector<string>& parts);

       // Constant
       // This should not have value the same as a valid category type.
       // The value -1 has been used throughout as it is a universally invalid
       // index.
       static const int INVALID_INDEX;
       
   //---------------------------------------------------------------------------
   private:
   //---------------------------------------------------------------------------
      // Variables--------------------------------------------------------------

      string                  p_name;
      vector<FFType*>         p_ffTypes;
      map<string, int>        p_ffTypeNames; //map names to FFType vec index
      vector<FFItem*>         p_ffItems;// pointers to removed new items will be set to null
      FFParamSet              p_ffparams;
      int                     p_atomicNoID;
      map<int,int,less<int> > p_partialMatch;

      // methods
      void           clear( void);
      bool           isValidTypeID( int) const;
      bool           isValidItemID( int) const;
      bool           newSourceForType( int, int);
      void           savePartialMatch( int, const string&, string&);
      bool           endMatches( string&, string&);
      void           orderNameComponents( string&) ;
      void           parseTypeName( const string&, vector<string>&, int);
      FFType*        getType( int typeID) const throw (IndexOutOfRangeException);
      FFType*        getType( const string& typeName) throw (InvalidException);
      FFType::TYPE_SRC_STATUS getTypeSrcStatus( FFType*, int) const;

}; // FFCategory

#endif /* FFCATEGORY_HH */
