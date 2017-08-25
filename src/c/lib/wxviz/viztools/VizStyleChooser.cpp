#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "VizStyleChooser.H"
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
using std::cout;
using std::endl;
using std::ends;

#include <strstream>
using std::ostrstream;

#include <string>
using std::string;

#include "dsm/ResourceDescriptor.H"

#include "tdat/DisplayDescriptor.H"
#include "tdat/DisplayStyle.H"

#include "util/StringTokenizer.H"
#include "util/Spectrum.H"
#include "util/EventDispatcher.H"

#include "viz/CSStyleCmd.H"

#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxColorDialog.H"
#include "wxgui/ewxConfig.H"
#include "wxgui/ewxListBox.H"
#include "wxgui/ewxMessageDialog.H"
#include "wxgui/ewxOpenCloseToggle.H"
#include "wxgui/ewxRadioButton.H"
#include "wxgui/ewxSlider.H"
#include "wxgui/ewxWindowUtils.H"
#include "wxgui/WxColorSchemeChooser.H"

#include "wxviz/VizStyleChooser.H"

static const char *v_spectrums[] = {"Rainbow", "Red-White-Blue", "Red", "Blue", "Green"};

/**
 * VizStyleChooser constructors
 * Caller should call initializeGUI after the toolkit is connected.
 */
VizStyleChooser::VizStyleChooser( ) : Listener("VizStyle")
{
   p_hasResidues = -1;
   p_hasBioRes = -1;
   restoreSettings();
   subscribe();
}


/**
 * VizStyleChooser constructors
 * Caller should call initializeGUI after the toolkit is connected.
 */
VizStyleChooser::VizStyleChooser( wxWindow* parent,
      wxWindowID id, const wxString& caption,
      const wxPoint& pos, const wxSize& size,
      long style ) : Listener("VizStyle")
{
   p_hasResidues = -1;
   p_hasBioRes = -1;
   Create( parent, id, caption, pos, size, style );
   restoreSettings();
   subscribe();
}



VizStyleChooser::~VizStyleChooser()
{
   unsubscribe();
   saveSettings();
}



void VizStyleChooser::initializeGUI()
{
   ewxChoice *choice;

   choice = ((ewxChoice*)FindWindow(ID_CHOICE_DSPLY_ATOMSPHERE));
   choice->Append("ATOMSPHERE_HEMISPHERES");
   choice->Append("ATOMSPHERE_FULLSPHERES");
   choice->Append("ATOMSPHERE_ARRAY_HEMISPHERES");
   choice->Append("ATOMSPHERE_ARRAY_FULLSPHERES");
   choice->Append("ATOMSPHERE_GLUSPHERE");
   choice->Append("ATOMSPHERE_BILLBOARD");
   choice->Append("ATOMSPHERE_SOSPHERE");
   choice->Append("ATOMSPHERE_LOD");
   choice->Append("ATOMSPHERE_LOD_GEOMETRY");
   choice = ((ewxChoice*)FindWindow(ID_CHOICE_DSPLY_BONDCYLINDER));
   choice->Append("BONDCYLINDER");
   choice->Append("BONDCYLINDER_ARRAY");
   choice->Append("BONDCYLINDER_GLUCYLINDER");
   choice->Append("BONDCYLINDER_SOCYLINDER");
   choice->Append("BONDCYLINDER_SEMI");
   choice->Append("BONDCYLINDER_LOD");
   ewxOpenCloseToggle *toggle 
     = ((ewxOpenCloseToggle*)FindWindow(ID_TOGGLE_DSPLY_SHOW));
   toggle->close();
   /**
    *  Get default values from a new DisplayDescriptor
    */
   DisplayDescriptor dd;
   refreshGUI(dd);

   //
   // Mimic user picking a display style and refresh the display accordingly.
   //
   wxCommandEvent event;
   OnChoiceDsplyDsoptSelected( event );

   wxSizer *sizer;
   vector<wxColour> colors;

   wxPanel *spectrum = ((wxPanel*)FindWindow(ID_PANEL_DSPLY_SPECTRUM));
   sizer = spectrum->GetSizer();
   p_spectrum = new WxColorSchemeChooser(5, 3, 3, spectrum);

   int max = 100;
   int slices = 50;
   int step = max/slices;

   Spectrum s(1,max,slices);
   float r,g,b;

   colors.clear();
   for (int x=1; x<max; x+=step) {
      s.getRainbowValue(x, r, g, b);
      colors.push_back(ewxColor(r, g, b));
   }
   p_spectrum->addSchemeButton(colors);

   colors.clear();
   for (int x=1; x<max; x+=step) {
      s.getPosNegMidpointValue(x, r, g, b);
      colors.push_back(ewxColor(r, g, b));
   }
   p_spectrum->addSchemeButton(colors);

   colors.clear();
   for (int x=1; x<max; x+=step) {
      s.getRedValue(x, r, g, b);
      colors.push_back(ewxColor(r, g, b));
   }
   p_spectrum->addSchemeButton(colors);

   colors.clear();
   for (int x=1; x<max; x+=step) {
      s.getBlueValue(x, r, g, b);
      colors.push_back(ewxColor(r, g, b));
   }
   p_spectrum->addSchemeButton(colors);

   colors.clear();
   for (int x=1; x<max; x+=step) {
      s.getGreenValue(x, r, g, b);
      colors.push_back(ewxColor(r, g, b));
   }
   p_spectrum->addSchemeButton(colors);

   p_spectrum->setSelection(0);
   sizer->Add(p_spectrum, 0, wxEXPAND, 0);

   wxPanel *swoth = ((wxPanel*)FindWindow(ID_PANEL_DSPLY_SWOTH));
   sizer = swoth->GetSizer();

   // The first argument dictates how many entries there will be per
   // row before a new row is automatically started.  The other 
   // two numbers are margins
   p_chooser = new WxColorSchemeChooser(5, 3, 3, swoth);

   colors.clear();
   colors.push_back(wxColour(205, 205, 0));
   colors.push_back(wxColour(51, 51, 255));
   colors.push_back(wxColour(255, 51, 51));
   p_chooser->addSchemeButton(colors);

   colors.clear();
   colors.push_back(wxColour(255, 0, 0));
   colors.push_back(wxColour(255, 255, 255));
   colors.push_back(wxColour(0, 0, 255));
   p_chooser->addSchemeButton(colors);

   colors.clear();
   colors.push_back(wxColour(255, 0, 0));
   colors.push_back(wxColour(0, 0, 255));
   colors.push_back(wxColour(0, 255, 0));
   p_chooser->addSchemeButton(colors);

   p_chooser->setSelection(1);

   sizer->Add(p_chooser, 0, wxEXPAND, 0);

   requestStyles();
}


