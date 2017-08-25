#ifndef _OPTIMIZEPANEL_H_
#define _OPTIMIZEPANEL_H_

/*!
 * Includes
 */

#include "wxgui/OptimizePanelGUI.H"

class MDPanelHelper;
class OptimizeModel;

/**
 * GUI implementation of OptimizePanel in MD editors
 */
class OptimizePanel: public OptimizePanelGUI
{    

   public:
      /// Constructors
      OptimizePanel(MDPanelHelper *helper );
      OptimizePanel( MDPanelHelper *helper, wxWindow* parent,
            wxWindowID id = SYMBOL_OPTIMIZEPANELGUI_IDNAME,
            const wxPoint& pos = SYMBOL_OPTIMIZEPANELGUI_POSITION,
            const wxSize& size = SYMBOL_OPTIMIZEPANELGUI_SIZE,
            long style = SYMBOL_OPTIMIZEPANELGUI_STYLE );

      virtual ~OptimizePanel( );

      void setOptimizeModel(OptimizeModel *model);
      OptimizeModel* getOptimizeModel();
      void refreshGUI();
      void initializeGUI();
      void showOptimizePanel();

      /**
       *  Optimize panel event handlers
       */
      /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_OPT_USESD
      virtual    void OnCheckboxOptUsesdClick( wxCommandEvent& event );

      /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_OPT_SD_MAXSTP
      virtual    void OnTextctrlOptSdMaxstpEnter( wxCommandEvent& event );

      /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_OPT_ISTEP
      virtual    void OnTextctrlOptIstepEnter( wxCommandEvent& event );

      /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRLOPT_TOL
      virtual    void OnTextctrlOptTolEnter( wxCommandEvent& event );

      /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_OPT_MAXSZ
      virtual    void OnTextctrlOptMaxszEnter( wxCommandEvent& event );

      /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_OPT_USECG
      virtual    void OnCheckboxOptUsecgClick( wxCommandEvent& event );

      /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_OPT_CG_MAXSTP
      virtual    void OnTextctrlOptCgMaxstpEnter( wxCommandEvent& event );

      /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_OPT_ISRCH
      virtual    void OnTextctrlOptIsrchEnter( wxCommandEvent& event );

      /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_OPT_THRESH
      virtual    void OnTextctrlOptThreshEnter( wxCommandEvent& event );

      /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_OPT_REFRESH
      virtual    void OnTextctrlOptRefreshEnter( wxCommandEvent& event );

   protected:
      OptimizeModel *p_model;
      MDPanelHelper *p_helper;
};

#endif
    // _OPTIMIZEPANEL_H_
