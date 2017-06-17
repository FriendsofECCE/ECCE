#ifndef KEYVALUEPROPERTYPANEL_H_
#define KEYVALUEPROPERTYPANEL_H_

#include "PropertyPanel.H"

class KeyValuePropertyPanel : public PropertyPanel
{
  DECLARE_DYNAMIC_CLASS(KeyValuePropertyPanel)

  public:
    KeyValuePropertyPanel();
    KeyValuePropertyPanel(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="KeyValuePropertyPanel");
    virtual bool Create(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="KeyValuePropertyPanel");
    virtual ~KeyValuePropertyPanel();

    virtual void refresh();

  protected:
    virtual void initialize();
};

#endif // KEYVALUEPROPERTYPANEL_H_