/**
 * Update the value fields associated with each slider.
 */
void VizStyleChooser::updateSliders()
{
   wxCommandEvent event;

   OnSliderDsplyLnwdthUpdated( event );
   OnSliderDsplyCylrqUpdated( event );
   OnSliderDsplyCylradUpdated( event );
   OnSliderDsplyResradUpdated( event );
   OnSliderDsplySphrqUpdated( event );
   OnSliderDsplySphsclUpdated( event );
   OnSliderDsplyCoilwUpdated( event );
   OnSliderDsplyHlxradUpdated( event );
   OnSliderDsplyHlxwUpdated( event );
   OnSliderDsplyShtwUpdated( event );
   OnSliderDsplyRbthkUpdated( event );
   OnSliderDsplyCcylrqUpdated( event );
   OnSliderDsplyCylrUpdated( event );
   OnSliderDsplyTuberUpdated( event );
}



void VizStyleChooser::restoreSettings()
{
   ewxConfig * config = ewxConfig::getConfig("vizstyles.ini");
   restoreWindowSettings(config, true);
}


void VizStyleChooser::saveSettings()
{
   ewxConfig * config = ewxConfig::getConfig("vizstyles.ini");
   saveWindowSettings(config,true);
   config->Flush();
}


void VizStyleChooser::setPanelsFromDescriptor(const DisplayDescriptor& dd)
{
   ewxSlider *slider;
   ewxRadioButton *radio;
   ewxCheckBox *check;
   ewxChoice *choice;
   int ival;
   float rval;

   // Line panel
   slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_LNWDTH));
   slider->SetValue(dd.getLineWidth());

   // Cylinder panel
   if (dd.getCapStyle() == 0) {
     radio = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_DSPLY_NONE));
     radio->SetValue(true);
   } else if (dd.getCapStyle() == 1) {
     radio = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_DSPLY_FLAT));
     radio->SetValue(true);
   } else if (dd.getCapStyle() == 2) {
     radio = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_DSPLY_ROUND));
     radio->SetValue(true);
   }
   slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_CYLRQ));
   slider->SetValue(dd.getCylinderRQ());
   slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_CYLRAD));
   slider->SetValue(dd.getCylinderRadius());
   slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_RESRAD));
   ival = (int)(10.0*dd.getResidueCylinderRadius()+0.5);
   slider->SetValue(ival);
   ival = dd.getBondCylinderOpt();
   ival = dd.getBondCylinderOpt();
   choice = ((ewxChoice*)FindWindow(ID_CHOICE_DSPLY_BONDCYLINDER));
   ival = (ival-dd.getCapStyle())/3;
   choice->SetSelection(ival);

   // Sphere panel
   slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_SPHSCL));
   slider->SetValue(dd.getSphereSize());
   slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_SPHRQ));
   slider->SetValue(dd.getSphereRQ());
   choice = ((ewxChoice*)FindWindow(ID_CHOICE_DSPLY_ATOMSPHERE));
   choice->SetSelection(dd.getAtomSphereOpt());

   // Ribbon panel
   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_DSPLY_BLNDCL));
   check->SetValue(dd.getRibbonBlended());
   check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_DSPLY_RNDRB));
   check->SetValue(dd.getRibbonRounded());
   slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_COILW));
   ival = (int)(10.0*dd.getCoilWidth()+0.5);
   slider->SetValue(ival);
   slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_HLXW));
   ival = (int)(10.0*dd.getHelixWidth()+0.5);
   slider->SetValue(ival);
   slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_SHTW));
   ival = (int)(10.0*dd.getSheetWidth()+0.5);
   slider->SetValue(ival);
   slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_HLXRAD));
   rval = dd.getHelixRadius();
   ival = (int)(rval*10.0+0.5);
   slider->SetValue(ival);
   slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_RBTHK));
   rval = dd.getRibbonThickness();
   ival = (int)(rval*20.0+0.5);
   slider->SetValue(ival);

   //Cartoon panel
   slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_CYLR));
   ival = (int)(10.0*dd.getCartoonCylinderRadius()+0.5);
   slider->SetValue(ival);
   slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_CCYLRQ));
   slider->SetValue(dd.getCartoonCylinderRQ());
   slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_TUBER));
   ival = (int)(10.0*dd.getCartoonTubeRadius()+0.5);
   slider->SetValue(ival);

}



void VizStyleChooser::refreshGUI(const string& name)
{
   WxVizToolFW& fw = getFW();
   SGContainer& sg = fw.getSceneGraph();
   DisplayDescriptor dd = sg.getDisplayDescriptor(name);

   // Set display style
   string display = dd.getStyle();
   ewxChoice *menu = ((ewxChoice*)FindWindow(ID_CHOICE_DSPLY_DSOPT));
   int selection = menu->FindString(display.c_str());
   if (selection != wxNOT_FOUND) {
     menu->SetSelection(selection);
   }
   // Set color scheme options based on display style
   if (p_hasResidues == 1) {
     setCSResidueOptions();
   } else {
     setCSNoResidueOptions();
   }
   // Show appropriate panels based on display style
   updateDisplayPanel(display);

   // Set color scheme
   string CSselection = dd.getColorScheme();
   menu = ((ewxChoice*)FindWindow(ID_CHOICE_DSPLY_CSOPT));
   selection = menu->FindString(CSselection.c_str());
   if (selection != wxNOT_FOUND) {
     menu->SetSelection(selection);
   }

   refreshGUI(dd);

   // Probably not all these are necessary
   p_mainSizer->Layout();
   Layout();
   Fit();
}

