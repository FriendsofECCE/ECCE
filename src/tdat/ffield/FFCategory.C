////////////////////////////////////////////////////////////////////////////////
// FFCategory Class
//
//
// DESIGN: The Force Field Category class contains the information and methods
//         for handling force field categories, for example, atoms, bonds, 
//         angles, torsions, etc. Each category contains FFTypes with 
//         associated FFItems.  An example atom type would be HC; this type 
//         might have several associated items - each from a differenc source
//         (.par) file. All the FFItems in a category are assumed to 
//         have the same parameter count, formula, and parameter labels.
//         Therefore, the formula and parameter labels are tracked by the 
//         FFCategory.  The FFCategory tracks all the FFTypes which are mapped 
//         by the type name or ID. The FFCategory also tracks all the FFItems 
//         associated with its member FFTypes.  For convenience, the FFItems 
//         are stored in an array of all the categories FFItems.  Each type 
//         tracks its associated FFItems by their index in the category's array
//         of FFItems. Removed items will be eliminated from the list of items 
//         tracked by its FFType.  In the categories array of FFItems, the 
//         source id of a removed item will be changed to -1.
//
////////////////////////////////////////////////////////////////////////////////

#include <iostream>
  using std::cout;
  using std::endl;

#include "util/EcceException.H"
#include "util/BadValueException.H"
#include "util/NullPointerException.H"
#include "util/ErrMsg.H"
#include "util/STLUtil.H"

#include "tdat/FFCategory.H"

const int   FFCategory::INVALID_INDEX = -1;

typedef map< int, int, less<int> >::value_type intIntMapPair;

//-------------------Constructor------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
// Description: Constructs empty FFCategory object
//
////////////////////////////////////////////////////////////////////////////////
FFCategory::FFCategory( string& name)
   : p_name( name)
{
   p_ffTypes.clear();  // Vector for storing pointers to force field types 
                       //   (FFType*) in this category.  All types for this 
                       //   category are stored here in the order they are 
                       //   parsed from the source files.
   p_ffTypeNames.clear();
   p_ffItems.clear();  // Vector for storing pointers to all force field items
                       //   (FFItem*) in this category.  These are the line 
                       //   items from the source .par files.  Each FFItem is 
                       //   associated with an FFType.  A given FFType may have
                       //   up to n different items where n is the number of
                       //   source .par files.  Each FFType maintins a list
                       //    of indices in this vector for its associated 
                       //    FFItems.  If a FFItem is "removed" - only in the
                       //    case of a new item - the FFItem* is nulled so as
                       //    not to perturb the indexing of subsequent FFItem*s.
}

//-------------------Destructor-------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
// Description: Deallocates dynamically allocated space.
//
////////////////////////////////////////////////////////////////////////////////
FFCategory::~FFCategory( void)
{
  clear();
}

//-------------------Public Methods---------------------------------------------
////////////////////////////////////////////////////////////////////////////////
// Description:  Returns the index of the source of the force field item
//               specified by the argument.  Maps the item index to its source
//               index.
//
// Param: idx is the index of a line item from a parameter (.par) file.
//
// Return: the index of the source file for the specified item.  If the item
//         index is invalid, the return source index is also invalid.
//
////////////////////////////////////////////////////////////////////////////////
int FFCategory::getItemSourceID( int itemID) const  throw(std::range_error)
{
   int srcID = INVALID_INDEX;
   if ( isValidItemID( itemID))
      srcID = p_ffItems[itemID]->getSourceID();
   else
      throw std::range_error(
                         "Invalid item index in FFCategory::getItemSourceID()");
   return srcID;
}

////////////////////////////////////////////////////////////////////////////////
// Description: Captures the given type's item's source indices
//              this force field category.
//
// Param: typeID index into vector of FFTypes
// Param: ids    empty vector for returning source indices
//
// Return: none.
//
//////////////////////////////////////////////////////////////////////////////
void FFCategory::getTypeSourceIDs( int typeID, vector<int>& ids)  const
{
   if ( isValidTypeID( typeID))  {
      FFType *fftype = p_ffTypes[typeID];
      if ( fftype != (FFType*)0)  {
         int itemID = FFType::INVALID_ID;
         for ( int i=0;i<fftype->getItemCount();i++)  {
            itemID =  fftype->getItemID( i);
            if ( isValidItemID( itemID))
               ids.push_back( p_ffItems[itemID]->getSourceID());
         }
      }
      else
         cerr << "FFCategory::getTypeSourceIDs(" << typeID << " returned null pointer" << endl;
   }
}

////////////////////////////////////////////////////////////////////////////////
// Description: Returns the number of force field types associated with
//              this force field category.
//
// Param: none.
//
// Return: A count of the item types for this category.
//
//////////////////////////////////////////////////////////////////////////////
int FFCategory::getTypeCount( void)  const
{
   BADVALUEEXCEPTION( p_ffTypes.size() == p_ffTypeNames.size(), 
         "FFCategory::getTypeCount() types size mismatch");

   return p_ffTypes.size();
}

