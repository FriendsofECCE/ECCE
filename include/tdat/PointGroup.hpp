///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: PointGroup.H
//
//
// CLASS SYNOPSIS:
//
// EXPORTED TYPES:
//    Class PointGroup
//
// LOCAL TYPES:
//
// SEE ALSO:
//      
// DESCRIPTION:
//   This class povides read-only access to PointGroup symmetry data.
//   It was not designed to be objectstore persistent.
//
//   Each point group (determined by the locations of the nuclei)
//   has its own set of symmetry labels (also known by the formidable
//   moniker "irreducible representations" or "irreps").  Each vibrational
//   mode, molecular orbital, or electronic state can always be assigned
//   to one of the irreps of the nuclear symmetry group.
//  
//   There are a couple of naming conventions for the groups; these
//   are the "Schoenflies" notation, favored by chemists.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef POINTGROUP_HH
#define POINTGROUP_HH

#include <iostream>
  using std::ostream;

#include <string>
#include <vector>
  using std::string;
  using std::vector;


class PointGroup
{

   public:
      // Constructors
      PointGroup(const string& name);
      PointGroup(const PointGroup&);

      // Virtual Destructor
      virtual ~PointGroup(void);

      // Operators
      PointGroup& operator=(const PointGroup&);
      bool operator==(const PointGroup&) const;
      bool operator!=(const PointGroup&) const;

      // Modifiers

      // Accessors
      string name(void) const;
      bool isValid(const string& irrepName) const;

      static bool isGroup(const string& groupName);
      static vector<string> getGroups();

      // Rather than create a new class, use PointGroup to provide
      // access to to space group names.  This is because we are using
      // the fragment pointGroup field at least for now.
      // There is no equivalent to "irreps" at this time.
      static bool isSpaceGroup(const string& groupName);
      static vector<string> getSpaceGroups();

   protected:


   private:

      // State
      string p_groupName;
      vector<string> *p_irreps;

      PointGroup(void);    // for loading
      static void initialize(void);
      static void dump(void);
      static vector<PointGroup*>  *p_extent;
      PointGroup const *get(const string& name) const;
      static vector<string> p_spnames;

      // Friends
      friend ostream& operator<< (ostream& os, const PointGroup&);

}; // PointGroup

#endif /* POINTGROUP_HH */