void VizStyleChooser::refreshGUI(const DisplayDescriptor& dd)
{
   ewxChoice *choice;
   choice = ((ewxChoice*)FindWindow(ID_CHOICE_DSPLY_DSOPT));
   choice->SetStringSelection(dd.getStyle().c_str());

   choice = ((ewxChoice*)FindWindow(ID_CHOICE_DSPLY_CSOPT));
   choice->SetStringSelection(dd.getColorScheme().c_str());

   // The style support panels
   setPanelsFromDescriptor(dd);

   // The color scheme support panels
   updateSchemePanel(dd);

   wxCommandEvent event;
   OnToggleDsplyShowClick(event);
   updateSliders();
}



void VizStyleChooser::setDSResidueOptions()
{
   ewxChoice *choice;
   choice = ((ewxChoice*)FindWindow(ID_CHOICE_DSPLY_DSOPT));
   choice->Clear();
   vector<string> names = DisplayStyle::getAllNames(false);
   for (size_t idx=0; idx<names.size(); idx++) {
      choice->Append(names[idx].c_str());
   }
   choice->Select(0);
}



void VizStyleChooser::setDSNoResidueOptions()
{
   ewxChoice *choice;
   choice = ((ewxChoice*)FindWindow(ID_CHOICE_DSPLY_DSOPT));
   choice->Clear();
   vector<string> names = DisplayStyle::getNoResidueNames(false);
   for (size_t idx=0; idx<names.size(); idx++) {
      choice->Append(names[idx].c_str());
   }
   choice->Select(0);
}



void VizStyleChooser::setCSResidueOptions()
{
   ewxChoice *choice;
   choice = ((ewxChoice*)FindWindow(ID_CHOICE_DSPLY_DSOPT));
   string DSOpt = choice->GetStringSelection().c_str();
   choice = ((ewxChoice*)FindWindow(ID_CHOICE_DSPLY_CSOPT));
   string CSOpt = choice->GetStringSelection().c_str();
   choice->Clear();
   
   // Get color scheme options from display descriptor
   bool hasBio = p_hasBioRes > 0 ? true : false ;
   vector<string> cs_list = DisplayDescriptor::getCSResidueOptions(DSOpt, hasBio);
   for (size_t i=0; i<cs_list.size(); i++) {
     choice->Append(cs_list[i].c_str());
   }

   int place = choice->FindString(CSOpt.c_str());
   if (place != wxNOT_FOUND) {
     choice->SetSelection(place);
   } else {
     choice->Select(0);
   }
}



void VizStyleChooser::setCSNoResidueOptions()
{
   ewxChoice *choice;
   choice = ((ewxChoice*)FindWindow(ID_CHOICE_DSPLY_CSOPT));
   choice->Clear();
   // Get color scheme options from display descriptor
   vector<string> cs_list = DisplayDescriptor::getCSNoResidueOptions();
   for (size_t i=0; i<cs_list.size(); i++) {
     choice->Append(cs_list[i].c_str());
   }
   choice->Select(0);
}



/**
 * Initiates refresh of the gui by requesting the list of styles.
 */
void VizStyleChooser::reset()
{
   requestStyles();
}

/**
 * Handle user closing with window manager button.
 */
void VizStyleChooser::OnCloseWindow( wxCloseEvent& event )
{
   Show(false);
}

void VizStyleChooser::quit()
{
   Destroy();
}


/**
 * Destroy window.
 */
void VizStyleChooser::OnButtonDsplyCloseClick( wxCommandEvent& event )
{
   Show(false);
}

/**
 * Save current display style as preference.
 */
void VizStyleChooser::OnButtonDsplySaveClick( wxCommandEvent& event )
{
   ewxConfig * config = ewxConfig::getConfig("vizstyles.ini");

   DisplayDescriptor *dd = buildDescriptor();
   string style = dd->getStyle();
   config->Write(style.c_str(),dd->toString().c_str());
   config->Flush();
   delete dd;

   event.Skip();
}

/**
 * Reset all values to defaults.
 */
void VizStyleChooser::OnButtonDsplyResetClick( wxCommandEvent& event )
{
   DisplayDescriptor dd;

   // Reset values from descriptor
   setPanelsFromDescriptor(dd);

   // Update static text for sliders
   updateSliders();
   event.Skip();
}

/**
 * When an item is selected, ask builder to:
 * <li>select
 * <li>tell us the * sytle and scheme and any other information.
 */
void VizStyleChooser::OnListboxDsplyGroupsSelected( wxCommandEvent& event )
{
   ewxListBox *list = ((ewxListBox*)FindWindow(ID_LISTBOX_DSPLY_GROUPS));

   // Select atoms in current display style
   // Note that this breaks down with residue styles because we have
   // no way of showing selection
   WxVizToolFW& fw = getFW();
   SGContainer& sg = fw.getSceneGraph();
   string name = list->GetStringSelection().c_str();
   sg.selectStyle(name);
   SGFragment *frag = sg.getFragment();
   if (frag->numAtoms() > 0) {
      setSelection(frag->m_atomHighLight, frag->m_bondHighLight);
      EventDispatcher::getDispatcher().publish(Event("SelectionChanged"));
   }


   refreshGUI(name);
}


/**
 * Add a new display style based on the current option menu selections.
 */
