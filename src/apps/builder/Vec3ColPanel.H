#ifndef VEC3COLPANEL__H
#define VEC3COLPANEL__H


#include "PropertyPanel.H"


class wxGrid;

/**
 * A panel that displays a PropVector data structure as a table
 * The metadata doesn't say what the table shape is but this class
 * assumes its for 3space (ie 3 columns).  Examples include
 * octapole, polarizability...
 * Values are assumed to be doubles
 */
class Vec3ColPanel : public PropertyPanel
                
{
  DECLARE_DYNAMIC_CLASS(Vec3ColPanel)
  DECLARE_EVENT_TABLE()

  public:
    Vec3ColPanel();
    Vec3ColPanel(IPropCalculation *calculation,
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
    virtual ~Vec3ColPanel();

   protected:
     void fillTable();
     string getConfigKey();

   private:
      /* The report oriented table showing Moments. Could be dipole... */
      wxGrid *p_grid;  
};

#endif // VEC3COLPANEL__H

