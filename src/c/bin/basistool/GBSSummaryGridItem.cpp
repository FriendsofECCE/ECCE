
#include "GBSSummaryGridItem.H"


GBSSummaryGridItem::GBSSummaryGridItem(string orbital)
{
    p_orbital = orbital;

    for (int i = 0; i < 4; i++)
    {
        p_fieldValues[i] = 0;
    }
}


GBSSummaryGridItem::~GBSSummaryGridItem()
{
}

string GBSSummaryGridItem::getOrbital()
{
    return p_orbital;
}


void GBSSummaryGridItem::setFieldValue(ItemField field, int value)
{
    
    p_fieldValues[field] = value;
}


int GBSSummaryGridItem::getFieldValue(ItemField field)
{
    return p_fieldValues[field];
}

