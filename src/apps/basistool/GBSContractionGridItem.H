#ifndef GBSCONTRACTIONGRIDITEM_HH
#define GBSCONTRACTIONGRIDITEM_HH

#include <string>
    using std::string;
#include <vector>
    using std::vector;

#include "util/STLUtil.H"
#include "util/StringConverter.H"

#include "dsm/TGBSConfig.H"
#include "dsm/TGaussianBasisSet.H"

class GBSContractionGridItem
{
    public:
        GBSContractionGridItem(const TGaussianBasisSet *gbs, string cntrc, int shellIdx, ContractionInfo *info);
        ~GBSContractionGridItem();

        bool getUncontractAllowed();
        void setUncontractAllowed(bool allwd);

        bool getDeleteAllowed();
        void toggleDeleted();
        void toggleUncontracted();
        void setDeleteAllowed(bool allwd);

        bool isUncontracted();
        bool isDeleted();

        string getBasisSetName();
        const TGaussianBasisSet * getBasisSet();
        string getContraction();
        int getShellIndex();


    private:
        ContractionInfo *p_cntrcInfo;
        int p_shellIdx;
        const TGaussianBasisSet   *p_basisSet;
        string              p_cntrctn;
//        bool                p_uncntrct;
        bool                p_uncntrctAllwd;
//        bool                p_delete;
        bool                p_deleteAllwd;

};

#endif      //  GBSCONTRACTIONGRIDITEM_HH
