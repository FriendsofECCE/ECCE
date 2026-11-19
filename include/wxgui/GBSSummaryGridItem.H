#ifndef GBSSUMMARYGRIDITEM_HH
#define GBSSUMMARYGRIDITEM_HH

#include <string>
    using std::string;
#include <vector>
    using std::vector;

class GBSSummaryGridItem
{
    public:
        enum ItemField { PRIMARY, CONTRACTED, TIGHT, DIFFUSE };

        GBSSummaryGridItem(string orbital);
        ~GBSSummaryGridItem();

        string getOrbital();

        void setFieldValue(ItemField field, int value);
        int getFieldValue(ItemField field);

    private:
        int              p_fieldValues[4];
        string           p_orbital;
};

#endif      //  GBSSUMMARYGRIDITEM_HH
