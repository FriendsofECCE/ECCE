/**
 * @file
 *
 *
 */
#ifndef CONSTRAINTMODEL_HH
#define CONSTRAINTMODEL_HH


#include <string>
#include <vector>
  using std::string;
  using std::vector;

#include "util/IOException.H"


/**
 */
class ConstraintModel 
{

   public:
      /* Default values */
      static const bool USESHAKE;
      static const int SHAKEOPT;
      static const int MAXSLVITER;
      static const double SLVTOL;
      static const int MAXSLUITER;
      static const int FIXOPTION;
      static const int SELECTOPTION;
      static const double SLUTOL;
      static const double RADIUS;
      static const bool HEAVYONLY;
      static const bool PERMANENT;

      /* Constructors */
      ConstraintModel();

      virtual ~ConstraintModel();

      /* Modifiers */
      void setURL(string url);
      void setUseSHAKE(bool flag);
      void setSHAKEOpt(int opt);
      void setMaxSolventIter(int iter);
      void setSolventTol(double tol);
      void setMaxSoluteIter(int iter);
      void setSoluteTol(double tol);
      void setFixOption(int opt);
      void setSelectOption(int opt);
      void appendFixAtomCmd(string cmd);
      void clearFixAtomCmd();
      void deleteFixAtomCmd(int id);
      void setFixAtomRadius(double rad);
      void setPermanentOption(bool flag);
      void setHeavyAtomsOnly(bool flag);

      /* Accessors */
      string getURL() const;
      bool getUseSHAKE() const;
      int getSHAKEOpt() const;
      int getMaxSolventIter() const;
      double getSolventTol() const;
      int getMaxSoluteIter() const;
      double getSoluteTol() const;
      int getFixOption() const;
      int getSelectOption() const;
      vector<string> getFixAtomCmds() const;
      double getFixAtomRadius() const;
      bool getPermanentOption() const;
      bool getHeavyAtomsOnly() const;

      /**
       *  Reset all values in model back to default values.
       */
      void reset();

   protected:

   private:
     string p_url;
     bool p_useSHAKE;
     int p_SHAKEOpt;
     int p_maxSlvIter;
     double p_slvTol;
     int p_maxSluIter;
     int p_fixOpt;
     int p_selectOpt;
     double p_sluTol;
     double p_radius;
     bool p_permanent;
     bool p_heavyOnly;

     vector<string> p_fixAtoms;

};
#endif
