/**
 * @file
 *
 * Periodic table panel class.
 *
 *
 */

#include <iomanip>
using std::ios;
#include <fstream>
using std::ofstream;
using std::ifstream;

#include "dsm/ResourceDescriptor.H"

#include "tdat/TPerTab.H"

#include "util/Ecce.H"
#include "util/Event.H"
#include "util/EventDispatcher.H"
#include "util/JMSMessage.H"
#include "util/JMSPublisher.H"
#include "util/Listener.H"
#include "util/Preferences.H"

#include "wxgui/ewxMenu.H"
#include "wxgui/PerTabPanel.H"
#include "wxgui/PTDataFrame.H"

using namespace ecce;

BEGIN_EVENT_TABLE( PerTabPanel, ewxPanel )
  
  EVT_CONTEXT_MENU( PerTabPanel::OnContextMenu )
  EVT_MENU( ID_ITEM_PHYS,	PerTabPanel::OnMenuClick )
  EVT_MENU( ID_ITEM_METAL,	PerTabPanel::OnMenuClick )
  EVT_MENU( ID_ITEM_DEFAULT,	PerTabPanel::OnMenuClick )
  EVT_MENU( ID_ITEM_USER,	PerTabPanel::OnMenuClick )
  EVT_MENU( ID_ITEM_SHOWNUM,	PerTabPanel::OnMenuClick )
  EVT_MENU( ID_ITEM_SHOWDATA,	PerTabPanel::OnMenuClick )
  
END_EVENT_TABLE()


// Default Colors
wxColour PerTabPanel::p_physColor[4] =
{
  wxColour(191, 191, 191),	// solidElement color
  wxColour(222, 198, 198),	// gasElement color
  wxColour(198, 214, 231),	// liquidElement color
  wxColour(255, 255, 198)	// syntheticElement color
};


wxColour PerTabPanel::p_metalColor[3] =
{
  wxColour(198, 214, 231),	// nonMetallic color
  wxColour(140, 185, 190),	// semiMetallic color
  wxColour(191, 191, 191)	// metallic color
};


wxColour PerTabPanel::p_contextColor = wxColour(255, 165, 0);


string PerTabPanel::p_prefFile = "PerTable";


PerTabPanel::PerTabPanel(wxWindow* parent,
                         bool isMultiSelect,
                         int colorScheme,
                         bool showNumber,
                         bool showGhost,
                         bool isMini,
                         wxWindowID id,
                         const wxPoint& pos,
                         const wxSize& size,
                         long style)
  : ewxPanel( parent, id, pos, size, style ),
    Listener("Periodic Table")
{
  subscribe();
  
  createControls(isMultiSelect, colorScheme, showNumber, showGhost, isMini);
}


PerTabPanel::PerTabPanel(wxWindow* parent,
                         wxWindowID id, const wxPoint& pos,
                         const wxSize& size, long style)
  : ewxPanel( parent, id, pos, size, style ),
    Listener("Periodic Table")
{
  subscribe();

  createControls(true, ID_ITEM_DEFAULT, true, false, false);
}