void VizStyleChooser::OnButtonDsplyAddClick( wxCommandEvent& event )
{

   DisplayDescriptor *display = buildDescriptor();
   string name = getNewName(*display);
   display->setName(name);

   // Keep track of which item to select after a refresh
   // Do before executing command which triggers refresh via event
   p_nameToSelect =  name;


   WxVizToolFW& fw = getFW();
   SGContainer& sg = fw.getSceneGraph();
   SGFragment *frag = sg.getFragment();
   bool all = false;
   if (frag->m_atomHighLight.size() == 0 &&
         frag->m_bondHighLight.size() == 0) {
      all = true;
   }
   Command *cmd = new CSStyleCmd("Create Style", &sg);
   cmd->getParameter("descriptor")->setString(display->toString());
   cmd->getParameter("all")->setBoolean(all);

   fw.execute(cmd);

   delete display;

   requestStyles();
}

void VizStyleChooser::OnButtonDsplyUpdateClick( wxCommandEvent& event )
{
   ewxListBox *lbox = ((ewxListBox*)FindWindow(ID_LISTBOX_DSPLY_GROUPS));
   string name = lbox->GetStringSelection().c_str();
   if (name == "" ) {
      string msg = "You must select display style before updating";
      ewxMessageDialog prompt(this,msg.c_str(),"Display Style Update",
                  wxOK);
      prompt.ShowModal();
      return;
   }
   DisplayDescriptor *display = buildDescriptor();

   // Keep track of which item to select after a refresh
   // Do before cmd which triggers refresh via event
   p_nameToSelect =  name;

   display->setName(name);

   WxVizToolFW& fw = getFW();
   SGContainer& sg = fw.getSceneGraph();
   SGFragment *frag = sg.getFragment();
   bool all = false;
   if (frag->m_atomHighLight.size() == 0 &&
         frag->m_bondHighLight.size() == 0) {
      all = true;
   }
   Command *cmd = new CSStyleCmd("Update Style", &sg);
   cmd->getParameter("descriptor")->setString(display->toString());
   cmd->getParameter("all")->setBoolean(all);

   fw.execute(cmd);


   delete display;

}

void VizStyleChooser::OnButtonDsplyRemoveClick( wxCommandEvent& event )
{
   ewxListBox *lbox = ((ewxListBox*)FindWindow(ID_LISTBOX_DSPLY_GROUPS));
   int choice = lbox->GetSelection();
   if (choice >= 0) {
      if (lbox->GetCount() > 1) {
         string name = lbox->GetString(lbox->GetSelection()).c_str();


         WxVizToolFW& fw = getFW();
         SGContainer& sg = fw.getSceneGraph();
         sg.removeStyle(name);
         sg.getFragment()->touchNumbers();
         sg.touchChemDisplay();

         EventDispatcher::getDispatcher().publish(Event("StyleChange"));
      } else {
         ewxMessageDialog msgdlg(this, "You cannot not delete the only remaining style.", "Style Delete Error");
         msgdlg.ShowModal();

      }

   }
}

/**
 *  User picks a display style so: reset color scheme options, 
 *  display any relevant subpanels, initialize widgets from preferencses.
 */
void VizStyleChooser::OnChoiceDsplyDsoptSelected( wxCommandEvent& event )
{
  ewxChoice *menu = ((ewxChoice*)FindWindow(ID_CHOICE_DSPLY_CSOPT));
  string CSselection = menu->GetStringSelection().c_str();
  wxPanel *swoth = ((wxPanel*)FindWindow(ID_PANEL_DSPLY_SWOTH));
  swoth->Hide();
  if (p_hasResidues == 1) {
    setCSResidueOptions();
  } else {
    setCSNoResidueOptions();
  }
  menu = ((ewxChoice*)FindWindow(ID_CHOICE_DSPLY_DSOPT));
  string display = menu->GetStringSelection().c_str();
  updateDisplayPanel(display);
  menu = ((ewxChoice*)FindWindow(ID_CHOICE_DSPLY_CSOPT));
  int selection = menu->FindString(CSselection.c_str());
  if (selection != wxNOT_FOUND) {
    menu->SetSelection(selection);
  }


  if (CSselection != "") {
     ewxConfig *config = ewxConfig::getConfig("vizstyles.ini");
     wxString ddstr = config->Read(display.c_str());
     DisplayDescriptor dd(ddstr.c_str());
     refreshGUI(dd);
  } else {
     // This should not really ever happen
     // only update the scheme panel from defaults in this case
     DisplayDescriptor dd;
     dd.setColorScheme(CSselection);
     updateSchemePanel(dd);
  }


  // Probably not all these are necessary
  p_mainSizer->Layout();
  Layout();
  Fit();
}

