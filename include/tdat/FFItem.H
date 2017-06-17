/**
 * @file
 *
 *
 */

#ifndef FFITEM_HH
#define FFITEM_HH

#include <vector>
#include <string>

using std::vector;
using std::string;

#include "util/IndexOutOfRangeException.H"

/**
 * The FFItem (Force Field Item) class comprises information and methods for 
 * handling individual force field line items is in an amber .par file
 * as well as such items defined by a user.
 */

class FFItem {

public:
   FFItem( const int sourceID);
   FFItem( const int sourceID, vector<string>& params, string& annot);

   ~FFItem(void);

         void            addParameter( const string& param_value);
         void            setAnnotation( const string& annot);
         void            remove( void);
         void            print( void) const;

         int             getSourceID( void) const;
         int             getParamCount( void) const;
   const string&         getParam( const int paramID) const 
                                              throw (IndexOutOfRangeException);
   const vector<string>& getParams( void) const;
   const string&         getAnnotation( void) const;
         bool            isRemoved( void) const;

   static const int INVALID_ID;

private:
      int            p_sourceID;
      string         p_annotation;
      vector<string> p_ffParameters;
}; 

#endif /* FFITEM_HH */
