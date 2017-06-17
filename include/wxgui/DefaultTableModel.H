/**
 * @file
 * @author Ken Swanson
 *
 *  This class is used in conjunction with WxTableView, and contains the
 *  data-centric representation of a table of elements.
 *
 */


#ifndef DEFAULTTABLEMODEL_HH
#define DEFAULTTABLEMODEL_HH

#include <string>
    using std::string;
#include <vector>
    using std::vector;

#include "util/Preferences.H"

#include "wxgui/TableElement.H"
#include "wxgui/TableColumn.H"


class DefaultTableModel
{
    public:
        DefaultTableModel(int numCols);
        ~DefaultTableModel();

        int size();
        int getColumnCount();
        const string getColumnLabel(int col);
        void setColumnLabel(const int col, const string label);

        TableColumn* getColumn(const int idx);
        void addElement(TableElement *elmnt);
        TableElement* elementAt(const int idx);
        void removeAllElements();
        void toggleSortDescending();
        void setSortColumn(const int col);
        int getSortColumn();
        bool getSortDescending();
        void setSortDescending(bool desc);
        int compare(TableElement* elmnt1, TableElement* elmnt2);

        bool isColumnVisible(const int col);
        void setColumnVisible(const int col, const bool vsbl);

        int getColumnElementField(const int col);
        void setColumnElementField(const int col, const int field);

        void saveSettings(Preferences *prefs, string topic);
        void loadSettings(Preferences *prefs, string topic);


    protected:
        vector<TableColumn*>    p_columns;

        int                     p_sortColumn;
        bool                    p_sortDesc;
        vector<TableElement*>   p_elmnts;

    private:


};

#endif
