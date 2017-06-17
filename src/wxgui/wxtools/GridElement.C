/**
 * @file
 *
 *
 */

#ifndef WX_PRECOMP
#include "wx/wx.h"
#include "wx/generic/grid.h"
#endif

#include <stdlib.h>
#include <iostream>
  using std::cout;
  using std::cerr;
  using std::endl;


#include "util/StringConverter.H"
#include "util/Preferences.H"
#include "util/PreferenceLabels.H"
#include "dsm/AtomProperty.H"
#include "wxgui/GridElement.H"



GridElement::GridElement() 
{
  p_value = "";
  p_row = 0;
  p_col = 0;
  p_property = new GridProperty;
}


GridElement::GridElement(wxString val, GridProperty *property) 
{
  p_row = 0;
  p_col = 0;
  p_property = new GridProperty(*property);
  setValue(val);
}


GridElement::~GridElement()
{
  if (p_property != 0) {
    delete p_property;
    p_property = 0;
  }
}


int GridElement::compare(GridElement *element,
                         bool caseSensitive)
{
  int ret = 1;

  GridElement *e1 = this;
  GridElement *e2 = element;

  if (e2 != 0) {

    GridProperty *e1_prop = e1->getProperty();
    GridProperty *e2_prop = e2->getProperty();
    wxString e1_val = e1->getValue();
    wxString e2_val = e2->getValue();

    // If properties are different, then do a simple string comparison
    if (!((*e1_prop) == (*e2_prop))) {
      if (caseSensitive) {
        ret = e1_val.Cmp(e2_val);
      }
      else {
        ret = e1_val.CmpNoCase(e2_val);
      }
    }
    else {
      // Comparison based on the property type
      GridProperty::PropertyType propType = e1_prop->getType();
      switch (propType) {
        case GridProperty::PT_RESOURCE_ICON:
        case GridProperty::PT_CDATA:
        case GridProperty::PT_CHECK:
        case GridProperty::PT_RUNSTATE_ICON:
        case GridProperty::PT_ICON:
          if (caseSensitive) {
            ret = e1_val.Cmp(e2_val);
          }
          else {
            ret = e1_val.CmpNoCase(e2_val);
          }
          break;
        case GridProperty::PT_DATE_TIME:
          {
            long e1_dt_val = (e1_val.empty()) ? (0) :
              TDateTime::toSeconds(e1_val.c_str(),
                                   getDateFormatPref(),
                                   getTimeFormatPref());
            long e2_dt_val = (e2_val.empty()) ? (0) :
              TDateTime::toSeconds(e2_val.c_str(),
                                   getDateFormatPref(),
                                   getTimeFormatPref());

            ret = (e1_dt_val < e2_dt_val) ? (-1) :
                  ((e1_dt_val > e2_dt_val) ? (1) : (0)); }
          break;
        case GridProperty::PT_NUMERIC:
        case GridProperty::PT_FLOAT:
          {
            double e1_d_val, e2_d_val;
            StringConverter::toDouble(e1_val.c_str(), e1_d_val);
            StringConverter::toDouble(e2_val.c_str(), e2_d_val);
            ret = (e1_d_val < e2_d_val) ? (-1) :
                  ((e1_d_val > e2_d_val) ? (1) : (0));
          }
          break;
        default:
          if (caseSensitive) {
            ret = e1_val.Cmp(e2_val);
          }
          else {
            ret = e1_val.CmpNoCase(e2_val);
          }
          break;
      }
    }
  }
  return ret;
}


wxString GridElement::getValue()
{
  return p_value;
}


void GridElement::setValue(const wxString& val)
{
  p_value = val;

  // Convert value, if neccessary, based on type
  GridProperty::PropertyType propType =
    p_property->getType();

  if (propType ==  GridProperty::PT_DATE_TIME) {
    p_value = getDateTimeValue();
  }
  if (propType == GridProperty::PT_CHECK) {
    p_value = getCheckValue();
  }
  if (propType == GridProperty::PT_ICON) {
    p_value = getIconValue();
  }
  if (propType == GridProperty::PT_RESOURCE_ICON) {
    p_value = getResourceIconValue();
  }
}


GridProperty *GridElement::getProperty() const
{
  return p_property;
}


GridProperty::PropertyType GridElement::getPropertyType()
{
  return p_property->getType();
}


wxGridCellAttr *GridElement::getAttributes()
{
  wxGridCellAttr *cellAttr = new wxGridCellAttr();

  // Set read only by default
  cellAttr->SetReadOnly();

  // Set to String Renderer
  cellAttr->SetRenderer(new wxGridCellStringRenderer);
  
  return cellAttr;
}


bool GridElement::hasDefaultWidth()
{
  return false;
}


int GridElement::getDefaultWidth()
{
  return 0;
}


int GridElement::getRow() const
{
  return p_row;
}

int GridElement::getCol() const
{
  return p_col;
}


void GridElement::setRow(int row)
{
  p_row = row;
}

void GridElement::setCol(int col)
{
  p_col = col;
}


TDateTime::DateFormat GridElement::getDateFormatPref()
{
  Preferences *eccePref = Preferences::getGlobalPref();
  int dateFormat;
  eccePref->getInt(PrefLabels::DATEFORMAT, dateFormat);

  // Set to a default date format if necessary
  if (!((TDateTime::DateFormat)dateFormat >= TDateTime::AmericanLocale &&
        (TDateTime::DateFormat)dateFormat <= TDateTime::TOTALDATE) ) {
    // Invalid date format - set to default
    dateFormat = TDateTime::AmericanLocale;
  }

  return (TDateTime::DateFormat)dateFormat;
}


TDateTime::TimeFormat GridElement::getTimeFormatPref()
{
  Preferences *eccePref = Preferences::getGlobalPref();
  int timeFormat;
  eccePref->getInt(PrefLabels::TIMEFORMAT, timeFormat);

  // Set to a default time format if necessary
  if (!((TDateTime::TimeFormat)timeFormat >= TDateTime::HM24 &&
        (TDateTime::TimeFormat)timeFormat <= TDateTime::TOTALTIME)) {
    timeFormat = TDateTime::HM24;
  }

  return (TDateTime::TimeFormat)timeFormat;
}


wxString GridElement::getDateTimeValue()
{
  wxString ret = "";

  if (!p_value.empty()) {

    TDateTime::DateFormat dateFormat =
      getDateFormatPref();

    TDateTime::TimeFormat timeFormat =
      getTimeFormatPref();

    TDateTime datetime(p_value.c_str());

    ret = datetime.toString(dateFormat, timeFormat).c_str();
  }

  return ret;
}


wxString GridElement::getCheckValue()
{
  wxString ret = "1";

  if (p_value.empty()) {
    ret = "0";
  }
  if (p_value.Cmp("false") == 0) {
    ret = "0";
  }

  return ret;
}


/**
 * @todo should return the absolute file path for icon
 */
wxString GridElement::getIconValue()
{
  return p_value;
}


wxString GridElement::getResourceIconValue()
{
  return p_value;
}



