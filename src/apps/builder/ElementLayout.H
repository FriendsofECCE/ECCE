// Geometry, categories and search of the Builder's element chooser.  No wx,
// so tests/builder/elementlayout_test.C can check it without a display.
#ifndef ELEMENTLAYOUT_H
#define ELEMENTLAYOUT_H

#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>
#include <vector>

#include "tdat/TPerTab.H"

namespace ElementLayout {

enum Category { AlkaliMetal, AlkalineEarth, TransitionMetal, PostTransition,
                Metalloid, Nonmetal, Halogen, NobleGas, Lanthanide, Actinide,
                NumCategories };

struct Cell { int row, col; };

// Main table rows 0-6 and 18 columns; the f-block sits in rows 8 and 9 under
// columns 2-16, and group 3 of rows 5 and 6 holds the "57-71"/"89-103" tiles.
const int NUM_ROWS = 10;
const int NUM_COLS = 18;
const int F_ROW_LA = 8, F_ROW_AC = 9;
const int PLACEHOLDER_LA = 0, PLACEHOLDER_AC = 1;

inline Cell cellOf(int z)
{
  if (z == 1) return {0, 0};
  if (z == 2) return {0, 17};
  if (z >= 3 && z <= 4) return {1, z - 3};
  if (z >= 5 && z <= 10) return {1, z - 5 + 12};
  if (z >= 11 && z <= 12) return {2, z - 11};
  if (z >= 13 && z <= 18) return {2, z - 13 + 12};
  if (z >= 19 && z <= 36) return {3, z - 19};
  if (z >= 37 && z <= 54) return {4, z - 37};
  if (z >= 55 && z <= 56) return {5, z - 55};
  if (z >= 57 && z <= 71) return {F_ROW_LA, z - 57 + 2};
  if (z >= 72 && z <= 86) return {5, z - 72 + 3};
  if (z >= 87 && z <= 88) return {6, z - 87};
  if (z >= 89 && z <= 103) return {F_ROW_AC, z - 89 + 2};
  if (z >= 104 && z <= 118) return {6, z - 104 + 3};
  return {-1, -1};
}

inline Cell placeholderCell(int which) { return {which ? 6 : 5, 2}; }

inline Category categoryOf(int z)
{
  if (z == 1 || z == 6 || z == 7 || z == 8 || z == 15 || z == 16 || z == 34)
    return Nonmetal;
  if (z == 9 || z == 17 || z == 35 || z == 53 || z == 85) return Halogen;
  if (z == 2 || z == 10 || z == 18 || z == 36 || z == 54 || z == 86 ||
      z == 118) return NobleGas;
  if (z == 3 || z == 11 || z == 19 || z == 37 || z == 55 || z == 87)
    return AlkaliMetal;
  if (z == 4 || z == 12 || z == 20 || z == 38 || z == 56 || z == 88)
    return AlkalineEarth;
  if (z == 5 || z == 14 || z == 32 || z == 33 || z == 51 || z == 52)
    return Metalloid;
  if (z >= 57 && z <= 71) return Lanthanide;
  if (z >= 89 && z <= 103) return Actinide;
  if (z == 13 || z == 31 || z == 49 || z == 50 || z == 81 || z == 82 ||
      z == 83 || z == 84 || z == 113 || z == 114 || z == 115 || z == 116)
    return PostTransition;
  if (z == 117) return Halogen;
  return TransitionMetal;
}

inline const char *categoryName(Category c)
{
  static const char *n[] = {"Alkali metal", "Alkaline earth",
    "Transition metal", "Post-transition metal", "Metalloid", "Nonmetal",
    "Halogen", "Noble gas", "Lanthanide", "Actinide"};
  return n[c];
}

// Thin border colours, Okabe-Ito based so they stay apart for colour-blind
// users; the tile fill is the atom colour, so the border carries the class.
inline unsigned categoryRGB(Category c)
{
  static const unsigned rgb[] = {0xD55E00, 0xE69F00, 0x0072B2, 0x009E73,
                                 0x8C564B, 0x56B4E9, 0xCC79A7, 0x7F7F7F,
                                 0xF0E442, 0x9467BD};
  return rgb[c];
}

// Black or white, whichever has the higher WCAG contrast against the fill.
inline bool darkTextOn(int r, int g, int b)
{
  auto lin = [](int v) {
    double s = v / 255.0;
    return s <= 0.03928 ? s / 12.92 : std::pow((s + 0.055) / 1.055, 2.4);
  };
  const double L = 0.2126 * lin(r) + 0.7152 * lin(g) + 0.0722 * lin(b);
  return (L + 0.05) / 0.05 >= 1.05 / (L + 0.05);
}

inline const std::vector<std::string>& commonElements()
{
  static const std::vector<std::string> c = {"H", "C", "N", "O", "F", "P",
    "S", "Cl", "Br", "I", "Na", "K", "Mg", "Ca", "Fe", "Cu", "Zn"};
  return c;
}

inline std::string lower(std::string s)
{
  for (char &ch : s) ch = (char)std::tolower((unsigned char)ch);
  return s;
}

// Atomic numbers matching text, best first.  An exact symbol or name is the
// only result, so "c" picks carbon although "calcium" starts with it too.
inline std::vector<int> search(const std::string& text)
{
  std::vector<int> hits;
  std::string t = lower(text);
  t.erase(0, t.find_first_not_of(" \t"));
  t.erase(t.find_last_not_of(" \t") + 1);
  if (t.empty()) return hits;
  TPerTab tpt;
  std::vector<int> prefix;
  for (int z = 1; z <= 118; z++) {
    const std::string sym = lower(tpt.atomicSymbol(z));
    const std::string name = lower(tpt.name(z));
    if (sym == t || name == t) return std::vector<int>(1, z);
    if (name.compare(0, t.size(), t) == 0) prefix.push_back(z);
  }
  return prefix;
}

// Most recent first, no duplicates, at most max entries.
inline void pushRecent(std::vector<std::string>& list, const std::string& sym,
                       size_t max = 8)
{
  list.erase(std::remove(list.begin(), list.end(), sym), list.end());
  list.insert(list.begin(), sym);
  if (list.size() > max) list.resize(max);
}

}  // namespace ElementLayout
#endif
