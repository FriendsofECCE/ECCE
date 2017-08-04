#ifndef GBSSUMMARYGRIDMODEL_HH
#define GBSSUMMARYGRIDMODEL_HH

#include <string>
    using std::string;
#include <vector>
    using std::vector;

#include "GBSSummaryGridItem.H"


class GBSSummaryGridModel
{
    public:
        GBSSummaryGridModel(TGBSConfig *cnfg, string symbol);
        ~GBSSummaryGridModel();

//        GBSSummaryGridItem *getItem(int k);
        int getItemFieldValue(int k, GBSSummaryGridItem::ItemField field);
        void setItemFieldValue(int k, GBSSummaryGridItem::ItemField field, int value);
        string formatItemField(int k, GBSSummaryGridItem::ItemField field);

        static string getFieldLabel(GBSSummaryGridItem::ItemField field);
        int size();

   ///     static GBSSummaryGridItem::ItemField getItemField();

    private:
        vector<GBSSummaryGridItem *>         *p_gridItems;

};

#endif      //  GBSSUMMARYGRIDMODEL_HH
