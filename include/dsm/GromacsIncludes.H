/**
 * @file
 */
#ifndef GROMACSINCLUDES_HH
#define GROMACSINCLUDES_HH

#include <string>
#include <vector>
using std::string;
using std::vector;

/**
 * The files a GROMACS topology pulls in with #include, found on the disk the
 * topology was picked from.  A force field kept next to the topology
 * (min.ff/forcefield.itp) is not part of GROMACS, so it has to travel with
 * the job; GROMACS's own libraries (share/gromacs/top) are found by grompp
 * and are left alone.
 */
class GromacsIncludes
{
   public:
      struct File {
         string name;   ///< path relative to the topology, as grompp will see it
         string path;   ///< where it was read from
      };

      /**
       * Follows every #include of the topology, recursively, in both branches
       * of any #ifdef.  A name is looked for next to the file that includes
       * it, then next to the topology.
       *
       * @param found    files that exist beside the topology, each once
       * @param system   includes found only in GROMACS's own library
       * @param missing  includes found nowhere
       */
      static void collect(const string& topPath, vector<File>& found,
                          vector<string>& system, vector<string>& missing);

      /**
       * The name a file with a sub-folder is stored under: the data server
       * and the job's file transfer keep one flat list of names, so the
       * folder separator is spelled out and put back by the job script.
       */
      static string encodeName(const string& relativePath);
      static string decodeName(const string& storedName);
};
#endif
