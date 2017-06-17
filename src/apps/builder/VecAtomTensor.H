#ifndef VECATOMTENSOR__H
#define VECATOMTENSOR__H

#include "util/Event.H"
  using ecce::Event;
#include "util/Listener.H"
  using ecce::Listener;


#include "wxgui/TearableContentProvider.H"

#include "VizPropertyPanel.H"


class ewxChoice;
class SliderCombo;
class wxGrid;
class TensorTable;


/**
 * Panel for properties such as SHIELDTENSOR DIPOLETENSOR EFIELDGRAD
 * The design diverges from the old calcviewer in that we only show a list
 * of atoms instead of a table of data for each atom.
 * User must ask for a dialog to see the actual data.
 */
class VecAtomTensor : public VizPropertyPanel,
                      public TearableContentProvider,
                      public Listener
{
   DECLARE_DYNAMIC_CLASS(VecAtomTensor)
   DECLARE_EVENT_TABLE()

   public:

      static const wxWindowID ID_DIALOG_VECATOMTENSOR;
      static const wxWindowID ID_VECATOMTENSOR_SLIDER;
      static const wxWindowID ID_BUTTON_VECATOMTENSOR_POSCOLOR;
      static const wxWindowID ID_BUTTON_VECATOMTENSOR_NEGCOLOR;
      static const wxWindowID ID_VECATOMTENSOR_GRID;
      static const wxWindowID ID_VECATOMTENSOR_CHOICE;
      static const wxWindowID ID_VECATOMTENSOR_SHOWDATA;

      VecAtomTensor();
      VecAtomTensor(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="Tensor");
      virtual bool Create(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="Tensor");
      virtual ~VecAtomTensor();

      virtual void receiveFocus();
      virtual void loseFocus();

   protected:
      virtual void fillTable();
      virtual void showVectors(bool guessScale);
      virtual string getConfigKey();

      virtual void OnPosColorClick( wxCommandEvent& event);
      virtual void OnNegColorClick( wxCommandEvent& event);
      virtual void OnEndSliderMotion(wxScrollEvent& event);
      virtual void OnSliderTextEnter(wxCommandEvent& event);
      virtual void OnRowSelection(wxGridEvent& event);
      virtual void OnGridRangeSelect(wxGridRangeSelectEvent& event);
      virtual void OnChoiceSelect(wxCommandEvent& event);
      virtual void OnMenuClick( wxCommandEvent& event );

      // Listener virtuals
      virtual void eventMCB(const Event& event);

      virtual wxWindow* GetTearableContent();

      wxGrid *p_grid;  

      SliderCombo *p_slider;

      /* The first time, we want to let the command guess the scale
         but after that, stick with the user's choice. */
      bool p_guessScale;

      set<int> p_selectedRows;

      ewxChoice *p_selectOptions;

      bool p_internalSelect;

      vector<TensorTable*> p_tables;

};

#endif // VECATOMTENSOR__H
