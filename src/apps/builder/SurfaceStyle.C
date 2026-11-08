#include "SurfaceStyle.H"

#include <cstdio>
#include <cstdlib>

#include "wxgui/ewxConfig.H"
#include "viz/SGLattice.H"
#include <wx/wx.h>
#include "tdat/PropVector.H"
#include "tdat/PropVecString.H"
#include "MoPanel.H"

namespace {

const char *INIFILE = "wxbuilder.ini";
const char *K_POS = "Display/PosColor", *K_NEG = "Display/NegColor";
const char *K_OPACITY = "Display/Opacity", *K_CELL = "Display/CellColor";
const char *K_CELLLABELS = "Display/CellLabels";

const double PRESETS[4][6] = {
  {1.0, 0.0, 0.0,   0.0, 1.0, 0.0},
  {1.0, 0.0, 0.0,   0.0, 0.2, 1.0},
  {0.0, 0.2, 1.0,   1.0, 1.0, 0.0},
  {0.95, 0.95, 0.95, 0.2, 0.2, 0.2}};

ewxConfig *cfg() { return ewxConfig::getConfig(INIFILE); }

bool readRGB(const char *key, double rgb[3])
{
  wxString s = cfg()->Read(key, "");
  return !s.empty() &&
         sscanf(s.mb_str(), "%lf,%lf,%lf", rgb, rgb + 1, rgb + 2) == 3;
}

void writeRGB(const char *key, const double rgb[3])
{
  cfg()->Write(key, wxString::Format("%.4f,%.4f,%.4f", rgb[0], rgb[1], rgb[2]));
  cfg()->Flush();
}

}  // namespace

namespace SurfaceStyle {

bool customColors(double pos[3], double neg[3])
{
  return readRGB(K_POS, pos) && readRGB(K_NEG, neg);
}

void colors(double pos[3], double neg[3])
{
  if (customColors(pos, neg)) return;
  const double *p = PRESETS[presetIndex()];
  for (int i = 0; i < 3; i++) { pos[i] = p[i]; neg[i] = p[3 + i]; }
}

void setColors(const double pos[3], const double neg[3])
{
  writeRGB(K_POS, pos);
  writeRGB(K_NEG, neg);
}

int presetIndex()
{
  int id;
  cfg()->Read("MO/Colors", &id, MoPanel::presetId(0));
  int i = id - MoPanel::presetId(0);
  return i < 0 || i > 3 ? 0 : i;
}

void clearCustom()
{
  cfg()->DeleteEntry(K_POS);
  cfg()->DeleteEntry(K_NEG);
  cfg()->Flush();
}

void setPreset(int index)
{
  static const char *cubeNames[4] = {"Red and Green", "Red and Blue",
                                     "Blue and Yellow", "Light and Dark"};
  if (index < 0 || index > 3) return;
  cfg()->Write("MO/Colors", MoPanel::presetId(index));
  cfg()->Write("/CUBE/Colors", cubeNames[index]);
  clearCustom();
}

bool opacityIsSet() { return cfg()->HasEntry(K_OPACITY); }

int opacityPercent(const std::string& legacyKey, bool legacyDefaultSemi)
{
  if (opacityIsSet()) return (int)cfg()->Read(K_OPACITY, 50L);
  bool semi = legacyDefaultSemi;
  wxString v = cfg()->Read(legacyKey, "");
  if (!v.empty()) semi = (v == "Semi Transparent") ||
                         (v == "9789" /* MoPanel SEMITRANSPARENT */);
  return semi ? 50 : 100;
}

void setOpacity(int percent)
{
  cfg()->Write(K_OPACITY, (long)(percent < 0 ? 0 : percent > 100 ? 100 : percent));
  cfg()->Flush();
}

void clearOpacity()
{
  cfg()->DeleteEntry(K_OPACITY);
  cfg()->Flush();
}

double transparency(double legacy)
{
  if (!opacityIsSet()) return legacy;
  return 1.0 - cfg()->Read(K_OPACITY, 50L) / 100.0;
}

int surfaceType()
{
  int id;
  cfg()->Read("MO/Surface", &id, MoPanel::surfaceId(0));
  int i = id - MoPanel::surfaceId(0);
  return i < 0 || i > 2 ? 0 : i;
}

void setSurfaceType(int type)
{
  static const char *cubeNames[3] = {"Solid", "Mesh", "Contour"};
  if (type < 0 || type > 2) return;
  cfg()->Write("MO/Surface", MoPanel::surfaceId(type));
  cfg()->Write("/CUBE/Surface", cubeNames[type]);
  cfg()->Flush();
}

void cellColor(double rgb[3])
{
  if (!readRGB(K_CELL, rgb)) rgb[0] = rgb[1] = rgb[2] = 0.5;
}

void setCellColor(const double rgb[3])
{
  writeRGB(K_CELL, rgb);
  SGLattice::setEdgeColor((float)rgb[0], (float)rgb[1], (float)rgb[2]);
}

bool cellLabels()
{
  bool on;
  cfg()->Read(K_CELLLABELS, &on, true);
  return on;
}

void setCellLabels(bool on)
{
  cfg()->Write(K_CELLLABELS, on);
  cfg()->Flush();
  SGLattice::setLabelsShown(on);
}

void applyCell()
{
  double c[3];
  cellColor(c);
  SGLattice::setEdgeColor((float)c[0], (float)c[1], (float)c[2]);
  SGLattice::setLabelsShown(cellLabels());
}

void reset()
{
  clearCustom();
  clearOpacity();
  setPreset(0);
  setSurfaceType(0);
  cfg()->DeleteEntry(K_CELL);
  cfg()->DeleteEntry(K_CELLLABELS);
  cfg()->Flush();
  applyCell();
}

}  // namespace SurfaceStyle
