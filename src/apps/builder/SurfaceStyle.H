// Global appearance of isosurfaces (MOs, densities, cubes) and of the unit
// cell, kept in the preferences file so it survives a restart (#881).
#ifndef SURFACESTYLE_H
#define SURFACESTYLE_H

#include <string>

namespace SurfaceStyle {

/// Free lobe colours; false when the four presets are in force.
bool customColors(double pos[3], double neg[3]);
/// The colours a surface is drawn with: custom ones, else the preset.
void colors(double pos[3], double neg[3]);
void setColors(const double pos[3], const double neg[3]);
/// Preset 0..3 = R/G, R/B, B/Y, L/D; clears custom colours.
void setPreset(int index);
int presetIndex();
void clearCustom();

/// Opacity in percent; unset means the old Semi-Transparent/Opaque choice.
bool opacityIsSet();
int opacityPercent(const std::string& legacyKey, bool legacyDefaultSemi);
void setOpacity(int percent);
void clearOpacity();
/// 1 - opacity, or `legacy` when no opacity was set.
double transparency(double legacy);

/// MO/cube surface drawing: 0 solid, 1 mesh, 2 contour.
int surfaceType();
void setSurfaceType(int type);

void cellColor(double rgb[3]);
void setCellColor(const double rgb[3]);
bool cellLabels();
void setCellLabels(bool on);
/// Push the stored cell settings into SGLattice (at start-up).
void applyCell();

/// Forget every setting kept here.
void reset();

}  // namespace SurfaceStyle

#endif
