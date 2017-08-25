/**
 * @file
 *
 *
 */
#ifndef FILESMODEL_HH
#define FILESMODEL_HH

#include <string>
  using std::string;
#include <vector>
  using std::vector;

#include "util/IOException.H"


/**
 */
class FilesModel 
{

   public:
      /* Default values */
      static const bool PRINTMD;
      static const int PRINTMDFREQ;
      static const bool SOLVENTENERGY;
      static const bool SOLUTEENERGY;
      static const bool PRINTSTAT;
      static const int STATFREQ;
      static const bool SOLUTEBONDS;
      static const int BONDFREQ;
      static const bool SOLUTEFRCS;
      static const int FORCEFREQ;
      static const bool PMFDATA;
      static const int PMFFREQ;
      static const bool PRINTTOP;
      static const bool NBONDPAR;
      static const bool SLVBONDPAR;
      static const bool SLUBONDPAR;
      static const int RSTFREQ;
      static const bool KEEPRST;
      static const bool WRITETRJ;
      static const int TRJSTORAGE;
      static const bool USEBATCH;
      static const int BATCHSZ;
      static const bool COORDSLUPRT;
      static const int COORDSLUFRQ;
      static const bool COORDSLVPRT;
      static const int COORDSLVFRQ;
      static const bool VELSLUPRT;
      static const int VELSLUFRQ;
      static const bool VELSLVPRT;
      static const int VELSLVFRQ;
      static const bool FRCSLUPRT;
      static const int FRCSLUFRQ;
      static const bool FRCSLVPRT;
      static const int FRCSLVFRQ;
      static const bool WRTPOL;
      static const int POLOPT;
      static const int POLSLUFRQ;
      static const int POLSLVFRQ;
      static const bool AVGPROP;
      static const int PROPFREQ;


      /* Constructors */
      FilesModel();

      virtual ~FilesModel();

      /* Accessors */
      bool getPrintMD() const;
      int getPrintMDFreq() const;
      bool getSolventEnergy() const;
      bool getSoluteEnergy() const;
      bool getPrintStat() const;
      int getStatFreq() const;
      bool getSoluteBonds() const;
      int getBondFreq() const;
      bool getSoluteForces() const;
      int getForceFreq() const;
      bool getPmfData() const;
      int getPmfFreq() const;
      bool getPrintTop() const;
      bool getNbondPar() const;
      bool getSolventBondPar() const;
      bool getSoluteBondPar() const;
      int getRestartFreq() const;
      bool getKeepRestart() const;
      bool getWriteTrajectory() const;
      int getTrajectoryStorage() const;
      bool getUseBatches() const;
      int getBatchSize() const;
      bool getCoordsSolutePrint() const;
      int getCoordsSoluteFreq() const;
      bool getCoordsSolventPrint() const;
      int getCoordsSolventFreq() const;
      bool getVelocitySolutePrint() const;
      int getVelocitySoluteFreq() const;
      bool getVelocitySolventPrint() const;
      int getVelocitySolventFreq() const;
      bool getForcesSolutePrint() const;
      int getForcesSoluteFreq() const;
      bool getForcesSolventPrint() const;
      int getForcesSolventFreq() const;
      bool getWritePolarization() const;
      int getPolarizationOpt() const;
      int getPolarizationSoluteFreq() const;
      int getPolarizationSolventFreq() const;
      bool getAvgProperties() const;
      int getPropertyFreq() const;

      /* Modifiers */
      void setPrintMD(bool flag);
      void setPrintMDFreq(int frq);
      void setSolventEnergy(bool flag);
      void setSoluteEnergy(bool flag);
      void setPrintStat(bool flag);
      void setStatFreq(int frq);
      void setSoluteBonds(bool flag);
      void setBondFreq(int frq);
      void setSoluteForces(bool flag);
      void setForceFreq(int frq);
      void setPmfData(bool flag);
      void setPmfFreq(int frq);
      void setPrintTop(bool flag);
      void setNbondPar(bool flag);
      void setSolventBondPar(bool flag);
      void setSoluteBondPar(bool flag);
      void setRestartFreq(int freq);
      void setKeepRestart(bool flag);
      void setTrajectoryStorage(int store);
      void setUseBatches(bool flag);
      void setBatchSize(int size);
      void setCoordsSolutePrint(bool flag);
      void setCoordsSoluteFreq(int freq);
      void setCoordsSolventPrint(bool flag);
      void setCoordsSolventFreq(int freq);
      void setVelocitySolutePrint(bool flag);
      void setVelocitySoluteFreq(int freq);
      void setVelocitySolventPrint(bool flag);
      void setVelocitySolventFreq(int freq);
      void setForcesSolutePrint(bool flag);
      void setForcesSoluteFreq(int freq);
      void setForcesSolventPrint(bool flag);
      void setForcesSolventFreq(int freq);
      void setWritePolarization(bool flag);
      void setPolarizationOpt(int opt);
      void setPolarizationSoluteFreq(int freq);
      void setPolarizationSolventFreq(int freq);
      void setAvgProperties(bool flag);
      void setPropertyFreq(int freq);

      /**
       * Reset model values to defaults
       */
      void reset();

   protected:

   private:
      bool p_printMD;
      int p_printMDFreq;
      bool p_solventEnergy;
      bool p_soluteEnergy;
      bool p_printStat;
      int p_statFreq;
      bool p_soluteBonds;
      int p_bondFreq;
      bool p_soluteForces;
      int p_forceFreq;
      bool p_printTop;
      bool p_nbondPar;
      bool p_solventBondPar;
      bool p_soluteBondPar;
      int p_restartFreq;
      bool p_keepRestart;
      int p_trajectoryStorage;
      bool p_useBatches;
      int p_batchSize;
      bool p_writeCoords;
      bool p_soluteCoordsPrint;
      int p_soluteCoordsFreq;
      bool p_solventCoordsPrint;
      int p_solventCoordsFreq;
      bool p_writeVelocity;
      bool p_soluteVelocityPrint;
      int p_soluteVelocityFreq;
      bool p_solventVelocityPrint;
      int p_solventVelocityFreq;
      bool p_writeForces;
      bool p_soluteForcesPrint;
      int p_soluteForcesFreq;
      bool p_solventForcesPrint;
      int p_solventForcesFreq;
      bool p_writePolarization;
      int p_polarizationOpts;
      int p_solutePolarizationFreq;
      int p_solventPolarizationFreq;
      bool p_useAvgProperties;
      int p_propertyFreq;
      bool p_pmfData;
      int p_pmfFreq;
};
#endif
