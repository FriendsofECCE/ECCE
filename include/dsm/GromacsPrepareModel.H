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
      // genion -neutral; off leaves the net charge of the structure as it is
      bool neutralise() const { return p_neut; }
      string boxType() const { return p_box; }
      double boxDistance() const { return p_dist; }
      double ionConcentration() const { return p_conc; }
      // genion -rmin (nm): how close an ion may sit to solute and to other ions
      double ionMinDistance() const { return p_rmin; }
      string positiveIon() const { return p_pname; }
      string negativeIon() const { return p_nname; }

      // SOURCE minff: what scripts/minff_topology.py was asked and what it
      // made.  The recipe does not need them (the files are staged); the
      // editor shows them again when the task is reopened.
      string minffVariant() const { return p_mvariant; }
      string minffMineral() const { return p_mmineral; }
      int minffAngleK() const { return p_mangle; }
      string minffReplicate() const { return p_mrep; }
      string minffSubstitutions() const { return p_msubs; }
      string minffSeed() const { return p_mseed; }
      string minffCell() const { return p_mcell; }
      string minffSummary() const { return p_msummary; }
      // min.ff's ion parameter set: HFE or IOD
      string minffIonSet() const { return p_mions; }

      void source(const string& v) { p_source = v; }
      void forceField(const string& v) { p_ff = v; }
      void waterModel(const string& v) { p_water = v; }
      void histidine(const string& v) { p_his = v; }
      void ignoreHydrogens(bool v) { p_ignh = v; }
      void mergeChains(bool v) { p_merge = v; }
      void neutralise(bool v) { p_neut = v; }
      void boxType(const string& v) { p_box = v; }
      void boxDistance(double v) { p_dist = v; }
      void ionConcentration(double v) { p_conc = v; }
      void ionMinDistance(double v) { p_rmin = v; }
      void positiveIon(const string& v) { p_pname = v; }
      void negativeIon(const string& v) { p_nname = v; }
      void minffVariant(const string& v) { p_mvariant = v; }
      void minffMineral(const string& v) { p_mmineral = v; }
      void minffAngleK(int v) { p_mangle = v; }
      void minffReplicate(const string& v) { p_mrep = v; }
      void minffSubstitutions(const string& v) { p_msubs = v; }
      void minffSeed(const string& v) { p_mseed = v; }
      void minffCell(const string& v) { p_mcell = v; }
      void minffSummary(const string& v) { p_msummary = v; }
      void minffIonSet(const string& v) { p_mions = v; }

      /** The defaults; md.gmxprep and gensub carry the same ones. */
      void reset();

      virtual void generateInputFile();
      virtual void run();

   private:
      string p_source, p_ff, p_water, p_his, p_box, p_pname, p_nname;
      string p_mvariant, p_mmineral, p_mrep, p_msubs, p_mseed, p_mcell, p_msummary, p_mions;
      int p_mangle;
      bool p_ignh, p_merge, p_neut;
      double p_dist, p_conc, p_rmin;
};

#endif