void PerTabPanel::createControls(bool isMultiSelect,
                                 int colorScheme,
                                 bool showNumber,
                                 bool showGhost,
                                 bool isMini)
{
  SetAutoLayout(false);

  // Initialize global data
  p_tPerTab = new TPerTab(false);
  p_refTPerTab = new TPerTab(true);
  p_prefs = new Preferences(p_prefFile);
  p_numAtoms = p_refTPerTab->numAtoms();


  // Copy parameters
  p_isMultiSelect = isMultiSelect;
  p_colorScheme = colorScheme;
  p_showNumber = showNumber && !isMini;
  p_isMini = isMini;
  p_selectCount = 0;
  p_currentElement = 0;
  p_elementInfo.reserve(p_numAtoms);

  int i;
  for (i=0; i<p_numAtoms; i++) {
    p_elementInfo[i] = new ElementInfo;
    p_elementInfo[i]->cornerMark = false;
    p_elementInfo[i]->defaultColor = p_refTPerTab->colorName(i);
  }

  mergeUserElementPreferences();

  // Set the top sizer to be a box sizer
  // Only one component will be added
  // Use the sizer to support add-on's for future development
  // eg. insert a status area at the bottom of the panel
  p_topSizer = new wxBoxSizer(wxVERTICAL);
  this->SetSizer(p_topSizer);

  
  // Setup Element Sizer
  // Capable to insert components at an arbitrary coordinate
  p_elementSizer = new wxGridBagSizer(1, 1);
  int padding = 15;
  if (p_isMini)
    padding = 2;
  p_topSizer->Add(p_elementSizer, 1, wxEXPAND|wxALL|wxALIGN_CENTER, padding);

  p_elementSizer->SetEmptyCellSize(wxSize(1,1));

  // Container of all the Element Buttons
  p_elements = new ElementButton * [p_numAtoms];

  // Insert Element Buttons into the Element Sizer
  // Deliberately leave row 7 empty for the gap
  for (i=0; i<p_numAtoms; i++) {
    p_elements[i] = new ElementButton(i, this, ID_ELEMENTBASE+i);
    p_elementSizer->Add((wxWindow *)(p_elements[i]),
                        wxGBPosition(p_refTPerTab->row(i)+p_refTPerTab->row(i)/8-1,
                                     p_refTPerTab->column(i)-1),
                        wxGBSpan(1, 1), wxEXPAND|wxALL|wxALIGN_CENTER, 0);

    p_elementInfo[i]->covalentRadius = p_tPerTab->covalentRadius(i);
    p_elementInfo[i]->vanderWaalsRadius = p_tPerTab->vwr(i);
    p_elementInfo[i]->bondFormationTolerance = p_tPerTab->covalentRadiusTolerance(i);
  }

  // Make button columns growable with uniform size
  for (i=0; i<18; i++) {
    p_elementSizer->AddGrowableCol(i, 1);
  }

  // Make button rows growable with uniform size
  for (i=0; i<10; i++) {
    // Set the empty row quarter size of a regular row
    if (i == 7)
      p_elementSizer->AddGrowableRow(i, 1);
    else
      p_elementSizer->AddGrowableRow(i, 4);
  }
  
  restoreSettings(* p_prefs);
  // Create long version right click menu
  // Will be appended to the menu bar in the parent frame
  p_longViewMenu = new ewxMenu;
  //  p_longViewMenu->Append(ID_ITEM_UNIFORM, "Uniform Color", "", wxITEM_RADIO);
  p_longViewMenu->Append(ID_ITEM_PHYS, "Physical State", "", wxITEM_RADIO);
  p_longViewMenu->Append(ID_ITEM_METAL, "Metalic State", "", wxITEM_RADIO);
  p_longViewMenu->Append(ID_ITEM_DEFAULT, "Default Colors", "", wxITEM_RADIO);
  p_longViewMenu->Append(ID_ITEM_USER, "Custom Colors", "", wxITEM_RADIO);
  p_longViewMenu->Check(p_colorScheme, TRUE);
  p_longViewMenu->AppendSeparator();
  p_longViewMenu->Append(ID_ITEM_SHOWNUM, "Show Numbers", "", wxITEM_CHECK);
  p_longViewMenu->Check(ID_ITEM_SHOWNUM, p_showNumber);
  p_longViewMenu->AppendSeparator();
  p_longViewMenu->Append(ID_ITEM_SHOWDATA, "Atomic Data...", "", wxITEM_NORMAL);

  // Create short version right click menu
  // Will be appended to the menu bar in the parent frame
  p_shortViewMenu = new ewxMenu;
  //  p_shortViewMenu->Append(ID_ITEM_UNIFORM, "Uniform Color", "", wxITEM_RADIO);
  p_shortViewMenu->Append(ID_ITEM_PHYS, "Physical State", "", wxITEM_RADIO);
  p_shortViewMenu->Append(ID_ITEM_METAL, "Metalic State", "", wxITEM_RADIO);
  p_shortViewMenu->Append(ID_ITEM_DEFAULT, "Default Colors", "", wxITEM_RADIO);
  p_shortViewMenu->Append(ID_ITEM_USER, "Custom Colors", "", wxITEM_RADIO);
  p_shortViewMenu->Check(p_colorScheme, TRUE);
  p_shortViewMenu->AppendSeparator();
  p_shortViewMenu->Append(ID_ITEM_SHOWNUM, "Show Numbers", "", wxITEM_CHECK);
  p_shortViewMenu->Check(ID_ITEM_SHOWNUM, p_showNumber);

  // Create per-element version right click menu
  p_dataMenu = new ewxMenu;
  p_dataMenu->Append(ID_ITEM_SHOWDATA, "Atomic Data...", "", wxITEM_NORMAL);

  recolorTable();
  SetAutoLayout(TRUE);

  p_elements[0]->Show(showGhost);
  p_elements[p_numAtoms-1]->Show(false);

  p_dataFrame = new PTDataFrame(0, this);
  p_dataFrame->Show(false);
}


