/**
 *  @file
 *  @author Ken Swanson
 *
 *  Represents a column within the WxTableView.
 *
 */


#ifndef TABLECOLUMN_HH
#define TABLECOLUMN_HH

#include <string>
    using std::string;

#include "util/Preferences.H"


class TableColumn
{
    public:
        //  Constructor, Destructor
        TableColumn(string label);
        TableColumn(string label, int field);
        ~TableColumn();

        string getLabel();
        void setLabel(string label);

        int getPreferredWidth();
        void setPreferredWidth(int width);

        int getMinimumWidth();
        void setMinMaxWidth(int wmin, int wmax);

        int getMaximumWidth();

        int getTextAlignment();
        void setTextAlignment(int align);

        bool isVisible();
        void setVisible(bool vsbl);

        int getElementField();
        void setElementField(int idx);

        int getSizeAdjustWeight();
        void setSizeAdjustWeight(int weight);

        bool getUserResizeAllowed();
        void setUserResizeAllowed(bool allwd);

        void saveSettings(Preferences *prefs, string topic);
        void loadSettings(Preferences *prefs, string topic);


    private:
        string          p_label;
        int             p_prfWidth;
        int             p_minWidth;
        int             p_maxWidth;
        int             p_textAlign;
        bool            p_visible;
        int             p_elmtField;
        int             p_resizeWeight;

        bool            p_userResizeAllwd;

};

#endif