////////////////////////////////////////////////////////////////////////////////
// Description: Retrieves, for this category, lists of the type name and pointer
//              to the superceeding item from the given sources.  There are 
//              actually two parallel lists: one for the names and one for the 
//              item pointers.  The lists are ordered and correspond; for 
//              example, the second name is associated with the second item
//              pointer.  In the case of mulitple sources for a type name, only
//              the item from the superceeding source is listed.
//
// Param: srcIDs vector of applicable source indices (0 = first source index)
// Param: names  names of types of this category from one or more of the sources
// Param: items  pointer to the superceeding item associated with the name above
//
// Return: none
//
////////////////////////////////////////////////////////////////////////////////
void FFCategory::getFFItems( vector<int>& srcIDs, vector<string>& names, 
                                                      vector<FFItem*>& items)  
{
   FFItem *ffitem = (FFItem*)0;
   int itemID = -1;
   int i;

   // go through all the FFTypes
   vector<FFType*>::iterator titer = p_ffTypes.begin();
   while ( titer != p_ffTypes.end())  {

      // If the FFType has some items, go through all the items until find an
      //      FFItem with a source id that matches an id in the srcIDs list.
      // The item list must be processed from the back (most superceeding src)
      for ( i=(*titer)->getItemCount() -1;i>=0;i--)  {

         itemID = (*titer)->getItemID( i);  // trust range of i
         if ( isValidItemID( itemID)) {

            ffitem = p_ffItems[itemID];
            NULLPOINTEREXCEPTION(ffitem!=0,
                      "FFCategory::getFFItems() found null FFItem pointer");

            // test if this item has a source id matching one of the srcIDs
            if ( find( srcIDs.begin(), srcIDs.end(), ffitem->getSourceID()) !=
                          srcIDs.end())  {
               // when a match, save the FFType ptr and the item ptr and break
               names.push_back( (*titer)->getName());
               items.push_back( ffitem);
               break;   // go to next FFType
            }
         }  // ignore invalide item ids
      } // end of foreach item of the current type
      titer++;
   } // end of while more FFTypes
}

////////////////////////////////////////////////////////////////////////////////
// Description: Retrieves a pointer to the FFItem instance specified by the
//                type and source indices.  If the retrieval fails, a null
//                pointer is returned.
////////////////////////////////////////////////////////////////////////////////
const FFItem* FFCategory::getFFItem( int typeID, int idx) 
                                                         throw(std::range_error)
{
   FFItem* retVal = 0;

   int itemID = getTypeItemID( typeID, idx); // possibe range exception
   if ( isValidItemID( itemID))  {
      retVal = p_ffItems[ itemID];
   }
   else
      throw std::range_error("Invalid index range in FFCategory::getFFItem()");

   return retVal;
}
   

////////////////////////////////////////////////////////////////////////////////
// Description: Returns a pointer to a string containing the name of the force
//              field type indexed by the argument.
//
// Param: typeID is the index of an item type for this force field category.
//
// Return: a pointer to the name of the specified type object. If the type 
//         index is invalid, a null pointer is returned.
//
////////////////////////////////////////////////////////////////////////////////
string FFCategory::getTypeName( int typeID)  const throw( std::range_error)
{
   string name;
   if ( isValidTypeID( typeID))
      name =  p_ffTypes[typeID]->getName();
   else
      throw std::range_error("Invalid type index in FFCategory::getTypeName()");
   return name;
}

////////////////////////////////////////////////////////////////////////////////
// Description: Calls the specified type to load the atomic components as 
//              given in the type name as individual strings in the input 
//              vector.  The strings must be deleted by the caller.
//
// Param: typeID specifies the type of this force field category.
// Param: components is an empty vector which is filled with the atomic
//                   components of the type name.  Possible problems:
//                   1. If the catID is invalid, the vector is unchanged.
//                   2. If the type id is invalid and the type name is not null,
//                      the vector is loaded with the components from the name.
//                   3. If the type id is invalid and the type name is null,
//                      the vector is loaded with the correct number of blank
//                      name components (empty strings) for the category.
//
////////////////////////////////////////////////////////////////////////////////
void FFCategory::getTypeNameComponents( const string& typeName,
                                       vector<string>& components, int numAtoms)
{
   parseTypeName( typeName, components, numAtoms);
}

///////////////////////////////////////////////////////////////////////////////
// Description: Parses the type name into its atom name components including
//              the empty names - all in the correct order as presented in
//              the tyep name where atom names are separated by dashes and
//              atom names may be empty (acting as wildcards).

//              Loads the input vector with the atomic components of the
//              type name.  For example, a bond named "CA-C*" would result
//              in the vector with two elements, "CA" and "C*".  Whereas
//              names with unspecified components have no character
//              representation, the return vector must have an empty string
//              for unspecified components.  For example, a torsion named
//              "--CA-" has 3 unspecified components.  The resulting vector
//              would have four elements, "", "", "CA", "".  These empty
//              strings are necessary cues to the calling entity.
////////////////////////////////////////////////////////////////////////////////
void FFCategory::parseTypeName( const string& typeName, vector<string>& ret)
{
   ret.clear();

   string name = typeName;
   STLUtil::stripLeadingAndTrailingWhiteSpace(name);

   string empty;

   if ( !name.empty())  {
      int size = name.size();
      int sidx = 0;
      int pos = 0;

      while (sidx < size && pos != string::npos) {
         pos = name.find("-", sidx);
         if (pos == sidx) {
            ret.push_back(empty);
            sidx = pos + 1;
         } else if (pos != string::npos) {
            ret.push_back( name.substr(sidx, pos-sidx) );
            sidx = pos + 1;
         }
      }
      // Now handle the case where we ran out of -.  We need to either push
      // and empty string if the cur index == '-' or the remaining characters.
      if ( sidx < name.size())  {
         ret.push_back( name.substr(sidx) );
      }
      else if ( sidx == name.size())  {
         ret.push_back( empty);
      }
   }
}



