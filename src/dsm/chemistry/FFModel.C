///////////////////////////////////////////////////////////////////////////////
// FFModel Class
//
//
// DESIGN: TODO
//              <Discuss overall design/purpose of class.>
//
///////////////////////////////////////////////////////////////////////////////

#include <strstream>
using std::ostrstream;

#include "util/ErrMsg.H"

#include "tdat/FFRules.H"
#include "tdat/FFDesc.H"

#include "dsm/FFModel.H"
#include "dsm/FFFactory.H"
#include "dsm/NWChemDesc.H"

FFModel* FFModel::p_instance = 0;

//-------------------Constructor (protected) --------------------
////////////////////////////////////////////////////////////////////////////////
// Description: Constructs the force field data model.
//
////////////////////////////////////////////////////////////////////////////////
FFModel::FFModel( void)
: p_newItemCount ( 0), p_newSourceID (0)
{
}
 
//-------------------Destructor (public) ------------------------
////////////////////////////////////////////////////////////////////////////////
// Description: Releases the dynamically allocated memory.
//
////////////////////////////////////////////////////////////////////////////////
FFModel::~FFModel( void)
{
   for ( int i=0;i<p_ffCategories.size();i++)
     delete p_ffCategories[i];
   delete p_ffDesc;
}


//-------------------Singlton instance() (public) ------------------------
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
FFModel* FFModel::instance()
{
   if ( p_instance == 0)  {
      p_instance = new FFModel();
   }
   return p_instance;
}

//-------------------Public Methods---------------------------------
////////////////////////////////////////////////////////////////////////////////
//  Description: Retrieves the maximum number of parameters across all 
//               category descritors - may be for a category not actually
//               present, but potentially present.
//
//  Return: number of parameters for the force field category with the most
//          parameters.
////////////////////////////////////////////////////////////////////////////////
int FFModel::getMaxParameterColumns( void)
{
   int max = 0, count = 0;
   for ( int i=0;i<p_ffDesc->size();i++)  {
     count = p_ffDesc->getCategory(i).getNumParams();
     if ( max < count)
        max = count;
   }
   return max;
}

////////////////////////////////////////////////////////////////////////////////
// Description: Returns the number of registered force field categories. (not
//              FFCategoryDesc's).
//
// Return: count of force field categories in this model
//
////////////////////////////////////////////////////////////////////////////////
int FFModel::getCategoryCount( void) const
{
   return p_ffCategories.size();
}

////////////////////////////////////////////////////////////////////////////////
// Description: Returns force field category label associated with
//              supplied category index.  If the index value is 
//              invalid, an empty string is returned.
//
// Param: catIdx is the category index that specifies the force field category
//
// Return: category label string
//
////////////////////////////////////////////////////////////////////////////////
string FFModel::getCategoryLabel( int catIdx) const
{
   string name = p_ffCategories[catIdx]->getName();
   return p_ffDesc->getCategoryByName( name).getLabel();
}

////////////////////////////////////////////////////////////////////////////////
// Description: Returns the type index for the specified category index and
//              type name.  If the category index is invalid or the type name 
//              is not found, an invalid FFType index value is returned.
//
// Param: catIdx is the category index that specifies the force field category
// Param: name is a pointer to a string containing a type name
//
// Return: type index that specified by the category index and type name
//
////////////////////////////////////////////////////////////////////////////////
int FFModel::mapTypeNameToID( int catID, const string& name) const
{
   int typeID = FFType::INVALID_ID;
   if ( isValidCategoryIndex( catID))
      typeID = p_ffCategories[catID]->getMatchingTypeID( name);
   return typeID;
}

////////////////////////////////////////////////////////////////////////////////
// Description: Returns the FFDesc associated with the model
////////////////////////////////////////////////////////////////////////////////
const FFDesc& FFModel::getDesc()
{
   return *p_ffDesc;
}