void VizStyleChooser::updateDisplayPanel(const string& style)
{
  wxPanel *lnwdthPanel = ((wxPanel*)FindWindow(ID_PANEL_DSPLY_LNWDTH));
  wxPanel *cylinderPanel = ((wxPanel*)FindWindow(ID_PANEL_DSPLY_CYLNDR));
  wxPanel *spherePanel = ((wxPanel*)FindWindow(ID_PANEL_DSPLY_SPHERE));
  wxPanel *ribbonPanel = ((wxPanel*)FindWindow(ID_PANEL_DSPLY_RIBBON));
  wxPanel *cartoonPanel = ((wxPanel*)FindWindow(ID_PANEL_DSPLY_CRTN));
  lnwdthPanel->Hide();
  cylinderPanel->Hide();
  spherePanel->Hide();
  ribbonPanel->Hide();
  cartoonPanel->Hide();
  if (style == "Ball And Wireframe" || style == "Wireframe" ||
      style == "Residue Wire" || style == "Line Ribbon") {
    lnwdthPanel->Show();
  }
  if (style == "Ball And Stick" || style == "Stick" ||
      style == "Residue Cylinders") {
    cylinderPanel->Show();
    if (style == "Residue Cylinders") {
      FindWindow(wxID_STATIC_DSPLY_CYLRQTXT)->Hide();
      FindWindow(ID_SLIDER_DSPLY_CYLRQ)->Hide();
      FindWindow(wxID_STATIC_DSPLY_CYLRQ)->Hide();
      FindWindow(wxID_STATIC_DSPLY_CYLRQMIN)->Hide();
      FindWindow(wxID_STATIC_DSPLY_CYLRQMAX)->Hide();
      FindWindow(wxID_STATIC_DSPLY_RESRADTXT)->Show();
      FindWindow(ID_SLIDER_DSPLY_RESRAD)->Show();
      FindWindow(wxID_STATIC_DSPLY_RESRAD)->Show();
      FindWindow(wxID_STATIC_DSPLY_RESRADMIN)->Show();
      FindWindow(wxID_STATIC_DSPLY_RESRADMAX)->Show();
      FindWindow(wxID_STATIC_DSPLY_CYLRADTXT)->Hide();
      FindWindow(ID_SLIDER_DSPLY_CYLRAD)->Hide();
      FindWindow(wxID_STATIC_DSPLY_CYLRAD)->Hide();
      FindWindow(wxID_STATIC_DSPLY_CYLRADMIN)->Hide();
      FindWindow(wxID_STATIC_DSPLY_CYLRADMAX)->Hide();
    } else {
      FindWindow(wxID_STATIC_DSPLY_CYLRQTXT)->Show();
      FindWindow(ID_SLIDER_DSPLY_CYLRQ)->Show();
      FindWindow(wxID_STATIC_DSPLY_CYLRQ)->Show();
      FindWindow(wxID_STATIC_DSPLY_CYLRQMIN)->Show();
      FindWindow(wxID_STATIC_DSPLY_CYLRQMAX)->Show();
      FindWindow(wxID_STATIC_DSPLY_RESRADTXT)->Hide();
      FindWindow(ID_SLIDER_DSPLY_RESRAD)->Hide();
      FindWindow(wxID_STATIC_DSPLY_RESRAD)->Hide();
      FindWindow(wxID_STATIC_DSPLY_RESRADMIN)->Hide();
      FindWindow(wxID_STATIC_DSPLY_RESRADMAX)->Hide();
      FindWindow(wxID_STATIC_DSPLY_CYLRADTXT)->Show();
      FindWindow(ID_SLIDER_DSPLY_CYLRAD)->Show();
      FindWindow(wxID_STATIC_DSPLY_CYLRAD)->Show();
      FindWindow(wxID_STATIC_DSPLY_CYLRADMIN)->Show();
      FindWindow(wxID_STATIC_DSPLY_CYLRADMAX)->Show();
    }
  }
  if (style == "Ball And Wireframe" || style == "Ball And Stick") {
    spherePanel->Show();
    FindWindow(wxID_STATIC_DSPLY_SPHSCLTXT)->Show();
    FindWindow(ID_SLIDER_DSPLY_SPHSCL)->Show();
    FindWindow(wxID_STATIC_DSPLY_SPHSCL)->Show();
    FindWindow(wxID_STATIC_DSPLY_SPHSCLMIN)->Show();
    FindWindow(wxID_STATIC_DSPLY_SPHSCLMAX)->Show();
  }
  if (style == "CPK") {
    spherePanel->Show();
    FindWindow(wxID_STATIC_DSPLY_SPHSCLTXT)->Hide();
    FindWindow(ID_SLIDER_DSPLY_SPHSCL)->Hide();
    FindWindow(wxID_STATIC_DSPLY_SPHSCL)->Hide();
    FindWindow(wxID_STATIC_DSPLY_SPHSCLMIN)->Hide();
    FindWindow(wxID_STATIC_DSPLY_SPHSCLMAX)->Hide();
  }
  if (style == "Line Ribbon" || style == "Flat Ribbon" ||
      style == "Solid Ribbon") {
    ribbonPanel->Show();
    if (style == "Solid Ribbon") {
      FindWindow(ID_CHECKBOX_DSPLY_RNDRB)->Show();
      FindWindow(wxID_STATIC_DSPLY_RBTHKTXT)->Show();
      FindWindow(ID_SLIDER_DSPLY_RBTHK)->Show();
      FindWindow(wxID_STATIC_DSPLY_RBTHK)->Show();
      FindWindow(wxID_STATIC_DSPLY_RBTHKMIN)->Show();
      FindWindow(wxID_STATIC_DSPLY_RBTHKMAX)->Show();
    } else {
      FindWindow(ID_CHECKBOX_DSPLY_RNDRB)->Hide();
      FindWindow(wxID_STATIC_DSPLY_RBTHKTXT)->Hide();
      FindWindow(ID_SLIDER_DSPLY_RBTHK)->Hide();
      FindWindow(wxID_STATIC_DSPLY_RBTHK)->Hide();
      FindWindow(wxID_STATIC_DSPLY_RBTHKMIN)->Hide();
      FindWindow(wxID_STATIC_DSPLY_RBTHKMAX)->Hide();
    }
  }
  if (style == "Cartoon") {
    cartoonPanel->Show();
  }


}


/**
 * User picks a color scheme so hide/show correct subpanels
 */
void VizStyleChooser::OnChoiceDsplyCsoptSelected( wxCommandEvent& event )
{
   ewxChoice *menu = ((ewxChoice*)FindWindow(ID_CHOICE_DSPLY_CSOPT));
   // only update the scheme panel from defaults in this case
   DisplayDescriptor dd;
   dd.setColorScheme(menu->GetStringSelection().c_str());
   updateSchemePanel(dd);
   p_mainSizer->Layout();
   Fit();
}



