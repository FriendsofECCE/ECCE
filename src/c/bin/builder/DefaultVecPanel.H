#ifndef DEFAULTVECPANEL__H
#define DEFAULTVECPANEL__H


#include "PropertyPanel.H"


class wxGrid;

/**
 * A panel that displays a PropVector data structure as a two
 * column table where column 1 is the row label and column 2 is
 * the value.
 * 
 */
class DefaultVecPanel : public PropertyPanel
                
{
  DECLARE_DYNAMIC_CLASS(DefaultVecPanel)
  DECLARE_EVENT_TABLE()

  public:
    DefaultVecPanel();
    DefaultVecPanel(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="Table");
    virtual bool Create(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="Table");
    virtual ~DefaultVecPanel();


   protected:
     virtual void fillTable();


     /* The report oriented table showing Moments. Could be dipole... */
     wxGrid *p_grid;  



};

#endif // DEFAULTVECPANEL__H