////////////////////////////////////////////////////////////////////////////////
// Description: Parse type names and validate results size.
//
// Param: typeName is the name to be parsed
// Param: components is a reverence to a empty vector which is filled by this
//                   method with strings for each component of this type from
//                   type name.  If the type name is null, the vector remains
//                   empty.

////////////////////////////////////////////////////////////////////////////////
void FFCategory::parseTypeName( const string& typeName, 
                                vector<string>& components, int numAtoms)
{
   if ( !typeName.empty() ) {
      parseTypeName(typeName, components);

      BADVALUEEXCEPTION( components.size() == numAtoms, 
            "FFCategory::parseTypeName() found incorrect number of components");
   }
   else  {  // name was empty, create vector with correct number of empties
      string empty;
      components.resize( numAtoms, empty );
   }
}




////////////////////////////////////////////////////////////////////////////////
// Description: Returns a pointer to a string containing the name of the force
//              field type indexed by the argument. Also returns with the
//              flag set to indicate source information.
//
// Param: typeID is the index of an item type for this force field category.
// Param: srcFlag indicates either that a new parameter set has been defined for
//                the specified type or the number of source files defining
//                this type.
//
// Return: a pointer to the name of the specified type object. If the type 
//         index is invalid, a null pointer is returned.
//
////////////////////////////////////////////////////////////////////////////////
string FFCategory::getTypeNameAndSrcStatus( int typeID, 
                                        int newSrcID, int* srcFlag)  const
{
   string name;
   if ( isValidTypeID( typeID))  {
      FFType *fftype = p_ffTypes[typeID];
      name =  fftype->getName();
      if ( srcFlag != NULL)
         *srcFlag = getTypeSrcStatus( fftype, newSrcID);
   }
   return name;
}

////////////////////////////////////////////////////////////////////////////////
// Description: Returns a flag set to indicate source information for the
//              indicated type of this category.
//
// Param: fftype is an index to a type for this force field category.
// Param: newSrcID source index for new source
//
// Return: source status for specified type of this force field category.  If 
//         the type index is invalid, an invalid status is returned.
//
////////////////////////////////////////////////////////////////////////////////
FFType::TYPE_SRC_STATUS FFCategory::getTypeSrcStatus( int typeID, int newSrcID)  const
{
   FFType::TYPE_SRC_STATUS stat = FFType::NO_ASSIGNED;
   if ( isValidTypeID( typeID))  {
      stat = getTypeSrcStatus( p_ffTypes[typeID], newSrcID);
   }
   return stat;
}
////////////////////////////////////////////////////////////////////////////////
// Description: Returns a flag set to indicate source information for the
//              indicated type of this category.
//
// Param: fftype   a pointer to a type for this force field category.
// Param: newSrcID source index for new source
//
// Return: source status for input type of this force field category.  If 
//         the type index is invalid, an invalid status is returned.
//
////////////////////////////////////////////////////////////////////////////////
FFType::TYPE_SRC_STATUS FFCategory::getTypeSrcStatus( FFType* fftype, int newSrcID)  const
{
   FFType::TYPE_SRC_STATUS stat = FFType::INVALID_ASSIGNED;

   if ( fftype != (FFType*)0)  {    // continue only if fftype is not null
      int lastItemID = INVALID_INDEX;
      try  {
         lastItemID = fftype->getLastItemID();
      }
      catch (InvalidException e)  {  // ignore exception in this case
      }
      if ( isValidItemID( lastItemID))  {     // fftype had not items, pass
         if ( p_ffItems[lastItemID]->getSourceID() == newSrcID)
            stat = FFType::NEWLY_ASSIGNED;
         else if ( fftype->getItemCount() > 1)
            stat = FFType::MULTI_ASSIGNED;
         else if  ( fftype->getItemCount() == 1)
            stat = FFType::SINGLE_ASSIGNED;
         else {
            EE_ASSERT( 1, EE_FATAL, 
        "Unrecognized status for type sources, FFCategory::getTypeSrcStatus()");
         }
      }
   }
   return stat;
}

/**
 *  Retrieves the atomic number of the current (most recently defined) item
 *  of the specified force field Atom type.
 *
 *  @param typeID is the index of a force field type in the Atom category
 *
 *  @return atomic number of current item of the indexed force field type.
 *
 *  @throw InvalidException if this is not the Atom category or if the
 *  specified FFType has no defined items.
 *  @throw IndexOutOfRangeException if either the FFType index is out of range
 *  or the current item index is out of range.
 */
int FFCategory::getAtomicNo( int typeID)  throw (InvalidException, 
                                                 IndexOutOfRangeException)
{
   //  test if this is category contains Atoms
   if ( getCategoryType() != ATOM_TYPE)
      throw InvalidException("Failed request for atomic number (non-Atom ffcategory): "
                                  + getName(), WHERE);

   // retrieve the index of the last item added to the specified FFType    
   int itemID = 0;
   try  {
      itemID = getType( typeID)->getLastItemID();
   }
   catch (InvalidException ie)  {
      throw InvalidException("Failed request for atomic number from itemless fftype: "
                                + getType( typeID)->getName(), WHERE);
   }
   // don't catch IndexOutOfRangeException, pass it on

   // test if the returned index of the last item for this type is valid
   if ( ! isValidItemID( itemID))
      throw IndexOutOfRangeException(0, p_ffItems.size(), itemID, WHERE);

   string atomicNo = p_ffItems[itemID]->getParam( p_atomicNoID);

   return atoi(atomicNo.c_str());
}

