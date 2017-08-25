#ifndef GBSCONTRACTIONGRIDMODEL_HH
#define GBSCONTRACTIONGRIDMODEL_HH

#include <string>
    using std::string;
#include <vector>
    using std::vector;

#include "GBSContractionGridItem.H"


class GBSContractionGridModel
{
    public:
        GBSContractionGridModel(TGBSConfig *cnfg, string symbol);
        ~GBSContractionGridModel();

        string getItemBasisSetName(int k);
        const TGaussianBasisSet *getItemBasisSet(int k);
        string getItemContraction(int k);

        int getTotalFrom(int shellIdx);
        int getTotalTo(int shellIdx);
        bool getItemUncontractAllowed(int k);
        bool isItemUncontracted(int k);
        void toggleItemUncontracted(int k);
        void toggleItemDeleted(int k);
        int getItemShellIndex(int k);

        //bool setItemUncontracted(int k, bool v);
        bool isItemDeleted(int k);
        //bool setItemDeleted(int k, bool v);
        int size();

    private:
        string                                    p_symbol;
        vector<GBSContractionGridItem *>         *p_gridItems;
        vector<int>                              *p_totalFrom;
        vector<int>                              *p_totalTo;


};

#endif      //  GBSCONTRACTIONGRIDMODEL_HH
