// The Builder's Display panel (#881): every rendering setting in one place.
// Style controls drive the Custom Style dialog's own logic (VizStyleChooser);
// colours, labels and axes go through the Render menu's actions; surface
// colours/opacity and cell settings are global (SurfaceStyle).
#ifndef DISPLAYPANEL_H
#define DISPLAYPANEL_H

#include <functional>
#include <string>
#include <vector>

#include <wx/wx.h>
#include <wx/scrolwin.h>

class VizStyleChooser;

struct DisplayHost {
  std::function<VizStyleChooser*()> chooser;
  std::function<void()> openCustomStyle;
  /// Fire the Render menu action `id`; a check item is set to `on` first.
  std::function<void(int id, bool check, bool on)> menuAction;
  std::function<bool(int id)> checked;
  std::function<void(double rgb[3])> getBackground, getForeground;
  std::function<void(double r, double g, double b)> setBackground, setForeground;
  std::function<void()> pickBackground, pickForeground;
  /// Redraw every open MO/density/cube panel and the unit cell.
  std::function<void()> surfacesChanged;
  std::function<void()> resetAll;
};

class DisplayPanel : public wxScrolledWindow {
public:
  DisplayPanel(wxWindow *parent, const DisplayHost& host);

  /// Re-read every control from the scene and the preferences.
  void refresh();
  /// One scripted control: "style ...", "slider ...", etc. (BuilderDisplayHooks.C).
  bool script(const std::vector<std::string>& w, std::string& err);
  std::string info();

private:
  class Swatch;
  void build();
  void applyStyle();
  void applyAutomatic(bool sphere, bool on);
  void applySlider(const std::string& name, int v);
  void applyLobes(const double pos[3], const double neg[3]);
  void applyPreset(int i);
  void applySurfaceType(int t);
  void applyOpacity(int percent);
  void applyCell(const double rgb[3]);
  void applyCellLabels(bool on);
  void applyToggle(int id, bool on);
  void applyAtomLabel(int index);
  void applyReset();
  void surfaces();

  DisplayHost p_h;
  wxChoice *p_style, *p_scheme, *p_atomLabels;
  wxSlider *p_sl[5];       // sphscl, cylrad, sphrq, cylrq, opacity
  wxStaticText *p_val[5];
  wxCheckBox *p_auto[2];   // sphere, cylinder
  Swatch *p_bg, *p_fg, *p_pos, *p_neg, *p_cell;
  wxRadioButton *p_surf[3];
  wxSlider *p_opacity;
  wxStaticText *p_opacityVal;
  wxCheckBox *p_bond, *p_res, *p_sum, *p_axes, *p_cellLabels;
  bool p_building = true;
};

#endif
