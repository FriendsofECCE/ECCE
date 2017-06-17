/**
 * @file
 *
 *
 */
// DESIGN:  FFItem contains information and methods for handling individual
//          force field line items in a .par file.  They also handle such items
//          defined by a user.

#include <string.h>
#include <iostream>
  using std::cout;
  using std::endl;

// #include "util/ErrMsg.H"
#include "tdat/FFItem.H"

const int FFItem::INVALID_ID = -1;

/**
 * Constructs a FFItem (Force Field Item) instance.  The index of the 
 * force field source is set upon instantiation and is not changeable.
 *
 * @param sourceID Index of the source of this force field item.
 *
 */
FFItem::FFItem( const int sourceID)
:  p_sourceID ( sourceID)                   // ASSUME valid sourceID
{
}

/**
 * Constructs a FFItem (Force Field Item) instance for all force field categories * except Atoms.  All elements of the force field item are specified as input to 
 * this constructor.  The correct number of parameters must be supplied; 
 * parameter values may be empty strings, but not null pointers.
 *
 * @param sourceID Index of the source of this force field item.
 * @param params Vector of parameter values given as strings.
 * @param annot Annotation for this force field item.
 */
FFItem::FFItem( const int sourceID, vector<string>& params, string& annot)
:  p_sourceID ( sourceID)                         // ASSUME valid sourceID
{
   vector<string>::iterator iter = params.begin();
   while ( iter != params.end())  {
      p_ffParameters.push_back( *iter);
      iter++;
   }
   p_annotation = annot;
}

/**
 * Destructor for FFItem
 */
FFItem::~FFItem( void)
{
   p_ffParameters.clear();
}

/**
 * Registers the annotation of this FFItem instance
 *
 * @param annot annotation string
 *
 */
void FFItem::setAnnotation( const string &annot)
{
   p_annotation = annot;
}

/**
 * Registers the force field item's parameters.  The order of 
 * parameters in the FFItem's params vector is the same as the
 * registration order of the individual parameters.
 *
 * @param param_value Parameter value as a string
 *
 */
void FFItem::addParameter( const string &param_value)
{
   p_ffParameters.push_back( param_value);
}

/**
 * Retrieves the index of this force field item's source.
 *
 * @return index of this instance's source
 *
 */
int FFItem::getSourceID( void) const
{
   return p_sourceID;
}

/**
 * Retrieves the number of parameters in the parameter vector.
 *
 * @return number of parameters
 *
 */
int FFItem::getParamCount( void) const
{
   return p_ffParameters.size();
}

/**
 * Retrieves a parameter specified by the input parameter index.
 *
 * @param paramID index of parameter in parameter vector (params)
 *
 * @return parameter value specified by given index
 *
 * @throw IndexOutOfRangeException
 *
 */
const string& FFItem::getParam( const int paramID) const 
                                      throw (IndexOutOfRangeException)
{
  if ( paramID < 0 || paramID >= p_ffParameters.size()) {
     throw IndexOutOfRangeException( 0, p_ffParameters.size(), paramID, WHERE);
  }
  return p_ffParameters[paramID];
}

/**
 * Retrieves the vector of parameter values.
 *
 * @return vector of parameter values
 *
 */
const vector<string>& FFItem::getParams() const
{
   return p_ffParameters;
}

/**
 * Retrieve the annotation
 *
 * @return annotation for this force field item as provided by its source 
 *
 */
const string&  FFItem::getAnnotation( void) const
{
   return p_annotation;
}

/**
 *  Remove this force field item by clearing all its parameter values and
 *  annotation.  Reset the source index to invalid value. Force field items, 
 *  once removed from an FFType, are not re-used, but remain in place in
 *  the parent category's vector of FFItems to maintain the integrity of
 *  indices of other FFItems.  Clearing the parameters markes the FFItem
 *  as "removed".
 */
void FFItem::remove( void) 
{
   p_sourceID = INVALID_ID;
   p_annotation = "";
   p_ffParameters.clear();
}

/**
 *  Tests if this force field item has been removed (contents emptied).
 *
 * @return true if item has been removed, false otherwise.
 *
 */
bool FFItem::isRemoved( void)  const {
  bool removed = false;
  if ( p_sourceID == INVALID_ID)  {
     removed = true;
  }
  return removed;
}

#ifdef DEBUG_ONLY
////////////////////////////////////////////////////////////////////////////////
// Description: Prints FFItem values to standard out
//
////////////////////////////////////////////////////////////////////////////////
void FFItem::print( void)  const
{
   cout << "####### ITEM #######" << endl;
   cout << "source index is " << p_sourceID << endl;

   if ( p_annotation.empty())
      cout << "No annotation" << endl;
   else
      cout << "Annotation is " << p_annotation.c_str() << endl;

   cout << p_ffParameters.size() << " parameters:" << endl;
   if ( p_ffParameters.size() > 0)  {
      cout << "       ";
      cout << p_ffParameters[0].c_str();
      for ( int i=1;i<p_ffParameters.size();i++)
         cout << ",  " << p_ffParameters[i].c_str();
      cout << endl;
   }
}
#endif