////////////////////////////////////////////////////////////////////////////////
// Description: Calls the specified category to load the atomic components as 
//              given in the specified type name as individual strings in the 
//              input vector.  The strings must be deleted by the caller.
//
// Param: catID      specifies the force field category.
// Param: typeName   specifies the type of this force field category.
// Param: components is an empty vector which is filled with the atomic
//                   components of the type name.  Possible problems:
//                 1. If the catID is invalid, a range_error exception is thrown
//                 2. If the type name is empty, the correct number of empty
//                    components is loaded into the vector.
//                 3. If the type name is invalid,
//                      the vector is loaded with the correct number of blank
//                      name components (empty strings) for the category.
//
//////////////////////////////////////////////////////////////////////////////
void FFModel::getTypeNameComponents( int catID, const string& typeName, 
                                     vector<string>& components) const
                                                         throw(std::range_error)
{
   int numAtoms = 0;
   if ( isValidCategoryIndex( catID))  {

      numAtoms = p_ffDesc->getCategoryByName(p_ffCatNames[catID]).getNumAtoms();

      if ( p_ffCategories.empty())  {
         string empty;
         for ( int i=0;i<numAtoms;i++)
            components.push_back( empty);
      }
      else  {
         p_ffCategories[catID]->getTypeNameComponents( typeName, components, 
                                                                      numAtoms);
      }
   }
   else
      throw std::range_error(
            "Invalid category index range in FFModel::getTypeNameComponents()");
}

////////////////////////////////////////////////////////////////////////////////
// Description: Returns the source status for the specified type of the
//              specified category.
//
// Param: catIdx is the category index that specifies the force field category
// Param: typeName is the name that specifies the type within the category
//
// Return: source status of the specified type.  If the category index is 
//         invalid or the name is not matched to a valid type index, an invalid
//          status is returned.
//              
////////////////////////////////////////////////////////////////////////////////
int FFModel::getTypeSrcStatus( int catIdx, const string& typeName)
{
   int typeID = mapTypeNameToID( catIdx, typeName);
   return p_ffCategories[catIdx]->getTypeSrcStatus( typeID, p_newSourceID);
}

////////////////////////////////////////////////////////////////////////////////
// Description: Returns the type name for the specified category and type 
//              indices.  If the index for either the category or type is
//              invalid, a null pointer is returned.
//              
// Param: catIdx is the category index that specifies the force field category
// Param: typeIdx is the type index that specified the type within the category
// Param: srcFlag is pointer to a source type indicator set by this method
//
// Return: name of type specified by type and category indices
//              
////////////////////////////////////////////////////////////////////////////////
string FFModel::mapTypeIDToName( int catIdx, int typeIdx, int* srcFlag)
{
   string name;
   if ( isValidCategoryIndex( catIdx))
      name = p_ffCategories[catIdx]->getTypeNameAndSrcStatus( typeIdx, 
                                                    p_newSourceID, srcFlag);
   return name;
}
string FFModel::mapTypeIDToName( int catIdx, int typeIdx)
{
   string name;
   if ( isValidCategoryIndex( catIdx))  {
      try  {
         name = p_ffCategories[catIdx]->getTypeName( typeIdx);
      }
      catch(std::range_error& )  {
      }
   }
   return name;
}

/**
 * Retrieves the atomic number for the current item of the  specified type 
 * in the Atoms force field category.
 *
 * @param typeID index specifying an atom type
 *
 * @return: atomic number of current (or latest) item of the type specified or
 *          zero if no Atoms force field has been defined.
 *
 */
int FFModel::getAtomicNo(  int typeID) const 
{
   int atomicNo = 0;
   for ( int i=0;i<p_ffCategories.size();i++)  {
      if ( p_ffCategories[i]->getCategoryType() == FFCategory::ATOM_TYPE)  {
         atomicNo = p_ffCategories[i]->getAtomicNo( typeID);
      }
   }

   return atomicNo;
}

