/**
 * @file
 * @author Ken Swanson
 *
 *  This class is used in conjunction with WxTableView, and contains the
 *  data-centric representation of a table of elements.
 *
 */

#include <stdlib.h>
#include "wxgui/DefaultTableModel.H"

#include "util/StringConverter.H"

/**
 *  Constructor for DefaultTableModel
 *
 *  Create new DefaultTableModel with numCols columns.  Columns are numbered
 *  from 0 to (numcols-1), and initialized with default names.  The index of
 *  the column on which to sort is initialized as 0 (the first column), and
 *  the value of the sortDescending attribute is set to false.  Upon
 *  instantiation, the number of elements within the model is 0.
 */
DefaultTableModel::DefaultTableModel(int numCols)
{
    TableColumn *col;

    for (int j = 0; j < numCols; j++)
    {
        col = new TableColumn("Column " + j);
        col->setElementField(j);
        p_columns.push_back(col);
    }

    p_sortColumn = 0;
    p_sortDesc = false;
}


/**
 *  Destructor for DefaultTableModel
 */
DefaultTableModel::~DefaultTableModel()
{
    int numcols = p_columns.size();

    for (int j = 0; j < numcols; j++)
    {
        delete p_columns[j];
    }

    p_columns.clear();

}


void DefaultTableModel::saveSettings(Preferences *prefs, string topic)
{
    string key = "";
    string subTopic = "";
    int j, numClmns = 0;

    key = topic + ".SortColumnIndex";
    prefs->setInt(key, p_sortColumn);

    key = topic + ".SortDescending";
    prefs->setBool(key, p_sortDesc);

    numClmns = p_columns.size();

    for (j = 0; j < numClmns; j++)
    {
        subTopic = topic + ".Column#" + StringConverter::toString(j);
        p_columns[j]->saveSettings(prefs, subTopic);
    }

}



void DefaultTableModel::loadSettings(Preferences *prefs, string topic)
{
    string key = "";
    string subTopic = "";
    int j, numClmns = 0;

    key = topic + ".SortColumnIndex";

    if (prefs->isDefined(key))
        prefs->getInt(key, p_sortColumn);

    key = topic + ".SortDescending";

    if (prefs->isDefined(key))
        prefs->getBool(key, p_sortDesc);

    numClmns = p_columns.size();

    for (j = 0; j < numClmns; j++)
    {
        subTopic = topic + ".Column#" + StringConverter::toString(j);
        p_columns[j]->loadSettings(prefs, subTopic);
    }
}


/**
 *  Returns the number of elements within the model.
 */
int DefaultTableModel::size()
{
    return p_elmnts.size();
}


/**
 *  Returns the number of columns.
 */
int DefaultTableModel::getColumnCount()
{
    return p_columns.size();
}


/**
 *  Returns the label of the indicated column.
 */
const string DefaultTableModel::getColumnLabel(const int col)
{
    return p_columns[col]->getLabel();
}


/**
 *  Sets the label of the indicated column to label.
 */
void DefaultTableModel::setColumnLabel(const int col, const string label)
{
    p_columns[col]->setLabel(label);
}


TableColumn* DefaultTableModel::getColumn(const int idx)
{
    return p_columns[idx];
}


/**
 *  Indicates whether or not the specified column is visible in the WxTableView.
 */
bool DefaultTableModel::isColumnVisible(const int col)
{
    return p_columns[col]->isVisible();
}


/**
 *  Modifies whether or not the specified column is visible in the WxTableView.
 */
void DefaultTableModel::setColumnVisible(const int col, const bool vsbl)
{
    p_columns[col]->setVisible(vsbl);
}


/**
 *  Indicates the index of the TableElement field referenced by the specified column.
 */
int DefaultTableModel::getColumnElementField(const int col)
{
    return p_columns[col]->getElementField();
}


/**
 *  Modifies index of the TableElement field referenced by the specified column.
 */
void DefaultTableModel::setColumnElementField(const int col, const int field)
{
    p_columns[col]->setElementField(field);
}


/**
 *  Returns a reference to the element at the indicated index
 */
TableElement* DefaultTableModel::elementAt(const int idx)
{
    return p_elmnts[idx];
}


/**
 *  Adds the element to the internal collection of elements.
 *
 *  No verification is made to ensure that the element does
 *  not already exist in the table.
 */
void DefaultTableModel::addElement(TableElement *elmnt)
{
    p_elmnts.push_back(elmnt);
}


/**
 *  Clears all elements from the table.
 */
void DefaultTableModel::removeAllElements()
{
    p_elmnts.clear();
}


/**
 *  Reverses the sort order.
 */
void DefaultTableModel::toggleSortDescending()
{
    p_sortDesc = !p_sortDesc;
}


/**
 *  Sets the index of the sort column to the indicated column
 */
void DefaultTableModel::setSortColumn(const int col)
{
    p_sortColumn = col;
}


/**
 *  Returns the index of the sort column
 */
int DefaultTableModel::getSortColumn()
{
    return p_sortColumn;
}


/**
 *  Returns true or false depending on whether the sortDescending
 *  flag is set.
 */
bool DefaultTableModel::getSortDescending()
{
    return p_sortDesc;
}


void DefaultTableModel::setSortDescending(bool desc)
{
    p_sortDesc = desc;
}


/**
 *  Compares two elements on the current sort column, and adjusts the
 *  result depending on the sortDescending flag.
 *
 *  Applications do not normally need to call this method, as its primary
 *  purpose is to provide functionality for the EcceTableView window.
 */
int DefaultTableModel::compare(TableElement* elmnt1, TableElement* elmnt2)
{
    int result;

    result = elmnt1->compareTo(elmnt2, p_sortColumn);

    if (p_sortDesc)
        result = -result;

    return result;
}