////////////////////////////////////////////////////////////////////////////////
// Description: Returns a count of the items associated with the specified type.
//
// Param: typeID is the index of an item type for this force field category.
//
// Return: count of the items associated with this type for this force field
//         category.  If the type index is invalid, a count of 0 is returned.
//
////////////////////////////////////////////////////////////////////////////////
int FFCategory::getTypeItemCount( int typeID)  const
{
   int count = 0;
   if ( isValidTypeID( typeID))
      count =  p_ffTypes[typeID]->getItemCount();   // protected
   return count;
}

////////////////////////////////////////////////////////////////////////////////
// Description: Retrieves the index for the item specified by the type and
//              source. 
//
// Param: typeID is the index of a item type for this force field category.
// Param: idx    is the index into the type's itemID vector
//
// Return: index of the specified force field item. If the input indices are
//         invalid, an invalid item index is returned.
//
////////////////////////////////////////////////////////////////////////////////
int FFCategory::getTypeItemID( int typeID, int idx) const 
                                                         throw(std::range_error)
{
   int itemID;                    // does not return if any indices are invalid
   if ( isValidTypeID( typeID))  {
      itemID = p_ffTypes[typeID]->getItemID( idx);
   }
   else  {
      throw std::range_error(
                          "Invalid index range in FFCategory::getTypeItemID()");
   }
      
   return itemID;
}

////////////////////////////////////////////////////////////////////////////////
// Description: Returns the index of force field type with name matching
//              input string for this category.  If no match is found and the 
//              initFlag is set to true, the name is understood to be a new 
//              type and it is added to the vector of FFType names and the new 
//              index value is returned.  If no match is found and the
//              initFlag is set to false (the default value), the value
//              FFType::INVALID_ID is returned.
//
// Param: typeName is pointer to a possible type name for this category.
// Param: initFlag indicates if this is an initial state or later.  By default
//                 it is set to False.
//
// Return: index for the named type if the type belongs to this category.  If
//         in initial state and the type name is not found, it is added.  In 
//         the non-initial state and the type name is not found, an invalid 
//         index is returned.
//
////////////////////////////////////////////////////////////////////////////////
//int FFCategory::getTypeID( const string& typeName, bool initFlag)
int FFCategory::getTypeID( string typeName, bool initFlag)
{
   int typeID = FFType::INVALID_ID;

   // check intiFlag set (==true), then this call could be used to create
   //   new type - only in this case do we want to check the alpha order of
   //   components.
   if ( initFlag == true)  {
      orderNameComponents( typeName);
   }

   map<string,int>::iterator iter = p_ffTypeNames.find( typeName);
   if ( iter != p_ffTypeNames.end())  {   // found existing FFType with typeName
      typeID = iter->second;              // grab type index
   }
   else if ( initFlag == true) {  // if this is new and the flag is set, add it
      typeID = p_ffTypes.size();
      p_ffTypeNames.insert( std::make_pair( typeName, typeID));
      p_ffTypes.push_back( new FFType( typeName));
   }
   return typeID;
}
void FFCategory::orderNameComponents( string& name)
{
  // bool retVal = false;

   vector<string> atoms;
   eCategoryTypes type = getCategoryType();

   if ( type != ATOM_TYPE)  {
      parseTypeName( name, atoms);
      string swap;
      switch (type)  {
         case CROSS_TYPE:
         case BOND_TYPE:// same treatment as for CROSS_TYPE
                        // assume no wildcards
                        // A-B; if A <= B then A-B else B-A
                        // * < A or B ; only A-* works
                        if ( atoms[0].compare( atoms[1]) > 0)  {
//cerr << "FFCategory::orderNameComponents() swapping " << name << " for ";
                           name = atoms[1] + "-" + atoms[0];
//cerr << name << endl;
                           // retVal = true;                          
                        }
                        break;
         case ANGLE_TYPE:// A-B-C; if A <= C then A-B-C else C-B-A
                        // * < A or B or C; only A-B-* works
                        // assume no wildcards
                        if ( atoms[0].compare( atoms[2]) > 0)  {
//cerr << "FFCategory::orderNameComponents() swapping " << name << " for ";
                           name = atoms[2] + "-" + atoms[1] + "-" + atoms[0];
//cerr << name << endl;
                           // retVal = true;                          
                        }
                        break;
         case PROPER_TYPE://A-B-C-D; if B<=C then A-B-C-D else D-C-B-A
                          // * < A or B or C or D
                        if ( atoms[1].compare( atoms[2]) > 0)  {
//cerr << "FFCategory::orderNameComponents() swapping " << name << " for ";
                           name = atoms[3] + "-" + atoms[2] + "-" + atoms[1]
                                           + "-" + atoms[0];
//cerr << name << endl;
                           // retVal = true;                          
                        }
                        break;
         case IMPROPER_TYPE://A-B-C-D; if A<=B then A-B-C-D else B-A-C-D
                            // * < A or B or C or D
                        if ( atoms[0].compare( atoms[1]) > 0)  {
//cerr << "FFCategory::orderNameComponents() swapping " << name << " for ";
                           name = atoms[1] + "-" + atoms[0] + "-" + atoms[2]
                                           + "-" + atoms[3];
//cerr << name << endl;
                           // retVal = true;                          
                        }
                        break;
         case ATOM_TYPE:
         case CAT_TYPE_COUNT:
         case INVALID_TYPE:
                        // catch uncaught cases
                        break;
     }
   }
   // no need to re-order atom name
}