void VizStyleChooser::updateSchemePanel(const DisplayDescriptor& dd)
{
   wxPanel *blockPanel = ((wxPanel*)FindWindow(ID_PANEL_DSPLY_BLOCKSCHEME));
   wxPanel *spectrumPanel = ((wxPanel*)FindWindow(ID_PANEL_DSPLY_SPECTRUM));
   wxPanel *swoth = ((wxPanel*)FindWindow(ID_PANEL_DSPLY_SWOTH));
   blockPanel->Hide();
   spectrumPanel->Hide();
   swoth->Hide();

   string scheme = dd.getColorScheme();
   if (scheme == "Block Color") {
      blockPanel->Show();
      wxPanel *p = ((wxPanel*)FindWindow(ID_PANEL_DSPLY_BLOCKCOLOR));
      p->SetBackgroundColour(wxColor(dd.getBlockColor().c_str()));
   }
   if (scheme == "Atom Charge" || scheme == "Position from CM" ||
       scheme == "Atom Index" || scheme == "Residue Index" ||
       scheme == "Chains" || scheme == "Residue Name") {
      spectrumPanel->Show();
      //   choice = ((ewxChoice*)FindWindow(ID_CHOICE_DSPLY_SPECTRUM));
      //   choice->SetStringSelection(dd.getSpectrumType().c_str());
      string type = dd.getSpectrumType();
      for (size_t idx=0; idx<sizeof(v_spectrums)/sizeof(char*); idx++) {
         if (type == v_spectrums[idx]) {
            p_spectrum->setSelection(idx);
         }
      }
   } else if (scheme == "Hydrophobicity" || scheme == "Secondary Structure" ||
              scheme == "Residue Charge") {
      swoth->Show();
      p_chooser->setSelection(dd.get3ColorStyle());
      if (scheme == "Hydrophobicity") {
         p_chooser->setToolTip("Hydrophylic-Neutral-Hydrophobic");
      } else if (scheme == "Residue Charge") {
         p_chooser->setToolTip("Negative-Neutral-Positive");
      } else if (scheme == "Secondary Structure") {
         p_chooser->setToolTip("Sheet-Coil-Helix");
      }
   }
}


/**
 * Generate a new unique name.
 */
string VizStyleChooser::getNewName(const DisplayDescriptor& dd)
{
   ewxListBox *lbox = ((ewxListBox*)FindWindow(ID_LISTBOX_DSPLY_GROUPS));
#ifdef usenicenames
// NOTE This creates new problems because the names become wrong as soon
// as the user does an update

   // First generate a decent name based on the style and scheme
   string name = dd.getStyle();
   name += "-";
   name += dd.getColorScheme();

   // Next, ensure that is unique since the above chosen attributes don't
   // guarantee uniqueness.
   if (lbox->FindString(name.c_str()) >=  0) {
      // Need to work harder to make a unique name
      char buf[128];
      for (int idx=0; ; idx++) {
        sprintf(buf, "%s%d", name.c_str(), idx);
        if (lbox->FindString(buf) ==  wxNOT_FOUND) {
           name = buf;
           break;
        }
      }
   }

   return name;
#else
   int num = 1;
   int nItems = lbox->GetCount();
   for (int idx=0; idx<nItems; idx++) {
      string name = lbox->GetString(idx).c_str();
      if (name.find("Custom") == 0) {
         int cur = 1;
         sscanf(name.c_str(), "Custom%d",&cur);
         if (cur >= num) num = cur +1;
      }
   }
   char buf[20];
   sprintf(buf, "Custom%d", num);

   return buf;


#endif

}

