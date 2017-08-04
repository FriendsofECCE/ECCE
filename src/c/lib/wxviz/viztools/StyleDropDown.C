#include "util/Event.H"
using namespace ecce;

//#include "wxgui/ewxWindowUtils.H"
//#include "wxgui/WindowEvent.H"

#include "wxviz/ViewerEvtHandler.H"
#include "wxviz/StyleDropDown.H"


IMPLEMENT_DYNAMIC_CLASS( StyleDropDown, wxToolBar )

BEGIN_EVENT_TABLE( StyleDropDown, wxToolBar )
  EVT_UPDATE_UI_RANGE(ViewerEvtHandler::ID_STYLE_RESIDUE_WIRE,
                      ViewerEvtHandler::ID_STYLE_CARTOON,
                      StyleDropDown::UICanUseResidueStyles)
END_EVENT_TABLE()

StyleDropDown::StyleDropDown( )
  : wxToolBar(),
    WxVizTool(),
    Listener("StyleDropDown")
{
  subscribe();
}


StyleDropDown::StyleDropDown(wxWindow* parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
  : wxToolBar(),
    WxVizTool(),
    Listener("StyleDropDown")
{
  subscribe();
  Create(parent, id, pos, size, style, name);
}


bool StyleDropDown::Create(wxWindow* parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
{
  if (!wxToolBar::Create(parent, id, pos, size, style, name)) {
    wxFAIL_MSG( wxT("StyleDropDown creation failed") );
    return false;
  }

  AddTool(ViewerEvtHandler::ID_STYLE_BALL_WIREFRAME,
          "Ball And Wireframe", wxNullBitmap);
  AddTool(ViewerEvtHandler::ID_STYLE_BALL_STICK,
          "Ball And Stick", wxNullBitmap);
  AddTool(ViewerEvtHandler::ID_STYLE_STICK,
          "Stick", wxNullBitmap);
  AddTool(ViewerEvtHandler::ID_STYLE_WIREFRAME,
          "Wireframe", wxNullBitmap);
  AddTool(ViewerEvtHandler::ID_STYLE_CPK,
          "CPK", wxNullBitmap);
  AddTool(ViewerEvtHandler::ID_STYLE_RESIDUE_WIRE,
          "Residue Wire", wxNullBitmap);
  AddTool(ViewerEvtHandler::ID_STYLE_RESIDUE_CYLINDERS,
          "Residue Cylinders", wxNullBitmap);
  AddTool(ViewerEvtHandler::ID_STYLE_LINE_RIBBON,
          "Line Ribbon", wxNullBitmap);
  AddTool(ViewerEvtHandler::ID_STYLE_FLAT_RIBBON,
          "Flat Ribbon", wxNullBitmap);
  AddTool(ViewerEvtHandler::ID_STYLE_SOLID_RIBBON,
          "Solid Ribbon", wxNullBitmap);
  AddTool(ViewerEvtHandler::ID_STYLE_CARTOON,
          "Cartoon", wxNullBitmap);
  AddTool(ViewerEvtHandler::ID_STYLE_CUSTOM,
          "Custom...", wxNullBitmap);

  return true;
}


StyleDropDown::~StyleDropDown()
{
  unsubscribe();
}

/**
 * Override this method so we can immediately update the menu sensitivity.
 */
void StyleDropDown::connectToolKitFW(WxVizToolFW * fw)
{
   WxVizTool::connectToolKitFW(fw);
   eventMCB(Event("GeomChange"));
}


void StyleDropDown::eventMCB(const Event& event)
{
  /*
   string name = event.getName();
   string value = event.getValue();


   if (name == "ResidueChange" || name == "GeomChange") {
      WxVizToolFW& fw = getFW();
      SGContainer& sg = fw.getSceneGraph();
      SGFragment *frag = sg.getFragment();

      bool hasBioResidues = frag->hasBiologicalResidues();
      wxButton *b = 0;

      b = (wxButton*)FindWindow(ViewerEvtHandler::ID_STYLE_RESIDUE_WIRE);
      b->Enable(hasBioResidues);

      b = (wxButton*)FindWindow(ViewerEvtHandler::ID_STYLE_RESIDUE_CYLINDERS);
      b->Enable(hasBioResidues);

      b = (wxButton*)FindWindow(ViewerEvtHandler::ID_STYLE_LINE_RIBBON);
      b->Enable(hasBioResidues);

      b = (wxButton*)FindWindow(ViewerEvtHandler::ID_STYLE_FLAT_RIBBON);
      b->Enable(hasBioResidues);

      b = (wxButton*)FindWindow(ViewerEvtHandler::ID_STYLE_SOLID_RIBBON);
      b->Enable(hasBioResidues);

      b = (wxButton*)FindWindow(ViewerEvtHandler::ID_STYLE_CARTOON);
      b->Enable(hasBioResidues);
   }

   */
}


void StyleDropDown::UICanUseResidueStyles(wxUpdateUIEvent& event)
{
  WxVizToolFW& fw = getFW();
  SGContainer& sg = fw.getSceneGraph();
  SGFragment *frag = sg.getFragment();
  event.Enable(frag->hasBiologicalResidues());
}