////////////////////////////////////////////////////////////////////////////////
// Description: Retrieves the number of types for the specified force field
//              category.
//
// Params: catIdx is an index specifying a force field category, e.g., Atom
//
// Return: Count of the types of the specified force field category.  If the
//         category index is invalid, a count of zero is returned.
//
////////////////////////////////////////////////////////////////////////////////
int FFModel::getTypeCount( int catIdx)  const
{
   int count = 0;
   if ( isValidCategoryIndex( catIdx))
      count = p_ffCategories[catIdx]->getTypeCount();
   return count;
}

////////////////////////////////////////////////////////////////////////////////
// Description: Retrieve a count of the items for the specified force field
//              type for the specified category.
//
// Params: catIdx is an index specifying a force field category, e.g., Atom
//         typeIdx is an index specifying a type for the given category.
//
// Return: count of the number of items for the specified category and type.
//         A count of 0 is returned if either the catIdx or typeIdx is invalid.
//
////////////////////////////////////////////////////////////////////////////////
int FFModel::getTypeItemCount( int catIdx, int typeIdx)  const
{
   int count = 0;
   if ( isValidCategoryIndex( catIdx))
      count = p_ffCategories[catIdx]->getTypeItemCount( typeIdx);
   return count;
}

////////////////////////////////////////////////////////////////////////////////
// Description: Returns string with name of source specified by the 
//              argument indices for force field category, type, and item.  If
//              one or more of the category, type, or item indices are invalid,
//              an empty string is returned.
//
// Param: catID  is an index of force field categories.
// Param: typeID is an index of force field types specific to the indicated
//               category.
// Param: idx    is the index of force field items given for the specific
//               category and type.
//
////////////////////////////////////////////////////////////////////////////////
string  FFModel::getSourceName( int catID, int typeID, int idx) const
{
   string name;
   if ( isValidCategoryIndex( catID))  {

      // Retrieve the item index relative to all the items in a category.
      try  {
         int itemID = p_ffCategories[catID]->getTypeItemID( typeID, idx);
         int srcID = p_ffCategories[catID]->getItemSourceID( itemID);
         name = p_ffSources.getSourceName( srcID);
      }
      catch( std::range_error&  re)  { // ignore and return empty name
      }
   }
   return name;
}

////////////////////////////////////////////////////////////////////////////////
// Description: Collects all the source names (in order of input) for the
//              type of the category indicated by the indices.
//
// Param: catID   index of category
// Param: typeID  index of type in the indicated category
// Param: sources reference to empty vector for returning the source names.
//
////////////////////////////////////////////////////////////////////////////////
void  FFModel::getTypeSources( int catID, int typeID, vector<string>& sources) const
{
   if ( isValidCategoryIndex( catID))  {

      try  {
        vector<int> sourceIDs;
        p_ffCategories[catID]->getTypeSourceIDs( typeID, sourceIDs);
        for ( int i=0;i<sourceIDs.size();i++)  {
           sources.push_back(p_ffSources.getSourceName( sourceIDs[i]));
        }
      }
      catch( std::range_error&  re)  { // ignore and return empty name
      }
   }
}

////////////////////////////////////////////////////////////////////////////////
// Description: Loads source names for this model into supplied vector.
//
// Param: names reference to vector to be loaded
//
////////////////////////////////////////////////////////////////////////////////
void FFModel::getSourceNames( vector<string>& names) const  {
   names.clear();
   int i = 0;
   while ( i < p_ffSources.getSourceCount()) 
      names.push_back( p_ffSources.getSourceName( i++));
   if ( p_newItemCount > 0)
      names.push_back( p_ffSources.getNewSourceName()); // get the new label
}

////////////////////////////////////////////////////////////////////////////////
// Description: Returns the parameters specified by the category identifier,
//              type identifier and source identifier.
//
// Param: catIdx is the force field category identifier (index).
//                field category and type.
//
// Return: pointer to a parameter set object which contains the specified
//         parameter labels and values.
//
////////////////////////////////////////////////////////////////////////////////
const FFCategoryDesc& FFModel::getCatDesc( int catIdx) const 
                                                        throw(std::range_error)
{
   if ( catIdx < 0 || catIdx >= p_ffCatNames.size())
      throw std::range_error("Illegal range in FFModel::getCatDesc()");

   return p_ffDesc->getCategoryByName( p_ffCatNames[catIdx]);
}

