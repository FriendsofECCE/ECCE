#ifndef MULLIKENPANEL_H
#define MULLIKENPANEL_H

#include "util/Event.H"
  using ecce::Event;
#include "util/Listener.H"
  using ecce::Listener;

#include "wxgui/TearableContentProvider.H"

#include "VizPropertyPanel.H"

class ewxButton;
class ewxStaticText;
class WxColorSchemeButton;
class wxTreeListCtrl;

/**
 * UI Panel to display Mulliken and Mulliken Shell Charges.
 * The UI shows the values of shell charges but the visualization
 * shows only the total atomic charges.
 */
class MullikenPanel : public VizPropertyPanel,
                      public Listener
{
  DECLARE_DYNAMIC_CLASS(MullikenPanel)
  DECLARE_EVENT_TABLE()

  static const wxWindowID IDTREELIST;

  public:
    MullikenPanel();
    MullikenPanel(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="MullikenVizPropertyPanel");
    virtual bool Create(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="MullikenVizPropertyPanel");
    virtual ~MullikenPanel();

   protected:
     // Listener virtuals
     virtual void eventMCB(const Event& event);

     void fillUI();
     void setSpectrumRange(double min, double max);
     void setColorTable();
     void saveColorTable();
     void restoreColorTable();
     void computeColors(const PropVector *vec,double min, double max);
     void clean();

     void OnAssign(wxCommandEvent& event);

     virtual void receiveFocus();
     virtual void loseFocus();

     /* widget to show atoms charges with shell charges as children */
     wxTreeListCtrl *p_treelist;

     /* Simple color spectrum display widget */
     WxColorSchemeButton *p_spectrum;

     /* label for minimum value */
     ewxStaticText *p_minLabel;

     /* label for maximum value */
     ewxStaticText *p_maxLabel;

     ewxButton *p_assign;

     void OnTreeSelChanged( wxTreeEvent& event );

   private:
     // For restoring color table
     float *p_defRed;
     float *p_defGreen;
     float *p_defBlue;

     // our colors
     float *p_red;
     float *p_green;
     float *p_blue;

     bool p_internalSelect;





};

#endif // MULLIKENPANEL_H