DisplayDescriptor *VizStyleChooser::buildDescriptor()
{
   wxPanel *panel;

   DisplayDescriptor *display = new DisplayDescriptor();
   ewxChoice *choice;
   choice = ((ewxChoice*)FindWindow(ID_CHOICE_DSPLY_DSOPT));
   display->setName(choice->GetStringSelection().c_str());
   display->setStyle(choice->GetStringSelection().c_str());
   choice = ((ewxChoice*)FindWindow(ID_CHOICE_DSPLY_CSOPT));
   display->setColorScheme(choice->GetStringSelection().c_str());

   // Now add in additional values optional depending on the scheme
   string scheme = choice->GetStringSelection().c_str();
   if (scheme == "Block Color") {
      wxPanel *p = ((wxPanel*)FindWindow(ID_PANEL_DSPLY_BLOCKCOLOR));
      ewxColor color(p->GetBackgroundColour());
      display->setBlockColor(color.ToString());
   }

   // Spectrum panel
   panel = ((wxPanel*)FindWindow(ID_PANEL_DSPLY_SPECTRUM));
   if (panel->IsEnabled()) {
     //choice = ((ewxChoice*)FindWindow(ID_CHOICE_DSPLY_SPECTRUM));
     //display->setSpectrumType(choice->GetStringSelection().c_str());
     int sel = p_spectrum->getSelection();
     display->setSpectrumType(v_spectrums[sel]);
   }

   // 3Color panel
   panel = ((wxPanel*)FindWindow(ID_PANEL_DSPLY_SWOTH));
   if (panel->IsEnabled()) {
      vector<wxColour> wxcolors = 
         p_chooser->getColors(p_chooser->getSelection());
      vector<string> colors;
      for (size_t idx=0; idx<wxcolors.size(); idx++) {
         colors.push_back(Color(wxcolors[idx].Red(),
                                wxcolors[idx].Green(),
                                wxcolors[idx].Blue()).xcolor());
      }
      display->set3ColorStyle(colors);
   }

   // Get Display Style panel settings
   float rval;
   ewxSlider *slider;
   ewxRadioButton *radio;
   ewxCheckBox *check;

   // Line panel
   panel = ((wxPanel*)FindWindow(ID_PANEL_DSPLY_LNWDTH));
   slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_LNWDTH));
   if (panel->IsEnabled()) {
     display->setLineWidth(slider->GetValue());
   }

   // Cylinder panel
   panel = ((wxPanel*)FindWindow(ID_PANEL_DSPLY_CYLNDR));
   if (panel->IsEnabled()) {
     slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_CYLRAD));
     display->setCylinderRadius(slider->GetValue());
     slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_RESRAD));
     rval = (float)(slider->GetValue())/10.0;
     display->setResidueCylinderRadius(rval);
     radio = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_DSPLY_NONE));
     int cap;
     if (radio->GetValue()) {
       cap = 0;
     }
     radio = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_DSPLY_FLAT));
     if (radio->GetValue()) {
       cap = 1;
     }
     radio = ((ewxRadioButton*)FindWindow(ID_RADIOBUTTON_DSPLY_ROUND));
     if (radio->GetValue()) {
       cap = 2;
     }
     display->setCapStyle(cap);
     slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_CYLRQ));
     display->setCylinderRQ(slider->GetValue());
     choice = ((ewxChoice*)FindWindow(ID_CHOICE_DSPLY_BONDCYLINDER));
     display->setBondCylinderOpt(choice->GetSelection()*3+cap);
   }

   // Sphere panel
   panel = ((wxPanel*)FindWindow(ID_PANEL_DSPLY_SPHERE));
   if (panel->IsEnabled()) {
     slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_SPHRQ));
     display->setSphereRQ(slider->GetValue());
     slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_SPHSCL));
     display->setSphereSize(slider->GetValue());
     choice = ((ewxChoice*)FindWindow(ID_CHOICE_DSPLY_ATOMSPHERE));
     display->setAtomSphereOpt(choice->GetSelection());
   }

   // Ribbon panel
   panel = ((wxPanel*)FindWindow(ID_PANEL_DSPLY_RIBBON));
   if (panel->IsEnabled()) {
     check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_DSPLY_BLNDCL));
     display->setRibbonBlended(check->GetValue());
     check = ((ewxCheckBox*)FindWindow(ID_CHECKBOX_DSPLY_RNDRB));
     display->setRibbonRounded(check->GetValue());
     slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_COILW));
     rval = (float)(slider->GetValue())/10.0;
     display->setCoilWidth(rval);
     slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_HLXW));
     rval = (float)(slider->GetValue())/10.0;
     display->setHelixWidth(rval);
     slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_HLXRAD));
     rval = (float)(slider->GetValue())/10.0;
     display->setHelixRadius(rval);
     slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_SHTW));
     rval = (float)(slider->GetValue())/10.0;
     display->setSheetWidth(rval);
     slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_RBTHK));
     rval = (float)(slider->GetValue())/20.0;
     display->setRibbonThickness(rval);
   }

   // Cartoon panel
   panel = ((wxPanel*)FindWindow(ID_PANEL_DSPLY_CRTN));
   if (panel->IsEnabled()) {
     slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_CCYLRQ));
     display->setCartoonCylinderRQ(slider->GetValue());
     slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_CYLR));
     rval = (float)(slider->GetValue())/10.0;
     display->setCartoonCylinderRadius(rval);
     slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_TUBER));
     rval = (float)(slider->GetValue())/10.0;
     display->setCartoonTubeRadius(rval);
   }

   return display;
}


/**
 * Initiate user selection of block color.
 */
void VizStyleChooser::OnButtonDsplyBlockeditClick( wxCommandEvent& event )
{
   chooseColor();
}


/**
 * Popup a dialog for user to pick a custom color.
 * If the user doesn't cancel, update the widget that shows the color.
 */
void VizStyleChooser::chooseColor()
{
   wxPanel *p = ((wxPanel*)FindWindow(ID_PANEL_DSPLY_BLOCKCOLOR));
   ewxColorDialog * colorDialog = 
      new ewxColorDialog( p->GetBackgroundColour(), this);


   if (colorDialog->ShowModal() == wxID_OK) {
      p->SetBackgroundColour(colorDialog->GetColor());
   }

}

void VizStyleChooser::OnSliderDsplyLnwdthUpdated( wxCommandEvent& event )
{
  ewxSlider* slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_LNWDTH));
  ewxStaticText* label = ((ewxStaticText*)FindWindow(wxID_STATIC_DSPLY_LNWDTH));
  char buf[120];
  sprintf(buf,"%d",slider->GetValue());
  label->SetLabel(buf);
}

void VizStyleChooser::OnSliderDsplyCylrqUpdated( wxCommandEvent& event )
{
  ewxSlider* slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_CYLRQ));
  ewxStaticText* label = ((ewxStaticText*)FindWindow(wxID_STATIC_DSPLY_CYLRQ));
  char buf[120];
  sprintf(buf,"%d",slider->GetValue());
  label->SetLabel(buf);
}

void VizStyleChooser::OnSliderDsplyCylradUpdated( wxCommandEvent& event )
{
  ewxSlider* slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_CYLRAD));
  ewxStaticText* label = ((ewxStaticText*)FindWindow(wxID_STATIC_DSPLY_CYLRAD));
  char buf[120];
  sprintf(buf,"%d",slider->GetValue());
  label->SetLabel(buf);
}

void VizStyleChooser::OnSliderDsplyResradUpdated( wxCommandEvent& event )
{
  ewxSlider* slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_RESRAD));
  ewxStaticText* label = ((ewxStaticText*)FindWindow(wxID_STATIC_DSPLY_RESRAD));
  char buf[120];
  float rval = slider->GetValue()/10.0;
  sprintf(buf,"%3.1f",rval);
  label->SetLabel(buf);
}

void VizStyleChooser::OnSliderDsplySphrqUpdated( wxCommandEvent& event )
{
  ewxSlider* slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_SPHRQ));
  ewxStaticText* label = ((ewxStaticText*)FindWindow(wxID_STATIC_DSPLY_SPHRQ));
  char buf[120];
  sprintf(buf,"%d",slider->GetValue());
  label->SetLabel(buf);
}

