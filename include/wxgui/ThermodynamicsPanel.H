#ifndef _THERMODYNAMICS_H_
#define _THERMODYNAMICS_H_

/*!
 * Includes
 */

#include "wxgui/ThermodynamicsPanelGUI.H"

class MDPanelHelper;
class ThermodynamicsModel;

/**
 * GUI implementation of ThermodynamicsPanel in MD editors
 */
class ThermodynamicsPanel: public ThermodynamicsPanelGUI
{    

   public:
      /// Constructors
      ThermodynamicsPanel(MDPanelHelper *helper );
      ThermodynamicsPanel( MDPanelHelper *helper, wxWindow* parent,
            wxWindowID id = SYMBOL_THERMODYNAMICSPANELGUI_IDNAME,
            const wxPoint& pos = SYMBOL_THERMODYNAMICSPANELGUI_POSITION,
            const wxSize& size = SYMBOL_THERMODYNAMICSPANELGUI_SIZE,
            long style = SYMBOL_THERMODYNAMICSPANELGUI_STYLE );

      virtual ~ThermodynamicsPanel( );

      void setThermodynamicsModel(ThermodynamicsModel *model);
      ThermodynamicsModel* getThermodynamicsModel();
      void initializeGUI();
      void refreshGUI();
      void configOptimizeCalc();

      void enablePmfPanel(bool enable=true);
      void enableIntegrationPanel(bool enable=true);

      void OnCheckboxThermPmfClick( wxCommandEvent& event );
      void OnRadiobuttonThermPmfStandardSelected( wxCommandEvent& event );
      void OnRadiobuttonThermPmfScaledSelected( wxCommandEvent& event );
      void OnTextctrlThermPmfScaledEnter( wxCommandEvent& event );
      void OnRadiobuttonThermPmfHarmonicSelected( wxCommandEvent& event );
      void OnTextctrlThermPmfHarmonicEnter( wxCommandEvent& event );
      void OnCheckboxThermIntegClick( wxCommandEvent& event );
      void OnRadiobuttonThermIntegNewSelected( wxCommandEvent& event );
      void OnRadiobuttonThermIntegRenewSelected( wxCommandEvent& event );
      void OnRadiobuttonThermIntegExtendSelected( wxCommandEvent& event );
      void OnChoiceThermIntegDirectionSelected( wxCommandEvent& event );
      void OnSpinctrlThermIntegEnsemblesNumUpdated( wxSpinEvent& event );
      void OnSpinctrlThermIntegEnsemblesMaxUpdated( wxSpinEvent& event );
      void OnTextctrlThermIntegErrorEnter( wxCommandEvent& event );
      void OnTextctrlThermIntegDriftEnter( wxCommandEvent& event );

   protected:
      ThermodynamicsModel *p_model;
      MDPanelHelper       *p_helper;
};

#endif // _THERMODYNAMICS_H_

