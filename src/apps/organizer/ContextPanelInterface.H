/**
 * @file
 * @author Lisong Sun
 *
 * The interface class for all the ContextPanel classes.
 *
 * All the different context panels that will show up in the wxcalcmgr are
 * subclassed from both this class and their own GUI base class. This class
 * itself doesn't have any implementation in it. The purpose of this design
 * is to let the wxcalcmgr having an uniform interface to the context panel.
 *
 */

#ifndef _CONTEXTPANEL_HH
#define _CONTEXTPANEL_HH

#include <vector>
  using std::vector;

#include "util/EcceURL.H"

#define wxID_PANEL_SELECTION 10200

class WxResourceTreeItemData;
class CalcMgr;
class Resource;
class wxWindow;
class ContextPanelFactory;
class EcceURL;

class ContextPanelInterface
{
public:

  friend class ContextPanelFactory;

  virtual ~ContextPanelInterface();

  virtual void refresh() = 0;

  virtual wxWindow * getWidget();

  virtual vector<EcceURL> getSelections();

  virtual void clearSelections();

  virtual void loseFocus();

  virtual void selectResource(const EcceURL & url);
  
protected:

  ContextPanelInterface(CalcMgr * calcMgr);

  ContextPanelInterface();

  CalcMgr * p_calcMgr;
};

#endif
