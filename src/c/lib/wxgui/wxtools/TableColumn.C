/**
 *  @file
 *  @author Ken Swanson
 *
 *  Represents a column within the WxTableView.
 *
 */


#include <algorithm>
    using std::min;
    using std::max;

#include <stdlib.h>

#include "wxgui/TableColumn.H"
#include "util/StringConverter.H"


TableColumn::TableColumn(string label)
{
    p_label = label;
    p_prfWidth = 100;
    p_minWidth = p_prfWidth;
    p_maxWidth = p_prfWidth;
    p_textAlign  = 0;
    p_elmtField = 0;
    p_visible = true;
    p_resizeWeight = 0;
    p_userResizeAllwd = true;
}


/**
 *  Allows specification of the label of the column, and the
 *  index of the TableElement field to which it refers.
 *  (Added 2005.1031)
 */
TableColumn::TableColumn(string label, int field)
{
    p_label = label;
    p_prfWidth = 100;
    p_minWidth = p_prfWidth;
    p_maxWidth = p_prfWidth;
    p_textAlign  = 0;
    p_elmtField = field;
    p_visible = true;
    p_resizeWeight = 0;
    p_userResizeAllwd = true;
}


TableColumn::~TableColumn()
{

}


void TableColumn::saveSettings(Preferences *prefs, string topic)
{
    string key = "";

    key = topic + ".PrefWidth";
    prefs->setInt(key, p_prfWidth);
}


void TableColumn::loadSettings(Preferences *prefs, string topic)
{
    string key = "";

    key = topic + ".PrefWidth";

    if (prefs->isDefined(key))
        prefs->getInt(key, p_prfWidth);
}


string TableColumn::getLabel()
{
    return p_label;
}


void TableColumn::setLabel(string label)
{
    p_label = label;
}


int TableColumn::getMinimumWidth()
{
    return p_minWidth;
}


void TableColumn::setMinMaxWidth(int wmin, int wmax)
{
    p_minWidth = wmin;
    p_maxWidth = wmax;

    if (p_prfWidth < p_minWidth)
        p_prfWidth = p_minWidth;

    else if (p_prfWidth > p_maxWidth)
        p_prfWidth = p_maxWidth;
}


int TableColumn::getMaximumWidth()
{
    return p_maxWidth;
}



int TableColumn::getPreferredWidth()
{
    return p_prfWidth;
}


void TableColumn::setPreferredWidth(int width)
{
    if (width < p_minWidth)
        p_prfWidth = p_minWidth;

    else if (width > p_maxWidth)
        p_prfWidth = p_maxWidth;

    else
        p_prfWidth = width;
}


int TableColumn::getSizeAdjustWeight()
{
    return p_resizeWeight;
}


void TableColumn::setSizeAdjustWeight(int weight)
{
    p_resizeWeight = weight;
}


bool TableColumn::getUserResizeAllowed()
{
    return p_userResizeAllwd;
}


void TableColumn::setUserResizeAllowed(bool allwd)
{
    p_userResizeAllwd = allwd;
}


int TableColumn::getTextAlignment()
{
    return p_textAlign;
}


void TableColumn::setTextAlignment(int align)
{
    p_textAlign = align;
}

/**
 *  Indicates whether or not the column is visible in the WxTableView.
 *  (Added 2005.1031)
 */
bool TableColumn::isVisible()
{
    return p_visible;
}

/**
 *  Modifies whether or not the column is visible in the WxTableView.
 *  (Added 2005.1031)
 */
void TableColumn::setVisible(bool vsbl)
{
    p_visible = vsbl;
}


/**
 *  Indicates index of the TableElement field referenced by this column.
 *  (Added 2005.1031)
 */
int TableColumn::getElementField()
{
    return p_elmtField;
}


/**
 *  Modifies index of the TableElement field referenced by this column.
 *  (Added 2005.1031)
 */
void TableColumn::setElementField(int field)
{
    p_elmtField = field;
}