PerTabPanel::~PerTabPanel()
{
  unsubscribe();
  p_elementInfo.clear();
  delete p_tPerTab;
  delete p_refTPerTab;
  delete p_prefs;
}


// Get the long version right click menu
ewxMenu * PerTabPanel::getLongViewMenu() const
{
  return p_longViewMenu;
}


// Get the short version right click menu
ewxMenu * PerTabPanel::getDataMenu() const
{
  return p_dataMenu;
}


// Whether show the number on the Element Button
bool PerTabPanel::isShowNumber()
{
  return p_showNumber;
}


// Get Element Button's triangle mark info
bool PerTabPanel::isShowMark(int atomicNum)
{
  return p_elementInfo[atomicNum]->cornerMark;
}


/**
 *  Show or hide the mark on the button corresponding to the
 *  elements indicated by the atomic number.
 */
void PerTabPanel::showMark(int atomicNum, bool show)
{
    p_elementInfo[atomicNum]->cornerMark = show;
}

/**
 *  Show marks on the buttons corresponding to the elements indicated
 *  by the contents of the symbols vector.  This method does not
 *  clear any existing marks.
 */
void PerTabPanel::showMarks(vector<const char*> *symbols)
{
    int atomicNumber;

    for (size_t i = 0; i < symbols->size(); i++)
    {
        atomicNumber = p_refTPerTab->atomicNumber((*symbols)[i]);
        p_elementInfo[atomicNumber]->cornerMark = true;
    }
}

/**
 *  Clear all marks.
 */
void PerTabPanel::resetMarks()
{
    for (int i = 0; i < p_numAtoms; i++)
    {
         p_elementInfo[i]->cornerMark = false;
    }
}


void PerTabPanel::fireSelectionChanged(int atomicNumber)
{
  if (p_isMini) {
    // Let other listeners know selection changed
    Event event("ElementChanged",
                p_refTPerTab->atomicSymbol(p_currentElement));
    EventDispatcher::getDispatcher().publish(event);
  }
  // we, the PerTabPanel, don't care about this event, but our parent might
  wxCommandEvent event(wxEVT_COMMAND_BUTTON_CLICKED, GetId());
  GetEventHandler()->ProcessEvent(event);
}


// Attached the long version right click menu
void PerTabPanel::OnContextMenu(wxContextMenuEvent& event)
{
  wxWindow *gparent;
  string gpname = "";
  if ((gparent = this->GetGrandParent()) != NULL) {
    gpname = gparent->GetName();
  }

  PopupMenu(gpname=="popup"? p_shortViewMenu: p_longViewMenu);
}


// Handle the menu events
// Mainly change color scheme and refresh Element Buttons
void PerTabPanel::OnMenuClick(wxCommandEvent& event)
{
  switch (event.GetId()) {
  case ID_ITEM_PHYS:
  case ID_ITEM_METAL:
  case ID_ITEM_DEFAULT:
  case ID_ITEM_USER:
    p_colorScheme = event.GetId();
    recolorTable();
    break;
  case ID_ITEM_SHOWNUM:
    p_showNumber = event.IsChecked();
    recolorTable();
    break;
  case ID_ITEM_SHOWDATA:
    p_dataFrame->Center();
    p_dataFrame->Show(true);
    p_dataFrame->Raise();
    break;
  default:
    event.Skip();
  }
  saveSettings(*p_prefs);
}


