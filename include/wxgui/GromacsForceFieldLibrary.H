/**
 * @file
 */
#ifndef _GROMACSFORCEFIELDLIBRARY_H_
#define _GROMACSFORCEFIELDLIBRARY_H_

#include <string>
#include <vector>
using std::string;
using std::vector;

/**
 * The GROMACS force fields a user imported, kept as <name>.ff folders under
 * <ECCE user config dir>/forcefields on the client.  A job gets one as task
 * input files (see GromacsPreparePanel), never by this path, so it works the
 * same against a central data server.  No dialogs here: the caller asks.
 */
namespace GromacsForceFieldLibrary
{
   struct Entry
   {
      string name;    // the folder name without .ff, what FF= holds
      string label;   // first line of forcefield.doc, else the name
      string path;    // the folder
   };

   enum Result { OK, CLASH, REFUSED };

   string directory();
   vector<Entry> list();
   bool find(const string& name, Entry& entry);

   /** Relative paths (with /) of every file of the folder. */
   vector<string> files(const Entry& entry);

   /** The water model ids of the folder's watermodels.dat, in its order. */
   vector<string> waterModels(const Entry& entry);

   /**
    * Validates a <name>.ff folder or a .zip holding one and copies it into
    * the library.  CLASH (name set) when the name is taken by the library or
    * by one of the built-in ids and replace is false; REFUSED has a message.
    */
   Result import(const string& source, bool replace,
                 const vector<string>& builtIn, string& name, string& message);
}

#endif
