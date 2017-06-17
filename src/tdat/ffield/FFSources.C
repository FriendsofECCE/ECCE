///////////////////////////////////////////////////////////////////////////////
// FFSources Class
//
//
// DESCRIPTION:  Handles source information. Assumes that the order of the
//               input file names reflects the order of supersession intended
//               by the user.  That is, the force field parameters in each file
//               supercede any values given in preceeding files (files with a
//               lower index).
//
///////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <iostream>
  using std::cout;
  using std::endl;

#include "util/ErrMsg.H"
#include "tdat/FFSources.H"

const char* FFSources::NEW_SOURCE = "new";

//-------------------Constructor---------------------------------
////////////////////////////////////////////////////////////////////////////////
// Description: Constructor creates an empty object.  Individual source elements
//              are loaded, cleared, or reloaded subsequently. The order that
//              the source elements are added is important.  Items from the 
//              last source loaded supercede matching items from previously
//              ordered sources.
//
////////////////////////////////////////////////////////////////////////////////
FFSources::FFSources( void) {
}

//-------------------Destructor-------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
// Description: Clear dynamically allocated memory.
//
////////////////////////////////////////////////////////////////////////////////
FFSources::~FFSources( void)  {
  clear();
}

////////////////////////////////////////////////////////////////////////////////
//-------------------Public Methods---------------------------------------------
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Description: Loads source elements specified by arguments.  Elements are
//              assumed to be added in reverse order of precedence (last element
//              superceeds previous elements.
//
// Params: ff_sources is a vector containing the force field parameter filenames
//         (full pathnames).
//
// Return: None.
//
////////////////////////////////////////////////////////////////////////////////
void FFSources::loadSources( vector<string>& ff_sources)
{
   // Clear resources
   clear();

   // for each source, ...
   for ( int sid=0;sid<ff_sources.size();sid++)  {

      // register this source
      SFile* source =  new SFile(ff_sources[sid]);
      p_sources.push_back(source);
   }
}

////////////////////////////////////////////////////////////////////////////////
// Description: Returns the index of the new source. This is always one larger 
//              than the list of bone fide (actual input) sources.
//
// Return: index of the new source
//
////////////////////////////////////////////////////////////////////////////////
int FFSources::getNewSourceID( void) const
{
   return p_sources.size();
}

////////////////////////////////////////////////////////////////////////////////
// Description: Retrieves a count of the sources
//
// Return: current count of the sources
//
////////////////////////////////////////////////////////////////////////////////
int FFSources::getSourceCount()  const
{
  return p_sources.size();
}

////////////////////////////////////////////////////////////////////////////////
// Description:  Returns path name for source indexed by the input 
//               argument. (does not include the file name.)
//
// Params: sid is a source index.
//
// Return: string containing the indicated source path or "new" label.
//
//
////////////////////////////////////////////////////////////////////////////////
string FFSources::getSourceName(int sid) const throw(IndexOutOfRangeException)
{
   string path;
   if ( sid >= 0 && sid < p_sources.size()) {
      path = p_sources[sid]->path();
   } else if ( sid == p_sources.size()) {
      path = NEW_SOURCE;
   } else {
      throw IndexOutOfRangeException(0, p_sources.size(), sid, WHERE);
   }

   return path;
}
////////////////////////////////////////////////////////////////////////////////
// Description:  Returns the name of the new source
//
// Params: 
// Return: name of new source
//
////////////////////////////////////////////////////////////////////////////////
string FFSources::getNewSourceName() const  {
   return NEW_SOURCE;
}

////////////////////////////////////////////////////////////////////////////////
// Description: Maps source name to associated source index.
//
// Params: sname name of source
//
// Return: index of named source
//
////////////////////////////////////////////////////////////////////////////////
int FFSources::getSourceID( string& sname) const
{
   int id = -1, count = 0;

   // check for match in the source names
   for ( count=0;count<p_sources.size();count++)  {
      if ( sname.compare( p_sources[count]->path()) == 0) {
         id = count;
         break;
      }
   }
    
   // check to see if string matches new source
   if (id == -1) {
     if (sname == NEW_SOURCE) {
       id = getNewSourceID();
     }
   }

   return id;
}

////////////////////////////////////////////////////////////////////////////////
//---------------------Private methods------------------------------------------
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Description: Clears the local storage resources.
//
// Params: None.
// Return: None.
//
////////////////////////////////////////////////////////////////////////////////
void FFSources::clear( void)
{
   for ( int i=0;i<p_sources.size();i++)
      delete  p_sources[i];
   p_sources.clear();   
}

