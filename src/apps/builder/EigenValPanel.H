#ifndef EIGENVALPANEL__H
#define EIGENVALPANEL__H

#include "util/Event.H"
  using ecce::Event;
#include "util/Listener.H"
  using ecce::Listener;

#include "wxviz/WxVizTool.H"

#include "DefaultVecPanel.H"


class wxGrid;

/**
 * A panel that displays eigenvalue properties.
 * PropVector<Component>
 * Shown are columns:
 *   index, atom, c1, c2, c3
 * 
 */
class EigenValPanel : public DefaultVecPanel,
                      public WxVizTool,
                      public Listener
                
{
  DECLARE_DYNAMIC_CLASS(EigenValPanel)
  DECLARE_EVENT_TABLE()

  public:
    EigenValPanel();
    EigenValPanel(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="Eigenvalue");
    virtual bool Create(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="Eigenvalue");
    virtual ~EigenValPanel();


   protected:
     virtual void fillTable();
     void OnGridRangeSelect(wxGridRangeSelectEvent& event);

     // Listener virtuals
     virtual void eventMCB(const Event& event);


     bool p_internalSelect;





};

#endif // EIGENVALPANEL__H

