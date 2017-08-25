/////////////////////////////////////////////////////////////////////////////
// Name:        VizStyleChooser.H
// Purpose:
// Author:      Bruce Palmer
// Modified by:
// RCS-ID:
// Licence:
/////////////////////////////////////////////////////////////////////////////

#ifndef VIZSTYLECHOOSER_H
#define VIZSTYLECHOOSER_H

#include "util/Listener.H"
using namespace ecce;

#include "dsm/ResourceDescriptor.H"

#include "wxgui/WxJMSListener.H"
#include "wxgui/WxJMSMessageDispatch.H"

#include "wxviz/VizStyleChooserGUI.H"
#include "wxviz/WxVizTool.H"



class DisplayDescriptor;
class Preferences;
class WxColorSchemeChooser;


/**
 *  VizStyleChooser class declaration
 */
class VizStyleChooser: public VizStyleChooserGUI,
		      public WxVizTool,
            public Listener
{
   public:
      VizStyleChooser();
      VizStyleChooser( wxWindow* parent,
            wxWindowID id = SYMBOL_VIZSTYLECHOOSERGUI_IDNAME,
            const wxString& caption = SYMBOL_VIZSTYLECHOOSERGUI_TITLE,
            const wxPoint& pos = SYMBOL_VIZSTYLECHOOSERGUI_POSITION,
            const wxSize& size = SYMBOL_VIZSTYLECHOOSERGUI_SIZE,
            long style = SYMBOL_VIZSTYLECHOOSERGUI_STYLE );

      virtual ~VizStyleChooser();

      void initializeGUI();
      void refreshGUI(const string& name);
      void refreshGUI(const DisplayDescriptor& dd);
      void updateSliders();
      void setPanelsFromDescriptor(const DisplayDescriptor& desc);
      void setCSResidueOptions();
      void setDSResidueOptions();
      void setCSNoResidueOptions();
      void setDSNoResidueOptions();
      void updateDisplayPanel(const string& styl);
      void updateSchemePanel(const DisplayDescriptor& dd);

      void reset();

      virtual void restoreSettings();
      virtual void saveSettings();

      void quit();

      /**
       * Event handlers
       */
      virtual    void OnListboxDsplyGroupsSelected( wxCommandEvent& event );
      virtual    void OnButtonDsplyAddClick( wxCommandEvent& event );
      virtual    void OnButtonDsplyRemoveClick( wxCommandEvent& event );
      virtual    void OnButtonDsplyUpdateClick( wxCommandEvent& event );
      virtual    void OnChoiceDsplyDsoptSelected( wxCommandEvent& event );
      virtual    void OnChoiceDsplyCsoptSelected( wxCommandEvent& event );
      virtual    void OnToggleDsplyShowClick( wxCommandEvent& event );

      virtual    void OnSliderDsplyLnwdthUpdated( wxCommandEvent& event );
      virtual    void OnSliderDsplyCylrqUpdated( wxCommandEvent& event );
      virtual    void OnSliderDsplyCylradUpdated( wxCommandEvent& event );
      virtual    void OnSliderDsplyResradUpdated( wxCommandEvent& event );
      virtual    void OnSliderDsplySphrqUpdated( wxCommandEvent& event );
      virtual    void OnSliderDsplySphsclUpdated( wxCommandEvent& event );
      virtual    void OnSliderDsplyCoilwUpdated( wxCommandEvent& event );
      virtual    void OnSliderDsplyHlxradUpdated( wxCommandEvent& event );
      virtual    void OnSliderDsplyHlxwUpdated( wxCommandEvent& event );
      virtual    void OnSliderDsplyShtwUpdated( wxCommandEvent& event );
      virtual    void OnSliderDsplyRbthkUpdated( wxCommandEvent& event );
      virtual    void OnSliderDsplyCcylrqUpdated( wxCommandEvent& event );
      virtual    void OnSliderDsplyCylrUpdated( wxCommandEvent& event );
      virtual    void OnSliderDsplyTuberUpdated( wxCommandEvent& event );
      virtual    void OnButtonDsplySaveClick( wxCommandEvent& event );
      virtual    void OnButtonDsplyCloseClick( wxCommandEvent& event );
      virtual    void OnButtonDsplyResetClick( wxCommandEvent& event );
      virtual    void OnButtonDsplyBlockeditClick( wxCommandEvent& event );
      virtual    void OnCloseWindow( wxCloseEvent& event );


      // Listener interface
      virtual void eventMCB(const Event& event);

   protected:

      string getNewName(const DisplayDescriptor& dd);
      DisplayDescriptor* buildDescriptor();


      void requestStyles();
      void chooseColor();


      /** Flag to keep track of whether options lists have been changed */
      int p_hasResidues;
      int p_hasBioRes;

      /** index of style to reselect after refresh */
      string p_nameToSelect;

      /** Pointer to color chooser panel */
      WxColorSchemeChooser *p_chooser;

      /** Pointer to spectrum color chooser panel */
      WxColorSchemeChooser *p_spectrum;

};
#endif
