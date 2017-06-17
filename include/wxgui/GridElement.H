/**
 * @file
 *
 *
 */
#ifndef GRIDELEMENT_HH
#define GRIDELEMENT_HH

#ifndef WX_PRECOMP
#include "wx/wx.h"
#include "wx/generic/grid.h"
#endif

#include "util/TDateTime.H"

#include "dsm/GridProperty.H"


#include <string>
    using std::string;


class wxGridCellAttr;


class GridElement
{
  public:

    GridElement();

    GridElement(wxString val, GridProperty *property);

    virtual ~GridElement();

    virtual int compare(GridElement *element, 
                        bool caseSensitive = false);
    virtual wxString getValue();
    virtual void setValue(const wxString& val);
    virtual GridProperty *getProperty() const;
    virtual GridProperty::PropertyType getPropertyType();
    virtual wxGridCellAttr *getAttributes();


    virtual int getDefaultWidth();
    virtual bool hasDefaultWidth();

    int getRow() const;
    int getCol() const;
    void setRow(int row);
    void setCol(int col);

  protected:

    virtual TDateTime::DateFormat getDateFormatPref();
    virtual TDateTime::TimeFormat getTimeFormatPref();
    virtual wxString getDateTimeValue();
    virtual wxString getCheckValue();
    virtual wxString getIconValue();
    virtual wxString getResourceIconValue();

    wxString p_value;
    int p_row;
    int p_col;
    GridProperty *p_property;

};


#endif
