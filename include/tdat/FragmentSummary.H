#ifndef FRAGMENTSUMMARY_H
#define FRAGMENTSUMMARY_H

#include <vector>
using std::vector;
#include <string>
using std::string;
#include <iostream>
using std::ostream;
using std::istream;

/**
 * Class to provide a subset of properties from the scene graph Fragment.
 * This is used to manage interactions between the builder and other
 * applications without having to copy the entire Fragment to the other
 * application.
 * Most data in this class passes through our messaging system so it is
 * designed to be very compact.  However, we add a few metadata items that
 * qualify as Fragment summary info but don't need to be communicated.
 */
class Fragment;

class FragmentSummary
{
   public:
      FragmentSummary();
      virtual ~FragmentSummary();

      /**
       * Accessors
       */
      void getDimensions(double* dimensions) const;
      int getNumSelected() const;
      int getAtom(int idx) const;
      int getParent(int idx) const;
      string getAtomName(int idx) const;
      string getAtomString(int idx) const;
      int    getSelectedModel() const;
      string getAltLocation() const;
      string getChain() const;
      string getName() const;

      /**
       * Modifiers
       */
      void setDimensions(double* dimensions);
      void setNumSelected(int num);
      void setAtom(int iatm, int idx);
      void setParent(int ires, int idx);
      void setAtomName(string name, int idx);
      void setSelectedModel(int model);
      void setAltLocation(const string& loc);
      void setChain(const string& chain);
      void setName(const string& name);

      /**
       * Initialize FragmentSummary to something harmless
       */
      void init();

      /**
       * Initialize FragmentSummary by doing a quick scan
       * of a PDB file
       */
      bool scanPDB(istream& is);

      /**
       * Update state of FragmentSummary from scene graph
       */
      void update(Fragment *frag, vector<int>);

      /**
       * Overwrite << and >> operators so that FragmentSummary objects can
       * be used with JMS communication layer
       */
      friend ostream& operator<<(ostream& os, const FragmentSummary& summary);
      friend istream& operator>>(istream& is, FragmentSummary& summary);

   private:
      double p_xmin,p_xmax,p_ymin,p_ymax,p_zmin,p_zmax;
      int p_natom,p_iatm1,p_iatm2,p_iparent1,p_iparent2,p_iatm3,p_iparent3;
      string p_atnam1,p_atnam2,p_atnam3;

      /** model user chose from pdb file; default -1; not part of >> or << */
      int p_selectedModel;

      /** alt location user chose from pdb file; not part of >> or << */
      string p_altLocation;

      /** chain user chose from pdb file; default ""; not part of >> or << */
      string p_chain;

      /** name user enters in builder; default ""; not part of >> or << */
      string p_name;

};

#endif
