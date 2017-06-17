/**
 * @file
 *
 *
 */
#ifndef NWDIRDYMODEL_HH
#define NWDIRDYMODEL_HH

#include <iostream>
  using std::istream;
  using std::ostream;

#include <string>
  using std::string;
#include <vector>
  using std::vector;

#include "util/IOException.H"

#include "tdat/ITaskModel.H"
#include "tdat/Fragment.H"


/**
 * Model for implementing NWChem DirDyVTST Editor.
 */
class NWDirdyModel : public ITaskModel
{

   public:
      /* Default parameters */
      static const bool AUTOSYM;
      static const double SYMTOL;
      static const double FRAGSEP;
      static const bool USESP;
      static const string CATEGORY;
      static const string THEORY;
      static const string SPCATEGORY;
      static const string SPTHEORY;
      static const bool EXPCOEFF;
      static const bool SPEXPCOEFF;
      static const double MEPINC;
      static const double SAVEINC;
      static const double HESSINC;
      static const double RMASS;
      static const bool TSTONLY;
      static const double SLOW;
      static const double SHIGH;
      static const bool SWITCH;
      static const int INTOPT;
      static const bool PRINT;
      static const bool AUTOZ;
      static const bool MEMORYFLAG;
      static const int MEMORY;
      static const bool DISKFLAG;
      static const int DISK;
      static const int NUMATOMS;


      /* Constructors */
      NWDirdyModel();
      NWDirdyModel(const string& url, const string& name);

      virtual ~NWDirdyModel();

      /* Accessors */
      bool getProductsFlag() const;
      int getNumSelectedAtoms() const;
      vector<int> getSelectionList() const;
      Fragment* getTransitionState() const;
      Fragment* getReactant1() const;
      Fragment* getReactant2() const;
      Fragment* getProduct1() const;
      Fragment* getProduct2() const;
      bool getAutoSym() const;
      double getSymmetryTol() const;
      double getFragmentSep() const;
      bool getUseSP() const;
      string getCategory() const;
      string getTheory() const;
      string getSPCategory() const;
      string getSPTheory() const;
      bool getExpCoeff() const;
      bool getSPExpCoeff() const;
      double getMEPIncrement() const;
      double getSaveIncrement() const;
      double getHessianIncrement() const;
      double getReducedMass() const;
      bool getTSTOnly() const;
      double getLowerSLimit() const;
      double getHigherSLimit() const;
      bool getSwitchSign() const;
      int getIntegrationOpt() const;
      bool getPrintFlag() const;
      bool getAutoZ() const;
      bool getMemoryFlag() const;
      int getMemory() const;
      bool getDiskFlag() const;
      int getDisk() const;
      bool getLinearTS() const;
      bool getLinearProd1() const;
      bool getLinearProd2() const;
      bool getLinearReact1() const;
      bool getLinearReact2() const;
      int getNumAtomsTS() const;
      int getNumAtomsProd1() const;
      int getNumAtomsProd2() const;
      int getNumAtomsReact1() const;
      int getNumAtomsReact2() const;

      /* Modifiers */
      void setProductsFlag(bool flag);
      void setNumSelectedAtoms(int natoms);
      void setSelectionList(vector<int> list);
      void setTransitionState(Fragment *frag);
      void setReactant1(Fragment *frag);
      void setReactant2(Fragment *frag);
      void setProduct1(Fragment *frag);
      void setProduct2(Fragment *frag);
      void setAutoSym(bool flag);
      void setSymmetryTol(double tol);
      void setFragmentSep(double sep);
      void setUseSP(bool flag);
      void setCategory(string cat);
      void setTheory(string thry);
      void setSPCategory(string cat);
      void setSPTheory(string thry);
      void setExpCoeff(bool flag);
      void setSPExpCoeff(bool flag);
      void setMEPIncrement(double inc);
      void setSaveIncrement(double inc);
      void setHessianIncrement(double inc);
      void setReducedMass(double mass);
      void setTSTOnly(bool flag);
      void setLowerSLimit(double lim);
      void setHigherSLimit(double lim);
      void setSwitchSign(bool flag);
      void setIntegrationOpt(int opt);
      void setPrintFlag(bool flag);
      void setAutoZ(bool flag);
      void setMemoryFlag(bool flag);
      void setMemory(int inc);
      void setDiskFlag(bool flag);
      void setDisk(int inc);
      void setLinearTS(bool flag);
      void setLinearProd1(bool flag);
      void setLinearProd2(bool flag);
      void setLinearReact1(bool flag);
      void setLinearReact2(bool flag);
      void setNumAtomsTS(int num);
      void setNumAtomsProd1(int num);
      void setNumAtomsProd2(int num);
      void setNumAtomsReact1(int num);
      void setNumAtomsReact2(int num);

      /* ITaskModel virtuals */
      virtual void generateInputFile() throw(InvalidException);
      virtual void run() throw(InvalidException);
      void reset();

   private:
      int p_numAtoms;
      vector<int> p_selectList;
      Fragment * p_transition;
      Fragment * p_react1;
      Fragment * p_react2;
      Fragment * p_prod1;
      Fragment * p_prod2;

      bool p_autosym;
      double p_symTol;
      double p_fragSep;
      bool p_useSP;
      string p_category;
      string p_theory;
      string p_SPCategory;
      string p_SPTheory;
      bool p_expCoeff;
      bool p_SPExpCoeff;
      double p_MEPInc;
      double p_saveInc;
      double p_hessInc;
      double p_rmass;
      bool p_tstOnly;
      double p_sLow;
      double p_sHigh;
      bool p_switch;
      int p_integrationOpt;
      bool p_print;
      bool p_products;
      bool p_autoz;
      bool p_memoryFlag;
      int p_memory;
      bool p_diskFlag;
      int p_disk;
      bool p_linearTS, p_linearProd1, p_linearProd2;
      bool p_linearReact1, p_linearReact2;
      int p_numAtomsTS, p_numAtomsProd1, p_numAtomsProd2;
      int p_numAtomsReact1, p_numAtomsReact2;

};
#endif
