/**
 * @file
 *
 *
 */
#ifndef CONTROLMODEL_HH
#define CONTROLMODEL_HH

#include <string>
  using std::string;
#include <vector>
  using std::vector;

#include "util/IOException.H"


/**
 *  Model for handling parameters from control panel
 */
class ControlModel 
{

   public:
     /* Default values */
     static const bool USEBALANCE;
     static const bool BALANCECELLSZ;
     static const bool BALANCEZONLY;
     static const bool REDISTRIBUTEPAIRS;
     static const double COUPLING;
     static const int BALANCEFREQ;
     static const int SYNCHOPT;
     static const bool ADVCONFIGOPT;
     static const bool PROCCONFIG;
     static const int XPROC;
     static const int YPROC;
     static const int ZPROC;
     static const bool CELLCONFIG;
     static const int XCELL;
     static const int YCELL;
     static const int ZCELL;
     static const bool USEMAXSLV;
     static const int MAXSLV;
     static const bool USEMAXSLU;
     static const int MAXSLU;
     static const bool PACK;


     /* Constructor */
     ControlModel();

     virtual ~ControlModel();

     /**
      *  Modifiers
      */
     void setLoadBalance(bool flag);
     void setBalanceCellSize(bool flag);
     void setBalanceZOnly(bool flag);
     void setRedistributePairs(bool flag);
     void setCouplingStrength(double param);
     void setAdjustFrequency(int freq);
     void setSynchOption(int opt);
     void setAdvancedConfigOpt(bool flag);
     void setProcessorConfig(bool flag);
     void setXProc(int dim);
     void setYProc(int dim);
     void setZProc(int dim);
     void setCellConfig(bool flag);
     void setXCell(int dim);
     void setYCell(int dim);
     void setZCell(int dim);
     void setUseMaxSolvent(bool flag);
     void setMaxSolvent(int num);
     void setUseMaxSolute(bool flag);
     void setMaxSolute(int num);
     void setPackOption(bool flag);

     /**
      *  Accessors
      */
     bool getLoadBalance() const;
     bool getBalanceCellSize() const;
     bool getBalanceZOnly() const;
     bool getRedistributePairs() const;
     double getCouplingStrength() const;
     int getAdjustFrequency() const;
     int getSynchOption() const;
     bool getAdvancedConfigOpt() const;
     bool getProcessorConfig() const;
     int getXProc() const;
     int getYProc() const;
     int getZProc() const;
     bool getCellConfig() const;
     int getXCell() const;
     int getYCell() const;
     int getZCell() const;
     bool getUseMaxSolvent() const;
     int getMaxSolvent() const;
     bool getUseMaxSolute() const;
     int getMaxSolute() const;
     bool getPackOption() const;

     /**
      *  Reset model values to default values
      */
     void reset();

   protected:

   private:
     bool p_loadBalance;
     bool p_balanceCellSize;
     bool p_balanceZOnly;
     bool p_redistributePairs;
     double p_balanceStrength;
     int p_adjustFreq;

     int p_synchOption;

     bool p_advancedConfig;
     bool p_setProcCnfg;
     int p_xProc;
     int p_yProc;
     int p_zProc;
     bool p_setCellCnfg;
     int p_xCell;
     int p_yCell;
     int p_zCell;
     bool p_setMaxSlv;
     int p_MaxSlv;
     bool p_setMaxSlu;
     int p_MaxSlu;
     bool p_pack;
};
#endif
