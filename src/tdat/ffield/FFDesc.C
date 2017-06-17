////////////////////////////////////////////////////////////////////////////////
// FFDesc Class
//
//
// DESIGN:  
//   This class is a container for FFCategoryDesc objects so as to provide
//   a wel defined interface.
//
////////////////////////////////////////////////////////////////////////////////

#include "tdat/FFDesc.H"

/**
 * Default Constructor.
 */
FFDesc::FFDesc( )
{
}

/**
 * Constructor.
 */
FFDesc::FFDesc( const string& name )  
{
   setName( name);
}

/**
 * Destructor.
 */
FFDesc::~FFDesc( )
{
   clear();
}


//-------------------Public Methods---------------------------------------------

/**
 * Returns name of this descriptor.
 */
string FFDesc::getName() const
{ 
   return p_name;
}

bool FFDesc::exists(const string& name) const
{
   return (index(name) != -1);
}

/**
 * Get the names of all categories.
 */
vector<string> FFDesc::getCategoryNames() const
{
   vector<string> ret;
   for (int idx=0; idx<size(); idx++) {
      ret.push_back((*this)[idx]->getName());

   }
   return ret;
}

/**
 * Get a category by index.
 */
const FFCategoryDesc& FFDesc::getCategory(int index) const
                         throw(std::range_error)
{
   if (index < 0 || index >= size()) 
      throw std::range_error("Illegal range in FFDesc::getCategory()");

   return *((*this)[index]);
}

/**
 * Get a category by name.
 */
const FFCategoryDesc& FFDesc::getCategoryByName(const string& name) const
                         throw(std::range_error)
{
   int idx = index(name);

   if (idx >= 0) return *((*this)[idx]);

   throw std::range_error("Illegal range in FFDesc::getCategoryByName()");

}


/**
 * Add a category.
 * The object itself is used directly - no copy is made.
 * FFDesc assumes responsibility for memory management.
 */
void FFDesc::addCategory(FFCategoryDesc *category)
{
   push_back(category);
}


/**
 * Removes all categories.
 * Memory for each category is freed.
 */
void FFDesc::clear() 
{
   vector<FFCategoryDesc*>::iterator it = begin();
   while (it != end()) {
      delete (*it);
      it++;
   }
   vector<FFCategoryDesc*>::clear();
}

/**
 * Removes a category of the given name.
 * Memory for each category is freed.
 */
void FFDesc::removeCategory(const string& name) throw(std::range_error)
{
   int idx = index(name);
   if (idx >= 0) {
      vector<FFCategoryDesc*>::iterator it = begin();
      it += idx;
      FFCategoryDesc *cat = (*it);
      erase(it);
      delete cat;
   } else {
     throw std::range_error("Illegal range in FFDesc::removeCategory()");
   }
}





//-------------------Protected Methods--------------------------------------------

void FFDesc::setName(const string& name)
{
   p_name = name;
}

int FFDesc::index(const string& name) const
{
   int ret = -1;
   for (int idx=0; ret == -1 && idx<size(); idx++) {
      if ((*this)[idx]->getName() == name) 
         ret = idx;
   }
   return ret;
}
