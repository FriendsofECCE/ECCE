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
   p_dist = 1.0;
   p_conc = 0.15;
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
