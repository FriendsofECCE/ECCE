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

#include <wx/panel.h>

#include <vector>
  using std::vector;

#include "util/EcceURL.H"

#include "ContextPanelInterface.H"

class Resource;
class CalcMgr;


ContextPanelInterface::ContextPanelInterface(CalcMgr * calcMgr)
{
  p_calcMgr = calcMgr;
}


ContextPanelInterface::~ContextPanelInterface()
{}


wxWindow * ContextPanelInterface::getWidget()
{
  return dynamic_cast<wxWindow*>(this);
}


vector<EcceURL> ContextPanelInterface::getSelections()
{
  vector<EcceURL> ret;
  return ret;
}


void ContextPanelInterface::clearSelections()
{}


void ContextPanelInterface::loseFocus()
{}


void ContextPanelInterface::selectResource(const EcceURL & url)
{}