////////////////////////////////////////////////////////////////////////////////
// Description: Ensures that the order of the FFCategories matches the FFModel's
//              enumerated types (e_ffCategories) based on the original order
//              of categories in the amber.par files.  Creates a map 
//              (p_ffCatNames) between the category index used by FFParams and 
//              the category name which is used to access FFCategoryDescs from 
//              FFDesc.
//
// Param:  none
// Throws: range_error if the hard-coded category name within does not match
//                     the name of one of the FFCategoryDescs.
// Return: none
//
////////////////////////////////////////////////////////////////////////////////
void FFModel::initializeFFCategories() throw (std::range_error) {

   p_ffCatNames.clear();
   p_ffCategories.clear();

   // Names used in amber.par file and in order of enumerated classes 
   //   FFCategory::eCategoryTypes (and amber.par file)
   // Check that all named FFCategories have matching (by name) FFCategoryDesc

   string name = "Atoms";
   try  {
      FFCategoryDesc ffcdesc = p_ffDesc->getCategoryByName( name); 
      p_ffCatNames.push_back( name);
      p_ffCategories.push_back( new FFCategory( name));
      int id = ffcdesc.getParamIDByLabel("Atomic No.");
      p_ffCategories[p_ffCategories.size()-1]->setAtomicNoID( id);

      name = "Cross";
      p_ffDesc->getCategoryByName( name); 
      p_ffCatNames.push_back( name);
      p_ffCategories.push_back( new FFCategory( name));

      name = "Bonds";
      p_ffDesc->getCategoryByName( name); 
      p_ffCatNames.push_back( name);
      p_ffCategories.push_back( new FFCategory( name));

      name = "Angles";
      p_ffDesc->getCategoryByName( name); 
      p_ffCatNames.push_back( name);
      p_ffCategories.push_back( new FFCategory( name));

      name = "Proper";
      p_ffDesc->getCategoryByName( name); 
      p_ffCatNames.push_back( name);
      p_ffCategories.push_back( new FFCategory( name));

      name = "Improper";
      p_ffDesc->getCategoryByName( name); 
      p_ffCatNames.push_back( name);
      p_ffCategories.push_back( new FFCategory( name));
   }
   catch( std::range_error&  re)  {
      cerr << " FFModel::initializeNameMap() failed to retrive category by name from FFDesc: " << name << endl;
      throw re;
   }
   catch( std::exception& e)  {
      EE_ASSERT( false, EE_FATAL, string("FFModel::initializeNameMap() failed on ") + name + ": " + e.what());
   }
}

////////////////////////////////////////////////////////////////////////////////
// Description: Retrieves a list of the FFType names and pointers to the 
//              associated superceeding FFItem specified by the force field
//              category index and the list of source indices.
//
// Param: catIdx index of target force field category; must be valid index
// Param: srcIDs indices of target force field sources; cannot be empty list
// Param: names  reference to vector of FFType names.
// Param: items  reference to vector of FFItem pointers.
//
////////////////////////////////////////////////////////////////////////////////
void FFModel::getFFItems( int catIdx, vector<int> srcIDs, 
                          vector<string>& names, vector<FFItem*>& items) const
{
   EE_ASSERT( isValidCategoryIndex( catIdx), EE_FATAL,
              "FFModel::getFFItems() fed bad category index");
   EE_ASSERT( srcIDs.size() > 0, EE_FATAL,
              "FFModel::getFFItems() fed empty srcIDs vector");

   // clear the vectors before populating them
   names.clear();
   items.clear();

   // sort the src indices.  Assume the larger the index, the highest level
   sort( srcIDs.begin(), srcIDs.end()); 

   p_ffCategories[catIdx]->getFFItems( srcIDs, names, items);
}

