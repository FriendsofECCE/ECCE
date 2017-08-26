/**
 * @file
 *
 *
 */
#ifndef GRIDPROPELEMENT_HH
#define GRIDPROPELEMENT_HH


#include "dsm/Resource.hpp"
#include "dsm/ResourceProperty.hpp"

#include "util/InvalidException.hpp"

#include "wxgui/GridElement.hpp"


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

