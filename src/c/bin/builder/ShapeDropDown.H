#ifndef SHAPEDROPDOWN_H
#define SHAPEDROPDOWN_H

#include <wx/toolbar.h>

#include "tdat/ShapeData.H"

#include "wxviz/WxVizTool.H"


class ShapeDropDown : public wxToolBar,
                      public WxVizTool
{    
  DECLARE_DYNAMIC_CLASS( ShapeDropDown )
  DECLARE_EVENT_TABLE()

  public:

    enum {
      ID_SHAPE_BASE = 6000
    };

    ShapeDropDown( );
    ShapeDropDown(ShapeData::Shape shape,
            wxWindow* parent,
            wxWindowID id = wxID_ANY,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            long style = wxNO_BORDER|wxTB_VERTICAL,
            const wxString& name = "Shapes");
    virtual ~ShapeDropDown();

    bool Create(ShapeData::Shape shape,
            wxWindow* parent,
            wxWindowID id = wxID_ANY,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            long style = wxNO_BORDER|wxTB_VERTICAL,
            const wxString& name = "Shapes");

    void select(ShapeData::Shape shape);

  protected:

    virtual void OnToolClick( wxCommandEvent& event );

    void addShape(ShapeData::Shape shape);

    int p_lastId;
};

#endif // SHAPEDROPDOWN_H
