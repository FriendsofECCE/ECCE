
#include "GBSContractionGridItem.H"


GBSContractionGridItem::GBSContractionGridItem(const TGaussianBasisSet *gbs, string cntrc, int shellIdx, ContractionInfo *info)
{
    p_cntrcInfo = info;
    p_basisSet = gbs;
    p_shellIdx = shellIdx;
    p_cntrctn = cntrc;

  //  p_uncntrct = false;
    p_uncntrctAllwd = true;
 //   p_delete = false;
    p_deleteAllwd = true;
}


GBSContractionGridItem::~GBSContractionGridItem()
{
}

string GBSContractionGridItem::getBasisSetName()
{
    return p_basisSet->p_name;
}


const TGaussianBasisSet * GBSContractionGridItem::getBasisSet()
{
    return p_basisSet;
}


string GBSContractionGridItem::getContraction()
{
    return p_cntrctn;
}


bool GBSContractionGridItem::getUncontractAllowed()
{
    return p_uncntrctAllwd;
}


void GBSContractionGridItem::setUncontractAllowed(bool allwd)
{
    p_uncntrctAllwd = allwd;
}


bool GBSContractionGridItem::getDeleteAllowed()
{
    return p_deleteAllwd;
}


void GBSContractionGridItem::setDeleteAllowed(bool allwd)
{
    p_deleteAllwd = allwd;
}



bool GBSContractionGridItem::isUncontracted()
{
    return p_cntrcInfo->uncontractFlags[p_shellIdx];
//    return p_uncntrct;
}

/*
void GBSContractionGridItem::setUncontracted(bool v)
{
    p_uncntrct = v;
}
*/

bool GBSContractionGridItem::isDeleted()
{
    return p_cntrcInfo->deleteFlags[p_shellIdx];
//    return p_delete;
}

void GBSContractionGridItem::toggleDeleted()
{
    p_cntrcInfo->deleteFlags[p_shellIdx] = !(p_cntrcInfo->deleteFlags[p_shellIdx]);
}


void GBSContractionGridItem::toggleUncontracted()
{
    p_cntrcInfo->uncontractFlags[p_shellIdx] = !(p_cntrcInfo->uncontractFlags[p_shellIdx]);
}


int GBSContractionGridItem::getShellIndex()
{
    return p_shellIdx;
}

/*
void GBSContractionGridItem::setDeleted(bool v)
{
    p_delete = v;
}
*/
