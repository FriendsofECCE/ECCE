#ifndef _LAYERBUILDER_H_
#define _LAYERBUILDER_H_

#include "wx/panel.h"
#include "wxviz/WxVizTool.H"

/**
 * Tools > Layer Builder: stacks the current structure and its cell as one
 * layer, N times, with ions and water in the gaps (PBCLayerCmd, #357).
 */
class LayerBuilder : public wxPanel, public WxVizTool
{
public:
  LayerBuilder(wxWindow *parent, wxWindowID id = wxID_ANY);

  static const wxWindowID ID_SPIN_LAYER_N;
  static const wxWindowID ID_TEXT_LAYER_SPACING;
  static const wxWindowID ID_TEXT_LAYER_SHIFTA;
  static const wxWindowID ID_TEXT_LAYER_SHIFTB;
  static const wxWindowID ID_TEXT_LAYER_ION1SYM;
  static const wxWindowID ID_SPIN_LAYER_ION1N;
  static const wxWindowID ID_TEXT_LAYER_ION1Q;
  static const wxWindowID ID_TEXT_LAYER_ION1MIN;
  static const wxWindowID ID_TEXT_LAYER_ION2SYM;
  static const wxWindowID ID_SPIN_LAYER_ION2N;
  static const wxWindowID ID_TEXT_LAYER_ION2Q;
  static const wxWindowID ID_TEXT_LAYER_ION2MIN;
  static const wxWindowID ID_SPIN_LAYER_WATER;
  static const wxWindowID ID_TEXT_LAYER_WATERMIN;
  static const wxWindowID ID_SPIN_LAYER_SEED;
  static const wxWindowID ID_BUTTON_LAYER_BUILD;

private:
  void OnButtonLayerBuild(wxCommandEvent& event);
};

#endif
