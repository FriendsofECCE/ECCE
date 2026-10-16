/**
 * @file
 */
#ifndef GROMACSPREPAREMODEL_HH
#define GROMACSPREPAREMODEL_HH

#include <string>
  using std::string;

#include "tdat/ITaskModel.H"

/**
 * The recipe of a GROMACS Prepare task: what pdb2gmx, editconf, solvate and
 * genion are asked for.  Its XML form is read by scripts/parsers/md.gmxprep,
 * which checks the values; the defaults here and there must agree.
 */
class GromacsPrepareModel : public ITaskModel
{
   public:
      GromacsPrepareModel();
      virtual ~GromacsPrepareModel();

      string source() const { return p_source; }
      string forceField() const { return p_ff; }
      string waterModel() const { return p_water; }
      string histidine() const { return p_his; }
      bool ignoreHydrogens() const { return p_ignh; }
      bool mergeChains() const { return p_merge; }
      string boxType() const { return p_box; }
      double boxDistance() const { return p_dist; }
      double ionConcentration() const { return p_conc; }
      string positiveIon() const { return p_pname; }
      string negativeIon() const { return p_nname; }

      void source(const string& v) { p_source = v; }
      void forceField(const string& v) { p_ff = v; }
      void waterModel(const string& v) { p_water = v; }
      void histidine(const string& v) { p_his = v; }
      void ignoreHydrogens(bool v) { p_ignh = v; }
      void mergeChains(bool v) { p_merge = v; }
      void boxType(const string& v) { p_box = v; }
      void boxDistance(double v) { p_dist = v; }
      void ionConcentration(double v) { p_conc = v; }
      void positiveIon(const string& v) { p_pname = v; }
      void negativeIon(const string& v) { p_nname = v; }

      /** The defaults; md.gmxprep and gensub carry the same ones. */
      void reset();

      virtual void generateInputFile();
      virtual void run();

   private:
      string p_source, p_ff, p_water, p_his, p_box, p_pname, p_nname;
      bool p_ignh, p_merge;
      double p_dist, p_conc;
};

#endif
