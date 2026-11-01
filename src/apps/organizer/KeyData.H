/**
 * @file
 *
 * The key-data box at the end of a recipe workflow (#710): the energies of
 * the steps, the reaction energy and barrier of a join, and DeltaG where the
 * recipe asks for it.  Computed from the parsed results; the panel only
 * draws what this returns.  No wx.
 */

#ifndef KEYDATA_HH
#define KEYDATA_HH

#include <string>
#include <utility>
#include <vector>

#include "tdat/EnergySums.H"

struct KeyStep {
  std::string name;
  std::string keyRole;       ///< reactant, product, ts, min, final or ""
  bool thermo = false;       ///< the recipe asks for G of this step
  bool complete = false;
  EnergyTerm term;           ///< its energy, thermochemistry and level
};

struct KeyDataResult {
  /// False when the completed steps differ in code or level of theory:
  /// energies of different methods are never compared.
  bool shown = true;
  std::string level;                    ///< "G16  B3LYP/6-31+G(d)"
  std::string note;                     ///< progress, or why nothing is shown
  std::vector<std::pair<std::string, std::string> > rows;   ///< label, value
  std::string text() const;             ///< the box as plain lines
};

KeyDataResult keyData(const std::vector<KeyStep>& steps);

#endif
