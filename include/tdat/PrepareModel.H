/**
 * @file
 *
 *
 */
#ifndef PREPAREMODEL_HH
#define PREPAREMODEL_HH

#include <iostream>
  using std::istream;
  using std::ostream;

#include <string>
  using std::string;
#include <vector>
  using std::vector;
#include <map>
  using std::map;

#include "util/IOException.H"

#include "tdat/ITaskModel.H"

/**
 * Model for implementing NWChem MD Prepare module GUI.
 */
class PrepareModel : public ITaskModel
{

   public:

      enum DefaultHistidine {HID, HIE, HIP};

      /* Default values */
      static const bool FIRSTTASK;
      static const bool NEWTOPOLOGY;
      static const int NUMMODELS;
      static const PrepareModel::DefaultHistidine DEFAULTHIS;
      static const bool SOLVATE;
      static const bool SETSOLVENT;
      static const int SOLVENTOPT;
      static const string SOLVENTNAME;
      static const string SOLVENTMODEL;
      static const int GEOMOPT;
      static const double TOUCH;
      static const double EXPANDVOL;
      static const double SOLVENTTOL;
      static const int INITIALATOM;
      static const int FINALATOM;
      static const int TRNSOPTION;
      static const double TRNSX;
      static const double TRNSY;
      static const double TRNSZ;
      static const int ORNTOPTION;
      static const double ANGLE;
      static const double ORIENTX;
      static const double ORIENTY;
      static const double ORIENTZ;
      static const int RMVOPT;
      static const bool XRANGE;
      static const bool YRANGE;
      static const bool ZRANGE;
      static const int REPOPTS;
      static const int INDMODCELL;
      static const double MCLLMINSEP;
      static const bool RANDXYZ;
      static const bool INVCOPY;
      static const bool USEZBOX;
      static const bool SETIONS;
      static const string IONOPTION;
      static const double CISCALE;
      static const bool ADDLNK;
      static const int CINUM;
      static const double MINSEPARATION;
      static const int CIGRID;
      static const double CIMINDIS;
      static const int BCOPT;
      static const bool USEMEM;
      static const int HEAPMEM;
      static const int STACKMEM;
      static const int GLOBALMEM;
      static const int PRINTLEVEL;
      static const bool USEFRAC;
      static const int FRAC1;
      static const int FRAC2;
      static const int FRAC3;
      static const int FRAC4;
      static const int FRAC5;
      static const int FRAC6;
      static const int FRAC7;
      static const int FRAC8;
      static const int FRAC9;
      static const int FRAC10;

      /* Constructors */
      PrepareModel();
      PrepareModel(const string& url, const string& name);

      virtual ~PrepareModel();

      /* Task Model virtuals */
      virtual void generateInputFile() throw(InvalidException);
      virtual void run() throw(InvalidException);

      bool prescanPDB(const string& url);
      void reset();

      /* Accessors */
      bool getIsFirstTask() const;
      string getPDBSource() const;
      int getNumModels() const;
      vector<string> getAltLocVec() const;
      vector<string> getChainVec() const;
      PrepareModel::DefaultHistidine getDefaultHistidine() const;
      bool getAddLinks() const;
      vector<string> getCounterIons() const;
      double getCounterIonScale() const;
      bool getSetSolvent() const;
      int getSolventOption() const;
      string getSolventName() const;
      string getSolventModel() const;
      bool getSolvateFlag() const;
      int getBoundaryCondition() const;
      int getGeometryOption() const;
      double getMinSluSluDist() const;
      double getMinSluSlvDist() const;
      double getExpandVolPar() const;
      double getEnvelopePar() const;
      vector<string> getCmdList() const;
      vector<string> getLinkList() const;
      vector<string> getCounterIonList() const;
      vector<string> getPmfList() const;
      map<string,int> getPmfSelection() const;
      string getPmfSelection(int selection);
      int getPmfSelection(string selection);
      int getInitialAtom() const;
      int getFinalAtom() const;
      int getTranslateOption() const;
      double getTranslateX() const;
      double getTranslateY() const;
      double getTranslateZ() const;
      int getOrientOption() const;
      double getAngle() const;
      double getOrientX() const;
      double getOrientY() const;
      double getOrientZ() const;
      int getRemoveOption() const;
      bool getRemoveXRange() const;
      bool getRemoveYRange() const;
      bool getRemoveZRange() const;
      int getRepeatOption() const;
      int getXRepeat() const;
      int getYRepeat() const;
      int getZRepeat() const;
      double getMinRepeatSeparation() const;
      bool getRandomX() const;
      bool getRandomY() const;
      bool getRandomZ() const;
      bool getInvertZCopy() const;
      bool getUseZBoxDim() const;
      int getSkipX() const;
      int getSkipY() const;
      int getSkipZ() const;
      bool getSetIons() const;
      string getCounterIonOpt() const;
      int getCINumber() const;
      int getCIGridSize() const;
      double getMinAtomCIDis() const;
      bool getUseMemory() const;
      int getHeapMemory() const;
      int getStackMemory() const;
      int getGlobalMemory() const;
      int getPrintLevel() const;
      bool getChangedTopology() const;
      bool getUseFractions() const;
      int getFraction1() const;
      int getFraction2() const;
      int getFraction3() const;
      int getFraction4() const;
      int getFraction5() const;
      int getFraction6() const;
      int getFraction7() const;
      int getFraction8() const;
      int getFraction9() const;
      int getFraction10() const;
      void getDefaultDims(double *dims) const;