// Repaint each Element Button
// Need to be called when color scheme is changed or mark scheme is changed
void PerTabPanel::recolorTable()
{
  for (int i=0; i<p_numAtoms; i++) {
    p_elements[i]->clearSizeInfo();
    p_elements[i]->Refresh();
  }

  SetMinSize(GetBestSize());
}


// Get the element button's color based on the color scheme and the status of the button
wxColour PerTabPanel::getElementColor(int atomicNum, bool isSunken)
{
  // color offset is used to signify the difference of sunken and raised buttons
  float colorOffset = isSunken ? 0.8 : 1.0;
  
  switch (p_colorScheme) {
  case ID_ITEM_PHYS:
    return offsetColor(p_physColor[p_tPerTab->state(atomicNum)], colorOffset);

  case ID_ITEM_METAL:
    return offsetColor(p_metalColor[p_tPerTab->metal(atomicNum)], colorOffset);

  case ID_ITEM_DEFAULT:
    return offsetColor((ewxColor) (p_elementInfo[atomicNum]->defaultColor), colorOffset);

  case ID_ITEM_USER:
    return offsetColor((ewxColor) (p_elementInfo[atomicNum]->userColor), colorOffset);
  }
  // Should never reach here.
  return wxColour("BLACK");
}


// Get the triangle mark's color
wxColour PerTabPanel::getContextColor(bool isSunken)
{
  return offsetColor( p_contextColor, isSunken ? 0.8 : 1.0);
}


// Get default and user color preferences, used by pop-up info dialog
wxColour PerTabPanel::getDefaultElementColor(int atomicNum) const
{
  return (ewxColor) (p_elementInfo[atomicNum]->defaultColor);
}


wxColour PerTabPanel::getUserElementColor(int atomicNum) const
{
  return (ewxColor) (p_elementInfo[atomicNum]->userColor);
}


double PerTabPanel::getDefaultCovRad(int atomicNum) const
{
  return p_refTPerTab->covalentRadius(atomicNum);
}


// Get default and user radii preferences, used by pop-up info dialog
double PerTabPanel::getDefaultVdwRad(int atomicNum) const
{
  return p_refTPerTab->vwr(atomicNum);
}


double PerTabPanel::getDefaultBft(int atomicNum) const
{
  return p_refTPerTab->covalentRadiusTolerance(atomicNum);
}


double PerTabPanel::getUserCovRad(int atomicNum) const
{
  return p_elementInfo[atomicNum]->covalentRadius;
}


double PerTabPanel::getUserVdwRad(int atomicNum) const
{
  return p_elementInfo[atomicNum]->vanderWaalsRadius;
}


double PerTabPanel::getUserBft(int atomicNum) const
{
  return p_elementInfo[atomicNum]->bondFormationTolerance;
}


// Set radii using default values
//   Save the change into preference file
void PerTabPanel::resetElementRadii(int atomicNum)
{
  p_elementInfo[atomicNum]->covalentRadius = getDefaultCovRad(atomicNum);
  p_elementInfo[atomicNum]->vanderWaalsRadius = getDefaultVdwRad(atomicNum);
  p_elementInfo[atomicNum]->bondFormationTolerance = getDefaultBft(atomicNum);
  writeRadiiOverrides();
  notifyPrefChanged();
}


// Set Radii using user input
//   Save the input into preference file
void PerTabPanel::setElementRadii(int atomicNum, double covRadInput,
				  double vdwRadInput, double bftInput)
{
  p_elementInfo[atomicNum]->covalentRadius = covRadInput;
  p_elementInfo[atomicNum]->vanderWaalsRadius = vdwRadInput;
  p_elementInfo[atomicNum]->bondFormationTolerance = bftInput;
  writeRadiiOverrides();
  notifyPrefChanged();
}


// Get the tPerTab object
TPerTab * PerTabPanel::getTPerTab() const
{
  return p_tPerTab;
}


// Get the tPerTab object
TPerTab * PerTabPanel::getRefTPerTab() const
{
  return p_refTPerTab;
}


void PerTabPanel::setCurrentElement(int atomicNum)
{
  p_currentElement = atomicNum;
  p_dataFrame->setContent(atomicNum);
}


void PerTabPanel::selectElement(int atomicNum)
{
    if (!(p_elements[atomicNum]->getStatus()))
    {
        if (!p_isMultiSelect)
            clearSelection();

        p_elements[atomicNum]->setStatus(true);
        p_selectCount++;
    }
}