#ifdef GET_TYPE
(const FFType)* FFCategory::getType( int typeID) throw (IndexOutOfRangeException)
{
   const FFType* type = (FFType*)0;
   if ( ! isValidTypeID( typeID))  {
      throw IndexOutOfRangeException( 0, p_ffTypes.size(), typeID, WHERE);
   }
   map<string, int>::iterator iter = p_ffTypeNames.find( typeName);
   if ( iter != p_ffTypeNames.end())  {

      EE_ASSERT( isValidTypeID( iter->second), EE_FATAL, 
                 "FFCategory::getType(), typename map with invalid type index");

      type = p_ffTypes[iter->second];
   }
   else  {
      throw std::invalid_argument("Invalid type name in FFCategory::getType()");
   }
   return type;
}
#endif

////////////////////////////////////////////////////////////////////////////////
// Description: Searches for a force field type with a name matching the
//              input name. The meaning of "matching" depends on the category.
//              As implemented, an atom, bond, or angle type name must match
//              the specified string exactly.  This assumes that the ordering
//              of component atoms in the name strings follow the same 
//              ordering convention - both in the .par file inputs and the
//              fragment inputs. Torsion and Inversion category inputs from
//              the .par files may contain "wild card" characters and the 
//              component atom ordering is more complex.
//
// Param: inName is pointer to a character string with a candidate name for 
//        a type in this category.
//
// Return: index of type with name matching the specified name. An invalid id 
//               value is returned if:
//                    1. no matching type name is found 
//                    2. the specified name is a null or empty string
//        
////////////////////////////////////////////////////////////////////////////////
int FFCategory::getMatchingTypeID( const string& inName)
{
   int typeID = FFType::INVALID_ID;
   if ( inName.size() > 0 )  {

      // count the dashes in the name.
      int count = 0;
      int i;
      for ( i=0;i< inName.size();i++)  {
         if ( inName[i] == '-')
            count++;
      }

      // process only if name has non-dash characters
      if ( count < inName.size())
      {
         if ( inName.find("Nub") == string::npos)
         {
            string rname;

            p_partialMatch.clear();

            // Inspect the name of each FFType in this FFCat. for matching name
            for ( i=0;i<p_ffTypes.size();i++)
            {
               rname = p_ffTypes[i]->getName();

               // First check for exact match 
               if (inName == rname) { 
                  // found an exact match
                  typeID = i;
                  break;
               }
               else  {                     // otherwise, check for partial match
                  savePartialMatch( i, inName, rname);
               }
            }
   
            // If no exact match was found, consider returning one of the 
            //    partial matches.
            if ( typeID == FFType::INVALID_ID)    // If no exact match, continue
            {
               if ( p_partialMatch.size() > 0) // If found partial matches, cont
               {
                 int score = 0;
                 map<int,int,less<int> >::iterator iter =p_partialMatch.begin();
                 while ( iter != p_partialMatch.end())
                 {
                    if ( (*iter).second > score)  {
                       score = (*iter).second;
                       typeID = (*iter).first;
                    }
                    iter++;
                 }
               }
            }
         }  //  string contains "Nub" - cannot be matched
      }     //  string contains only dashes 
   }
   return typeID;
}

