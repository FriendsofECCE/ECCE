#include "dsm/IPropCalculation.H"
#include "dsm/ICalculation.H"

#include "viz/ShowFragCmd.H"

#include "VizPropertyPanel.H"


IMPLEMENT_DYNAMIC_CLASS(VizPropertyPanel, PropertyPanel)


set<VizPropertyPanel*> VizPropertyPanel::p_allVizPropertyPanels;
map<string, set<VizPropertyPanel*> > VizPropertyPanel::p_vizPropertyPanels;


VizPropertyPanel::VizPropertyPanel()
  : PropertyPanel(),
    WxVizTool(),
    p_hasFocus(false),
    p_isPinned(false)
{
}


VizPropertyPanel::VizPropertyPanel(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
  : PropertyPanel(),
    WxVizTool(),
    p_hasFocus(false),
    p_isPinned(false)
{
  Create(calculation, parent, id, pos, size, style, name);
}


bool VizPropertyPanel::Create(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
{
  if (!PropertyPanel::Create(calculation, parent, id, pos, size, style, name)) {
    wxFAIL_MSG( wxT("VizPropertyPanel creation failed") );
    return false;
  }

  p_allVizPropertyPanels.insert(this);
  p_vizPropertyPanels[calculation->getURL().toString()].insert(this);

  return true;
}


VizPropertyPanel::~VizPropertyPanel()
{
  // due to the isRelevant check in PropertyPanel, we may not really have
  // a calculation at this point...
  if (getCalculation()) {
    p_vizPropertyPanels[getCalculation()->getURL()].erase(this);
    p_allVizPropertyPanels.erase(this);
  }
}


bool VizPropertyPanel::hasFocus() const
{
  return p_hasFocus;
}


bool VizPropertyPanel::isPinned() const
{
  return p_isPinned;
}


set<VizPropertyPanel*> VizPropertyPanel::getPanels(const string& url,
                                                   const string& name)
{
  set<VizPropertyPanel*> ret;

  if (url.empty()) {
    ret = p_allVizPropertyPanels;
  } else {
    ret = p_vizPropertyPanels[url];
    if (!name.empty()) {
      set<VizPropertyPanel*>::iterator it;
      for (it = ret.begin(); it != ret.end(); /*no inc*/) {
        if ((*it)->getName() != name) {
          ret.erase(it++);
        } else {
          ++it;
        }
      }
    }
  }

  return ret;
}


void VizPropertyPanel::setFocus(const bool& value)
{
  doFocus(value);
}


void VizPropertyPanel::setPinned(const bool& value)
{
  doPin(value);
}


void VizPropertyPanel::receiveFocus()
{
}


void VizPropertyPanel::loseFocus()
{
}


void VizPropertyPanel::doFocus(const bool& value)
{
  ensureInitialized();

  if (value) {
    // since this panel is "taking focus", make all other panels lose theirs
    // only if they currently have focus (saves from spurious loseFocus calls)
    set<VizPropertyPanel*> panels =
            getPanels(getCalculation()->getURL().toString());
    set<VizPropertyPanel*>::iterator panelIt;
    for (panelIt = panels.begin(); panelIt != panels.end(); ++panelIt) {
      if ((*panelIt)->hasFocus())
        (*panelIt)->setFocus(false);
    }
    receiveFocus();
  } else {
    p_isPinned = false;
    loseFocus();
  }
  p_hasFocus = value;
}


void VizPropertyPanel::doPin(const bool& value)
{
  ensureInitialized();

  if (value) {
    // since this panel is "adding focus", make non-pinned panels lose theirs
    // only if they currently have focus (saves from spurious loseFocus calls)
    set<VizPropertyPanel*> panels =
            getPanels(getCalculation()->getURL().toString());
    set<VizPropertyPanel*>::iterator panelIt;
    for (panelIt = panels.begin(); panelIt != panels.end(); ++panelIt) {
      if (!(*panelIt)->isPinned() && (*panelIt)->hasFocus()) {
        (*panelIt)->setFocus(false);
      }
    }
    p_hasFocus = true;
    receiveFocus();
  } else {
    p_hasFocus = false;
    loseFocus();
  }
  p_isPinned = value;
}

/**
 * The vector display uses the last step for the vector display.
 * @param index -1 means last; -2 means original
 */
void VizPropertyPanel::selectFragStep(int index)
{
   WxVizToolFW& fw = getFW();
   SGContainer& sg = fw.getSceneGraph();
   IPropCalculation *expt = getCalculation();
   ShowFragCmd *cmd = new ShowFragCmd("Show Fragment", &sg, expt);
   cmd->getParameter("index")->setInteger(index);
   fw.execute(cmd);
}


/** 
 * Decide if the data is relevent based on the code, runtype, and properties.
 * This code is an attempt to put all of our hardwired logic in one place.
 * Panel isRelevent functions can call this method.
 *
 * Code and runtypes are compared as substrings.
 * Any pnames match consitutes a match (or)
 */
bool VizPropertyPanel::isRelevent(IPropCalculation *expt, 
                                  const string& code,
                                  const string& runtype,
                                  const set<string>& pnames) const
{
   bool ret = true;

   ICalculation *escalc = dynamic_cast<ICalculation*>(expt);
   if (escalc) {
      const JCode* app = escalc->application();
      string runtype = escalc->runtype().name();
      if (app->name().find(code) != string::npos &&
          runtype.find(runtype) == 0) {
         // Now check the properties
         set<string> keys = getPropertyNames();
         set<string>::const_iterator pnamesIt;
         set<string>::const_iterator keysIt;
         for (pnamesIt = pnames.begin(); ret && pnamesIt != pnames.end(); pnamesIt++) {
           for (keysIt = keys.begin(); keysIt != keys.end(); keysIt++) {
              if (*keysIt == *pnamesIt) {
                 ret = false;
                 break;
              }
           }
         }
      }
   }

   return ret;
}

