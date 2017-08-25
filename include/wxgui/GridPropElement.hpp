/**
 * @file
 *
 *
 */
#ifndef GRIDPROPELEMENT_HH
#define GRIDPROPELEMENT_HH


#include "dsm/Resource.H"
#include "dsm/ResourceProperty.H"

#include "util/InvalidException.H"

#include "wxgui/GridElement.H"


class GridPropElement : public GridElement 
{
  public:

    GridPropElement(Resource *resource, 
                    ResourceProperty *property) throw(InvalidException);

    virtual ~GridPropElement();

    virtual wxGridCellAttr *getAttributes();

    virtual Resource *getResource() const;

  protected:

    virtual wxString getResourceIconValue();

    Resource *p_resource;

};

#endif

