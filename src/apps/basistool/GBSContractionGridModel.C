#include "util/STLUtil.H"

#include "dsm/TGBSConfig.H"
#include "dsm/TGaussianBasisSet.H"

#include "GBSContractionGridModel.H"


GBSContractionGridModel::GBSContractionGridModel(TGBSConfig *cnfg, string symbol)
{
    vector<const TGaussianBasisSet *> basisSets = cnfg->getGBSList(symbol);
    GBSToContInfoMap *infoMap = cnfg->getContractionInfoMap(symbol);
    GBSContractionGridItem *item;
    p_gridItems = new vector<GBSContractionGridItem *>;

    string uniqueKey;
    GBSToContInfoMap::iterator it;

    p_symbol = symbol;

    vector<int> from;
    vector<int> to;
    p_totalFrom = new vector<int>(TGaussianBasisSet::maxShell+1, 0);
    p_totalTo = new vector<int>(TGaussianBasisSet::maxShell+1, 0);

    int cnt = basisSets.size();

    for (int idx = 0; idx < cnt; idx++)
    {
        const TGaussianBasisSet *gbs = basisSets[idx];
        gbs->contractionInfo(symbol, from, to);
        uniqueKey = gbs->getUniqueKey();
        string gbsname = gbs->p_name;

        if (infoMap != NULL)
        {
            it = infoMap->find(uniqueKey);
        }
        else
        {
            //cout << "infoMap is null for " << tag << endl;
        }

        TGaussianBasisSet::GBSType gbstype = gbs->p_type;

        if ((gbstype != TGaussianBasisSet::appended_tight)
                && (gbstype != TGaussianBasisSet::appended_diffuse))
        {
            for (int jdx = 0; jdx < from.size(); jdx++)
            {
                if (from[jdx] > 0)
                {
                    string shell(TGaussianBasisSet::shell_formatter[jdx]);
                    STLUtil::toLower(shell);

                    string cntrctn = "(" + StringConverter::toString(from[jdx])
                                         + shell + ") / ["
                                         + StringConverter::toString(to[jdx])
                                         + shell + "]";

                    if (it != infoMap->end())
                    {
                        ContractionInfo *info = &((*it).second);
                        item = new GBSContractionGridItem(gbs, cntrctn, jdx, info);
                        p_gridItems->push_back(item);

                        if (from[jdx] == to[jdx])
                        {
                            item->setUncontractAllowed(false);
                        }

                        if (!item->isDeleted())
                        {
                            // then increment the total cont count
                            (*p_totalFrom)[jdx] = (*p_totalFrom)[jdx] + from[jdx];

                            if (item->isUncontracted())
                            {
                                (*p_totalTo)[jdx] = (*p_totalTo)[jdx] + from[jdx];
                            }
                            else
                            {
                                (*p_totalTo)[jdx] = (*p_totalTo)[jdx] + to[jdx];
                            }
                        }

                    }
                }
                else
                {
                    (*p_totalFrom)[jdx] = (*p_totalFrom)[jdx] + from[jdx];
                    (*p_totalTo)[jdx] = (*p_totalTo)[jdx] + to[jdx];
                }
            }
        }
    }
}


GBSContractionGridModel::~GBSContractionGridModel()
{
    p_gridItems->clear();
    delete p_gridItems;
}


int GBSContractionGridModel::getTotalFrom(int shellIdx)
{
    return (*p_totalFrom)[shellIdx];
}


int GBSContractionGridModel::getItemShellIndex(int k)
{
    return (*p_gridItems)[k]->getShellIndex();
}


int GBSContractionGridModel::getTotalTo(int shellIdx)
{
    return (*p_totalTo)[shellIdx];
}


const TGaussianBasisSet *GBSContractionGridModel::getItemBasisSet(int k)
{
    return (*p_gridItems)[k]->getBasisSet();
}


string GBSContractionGridModel::getItemBasisSetName(int k)
{
    return (*p_gridItems)[k]->getBasisSetName();
}


string GBSContractionGridModel::getItemContraction(int k)
{
    return (*p_gridItems)[k]->getContraction();
}



bool GBSContractionGridModel::getItemUncontractAllowed(int k)
{
    return (*p_gridItems)[k]->getUncontractAllowed();
}


bool GBSContractionGridModel::isItemUncontracted(int k)
{
    return (*p_gridItems)[k]->isUncontracted();
}


void GBSContractionGridModel::toggleItemUncontracted(int k)
{
    GBSContractionGridItem *item = (*p_gridItems)[k];
    vector<int> from;
    vector<int> to;

    item->toggleUncontracted();
    item->getBasisSet()->contractionInfo(p_symbol, from, to);

    for (int jdx = 0; jdx < from.size(); jdx++)
    {
        if (!item->isDeleted())
        {
            if (item->isUncontracted())
                (*p_totalTo)[jdx] = (*p_totalTo)[jdx] - from[jdx] + to[jdx];
            else
                (*p_totalTo)[jdx] = (*p_totalTo)[jdx] - to[jdx] + from[jdx];
        }
    }
}

bool GBSContractionGridModel::isItemDeleted(int k)
{
    return (*p_gridItems)[k]->isDeleted();
}


void GBSContractionGridModel::toggleItemDeleted(int k)
{
    GBSContractionGridItem *item = (*p_gridItems)[k];
    vector<int> from;
    vector<int> to;

    item->toggleDeleted();
    item->getBasisSet()->contractionInfo(p_symbol, from, to);

    for (int jdx = 0; jdx < from.size(); jdx++)
    {
        if (item->isDeleted())
        {
            // decrement the total cont counts
            (*p_totalFrom)[jdx] = (*p_totalFrom)[jdx] - from[jdx];

            if (item->isUncontracted())
                (*p_totalTo)[jdx] = (*p_totalTo)[jdx] - from[jdx];
            else
                (*p_totalTo)[jdx] = (*p_totalTo)[jdx] - to[jdx];
        }
        else
        {
            //  increment the total cont count
            (*p_totalFrom)[jdx] = (*p_totalFrom)[jdx] + from[jdx];

            if (item->isUncontracted())
                (*p_totalTo)[jdx] = (*p_totalTo)[jdx] + from[jdx];
            else
                (*p_totalTo)[jdx] = (*p_totalTo)[jdx] + to[jdx];
        }
    }
}


int GBSContractionGridModel::size()
{
    return p_gridItems->size();
}