      /* Modifiers */
      void setIsFirstTask(bool flag);
      void setPDBSource(string source);
      void setNumModels(int numModels);
      void setAltLocVec(vector<string> altLocVec);
      void setChainVec(vector<string> chainVec);
      void setDefaultHistidine(PrepareModel::DefaultHistidine def);
      void setAddLinks(bool flag);
      void setCounterIons(vector<string> ions);
      void setCounterIonScale(double scale);
      void setSetSolvent(bool flag);
      void setSolventOption(int opt);
      void setSolventName(string str);
      void setSolventModel(string str);
      void setSolvateFlag(bool flag);
      void setBoundaryCondition(int opt);
      void setGeometryOption(int opt);
      void setMinSluSluDist(double dist);
      void setMinSluSlvDist(double dist);
      void setExpandVolPar(double param);
      void setEnvelopePar(double param);
      void appendCmdList(string cmd);
      void deleteCmdList(int index);
      void clearCmdList();
      void insertCmdList(int index, string cmd);
      void appendLinkList(string link);
      void clearLinkList();
      void deleteLinkList(int index);
      void appendCounterIonList(string ion);
      void clearCounterIonList();
      void deleteCounterIonList(int index);
      void appendPmfCmdList(string cmd);
      void deletePmfCmdList(int index);
      void clearPmfCmdList();
      void insertPmfCmdList(int index, string cmd);
      int  addPmfSelection(string selection, int index = -1);
      void removePmfSelection(string selection);
      void removePmfSelection(int selection);
      void clearPmfSelection();
      void setInitialAtom(int iatom);
      void setFinalAtom(int iatom);
      void setTranslateOption(int opt);
      void setTranslateX(double value);
      void setTranslateY(double value);
      void setTranslateZ(double value);
      void setOrientOption(int opt);
      void setAngle(double value);
      void setOrientX(double value);
      void setOrientY(double value);
      void setOrientZ(double value);
      void setRemoveOption(int opt);
      void setRemoveXRange(bool flag);
      void setRemoveYRange(bool flag);
      void setRemoveZRange(bool flag);
      void setRepeatOption(int opt);
      void setXRepeat(int freq);
      void setYRepeat(int freq);
      void setZRepeat(int freq);
      void setMinRepeatSeparation(double value);
      void setRandomX(bool flag);
      void setRandomY(bool flag);
      void setRandomZ(bool flag);
      void setInvertZCopy(bool flag);
      void setUseZBoxDim(bool flag);
      void setSkipX(int freq);
      void setSkipY(int freq);
      void setSkipZ(int freq);
      void setSetIons(bool flag);
      void setCounterIonOpt(string opt);
      void setCINumber(int num);
      void setCIGridSize(int size);
      void setMinAtomCIDis(double dist);
      void setUseMemory(bool flag);
      void setHeapMemory(int mem);
      void setStackMemory(int mem);
      void setGlobalMemory(int mem);
      void setPrintLevel(int level);
      void setChangedTopology(bool flag);
      void setUseFractions(bool flag);
      void setFraction1(int frac);
      void setFraction2(int frac);
      void setFraction3(int frac);
      void setFraction4(int frac);
      void setFraction5(int frac);
      void setFraction6(int frac);
      void setFraction7(int frac);
      void setFraction8(int frac);
      void setFraction9(int frac);
      void setFraction10(int frac);
      void setDefaultDims(double *dims);

   protected:

   private:

      bool p_isFirst;
      /** Original PDB source file for prepare module */
      string p_PDBsource;

      int p_numModels;
      int p_pdbModel;
      char p_altloc;
      vector<string> p_altLocVec;
      char p_chain;
      vector<string> p_chainVec;

      /* Setting topology file, etc. */
      PrepareModel::DefaultHistidine p_defHistidine;
      bool p_setSolvent;
      int p_solventOpt;
      string p_solventName;
      string p_solventModel;
      vector<string> p_links;
      bool p_addLink;
      bool p_setIons;
      vector<string> p_cIons;
      string p_ionOption;
      int p_CINumber;
      double p_CIScale;
      int p_CIGrid;
      double p_CIMinDis;
      bool p_useMemory;
      int p_heap;
      int p_stack;
      int p_global;
      int p_printLevel;
      bool p_changedTopology;
      bool p_useFrac;
      int p_frac1;
      int p_frac2;
      int p_frac3;
      int p_frac4;
      int p_frac5;
      int p_frac6;
      int p_frac7;
      int p_frac8;
      int p_frac9;
      int p_frac10;

      /* Coordinate panel values */
      bool p_solvate;
      int p_BCOpt;
      int p_geomOpt;
      double p_minSluSluDis;
      double p_touch;
      double p_expandVol;
      double p_envelope;
      int p_initAtom;
      int p_finalAtom;
      int p_translateOption;
      double p_trnsX, p_trnsY, p_trnsZ;
      int p_orientOption;
      double p_angle;
      double p_orntX, p_orntY, p_orntZ;
      int p_removeOpt;
      bool p_xRange, p_yRange, p_zRange;
      int p_repeatOption;
      int p_xRepeat, p_yRepeat, p_zRepeat;
      double p_minRSep;
      bool p_ranX, p_ranY, p_ranZ;
      bool p_invertZCopy;
      bool p_useZBoxDim;
      int p_skipX, p_skipY, p_skipZ;

      /* Solvation and transformation commands */
      vector<string> p_coordCmd;

      /* Thermodynamics */
      vector<string> p_pmfCmd;
      map<string,int> p_pmfSelStringToInt;
      map<int,string> p_pmfSelIntToString;
      map<string,int> p_pmfSelCount;

      /* System dimensions */
      double p_defaultDims[6];
};
#endif