////////////////////////////////////////////////////////////////////////////////
// Description: Registers force field type by appending it to this category's
//              vector of force field types.
//
// Param: ffType is a pointer to a FFType object to be added to the FFType 
//               vector.
// Return: none.
//
////////////////////////////////////////////////////////////////////////////////
void FFCategory::addFFType( FFType *ffType)
{
   if ( ffType != (FFType*)0)  {
      string name = ffType->getName();
      map<string,int>::iterator p = p_ffTypeNames.find( name);
      if ( p == p_ffTypeNames.end())  {   // only if name not found
         p_ffTypeNames.insert( std::make_pair( name, p_ffTypes.size()));
         p_ffTypes.push_back( ffType); 
      }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Description: Registers force field item by appending it to this category's
//              vector of force field items.  It must also register the index
//              of the item (with respect to the vector of force field items)
//              with its associated force field type which keeps a vector
//              of associated item indices. Called only by FFSources or 
//              FFModel.
//
// Param: typeID is the index of a item type for this force field category.
// Param: ffItem is a pointer to an FFItem instance.
//
////////////////////////////////////////////////////////////////////////////////
void FFCategory::addFFItem( int typeID, FFItem *ffItem)
{
   // if try to add null FFItem to type that was just added, remove that type
   if ( ffItem == (FFItem*)0)  {
     if ( typeID == p_ffTypes.size() - 1 && 
          p_ffTypes[typeID]->getItemCount() == 0) {
        removeFFType( typeID);
      }
   }
   else if ( isValidTypeID( typeID))  {
      // Check to see if an item from the same source as already been added.
      if ( newSourceForType( typeID, ffItem->getSourceID()))
      {
         // Item from this source not yet added; add this item's index in
         //   the item vector to its type's item index list
         p_ffTypes[typeID]->addItemID( p_ffItems.size());
      }
      else
      {
         // Item from this source already added; no need to add another index
         //    to the types's list of indices
         // Substitue this item for the exising item in the category's item 
         //    list; that is, remove the last item before adding this item.
         delete p_ffItems[ p_ffItems.size() -1];  // delete the last item
         p_ffItems.pop_back();                    // pop it off the vector
      }

      // append this item to the item vector
      p_ffItems.push_back( ffItem);   
   }
//else
//cerr << "FFCategory::addFFItem() is not valid typeID" << endl;
}

////////////////////////////////////////////////////////////////////////////////
//  Description:  removes FFType instance from vector of types
//
//  Param: typeID  index of FFType instance
//  Return:  true if the model has changed, false otherwise
//
////////////////////////////////////////////////////////////////////////////////
bool FFCategory::removeFFType( int typeID) throw(std::invalid_argument)  {

   bool removed = false;

   if ( isValidTypeID(  typeID))  {

      // continue only if no items
      FFType* fftype = p_ffTypes[typeID];
      if ( fftype->getItemCount() == 0)  {

         // find the name in the name map and confirm match to type vector
         string name =  fftype->getName();
         map<string,int>::iterator p = p_ffTypeNames.find(name);
         if ( p != p_ffTypeNames.end())  {
            ///////////////// following section removes type ///////////////////
            if ( p->second == typeID)  {   // match

               // if type is last added, remove and no need to rebuild name map
               if (typeID == p_ffTypes.size() -1)  {
                  delete fftype;
                  p_ffTypes.pop_back();
                  p_ffTypeNames.erase( name);
               }
               else  {
                  delete fftype;
                  for ( int i=typeID+1;i<p_ffTypes.size();i++)  {
                     p_ffTypes[i-1] = p_ffTypes[i];
                  }
                  p_ffTypes.pop_back();
                  rebuildTypeNameMap();
               }
               removed = true;
            }
            //////////// above  section removes type /////////////////////////
            else
               throw std::invalid_argument(
                                 "Mismatched indices Category::removeFFType()");
         }
         else
            throw std::invalid_argument(
                              "Cannot find type name Category::removeFFType()");
      }
      else
          throw std::invalid_argument(
                      "Cannot remove Type with items Category::removeFFType()");
   }
   else
      throw std::invalid_argument(
                      "Type index out of range for FFCategory::removeFFType()");

   return removed;
}

////////////////////////////////////////////////////////////////////////////////
//  Description: Rebuilds map of type name to type index.
//
//  Param: none
//  Return: none
//
////////////////////////////////////////////////////////////////////////////////
void FFCategory::rebuildTypeNameMap()  {
   p_ffTypeNames.clear();
   int index = 0;
   vector<FFType*>::iterator iter = p_ffTypes.begin();
   while ( iter != p_ffTypes.end())  {
      p_ffTypeNames.insert( std::make_pair( (*iter)->getName(), index++));
      iter++;
   }
}

////////////////////////////////////////////////////////////////////////////////
// Description: Registers atomforce field item.  An atom type force field 
//              should set the FFType symbolic name which the data parameter
//              indicated by the input index.
//
// Param: typeID is the index of a item type for this force field category.
// Param: ffItem is a pointer to an FFItem instance.
// Param: aNumIdx is index into FFItem parameter set of the atomic number
//
////////////////////////////////////////////////////////////////////////////////
//void FFCategory::addFFItem( int typeID, FFItem *ffItem, int aNumIdx)
//{
//    addFFItem( typeID, ffItem);
//    FFType* ffType = getType( typeID);
//    if ( ffItem != (FFItem*)0  && ffType != (FFType*)0)
//      ffType->setAtomicNumber( ffItem->getParam( aNumIdx));
//}

////////////////////////////////////////////////////////////////////////////////
// Description: Returns a pointer to the force field type indexed by the 
//              argument.
//
// Param: typeID is the index of an item type for this force field category.
//
// Return: a pointer to the specified type object. If the type index is 
//         invalid, a null pointer is returned.
//
////////////////////////////////////////////////////////////////////////////////
FFType* FFCategory::getType( int typeID)  const throw (IndexOutOfRangeException) // private method
{
   if ( ! isValidTypeID( typeID))
      throw IndexOutOfRangeException( 0, p_ffTypes.size(), typeID, WHERE);

   return p_ffTypes[typeID];
}
////////////////////////////////////////////////////////////////////////////////
// Description: Returns pointer to the named FFType instance
//
// Param: typeName possible type name for this category.
// Param: initFlag indicates if this is an initial state or later.  By default
//                 it is set to False. When set to True, if there is no
//                 existing type with this name a new one is created.
//
// Return: Pointer to the named type if the type belongs to this category.  If
//         in initial state and the type name is not found, it is added.  In 
//         the non-initial state and the type name is not found, an 
//         invalid_argument exception is thrown.
//
////////////////////////////////////////////////////////////////////////////////
FFType* FFCategory::getType( const string& typeName) throw(InvalidException) // private method
{
   map<string, int>::iterator iter = p_ffTypeNames.find( typeName);
   if ( iter == p_ffTypeNames.end())
      throw InvalidException("Invalid type name: " + typeName, WHERE);

   return p_ffTypes[iter->second];
}

////////////////////////////////////////////////////////////////////////////////
// Description:  Removes a force field item from this category for the 
//               specified type. Removing a FFItem has to be done in two places.
//               1. Remove the FFItem id from the associated FFType's item id
//                  list.  
//               2. Remove the FFItem from the category's list of FFItems.
//                  Since a FFItem is associated with only one FFType, a FFItem
//                  can be removed without concern for other references to it 
//                  from other FFTypes.  However, actually removing the FFItem 
//                  from the FFItem array would invalidate the references to 
//                  FFItems with higher indices. For this reason, the "removed"
//                  FFItems are left on the list, but are marked as removed.
//               Note that if the last item of a FFType is removed, then the
//               FFType pointer can be removed from the vector of FFType
//               pointers as long as the tables are redrawn.
//
//
// Param: typeName reference to string of type name
// Param: idx      index that specifies the force field item in the named type
//
// Return: true if vector of FFTypes changed; false otherwise
//
////////////////////////////////////////////////////////////////////////////////
bool FFCategory::removeFFItem( const string& typeName, int idx) 
                                                   throw(std::invalid_argument)
{
   bool bVal = false;

   // retrieve type
   FFType *fftype = getType( typeName);  // could throw invalid_argument

   // remove item's id from the fftype's list of item ids and clear ffitem 
   try  {
      int ffitemID = fftype->getItemID( idx);
      fftype->removeItemID( idx);
      p_ffItems[ffitemID]->remove();
      bVal = true;      // succeeded, set return value to true
   }
   catch( IndexOutOfRangeException e)  {
     throw InvalidException("Failed attempt to remove FFItem from FFType: "
                                               + typeName, WHERE);
   }
   // if this was the only item, remove the type and return flag
   if ( fftype->getItemCount() == 0)  {
      map<string,int>::iterator iter = p_ffTypeNames.find( typeName);
      bVal = removeFFType( iter->second);
   }
   return bVal;
}

////////////////////////////////////////////////////////////////////////////////
//  Description:  adds FFItem instance
//
//  Param: typeName name of type
//  Param: ffItem   pointer to FFItem instance
//
////////////////////////////////////////////////////////////////////////////////
void FFCategory::addFFItem( const string& typeName, FFItem *ffItem)
{
   if ( ffItem != (FFItem*)0)  {
      addFFItem( getTypeID( typeName, true), ffItem);
   }
}

/** 
 *  Registers the index of force field parameter with the atomic number;
 *
 *  @param atomicNoID index of ffitem parameter containing atomic number.
 *
 */
void FFCategory::setAtomicNoID( int atomicNoID)
{
   p_atomicNoID = atomicNoID;
}

////////////////////////////////////////////////////////////////////////////////
// Description: Prints force field category elements to standard out.
//
// Param: none.
// Return: none.
//
////////////////////////////////////////////////////////////////////////////////
void FFCategory::print( void) const {
 
   cout << "####### FFCATEGORY #######" << endl;
   cout << "   " << p_ffTypes.size() << " types have been identified." << endl;
   cout << "   " << p_ffItems.size() << " items have been identified." << endl;
}

////////////////////////////////////////////////////////////////////////////////
//------------------ Private Methods -------------------------------------------

////////////////////////////////////////////////////////////////////////////////
// Description: Returns the index for this force field category as defined
//              in eCategoryTypes in FFCategory.H.
//
// Param: none.
//
// Return: this category's index for the force field.   If the category
//              name has not been set, or the name set is not a recognized
//              category name for this force field, an INVALID_INDEX value is
//              returned.
//
////////////////////////////////////////////////////////////////////////////////
string  FFCategory::getName( void) const
{
   return p_name;
}

////////////////////////////////////////////////////////////////////////////////
// Description: Based on this category, look for partial matches.  If found,
//              store both the index of the partial match and a score that
//              indicates the degree of match.  A higher score indicates a
//              higher degree of matching.
//
// Param: id is the index of the type under consideration.
// Param: inName is the candidate name we are trying to match.
// Param: rname is the name of the type under consideration.
//
// Return: none.
//
////////////////////////////////////////////////////////////////////////////////
void FFCategory::savePartialMatch(  int id, const string& inName, string& rname)
{
   switch ( getCategoryType())  // Returns enumerator value matching 
   {                             //   FFCategoryName and eCategoryTypes.
      case PROPER_TYPE: // Test for partial match for torsion type if string
         //    is not all dashes.
         // ASSUMES that if len=3, then all dashes
         if ( inName.size() != 3)
         {
            // Dashes at both start and end means two wild cards,
            if ( rname[0] == '-' && 
                  rname[rname.size() -1 ] == '-')  {
               // Check to see if rname is substring of inName.
               // Score for match to 2 wild cards is lowest score
               if (inName.find(rname) != string::npos) {
                  p_partialMatch.insert( intIntMapPair( id, 1)); 
               }
            }
            // Dash at start or end indicates a wild card. 
            else if (  rname[0] == '-' ||
                  rname[ rname.size() -1 ] == '-')  {
               // Check to see if rname is substring of inName.
               // Score for match to 1 wild card is higher (2)
               if ( inName.find(rname) != string::npos) {
                  p_partialMatch.insert( intIntMapPair( id, 2)); 
               }
            }
         }   // end of if not all dashes
         break;
      case IMPROPER_TYPE: // Test for partial match for improper type if inName
         {
            vector<string> bparts;
            parseTypeName(inName, bparts);

            if (bparts.size() != 4 ) {
               throw EcceException("bad parsing",__FILE__, __LINE__);
            }

            if (bparts[0].empty() || bparts[1].empty() ||
                  bparts[2].empty() || bparts[3].empty()) {
               // we decied its reasonable to declar that this
               // doesn't match
            } else {

               vector<string> aparts;
               parseTypeName(rname, aparts);
            if (aparts.size() != 4 ) {
               throw EcceException("bad parsing",__FILE__, __LINE__);
            }

               if (bparts[2] == aparts[2] && 
                     bparts[3] == aparts[3]) {
                  if (aparts[0].empty() && aparts[1].empty())
                     p_partialMatch.insert( intIntMapPair( id,1));
                  else if (!aparts[0].empty() && !aparts[1].empty()) {
                     if ((aparts[0] == bparts[0] &&
                              aparts[1] == bparts[1]) ||
                           (aparts[0] == bparts[1] &&
                            aparts[1] == bparts[0])) {
                        p_partialMatch.insert( intIntMapPair( id,2));
                     }
                  } else {
                     // WARNING
                  }
               }
            }
         }
         break;
      default:         
         break;
   }
}

/**
 *  Retrieves this category's type
 *
 * @return category type
 */
FFCategory::eCategoryTypes FFCategory::getCategoryType()  {
   eCategoryTypes type = INVALID_TYPE;
   string name = getName();
   if ( name.compare( "Atoms") == 0)         type = ATOM_TYPE;
   else if ( name.compare( "Cross") == 0)    type = CROSS_TYPE;
   else if ( name.compare( "Bonds") == 0)    type = BOND_TYPE;
   else if ( name.compare( "Angles") == 0)   type = ANGLE_TYPE;
   else if ( name.compare( "Proper") == 0)   type = PROPER_TYPE;
   else if ( name.compare( "Improper") == 0) type = IMPROPER_TYPE;
   return type;
}

////////////////////////////////////////////////////////////////////////////////
// Description: Indicates if the end of one string matches another string.
//
// Param: inName is the string to be tested.
// Param: r      is the string looked for at the end of inName.
//
// Return: True if the string r is found at the end of string inName, False
//         otherwise.
//
////////////////////////////////////////////////////////////////////////////////
bool FFCategory::endMatches( string& inName, string& r)
{
   bool matches = false;
   try  {
     if ( inName.substr( inName.size() - r.size()) == r)
        matches = true;
   }
   catch( std::out_of_range& e)  {
   }
   return matches;
}

////////////////////////////////////////////////////////////////////////////////
// Description: Checks for existing force field item of the specified type
//              from the specified source.  Assumes caller had verified that
//              the type index is valid. Assumes that all items from a single
//              source are added consecutively, ie, sources are processed one
//              at a time.
//
// Param: validtypeID is an index for an FFType object.
// Param: srcID is an index for a source.
//
// Return: true if the input source index does not match the source index of
//         any item already added to the input type.  false otherwise
//
////////////////////////////////////////////////////////////////////////////////
bool FFCategory::newSourceForType( int validtypeID, int srcID)
{
   bool rVal = true;

   // Get source id of last added item.  ASSUME the source ID of last added 
   //    item is the same or less than new item source id since items are
   //    added from one source at a time.
   // ASSUME typeID is valid - ie, checked by caller.

   int itemID = INVALID_INDEX;
   try  {
       itemID = p_ffTypes[validtypeID]->getLastItemID();
   }
   catch (InvalidException ie)  {   // ignore in this case
   }

   // reset rVal to false only if an item with this source alreay exists
   if ( isValidItemID( itemID) &&  srcID == p_ffItems[itemID]->getSourceID())
         rVal = false;

   return rVal;
}

////////////////////////////////////////////////////////////////////////////////
// Description: Tests if supplied index is valid index of the vector of force
//              field types belonging to this force field category.
//
// Param: typeID is a candidate FFYype index.
//
// Return: True if the candidate index is in range, False otherwise.
//
////////////////////////////////////////////////////////////////////////////////
bool FFCategory::isValidTypeID( int typeID) const
{
   return ( typeID >= 0 && typeID < p_ffTypes.size());
}

////////////////////////////////////////////////////////////////////////////////
// Description: Tests if supplied index is a valid force field item index into
//              the vector of force field items belonging to this category.
//
// Param: itemID is a candidate FFItem index.
//
// Return: True if the candidate index is valid, False otherwise.
//
////////////////////////////////////////////////////////////////////////////////
bool FFCategory::isValidItemID( int itemID) const
{
   return ( itemID >= 0 && itemID < p_ffItems.size() && 
             !p_ffItems[itemID]->isRemoved());
}

////////////////////////////////////////////////////////////////////////////////
// Description: Clear to deallocate heap only. Pointers are not set to null;
//              vectors are not cleared of pointers.  Categories are not to
//              be reused.
//
// Param: none.
// Return: none.
//
////////////////////////////////////////////////////////////////////////////////
void FFCategory::clear( void)  {

   int i;
   for ( i=0;i<p_ffTypes.size();i++)
     delete p_ffTypes[i];

   for ( i=0;i<p_ffItems.size();i++)
     delete p_ffItems[i];

   p_ffparams.reset();
}
