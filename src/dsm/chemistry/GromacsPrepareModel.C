#include "dsm/GromacsPrepareModel.H"
#include "util/NotImplementedException.H"

GromacsPrepareModel::GromacsPrepareModel()
{
   reset();
}

void GromacsPrepareModel::reset()
{
   p_source = "protein";
   p_ff = "amber99sb-ildn";
   p_water = "tip3p";
   p_his = "auto";
   p_box = "dodecahedron";
   p_pname = "NA";
   p_nname = "CL";
   p_ignh = true;
   p_merge = true;
   p_neut = true;
   p_dist = 1.0;
   p_conc = 0.15;
   p_rmin = 0.6;
   p_mvariant = "gminff";
   p_mmineral = "";
   p_mangle = 500;
   p_mrep = "1 1 1";
   p_msubs = "";
   p_mseed = "";
   p_mcell = "";
   p_msummary = "";
   p_mions = "HFE";
}

GromacsPrepareModel::~GromacsPrepareModel()
{
}

void GromacsPrepareModel::generateInputFile()
{
   throw NotImplementedException("generateInputFile not implemented", WHERE);
}

void GromacsPrepareModel::run()
{
   throw NotImplementedException("run not implemented", WHERE);
}
