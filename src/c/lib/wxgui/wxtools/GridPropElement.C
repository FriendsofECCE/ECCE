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

#include <map>
  using std::map;

#include <utility>
  using std::pair;
  using std::make_pair;


#include "util/EcceMap.H"
#include "util/StringConverter.H"
#include "util/InvalidException.H"
#include "util/Preferences.H"
#include "util/PreferenceLabels.H"
#include "util/TDateTime.H"

#include "dsm/ITraversable.H"
#include "dsm/ResourceType.H"
#include "dsm/GridProperty.H"

#include "wxgui/GridElement.H"
#include "wxgui/WxResourceImageList.H"
#include "wxgui/GridCellPropImageRenderer.H"
#include "wxgui/GridPropElement.H"



GridPropElement::GridPropElement(Resource *resource, ResourceProperty *property)
  throw(InvalidException)
{
  // Check to make sure valid Resource and ResourceProperty
  // pointers are provided
  if ( (resource == 0) || (property == 0) ) {
    throw InvalidException("Invalid Resource and/or ResourceProperty"
                           "pointers provided to GridPropElement "
                           "constructor", WHERE);
  }

  // Save local copy of Resource
  p_resource = resource;

  // Save local copy of ResourceProperty
  p_property = new ResourceProperty(*property);

  // Get property value from Resource
  // The original hack added the resource name to the resource's properties as
  // DAV:displayname because it was no longer stored.  This new hack avoids
  // trying to write to a resource which we don't have access to.
  if (property->getName() == "displayname") {
    setValue(resource->getName());
  }
  else {
    setValue(resource->getProp(property->getQName()).c_str());
  }
}


GridPropElement::~GridPropElement()
{}



wxGridCellAttr *GridPropElement::getAttributes()
{
  wxGridCellAttr *cellAttr = new wxGridCellAttr();

  // Set read only
  if (p_property->getReadOnly()) {
    cellAttr->SetReadOnly();
  }

  // Set Editor and Renderer
  switch (p_property->getType()) {
    case GridProperty::PT_CDATA:
    case GridProperty::PT_NUMERIC:
      cellAttr->SetRenderer(new wxGridCellStringRenderer);
      cellAttr->SetEditor(new wxGridCellTextEditor);
      cellAttr->SetAlignment(wxALIGN_LEFT, wxALIGN_CENTER);
      break;
    case GridProperty::PT_DATE_TIME:
      cellAttr->SetRenderer(new wxGridCellStringRenderer);
      cellAttr->SetAlignment(wxALIGN_CENTER, wxALIGN_CENTER);
      break;
    case GridProperty::PT_CHECK:
      cellAttr->SetRenderer(new wxGridCellBoolRenderer);
      break;
    case GridProperty::PT_ICON:
    case GridProperty::PT_RESOURCE_ICON:
    case GridProperty::PT_RUNSTATE_ICON:
      cellAttr->SetRenderer(new GridCellPropImageRenderer);
      break;
    default:
      cellAttr->SetRenderer(new wxGridCellStringRenderer);
      cellAttr->SetEditor(new wxGridCellTextEditor);
  }
  return cellAttr;
}


Resource *GridPropElement::getResource() const 
{
  return p_resource;
}


wxString GridPropElement::getResourceIconValue()
{
  wxString ret = "";

  int imageIdx = WxResourceImageList::getImageIndex(p_resource);
  ret = StringConverter::toString(imageIdx).c_str();
  return ret;
}