////////////////////////////////////////////////////////////////////////////////
// Description: Retrieves a pointer to the FFItem specified by the arguments
//
// Param: catIdx  index of target force field category; must be valid index
// Param: typeIdx index of target force field type; must be valid index
// Param: idx     index of target force field item per the force field type
//
// Return: pointer to specified FFItem instance
// Throws: range_error exception iff catIdx is invalid
////////////////////////////////////////////////////////////////////////////////
const FFItem* FFModel::getFFItem( int catIdx, int typeIdx, int idx)  const  
                                                        throw(std::range_error)
{
   const FFItem* ffitem = NULL;

   if ( isValidCategoryIndex( catIdx))  {
      try  {
         ffitem = p_ffCategories[catIdx]->getFFItem( typeIdx, idx);
      }
      catch( std::range_error&  e)  {  //ignore and just return null pointer
      }        // if valid catID, but no item - ignore assuming new type
   }
   else
      throw std::range_error("Invalid category index in FFModel::getFFItem()");

   return ffitem;
}

////////////////////////////////////////////////////////////////////////////////
// Description: Tests if this model instance is loaded.
//
// Param:  none.
// Return: true if it is loaded; false otherwise.
//
////////////////////////////////////////////////////////////////////////////////
bool FFModel::isLoaded( void)  const
{
   return  ! p_ffName.empty();
}

////////////////////////////////////////////////////////////////////////////////
// Description: Retrieves source index for new source.
//
// Param:  none.
// Return: index of new source.
//
////////////////////////////////////////////////////////////////////////////////
int FFModel::getNewSourceID( void)
{
   return p_newSourceID;
}

////////////////////////////////////////////////////////////////////////////////
// Description: Adds FFItem instance to the specified force field category and
//              type.
//
// Param: catID    category index that specifies the force field category
// Param: typeName pointer to string of type name
// Param: params   reference to vector of parameters as strings
// Param: annot    reference to annotation string
//
////////////////////////////////////////////////////////////////////////////////
void FFModel::addNewFFItem( int catID, const string& typeName, 
                                       vector<string>& params, string& annot)
{
//printf("FFModel::addNewFFItem( %d, %s\n",catID, (char*)typeName.c_str());
//vector<string>::iterator iter = params.begin();
//while (iter != params.end())  {
//printf("params include %s\n", (char*)params[3].c_str()); 
//}
   if ( isValidCategoryIndex( catID))  {

      p_ffCategories[catID]->addFFItem( typeName, new FFItem( p_newSourceID, 
                                                                params, annot));
      p_newItemCount++;

      // Once a valid catID is confirmed and the typeID is retrieved with 2nd
      //   argument set to true, the only failure can be from too many 
      //   params. In that case, item is returned from p_ffSources as null.
      notify( FFModel::READY); // XXX alternative to forcing all listeners to
                                  //     totally redisplay XXX???
   } }

////////////////////////////////////////////////////////////////////////////////
// Description: Request to remove new force field item specified by category
//              index and type name. If the item is removed and the model
//              therefore changed, notify model change listeners.
//
// Param: catID    category index that specifies the force field category
// Param: idx      index that specifies the force field item in the named type
// Param: typeName pointer to string of type name
//
// Return: true if item is removed, false otherwise
//
////////////////////////////////////////////////////////////////////////////////
bool FFModel::removeFFItem( int catID, int idx, const string& typeName)
{
   bool removed = false;
   if ( isValidCategoryIndex( catID))  {
      try  {
         int typeID = p_ffCategories[catID]->getTypeID( typeName);
         int itemID = p_ffCategories[catID]->getTypeItemID( typeID, idx);
         int srcID = p_ffCategories[catID]->getItemSourceID( itemID);
         removed = p_ffCategories[catID]->removeFFItem( typeName, idx);

         if ( removed)  {
            if ( srcID == p_newSourceID )  // update new item count
               p_newItemCount--;
            notify( FFModel::READY);       // notify model listeners
         }
      }
      catch(std::invalid_argument& )  {
      }
      catch(std::range_error& )  {
      }
   }
   return removed;
}

