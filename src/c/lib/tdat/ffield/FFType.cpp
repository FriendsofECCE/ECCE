/**
 *  @file
 *
 */

#include <iostream>
  using std::cout;
  using std::endl;

#include "tdat/FFType.H"

const int FFType::INVALID_ID = -1;

/**
 *  Construct an object with the name alone.
 *
 *  @param name
 */
FFType::FFType( const string& typeName)  
{
   p_name = typeName;
}

/**
 *  Destructor clears vector of FFItem indices.
 */
FFType::~FFType( void)
{
   p_ffItemIDs.clear();
}

/**
 *   Append input index to this type's vector of FFItem indices.
 *   The input index is an index into the parent FFCategory's
 *   vector of FFItems. The indexed item contains a set of parameters
 *   that characterize this type.
 */
void FFType::addItemID( int ffitemID)
{
   p_ffItemIDs.push_back( ffitemID);
}

/**
 *  Retrieve the index of the last added FFItem
 *
 *  @return index of the last FFItem in this type's list of force field items.
 *  @throw  InvalidException if no times are currently defined for
 *  this force field type.
 *
 */
int FFType::getLastItemID( void) const  throw (InvalidException)  {

  if ( p_ffItemIDs.empty()) {
#if 0000
    // This throws exceptions for the normal case of reading a .par file
    // whenever a new item is encountered.  To avoid a huge amount of
    // developer diagnostic output, just avoid this check/throw
    throw InvalidException("Request for last item of FFType with no FFItems: "
                            + p_name, WHERE);
#else
    return FFType::INVALID_ID;
#endif
  }

  return  p_ffItemIDs[ p_ffItemIDs.size() - 1];
}

/**
 *  Remove the specified FFItem index from this type's list of force field 
 *  item indices.
 *
 *  @param index index into this type's vector of FFItem indices specifying
 *  position of FFItem index to be removed. (Not the actual FFItem index.)
 *
 *  @throw IndexOutOfRangeException if input index is not valid (in range)
 */
void FFType::removeItemID( int index)  throw(IndexOutOfRangeException)
{
   if ( index < 0 || index >= p_ffItemIDs.size()) 
      throw IndexOutOfRangeException(0, p_ffItemIDs.size(), index, WHERE);

   for ( int i=index+1;i<p_ffItemIDs.size();i++) // move indices down vector
     p_ffItemIDs[i-1] = p_ffItemIDs[i];

   p_ffItemIDs.pop_back();  // remove last item
}

////////////////////////////////////////////////////////////////////////////////
// Description:  Tests if the input string matches this type's name.
//
// Param: name candidate type name
// Return: true if this type's name matches the input name, false otherwise
//
////////////////////////////////////////////////////////////////////////////////
//bool FFType::isNamed( const string& name) const
//{
//   return p_name.compare( name) == 0;
//}

/**
 *  Retrieve the force field type name
 *
 *  @return name of this force field type
 */
const string& FFType::getName( void) const
{
   return p_name;
}

/** 
 *  Retrieve the count of the force field items (FFItems) associated 
 *  with this force field type.
 *
 *  @return count of force field items associated with this type
 */
int FFType::getItemCount( void) const
{
   return p_ffItemIDs.size();
}

/**
 *  Retrieve the specified force field item index.
 *
 *  @param index index into this type's list of FFItem indices.
 *
 *  @return the FFItem index indexed by "index".
 *
 *  @throw IndexOutOfRangeException if input "index" is out of range.
 */
int FFType::getItemID(int index) const throw(IndexOutOfRangeException)
{
   if ( index < 0 || index >= p_ffItemIDs.size()) 
      throw IndexOutOfRangeException(0, p_ffItemIDs.size(), index, WHERE);

   return p_ffItemIDs[index];
}

/**
 *  Print the current values in this type to standard out.
 */
void FFType::print() const
{
   cout << "####### TYPE #######" << endl;
   cout << "Type name is " << p_name;
   cout << ", " << p_ffItemIDs.size() << " items IDs";
   for ( int i=0;i<p_ffItemIDs.size();i++)  {
      cout << ", " << p_ffItemIDs[i];
   }
   cout << endl;
}
