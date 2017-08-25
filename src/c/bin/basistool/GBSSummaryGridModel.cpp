#include "util/STLUtil.H"
#include "util/StringConverter.H"

#include "dsm/TGBSConfig.H"
#include "dsm/TGaussianBasisSet.H"

#include "GBSSummaryGridModel.H"


GBSSummaryGridModel::GBSSummaryGridModel(TGBSConfig *cnfg, string symbol)
{
    p_gridItems = new vector<GBSSummaryGridItem *>;
    GBSSummaryGridItem *item;

    int maxLValue = cnfg->max_l_value(symbol);

    for (int l = 0; l <= maxLValue; l++)
    {
        string s = "";
        s.append(TGaussianBasisSet::shell_formatter[l]);
        STLUtil::toLower(s);

        item = new GBSSummaryGridItem(s);
        p_gridItems->push_back(item);
    }
}


GBSSummaryGridModel::~GBSSummaryGridModel()
{
    p_gridItems->clear();
    delete p_gridItems;
}

int GBSSummaryGridModel::getItemFieldValue(int k, GBSSummaryGridItem::ItemField field)
{
    return ((*p_gridItems)[k])->getFieldValue(field);
}


void GBSSummaryGridModel::setItemFieldValue(int k, GBSSummaryGridItem::ItemField field, int value)
{
    (*p_gridItems)[k]->setFieldValue(field, value);
}

int GBSSummaryGridModel::size()
{
    return p_gridItems->size();
}


string GBSSummaryGridModel::formatItemField(int k, GBSSummaryGridItem::ItemField field)
{
    int value = (*p_gridItems)[k]->getFieldValue(field);

    string s = "";

    //  Primary and contracted always return a non-empty string.  Tight and diffuse
    //  return non-empty string only if value > 0.

    if (((field == GBSSummaryGridItem::PRIMARY)
            || (field == GBSSummaryGridItem::CONTRACTED))
            || (value > 0))
        s = StringConverter::toString(value);

    return s;
}


string GBSSummaryGridModel::getFieldLabel(GBSSummaryGridItem::ItemField field)
{
    string s = "";

    switch (field)
    {
        case GBSSummaryGridItem::PRIMARY:       s = "#Prim";    break;
        case GBSSummaryGridItem::CONTRACTED:    s = "#Cont";    break;
        case GBSSummaryGridItem::TIGHT:         s = "Tight";    break;
        case GBSSummaryGridItem::DIFFUSE:       s = "Diffuse";  break;
    }

    return s;
}

