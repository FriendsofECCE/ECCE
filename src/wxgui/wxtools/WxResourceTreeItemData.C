/**
 * @file
 * @author Lisong Sun
 *
 * A subclass derived from wxTreeItemData that contains a Resource pointer.
 * This class will be used by WxResourceTreeCtrl to construct a tree.
 *
 */


#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "ResourceTreeItemData.C"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <iostream>
using std::cerr;
using std::endl;

#include "wx/treectrl.h"

#include "dsm/Resource.H"

#include "wxgui/WxResourceTreeItemData.H"


/**
 * Constructor.
 * Only contains a Resource * as its data member.
 */
WxResourceTreeItemData::WxResourceTreeItemData(Resource * resource)
{
  p_resource = resource;
  //  cerr << "Create wx resource node: " << getUrl().toString()
  //       << endl;
}

/**
 * Destructor.
 * Do not delete p_resource here, the p_resource is deleted by its parent
 * Resource object.
 */
WxResourceTreeItemData::~WxResourceTreeItemData()
{
  //  cerr << "Delete wx resource node: " << getUrl().toString()
  //       << endl;
  p_resource = NULL;
}


/**
 * Get the Resource object associated with the node.
 */
Resource * WxResourceTreeItemData::getResource()
{
  return p_resource;
}


/**
 * Get the URL of current node.
 * The hidden root will have a NULL p_resource.
 */
EcceURL WxResourceTreeItemData::getUrl() const
{
  return p_resource ? p_resource->getURL() : EcceURL();
}


/**
 * Get the name of current node.
 * The hidden root will have a "" name.
 */
string WxResourceTreeItemData::getName() const
{
  return p_resource ? p_resource->getName(): "";
}
