///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: FFModel.H
//
//
// CLASS SYNOPSIS:
//
// EXPORTED TYPES:
//    Class FFModel
//
// SEE ALSO:
//
// DESCRIPTION: FFModel is implemented in the Singleton pattern - so that only
//                      a single instance is allowed.  FFModel inherits from
//                      the Subject class of the Observable pattern.  Objects
//                      can register for notification of changes to FFModel.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef FFMODEL_HH
#define FFMODEL_HH

#include <vector>
#include <string>
#include <algorithm>
#include <stdexcept>
  using std::vector;
  using std::string;
  using std::sort;

#include "util/Serializable.H"
#include "util/IOException.H"

#include "tdat/FFSources.H"
#include "tdat/FFDesc.H"
#include "tdat/FFCategoryDesc.H"
#include "tdat/FFCategory.H"

#include "dsm/ObserverSubject.H"

class FFModel : public Subject, public Serializable 
{

   /*=========================================================================*/
   private: 
   /*=========================================================================*/
      static FFModel*     p_instance;
      string              p_ffName;
      FFSources           p_ffSources;
      int                 p_newItemCount;
      int                 p_newSourceID;
      vector<FFCategory*> p_ffCategories;
      FFDesc*             p_ffDesc;
      vector<string>      p_ElectrostaticStrings;   // needed?
      vector<string>      p_RelativeStrings;        // needed?
      vector<string>      p_ParametersStrings;      // needed?
      vector<int>         p_selectedSources;
      vector< string>     p_ffCatNames;

      bool isValidCategoryIndex( int) const;
      void initializeFFCategories() throw(std::range_error);
      void clear( void);

   /*=========================================================================*/
   protected:
   /*=========================================================================*/
      FFModel( void);

   /*=========================================================================*/
   public:
   /*=========================================================================*/
      static FFModel* instance();

      // Destructor
      ~FFModel(void);

      // Modifiers
      void load( const string&, vector<string>&) throw(IOException);
      void addNewFFItem( int, const string&, vector<string>&, string&);
      bool removeFFItem( int, int, const string&);
      int  getNewSourceID( void);
      void setSelectedSources( vector<string>);
      void addElectrostaticString(string str);    // needed?
      void addRelativeString(string str);         // needed?
      void addParametersString(string str);       // needed?

      // Accessors
      bool         isLoaded( void) const;
      int          getMaxParameterColumns( void);
      int          getCategoryCount( void) const;
            
      FFCategory::eCategoryTypes getCategoryType( const string&) const;
      string       getCategoryLabel( int) const;
      int          getTypeCount( int) const;
      int          getAtomicNo( int typeID) const;
      int          getTypeItemCount( int, int) const;
      int          getTypeSrcStatus( int, const string&);
      void         getTypeSources( int, int, vector<string>&) const;
      string       getSourceName( int, int, int) const;
      void         getSourceNames( vector<string>&) const;
      void         getTypeNameComponents( int, const string&, 
                                vector<string>&) const throw(std::range_error);
      string       mapTypeIDToName( int, int);
      string       mapTypeIDToName( int, int, int*);
      int          mapTypeNameToID( int, const string&) const;
      void         getFFItems( int, vector<int>, vector<string>&,
                                                        vector<FFItem*>&) const;
      int          getCategoryID( string);
      FFCategory*  getCategory(int);
const FFItem*      getFFItem( int, int, int) const throw(std::range_error);
      vector<int>  selectedSources() const;
const FFDesc& getDesc( void);
      int numElectrostaticStrings(void);   // needed?
      int numRelativeStrings(void);        // needed?
      int numParametersStrings(void);      // needed?
const FFCategoryDesc& getCatDesc( int) const throw(std::range_error);


      enum ffNameStatus { VALID_TYPENAME,
                          NULL_TYPENAME, 
                          EMPTY_TYPENAME, 
                          INVALID_DASH_COUNT_TYPENAME,
                          ALL_DASHES_TYPENAME,
                          INVALID_ATOM_IN_TYPENAME};

}; // FFModel

#endif /* FFMODEL_HH */