void PerTabPanel::clearSelection()
{
    for (int i = 0; i < p_numAtoms; i++)
        p_elements[i]->setStatus(false);

    p_selectCount = 0;
}


void PerTabPanel::selectElement(int atomicNum, bool isSelected)
{
  p_selectCount = 0;
  int i;
  for (i=0; i<p_numAtoms; i++) {
    if (p_elements[i]->getStatus()) {
      if (!p_isMultiSelect)
	p_elements[i]->setStatus(false);
      else
	p_selectCount ++;
    }
  }

  if (!p_isMultiSelect && isSelected) {
    p_currentElement = atomicNum;
    p_elements[atomicNum]->setStatus(true);
    p_selectCount ++;
  }
}


vector<string> * PerTabPanel::getSelectionSymbols()
{
    vector<string> *vctSymbols = new vector<string>();

    for (int i = 0; i < p_numAtoms; i++)
    {
        if (p_elements[i]->getStatus())
        {
            vctSymbols->push_back(p_refTPerTab->atomicSymbol(i));
        }
    }

    return vctSymbols;
}



wxColour PerTabPanel::offsetColor(wxColour color, float offset)
{
  return wxColour( (unsigned char) (color.Red() * offset),
		   (unsigned char) (color.Green() * offset),
		   (unsigned char) (color.Blue() * offset) );
}


/////////////////////////////////////////////////////////////////////////////
// Description
//   Save setings that are general to the pertab template.  This
//   includes current color scheme, whether or not numbers are displayed.
//   The location, size, and number of instances of data popups is not
//   currently saved.  This could easily be added but may not be
//   desirable behavior.
/////////////////////////////////////////////////////////////////////////////
void PerTabPanel::saveSettings(Preferences& prefs)
{
  prefs.setBool("PerTab.Numbers",p_showNumber);
  prefs.setInt("PerTab.ColorScheme", p_colorScheme);
  prefs.saveFile();
}


/////////////////////////////////////////////////////////////////////////////
// Description
//   Restore what was saved in saveSettings().
/////////////////////////////////////////////////////////////////////////////
void PerTabPanel::restoreSettings(const Preferences& prefs)
{
  bool xbool=false;
  if (prefs.getBool("PerTab.Numbers",xbool)) {
    p_showNumber = xbool && !p_isMini;
  }

  int clrScheme;
  if (prefs.getInt("PerTab.ColorScheme",clrScheme)) {
    if (clrScheme >= ID_ITEM_PHYS && clrScheme <= ID_ITEM_USER)
      p_colorScheme = clrScheme;
    else
      p_colorScheme = ID_ITEM_DEFAULT;
  }
}


void PerTabPanel::setElementColor(int atomicNum, const ewxColor& color)
{
  p_elementInfo[atomicNum]->userColor = color.ToString();
  p_elements[atomicNum]->Refresh();
  
  string key;
  for (int i=0; i<p_numAtoms; i++) {
    key = p_refTPerTab->atomicSymbol(i);
    key += ".Color";
    if (p_elementInfo[i]->userColor == p_elementInfo[i]->defaultColor) {
      if (p_prefs->isDefined(key)) {
        p_prefs->remove_entry(key);
      }
    }
    else {
      p_prefs->setString(key, p_elementInfo[i]->userColor);
    }
  }
  p_prefs->saveFile();
  notifyPrefChanged();
  // The Color change evnt is not received by Builder itself, send an internal
  // event to cover that.
  if (p_isMini) {
    Event event("ElementColorChanged",
                p_refTPerTab->atomicSymbol(p_currentElement));
    EventDispatcher::getDispatcher().publish(event);
  }

}


void PerTabPanel::resetUserPreferences()
{  
  delete p_prefs;
  p_prefs = new Preferences(p_prefFile);
  mergeUserElementPreferences();
  recolorTable();
}


void PerTabPanel::mergeUserElementPreferences()
{
  string btnColorKey;
  string color;
  
  for (int i=0; i<p_numAtoms; i++) {
    btnColorKey = p_refTPerTab->atomicSymbol(i);
    btnColorKey += ".Color";
    if (p_prefs->getString(btnColorKey,color)) {
      p_elementInfo[i]->userColor = color;
    } else {
      p_elementInfo[i]->userColor = p_refTPerTab->colorName(i);
    }
  }
}


