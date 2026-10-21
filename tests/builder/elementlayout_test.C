// Element chooser layout (#222): every element has its own cell, the f-block
// sits below with placeholders, search finds symbols and names.
#include <cstdio>
#include <set>
#include <utility>
#include "ElementLayout.H"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

int main()
{
  using namespace ElementLayout;
  std::set<std::pair<int,int> > used;
  for (int z = 1; z <= 118; z++) {
    Cell c = cellOf(z);
    CHECK(c.row >= 0 && c.row < NUM_ROWS && c.col >= 0 && c.col < NUM_COLS);
    CHECK(used.insert(std::make_pair(c.row, c.col)).second);
  }
  for (int w = 0; w < 2; w++) {
    Cell p = placeholderCell(w);
    CHECK(used.insert(std::make_pair(p.row, p.col)).second);
  }
  CHECK(used.size() == 120);
  //  Row 0 holds H and He only; Be/B straddle the gap; La/Hf; Lu is in the f row.
  CHECK(cellOf(1).col == 0 && cellOf(2).col == 17 && cellOf(2).row == 0);
  CHECK(cellOf(4).col == 1 && cellOf(5).col == 12 && cellOf(5).row == 1);
  CHECK(cellOf(26).row == 3 && cellOf(26).col == 7);       // Fe: period 4, group 8
  CHECK(cellOf(72).row == 5 && cellOf(72).col == 3);       // Hf under Zr
  CHECK(cellOf(57).row == F_ROW_LA && cellOf(57).col == 2);
  CHECK(cellOf(71).row == F_ROW_LA && cellOf(71).col == 16);
  CHECK(cellOf(103).row == F_ROW_AC && cellOf(103).col == 16);
  CHECK(cellOf(118).row == 6 && cellOf(118).col == 17);
  CHECK(cellOf(54).col == 17 && cellOf(86).col == 17);

  TPerTab tpt;
  CHECK(search("Fe") == std::vector<int>(1, 26));
  CHECK(search("iron") == std::vector<int>(1, 26));
  CHECK(search("  IRON ") == std::vector<int>(1, 26));
  CHECK(search("c") == std::vector<int>(1, 6));
  CHECK(search("iro") == std::vector<int>(1, 26));
  CHECK(search("xyzzy").empty() && search("").empty());
  CHECK(search("ca").size() == 1 && search("ca")[0] == 20);   // symbol wins
  std::vector<int> m = search("sod");
  CHECK(m.size() == 1 && tpt.atomicSymbol(m[0]) == "Na");
  CHECK(search("tit").size() == 1);

  std::vector<std::string> r;
  for (const char *s : {"C", "O", "C", "N"}) pushRecent(r, s, 3);
  CHECK(r.size() == 3 && r[0] == "N" && r[1] == "C" && r[2] == "O");
  CHECK(categoryOf(26) == TransitionMetal && categoryOf(57) == Lanthanide &&
        categoryOf(17) == Halogen && categoryOf(11) == AlkaliMetal);
  CHECK(darkTextOn(255, 255, 255) && !darkTextOn(0, 0, 0));
  printf(failures ? "FAILED\n" : "OK\n");
  return failures ? 1 : 0;
}