////////////////////////////////////////////////////////////////////////////////
// Description:  Loads named the force field model from the named files.  Fails
//               if the force field name is a null pointer.
//
// Param: ffName is the force field name.
//        sources is a vector of force field parameter file names.
//
// Return: none.
//
// XXX synchronize
////////////////////////////////////////////////////////////////////////////////
void FFModel::load( const string& ffName, vector<string>& sources) 
                                           throw(IOException)
{
   EE_ASSERT( !ffName.empty(), EE_FATAL, "force field name is null");

   //---------------------------------------------------------------------------
   notify( FFModel::LOADING);

   //---------------------------------------------------------------------------
   // If the force field has already been defined, clear this model before
   //   re-loading it.
   //---------------------------------------------------------------------------
   clear();

   //---------------------------------------------------------------------------
   // register force field name
   //---------------------------------------------------------------------------
   p_ffName = ffName;

   try  {
      p_ffDesc = FFFactory().getFFDescriptor( p_ffName);
      initializeFFCategories();
   }
   catch( InvalidException&  ie)  {
      EE_ASSERT( false, EE_FATAL, string("FFModel::load() failed on ") + 
                                           p_ffName + ": " + ie.what());
   }
   catch( ParseException&  pe)  {
      EE_ASSERT( false, EE_FATAL, string("FFModel::load() failed to parse ") + 
                                           p_ffName + ": " + pe.what());
   } 
   catch( std::exception& e)  {
      EE_ASSERT( false, EE_FATAL, string("FFModel::load() failed on ") + 
                                           p_ffName + ": " + e.what());
   }
   p_ffSources.loadSources( sources);
   p_newSourceID = p_ffSources.getNewSourceID();
   NWChemDesc desc;
   desc.loadCategories(this, sources);

#ifdef SHOWING_NOT_USED
   //---------------------------------------------------------------------------
   // set showing flags
   //---------------------------------------------------------------------------
   p_srcShowing.resize(p_ffSources.getNumSources(), true);
   p_catShowing.resize(p_Categories.size(), true);
#endif

   //---------------------------------------------------------------------------
   notify( FFModel::READY);
   
}

//-------------------Private Methods---------------------------------
/////////////////////////////////////////////////////////////////
// Description: Tests if input parameter is a valid force field
//              category index.
//
// Param: idx is an index
//
// Return: true if the index is valid, false otherwise.
//
/////////////////////////////////////////////////////////////////
bool FFModel::isValidCategoryIndex( int idx) const
{
   return  idx >= 0 && idx < p_ffCategories.size(); 
}

////////////////////////////////////////////////////////////////////////////////
// Description: Clears exisiting local storage.
//
// Param: none.
// Return: none.
//
////////////////////////////////////////////////////////////////////////////////
void FFModel::clear( void)
{
   // clear name
   p_ffName = "";

   // FFSources handles clearing if and when new sources are loaded

   // clear categories
   for ( int i=0;i<p_ffCategories.size();i++)  {
     delete p_ffCategories[i];
   }
   p_ffCategories.clear();
   p_ffCatNames.clear();

   p_ElectrostaticStrings.clear();
   p_RelativeStrings.clear();
   p_ParametersStrings.clear();

   p_newItemCount = 0;
   p_selectedSources.clear();

#ifdef SHOWING_NOT_USED
   // clear boolean flags
   p_srcShowing.clear();
   p_catShowing.clear();
#endif
}

////////////////////////////////////////////////////////////////////////////////
// Description: Store list of selected sources
//
////////////////////////////////////////////////////////////////////////////////
void FFModel::setSelectedSources(vector<string> sources)
{
  p_selectedSources.clear();
  int id = -1;
  for ( int i=0;i<sources.size();i++)  {
     id = p_ffSources.getSourceID( sources[i]);
     if ( id != -1)
        p_selectedSources.push_back( id);
  }
}