void PerTabPanel::resetAllElementColor()
{
  string key;
  for (int i=0; i<p_numAtoms; i++) {
    p_elementInfo[i]->userColor = p_elementInfo[i]->defaultColor;
    p_elements[i]->Refresh();
  
    key = p_refTPerTab->atomicSymbol(i);
    key += ".Color";
    if (p_prefs->isDefined(key)) {
      p_prefs->remove_entry(key);
    }
  }
  p_prefs->saveFile();
  notifyPrefChanged();
}


void PerTabPanel::resetAllElementRadii()
{
  for (int i=0; i<p_numAtoms; i++) {
    p_elementInfo[i]->covalentRadius = getDefaultCovRad(i);
    p_elementInfo[i]->vanderWaalsRadius = getDefaultVdwRad(i);
    p_elementInfo[i]->bondFormationTolerance = getDefaultBft(i);
  }
  writeRadiiOverrides();
  notifyPrefChanged();
}


bool PerTabPanel::colorChanged()
{
  for (int i=0; i<p_numAtoms; i++) {
    if ( p_elementInfo[i]->userColor != p_elementInfo[i]->defaultColor )
      return true;
  }
  return false;
}


bool PerTabPanel::radiiChanged()
{
  for (int i=0; i<p_numAtoms; i++) {
    if ( !equal(p_elementInfo[i]->covalentRadius, getDefaultCovRad(i)) ||
	 !equal(p_elementInfo[i]->vanderWaalsRadius, getDefaultVdwRad(i)) ||
	 !equal(p_elementInfo[i]->bondFormationTolerance, getDefaultBft(i)) )
      return true;
  }
  return false;
}


bool PerTabPanel::equal( double a, double b)
{
  return (a - b) < 1e-6 && (b - a) < 1e-6;
}


void PerTabPanel::notifyPrefChanged()
{
  JMSPublisher publisher(PERTAB);
  JMSMessage *msg = publisher.newMessage();
  msg->addProperty("filename",p_prefFile);
  publisher.publish("ecce_preferences_pertab",*msg);
  delete msg;
}


void PerTabPanel::close()
{
  p_dataFrame->Destroy();
}


void PerTabPanel::writeRadiiOverrides()
{
  string radiiFile = Ecce::realUserPrefPath();
  radiiFile += "AtomicRadii";
  ofstream radiiStream(radiiFile.c_str(), (ios::out | ios::trunc));

  if (radiiStream) {
    radiiStream << "# This file contains user modified defaults for "
		<< "covalent and van der Waals\n# radii.  Values are not required for all "
		<< "elements since these values\n# just override system defaults.\n#\n";

    radiiStream << "# To override the value for an element, add the element "
		<< "name, van der Waals\n# radius, covalent radius then tolerance for "
		<< "determining bond formation\n#  (minus the the '#' comment) as in:\n"
		<< "#H: 1.1 0.4 1.2\n#\n# Order of elements is not significant.  Case is "
		<< "sensitive.\n#\n";

    for (int i=0; i<p_numAtoms; i++)
      if (p_elementInfo[i]->covalentRadius != p_refTPerTab->covalentRadius(i) ||
	  p_elementInfo[i]->vanderWaalsRadius != p_refTPerTab->vwr(i) ||
          p_elementInfo[i]->bondFormationTolerance != p_refTPerTab->covalentRadiusTolerance(i))
        radiiStream << p_refTPerTab->atomicSymbol(i) << ": "
		    << p_elementInfo[i]->vanderWaalsRadius << " "
		    << p_elementInfo[i]->covalentRadius << " "
		    << p_elementInfo[i]->bondFormationTolerance << "\n";
    
    radiiStream.close();
  }
}


bool PerTabPanel::isMini()
{
  return p_isMini;
}


void PerTabPanel::eventMCB(const Event& event)
{
  string name = event.getName();
  string value = event.getValue();

  if (name == "ElementChanged") {
    selectElement(p_refTPerTab->atomicNumber(value), true);
  }
}
