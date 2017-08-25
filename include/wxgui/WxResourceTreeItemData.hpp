/**
 * @file
 * @author Lisong Sun
 *
 * A subclass derived from wxTreeItemData that contains a Resource pointer.
 * This class will be used by WxResourceTreeCtrl to construct a tree.
 *
 */

#ifndef WXRESOURCETREEITEMDATA_H
#define WXRESOURCETREEITEMDATA_H

#include <string>
using std::string;

#include "wx/treectrl.h"


class Resource;
class EcceURL;

class WxResourceTreeItemData : public wxTreeItemData
{
public:
  WxResourceTreeItemData(Resource * resource = NULL);
  
  virtual ~WxResourceTreeItemData();
  
  virtual Resource * getResource();

  EcceURL getUrl() const;

  string getName() const;
  
protected:
  Resource * p_resource;
};

#endif