////////////////////////////////////////////////////////////////////////////////
// Description: return list of selected sources
//
////////////////////////////////////////////////////////////////////////////////
vector<int> FFModel::selectedSources() const
{
  return p_selectedSources;
}

////////////////////////////////////////////////////////////////////////////////
// Description: Returns the index of the named category.
////////////////////////////////////////////////////////////////////////////////
int FFModel::getCategoryID( string catName)  {

   bool found = false;

   // search all existing FFCategories for matching category name
   int i;
   for ( i=0;i<p_ffCategories.size();i++)  {
      if ( catName.compare( p_ffCategories[i]->getName()) == 0)  {
          found = true;
          break;
      }
   }
   // if a match is not found, create and add a new FFCategory with this name
   if ( !found)  {
      p_ffCategories.push_back( new FFCategory( catName)); 
   }
   return i;
}

/**
 *  Retrieves the category type specified by the input name.
 *
 *  @param catName name of the force field category, e.g., Atoms, Bonds
 *  @return force field category type
 */
FFCategory::eCategoryTypes FFModel::getCategoryType( const string& catName) const {

  FFCategory::eCategoryTypes type = FFCategory::INVALID_TYPE; 

   // search all existing FFCategories for matching category name
   for ( int i=0; i< p_ffCategories.size();i++)  {
      if ( catName.compare( p_ffCategories[i]->getName()) == 0)  {
          type = p_ffCategories[i]->getCategoryType();
          break;
      }
   }
   return type;
}
//FFModel::effCategories FFModel::getCategoryType( const string& name)  const {
//   effCategories type = INVALID_INDEX;
//   if ( name.compare( "Atoms") == 0)         type = atomCat;
//   else if ( name.compare( "Cross") == 0)    type = crossCat;
//   else if ( name.compare( "Bonds") == 0)    type = bondCat;
//   else if ( name.compare( "Angles") == 0)   type = angleCat;
//   else if ( name.compare( "Proper") == 0)   type = torsionCat;
//   else if ( name.compare( "Improper") == 0) type = improperCat;
//   return type;
//}

////////////////////////////////////////////////////////////////////////////////
// Description: adds a string to p_ElectrostaticStrings
////////////////////////////////////////////////////////////////////////////////
void FFModel::addElectrostaticString(string str)
{
  p_ElectrostaticStrings.push_back(str);
}
// REMOVE?
////////////////////////////////////////////////////////////////////////////////
// Description: adds a string to p_RelativeStrings
////////////////////////////////////////////////////////////////////////////////
void FFModel::addRelativeString(string str)
{
  p_RelativeStrings.push_back(str);
}
// REMOVE?
////////////////////////////////////////////////////////////////////////////////
// Description: adds a string to p_ParametersStrings
////////////////////////////////////////////////////////////////////////////////
void FFModel::addParametersString(string str)
{
  p_ParametersStrings.push_back(str);
}
// REMOVE?
////////////////////////////////////////////////////////////////////////////////
// Description: return number of strings in p_ElectrostaticStrings
////////////////////////////////////////////////////////////////////////////////
int FFModel::numElectrostaticStrings(void)
{
  return p_ElectrostaticStrings.size();
}
// REMOVE?
////////////////////////////////////////////////////////////////////////////////
// Description: return number of strings in p_RelativeStrings
////////////////////////////////////////////////////////////////////////////////
int FFModel::numRelativeStrings(void)
{
  return p_RelativeStrings.size();
}
// REMOVE?
////////////////////////////////////////////////////////////////////////////////
// Description: return number of strings in p_ParameterStrings
////////////////////////////////////////////////////////////////////////////////
int FFModel::numParametersStrings(void)
{
  return p_ParametersStrings.size();
}

////////////////////////////////////////////////////////////////////////////////
// Description: return category from p_ffCategories
////////////////////////////////////////////////////////////////////////////////
FFCategory* FFModel::getCategory(int index)
{
  return p_ffCategories[index];
}
