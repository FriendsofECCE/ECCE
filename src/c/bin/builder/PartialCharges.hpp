#ifndef PARTIALCHARGES_H
#define PARTIALCHARGES_H

#include "util/Event.H"
  using ecce::Event;
#include "util/Listener.H"
  using ecce::Listener;

#include "wxgui/TearableContentProvider.H"

#include "VizPropertyPanel.H"

class ewxButton;
class ewxChoice;
class ewxStaticText;
class WxColorSchemeButton;

/**
 * Panel for ESP calculation results.
 */
class PartialCharges : public VizPropertyPanel,
                        public TearableContentProvider,
                        public Listener
{
  DECLARE_DYNAMIC_CLASS(PartialCharges)
  DECLARE_EVENT_TABLE()

  public:
    PartialCharges();
    PartialCharges(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="PartialCharges");
    virtual bool Create(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="PartialCharges");
    virtual ~PartialCharges();

   protected:
     void fillUI();
     void setSpectrumRange(double min, double max);
     void OnGridRangeSelect(wxGridRangeSelectEvent& event);
     void setColorTable();
     void saveColorTable();
     void restoreColorTable();
     void fillSpectrumColors();
     void computeColors();
     void determineMinMax(int col, double& min, double& max);
     void clean();
     void addSpectrumOptions(vector<string>& labels);

     // Listener virtuals
     virtual void eventMCB(const Event& event);

     void OnColorAtomClick(wxCommandEvent& event);
     void OnLogScaleClick(wxCommandEvent& event);
     void OnAssign(wxCommandEvent& event);
     void OnSpectrumChoice(wxCommandEvent& event);

     string getConfigKey();


     virtual void receiveFocus();
     virtual void loseFocus();

     virtual wxWindow* GetTearableContent();

     /* widget to show atoms charges with shell charges as children */
     wxGrid *p_grid;

     ewxChoice *p_espType;
     ewxButton *p_assign;

     /* Simple color spectrum display widget */
     WxColorSchemeButton *p_spectrum;

     /* label for minimum value */
     ewxStaticText *p_minLabel;

     /* label for maximum value */
     ewxStaticText *p_maxLabel;

     /* flag to avoid recursion between gui and 3d selections */
     bool p_internalSelect;

     /* -1 = all neg; 0 = mix; 1 = all positive */
     int  p_spectrumRange; 

     int p_currentColumn;

     int p_colorTglId;
     int p_LogScaleTglId;

   private:
     // For restoring color table
     float *p_defRed;
     float *p_defGreen;
     float *p_defBlue;

     // our colors
     float *p_red;
     float *p_green;
     float *p_blue;




};

#endif // PARTIALCHARGES_H