void VizStyleChooser::OnSliderDsplySphsclUpdated( wxCommandEvent& event )
{
  ewxSlider* slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_SPHSCL));
  ewxStaticText* label = ((ewxStaticText*)FindWindow(wxID_STATIC_DSPLY_SPHSCL));
  char buf[120];
  sprintf(buf,"%d",slider->GetValue());
  label->SetLabel(buf);
}

void VizStyleChooser::OnSliderDsplyCoilwUpdated( wxCommandEvent& event )
{
  ewxSlider* slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_COILW));
  ewxStaticText* label = ((ewxStaticText*)FindWindow(wxID_STATIC_DSPLY_COILW));
  char buf[120];
  float rval = slider->GetValue()/10.0;
  sprintf(buf,"%3.1f",rval);
  label->SetLabel(buf);
}

void VizStyleChooser::OnSliderDsplyHlxradUpdated( wxCommandEvent& event )
{
  ewxSlider* slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_HLXRAD));
  ewxStaticText* label = ((ewxStaticText*)FindWindow(wxID_STATIC_DSPLY_HLXRAD));
  char buf[120];
  float rval = slider->GetValue()/10.0;
  sprintf(buf,"%3.1f",rval);
  label->SetLabel(buf);
}

void VizStyleChooser::OnSliderDsplyHlxwUpdated( wxCommandEvent& event )
{
  ewxSlider* slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_HLXW));
  ewxStaticText* label = ((ewxStaticText*)FindWindow(wxID_STATIC_DSPLY_HLXW));
  char buf[120];
  float rval = slider->GetValue()/10.0;
  sprintf(buf,"%3.1f",rval);
  label->SetLabel(buf);
}

void VizStyleChooser::OnSliderDsplyShtwUpdated( wxCommandEvent& event )
{
  ewxSlider* slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_SHTW));
  ewxStaticText* label = ((ewxStaticText*)FindWindow(wxID_STATIC_DSPLY_SHTW));
  char buf[120];
  float rval = slider->GetValue()/10.0;
  sprintf(buf,"%3.1f",rval);
  label->SetLabel(buf);
}

void VizStyleChooser::OnSliderDsplyRbthkUpdated( wxCommandEvent& event )
{
  ewxSlider* slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_RBTHK));
  ewxStaticText* label = ((ewxStaticText*)FindWindow(wxID_STATIC_DSPLY_RBTHK));
  char buf[120];
  float rval = slider->GetValue()/20.0;
  sprintf(buf,"%3.2f",rval);
  label->SetLabel(buf);
}

void VizStyleChooser::OnSliderDsplyCcylrqUpdated( wxCommandEvent& event )
{
  ewxSlider* slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_CCYLRQ));
  ewxStaticText* label = ((ewxStaticText*)FindWindow(wxID_STATIC_DSPLY_CCYLRQ));
  char buf[120];
  sprintf(buf,"%d",slider->GetValue());
  label->SetLabel(buf);
}

void VizStyleChooser::OnSliderDsplyCylrUpdated( wxCommandEvent& event )
{
  ewxSlider* slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_CYLR));
  ewxStaticText* label = ((ewxStaticText*)FindWindow(wxID_STATIC_DSPLY_CYLR));
  char buf[120];
  float rval = slider->GetValue()/10.0;
  sprintf(buf,"%3.1f",rval);
  label->SetLabel(buf);
}

void VizStyleChooser::OnSliderDsplyTuberUpdated( wxCommandEvent& event )
{
  ewxSlider* slider = ((ewxSlider*)FindWindow(ID_SLIDER_DSPLY_TUBER));
  ewxStaticText* label = ((ewxStaticText*)FindWindow(wxID_STATIC_DSPLY_TUBER));
  char buf[120];
  float rval = slider->GetValue()/10.0;
  sprintf(buf,"%3.1f",rval);
  label->SetLabel(buf);
}

void VizStyleChooser::OnToggleDsplyShowClick( wxCommandEvent& event )
{
  ewxOpenCloseToggle *toggle 
    = ((ewxOpenCloseToggle*)FindWindow(ID_TOGGLE_DSPLY_SHOW));
  wxPanel *panel = ((wxPanel*)FindWindow(ID_PANEL_DSPLY_CNTRLS));
  if (toggle->isOpen()) {
    p_mainSizer->Show(panel,true,true);
  } else {
    p_mainSizer->Show(panel,false,true);
  }
  p_mainSizer->Layout();
  Layout();
  Fit();
  event.Skip();
}


void VizStyleChooser::eventMCB(const Event& event)
{
   if (event.getName() == "StyleChange") {
      requestStyles();
   }
}


void VizStyleChooser::requestStyles()
{
   WxVizToolFW& fw = getFW();
   SGContainer& sg = fw.getSceneGraph();
   vector<string> styles = sg.getDisplayStyleNames();

   ewxListBox *lbox = ((ewxListBox*)FindWindow(ID_LISTBOX_DSPLY_GROUPS));
   lbox->Clear();
   for (size_t idx=0; idx<styles.size(); idx++) {
      lbox->Append(styles[idx].c_str());
   }
   string name = p_nameToSelect;
   int pos = lbox->FindString(name.c_str());
   if (pos == wxNOT_FOUND) pos = 0;
   lbox->Select(pos);
   refreshGUI(name);


   bool hasResidues = sg.getFragment()->numResidues() > 0 ;
   p_hasBioRes = sg.getFragment()->hasBiologicalResidues() > 0;

   if (hasResidues) {
      if (p_hasResidues == 0 || p_hasResidues == -1) {
         setDSResidueOptions();
         setCSResidueOptions();
      }
      p_hasResidues = 1;
   } else {
      if (p_hasResidues == 1 || p_hasResidues == -1) {
         setDSNoResidueOptions();
         setCSNoResidueOptions();
      }
      p_hasResidues = 0;
   }


}
